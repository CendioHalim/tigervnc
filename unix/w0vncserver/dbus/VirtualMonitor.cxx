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

#include <core/LogWriter.h>

#include "Proxy.h"
#include "VirtualMonitor.h"

using namespace dbus;

static core::LogWriter vlog("DBusVirtualMonitor");

VirtualMonitor::VirtualMonitor(const std::string &path)
{
  proxy = new Proxy("org.freedesktop.RemoteDesktop1", path.c_str(),
                    "org.freedesktop.RemoteDesktop1.Monitor");
}

VirtualMonitor::~VirtualMonitor()
{
  // FIXME: Wait until org.freedesktop.RemoteDesktop1.Monitor::Removed signal
  remove();
  delete proxy;
}

void VirtualMonitor::remove()
{
  GVariant* result;

  result = proxy->call("Remove", nullptr);
  g_variant_unref(result);
}

PipeWireInfo VirtualMonitor::openPipeWireStream(uint32_t cursorMode)
{
  GVariant* result;
  PipeWireInfo pw;
  GVariant* info;
  GVariantBuilder b;
  char* id;

  vlog.debug("Opening PipeWire stream");

  g_variant_builder_init(&b, G_VARIANT_TYPE("a{sv}"));
  g_variant_builder_add(&b, "{sv}", "cursor-mode",
                        g_variant_new_uint32(cursorMode));

  result = proxy->call("OpenPipeWireStream", g_variant_new("(a{sv})", &b));

  g_variant_get(result, "(s@a{sv})", &id, &info);
  pw.streamId = id;
  g_free(id);

  g_variant_lookup(info, "node-id", "u", &pw.nodeId);
  g_variant_lookup(info, "serial", "t", &pw.serial);

  // FIXME: Debug prints, remove later
  char* infoStr;
  infoStr = g_variant_print(info, true);
  vlog.debug("OpenPipeWireStream: stream %s, info %s", pw.streamId.c_str(), infoStr);
  g_free(infoStr);

  vlog.debug("node-id: %u, serial:  %lu", pw.nodeId , pw.serial);

  g_variant_unref(info);
  g_variant_unref(result);

  // FIXME: Add sanity checking/error handling here

  return pw;
}

void VirtualMonitor::closePipeWireStream(const std::string& streamId)
{
  GVariant* result;

  result = proxy->call("ClosePipeWireStream", g_variant_new("(s)", streamId.c_str()));
  g_variant_unref(result);
}
