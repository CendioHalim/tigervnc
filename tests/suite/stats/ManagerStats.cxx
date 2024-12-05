#include "ManagerStats.h"
#include "EncoderStats.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

namespace suite {

  double ManagerStats::delayedFramesRatio()
  {
    return (double)delayedFrames * 100 / writeUpdateStats.size();
  }

  double ManagerStats::medianWriteUpdateValue(bool comparator(
                                              const WriteUpdate& lhs,
                                              const WriteUpdate& rhs))
  {
    std::vector<WriteUpdate> copy = writeUpdateStats;
    std::nth_element(copy.begin(), copy.begin() + 1, copy.end(),
                     comparator);
    return copy[(int)(copy.size() / 2)].timeSpent;
  }

  double ManagerStats::medianTimeSpent()
  {
    return medianFrameDataValue(
      [](const WriteUpdate& lhs, const WriteUpdate& rhs) {
        return lhs.timeSpent < rhs.timeSpent;
      });
  }

  double ManagerStats::medianTimeRequired()
  {
    return medianFrameDataValue(
      [](const WriteUpdate& lhs, const WriteUpdate& rhs) {
        return lhs.timeRequired < rhs.timeRequired;
      });
  }

  double ManagerStats::medianEncodingtime()
  {
    return medianFrameDataValue(
      [](const WriteUpdate& lhs, const WriteUpdate& rhs) {
        return lhs.encodingMargin() < rhs.encodingMargin();
      });
  }

  double ManagerStats::medianFrameDataValue(bool comparator(
                                            const WriteUpdate& lhs,
                                            const WriteUpdate& rhs))
  {
    std::vector<WriteUpdate> copy = writeUpdateStats;
    std::nth_element(copy.begin(), copy.begin() + 1, copy.end(),
                     comparator);

    return copy[(int)(copy.size() / 2)].timeSpent;
  }

  double ManagerStats::meanTimeSpent()
  {
    double sum = 0;
    for (WriteUpdate& update : writeUpdateStats)
      sum += update.timeSpent;
    return sum / writeUpdateStats.size();
  }

  double ManagerStats::meanTimeRequired()
  {
    double sum = 0;
    for (WriteUpdate& update : writeUpdateStats)
      sum += update.timeRequired;
    return sum / writeUpdateStats.size();
  }

  double ManagerStats::meanEncodingMargin()
  {
    double sum = 0;
    for (WriteUpdate& update : writeUpdateStats)
      sum += update.encodingMargin();
    return sum / writeUpdateStats.size();
  }

  double ManagerStats::varianceTimeSpent()
  {
    const double mean = meanTimeSpent();
    const int size = writeUpdateStats.size();
    double variance = 0;
    for (int i = 0; i < size; i++)
      variance += (writeUpdateStats[i].timeSpent - mean) *
                  (writeUpdateStats[i].timeSpent - mean);

    // divide by N-1 for unbiased variance
    // https://en.wikipedia.org/wiki/Bessel%27s_correction
    variance /= (size - 1);
    return variance;
  }

  double ManagerStats::varianceTimeRequired()
  {
    const double mean = meanTimeRequired();
    const int size = writeUpdateStats.size();
    double variance = 0;
    for (int i = 0; i < size; i++) {
      variance += (writeUpdateStats[i].timeRequired - mean) *
                  (writeUpdateStats[i].timeRequired - mean);
    }

    variance /= (size - 1);
    return variance;
  }

  double ManagerStats::varianceEncodingMargin()
  {
    const double mean = meanEncodingMargin();
    const int size = writeUpdateStats.size();
    double variance = 0;
    for (int i = 0; i < size; i++) {
      variance += (writeUpdateStats[i].encodingMargin() - mean) *
                  (writeUpdateStats[i].encodingMargin() - mean);
    }

    variance /= (size - 1);
    return variance;
  }

  double ManagerStats::delayedFrameDelayPercentile(int percentile)
  {
    std::vector<WriteUpdate> delayedFrames;

    for (const WriteUpdate& frame : writeUpdateStats) {
      if (!frame.encodedInTime())
        delayedFrames.push_back(frame);
    }

    std::nth_element(
      delayedFrames.begin(), delayedFrames.begin() + 1,
      delayedFrames.end(),
      [](const WriteUpdate& lhs, const WriteUpdate& rhs) {
        return lhs.encodingMargin() < rhs.encodingMargin();
      });

    WriteUpdate delayedFrame =
      delayedFrames[(percentile / 100) * (delayedFrames.size() - 1)];

    double frameDelay = -delayedFrame.encodingMargin();
    return frameDelay;
  }

  void ManagerStats::addWriteUpdate(WriteUpdate update)
  {
    writeUpdateStats.push_back(update);
    if (!update.encodedInTime())
      delayedFrames++;
  }

  void ManagerStats::print()
  {
      int tableWidth = 40;
      int precision = 5;
      for (const auto& e : encoders) {
        EncoderStats* stats = e->stats();
        WriteUpdateStatistics writeUpdates = stats->writeUpdateStatistics();

        std::string encoderName = encoderClasstoString(e->encoderClass);

        // FIXME: This is one ugly mess...
        std::cout << "\n\t" << encoderName << " encoder: (seconds)\n\t\t"
                  << std::setprecision(precision) << std::fixed
                  << std::setw(tableWidth) << std::setfill(' ') << std::left
                  << "writeRect: " << std::right
                  << stats->writeRectEncodetime / 1e3
                  << "\n\t\t" << std::left  << std::setw(tableWidth)
                  << "writeSolidRect: " << std::right
                  << stats->compressionRatioSolidRects()
                  << "\n\t\t" << std::setw(tableWidth) << std::left
                  << "total: " << std::right
                               << (stats->writeRectEncodetime
                                + stats->writeSolidRectEncodetime) / 1e3
                  << "\n\t\t" << std::setfill('-')
                  << std::setw(tableWidth+precision+2)
                  << std::left << "" << std::setfill(' ')
                  << "\n\t\t" << std::setw(tableWidth) << std::left
                  << "# rects: " << std::right << stats->nRects
                  << "\n\t\t" << std::setw(tableWidth) << std::left
                  << "# solidRects: " << std::right
                  << stats->nSolidRects << "\n\t\t" << std::setfill('-')
                  << std::setw(tableWidth+precision+2)
                  << std::left << "" << std::setfill(' ')
                  << "\n\t\t" << std::setw(tableWidth) << std::left
                  << "MPx/s (rects):" << std::right
                  << stats->megaPixelsPerSecondRects() << "\n\t\t"
                  << std::setw(tableWidth) << std::left
                  << "MPx/s (solidRects:)" << std::right
                  << stats->megaPixelsPerSecondSolidRects() << "\n\t\t"
                  << std::setw(tableWidth) << std::left
                  << "MPx/s (combined)" << std::right
                  << stats->megaPixelsPerSecondCombined() << "\n\t\t"
                  << std::setfill('-')
                  << std::setw(tableWidth+precision+2)
                  << std::left << "" << std::setfill(' ') << "\n\t\t"
                  << std::setw(tableWidth) << std::left
                  << "Compression ratio rects: "
                  << stats->compressionRatioRects()
                  << "\n\t\t" << std::setw(tableWidth) << std::left
                  << "Compression ratio solidRects: " << std::right
                  << stats->compressionRatioSolidRects() << "\n\t\t"
                  << std::setw(tableWidth) << std::left
                  << "Compression ratio combined: "
                  << stats->compressionRatioCombined() << "\n\t\t"
                  << std::setfill('-') << std::setw(tableWidth)
                  << std::setw(tableWidth+precision+2)
                  << std::left << "" << std::setfill(' ') << "\n\t\t"
                  << std::setw(tableWidth) << std::left
                  << "Score (time * compression ratio):" << std::right
                  << stats->score() << "\n\t\t" << std::setfill('-')
                  << std::setw(tableWidth+precision+2)
                  << std::left << "" << "\n\t\t" << std::setw(tableWidth)
                  << std::setfill(' ')
                  << "EncodeManager writeUpdate mean (ms):" << std::right
                  <<  meanTimeSpent() << "\n\t\t" << std::left
                  << std::setfill(' ') << std::setw(tableWidth)
                  << "EncodeManager writeUpdate median:" << std::right
                  << medianTimeSpent() << "\n\t\t"
                  << std::left << std::setfill(' ') << std::setw(tableWidth)
                  << "EncodeManager writeUpdate variance:" << std::right
                  << varianceTimeSpent() << "\n\t\t" << std::left
                  << std::setfill(' ') << std::setw(tableWidth)
                  << "EncodeManager writeUpdate mean margin:" << std::right
                  << meanEncodingMargin() << "\n\t\t" << std::setw(tableWidth)
                  << std::setfill(' ') << std::left
                  << "EncodeManager delayed frames (%):"
                  << delayedFramesRatio() << "\n\t\t" << std::setfill('-')
                  << std::setw(tableWidth+precision+2)
                  << std::left << "" << "\n\t\t" << std::setw(tableWidth)
                  << std::setfill(' ')
                  << "Encoder writeUpdate mean (MPx/s):" << std::right
                  << writeUpdates.meanWriteUpdate << "\n\t\t" << std::left
                  << std::setfill(' ') << std::setw(tableWidth)
                  << "Encoder writeUpdate median:" << std::right
                  << writeUpdates.medianWriteUpdate << "\n\t\t"
                  << std::left << std::setfill(' ') << std::setw(tableWidth)
                  << "Encoder writeUpdate variance:" << std::right
                  << writeUpdates.varianceWriteUpdate
                  << std::endl;
      }
    }
}
