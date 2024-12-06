#ifndef __SUITE_CLIENT_H__
#define __SUITE_CLIENT_H__

#include <rfb/CConnection.h>
#include "Server.h"

namespace suite {
  class Client : public rfb::CConnection
  {
  public:
    Client(int width, int height, rfb::PixelFormat pf = fbPF);
    ~Client();

    // Sets InStream and sets a new CMsgReader with the new InStream
    void setInStream(DummyInStream* in);

    // Creates copy of buffer in OutStream and creates a new InStream
    // with copied data
    void copyOutStreamToInStream(DummyOutStream* out);

    virtual void resizeFramebuffer();
    virtual void framebufferUpdateStart();
    virtual void framebufferUpdateEnd();
    virtual bool dataRect(const rfb::Rect&, int);

    // Unused
    virtual void initDone() {};
    virtual void setCursor(int, int, const rfb::Point&, const uint8_t*) {};
    virtual void setCursorPos(const rfb::Point&) {};
    virtual void setColourMapEntries(int, int, uint16_t*) {};
    virtual void bell() {};

    rfb::SimpleUpdateTracker updates;
  protected:
    rfb::ManagedPixelBuffer* pb;
  private:
    DummyInStream *in;
    DummyOutStream *out;
    rfb::CMsgReader* reader;
    rfb::CMsgWriter* writer;
  };
}

#endif // __SUITE_CLIENT_H__
