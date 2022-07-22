# - Try to find Microsoft GDI+ build files.
# Once done this will define
#
#  GDI_PLUS_FOUND - system has GDI+
#  GDI_PLUS_INCLUDE_DIR - the GDI+ include directory
#  GDI_PLUS_LIBRARIES - Link these to use GDI+

if(GDI_PLUS_INCLUDE_DIR AND GDI_PLUS_LIBRARIES)
    set(GDI_PLUS_FIND_QUIETLY TRUE)
endif(GDI_PLUS_INCLUDE_DIR AND GDI_PLUS_LIBRARIES)

find_path(GDI_PLUS_INCLUDE_DIR GdiPlus.h )

find_library(GDI_PLUS_LIBRARIES gdiplus )

# handle the QUIETLY and REQUIRED arguments and set GDI_PLUS_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(GDI_PLUS DEFAULT_MSG GDI_PLUS_LIBRARIES GDI_PLUS_INCLUDE_DIR)

mark_as_advanced(GDI_PLUS_INCLUDE_DIR GDI_PLUS_LIBRARIES)
