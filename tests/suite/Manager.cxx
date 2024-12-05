#include "Manager.h"
#include "codec/timed/TimedEncoder.h"
#include "codec/timed/timedEncoderFactory.h"
#include "stats/ManagerStats.h"

#include <chrono>

#include <rfb/EncodeManager.h>
#include <rfb/Encoder.h>
#include <rfb/SConnection.h>
#include <rfb/encodings.h>
#include <rfb/UpdateTracker.h>

namespace suite {

  Manager::Manager(rfb::SConnection *conn, bool debug) : EncodeManager(conn),
                                                         SINGLE_ENCODER(false),
                                                         currentWriteUpdate(0)
  {
    // If this is a Manager meant for debugging, leave the
    // rfb::EncodeManager as is and keep one TimedEncoder to record
    // stats.
    if (debug) {
      stats_.encoders.push_back(constructTimedEncoder(encoderTight, conn));
      return;
    }

    // Free encoders from EncodeManager and replace with TimedEncoders
    for (rfb::Encoder* encoder : encoders)
      delete encoder;

    encoders[encoderRaw] = constructTimedEncoder(encoderRaw, conn);
    encoders[encoderRRE] = constructTimedEncoder(encoderRRE, conn);
    encoders[encoderHextile] = constructTimedEncoder(encoderHextile, conn);
    encoders[encoderTight] = constructTimedEncoder(encoderTight, conn);
    encoders[encoderTightJPEG] = constructTimedEncoder(encoderTightJPEG, conn);
    encoders[encoderZRLE] = constructTimedEncoder(encoderZRLE, conn);

    for (rfb::Encoder* encoder : encoders) {
      TimedEncoder* timedEncoder = dynamic_cast<TimedEncoder*>(encoder);
      stats_.encoders.push_back(timedEncoder);
    }
  }

  Manager::Manager(rfb::SConnection* conn, EncoderSettings settings)
                                         : EncodeManager(conn),
                                           SINGLE_ENCODER(true),
                                           currentWriteUpdate(0)
  {
    // Free encoders from EncodeManager and replace with TimedEncoders
    for (rfb::Encoder* e : encoders)
      delete e;

    EncoderClass encoderClass = settings.encoderClass;
    TimedEncoder* timedEncoder = constructTimedEncoder(encoderClass, conn);
    stats_.encoders.push_back(timedEncoder);
    setActiveEncoder(timedEncoder);
  }

  Manager::~Manager()
  {
    // Ugly hack, EncodeManager's destructor will call delete on all
    // elements in encoders. Since we sometimes use the same pointer for
    // all elements in encoders, we need to make sure there is something
    // to be freed.
    if (SINGLE_ENCODER) {
      for (uint i = 1; i < ENCODERS_COUNT; i++) {
        encoders[i] = dynamic_cast<rfb::Encoder*>
                      (constructTimedEncoder(encoderRaw, conn));
      }
    }
  }

  void Manager::writeUpdate(const rfb::UpdateInfo& ui,
                            const rfb::PixelBuffer* pb,
                            const rfb::RenderedCursor* renderedCursor)
  {
    EncodeManager::writeUpdate(ui, pb, renderedCursor);
  }


  unsigned int getArea(rfb::Region changed)
  {
    std::vector<rfb::Rect> rects;
    changed.get_rects(&rects);
    unsigned int totalArea = 0;
    for (const rfb::Rect r : rects) {
      totalArea += r.area();
    }
    return totalArea;
  }

  void Manager::writeUpdate(const rfb::UpdateInfo& ui,
                            const rfb::PixelBuffer* pb,
                            const rfb::RenderedCursor* renderedCursor,
                            uint frameTime)
  {
    updateCurrentWriteUpdate();
    auto start = std::chrono::system_clock::now();
    writeUpdate(ui, pb, renderedCursor);
    auto end = std::chrono::system_clock::now();
    auto time = std::chrono::duration<double, std::milli>(end-start);

    // We keep track of the time it takes to encode an entire frame,
    // and how much time there is left until the next frame occurs
    // (as it was recorded).
    WriteUpdate update {
      .timeRequired = frameTime,
      .timeSpent = time.count(),
      .size = getArea(ui.changed),
    };
    stats_.addWriteUpdate(update);
  }


  void Manager::updateCurrentWriteUpdate()
  {
    for (TimedEncoder* encoder : stats_.encoders) {
      encoder->currentWriteUpdate = currentWriteUpdate;
    }
    // Increment the currentWriteUpdate index so we can keep track
    // of which writeRect() belongs to which writeUpdate.
    currentWriteUpdate++;
  }

  void Manager::setActiveEncoder(TimedEncoder* encoder)
  {
    encoders[encoderRaw] = dynamic_cast<rfb::Encoder*>(encoder);
    encoders[encoderRRE] = dynamic_cast<rfb::Encoder*>(encoder);
    encoders[encoderHextile] = dynamic_cast<rfb::Encoder*>(encoder);
    encoders[encoderTight] = dynamic_cast<rfb::Encoder*>(encoder);
    encoders[encoderTightJPEG] = dynamic_cast<rfb::Encoder*>(encoder);
    encoders[encoderZRLE] = dynamic_cast<rfb::Encoder*>(encoder);
  }
}
