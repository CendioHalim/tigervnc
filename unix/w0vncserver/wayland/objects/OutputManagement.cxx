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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <wlr-output-management-unstable-v1.h>

#include <core/LogWriter.h>

#include "Display.h"
#include "Object.h"
#include "OutputManagement.h"

using namespace wayland;

static core::LogWriter vlog("WaylandOutputManagement");

const zwlr_output_mode_v1_listener OutputMode::listener = {
  .size = [](void* data, zwlr_output_mode_v1*, int32_t width, int32_t height) {
    OutputMode* mode = static_cast<OutputMode*>(data);
    mode->width = width;
    mode->height = height;
  },
  .refresh = [](void* data, zwlr_output_mode_v1*, int32_t refresh) {
    OutputMode* mode = static_cast<OutputMode*>(data);
    mode->refresh = refresh;
  },
  .preferred = [](void* data, zwlr_output_mode_v1*) {
    OutputMode* mode = static_cast<OutputMode*>(data);
    mode->preferred = true;
  },
  .finished = [](void* data, zwlr_output_mode_v1*) {
    OutputMode* mode = static_cast<OutputMode*>(data);
    mode->finished = true;
  },
};

OutputMode::OutputMode(zwlr_output_mode_v1* mode_)
  : mode(mode_), width(0), height(0), refresh(0), preferred(false),
    finished(false)
{
  if (mode)
    zwlr_output_mode_v1_add_listener(mode, &listener, this);
}

OutputMode::~OutputMode()
{
  if (mode)
    zwlr_output_mode_v1_release(mode);
}

const zwlr_output_head_v1_listener OutputHead::listener = {
  .name = [](void* data, zwlr_output_head_v1*, const char* name) {
    OutputHead* head = static_cast<OutputHead*>(data);
    head->name = name ? name : "";
  },
  .description = [](void* data, zwlr_output_head_v1*, const char* description) {
    OutputHead* head = static_cast<OutputHead*>(data);
    head->description = description ? description : "";
  },
  .physical_size = [](void* data, zwlr_output_head_v1*, int32_t width,
                      int32_t height) {
    OutputHead* head = static_cast<OutputHead*>(data);
    head->physicalWidth = width;
    head->physicalHeight = height;
    head->hasPhysicalSize = true;
  },
  .mode = [](void* data, zwlr_output_head_v1*, zwlr_output_mode_v1* mode) {
    OutputHead* head = static_cast<OutputHead*>(data);
    OutputMode* modeObj = new OutputMode(mode);
    head->modes.push_back(modeObj);
  },
  .enabled = [](void* data, zwlr_output_head_v1*, int32_t enabled) {
    OutputHead* head = static_cast<OutputHead*>(data);
    head->enabled = enabled != 0;
  },
  .current_mode = [](void* data, zwlr_output_head_v1*, zwlr_output_mode_v1* mode) {
    OutputHead* head = static_cast<OutputHead*>(data);
    head->currentMode = nullptr;
    for (OutputMode* entry : head->modes) {
      if (entry->getMode() == mode) {
        head->currentMode = entry;
        break;
      }
    }
  },
  .position = [](void* data, zwlr_output_head_v1*, int32_t x, int32_t y) {
    OutputHead* head = static_cast<OutputHead*>(data);
    head->positionX = x;
    head->positionY = y;
  },
  .transform = [](void* data, zwlr_output_head_v1*, int32_t transform) {
    OutputHead* head = static_cast<OutputHead*>(data);
    head->transform = transform;
  },
  .scale = [](void* data, zwlr_output_head_v1*, wl_fixed_t scale) {
    OutputHead* head = static_cast<OutputHead*>(data);
    head->scale = scale;
  },
  .finished = [](void* data, zwlr_output_head_v1*) {
    OutputHead* head = static_cast<OutputHead*>(data);
    head->finished = true;
  },
  .make = [](void* data, zwlr_output_head_v1*, const char* make) {
    OutputHead* head = static_cast<OutputHead*>(data);
    head->make = make ? make : "";
  },
  .model = [](void* data, zwlr_output_head_v1*, const char* model) {
    OutputHead* head = static_cast<OutputHead*>(data);
    head->model = model ? model : "";
  },
  .serial_number = [](void* data, zwlr_output_head_v1*, const char* serial) {
    OutputHead* head = static_cast<OutputHead*>(data);
    head->serialNumber = serial ? serial : "";
  },
  .adaptive_sync = [](void* data, zwlr_output_head_v1*, uint32_t state) {
    OutputHead* head = static_cast<OutputHead*>(data);
    head->adaptiveSyncState = state;
  },
};

OutputHead::OutputHead(zwlr_output_head_v1* head_)
  : head(head_), currentMode(nullptr), physicalWidth(0), physicalHeight(0),
    positionX(0), positionY(0), adaptiveSyncState(0), transform(0),
    scale(0), hasPhysicalSize(false), enabled(false), finished(false)
{
  if (head)
    zwlr_output_head_v1_add_listener(head, &listener, this);
}

OutputHead::~OutputHead()
{
  for (OutputMode* mode : modes)
    delete mode;

  if (head)
    zwlr_output_head_v1_release(head);
}

OutputConfigurationHead::OutputConfigurationHead(
    zwlr_output_configuration_head_v1* head_)
  : head(head_)
{
}

OutputConfigurationHead::~OutputConfigurationHead()
{
}

void OutputConfigurationHead::setMode(OutputMode* mode)
{
  if (!head || !mode)
    return;

  zwlr_output_configuration_head_v1_set_mode(head, mode->getMode());
}

void OutputConfigurationHead::setCustomMode(int32_t width, int32_t height,
                                            int32_t refresh)
{
  if (!head)
    return;

  zwlr_output_configuration_head_v1_set_custom_mode(head, width, height, refresh);
}

void OutputConfigurationHead::setPosition(int32_t x, int32_t y)
{
  if (!head)
    return;

  zwlr_output_configuration_head_v1_set_position(head, x, y);
}

void OutputConfigurationHead::setTransform(int32_t transform)
{
  if (!head)
    return;

  zwlr_output_configuration_head_v1_set_transform(head, transform);
}

void OutputConfigurationHead::setScale(wl_fixed_t scale)
{
  if (!head)
    return;

  zwlr_output_configuration_head_v1_set_scale(head, scale);
}

void OutputConfigurationHead::setAdaptiveSync(uint32_t state)
{
  if (!head)
    return;

  zwlr_output_configuration_head_v1_set_adaptive_sync(head, state);
}

const zwlr_output_configuration_v1_listener OutputConfiguration::listener = {
  .succeeded = [](void* data, zwlr_output_configuration_v1*) {
    OutputConfiguration* config = static_cast<OutputConfiguration*>(data);
    config->status = OutputConfiguration::Succeeded;
    vlog.debug("Output configuration succeeded");
  },
  .failed = [](void* data, zwlr_output_configuration_v1*) {
    OutputConfiguration* config = static_cast<OutputConfiguration*>(data);
    config->status = OutputConfiguration::Failed;
    vlog.debug("Output configuration failed");
  },
  .cancelled = [](void* data, zwlr_output_configuration_v1*) {
    OutputConfiguration* config = static_cast<OutputConfiguration*>(data);
    config->status = OutputConfiguration::Cancelled;
    vlog.debug("Output configuration cancelled");
  },
};

OutputConfiguration::OutputConfiguration(zwlr_output_configuration_v1* config_)
  : config(config_), status(Pending)
{
  if (config)
    zwlr_output_configuration_v1_add_listener(config, &listener, this);
}

OutputConfiguration::~OutputConfiguration()
{
  for (OutputConfigurationHead* head : heads)
    delete head;

  if (config)
    zwlr_output_configuration_v1_destroy(config);
}

OutputConfigurationHead* OutputConfiguration::enableHead(OutputHead* head)
{
  zwlr_output_configuration_head_v1* configHead = nullptr;

  if (!config || !head)
    return nullptr;

  configHead = zwlr_output_configuration_v1_enable_head(config,
                                                        head->getHead());
  if (!configHead)
    return nullptr;

  OutputConfigurationHead* headObj = new OutputConfigurationHead(configHead);
  heads.push_back(headObj);
  return headObj;
}

void OutputConfiguration::disableHead(OutputHead* head)
{
  if (!config || !head)
    return;

  zwlr_output_configuration_v1_disable_head(config, head->getHead());
}

void OutputConfiguration::apply()
{
  if (!config)
    return;

  zwlr_output_configuration_v1_apply(config);
}

void OutputConfiguration::test()
{
  if (!config)
    return;

  zwlr_output_configuration_v1_test(config);
}

const zwlr_output_manager_v1_listener OutputManager::listener = {
  .head = [](void* data, zwlr_output_manager_v1*, zwlr_output_head_v1* head) {
    OutputManager* manager = static_cast<OutputManager*>(data);
    OutputHead* headObj = new OutputHead(head);
    manager->heads.push_back(headObj);
  },
  .done = [](void* data, zwlr_output_manager_v1*, uint32_t serial) {
    OutputManager* manager = static_cast<OutputManager*>(data);
    manager->lastSerial = serial;
    manager->ready = true;
  },
  .finished = [](void* data, zwlr_output_manager_v1*) {
    OutputManager* manager = static_cast<OutputManager*>(data);
    manager->manager = nullptr;
  },
};

OutputManager::OutputManager(Display* display)
  : Object(display, "zwlr_output_manager_v1",
           &zwlr_output_manager_v1_interface),
    manager(nullptr), lastSerial(0), ready(false)
{
  manager = (zwlr_output_manager_v1*)boundObject;
  if (manager)
    zwlr_output_manager_v1_add_listener(manager, &listener, this);
}

OutputManager::~OutputManager()
{
  for (OutputHead* head : heads)
    delete head;

  if (manager)
    zwlr_output_manager_v1_stop(manager);
}

OutputConfiguration* OutputManager::createConfiguration(uint32_t serial)
{
  zwlr_output_configuration_v1* config = nullptr;

  if (!manager)
    return nullptr;

  config = zwlr_output_manager_v1_create_configuration(manager, serial);
  if (!config)
    return nullptr;

  return new OutputConfiguration(config);
}
