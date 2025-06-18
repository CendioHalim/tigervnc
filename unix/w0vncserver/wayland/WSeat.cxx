#include <core/LogWriter.h>
#include <wayland-client-protocol.h>

#include "WDisplay.h"
#include "WKeyboard.h"
#include "WSeat.h"

static core::LogWriter vlog("WSeat");

const wl_seat_listener WSeat::listener = {
  .capabilities = [](void* data, wl_seat* wlSeat,
                     uint32_t capabilities) {
    ((WSeat*)data)->seatCapabilities(data, wlSeat, capabilities);
  },
  .name = [](void*, wl_seat*, const char*) {},
};

WSeat::WSeat(WDisplay* display_)
  : WObject(display_, "wl_seat", &wl_seat_interface),
    seat(nullptr), display(display_), keyboard(nullptr)
{
  seat = (wl_seat*) boundObject;
  wl_seat_add_listener(seat, &listener, this);
  display->roundtrip();
}

WSeat::~WSeat()
{
  if (seat)
    wl_seat_destroy(seat);

  delete keyboard;
}

void WSeat::seatCapabilities(void* /* data */, wl_seat* /* wlSeat */,
                             uint32_t capabilities)
{
  if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
    vlog.debug("Keyboard detected");
    delete keyboard;
    keyboard = new WKeyboard(display, this);
  }
}
