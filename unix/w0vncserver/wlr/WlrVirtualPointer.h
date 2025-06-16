#ifndef _WLR_VIRTUAL_POINTER_H
#define _WLR_VIRTUAL_POINTER_H

#include "wlr-virtual-pointer-unstable-v1.h"

#include "../wayland/WObject.h"

namespace core { class Point; }

class WDisplay;
class WSeat;

class WlrVirtualPointer : public WObject {
public:
  WlrVirtualPointer(WDisplay* display, WSeat* seat, uint32_t maxWidth,
                    uint32_t maxHeight);
  ~WlrVirtualPointer();

  void pointerEvent(const core::Point& pos, uint16_t buttonMask);

private:
  void handleMove(const core::Point& pos, int time);
  void handleButton(uint16_t button, bool down, int time);
  void handleScroll(int16_t button, int time);

private:
  zwlr_virtual_pointer_manager_v1* manager;
  zwlr_virtual_pointer_v1* cursor;
  uint32_t maxWidth;
  uint32_t maxHeight;
  uint16_t oldButtonMask;
};
#endif // _WLR_VIRTUAL_POINTER_H