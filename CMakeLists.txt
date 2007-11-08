PROJECT(kicad)

CMAKE_MINIMUM_REQUIRED(VERSION 2.4.7 FATAL_ERROR)

#SET(CMAKE_VERBOSE_MAKEFILE ON)

SET(wxWidgets_USE_LIBS base core adv gl html net)

FIND_PACKAGE(wxWidgets REQUIRED)

IF(wxWidgets_FOUND)
	MESSAGE("-- wxWidgets found")
	
	INCLUDE(${wxWidgets_USE_FILE})

	INCLUDE_DIRECTORIES(
		${CMAKE_CURRENT_SOURCE_DIR}/include
		${CMAKE_CURRENT_SOURCE_DIR}/share)

	SUBDIRS(common 3d-viewer cvpcb eeschema gerbview kicad pcbnew)
ELSE(wxWidgets_FOUND)
	MESSAGE("wxWidgets is required to build KiCad!")
ENDIF(wxWidgets_FOUND)
