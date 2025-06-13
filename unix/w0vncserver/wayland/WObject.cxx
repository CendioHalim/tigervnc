#include <assert.h>

#include <wayland-client-core.h>
#include <wayland-client-protocol.h>

#include "../w0vncserver.h"
#include "WDisplay.h"
#include "WObject.h"


WObject::WObject(WDisplay* display, const char* interfaceName,
                 const wl_interface* interface)
  : boundObject(nullptr)
{
  wl_registry* registry;
  WObjectInfo* objectInfo;

  assert(display->interfaceAvailable(interfaceName));

  objectInfo = display->getObjectInfo(interfaceName);
  registry = display->getRegistry();

  boundObject = (wl_proxy*)wl_registry_bind(registry, objectInfo->name,
                                            interface,
                                            objectInfo->version);

  if (!boundObject) {
    fatal_error("Failed to bind to %s ", interfaceName);
    return;
  }
}
