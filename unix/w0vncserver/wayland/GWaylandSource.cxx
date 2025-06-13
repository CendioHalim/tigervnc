#include <assert.h>

#include <map>

#include <glib.h>

#include <wayland-client-core.h>

#include <core/LogWriter.h>

#include "../wayland/WDisplay.h"
#include "GWaylandSource.h"

static core::LogWriter vlog("GWaylandSource");

static std::map<GSource*, GWaylandSource*> sources;

GSourceFuncs GWaylandSource::sourceFuncs {
  .prepare = [](GSource* source, int* timeout) {
    return sources[source]->prepare(timeout);
  },
  .check = [](GSource* source) {
    return sources[source]->check();
  },
  .dispatch = [](GSource* source, GSourceFunc /* callback */,
                 void* /* userData */) {
    return sources[source]->dispatch();
  },
  .finalize = nullptr,
  .closure_callback = nullptr,
  .closure_marshal = nullptr
};

GWaylandSource::GWaylandSource(WDisplay* display_)
  : source(nullptr), display(display_), tag(nullptr)
{
  int fd;
  GIOCondition conditions;

  source = g_source_new(&sourceFuncs, sizeof(GSource));

  conditions = (GIOCondition) (G_IO_IN | G_IO_HUP | G_IO_ERR);
  fd = wl_display_get_fd(display_->getDisplay());

  tag = g_source_add_unix_fd(source, fd, conditions);
  prepared = false;

  sources[source] = this;
}

GWaylandSource::~GWaylandSource() {
  sources.erase(source);
  if (source && prepared)
    wl_display_cancel_read(display->getDisplay());
  if (source)
    g_source_destroy(source);
}

void GWaylandSource::attach(GMainContext* context) {
  g_source_attach(source, context);
}

int GWaylandSource::prepare(int* timeout)
{

  wl_display_flush(display->getDisplay());

  *timeout = -1;
  if (prepared)
    return FALSE;

  // We only want to call wl_display_prepare_read() once
  prepared = true;

  if (wl_display_prepare_read(display->getDisplay()) != 0) {
    if (wl_display_dispatch_pending(display->getDisplay()) < 0) {
      // FIXME: Stop here?
      vlog.error("Failed to flush wl_display: %s", strerror(errno));
    }
  }

  return FALSE;
}

int GWaylandSource::check()
{
  return g_source_query_unix_fd(source, tag) > 0;
}

int GWaylandSource::dispatch()
{
  GIOCondition events;

  events = g_source_query_unix_fd(source, tag);

  assert(prepared);

  if (events & G_IO_IN)
    wl_display_read_events(display->getDisplay());
  if (events & G_IO_HUP || events & G_IO_ERR)
    wl_display_cancel_read(display->getDisplay());

  wl_display_dispatch_pending(display->getDisplay());
  prepared = false;

  return G_SOURCE_CONTINUE;
}
