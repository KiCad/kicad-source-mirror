cmake_minimum_required(VERSION 3.9)

project(fast_float VERSION 8.0.2 LANGUAGES CXX)
set(FASTFLOAT_CXX_STANDARD 11 CACHE STRING "the C++ standard to use for fastfloat")
set(CMAKE_CXX_STANDARD ${FASTFLOAT_CXX_STANDARD})
option(FASTFLOAT_TEST "Enable tests" OFF)

if(FASTFLOAT_TEST)
  enable_testing()
  add_subdirectory(tests)
else(FASTFLOAT_TEST)
  message(STATUS "Tests are disabled. Set FASTFLOAT_TEST to ON to run tests.")
endif(FASTFLOAT_TEST)

option(FASTFLOAT_SANITIZE "Sanitize addresses" OFF)

if (NOT CMAKE_BUILD_TYPE)
  if(FASTFLOAT_SANITIZE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
  else()
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  endif()
endif()

option(FASTFLOAT_INSTALL "Enable install" ON)

if(FASTFLOAT_INSTALL)
  include(GNUInstallDirs)
endif()

add_library(fast_float INTERFACE)


option(FASTFLOAT_BENCHMARKS "Enable benchmarks" OFF)
if(FASTFLOAT_BENCHMARKS)
  add_subdirectory(benchmarks)
else(FASTFLOAT_BENCHMARKS)
  message(STATUS "Benchmarks are disabled. Set FASTFLOAT_BENCHMARKS to ON to build benchmarks (assumes C++17).")
endif(FASTFLOAT_BENCHMARKS)


add_library(FastFloat::fast_float ALIAS fast_float)
target_include_directories(
  fast_float
  INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(fast_float INTERFACE cxx_std_11)
if(FASTFLOAT_SANITIZE)
  target_compile_options(fast_float INTERFACE -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined -fno-sanitize-recover=all)
  target_link_libraries(fast_float INTERFACE -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined -fno-sanitize-recover=all)
  if (CMAKE_COMPILER_IS_GNUCC)
    target_link_libraries(fast_float INTERFACE -fuse-ld=gold)
  endif()
endif()

include(CheckCXXCompilerFlag)
unset(FASTFLOAT_COMPILER_SUPPORTS_PERMISSIVE)
CHECK_CXX_COMPILER_FLAG(/permissive- FASTFLOAT_COMPILER_SUPPORTS_PERMISSIVE)

if(FASTFLOAT_COMPILER_SUPPORTS_PERMISSIVE)
  target_compile_options(fast_float INTERFACE /permissive-)
endif()

if(FASTFLOAT_INSTALL)
  include(CMakePackageConfigHelpers)

  set(FASTFLOAT_VERSION_CONFIG "${CMAKE_CURRENT_BINARY_DIR}/module/FastFloatConfigVersion.cmake")
  set(FASTFLOAT_PROJECT_CONFIG "${CMAKE_CURRENT_BINARY_DIR}/module/FastFloatConfig.cmake")
  set(FASTFLOAT_CONFIG_INSTALL_DIR "${CMAKE_INSTALL_DATAROOTDIR}/cmake/FastFloat")

  if(${CMAKE_VERSION} VERSION_LESS "3.14")
    write_basic_package_version_file("${FASTFLOAT_VERSION_CONFIG}" VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
  else()
    write_basic_package_version_file("${FASTFLOAT_VERSION_CONFIG}" VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion ARCH_INDEPENDENT)
  endif()
  configure_package_config_file("cmake/config.cmake.in"
                                "${FASTFLOAT_PROJECT_CONFIG}"
                                INSTALL_DESTINATION "${FASTFLOAT_CONFIG_INSTALL_DIR}")

  install(DIRECTORY "${PROJECT_SOURCE_DIR}/include/fast_float" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
  install(FILES "${FASTFLOAT_PROJECT_CONFIG}" "${FASTFLOAT_VERSION_CONFIG}" DESTINATION "${FASTFLOAT_CONFIG_INSTALL_DIR}")
  install(EXPORT ${PROJECT_NAME}-targets NAMESPACE FastFloat:: DESTINATION "${FASTFLOAT_CONFIG_INSTALL_DIR}")

  install(TARGETS fast_float
          EXPORT ${PROJECT_NAME}-targets
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
          LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
endif()
