# Testing KiCad #

# Unit tests #

KiCad has a limited number of unit tests, which can be used to
check that certain functionality works.

Tests are registered using [CTest][], part of CMake. CTest gathers all the
disparate test programs and runs them. Most C++ unit
tests are written using the [Boost Unit Test framework][], but this is not
required to add a test to the testing suite.

The test CMake targets generally start with `qa_`, the names of the tests
within CTest are the same but without the `qa_` prefix.

## Running tests ##

You can run all tests after building with `make test` or `ctest`. The latter
option allows many CTest options which can be useful, especially in automated
or CI environments.

### Running specific tests ##

To run a specific test executable, you can just run with `ctest` or run
the executable directly. Running directly is often the simplest way when
working on a specific test and you want access to the test executable's
arguments. For example:

    # run the libcommon tests
    cd /path/to/kicad/build
    qa/common/qa_common [parameters]

For Boost unit tests, you can see the options for the test with `<test> --help`.
Common useful patterns:

* `<test> -t "Utf8/*"` runs all tests in the `Utf8` test suite.
* `<test> -t "Utf8/UniIterNull"` runs only a single test in a specific suite.
* `<test> -l all` adds more verbose debugging to the output.
* `<test> --list_content` lists the test suites and test cases within the
    test program. You can use these for arguments to `-l`.

You can rebuild just a specific test with CMake to avoid rebuilding
everything when working on a small area, e.g. `make qa_common`.

### Writing Boost tests ###

Boost unit tests are straightforward to write. Individual test cases can be
registered with:

    BOOST_AUTO_TEST_CASE( SomeTest )
    {
        BOOST_CHECK_EQUAL( 1, 1 );
    }

There is a range of functions like `BOOST_CHECK`, which are documented
[here][boost-test-functions]. Using the most specific function is preferred, as that
allows Boost to provide more detailed failures: `BOOST_CHECK( foo == bar )` only
reports a mismatch, `BOOST_CHECK_EQUAL( foo, bar )` will show the values of
each.

To output debug messages, you can use `BOOST_TEST_MESSAGE` in the unit tests,
which will be visible only if you set the `-l` parameter to `message` or higher.
This colours the text differently to make it stand out from other testing
messages and standard output.

You can also use `std::cout`, `printf`, `wxLogDebug` and so on for debug
messages inside tested functions (i.e. where you don't have access to the Boost
unit test headers). These will always be printed, so take care
to remove them before committing, or they'll show up when KiCad runs normally!

## Python modules ##

The Pcbnew Python modules have some test programs in the `qa` directory.
You must have the `KICAD_SCRIPTING_MODULES` option on in CMake to
build the modules and enable this target.

The main test script is `qa/test.py` and the test units are in
`qa/testcases`. All the test units can by run using `ctest python`, which
runs `test.py`.

You can also run an individual case manually, by making sure the
modules are built, adding them to `PYTHONPATH` and running the test
from the source tree:

    make pcbnew_python_module
    export PYTHONPATH=/path/to/kicad/build/pcbnew
    cd /path/to/kicad/source/qa
    python2 testcase/test_001_pcb_load.py

### Diagnosing segfaults ###

Although the module is Python, it links against a C++ library
(the same one used by KiCad Pcbnew), so it can segfault if the library
has a defect.

You can run the tests in GDB to trace this:

    $ gdb

    (gdb) file python2
    (gdb) run testcases/test_001_pcb_load.py

If the test segfaults, you will get a familiar backtrace, just like
if you were running pcbnew under GDB.

## Fuzz testing ##

It is possible to run fuzz testing on some parts of KiCad. To do this for a
generic function, you need to be able to pass some kind of input from the fuzz
testing tool to the function under test.

For example, to use the [AFL fuzzing tool][], you will need:

* A test executable that can:
** Receive input from `stdin` to be run by `afl-fuzz`.
** Optional: process input from a filename to allow `afl-tmin` to minimise the
   input files.
* To compile this executable with an AFL compiler, to enable the instrumentation
  that allows the fuzzer to detect the fuzzing state.

For example, the `qa_pcb_parse_input` executable can be compiled like this:

    mkdir build
    cd build
    cmake -DCMAKE_CXX_COMPILER=/usr/bin/afl-clang-fast++ -DCMAKE_C_COMPILER=/usr/bin/afl-clang-fast ../kicad_src
    make qa_pcb_parse_input

You may need to disable core dumps and CPU frequency scaling on your system (AFL
will warn you if you should do this). For example, as root:

    # echo core >/proc/sys/kernel/core_pattern
    # echo performance | tee cpu*/cpufreq/scaling_governor

To fuzz:

    afl-fuzz -i fuzzin -o fuzzout -m500 qa/pcb_parse_input/qa_pcb_parse_input

where:

* `-i` is a directory of files to use as fuzz input "seeds"
* `-o` is a directory to write the results (including inputs that provoke crashes
  or hangs)
* `-t` is the maximum time that a run is allowed to take before being declared a "hang"
* `-m` is the memory allowed to use (this often needs to be bumped, as KiCad code
  tends to use a lot of memory to initialise)

The AFL TUI will then display the fuzzing progress, and you can use the hang- or
crash-provoking inputs to debug code as needed.

[CTest]: https://cmake.org/cmake/help/latest/module/CTest.html
[Boost Unit Test framework]: https://www.boost.org/doc/libs/1_68_0/libs/test/doc/html/index.html
[boost-test-functions]: https://www.boost.org/doc/libs/1_68_0/libs/test/doc/html/boost_test/utf_reference/testing_tool_ref.html
[AFL fuzzing tool]: http://lcamtuf.coredump.cx/afl/