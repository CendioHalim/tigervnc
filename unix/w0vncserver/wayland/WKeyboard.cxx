#include <wayland-client-protocol.h>

#include <core/LogWriter.h>

#include "WDisplay.h"
#include "WSeat.h"
#include "WKeyboard.h"

static core::LogWriter vlog("WKeyboard");

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

WKeyboard::WKeyboard(WDisplay* display, WSeat* seat)
  : keyboardFormat(0), keyboardFd(0), keyboardSize(0), keyboard(nullptr)
{
  keyboard = wl_seat_get_keyboard(seat->getSeat());
  wl_keyboard_add_listener(keyboard, &listener, this);
  display->roundtrip();
}

WKeyboard::~WKeyboard()
{
  if (keyboard)
    wl_keyboard_destroy(keyboard);
}

void WKeyboard::handleKeyMap(uint32_t format, int32_t fd, uint32_t size)
{
  // https://wayland.app/protocols/wayland#wl_keyboard:enum:keymap_format
  // Format is either:
  //  no_keymap 0, or
  //  xkb_v1    1
  // FIXME: Implement libxkbcommon
  keyboardFormat = format;
  keyboardFd = fd;
  keyboardSize = size;
}

void WKeyboard::handleModifiers(uint32_t /* serial */,
                                uint32_t /* modsDepressed */,
                                uint32_t /* modsLatched */,
                                uint32_t /* modsLocket */,
                                uint32_t /* group */)
{
  // FIXME: Handle modifiers
}
