#include <assert.h>

#include <wayland-client-core.h>
#include <wayland-client-protocol.h>

#include <core/LogWriter.h>

#include "../w0vncserver.h"
#include "WObject.h"
#include "WDisplay.h"

static core::LogWriter vlog("WDisplay");

const wl_registry_listener WDisplay::listener = {
  .global = [](void* data, wl_registry* /* registry */, uint32_t name,
               const char* interface, uint32_t version) {
    ((WDisplay*)data)->handleGlobalRegistry(name, interface, version);
  },
  .global_remove = [](void* data, wl_registry* /* registry */,
                      uint32_t name) {
    ((WDisplay*)data)->handleGlobalRemove(name);
  }
};

WDisplay::WDisplay(const char* name)
  : display(nullptr), registry(nullptr)
{
  display = wl_display_connect(name);
  if (!display)
    fatal_error("Failed to connect to wayland display");

  registry = wl_display_get_registry(display);
  if (!registry)
    fatal_error("Failed to get registry");

  wl_registry_add_listener(registry, &listener, this);
  wl_display_roundtrip(display);
}

WDisplay::~WDisplay()
{
  wl_display_flush(display);
  wl_registry_destroy(registry);
  wl_display_disconnect(display);
}

bool WDisplay::interfaceAvailable(const char* interface)
{
  return objects.find(interface) != objects.end();
}

void WDisplay::roundtrip()
{
  wl_display_roundtrip(display);
}

WObjectInfo* WDisplay::getObjectInfo(const char* interface)
{
  return &objects[interface];
}

void WDisplay::handleGlobalRegistry(uint32_t name,
                                    const char* interface,
                                    uint32_t version)
{
  objects[interface] = { name, version };
}

void WDisplay::handleGlobalRemove(uint32_t name)
{
  vlog.debug("Removing global: %d", name);

  for (auto it = objects.begin(); it != objects.end(); ++it) {
    if (it->second.name == name) {
      objects.erase(it);
      break;
    }
  }
}
