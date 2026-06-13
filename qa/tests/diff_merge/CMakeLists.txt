# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.

set( QA_DIFF_MERGE_SRCS
    test_module.cpp
    test_diff_scene_helpers.cpp
    test_diff_types.cpp
    test_identity_reconciler.cpp
    test_merge_engine.cpp
    test_merge_validators.cpp
    test_property_value_converter.cpp
    test_diff_types_helpers.cpp
    test_lib_merge_applier.cpp
    test_doc_property_helpers.cpp
    test_auto_resolution.cpp
    test_diff_tree_grouping.cpp
    test_project_file_patch.cpp
    test_validation_pipeline.cpp
    test_diff_value_display.cpp
    test_renderer_output.cpp
    test_cli_visual_diff.cpp
)

if( WIN32 )
    if( MINGW )
        mingw_resource_compiler( qa_diff_merge )
    else()
        set( QA_DIFF_MERGE_RESOURCES ${CMAKE_SOURCE_DIR}/resources/msw/qa_common.rc )
    endif()
endif()

add_executable( qa_diff_merge
    ${QA_DIFF_MERGE_SRCS}
    ${QA_DIFF_MERGE_RESOURCES}
)

target_include_directories( qa_diff_merge PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/qa/mocks/include
)

target_compile_definitions( qa_diff_merge PRIVATE
    QA_KICAD_CLI_PATH="$<TARGET_FILE:kicad-cli>"
)

add_dependencies( qa_diff_merge kicad-cli )

target_link_libraries( qa_diff_merge
    common
    gal
    qa_utils
    ${wxWidgets_LIBRARIES}
    Boost::headers
    Boost::unit_test_framework
)

kicad_add_boost_test( qa_diff_merge qa_diff_merge )

setup_qa_env( qa_diff_merge )

# End-to-end smoke for the `kicad-cli mergetool` Phase 9 entry point.
# Runs the shell script against the just-built kicad-cli binary so the
# argument-handling and exit-code-translation contract stays honored.
if( UNIX )
    add_test(
        NAME qa_diff_merge_mergetool_e2e
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/mergetool_e2e.sh
                $<TARGET_FILE:kicad-cli>
    )

    set_tests_properties( qa_diff_merge_mergetool_e2e PROPERTIES
        DEPENDS kicad-cli
    )

    # Workflow E2E: requires kicad GUI binary + display server.  Skips
    # cleanly when either is missing so CI without xvfb still passes.
    add_test(
        NAME qa_diff_merge_mergetool_workflow_e2e
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/mergetool_workflow_e2e.sh
                $<TARGET_FILE:kicad-cli>
                ${CMAKE_SOURCE_DIR}/qa/data
    )

    set_tests_properties( qa_diff_merge_mergetool_workflow_e2e PROPERTIES
        DEPENDS "kicad-cli;kicad"
    )
endif()
