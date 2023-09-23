#cmake_minimum_required(VERSION 3.0.0)
project(nanodbc CXX)

# Default to honoring the visibility settings for static libraries
cmake_policy(SET CMP0063 NEW)

# nanodbc specific options
option(NANODBC_DISABLE_ASYNC "Disable async features entirely" OFF)
option(NANODBC_DISABLE_LIBCXX "Do not use libc++, if available." OFF)
option(NANODBC_ENABLE_BOOST "Use Boost for Unicode string convertions (requires Boost.Locale)" ON)
option(NANODBC_ENABLE_UNICODE "Enable Unicode support" ON)
option(NANODBC_ENABLE_WORKAROUND_NODATA "Enable SQL_NO_DATA workaround (see Issue #33)" OFF)

########################################
## nanodbc version
########################################
file(STRINGS VERSION.txt NANODBC_VERSION REGEX "[0-9]+\\.[0-9]+\\.[0-9]+")
string(REGEX REPLACE "^([0-9]+)\\.[0-9]+\\.[0-9]+" "\\1" NANODBC_VERSION_MAJOR "${NANODBC_VERSION}")
string(REGEX REPLACE "^[0-9]+\\.([0-9])+\\.[0-9]+" "\\1" NANODBC_VERSION_MINOR "${NANODBC_VERSION}")
string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+)" "\\1" NANODBC_VERSION_PATCH "${NANODBC_VERSION}")
message(STATUS "nanodbc version: ${NANODBC_VERSION}")

########################################
## require and enable C++0x/11/14
########################################
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
message(STATUS "nanodbc compile: C++${CMAKE_CXX_STANDARD}")

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_COMPILER_IS_GNUCXX)
  include(CheckCXXCompilerFlag)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Intel")
    string(REGEX REPLACE "[/-]W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    if (NOT (CMAKE_VERSION VERSION_LESS 3.6.0)) # Compiler features for Intel in CMake 3.6+
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Qstd=c++17")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /QaxCORE-AVX2")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:precise")
    set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG}   /Od")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O3")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Qipo")
elseif(MSVC)
  string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  if(MSVC_VERSION LESS 1700)
    message(FATAL_ERROR, "nanodbc requires C++11-compliant compiler")
  endif()
endif()

########################################
## nanodbc features
########################################
IF(NOT DEFINED NANODBC_ODBC_VERSION)
  message(STATUS "nanodbc feature: ODBC Version Override - OFF")
else()
  message(STATUS "nanodbc feature: ODBC Version Override - ${NANODBC_ODBC_VERSION}")
  add_definitions(-DNANODBC_ODBC_VERSION=${NANODBC_ODBC_VERSION})
endif()

if(NANODBC_DISABLE_ASYNC)
  add_definitions(-DNANODBC_DISABLE_ASYNC)
endif()
message(STATUS "nanodbc feature: Disable async features - ${NANODBC_DISABLE_ASYNC}")

if(NANODBC_ENABLE_UNICODE)
  add_compile_definitions(NANODBC_ENABLE_UNICODE)
  if(MSVC)
    # Sets "Use Unicode Character Set" property in Visual Studio projects
    add_compile_definitions(UNICODE _UNICODE)
  endif()
endif()
message(STATUS "nanodbc feature: Enable Unicode - ${NANODBC_ENABLE_UNICODE}")

if(NANODBC_ENABLE_BOOST)
  add_definitions(-DNANODBC_ENABLE_BOOST)
endif()
message(STATUS "nanodbc feature: Enable Boost - ${NANODBC_ENABLE_BOOST}")

if(NANODBC_ENABLE_WORKAROUND_NODATA)
  add_definitions(-DNANODBC_ENABLE_WORKAROUND_NODATA)
endif()
message(STATUS "nanodbc feature: Enable SQL_NO_DATA bug workaround - ${NANODBC_ENABLE_WORKAROUND_NODATA}")

########################################
## find unixODBC or iODBC config binary
########################################
if(UNIX)
  # Try to find unixODBC first via odbc_config program.
  find_program(ODBC_CONFIG odbc_config
    PATHS $ENV{ODBC_PATH}/bin /usr/bin /usr/local/bin)
  if(ODBC_CONFIG)
    message(STATUS "nanodbc build: ODBC on Unix - unixODBC")
    set(ODBCLIB odbc)
    execute_process(COMMAND ${ODBC_CONFIG} --include-prefix
      OUTPUT_VARIABLE ODBC_INCLUDE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(ODBC_CFLAGS "-I${ODBC_INCLUDE_DIR}")
    set(CMAKE_FLAGS "${CMAKE_FLAGS} ${ODBC_CFLAGS}")
    execute_process(COMMAND ${ODBC_CONFIG} --libs
      OUTPUT_VARIABLE ODBC_LINK_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()

  # Fallback to finding unixODBC via install paths
  if(NOT ODBC_CONFIG)
    find_path(UnixODBC_INCLUDE_DIR uodbc_stats.h
      /usr/include
      /usr/local/include
      /usr/include/odbc
      /usr/local/include/odbc
      /usr/include/libodbc
      /usr/local/include/libodbc)
    if(UnixODBC_INCLUDE_DIR)
      set(ODBC_CONFIG 1)
      message(STATUS "nanodbc build: ODBC on Unix - unixODBC")
      set(ODBCLIB odbc)
      set(ODBC_CFLAGS "-I${UnixODBC_INCLUDE_DIR} -DHAVE_UNISTD_H -DHAVE_PWD_H -DHAVE_SYS_TYPES_H -DHAVE_LONG_LONG -DSIZEOF_LONG_INT=8")
    endif()
  endif()

  # Fallback to using iODBC
  if(NOT ODBC_CONFIG)
    find_program(ODBC_CONFIG iodbc-config
      PATHS $ENV{ODBC_PATH}/bin /usr/bin /usr/local/bin)
    if(ODBC_CONFIG)
      message(STATUS "nanodbc build: ODBC on Unix - iODBC")
      set(ODBCLIB iodbc)
      execute_process(COMMAND ${ODBC_CONFIG} --cflags
        OUTPUT_VARIABLE ODBC_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
      set(CMAKE_FLAGS "${CMAKE_FLAGS} ${ODBC_CFLAGS}")
      execute_process(COMMAND ${ODBC_CONFIG} --libs
        OUTPUT_VARIABLE ODBC_LINK_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
      if(NANODBC_ENABLE_UNICODE)
        add_definitions(-DNANODBC_USE_IODBC_WIDE_STRINGS)
      endif()
    endif()
  endif()

  if(NOT ODBC_CONFIG)
    message(FATAL_ERROR "can not find a suitable odbc driver manager. Please install either 'unixODBC' or 'iODBC'.")
  endif()

  message(STATUS "ODBC compile flags: ${ODBC_CFLAGS}")
  message(STATUS "ODBC link flags: ${ODBC_LINK_FLAGS}")
endif()

########################################
## find ODBC libraries to link
########################################
if(UNIX)
  set(ODBC_LIBRARIES ${ODBCLIB})
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${ODBC_LINK_FLAGS}")
elseif(MSVC OR CMAKE_CXX_COMPILER_ID MATCHES "Intel")
  set(ODBC_LIBRARIES odbc32.lib odbccp32.lib Ws2_32.lib)
elseif(MINGW)
  set(ODBC_LIBRARIES odbc32 odbccp32)
endif()

########################################
## Mac OS X specifics for targets
########################################
if(APPLE)
  set(CMAKE_MACOSX_RPATH ON)
  message(STATUS "Use rpaths on Mac OS X - ${CMAKE_MACOSX_RPATH}")

  # AppleClang complains of unused `-I/path/` arguments.
  # These are harmless and can be safely ignored.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-command-line-argument")
endif()

########################################
## library target
########################################
add_library(nanodbc
        STATIC
        nanodbc/nanodbc.cpp
        nanodbc/nanodbc.h)

target_link_libraries(nanodbc Boost::locale ${ODBC_LIBRARIES})

if(APPLE)
  target_link_libraries(nanodbc ${ODBC_LINK_FLAGS})
endif()

target_include_directories(nanodbc SYSTEM
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/nanodbc>) # <prefix>/include/nanodbc

if(UNIX)
  set_target_properties(nanodbc PROPERTIES
    COMPILE_FLAGS "${ODBC_CFLAGS}")
endif()

if(NANODBC_ENABLE_UNICODE)
  add_compile_definitions(NANODBC_ENABLE_UNICODE)
  target_compile_definitions(nanodbc PUBLIC NANODBC_ENABLE_UNICODE)
endif()
