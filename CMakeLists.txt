PROJECT(kicad)

CMAKE_MINIMUM_REQUIRED(VERSION 2.4 FATAL_ERROR)

#SET(CMAKE_VERBOSE_MAKEFILE ON)

SET(wxWidgets_USE_LIBS base core adv gl html net)

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

IF(wxWidgets_FOUND)
	MESSAGE("-- wxWidgets found")
	
	INCLUDE(${wxWidgets_USE_FILE})

	INCLUDE_DIRECTORIES(
		${CMAKE_CURRENT_SOURCE_DIR}/include
		${CMAKE_CURRENT_SOURCE_DIR}/share)

	ADD_SUBDIRECTORY(common)
	ADD_SUBDIRECTORY(3d-viewer)
	ADD_SUBDIRECTORY(cvpcb)
	ADD_SUBDIRECTORY(eeschema)
	ADD_SUBDIRECTORY(gerbview)
	ADD_SUBDIRECTORY(kicad)
	ADD_SUBDIRECTORY(pcbnew)
ELSE(wxWidgets_FOUND)
	MESSAGE("wxWidgets is required to build KiCad!")
ENDIF(wxWidgets_FOUND)

ADD_SUBDIRECTORY(internat)
ADD_SUBDIRECTORY(library)
ADD_SUBDIRECTORY(modules)
ADD_SUBDIRECTORY(template)
