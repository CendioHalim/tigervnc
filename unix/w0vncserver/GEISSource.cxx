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

#include <map>

#include <glib.h>

#include "EISClient.h"
#include "GEISSource.h"

static std::map<GSource*, GEISSource*> sources;

GSourceFuncs GEISSource::sourceFuncs = {
  .prepare = [](GSource* source, int* timeout) {
    assert(sources.count(source) != 0);
    return sources[source]->prepare(timeout);
  },
  .check = nullptr,
  .dispatch = [](GSource* source, GSourceFunc, void*) {
    assert(sources.count(source) != 0);
    return sources[source]->dispatch();
  },
  .finalize = nullptr,
  .closure_callback = nullptr,
  .closure_marshal = nullptr
};

GEISSource::GEISSource(EISClient* client, int fd)
  : eisClient(client)
{
  source = g_source_new(&sourceFuncs, sizeof(GSource));
  sources[source] = this;

  g_source_add_unix_fd(source, fd, G_IO_IN);

  g_source_attach(source, g_main_context_default());
}

GEISSource::~GEISSource()
{
  sources.erase(source);
  g_source_destroy(source);
  g_source_unref(source);
}

int GEISSource::prepare(int* timeout)
{
  *timeout = -1;
  return FALSE;
}

int GEISSource::dispatch()
{
  eisClient->processEvents();
  return G_SOURCE_CONTINUE;
}
