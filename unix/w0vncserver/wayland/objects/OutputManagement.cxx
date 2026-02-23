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

#include <stdexcept>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

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
    // FIXME: Signal that this object should be destroyed
    mode->finished = true;
  },
};

OutputMode::OutputMode(zwlr_output_mode_v1* mode_)
  : mode(mode_), width(0), height(0), refresh(0), preferred(false),
    finished(false)
{
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
    // FIXME: Use this name to match the output with a wl_output. They
    // are guaranteed to match. Same with description.
    head->name = name;
  },
  .description = [](void* data, zwlr_output_head_v1*, const char* description) {
    OutputHead* head = static_cast<OutputHead*>(data);
    head->description = description;
  },
  .physical_size = [](void* data, zwlr_output_head_v1*, int32_t width,
                      int32_t height) {
    OutputHead* head = static_cast<OutputHead*>(data);
    head->physicalWidth = width;
    head->physicalHeight = height;
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
    // FIXME: Check if we need/should wrap OutputMode in its own class
    // If not, we don't need this loop.
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
    // FIXME: This means we should destroy the head. Signal this somehow
    head->finished = true;
  },
  .make = [](void* data, zwlr_output_head_v1*, const char* make) {
    OutputHead* head = static_cast<OutputHead*>(data);
    head->make = make;
  },
  .model = [](void* data, zwlr_output_head_v1*, const char* model) {
    OutputHead* head = static_cast<OutputHead*>(data);
    head->model = model;
  },
  .serial_number = [](void* data, zwlr_output_head_v1*, const char* serial) {
    OutputHead* head = static_cast<OutputHead*>(data);
    head->serialNumber = serial;
  },
  .adaptive_sync = [](void* data, zwlr_output_head_v1*, uint32_t state) {
    OutputHead* head = static_cast<OutputHead*>(data);
    head->adaptiveSyncState = state;
  },
};

OutputHead::OutputHead(zwlr_output_head_v1* head_)
  : head(head_), currentMode(nullptr), physicalWidth(0), physicalHeight(0),
    positionX(0), positionY(0), adaptiveSyncState(0), transform(0),
    scale(0), enabled(false), finished(false)
{
  zwlr_output_head_v1_add_listener(head, &listener, this);
}

OutputHead::~OutputHead()
{
  for (OutputMode* mode : modes)
    delete mode;

  zwlr_output_head_v1_release(head);
}

OutputConfigurationHead::OutputConfigurationHead(
    zwlr_output_configuration_head_v1* head_)
  : head(head_)
{
}

OutputConfigurationHead::~OutputConfigurationHead()
{
  // FIXME: Cleanup
}

void OutputConfigurationHead::setMode(OutputMode* mode)
{
  zwlr_output_configuration_head_v1_set_mode(head, mode->getMode());
}

void OutputConfigurationHead::setCustomMode(int32_t width, int32_t height,
                                            int32_t refresh)
{
  zwlr_output_configuration_head_v1_set_custom_mode(head, width, height, refresh);
}

void OutputConfigurationHead::setPosition(int32_t x, int32_t y)
{
  zwlr_output_configuration_head_v1_set_position(head, x, y);
}

void OutputConfigurationHead::setTransform(int32_t transform)
{
  zwlr_output_configuration_head_v1_set_transform(head, transform);
}

void OutputConfigurationHead::setScale(wl_fixed_t scale)
{
  zwlr_output_configuration_head_v1_set_scale(head, scale);
}

void OutputConfigurationHead::setAdaptiveSync(uint32_t state)
{
  zwlr_output_configuration_head_v1_set_adaptive_sync(head, state);
}

const zwlr_output_configuration_v1_listener OutputConfiguration::listener = {
  .succeeded = [](void* data, zwlr_output_configuration_v1*) {
    OutputConfiguration* config = static_cast<OutputConfiguration*>(data);
    config->status = OutputConfiguration::Succeeded;
    vlog.debug("Output configuration succeeded");
    // FIXME: Signal to parent that we should be deleted
    if (config->onComplete)
      config->onComplete(config->status);
  },
  .failed = [](void* data, zwlr_output_configuration_v1*) {
    OutputConfiguration* config = static_cast<OutputConfiguration*>(data);
    config->status = OutputConfiguration::Failed;
    vlog.debug("Output configuration failed");
    if (config->onComplete)
      config->onComplete(config->status);
  },
  .cancelled = [](void* data, zwlr_output_configuration_v1*) {
    OutputConfiguration* config = static_cast<OutputConfiguration*>(data);
    config->status = OutputConfiguration::Cancelled;
    vlog.debug("Output configuration cancelled");
    if (config->onComplete)
      config->onComplete(config->status);
  },
};

OutputConfiguration::OutputConfiguration(zwlr_output_manager_v1* manager, uint32_t serial)
  : config(nullptr), status(Pending)
{
  config = zwlr_output_manager_v1_create_configuration(manager, serial);
  if (!config)
    throw std::runtime_error("Failed to create output configuration");

  zwlr_output_configuration_v1_add_listener(config, &listener, this);
}

OutputConfiguration::~OutputConfiguration()
{
  for (OutputConfigurationHead* head : heads)
    delete head;

  zwlr_output_configuration_v1_destroy(config);
}

OutputConfigurationHead* OutputConfiguration::enableHead(OutputHead* head)
{
  zwlr_output_configuration_head_v1* configHead;
  OutputConfigurationHead* configurationHead;

  // FIXME: Move this to OutputConfigurationHead constructor
  configHead = zwlr_output_configuration_v1_enable_head(config, head->getHead());
  if (!configHead)
    throw std::runtime_error("Failed to enable output head");

  configurationHead = new OutputConfigurationHead(configHead);
  heads.push_back(configurationHead);
  return configurationHead;
}

void OutputConfiguration::disableHead(OutputHead* head)
{
  // FIXME: Signal that we should be deleted?
  zwlr_output_configuration_v1_disable_head(config, head->getHead());
}

void OutputConfiguration::apply()
{
  // FIXME: This is where we should set status to Pending, no?
  zwlr_output_configuration_v1_apply(config);
}

void OutputConfiguration::test()
{
  // FIXME: Should we do a test first, then a new OutputConfiguration to apply?
  // Unclear how to use the protocol.
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
    // FIXME: We probably want to signal that we should be destroyed.
    manager->manager = nullptr;
  },
};

OutputManager::OutputManager(Display* display)
  : Object(display, "zwlr_output_manager_v1",
           &zwlr_output_manager_v1_interface),
    manager(nullptr), lastSerial(0), ready(false)
{
  manager = (zwlr_output_manager_v1*)boundObject;

  zwlr_output_manager_v1_add_listener(manager, &listener, this);
  display->roundtrip();
}

OutputManager::~OutputManager()
{
  for (OutputHead* head : heads)
    delete head;

  zwlr_output_manager_v1_stop(manager);
}

OutputConfiguration* OutputManager::createConfiguration(uint32_t serial)
{
    return new OutputConfiguration(manager, serial);
}
