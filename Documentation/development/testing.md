# Testing KiCad #

# Unit tests #

KiCad has a limited number of unit tests, which can be used to
check that certain functionality works.

## Python modules ##

The Pcbnew Python modules have some test programs in the `qa` directory.
You must have the `KICAD_SCRIPTING_MODULES` option on in CMake to
build the modules and enable this target.

The main test script is `qa/test.py` and the test units are in
`qa/testcases`. All the test units can by run using `make qa`, which
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
