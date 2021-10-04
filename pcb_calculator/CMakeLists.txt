add_definitions(-DPCB_CALCULATOR_BUILD)

include_directories( BEFORE ${INC_BEFORE} )
include_directories(
    dialogs
    ${CMAKE_SOURCE_DIR}/common
    ${INC_AFTER}
    )

set( PCB_CALCULATOR_SRCS
    eserie.cpp
    board_classes_values.cpp
    common_data.cpp
    electrical_spacing_values.cpp
    params_read_write.cpp
    pcb_calculator_frame.cpp
    pcb_calculator_settings.cpp
    datafile_read_write.cpp
    panel_attenuators.cpp
    panel_regulator.cpp
    panel_color_code.cpp
    panel_track_width.cpp
    panel_via_size.cpp
    transline_ident.cpp
    widgets/unit_selector.cpp
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
    dialogs/dialog_regulator_form_base.cpp
    dialogs/dialog_regulator_form.cpp
    dialogs/panel_attenuators_base.cpp
    dialogs/panel_regulator_base.cpp
    dialogs/panel_color_code_base.cpp
    dialogs/panel_via_size_base.cpp
    dialogs/panel_track_width_base.cpp
    ../common/env_vars.cpp      # needed on MSW to avoid a link issue (a symbol not found)
    )

if( MINGW )
    # PCB_CALCULATOR_RESOURCES variable is set by the macro.
    mingw_resource_compiler( pcb_calculator )
else()
    set( PCB_CALCULATOR_RESOURCES ${CMAKE_SOURCE_DIR}/resources/msw/pcb_calculator.rc )
endif()

if( APPLE )
    # setup bundle
    set( PCB_CALCULATOR_RESOURCES pcb_calculator.icns )
    set_source_files_properties( "${CMAKE_CURRENT_SOURCE_DIR}/pcb_calculator.icns" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
        )
    set( MACOSX_BUNDLE_ICON_FILE pcb_calculator.icns )
    set( MACOSX_BUNDLE_GUI_IDENTIFIER org.kicad.kicad )
    set( MACOSX_BUNDLE_NAME pcb_calculator )
endif()

add_executable( pcb_calculator WIN32 MACOSX_BUNDLE
    ${CMAKE_SOURCE_DIR}/common/single_top.cpp
    ${CMAKE_SOURCE_DIR}/common/pgm_base.cpp
    ${PCB_CALCULATOR_RESOURCES}
    )
set_source_files_properties( ${CMAKE_SOURCE_DIR}/common/single_top.cpp PROPERTIES
    COMPILE_DEFINITIONS "TOP_FRAME=FRAME_CALC;BUILD_KIWAY_DLL"
    )
target_link_libraries( pcb_calculator
    #singletop  # replaces common, giving us restrictive control and link warnings.
    # There's way too much crap coming in from common yet.
    common
    ${wxWidgets_LIBRARIES}
    )

if( MAKE_LINK_MAPS )
    set_target_properties( pcb_calculator PROPERTIES
        LINK_FLAGS "-Wl,-cref,-Map=pcb_calculator.map" )
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
    markdown_lib
    ${wxWidgets_LIBRARIES}
    )
set_source_files_properties( pcb_calculator.cpp PROPERTIES
    # The KIFACE is in pcb_calculator.cpp, export it:
    COMPILE_DEFINITIONS     "BUILD_KIWAY_DLL;COMPILING_DLL"
    )

if( MAKE_LINK_MAPS )
    set_target_properties( pcb_calculator_kiface PROPERTIES
        LINK_FLAGS "-Wl,-cref,-Map=_pcb_calculator.kiface.map" )
endif()

# if building pcb_calculator, then also build pcb_calculator_kiface if out of date.
add_dependencies( pcb_calculator pcb_calculator_kiface )

# these 2 binaries are a matched set, keep them together
if( APPLE )
    set_target_properties( pcb_calculator PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${PROJECT_BINARY_DIR}/pcb_calculator/Info.plist
        )

    # puts binaries into the *.app bundle while linking
    set_target_properties( pcb_calculator_kiface PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${OSX_BUNDLE_BUILD_KIFACE_DIR}
        )

    # put individual bundle outside of main bundle as a first step
    # will be pulled into the main bundle when creating main bundle
    install( TARGETS pcb_calculator
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )
    install( CODE "
        # override default embedded path settings
        ${OSX_BUNDLE_OVERRIDE_PATHS}

        # do all the work
        include( BundleUtilities )
        fixup_bundle( ${KICAD_BIN}/pcb_calculator.app/Contents/MacOS/pcb_calculator
            \"\"
            \"\"
            )
        " COMPONENT Runtime
        )
else()
    install( TARGETS pcb_calculator
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )
    install( TARGETS pcb_calculator_kiface
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )
endif()

if( KICAD_WIN32_INSTALL_PDBS )
    # Get the PDBs to copy over for MSVC
    install(FILES $<TARGET_PDB_FILE:pcb_calculator> DESTINATION ${KICAD_BIN})
    install(FILES $<TARGET_PDB_FILE:pcb_calculator_kiface> DESTINATION ${KICAD_BIN})
endif()

# auto-generate pcb_calculator_datafile.h and pcb_calculator_datafile_keywords.cpp
# for the storage data file format.
make_lexer(
    pcb_calculator_kiface
    pcb_calculator_datafile.keywords
    pcb_calculator_datafile_lexer.h
    pcb_calculator_datafile_keywords.cpp
    PCBCALC_DATA_T
   )

#
# Conversion of .html doc source files to .h files included in cpp sources
#
# Function html_doc2h : converts a single *.html text file to a *.h header
function( html_doc2h inputFile )
    add_custom_command(
        OUTPUT ${inputFile}.h
        COMMAND ${CMAKE_COMMAND} -DinputFile=${inputFile}.html -DoutputFile=${inputFile}.h
            -P ${CMAKE_MODULE_PATH}/BuildSteps/Html2C.cmake
        DEPENDS ${inputFile}.html ${CMAKE_MODULE_PATH}/BuildSteps/Html2C.cmake
        COMMENT "creating ${inputFile}.h from ${inputFile}.html"
        )
endfunction()
#
# Conversion of .md doc source files to .h files included in cpp sources
#
# Function md_doc2h : converts a single *.md text file to a *.h header
function( md_doc2h inputFile )
    add_custom_command(
        OUTPUT ${inputFile}.h

        COMMAND ${CMAKE_COMMAND} -DinputFile=${inputFile}.md -DoutputFile=${inputFile}.h
            -P ${CMAKE_MODULE_PATH}/BuildSteps/markdown2C.cmake
        DEPENDS ${inputFile}.md ${CMAKE_MODULE_PATH}/BuildSteps/markdown2C.cmake
        COMMENT "creating ${inputFile}.h from ${inputFile}.md"
        )
endfunction()

    md_doc2h( ${CMAKE_CURRENT_SOURCE_DIR}/attenuators/bridget_tee_formula )
    md_doc2h( ${CMAKE_CURRENT_SOURCE_DIR}/attenuators/splitter_formula )
    md_doc2h( ${CMAKE_CURRENT_SOURCE_DIR}/attenuators/pi_formula )
    md_doc2h( ${CMAKE_CURRENT_SOURCE_DIR}/attenuators/tee_formula )
    md_doc2h( ${CMAKE_CURRENT_SOURCE_DIR}/tracks_width_versus_current_formula )
    md_doc2h( ${CMAKE_CURRENT_SOURCE_DIR}/eserie_help )

set( DOCS_LIST
        ${CMAKE_CURRENT_SOURCE_DIR}/attenuators/pi_formula.h
        ${CMAKE_CURRENT_SOURCE_DIR}/attenuators/tee_formula.h
        ${CMAKE_CURRENT_SOURCE_DIR}/attenuators/bridget_tee_formula.h
        ${CMAKE_CURRENT_SOURCE_DIR}/attenuators/splitter_formula.h
        ${CMAKE_CURRENT_SOURCE_DIR}/tracks_width_versus_current_formula.h
        ${CMAKE_CURRENT_SOURCE_DIR}/eserie_help.h
    )

set_source_files_properties( attenuators/attenuator_classes.cpp
    PROPERTIES OBJECT_DEPENDS "${DOCS_LIST}"
    )
