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
# along with this program; if not, you may find one here:
# http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
# or you may search the http://www.gnu.org website for the version 2 license,
# or you may write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

find_package( wxWidgets 3.0.0 COMPONENTS gl aui adv html core net base xml stc REQUIRED )

set( QA_COMMON_SRCS
    # This is needed for the global mock objects
    ${CMAKE_SOURCE_DIR}/qa/mocks/kicad/common_mocks.cpp

    # The main test entry points
    test_module.cpp

    wximage_test_utils.cpp

    test_angle_item.cpp

    test_array_axis.cpp
    test_base_set.cpp
    test_bitmap_base.cpp
    test_clipboard.cpp
    test_collector.cpp
    test_color4d.cpp
    test_commit.cpp
    test_coroutine.cpp
    test_eda_shape.cpp
    test_eda_text.cpp
    test_embedded_file_compress.cpp
    test_expand_text_vars.cpp
    test_file_history.cpp
    test_filename_resolver.cpp
    test_hotkey_store.cpp
    test_increment.cpp
    test_ki_any.cpp
    test_library_tables.cpp
    test_markup_parser.cpp
    test_kicad_string.cpp
    test_kicad_stroke_font.cpp
    test_pdf_unicode_plot.cpp
    test_kiid.cpp
    test_layer_ids.cpp
    test_layer_range.cpp
    test_lset.cpp
    test_markup_parser.cpp
    test_notifications_manager.cpp
    test_property.cpp
    test_property_holder.cpp
    test_reporting.cpp
    test_refdes_utils.cpp
    test_grid_helper.cpp
    test_string_utils.cpp
    test_richio.cpp
    test_text_attributes.cpp
    text_eval/test_text_eval_parser.cpp
    text_eval/test_text_eval_parser_core.cpp
    text_eval/test_text_eval_parser_datetime.cpp
    text_eval/test_text_eval_parser_integration.cpp
    text_eval/test_text_eval_numeric_compat.cpp
    text_eval/test_text_eval_render.cpp
    test_title_block.cpp
    test_types.cpp
    test_utf8.cpp
    test_wildcards_and_files_ext.cpp
    test_wx_filename.cpp

    libeval/test_numeric_evaluator.cpp

    io/test_csv.cpp
    io/altium/test_altium_parser.cpp
    io/altium/test_altium_parser_utils.cpp
    io/cadstar/test_cadstar_parts_parser.cpp

    io/cadstar/test_cadstar_archive_parser.cpp

    view/test_zoom_controller.cpp

    # GAL tests
    test_gal_xor_mode.cpp

    # Graphics import tests
    test_graphics_importer_buffer.cpp

    # Project template tests (source included directly since it's from kicad executable)
    test_project_template.cpp
    ${CMAKE_SOURCE_DIR}/kicad/project_template.cpp
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
    qa_utils
    Boost::headers
    Boost::unit_test_framework
    ${wxWidgets_LIBRARIES}
    ZLIB::ZLIB
    )

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/kicad
    ${CMAKE_SOURCE_DIR}/pcbnew
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/qa/mocks/include
    ${INC_AFTER}
)

if( KICAD_TEST_DATABASE_LIBRARIES )
    set_source_files_properties( test_database.cpp PROPERTIES
            COMPILE_DEFINITIONS "QA_DATABASE_FILE_LOCATION=(\"${CMAKE_SOURCE_DIR}/qa/data/dblib\")"
            )
endif()

kicad_add_boost_test( qa_common qa_common )

setup_qa_env( qa_common )
