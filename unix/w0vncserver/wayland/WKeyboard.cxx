#include <sys/mman.h>

#include <xkbcommon/xkbcommon.h>
#include <wayland-client-protocol.h>

#include <core/LogWriter.h>

#include "../w0vncserver.h"
#include "WDisplay.h"
#include "WSeat.h"
#include "WKeyboard.h"

static core::LogWriter vlog("WKeyboard");

extern const unsigned short code_map_qnum_to_xorgevdev[];
extern const unsigned int code_map_qnum_to_xorgevdev_len;

const wl_keyboard_listener WKeyboard::listener = {
  .keymap = [](void* data, wl_keyboard* /* wl_keyboard */,
                uint32_t format, int32_t fd, uint32_t size) {
    ((WKeyboard*)data)->handleKeyMap(format, fd, size);
  },
  .enter = [](void*, wl_keyboard*, uint32_t, wl_surface*,
              wl_array*) {},
  .leave = [](void*, wl_keyboard*, uint32_t, wl_surface*) {},
  .key = [](void*, wl_keyboard*, uint32_t, uint32_t, uint32_t,
            uint32_t) {},
  .modifiers = [](void* data, struct wl_keyboard* /* wl_keyboard */,
                  uint32_t serial, uint32_t modsDepressed,
                  uint32_t modsLatched, uint32_t modsLocked,
                  uint32_t group) {
    ((WKeyboard*)data)->handleModifiers(serial, modsDepressed,
                                        modsLatched,
                                        modsLocked, group);
  },
  .repeat_info = [](void*, wl_keyboard*, int32_t, int32_t) {}
};

struct XkbContext {
  xkb_context* ctx;
  xkb_state* state;
  xkb_keymap* keymap;
};

WKeyboard::WKeyboard(WDisplay* display, WSeat* seat)
  : keyboardFormat(0), keyboardFd(0), keyboardSize(0),
    keyboard(nullptr), keyMap(nullptr), context(nullptr)
{
  xkb_context* ctx;

  keyboard = wl_seat_get_keyboard(seat->getSeat());
  wl_keyboard_add_listener(keyboard, &listener, this);
  display->roundtrip();

  context = new XkbContext();

  ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  if (!ctx) // FIXME: fallback?
    fatal_error("Failed to create xkb context");
  context->ctx = ctx;
}

WKeyboard::~WKeyboard()
{
  if (keyboard)
    wl_keyboard_destroy(keyboard);
  if(keyMap)
    munmap(keyMap, keyboardSize);

  if (context) {
    xkb_context_unref(context->ctx);
    xkb_state_unref(context->state);
    xkb_keymap_unref(context->keymap);
    delete context;
  }
}

bool WKeyboard::updateModifiers(uint32_t keycode, bool down)
{
  xkb_state_component changed;

  changed = xkb_state_update_key(context->state, keycode,
                                 down ? XKB_KEY_DOWN : XKB_KEY_UP);

  modifiers.modsDepressed = xkb_state_serialize_mods(context->state,
                                                     XKB_STATE_MODS_DEPRESSED);
  modifiers.modsLatched = xkb_state_serialize_mods(context->state,
                                                   XKB_STATE_MODS_LATCHED);
  modifiers.modsLocked = xkb_state_serialize_mods(context->state,
                                                  XKB_STATE_MODS_LOCKED);
  modifiers.group = xkb_state_serialize_mods(context->state,
                                             XKB_STATE_MODS_EFFECTIVE);
  modifiers.ledState = xkb_state_serialize_mods(context->state,
                                                XKB_STATE_LEDS);
  return changed;
}

void WKeyboard::handleKeyMap(uint32_t format, int32_t fd, uint32_t size)
{
  // https://wayland.app/protocols/wayland#wl_keyboard:enum:keymap_format
  // Format is either:
  //  no_keymap 0, or
  //  xkb_v1    1
  if (keyMap)
    munmap(keyMap, keyboardSize);
  keyboardFormat = format;
  keyboardFd = fd;
  keyboardSize = size;

  if (keyboardFormat == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
    xkb_keymap* map;
    xkb_state* state;

    keyMap = (char*)mmap(nullptr, keyboardSize, PROT_READ, MAP_PRIVATE,
                         keyboardFd, 0);
    if (!keyMap)
      fatal_error("Failed to map keymap");

    if (context) {
      map = xkb_keymap_new_from_buffer(context->ctx, keyMap,
                                       keyboardSize,
                                       XKB_KEYMAP_FORMAT_TEXT_V1,
                                       XKB_KEYMAP_COMPILE_NO_FLAGS);
      if (!map)
        fatal_error("Failed to create xkb keymap");

      if (context->keymap)
        xkb_keymap_unref(context->keymap);
      context->keymap = map;

      state = xkb_state_new(context->keymap);
      if (!state)
        fatal_error("Failed to create xkb state");

      if (context->state)
        xkb_state_unref(context->state);
      context->state = state;

      vlog.debug("Keymap updated");
    }
  } else {
    // FIXME: implement fallback?
    fatal_error("Unsupported keymap format");
    return;
  }
}

uint32_t WKeyboard::keysymToKeycode(int keysym)
{
  xkb_layout_index_t numLayouts;
  xkb_keycode_t min;
  xkb_keycode_t max;

  min = xkb_keymap_min_keycode(context->keymap);
  max = xkb_keymap_max_keycode(context->keymap);

  for (xkb_keycode_t current = min; current <= max; current++) {
    numLayouts = xkb_keymap_num_layouts_for_key(context->keymap, current);
    for (xkb_layout_index_t layout = 0; layout < numLayouts; layout++) {
      xkb_level_index_t numLevels;

      numLevels = xkb_keymap_num_levels_for_key(context->keymap, current, layout);

      for (xkb_level_index_t level = 0; level < numLevels; level++) {
        const xkb_keysym_t* syms;
        int numSyms;

        numSyms = xkb_keymap_key_get_syms_by_level(
          context->keymap, current, layout, level, &syms);
        for (int i = 0; i < numSyms; i++) {
          if (syms[i] == (xkb_keysym_t)keysym)
            return current;
        }
      }
    }
  }

  return XKB_KEYCODE_INVALID;
}

uint32_t WKeyboard::getReusableKeycode()
{
  xkb_layout_index_t numLayouts;
  xkb_keycode_t min;
  xkb_keycode_t max;
  bool keycodeMapped;

  min = xkb_keymap_min_keycode(context->keymap);
  max = xkb_keymap_max_keycode(context->keymap);

  for (xkb_keycode_t current = min; current <= max; current++) {
    keycodeMapped = false;
    numLayouts = xkb_keymap_num_layouts_for_key(context->keymap, current);
    for (xkb_layout_index_t layout = 0; layout < numLayouts; layout++) {
      xkb_level_index_t numLevels;
      numLevels = xkb_keymap_num_levels_for_key(context->keymap, current, layout);
      for (xkb_level_index_t level = 0; level < numLevels; level++) {
        const xkb_keysym_t* syms;
        int numSyms;

        numSyms = xkb_keymap_key_get_syms_by_level( context->keymap, current, layout, level, &syms);
        if (numSyms && syms[0] != XKB_KEY_NoSymbol) {
          keycodeMapped = true;
          break;
        }
      }
    }
    if (!keycodeMapped)
      return current;
  }

  return XKB_KEYCODE_INVALID;
}


void WKeyboard::handleModifiers(uint32_t /* serial */, uint32_t modsDepressed,
                                uint32_t modsLatched,
                                uint32_t modsLocked, uint32_t group)
{

  modifiers.modsDepressed = modsDepressed;
  modifiers.modsLatched = modsLatched;
  modifiers.modsLocked = modsLocked;
  modifiers.group = group;

  // FIXME: What do we set the layouts to?
  xkb_state_update_mask(context->state, modsDepressed, modsLatched,
                        modsLocked, 0, 0, 0);
}
