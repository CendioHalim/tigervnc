#ifndef __W_SHM_H__
#define __W_SHM_H__

#include <wayland-client-protocol.h>

#include "WObject.h"

class WDisplay;

class WShm : public WObject {
public:
  WShm(WDisplay* display);
  ~WShm();

  wl_shm* getShm() { return shm; }

private:
  static const wl_shm_listener listener;
  wl_shm* shm;

};

#endif // __W_SHM_H__