#include "EncoderStats.h"
#include <algorithm>
#include <cmath>

namespace suite {
  double EncoderStats::compressionRatioRects()
  {
    return (double) outputSizeRects / inputSizeRects;
  }

  double EncoderStats::compressionRatioSolidRects()
  {
    return (double) outputSizeSolidRects / inputSizeSolidRects;
  }

  double EncoderStats::compressionRatioCombined()
  {
    return (double) (outputSizeRects + outputSizeSolidRects)
                  / (inputSizeRects + inputSizeSolidRects);
  }

  double EncoderStats::megaPixelsPerSecondRects()
  {
    return (inputSizeRects / 1e6) / (writeRectEncodetime / 1e3);
  }

  double EncoderStats::megaPixelsPerSecondSolidRects()
  {
    return (inputSizeSolidRects / 1e6) / (writeSolidRectEncodetime / 1e3);
  }

  double EncoderStats::megaPixelsPerSecondCombined()
  {
    return ((inputSizeRects + inputSizeSolidRects) / 1e6 )
         / ((writeRectEncodetime + writeSolidRectEncodetime) / 1e3);
  }

  // Returns a "score" which is a function of time
  // per compressed data
  double EncoderStats::score()
  {
    return (writeRectEncodetime + writeSolidRectEncodetime)
         * (compressionRatioCombined());
  }

  struct WriteUpdateStatistics EncoderStats::writeUpdateStatistics()
  {
    std::vector<double> mPxPerSecond;
    double sumMPxPerSecond = 0;
    for (const auto& update : writeUpdates) {
      double timeSum = 0;
      double pixels = 0;

      // Sum all updates belonging to the same writeUpdate
      for (const auto& rects : update.second.writeRects) {
        timeSum += rects.timeSpent;
        pixels += (rects.pixelCount * BPP);
      }

      for (const auto& solidRects : update.second.writeSolidRects) {
        timeSum += solidRects.timeSpent;
        pixels += (solidRects.pixelCount);
      }

      double mps = pixels / (timeSum * 10e6);
      sumMPxPerSecond += mps;
      mPxPerSecond.push_back(mps);
    }

    std::nth_element(mPxPerSecond.begin(),
                     mPxPerSecond.begin() + 1, mPxPerSecond.end());
    double median = mPxPerSecond[mPxPerSecond.size() / 2];

    double mean = sumMPxPerSecond / mPxPerSecond.size();
    double variance = 0;
    for (uint i = 0; i < mPxPerSecond.size(); i++)
      variance += (mPxPerSecond[i] - mean) * (mPxPerSecond[i] - mean);

    variance /= (mPxPerSecond.size() - 1);

    struct WriteUpdateStatistics stats {
      .meanWriteUpdate = mean,
      .medianWriteUpdate = median,
      .varianceWriteUpdate = variance,
      .stdWriteUpdate = std::sqrt(variance),
    };
    return stats;
  }
}
