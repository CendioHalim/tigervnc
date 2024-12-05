#ifndef __SUITE_TIMEDENCODER_H__
#define __SUITE_TIMEDENCODER_H__

#include "../../stats/EncoderStats.h"
#include <rdr/InStream.h>
#include <rdr/MemOutStream.h>
#include <rdr/OutStream.h>
#include <rfb/Encoder.h>
#include <rfb/PixelBuffer.h>
#include <rfb/SConnection.h>
#include <chrono>
#include <stdexcept>
#include <string>
namespace suite {

  // Copied from EncodeManager.cxx
  namespace enumEncoder {
    enum EncoderClass {
      encoderRaw,
      encoderRRE,
      encoderHextile,
      encoderTight,
      encoderTightJPEG,
      encoderZRLE,
      encoderClassMax,
    };

    enum PseudoEncodingLevel {
      NONE = -1,
      ZERO,
      ONE,
      TWO,
      THREE,
      FOUR,
      FIVE,
      SIX,
      SEVEN,
      EIGHT,
      NINE
    };

    struct EncoderSettings {
      EncoderClass encoderClass;
      int *rfbEncoding;
      size_t encodingSize;
      PseudoEncodingLevel quality;
      PseudoEncodingLevel compression;
      std::string name;
    };

    inline std::string encoderClasstoString(EncoderClass encoderClass)
    {
      switch (encoderClass) {
        case encoderRaw:
          return "Raw";
        case encoderRRE:
          return "RRE";
        case encoderHextile:
          return "Hextile";
        case encoderTight:
          return "Tight";
        case encoderTightJPEG:
          return "TightJPEG";
        case encoderZRLE:
          return "ZRLE";
        default:
          throw std::logic_error("EncoderClass not implemented");
      }
    }
  // Copied from EncodeManager.cxx
  inline const char *encoderClassName(EncoderClass klass)
  {
    switch (klass) {
    case encoderRaw:
      return "Raw";
    case encoderRRE:
      return "RRE";
    case encoderHextile:
      return "Hextile";
    case encoderTight:
      return "Tight";
    case encoderTightJPEG:
      return "Tight (JPEG)";
    case encoderZRLE:
      return "ZRLE";
    case encoderClassMax:
      break;
    }

    return "Unknown Encoder Class";
  }

}


  // Forward declarations
  struct WriteUpdate;

  class TimedEncoder : public rfb::Encoder
  {
  public:
    TimedEncoder(enumEncoder::EncoderClass encoderclass,
                 rfb::Encoder* encoder, rfb::SConnection* sconn);
    ~TimedEncoder();

    void startWriteRectTimer();
    void stopWriteRectTimer(const rfb::PixelBuffer* pb);
    void startWriteSolidRectTimer();
    void stopWriteSolidRectTimer(int width, int height);
    const enumEncoder::EncoderClass encoderClass;
    EncoderStats* stats() { return _stats; };

    bool isSupported() { return true; };
    void writeRect(const rfb::PixelBuffer* pb,
                   const rfb::Palette& palette);

    void writeSolidRect(int width, int height,
                        const rfb::PixelFormat& pf,
                        const uint8_t* colour);

    void setCompressLevel(int level);
    void setQualityLevel(int level);
    void setFineQualityLevel(int quality, int subsampling);
    int getCompressLevel();
    int getQualityLevel();

    uint currentWriteUpdate;
  private:
    rdr::MemOutStream *encoderOutstream;
    rdr::OutStream* os;
    rdr::InStream* is;
    rfb::SConnection* conn_;
    EncoderStats* _stats;
    std::chrono::system_clock::time_point writeRectStart;
    std::chrono::system_clock::time_point writeSolidRectStart;
    rfb::Encoder *encoder;
  };
}

#endif // __SUITE_TIMEDENCODER_H__
