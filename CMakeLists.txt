# Our project is called 'kicad' this is how it will be called in
# visual studio, and in our makefiles. 
PROJECT(kicad)

CMAKE_MINIMUM_REQUIRED(VERSION 2.4 FATAL_ERROR)

# Uncomment this line if you want verbose build messages.
#SET(CMAKE_VERBOSE_MAKEFILE ON)

# Here you can define what libraries of wxWidgets you need for your
# application. You can figure out what libraries you need here;
# http://www.wxwidgets.org/manuals/2.8/wx_librarieslist.html
SET(wxWidgets_USE_LIBS base core adv gl html net)

# We need the Find package for wxWidgets to work.
FIND_PACKAGE(wxWidgets REQUIRED)

#Locations for install targets
IF(UNIX)
	#SET(CMAKE_INSTALL_PATH /usr/local)
	SET(KICAD_BIN bin CACHE PATH "Location of KiCad binaries.")
	SET(KICAD_PLUGINS lib/kicad/plugins CACHE PATH "Location of KiCad plugins.")
	SET(KICAD_DOCS share/doc/kicad CACHE PATH "Location of KiCad documentation files.")
	SET(KICAD_DATA share/kicad CACHE PATH "Location of KiCad data files.")
	SET(KICAD_MODULES ${KICAD_DATA}/modules)
	SET(KICAD_LIBRARY ${KICAD_DATA}/library)
	SET(KICAD_INTERNAT ${KICAD_DATA}/internat)
	SET(KICAD_TEMPLATE ${KICAD_DATA}/template)
ENDIF(UNIX)

IF(WIN32)
ENDIF(WIN32)

#Should this go to IF(UNIX)?
IF(APPLE)
ENDIF(APPLE)

# Did we find wxWidgets ? This condition will fail
# for as long as the internal vars do not point to
# the proper wxWidgets configuration.
MESSAGE("-- Check for installed wxWidgets:")
IF(wxWidgets_FOUND)
	MESSAGE("      wxWidgets found")
	
	# Include wxWidgets macros.
        INCLUDE(${wxWidgets_USE_FILE})

        # We define the include paths here. 
        # Include dirs for wxWidgets was defined before.
	INCLUDE_DIRECTORIES(
		${CMAKE_CURRENT_SOURCE_DIR}/include
		${CMAKE_CURRENT_SOURCE_DIR}/share)

	# CMake will look at this dirs for nested 'CMakeLists.txt' files
        ADD_SUBDIRECTORY(common)
	ADD_SUBDIRECTORY(3d-viewer)
	ADD_SUBDIRECTORY(cvpcb)
	ADD_SUBDIRECTORY(eeschema)
	ADD_SUBDIRECTORY(gerbview)
	ADD_SUBDIRECTORY(kicad)
	ADD_SUBDIRECTORY(pcbnew)
ELSE(wxWidgets_FOUND)
        # For convenience. When we cannot continue, inform the user.
        MESSAGE("      Error: wxWidgets doesn't found (it is required to build KiCad!)")
ENDIF(wxWidgets_FOUND)

# CMake will look at this dirs for nested 'CMakeLists.txt' files
ADD_SUBDIRECTORY(internat)
ADD_SUBDIRECTORY(library)
ADD_SUBDIRECTORY(modules)
ADD_SUBDIRECTORY(template)
