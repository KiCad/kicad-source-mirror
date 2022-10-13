cmake_minimum_required(VERSION 3.12.4)

project(argparse
        VERSION 2.9.0
        DESCRIPTION "A single header argument parser for C++17"
        HOMEPAGE_URL "https://github.com/p-ranav/argparse"
        LANGUAGES CXX
)

option(ARGPARSE_BUILD_TESTS OFF)
option(ARGPARSE_LONG_VERSION_ARG_ONLY OFF)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
string(REPLACE "/${CMAKE_LIBRARY_ARCHITECTURE}" "" CMAKE_INSTALL_LIBDIR_ARCHIND "${CMAKE_INSTALL_LIBDIR}")


add_library(argparse INTERFACE)
add_library(argparse::argparse ALIAS argparse)


if (ARGPARSE_LONG_VERSION_ARG_ONLY)
	target_compile_definitions(argparse INTERFACE ARGPARSE_LONG_VERSION_ARG_ONLY=true)
endif ()

target_compile_features(argparse INTERFACE cxx_std_17)
target_include_directories(argparse INTERFACE
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>)

if(ARGPARSE_BUILD_SAMPLES)
  add_subdirectory(samples)
endif()

if(ARGPARSE_BUILD_TESTS)
  add_subdirectory(test)
endif()

## Modified to remove undesirable install steps