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

#include <gio/gio.h>
#include <glib.h>

#include "NameWatcher.h"

using namespace dbus;

NameWatcher::NameWatcher(const char* name,
                         std::function<void()> appearedCb_,
                         std::function<void()> vanishedCb_)
  : appearedCb(appearedCb_), vanishedCb(vanishedCb_)
{
  id = g_bus_watch_name(G_BUS_TYPE_SESSION, name, G_BUS_NAME_WATCHER_FLAGS_NONE,
                        [](GDBusConnection*, const char*,const char*, void* userData) {
                             NameWatcher* self = (NameWatcher*)userData;
                             if (self->appearedCb)
                               self->appearedCb();
                        },
                        [](GDBusConnection*, const char*, void* userData) {
                            NameWatcher* self = (NameWatcher*)userData;
                            if (self->vanishedCb)
                              self->vanishedCb();
                        }, this, nullptr);
}

NameWatcher::~NameWatcher()
{
  g_bus_unwatch_name(id);
}
