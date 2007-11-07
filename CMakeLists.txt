##---------------------------------------------------------------------------
## $RCSfile: CMakeLists.txt $
## $Source: CMakeLists.txt $
## $Revision: 1.48 $
## $Date: Feb 26, 2006 10:39:43 PM $
##---------------------------------------------------------------------------
## Author:      Jorgen Bodde
## Copyright:   (c) Jorgen Bodde
## License:     wxWidgets License
##---------------------------------------------------------------------------
 
##---------------------------------------------------
## Please set your wxWidgets configuration here
##--------------------------------------------------- 
 
SET(CMAKE_FIND_LIBRARY_PREFIXES "")
SET(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
 
# Here you can define what libraries of wxWidgets you need for your
# application. You can figure out what libraries you need here;
# http://www.wxwidgets.org/manuals/2.8/wx_librarieslist.html
SET(wxWidgets_USE_LIBS base core gl net)
 
# We need the Find package for wxWidgets to work
FIND_PACKAGE(wxWidgets)
 
##---------------------------------------------------
## Actual config file starts here
##--------------------------------------------------- 
 
# Did we find wxWidgets ? This condition will fail
# for as long as the internal vars do not point to
# the proper wxWidgets configuration
IF(wxWidgets_FOUND)
     
    # Include wxWidgets macros
    INCLUDE(${wxWidgets_USE_FILE})
 
    # Our project is called 'minimal' this is how it will be called in
    # visual studio, and in our makefiles. 
    PROJECT (KICAD_EDA)

    # We define the include paths here, our minimal source dir is one, 
    # and also the include dirs defined by wxWidgets
    INCLUDE_DIRECTORIES(
                        ${CMAKE_BINARY_DIR}
                        ${CMAKE_SOURCE_DIR}
                        ${CMAKE_SOURCE_DIR}/include
                        ${CMAKE_SOURCE_DIR}/common
                        ${CMAKE_SOURCE_DIR}/kicad/bitmaps
                        ${CMAKE_SOURCE_DIR}/common/bitmaps
                        ${wxWidgets_INCLUDE_DIRS}
                       )
 
ELSE(wxWidgets_FOUND)
    # For convenience. When we cannot continue, inform the user
    MESSAGE("wxWidgets not found!")
ENDIF(wxWidgets_FOUND)

add_subdirectory (common)
add_subdirectory (kicad)

