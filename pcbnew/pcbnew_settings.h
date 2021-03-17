/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PCBNEW_SETTINGS_H_
#define PCBNEW_SETTINGS_H_

#include <settings/app_settings.h>
#include <pcb_display_options.h>

namespace PNS
{
    class ROUTING_SETTINGS;
}

enum class MAGNETIC_OPTIONS
{
    NO_EFFECT,
    CAPTURE_CURSOR_IN_TRACK_TOOL,
    CAPTURE_ALWAYS
};

struct MAGNETIC_SETTINGS
{
    MAGNETIC_OPTIONS pads;
    MAGNETIC_OPTIONS tracks;
    bool             graphics;

    MAGNETIC_SETTINGS()
            : pads( MAGNETIC_OPTIONS::CAPTURE_CURSOR_IN_TRACK_TOOL ),
              tracks( MAGNETIC_OPTIONS::CAPTURE_CURSOR_IN_TRACK_TOOL ),
              graphics( false )
    {
    }
};

enum class TRACK_DRAG_ACTION
{
    MOVE,
    DRAG,
    DRAG_FREE_ANGLE
};

#if defined(KICAD_SCRIPTING) && defined(KICAD_SCRIPTING_ACTION_MENU)
typedef std::vector<std::pair<wxString, bool>> ACTION_PLUGIN_SETTINGS_LIST;
#endif


class PCBNEW_SETTINGS : public APP_SETTINGS_BASE
{
public:
    struct AUI_PANELS
    {
        int  appearance_panel_tab;
        int  right_panel_width;
        bool show_layer_manager;
    };

    struct DIALOG_CLEANUP
    {
        bool cleanup_vias;
        bool delete_dangling_vias;
        bool cleanup_tracks_in_pad;
        bool cleanup_unconnected;
        bool cleanup_short_circuits;
        bool merge_segments;
    };

    struct DIALOG_DRC
    {
        bool refill_zones;
        bool test_all_track_errors;
        bool test_footprints;
        int  severities;
    };

    struct DIALOG_EXPORT_IDF
    {
        bool   auto_adjust;
        int    ref_units;
        double ref_x;
        double ref_y;
        bool   units_mils;
    };

    struct DIALOG_EXPORT_STEP
    {
        int    origin_mode;
        int    origin_units;
        double origin_x;
        double origin_y;
        bool   no_virtual;
    };

    struct DIALOG_EXPORT_SVG
    {
        bool             black_and_white;
        bool             mirror;
        bool             one_file;
        bool             plot_board_edges;
        int              page_size;
        wxString         output_dir;
        std::vector<int> layers;
    };

    struct DIALOG_EXPORT_VRML
    {
        int    units;
        bool   copy_3d_models;
        bool   use_relative_paths;
        bool   use_plain_pcb;
        int    ref_units;
        double ref_x;
        double ref_y;
        int    origin_mode;
    };

    struct DIALOG_FOOTPRINT_WIZARD_LIST
    {
        int width;
        int height;
    };

    struct DIALOG_GENERATE_DRILL
    {
        bool merge_pth_npth;
        bool minimal_header;
        bool mirror;
        bool unit_drill_is_inch;
        bool use_route_for_oval_holes;
        int  drill_file_type;
        int  map_file_type;
        int  zeros_format;
    };

    struct DIALOG_IMPORT_GRAPHICS
    {
        int         layer;
        bool        interactive_placement;
        wxString    last_file;
        double      line_width;
        int         line_width_units;
        int         origin_units;
        double      origin_x;
        double      origin_y;
        int         dxf_units;
    };

    struct DIALOG_NETLIST
    {
        int  report_filter;
        bool update_footprints;
        bool delete_shorting_tracks;
        bool delete_extra_footprints;
        bool delete_single_pad_nets;
        bool associate_by_ref_sch;
    };

    struct DIALOG_PLACE_FILE
    {
        int  units;
        int  file_options;
        int  file_format;
        bool include_board_edge;
    };

    struct DIALOG_PLOT
    {
        int    all_layers_on_one_page;
        int    pads_drill_mode;
        double fine_scale_x;
        double fine_scale_y;
        double ps_fine_width_adjust;
        bool   check_zones_before_plotting;
        bool   mirror;
    };

    struct DIALOG_REANNOTATE
    {
        bool     sort_on_fp_location;
        bool     remove_front_prefix;
        bool     remove_back_prefix;
        bool     update_schematic;
        bool     exclude_locked;
        int      grid_index;
        int      sort_code;
        int      annotation_choice;
        int      report_severity;
        wxString front_refdes_start;
        wxString back_refdes_start;
        wxString front_prefix;
        wxString back_prefix;
        wxString exclude_list;
        wxString report_file_name;
    };

    struct FOOTPRINT_CHOOSER
    {
        int width;
        int height;
        int sash_h;
        int sash_v;
    };

    struct ZONES
    {
        int         hatching_style;
        wxString    net_filter;
        int         net_sort_mode;
        double      clearance;
        double      min_thickness;
        double      thermal_relief_gap;
        double      thermal_relief_copper_width;
    };

    PCBNEW_SETTINGS();

    virtual ~PCBNEW_SETTINGS();

    virtual bool MigrateFromLegacy( wxConfigBase* aLegacyConfig ) override;

    AUI_PANELS m_AuiPanels;

    DIALOG_CLEANUP m_Cleanup;

    DIALOG_DRC m_DrcDialog;

    DIALOG_EXPORT_IDF m_ExportIdf;

    DIALOG_EXPORT_STEP m_ExportStep;

    DIALOG_EXPORT_SVG m_ExportSvg;

    DIALOG_EXPORT_VRML m_ExportVrml;

    DIALOG_FOOTPRINT_WIZARD_LIST m_FootprintWizardList;

    DIALOG_GENERATE_DRILL m_GenDrill;

    DIALOG_IMPORT_GRAPHICS m_ImportGraphics;

    DIALOG_NETLIST m_NetlistDialog;

    DIALOG_PLACE_FILE m_PlaceFile;

    DIALOG_PLOT m_Plot;

    DIALOG_REANNOTATE m_Reannotate;

    FOOTPRINT_CHOOSER m_FootprintChooser;

    ZONES m_Zones;

    WINDOW_SETTINGS m_FootprintViewer;

    WINDOW_SETTINGS m_FootprintWizard;

    PCB_DISPLAY_OPTIONS m_Display;

    MAGNETIC_SETTINGS m_MagneticItems;

    TRACK_DRAG_ACTION m_TrackDragAction;

    bool m_Use45DegreeGraphicSegments;   // True to constraint graphic lines to horizontal,
    // vertical and 45º
    bool m_FlipLeftRight;                // True: Flip footprints across Y axis
    // False: Flip footprints across X axis

    bool m_AddUnlockedPads;             // True: Pads are unlocked when new footprints are added to the board
                                        // False: Pads are locked in new footprints

    bool m_PolarCoords;

    int m_RotationAngle;

    bool m_ShowPageLimits;

    wxString m_FootprintTextShownColumns;

    std::unique_ptr<PNS::ROUTING_SETTINGS> m_PnsSettings;

    bool m_FootprintViewerAutoZoom;     ///< Should the footprint viewer auto zoom on select

    double m_FootprintViewerZoom;       ///< The last zoom level in the footprint viewer

    wxString m_lastFootprintLibDir;

    wxString m_lastFootprint3dDir;

#if defined(KICAD_SCRIPTING) && defined(KICAD_SCRIPTING_ACTION_MENU)
    ACTION_PLUGIN_SETTINGS_LIST m_VisibleActionPlugins;
#endif

protected:

    virtual std::string getLegacyFrameName() const override { return "PcbFrame"; }

};

#endif
