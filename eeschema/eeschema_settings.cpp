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

#include <functional>

#include <dialogs/dialog_bom_cfg_lexer.h>
#include <eda_draw_frame.h>
#include <eeschema_settings.h>
#include <layer_ids.h>
#include <symbol_editor_settings.h>
#include <string_utils.h>
#include <pgm_base.h>
#include <settings/common_settings.h>
#include <settings/json_settings_internals.h>
#include <settings/parameters.h>
#include <settings/settings_manager.h>
#include <settings/aui_settings.h>
#include <wx/config.h>
#include <widgets/ui_common.h>
#include <default_values.h>    // For some default values

using namespace T_BOMCFG_T;     // for the BOM_CFG_PARSER parser and its keywords

/// Update the schema version whenever a migration is required.
const int eeschemaSchemaVersion = 3;


/// Default value for bom.plugins
const nlohmann::json defaultBomPlugins =
        {
            {
                { "name", "bom_csv_grouped_extra" },
                { "path", "bom_csv_grouped_extra.py" }
            },
            {
                { "name", "bom_csv_grouped_by_value" },
                { "path", "bom_csv_grouped_by_value.py" }
            },
            {
                { "name", "bom_csv_grouped_by_value_with_fp" },
                { "path", "bom_csv_grouped_by_value_with_fp.py" }
            },
        };


const wxAuiPaneInfo& defaultNetNavigatorPaneInfo()
{
    static wxAuiPaneInfo paneInfo;

    paneInfo.Name( wxS( "NetNavigator" ) )
            .Caption( _( "Net Navigator" ) )
            .CaptionVisible( true )
            .PaneBorder( true )
            .Left().Layer( 3 ).Position( 0 )
            .TopDockable( false )
            .BottomDockable( false )
            .CloseButton( true )
            .MinSize( 120, 60 )
            .BestSize( 200, 200 )
            .FloatingSize( 200, 200 )
            .FloatingPosition( 50, 200 )
            .Show( false );

    return paneInfo;
}


const wxAuiPaneInfo& defaultPropertiesPaneInfo( wxWindow* aWindow )
{
    static wxAuiPaneInfo paneInfo;

    paneInfo.Name( EDA_DRAW_FRAME::PropertiesPaneName() )
            .Caption( _( "Properties" ) )
            .CaptionVisible( true )
            .PaneBorder( true )
            .Left().Layer( 3 ).Position( 2 )
            .TopDockable( false )
            .BottomDockable( false )
            .CloseButton( true )
            .MinSize( aWindow->FromDIP( wxSize( 240, 60 ) ) )
            .BestSize( aWindow->FromDIP( wxSize( 300, 200 ) ) )
            .FloatingSize( aWindow->FromDIP( wxSize( 300, 400 ) ) )
            .FloatingPosition( aWindow->FromDIP( wxPoint( 50, 200 ) ) )
            .Show( true );

    return paneInfo;
}


const wxAuiPaneInfo& defaultSchSelectionFilterPaneInfo( wxWindow* aWindow )
{
    static wxAuiPaneInfo paneInfo;

    paneInfo.Name( wxS( "SelectionFilter" ) )
            .Caption( _( "Selection Filter" ) )
            .CaptionVisible( true )
            .PaneBorder( false )
            .Left().Layer( 3 ).Position( 4 )
            .TopDockable( false )
            .BottomDockable( false )
            .CloseButton( true )
            .MinSize( aWindow->FromDIP( wxSize( 180, -1 ) ) )
            .BestSize( aWindow->FromDIP( wxSize( 180, -1 ) ) )
            .Show( true );

    return paneInfo;
}


const wxAuiPaneInfo& defaultDesignBlocksPaneInfo( wxWindow* aWindow )
{
    static wxAuiPaneInfo paneInfo;

    paneInfo.Name( EDA_DRAW_FRAME::DesignBlocksPaneName() )
            .Caption( _( "Design Blocks" ) )
            .CaptionVisible( true )
            .PaneBorder( true )
            .Right().Layer( 3 ).Position( 2 )
            .TopDockable( false )
            .BottomDockable( false )
            .CloseButton( true )
            .MinSize( aWindow->FromDIP( wxSize( 240, 60 ) ) )
            .BestSize( aWindow->FromDIP( wxSize( 300, 200 ) ) )
            .FloatingSize( aWindow->FromDIP( wxSize( 800, 600 ) ) )
            .FloatingPosition( aWindow->FromDIP( wxPoint( 50, 200 ) ) )
            .Show( true );

    return paneInfo;
}


EESCHEMA_SETTINGS::EESCHEMA_SETTINGS() :
        APP_SETTINGS_BASE( "eeschema", eeschemaSchemaVersion ),
        m_Appearance(),
        m_AutoplaceFields(),
        m_Drawing(),
        m_FindReplaceExtra(),
        m_Input(),
        m_PageSettings(),
        m_AnnotatePanel(),
        m_BomPanel(),
        m_FieldEditorPanel(),
        m_LibViewPanel(),
        m_NetlistPanel(),
        m_SymChooserPanel(),
        m_ImportGraphics(),
        m_Selection(),
        m_Simulator(),
        m_RescueNeverShow( false )
{
    m_params.emplace_back( new PARAM<wxString>( "appearance.edit_symbol_visible_columns",
            &m_Appearance.edit_symbol_visible_columns, "0 1 2 3 4 5 6 7" ) );

    m_params.emplace_back( new PARAM<int>( "appearance.edit_symbol_width",
            &m_Appearance.edit_symbol_width, -1 ) );

    m_params.emplace_back( new PARAM<int>( "appearance.edit_symbol_height",
            &m_Appearance.edit_symbol_height, -1 ) );

    m_params.emplace_back( new PARAM<wxString>( "appearance.edit_sheet_visible_columns",
            &m_Appearance.edit_sheet_visible_columns, "0 1 2 3 4 5 6 7" ) );

    m_params.emplace_back( new PARAM<wxString>( "appearance.edit_label_visible_columns",
            &m_Appearance.edit_label_visible_columns, "0 1 2 3 4 5 6 7" ) );

    m_params.emplace_back( new PARAM<int>( "appearance.edit_label_width",
            &m_Appearance.edit_label_width, -1 ) );

    m_params.emplace_back( new PARAM<int>( "appearance.edit_label_height",
            &m_Appearance.edit_label_height, -1 ) );

    m_params.emplace_back( new PARAM<int>( "appearance.erc_severities",
            &m_Appearance.erc_severities, RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING ) );

    m_params.emplace_back( new PARAM<bool>( "appearance.footprint_preview",
            &m_Appearance.footprint_preview, true ) );

    m_params.emplace_back( new PARAM<bool>( "appearance.print_sheet_reference",
            &m_Appearance.print_sheet_reference, true ) );

    m_params.emplace_back( new PARAM<wxString>( "appearance.default_font",
            &m_Appearance.default_font, "KiCad Font" ) );

    m_params.emplace_back( new PARAM<bool>( "appearance.show_hidden_pins",
            &m_Appearance.show_hidden_pins, false ) );

    m_params.emplace_back( new PARAM<bool>( "appearance.show_hidden_fields",
            &m_Appearance.show_hidden_fields, false ) );

    m_params.emplace_back( new PARAM<bool>( "appearance.show_directive_labels",
            &m_Appearance.show_directive_labels, true ) );

    m_params.emplace_back( new PARAM<bool>( "appearance.show_erc_warnings",
            &m_Appearance.show_erc_warnings, true ) );

    m_params.emplace_back( new PARAM<bool>( "appearance.show_erc_errors",
            &m_Appearance.show_erc_errors, true ) );

    m_params.emplace_back( new PARAM<bool>( "appearance.show_erc_exclusions",
            &m_Appearance.show_erc_exclusions, false ) );

    m_params.emplace_back( new PARAM<bool>( "appearance.mark_sim_exclusions",
            &m_Appearance.mark_sim_exclusions, true ) );

    m_params.emplace_back( new PARAM<bool>( "appearance.show_op_voltages",
            &m_Appearance.show_op_voltages, true ) );

    m_params.emplace_back( new PARAM<bool>( "appearance.show_op_currents",
            &m_Appearance.show_op_currents, true ) );

    m_params.emplace_back( new PARAM<bool>( "appearance.show_pin_alt_icons",
            &m_Appearance.show_pin_alt_icons, true ) );

    m_params.emplace_back( new PARAM<bool>( "appearance.show_illegal_symbol_lib_dialog",
            &m_Appearance.show_illegal_symbol_lib_dialog, true ) );

    m_params.emplace_back( new PARAM<bool>( "appearance.show_page_limits",
            &m_Appearance.show_page_limits, true ) );

    m_params.emplace_back( new PARAM<bool>( "appearance.show_sexpr_file_convert_warning",
            &m_Appearance.show_sexpr_file_convert_warning, true ) );

    m_params.emplace_back( new PARAM<bool>( "appearance.show_sheet_filename_case_sensitivity_dialog",
            &m_Appearance.show_sheet_filename_case_sensitivity_dialog, true ) );

    m_params.emplace_back( new PARAM<bool>( "aui.show_schematic_hierarchy",
            &m_AuiPanels.show_schematic_hierarchy, true ) );

    m_params.emplace_back( new PARAM<int>( "aui.hierarchy_panel_docked_width",
            &m_AuiPanels.hierarchy_panel_docked_width, -1 ) );

    m_params.emplace_back( new PARAM<int>( "aui.hierarchy_panel_docked_height",
            &m_AuiPanels.hierarchy_panel_docked_height, -1 ) );

    m_params.emplace_back( new PARAM<int>( "aui.hierarchy_panel_float_width",
            &m_AuiPanels.hierarchy_panel_float_width, -1 ) );

    m_params.emplace_back( new PARAM<int>( "aui.hierarchy_panel_float_height",
            &m_AuiPanels.hierarchy_panel_float_height, -1 ) );

    m_params.emplace_back( new PARAM<bool>( "aui.design_blocks_show",
            &m_AuiPanels.design_blocks_show, false ) );

    m_params.emplace_back( new PARAM<int>( "aui.design_blocks_panel_docked_width",
            &m_AuiPanels.design_blocks_panel_docked_width, -1 ) );

    m_params.emplace_back( new PARAM<int>( "aui.design_blocks_panel_float_width",
            &m_AuiPanels.design_blocks_panel_float_width, -1 ) );

    m_params.emplace_back( new PARAM<int>( "aui.design_blocks_panel_float_height",
            &m_AuiPanels.design_blocks_panel_float_height, -1 ) );

    m_params.emplace_back( new PARAM<bool>( "aui.schematic_hierarchy_float",
            &m_AuiPanels.schematic_hierarchy_float, false ) );

    m_params.emplace_back( new PARAM<int>( "aui.search_panel_height",
            &m_AuiPanels.search_panel_height, -1 ) );

    m_params.emplace_back( new PARAM<int>( "aui.search_panel_width",
            &m_AuiPanels.search_panel_width, -1 ) );

    m_params.emplace_back( new PARAM<int>( "aui.search_panel_dock_direction",
            &m_AuiPanels.search_panel_dock_direction, 3 ) );

    m_params.emplace_back( new PARAM<bool>( "aui.show_search",
            &m_AuiPanels.show_search, false ) );

    m_params.emplace_back( new PARAM<bool>( "aui.show_net_nav_panel",
            &m_AuiPanels.show_net_nav_panel, false ) );

    m_params.emplace_back( new PARAM<bool>( "aui.float_net_nav_panel",
            &m_AuiPanels.float_net_nav_panel, false ) );

    m_params.emplace_back( new PARAM<wxSize>( "aui.net_nav_panel_docked_size",
            &m_AuiPanels.net_nav_panel_docked_size, wxSize( 120, -1 ) ) );

    m_params.emplace_back( new PARAM<wxPoint>( "aui.net_nav_panel_float_pos",
            &m_AuiPanels.net_nav_panel_float_pos, wxPoint( 50, 200 ), false ) );

    m_params.emplace_back( new PARAM<wxSize>( "aui.net_nav_panel_float_size",
            &m_AuiPanels.net_nav_panel_float_size, wxSize( 200, 200 ) ) );

    m_params.emplace_back( new PARAM<bool>( "aui.show_properties",
            &m_AuiPanels.show_properties, true ) );

    m_params.emplace_back( new PARAM<int>( "aui.properties_panel_width",
            &m_AuiPanels.properties_panel_width, -1 ) );

    m_params.emplace_back( new PARAM<float>( "aui.properties_splitter_proportion",
            &m_AuiPanels.properties_splitter, 0.5f ) );

    m_params.emplace_back( new PARAM<bool>( "autoplace_fields.enable",
            &m_AutoplaceFields.enable, true ) );

    m_params.emplace_back( new PARAM<bool>( "autoplace_fields.allow_rejustify",
            &m_AutoplaceFields.allow_rejustify, true ) );

    m_params.emplace_back( new PARAM<bool>( "autoplace_fields.align_to_grid",
            &m_AutoplaceFields.align_to_grid, true ) );

    m_params.emplace_back( new PARAM<int>( "drawing.default_bus_thickness",
            &m_Drawing.default_bus_thickness, DEFAULT_BUS_WIDTH_MILS ) );

    m_params.emplace_back( new PARAM<int>( "drawing.default_junction_size",
            &m_Drawing.default_junction_size, DEFAULT_JUNCTION_DIAM ) );

    m_params.emplace_back( new PARAM<int>( "drawing.pin_symbol_size",
            &m_Drawing.pin_symbol_size, DEFAULT_TEXT_SIZE / 2 ) );

    m_params.emplace_back( new PARAM<double>( "drawing.text_offset_ratio",
            &m_Drawing.text_offset_ratio, 0.08 ) );

    m_params.emplace_back( new PARAM<int>( "drawing.default_line_thickness",
            &m_Drawing.default_line_thickness, DEFAULT_LINE_WIDTH_MILS ) );

    m_params.emplace_back( new PARAM<int>( "drawing.default_repeat_offset_x",
            &m_Drawing.default_repeat_offset_x, 0 ) );

    m_params.emplace_back( new PARAM<int>( "drawing.default_repeat_offset_y",
            &m_Drawing.default_repeat_offset_y, 100 ) );

    m_params.emplace_back( new PARAM<int>( "drawing.default_wire_thickness",
            &m_Drawing.default_wire_thickness, DEFAULT_WIRE_WIDTH_MILS ) );

    m_params.emplace_back( new PARAM<int>( "drawing.default_text_size",
            &m_Drawing.default_text_size, DEFAULT_TEXT_SIZE ) );

    m_params.emplace_back( new PARAM<wxString>( "drawing.field_names",
            &m_Drawing.field_names, "" ) );

    m_params.emplace_back( new PARAM<int>( "drawing.line_mode",
            &m_Drawing.line_mode, LINE_MODE::LINE_MODE_90 ) );

    m_params.emplace_back( new PARAM<int>( "editing.arc_edit_mode",
            reinterpret_cast<int*>( &m_Drawing.arc_edit_mode ),
            static_cast<int>( ARC_EDIT_MODE::KEEP_CENTER_ADJUST_ANGLE_RADIUS ) ) );

    m_params.emplace_back( new PARAM<bool>( "drawing.auto_start_wires",
            &m_Drawing.auto_start_wires, true ) );

    m_params.emplace_back( new PARAM<int>( "drawing.repeat_label_increment",
            &m_Drawing.repeat_label_increment, 1 ) );

    m_params.emplace_back( new PARAM<COLOR4D>( "drawing.default_sheet_border_color",
            &m_Drawing.default_sheet_border_color, COLOR4D::UNSPECIFIED ) );

    m_params.emplace_back( new PARAM<COLOR4D>( "drawing.default_sheet_background_color",
            &m_Drawing.default_sheet_background_color, COLOR4D::UNSPECIFIED ) );

    m_params.emplace_back( new PARAM_ENUM<POWER_SYMBOLS>( "drawing.new_power_symbols",
            &m_Drawing.new_power_symbols, POWER_SYMBOLS::DEFAULT, POWER_SYMBOLS::DEFAULT, POWER_SYMBOLS::LOCAL ) );

    m_params.emplace_back( new PARAM_LIST<double>( "drawing.junction_size_mult_list",
            &m_Drawing.junction_size_mult_list, { 0.0, 1.7, 4.0, 6.0, 9.0, 12.0 } ) );

    m_params.emplace_back( new PARAM<int>( "drawing.junction_size_choice",
            &m_Drawing.junction_size_choice, 3 ) );

    m_params.emplace_back( new PARAM<int>( "drawing.hop_over_size_choice",
            &m_Drawing.hop_over_size_choice, 0 ) );

    m_params.emplace_back( new PARAM<bool>( "find_replace.search_all_fields",
            &m_FindReplaceExtra.search_all_fields, false ) );

    m_params.emplace_back( new PARAM<bool>( "find_replace.search_all_pins",
            &m_FindReplaceExtra.search_all_pins, false ) );

    m_params.emplace_back( new PARAM<bool>( "find_replace.search_current_sheet_only",
            &m_FindReplaceExtra.search_current_sheet_only, false ) );

    m_params.emplace_back( new PARAM<bool>( "find_replace.replace_references",
            &m_FindReplaceExtra.replace_references, false ) );

    m_params.emplace_back( new PARAM<bool>( "input.drag_is_move",
            &m_Input.drag_is_move, false ) );

    m_params.emplace_back( new PARAM<bool>( "input.esc_clears_net_highlight",
            &m_Input.esc_clears_net_highlight, true ) );

    m_params.emplace_back( new PARAM<int>( "selection.thickness",
            &m_Selection.selection_thickness, 3 ) );

    m_params.emplace_back( new PARAM<int>( "selection.highlight_thickness",
            &m_Selection.highlight_thickness, 2 ) );

    m_params.emplace_back( new PARAM<bool>( "selection.draw_selected_children",
            &m_Selection.draw_selected_children, true ) );

    m_params.emplace_back( new PARAM<bool>( "selection.fill_shapes",
            &m_Selection.fill_shapes, false ) );

    m_params.emplace_back( new PARAM<bool>( "selection.highlight_netclass_colors",
            &m_Selection.highlight_netclass_colors, false ) );

    m_params.emplace_back( new PARAM<int>( "selection.highlight_netclass_colors_thickness",
            &m_Selection.highlight_netclass_colors_thickness, 15, 0, 50 ) );

    m_params.emplace_back( new PARAM<double>( "selection.highlight_netclass_colors_alpha",
            &m_Selection.highlight_netclass_colors_alpha, 0.6, 0, 1 ) );

    m_params.emplace_back( new PARAM<bool>( "annotation.automatic",
            &m_AnnotatePanel.automatic, true ) );

    m_params.emplace_back( new PARAM<bool>( "annotation.recursive",
            &m_AnnotatePanel.recursive, true ) );

    m_params.emplace_back( new PARAM<int>( "annotation.scope",
            &m_AnnotatePanel.scope, 0, 0, 2 ) );

    m_params.emplace_back( new PARAM<int>( "annotation.options",
            &m_AnnotatePanel.options, 0, 0, 2 ) );

    m_params.emplace_back( new PARAM<int>( "annotation.messages_filter",
            &m_AnnotatePanel.messages_filter, -1 ) );


    m_params.emplace_back( new PARAM<wxString>( "bom.selected_plugin",
            &m_BomPanel.selected_plugin, "" ) );

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "bom.plugins",
            std::bind( &EESCHEMA_SETTINGS::bomSettingsToJson, this ),
            [&]( const nlohmann::json& aObj )
            {
                if( !aObj.is_array() )
                    return;

                const nlohmann::json& list = aObj.empty() ? defaultBomPlugins : aObj;

                m_BomPanel.plugins = bomSettingsFromJson( list );
            },
            defaultBomPlugins ) );

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "netlist.plugins",
            std::bind( &EESCHEMA_SETTINGS::netlistSettingsToJson, this ),
            [&]( const nlohmann::json& aObj )
            {
                if( !aObj.is_array() )
                    return;

                if( aObj.empty() )
                    return;

                const nlohmann::json& list = aObj;

                m_NetlistPanel.plugins = netlistSettingsFromJson( list );
            },
            nullptr ) );

    m_params.emplace_back( new PARAM<bool>( "page_settings.export_paper",
            &m_PageSettings.export_paper, false ) );

    m_params.emplace_back( new PARAM<bool>( "page_settings.export_revision",
            &m_PageSettings.export_revision, false ) );

    m_params.emplace_back( new PARAM<bool>( "page_settings.export_date",
            &m_PageSettings.export_date, false ) );

    m_params.emplace_back( new PARAM<bool>( "page_settings.export_title",
            &m_PageSettings.export_title, false ) );

    m_params.emplace_back( new PARAM<bool>( "page_settings.export_company",
            &m_PageSettings.export_company, false ) );

    m_params.emplace_back( new PARAM<bool>( "page_settings.export_comment1",
            &m_PageSettings.export_comment1, false ) );

    m_params.emplace_back( new PARAM<bool>( "page_settings.export_comment2",
            &m_PageSettings.export_comment2, false ) );

    m_params.emplace_back( new PARAM<bool>( "page_settings.export_comment3",
            &m_PageSettings.export_comment3, false ) );

    m_params.emplace_back( new PARAM<bool>( "page_settings.export_comment4",
            &m_PageSettings.export_comment4, false ) );

    m_params.emplace_back( new PARAM<bool>( "page_settings.export_comment5",
            &m_PageSettings.export_comment5, false ) );

    m_params.emplace_back( new PARAM<bool>( "page_settings.export_comment6",
            &m_PageSettings.export_comment6, false ) );

    m_params.emplace_back( new PARAM<bool>( "page_settings.export_comment7",
            &m_PageSettings.export_comment7, false ) );

    m_params.emplace_back( new PARAM<bool>( "page_settings.export_comment8",
            &m_PageSettings.export_comment8, false ) );

    m_params.emplace_back( new PARAM<bool>( "page_settings.export_comment9",
            &m_PageSettings.export_comment9, false ) );

    m_params.emplace_back( new PARAM_MAP<int>( "field_editor.field_widths",
            &m_FieldEditorPanel.field_widths, {} ) );

    m_params.emplace_back( new PARAM<int>( "field_editor.width",
            &m_FieldEditorPanel.width, 0 ) );

    m_params.emplace_back( new PARAM<int>( "field_editor.height",
            &m_FieldEditorPanel.height, 0 ) );

    m_params.emplace_back( new PARAM<int>( "field_editor.page",
            &m_FieldEditorPanel.page, 0 ) );

    m_params.emplace_back( new PARAM<wxString>( "field_editor.export_filename",
            &m_FieldEditorPanel.export_filename, wxT( "" ) ) );

    m_params.emplace_back( new PARAM<int>( "field_editor.selection_mode",
            &m_FieldEditorPanel.selection_mode, 0 ) );

    m_params.emplace_back( new PARAM<int>( "field_editor.scope",
            &m_FieldEditorPanel.scope, 0 ) );

    addParamsForWindow( &m_Simulator.window, "simulator.window", 500, 400 );

    m_params.emplace_back( new PARAM<int>( "simulator.plot_panel_width",
            &m_Simulator.view.plot_panel_width, 0 ) );

    m_params.emplace_back( new PARAM<int>( "simulator.plot_panel_height",
            &m_Simulator.view.plot_panel_height, 0 ) );

    m_params.emplace_back( new PARAM<int>( "simulator.signal_panel_height",
            &m_Simulator.view.signal_panel_height, 0 ) );

    m_params.emplace_back( new PARAM<int>( "simulator.cursors_panel_height",
            &m_Simulator.view.cursors_panel_height, 0 ) );

    m_params.emplace_back( new PARAM<int>( "simulator.measurements_panel_height",
            &m_Simulator.view.measurements_panel_height, 0 ) );

    m_params.emplace_back( new PARAM<bool>( "simulator.white_background",
            &m_Simulator.view.white_background, false ) );

    m_params.emplace_back( new PARAM_ENUM<SIM_MOUSE_WHEEL_ACTION>( "simulator.mouse_wheel_actions.vertical_unmodified",
            &m_Simulator.preferences.mouse_wheel_actions.vertical_unmodified,
            SIM_MOUSE_WHEEL_ACTION::ZOOM, SIM_MOUSE_WHEEL_ACTION::NONE,
            SIM_MOUSE_WHEEL_ACTION::ZOOM_VERTICALLY ) );

    m_params.emplace_back( new PARAM_ENUM<SIM_MOUSE_WHEEL_ACTION>( "simulator.mouse_wheel_actions.vertical_with_ctrl",
            &m_Simulator.preferences.mouse_wheel_actions.vertical_with_ctrl,
            SIM_MOUSE_WHEEL_ACTION::PAN_LEFT_RIGHT, SIM_MOUSE_WHEEL_ACTION::NONE,
            SIM_MOUSE_WHEEL_ACTION::ZOOM_VERTICALLY ) );

    m_params.emplace_back( new PARAM_ENUM<SIM_MOUSE_WHEEL_ACTION>( "simulator.mouse_wheel_actions.vertical_with_shift",
            &m_Simulator.preferences.mouse_wheel_actions.vertical_with_shift,
            SIM_MOUSE_WHEEL_ACTION::PAN_UP_DOWN, SIM_MOUSE_WHEEL_ACTION::NONE,
            SIM_MOUSE_WHEEL_ACTION::ZOOM_VERTICALLY ) );

    m_params.emplace_back( new PARAM_ENUM<SIM_MOUSE_WHEEL_ACTION>( "simulator.mouse_wheel_actions.vertical_with_alt",
            &m_Simulator.preferences.mouse_wheel_actions.vertical_with_alt,
            SIM_MOUSE_WHEEL_ACTION::NONE, SIM_MOUSE_WHEEL_ACTION::NONE,
            SIM_MOUSE_WHEEL_ACTION::ZOOM_VERTICALLY ) );

    m_params.emplace_back( new PARAM_ENUM<SIM_MOUSE_WHEEL_ACTION>( "simulator.mouse_wheel_actions.horizontal",
            &m_Simulator.preferences.mouse_wheel_actions.horizontal, SIM_MOUSE_WHEEL_ACTION::NONE,
            SIM_MOUSE_WHEEL_ACTION::NONE, SIM_MOUSE_WHEEL_ACTION::ZOOM_VERTICALLY ) );

    m_params.emplace_back( new PARAM<int>( "symbol_chooser.sash_pos_h",
            &m_SymChooserPanel.sash_pos_h, -1 ) );

    m_params.emplace_back( new PARAM<int>( "symbol_chooser.sash_pos_v",
            &m_SymChooserPanel.sash_pos_v, -1 ) );

    m_params.emplace_back( new PARAM<int>( "symbol_chooser.width",
            &m_SymChooserPanel.width, -1 ) );

    m_params.emplace_back( new PARAM<int>( "symbol_chooser.height",
            &m_SymChooserPanel.height, -1 ) );

    m_params.emplace_back( new PARAM<int>( "symbol_chooser.sort_mode",
            &m_SymChooserPanel.sort_mode, 0 ) );

    m_params.emplace_back( new PARAM<bool>( "import_graphics.interactive_placement",
            &m_ImportGraphics.interactive_placement, true ) );

    m_params.emplace_back( new PARAM<int>( "import_graphics.line_width_units",
            &m_ImportGraphics.dxf_line_width_units, 0 ) );

    m_params.emplace_back( new PARAM<double>( "import_graphics.line_width",
            &m_ImportGraphics.dxf_line_width, 0.2 ) );

    m_params.emplace_back( new PARAM<int>( "import_graphics.origin_units",
            &m_ImportGraphics.origin_units, 0 ) );

    m_params.emplace_back( new PARAM<double>( "import_graphics.origin_x",
            &m_ImportGraphics.origin_x, 0 ) );

    m_params.emplace_back( new PARAM<double>( "import_graphics.origin_y",
            &m_ImportGraphics.origin_y, 0 ) );

    m_params.emplace_back( new PARAM<int>( "import_graphics.dxf_units",
            &m_ImportGraphics.dxf_units, 0 ) );

    m_params.emplace_back( new PARAM<bool>( "system.never_show_rescue_dialog",
            &m_RescueNeverShow, false ) );

    addParamsForWindow( &m_LibViewPanel.window, "lib_view.window", 500, 400 );

    m_params.emplace_back( new PARAM<int>( "lib_view.lib_list_width",
            &m_LibViewPanel.lib_list_width, 150 ) );

    m_params.emplace_back( new PARAM<int>( "lib_view.cmp_list_width",
            &m_LibViewPanel.cmp_list_width, 150 ) );

    m_params.emplace_back( new PARAM<bool>( "lib_view.show_pin_electrical_type",
            &m_LibViewPanel.show_pin_electrical_type, true ) );

    m_params.emplace_back( new PARAM<wxString>( "system.last_symbol_lib_dir",
            &m_lastSymbolLibDir, "" ) );

    // Migrations
    registerMigration( 0, 1,
            [&]() -> bool
            {
                // Version 0 to 1: BOM plugin settings moved from sexpr to JSON
                return migrateBomSettings();
            } );


    registerMigration( 1, 2,
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

    registerMigration( 2, 3,
           [&]() -> bool
           {
                // This is actually a migration for APP_SETTINGS_BASE::m_LibTree
                return migrateLibTreeWidth();
           } );
}


bool EESCHEMA_SETTINGS::MigrateFromLegacy( wxConfigBase* aCfg )
{
    bool ret = APP_SETTINGS_BASE::MigrateFromLegacy( aCfg );

    // Now modify the loaded grid selection, because in earlier versions the grids index was shared
    // between all applications and started at 1000 mils.  There is a 4-position offset between
    // this index and the possible eeschema grids list that we have to subtract.
    std::string gridSizePtr = "window.grid.last_size";

    if( std::optional<int> currentSize = Get<int>( gridSizePtr ) )
    {
        Set( gridSizePtr, *currentSize - 4 );
    }
    else
    {
        // Otherwise, default grid size should be 50 mils; index 1
        Set( gridSizePtr,  1 );
    }

    ret &= fromLegacy<bool>( aCfg, "FootprintPreview",   "appearance.footprint_preview" );
    ret &= fromLegacy<bool>( aCfg, "NavigatorStaysOpen", "appearance.navigator_stays_open" );
    ret &= fromLegacy<bool>( aCfg, "PrintSheetReferenceAndTitleBlock",
            "appearance.print_sheet_reference" );
    ret &= fromLegacy<bool>( aCfg, "ShowHiddenPins",     "appearance.show_hidden_pins" );
    ret &= fromLegacy<bool>( aCfg, "ShowIllegalSymbolLibDialog",
            "appearance.show_illegal_symbol_lib_dialog" );
    ret &= fromLegacy<bool>( aCfg, "ShowPageLimits",     "appearance.show_page_limits" );
    ret &= fromLegacy<bool>( aCfg, "ShowSheetFileNameCaseSensitivityDlg",
            "appearance.show_sheet_filename_case_sensitivity_dialog" );

    ret &= fromLegacy<bool>( aCfg, "AutoplaceFields",     "autoplace_fields.enable" );
    ret &= fromLegacy<bool>( aCfg, "AutoplaceJustify",    "autoplace_fields.allow_rejustify" );
    ret &= fromLegacy<bool>( aCfg, "AutoplaceAlign",      "autoplace_fields.align_to_grid" );

    ret &= fromLegacy<int>(  aCfg, "DefaultBusWidth",      "drawing.default_bus_thickness" );
    ret &= fromLegacy<int>(  aCfg, "DefaultJunctionSize",  "drawing.default_junction_size" );
    ret &= fromLegacy<int>(  aCfg, "DefaultDrawLineWidth", "drawing.default_line_thickness" );
    ret &= fromLegacy<int>(  aCfg, "RepeatStepX",          "drawing.default_repeat_offset_x" );
    ret &= fromLegacy<int>(  aCfg, "RepeatStepY",          "drawing.default_repeat_offset_y" );
    ret &= fromLegacy<int>(  aCfg, "DefaultWireWidth",     "drawing.default_wire_thickness" );
    ret &= fromLegacyString( aCfg, "FieldNames",           "drawing.field_names" );
    ret &= fromLegacy<bool>( aCfg, "HorizVertLinesOnly",   "drawing.line_mode" );
    ret &= fromLegacy<int>(  aCfg, "RepeatLabelIncrement", "drawing.repeat_label_increment" );

    ret &= fromLegacy<bool>( aCfg, "DragActionIsMove",     "input.drag_is_move" );

    ret &= fromLegacy<int>(  aCfg, "SelectionThickness",      "selection.thickness" );
    ret &= fromLegacy<bool>( aCfg, "SelectionDrawChildItems", "selection.draw_selected_children" );
    ret &= fromLegacy<bool>( aCfg, "SelectionFillShapes",     "selection.fill_shapes" );
    ret &= fromLegacy<bool>( aCfg, "SelectPinSelectSymbolOpt",
            "selection.select_pin_selects_symbol" );

    ret &= fromLegacy<int>(  aCfg, "AnnotateFilterMsg",       "annotation.messages_filter" );

    ret &= fromLegacyString( aCfg, "bom_plugin_selected",     "bom.selected_plugin" );
    ret &= fromLegacyString( aCfg, "bom_plugins",             "bom.plugins" );

    migrateBomSettings();

    ret &= fromLegacyString( aCfg, "SymbolFieldsShownColumns",
            "edit_sch_component.visible_columns" );

    ret &= fromLegacy<bool>( aCfg, "PageSettingsExportRevision", "page_settings.export_revision" );
    ret &= fromLegacy<bool>( aCfg, "PageSettingsExportDate",     "page_settings.export_date" );
    ret &= fromLegacy<bool>( aCfg, "PageSettingsExportTitle",    "page_settings.export_title" );
    ret &= fromLegacy<bool>( aCfg, "PageSettingsExportCompany",  "page_settings.export_company" );
    ret &= fromLegacy<bool>( aCfg, "PageSettingsExportComment1", "page_settings.export_comment1" );
    ret &= fromLegacy<bool>( aCfg, "PageSettingsExportComment2", "page_settings.export_comment2" );
    ret &= fromLegacy<bool>( aCfg, "PageSettingsExportComment3", "page_settings.export_comment3" );
    ret &= fromLegacy<bool>( aCfg, "PageSettingsExportComment4", "page_settings.export_comment4" );
    ret &= fromLegacy<bool>( aCfg, "PageSettingsExportComment5", "page_settings.export_comment5" );
    ret &= fromLegacy<bool>( aCfg, "PageSettingsExportComment6", "page_settings.export_comment6" );
    ret &= fromLegacy<bool>( aCfg, "PageSettingsExportComment7", "page_settings.export_comment7" );
    ret &= fromLegacy<bool>( aCfg, "PageSettingsExportComment8", "page_settings.export_comment8" );
    ret &= fromLegacy<bool>( aCfg, "PageSettingsExportComment9", "page_settings.export_comment9" );

    #if 0   // To do: move this code to the new netlist plugin management in settings
    {
        constexpr int max_custom_commands = 8;  // from DIALOG_NETLIST
        nlohmann::json js_cmd   = nlohmann::json::array();
        nlohmann::json js_title = nlohmann::json::array();
        wxString cmd_key, title_key, cmd, title;

        for( int i = 1; i <= max_custom_commands; ++i )
        {
            // Only migrate if both title and command are specified
            cmd_key.Printf( "CustomNetlistCommand%d", i );
            title_key.Printf( "CustomNetlistTitle%d", i );

            if( aCfg->Read( cmd_key, &cmd ) && aCfg->Read( title_key, &title )
                && !cmd.IsEmpty() && !title.IsEmpty() )
            {
                js_cmd.push_back( cmd.ToUTF8() );
                js_title.push_back( title.ToUTF8() );
            }
        }

        Set( "netlist.custom_command_titles", js_title );
        Set( "netlist.custom_command_paths", js_cmd );
    }
    #endif

    {
        // NOTE(JE) These parameters should move to project-local storage before V6, but we are
        // migrating them here in order to preserve data.  There is a bug here that is preserved:
        // keys are taken directly from the (translated) UI and stored in the config, so if the user
        // changes languages the keys will no longer work.
        aCfg->SetPath( "SymbolFieldEditor/Show/" );

        nlohmann::json js = nlohmann::json( {} );
        wxString key;
        bool     value = false;
        long     index = 0;

        while( aCfg->GetNextEntry( key, index ) )
        {
            if( aCfg->Read( key, &value ) )
            {
                std::string key_utf( key.ToUTF8() );

                try
                {
                    js[ std::move( key_utf ) ] = value;
                }
                catch(...)
                {
                    continue;
                }
            }
        }

        Set( "field_editor.fields_show", js );

        aCfg->SetPath( "../GroupBy" );

        while( aCfg->GetNextEntry( key, index ) )
        {
            if( aCfg->Read( key, &value ) )
            {
                std::string key_utf( key.ToUTF8() );

                try
                {
                    js[ std::move( key_utf ) ] = value;
                }
                catch(...)
                {
                    continue;
                }
            }
        }

        Set( "field_editor.fields_group_by", js );

        aCfg->SetPath( "../.." );
    }

    ret &= fromLegacy<bool>(   aCfg, "PlotModeColor",     "plot.color" );
    ret &= fromLegacy<int>(    aCfg, "PlotFormat",        "plot.format" );
    ret &= fromLegacy<bool>(   aCfg, "PlotFrameRef",      "plot.frame_reference" );

    ret &= fromLegacy<int>(    aCfg, "SIM_PLOT_FRAMEPos_x",       "simulator.window.pos_x" );
    ret &= fromLegacy<int>(    aCfg, "SIM_PLOT_FRAMEPos_y",       "simulator.window.pos_y" );
    ret &= fromLegacy<int>(    aCfg, "SIM_PLOT_FRAMESize_x",      "simulator.window.size_x" );
    ret &= fromLegacy<int>(    aCfg, "SIM_PLOT_FRAMESize_y",      "simulator.window.size_y" );
    ret &= fromLegacy<bool>(   aCfg, "SIM_PLOT_FRAMEMaximized",   "simulator.window.maximized" );
    ret &= fromLegacyString(   aCfg, "SIM_PLOT_FRAMEPerspective", "simulator.window.perspective" );
    ret &= fromLegacyString(   aCfg, "SIM_PLOT_FRAMEMostRecentlyUsedPath", "simulator.mru_path" );
    ret &= fromLegacy<int>(    aCfg, "SimPlotPanelWidth",        "simulator.plot_panel_width" );
    ret &= fromLegacy<int>(    aCfg, "SimPlotPanelHeight",       "simulator.plot_panel_height" );
    ret &= fromLegacy<int>(    aCfg, "SimSignalPanelHeight",     "simulator.signal_panel_height" );
    ret &= fromLegacy<int>(    aCfg, "SimCursorsPanelHeight",    "simulator.cursors_panel_height" );
    ret &= fromLegacy<bool>(   aCfg, "SimPlotWhiteBg",           "simulator.white_background" );

    ret &= fromLegacy<int>(  aCfg, "SymbolChooserHSashPosition", "symbol_chooser.sash_pos_h" );
    ret &= fromLegacy<int>(  aCfg, "SymbolChooserVSashPosition", "symbol_chooser.sash_pos_v" );
    ret &= fromLegacy<int>(  aCfg, "SymbolChooserWidth",         "symbol_chooser.width" );
    ret &= fromLegacy<int>(  aCfg, "SymbolChooserHeight",        "symbol_chooser.height" );

    const std::string vlf = "ViewlibFrame";
    ret &= fromLegacy<bool>( aCfg, vlf + "Maximized",            "lib_view.window.maximized" );
    ret &= fromLegacyString( aCfg, vlf + "MostRecentlyUsedPath", "lib_view.window.mru_path" );
    ret &= fromLegacy<int>(  aCfg, vlf + "Size_x",               "lib_view.window.size_x" );
    ret &= fromLegacy<int>(  aCfg, vlf + "Size_y",               "lib_view.window.size_y" );
    ret &= fromLegacyString( aCfg, vlf + "Perspective",          "lib_view.window.perspective" );
    ret &= fromLegacy<int>(  aCfg, vlf + "Pos_x",                "lib_view.window.pos_x" );
    ret &= fromLegacy<int>(  aCfg, vlf + "Pos_y",                "lib_view.window.pos_y" );
    ret &= fromLegacy<int>(  aCfg, "ViewLiblistWidth",           "lib_view.lib_list_width" );
    ret &= fromLegacy<int>(  aCfg, "ViewCmplistWidth",           "lib_view.cmp_list_width" );
    ret &= fromLegacy<bool>( aCfg,
            "ViewCmpShowPinElectricalType", "lib_view.show_pin_electrical_type" );
    ret &= fromLegacy<bool>( aCfg, vlf + "ShowGrid",             "lib_view.grid.show" );
    ret &= fromLegacy<int>(  aCfg, vlf + "_LastGridSize",        "lib_view.grid.last_size" );

    ret &= fromLegacy<bool>( aCfg, "RescueNeverShow",     "system.never_show_rescue_dialog" );

    // Legacy version stored this setting in eeschema, so move it to common if it exists
    bool tmp;

    if( aCfg->Read( "MoveWarpsCursor", &tmp ) )
        Pgm().GetCommonSettings()->m_Input.warp_mouse_on_move = tmp;

    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();
    COLOR_SETTINGS*   cs = mgr.GetMigratedColorSettings();

    auto migrateLegacyColor =
            [&] ( const std::string& aKey, int aLayerId )
            {
                wxString str;

                if( aCfg->Read( aKey, &str ) )
                    cs->SetColor( aLayerId, COLOR4D( str ) );
            };

    migrateLegacyColor( "Color4DBgCanvasEx",        LAYER_SCHEMATIC_BACKGROUND );
    migrateLegacyColor( "Color4DBodyBgEx",          LAYER_DEVICE_BACKGROUND );
    migrateLegacyColor( "Color4DBodyEx",            LAYER_DEVICE );
    migrateLegacyColor( "Color4DBrightenedEx",      LAYER_BRIGHTENED );
    migrateLegacyColor( "Color4DBusEx",             LAYER_BUS );
    migrateLegacyColor( "Color4DConnEx",            LAYER_JUNCTION );
    migrateLegacyColor( "Color4DCursorEx",          LAYER_SCHEMATIC_CURSOR );
    migrateLegacyColor( "Color4DErcEEx",            LAYER_ERC_ERR );
    migrateLegacyColor( "Color4DErcWEx",            LAYER_ERC_WARN );
    migrateLegacyColor( "Color4DFieldEx",           LAYER_FIELDS );
    migrateLegacyColor( "Color4DGLabelEx",          LAYER_GLOBLABEL );
    migrateLegacyColor( "Color4DGridEx",            LAYER_SCHEMATIC_GRID );
    migrateLegacyColor( "Color4DHLabelEx",          LAYER_HIERLABEL );
    migrateLegacyColor( "Color4DHiddenEx",          LAYER_HIDDEN );
    migrateLegacyColor( "Color4DLLabelEx",          LAYER_LOCLABEL );
    migrateLegacyColor( "Color4DNoConnectEx",       LAYER_NOCONNECT );
    migrateLegacyColor( "Color4DNoteEx",            LAYER_NOTES );
    migrateLegacyColor( "Color4DPinEx",             LAYER_PIN );
    migrateLegacyColor( "Color4DPinNameEx",         LAYER_PINNAM );
    migrateLegacyColor( "Color4DPinNumEx",          LAYER_PINNUM );
    migrateLegacyColor( "Color4DReferenceEx",       LAYER_REFERENCEPART );
    migrateLegacyColor( "Color4DShadowEx",          LAYER_SELECTION_SHADOWS );
    migrateLegacyColor( "Color4DSheetEx",           LAYER_SHEET );
    migrateLegacyColor( "Color4DSheetFileNameEx",   LAYER_SHEETFILENAME );
    migrateLegacyColor( "Color4DSheetLabelEx",      LAYER_SHEETLABEL );
    migrateLegacyColor( "Color4DSheetNameEx",       LAYER_SHEETNAME );
    migrateLegacyColor( "Color4DValueEx",           LAYER_VALUEPART );
    migrateLegacyColor( "Color4DWireEx",            LAYER_WIRE );
    migrateLegacyColor( "Color4DWorksheetEx",       LAYER_SCHEMATIC_DRAWINGSHEET );

    mgr.SaveColorSettings( cs, "schematic" );

    Set( "appearance.color_theme", cs->GetFilename() );

    // LibEdit settings were stored with eeschema.  If eeschema is the first app to run,
    // we need to migrate the LibEdit settings here

    if( SYMBOL_EDITOR_SETTINGS* sym_edit_cfg = GetAppSettings<SYMBOL_EDITOR_SETTINGS>( "symbol_editor" ) )
    {
        sym_edit_cfg->MigrateFromLegacy( aCfg );
        sym_edit_cfg->Load();
    }

    return ret;
}


/**
 * Used for parsing legacy-format bom plugin configurations.  Only used for migrating into
 * EESCHEMA_SETTINGS JSON format.
 */
class BOM_CFG_PARSER : public DIALOG_BOM_CFG_LEXER
{
    std::vector<EESCHEMA_SETTINGS::BOM_PLUGIN_SETTINGS>* m_pluginList;

public:
    BOM_CFG_PARSER( std::vector<EESCHEMA_SETTINGS::BOM_PLUGIN_SETTINGS>* aPluginList,
                    const char* aData, const wxString& aSource );

    void Parse();

private:
    void parseGenerator();
};


std::vector<EESCHEMA_SETTINGS::BOM_PLUGIN_SETTINGS> EESCHEMA_SETTINGS::DefaultBomPlugins()
{
    return bomSettingsFromJson( defaultBomPlugins );
}


bool EESCHEMA_SETTINGS::migrateBomSettings()
{
    if( !Contains( "bom.plugins" ) )
        return false;

    wxString list = *Get<wxString>( "bom.plugins" );

    BOM_CFG_PARSER cfg_parser( &m_BomPanel.plugins, TO_UTF8( list ), wxT( "plugins" ) );

    try
    {
        cfg_parser.Parse();
    }
    catch( const IO_ERROR& )
    {
        return false;
    }

    // Parser will have loaded up our array, let's dump it out to JSON
    At( "bom.plugins" ) = bomSettingsToJson();

    return true;
}


nlohmann::json EESCHEMA_SETTINGS::bomSettingsToJson() const
{
    nlohmann::json js = nlohmann::json::array();

    for( const BOM_PLUGIN_SETTINGS& plugin : m_BomPanel.plugins )
    {
        nlohmann::json pluginJson;

        pluginJson["name"]    = plugin.name.ToUTF8();
        pluginJson["path"]    = plugin.path.ToUTF8();
        pluginJson["command"] = plugin.command.ToUTF8();

        js.push_back( pluginJson );
    }

    return js;
}


std::vector<EESCHEMA_SETTINGS::BOM_PLUGIN_SETTINGS> EESCHEMA_SETTINGS::bomSettingsFromJson(
        const nlohmann::json& aObj )
{
    std::vector<EESCHEMA_SETTINGS::BOM_PLUGIN_SETTINGS> ret;

    wxASSERT( aObj.is_array() );

    for( const nlohmann::json& entry : aObj )
    {
        if( entry.empty() || !entry.is_object() )
            continue;

        if( !entry.contains( "name" ) || !entry.contains( "path" ) )
            continue;

        BOM_PLUGIN_SETTINGS plugin( entry.at( "name" ).get<wxString>(),
                                    entry.at( "path" ).get<wxString>() );

        if( entry.contains( "command" ) )
            plugin.command = entry.at( "command" ).get<wxString>();

        ret.emplace_back( plugin );
    }

    return ret;
}


nlohmann::json EESCHEMA_SETTINGS::netlistSettingsToJson() const
{
    nlohmann::json js = nlohmann::json::array();

    for( const NETLIST_PLUGIN_SETTINGS& plugin : m_NetlistPanel.plugins )
    {
        nlohmann::json pluginJson;

        pluginJson["name"]    = plugin.name.ToUTF8();
        pluginJson["path"]    = plugin.path.ToUTF8();
        pluginJson["command"] = plugin.command.ToUTF8();

        js.push_back( pluginJson );
    }

    return js;
}


std::vector<EESCHEMA_SETTINGS::NETLIST_PLUGIN_SETTINGS> EESCHEMA_SETTINGS::netlistSettingsFromJson(
        const nlohmann::json& aObj )
{
    std::vector<EESCHEMA_SETTINGS::NETLIST_PLUGIN_SETTINGS> ret;

    wxASSERT( aObj.is_array() );

    for( const nlohmann::json& entry : aObj )
    {
        if( entry.empty() || !entry.is_object() )
            continue;

        if( !entry.contains( "name" ) || !entry.contains( "path" ) )
            continue;

        NETLIST_PLUGIN_SETTINGS plugin( entry.at( "name" ).get<wxString>(),
                                    entry.at( "path" ).get<wxString>() );

        if( entry.contains( "command" ) )
            plugin.command = entry.at( "command" ).get<wxString>();

        ret.emplace_back( plugin );
    }

    return ret;
}


BOM_CFG_PARSER::BOM_CFG_PARSER( std::vector<EESCHEMA_SETTINGS::BOM_PLUGIN_SETTINGS>* aPluginList,
                                const char* aLine, const wxString& aSource ) :
        DIALOG_BOM_CFG_LEXER( aLine, aSource )
{
    wxASSERT( aPluginList );
    m_pluginList = aPluginList;
}


void BOM_CFG_PARSER::Parse()
{
    T token;

    while( ( token = NextTok() ) != T_RIGHT )
    {
        if( token == T_EOF)
            break;

        if( token == T_LEFT )
            token = NextTok();

        if( token == T_plugins )
            continue;

        switch( token )
        {
        case T_plugin:   // Defines a new plugin
            parseGenerator();
            break;

        default:
//            Unexpected( CurText() );
            break;
        }
    }
}


void BOM_CFG_PARSER::parseGenerator()
{
    wxString str;
    EESCHEMA_SETTINGS::BOM_PLUGIN_SETTINGS settings;

    NeedSYMBOLorNUMBER();
    settings.path = FromUTF8();

    T token;

    while( ( token = NextTok() ) != T_RIGHT )
    {
        if( token == T_EOF)
            break;

        switch( token )
        {
        case T_LEFT:
            break;

        case T_cmd:
            NeedSYMBOLorNUMBER();

            settings.command = FromUTF8();

            NeedRIGHT();
            break;

        case T_opts:
        {
            NeedSYMBOLorNUMBER();

            wxString option = FromUTF8();

            if( option.StartsWith( wxS( "nickname=" ), &str ) )
                settings.name = str;

            NeedRIGHT();
            break;
        }

        default:
            Unexpected( CurText() );
            break;
        }
    }

    m_pluginList->emplace_back( settings );
}
