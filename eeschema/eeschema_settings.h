/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef _EESCHEMA_SETTINGS_H
#define _EESCHEMA_SETTINGS_H

#include <settings/app_settings.h>


using KIGFX::COLOR4D;


class EESCHEMA_SETTINGS : public APP_SETTINGS_BASE
{
public:

    struct APPEARANCE
    {
        wxString edit_component_visible_columns;
        wxString edit_sheet_visible_columns;
        int  erc_severities;
        bool footprint_preview;
        bool navigator_stays_open;
        bool print_sheet_reference;
        bool show_hidden_pins;
        bool show_hidden_fields;
        bool show_illegal_symbol_lib_dialog;
        bool show_page_limits;
        bool show_sexpr_file_convert_warning;
        bool show_sheet_filename_case_sensitivity_dialog;
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

    struct DRAWING
    {
        int      default_bus_thickness;
        int      default_junction_size;
        int      default_line_thickness;
        int      default_repeat_offset_x;
        int      default_repeat_offset_y;
        int      default_wire_thickness;
        int      default_text_size;
        int      pin_symbol_size;
        double   text_offset_ratio;
        COLOR4D  default_sheet_border_color;
        COLOR4D  default_sheet_background_color;
        wxString field_names;
        bool     hv_lines_only;
        int      repeat_label_increment;
        bool     intersheets_ref_show;
        bool     intersheets_ref_own_page;
        bool     intersheets_ref_short;
        wxString intersheets_ref_prefix;
        wxString intersheets_ref_suffix;
        bool     auto_start_wires;
        std::vector<double> junction_size_mult_list;
        // Pulldown index for user default junction dot size (e.g. smallest = 0, small = 1, etc)
        int      junction_size_choice;
        double   junction_size_mult; // User selected default multiplier for junction dot size
    };

    struct INPUT
    {
        bool drag_is_move;
    };

    struct SELECTION
    {
        int  thickness;
        bool draw_selected_children;
        bool fill_shapes;
        bool select_pin_selects_symbol;
        bool text_as_box;
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
        int method;
        int messages_filter;
        int sort_order;
    };

    struct PANEL_BOM
    {
        wxString selected_plugin;
        std::vector<BOM_PLUGIN_SETTINGS> plugins;
    };

    struct PANEL_FIELD_EDITOR
    {
        std::map<std::string, bool> fields_show;
        std::map<std::string, bool> fields_group_by;
        std::map<std::string, int> column_widths;
    };

    struct PANEL_LIB_VIEW
    {
        int lib_list_width;
        int cmp_list_width;
        bool show_pin_electrical_type;
        WINDOW_SETTINGS window;
    };

    struct PANEL_NETLIST
    {
        std::vector<wxString> custom_command_titles;
        std::vector<wxString> custom_command_paths;
    };

    struct PANEL_PLOT
    {
        bool     background_color;
        bool     color;
        wxString color_theme;
        int      format;
        bool     frame_reference;
        int      hpgl_paper_size;
        double   hpgl_pen_size;
        int      hpgl_origin;
    };

    struct PANEL_SYM_CHOOSER
    {
        int  sash_pos_h;
        int  sash_pos_v;
        int  width;
        int  height;
        bool keep_symbol;
        bool place_all_units;
    };

    struct SIMULATOR
    {
        int plot_panel_width;
        int plot_panel_height;
        int signal_panel_height;
        int cursors_panel_height;
        bool white_background;
        WINDOW_SETTINGS window;
    };

    EESCHEMA_SETTINGS();

    virtual ~EESCHEMA_SETTINGS() {}

    virtual bool MigrateFromLegacy( wxConfigBase* aLegacyConfig ) override;

    static std::vector<BOM_PLUGIN_SETTINGS> DefaultBomPlugins();

    APPEARANCE m_Appearance;

    AUTOPLACE_FIELDS m_AutoplaceFields;

    DRAWING m_Drawing;

    INPUT m_Input;

    PAGE_SETTINGS m_PageSettings;

    PANEL_ANNOTATE m_AnnotatePanel;

    PANEL_BOM m_BomPanel;

    PANEL_FIELD_EDITOR m_FieldEditorPanel;

    PANEL_LIB_VIEW m_LibViewPanel;

    PANEL_NETLIST m_NetlistPanel;

    PANEL_PLOT m_PlotPanel;

    PANEL_SYM_CHOOSER m_SymChooserPanel;

    SELECTION m_Selection;

    SIMULATOR m_Simulator;

    bool m_RescueNeverShow;

    wxString m_lastSymbolLibDir;

protected:

    virtual std::string getLegacyFrameName() const override { return "SchematicFrame"; }

private:

    bool migrateBomSettings();

    nlohmann::json bomSettingsToJson() const;

    static std::vector<BOM_PLUGIN_SETTINGS> bomSettingsFromJson( const nlohmann::json& aObj );
};


#endif
