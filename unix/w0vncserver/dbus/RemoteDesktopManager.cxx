#include <core/LogWriter.h>

#include "dbusConstants.h"
#include "Proxy.h"
#include "RemoteDesktopManager.h"

using namespace dbus;

static core::LogWriter vlog("DBusRemoteDesktopManager");


RemoteDesktopManager::RemoteDesktopManager()
{
  remoteDesktop = new Proxy("org.freedesktop.RemoteDesktop1",
                            "/org/freedesktop/RemoteDesktop1",
                            "org.freedesktop.RemoteDesktop1");
}

RemoteDesktopManager::~RemoteDesktopManager()
{
  delete remoteDesktop;
}

std::string RemoteDesktopManager::createSession(bool persistent,
                                                const std::string& persistenceToken,
                                                bool allowTakeOver)
{
  GVariantBuilder options;
  GVariantBuilder sessionOptions;
  GVariant* result;
  char* path;
  std::string sessionPath;

  g_variant_builder_init(&options, G_VARIANT_TYPE("a{sv}"));

  g_variant_builder_add(&options, "{sv}", "access-mode",
                        g_variant_new_uint32(ACCESS_MODE_HEADLESS));

  g_variant_builder_add(&options, "{sv}", "persistent",
                        g_variant_new_boolean(persistent ? TRUE : FALSE));

  if (persistent && !persistenceToken.empty())
    g_variant_builder_add(&options, "{sv}", "persistence-token",
                          g_variant_new_string(persistenceToken.c_str()));

  if (allowTakeOver)
    g_variant_builder_add(&options, "{sv}", "allow-take-over-physical",
                          g_variant_new_boolean(TRUE));

  g_variant_builder_init(&sessionOptions, G_VARIANT_TYPE("a{sv}"));
  g_variant_builder_add(&sessionOptions, "{sv}",
                        "allow-creating-monitors", g_variant_new_boolean(TRUE));

  result = remoteDesktop->call("CreateSession",
                               g_variant_new("(a{sv}a{sv})", &options,
                                             &sessionOptions));

  g_variant_get(result, "(o)", &path);
  g_variant_unref(result);

  sessionPath = path;
  g_free(path);

  return sessionPath;
}

uint32_t RemoteDesktopManager::getVersion()
{
  GVariant* property;
  uint32_t version;

  property = g_dbus_proxy_get_cached_property(remoteDesktop->getProxy(), "Version");

  if (!property)
    return 0;

  version = g_variant_get_uint32(property);
  g_variant_unref(property);

  return version;
}
