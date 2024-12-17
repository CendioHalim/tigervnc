#ifndef __SUITE_RECORDERSTATS_H__
#define __SUITE_RECORDERSTATS_H__

#include "../recorder/util.h"
#include <vector>

namespace suite {
  struct RecorderStats {
    std::vector<ImageUpdateStats> stats;

    void addUpdate(int lostArea, int overdimension, double encodingTime,
                   double margin);
    void addUpdate(ImageUpdateStats stats);
  };
}

#endif // __SUITE_RECORDERSTATS_H__
