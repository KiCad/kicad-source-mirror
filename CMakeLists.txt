# Our project is called 'kicad'.This is how it will be called in
# Visual Studio and in our makefiles.
PROJECT(kicad)

CMAKE_MINIMUM_REQUIRED(VERSION 2.4.6 FATAL_ERROR)

# Uncomment this line if you want verbose build messages.
#SET(CMAKE_VERBOSE_MAKEFILE ON)

# Set default flags for Debug build.
SET(CMAKE_C_FLAGS_DEBUG "-Wall -g3 -ggdb3 -DDEBUG")
SET(CMAKE_CXX_FLAGS_DEBUG "-Wall -g3 -ggdb3 -DDEBUG")

# Set default flags for Release build.
SET(CMAKE_C_FLAGS_RELEASE "-Wall -O2 -DNDEBUG")
SET(CMAKE_CXX_FLAGS_RELEASE "-Wall -O2 -DNDEBUG")
SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "-s")

# Here you can define what libraries of wxWidgets you need for your
# application. You can figure out what libraries you need here;
# http://www.wxwidgets.org/manuals/2.8/wx_librarieslist.html
SET(wxWidgets_USE_LIBS base core adv gl html net)

# We need the Find package for wxWidgets to work.
FIND_PACKAGE(wxWidgets REQUIRED)

# Locations for install targets.
IF(UNIX)
	IF(APPLE)
	ELSE(APPLE)
	# Like all variables, CMAKE_INSTALL_PREFIX can be over-ridden on the command line.
	SET(CMAKE_INSTALL_PREFIX /usr/local)
	# When used later, "bin" and others with no leading / is relative to CMAKE_INSTALL_PREFIX.
	SET(KICAD_BIN bin CACHE PATH "Location of KiCad binaries.")
	SET(KICAD_PLUGINS lib/kicad/plugins CACHE PATH "Location of KiCad plugins.")
	SET(KICAD_DOCS share/doc/kicad CACHE PATH "Location of KiCad documentation files.")
	SET(KICAD_DATA share/kicad CACHE PATH "Location of KiCad data files.")
	SET(KICAD_MODULES ${KICAD_DATA}/modules)
	SET(KICAD_LIBRARY ${KICAD_DATA}/library)
	SET(KICAD_INTERNAT ${KICAD_DATA}/internat)
	SET(KICAD_TEMPLATE ${KICAD_DATA}/template)
	ENDIF(APPLE)
ENDIF(UNIX)

IF(WIN32)
	# Like all variables, CMAKE_INSTALL_PREFIX can be over-ridden on the command line.
	SET(CMAKE_INSTALL_PREFIX c:/kicad)
	# When used later, "winexe" and others with no leading / is relative to CMAKE_INSTALL_PREFIX.
	SET(KICAD_BIN winexe CACHE PATH "Location of KiCad binaries.")
	SET(KICAD_PLUGINS ${KICAD_BIN}/plugins)
	SET(KICAD_DOCS help)
	SET(KICAD_MODULES modules)
	SET(KICAD_LIBRARY library)
	SET(KICAD_INTERNAT internat)
	SET(KICAD_TEMPLATE template)
ENDIF(WIN32)

# Did we find wxWidgets ? This condition will fail
# for as long as the internal vars do not point to
# the proper wxWidgets configuration.
IF(wxWidgets_FOUND)
	MESSAGE(STATUS "Check for installed wxWidgets -- found")

	# Include wxWidgets macros.
	INCLUDE(${wxWidgets_USE_FILE})

	# We define the include paths here.
	# Include dirs for wxWidgets was defined before.
	INCLUDE_DIRECTORIES(
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_CURRENT_SOURCE_DIR}/share)

	# CMake will look in these dirs for nested 'CMakeLists.txt' files.
	ADD_SUBDIRECTORY(common)
	ADD_SUBDIRECTORY(3d-viewer)
	ADD_SUBDIRECTORY(cvpcb)
	ADD_SUBDIRECTORY(eeschema)
	ADD_SUBDIRECTORY(gerbview)
	ADD_SUBDIRECTORY(kicad)
	ADD_SUBDIRECTORY(pcbnew)
ELSE(wxWidgets_FOUND)
	# For convenience. When we cannot continue, inform the user.
	MESSAGE(STATUS "Check for installed wxWidgets -- not found")
	MESSAGE(FATAL_ERROR "wxWidgets was not found - it is required to build KiCad")
ENDIF(wxWidgets_FOUND)

# CMake will look at this dirs for nested 'CMakeLists.txt' files.
ADD_SUBDIRECTORY(internat)
ADD_SUBDIRECTORY(help)
ADD_SUBDIRECTORY(library)
ADD_SUBDIRECTORY(modules)
ADD_SUBDIRECTORY(template)
