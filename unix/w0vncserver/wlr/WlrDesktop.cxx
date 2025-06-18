#include <assert.h>
#include <sys/mman.h>

#include <glib.h>
#include <wayland-client.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>

#include <core/LogWriter.h>
#include <rfb/VNCServerST.h>

#include "../w0vncserver.h"
#include "../wayland/GWaylandSource.h"
#include "../wayland/WDisplay.h"
#include "../wayland/WOutput.h"
#include "../wayland/WShm.h"
#include "../wayland/WSeat.h"
#include "../wayland/WDisplay.h"
#include "WlrVirtualKeyboard.h"
#include "WlrVirtualPointer.h"
#include "WlrPixelBuffer.h"
#include "WlrDesktop.h"

static core::LogWriter vlog("WlrDesktop");

bool WlrDesktop::available()
{
  WDisplay display;

  return display.interfaceAvailable("zwlr_screencopy_manager_v1") &&
         display.interfaceAvailable("zwlr_virtual_pointer_manager_v1");
}

WlrDesktop::WlrDesktop(GMainLoop* loop_)
  : server(nullptr), loop(loop_), display(nullptr),
    seat(nullptr), pb(nullptr), wlrSource(nullptr)
{
  assert(available());

  display = new WDisplay();
  if (!display)
    fatal_error("Failed to connect to wayland display");

  output = new WOutput(display);
  seat = new WSeat(display);
}

WlrDesktop::~WlrDesktop()
{
  delete pb;
  delete wlrSource;
  delete virtualPointer;
  delete virtualKeyboard;
  delete seat;
  delete output;
  delete display;
}

void WlrDesktop::init(rfb::VNCServer* vs)
{
  server = vs;
}

void WlrDesktop::start()
{
  std::function<void()> cb = [this]() {
    server->setPixelBuffer(pb);
    virtualPointer = new WlrVirtualPointer(display, seat, pb->width(),
                                           pb->height());
    virtualKeyboard = new WlrVirtualKeyboard(display, seat);
  };

  pb = new WlrPixelBuffer(display, output, server, cb);

  wlrSource = new GWaylandSource(display);
  wlrSource->attach(g_main_loop_get_context(loop));
}

void WlrDesktop::stop()
{
  server->setPixelBuffer(nullptr);

  delete virtualKeyboard;
  virtualKeyboard = nullptr;

  delete wlrSource;
  wlrSource = nullptr;

  delete virtualPointer;
  virtualPointer = nullptr;

  delete pb;
  pb = nullptr;
}

void WlrDesktop::frameTick(uint64_t /* msc */)
{
  // FIXME: Should we use the monitor refresh rate instead?
  pb->captureFrame();
}

void WlrDesktop::pointerEvent(const core::Point& pos,
                              uint16_t buttonMask)
{
  virtualPointer->pointerEvent(pos, buttonMask);
}

void WlrDesktop::keyEvent(uint32_t keysym, uint32_t keycode, bool down)
{
  virtualKeyboard->keyEvent(keysym, keycode, down);
}

void WlrDesktop::queryConnection(network::Socket* sock,
                                 const char* /* userName */)
{
  // FIXME: Implement this.
  server->approveConnection(sock, false,
                            "Unable to query the local user to accept the connection.");
}

void WlrDesktop::terminate()
{
  kill(getpid(), SIGTERM);
}
