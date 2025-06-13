#include <sys/mman.h>

#include <stdexcept>

#include <core/LogWriter.h>

#include "WShm.h"
#include "WShmPool.h"

static core::LogWriter vlog("WShmPool");


WShmPool::WShmPool(WShm* shm, int fd, size_t size_)
  : pool(nullptr), data(nullptr), size(size_)
{
  data = (uint8_t*)mmap(nullptr, size, PROT_READ | PROT_WRITE,
                           MAP_SHARED, fd, 0);
  if (data == MAP_FAILED)
    throw std::runtime_error("Failed to map shm");

  pool = wl_shm_create_pool(shm->getShm(), fd, size);
}

WShmPool::~WShmPool()
{
  if (munmap(data, size) < 0) {
    // FIXME: fatal_error?
    vlog.error("Failed to munmap shm");
  }
  wl_shm_pool_destroy(pool);
}

wl_buffer* WShmPool::createBuffer(int32_t offset, int32_t width,
                                  int32_t height, int32_t stride,
                                  uint32_t format) {
  return wl_shm_pool_create_buffer(pool, offset, width, height, stride, format);
}