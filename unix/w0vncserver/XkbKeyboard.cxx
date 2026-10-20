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

#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#include <stdexcept>

#include <core/LogWriter.h>
#include <rfb/ledStates.h>

#include "XkbKeyboard.h"

extern const unsigned int code_map_xkb_to_qnum_len;
extern const struct _code_map_xkb_to_qnum {
  const char* from;
  const unsigned short to;
} code_map_xkb_to_qnum[];

static core::LogWriter vlog("XkbKeyboard");


XkbKeyboard::XkbKeyboard(const char* keymap, size_t size)
  : context(nullptr)
{
  xkb_context* ctx;
  xkb_keymap* map;
  xkb_state* state;

  ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  if (!ctx)
    throw std::bad_alloc();

  memset(codeMapQnumToKeyCode, 0, sizeof(codeMapQnumToKeyCode));

  map = xkb_keymap_new_from_buffer(ctx, keymap, size,
                                   XKB_KEYMAP_FORMAT_TEXT_V1,
                                   XKB_KEYMAP_COMPILE_NO_FLAGS);
  if (!map)
    throw std::runtime_error("Failed to create xkb keymap from layout buffer string");

  state = xkb_state_new(map);
  if (!state) {
    xkb_keymap_unref(map);
    throw std::runtime_error("Failed to create xkb state machine layer for incoming map");
  }

  context = new XkbContext();
  context->ctx = ctx;
  context->keymap = map;
  context->state = state;

  generateKeycodeMap();
}

XkbKeyboard::~XkbKeyboard()
{
  clearKeyMap();
  delete context;
}

bool XkbKeyboard::updateModifiers(uint32_t modsDepressed,
                                  uint32_t modsLatched,
                                  uint32_t modsLocked,
                                  uint32_t group)
{
  xkb_state_component changed;

  if (!context || !context->state)
    return 0;

  changed = xkb_state_update_mask(context->state, modsDepressed,
                                  modsLatched, modsLocked, group, 0, 0);

  return changed != 0;
}

bool XkbKeyboard::updateState(uint32_t keycode, bool down,
                              uint32_t* modsDepressed,
                              uint32_t* modsLatched,
                              uint32_t* modsLocked, uint32_t* group)
{
  xkb_state_component changed;

  if (!context || !context->state)
    return false;

  changed = xkb_state_update_key(context->state, keycode,
                                 down ? XKB_KEY_DOWN : XKB_KEY_UP);

  *modsDepressed = xkb_state_serialize_mods(context->state, XKB_STATE_MODS_DEPRESSED);
  *modsLatched = xkb_state_serialize_mods(context->state, XKB_STATE_MODS_LATCHED);
  *modsLocked = xkb_state_serialize_mods(context->state, XKB_STATE_MODS_LOCKED);
  *group = xkb_state_serialize_mods(context->state, XKB_STATE_LAYOUT_EFFECTIVE);

  return changed != 0;
}

uint32_t XkbKeyboard::keysymToKeycode(int keysym)
{
  xkb_keycode_t min;
  xkb_keycode_t max;

  if (!context || !context->keymap)
    return XKB_KEYCODE_INVALID;

  min = xkb_keymap_min_keycode(context->keymap);
  max = xkb_keymap_max_keycode(context->keymap);

  for (xkb_keycode_t keycode = min; keycode <= max; keycode++) {
    if (xkb_state_key_get_one_sym(context->state, keycode) == (xkb_keysym_t)keysym)
      return keycode;
  }

  return XKB_KEYCODE_INVALID;
}

uint32_t XkbKeyboard::rfbcodeToKeycode(uint32_t rfbcode)
{
  if (!context)
    return XKB_KEYCODE_INVALID;
  if (rfbcode >= sizeof(codeMapQnumToKeyCode))
    return XKB_KEYCODE_INVALID;
  if (codeMapQnumToKeyCode[rfbcode] == 0)
    return XKB_KEYCODE_INVALID;
  return codeMapQnumToKeyCode[rfbcode];
}


unsigned int XkbKeyboard::getLEDState()
{
  unsigned int ledState;

  if (!context || !context->state)
    return 0;

  ledState = 0;
  if (xkb_state_led_name_is_active(context->state, XKB_LED_NAME_SCROLL))
    ledState |= rfb::ledScrollLock;
  if (xkb_state_led_name_is_active(context->state, XKB_LED_NAME_NUM))
    ledState |= rfb::ledNumLock;
  if (xkb_state_led_name_is_active(context->state, XKB_LED_NAME_CAPS))
    ledState |= rfb::ledCapsLock;

  return ledState;
}

void XkbKeyboard::generateKeycodeMap()
{
  if (!context || !context->keymap)
    return;

  memset(codeMapQnumToKeyCode, 0, sizeof(codeMapQnumToKeyCode));

  /* Here, we generate a mapping from qnumcode (RFB) to our keyboard's
   * configured keycodes. We iterate through code_map_xkb_to_qnum in
   * reverse and try to find matches between the keycodemap and our
   * keymap.
   *
   * Note that we have to iterate through the xkb_to_qnum map in reverse
   * (instead of using a qnum_to_xkb map) since the mapping between
   * keycodes and keynames is one-to-many. This means that the same
   * keycode may be mapped by multiple keynames (such as TLDE and AB00
   * both mapping to 0x29).
   *
   * The only exception to this rule is SYRQ, which for historical
   * reasons is mapped by both 0x54 and 0xb7.
   */

  for (unsigned int rfbcode = 1; rfbcode < 255; rfbcode++) {
    unsigned int xkbKeyCode;
    bool rfbFound;

    xkbKeyCode = 0;
    rfbFound = false;
    for (unsigned int j = 0; j < code_map_xkb_to_qnum_len; j++) {
      const char* keyName;

      if (code_map_xkb_to_qnum[j].to != rfbcode)
        continue;

      rfbFound = true;
      keyName = code_map_xkb_to_qnum[j].from;
      assert(keyName);

      xkbKeyCode = xkb_keymap_key_by_name(context->keymap, keyName);
      if (xkbKeyCode != XKB_KEYCODE_INVALID)
        break;
    }

    if (xkbKeyCode != 0 && xkbKeyCode != XKB_KEYCODE_INVALID)
      codeMapQnumToKeyCode[rfbcode] = xkbKeyCode;
    else if (rfbFound)
      vlog.debug("No mapping found for key 0x%04x", rfbcode);
  }
}

void XkbKeyboard::clearKeyMap()
{
  memset(codeMapQnumToKeyCode, 0, sizeof(codeMapQnumToKeyCode));
  if (context) {
    if (context->state)
      xkb_state_unref(context->state);
    if (context->keymap)
      xkb_keymap_unref(context->keymap);
    if (context->ctx)
      xkb_context_unref(context->ctx);
    context->state = nullptr;
    context->keymap = nullptr;
    context->ctx = nullptr;
  }
}
