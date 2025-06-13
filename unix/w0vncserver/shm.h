#ifndef __SHM_H__
#define __SHM_H__
extern "C" {
  #include <sys/types.h>
  int allocate_shm_file(size_t size);
}
#endif // __SHM_H__