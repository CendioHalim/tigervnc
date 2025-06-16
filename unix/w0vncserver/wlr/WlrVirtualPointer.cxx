#include <assert.h>

#include <stdexcept>
#include <chrono>

#include "wlr-virtual-pointer-unstable-v1.h"

#include <core/Rect.h>

#include "../wayland/WSeat.h"
#include "../wayland/WDisplay.h"
#include "WlrVirtualPointer.h"

#define BUTTONS 9

// evdev input event codes
#define BTN_LEFT    0x110
#define BTN_RIGHT   0x111
#define BTN_MIDDLE  0x112
#define BTN_SIDE    0x113
#define BTN_EXTRA   0x114

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
    throw std::runtime_error("Unknown mouse button");
  }
}


WlrVirtualPointer::WlrVirtualPointer(WDisplay* display, WSeat* seat,
                                     uint32_t maxWidth_,
                                     uint32_t maxHeight_)
: WObject(display, "zwlr_virtual_pointer_manager_v1",
         &zwlr_virtual_pointer_manager_v1_interface),
  maxWidth(maxWidth_), maxHeight(maxHeight_), oldButtonMask(0)
{
  manager = (zwlr_virtual_pointer_manager_v1*) boundObject;

  cursor = zwlr_virtual_pointer_manager_v1_create_virtual_pointer(manager,
                                                                  seat->getSeat());
  if (!cursor)
    throw std::runtime_error("Failed to create virtual pointer");

  zwlr_virtual_pointer_v1_axis_source(cursor, WL_POINTER_AXIS_SOURCE_WHEEL);
}

WlrVirtualPointer::~WlrVirtualPointer()
{
  if (manager)
    zwlr_virtual_pointer_manager_v1_destroy(manager);
  if (cursor)
    zwlr_virtual_pointer_v1_destroy(cursor);
}

void WlrVirtualPointer::pointerEvent(const core::Point& pos,
                                     uint16_t buttonMask)
{
  timespec ts;
  uint32_t time;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  time = (static_cast<uint64_t>(ts.tv_sec) * 1000 + ts.tv_nsec / 1000000) &
         0xFFFFFFFF;

  zwlr_virtual_pointer_v1_motion_absolute(cursor, time, pos.x,
                                          pos.y, maxWidth,
                                          maxHeight);
  zwlr_virtual_pointer_v1_frame(cursor);

  if (buttonMask == oldButtonMask)
    return;

  for (int32_t i = 0; i < BUTTONS; i++) {
    if ((buttonMask ^ oldButtonMask) & (1 << i)) {
      if (i > 2 && i < 7)
        handleScroll(i, time);
      else
        handleButton(i, buttonMask & (1 << i), time);
    }
  }

  zwlr_virtual_pointer_v1_frame(cursor);

  oldButtonMask = buttonMask;
}

void WlrVirtualPointer::handleButton(uint16_t button, bool down, int time)
{
  zwlr_virtual_pointer_v1_button(cursor, time, getInputCode(button),
                                 down);
}

void WlrVirtualPointer::handleScroll(int16_t button, int time)
{
  assert(button > 2 && button < 7);

  int32_t axis;
  int value;
  int discrete;

  // FIXME: Not sure what this is supposed to be.
  value = 1;

  switch (button) {
  case WHEEL_VERTICAL_DOWN:
    axis = WL_POINTER_AXIS_VERTICAL_SCROLL;
    discrete = -1;
    break;
    case WHEEL_VERTICAL_UP:
    axis = WL_POINTER_AXIS_VERTICAL_SCROLL;
    discrete = 1;
    break;
  case WHEEL_HORIZONTAL_LEFT:
    axis = WL_POINTER_AXIS_HORIZONTAL_SCROLL;
    discrete = -1;
    break;
  case WHEEL_HORIZONTAL_RIGHT:
    axis = WL_POINTER_AXIS_HORIZONTAL_SCROLL;
    discrete = -1;
    break;
  default:
    assert(false);
  }

  zwlr_virtual_pointer_v1_axis_discrete(cursor, time, axis, value,
                                        discrete);
}
