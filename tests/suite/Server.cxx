#include "Server.h"
#include "Manager.h"
#include "codec/timed/TimedEncoder.h"
#include <rfb/SConnection.h>
#include <rfb/SMsgWriter.h>

namespace suite {

  Server::Server(int width, int height, rfb::PixelFormat pf, bool debug)
                                      : settings(defaultEncoderSettings)
  {
    init(width, height, pf);
    manager = new Manager(this, debug);
  }

  Server::Server(int width, int height, EncoderSettings settings,
                                        rfb::PixelFormat pf)
                                      : settings(settings)
  {
    init(width, height, pf);
    manager = new Manager(this, settings);
  }

  void Server::init(int width, int height, rfb::PixelFormat pf)
  {
    out = new DummyOutStream();
    in = new DummyInStream();
    setStreams(in, out);
    setWriter(new rfb::SMsgWriter(&client, out));

    this->updates = rfb::SimpleUpdateTracker();

    setPixelFormat(fbPF);
    client.setPF(fbPF);

    pb = new rfb::ManagedPixelBuffer(pf, width,  height);
    setEncodings(sizeof(encodings) / sizeof(*encodings), encodings);
  }

  Server::~Server()
  {
    delete out;
    delete in;
    delete manager;
    delete pb;
  }

  void Server::loadImage(const Image *image, int x, int y)
  {
    rfb::Rect rect(x,y, x + image->width, y + image->height);
    int stride;
    pb->getBuffer(rect, &stride);
    pb->imageRect(rect, image->getBuffer());

    rfb::UpdateInfo ui;
    const rfb::Region changed(rect);

    updates.add_changed(rect);
    updates.getUpdateInfo(&ui, changed);

    manager->writeUpdate(ui, pb, NULL, image->frameTime);
  }

  void Server::writeUpdate(const rfb::UpdateInfo& ui,
                           const rfb::PixelBuffer* pb)
  {
    manager->writeUpdate(ui, pb, NULL);
  }

  ManagerStats Server::stats()
  {
    return manager->stats();
  }
}
