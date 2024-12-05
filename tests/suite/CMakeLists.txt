include_directories(${CMAKE_SOURCE_DIR}/common)
include_directories(${CMAKE_SOURCE_DIR}/common/os)
include_directories(${CMAKE_SOURCE_DIR}/unix)
include_directories(${CMAKE_SOURCE_DIR}/unix/common)
include_directories(${X11_INCLUDE_DIR})
include_directories(${FLTK_INCLUDE_DIR})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

add_executable(suite suite.cxx)
target_sources(suite PRIVATE
  codec/timed/TimedEncoder.cxx
  stats/EncoderStats.cxx
)

target_compile_features(suite PRIVATE cxx_std_11)
target_link_libraries(suite PRIVATE rfb)

# Recording tool
add_subdirectory(recorder)
