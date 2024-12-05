#include "timedEncoderFactory.h"
#include "TimedEncoder.h"
#include "rfb/Encoder.h"
#include "rfb/HextileEncoder.h"
#include "rfb/RREEncoder.h"
#include "rfb/RawEncoder.h"
#include "rfb/TightEncoder.h"
#include "rfb/TightJPEGEncoder.h"
#include "rfb/ZRLEEncoder.h"
#include <rfb/SConnection.h>
#include <stdexcept>

namespace suite {

  TimedEncoder* constructTimedEncoder(enumEncoder::EncoderClass encoder,
                                      rfb::SConnection* sconn)
  {
    rfb::Encoder* e;
    switch(encoder) {
    case enumEncoder::encoderRaw:
      e = new rfb::RawEncoder(sconn);
      break;
    case enumEncoder::encoderRRE:
      e = new rfb::RREEncoder(sconn);
      break;
    case enumEncoder::encoderHextile:
      e = new rfb::HextileEncoder(sconn);
      break;
    case enumEncoder::encoderTight:
      e = new rfb::TightEncoder(sconn);
      break;
    case enumEncoder::encoderTightJPEG:
      e = new rfb::TightJPEGEncoder(sconn);
      break;
    case enumEncoder::encoderZRLE:
      e = new rfb::ZRLEEncoder(sconn);
      break;
    default:
      throw std::logic_error("decoder not implemented");
    }
    return new TimedEncoder(encoder, e, sconn);
  }
}
