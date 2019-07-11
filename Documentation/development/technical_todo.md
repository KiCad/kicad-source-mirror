# Technical To-Do List {#technical_todo}

Some work is blocked for technical reasons, and can only be done once
certain other criteria are met (e.g. library versions).

Because KiCad supports distributions with relatively long support cycles,
many of these workarounds will be required for some time, so they are easily
forgotten.

This section collects these so they can be dealt with when the project's
minimum version for that dependency is met. This kind of code should always be
documented in detail at the place of use, so that it's clear what action should
be taken in future.

[TOC]

# C++ Versions {#todo_cpp_version}

Newer C++ versions include features that make some of our current code unnecessary.

## C++11 {#todo_cpp_11}

This C++ standard version is already used by KiCad, but much code that pre-dates
the version switch exists and can be tidied.

* [`std::auto_ptr`](https://en.cppreference.com/w/cpp/memory/auto_ptr)
  should be changed to [`std::unique_ptr`](https://en.cppreference.com/w/cpp/memory/unique_ptr).
  `auto_ptr` is removed entirely in C++17.

## C++14 {#todo_cpp_14}

Compiler support:

* GCC: complete by GCC 5: [GCC C++14 support](https://gcc.gnu.org/projects/cxx-status.html#cxx14).

Provides:

* [`std::make_unique`](https://en.cppreference.com/w/cpp/memory/unique_ptr/make_unique)
  is available. The "polyfill" in `make_unique.h` is no longer required and can
  be removed.

## C++17 {#todo_cpp_17}

Compiler support:

* GCC: from GCC 5 to 7: [GCC C++17 support](https://gcc.gnu.org/projects/cxx-status.html#cxx17).

Provides:

* `OPT<>` can be replaced with [`std::optional`](https://en.cppreference.com/w/cpp/utility/optional).
  Header `core/optional.h` can be removed and replaced with the built-in `<optional>`.
  Needs [GCC 7][].
* [`std::filesystem`](https://en.cppreference.com/w/cpp/filesystem)
  can replace `boost::filesystem` and the Boost dependency can be dropped entirely
  from the CMake files.

# Compilers {#compilers}

Some compilers have bugs or limitations to C++ standard support.

## GCC {#todo_gcc}

Current versions:

* Debian Stretch: [6.3.0](https://packages.debian.org/stretch/gcc)
* Debian Buster: [8.3.0](https://packages.debian.org/buster/gcc)
* Ubuntu 14.04: [4.8.2](https://packages.ubuntu.com/trusty/gcc)
* Ubuntu 16.04: [5.3.1](https://packages.ubuntu.com/xenial/gcc)
* Ubuntu 18.04: [7.3.0](https://packages.ubuntu.com/bionic/gcc)

### GCC 7 {#todo_gcc_7}

* Fixes [bug #56480](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=56480)
  (Explicit specialization in a different namespace). This
  means the `BOOST_TEST_PRINT_NAMESPACE_OPEN` macro is not required, a namespace
  alias can be used instead.

# 3rd Party Libraries # {#todo_third_party}

There are several places in KiCad where workarounds exist for defects in 3rd
party library limitations or defects.

## Boost ## {#todo_boost}

Current versions:

* Debian Stretch: [1.62](https://packages.debian.org/stretch/libboost-dev)
* Ubuntu 14.04: [1.54](https://packages.ubuntu.com/trusty/libboost-dev)
* Ubuntu 16.04: [1.58](https://packages.ubuntu.com/xenial/libboost-dev)
* Ubuntu 18.04: [1.65](https://packages.ubuntu.com/bionic/libboost-dev)

### 1.59 {#boost_1_59}

* Boost 1.59 brings a major overhaul of the [Boost test][] library.
    * Many "Polyfills" un `unit_test_utils.h` can be removed:
        * `BOOST_TEST_CONTEXT` and `BOOST_TEST_INFO` are available, remove the
          polyfill macros.
        * `boost::unit_test::expected_failures` is available, so can remove the
          `HAVE_EXPECTED_FAILURES` guard macro.
        * `BOOST_TEST_PRINT_NAMESPACE_OPEN` doesn't need to switch the namespace

### 1.64 #### {#todo_boost_1_64}

* [Boost test][]
    * Boost test print helper for `nullptr_t` exists, can remove our polyfill
    * The [`boost_test_print_type`](https://www.boost.org/doc/libs/master/libs/test/doc/html/boost_test/test_output/test_tools_support_for_logging/testing_tool_output_disable.html)
      customisation point exists and the old code using `BOOST_TEST_PRINT_NAMESPACE_OPEN`
      can be converted to use that.

## CMake ### {#todo_cmake}

Current versions:

* Debian Stretch: [3.7.2](https://packages.debian.org/stretch/cmake)
* Ubuntu 14.04: [2.8.12](https://packages.ubuntu.com/trusty/cmake)
* Ubuntu 16.04: [3.5.1](https://packages.ubuntu.com/xenial/cmake)
* Ubuntu 18.04: [3.10.2](https://packages.ubuntu.com/bionic/cmake)

### 3.1 {#todo_cmake_3_1}

* Can remove [CMake policy CMP0025](https://cmake.org/cmake/help/v3.0/policy/CMP0025.html)
  from root CMake

### 3.3 {#todo_cmake_3_3}

* Can remove check for [CMake policy CMP0063](https://cmake.org/cmake/help/git-stage/policy/CMP0063.html)
  entirely (the policy exists as of v3.3 and the default is correct).

### 3.5 {#todo_cmake_3_5}

* Can use `target_link_libraries(foo Boost::boost)` for Boost header-only libraries,
  rather than `${Boost_INCLUDE_DIRS}`.
  C.f. [Github commit](https://github.com/Kitware/CMake/commit/3f9b081f6ee85e0691c36496d989edbe8382589d)

## OpenSSL {#todo_openssl}

Current versions:

* Debian Stretch: [1.0.1](https://packages.debian.org/stretch/openssl)
* Ubuntu 14.04: [1.0.1](https://packages.ubuntu.com/trusty/openssl)
* Ubuntu 16.04: [1.0.2](https://packages.ubuntu.com/xenial/openssl)
* Ubuntu 18.04: [1.1.0](https://packages.ubuntu.com/bionic/openssl)

### 1.1.0 {#todo_openssl_1_1_0}

* Can remove all locking code from `kicad_curl.cpp`. From v1.1.0, this is no
  longer required. C.f. [OpenSSL issue 1260](https://github.com/openssl/openssl/issues/1260)

## wxWidgets {#todo_wx}

Current versions:

* Debian Stretch: [3.0.2](https://packages.debian.org/stretch/wx-common)
* Ubuntu 14.04: [3.0.0](https://packages.ubuntu.com/trusty/wx-common)
* Ubuntu 16.04: [3.0.2](https://packages.ubuntu.com/xenial/wx-common)
* Ubuntu 18.04: [3.0.4](https://packages.ubuntu.com/bionic/wx-common)

### 3.1 {#todo_wx_3_1}

This is a development release, but it brings its own set of features and fixes.
Most of these will not be available in general distributions until v3.2.

* DPI scaling on GTK+ available (also needs GTK+ 3.10).
  C.f. [this commit](https://github.com/wxWidgets/wxWidgets/commit/f95fd11e08482697c3b0c0a9d2ccd661134480ee)
  `dpi_scaling.cpp` should continue to work normally, but the config should
  no longer be required and the scaling should auto-detect.


[Boost test]: https://github.com/boostorg/test
[GCC 7]: https://gcc.gnu.org/gcc-7/changes.html
