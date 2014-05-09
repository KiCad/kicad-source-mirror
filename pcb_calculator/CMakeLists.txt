
include_directories( BEFORE ${INC_BEFORE} )
include_directories(
    bitmaps
    dialogs
    transline
    attenuators
    ../polygon
    ../common
    ${INC_AFTER}
    )

set( PCB_CALCULATOR_SRCS
    attenuators.cpp
    board_classes_values.cpp
    colorcode.cpp
    electrical_spacing_values.cpp
    params_read_write.cpp
    pcb_calculator_frame.cpp
    datafile_read_write.cpp
    regulators_funct.cpp
    tracks_width_versus_current.cpp
    transline_ident.cpp
    UnitSelector.cpp
    pcb_calculator_datafile_keywords.cpp
    transline/transline.cpp
    transline/c_microstrip.cpp
    transline/microstrip.cpp
    transline/coplanar.cpp
    transline/coax.cpp
    transline/rectwaveguide.cpp
    transline/stripline.cpp
    transline/twistedpair.cpp
    transline_dlg_funct.cpp
    attenuators/attenuator_classes.cpp
    dialogs/pcb_calculator_frame_base.cpp
    dialogs/dialog_regulator_data_base.cpp
    )

if( MINGW )
    # PCB_CALCULATOR_RESOURCES variable is set by the macro.
    mingw_resource_compiler(pcb_calculator)
endif()


if( APPLE )
    set( PCB_CALCULATOR_RESOURCES pcb_calculator.icns )
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/pcb_calculator.icns" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
        )
    set( MACOSX_BUNDLE_ICON_FILE pcb_calculator.icns )
    set( MACOSX_BUNDLE_GUI_IDENTIFIER org.kicad-eda.pcb_calculator )
    set( MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist )
endif()

# auto-generate pcb_calculator_datafile.h and pcb_calculator_datafile_keywords.cpp
# for the storage data file format.
make_lexer(
    ${CMAKE_CURRENT_SOURCE_DIR}/pcb_calculator_datafile.keywords
    ${CMAKE_CURRENT_SOURCE_DIR}/pcb_calculator_datafile_lexer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/pcb_calculator_datafile_keywords.cpp
    PCBCALC_DATA_T

    # Pass header file with dependency on *_lexer.h as extra_arg
    datafile_read_write.h
   )


if( USE_KIWAY_DLLS )
#if( false )

    add_executable( pcb_calculator WIN32 MACOSX_BUNDLE
        ../common/single_top.cpp
        ../common/pgm_base.cpp
        ${PCB_CALCULATOR_RESOURCES}
        )
    set_source_files_properties( ../common/single_top.cpp PROPERTIES
        COMPILE_DEFINITIONS "TOP_FRAME=FRAME_CALC;BUILD_KIWAY_DLL"
        )
    target_link_libraries( pcb_calculator
        #singletop  # replaces common, giving us restrictive control and link warnings.
        # There's way too much crap coming in from common yet.
        common
        bitmaps
        ${wxWidgets_LIBRARIES}
        )
    if( MAKE_LINK_MAPS )
        set_target_properties( pcb_calculator PROPERTIES
            LINK_FLAGS "${TO_LINKER},-cref ${TO_LINKER},-Map=pcb_calculator.map" )
    endif()

    # the main pcb_calculator program, in DSO form.
    add_library( pcb_calculator_kiface MODULE
        pcb_calculator.cpp
        ${PCB_CALCULATOR_SRCS}
        )
    set_target_properties( pcb_calculator_kiface PROPERTIES
        OUTPUT_NAME     pcb_calculator
        PREFIX          ${KIFACE_PREFIX}
        SUFFIX          ${KIFACE_SUFFIX}
        )
    target_link_libraries( pcb_calculator_kiface
        common
        bitmaps
        polygon
        ${wxWidgets_LIBRARIES}
        )
    set_source_files_properties( pcb_calculator.cpp PROPERTIES
        # The KIFACE is in pcb_calculator.cpp, export it:
        COMPILE_DEFINITIONS     "BUILD_KIWAY_DLL;COMPILING_DLL"
        )
    if( MAKE_LINK_MAPS )
        set_target_properties( pcb_calculator_kiface PROPERTIES
            LINK_FLAGS "${TO_LINKER},-cref ${TO_LINKER},-Map=_pcb_calculator.kiface.map" )
    endif()

    # if building pcb_calculator, then also build pcb_calculator_kiface if out of date.
    add_dependencies( pcb_calculator pcb_calculator_kiface )

    # these 2 binaries are a matched set, keep them together
    install( TARGETS pcb_calculator
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )
    if( APPLE )
        # puts the *.kiface into the *.app bundle while linking
        set_target_properties( pcb_calculator_kiface PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/pcb_calculator.app/Contents/MacOS/
            )
    else()
        install( TARGETS pcb_calculator_kiface
            DESTINATION ${KICAD_BIN}
            COMPONENT binary
            )
    endif()

    if( APPLE )
        # copies kiface into the bundle
        add_custom_target( _pcb_calculator_kiface_copy ALL
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/pcb_calculator/_pcb_calculator.kiface "${CMAKE_BINARY_DIR}/pcb_calculator/pcb_calculator.app/Contents/MacOS/"
            DEPENDS pcb_calculator_kiface
            COMMENT "Copying kiface into pcb_calculator"
        )
    endif()

else()

    add_executable( pcb_calculator WIN32 MACOSX_BUNDLE
        ../common/single_top.cpp
        pcb_calculator.cpp
        ${PCB_CALCULATOR_SRCS}
        ${PCB_CALCULATOR_RESOURCES}
        )
    set_source_files_properties( ../common/single_top.cpp PROPERTIES
        COMPILE_DEFINITIONS     "TOP_FRAME=0"
        )
    set_source_files_properties( pcb_calculator.cpp PROPERTIES
        COMPILE_DEFINITIONS     "COMPILING_DLL"
        )
    target_link_libraries( pcb_calculator
        common
        bitmaps
        polygon
        ${wxWidgets_LIBRARIES}
        )
    install( TARGETS pcb_calculator
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )

endif()


if( APPLE )
    set_target_properties( pcb_calculator PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
        )
endif()
