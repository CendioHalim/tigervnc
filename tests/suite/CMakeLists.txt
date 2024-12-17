include_directories(${CMAKE_SOURCE_DIR}/common)
include_directories(${CMAKE_SOURCE_DIR}/common/os)
include_directories(${CMAKE_SOURCE_DIR}/unix)
include_directories(${CMAKE_SOURCE_DIR}/unix/common)
include_directories(${X11_INCLUDE_DIR})
include_directories(${FLTK_INCLUDE_DIR})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

add_executable(suite suite.cxx)
target_sources(suite PRIVATE
  Manager.cxx
  Server.cxx
  Client.cxx
  Benchmark.cxx
  io/FrameInStream.cxx
  codec/decoderFactory.cxx
  codec/Image.cxx
  codec/ImageDecoder.cxx
  codec/PPMDecoder.cxx
  codec/JPEGDecoder.cxx
  codec/timed/TimedEncoder.cxx
  codec/timed/timedEncoderFactory.cxx
  stats/EncoderStats.cxx
  stats/ManagerStats.cxx
  streams/DummyInStream.cxx
  streams/DummyOutStream.cxx
)

target_compile_features(suite PRIVATE cxx_std_11)
target_link_libraries(suite PRIVATE rfb)

# Recording tool
add_subdirectory(recorder)
