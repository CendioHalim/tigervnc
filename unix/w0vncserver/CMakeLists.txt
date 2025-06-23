if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(FATAL_ERROR "cmake must be invoked with the top level directory")
endif()

add_executable(w0vncserver
  ../x0vncserver/qnum_to_xorgevdev.c
  portals/PortalProxy.cxx
  portals/RemoteDesktop.cxx
  wayland/GWaylandSource.cxx
  wayland/WOutput.cxx
  wayland/WObject.cxx
  wayland/WDisplay.cxx
  wayland/WSeat.cxx
  wayland/WShm.cxx
  wayland/WShmPool.cxx
  wayland/WKeyboard.cxx
  wlr/WlrDesktop.cxx
  wlr/WlrPixelBuffer.cxx
  wlr/WlrScreencopyManager.cxx
  wlr/WlrVirtualPointer.cxx
  wlr/WlrVirtualKeyboard.cxx
  shm.c
  w0vncserver.cxx
  GSocketSource.cxx
  PipeWirePixelBuffer.cxx
  PipeWireSource.cxx
  PipeWireStream.cxx
  PortalDesktop.cxx
  RFBTimerSource.cxx
)

# Generate protocol headers
set(GEN_WAYLAND_SRCS "")
set(WLR_PROTOCOLS
  wlr-screencopy-unstable-v1
  wlr-virtual-pointer-unstable-v1
  virtual-keyboard-unstable-v1
)

foreach(P ${WLR_PROTOCOLS})
  set(XML ${CMAKE_CURRENT_SOURCE_DIR}/protocols/${P}.xml)
  set(H ${CMAKE_CURRENT_BINARY_DIR}/${P}.h)
  set(C ${CMAKE_CURRENT_BINARY_DIR}/${P}.c)

  add_custom_command(
    OUTPUT ${C}
    BYPRODUCTS ${C}
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} private-code ${XML} ${C}
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} client-header ${XML} ${H}
    DEPENDS ${XML}
    COMMENT "Generating ${P}.c and ${P}.h"
  )

  message(STATUS "Generating ${P}.c and ${P}.h")

  set_source_files_properties(${H} ${C} PROPERTIES GENERATED TRUE)

  list(APPEND GEN_WAYLAND_SRCS ${C})
endforeach()

message(STATUS "Current binary dir: ${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "Generated sources: ${GEN_WAYLAND_SRCS}")


target_sources(w0vncserver PRIVATE ${GEN_WAYLAND_SRCS})

target_include_directories(w0vncserver SYSTEM PUBLIC ${GLIB_INCLUDE_DIRS})
target_include_directories(w0vncserver SYSTEM PUBLIC ${GIO_INCLUDE_DIRS})
target_include_directories(w0vncserver SYSTEM PUBLIC ${GOBJECT_INCLUDE_DIRS})
target_include_directories(w0vncserver SYSTEM PUBLIC ${PIXMAN_INCLUDE_DIRS})
target_include_directories(w0vncserver SYSTEM PUBLIC ${PIPEWIRE_INCLUDE_DIRS})
target_include_directories(w0vncserver SYSTEM PUBLIC ${UUID_INCLUDE_DIRS})
target_include_directories(w0vncserver SYSTEM PUBLIC ${WAYLAND_INCLUDE_DIRS})
target_include_directories(w0vncserver SYSTEM PUBLIC ${XKBCOMMON_INCLUDE_DIRS})
target_include_directories(w0vncserver PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(w0vncserver core rfb network rdr)
target_link_libraries(w0vncserver ${GLIB_LIBRARIES})
target_link_libraries(w0vncserver ${GIO_LIBRARIES})
target_link_libraries(w0vncserver ${GOBJECT_LIBRARIES})
target_link_libraries(w0vncserver ${PIXMAN_LIBRARIES})
target_link_libraries(w0vncserver ${PIPEWIRE_LIBRARIES})
target_link_libraries(w0vncserver ${UUID_LIBRARIES})
target_link_libraries(w0vncserver ${XKBCOMMON_LIBRARIES})
target_link_libraries(w0vncserver ${WAYLAND_LIBRARIES})

install(TARGETS w0vncserver DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
install(FILES w0vncserver.man DESTINATION ${CMAKE_INSTALL_FULL_MANDIR}/man1 RENAME w0vncserver.1)
