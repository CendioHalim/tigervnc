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

#ifndef __PIPEWIRE_STREAM_H__
#define __PIPEWIRE_STREAM_H__

#include <pipewire/context.h>
#include <stdint.h>

#include <list>

#include <pipewire/stream.h>

namespace rfb { class PixelFormat; }

class PipeWireSource;
class PipeWirePixelBuffer;
struct PipeWireStreamData;
struct StreamContext;

class PipeWireStream {
public:
  PipeWireStream(pw_core* core, int nodeId, PipeWirePixelBuffer* pb);
  virtual ~PipeWireStream();

private:
  void start(int nodeId);

  void handleStreamStateChanged(enum pw_stream_state old,
                                enum pw_stream_state state,
                                const char* error);
  void handleStreamParamChanged(uint32_t id, const spa_pod* param);
  void handleProcess();

  rfb::PixelFormat convertPixelformat(int spaFormat);

private:
  PipeWirePixelBuffer* pb;
  int pipeWireFd;
  pw_core* core;
  pw_stream* stream;
  spa_hook streamListener;
  uint32_t width;
  uint32_t height;
  uint32_t x;
  uint32_t y;
  static const pw_stream_events streamEventsHandler;
};

#endif //__PIPEWIRE_STREAM_H__
