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

#ifndef __EIS_CLIENT_H__
#define __EIS_CLIENT_H__

#include <stdint.h>

#include <functional>

class GEISSource;
class XkbKeyboard;

struct ei;
struct ei_device;
struct ei_seat;

struct EiDevice {
  ei_device* device;
  bool active;
  ~EiDevice();
};

class EISClient {
public:
  EISClient(int fd, std::function<void(unsigned int)> setLEDstate);
  virtual ~EISClient();

  void pointerMotionAbsolute(int x, int y);
  void pointerButton(int button, bool down);
  void pointerScroll(int16_t button);

  void keyboardKey(uint32_t keysym, uint32_t keycode, bool down);

private:
  friend class GEISSource;

  // Process incoming EIS events
  void processEvents();

  void handleSeatAdded(ei_seat* seat);
  void handleSeatRemoved(ei_seat* seat);
  bool handleDeviceAdded(ei_device* dev);
  void handleDeviceRemoved(ei_device* dev);
  void handleDevicePaused(ei_device* dev);
  void handleDeviceResumed(ei_device* dev);

  // Load the keymap of a keyboard device into xkbKeyboard
  bool loadKeymap(ei_device* dev);

private:
  GEISSource* source;
  struct ei* ei;
  ei_seat* seat;
  EiDevice* pointer;
  EiDevice* button;
  EiDevice* scroll;
  EiDevice* keyboard;
  XkbKeyboard* xkbKeyboard;
  std::function<void(unsigned int)> setLEDstate;
};

#endif // __EIS_CLIENT_H__
