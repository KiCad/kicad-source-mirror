
set( STAND_ALONE true )

if( STAND_ALONE )
    project(kicad-new)

    cmake_minimum_required(VERSION 2.6.4 FATAL_ERROR)

    set( PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../ )

    # Path to local CMake modules.
    set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)

    include(CheckFindPackageResult)

    find_package(Doxygen)

    # On Apple only wxwidgets 2.9 or higher doesn't need to find aui part of base
    if(APPLE)
        find_package(wxWidgets COMPONENTS gl adv html core net base xml QUIET)
    else()
        find_package(wxWidgets COMPONENTS gl aui adv html core net base xml QUIET)
    endif()

    check_find_package_result( wxWidgets_FOUND "wxWidgets" )

    # Include wxWidgets macros.
    include( ${wxWidgets_USE_FILE} )

    include_directories( ${PROJECT_SOURCE_DIR}/include )

    if(CMAKE_COMPILER_IS_GNUCXX)
        # Set default flags for Release build.
        set(CMAKE_C_FLAGS_RELEASE "-Wall -O2 -DNDEBUG ")
        set(CMAKE_CXX_FLAGS_RELEASE "-Wall -O2 -DNDEBUG")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-s -static-libgcc")

        # Set default flags for Debug build.
        set(CMAKE_C_FLAGS_DEBUG "-Wall -g3 -ggdb3 -DDEBUG")
        set(CMAKE_CXX_FLAGS_DEBUG "-Wall -g3 -ggdb3 -DDEBUG")
    endif(CMAKE_COMPILER_IS_GNUCXX)

    include(Functions)

endif()



#================================================
# Doxygen Output
#================================================
if(DOXYGEN_FOUND)
    add_custom_target( new-docs ${DOXYGEN_EXECUTABLE}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS Doxyfile )
else(DOXYGEN_FOUND)
    message( STATUS "WARNING: Doxygen not found - new-docs (Source Docs) target not created" )
endif()


include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )

add_executable( test_dir_lib_source sch_dir_lib_source.cpp )
target_link_libraries( test_dir_lib_source ${wxWidgets_LIBRARIES} )

add_executable( test_lib_table sch_lib_table.cpp sch_lib_table_keywords.cpp )
target_link_libraries( test_lib_table ${wxWidgets_LIBRARIES} )

add_executable( test_sch_part sch_part.cpp )
target_link_libraries( test_lib_table ${wxWidgets_LIBRARIES} )


make_lexer(
    ${CMAKE_CURRENT_SOURCE_DIR}/sch_lib_table.keywords
    ${CMAKE_CURRENT_SOURCE_DIR}/sch_lib_table_lexer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/sch_lib_table_keywords.cpp
    ELT_T
    )

