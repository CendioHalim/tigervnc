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

#include <core/i18n.h>
#include <core/LogWriter.h>

#include "Proxy.h"
#include "RemoteDesktopSession.h"

static core::LogWriter vlog("DBusRemoteDesktop");

using namespace dbus;

RemoteDesktopSession::RemoteDesktopSession(const std::string& path)
  : objectPath(path)
{
  proxy = new Proxy("org.freedesktop.RemoteDesktop1",
                    objectPath.c_str(),
                    "org.freedesktop.RemoteDesktop1.Session");
}

RemoteDesktopSession::~RemoteDesktopSession()
{
  destroy(true);
  // FIXME: Wait for org.freedesktop.RemoteDesktop1.Session::Removed
  delete proxy;
}

void RemoteDesktopSession::destroy(bool removePersistent)
{
  GVariant* result;
  GVariantBuilder b;

  g_variant_builder_init(&b, G_VARIANT_TYPE("a{sv}"));
  g_variant_builder_add(&b, "{sv}", "remove",
                        g_variant_new_boolean(removePersistent ? TRUE :
                                                                 FALSE));

  result = proxy->call("Destroy", g_variant_new("(a{sv})", &b));
  g_variant_unref(result);
}

int RemoteDesktopSession::connectToEIS()
{
  GVariant* result;
  GUnixFDList* fd_list;
  int handle;
  int fd;
  GVariantBuilder b;
  GError* error = nullptr;

  g_variant_builder_init(&b, G_VARIANT_TYPE("a{sv}"));

  result = proxy->callWithUnixFdList("ConnectToEIS",
                                     g_variant_new("(a{sv})", &b), nullptr, &fd_list);

  g_variant_get(result, "(h)", &handle);
  fd = g_unix_fd_list_get(fd_list, handle, &error);
  g_object_unref(fd_list);

  if (error) {
    vlog.error(_("Error connecting to EIS: %s"), error->message);
    g_error_free(error);
    return 0;
  }

  g_variant_unref(result);

  return fd;
}

std::string RemoteDesktopSession::createVirtualMonitor(const std::string& name,
                                                       bool persistent)
{
  GVariant* result;
  char* path;
  std::string monitorPath;
  GVariantBuilder b;

  vlog.debug("Creating virtual monitor %s", name.c_str());

  g_variant_builder_init(&b, G_VARIANT_TYPE("a{sv}"));
  g_variant_builder_add(&b, "{sv}", "name", g_variant_new_string(name.c_str()));

  // FIXME: Keep track if session is persistent and check it here.
  if (persistent)
    g_variant_builder_add(&b, "{sv}", "persistent", g_variant_new_boolean(TRUE));

  result = proxy->call("CreateVirtualMonitor", g_variant_new("(a{sv})", &b));

  g_variant_get(result, "(o)", &path);
  g_variant_unref(result);

  monitorPath = path;
  g_free(path);

  return monitorPath;
}

std::string RemoteDesktopSession::enableClipboard()
{
  GVariant* result;
  char* path;
  std::string clipboardPath;

  result = proxy->call("EnableClipboard", nullptr);

  g_variant_get(result, "(o)", &path);
  g_variant_unref(result);

  clipboardPath = path;
  g_free(path);

  return clipboardPath;
}

uint32_t RemoteDesktopSession::getVersion()
{
  GVariant* result;
  uint32_t state;

  result = proxy->getProperty("Version");

  if (!result)
    return 0;

  state = g_variant_get_uint32(result);
  g_variant_unref(result);

  return state;
}
