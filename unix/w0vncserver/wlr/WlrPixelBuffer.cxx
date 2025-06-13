#include <assert.h>
#include <stdexcept>
#include <unistd.h>

#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>

#include <core/LogWriter.h>
#include <rfb/VNCServerST.h>

#include "WlrScreencopyManager.h"

#include "../w0vncserver.h"
#include "../wayland/WOutput.h"
#include "../wayland/WDisplay.h"
#include "WlrPixelBuffer.h"

static core::LogWriter vlog("WlrPixelBuffer");

WlrPixelBuffer::WlrPixelBuffer(WDisplay* display, WOutput* output_,
                               rfb::VNCServer* server_,
                               std::function<void()> desktopReadyCallback_)
  : WlrScreencopyManager(display, output_), captureInProgress(false),
    firstFrame(true), desktopReadyCallback(desktopReadyCallback_),
    server(server_)
{
  captureFrame();
}

WlrPixelBuffer::~WlrPixelBuffer()
{
  if (captureInProgress)
    server->unblockUpdates();
}

void WlrPixelBuffer::captureFrame()
{
  // We're too slow, skip this frame
  if (captureInProgress) {
    // FIXME: We're likely to make two calls to startFrameCapture()
    // for our first frame. Let's filter out the message the first time
    // as it is annoying.
    if (!firstFrame)
      vlog.debug("Frame capture already in progress, skipping frame");
    return;
  }

  if (!firstFrame)
    server->blockUpdates();

  // We call writeUpdate() in VNCServerST::startDesktop(), which is
  // where this pixelbuffer is initialized. We can only block updates
  // after a writeUpdate() call has been made, not before one.
  // FIXME: This is a bit ugly, rework this?
  captureInProgress = true;

  WlrScreencopyManager::captureFrame();
}

void WlrPixelBuffer::captureFrameDone()
{
  assert(captureInProgress);

  captureInProgress = false;

  // We need to capture our first frame before we know which format
  // the display is using.
  if (firstFrame) {
    try {
      format = getPixelFormat();
    } catch (std::runtime_error& e) {
      fatal_error("Failed to get pixel format: %s", e.what());
      return;
    }

    setBuffer(output->getWidth(), output->getHeight(), getBufferData(),
              output->getWidth());

    desktopReadyCallback();
  } else {
    server->unblockUpdates();
  }

  firstFrame = false;

  WlrScreencopyManager::captureFrameDone();

  server->add_changed({{0, 0, width(), height()}});
}
