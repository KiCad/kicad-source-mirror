# CMake script for finding libngspice
# (C) CERN 2016
# Author: Maciej Suminski <maciej.suminski@cern.ch>

find_path( NGSPICE_INCLUDE_DIR ngspice/sharedspice.h
    PATHS ${NGSPICE_INCLUDE_PATH}
    PATH_SUFFIXES share/ngspice/include
)

find_library( NGSPICE_LIBRARY ngspice
    PATHS ${NGSPICE_LIBRARY_PATH}
)

include( FindPackageHandleStandardArgs )

find_package_handle_standard_args( ngspice
  REQUIRED_VARS
    NGSPICE_INCLUDE_DIR
    NGSPICE_LIBRARY
)

mark_as_advanced(
    NGSPICE_INCLUDE_DIR
    NGSPICE_LIBRARY
)
