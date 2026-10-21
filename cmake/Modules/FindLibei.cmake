#[=======================================================================[.rst:
FindLibei
----------
Find the libei library (Emulated Input)
Result variables
^^^^^^^^^^^^^^^^
This module will set the following variables if found:
``LIBEI_INCLUDE_DIRS``
  where to find libei.h, etc.
``LIBEI_LIBRARIES``
  the libraries to link against to use libei.
``LIBEI_FOUND``
  TRUE if found
#]=======================================================================]

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_Libei QUIET libei-1.0)
endif()

find_path(Libei_INCLUDE_DIR NAMES libei.h
  HINTS
    ${PC_Libei_INCLUDE_DIRS}
)
mark_as_advanced(Libei_INCLUDE_DIR)

find_library(Libei_LIBRARY NAMES ei
  HINTS
    ${PC_Libei_LIBRARY_DIRS}
)
mark_as_advanced(Libei_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libei
  REQUIRED_VARS
    Libei_LIBRARY Libei_INCLUDE_DIR
)

if(Libei_FOUND)
  set(LIBEI_LIBRARIES ${Libei_LIBRARY})
  set(LIBEI_INCLUDE_DIRS ${Libei_INCLUDE_DIR})
endif()
