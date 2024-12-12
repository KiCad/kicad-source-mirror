#  This program source code file is part of KICAD, a free EDA CAD application.
#
# Copyright (C) 2011 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
# Copyright (C) 2011-2022 Kicad Developers, see AUTHORS.txt for contributors.
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, you may find one here:
#  http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
#  or you may search the http://www.gnu.org website for the version 2 license,
#  or you may write to the Free Software Foundation, Inc.,
#  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
#

# In this directory, a number of different processes are managed:
#
# 1) PNG files are created from SVG files for each icon theme and needed bitmap resolution
# 2) An images.tar.gz file is created from all the PNG files

# Step 2) is universal and is done for any builder, whereas step 1) is
# optional and depend on MAINTAIN_PNGS being defined in CMake.  The reason we
# can skip 1) is that the *.PNG files are part of the source tree so
# do not need to rebuilt by a typical builder.  However, because the *.PNG files
# are part of the source tree, and subject to version control, they should be built
# only when needed, otherwise this results in a diff for the version control system.
# Therefore step 1) is driven by CMake (if MAINTAIN_PNGS) which gives
# us conditional *.PNG building based on an edit to the respective *.SVG file.

# If MAINTAIN_PNGS is not defined, then you are a normal builder and no special
# tools are required.  If MAINTAIN_PNGS is defined, then you are a PNG maintainer
# and will need the following tools findable in your PATH:
#
# 1) inkscape - command line mode is used, must be on your PATH
# 2) pngcrush - this program must be on your PATH
#

# lower case is used for local variables, uppercase for global variables

option( MAINTAIN_PNGS
        "Set to true if you are a PNG maintainer and have the required tools given in the resources/bitmaps_png/CMakeLists.txt file (default OFF)."
        OFF)

# Used Only to maintain PNG files (and therefore to recreate .cpp files)
# The png2cpp creates files with native End of Line format.

# List of themes - each corresponds to a folder of SVGS in ./resources/bitmaps_png/sources/
set( THEMES
    light
    dark )

# Plan for three sizes of bitmaps:
# SMALL - for menus         - 16 x 16
# MID   - for toolbars      - 24 x 24
# BIG   - for program icons - 48 x 48
# which are given on three basename lists: BMAPS_SMALL, BMAPS_MID,and BMAPS_BIG
# respectively.  The basename is without file extension and without path.
# A corresponding ${basename}.svg file must exist in 'sources' dir below here.


# small icons (16x16) needed in listboxes and dialog buttons
set( BMAPS_SMALL
    e_24
    e_48
    e_96
    e_192
    git_add
    git_changed_ahead
    git_conflict
    git_delete
    git_good_check
    git_modified
    git_out_of_date
    icon_bitmap2component_16
    icon_eeschema_16
    icon_gerbview_16
    icon_kicad_16
    icon_kicad_nightly_16
    icon_libedit_16
    icon_modedit_16
    icon_pagelayout_editor_16
    icon_pcbcalculator_16
    icon_pcbnew_16
    label_align_left
    label_align_right
    label_align_top
    label_align_bottom
    list_nets_16
    notifications
    options_generic_16
    pinorient_right
    pinorient_left
    pinorient_up
    pinorient_down
    pinshape_normal
    pinshape_invert
    pinshape_clock_normal
    pinshape_clock_invert
    pinshape_active_low_input
    pinshape_clock_active_low
    pinshape_active_low_output
    pinshape_clock_fall
    pinshape_nonlogic
    pintype_input
    pintype_output
    pintype_bidi
    pintype_3states
    pintype_passive
    pintype_notspecif
    pintype_powerinput
    pintype_poweroutput
    pintype_opencoll
    pintype_openemit
    pintype_nic
    pintype_noconnect
    small_down
    small_edit
    small_folder
    small_library
    small_new_window
    small_plus
    small_refresh
    small_sort_desc
    small_trash
    small_up
    small_warning
    text_horizontal
    text_vertical
    text_align_left
    text_align_center
    text_align_right
    text_align_bottom
    text_align_middle
    text_align_top
    text_valign_top
    text_valign_center
    text_valign_bottom
    text_visibility
    text_visibility_off
    text_bold
    text_italic
    text_mirrored
    tree_nosel
    tree_sel
    visibility
    visibility_off
    www
    )


# image basenames that go into the toolbar sized destinations, i.e. 24x24
set( BMAPS_MID
    about
    add_aligned_dimension
    add_arc
    add_barcode
    add_bezier
    add_board
    add_bus2bus
    add_bus
    add_center_dimension
    add_class_flag
    add_circle
    add_component
    add_corner
    add_dashed_line
    add_document
    add_glabel
    add_graphical_segments
    add_graphical_polygon
    add_hierarchical_label
    add_hierar_pin
    add_hierarchical_subsheet
    add_junction
    add_keepout_area
    add_label
    add_leader
    add_library
    add_line2bus
    add_line_label
    add_line
    add_orthogonal_dimension
    add_pcb_target
    add_point
    add_power
    add_radial_dimension
    add_rectangle
    add_symbol_to_schematic
    add_textbox
    add_tracks
    add_via
    add_zone_cutout
    add_zone
    align_elements_to_grid
    align_items
    align_items_left
    align_items_right
    align_items_top
    align_items_bottom
    align_items_center
    align_items_middle
    anchor
    annotate_increment
    annotate_down_right
    annotate_right_down
    annotate
    apply_pad_settings
    array
    auto_associate
    auto_track_width
    autoplace_fields
    axis3d_back
    axis3d_bottom
    axis3d_front
    axis3d_left
    axis3d_right
    axis3d_top
    axis3d
    body_style
    break_line
    bug
    bus_definition_tool
    cancel
    chamfer
    change_entry_orient
    checked_ok
    cleanup_graphics
    cleanup_tracks_and_vias
    color_materials
    component_select_unit
    config
    contrast_mode
    convert
    copy
    copy_pad_settings
    cursor_fullscreen
    cursor_fullscreen45
    cursor_shape
    cursor
    custom_pad_to_primitives
    curved_ratsnest
    cut
    datasheet
    delete_association
    delete_cursor
    delete_gerber
    delete_sheet
    directory
    directory_browser
    directory_open
    distribute_horizontal_centers
    distribute_horizontal_gaps
    distribute_vertical_centers
    distribute_vertical_gaps
    down
    drag
    drag_segment
    drag_segment_withslope
    drc
    duplicate
    edit_comp_footprint
    edit_comp_ref
    edit_comp_value
    editor
    edit_cmp_symb_links
    edit
    edge_to_copper_clearance
    enter_sheet
    ercerr
    erc_green
    ercwarn
    erc
    exit
    exchange
    export3d
    export_cmp
    export_dsn
    export_idf
    export_file
    export_footprint_names
    export_gltf
    export_module
    export_part
    export_png
    export_step
    export_svg
    export_to_pcbnew
    fabrication
    file_bom
    file_cir
    file_drl
    file_dsn
    file_gbr
    file_gerber_job
    file_html
    file_idf
    file_pdf
    file_pos
    file_svg
    fill_zone
    fillet
    filter
    find
    find_replace
    flag
    flip_board
    gbr_select_mode1
    gbr_select_mode2
    gerbview_show_negative_objects
    general_deletions
    general_ratsnest
    grid_select
    grid_select_axis
    grid_override
    grid
    group
    group_enter
    group_leave
    group_remove
    group_ungroup
    heal_shapes
    help
    help_online
    hidden_pin
    hide_ratsnest
    hierarchy_nav
    hotkeys
    hole_to_hole_clearance
    hole_to_copper_clearance
    hv45mode
    icon_bitmap2component_24
    icon_cvpcb_24
    icon_eeschema_24
    icon_footprint_browser
    icon_gerbview_24
    icon_kicad_24
    icon_libedit_24
    icon_modedit_24
    icon_pagelayout_editor_24
    icon_pcbcalculator_24
    icon_pcbnew_24
    icon_pcm_24
    import3d
    image
    import_brd_file
    import_document
    import_footprint_names
    import_hierarchical_label
    import_module
    import_part
    import_project
    import_vector
    import
    info
    insert_module_board
    intersect_polygons
    jobset
    jobset_open
    language
    lasso
    layers_manager
    leave_sheet
    left
    libedit
    lib_next
    lib_previous
    library_browser
    library_archive
    library_archive_as
    library
    library_table
    lines_any
    lines90
    list_nets
    load_drill
    load_gerber
    load_module_board
    lock_unlock
    locked
    marker_exclude
    marker_next
    marker_previous
    measurement
    merge_polygons
    mirror_h
    mirror_v
    mode_module
    module_editor
    module_wizard
    module_filtered_list
    module_options
    module_pin_filtered_list
    module_library_list
    module
    morgan1
    morgan2
    move_exactly
    move
    move_relative
    mw_add_gap
    mw_add_line
    mw_add_shape
    mw_add_stub_arc
    mw_add_stub
    net_highlight
    net_highlight_schematic
    netlist
    new_component
    new_footprint
    new_generic
    new_library
    new_project
    new_project_from_template
    noconn
    normal
    open_project
    open_project_demo
    options_3drender
    options_board
    options_generic
    options_pad
    options_schematic
    opt_show_polygon
    ortho
    outset_from_selection
    pack_footprints
    pad_sketch
    pad
    pad_enumerate
    pad_number
    pads_mask_layers
    pads_remove
    path
    pagelayout_normal_view_mode
    pagelayout_special_view_mode
    part_properties
    paste
    paste_special
    pcb_target
    pin2pin
    pin_size_to
    pin_show_etype
    pin_table
    pin
    plot
    polar_coord
    post_bom
    post_compo
    post_d356
    post_drill
    post_gencad
    post_gerber
    post_rpt
    post_xml
    post_odb
    preference
    print_button
    project
    project_clone
    project_close
    project_kicad
    ps_diff_pair
    ps_diff_pair_gap
    ps_diff_pair_tune_length
    ps_diff_pair_tune_phase
    ps_tune_length
    push_pad_settings
    puzzle_piece
    py_script
    ray_tracing
    recent
    redo
    refresh
    reload
    rescue
    restore_from_file
    right
    router_len_tuner
    router_len_tuner_setup
    router_len_tuner_amplitude_decr
    router_len_tuner_amplitude_incr
    router_len_tuner_dist_decr
    router_len_tuner_dist_incr
    rotate_ccw
    rotate_cw
    rotate_ccw_x
    rotate_cw_x
    rotate_ccw_y
    rotate_cw_y
    rotate_ccw_z
    rotate_cw_z
    save_as
    save
    select_layer_pair
    select_w_layer
    select_same_sheet
    shape_3d
    shape_3d_back
    sheetset
    show_board_body
    simulator
    sim_add_plot
    sim_command
    sim_run
    sim_stop
    sim_tune
    sim_probe
    sim_add_signal
    slice_line
    search_tree
    set_origin
    show_dcodenumber
    show_dnp
    show_footprint
    show_mod_edge
    show_ratsnest
    showtrack
    show_not_in_posfile
    show_other
    show_tht
    show_smt
    show_zone
    show_zone_disable
    show_zone_outline_only
    show_zone_triangulation
    show_all_layers
    show_no_layers
    show_no_copper_layers
    show_all_copper_layers
    show_all_front_layers
    show_all_back_layers
    show_front_assembly_layers
    show_back_assembly_layers
    special_tools
    spreadsheet
    subtract_polygons
    swap
    swap_layer
    switch_corner_rounding_shape
    table
    table_add_column_after
    table_add_column_before
    table_add_row_above
    table_add_row_below
    table_delete_column
    table_delete_row
    table_edit
    table_select
    table_select_column
    table_select_row
    text
    text_sketch
    thermal_spokes
    three_d
    tool_ratsnest
    tools
    trash
    undo
    unit_inch
    unit_mil
    unit_mm
    unknown
    unlocked
    unzip
    up
    update_pcb_from_sch
    update_sch_from_pcb
    via
    via_annulus
    via_buried
    via_microvia
    via_sketch
    via_diameter
    via_hole_diameter
    width_conn
    width_track_via
    width_track
    zip
    zone_duplicate
    zone_fillet
    zone_unfill
    zoom_area
    zoom_auto_fit_in_page
    zoom_fit_in_page
    zoom_fit_to_objects
    zoom_center_on_screen
    zoom_in
    zoom_out
    zoom_in_horizontally
    zoom_out_horizontally
    zoom_in_vertically
    zoom_out_vertically
    zoom_selection
    )

# 24 x 24 for internal icons
set( BMAPS_24
    )

# 32 x 32 for icon sets
set( BMAPS_32
    icon_bitmap2component_32
    icon_eeschema_32
    icon_gerbview_32
    icon_kicad_32
    icon_kicad_nightly_32
    icon_libedit_32
    icon_modedit_32
    icon_pagelayout_editor_32
    icon_pcbcalculator_32
    icon_pcbnew_32
    )

# 48 x 48 for now
set( BMAPS_BIG
    dialog_warning
    icon_3d
    icon_cvpcb
    icon_eeschema
    icon_gerbview
    icon_kicad
    icon_kicad_nightly
    icon_libedit
    icon_modedit
    icon_pcbnew
    icon_bitmap2component
    icon_pagelayout_editor
    icon_pcbcalculator
    icon_pcm
    reannotate_down_left
    reannotate_down_right
    reannotate_left_down
    reannotate_left_up
    reannotate_right_down
    reannotate_right_up
    reannotate_up_left
    reannotate_up_right
    icon_pagelayout_editor
    wizard_add_fplib_icon
    )

# 64 x 64 for icon sets
set( BMAPS_64
    icon_cvpcb
    icon_eeschema
    icon_gerbview
    icon_kicad
    icon_kicad_nightly
    icon_libedit
    icon_modedit
    icon_pcbnew
    icon_pagelayout_editor
    icon_bitmap2component
    icon_pcbcalculator
    icon_pcm
    )

# 128 x 128 for icon sets
set( BMAPS_128
    icon_cvpcb
    icon_eeschema
    icon_gerbview
    icon_kicad
    icon_kicad_nightly
    icon_libedit
    icon_modedit
    icon_pcbnew
    icon_pagelayout_editor
    icon_bitmap2component
    icon_pcbcalculator
    icon_pcm
    )

# 256 x 256 for icon sets (windows namely)
set( BMAPS_256
    icon_cvpcb
    icon_eeschema
    icon_gerbview
    icon_kicad
    icon_kicad_nightly
    icon_libedit
    icon_modedit
    icon_pcbnew
    icon_pagelayout_editor
    icon_bitmap2component
    icon_pcbcalculator
    icon_pcm
    )

# Teardrop images (275x130 native size) for teardrop dialogs
    set( BMAPS_TEARDROPS
    teardrop_sizes
    teardrop_rect_sizes
    teardrop_track_sizes
    )

# Tuning images (250x156 native size )
    set( BMAPS_TUNING
    tune_diff_pair_length_legend
    tune_single_track_length_legend
    )

# Tuning image (250x110 native size )
    set( BMAPS_TUNING_SKEW
    tune_diff_pair_skew_legend
    )

# Images in this section are generated at whatever resolution is set in the source SVG file
# Use Inkscape Document Properties > Custom Size to set (make sure units are pixels)
# These are used for images in the UI that are not-square or very large (infographics, etc)
set( BMAPS_OTHER
    att_bridge
    att_pi
    att_splitter
    att_tee
    c_microstrip
    coax
    color_code_multiplier
    color_code_tolerance
    color_code_value
    color_code_value_and_name
    coupled_stripline
    creepage_clearance
    cpw
    cpw_back
    light
    microstrip
    microstrip_zodd_zeven
    pads_npth
    pads_npth_bottom
    pads_npth_top
    pads_npth_top_bottom
    pads_remove_unused
    pads_remove_unused_keep_bottom
    pads_reset_unused
    rectwaveguide
    regul
    regul_3pins
    splash
    stripline
    stroke_dash
    stroke_dashdot
    stroke_dashdotdot
    stroke_dot
    stroke_solid
    stroke_none
    twistedpair
    viacalc
    constraint_absolute_lenght
    constraint_absolute_lenght_2
    constraint_allow_fillets_outside_zone_outline
    constraint_allowed_orientation
    constraint_basic_clearance
    constraint_board_outline_clearance
    constraint_copper_to_edge_clearance
    constraint_copper_to_hole_clearance
    constraint_corner_style
    constraint_creepage_distance
    constraint_daisy_chain_stub
    constraint_daisy_chain_stub_2
    constraint_hole_to_hole_clearance
    constraint_hole_size
    constraint_hole_to_hole_clearance
    constraint_hole_to_hole_distance
    constraint_matched_lenght_all_traces_in_group
    constraint_matched_lenght_diff_pair
    constraint_maximum_allowed_deviation
    constraint_maximum_via_count
    constraint_minimum_acute_angle
    constraint_minimum_angular_ring
    constraint_minimum_annular_width
    constraint_minimum_clearance
    constraint_minimum_connection_width
    constraint_minimum_item_clearance
    constraint_minimum_soldermask_silver
    constraint_minimum_text_height_and_thickness
    constraint_minimum_thermal_relief_spoke_count
    constraint_minimum_through_hole
    constraint_minimum_track_width
    constraint_minimum_uvia_diameter
    constraint_minimum_uvia_hole
    constraint_minimum_via_diameter
    constraint_net_antenna
    constraint_parallel_limit
    constraint_permitted_layers
    constraint_routing_diff_pair
    constraint_routing_width
    constraint_short_circuit
    constraint_silk_to_silk_clearance
    constraint_silk_to_soldermask_clearance
    constraint_SMD_corner
    constraint_SMD_entry
    constraint_SMD_to_plane_plus
    constraint_soldermask_expansion
    constraint_solderpaste_expansion
    constraint_unrouted
    constraint_via_style
    constraint_vias_under_SMD
    )


# @todo keep these in sync with .bzrignore
set( TMP_DIR   "${CMAKE_CURRENT_BINARY_DIR}/tmp" )

# true to generate log files, false to build icon files only
# generate logs is useful only if you have problems
set( CREATE_LOG_FILES false )

#set( BITMAP_INFO_FILE "${CMAKE_SOURCE_DIR}/include/bitmaps/bitmap_info.h" )
set( BITMAP_INFO_FILE "${CMAKE_SOURCE_DIR}/common/bitmap_info.cpp" )
set( BITMAP_INFO_TEMPLATE "${CMAKE_SOURCE_DIR}/include/bitmaps/bitmap_info.cpp.in" )
set( BITMAP_INFO_LIST "" )

set( ALL_PNGS "" )

function( svg2png inputFile outFile pngWidth pngHeight )
    #message( "svg2png( inputFile: ${inputFile} outFile: ${outFile} pngWidth: ${pngWidth} pngHeight: ${pngHeight})")

    if( CREATE_LOG_FILES )
        set( logfile "${TMP_DIR}/${bmn}.inkscape.log" )
    else()
        if( MSVC )
            set( logfile "nul" )
        else()
            set( logfile "null" )
        endif()
    endif()

    if( ${pngWidth} EQUAL -1 )
        set( sizeArgs --export-area-page )
    else()
        set( sizeArgs -w ${pngWidth} -h ${pngHeight} )
    endif()

    add_custom_command(
        OUTPUT ${outFile}
        # this Inkscape command line is for old Inkscape version < 1.0
        # COMMAND ${Inkscape_EXECUTABLE} --without-gui --export-area-snap -f ${inputFile} --export-png ${outFile} -w ${pngWidth} -h ${pngHeight} > ${logfile}

        # this Inkscape command line is for Inkscape >= 1.0
        COMMAND ${Inkscape_EXECUTABLE} --export-area-snap --export-type=png ${inputFile}
                --export-filename ${outFile} ${sizeArgs} > ${logfile}

        DEPENDS ${inputFile}
        COMMENT "Creating ${pngHeight} pixel tall ${outFile}"
        VERBATIM
        )
endfunction()


# Function png2png
# converts a basic PNG to one
function( png2png inputFile outFile )

    if( CREATE_LOG_FILES )
        set( logfile "${TMP_DIR}/${bmn}.pngcrush.log" )
    else()
        if( MSVC )
            set( logfile "nul" )
        else()
            set( logfile "null" )
        endif()
    endif()

    add_custom_command(
        OUTPUT  ${outFile}

        # pngcrush all icons without background to remove any extraneous text records.
        COMMAND ${pngcrush_EXECUTABLE} -rem alla ${inputFile} ${outFile} > ${logfile}

        DEPENDS ${inputFile}

        COMMENT "Creating cleaned file ${outFile}"
        )
endfunction()


# Function bitmap_dir
# Creates build targets for all the source SVGs in the given list, output at the given resolution
# Will create targets for each theme in the THEMES list.
# A height of -1 means to export with the page area rather than scaling to a given resolution.
# This is used for the BMAPS_OTHER list which have varying output sizes.
function( bitmap_dir pngWidth pngHeight bmapList )

    set( pngDir "${CMAKE_CURRENT_SOURCE_DIR}/png" )

    file( MAKE_DIRECTORY ${pngDir} )

    #file( REMOVE_RECURSE ${TMP_DIR} )
    file( MAKE_DIRECTORY ${TMP_DIR} )

    foreach( theme ${THEMES} )
        if( NOT ${theme} STREQUAL "light" )
            set( themeTag "_${theme}" )
        else()
            set( themeTag "" )
        endif()

        if( ${pngHeight} EQUAL -1 )
            set( heightTag "" )
        else()
            set( heightTag "_${pngHeight}" )
        endif()

        foreach( bmn ${bmapList} )

            set( svgFile "${CMAKE_CURRENT_SOURCE_DIR}/sources/${theme}/${bmn}.svg" )
            set( pngFile "${bmn}${themeTag}${heightTag}.png" )
            set( pngPath "${pngDir}/${pngFile}" )
            set( tmpFile "${TMP_DIR}/${pngFile}" )

            if( NOT EXISTS ${svgFile} )
                message( "Warning: Source SVG ${svgFile} does not exist!" )
                continue()
            endif()

            #svg2png( inputFile outFile pngWidth pngHeight )
            svg2png( ${svgFile} ${tmpFile} ${pngWidth} ${pngHeight} )

            #png2png( inputFile outFile )
            png2png( ${tmpFile} ${pngPath} )

            set( bitmapInfo
            "    aBitmapInfoCache[BITMAPS::${bmn}].emplace_back( BITMAPS::${bmn}, wxT( \"${pngFile}\" ), ${pngHeight}, wxT( \"${theme}\" ) );\n"
            )

            set( BITMAP_INFO_LIST "${BITMAP_INFO_LIST}${bitmapInfo}" )
            list( APPEND ALL_PNGS ${pngPath} )
        endforeach()
    endforeach()

    set( ALL_PNGS ${ALL_PNGS} PARENT_SCOPE )
    set( BITMAP_INFO_LIST ${BITMAP_INFO_LIST} PARENT_SCOPE )

endfunction()


include( FindUnixCommands )

if( TAR STREQUAL "TAR-NOTFOUND" )
    # CMake on Windows CI seems to struggle to find tar even though it's in PATH
    # Let's help it out as modern windows (Server 2019 and 10 provide tar)
    if( MSVC )
        # yolo
        set( TAR "C:\\Windows\\System32\\tar.exe" )
    else()
        message( FATAL_ERROR "Could not find the tar program." )
    endif()
endif()

set( BITMAP_ARCHIVE_PATH ${CMAKE_BINARY_DIR}/resources/images.tar.gz )

if( NOT ALL_PNGS )
    file( GLOB_RECURSE ALL_PNGS ${CMAKE_CURRENT_SOURCE_DIR}/png/*.png )
endif()

if( MINGW )
    # This seems like a huge hack, but I can't find a better way.  If anyone else does, please
    # feel free to replace this with it...
    file( TO_CMAKE_PATH "${BITMAP_ARCHIVE_PATH}" TAR_OUTPUT_FILE )
    string( REGEX REPLACE "^([a-zA-Z]):/" "/\\1/" TAR_OUTPUT_FILE "${TAR_OUTPUT_FILE}" )
else()
    set( TAR_OUTPUT_FILE "${BITMAP_ARCHIVE_PATH}" )
endif()

set( TARFLAGS cfz )
if( DEFINED ENV{SOURCE_DATE_EPOCH} )
    set( TARFLAGS --sort=name --mtime=@$ENV{SOURCE_DATE_EPOCH} --clamp-mtime --owner=0 --group=0 --numeric-owner --pax-option=exthdr.name=%d/PaxHeaders/%f,delete=atime,delete=ctime -czf )
endif()

# This is a dummy target to bind the custom command to something we can reuse in other calls
# i.e. we want to dynamically make bitmap_archive depend on generate_pngs target after wards
add_custom_target( bitmap_archive_bundle ALL )

# We use a custom command here because it will avoid running the command
# on every cmake build if there are no depends changes
add_custom_command(
    OUTPUT ${BITMAP_ARCHIVE_PATH}
    COMMAND ${TAR} ${TARFLAGS} "${TAR_OUTPUT_FILE}" .
    DEPENDS bitmap_archive_bundle ${ALL_PNGS}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/png
    COMMENT "Creating image archive ${TAR_OUTPUT_FILE}"
    VERBATIM
    )

# This is the second target we need to require the custom_command to generate and potentially run
add_custom_target( bitmap_archive_build ALL
                DEPENDS ${BITMAP_ARCHIVE_PATH})

install( FILES ${BITMAP_ARCHIVE_PATH}
    DESTINATION ${KICAD_DATA}/resources
    )

if( MAINTAIN_PNGS )

    # Inkscape is required to convert SVG files to PNG files.
    set( Inkscape_FOUND FALSE )

    if( NOT Inkscape_FOUND )
        find_program( Inkscape_EXECUTABLE inkscape
                      PATHS "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Inkscape;InstallDir]" "$ENV{PROGRAMFILES}\\Inkscape\\bin"
                      DOC "Inkscape vector drawing program." )

        if( NOT Inkscape_EXECUTABLE )
            message( FATAL_ERROR "Could not find the Inkscape vector drawing program." )
        else( NOT Inkscape_EXECUTABLE )
            set( Inkscape_FOUND TRUE )
            set( Inkscape_EXECUTABLE ${Inkscape_EXECUTABLE}
                 CACHE FILEPATH "Path and file name of the Inkscape program." )
            message( STATUS "The Inkscape vector drawing program found." )
        endif( NOT Inkscape_EXECUTABLE )
    endif( NOT Inkscape_FOUND )

    # pngcrush is required to reduce the size of the converted PNG files.
    set( pngcrush_FOUND FALSE )

    if( NOT pngcrush_FOUND )
        find_program( pngcrush_EXECUTABLE pngcrush DOC "The PNG size reduction program." )

        if( NOT pngcrush_EXECUTABLE )
            message( FATAL_ERROR "Could not find the pngcrush PNG size reduction program." )
        else( NOT pngcrush_EXECUTABLE )
            set( pngcrush_FOUND TRUE )
            set( pngcrush_EXECUTABLE ${pngcrush_EXECUTABLE}
                 CACHE FILEPATH "Path and file name of the pngcrush program." )
            message( STATUS "The pngcrush PNG size reduction program found." )
        endif( NOT pngcrush_EXECUTABLE )
    endif( NOT pngcrush_FOUND )

    # these 3 cmake commands for each desired bitmap size set, repeat as needed:
    bitmap_dir( 16 16 "${BMAPS_SMALL}" )
    bitmap_dir( 32 32 "${BMAPS_SMALL}" )    # 2x HiDPI

    bitmap_dir( 24 24 "${BMAPS_MID}" )  # "primary" size first
    bitmap_dir( 16 16 "${BMAPS_MID}" )  # Small toolbar mode
    bitmap_dir( 32 32 "${BMAPS_MID}" )  # Large toolbar mode
    bitmap_dir( 48 48 "${BMAPS_MID}" )  # HiDPI normal size
    bitmap_dir( 64 64 "${BMAPS_MID}" )  # HiDPI large size

    bitmap_dir( 32 32 "${BMAPS_32}" )

    bitmap_dir( 24 24 "${BMAPS_24}" )

    bitmap_dir( 64 64 "${BMAPS_64}" )

    bitmap_dir( 128 128 "${BMAPS_128}" )

    bitmap_dir( 256 256 "${BMAPS_256}" )

    bitmap_dir( 48 48 "${BMAPS_BIG}" )

    bitmap_dir( 275 130 "${BMAPS_TEARDROPS}" )

    bitmap_dir( 550 260 "${BMAPS_TEARDROPS}" )

    bitmap_dir( 250 156 "${BMAPS_TUNING}" )
    bitmap_dir( 500 312 "${BMAPS_TUNING}" )

    bitmap_dir( 250 110 "${BMAPS_TUNING_SKEW}" )
    bitmap_dir( 500 220 "${BMAPS_TUNING_SKEW}" )

    bitmap_dir( -1 -1 "${BMAPS_OTHER}" )

    configure_file( ${BITMAP_INFO_TEMPLATE} ${BITMAP_INFO_FILE}  NEWLINE_STYLE UNIX )

    add_custom_target( generate_pngs ALL
        DEPENDS ${ALL_PNGS}
        COMMENT "Creating PNGs from SVGs"
        )

    add_dependencies( bitmap_archive_bundle  generate_pngs )

endif( MAINTAIN_PNGS )
