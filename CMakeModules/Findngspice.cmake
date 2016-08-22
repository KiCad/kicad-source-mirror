# CMake script for finding libngspice
# (C) CERN 2016
# Author: Maciej Suminski <maciej.suminski@cern.ch>

find_path( NGSPICE_INCLUDE_DIR ngspice/sharedspice.h
    PATHS ${NGSPICE_ROOT_DIR} $ENV{NGSPICE_ROOT_DIR} ${NGSPICE_INCLUDE_PATH}
    PATH_SUFFIXES src/include share/ngspice/include share/ngspice/include/ngspice
)

find_library( NGSPICE_LIBRARY ngspice
    PATHS ${NGSPICE_ROOT_DIR} $ENV{NGSPICE_ROOT_DIR} ${NGSPICE_LIBRARY_PATH}
    PATH_SUFFIXES src/.libs lib
)

include( FindPackageHandleStandardArgs )

if( ${NGSPICE_INCLUDE_DIR} STREQUAL "NGSPICE_INCLUDE_DIR-NOTFOUND" OR ${NGSPICE_LIBRARY} STREQUAL "NGSPICE_LIBRARY-NOTFOUND" )
    message( "" )
    message( "*** NGSPICE library missing ***" )
    message( "Most of ngspice packages do not provide the required libngspice library." )
    message( "You can either compile ngspice configured with --with-ngshared parameter" )
    message( "or run a script that does the job for you:" )
    message( "  wget https://orson.net.pl/pub/libngspice/get_libngspice_so.sh" )
    message( "  chmod +x get_libngspice_so.sh" )
    message( "  ./get_libngspice_so.sh" )
    message( "  sudo ./get_libngspice_so.sh install" )
    message( "" )
endif()

find_package_handle_standard_args( ngspice
  REQUIRED_VARS
    NGSPICE_INCLUDE_DIR
    NGSPICE_LIBRARY
)

mark_as_advanced(
    NGSPICE_INCLUDE_DIR
    NGSPICE_LIBRARY
)
