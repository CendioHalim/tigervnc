#include <stdexcept>

#include "xdg-output-unstable-v1.h"

#include "WDisplay.h"
#include "WOutput.h"
#include "WXdgOutputManager.h"


const zxdg_output_v1_listener WXdgOutputManager::listener = {
  .logical_position = [](void* data, zxdg_output_v1* /* xdg_output */,
                         int32_t x, int32_t y) {
    ((WXdgOutputManager*)data)->handleLogicalPosition(x, y);
  },
  .logical_size = [](void* data, zxdg_output_v1* /* xdg_output */,
                     int32_t width, int32_t height) {
    ((WXdgOutputManager*)data)->handleLogicalSize(width, height);
  },
  .done = [](void* data, zxdg_output_v1* /* xdg_output */) {
    ((WXdgOutputManager*)data)->handleDone();
  },
  .name = [](void* data, zxdg_output_v1* /* xdg_output */,
             const char* name) {
    ((WXdgOutputManager*)data)->handleName(name);
  },
  .description = [](void* data, zxdg_output_v1* /* xdg_output */,
                    const char* description) {
    ((WXdgOutputManager*)data)->handleDescription(description);
  }
};

WXdgOutputManager::WXdgOutputManager(WDisplay* display, WOutput* output)
  : WObject(display, "zxdg_output_manager_v1",
            &zxdg_output_manager_v1_interface), manager(nullptr)
{
  manager = (zxdg_output_manager_v1*) boundObject;
  xdgOutput = zxdg_output_manager_v1_get_xdg_output(manager,
                                                    output->getOutput());

  if (!xdgOutput)
    throw std::runtime_error("Failed to create xdg_output");

  zxdg_output_v1_add_listener(xdgOutput, &listener, this);
  display->roundtrip();
}

WXdgOutputManager::~WXdgOutputManager()
{
  if (xdgOutput)
    zxdg_output_v1_destroy(xdgOutput);
  if (manager)
    zxdg_output_manager_v1_destroy(manager);
}

void WXdgOutputManager::handleLogicalPosition(int32_t /* x */, int32_t /* y */)
{
}

void WXdgOutputManager::handleLogicalSize(int32_t width, int32_t height)
{
  logicalWidth = width;
  logicalHeight = height;
}

void WXdgOutputManager::handleDone()
{
}

void WXdgOutputManager::handleName(const char* /* name */)
{
}

void WXdgOutputManager::handleDescription(const char* /* description */)
{
}