#ifndef __SUITE_FRAME_IN_STREAM_H__
#define __SUITE_FRAME_IN_STREAM_H__

#include "../codec/ImageDecoder.h"
#include "../stats/RecorderStats.h"

namespace suite {

  struct HeaderData {
    int width;
    int height;
    double interval;
    std::string decoder;
  };

  // This class parses an istream that contains data following the
  // structure defined in FrameOutStream.
  //
  // FIXME: Probably want to wrap this class so that it is more seamless
  // to use. We shouldn't have to call parseHeader() before readImage().
  class FrameInStream
  {
  public:
    FrameInStream();
    ~FrameInStream();

    // Parses one image from the isteam.
    // Should be called repeatedly in a loop to parse an entire file.
    Image* readImage(std::istream& is);
    Image* readImage(std::istream& is, RecorderStats& recorderStats);

    // Parses the header of the file and returns a pair with the
    // width and size of the framebuffer.
    // Needs to be run before readImage as it also sets the decoder.
    HeaderData parseHeader(std::istream& is);
  protected:
    ImageDecoder* decoder;
  private:
    bool headerParsed;
  };
}

#endif // __SUITE_FRAME_IN_STREAM_H__
