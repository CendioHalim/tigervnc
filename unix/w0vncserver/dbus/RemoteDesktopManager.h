#ifndef __DBUS_REMOTE_DESKTOP_MANAGER_H__
#define __DBUS_REMOTE_DESKTOP_MANAGER_H__

#include <stdint.h>

#include <string>

namespace dbus {
  class Proxy;

  // FIXME: Should this class be called the Manager?
  class RemoteDesktopManager {
  public:
    RemoteDesktopManager();
    ~RemoteDesktopManager();

    std::string createSession(bool persistent = false,
                              const std::string& persistenceToken = "",
                              bool allowTakeOver = true);
    uint32_t getVersion();

  private:
    Proxy* remoteDesktop;
  };
}

#endif // __DBUS_REMOTE_DESKTOP_MANAGER_H__
