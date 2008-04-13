project(kicad)

cmake_minimum_required(VERSION 2.4.6 FATAL_ERROR)

# Path to local CMake modules.
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)

# Command line option to enable or disable building minizip. Minizip
# building is enabled by default. Use -DKICAD_MINZIP=OFF to disable
# building minizip.
option(KICAD_MINIZIP "enable/disable building minizip (default ON)" ON)

# Comment this out if you don't want to build with Python support.
# OPTION(KICAD_PYTHON "enable/disable building with Python support (default OFF)")

# Set flags for GCC.
if(CMAKE_COMPILER_IS_GNUCXX)
    # Set default flags for Release build.
    set(CMAKE_C_FLAGS_RELEASE "-Wall -O2 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-Wall -O2 -DNDEBUG")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-s")

    # Set default flags for Debug build.
    set(CMAKE_C_FLAGS_DEBUG "-Wall -g3 -ggdb3 -DDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "-Wall -g3 -ggdb3 -DDEBUG")
endif(CMAKE_COMPILER_IS_GNUCXX)

# Locations for install targets.
set(KICAD_BIN bin CACHE PATH "Location of KiCad binaries.")

if(UNIX)
    if(APPLE)
    else(APPLE)
        # Like all variables, CMAKE_INSTALL_PREFIX can be over-ridden on the command line.
        set(CMAKE_INSTALL_PREFIX /usr/local CACHE PATH "")
        # Everything without leading / is relative to CMAKE_INSTALL_PREFIX.
        set(KICAD_PLUGINS lib/kicad/plugins CACHE PATH "Location of KiCad plugins.")
        set(KICAD_DATA share/kicad CACHE PATH "Location of KiCad data files.")
        set(KICAD_DOCS share/doc/kicad CACHE PATH "Location of KiCad documentation files.")
    endif(APPLE)
endif(UNIX)

if(WIN32)
    # Like all variables, CMAKE_INSTALL_PREFIX can be over-ridden on the command line.
    set(CMAKE_INSTALL_PREFIX c:/kicad CACHE PATH "")
    # Everything without leading / is relative to CMAKE_INSTALL_PREFIX.
    set(KICAD_PLUGINS ${KICAD_BIN}/plugins CACHE PATH "Location of KiCad plugins.")
    set(KICAD_DATA share CACHE PATH "Location of KiCad data files.")
    set(KICAD_DOCS doc CACHE PATH "Location of KiCad documentation files.")
endif(WIN32)

set(KICAD_DEMOS ${KICAD_DATA}/demos CACHE PATH "Location of KiCad demo files.")
set(KICAD_INTERNAT ${KICAD_DATA}/internat CACHE PATH "Location of KiCad i18n files.")
set(KICAD_TEMPLATE ${KICAD_DATA}/template CACHE PATH "Location of KiCad template files.")

mark_as_advanced(KICAD_BIN KICAD_PLUGINS KICAD_DATA KICAD_DOCS
    KICAD_DEMOS KICAD_INTERNAT KICAD_TEMPLATE)

# Find libraries that are needed to build KiCad.
include(CheckFindPackageResult)

find_package(OpenGL)
check_find_package_result(OPENGL_FOUND "OpenGL")

find_package(Boost)
check_find_package_result(Boost_FOUND "Boost")

# Here you can define what libraries of wxWidgets you need for your
# application. You can figure out what libraries you need here;
# http://www.wxwidgets.org/manuals/2.8/wx_librarieslist.html
set(wxWidgets_USE_LIBS base core adv gl html net)
find_package(wxWidgetsCVS)
check_find_package_result(wxWidgets_FOUND "wxWidgets")

# Include wxWidgets macros.
include(${wxWidgets_USE_FILE})

# Include paths.
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/share)

# CMake will look in these dirs for nested 'CMakeLists.txt' files.
# Binaries.
add_subdirectory(3d-viewer)
add_subdirectory(common)
add_subdirectory(cvpcb)
add_subdirectory(eeschema)
add_subdirectory(gerbview)
add_subdirectory(kicad)
add_subdirectory(pcbnew)
add_subdirectory(polygon)
# Resources.
add_subdirectory(demos)
add_subdirectory(internat)
add_subdirectory(template)

install(FILES install.txt DESTINATION ${KICAD_DOCS}
    COMPONENT resources)
install(FILES freeroute.jnlp DESTINATION ${KICAD_BIN}
    COMPONENT resources)

if(UNIX)
    install(FILES share/kicad.desktop
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications
        COMPONENT resources)
    install(FILES bitmaps/kicad.png
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/pixmaps
        COMPONENT resources)
endif(UNIX)
