#include "Benchmark.h"
#include "io/FrameInStream.h"
#include "stats/RecorderStats.h"

#include <rdr/Exception.h>
#include <rfb/EncodeManager.h>
#include <rfb/Exception.h>
#include <rfb/encodings.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sys/stat.h>

using namespace suite;

Benchmark::Benchmark(std::string filename, const int32_t* encodings,
                     const size_t len)
  : debugServer_(nullptr), filename(filename), encodings_(encodings),
    encodingsLength_(len)
{
  struct stat buf;
  FrameInStream is;
  std::ifstream file;
  HeaderData header;

  if (stat (filename.c_str(), &buf) != 0)
    throw std::ios_base::failure("file does not exist");

  is =  FrameInStream();
  file = std::ifstream(filename);

  header = is.parseHeader(file);

  width_ = header.width;
  height_ = header.height;
#ifdef _DEBUG
  try {
    debugServer_ = new Server(header.width, header.height, fbPF, true);
  } catch (rfb::Exception &e) {
    throw e;
  }
#endif // _DEBUG
}

Benchmark::~Benchmark()
{
#ifdef _DEBUG
  delete debugServer_;
#endif // _DEBUG
}

void Benchmark::runBenchmark()
{
  EncoderSettings* settings;

  settings = new EncoderSettings[ENCODERS_COUNT];

  for (uint i = 0; i < ENCODERS_COUNT; i++) {
    settings[i] = encoderSettings(static_cast<EncoderClass>(i));
  }

  runBenchmark(settings, ENCODERS_COUNT);

  delete [] settings;
}

void Benchmark::runBenchmark(EncoderSettings* settings, size_t len)
{
  FrameInStream is;
  std::ifstream file;
  std::map<EncoderClass, Server*> servers;

  is = FrameInStream();
  file = std::ifstream(filename);
  is.parseHeader(file); // FIXME: Don't parse header twice

  // Use one Server instance per encoding
  for (uint i = 0; i < len; i++) {
    EncoderSettings setting = settings[i];
    // All encoders in the server will be of one encoder type.
    Server* s = new Server(width(), height(), setting);

    s->setEncodings(setting.encodingSize, setting.rfbEncoding);
    servers[setting.encoderClass] = s;
  }

  std::cout << "Starting benchmark using \"" << filename << "\"\n";
  RecorderStats recorderStats;
  while (file.peek() != EOF) {
    const Image* image = is.readImage(file, recorderStats);

    // For each encoding we want to test, we load an image and loop
    // through all servers
    for (uint i = 0; i < len; i++) {
      EncoderSettings setting = settings[i];
      Server* server = servers[setting.encoderClass];

      server->loadImage(image, image->x_offset, image->y_offset);
      server->out->clear();
    }

#ifdef _DEBUG
    debugServer_->loadImage(image, image->x_offset, image->y_offset);
#endif // _DEBUG
    delete image;
  }
  std::cout << "Benchmarking complete!\n";

  // Loop through each server and print the corresponding statistics
  for (auto &s : servers) {
    // FIXME: Refactor this to a separate function
    std::string encoderRequested = encoderClasstoString(s.first);
    Server* server = s.second;
    ManagerStats managerStats = server->stats();

    if (!managerStats.encoders.size())
      continue; // FIXME: throw/log error?

    managerStats.print();
    delete server;
  }
}

EncoderSettings Benchmark::encoderSettings(EncoderClass encoderClass,
                                           PseudoEncodingLevel quality,
                                           PseudoEncodingLevel compression)
{
  int* encodings;
  EncoderSettings settings;

  encodings = new int[2];
  encodings[0] = rfb::encodingRaw;
  encodings[1] = rfb::pseudoEncodingCompressLevel0 + compression;

  settings = EncoderSettings{
    .encoderClass = encoderClass,
    .rfbEncoding = encodings,
    .encodingSize = 2,
    .quality = quality,
    .compression = compression,
    .name = encoderClasstoString(encoderClass),
  };

  switch (encoderClass) {
  case encoderRaw:
    encodings[0] = rfb::encodingRaw;
    break;
  case encoderRRE:
    encodings[0] = rfb::encodingRRE;
    break;
  case encoderHextile:
    encodings[0] = rfb::encodingHextile;
    break;
  case encoderTight:
    encodings[0] = rfb::encodingTight;
    break;
  case encoderTightJPEG:
    if (quality == NONE)
      settings.quality = TWO;
    delete [] encodings;
    encodings = new int[3];
    encodings[0] = rfb::encodingTight;
    encodings[1] = rfb::pseudoEncodingQualityLevel0 + quality;
    encodings[2] = rfb::pseudoEncodingCompressLevel0 + compression;
    settings.rfbEncoding = encodings;
    settings.encodingSize = 3;
    break;
  case encoderZRLE:
    encodings[0] = rfb::encodingZRLE;
    break;
  default:
    throw std::logic_error("EncoderClass not implemented");
  }
  return settings;
}
