#ifndef __W_KEYBOARD_H__
#define __W_KEYBOARD_H__

#include <wayland-client-protocol.h>

class WDisplay;
class WSeat;

class WKeyboard {
public:
  WKeyboard(WDisplay* display, WSeat *seat);
  ~WKeyboard();

  uint32_t getFormat() const { return keyboardFormat; }
  int fd() const { return keyboardFd; }
  int size() const { return keyboardSize; }

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
  static const wl_keyboard_listener listener;
};
#endif // __W_KEYBOARD_H__
