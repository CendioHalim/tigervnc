#include "Benchmark.h"
#include "Client.h"

#include <iostream>
#include <rfb/Exception.h>

using namespace suite;

int main(int argc, char** argv)
{

  if (argc < 2) {
    std::cerr << "Error, incorrect arguments\n"
              << "Usage:\n\t"
              << argv[0] << " <inputfile>\n";
    exit(1);
  }

  std::string filename = argv[1];

  Benchmark *b;
  try {
    b = new Benchmark(filename);
  } catch (rfb::Exception &e) {
    std::cerr << "Error: " << e.str() << std::endl;
    exit(1);
  } catch (std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    exit(1);
  }

  b->runBenchmark();

#ifdef _DEBUG
  // Copy Server OutStream to Client InStream
  Client *client = new Client(b->width(), b->height());
  client->copyOutStreamToInStream(b->debugServer_->out);
  // Decode input & dump to images to file
  try {
    while (true) {
      client->processMsg();
    }
  } catch (rdr::EndOfStream &e) {
  } catch (rdr::Exception &e) {
    fprintf(stderr, "Failed to run rfb file: %s\n", e.str());
    exit(1);
  }
  delete client;
#endif // _DEBUG

  delete b;
  exit(0);
}
