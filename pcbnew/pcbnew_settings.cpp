/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright The KiCad Developers, see AUTHORS.txt for contributors.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, you may find one here:
* http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
* or you may search the http://www.gnu.org website for the version 2 license,
* or you may write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include <pybind11/pybind11.h>

#include <common.h>
#include <footprint_editor_settings.h>
#include <layer_ids.h>
#include <lset.h>
#include <pcbnew_settings.h>
#include <pgm_base.h>
#include <router/pns_routing_settings.h>
#include <settings/common_settings.h>
#include <settings/nested_settings.h>
#include <settings/parameters.h>
#include <settings/settings_manager.h>
#include <wx/config.h>
#include <wx/tokenzr.h>
#include <base_units.h>

#include "../3d-viewer/3d_viewer/eda_3d_viewer_settings.h"


///! Update the schema version whenever a migration is required
const int pcbnewSchemaVersion = 5;


PCBNEW_SETTINGS::PCBNEW_SETTINGS()
        : PCB_VIEWERS_SETTINGS_BASE( "pcbnew", pcbnewSchemaVersion ),
          m_AuiPanels(),
          m_FootprintChooser(),
          m_FootprintViewer(),
          m_FootprintWizard(),
          m_Display(),
          m_TrackDragAction( TRACK_DRAG_ACTION::DRAG ),
          m_ArcEditMode( ARC_EDIT_MODE::KEEP_CENTER_ADJUST_ANGLE_RADIUS ),
          m_CtrlClickHighlight( false ),
          m_AngleSnapMode( LEADER_MODE::DIRECT ),
          m_FlipDirection( FLIP_DIRECTION::TOP_BOTTOM ),
          m_ESCClearsNetHighlight( true ),
          m_PolarCoords( false ),
          m_RotationAngle( ANGLE_90 ),
          m_ShowPageLimits( true ),
          m_ShowCourtyardCollisions( true ),
          m_AutoRefillZones( false ),
          m_AllowFreePads( false ),
          m_PnsSettings( nullptr ),
          m_FootprintViewerLibListWidth( 200 ),
          m_FootprintViewerFPListWidth( 300 )
{
    m_MagneticItems.pads      = MAGNETIC_OPTIONS::CAPTURE_CURSOR_IN_TRACK_TOOL;
    m_MagneticItems.tracks    = MAGNETIC_OPTIONS::CAPTURE_CURSOR_IN_TRACK_TOOL;
    m_MagneticItems.graphics  = false;
    m_MagneticItems.allLayers = false;

    m_params.emplace_back( new PARAM<bool>( "aui.show_layer_manager",
            &m_AuiPanels.show_layer_manager, true ) );

    m_params.emplace_back( new PARAM<int>( "aui.right_panel_width",
            &m_AuiPanels.right_panel_width, -1 ) );

    m_params.emplace_back( new PARAM<int>( "aui.net_inspector_width",
            &m_AuiPanels.net_inspector_width, -1 ) );

    m_params.emplace_back( new PARAM<int>( "aui.properties_panel_width",
            &m_AuiPanels.properties_panel_width, -1 ) );

    m_params.emplace_back( new PARAM<float>( "aui.properties_splitter_proportion",
            &m_AuiPanels.properties_splitter, 0.5f ) );

    m_params.emplace_back( new PARAM<int>( "aui.search_panel_height",
            &m_AuiPanels.search_panel_height, -1 ) );

    m_params.emplace_back( new PARAM<int>( "aui.search_panel_width",
            &m_AuiPanels.search_panel_width, -1 ) );

    m_params.emplace_back( new PARAM<int>( "aui.search_panel_dock_direction",
            &m_AuiPanels.search_panel_dock_direction, 3 ) );

    m_params.emplace_back( new PARAM<int>( "aui.appearance_panel_tab",
            &m_AuiPanels.appearance_panel_tab, 0, 0, 2 ) );

    m_params.emplace_back( new PARAM<bool>( "aui.appearance_expand_layer_display",
            &m_AuiPanels.appearance_expand_layer_display, false ) );

    m_params.emplace_back( new PARAM<bool>( "aui.appearance_expand_net_display",
            &m_AuiPanels.appearance_expand_net_display, false ) );

    m_params.emplace_back( new PARAM<bool>( "aui.show_properties",
            &m_AuiPanels.show_properties, false ) );

    m_params.emplace_back( new PARAM<bool>( "aui.show_search",
            &m_AuiPanels.show_search, false ) );

    m_params.emplace_back( new PARAM<bool>( "aui.show_net_inspector",
            &m_AuiPanels.show_net_inspector, false ) );

    m_params.emplace_back( new PARAM<bool>( "aui.design_blocks_show", &m_AuiPanels.design_blocks_show, false ) );

    m_params.emplace_back( new PARAM<int>( "aui.design_blocks_panel_docked_width",
                                           &m_AuiPanels.design_blocks_panel_docked_width, -1 ) );

    m_params.emplace_back(
            new PARAM<int>( "aui.design_blocks_panel_float_width", &m_AuiPanels.design_blocks_panel_float_width, -1 ) );

    m_params.emplace_back( new PARAM<int>( "aui.design_blocks_panel_float_height",
                                           &m_AuiPanels.design_blocks_panel_float_height, -1 ) );

    m_params.emplace_back( new PARAM<int>( "footprint_chooser.width",
            &m_FootprintChooser.width, -1 ) );

    m_params.emplace_back( new PARAM<int>( "footprint_chooser.height",
            &m_FootprintChooser.height, -1 ) );

    m_params.emplace_back( new PARAM<int>( "footprint_chooser.sash_h",
            &m_FootprintChooser.sash_h, -1 ) );

    m_params.emplace_back( new PARAM<int>( "footprint_chooser.sash_v",
            &m_FootprintChooser.sash_v, -1 ) );

    m_params.emplace_back( new PARAM<int>( "footprint_chooser.sort_mode",
            &m_FootprintChooser.sort_mode, 0 ) );

    m_params.emplace_back( new PARAM<bool>( "footprint_chooser.filter_on_pin_count",
            &m_FootprintChooser.filter_on_pin_count, false ) );

    m_params.emplace_back( new PARAM<bool>( "footprint_chooser.use_fp_filters",
            &m_FootprintChooser.use_fp_filters, false ) );

    m_params.emplace_back( new PARAM<bool>( "editing.flip_left_right",
            reinterpret_cast<bool*>( &m_FlipDirection ),
            static_cast<bool>( FLIP_DIRECTION::LEFT_RIGHT ) ) );

    m_params.emplace_back( new PARAM<bool>( "editing.esc_clears_net_highlight",
            &m_ESCClearsNetHighlight, true ) );

    m_params.emplace_back( new PARAM<bool>( "editing.show_courtyard_collisions",
            &m_ShowCourtyardCollisions, true ) );

    m_params.emplace_back( new PARAM<bool>( "editing.magnetic_graphics",
            &m_MagneticItems.graphics, true ) );

    m_params.emplace_back( new PARAM<int>( "editing.magnetic_pads",
            reinterpret_cast<int*>( &m_MagneticItems.pads ),
            static_cast<int>( MAGNETIC_OPTIONS::CAPTURE_CURSOR_IN_TRACK_TOOL ) ) );

    m_params.emplace_back( new PARAM<int>( "editing.magnetic_tracks",
            reinterpret_cast<int*>( &m_MagneticItems.tracks ),
            static_cast<int>( MAGNETIC_OPTIONS::CAPTURE_CURSOR_IN_TRACK_TOOL ) ) );

    m_params.emplace_back( new PARAM<bool>( "editing.magnetic_all_layers",
            &m_MagneticItems.allLayers, false ) );

    m_params.emplace_back( new PARAM<bool>( "editing.polar_coords",
            &m_PolarCoords, false ) );

    m_params.emplace_back( new PARAM<int>( "editing.track_drag_action",
            reinterpret_cast<int*>( &m_TrackDragAction ),
            static_cast<int>( TRACK_DRAG_ACTION::DRAG ) ) );

    m_params.emplace_back( new PARAM<int>( "editing.arc_edit_mode",
            reinterpret_cast<int*>( &m_ArcEditMode ),
            static_cast<int>( ARC_EDIT_MODE::KEEP_CENTER_ADJUST_ANGLE_RADIUS ) ) );

    m_params.emplace_back( new PARAM<bool>( "editing.ctrl_click_highlight",
            &m_CtrlClickHighlight, false ) );

    m_params.emplace_back( new PARAM<int>( "editing.pcb_angle_snap_mode",
            reinterpret_cast<int*>( &m_AngleSnapMode ),
            static_cast<int>( LEADER_MODE::DIRECT ) ) );

    m_params.emplace_back( new PARAM<bool>( "editing.auto_fill_zones",
            &m_AutoRefillZones, false ) );

    m_params.emplace_back( new PARAM<bool>( "editing.allow_free_pads",
            &m_AllowFreePads, false ) );

    m_params.emplace_back( new PARAM_LAMBDA<int>( "editing.rotation_angle",
            [this] () -> int
            {
                int rot = m_RotationAngle.AsTenthsOfADegree();

                // Don't store values larger than 360 degrees
                return rot % 3600;
            },
            [this] ( int aVal )
            {
                if( aVal )
                    m_RotationAngle = EDA_ANGLE( aVal, TENTHS_OF_A_DEGREE_T );

                // A misconfiguration allowed some angles to be stored as tenth of a degree but read
                // as tens of degrees.  By disallowing storage of values larger than 360, we can weed out
                // those invalid values here.
                while( m_RotationAngle > ANGLE_360 )
                    m_RotationAngle = m_RotationAngle / 100;

            },
            900 ) );

    m_params.emplace_back( new PARAM<bool>( "pcb_display.graphic_items_fill",
            &m_ViewersDisplay.m_DisplayGraphicsFill, true ) );

    m_params.emplace_back( new PARAM<int>( "pcb_display.max_links_shown",
            &m_Display.m_MaxLinksShowed, 3, 0, 15 ) );

    m_params.emplace_back( new PARAM<bool>( "pcb_display.graphics_fill",
            &m_ViewersDisplay.m_DisplayGraphicsFill, true ) );

    m_params.emplace_back( new PARAM<bool>( "pcb_display.text_fill",
            &m_ViewersDisplay.m_DisplayTextFill, true ) );

    m_params.emplace_back( new PARAM<int>( "pcb_display.net_names_mode",
            &m_Display.m_NetNames, 3, 0, 3 ) );

    m_params.emplace_back( new PARAM<bool>( "pcb_display.pad_clearance",
            &m_Display.m_PadClearance, true ) );

    m_params.emplace_back( new PARAM<bool>( "pcb_display.pad_use_via_color_for_normal_th_padstacks",
            &m_Display.m_UseViaColorForNormalTHPadstacks, false ) );

    m_params.emplace_back( new PARAM<bool>( "pcb_display.pad_fill",
            &m_ViewersDisplay.m_DisplayPadFill, true ) );

    m_params.emplace_back( new PARAM<bool>( "pcb_display.pad_numbers",
            &m_ViewersDisplay.m_DisplayPadNumbers, true ) );

    m_params.emplace_back( new PARAM<bool>( "pcb_display.ratsnest_global",
            &m_Display.m_ShowGlobalRatsnest, true ) );

    m_params.emplace_back( new PARAM<bool>( "pcb_display.ratsnest_footprint",
            &m_Display.m_ShowModuleRatsnest, true ) );

    m_params.emplace_back( new PARAM<bool>( "pcb_display.ratsnest_curved",
            &m_Display.m_DisplayRatsnestLinesCurved, false ) );

    m_params.emplace_back( new PARAM<double>( "pcb_display.ratsnest_thickness",
            &m_Display.m_RatsnestThickness, 0.5 ) );

    m_params.emplace_back( new PARAM<int>( "pcb_display.track_clearance_mode",
            reinterpret_cast<int*>( &m_Display.m_TrackClearance ), SHOW_WITH_VIA_WHILE_ROUTING ) );

    m_params.emplace_back( new PARAM<bool>( "pcb_display.track_fill",
            &m_Display.m_DisplayPcbTrackFill, true ) );

    m_params.emplace_back( new PARAM<bool>( "pcb_display.via_fill",
            &m_Display.m_DisplayViaFill, true ) );

    m_params.emplace_back( new PARAM<int>( "pcb_display.origin_mode",
            reinterpret_cast<int*>( &m_Display.m_DisplayOrigin ),
            PCB_DISPLAY_ORIGIN::PCB_ORIGIN_PAGE ) );

    m_params.emplace_back( new PARAM<bool>( "pcb_display.origin_invert_x_axis",
            &m_Display.m_DisplayInvertXAxis, false ) );

    m_params.emplace_back( new PARAM<bool>( "pcb_display.origin_invert_y_axis",
            &m_Display.m_DisplayInvertYAxis, false ) );

    m_params.emplace_back( new PARAM<bool>( "pcb_display.force_show_fields_when_fp_selected",
            &m_Display.m_ForceShowFieldsWhenFPSelected, true ) );

    m_params.emplace_back( new PARAM<bool>( "pcb_display.live_3d_refresh",
            &m_Display.m_Live3DRefresh, false ) );

    m_params.emplace_back( new PARAM<bool>( "pcb_display.show_page_borders",
            &m_ShowPageLimits, true ) );

    m_params.emplace_back( new PARAM<bool>( "export_d356.doNotExportUnconnectedPads",
            &m_ExportD356.doNotExportUnconnectedPads, false ) );

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "action_plugins",
            [&]() -> nlohmann::json
            {
                nlohmann::json js = nlohmann::json::array();

                for( const auto& pair : m_VisibleActionPlugins )
                    js.push_back( nlohmann::json( { { pair.first.ToUTF8(), pair.second } } ) );

                return js;
            },
            [&]( const nlohmann::json& aObj )
            {
                m_VisibleActionPlugins.clear();

                if( !aObj.is_array() )
                {
                    return;
                }

                for( const auto& entry : aObj )
                {
                    if( entry.empty() || !entry.is_object() )
                        continue;

                    for( const auto& pair : entry.items() )
                    {
                        m_VisibleActionPlugins.emplace_back( std::make_pair(
                                wxString( pair.key().c_str(), wxConvUTF8 ), pair.value() ) );
                    }
                }
            },
            nlohmann::json::array() ) );

    addParamsForWindow( &m_FootprintViewer, "footprint_viewer" );

    m_params.emplace_back( new PARAM<double>( "footprint_viewer.zoom",
            &m_FootprintViewerZoom, 1.0 ) );

    m_params.emplace_back( new PARAM<bool>( "footprint_viewer.autozoom",
            &m_FootprintViewerAutoZoomOnSelect, true ) );

    m_params.emplace_back( new PARAM<int>( "footprint_viewer.lib_list_width",
            &m_FootprintViewerLibListWidth, 200 ) );

    m_params.emplace_back( new PARAM<int>( "footprint_viewer.fp_list_width",
            &m_FootprintViewerFPListWidth, 300 ) );

    addParamsForWindow( &m_FootprintWizard, "footprint_wizard" );

    m_params.emplace_back( new PARAM<wxString>( "system.last_footprint_lib_dir",
            &m_LastFootprintLibDir, "" ) );

    m_params.emplace_back( new PARAM<wxString>( "system.last_footprint3d_dir",
            &m_LastFootprint3dDir, "" ) );

    m_params.emplace_back( new PARAM<bool>( "DRC.report_all_track_errors",
            &m_DRCDialog.report_all_track_errors, false ) );

    m_params.emplace_back( new PARAM<bool>( "DRC.crossprobe",
            &m_DRCDialog.crossprobe, true ) );

    m_params.emplace_back( new PARAM<bool>( "DRC.scroll_on_crossprobe",
            &m_DRCDialog.scroll_on_crossprobe, true ) );

    registerMigration( 0, 1,
            [&]()
            {
                if( std::optional<int> optval = Get<int>( "pcb_display.rotation_angle" ) )
                    Set( "editing.rotation_angle", *optval );

                try
                {
                    At( "pcb_display" ).erase( "rotation_angle" );
                }
                catch( ... )
                {}

                return true;
            } );

    registerMigration( 1, 2,
            [&]()
            {
                // In version 1 this meant "after Zone Properties dialog", but it now means
                // "everywhere" so we knock it off on transition.
                Set( "editing.auto_fill_zones", false );

                return true;
            } );


    registerMigration( 2, 3,
            [&]() -> bool
            {
                // We used to have a bug on GTK which would set the lib tree column width way
                // too narrow.
                if( std::optional<int> optval = Get<int>( "lib_tree.column_width" ) )
                {
                    if( optval < 150 )
                        Set( "lib_tree.column_width",  300 );
                }

                return true;
            } );

    registerMigration( 3, 4,
                       [&]() -> bool
                       {
                           // This is actually a migration for APP_SETTINGS_BASE::m_LibTree
                           return migrateLibTreeWidth();
                       } );

    registerMigration( 4, 5,
                       [&]() -> bool
                       {
                           // This default proved to be unpopular; bump it off for everyone
                           Set( "editing.auto_fill_zones", false );
                           return true;
                       } );
}


PCBNEW_SETTINGS::~PCBNEW_SETTINGS() = default;


bool PCBNEW_SETTINGS::MigrateFromLegacy( wxConfigBase* aCfg )
{
    bool ret = APP_SETTINGS_BASE::MigrateFromLegacy( aCfg );

    const std::string f = getLegacyFrameName();

    //
    // NOTE: there's no value in line-wrapping these; it just makes the table unreadable.
    //
    ret &= fromLegacy<bool>( aCfg, "ShowLayerManagerTools",         "aui.show_layer_manager" );

    ret &= fromLegacy<int>(  aCfg, "FootprintChooserHSashPosition", "footprint_chooser.sash_h" );
    ret &= fromLegacy<int>(  aCfg, "FootprintChooserVSashPosition", "footprint_chooser.sash_v" );
    ret &= fromLegacy<int>(  aCfg, "FootprintChooserWidth",         "footprint_chooser.width" );
    ret &= fromLegacy<int>(  aCfg, "FootprintChooserHeight",        "footprint_chooser.height" );

    ret &= fromLegacy<bool>(  aCfg, "FlipLeftRight",                "editing.flip_left_right" );
    ret &= fromLegacy<bool>(  aCfg, "MagneticGraphics",             "editing.magnetic_graphics" );
    ret &= fromLegacy<int>(   aCfg, "MagneticPads",                 "editing.magnetic_pads" );
    ret &= fromLegacy<int>(   aCfg, "MagneticTracks",               "editing.magnetic_tracks" );
    ret &= fromLegacy<bool>(  aCfg, "DisplayPolarCoords",           "editing.polar_coords" );
    ret &= fromLegacy<bool>(  aCfg, "Use45DegreeGraphicSegments",   "editing.use_45_degree_graphic_segments" );

    ret &= fromLegacy<bool>(  aCfg, "PcbAffT",                      "pcb_display.graphic_items_fill" );
    ret &= fromLegacy<int>(   aCfg, "MaxLnkS",                      "pcb_display.max_links_shown" );
    ret &= fromLegacy<bool>(  aCfg, "ModAffC",                      "pcb_display.footprint_edge_fill" );
    ret &= fromLegacy<bool>(  aCfg, "ModAffT",                      "pcb_display.footprint_text_fill" );
    ret &= fromLegacy<int>(   aCfg, "ShowNetNamesMode",             "pcb_display.net_names_mode" );
    ret &= fromLegacy<int>(   aCfg, "PcbDisplayOrigin",             "pcb_display.origin_mode" );
    ret &= fromLegacy<bool>(  aCfg, "PcbInvertXAxis",               "pcb_display.origin_invert_x_axis" );
    ret &= fromLegacy<bool>(  aCfg, "PcbInvertYAxis",               "pcb_display.origin_invert_y_axis" );
    ret &= fromLegacy<bool>(  aCfg, "PadAffG",                      "pcb_display.pad_clearance" );
    ret &= fromLegacy<bool>(  aCfg, "PadFill",                      "pcb_display.pad_fill" );
    ret &= fromLegacy<bool>(  aCfg, "PadSNum",                      "pcb_display.pad_numbers" );
    ret &= fromLegacy<bool>(  aCfg, "ShowRatsnestLines",            "pcb_display.ratsnest_global" );
    ret &= fromLegacy<bool>(  aCfg, "ShowRatsnestModuleLines",      "pcb_display.ratsnest_footprint" );
    ret &= fromLegacy<bool>(  aCfg, "CurvedRatsnestLines",          "pcb_display.ratsnest_curved" );
    ret &= fromLegacy<int>(   aCfg, "RotationAngle",                "pcb_display.rotation_angle" );
    ret &= fromLegacy<int>(   aCfg, "TrackDisplayClearance",        "pcb_display.track_clearance_mode" );
    ret &= fromLegacy<bool>(  aCfg, "DisplayTrackFilled",           "pcb_display.track_fill" );
    ret &= fromLegacy<bool>(  aCfg, "ViaFill",                      "pcb_display.via_fill" );
    ret &= fromLegacy<int>(   aCfg, "PcbShowZonesMode",             "pcb_display.zone_mode" );

    ret &= fromLegacy<double>( aCfg, "PlotLineWidth_mm",            "plot.line_width" );

    {
        nlohmann::json js = nlohmann::json::array();
        wxString       key;
        bool           val = false;

        for( unsigned i = 0; i < PCB_LAYER_ID_COUNT; ++i  )
        {
            key.Printf( wxT( "PlotSVGLayer_%d" ), i );

            if( aCfg->Read( key, &val ) && val )
                js.push_back( i );
        }

        Set( "export_svg.layers", js );
    }

    {
        nlohmann::json js = nlohmann::json::array();

        wxString packed;

        if( aCfg->Read( "ActionPluginButtons", &packed ) )
        {
            wxStringTokenizer pluginSettingsTokenizer = wxStringTokenizer( packed, ";" );

            while( pluginSettingsTokenizer.HasMoreTokens() )
            {
                nlohmann::json row;
                wxString plugin = pluginSettingsTokenizer.GetNextToken();
                wxStringTokenizer pluginTokenizer = wxStringTokenizer( plugin, "=" );

                if( pluginTokenizer.CountTokens() != 2 )
                {
                    // Bad config
                    continue;
                }

                std::string key( pluginTokenizer.GetNextToken().ToUTF8() );
                bool        value( pluginTokenizer.GetNextToken().Cmp( wxT( "Visible" ) ) == 0 );

                js.push_back( nlohmann::json( { { key, value } } ) );
            }
        }

        Set( "action_plugins", js );
    }

    //
    // NOTE: there's no value in line-wrapping these; it just makes the table unreadable.
    //
    ret &= fromLegacy<int>(    aCfg, "Zone_Ouline_Hatch_Opt", "zones.hatching_style" );
    ret &= fromLegacy<int>(    aCfg, "Zone_NetSort_Opt",      "zones.net_sort_mode" );
    ret &= fromLegacy<double>( aCfg, "Zone_Clearance",        "zones.clearance" );
    ret &= fromLegacy<double>( aCfg, "Zone_Thickness",        "zones.min_thickness" );
    ret &= fromLegacy<double>( aCfg, "Zone_TH_Gap",           "zones.thermal_relief_gap" );
    ret &= fromLegacy<double>( aCfg, "Zone_TH_Copper_Width",  "zones.thermal_relief_copper_width" );

    ret &= fromLegacy<int>(    aCfg, "PrintSinglePage",          "plot.all_layers_on_one_page" );
    ret &= fromLegacy<int>(    aCfg, "PrintPadsDrillOpt",        "plot.pads_drill_mode" );
    ret &= fromLegacy<double>( aCfg, "PlotXFineScaleAdj",        "plot.fine_scale_x" );
    ret &= fromLegacy<double>( aCfg, "PlotYFineScaleAdj",        "plot.fine_scale_y" );
    ret &= fromLegacy<double>( aCfg, "PSPlotFineWidthAdj",       "plot.ps_fine_width_adjust" );
    ret &= fromLegacy<bool>(   aCfg, "CheckZonesBeforePlotting", "plot.check_zones_before_plotting" );

    ret &= fromLegacy<int>( aCfg, "FpWizardListWidth",        "footprint_wizard_list.width" );
    ret &= fromLegacy<int>( aCfg, "FpWizardListHeight",       "footprint_wizard_list.height" );

    migrateWindowConfig( aCfg, "ModViewFrame", "footprint_viewer" );

    ret &= fromLegacy<bool>( aCfg, "ModViewFrameAutoZoom",   "footprint_viewer.auto_zoom" );
    ret &= fromLegacy<double>( aCfg, "ModViewFrameZoom",     "footprint_viewer.zoom" );

    migrateWindowConfig( aCfg, "FootprintWizard", "footprint_wizard" );
    ret &= fromLegacyString( aCfg, "Fpwizard_auiPerspective", "footprint_wizard.perspective" );


    const std::string p = "pcbnew.InteractiveRouter.";

    Set( "tools.pns.meta", nlohmann::json( {
                                               { "filename", "pns" },
                                               { "version", 0 }
                                           } ) );

    ret &= fromLegacy<int>(  aCfg, p + "Mode",                  "tools.pns.mode" );
    ret &= fromLegacy<int>(  aCfg, p + "OptimizerEffort",       "tools.pns.effort" );
    ret &= fromLegacy<bool>( aCfg, p + "RemoveLoops",           "tools.pns.remove_loops" );
    ret &= fromLegacy<bool>( aCfg, p + "SmartPads",             "tools.pns.smart_pads" );
    ret &= fromLegacy<bool>( aCfg, p + "ShoveVias",             "tools.pns.shove_vias" );
    ret &= fromLegacy<bool>( aCfg, p + "StartDiagonal",         "tools.pns.start_diagonal" );
    ret &= fromLegacy<int>(  aCfg, p + "ShoveTimeLimit",        "tools.pns.shove_time_limit" );
    ret &= fromLegacy<int>(  aCfg, p + "ShoveIterationLimit",   "tools.pns.shove_iteration_limit" );
    ret &= fromLegacy<int>(  aCfg, p + "WalkaroundIterationLimit", "tools.pns.walkaround_iteration_limit" );
    ret &= fromLegacy<bool>( aCfg, p + "JumpOverObstacles",     "tools.pns.jump_over_obstacles" );
    ret &= fromLegacy<bool>( aCfg, p + "SmoothDraggedSegments", "tools.pns.smooth_dragged_segments" );
    ret &= fromLegacy<bool>( aCfg, p + "CanViolateDRC",         "tools.pns.can_violate_drc" );
    ret &= fromLegacy<bool>( aCfg, p + "SuggestFinish",         "tools.pns.suggest_finish" );
    ret &= fromLegacy<bool>( aCfg, p + "FreeAngleMode",         "tools.pns.free_angle_mode" );
    ret &= fromLegacy<bool>( aCfg, p + "InlineDragEnabled",     "editing.track_drag_action" );

    // Initialize some new PNS settings to legacy behaviors if coming from legacy
    Set( "tools.pns.fix_all_segments", false );

    // Migrate color settings that were stored in the pcbnew config file

    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();
    COLOR_SETTINGS*   cs = mgr.GetMigratedColorSettings();

    auto migrateLegacyColor =
            [&] ( const std::string& aKey, int aLayerId )
            {
                wxString str;

                if( aCfg->Read( aKey, &str ) )
                    cs->SetColor( aLayerId, COLOR4D( str ) );
            };

    for( int i = 0; i < PCB_LAYER_ID_COUNT; ++i )
    {
        wxString layer = LSET::Name( PCB_LAYER_ID( i ) );
        migrateLegacyColor( "Color4DPCBLayer_" + layer.ToStdString(), PCB_LAYER_ID( i ) );
    }

    migrateLegacyColor( "Color4DAnchorEx",           LAYER_ANCHOR );
    migrateLegacyColor( "Color4DAuxItems",           LAYER_AUX_ITEMS );
    migrateLegacyColor( "Color4DGrid",               LAYER_GRID );
    migrateLegacyColor( "Color4DNonPlatedEx",        LAYER_NON_PLATEDHOLES );
    migrateLegacyColor( "Color4DPCBBackground",      LAYER_PCB_BACKGROUND );
    migrateLegacyColor( "Color4DPCBCursor",          LAYER_CURSOR );
    migrateLegacyColor( "Color4DRatsEx",             LAYER_RATSNEST );
    migrateLegacyColor( "Color4DViaBBlindEx",        LAYER_VIA_BLIND );
    migrateLegacyColor( "Color4DViaBBlindEx",        LAYER_VIA_BURIED );
    migrateLegacyColor( "Color4DViaMicroEx",         LAYER_VIA_MICROVIA );
    migrateLegacyColor( "Color4DViaThruEx",          LAYER_VIA_THROUGH );
    migrateLegacyColor( "Color4DWorksheet",          LAYER_DRAWINGSHEET );
    migrateLegacyColor( "Color4DGrid",               LAYER_PAGE_LIMITS );

    mgr.SaveColorSettings( cs, "board" );

    Set( "appearance.color_theme", cs->GetFilename() );

    double x = 0, y = 0;

    if( aCfg->Read( f + "PcbUserGrid_X", &x ) && aCfg->Read( f + "PcbUserGrid_Y", &y ) )
    {
        EDA_UNITS u = static_cast<EDA_UNITS>( aCfg->ReadLong( f + "PcbUserGrid_Unit",
                                                              static_cast<long>( EDA_UNITS::INCH ) ) );

        // Convert to internal units
        x = EDA_UNIT_UTILS::UI::FromUserUnit( pcbIUScale, u, x );
        y = EDA_UNIT_UTILS::UI::FromUserUnit( pcbIUScale, u, y );

        Set( "window.grid.user_grid_x", EDA_UNIT_UTILS::UI::StringFromValue( pcbIUScale, u, x ) );
        Set( "window.grid.user_grid_y", EDA_UNIT_UTILS::UI::StringFromValue( pcbIUScale, u, y ) );
    }

    // Footprint editor settings were stored in pcbnew config file.  Migrate them here.
    FOOTPRINT_EDITOR_SETTINGS* fpedit = GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" );
    fpedit->MigrateFromLegacy( aCfg );
    fpedit->Load();

    // Same with 3D viewer
    EDA_3D_VIEWER_SETTINGS* viewer3d = GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" );
    viewer3d->MigrateFromLegacy( aCfg );
    viewer3d->Load();

    return ret;
}

//namespace py = pybind11;
//
//PYBIND11_MODULE( pcbnew, m )
//{
//    py::class_<PCBNEW_SETTINGS>( m, "settings" )
//            .def_readwrite( "Use45DegreeGraphicSegments", &PCBNEW_SETTINGS::m_Use45DegreeGraphicSegments )
//            .def_readwrite( "FlipLeftRight", &PCBNEW_SETTINGS::m_FlipDirection )
//            .def_readwrite( "AddUnlockedPads", &PCBNEW_SETTINGS::m_AddUnlockedPads)
//            .def_readwrite( "UsePolarCoords", &PCBNEW_SETTINGS::m_PolarCoords)
//            .def_readwrite( "RotationAngle", &PCBNEW_SETTINGS::m_RotationAngle)
//            .def_readwrite( "ShowPageLimits", &PCBNEW_SETTINGS::m_ShowPageLimits)
//            ;
//}
