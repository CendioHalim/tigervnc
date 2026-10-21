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

#ifndef __REMOTE_DESKTOP_1_DESKTOP_H__
#define __REMOTE_DESKTOP_1_DESKTOP_H__

#include <rfb/SDesktop.h>

namespace dbus {
  class RemoteDesktopManager;
  class RemoteDesktopSession;
  class VirtualMonitor;
}

namespace rfb {
  class SConnection;
}

class PipeWirePixelBuffer;
class EISClient;

class HeadlessDesktop : public rfb::SDesktop {
public:
  HeadlessDesktop();
  virtual ~HeadlessDesktop();

  // -=- SDesktop interface
  void init(rfb::VNCServer* vs) override;
  void start() override;
  void stop() override;
  void queryConnection(rfb::SConnection* conn) override;
  void terminate() override;
  unsigned int setScreenLayout(int fb_width, int fb_height,
                               const rfb::ScreenSet& layout) override;
  void keyEvent(uint32_t keysym, uint32_t keycode, bool down) override;
  void pointerEvent(const core::Point& pos, uint16_t buttonMask) override;

  static bool available();

protected:
  rfb::VNCServer* server;
  dbus::RemoteDesktopManager* manager;
  dbus::RemoteDesktopSession* session;
  dbus::VirtualMonitor* monitor;
  PipeWirePixelBuffer* pb;
  EISClient* eisClient;
  uint16_t oldButtonMask;
};

#endif // __REMOTE_DESKTOP_1_DESKTOP_H__
