cmake_policy(PUSH)
#cmake_policy(VERSION 3.1)

# Default to honoring the visibility settings for static libraries
cmake_policy(SET CMP0063 NEW)

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Compute the installation prefix relative to this file.
get_filename_component(_IMPORT_PREFIX "${CMAKE_CURRENT_LIST_FILE}" PATH)
if(_IMPORT_PREFIX STREQUAL "/")
  set(_IMPORT_PREFIX "")
endif()

set(TDxWare_INCLUDE_DIR "${_IMPORT_PREFIX}/inc")
set(TDxWare_INCLUDE_DIRS ${TDxWare_INCLUDE_DIR})

# ------------------------------------------------------------------------
#  Add imported targets
# ------------------------------------------------------------------------
if(NOT TARGET 3DxWare::headers)
  add_library(3DxWare::headers INTERFACE IMPORTED)
  if(TDxWare_INCLUDE_DIRS)
    set_target_properties(3DxWare::headers PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${TDxWare_INCLUDE_DIRS}")
  endif()
endif()

if(NOT TARGET TDxNavlib)
  set(NAVLIB_LIB_SRCS
        src/navlib_stub.c
        src/navlib_load.cpp
        )

  add_library(TDxNavlib STATIC ${NAVLIB_LIB_SRCS})
  target_include_directories( TDxNavlib PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/inc" )
  set_target_properties(TDxNavlib PROPERTIES
                        INTERFACE_INCLUDE_DIRECTORIES "${TDxWare_INCLUDE_DIRS}")

  set_target_properties(TDxNavlib PROPERTIES
                        IMPORTED_LINK_INTERFACE_LANGUAGES "CXX")
endif()

if(NOT TARGET 3DxWare::Navlib)
  add_library(3DxWare::Navlib ALIAS TDxNavlib)
endif()

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
cmake_policy(POP)
