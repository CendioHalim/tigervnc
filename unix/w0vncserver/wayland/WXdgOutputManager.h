#ifndef _W_XDG_OUTPUT_MANAGER_H_
#define _W_XDG_OUTPUT_MANAGER_H_

#include "WObject.h"

struct zxdg_output_manager_v1;
struct zxdg_output_v1;
struct zxdg_output_v1_listener;
class WOutput;

class WXdgOutputManager : public WObject {
public:
  WXdgOutputManager(WDisplay* display, WOutput* output);
  ~WXdgOutputManager();

  int32_t getLogicalWidth() const { return logicalWidth; }
  int32_t getLogicalHeight() const { return logicalHeight; }

private:
  void handleLogicalPosition(int32_t x, int32_t y);
  void handleLogicalSize(int32_t width, int32_t height);
  void handleDone();
  void handleName(const char* name);
  void handleDescription(const char* description);

private:
  int32_t logicalWidth;
  int32_t logicalHeight;
  zxdg_output_manager_v1* manager;
  zxdg_output_v1* xdgOutput;
  static const zxdg_output_v1_listener listener;
};


#endif // _W_XDG_OUTPUT_MANAGER_H_
