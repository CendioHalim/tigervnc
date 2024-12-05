#ifndef __SUITE_DUMMY_OUT_STREAM_H__
#define __SUITE_DUMMY_OUT_STREAM_H__
#include <rdr/MemOutStream.h>
#include <rdr/OutStream.h>

class DummyOutStream : public rdr::MemOutStream
{
public:
  DummyOutStream();
  ~DummyOutStream();

  virtual size_t length();
  size_t size();
  uint8_t* dump();
  bool write(uint8_t* data, size_t length);
};

#endif // __SUITE_DUMMY_OUT_STREAM_H__
