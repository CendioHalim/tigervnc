#ifndef __SUITE_DUMMYINSTREAM_H__
#define __SUITE_DUMMYINSTREAM_H__
#include <rdr/MemInStream.h>
#include <rdr/InStream.h>

class DummyInStream : public rdr::MemInStream
{
public:
  DummyInStream();
  ~DummyInStream();
  DummyInStream(const uint8_t* data, size_t length);
};

#endif // __SUITE_DUMMYINSTREAM_H__
