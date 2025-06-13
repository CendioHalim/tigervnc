#include <assert.h>
#include <unistd.h>

#include <stdexcept>

#include <core/LogWriter.h>

#include <core/string.h>
#include <rfb/PixelFormat.h>

#include "wlr-screencopy-unstable-v1.h"

#include "../shm.h"
#include "../w0vncserver.h"
#include "../wayland/WDisplay.h"
#include "../wayland/WObject.h"
#include "../wayland/WOutput.h"
#include "../wayland/WShm.h"
#include "../wayland/WShmPool.h"
#include "WlrScreencopyManager.h"

static core::LogWriter vlog("WlrScreencopyManager");

const zwlr_screencopy_frame_v1_listener WlrScreencopyManager::listener = {
  .buffer = [](void* data, zwlr_screencopy_frame_v1* /* frame */,
               uint32_t format, uint32_t width, uint32_t height,
               uint32_t stride) {
    ((WlrScreencopyManager*)data)->handleScreencopyBuffer(format, width, height, stride);
  },
  .flags = [](void* data, zwlr_screencopy_frame_v1* /* frame */, uint32_t flags) {
    ((WlrScreencopyManager*)data)->handleScreencopyFlags(flags);
  },
  .ready = [](void* data, zwlr_screencopy_frame_v1* /* frame */,
              uint32_t /* tvSecHi */, uint32_t /* tvSecLo */,
              uint32_t /* tvNsec */) {
    ((WlrScreencopyManager*)data)->handleScreencopyReady();
  },
  .failed = [](void* data, zwlr_screencopy_frame_v1* /* frame */) {
     ((WlrScreencopyManager*)data)->handleScreencopyFailed();
  },
  .damage = [](void* data, zwlr_screencopy_frame_v1* /* frame */,
                uint32_t x, uint32_t y, uint32_t width,
                uint32_t height) {
    ((WlrScreencopyManager*)data)->handleScreencopyDamage(x, y, width, height);
  },
  .linux_dmabuf = [](void* data, zwlr_screencopy_frame_v1* /* frame */,
                      uint32_t format, uint32_t width,
                      uint32_t height) {
    ((WlrScreencopyManager*)data)->handleScreencopyLinuxDmabuf(format, width, height);
  },
  .buffer_done = [](void* data, zwlr_screencopy_frame_v1* /* frame */) {
    ((WlrScreencopyManager*)data)->handleScreencopyBufferDone();
  }
};

struct BufferInfo {
  uint32_t format;
  uint32_t width;
  uint32_t height;
  uint32_t stride;
};

WlrScreencopyManager::WlrScreencopyManager(WDisplay* display,
                                           WOutput* output_)
 : WObject(display, "zwlr_screencopy_manager_v1",
   &zwlr_screencopy_manager_v1_interface),
   output(output_), screencopyManager(nullptr), frame(nullptr),
   info(nullptr), shm(nullptr), pool(nullptr), buffer(nullptr)
{
  size_t size;

  screencopyManager = (zwlr_screencopy_manager_v1*) boundObject;

  display->roundtrip();

  shm = new WShm(display);

  size = output_->getWidth() * output_->getHeight() * 4;
  assert(size);

  initBuffers(size);
}

WlrScreencopyManager::~WlrScreencopyManager()
{
  if (screencopyManager)
    zwlr_screencopy_manager_v1_destroy(screencopyManager);
  if (frame)
    zwlr_screencopy_frame_v1_destroy(frame);
  if (buffer)
    wl_buffer_destroy(buffer);

  delete info;
  delete shm;
  delete pool;
}

uint8_t* WlrScreencopyManager::getBufferData()
{
  return pool->getData();
}

void WlrScreencopyManager::captureFrame()
{
  assert(frame == nullptr);

  // FIXME: Handle multiple outputs
  frame = zwlr_screencopy_manager_v1_capture_output(screencopyManager,
                                                    1, output->getOutput());
  zwlr_screencopy_frame_v1_add_listener(frame, &listener, this);
}

void WlrScreencopyManager::captureFrameDone()
{
  assert(frame != nullptr);

  zwlr_screencopy_frame_v1_destroy(frame);
  frame = nullptr;
}

void WlrScreencopyManager::initBuffers(size_t size)
{
  int fd;

  fd = allocate_shm_file(size);
  if (fd < 0) {
    fatal_error("Failed to allocate shm");
    return;
  }

  pool = new WShmPool(shm, fd, size);
  if (!pool) {
    fatal_error("Failed to create shm pool");
    return;
  }

  close(fd);
}

rfb::PixelFormat WlrScreencopyManager::convertPixelformat(uint32_t format)
{
  vlog.debug("Format %d", format);
  switch (format) {
    case WL_SHM_FORMAT_XRGB8888:
    case WL_SHM_FORMAT_ARGB8888:
      return rfb::PixelFormat(32, 24, false, true, 255, 255, 255,
                              16, 8, 0);
    case WL_SHM_FORMAT_RGBX8888:
    case WL_SHM_FORMAT_RGBA8888:
      return rfb::PixelFormat(32, 24, false, true, 255, 255, 255,
                              24, 16, 8);
    case WL_SHM_FORMAT_XBGR8888:
    case WL_SHM_FORMAT_ABGR8888:
      return rfb::PixelFormat(32, 24, false, true, 255, 255, 255,
                              0, 8, 16);
    default:
      throw std::runtime_error(core::format("format %d not supported",
                                            format));
  }
}

void WlrScreencopyManager::handleScreencopyBuffer(uint32_t format,
                                                  uint32_t width,
                                                  uint32_t height,
                                                  uint32_t stride)
{
  delete info;

  info = new BufferInfo {
    .format = format,
    .width = width,
    .height = height,
    .stride = stride
  };
}

void WlrScreencopyManager::handleScreencopyFlags(uint32_t /* flags */)
{
  // FIXME: This tells us if the contents are y-inverted.
  //        Should probably handle this.
}

void WlrScreencopyManager::handleScreencopyReady()
{
  captureFrameDone();
}

rfb::PixelFormat WlrScreencopyManager::getPixelFormat()
{
  assert(info);

  rfb::PixelFormat pf;

  try {
    pf = convertPixelformat(info->format);
  } catch (std::runtime_error& e) {
    throw;
  }

  return pf;
}

void WlrScreencopyManager::handleScreencopyFailed()
{
  zwlr_screencopy_frame_v1_destroy(frame);
  frame = nullptr;
  fatal_error("Frame could not be copied");
}

void WlrScreencopyManager::handleScreencopyDamage(uint32_t /* x */,
                                                  uint32_t /* y */,
                                                  uint32_t /* width */,
                                                  uint32_t /* height */)
{
  // FIXME: Implement damage.
}

void WlrScreencopyManager::handleScreencopyLinuxDmabuf(uint32_t /* format */,
                                                       uint32_t /* width */,
                                                       uint32_t /* height */)
{
  // FIXME: Implement this?
}

void WlrScreencopyManager::handleScreencopyBufferDone()
{
  if (!buffer) {
    // FIXME: Handle more formats, and don't hardcode RGBx
    // FIXME: Check if buffer paramters have changed
    // FIXME: Sanity check with BufferInfo
    buffer = pool->createBuffer(0, output->getWidth(), output->getHeight(),
                                output->getWidth() * 4, info->format);
  }

  zwlr_screencopy_frame_v1_copy(frame, buffer);
}