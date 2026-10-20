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

#ifndef __DBUS_PROXY_H__
#define __DBUS_PROXY_H__

#include <stdint.h>

#include <functional>
#include <string>
#include <vector>
#include <list>

#include <gio/gio.h>

namespace dbus {

  class Proxy {
  public:
    Proxy(const char* name, const char* objectPath,
          const char* interfaceName);
    ~Proxy();

    GVariant* call(const char* method, GVariant* parameters,
                   GDBusCallFlags flags = G_DBUS_CALL_FLAGS_NONE);
    GVariant* callWithUnixFdList(const char* method, GVariant* parameters,
                                 GUnixFDList* fdList, GUnixFDList** outFdList,
                                 GDBusCallFlags flags = G_DBUS_CALL_FLAGS_NONE);

    GVariant* getProperty(const char* property);

    // Subscribe to a DBUS signal. signalCallback() is run whenever
    // the signal is emitted
    void subscribe(const char* member,
                   std::function<void(GVariant* parameters)>
                     signalCallback);
    // Unsubscribe from a DBUS signal
    void unsubscribe(uint32_t signalId);

    // Checks if interfaces are available
    bool interfacesAvailable(std::vector<std::string> interfaces);

    GDBusProxy* getProxy() const { return proxy; }

  private:
    GDBusConnection* connection;
    GDBusProxy* proxy;
    std::list<uint32_t> pendingCalls;
    std::list<uint32_t> subscribedSignals;
  };
}

#endif // __DBUS_PROXY_H__
