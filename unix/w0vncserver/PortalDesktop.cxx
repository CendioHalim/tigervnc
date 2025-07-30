/* Copyright 2025 Adam Halim for Cendio AB
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <exception>

#include <gio/gio.h>

#include <rfb/ScreenSet.h>
#include <rfb/VNCServerST.h>
#include <core/LogWriter.h>

#include "w0vncserver.h"
#include "portals/RemoteDesktop.h"
#include "portals/PortalProxy.h"
#include "wayland/WDisplay.h"
#include "wayland/WOutput.h"
#include "wayland/WXdgOutputManager.h"
#include "wayland/GWaylandSource.h"
#include "PipeWirePixelBuffer.h"
#include "PortalDesktop.h"

static core::LogWriter vlog("PortalDesktop");

PortalDesktop::PortalDesktop(GMainLoop* loop_)
  : server(nullptr), remoteDesktop(nullptr), pb(nullptr),
    display(nullptr), output(nullptr), waylandSource(nullptr),
    loop(loop_), xdgOutputManager(nullptr)
{
  display = new WDisplay();
  output = new WOutput(display);
  xdgOutputManager = new WXdgOutputManager(display, output);
}

PortalDesktop::~PortalDesktop()
{
  delete remoteDesktop;
  delete waylandSource;
  delete xdgOutputManager;
  delete output;
  delete display;
  delete pb;
}

void PortalDesktop::init(rfb::VNCServer* vs)
{
  server = vs;
}

void PortalDesktop::start()
{
  assert(!remoteDesktop);

  std::function<void(int, uint32_t)> cb = [this](int fd, uint32_t id) {
    try {
      pb = new PipeWirePixelBuffer(fd, id, server);
      waylandSource = new GWaylandSource(display);
      waylandSource->attach(g_main_loop_get_context(loop));
    } catch (std::exception& e) {
      fatal_error("error initializing PipeWirePixelBuffer: %s", e.what());
    }
  };

  remoteDesktop = new RemoteDesktop(cb);
  remoteDesktop->createSession();
}

void PortalDesktop::stop()
{
  server->setPixelBuffer(nullptr);

  delete pb;
  pb = nullptr;

  delete remoteDesktop;
  remoteDesktop = nullptr;

  delete waylandSource;
  waylandSource = nullptr;
}

void PortalDesktop::queryConnection(network::Socket* sock,
                                    const char* /* userName */)
{
  // FIXME: Implement this.
  server->approveConnection(sock, false,
                            "Unable to query the local user to accept the connection.");
}

void PortalDesktop::terminate()
{
  kill(getpid(), SIGTERM);
}

unsigned int PortalDesktop::setScreenLayout(int /* fb_width */,
                                            int /* fb_height */,
                                            const rfb::ScreenSet& /*  layout */)
{
  return rfb::resultProhibited;
}

void PortalDesktop::keyEvent(uint32_t keysym, uint32_t keycode, bool down)
{
  remoteDesktop->keyEvent(keysym, keycode, down);
}

void PortalDesktop::pointerEvent(const core::Point& pos,
                            uint16_t buttonMask)
{
  double xScaleFactor = (double)pb->width() / xdgOutputManager->getLogicalWidth();
  double yScaleFactor = (double)pb->height() / xdgOutputManager->getLogicalHeight();

  remoteDesktop->pointerEvent(pos.x / xScaleFactor, pos.y / yScaleFactor, buttonMask);
}

bool PortalDesktop::available()
{
  std::vector<std::string> interfaces = {
    "org.freedesktop.portal.RemoteDesktop",
    "org.freedesktop.portal.ScreenCast",
  };

  return PortalProxy::interfacesAvailable(interfaces);
}
