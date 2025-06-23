#ifndef __W_KEYBOARD_H__
#define __W_KEYBOARD_H__

#include <wayland-client-protocol.h>
#include <xkbcommon/xkbcommon.h>

class WDisplay;
class WSeat;
class WShm;
struct XkbContext;

struct KeyboardModifiersState {
  xkb_mod_mask_t modsDepressed;
  xkb_mod_mask_t modsLatched;
  xkb_mod_mask_t modsLocked;
  xkb_layout_index_t group;
  int ledState;
};

class WKeyboard {
public:
  WKeyboard(WDisplay* display, WSeat *seat);
  ~WKeyboard();

  uint32_t getFormat() const { return keyboardFormat; }
  int fd () const { return keyboardFd; }
  int size () const { return keyboardSize; }
  KeyboardModifiersState getModifiers() const { return modifiers; }
  // Returns true if the modifier state changed
  bool updateModifiers(uint32_t keycode, bool down);
  uint32_t keysymToKeycode(int keycode);
  uint32_t getReusableKeycode();

private:
  void handleKeyMap(uint32_t format, int32_t fd, uint32_t size);
  void handleModifiers(uint32_t serial, uint32_t modsDepressed,
                       uint32_t modsLatched, uint32_t modsLocked,
                       uint32_t group);

private:
  uint32_t keyboardFormat;
  int keyboardFd;
  int keyboardSize;
  wl_keyboard* keyboard;
  char* keyMap;
  static const wl_keyboard_listener listener;
  KeyboardModifiersState modifiers;
  XkbContext* context;
};
#endif // __W_KEYBOARD_H__
