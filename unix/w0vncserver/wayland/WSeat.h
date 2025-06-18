#ifndef __W_SEAT_H__
#define __W_SEAT_H__

#include <wayland-client-protocol.h>

#include "WObject.h"

class WDisplay;
class WKeyboard;

class WSeat : public WObject {
public:
  WSeat(WDisplay* display);
  ~WSeat();

  wl_seat* getSeat() const { return seat; }
  WKeyboard* getKeyboard() const { return keyboard; }

private:
 void seatCapabilities(void* data, wl_seat* wlSeat,
                       uint32_t capabilities);

private:
  wl_seat* seat;
  WDisplay* display;
  WKeyboard* keyboard;
  static const wl_seat_listener listener;
};

#endif // __W_SEAT_H__