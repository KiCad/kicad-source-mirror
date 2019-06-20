# Building KiCad from Source #
If you are a user and not a developer, please consider using one of the prebuilt packages
of KiCad which can be found at the [download][] page on the [KiCad website][].  Building KiCad
from source is not for the faint of heart and is not recommended unless you have reasonable
software development experience.  This document contains the instructions on how to build KiCad
from source on the supported platforms.  It is not intended as a guide for installing or building
[library dependencies](#library_dependencies).  Please consult your platforms documentation for
installing packages or the source code when building the library dependencies.  Currently the
supported platforms are Windows Versions 7-10, just about any version of Linux, and macOS
10.9-10.13.  You may be able to build KiCad on other platforms but it is not supported.  On
Windows and Linux the [GNU GCC][] is the only supported compiler and on macOS [Clang][] is the
only supported compiler.

[TOC]

# Development Tools # {#development_tools}

Before you begin building KiCad, there are a few tools required in addition to your compiler.
Some of these tools are required to build from source and some are optional.

## CMake Build Configuration Tool ## {#cmake}

[CMake][] is the build configuration and makefile generation tool used by KiCad.  It is required.


## GIT Version Control System ## {#git}

The official source code repository is hosted on [Launchpad][] and requires [git][] to get
the latest source. If you prefer to use [GitHub][] there is a read only mirror of the official
KiCad repository. Do not submit pull requests to GitHub. Changes should be sent to the KiCad
developer's [mailing list][] using `git format-patch` and attaching the patch with [PATCH] at
the beginning of the subject or using `git send-email` to send your commit directly to the
developer's [mailing list][].

## Doxygen Code Documentation Generator ## {#doxygen_section}

The KiCad source code is documented using [Doxygen][] which parses the KiCad source code files
and builds a dependency tree along with the source documentation into HTML.  Doxygen is only
required if you are going to build the KiCad documentation.

## SWIG Simplified Wrapper and Interface Generator ## {#swig}

[SWIG][] is used to generate the Python scripting language extensions for KiCad.  SWIG is not
required if you are not going to build the KiCad scripting extension.


# Library Dependencies # {#library_dependencies}

This section includes a list of library dependencies required to build KiCad.  It does not
include any dependencies of the libraries.  Please consult the library's documentation for any
additional dependencies.  Some of these libraries are optional depending on you build
configuration.  This is not a guide on how to install the library dependencies using you systems
package management tools or how to build the library from source.  Consult the appropriate
documentation to perform these tasks.

## wxWidgets Cross Platform GUI Library## {#wxwidgets}

[wxWidgets][] is the graphical user interface (GUI) library used by KiCad.  The current minimum
version is 3.0.0.  However, 3.0.2 should be used whenever possible as there are some known bugs
in prior versions that can cause problems on some platforms.  Please note that there are also
some platform specific patches that must be applied before building wxWidgets from source.  These
patches can be found in the [patches folder][] in the KiCad source.  These patches are named by
the wxWidgets version and platform name they should be applied against.  wxWidgets must be built
with the --with-opengl option.  If you installed the packaged version of wxWidgets on your system,
verify that it was built with this option.

## Boost C++ Libraries ## {#boost}

The [Boost][] C++ library is required only if you intend to build KiCad with the system installed
version of Boost instead of the default internally built version.  If you use the system installed
version of Boost, version 1.56 or greater is required.  Please note there are some platform
specific patches required to build a working Boost library.  These patches can be found in the
[patches folder][] in the KiCad source.  These patches are named by the platform name they should
be applied against.

## GLEW OpenGL Extension Wrangler Library ## {#glew}

The [OpenGL Extension Wrangler][GLEW] is an OpenGL helper library used by the KiCad graphics
abstraction library [GAL] and is always required to build KiCad.

## GLM OpenGL Mathematics Library  ## {#glm}

The [OpenGL Mathematics Library][GLM] is an OpenGL helper library used by the KiCad graphics
abstraction library [GAL] and is always required to build KiCad.

## GLUT OpenGL Utility Toolkit Library ## {#glut}

The [OpenGL Utility Toolkit][GLUT] is an OpenGL helper library used by the KiCad graphics
abstraction library [GAL] and is always required to build KiCad.

## Cairo 2D Graphics Library ## {#cairo}

The [Cairo][] 2D graphics library is used as a fallback rendering canvas when OpenGL is not
available and is always required to build KiCad.

## Python Programming Language ## {#python}

The [Python][] programming language is used to provide scripting support to KiCad.  It needs
to be installed unless the [KiCad scripting](#kicad_scripting) build configuration option is
disabled.

## wxPython Library ## {#wxpython}

The [wxPython][] library is used to provide a scripting console for Pcbnew.  It needs to be
installed unless the [wxPython scripting](#wxpython_scripting) build configuration option is
disabled.  When building KiCad with wxPython support, make sure the version of the wxWidgets
library and the version of wxPython installed on your system are the same.  Mismatched versions
have been known to cause runtime issues.

## Curl Multi-Protocol File Transfer Library ## {#curl}

The [Curl Multi-Protocol File Transfer Library][libcurl] is used to provide secure internet
file transfer access for the [GitHub][] plug in.  This library needs to be installed unless
the GitHub plug build option is disabled.

## OpenCascade Library ## {#oce}

The [OpenCascade Community Edition (OCE)][liboce] is used to provide support for loading and saving
3D model file formats such as STEP.  This library needs to be installed unless the OCE build
option is disabled.

[Open CASCSADE Technology (OCC)][libocc] should also work as an alternative to OCE. Selection of
library Cascade library can be specified at build time.  See the [STEP/IGES support](#oce_opt)
section.

## Ngspice Library ## {#ngspice}

The [Ngspice Library][libngspice] is used to provide Spice simulation support in the schematic
editor.  Make sure the the version of ngspice library used was built with the--with-ngshared
option.  This library needs to be installed unless the Spice build option is disabled.

# KiCad Build Configuration Options # {#build_opts}

KiCad has many build options that can be configured to build different options depending on
the availability of support for each option on a given platform.  This section documents
these options and their default values.

## Scripting Support ## {#scripting_opt}

The KICAD_SCRIPTING option is used to enable building the Python scripting support into Pcbnew.
This options is enabled by default, and will disable all other KICAD_SCRIPTING_* options when
it is disabled.

## Python 3 Scripting Support ## {#python3}

The KICAD_SCRIPTING_PYTHON3 option is used to enable using Python 3 for the scripting support
instead of Python 2.  This option is disabled by default and only is relevant if
[KICAD_SCRIPTING](#scripting_opt) is enabled.

## Scripting Module Support ## {#scripting_mod_opt}

The KICAD_SCRIPTING_MODULES option is used to enable building and installing the Python modules
supplied by KiCad.  This option is enabled by default, but will be disabled if
[KICAD_SCRIPTING](#scripting_opt) is disabled.

## wxPython Scripting Support ## {#wxpython_opt}

The KICAD_SCRIPTING_WXPYTHON option is used to enable building the wxPython interface into
Pcbnew including the wxPython console.  This option is enabled by default, but will be disabled if
[KICAD_SCRIPTING](#scripting_opt) is disabled.

## wxPython Phoenix Scripting Support ## {#wxpython_phoenix}

The KICAD_SCRIPTING_WXPYTHON_PHOENIX option is used to enable building the wxPython interface with
the new Phoenix binding instead of the legacy one.  This option is disabled by default, and
enabling it requires [KICAD_SCRIPTING](#scripting_opt) to be enabled.

## Python Scripting Action Menu Support ## {#python_action_menu_opt}

The KICAD_SCRIPTING_ACTION_MENU option allows Python scripts to be added directly to the Pcbnew
menu.  This option is enabled by default, but will be disabled if
[KICAD_SCRIPTING](#scripting_opt) is disabled.  Please note that this option is highly
experimental and can cause Pcbnew to crash if Python scripts create an invalid object state
within Pcbnew.

## GitHub Plugin ## {#github_opt}

The BUILD_GITHUB_PLUGIN option is used to control if the GitHub plug in is built.  This option is
enabled by default.

## Integrated Spice simulator ## {#spice_opt}

The KICAD_SPICE option is used to control if the Spice simulator interface for Eeschema is
built.  When this option is enabled, it requires [ngspice][] to be available as a shared
library.  This option is enabled by default.

## STEP/IGES support for the 3D viewer ## {#oce_opt}

The KICAD_USE_OCE is used for the 3D viewer plugin to support STEP and IGES 3D models. Build tools
and plugins related to OpenCascade Community Edition (OCE) are enabled with this option. When
enabled it requires [liboce][] to be available, and the location of the installed OCE library to be
passed via the OCE_DIR flag.  This option is enabled by default.

Alternatively KICAD_USE_OCC can be used instead of OCE. Both options are not supposed to be enabled
at the same time.

## Demos and Examples ## {#demo_install_opt}

The KiCad source code includes some demos and examples to showcase the program. You can choose
whether install them or not with the KICAD_INSTALL_DEMOS option. You can also select where to
install them with the KICAD_DEMOS variable. On Linux the demos are installed in
$PREFIX/share/kicad/demos by default.

## Quality assurance (QA) unit tests ## {#quality_assurance_tests_opt}

The KICAD_BUILD_QA_TESTS option allows building unit tests binaries for quality assurance as part
of the default build. This option is enabled by default.

If this option is disabled, the QA binaries can still be built by manually specifying the target.
For example, with `make`:

* Build all QA binaries: `make qa_all`
* Build a specific test: `make qa_pcbnew`
* Build all unit tests: `make qa_all_tests`
* Build all test tool binaries: `make qa_all_tools`

For more information about testing KiCad, see [this page](testing.md).

## KiCad Build Version ## {#build_version_opt}

The KiCad version string is defined by the output of `git describe --dirty` when git is available
or the version string defined in CMakeModules/KiCadVersion.cmake with the value of
KICAD_VERSION_EXTRA appended to the former.  If the KICAD_VERSION_EXTRA variable is not define,
it is not appended to the version string.  If the KICAD_VERSION_EXTRA  variable is defined it
is appended along with a leading '-' to the full version string as follows:

    (KICAD_VERSION[-KICAD_VERSION_EXTRA])

The build script automatically creates the version string information from the [git][] repository
information as follows:

    (5.0.0-rc2-dev-100-g5a33f0960)
     |
     output of `git describe --dirty` if git is available.


## KiCad Config Directory ## {#config_dir_opt}

The default KiCad configuration directory is `kicad`.  On Linux this is located at
`~/.config/kicad`, on MSW, this is `C:\Documents and Settings\username\Application Data\kicad` and
on MacOS, this is `~/Library/Preferences/kicad`.  If the installation package would like to, it may
specify an alternate configuration name instead of `kicad`.  This may be useful for versioning
the configuration parameters and allowing the use of, e.g. `kicad5` and `kicad6` concurrently without
losing configuration data.

This is set by specifying the KICAD_CONFIG_DIR string at compile time.

# Getting the KiCad Source Code ## {#getting_src}

There are several ways to get the KiCad source.  If you want to build the stable version you
can down load the source archive from the [KiCad Launchpad][] developers page.  Use tar or some
other archive program to extract the source on your system.  If you are using tar, use the
following command:

    tar -xaf kicad_src_archive.tar.xz

If you are contributing directly to the KiCad project on Launchpad, you can create a local
copy on your machine by using the following command:

    git clone -b master https://git.launchpad.net/kicad

Here is a list of source links:

Stable release archive: https://launchpad.net/kicad/5.0/5.0.2/+download/kicad-5.0.2.tar.xz

Development branch: https://code.launchpad.net/~kicad-product-committers/kicad/+git/product-git/+ref/master

GitHub mirror: https://github.com/KiCad/kicad-source-mirror

# Building KiCad on Linux # {#build_linux}

To perform a full build on Linux, run the following commands:

    cd <your kicad source mirror>
    mkdir -p build/release
    mkdir build/debug               # Optional for debug build.
    cd build/release
    cmake -DCMAKE_BUILD_TYPE=Release \
          ../../
    make
    sudo make install

If the CMake configuration fails, determine the missing dependencies and install them on your
system.  By default, CMake sets the install path on Linux to /usr/local.  Use the
CMAKE_INSTALL_PREFIX option to specify a different install path.

# Building KiCad on Windows # {#build_windows}

The preferred Windows build environment is [MSYS2][].  The [MinGW][] build environment is still
supported but it is not recommended because the developer is responsible for building *all* of
the dependencies from source which is a huge and frustrating undertaking.  The [MSYS2][] project
provides packages for all of the require dependencies to build KiCad.  To setup the [MSYS2][]
build environment, depending on your system download and run either the [MSYS2 32-bit Installer][]
or the [MSYS2 64-bit Installer][].  After the installer is finished, update to the latest
package versions by running the `msys2_shell.cmd` file located in the MSYS2 install path and
running the command `pacman -Syu`.  If the msys2-runtime package is updated, close the shell
and run `msys2_shell.cmd`.

## Building using MSYS2 ## {#msys2_build}

The following commands assume you are building for 64-bit Windows, and that you already have
the KiCad source code in a folder called `kicad-source` in your home directory.  See below
for changes if you need to build for 32-bit instead.  Run `mingw64.exe` from the MSYS2
install path. At the command prompt run the the following commands:

    pacman -S base-devel \
              git \
              mingw-w64-x86_64-cmake \
              mingw-w64-x86_64-doxygen \
              mingw-w64-x86_64-gcc \
              mingw-w64-x86_64-python2 \
              mingw-w64-x86_64-pkg-config \
              mingw-w64-x86_64-swig \
              mingw-w64-x86_64-boost \
              mingw-w64-x86_64-cairo \
              mingw-w64-x86_64-glew \
              mingw-w64-x86_64-curl \
              mingw-w64-x86_64-wxPython \
              mingw-w64-x86_64-wxWidgets \
              mingw-w64-x86_64-toolchain \
              mingw-w64-x86_64-glm \
              mingw-w64-x86_64-oce \
              mingw-w64-x86_64-ngspice
    cd kicad-source
    mkdir -p build/release
    mkdir build/debug               # Optional for debug build.
    cd build/release
    cmake -DCMAKE_BUILD_TYPE=Release \
          -G "MSYS Makefiles" \
          -DCMAKE_PREFIX_PATH=/mingw64 \
          -DCMAKE_INSTALL_PREFIX=/mingw64 \
          -DDEFAULT_INSTALL_PATH=/mingw64 \
          ../../
    make install

For 32-bit builds, run `mingw32.exe` and change `x86_64` to `i686` in the package names and
change the paths in the cmake configuration from `/mingw64` to `/mingw32`.

For debug builds, run the cmake command with `-DCMAKE_BUILD_TYPE=Debug` from the `build/debug` folder.

## Known MSYS2 Build Issues ## {#known_issues_msys2}

There are some known issues that are specific to MSYS2.  This section provides a list of the
currently known issues when building KiCad using MSYS2.


### 64-bit Package of Boost 1.59 ### {#ki_msys2_64bit_boost}

The context library of the x86_64 package of Boost version 1.59 is broken and will cause KiCad
to crash.  You must downgrade to version 1.57 by running the command:

    pacman -U /var/cache/pacman/pkg/mingw-w64-x86_64-boost-1.57.0-4-any.pkg.tar.xz

If the file mingw-w64-x86_64-boost-1.57.0-4-any.pkg.tar.xz is no longer in your pacman cache,
you will have to download it from the [MSYS2 64-bit SourceForge repo][].  You should also
configure pacman to prevent upgrading the 64-bit Boost package by adding:

    IgnorePkg = mingw-w64-x86_64-boost

to your /etc/pacman.conf file.

### Building with Boost 1.70 ### {#ki_msys2_boost_1_70}

There is an issue building KiCad with Boost version 1.70 due to CMake not defining the proper
link libraries during configuration.  Boost 1.70 can be used but `-DBoost_NO_BOOST_CMAKE=ON`
needs to be added during CMake configuration to insure the link libraries are properly generated.

### Building OCE from source

KiCad requires OCE by default, and the version installed by `pacman` can cause build errors in
x86_64 systems as of March 2018.  In order to work around this, you can build OCE from source on
these systems.  Building OCE on Windows requires that you place the source code in a very short
directory path, otherwise you will run into errors caused by the maximum path length on Windows.
In the example below, the `MINGW-packages` repository is cloned to `/c/mwp`, which is equivalent to
`C:\mwp` in Windows path terminology.  You may wish to change the destination of the `git clone`
command if you do not want to place it on the root of your C drive, but if you run in to strange
compilation errors about missing files, it is probably because your path is too long.

    git clone https://github.com/Alexpux/MINGW-packages /c/mwp
    cd /c/mwp/mingw-w64-oce
    makepkg-mingw -is

# Building KiCad on macOS # {#build_osx}

As of V5, building and packaging for macOS can be done using [kicad-mac-builder][],
which downloads, patches, builds, and packages for macOS.  It is used to create the official
releases and nightlies, and it reduces the complexity of setting up a build environment to a command
or two.  Usage of kicad-mac-builder is detailed at on its website.

If you wish to build without kicad-mac-builder, please use the following and its source code
as reference. Building on macOS requires building dependency libraries that require patching
in order to work correctly.

In the following set of commands, replace the macOS version number (i.e. 10.11) with the desired
minimum version.  It may be easiest to build for the same version you are running.

KiCad currently won't work with a stock version of wxWidgets that can be downloaded or
installed by package managers like MacPorts or Homebrew. To avoid having to deal with
patches a [KiCad fork of wxWidgets][] is being maintained on GitHub. All the needed patches
and some other fixes/improvements are contained in the `kicad/macos-wx-3.0` branch.

To perform a wxWidgets build, execute the following commands:

    cd <your wxWidgets build folder>
    git clone -b kicad/macos-wx-3.0 https://github.com/KiCad/wxWidgets
    mkdir wx-build
    cd wx-build
    ../wxWidgets/configure \
        --prefix=`pwd`/../wx-bin \
        --with-opengl \
        --enable-aui \
        --enable-utf8 \
        --enable-html \
        --enable-stl \
        --with-libjpeg=builtin \
        --with-libpng=builtin \
        --with-regex=builtin \
        --with-libtiff=builtin \
        --with-zlib=builtin \
        --with-expat=builtin \
        --without-liblzma \
        --with-macosx-version-min=10.11 \
        --enable-universal-binary=i386,x86_64 \
        CC=clang \
        CXX=clang++
    make
    make install

If everything works you will find the wxWidgets binaries in `<your wxWidgets build folder>/wx-bin`.
Now, build a basic KiCad without Python scripting using the following commands:

    cd <your kicad source mirror>
    mkdir -p build/release
    mkdir build/debug               # Optional for debug build.
    cd build/release
    cmake -DCMAKE_C_COMPILER=clang \
          -DCMAKE_CXX_COMPILER=clang++ \
          -DCMAKE_OSX_DEPLOYMENT_TARGET=10.11 \
          -DwxWidgets_CONFIG_EXECUTABLE=<your wxWidgets build folder>/wx-bin/bin/wx-config \
          -DKICAD_SCRIPTING=OFF \
          -DKICAD_SCRIPTING_MODULES=OFF \
          -DKICAD_SCRIPTING_WXPYTHON=OFF \
          -DCMAKE_INSTALL_PREFIX=../bin \
          -DCMAKE_BUILD_TYPE=Release \
          ../../
    make
    make install

If the CMake configuration fails, determine the missing dependencies and install them on your
system or disable the corresponding KiCad feature. If everything works you will get self-contained
application bundles in the `build/bin` folder.

Building KiCad with Python scripting is more complex and won't be covered in detail here.
You will have to build wxPython against the wxWidgets source of the KiCad fork - a stock wxWidgets
that might be bundled with the wxPython package won't work. Please see wxPython documentation
or [macOS bundle build scripts][] (`compile_wx.sh`) on how to do this. Then, use a CMake
configuration as follows to point it to your own wxWidgets/wxPython:

    cmake -DCMAKE_C_COMPILER=clang \
          -DCMAKE_CXX_COMPILER=clang++ \
          -DCMAKE_OSX_DEPLOYMENT_TARGET=10.9 \
          -DwxWidgets_CONFIG_EXECUTABLE=<your wxWidgets build folder>/wx-bin/bin/wx-config \
          -DPYTHON_EXECUTABLE=<path-to-python-exe>/python \
          -DPYTHON_SITE_PACKAGE_PATH=<your wxWidgets build folder>/wx-bin/lib/python2.7/site-packages \
          -DCMAKE_INSTALL_PREFIX=../bin \
          -DCMAKE_BUILD_TYPE=Release \
          ../../

# Known Issues # {#known_issues}

There are some known issues that effect all platforms.  This section provides a list of the
currently known issues when building KiCad on any platform.

## Boost C++ Library Issues ## {#boost_issue}

As of version 5 of [GNU GCC][], using the default configuration of downloading, patching, and
building of Boost 1.54 will cause the KiCad build to fail.  Therefore a newer version of Boost
must be used to build KiCad.  If your system has Boost 1.56 or greater installed, you job is
straight forward.  If your system does not have Boost 1.56 or greater installed, you will have
to download and [build Boost][] from source.  If you are building Boost on windows using [MinGW][]
you will have to apply the Boost patches in the KiCad source [patches folder][].


[download]: http://kicad-pcb.org/download/
[KiCad website]: http://kicad-pcb.org/
[KiCad Launchpad]: https://launchpad.net/kicad
[GNU GCC]: https://gcc.gnu.org/
[Clang]: http://clang.llvm.org/
[CMake]: https://cmake.org/
[Launchpad]: https://code.launchpad.net/kicad/
[GIT]: https://git-scm.com/
[GitHub]: https://github.com/KiCad/kicad-source-mirror
[ngspice]: http://ngspice.sourceforge.net/
[Doxygen]: http://www.doxygen.nl
[mailing list]: https://launchpad.net/~kicad-developers
[SWIG]: http://www.swig.org/
[wxWidgets]: http://wxwidgets.org/
[patches folder]: http://bazaar.launchpad.net/~kicad-product-committers/kicad/product/files/head:/patches/
[Boost]: http://www.boost.org/
[GLEW]: http://glew.sourceforge.net/
[GLUT]: https://www.opengl.org/resources/libraries/glut/
[Cairo]: http://cairographics.org/
[Python]: https://www.python.org/
[wxPython]: http://wxpython.org/
[MSYS2]: http://www.msys2.org/
[MSYS2 32-bit Installer]: http://repo.msys2.org/distrib/i686/msys2-i686-20161025.exe
[MSYS2 64-bit Installer]: http://repo.msys2.org/distrib/x86_64/msys2-x86_64-20161025.exe
[PKGBUILD]: https://github.com/Alexpux/MINGW-packages/blob/master/mingw-w64-kicad-git/PKGBUILD
[kicad-mac-builder]:https://github.com/KiCad/kicad-mac-builder
[KiCad fork of wxWidgets]:https://github.com/KiCad/wxWidgets
[MinGW]: http://mingw.org/
[build Boost]: http://www.boost.org/doc/libs/1_59_0/more/getting_started/index.html
[MSYS2 64-bit SourceForge repo]: http://sourceforge.net/projects/msys2/files/REPOS/MINGW/x86_64/
[libcurl]: http://curl.haxx.se/libcurl/
[GLM]: http://glm.g-truc.net/
[git]: https://git-scm.com/
[liboce]: https://github.com/tpaviot/oce
[libocc]: https://www.opencascade.com/content/overview
[libngspice]: https://sourceforge.net/projects/ngspice/
