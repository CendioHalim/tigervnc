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

#ifndef __REMOTE_DESKTOP_H__
#define __REMOTE_DESKTOP_H__

#include <stdint.h>

#include <functional>
#include <string>
#include <vector>
#include <list>
#include <map>

#include <gio/gio.h>

namespace rfb { class VNCServer; }

class PortalProxy;
struct ClipboardEntry {
  std::string data;
  std::string mimeType;
};

struct PendingData {
  ClipboardEntry data;
  uint32_t serial;
  int fd;
};


class RemoteDesktop {
public:
  RemoteDesktop(std::string restoreToken,
                std::function<void(int fd, uint32_t nodeId)> startPipewireCb,
                std::function<void(const char*)> cancelStartCb);
  ~RemoteDesktop();

  // Methods called from SDesktop

  // keyEvent
  void notifyKeyboardKeysym(uint32_t keysym, bool down);
  // keyEvent using rawKeyboard
  void notifyKeyboardKeycode(uint32_t xtcode, bool down);
  // pointerEvent
  void notifyPointerMotionAbsolute(int32_t x, int32_t y, uint16_t buttonMask);

  // Create a Portal session
  void createSession();

  std::string getRestoreToken() const { return restoreToken; }

  void setSelection(const char* data);
  void setSelection(const char* data, std::vector<const char*> mimeTypes[]);
private:
  void selectionWrite(uint32_t serial);
  void selectionWriteDone(uint32_t serial, bool success);
  void closeSession();
  void selectDevices();
  void selectSources();
  void requestClipboard();
  void start();
  void openPipewireRemote();

  // Portal signal callbacks
  void handleCreateSession(GVariant *parameters);
  void handleStart(GVariant *parameters);
  void handleSelectDevices(GVariant *parameters);
  void handleSelectSources(GVariant *parameters);
  void handleRequestClipboard(GVariant *parameters);
  void handleOpenPipewireRemote(GObject *proxy, GAsyncResult *res);

  // Clipboard
  void handleSelectionWrite(GObject* proxy, GAsyncResult* res);
  void handleSelectionTransfer(GVariant* parameters);
  void handleSelectionOwnerChanged(GVariant* parameters);

  // pointerEvent help functions
  void notifyPointerButton(int32_t button, bool down);
  void notifyPointerAxisDiscrete(int32_t button);

  // Parses ScreenCast streams. Returns false on error
  bool parseStreams(GVariant* streams);

  // Loads the restore token, returns false on error
  bool loadRestoreToken();
  // Stores the restore token, returns false on error
  bool storeRestoreToken(const char* restoreToken);
  // Resets the restore token, returns false on error
  bool clearRestoreToken();

private:
  bool sessionStarted;
  uint16_t oldButtonMask;
  uint32_t selectedDevices;
  bool clipboardEnabled;
  std::string sessionHandle;
  std::list<PendingData> pendingData;
  std::vector<ClipboardEntry> clientData;

  uint32_t pipewireNodeId;
  PortalProxy* remoteDesktop;
  PortalProxy* screenCast;
  PortalProxy* clipboard;
  PortalProxy* session;

  std::string restoreToken;

  std::map<uint32_t, int32_t> pendingSelections;
  std::function<void(int fd, uint32_t nodeId)> startPipewireCb;
  std::function<void(const char* reason)> cancelStartCb;
};

#endif // __REMOTE_DESKTOP_H__
