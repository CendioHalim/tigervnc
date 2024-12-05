#ifndef __SUITE_MANAGERSTATS_H__
#define __SUITE_MANAGERSTATS_H__

#include "../codec/timed/TimedEncoder.h"
#include <cmath>
#include <vector>

namespace suite {

  // writeUpdate() stats
  struct WriteUpdate {
    uint timeRequired;
    double timeSpent;
    unsigned int size;

    // Was the frame encoded within the time budget?
    bool encodedInTime() const { return timeSpent < timeRequired; }
    // How much time (in milliseconds) do we have left, or did we
    // surpass the budget with?
    int encodingMargin() const { return timeRequired - timeSpent; }
  };

  struct ManagerStats {
    int delayedFrames = 0;
    std::vector<WriteUpdate> writeUpdateStats;
    std::vector<TimedEncoder*> encoders;

    double delayedFramesRatio();
    double medianWriteUpdateValue(bool comparator(const WriteUpdate& lhs,
                                  const WriteUpdate& rhs));
    double medianTimeSpent();
    double medianTimeRequired();
    double medianEncodingtime();
    double medianFrameDataValue(bool comparator(const WriteUpdate& lhs,
                                const WriteUpdate& rhs));
    double meanTimeSpent();
    double meanTimeRequired();
    double meanEncodingMargin();
    double varianceTimeSpent();
    double varianceTimeRequired();
    double varianceEncodingMargin();
    double stdTimeSpent() { return std::sqrt(varianceTimeSpent()); }
    double stdTimeRequired() { return std::sqrt(varianceTimeRequired()); }
    double stdEncodingMargin() { return std::sqrt(varianceEncodingMargin()); }
    double delayedFrameDelayPercentile(int percentile);
    void addWriteUpdate(WriteUpdate update);
    void print();
  };
}

#endif // __SUITE_MANAGERSTATS_H__
