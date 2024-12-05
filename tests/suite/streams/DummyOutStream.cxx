#include "DummyOutStream.h"

DummyOutStream::DummyOutStream() : rdr::MemOutStream()
{
}

DummyOutStream::~DummyOutStream()
{
}

size_t DummyOutStream::length()
{
  return ptr - start;
}


bool DummyOutStream::write(uint8_t* data, size_t length)
{
  overrun(length);
  memcpy(start, data, length);
  reposition(length);
  return true;

}
size_t DummyOutStream::size()
{
  return end - ptr;
}

uint8_t* DummyOutStream::dump()
{
  return start;
}
