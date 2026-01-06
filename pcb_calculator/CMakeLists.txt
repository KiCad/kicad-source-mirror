add_compile_definitions( PCB_CALCULATOR_BUILD )

include_directories( BEFORE ${INC_BEFORE} )
include_directories(
    dialogs
    ${CMAKE_SOURCE_DIR}/common
    ${INC_AFTER}
    )

set( PCB_CALCULATOR_SRCS
    common_data.cpp
    eseries.cpp
    params_read_write.cpp
    pcb_calculator_frame.cpp
    pcb_calculator_settings.cpp
    datafile_read_write.cpp
    calculator_panels/panel_rf_attenuators.cpp
    calculator_panels/panel_rf_attenuators_base.cpp
    calculator_panels/panel_board_class.cpp
    calculator_panels/panel_board_class_base.cpp
    calculator_panels/panel_cable_size.cpp
    calculator_panels/panel_cable_size_base.cpp
    calculator_panels/panel_color_code.cpp
    calculator_panels/panel_color_code_base.cpp
    calculator_panels/panel_eseries_display.cpp
    calculator_panels/panel_eseries_display_base.cpp
    calculator_panels/panel_galvanic_corrosion.cpp
    calculator_panels/panel_galvanic_corrosion_base.cpp
    calculator_panels/panel_electrical_spacing.cpp
    calculator_panels/panel_electrical_spacing_base.cpp
    calculator_panels/iec60664.cpp
    calculator_panels/panel_electrical_spacing_iec60664.cpp
    calculator_panels/panel_electrical_spacing_iec60664_base.cpp
    calculator_panels/panel_electrical_spacing_ipc2221.cpp
    calculator_panels/panel_electrical_spacing_ipc2221_base.cpp
    calculator_panels/panel_r_calculator.cpp
    calculator_panels/panel_r_calculator_base.cpp
    calculator_panels/panel_fusing_current.cpp
    calculator_panels/panel_fusing_current_base.cpp
    calculator_panels/panel_regulator.cpp
    calculator_panels/panel_regulator_base.cpp
    calculator_panels/panel_track_width.cpp
    calculator_panels/panel_track_width_base.cpp
    calculator_panels/panel_transline.cpp
    calculator_panels/panel_transline_base.cpp
    calculator_panels/panel_via_size.cpp
    calculator_panels/panel_via_size_base.cpp
    calculator_panels/panel_wavelength.cpp
    calculator_panels/panel_wavelength_base.cpp
    transline_ident.cpp
    widgets/unit_selector.cpp
    transline/transline.cpp
    transline/c_microstrip.cpp
    transline/microstrip.cpp
    transline/coplanar.cpp
    transline/coax.cpp
    transline/rectwaveguide.cpp
    transline/stripline.cpp
    transline/c_stripline.cpp
    transline/twistedpair.cpp
    transline_dlg_funct.cpp
    attenuators/attenuator_classes.cpp
    dialogs/dialog_regulator_form_base.cpp
    dialogs/dialog_regulator_form.cpp
    pcb_calculator_utils.cpp
    resistor_substitution_utils.cpp
    )

if( WIN32 )
    if( MINGW )
        # PCB_CALCULATOR_RESOURCES variable is set by the macro.
        mingw_resource_compiler( pcb_calculator )
    else()
        set( PCB_CALCULATOR_RESOURCES ${CMAKE_SOURCE_DIR}/resources/msw/pcb_calculator.rc )
    endif()
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
    ${PCB_CALCULATOR_RESOURCES}
    )

set_source_files_properties( ${CMAKE_SOURCE_DIR}/common/single_top.cpp PROPERTIES
    COMPILE_DEFINITIONS "TOP_FRAME=FRAME_CALC;BUILD_KIWAY_DLL"
    )
target_link_libraries( pcb_calculator
    kicommon
    ${wxWidgets_LIBRARIES}
    )

target_link_options( pcb_calculator PRIVATE
    $<$<BOOL:${KICAD_MAKE_LINK_MAPS}>:-Wl,--cref,-Map=pcb_calculator.map>
    )

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

target_link_options( pcb_calculator_kiface PRIVATE
    $<$<BOOL:${KICAD_MAKE_LINK_MAPS}>:-Wl,--cref,-Map=_pcb_calculator.kiface.map>
    )

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

    set_target_properties( pcb_calculator PROPERTIES INSTALL_RPATH
            "@executable_path/../Frameworks;@executable_path/../Frameworks/Python.framework" )
    set_target_properties( pcb_calculator_kiface PROPERTIES INSTALL_RPATH
            "@executable_path/../Frameworks;@executable_path/../Frameworks/Python.framework" )
    set_target_properties( pcb_calculator_kiface PROPERTIES BUILD_WITH_INSTALL_RPATH 1 )

    # put individual bundle outside of main bundle as a first step
    # will be pulled into the main bundle when creating main bundle
    install( TARGETS pcb_calculator
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )

    install( CODE "
        set( KICAD_CMAKE_MODULE_PATH \"${KICAD_CMAKE_MODULE_PATH}\" )
        set( KICAD_BIN \"${KICAD_BIN}\" )
        set( OSX_BUNDLE_INSTALL_BIN_DIR \"${OSX_BUNDLE_INSTALL_BIN_DIR}\" )
        set( OSX_BUNDLE_INSTALL_LIB_DIR \"${OSX_BUNDLE_INSTALL_LIB_DIR}\" )
    " )

    install( CODE [[
        include( ${KICAD_CMAKE_MODULE_PATH}/InstallSteps/InstallMacOS.cmake )

        # Install any dependencies (this will generally duplicate kicad.app but we can't be sure)
        install_runtime_deps( "${KICAD_BIN}/pcb_calculator.app/Contents/MacOS/pcb_calculator"
            ""
            ""
            )
    ]] )
else()
    if( MSVC )
        target_sources( pcb_calculator_kiface PRIVATE ${CMAKE_SOURCE_DIR}/resources/msw/pcb_calculator-dll.rc  )
    endif()

    install( TARGETS pcb_calculator
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )
    install( TARGETS pcb_calculator_kiface
        DESTINATION ${KICAD_KIFACE}
        COMPONENT binary
        )
endif()

if( KICAD_WIN32_INSTALL_PDBS )
    # Get the PDBs to copy over for MSVC
    install(FILES $<TARGET_PDB_FILE:pcb_calculator> DESTINATION ${KICAD_BIN})
    install(FILES $<TARGET_PDB_FILE:pcb_calculator_kiface> DESTINATION ${KICAD_KIFACE})
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
            -P ${KICAD_CMAKE_MODULE_PATH}/BuildSteps/Html2C.cmake
        DEPENDS ${inputFile}.html ${KICAD_CMAKE_MODULE_PATH}/BuildSteps/Html2C.cmake
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
            -P ${KICAD_CMAKE_MODULE_PATH}/BuildSteps/markdown2C.cmake
        DEPENDS ${inputFile}.md ${KICAD_CMAKE_MODULE_PATH}/BuildSteps/markdown2C.cmake
        COMMENT "creating ${inputFile}.h from ${inputFile}.md"
        )
endfunction()

    md_doc2h( ${CMAKE_CURRENT_SOURCE_DIR}/attenuators/bridget_tee_formula )
    md_doc2h( ${CMAKE_CURRENT_SOURCE_DIR}/attenuators/splitter_formula )
    md_doc2h( ${CMAKE_CURRENT_SOURCE_DIR}/attenuators/pi_formula )
    md_doc2h( ${CMAKE_CURRENT_SOURCE_DIR}/attenuators/tee_formula )
    md_doc2h( ${CMAKE_CURRENT_SOURCE_DIR}/tracks_width_versus_current_formula )
    md_doc2h( ${CMAKE_CURRENT_SOURCE_DIR}/eseries_display_help )
    md_doc2h( ${CMAKE_CURRENT_SOURCE_DIR}/r_calculator_help )
    md_doc2h( ${CMAKE_CURRENT_SOURCE_DIR}/fusing_current_help )
    md_doc2h( ${CMAKE_CURRENT_SOURCE_DIR}/galvanic_corrosion_help )
    md_doc2h( ${CMAKE_CURRENT_SOURCE_DIR}/iec60664_help )

set( DOCS_LIST
        ${CMAKE_CURRENT_SOURCE_DIR}/attenuators/pi_formula.h
        ${CMAKE_CURRENT_SOURCE_DIR}/attenuators/tee_formula.h
        ${CMAKE_CURRENT_SOURCE_DIR}/attenuators/bridget_tee_formula.h
        ${CMAKE_CURRENT_SOURCE_DIR}/attenuators/splitter_formula.h
        ${CMAKE_CURRENT_SOURCE_DIR}/tracks_width_versus_current_formula.h
        ${CMAKE_CURRENT_SOURCE_DIR}/eseries_display_help.h
        ${CMAKE_CURRENT_SOURCE_DIR}/r_calculator_help.h
        ${CMAKE_CURRENT_SOURCE_DIR}/fusing_current_help.h
        ${CMAKE_CURRENT_SOURCE_DIR}/galvanic_corrosion_help.h
        ${CMAKE_CURRENT_SOURCE_DIR}/iec60664_help.h
    )

set_source_files_properties( attenuators/attenuator_classes.cpp
    PROPERTIES OBJECT_DEPENDS "${DOCS_LIST}"
    )
