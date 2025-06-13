#ifndef __G_WLR_SOURCE_H__
#define __G_WLR_SOURCE_H__

#include <glib.h>

class WDisplay;

class GWaylandSource {
public:
  GWaylandSource(WDisplay* display);
  ~GWaylandSource();

  void attach(GMainContext* context);
private:
  int prepare(int* timeout);
  int check();
  int dispatch();

private:
  GSource* source;
  WDisplay* display;
  void* tag;
  bool prepared;
  static GSourceFuncs sourceFuncs;
};

#endif // __G_WLR_SOURCE_H__