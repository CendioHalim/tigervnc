#include "WDisplay.h"
#include <wayland-client-protocol.h>
#include "WShm.h"

WShm::WShm(WDisplay* display)
  : WObject(display, "wl_shm", &wl_shm_interface), shm(nullptr)
{
  shm = (wl_shm*) boundObject;
}

WShm::~WShm()
{
  wl_shm_destroy(shm);
}