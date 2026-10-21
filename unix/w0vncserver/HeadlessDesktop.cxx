/* Copyright 2026 Adam Halim for Cendio AB
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

#include <stdexcept>

#include <gio/gio.h>
#include <glib.h>

#include <core/LogWriter.h>
#include <core/i18n.h>
#include <core/string.h>

#include <rfb/VNCServer.h>

#include "dbus/VirtualMonitor.h"
#include "dbus/RemoteDesktopManager.h"
#include "dbus/RemoteDesktopSession.h"
#include "pipewire/PipeWirePixelBuffer.h"
#include "EISClient.h"
#include "HeadlessDesktop.h"

#define BUTTONS 9

static core::LogWriter vlog("HeadlessDesktop");

HeadlessDesktop::HeadlessDesktop()
  : server(nullptr), manager(nullptr), session(nullptr),
    monitor(nullptr), pb(nullptr), eisClient(nullptr), oldButtonMask(0)
{
}

HeadlessDesktop::~HeadlessDesktop()
{
  delete pb;

  // FIXME: Wait until org.freedesktop.RemoteDesktop1.Session::Removed signal...
  delete monitor;
  delete session;
  delete manager;
}

void HeadlessDesktop::init(rfb::VNCServer* vs)
{
  server = vs;
}

void HeadlessDesktop::start()
{
  std::string sessionPath;
  std::string monitorPath;
  int eisFd;
  dbus::PipeWireInfo pwInfo;

  manager = new dbus::RemoteDesktopManager();
  sessionPath = manager->createSession();
  session = new dbus::RemoteDesktopSession(sessionPath);
  monitorPath = session->createVirtualMonitor("VNC-0");
  // FIXME: Handle multiple monitors
  monitor = new dbus::VirtualMonitor(monitorPath);

  eisFd = session->connectToEIS();
  if (!eisFd)
      vlog.error("Failed to get EIS FD, input disabled.");
  else
    eisClient = new EISClient(eisFd, [this](unsigned int state) {
      server->setLEDState(state);
    });

  pwInfo = monitor->openPipeWireStream();

  pb = new PipeWirePixelBuffer(pwInfo.nodeId, pwInfo.serial, server);
}

void HeadlessDesktop::stop()
{
  server->setPixelBuffer(nullptr);

  delete pb;
  pb = nullptr;

  delete eisClient;
  eisClient = nullptr;

  delete monitor;
  monitor = nullptr;
  delete session;
  session = nullptr;
}

void HeadlessDesktop::queryConnection(rfb::SConnection* conn)
{
  // FIXME: Implement this.
  server->approveConnection(conn, false,
                            "Unable to query the local user to accept the connection.");
}

void HeadlessDesktop::terminate()
{
  kill(getpid(), SIGTERM);
}

unsigned int HeadlessDesktop::setScreenLayout(int /* fb_width */,
                                              int /* fb_height */,
                                              const rfb::ScreenSet& /*  layout */)
{
  return rfb::resultProhibited;
}

void HeadlessDesktop::keyEvent(uint32_t keysym, uint32_t keycode,
                               bool down)
{
  if (!eisClient)
    return;

  eisClient->keyboardKey(keysym, keycode, down);
}

void HeadlessDesktop::pointerEvent(const core::Point& pos,
                                   uint16_t buttonMask)
{
  if (!eisClient)
    return;

  eisClient->pointerMotionAbsolute(pos.x, pos.y);

  if (buttonMask == oldButtonMask)
    return;

  for (int32_t i = 0; i < BUTTONS; i++) {
    if ((buttonMask ^ oldButtonMask) & (1 << i)) {
      if (i > 2 && i < 7)
        eisClient->pointerScroll(i);
      else
        eisClient->pointerButton(i, buttonMask & (1 << i));
    }
  }

  oldButtonMask = buttonMask;
}

bool HeadlessDesktop::available()
{
  GVariant* result;
  GDBusConnection* connection;
  GError* error = nullptr;
  int hasOwner;

  connection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);

  if (error) {
    std::string error_message =
      core::format(_("Failed to connect to D-Bus: %s"), error->message);
    g_error_free(error);
    throw std::runtime_error(error_message);
  }

  result = g_dbus_connection_call_sync(connection,
                                       "org.freedesktop.DBus",
                                       "/org/freedesktop/DBus",
                                       "org.freedesktop.DBus",
                                       "NameHasOwner",
                                       g_variant_new("(s)", "org.freedesktop.RemoteDesktop1"),
                                       G_VARIANT_TYPE("(b)"),
                                       G_DBUS_CALL_FLAGS_NONE,
                                       3000,
                                       nullptr,
                                       &error);

  if (error) {
    g_object_unref(connection);
    vlog.debug("NameHasOwner call failed: %s", error->message);
    g_error_free(error);
    return false;
  }

  hasOwner = FALSE;
  g_variant_get(result, "(b)", &hasOwner);

  g_variant_unref(result);
  g_object_unref(connection);

  return hasOwner != FALSE;
}
