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

#include <stdexcept>
#include <sys/mman.h>
#include <assert.h>
#include <linux/input-event-codes.h>

#include <libei.h>

#include <core/i18n.h>
#include <core/LogWriter.h>
#include <rfb/KeysymStr.h>

#include "parameters.h"
#include "GEISSource.h"
#include "XkbKeyboard.h"
#include "EISClient.h"

static core::LogWriter vlog("EISClient");

static int sequence = 0;

// Scroll wheel events
#define WHEEL_VERTICAL_DOWN    3
#define WHEEL_VERTICAL_UP      4
#define WHEEL_HORIZONTAL_LEFT  5
#define WHEEL_HORIZONTAL_RIGHT 6

static int getInputCode(uint32_t button)
{
  switch (button) {
  case 0x0:
    return BTN_LEFT;
  case 0x1:
    return BTN_MIDDLE;
  case 0x2:
    return BTN_RIGHT;
  case 0x7:
    return BTN_SIDE;
  case 0x8:
    return BTN_EXTRA;
  default:
    assert(false);
  }
}

EiDevice::~EiDevice()
{
  // FIXME: Do we need to do something more here?
  ei_device_unref(device);
}

EISClient::EISClient(int fd_, std::function<void(unsigned int)> setLEDstate_)
  : source(nullptr), ei(nullptr), seat(nullptr), pointer(nullptr),
    button(nullptr), scroll(nullptr), keyboard(nullptr),
    xkbKeyboard(nullptr), setLEDstate(setLEDstate_)
{
  ei = ei_new_sender(nullptr);

  if (!ei)
    throw std::runtime_error(_("Failed to set up libei"));

  if (ei_setup_backend_fd(ei, fd_) < 0) {
    ei_unref(ei);
    throw std::runtime_error(_("Failed to set up libei"));
  }

  source = new GEISSource(this, ei_get_fd(ei));
}

EISClient::~EISClient()
{
  delete source;
  delete xkbKeyboard;

  delete pointer;
  delete button;
  delete scroll;
  delete keyboard;

  ei_disconnect(ei);
  ei_unref(ei);
}

void EISClient::processEvents()
{
  ei_event* ev;

  ei_dispatch(ei);

  while ((ev = ei_get_event(ei))) {
    switch (ei_event_get_type(ev)) {
    case EI_EVENT_CONNECT:
      vlog.debug("Connected to EIS server");
      break;
    case EI_EVENT_DISCONNECT:
      // FIXME: Cleanup here, we should stop the source and this object
      // should be deleted. Will need a callback from the parent object.
      vlog.debug("Disconnected from EIS server");
      break;
    case EI_EVENT_SEAT_ADDED:
      handleSeatAdded(ei_event_get_seat(ev));
      break;
    case EI_EVENT_SEAT_REMOVED:
      handleSeatRemoved(ei_event_get_seat(ev));
      break;
    case EI_EVENT_DEVICE_ADDED:
      // FIXME: Do more than just log on error?
      if (!handleDeviceAdded(ei_event_get_device(ev)))
        vlog.error("Failed to add device");
      break;
    case EI_EVENT_DEVICE_REMOVED:
      handleDeviceRemoved(ei_event_get_device(ev));
      break;
    case EI_EVENT_DEVICE_PAUSED:
      handleDevicePaused(ei_event_get_device(ev));
      break;
    case EI_EVENT_DEVICE_RESUMED:
      handleDeviceResumed(ei_event_get_device(ev));
      break;
    case EI_EVENT_KEYBOARD_MODIFIERS:
      // FIXME: We should probably try and sync our keyboard state here
      break;
    case EI_EVENT_SYNC:
      // FIXME: Nothing useful here right now...
      break;
    default:
      break;
    }
    ei_event_unref(ev);
  }
}

void EISClient::handleSeatAdded(ei_seat* seat_)
{
  if (!seat_)
    return;

  if (seat) {
    vlog.debug("Only one seat supported, ignoring seat '%s'",
               ei_seat_get_name(seat_));
    return;
  }

  ei_seat_ref(seat_);
  seat = seat_;

  vlog.debug("Seat '%s' added", ei_seat_get_name(seat_));

  if (!ei_seat_has_capability(seat_, EI_DEVICE_CAP_KEYBOARD))
    vlog.info("No keyboard support");
  if (!ei_seat_has_capability(seat_, EI_DEVICE_CAP_POINTER_ABSOLUTE))
    vlog.info("No absolute pointer support");
  if (!ei_seat_has_capability(seat_, EI_DEVICE_CAP_BUTTON))
    vlog.info("No pointer button support");
  if (!ei_seat_has_capability(seat_, EI_DEVICE_CAP_SCROLL))
    vlog.info("No scroll wheel support");

  /* FIXME: From the documentation:
   *   It is an application bug to call this function for a capability already
   *   bound - call ei_seat_unbind_capabilities() first.
   * We should keep track of this.
   */
  ei_seat_bind_capabilities(seat,
                            EI_DEVICE_CAP_KEYBOARD,
                            EI_DEVICE_CAP_POINTER_ABSOLUTE,
                            EI_DEVICE_CAP_BUTTON,
                            EI_DEVICE_CAP_SCROLL,
                            nullptr);
}

void EISClient::handleSeatRemoved(ei_seat* seat_)
{
  if (!seat_)
    return;

  if (seat != seat_)
    return;

  vlog.debug("Seat '%s' removed", ei_seat_get_name(seat));
  ei_seat_unref(seat);
  seat = nullptr;
}

bool EISClient::handleDeviceAdded(ei_device* dev)
{
  if (!keyboard && ei_device_has_capability(dev, EI_DEVICE_CAP_KEYBOARD)) {
    if (!loadKeymap(dev))
      vlog.error(_("Failed to load keymap"));
    else
      keyboard = new EiDevice{ei_device_ref(dev), false};
  }
  if (!pointer && ei_device_has_capability(dev, EI_DEVICE_CAP_POINTER_ABSOLUTE))
    pointer = new EiDevice{ei_device_ref(dev), false};
  if (!button && ei_device_has_capability(dev, EI_DEVICE_CAP_BUTTON))
    button = new EiDevice{ei_device_ref(dev), false};
  if (!scroll && ei_device_has_capability(dev, EI_DEVICE_CAP_SCROLL))
    scroll = new EiDevice{ei_device_ref(dev), false};

  {
    // FIXME: For debugging, remove later
    if (ei_device_has_capability(dev, EI_DEVICE_CAP_KEYBOARD))
      vlog.debug("Keyboard device '%s' added", ei_device_get_name(dev));
    if (ei_device_has_capability(dev, EI_DEVICE_CAP_POINTER_ABSOLUTE))
      vlog.debug("Absolute pointer device '%s' added", ei_device_get_name(dev));
    if (ei_device_has_capability(dev, EI_DEVICE_CAP_BUTTON))
      vlog.debug("Button device '%s' added", ei_device_get_name(dev));
    if (ei_device_has_capability(dev, EI_DEVICE_CAP_SCROLL))
      vlog.debug("Scroll device '%s' added", ei_device_get_name(dev));
  }

  return true;
}

void EISClient::handleDeviceRemoved(ei_device* dev)
{
  vlog.debug("Device '%s' removed", ei_device_get_name(dev));

  if (pointer && pointer->device == dev)
    delete pointer;
  if (button && button->device == dev)
    delete button;
  if (scroll && scroll->device == dev)
    delete scroll;
  if (keyboard && keyboard->device == dev)
    delete keyboard;
}

void EISClient::handleDevicePaused(ei_device* dev)
{
  if (!dev)
    return;

  vlog.debug("Device '%s' paused", ei_device_get_name(dev));
  if (pointer && pointer->device == dev)
    pointer->active = false;
  if (button && button->device == dev)
    button->active = false;
  if (scroll && scroll->device == dev)
    scroll->active = false;
  if (keyboard && keyboard->device == dev)
    keyboard->active = false;
}

void EISClient::handleDeviceResumed(ei_device* dev)
{
  if (!dev)
    return;

  vlog.debug("Device '%s' resumed", ei_device_get_name(dev));
  if (pointer && pointer->device == dev) {
    ei_device_start_emulating(pointer->device, ++sequence);
    pointer->active = true;
  }
  if (button && button->device == dev) {
    ei_device_start_emulating(button->device, ++sequence);
    button->active = true;
  }
  if (scroll && scroll->device == dev) {
    ei_device_start_emulating(scroll->device, ++sequence);
    scroll->active = true;
  }
  if (keyboard && keyboard->device == dev) {
    ei_device_start_emulating(keyboard->device, ++sequence);
    keyboard->active = true;
  }
}

bool EISClient::loadKeymap(ei_device* dev)
{
  ei_keymap* keymap;
  size_t size;
  void* data;
  XkbKeyboard* xkb;

  keymap = ei_device_keyboard_get_keymap(dev);
  if (!keymap) {
    vlog.error(_("Keyboard device has no keymap"));
    return false;
  }

  size = ei_keymap_get_size(keymap);
  data = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, ei_keymap_get_fd(keymap), 0);
  if (data == MAP_FAILED) {
    vlog.error("Failed to map keymap: %s", strerror(errno));
    return false;
  }

  xkb = nullptr;
  try {
    xkb = new XkbKeyboard((const char*)data, size);
  } catch (std::exception& e) {
    vlog.error(_("Failed to load keymap: %s"), e.what());
  }

  munmap(data, size);

  if (!xkb)
    return false;

  delete xkbKeyboard;
  xkbKeyboard = xkb;

  return true;
}

void EISClient::pointerMotionAbsolute(int x, int y)
{
  // FIXME: This will spam the log
  if (!pointer || !pointer->active) {
    vlog.error(_("No active pointer device"));
    return;
  }

  // Bounds check
  if (!ei_device_get_region_at(pointer->device, x, y)) {
    vlog.debug("Pointer out of bounds, ignoring: (%d, %d)", x, y);
    return;
  }

  ei_device_pointer_motion_absolute(pointer->device, (double)x, (double)y);
  ei_device_frame(pointer->device, ei_now(ei));
}

void EISClient::pointerButton(int btn, bool down)
{
  if (!button || !button->active) {
    vlog.error(_("No active button device"));
    return;
  }

  ei_device_button_button(button->device, getInputCode(btn), down);
  ei_device_frame(button->device, ei_now(ei));
}

void EISClient::pointerScroll(int16_t btn)
{
  if (!scroll || !scroll->active) {
    vlog.error(_("No active scroll device"));
    return;
  }

  switch (btn) {
  case WHEEL_VERTICAL_DOWN:
    ei_device_scroll_discrete(scroll->device, 0, -120);
    break;
  case WHEEL_VERTICAL_UP:
    ei_device_scroll_discrete(scroll->device, 0, 120);
    break;
  case WHEEL_HORIZONTAL_LEFT:
    ei_device_scroll_discrete(scroll->device, -120, 0);
    break;
  case WHEEL_HORIZONTAL_RIGHT:
    ei_device_scroll_discrete(scroll->device, 120, 0);
    break;
  default:
    assert(false);
  }

  ei_device_frame(scroll->device, ei_now(ei));
}

void EISClient::keyboardKey(uint32_t keysym, uint32_t keycode, bool down)
{
  uint32_t key;
  uint32_t modsDepressed;
  uint32_t modsLatched;
  uint32_t modsLocked;
  uint32_t group;
  bool changed;

  if (!keyboard || !keyboard->active) {
    vlog.error(_("No active keyboard device"));
    return;
  }

  if (!xkbKeyboard) {
    // FIXME: This will spam the log
    vlog.error(_("Keyboard not initialized"));
    return;
  }

  key = XKB_KEYCODE_INVALID;

  if (rawKeyboard)
    key = xkbKeyboard->rfbcodeToKeycode(keycode);

  if (key == XKB_KEYCODE_INVALID) {
    key = xkbKeyboard->keysymToKeycode(keysym);
    if (key == XKB_KEYCODE_INVALID) {
      // FIXME: Mapping new keys is not possible right now as there
      // is no API to do this. What we dould want to do here is find a
      // free(-ish) keycode and map the keysym/keycode. We need a way
      // to update the keymap in the compositor and make sure our xkb
      // keymap is up-to-date.
      vlog.debug("Unable to map keysym XK_%s (0x%04x), ignoring key press",
                 KeySymName(keysym), keysym);
      return;
    }
  }


  ei_device_keyboard_key(keyboard->device, key - 8, down);
  ei_device_frame(keyboard->device, ei_now(ei));

  changed = xkbKeyboard->updateState(key, down, &modsDepressed,
                                     &modsLatched, &modsLocked, &group);
  if (changed)
    setLEDstate(xkbKeyboard->getLEDState());
}
