/* Copyright 2026 Tobias Fahleson for Cendio AB
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

#ifndef __WAYLAND_OUTPUT_MANAGEMENT_H__
#define __WAYLAND_OUTPUT_MANAGEMENT_H__

#include <stdint.h>

#include <string>
#include <vector>

#include <wayland-client-core.h>

#include "Object.h"

struct zwlr_output_manager_v1;
struct zwlr_output_head_v1;
struct zwlr_output_mode_v1;
struct zwlr_output_manager_v1_listener;
struct zwlr_output_head_v1_listener;
struct zwlr_output_mode_v1_listener;
struct zwlr_output_configuration_v1;
struct zwlr_output_configuration_v1_listener;
struct zwlr_output_configuration_head_v1;

namespace wayland {
  class Display;
  class Object;

  class OutputMode {
  public:
    OutputMode(zwlr_output_mode_v1* mode_);
    ~OutputMode();

    zwlr_output_mode_v1* getMode() const { return mode; }
    int32_t getWidth() const { return width; }
    int32_t getHeight() const { return height; }
    int32_t getRefresh() const { return refresh; }
    bool isPreferred() const { return preferred; }

  private:
    static const zwlr_output_mode_v1_listener listener;

    zwlr_output_mode_v1* mode;
    int32_t width;
    int32_t height;
    int32_t refresh;
    bool preferred;
    bool finished;
  };

  class OutputHead {
  public:
    OutputHead(zwlr_output_head_v1* head_);
    ~OutputHead();

    zwlr_output_head_v1* getHead() const { return head; }
    const std::vector<OutputMode*>& getModes() const { return modes; }

  private:
    static const zwlr_output_head_v1_listener listener;

    zwlr_output_head_v1* head;
    std::vector<OutputMode*> modes;
    OutputMode* currentMode;
    std::string name;
    std::string description;
    std::string make;
    std::string model;
    std::string serialNumber;
    int32_t physicalWidth;
    int32_t physicalHeight;
    int32_t positionX;
    int32_t positionY;
    uint32_t adaptiveSyncState;
    int32_t transform;
    wl_fixed_t scale;
    bool hasPhysicalSize;
    bool enabled;
    bool finished;
  };

  class OutputConfigurationHead {
  public:
    OutputConfigurationHead(zwlr_output_configuration_head_v1* head_);
    ~OutputConfigurationHead();

    void setMode(OutputMode* mode);
    void setCustomMode(int32_t width, int32_t height, int32_t refresh);
    void setPosition(int32_t x, int32_t y);
    void setTransform(int32_t transform);
    void setScale(wl_fixed_t scale);
    void setAdaptiveSync(uint32_t state);

  private:
    zwlr_output_configuration_head_v1* head;
  };

  class OutputConfiguration {
  public:
    enum Status {
      Pending,
      Succeeded,
      Failed,
      Cancelled
    };

    OutputConfiguration(zwlr_output_configuration_v1* config_);
    ~OutputConfiguration();

    OutputConfigurationHead* enableHead(OutputHead* head);
    void disableHead(OutputHead* head);
    void apply();
    void test();
    Status getStatus() const { return status; }

  private:
    static const zwlr_output_configuration_v1_listener listener;

    zwlr_output_configuration_v1* config;
    std::vector<OutputConfigurationHead*> heads;
    Status status;
  };

  class OutputManager : public Object {
  public:
    OutputManager(Display* display);
    ~OutputManager();

    OutputConfiguration* createConfiguration(uint32_t serial);
    const std::vector<OutputHead*>& getHeads() const { return heads; }
    uint32_t getLastSerial() const { return lastSerial; }
    bool isReady() const { return ready; }

  private:
    static const zwlr_output_manager_v1_listener listener;

    zwlr_output_manager_v1* manager;
    std::vector<OutputHead*> heads;
    uint32_t lastSerial;
    bool ready;
  };
};

#endif // __WAYLAND_OUTPUT_MANAGEMENT_H__
