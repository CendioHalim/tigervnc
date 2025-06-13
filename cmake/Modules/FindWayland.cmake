#[=======================================================================[.rst:
FindWayland
----------
Find the Wayland library
Result variables
^^^^^^^^^^^^^^^^
This module will set the following variables if found:
``WAYLAND_INCLUDE_DIRS``
  where to find wayland-client.h.h, etc.
``WAYLAND_LIBRARIES``
  the libraries to link against to use libwayland.
``WAYLAND_FOUND``
  TRUE if found
#]=======================================================================]

find_package(PkgConfig QUIET)

if (PKG_CONFIG_FOUND)
  pkg_check_modules(PC_Wayland_client QUIET wayland-client)
endif()

find_program(Wayland_scanner_EXECUTABLE
    NAMES wayland-scanner
)

find_path(Wayland_client_INCLUDE_DIR NAMES wayland-client.h
  PATH_SUFFIXES
    wayland
  HINTS
    ${PC_Wayland_client_INCLUDE_DIRS}
)
mark_as_advanced(Wayland_client_INCLUDE_DIR)

find_library(Wayland_LIBRARIES NAMES wayland-client
  HINTS
    ${PC_Wayland_client_LIBRARY_DIRS}
)
mark_as_advanced(Wayland_LIBRARIES)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Wayland
  REQUIRED_VARS
    Wayland_client_INCLUDE_DIR Wayland_LIBRARIES Wayland_scanner_EXECUTABLE
)

if(Wayland_FOUND)
  set(WAYLAND_INCLUDE_DIRS ${Wayland_client_INCLUDE_DIR} ${Wayland_protocols_INCLUDE_DIR} ${Wayland_server_INCLUDE_DIR})
  set(WAYLAND_LIBRARIES ${Wayland_LIBRARIES})
  set(WAYLAND_SCANNER_EXECUTABLE ${Wayland_scanner_EXECUTABLE})
endif()
