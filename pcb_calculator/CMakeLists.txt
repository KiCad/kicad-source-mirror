include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}/bitmaps
                    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs
                    ${CMAKE_CURRENT_SOURCE_DIR}/transline
                    ${CMAKE_CURRENT_SOURCE_DIR}/attenuators
                    )

set(PCB_CALCULATOR_SRCS
    attenuators.cpp
    board_classes_values.cpp
    pcb_calculator.cpp
    pcb_calculator_frame.cpp
    colorcode.cpp
    params_read_write.cpp
    regulators_funct.cpp
    transline_ident.cpp
    UnitSelector.cpp
    transline/transline.cpp
    transline/c_microstrip.cpp
    transline/microstrip.cpp
    transline/coplanar.cpp
    transline/coax.cpp
    transline/rectwaveguide.cpp
    transline/stripline.cpp
    transline/twistedpair.cpp
    transline_dlg_funct.cpp
    dialogs/pcb_calculator_frame_base.cpp
    attenuators/attenuator_classes.cpp
    )

if(WIN32)
    if(MINGW)
        # PCB_CALCULATOR_RESOURCES variable is set by the macro.
        mingw_resource_compiler(pcb_calculator)
    else(MINGW)
        set(PCB_CALCULATOR_RESOURCES pcb_calculator.rc)
    endif(MINGW)
endif(WIN32)


if(APPLE)
    set(PCB_CALCULATOR_RESOURCES pcb_calculator.icns)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/pcb_calculator.icns"
                                PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set(MACOSX_BUNDLE_ICON_FILE pcb_calculator.icns)
    set(MACOSX_BUNDLE_GUI_IDENTIFIER org.kicad-eda.pcb_calculator)
endif(APPLE)



add_executable(pcb_calculator WIN32 MACOSX_BUNDLE
                            ${PCB_CALCULATOR_SRCS}
                            ${PCB_CALCULATOR_RESOURCES})


if(APPLE)
    set_target_properties(pcb_calculator PROPERTIES MACOSX_BUNDLE_INFO_PLIST
        ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist)
endif(APPLE)

target_link_libraries( pcb_calculator
                        ${wxWidgets_LIBRARIES}
                        )

install(TARGETS pcb_calculator
        DESTINATION ${KICAD_BIN}
        COMPONENT binary)

