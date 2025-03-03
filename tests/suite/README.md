# TigerVNC Performance Testing Sutie

This directory contains a testing suite for evaluating the encoding performance in TigerVNC. It consists of two programs:

1. **X11 session recorder** - A program that can record a local X
   display & store a recorded session to a file. The recorder tries
   to mimic `x0vncserver` and uses the `DAMAGE` extension to detect
   regions in the display that are updated.

    The output format is described in `io/FrameOutStream.h` and is meant
    to represent changes in the display similarly to RFB framebuffer
    updates.

    A recorded session file can be used to run benchmarks with the
    second program.

2. **Testing suite** - Takes a recorded session file and runs benchmarks
   on it. On a high level, the testing suite contains wrappers around
   `rfb::EncodeManager` and `rfb::Encoder` to measure the encoding times
   and compression ratio.

   The testing suite will read through the recorded session file and
   "feed" framebuffer updates to a `rfb::EncodeManager`. From the
   encoder's perspective, it does not see the difference between
   "real" framebuffer updates, and the ones stores in the session file.

   When benchmarking is complete, the program will output the
   compression ratios and encoding times for each encoder.

   By default, a benchmark will measure each of the 6 available encoders
   in TigerVNC performance (Raw, Hextile, RRE, Tight, TightJPEG, ZRLE).

### Quick start

First, record a session:
```
recorder <X display> <output filename> <framerate> <encoder (default=JPEG)>

# To record your local X display at 60 FPS using JPEG encoding:
recorder $DISPLAY out.data 60 JPEG
```
Then run benchmarks using the session:

```
suite <inputfile>
```