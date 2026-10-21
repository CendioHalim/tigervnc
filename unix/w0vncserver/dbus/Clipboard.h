#ifndef __DBUS_CLIPBOARD_H__
#define __DBUS_CLIPBOARD_H__

#include <stdint.h>

#include <string>
#include <functional>
#include <set>
#include <queue>

#include <gio/gio.h>

struct PendingData;
struct ReadContext;

namespace dbus {
  class Proxy;

  class Clipboard {
  public:
    Clipboard(const std::string& path,
              std::function<void(const char* data)> sendClipboardDataCb,
              std::function<void(bool available)> clipboardAnnounceCb,
              std::function<void()> clipboardRequestCb);
    ~Clipboard();

    void destroy();

    // Clipboard methods
    void setSelection();
    void selectionRead();

    // Called from rfb::SDesktop, writes to all pending requests
    void selectionWrite(const char* data);

    // Resets the clipboard when data is no longer avaialble.
    void clearSelection();

  private:
    // Clipboard methods
    void selectionWrite(PendingData pendingData, const char* data);
    void selectionWriteDone(uint32_t serial, bool success);

    // Clipboard signals
    void handleSelectionTransfer(GVariant *parameters);
    void handleSelectionOwnerChanged(GVariant *parameters);

    // Asynchronously reads the clipboard data. This will send the
    // clipboard data to the client when it has finished reading.
    void handleReadDataCallback(GAsyncResult *res);

    void clearPendingSerials();

  private:
    Proxy* proxy;
    gboolean selectionOwner;
    ReadContext* readContext;

    const char* pendingReadMimeType;
    GInputStream* readStream;
    GCancellable* readCancellable;
    std::string readBuffer;

    std::set<std::string> availableMimeTypes;
    std::queue<PendingData> pendingSerials;
    std::function<void(const char* data)> sendClipboardDataCb;
    std::function<void(bool available)> clipboardAnnounceCb;
    std::function<void()> clipboardRequestCb;
  };
}

#endif // __DBUS_CLIPBOARD_H__