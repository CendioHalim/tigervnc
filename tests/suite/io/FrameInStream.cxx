#include "FrameInStream.h"
#include "../codec/decoderFactory.h"
#include <ios>
#include <iostream>

namespace suite {

  FrameInStream::FrameInStream()
    : decoder(nullptr), headerParsed(false)
  {
  }

  FrameInStream::~FrameInStream()
  {
    delete decoder;
  }

  Image* FrameInStream::readImage(std::istream& is)
  {
    RecorderStats s; // ignored
    return readImage(is, s);
  }

  Image* FrameInStream::readImage(std::istream& is,
                                  RecorderStats& recorderStats)
  {
    if (!headerParsed)
      throw std::logic_error("need to parse header before reading image");

    int size, width, height, x_offset, y_offset;
    double frameTime;
    ImageUpdateStats stats;

    if (is.peek() == EOF)
      throw std::ios_base::failure("error reading from stream");

    is >> size >> width >> height >> x_offset >> y_offset >> frameTime
       >> stats.lostDataArea >> stats.overDimensionedArea
       >> stats.encodingTime >> stats.margin;
    is.ignore();

    recorderStats.stats.push_back(stats);

    uint8_t* data = new uint8_t[size];
    is.read((char*)data, size);

    Image* image = decoder->decodeImageFromMemory(data, width, height,
                                                  size, x_offset,
                                                  y_offset);
    delete [] data;
    image->frameTime = frameTime;
    image->stats = stats;
    return image;
  }

  HeaderData FrameInStream::parseHeader(std::istream& is)
  {
    if (headerParsed)
      throw std::logic_error("header already parsed");

    std::string decoder;
    int width;
    int height;
    double interval;
    is >> decoder >> width >> height >> interval;

    HeaderData header {
      .width = width,
      .height = height,
      .interval = interval,
      .decoder = decoder,
    };

    this->decoder = constructDecoder(decoder);
    headerParsed = true;
    return header;
  }
}
