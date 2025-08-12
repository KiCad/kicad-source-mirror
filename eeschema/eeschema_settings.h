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

#pragma once

#include <wx/aui/framemanager.h>

#include <settings/app_settings.h>
#include <sim/sim_preferences.h>

using KIGFX::COLOR4D;


extern const wxAuiPaneInfo& defaultNetNavigatorPaneInfo();
extern const wxAuiPaneInfo& defaultPropertiesPaneInfo( wxWindow* aWindow );
extern const wxAuiPaneInfo& defaultSchSelectionFilterPaneInfo( wxWindow* aWindow );
extern const wxAuiPaneInfo& defaultDesignBlocksPaneInfo( wxWindow* aWindow );



enum LINE_MODE
{
    LINE_MODE_FREE          = 0,
    LINE_MODE_90            = 1,
    LINE_MODE_45            = 2,

    LINE_MODE_COUNT,
};


enum class POWER_SYMBOLS
{
    DEFAULT = 0,
    GLOBAL,
    LOCAL
};

class EESCHEMA_SETTINGS : public APP_SETTINGS_BASE
{
public:
    struct APPEARANCE
    {
        wxString edit_symbol_visible_columns;
        int edit_symbol_width;
        int edit_symbol_height;
        wxString edit_sheet_visible_columns;
        wxString edit_label_visible_columns;
        int edit_label_width;
        int edit_label_height;
        bool edit_label_multiple;
        int  erc_severities;
        bool footprint_preview;
        bool print_sheet_reference;
        wxString default_font;
        bool show_hidden_pins;
        bool show_hidden_fields;
        bool show_directive_labels;
        bool mark_sim_exclusions;
        bool show_erc_warnings;
        bool show_erc_errors;
        bool show_erc_exclusions;
        bool show_op_voltages;
        bool show_op_currents;
        bool show_pin_alt_icons;
        bool show_illegal_symbol_lib_dialog;
        bool show_page_limits;
        bool show_sexpr_file_convert_warning;
        bool show_sheet_filename_case_sensitivity_dialog;
    };

    struct AUI_PANELS
    {
        int  hierarchy_panel_docked_width;  // width of hierarchy tree panel and pane when docked
        int  hierarchy_panel_docked_height; // height of hierarchy tree panel and pane when docked
        int  hierarchy_panel_float_width;   // width of hierarchy tree panel when floating
        int  hierarchy_panel_float_height;  // height of hierarchy tree panel when floating
        int  search_panel_height;           // height of the search panel
        int  search_panel_width;            // width of the search panel
        int  search_panel_dock_direction;   // docking direction of the search panel
        bool schematic_hierarchy_float;     // show hierarchy tree panel as floating
        bool show_schematic_hierarchy;      // show hierarchy tree pane
        bool show_search;                   // show the search panel
        wxSize net_nav_panel_docked_size;
        wxPoint net_nav_panel_float_pos;
        wxSize net_nav_panel_float_size;
        bool float_net_nav_panel;
        bool show_net_nav_panel;
        int  properties_panel_width;
        float properties_splitter;
        bool show_properties;
        bool design_blocks_show;
        int  design_blocks_panel_docked_width;
        int  design_blocks_panel_float_width;
        int  design_blocks_panel_float_height;
    };

    struct AUTOPLACE_FIELDS
    {
        bool enable;
        bool allow_rejustify;
        bool align_to_grid;
    };

    struct BOM_PLUGIN_SETTINGS
    {
        BOM_PLUGIN_SETTINGS() = default;

        BOM_PLUGIN_SETTINGS( const wxString& aName, const wxString& aPath ) :
                name( aName ),
                path( aPath )
        {}

        wxString name;
        wxString path;
        wxString command;
    };

    struct NETLIST_PLUGIN_SETTINGS
    {
        NETLIST_PLUGIN_SETTINGS() = default;

        NETLIST_PLUGIN_SETTINGS( const wxString& aName, const wxString& aPath ) :
                name( aName ),
                path( aPath )
        {}

        wxString name;
        wxString path;
        wxString command;
    };

    struct DRAWING
    {
        int                 default_bus_thickness;
        int                 default_junction_size;
        int                 default_line_thickness;
        int                 default_repeat_offset_x;
        int                 default_repeat_offset_y;
        int                 default_wire_thickness;
        int                 default_text_size;
        int                 pin_symbol_size;
        double              text_offset_ratio;
        COLOR4D             default_sheet_border_color;
        COLOR4D             default_sheet_background_color;
        POWER_SYMBOLS       new_power_symbols;
        wxString            field_names;
        int                 line_mode;
        ARC_EDIT_MODE       arc_edit_mode;
        int                 repeat_label_increment;
        bool                intersheets_ref_show;
        bool                intersheets_ref_own_page;
        bool                intersheets_ref_short;
        wxString            intersheets_ref_prefix;
        wxString            intersheets_ref_suffix;
        bool                auto_start_wires;
        std::vector<double> junction_size_mult_list;

        // Pulldown index for user default junction dot size (e.g. none = 0, smallest = 1, small = 2, etc)
        int                 junction_size_choice;
        int                 hop_over_size_choice;
    };

    struct INPUT
    {
        bool drag_is_move;
        bool esc_clears_net_highlight;
    };

    struct SELECTION
    {
        int  selection_thickness;
        int  highlight_thickness;
        bool draw_selected_children;
        bool fill_shapes;
        bool highlight_netclass_colors;
        int    highlight_netclass_colors_thickness;
        double highlight_netclass_colors_alpha;
    };

    struct PAGE_SETTINGS
    {
        bool export_paper;
        bool export_revision;
        bool export_date;
        bool export_title;
        bool export_company;
        bool export_comment1;
        bool export_comment2;
        bool export_comment3;
        bool export_comment4;
        bool export_comment5;
        bool export_comment6;
        bool export_comment7;
        bool export_comment8;
        bool export_comment9;
    };

    struct PANEL_ANNOTATE
    {
        bool automatic;
        bool recursive;
        int scope;
        int options;
        int messages_filter;
    };

    struct PANEL_BOM
    {
        wxString selected_plugin;
        std::vector<BOM_PLUGIN_SETTINGS> plugins;
    };

    struct PANEL_FIELD_EDITOR
    {
        std::map<std::string, int> field_widths;
        int                        width;
        int                        height;
        int                        page;
        wxString                   export_filename;
        int                        selection_mode;
        int                        scope;
    };

    struct PANEL_LIB_VIEW
    {
        int lib_list_width;
        int cmp_list_width;
        bool show_pin_electrical_type;
        bool show_pin_numbers;
        WINDOW_SETTINGS window;
    };

    struct PANEL_NETLIST
    {
        std::vector<NETLIST_PLUGIN_SETTINGS> plugins;
    };

    struct PANEL_SYM_CHOOSER
    {
        int  sash_pos_h;
        int  sash_pos_v;
        int  width;
        int  height;
        int  sort_mode;
    };

    struct DIALOG_IMPORT_GRAPHICS
    {
        bool     interactive_placement;
        wxString last_file;
        double   dxf_line_width;
        int      dxf_line_width_units;
        int      origin_units;
        double   origin_x;
        double   origin_y;
        int      dxf_units;
    };

    struct SIMULATOR
    {
        struct VIEW
        {
            int  plot_panel_width;
            int  plot_panel_height;
            int  signal_panel_height;
            int  cursors_panel_height;
            int  measurements_panel_height;
            bool white_background;
        };

        VIEW            view;
        WINDOW_SETTINGS window;
        SIM_PREFERENCES preferences;
    };

    struct FIND_REPLACE_EXTRA
    {
        bool search_all_fields;
        bool search_metadata;
        bool search_all_pins;
        bool search_current_sheet_only;
        bool search_selected_only;

        bool replace_references;
    };

    EESCHEMA_SETTINGS();

    virtual ~EESCHEMA_SETTINGS() {}

    virtual bool MigrateFromLegacy( wxConfigBase* aLegacyConfig ) override;

    static std::vector<BOM_PLUGIN_SETTINGS> DefaultBomPlugins();


protected:
    virtual std::string getLegacyFrameName() const override { return "SchematicFrame"; }

private:
    bool migrateBomSettings();

    nlohmann::json bomSettingsToJson() const;

    static std::vector<BOM_PLUGIN_SETTINGS> bomSettingsFromJson( const nlohmann::json& aObj );

    nlohmann::json netlistSettingsToJson() const;
    static std::vector<NETLIST_PLUGIN_SETTINGS> netlistSettingsFromJson( const nlohmann::json& aObj );

public:
    APPEARANCE m_Appearance;

    AUTOPLACE_FIELDS m_AutoplaceFields;

    AUI_PANELS m_AuiPanels;

    DRAWING m_Drawing;

    FIND_REPLACE_EXTRA m_FindReplaceExtra;

    INPUT m_Input;

    PAGE_SETTINGS m_PageSettings;

    PANEL_ANNOTATE m_AnnotatePanel;

    PANEL_BOM m_BomPanel;

    PANEL_FIELD_EDITOR m_FieldEditorPanel;

    PANEL_LIB_VIEW m_LibViewPanel;

    PANEL_NETLIST m_NetlistPanel;

    PANEL_SYM_CHOOSER m_SymChooserPanel;

    DIALOG_IMPORT_GRAPHICS m_ImportGraphics;

    SELECTION m_Selection;

    SIMULATOR m_Simulator;

    bool m_RescueNeverShow;

    wxString m_lastSymbolLibDir;
};

