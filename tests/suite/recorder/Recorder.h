#ifndef __SUITE_RECORDER_H__
#define __SUITE_RECORDER_H__

#include "../../unix/x0vncserver/Geometry.h"
#include "../../unix/x0vncserver/Image.h"
#include "../io/FrameOutStream.h"
#include <X11/extensions/Xdamage.h>
#include <string>
#include <vector>

namespace suite {

  /* This class is used to record an X display to file. */
  class Recorder
  {
  public:
    Recorder(std::string filename, ImageEncoder* decoder,
             std::string display, int framerate);
    ~Recorder();

    // Starts recording to file
    void startRecording();
    void stopRecording();

    bool handleEvents(std::vector<XEvent>& events);
    protected:
    void handleDamagedRect(rfb::Rect &damagedRect, ImageUpdateStats stats);
    rfb::Rect rectFromEvent(XEvent& event);
    Damage damage;
    int xdamageEventBase;

  private:
    Display* dpy;
    FrameOutStream* fs;
    Geometry* geo;
    ImageFactory factory;
    ImageEncoder* encoder_;
    const double interval;
    const double intervalThreshold;
    suite::Image* lastImage;
    double encodeTime_;
    ImageUpdateStats lastImageStats;
  };
}

#endif // __SUITE_RECORDER_H__
