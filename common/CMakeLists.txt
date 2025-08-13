# Add all the warnings to the files
if( COMPILER_SUPPORTS_WARNINGS )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARN_FLAGS_CXX}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARN_FLAGS_C}")
endif()

set( INC_AFTER ${INC_AFTER} ${NGSPICE_INCLUDE_DIR} )

include_directories( BEFORE ${INC_BEFORE} )
include_directories(
    ./dialogs
    ./widgets
    ./dialog_about
    ${CMAKE_SOURCE_DIR}/resources/bitmaps_png
    ${CMAKE_SOURCE_DIR}/3d-viewer
    ${CMAKE_SOURCE_DIR}/pcbnew
    ${CMAKE_SOURCE_DIR}/kicad
    ${INC_AFTER}
    )

# Get the GAL Target
add_subdirectory( gal )

# Only for win32 cross compilation using MXE
add_compile_definitions( $<$<AND:$<BOOL:${WIN32}>,$<BOOL:${MSYS}>>:GLEW_STATIC> )


# The build version string defaults to the value in the KiCadVersion.cmake file.
# If being built inside a git repository, the git tag and commit hash are used to create
# a new version string instead. The user can supply an additional string to be appended
# to the end inside the KICAD_VERSION_EXTRA variable
set( KICAD_VERSION_EXTRA "" CACHE STRING
    "User defined configuration string to append to KiCad version." )

# Generate version header file.
add_custom_target(
    version_header ALL
    COMMAND ${CMAKE_COMMAND}
        -DKICAD_VERSION_EXTRA=${KICAD_VERSION_EXTRA}
        -DOUTPUT_FILE=${CMAKE_BINARY_DIR}/kicad_build_version.h
        -DTEXT_OUTPUT_FILE=${CMAKE_BINARY_DIR}/kicad_build_version.txt
        -DSRC_PATH=${PROJECT_SOURCE_DIR}
        -DKICAD_CMAKE_MODULE_PATH=${KICAD_CMAKE_MODULE_PATH}
        -P ${KICAD_CMAKE_MODULE_PATH}/BuildSteps/WriteVersionHeader.cmake
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR} BYPRODUCTS ${CMAKE_BINARY_DIR}/kicad_build_version.h
    DEPENDS ${KICAD_CMAKE_MODULE_PATH}/BuildSteps/WriteVersionHeader.cmake
    COMMENT "Generating version string header"
    )

add_custom_target(
    banned_functions ALL
    COMMAND ${CMAKE_COMMAND}
        -DBANNED_DIRS="${PROJECT_SOURCE_DIR}/common/io;${PROJECT_SOURCE_DIR}/pcbnew/pcb_io;${PROJECT_SOURCE_DIR}/eeschema/sch_io"
        -P ${KICAD_CMAKE_MODULE_PATH}/CheckBannedFunctions.cmake
)
# A shared library used by multiple *.kiface files and one or two program
# launchers.  Object files can migrate into here over time, but only if they are
# surely needed and certainly used from more than one place without recompilation.
# Functions and data all need to use the #include <import_export.h> and be declared
# as APIEXPORT
set( KICOMMON_SRCS
    # Fonts
    newstroke_font.cpp
    font/fontconfig.cpp
    font/version_info.cpp

    # Gal
    gal/color4d.cpp
    gal/opengl/gl_context_mgr.cpp
    # Jobs
    jobs/job.cpp
    jobs/job_dispatcher.cpp
    jobs/job_registry.cpp
    jobs/jobs_output_archive.cpp
    jobs/jobs_output_folder.cpp
    jobs/job_export_pcb_drill.cpp
    jobs/job_export_pcb_dxf.cpp
    jobs/job_export_pcb_gerber.cpp
    jobs/job_export_pcb_gerbers.cpp
    jobs/job_export_pcb_hpgl.cpp
    jobs/job_export_pcb_ipc2581.cpp
    jobs/job_export_pcb_ipcd356.cpp
    jobs/job_export_pcb_odb.cpp
    jobs/job_export_pcb_pdf.cpp
    jobs/job_export_pcb_plot.cpp
    jobs/job_export_pcb_pos.cpp
    jobs/job_export_pcb_ps.cpp
    jobs/job_export_pcb_svg.cpp
    jobs/job_export_pcb_gencad.cpp
    jobs/job_export_pcb_3d.cpp
    jobs/job_export_sch_bom.cpp
    jobs/job_export_sch_netlist.cpp
    jobs/job_export_sch_plot.cpp
    jobs/job_export_sch_pythonbom.cpp
    jobs/job_special_copyfiles.cpp
    jobs/job_special_execute.cpp
    jobs/jobset.cpp
    jobs/job_fp_export_svg.cpp
    jobs/job_fp_upgrade.cpp
    jobs/job_pcb_render.cpp
    jobs/job_pcb_drc.cpp
    jobs/job_rc.cpp
    jobs/job_sch_erc.cpp
    jobs/job_sym_export_svg.cpp
    jobs/job_sym_upgrade.cpp

    kicad_curl/kicad_curl.cpp
    kicad_curl/kicad_curl_easy.cpp

    settings/app_settings.cpp
    settings/aui_settings.cpp
    settings/bom_settings.cpp
    settings/color_settings.cpp
    settings/common_settings.cpp
    settings/grid_settings.cpp
    settings/json_settings.cpp
    settings/kicad_settings.cpp
    settings/layer_settings_utils.cpp
    settings/nested_settings.cpp
    settings/parameters.cpp
    settings/settings_manager.cpp

    project/board_project_settings.cpp
    project/component_class_settings.cpp
    project/net_settings.cpp
    project/project_archiver.cpp
    project/project_file.cpp
    project/project_local_settings.cpp
    project/time_domain_parameters.cpp

    # This is basically a settings object, but for the toolbar
    tool/ui/toolbar_configuration.cpp

    dialogs/dialog_migrate_settings.cpp
    dialogs/dialog_migrate_settings_base.cpp
    dialogs/dialog_rc_job.cpp
    dialogs/dialog_rc_job_base.cpp

    widgets/bitmap_button.cpp
    widgets/html_window.cpp
    widgets/kistatusbar.cpp
    widgets/number_badge.cpp
    widgets/progress_reporter_base.cpp
    widgets/std_bitmap_button.cpp
    widgets/ui_common.cpp
    widgets/wx_html_report_panel.cpp
    widgets/wx_html_report_panel_base.cpp

    database/database_lib_settings.cpp

    design_block_lib_table.cpp
    design_block_io.cpp
    design_block_info.cpp
    design_block_info_impl.cpp

    advanced_config.cpp
    app_monitor.cpp
    asset_archive.cpp
    array_axis.cpp
    array_options.cpp
    background_jobs_monitor.cpp
    bitmap.cpp
    bitmap_info.cpp
    bitmap_store.cpp
    build_version.cpp
    cli_progress_reporter.cpp
    common.cpp
    config_params.cpp
    confirm.cpp
    dialog_shim.cpp
    dpi_scaling.cpp
    dpi_scaling_common.cpp
    dsnlexer.cpp
    eda_pattern_match.cpp
    eda_units.cpp
    env_vars.cpp
    exceptions.cpp
    gestfich.cpp
    increment.cpp
    json_conversions.cpp
    json_schema_validator.cpp
    kidialog.cpp
    kiid.cpp
    kiway.cpp
    kiway_express.cpp
    kiway_holder.cpp
    launch_ext.cpp
    lib_table_base.cpp
    layer_id.cpp
    lib_id.cpp
    locale_io.cpp
    lseq.cpp
    lset.cpp
    markup_parser.cpp
    netclass.cpp
    notifications_manager.cpp
    page_info.cpp
    paths.cpp
    project.cpp
    reporter.cpp
    richio.cpp
    search_stack.cpp
    searchhelpfilefullpath.cpp
    string_utils.cpp
    systemdirsappend.cpp
    thread_pool.cpp
    ui_events.cpp
    title_block.cpp
    trace_helpers.cpp
    wildcards_and_files_ext.cpp
    wx_filename.cpp

    singleton.cpp
    pgm_base.cpp

    ../scripting/python_scripting.cpp

    io/kicad/kicad_io_utils.cpp    # needed by richio
    io/io_base.cpp
    io/io_utils.cpp

    api/serializable.cpp
    api/api_utils.cpp
    )

if( UNIX AND NOT APPLE )
    list( APPEND KICOMMON_SRCS
        spacenav/libspnav_driver.cpp
    )
endif()

if( KICAD_IPC_API )
    set( KICOMMON_SRCS
        ${KICOMMON_SRCS}
        api/api_handler.cpp
        api/api_plugin.cpp
        api/api_plugin_manager.cpp
        api/api_server.cpp

        ../scripting/python_manager.cpp
    )
endif()

add_library( kicommon SHARED
    ${KICOMMON_SRCS}
    )

if( KICAD_USE_PCH )
    target_precompile_headers( kicommon
        PRIVATE
            <kicommon.h>
            <algorithm>
            <map>
            <memory>
            <optional>
            <set>
            <string>
            <vector>
            <wx/wx.h>
             )
endif()

set_target_properties(kicommon PROPERTIES CXX_VISIBILITY_PRESET hidden)

target_link_libraries( kicommon
    core
    kiapi
    kimath
    kiplatform
    nlohmann_json
    nlohmann_json_schema_validator
    fmt::fmt
    CURL::libcurl
    picosha2
    rapidcsv
    ${ZSTD_LIBRARY}
    ${wxWidgets_LIBRARIES}
    ${LIBGIT2_LIBRARIES}
    ${SPNAV_LIBRARIES}

    # needed by kiid to allow linking for Boost for the UUID against bcrypt (msys2 only)
    ${EXTRA_LIBS}

    # outline font support
   	${FREETYPE_LIBRARIES}
   	${HarfBuzz_LIBRARIES}
	${Fontconfig_LIBRARIES}

    # needed because of python_scripting.cpp
    ${PYTHON_LIBRARIES}
    )


if( KICAD_USE_SENTRY )
    target_link_libraries( kicommon
        sentry )

    set_property(SOURCE app_monitor.cpp APPEND PROPERTY COMPILE_DEFINITIONS KICAD_SENTRY_DSN="${KICAD_SENTRY_DSN}")
endif()

if( KICAD_IPC_API )
    target_link_libraries( kicommon
        kinng
        )
endif()

include( ${KICAD_CMAKE_MODULE_PATH}/KiCadVersion.cmake )
include( ${KICAD_CMAKE_MODULE_PATH}/CreateGitVersionHeader.cmake )
create_git_version_header(${CMAKE_SOURCE_DIR})

# Extract the major and minor build version as a string
string( REGEX MATCH
        "([0-9]+)\\.([0-9]+)\\.([0-9]+)"
        KICAD_MAJOR_MINOR_PATCH_VERSION
        "${KICAD_VERSION}"
    )

set_target_properties( kicommon PROPERTIES
    OUTPUT_NAME     kicommon
    SOVERSION      ${KICAD_MAJOR_MINOR_PATCH_VERSION}
    )

install( TARGETS kicommon
    RUNTIME DESTINATION ${KICAD_LIB}
    LIBRARY DESTINATION ${KICAD_LIB}
    COMPONENT binary
    )

if( APPLE )
    # puts library into the main kicad.app bundle in build tree
    set_target_properties( kicommon PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${OSX_BUNDLE_BUILD_LIB_DIR}"
        INSTALL_NAME_DIR "${OSX_BUNDLE_BUILD_LIB_DIR}"
    )
    set_target_properties( kicommon PROPERTIES INSTALL_RPATH
            "@executable_path/../Frameworks" )
    set_target_properties( kicommon PROPERTIES BUILD_WITH_INSTALL_RPATH 1 )
endif()

target_compile_definitions( kicommon PRIVATE KICOMMON_DLL=1 )

target_include_directories( kicommon
    PUBLIC
        .
        ${CMAKE_BINARY_DIR}
        $<TARGET_PROPERTY:dynamic_bitset,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:magic_enum,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:pegtl,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:expected,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:kiapi,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:picosha2,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:nlohmann_json_schema_validator,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:thread-pool,INTERFACE_INCLUDE_DIRECTORIES>
    )

add_dependencies( kicommon pegtl version_header )

if( MSVC )
    target_sources( kicommon PRIVATE ${CMAKE_SOURCE_DIR}/resources/msw/kicommon-dll.rc )
endif()

set( COMMON_ABOUT_DLG_SRCS
    dialog_about/AboutDialog_main.cpp
    dialog_about/dialog_about.cpp
    dialog_about/dialog_about_base.cpp
    )

set( COMMON_GIT_DLG_SRCS
    dialogs/git/dialog_git_commit.cpp
    dialogs/git/dialog_git_switch.cpp
    dialogs/git/dialog_git_auth.cpp
    dialogs/git/dialog_git_repository.cpp
    dialogs/git/dialog_git_repository_base.cpp
    dialogs/git/panel_git_repos.cpp
    dialogs/git/panel_git_repos_base.cpp

    )

set( COMMON_DLG_SRCS
    ${COMMON_GIT_DLG_SRCS}
    dialogs/dialog_assign_netclass.cpp
    dialogs/dialog_assign_netclass_base.cpp
    dialogs/dialog_book_reporter.cpp
    dialogs/dialog_book_reporter_base.cpp
    dialogs/dialog_color_picker.cpp
    dialogs/dialog_color_picker_base.cpp
    dialogs/dialog_configure_paths.cpp
    dialogs/dialog_configure_paths_base.cpp
    dialogs/dialog_design_block_properties.cpp
    dialogs/dialog_design_block_properties_base.cpp
    dialogs/dialog_display_html_text_base.cpp
    dialogs/dialog_edit_library_tables.cpp
    dialogs/dialog_global_lib_table_config.cpp
    dialogs/dialog_global_lib_table_config_base.cpp
    dialogs/dialog_global_design_block_lib_table_config.cpp
    dialogs/dialog_grid_settings.cpp
    dialogs/dialog_grid_settings_base.cpp
    dialogs/dialog_group_properties.cpp
    dialogs/dialog_group_properties_base.cpp
    dialogs/dialog_hotkey_list.cpp
    dialogs/dialog_HTML_reporter_base.cpp
    dialogs/dialog_import_choose_project.cpp
    dialogs/dialog_import_choose_project_base.cpp
    dialogs/dialog_multi_unit_entry.cpp
    dialogs/dialog_page_settings_base.cpp
    dialogs/dialog_paste_special.cpp
    dialogs/dialog_paste_special_base.cpp
    dialogs/dialog_plugin_options.cpp
    dialogs/dialog_plugin_options_base.cpp
    dialogs/dialog_text_entry_base.cpp
    dialogs/dialog_page_settings.cpp
    dialogs/dialog_print_generic.cpp
    dialogs/dialog_print_generic_base.cpp
    dialogs/dialog_text_entry.cpp
    dialogs/dialog_unit_entry.cpp
    dialogs/dialog_unit_entry_base.cpp
    dialogs/eda_list_dialog.cpp
    dialogs/eda_list_dialog_base.cpp
    dialogs/eda_reorderable_list_dialog.cpp
    dialogs/eda_reorderable_list_dialog_base.cpp
    dialogs/eda_view_switcher.cpp
    dialogs/eda_view_switcher_base.cpp
    dialogs/hotkey_cycle_popup.cpp
    dialogs/html_message_box.cpp
    dialogs/panel_color_settings_base.cpp
    dialogs/panel_color_settings.cpp
    dialogs/panel_common_settings.cpp
    dialogs/panel_common_settings_base.cpp
    dialogs/panel_design_block_lib_table_base.cpp
    dialogs/panel_design_block_lib_table.cpp
    dialogs/panel_embedded_files.cpp
    dialogs/panel_embedded_files_base.cpp
    dialogs/panel_gal_display_options.cpp
    dialogs/panel_hotkeys_editor.cpp
    dialogs/panel_image_editor.cpp
    dialogs/panel_image_editor_base.cpp
    dialogs/panel_grid_settings.cpp
    dialogs/panel_grid_settings_base.cpp
    dialogs/panel_maintenance.cpp
    dialogs/panel_maintenance_base.cpp
    dialogs/panel_mouse_settings.cpp
    dialogs/panel_mouse_settings_base.cpp
    dialogs/panel_spacemouse.cpp
    dialogs/panel_spacemouse_base.cpp
    dialogs/panel_packages_and_updates.cpp
    dialogs/panel_packages_and_updates_base.cpp
    dialogs/panel_plugin_settings.cpp
    dialogs/panel_plugin_settings_base.cpp
    dialogs/panel_setup_netclasses.cpp
    dialogs/panel_printer_list.cpp
    dialogs/panel_printer_list_base.cpp

    dialogs/panel_setup_netclasses_base.cpp
    dialogs/panel_setup_severities.cpp
    dialogs/panel_text_variables.cpp
    dialogs/panel_text_variables_base.cpp
    dialogs/panel_toolbar_customization.cpp
    dialogs/panel_toolbar_customization_base.cpp
    )

if( KICAD_USE_SENTRY )
    list( APPEND COMMON_DLG_SRCS
    dialogs/panel_data_collection.cpp
    dialogs/panel_data_collection_base.cpp )
endif()

set( COMMON_WIDGET_SRCS
    widgets/app_progress_dialog.cpp
    widgets/wx_bitmap_combobox.cpp
    widgets/bitmap_toggle.cpp
    widgets/button_row_panel.cpp
    widgets/color_swatch.cpp
    widgets/design_block_pane.cpp
    widgets/filter_combobox.cpp
    widgets/font_choice.cpp
    widgets/footprint_choice.cpp
    widgets/footprint_diff_widget.cpp
    widgets/footprint_preview_widget.cpp
    widgets/footprint_select_widget.cpp
    widgets/gal_options_panel.cpp
    widgets/gal_options_panel_base.cpp
    widgets/grid_bitmap_toggle.cpp
    widgets/grid_button.cpp
    widgets/grid_checkbox.cpp
    widgets/grid_color_swatch_helpers.cpp
    widgets/grid_combobox.cpp
    widgets/grid_icon_text_helpers.cpp
    widgets/grid_text_button_helpers.cpp
    widgets/grid_text_helpers.cpp
    widgets/indicator_icon.cpp
    widgets/wx_infobar.cpp
    widgets/layer_box_selector.cpp
    widgets/layer_presentation.cpp
    widgets/lib_tree.cpp
    widgets/listbox_tricks.cpp
    widgets/mathplot.cpp
    widgets/msgpanel.cpp
    widgets/paged_dialog.cpp
    widgets/panel_design_block_chooser.cpp
    widgets/properties_panel.cpp
    widgets/search_pane.cpp
    widgets/search_pane_base.cpp
    widgets/search_pane_tab.cpp
    widgets/split_button.cpp
    widgets/stepped_slider.cpp
    widgets/text_ctrl_eval.cpp
    widgets/unit_binder.cpp
    widgets/widget_save_restore.cpp
    widgets/widget_hotkey_list.cpp
    widgets/wx_aui_art_providers.cpp
    widgets/wx_aui_utils.cpp
    widgets/wx_busy_indicator.cpp
    widgets/wx_collapsible_pane.cpp
    widgets/wx_combobox.cpp
    widgets/wx_dataviewctrl.cpp
    widgets/wx_ellipsized_static_text.cpp
    widgets/wx_grid.cpp
    widgets/wx_grid_autosizer.cpp
    widgets/wx_html_report_box.cpp
    widgets/wx_listbox.cpp
    widgets/wx_panel.cpp
    widgets/wx_progress_reporters.cpp
    widgets/wx_splitter_window.cpp
    widgets/wx_treebook.cpp
    widgets/webview_panel.cpp
    )

set( COMMON_DRAWING_SHEET_SRCS
    drawing_sheet/ds_data_item.cpp
    drawing_sheet/ds_data_model.cpp
    drawing_sheet/ds_data_model_io.cpp
    drawing_sheet/drawing_sheet_default_description.cpp
    drawing_sheet/ds_draw_item.cpp
    drawing_sheet/ds_proxy_undo_item.cpp
    drawing_sheet/ds_proxy_view_item.cpp
    drawing_sheet/drawing_sheet_parser.cpp
    )

set( COMMON_PREVIEW_ITEMS_SRCS
    preview_items/anchor_debug.cpp
    preview_items/arc_assistant.cpp
    preview_items/arc_geom_manager.cpp
    preview_items/bezier_assistant.cpp
    preview_items/bezier_geom_manager.cpp
    preview_items/centreline_rect_item.cpp
    preview_items/construction_geom.cpp
    preview_items/draw_context.cpp
    preview_items/item_drawing_utils.cpp
    preview_items/polygon_geom_manager.cpp
    preview_items/polygon_item.cpp
    preview_items/preview_utils.cpp
    preview_items/ruler_item.cpp
    preview_items/selection_area.cpp
    preview_items/simple_overlay_item.cpp
    preview_items/snap_indicator.cpp
    preview_items/two_point_assistant.cpp
    )

set( PLOTTERS_CONTROL_SRCS
    plotters/plotter.cpp
    plotters/DXF_plotter.cpp
    plotters/GERBER_plotter.cpp
    plotters/PDF_plotter.cpp
    plotters/PS_plotter.cpp
    plotters/SVG_plotter.cpp
    plotters/common_plot_functions.cpp
    )

set( COMMON_IO_SRCS

    # Altium
    io/altium/altium_binary_parser.cpp
    io/altium/altium_ascii_parser.cpp
    io/altium/altium_parser_utils.cpp
    io/altium/altium_props_utils.cpp

    # Cadstar
    io/cadstar/cadstar_archive_parser.cpp
    io/cadstar/cadstar_parts_lib_parser.cpp

    # Eagle
    io/eagle/eagle_parser.cpp

    # EasyEDA
    io/easyeda/easyeda_parser_base.cpp
    io/easyeda/easyeda_parser_structs.cpp

    # EasyEDA pro
    io/easyedapro/easyedapro_parser.cpp
    io/easyedapro/easyedapro_import_utils.cpp

    io/csv.cpp
    )

set ( COMMON_TRANSLINE_CALCULATION_SRCS
    transline_calculations/transline_calculation_base.cpp
    transline_calculations/microstrip.cpp
    transline_calculations/coupled_microstrip.cpp
    transline_calculations/stripline.cpp
    transline_calculations/coupled_stripline.cpp
    )

set( COMMON_IMPORT_GFX_SRCS
    import_gfx/graphics_import_mgr.cpp
    import_gfx/graphics_importer.cpp
    import_gfx/graphics_importer_buffer.cpp
    import_gfx/dxf_import_plugin.cpp
    import_gfx/svg_import_plugin.cpp
    )

set( COMMON_GIT_SRCS
    git/git_add_to_index_handler.cpp
    git/git_branch_handler.cpp
    git/git_clone_handler.cpp
    git/git_commit_handler.cpp
    git/git_config_handler.cpp
    git/git_init_handler.cpp
    git/git_pull_handler.cpp
    git/git_push_handler.cpp
    git/git_remove_from_index_handler.cpp
    git/git_resolve_conflict_handler.cpp
    git/git_revert_handler.cpp
    git/git_status_handler.cpp
    git/git_sync_handler.cpp
    git/project_git_utils.cpp
    git/kicad_git_common.cpp
    git/kicad_git_errors.cpp
    git/project_git_utils.cpp
    )

set( COMMON_SRCS
    ${LIB_KICAD_SRCS}
    ${COMMON_ABOUT_DLG_SRCS}
    ${COMMON_DLG_SRCS}
    ${COMMON_WIDGET_SRCS}
    ${COMMON_DRAWING_SHEET_SRCS}
    ${COMMON_PREVIEW_ITEMS_SRCS}
    ${PLOTTERS_CONTROL_SRCS}
    ${COMMON_IO_SRCS}
	${FONT_SRCS}
    ${COMMON_IMPORT_GFX_SRCS}
    ${COMMON_GIT_SRCS}
    ${COMMON_TRANSLINE_CALCULATION_SRCS}
    base_screen.cpp
    bin_mod.cpp
    bitmap_base.cpp
    board_printout.cpp
    clipboard.cpp
    commit.cpp
    draw_panel_gal.cpp
    gal_display_options_common.cpp
    gr_text.cpp
    eda_base_frame.cpp
    eda_dde.cpp
    eda_doc.cpp
    eda_draw_frame.cpp
    eda_group.cpp
    eda_item.cpp
    eda_shape.cpp
    eda_text.cpp
    embedded_files.cpp
    env_paths.cpp
    executable_names.cpp
    filename_resolver.cpp
    file_history.cpp
    filter_reader.cpp
    footprint_filter.cpp
    footprint_info.cpp
    gbr_metadata.cpp
    gr_basic.cpp
    grid_tricks.cpp
    hotkey_store.cpp
    hotkeys_basic.cpp
    kiface_base.cpp
    kiway_player.cpp
    lib_table_grid_tricks.cpp
    lib_tree_model.cpp
    lib_tree_model_adapter.cpp
    design_block_tree_model_adapter.cpp
    marker_base.cpp
    origin_transforms.cpp
    printout.cpp
    ptree.cpp
    rc_item.cpp
    refdes_utils.cpp
    reference_image.cpp
    render_settings.cpp
    scintilla_tricks.cpp
    status_popup.cpp
    stroke_params.cpp
    template_fieldnames.cpp
    textentry_tricks.cpp
    undo_redo_container.cpp
    validators.cpp
    drawing_sheet/ds_painter.cpp
    xnode.cpp
    view/wx_view_controls.cpp
    )

set( COMMON_SRCS
    ${COMMON_SRCS}

    origin_viewitem.cpp

    tool/action_manager.cpp
    tool/action_menu.cpp
    tool/action_toolbar.cpp
    tool/actions.cpp
    tool/common_control.cpp
    tool/construction_manager.cpp
    tool/common_tools.cpp
    tool/conditional_menu.cpp
    tool/design_block_control.cpp
    tool/edit_constraints.cpp
    tool/edit_points.cpp
    tool/editor_conditions.cpp
    tool/embed_tool.cpp
    tool/grid_helper.cpp
    tool/grid_menu.cpp
    tool/group_tool.cpp
    tool/library_editor_control.cpp
    tool/picker_tool.cpp
    tool/point_editor_behavior.cpp
    tool/properties_tool.cpp
    tool/selection.cpp
    tool/selection_tool.cpp
    tool/selection_conditions.cpp
    tool/tool_action.cpp
    tool/tool_base.cpp
    tool/tool_dispatcher.cpp
    tool/tool_event.cpp
    tool/tools_holder.cpp
    tool/tool_interactive.cpp
    tool/tool_manager.cpp
    tool/tool_menu.cpp
    tool/zoom_menu.cpp
    tool/zoom_tool.cpp

    settings/cvpcb_settings.cpp

    properties/color4d_variant.cpp
    properties/eda_angle_variant.cpp
    properties/pg_cell_renderer.cpp
    properties/pg_editors.cpp
    properties/pg_properties.cpp
    properties/property_mgr.cpp
    properties/std_optional_variants.cpp

    database/database_connection.cpp

    http_lib/http_lib_connection.cpp
    http_lib/http_lib_settings.cpp

    api/api_enums.cpp
    )

# This needs to be in the common library until draw_panel_gal is moved to kicommon
if( UNIX AND NOT APPLE )
    list( APPEND COMMON_SRCS
        spacenav/spnav_2d_plugin.cpp
    )
endif()

    if( KICAD_IPC_API )
    set( COMMON_SRCS
        ${COMMON_SRCS}
        api/api_handler_common.cpp
        api/api_handler_editor.cpp
    )
endif()

add_library( common STATIC
    ${COMMON_SRCS}
    )

if( KICAD_USE_PCH )
    target_precompile_headers( kicommon
        PRIVATE
            <kicommon.h>
            <algorithm>
            <map>
            <memory>
            <optional>
            <set>
            <string>
            <vector>
            <wx/wx.h>
             )
endif()

add_dependencies( common version_header )
add_dependencies( common compoundfilereader )  # used by altium_parser.cpp

target_link_libraries( common
    libcontext
    kimath
    kiplatform
    kicommon
    core
    fmt::fmt
    gal
    nanosvg
    dxflib_qcad
    tinyspline_lib
    scripting
    nlohmann_json
    pybind11::embed
    compoundfilereader
    Boost::headers
    # Database support needs these two
    nanodbc # for now; maybe hoist out of common
    Boost::locale
    ${wxWidgets_LIBRARIES}
    ${EXTRA_LIBS}
    # outline font support
   	${FREETYPE_LIBRARIES}
   	${HarfBuzz_LIBRARIES}
	${Fontconfig_LIBRARIES}
    )

if( UNIX AND NOT APPLE )
    target_link_libraries( common ${Poppler_LIBRARIES} )
endif()

if( KICAD_USE_SENTRY )
    target_link_libraries( common
        sentry )
endif()

if( KICAD_IPC_API )
    target_link_libraries( common
        kinng
        )
endif()

target_include_directories( common
    PUBLIC
        .
        ${CMAKE_BINARY_DIR}
        $<TARGET_PROPERTY:argparse::argparse,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:kiapi,INTERFACE_INCLUDE_DIRECTORIES>
    )

# text markup support
add_dependencies( common pegtl )

target_include_directories( common PUBLIC
    $<TARGET_PROPERTY:pegtl,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:magic_enum,INTERFACE_INCLUDE_DIRECTORIES>
    )

target_include_directories( common SYSTEM PUBLIC
    $<TARGET_PROPERTY:nanodbc,INTERFACE_INCLUDE_DIRECTORIES>
    )

set( PCB_COMMON_SRCS
    fp_lib_table.cpp
    hash_eda.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_base_frame.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcbexpr_evaluator.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcbexpr_functions.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/board_commit.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/board_connected_item.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/board_design_settings.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/teardrop/teardrop_parameters.cpp     #needed by board_design_settings.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/router/pns_meander.cpp               #needed by board_design_settings.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/board.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/component_classes/component_class.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/component_classes/component_class_assignment_rule.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/component_classes/component_class_cache_proxy.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/component_classes/component_class_manager.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/board_item.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_dimension.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_shape.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_group.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_marker.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/footprint.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/fix_board_shape.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/netinfo_item.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/netinfo_list.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pad.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pad_utils.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/padstack.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_target.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_reference_image.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_field.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_table.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_tablecell.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_text.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_textbox.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/project_pcb.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/board_stackup_manager/board_stackup.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_track.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_generator.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/zone.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/collectors.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/connectivity/connectivity_algo.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/connectivity/connectivity_items.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/connectivity/connectivity_data.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/connectivity/from_to_cache.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/convert_shape_list_to_polygon.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/drc/drc_engine.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/drc/drc_cache_generator.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/drc/drc_item.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/drc/drc_rule.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/drc/drc_rule_condition.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/drc/drc_rule_parser.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/drc/drc_test_provider.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/footprint_editor_settings.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/generators_mgr.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/kicad_clipboard.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/netlist_reader/kicad_netlist_reader.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/netlist_reader/legacy_netlist_reader.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/netlist_reader/netlist_reader.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_draw_panel_gal.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/netlist_reader/pcb_netlist.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_origin_transforms.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_painter.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_plot_params.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_screen.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_view.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcbnew_settings.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/ratsnest/ratsnest_data.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/ratsnest/ratsnest_view_item.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/sel_layer.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/teardrop/teardrop.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/teardrop/teardrop_parameters.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/teardrop/teardrop_utils.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/zone_settings.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_board_outline.cpp

    # IO files
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_io/pcb_io.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_io/pcb_io_mgr.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_io/kicad_legacy/pcb_io_kicad_legacy.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_io/kicad_sexpr/pcb_io_kicad_sexpr_parser.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_io/eagle/pcb_io_eagle.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_io/geda/pcb_io_geda.cpp

    ${CMAKE_SOURCE_DIR}/pcbnew/tools/pcb_grid_helper.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/tools/pcb_actions.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/tools/pcb_editor_conditions.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/tools/pcb_viewer_tools.cpp

    ${CMAKE_SOURCE_DIR}/pcbnew/length_delay_calculation/length_delay_calculation.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/length_delay_calculation/length_delay_calculation_item.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/length_delay_calculation/time_domain_parameters_user_defined.cpp

    widgets/net_selector.cpp
)

set( PCB_COMMON_SRCS
    ${PCB_COMMON_SRCS}
    ${CMAKE_SOURCE_DIR}/pcbnew/api/api_pcb_enums.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/api/api_pcb_utils.cpp
)

add_library( pcbcommon STATIC ${PCB_COMMON_SRCS} )

if( KICAD_USE_PCH )
    target_precompile_headers( pcbcommon
        PRIVATE
            <vector>
            <map>
            <memory>
            <unordered_map>
            <board.h>
            <footprint.h>
            <zone.h>
            <connectivity/connectivity_data.h>
            <drc/drc_engine.h> )
endif()

target_compile_definitions(pcbcommon PRIVATE PCBNEW)

target_include_directories( pcbcommon PRIVATE
    )

target_link_libraries( pcbcommon PUBLIC
    core
    common
    delaunator
    kimath
    kiplatform
    nanoflann
)

if( APPLE OR NOT UNIX )
    # On Windows/MacOS, the SpaceMouse support is provided by the navlib subdirectory
    message( STATUS "Including 3Dconnexion SpaceMouse navigation support in pcbcommon" )
    add_subdirectory( ../pcbnew/navlib ./navlib)
    target_link_libraries( pcbcommon PUBLIC pcbnew_navlib)
endif()

add_dependencies( pcbcommon delaunator )

# The lemon grammar for the numeric evaluator
generate_lemon_grammar(
    kicommon
    libeval
    libeval/numeric_evaluator.cpp
    libeval/grammar.lemon
    )

# The lemon grammar for the expression compiler
generate_lemon_grammar(
    kicommon
    libeval_compiler
    libeval_compiler/libeval_compiler.cpp
    libeval_compiler/grammar.lemon
    )

# auto-generate stroke_params_lexer.h and stroke_params_keywords.cpp
# Called twice one for common and one for gal, to ensure the files are created
# on all devel tools ( Linux and msys2 )
# works on Linux:
make_lexer_export(
    kicommon
    stroke_params.keywords
    stroke_params_lexer.h
    stroke_params_keywords.cpp
    STROKEPARAMS_T
    KICOMMON_API
    kicommon.h
    )

# auto-generate netlist_lexer.h and netlist_keywords.cpp
make_lexer_export(
    kicommon
    netlist.keywords
    netlist_lexer.h
    netlist_keywords.cpp
    NL_T
    KICOMMON_API
    kicommon.h
    )

# auto-generate pcb_plot_params_lexer.h and pcb_plot_params_keywords.cpp
make_lexer(
    pcbcommon
    pcb_plot_params.keywords
    pcb_plot_params_lexer.h
    pcb_plot_params_keywords.cpp
    PCBPLOTPARAMS_T
    )

# auto-generate drc_rules_lexer.h and drc_rules_keywords.cpp
make_lexer_export(
    kicommon
    drc_rules.keywords
    drc_rules_lexer.h
    drc_rules_keywords.cpp
    DRCRULE_T
    KICOMMON_API
    kicommon.h
    )


# auto-generate pcbnew_sexpr.h and pcbnew_sexpr.cpp
make_lexer(
    pcbcommon
    pcb.keywords
    pcb_lexer.h
    pcb_keywords.cpp
    PCB_KEYS_T
    )

# auto-generate s-expression library table code.
make_lexer_export(
    kicommon
    lib_table.keywords
    lib_table_lexer.h
    lib_table_keywords.cpp
    LIB_TABLE_T
    KICOMMON_API
    kicommon.h
    )

# auto-generate s-expression template fieldnames lexer and keywords.
make_lexer_export(
    kicommon
    template_fieldnames.keywords
    template_fieldnames_lexer.h
    template_fieldnames_keywords.cpp
    TFIELD_T
    KICOMMON_API
    kicommon.h
    )

# auto-generate page layout reader s-expression page_layout_reader_lexer.h
# and title_block_reader_keywords.cpp.
make_lexer_export(
    kicommon
    drawing_sheet/drawing_sheet.keywords
    drawing_sheet/drawing_sheet_lexer.h
    drawing_sheet/drawing_sheet_keywords.cpp
    DRAWINGSHEET_T
    KICOMMON_API
    kicommon.h
    )

# auto-generate embedded files lexer and keywords
make_lexer_export(
    kicommon
    embedded_files.keywords
    embedded_files_lexer.h
    embedded_files_keywords.cpp
    EMBEDDED_FILES_T
    KICOMMON_API
    kicommon.h
)

# This one gets made only when testing.
# to build it, first enable #define STAND_ALONE at top of dsnlexer.cpp
add_executable( dsntest EXCLUDE_FROM_ALL dsnlexer.cpp )
target_link_libraries( dsntest common ${wxWidgets_LIBRARIES} rt )
