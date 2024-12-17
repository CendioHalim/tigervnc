#include "RecorderStats.h"

namespace suite {
  void RecorderStats::addUpdate(int lostArea, int overdimension,
                                double encodingTime, double margin)
  {
    ImageUpdateStats stats{
      .lostDataArea = lostArea,
      .overDimensionedArea = overdimension,
      .encodingTime = encodingTime,
      .margin = margin,
    };
    addUpdate(stats);
  }

  void RecorderStats::addUpdate(ImageUpdateStats stats)
  {
    this->stats.push_back(stats);
  }
}
