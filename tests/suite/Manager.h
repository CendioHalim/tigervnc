#ifndef __SUITE_MANAGER_H__
#define __SUITE_MANAGER_H__
#include "codec/timed/TimedEncoder.h"
#include "stats/ManagerStats.h"
#include <rfb/EncodeManager.h>
#include <rfb/SConnection.h>
#include <rfb/encodings.h>
#include <string>

namespace suite {

  using namespace enumEncoder;

  inline std::string encodingToString(const int encoding)
  {
    switch (encoding) {
    case rfb::encodingRaw:
      return "Raw";
    case rfb::encodingCopyRect:
      return "CopyRect";
    case rfb::encodingRRE:
        return "RRE";
    case rfb::encodingCoRRE:
      return "CoRRE";
    case rfb::encodingHextile:
      return "Hextile";
    case rfb::encodingTight:
      return "Tight";
    case rfb::encodingZRLE:
      return "ZRLE";
    }
    return std::to_string(encoding);
  }

  static const int ENCODERS_COUNT = 6;

  class Manager : public rfb::EncodeManager
  {
  public:
    Manager(class rfb::SConnection *conn, bool debug);
    // Creates an EncodingManager that only uses the specified Encoder
    // for all encodings.
    Manager(class rfb::SConnection *conn, EncoderSettings settings);
    ~Manager();

    void writeUpdate(const rfb::UpdateInfo& ui, const rfb::PixelBuffer* pb,
                     const rfb::RenderedCursor* renderedCursor);

    void writeUpdate(const rfb::UpdateInfo& ui, const rfb::PixelBuffer* pb,
                     const rfb::RenderedCursor* renderedCursor,
                     uint frameTime);

    ManagerStats stats() { return stats_; };
    void updateCurrentWriteUpdate();
    void setActiveEncoder(TimedEncoder* encoder);

  protected:
    const bool SINGLE_ENCODER;
    ManagerStats stats_;
    uint currentWriteUpdate;
  };
}

#endif // __SUITE_MANAGER_H__
