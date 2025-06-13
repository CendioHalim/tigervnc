
#ifndef __W_DISPLAY_H__
#define __W_DISPLAY_H__

#include <stdint.h>

#include <string>
#include <map>

struct wl_display;
struct wl_registry;
struct wl_registry_listener;
struct wl_interface;
struct wl_proxy;
class WObject;

struct WObjectInfo {
  uint32_t name;
  uint32_t version;
};

class WDisplay {
public:
  WDisplay(const char* name = nullptr);
  ~WDisplay();

  bool interfaceAvailable(const char* interface);
  void roundtrip();

  wl_display* getDisplay() const { return display; }
  wl_registry* getRegistry() const { return registry; }
  WObjectInfo* getObjectInfo(const char* interface);

private:
  void handleGlobalRegistry(uint32_t name, const char *interface,
                            uint32_t version);
  void handleGlobalRemove(uint32_t name);

private:
  wl_display* display;
  wl_registry* registry;
  static const struct wl_registry_listener listener;
  std::map<std::string, WObjectInfo> objects;
};

#endif // __W_DISPLAY_H__