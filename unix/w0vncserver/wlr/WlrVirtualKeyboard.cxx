#include <time.h>

#include <core/LogWriter.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include "virtual-keyboard-unstable-v1.h"

#include "../w0vncserver.h"
#include "../wayland/WDisplay.h"
#include "../wayland/WSeat.h"
#include "../wayland/WKeyboard.h"
#include "WlrVirtualKeyboard.h"

extern const unsigned short code_map_qnum_to_xorgevdev[];
extern const unsigned int code_map_qnum_to_xorgevdev_len;

WlrVirtualKeyboard::WlrVirtualKeyboard(WDisplay* display, WSeat* seat_)
  : WObject(display, "zwp_virtual_keyboard_manager_v1",
            &zwp_virtual_keyboard_manager_v1_interface),
    modifierState(nullptr), seat(seat_)
{
  if (!seat->getKeyboard()->size())
    fatal_error("Keyboard keymap is not set");

  manager = (zwp_virtual_keyboard_manager_v1*) boundObject;

  keyboard = zwp_virtual_keyboard_manager_v1_create_virtual_keyboard(manager,
                                                                     seat->getSeat());
  if (!keyboard)
    fatal_error("Failed to create virtual keyboard");

  zwp_virtual_keyboard_v1_keymap(keyboard,
                                 seat->getKeyboard()->getFormat(),
                                 seat->getKeyboard()->fd(),
                                 seat->getKeyboard()->size());

  modifierState = new KeyboardModifiersState{0, 0, 0, 0, 0};
}

WlrVirtualKeyboard::~WlrVirtualKeyboard()
{
  if (manager)
    zwp_virtual_keyboard_manager_v1_destroy(manager);
  if (keyboard)
    zwp_virtual_keyboard_v1_destroy(keyboard);

  delete modifierState;
}

void WlrVirtualKeyboard::keyEvent(uint32_t /* keysym */, uint32_t keycode,
                                  bool down)
{
  timespec ts;
  uint32_t time;
  bool updated;
  if (keycode < code_map_qnum_to_xorgevdev_len)
    keycode = code_map_qnum_to_xorgevdev[keycode];

  // FIXME: look at keysym

  clock_gettime(CLOCK_MONOTONIC, &ts);
  time = (static_cast<uint64_t>(ts.tv_sec) * 1000 + ts.tv_nsec / 1000000) &
         0xFFFFFFFF;

  zwp_virtual_keyboard_v1_key(keyboard, time, keycode - 8, down);

  updated = seat->getKeyboard()->updateModifiers(keycode, down);

  if (updated) {
    KeyboardModifiersState state;
    state = seat->getKeyboard()->getModifiers();
    zwp_virtual_keyboard_v1_modifiers(keyboard, state.modsDepressed,
                                      state.modsLatched,
                                      state.modsLocked, state.group);
  }
}
