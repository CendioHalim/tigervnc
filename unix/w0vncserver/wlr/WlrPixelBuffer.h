#ifndef __WLR_PIXELBUFFER_H__
#define __WLR_PIXELBUFFER_H__

#include <functional>

#include <rfb/PixelBuffer.h>

#include "WlrScreencopyManager.h"

namespace rfb { class VNCServer; }

struct wl_buffer;
struct BufferInfo;
class WOutput;
class WDisplay;
class WShm;
class WShmPool;

class WlrPixelBuffer : public rfb::FullFramePixelBuffer,
                       public WlrScreencopyManager {
public:
  WlrPixelBuffer(WDisplay* display, WOutput* output,
                 rfb::VNCServer* server,
                 std::function<void()> desktopReadyCallback);
  ~WlrPixelBuffer();

  virtual void captureFrame() override;

protected:
  virtual void captureFrameDone() override;

private:
  bool captureInProgress;
  bool firstFrame;
  std::function<void()> desktopReadyCallback;
  rfb::VNCServer* server;
};
#endif // __WLR_PIXELBUFFER_H__
