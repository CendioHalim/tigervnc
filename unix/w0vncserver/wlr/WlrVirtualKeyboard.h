#ifndef __WLR_VIRTUAL_KEYBOARD_H__
#define __WLR_VIRTUAL_KEYBOARD_H__

#include <stdint.h>

#include "../wayland/WObject.h"

struct zwp_virtual_keyboard_manager_v1;
struct zwp_virtual_keyboard_v1;
class WDisplay;
class WSeat;

class WlrVirtualKeyboard : public WObject{
public:
  WlrVirtualKeyboard(WDisplay* display, WSeat* seat);
  ~WlrVirtualKeyboard();

  void keyEvent(uint32_t keysym, uint32_t keycode, bool down);

private:
  zwp_virtual_keyboard_manager_v1* manager;
  zwp_virtual_keyboard_v1* keyboard;
};
#endif // __WLR_VIRTUAL_KEYBOARD_H__
