# Add all the warnings to the files
if( COMPILER_SUPPORTS_WARNINGS )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARN_FLAGS_CXX}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARN_FLAGS_C}")
endif()

if( KICAD_IDF_TOOLS )
    add_subdirectory( idftools )
endif()

if( WIN32 AND KICAD_BUILD_NATIVE_MODEL_PREVIEW )
    target_sources( kicommon PRIVATE
        ${CMAKE_SOURCE_DIR}/3d-viewer/dialogs/model_preview_manager_windows.cpp )
    target_link_libraries( kicommon Version )
    add_subdirectory( preview_handlers )
endif()
