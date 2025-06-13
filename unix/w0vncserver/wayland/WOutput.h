#ifndef __W_OUTPUT_H__
#define __W_OUTPUT_H__

#include <string>

#include <wayland-client-protocol.h>

#include "WObject.h"

class WDisplay;
struct wl_output_listener;

struct Mode {
  uint32_t flags;
  int32_t width;
  int32_t height;
  int32_t refresh;
};

class WOutput : public WObject {
public:
  WOutput(WDisplay* display);
  ~WOutput();

  wl_output* getOutput() const { return output; }
  uint32_t getWidth() const { return mode.width; }
  uint32_t getHeight() const { return mode.height; }

private:
  void handleGeometry(int32_t x, int32_t y, int32_t physical_width,
                      int32_t physical_height, int32_t subpixel,
                      const char* make, const char* model,
                      int32_t transform);
  void handleMode(uint32_t flags, int32_t width, int32_t height,
                  int32_t refresh);
  void handleDone();
  void handleScale(int32_t factor);
  void handleName(const char* name);
  void handleDescription(const char* description);

private:
  wl_output* output;
  Mode mode;
  std::string name;
  std::string description;
  static const wl_output_listener listener;
};

#endif // __W_OUTPUT_H__