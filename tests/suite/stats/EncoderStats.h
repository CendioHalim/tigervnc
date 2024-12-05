#ifndef __SUITE_ENCODERSTATSS_H__
#define __SUITE_ENCODERSTATSS_H__

#include <map>
#include <string>
#include <vector>

namespace suite {
  static const int BPP = 4;

  struct WriteRect {
    double timeSpent;
    unsigned long long pixelCount;

    friend bool operator<(const WriteRect& lhs, const WriteRect& rhs)
    {
      return lhs.timeSpent < rhs.timeSpent;
    }
  };

  struct WriteRects {
    std::vector<WriteRect> writeRects;
    std::vector<WriteRect> writeSolidRects;
  };

  struct WriteUpdateStatistics {
    double meanWriteUpdate;
    double medianWriteUpdate;
    double varianceWriteUpdate;
    double stdWriteUpdate;
  };

  struct EncoderStats {
    long double writeRectEncodetime;
    long double writeSolidRectEncodetime;
    unsigned long long inputSizeRects;
    unsigned long long outputSizeRects;
    unsigned long long inputSizeSolidRects;
    unsigned long long outputSizeSolidRects;
    int nRects;
    int nSolidRects;
    std::string name;
    std::map<int,WriteRects> writeUpdates;  // Encoder::writeRect

    double compressionRatioRects();
    double compressionRatioSolidRects();
    double compressionRatioCombined();
    double megaPixelsPerSecondRects();
    double megaPixelsPerSecondSolidRects();
    double megaPixelsPerSecondCombined();
    double score();

    struct WriteUpdateStatistics writeUpdateStatistics();
  };
}

#endif // __SUITE_ENCODERSTATS_H__
