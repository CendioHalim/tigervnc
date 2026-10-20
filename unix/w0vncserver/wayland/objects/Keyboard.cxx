/* Copyright 2025-2026 Adam Halim for Cendio AB
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
#include <sys/mman.h>

#include <string.h>

#include <wayland-client-protocol.h>

#include <core/LogWriter.h>
#include <core/i18n.h>

#include <rfb/ledStates.h>

#include "../../w0vncserver.h"
#include "../../XkbKeyboard.h"
#include "Display.h"
#include "Seat.h"
#include "Keyboard.h"

using namespace wayland;

static core::LogWriter vlog("Keyboard");

const wl_keyboard_listener Keyboard::listener = {
  .keymap = [](void* data, wl_keyboard*, uint32_t format,
               int32_t fd, uint32_t size) {
    ((Keyboard*)data)->handleKeyMap(format, fd, size);
  },
  .enter = [](void*, wl_keyboard*, uint32_t, wl_surface*,
              wl_array*) {},
  .leave = [](void*, wl_keyboard*, uint32_t, wl_surface*) {},
  .key = [](void*, wl_keyboard*, uint32_t, uint32_t, uint32_t,
            uint32_t) {},
  .modifiers = [](void* data, struct wl_keyboard*,
                  uint32_t serial, uint32_t modsDepressed,
                  uint32_t modsLatched, uint32_t modsLocked,
                  uint32_t group) {
    ((Keyboard*)data)->handleModifiers(serial, modsDepressed,
                                       modsLatched,
                                       modsLocked, group);
  },
  .repeat_info = [](void*, wl_keyboard*, int32_t, int32_t) {}
};

Keyboard::Keyboard(Display* display, Seat* seat,
                   std::function<void(unsigned int)> setLEDstate_)
  : keyboardFormat(0), keyboardFd(0), keyboardSize(0),
    keyboard(nullptr), xkbKeyboard(nullptr), keyMap(nullptr),
    setLEDstate(setLEDstate_)
{
  keyboard = wl_seat_get_keyboard(seat->getSeat());
  wl_keyboard_add_listener(keyboard, &listener, this);
  display->roundtrip();

  memset(codeMapQnumToKeyCode, 0, sizeof(codeMapQnumToKeyCode));
}

Keyboard::~Keyboard()
{
  if (keyboard)
    wl_keyboard_destroy(keyboard);
  delete xkbKeyboard;
}

bool Keyboard::updateState(uint32_t keycode, bool down,
                           uint32_t* modsDepressed,
                           uint32_t* modsLatched, uint32_t* modsLocked,
                           uint32_t* group)
{
  bool changed;

  if (!xkbKeyboard)
    return false;

  changed = xkbKeyboard->updateState(keycode, down, modsDepressed,
                                     modsLatched, modsLocked, group);
  if (changed)
    setLEDstate(getLEDState());

  return changed != 0;
}

void Keyboard::handleKeyMap(uint32_t format, int32_t fd, uint32_t size)
{
  // https://wayland.app/protocols/wayland#wl_keyboard:enum:keymap_format
  // Format is either:
  //  no_keymap 0, or
  //  xkb_v1    1
  if (format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
    char* newKeyMap;

    newKeyMap = (char*)mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (newKeyMap == MAP_FAILED) {
      vlog.error(_("Failed to access keyboard layout: %s"),
                 strerror(errno));
      return;
    }

    if (keyMap && strcmp(keyMap, newKeyMap) == 0) {
      // Keymap unchanged, no need to update anything.
      // This is a workaround for where zwp_virtual_keyboard_v1_keymap()
      // will trigger a keymap event with the same keymap as before
      return;
    }

    if (keyMap)
      munmap(keyMap, keyboardSize);

    keyMap = newKeyMap;

    xkbKeyboard = new XkbKeyboard(keyMap, size);

    keyboardFormat = format;
    keyboardFd = fd;
    keyboardSize = size;
  } else {
    vlog.error(_("Unsupported keymap format: %d"), format);
    delete xkbKeyboard;
    xkbKeyboard = nullptr;
  }
}

uint32_t Keyboard::keysymToKeycode(int keysym)
{
  if (!xkbKeyboard)
    return XKB_KEYCODE_INVALID;

  return xkbKeyboard->keysymToKeycode(keysym);
}

uint32_t Keyboard::rfbcodeToKeycode(uint32_t rfbcode)
{
  if (!xkbKeyboard)
    return XKB_KEYCODE_INVALID;
  return xkbKeyboard->rfbcodeToKeycode(rfbcode);
}

void Keyboard::handleModifiers(uint32_t /* serial */,
                               uint32_t modsDepressed,
                               uint32_t modsLatched,
                               uint32_t modsLocked, uint32_t group)
{
  bool changed;

  if (!xkbKeyboard)
    return;

  // FIXME: What do we set the latched/locked layouts to?
  changed = xkbKeyboard->updateModifiers(modsDepressed, modsLatched,
                                         modsLocked, group);

  if (changed & XKB_STATE_LEDS)
    setLEDstate(getLEDState());
}

unsigned int Keyboard::getLEDState()
{
  if (!xkbKeyboard)
    return 0;

  return xkbKeyboard->getLEDState();
}
