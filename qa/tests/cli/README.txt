To run these tests manually, you may need to set some or all of
the following environment variables:

# Replace with the path to kicad-cli in your build environment
KICAD_CLI=/path/to/kicad/cmake-build-debug/kicad/KiCad.app/Contents/MacOS/kicad-cli
KICAD_RUN_FROM_BUILD_DIR=1

On macOS you will also need to take the following steps if you want to
test kicad-cli from the build dir:

1) Symlink the contents of the folder:

    /path/to/kicad-mac-builder/build/python-dest/Library/Frameworks/Python.framework/Versions/3.9/

   from your kicad-mac-builder folder into the folder:

   /path/to/kicad/cmake-build-debug/kicad/KiCad.app/Contents/Frameworks/Python.framework/Versions/3.9/

   inside your KiCad build directory.

2) Set the following environment variable so that the Python dll is found:

   DYLD_LIBRARY_PATH=/path/to/kicad-mac-builder/build/python-dest/
