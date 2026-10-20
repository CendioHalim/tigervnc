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

#ifndef __XKB_KEYBOARD_H__
#define __XKB_KEYBOARD_H__

#include <functional>

#include <xkbcommon/xkbcommon.h>

struct XkbContext {
  xkb_context* ctx;
  xkb_state* state;
  xkb_keymap* keymap;
};

class XkbKeyboard {
public:
  XkbKeyboard(const char* keyMap, size_t size);
  ~XkbKeyboard();

  bool updateModifiers(uint32_t modsDepressed, uint32_t modsLatched,
                       uint32_t modsLocked, uint32_t group);
  bool updateState(uint32_t keycode, bool down, uint32_t* modsDepressed,
                   uint32_t* modsLatched, uint32_t* modsLocked,
                   uint32_t* group);

  uint32_t keysymToKeycode(int keycode);
  uint32_t rfbcodeToKeycode(uint32_t rfbcode);

  unsigned int getLEDState();

private:
  void generateKeycodeMap();
  void clearKeyMap();
  char* keyMap;
  XkbContext* context;
  // Called when the LED state has changed
  std::function<void(unsigned int)> setLEDstate;
  uint32_t codeMapQnumToKeyCode[256];
};

#endif  // __XKB_KEYBOARD_H__
