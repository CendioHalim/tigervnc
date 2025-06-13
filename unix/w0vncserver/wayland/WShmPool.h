#ifndef __W_SHM_POOL_H__
#define __W_SHM_POOL_H__

#include <stddef.h>
#include <stdint.h>

class WShm;
struct wl_shm_pool;
struct wl_buffer;

class WShmPool {
public:
  WShmPool(WShm* shm, int fd, size_t size);
  ~WShmPool();

  wl_shm_pool* getShmPool() { return pool; }
  uint8_t* getData() { return data; }

  wl_buffer* createBuffer(int32_t offset, int32_t width, int32_t height,
                          int32_t stride, uint32_t format);

private:
  wl_shm_pool* pool;
  uint8_t* data;
  size_t size;
};

#endif // __W_SHM_POOL_H__
