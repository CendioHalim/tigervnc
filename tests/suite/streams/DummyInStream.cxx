#include "DummyInStream.h"

static const size_t BUF_SIZE = 10 << 20; // 10MB

DummyInStream::DummyInStream() : rdr::MemInStream(new uint8_t[BUF_SIZE],
                                                  BUF_SIZE, false)
{
}

DummyInStream::~DummyInStream()
{
}
DummyInStream::DummyInStream(const uint8_t* data, size_t length)
                           : rdr::MemInStream(data, length, true)
{
}
