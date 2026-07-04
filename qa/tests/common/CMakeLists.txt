# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2018, 2023 KiCad Developers, see CHANGELOG.TXT for contributors.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

find_package( wxWidgets 3.0.0 COMPONENTS gl aui adv html core net base xml stc REQUIRED )

set( QA_COMMON_SRCS
    # This is needed for the global mock objects
    ${CMAKE_SOURCE_DIR}/qa/mocks/kicad/common_mocks.cpp

    # The main test entry points
    test_module.cpp

    wximage_test_utils.cpp

    test_3d_model_triangle.cpp

    test_angle_item.cpp

    test_action_manager.cpp
    test_array_axis.cpp
    test_atomic_save.cpp
    test_backup_settings.cpp
    test_base_set.cpp
    test_bitmap_base.cpp
    test_clipboard.cpp
    test_collector.cpp
    test_color4d.cpp
    test_commit.cpp
    test_coroutine.cpp
    test_design_block_io.cpp
    test_eda_shape.cpp
    test_eda_text.cpp
    test_editor_tabs_panel.cpp
    test_unit_binder.cpp
    test_wx_grid.cpp
    test_embedded_file_compress.cpp
    test_expand_text_vars.cpp
    test_text_var_dependency.cpp
    test_drawing_sheet_reactive.cpp
    test_file_history.cpp
    test_filename_resolver.cpp
    test_hashtables.cpp
    test_hotkey_store.cpp
    test_increment.cpp
    test_inplace_function.cpp
    test_item_realignment.cpp
    test_ki_any.cpp
    test_library_tables.cpp
    test_lockfile.cpp
    test_loop_safe_collect.cpp
    test_markup_parser.cpp
    test_kicad_string.cpp
    test_kicad_stroke_font.cpp
    test_outline_font.cpp
    test_dxf_plotter.cpp
    test_gerber_plotter.cpp
    test_pdf_unicode_plot.cpp
    test_png_plotter.cpp
    test_kiid.cpp
    test_layer_ids.cpp
    test_lib_id.cpp
    test_lib_tree_model_adapter.cpp
    test_layer_range.cpp
    test_lset.cpp
    test_markup_parser.cpp
    test_notifications_manager.cpp
    test_property.cpp
    test_property_holder.cpp
    test_reporting.cpp
    test_rc_tree_model.cpp
    test_refdes_utils.cpp
    test_wx_data_view_hyperlink_renderer.cpp
    test_grid_helper.cpp
    test_string_utils.cpp
    test_richio.cpp
    test_text_attributes.cpp
    test_tool_dispatcher.cpp
    git/test_libgit_backend.cpp
    git/test_git_compare_handler.cpp
    text_eval/test_text_eval_parser.cpp
    text_eval/test_text_eval_parser_core.cpp
    text_eval/test_text_eval_parser_datetime.cpp
    text_eval/test_text_eval_parser_vcs.cpp
    text_eval/test_text_eval_parser_integration.cpp
    text_eval/test_text_eval_numeric_compat.cpp
    text_eval/test_text_eval_render.cpp
    test_title_block.cpp
    test_types.cpp
    test_utf8.cpp
    test_wildcards_and_files_ext.cpp
    test_wx_any_utils.cpp
    test_wx_filename.cpp

    libeval/test_numeric_evaluator.cpp

    transline_calculations/test_coax.cpp
    transline_calculations/test_coplanar.cpp
    transline_calculations/test_dielectric_selector.cpp
    transline_calculations/test_djordjevic_sarkar.cpp
    transline_calculations/test_djordjevic_sarkar_integration.cpp
    transline_calculations/test_coupled_microstrip.cpp
    transline_calculations/test_coupled_stripline.cpp
    transline_calculations/test_coupled_stripline_offset.cpp
    transline_calculations/test_microstrip_cover.cpp
    transline_calculations/test_rectwaveguide.cpp
    transline_calculations/test_soldermask.cpp
    transline_calculations/test_transline_roundtrip.cpp
    transline_calculations/test_twistedpair.cpp

    io/test_csv.cpp
    io/altium/test_altium_parser.cpp
    io/altium/test_altium_parser_utils.cpp
    io/cadstar/test_cadstar_parts_parser.cpp

    io/cadstar/test_cadstar_archive_parser.cpp

    io/pads/test_pads_attribute_mapper.cpp
    io/pads/test_pads_common.cpp
    io/pads/test_pads_unit_converter.cpp

    test_kigit_common.cpp
    test_kigit_orphan_registry.cpp
    test_throttle.cpp
    view/test_zoom_controller.cpp
    view/test_view_null_safety.cpp

    test_netname_validator.cpp

    # GAL tests
    test_gal_xor_mode.cpp
    test_gal_resize_strategy.cpp

    # Preview item / drawing utility tests
    test_dashed_line.cpp

    # Graphics import tests
    test_graphics_importer_buffer.cpp
    test_svg_import.cpp

    # Project template tests (source included directly since it's from kicad executable)
    test_project_template.cpp
    ${CMAKE_SOURCE_DIR}/kicad/project_template.cpp

    # Project file tests
    test_project_file.cpp
    test_settings_manager.cpp
    test_net_settings.cpp
    test_component_class_settings.cpp
    test_project_local_settings.cpp
    test_remote_provider_metadata.cpp
    test_remote_provider_client.cpp
    test_remote_provider_settings.cpp
    test_oauth_loopback_server.cpp
    test_oauth_pkce.cpp
    test_secure_token_store.cpp

    # Job tests
    test_job_export_pcb_variants.cpp
    test_job_export_sch_plot.cpp
    test_job_sch_erc.cpp
    test_jobs_runner.cpp
    test_jobs_output_archive.cpp

    # Translation integrity tests
    test_translation_format_strings.cpp

    # Navlib safe initialization tests
    test_navlib_safe_init.cpp
)

if( KICAD_TEST_DATABASE_LIBRARIES )
    set( QA_COMMON_SRCS ${QA_COMMON_SRCS} test_database.cpp )
endif()

if( WIN32 )
    # We want to declare a resource manifest on Windows to enable UTF8 mode
    # Without UTF8 mode, some random IO tests may fail, we set the active code page on
    # normal kicad to UTF8 as well
    if( MINGW )
        # QA_COMMON_RESOURCES variable is set by the macro.
        mingw_resource_compiler( qa_common )
    else()
        set( QA_COMMON_RESOURCES ${CMAKE_SOURCE_DIR}/resources/msw/qa_common.rc )
    endif()
endif()


# Test executable for non-program-specific code
add_executable( qa_common
    ${QA_COMMON_SRCS}
    ${QA_COMMON_RESOURCES} )

target_link_libraries( qa_common
    common
    libcontext
    gal
    pads_common
    qa_utils
    Boost::headers
    Boost::unit_test_framework
    ${wxWidgets_LIBRARIES}
    ZLIB::ZLIB
    )

if( TARGET api_schema_build_copy )
    add_dependencies( qa_common api_schema_build_copy )
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/kicad
    ${CMAKE_SOURCE_DIR}/pcbnew
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/common/io/pads
    ${CMAKE_SOURCE_DIR}/qa/mocks/include
    ${INC_AFTER}
)

set_source_files_properties( test_remote_provider_metadata.cpp test_remote_provider_client.cpp
        test_translation_format_strings.cpp PROPERTIES
        COMPILE_DEFINITIONS "QA_SRC_ROOT=(\"${CMAKE_SOURCE_DIR}\")"
        )

if( KICAD_TEST_DATABASE_LIBRARIES )
    set_source_files_properties( test_database.cpp PROPERTIES
            COMPILE_DEFINITIONS "QA_DATABASE_FILE_LOCATION=(\"${CMAKE_SOURCE_DIR}/qa/data/dblib\")"
            )
endif()

kicad_add_boost_test( qa_common qa_common )

setup_qa_env( qa_common )
