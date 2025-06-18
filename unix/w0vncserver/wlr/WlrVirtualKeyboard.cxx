#include "virtual-keyboard-unstable-v1.h"

#include "../w0vncserver.h"
#include "../wayland/WDisplay.h"
#include "../wayland/WSeat.h"
#include "../wayland/WKeyboard.h"
#include "WlrVirtualKeyboard.h"

WlrVirtualKeyboard::WlrVirtualKeyboard(WDisplay* display, WSeat* seat)
  : WObject(display, "zwp_virtual_keyboard_manager_v1",
            &zwp_virtual_keyboard_manager_v1_interface)
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
}

WlrVirtualKeyboard::~WlrVirtualKeyboard()
{
  if (manager)
    zwp_virtual_keyboard_manager_v1_destroy(manager);
  if (keyboard)
    zwp_virtual_keyboard_v1_destroy(keyboard);
}

void WlrVirtualKeyboard::keyEvent(uint32_t keysym, uint32_t keycode, bool down)
{
  zwp_virtual_keyboard_v1_key(keyboard, keysym, keycode, down);
}
