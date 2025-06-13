#ifndef __WLR_SCREENCOPY_H__
#define __WLR_SCREENCOPY_H__

#include "wlr-screencopy-unstable-v1.h"

#include "../wayland/WObject.h"

namespace rfb { class PixelFormat; }

class WDisplay;
class WShm;
class WShmPool;
class WOutput;
struct BufferInfo;

class WlrScreencopyManager : public WObject {
public:
  WlrScreencopyManager(WDisplay* display, WOutput* output);
  virtual ~WlrScreencopyManager();

  uint8_t* getBufferData();

  // Capture the next frame. This function is asynchronous.
  // Framebuffer data is available after handleScreencopyReady() has
  // been called.
  virtual void captureFrame() = 0;

protected:
  // Called when the buffer is safe to read from, the frame is ready.
  virtual void captureFrameDone() = 0;

  rfb::PixelFormat getPixelFormat();

private:
  void initBuffers(size_t size);
  rfb::PixelFormat convertPixelformat(uint32_t format);

  // zwlr_screencopy_frame_v1_listener handlers
  void handleScreencopyBuffer(uint32_t format, uint32_t width, uint32_t height,
                              uint32_t stride);
  void handleScreencopyFlags(uint32_t flags);
  void handleScreencopyReady();
  void handleScreencopyFailed();
  void handleScreencopyDamage(uint32_t x, uint32_t y, uint32_t width,
                              uint32_t height);
  void handleScreencopyLinuxDmabuf(uint32_t format, uint32_t width,
                                   uint32_t height);
  void handleScreencopyBufferDone();

protected:
  WOutput* output;

private:
  zwlr_screencopy_manager_v1* screencopyManager;
  zwlr_screencopy_frame_v1* frame;
  BufferInfo* info;
  WShm* shm;
  WShmPool* pool;
  wl_buffer* buffer;
  static const zwlr_screencopy_frame_v1_listener listener;
};

#endif // __WLR_SCREENCOPY_H__