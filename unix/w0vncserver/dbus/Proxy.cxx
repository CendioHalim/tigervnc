/* Copyright 2026 Adam Halim for Cendio AB
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <stdexcept>

#include <glib.h>
#include <gio/gio.h>

#include <core/LogWriter.h>
#include <core/i18n.h>
#include <core/string.h>

#include "Proxy.h"

static core::LogWriter vlog("DBusProxy");

using namespace dbus;

Proxy::Proxy(const char* name, const char* objectPath,
             const char* interfaceName)
  : proxy(nullptr)
{
  GError* error = nullptr;

  connection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);

  if (error) {
    std::string error_message =
      core::format(_("Failed to connect to D-Bus: %s"), error->message);
    g_error_free(error);
    throw std::runtime_error(error_message);
  }

  proxy = g_dbus_proxy_new_sync(connection, G_DBUS_PROXY_FLAGS_NONE,
                                nullptr, name, objectPath,
                                interfaceName, nullptr, &error);

  if (error) {
    std::string error_message =
      core::format(_("Failed to connect to D-Bus interface \"%s\" at "
                     "\"%s\" from \"%s\": %s"),
                   interfaceName, objectPath, name, error->message);
    g_error_free(error);
    throw std::runtime_error(error_message);
  }
}

Proxy::~Proxy() {
  if (connection) {
    for (uint32_t signalId : subscribedSignals)
      g_dbus_connection_signal_unsubscribe(connection, signalId);

    g_object_unref(connection);
  }

  if (proxy)
    g_object_unref(proxy);
}

GVariant* Proxy::call(const char* method, GVariant* parameters,
                      GDBusCallFlags flags)
{
  GVariant* result;
  GError* error = nullptr;

  result = g_dbus_proxy_call_sync(proxy, method, parameters,
                                  flags, 3000, nullptr, &error);

  if (error) {
    std::string error_message =
      core::format(_("Failed to call D-Bus method \"%s\": %s"),
                   method, error->message);
    g_error_free(error);
    throw std::runtime_error(error_message);
  }

  return result;
}

GVariant* Proxy::callWithUnixFdList(const char* method, GVariant* parameters,
                                    GUnixFDList* fdList, GUnixFDList** outFdList,
                                    GDBusCallFlags flags)
{
  GError* error = nullptr;
  GVariant* result;

  result = g_dbus_proxy_call_with_unix_fd_list_sync(proxy, method,
                                                    parameters, flags,
                                                    3000, fdList,
                                                    outFdList, nullptr,
                                                    &error);

  if (error) {
    std::string msg(error->message);
    g_error_free(error);
    throw std::runtime_error(msg);
  }

  return result;
}

GVariant* Proxy::getProperty(const char* property)
{
  return g_dbus_proxy_get_cached_property(proxy, property);
}

void Proxy::subscribe(const char* member,
                      std::function<void(GVariant* parameters)> signalCallback)
{
  struct Callback {
    std::function<void(GVariant* parameters)> callback;
  };

  Callback* callback;
  uint32_t signalId;

  callback = new Callback;
  callback->callback = signalCallback;

  signalId = g_dbus_connection_signal_subscribe(
               connection,
               g_dbus_proxy_get_name(proxy),
               g_dbus_proxy_get_interface_name(proxy),
               member,
               g_dbus_proxy_get_object_path(proxy),
               nullptr,
               G_DBUS_SIGNAL_FLAGS_NONE,
               [](GDBusConnection*, const char*, const char*,
                  const char*, const char*,
                  GVariant* parameters, void* userData) {
                    ((Callback*)userData)->callback(parameters);
               },
               callback, [](void* userData) {
                 delete (Callback*)userData;
               });

  subscribedSignals.push_back(signalId);
}

void Proxy::unsubscribe(uint32_t signalId)
{
  g_dbus_connection_signal_unsubscribe(connection, signalId);
  subscribedSignals.remove(signalId);
}

bool Proxy::interfacesAvailable(const std::vector<std::string> interfaces)
{
  GError* error = nullptr;
  GDBusConnection* conn;
  GVariant* result;
  GDBusNodeInfo* nodeInfo;
  GDBusInterfaceInfo* interfaceInfo;
  const char* introspectionXml;
  bool interfaceMissing;

  conn = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);

  if (error) {
    std::string error_message =
      core::format(_("Failed to connect to D-Bus: %s"), error->message);
    g_error_free(error);
    throw std::runtime_error(error_message);
  }

  result = g_dbus_connection_call_sync(conn,
                              g_dbus_proxy_get_name(proxy),
                              g_dbus_proxy_get_object_path(proxy),
                              "org.freedesktop.DBus.Introspectable",
                              "Introspect",
                              nullptr,
                              G_VARIANT_TYPE("(s)"),
                              G_DBUS_CALL_FLAGS_NONE,
                              3000,
                              nullptr,
                              &error);

  if (error) {
    std::string error_message(error->message);
    g_error_free(error);
    g_object_unref(conn);
    throw std::runtime_error("Introspect call failed: " + error_message);
  }

  g_variant_get(result, "(&s)", &introspectionXml);

  nodeInfo = g_dbus_node_info_new_for_xml(introspectionXml, &error);
  g_variant_unref(result);

  if (error) {
    std::string error_message = core::format(
      _("Failed to enumerate DBus interfaces: %s"), error->message);
    g_error_free(error);
    throw std::runtime_error(error_message);
  }

  interfaceMissing = false;
  for (const std::string& interface_ : interfaces) {
    interfaceInfo = g_dbus_node_info_lookup_interface(nodeInfo, interface_.c_str());
    if (!interfaceInfo) {
      vlog.debug("Interface '%s' not found", interface_.c_str());
      interfaceMissing = true;
    }
  }

  g_dbus_node_info_unref(nodeInfo);
  g_object_unref(conn);

  return !interfaceMissing;
}
