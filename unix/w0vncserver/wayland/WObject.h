#ifndef __W_OBJECT_H__
#define __W_OBJECT_H__

#include <stdint.h>

struct wl_interface;
struct wl_proxy;
class WDisplay;

class WObject {
protected:
  WObject(WDisplay* display, const char* name,
          const wl_interface* interface);
public:
  virtual ~WObject() {};
protected:
  wl_proxy* boundObject;
};

#endif // __W_OBJECT_H__
