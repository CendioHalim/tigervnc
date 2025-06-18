
#ifndef __WLR_DESKTOP_H__
#define __WLR_DESKTOP_H__

#include <glib.h>

#include <rfb/SDesktop.h>

namespace rfb { class VNCServer; }

class GWaylandSource;
class WlrPixelBuffer;
class WOutput;
class WDisplay;
class WSeat;
class WShm;
class WlrVirtualPointer;
class WlrVirtualKeyboard;

class WlrDesktop : public rfb::SDesktop
{
public:
  WlrDesktop(GMainLoop* loop);
  virtual ~WlrDesktop();

  // // -=- SDesktop interface
  void init(rfb::VNCServer* vs) override;
  void start() override;
  virtual void stop() override;
  virtual void frameTick(uint64_t msc) override;
  virtual void keyEvent(uint32_t keysym, uint32_t keycode,
                        bool down) override;
  virtual void pointerEvent(const core::Point& pos,
                            uint16_t buttonMask) override;
  void queryConnection(network::Socket* sock,
                       const char* userName) override;
  void terminate() override;

  // Check if necessary wlr protocols are available
  static bool available();

protected:
  rfb::VNCServer* server;

private:
  uint16_t oldButtonMask;
  GMainLoop* loop;
  WDisplay* display;
  WSeat* seat;
  WOutput* output;
  WlrPixelBuffer* pb;
  GWaylandSource* wlrSource;
  WlrVirtualPointer* virtualPointer;
  WlrVirtualKeyboard* virtualKeyboard;
};

#endif // __WLR_DESKTOP_H__
