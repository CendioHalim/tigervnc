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

#include "rfb/screenTypes.h"
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <limits.h>
#include <stdlib.h>
#include <memory>
#include <algorithm>

#include <glib.h>
#include <wayland-client.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>

#include <core/LogWriter.h>
#include <rfb/VNCServerST.h>

#include "../w0vncserver.h"
#include "../parameters.h"
#include "objects/Display.h"
#include "objects/DataControl.h"
#include "objects/Output.h"
#include "objects/Seat.h"
#include "objects/VirtualPointer.h"
#include "objects/VirtualKeyboard.h"
#include "objects/OutputManagement.h"
#include "GWaylandSource.h"
#include "WaylandPixelBuffer.h"
#include "WaylandDesktop.h"

static core::LogWriter vlog("WaylandDesktop");

#define BUTTONS 9

WaylandDesktop::WaylandDesktop(GMainLoop* loop_)
  : server(nullptr), pb(nullptr), loop(loop_), waylandSource(nullptr),
    display(nullptr), seat(nullptr), virtualPointer(nullptr),
    virtualKeyboard(nullptr), dataControl(nullptr), outputManager(nullptr)
{
  assert(available());

  context = g_main_loop_get_context(loop_);

  display = new wayland::Display();
  output = new wayland::Output(display);
  seat = new wayland::Seat(display, std::bind(&WaylandDesktop::setLEDState,
                                              this, std::placeholders::_1));
}

WaylandDesktop::~WaylandDesktop()
{
  delete pb;
  delete waylandSource;
  delete virtualPointer;
  delete virtualKeyboard;
  delete outputManager;
  delete seat;
  delete output;
  delete display;
}

void WaylandDesktop::init(rfb::VNCServer* vs)
{
  server = vs;
}

void WaylandDesktop::start()
{
  std::function<void()> desktopReadyCb = [this]() {
    try {
      virtualPointer = new wayland::VirtualPointer(display, seat);
    } catch (std::exception& e) {
      vlog.error("%s - pointer will be disabled", e.what());
    }
    try {
      virtualKeyboard = new wayland::VirtualKeyboard(display, seat);
    } catch (std::exception& e) {
      vlog.error("%s - keyboard will be disabled", e.what());
    }

    if (display->interfaceAvailable("ext_data_control_manager_v1")) {
      std::function<void(bool available)> clipboardAnnounceCb = [this](bool available) {
        server->announceClipboard(available);
      };
      std::function<void(const char* data)> sendClipboardData = [this](const char* data) {
        server->sendClipboardData(data);
      };
      std::function<void()> clipboardRequestCb = [this]() {
        server->requestClipboard();
      };

      dataControl = new wayland::DataControl(display, seat,
                                             clipboardAnnounceCb,
                                             clipboardRequestCb,
                                             sendClipboardData);
    } else {
      vlog.info("ext-data-control-v1 not available, Clipboard disabled");
    }
    if (display->interfaceAvailable("zwlr_output_manager_v1")) {
      outputManager = new wayland::OutputManager(display);
    } else {
      vlog.info("zwlr-output-manager-v1 not available, resizing will be disabled");
    }

    server->setPixelBuffer(pb);
    server->setLEDState(virtualKeyboard->getLEDState());
  };

  try {
    pb = new WaylandPixelBuffer(display, output, server, desktopReadyCb);
  } catch (std::exception& e) {
    vlog.error("Error initializing pixel buffer: %s", e.what());
    server->closeClients("Failed to start remote desktop session");
  }

  waylandSource = new GWaylandSource(display);
  waylandSource->attach(g_main_loop_get_context(loop));
}

void WaylandDesktop::stop()
{
  server->setPixelBuffer(nullptr);

  delete virtualKeyboard;
  virtualKeyboard = nullptr;

  delete waylandSource;
  waylandSource = nullptr;

  delete virtualPointer;
  virtualPointer = nullptr;

  delete pb;
  pb = nullptr;

  delete dataControl;
  dataControl = nullptr;
}

void WaylandDesktop::pointerEvent(const core::Point& pos, uint16_t buttonMask)
{
  if (!virtualPointer)
    return;

  virtualPointer->motionAbsolute(pos.x, pos.y, pb->width(), pb->height());

  if (buttonMask == oldButtonMask)
    return;

  for (int32_t i = 0; i < BUTTONS; i++) {
    if ((buttonMask ^ oldButtonMask) & (1 << i)) {
      if (i > 2 && i < 7)
        virtualPointer->axisDiscrete(i);
      else
        virtualPointer->button(i, buttonMask & (1 << i));
    }
  }

  oldButtonMask = buttonMask;
}

void WaylandDesktop::startScreenLayoutAsync(int fb_width, int fb_height,
                                            const rfb::ScreenSet& layout)
{
  wayland::OutputConfiguration* config;
  wayland::OutputConfigurationHead* headConfig;
  bool snapped;

  vlog.debug("@ Wayland setScreenLayout request %dx%d, screens=%d",
             fb_width, fb_height, layout.num_screens());

  if (!display->interfaceAvailable("zwlr_output_manager_v1")) {
    vlog.error("zwlr-output-manager-v1 not available");
    server->setScreenLayoutDone(rfb::resultNoResources);
    return;
  }

  if (pb && pb->width() == fb_width && pb->height() == fb_height) {
    server->setScreenLayoutDone((rfb::resultSuccess));
    return;
  }

  if (layout.num_screens() != 1) {
    vlog.error("only single screen layout supported");
    server->setScreenLayoutDone(rfb::resultNoResources);
    return;
  }

  if (!outputManager->isReady()) {
    // FIXME: Not sure if this should be needed
    return;
  }

  const std::vector<wayland::OutputHead*>& heads = outputManager->getHeads();
  vlog.debug("got %zu heads", heads.size());
  if (heads.empty()) {
    server->setScreenLayoutDone(rfb::resultNoResources);
    return;
  }

  vlog.debug("create configuration with serial=%u", outputManager->getLastSerial());
  // FIXME: Match output with name/description instead of getting last serial
  try {
    config = outputManager->createConfiguration(outputManager->getLastSerial());
  } catch (std::exception& e) {
    vlog.error("Failed to resize desktop: %s", e.what());
    server->setScreenLayoutDone(rfb::resultNoResources);
    return;
  }

  vlog.debug("enable primary head");
  try {
    headConfig = config->enableHead(heads.front());
  } catch (std::exception& e) {
    delete config;
    vlog.error("Failed to resize desktop: %s", e.what());
    server->setScreenLayoutDone(rfb::resultNoResources);
    return;
  }

  const rfb::Screen& screen = *layout.begin();
  vlog.debug("set head pos %d,%d size %dx%d",
             screen.dimensions.tl.x, screen.dimensions.tl.y,
             screen.dimensions.width(), screen.dimensions.height());
  headConfig->setPosition(screen.dimensions.tl.x, screen.dimensions.tl.y);

  const std::vector<wayland::OutputMode*>& modes = heads.front()->getModes();
  const int32_t reqWidth = screen.dimensions.width();
  const int32_t reqHeight = screen.dimensions.height();
  wayland::OutputMode* bestMode = nullptr;
  bool exactMatch = false;

  for (wayland::OutputMode* mode : modes) {
    if (mode->getWidth() == reqWidth && mode->getHeight() == reqHeight) {
      if (!bestMode || mode->isPreferred() ||
          mode->getRefresh() > bestMode->getRefresh())
        bestMode = mode;
      exactMatch = true;
    }
  }

  if (!bestMode) {
    wayland::OutputMode* smallestMode = nullptr;
    int64_t smallestArea = INT64_MAX;
    int64_t bestArea = -1;

    for (wayland::OutputMode* mode : modes) {
      const int32_t w = mode->getWidth();
      const int32_t h = mode->getHeight();
      const int64_t area = static_cast<int64_t>(w) * h;

      if (area < smallestArea ||
          (area == smallestArea && mode->isPreferred())) {
        smallestArea = area;
        smallestMode = mode;
      }

      if (w <= reqWidth && h <= reqHeight) {
        if (area > bestArea ||
            (area == bestArea && mode->isPreferred())) {
          bestArea = area;
          bestMode = mode;
        }
      }
    }

    if (!bestMode)
      bestMode = smallestMode;
  }

  if (bestMode) {
    if (exactMatch) {
      vlog.debug("using advertised mode %dx%d@%d",
                 bestMode->getWidth(), bestMode->getHeight(),
                 bestMode->getRefresh());
      snapped = false;
    } else {
      vlog.debug("snapping %dx%d -> %dx%d@%d",
                 reqWidth, reqHeight,
                 bestMode->getWidth(), bestMode->getHeight(),
                 bestMode->getRefresh());
      snapped = true;
    }
    headConfig->setMode(bestMode);
  } else {
    vlog.debug("no matching mode for %dx%d",
               reqWidth, reqHeight);
    delete config;
    server->setScreenLayoutDone(rfb::resultInvalid);
    return;
  }

  // FIXME: This entire callback should be moved to the outputManager.finished callback
  config->setCompletionCallback([this, snapped, config](wayland::OutputConfiguration::Status status) {
    unsigned int result = rfb::resultInvalid;
    if (status == wayland::OutputConfiguration::Succeeded) {
      result = snapped ? rfb::resultInvalid : rfb::resultSuccess;
    } else if (status == wayland::OutputConfiguration::Pending) {
      result = rfb::resultNoResources;
    } else {
      result = rfb::resultInvalid;
    }
    server->setScreenLayoutDone(result);
    delete config;
  });

  vlog.debug("apply configuration");
  config->apply();
  wl_display_flush(display->getDisplay());
}

void WaylandDesktop::keyEvent(uint32_t keysym, uint32_t keycode, bool down)
{
  if (!virtualKeyboard)
    return;

  virtualKeyboard->key(keysym, keycode, down);
}

void WaylandDesktop::queryConnection(network::Socket* sock,
                                     const char* /* userName */)
{
  // FIXME: Implement this.
  server->approveConnection(sock, false,
                            "Unable to query the local user to accept the connection.");
}

void WaylandDesktop::terminate()
{
  kill(getpid(), SIGTERM);
}

void WaylandDesktop::setScreenLayout(int fb_width, int fb_height,
                                      const rfb::ScreenSet& layout)
{
  if (!outputManager) {
    server->setScreenLayoutDone(rfb::resultProhibited);
    return;
  }

  if (g_main_context_is_owner(context)) {
    vlog.debug("called on main loop thread");
    startScreenLayoutAsync(fb_width, fb_height, layout);
    server->setScreenLayoutDone(rfb::resultNoResources);
  }
}

void WaylandDesktop::handleClipboardRequest()
{
  if (!dataControl)
    return;

  dataControl->receive();
}

void WaylandDesktop::handleClipboardAnnounce(bool available)
{
  if (!dataControl)
    return;

  if (available) {
    dataControl->setSelection();
    if (setPrimary)
      dataControl->setPrimarySelection();
  } else {
    dataControl->clearSelection();
    if (setPrimary)
      dataControl->clearPrimarySelection();
  }
}

void WaylandDesktop::handleClipboardData(const char* data)
{
  if (!dataControl)
    return;

  dataControl->writePending(data);
}

bool WaylandDesktop::available()
{
  wayland::Display display;

  return display.interfaceAvailable("zwlr_screencopy_manager_v1") &&
         display.interfaceAvailable("zwlr_virtual_pointer_manager_v1") &&
         display.interfaceAvailable("zwp_virtual_keyboard_manager_v1");
}

void WaylandDesktop::setLEDState(unsigned int state)
{
  if (server)
    server->setLEDState(state);
}
