#include <assert.h>

#include <wayland-client-protocol.h>

#include <core/LogWriter.h>

#include "../w0vncserver.h"
#include "WDisplay.h"
#include "WObject.h"
#include "WOutput.h"

const wl_output_listener WOutput::listener = {
  .geometry = [](void* data, wl_output* /* output */, int32_t x,
                 int32_t y, int32_t physicalWidth,
                 int32_t physicalHeight, int32_t subpixel,
                 const char* make, const char* model,
                 int32_t transform) {
    ((WOutput*)data)->handleGeometry(x, y, physicalWidth, physicalHeight,
                                     subpixel, make, model, transform);
  },
  .mode = [](void* data, wl_output* /* output */, uint32_t flags,
             int32_t width, int32_t height, int32_t refresh) {
    ((WOutput*)data)->handleMode(flags, width, height, refresh);
  },
  .done = [](void* data, wl_output* /* output */) {
    ((WOutput*)data)->handleDone();
  },
  .scale = [](void* data, wl_output* /* output*/ , int32_t factor) {
    ((WOutput*)data)->handleScale(factor);
  },
  .name = [](void* data, wl_output* /* output */, const char* name) {
    ((WOutput*)data)->handleName(name);
  },
  .description = [](void* data, wl_output* /* output */,
                    const char* description) {
    ((WOutput*)data)->handleDescription(description);
  }
};

static core::LogWriter vlog("WOutput");

WOutput::WOutput(WDisplay* display)
  : WObject(display, "wl_output", &wl_output_interface),
    output(nullptr), mode({0, 0, 0, 0})
{
  output = (wl_output*) boundObject;

  wl_output_add_listener(output, &listener, this);
  display->roundtrip();
}

WOutput::~WOutput()
{
  wl_output_destroy(output);
}

void WOutput::resizeComplete()
{
  assert(resized);
  resized = false;
}


void WOutput::handleGeometry(int32_t /* x */, int32_t /* y */,
                             int32_t /* physical_width */,
                             int32_t /* physical_height */,
                             int32_t /* subpixel */,
                             const char* /* make */,
                             const char*  /* model */,
                             int32_t /* transform */)
{
  // FIXME: This gives us the position/size etc. of the output
  // (presumably a screen). Keep track of this when we want to
  // handle multiple monitors.
}

void WOutput::handleMode(uint32_t flags, int32_t width, int32_t height,
                         int32_t refresh)
{
  Mode mode_;

  // FIXME: Handle multiple screens
  // FIXME: The flags "describe properties of an output mode".
  //        possible values are 0x1 - current
  //                            0x2 - preferred
  //        Don't think we care?
  mode_ = {
    .flags = flags,
    .width = width,
    .height = height,
    .refresh = refresh,
  };

  if (mode.width) {
    resized = true;
    vlog.debug("Resized from %dx%d to %dx%d", mode.width, mode.height,
               mode_.width, mode_.height);
  }

  mode = mode_;
}

void WOutput::handleDone()
{
  // All properties have been sent.
}

void WOutput::handleScale(int32_t /* factor */)
{
  // FIXME: Handle scaling
}

void WOutput::handleName(const char* /* name */)
{
  // Output name, e.g. "HDMI-1", "WL-1" etc.
}

void WOutput::handleDescription(const char* /* description */)
{
  // Optional description, e.g. "Dell inc. 24"
}
