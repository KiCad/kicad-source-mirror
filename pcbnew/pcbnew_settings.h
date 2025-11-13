/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#pragma once

#include <core/mirror.h> // for FLIP_DIRECTION
#include <geometry/eda_angle.h>
#include <geometry/geometry_utils.h>
#include <settings/app_settings.h>
#include <pcb_display_options.h>

namespace PNS
{
    class ROUTING_SETTINGS;
}


// Settings for the CONVERT_TOOL.
enum CONVERT_STRATEGY
{
    COPY_LINEWIDTH,
    CENTERLINE,
    BOUNDING_HULL
};


struct CONVERT_SETTINGS
{
    CONVERT_STRATEGY m_Strategy;
    int              m_Gap;
    int              m_LineWidth;
    bool             m_DeleteOriginals;
};


enum class MAGNETIC_OPTIONS
{
    NO_EFFECT = 0,
    CAPTURE_CURSOR_IN_TRACK_TOOL,
    CAPTURE_ALWAYS
};


struct MAGNETIC_SETTINGS
{
    MAGNETIC_OPTIONS pads;
    MAGNETIC_OPTIONS tracks;
    bool             graphics;
    bool             allLayers;

    MAGNETIC_SETTINGS() :
            pads( MAGNETIC_OPTIONS::CAPTURE_CURSOR_IN_TRACK_TOOL ),
            tracks( MAGNETIC_OPTIONS::CAPTURE_CURSOR_IN_TRACK_TOOL ),
            graphics( false ),
            allLayers( false )
    { }
};


enum class TRACK_DRAG_ACTION
{
    MOVE,
    DRAG,
    DRAG_FREE_ANGLE
};


enum TRACK_CLEARANCE_MODE
{
    DO_NOT_SHOW_CLEARANCE = 0,
    SHOW_WHILE_ROUTING,
    SHOW_WITH_VIA_WHILE_ROUTING,
    SHOW_WITH_VIA_WHILE_ROUTING_OR_DRAGGING,
    SHOW_WITH_VIA_ALWAYS
};


enum PCB_DISPLAY_ORIGIN
{
    PCB_ORIGIN_PAGE = 0,
    PCB_ORIGIN_AUX,
    PCB_ORIGIN_GRID,
};


typedef std::vector<std::pair<wxString, bool>> ACTION_PLUGIN_SETTINGS_LIST;


// base class to handle Pcbnew SETTINGS also used in Cvpcb
class PCB_VIEWERS_SETTINGS_BASE : public APP_SETTINGS_BASE
{
public:
    struct VIEWERS_DISPLAY_OPTIONS
    {
        LEADER_MODE m_AngleSnapMode;
        bool    m_DisplayGraphicsFill;
        bool    m_DisplayTextFill;
        bool    m_DisplayPadNumbers;
        bool    m_DisplayPadFill;
    };

    double m_FootprintViewerZoom;               ///< The last zoom level used (0 for auto)
    bool   m_FootprintViewerAutoZoomOnSelect;   ///< true to use automatic zoom on fp selection

    VIEWERS_DISPLAY_OPTIONS m_ViewersDisplay;

    PCB_VIEWERS_SETTINGS_BASE( const std::string& aFilename, int aSchemaVersion ):
        APP_SETTINGS_BASE( aFilename, aSchemaVersion ),
        m_FootprintViewerZoom( 1.0 ),
        m_FootprintViewerAutoZoomOnSelect( true )
    {
        m_ViewersDisplay.m_AngleSnapMode = LEADER_MODE::DIRECT;
        m_ViewersDisplay.m_DisplayGraphicsFill = true;
        m_ViewersDisplay.m_DisplayTextFill = true;
        m_ViewersDisplay.m_DisplayPadNumbers = true;
        m_ViewersDisplay.m_DisplayPadFill = true;
    }

    virtual ~PCB_VIEWERS_SETTINGS_BASE() {};
};


class PCBNEW_SETTINGS : public PCB_VIEWERS_SETTINGS_BASE
{
public:
    struct AUI_PANELS
    {
        int   appearance_panel_tab;
        bool  appearance_expand_layer_display;
        bool  appearance_expand_net_display;
        int   right_panel_width;
        int   properties_panel_width;
        int   net_inspector_width;
        float properties_splitter;
        int   search_panel_height;
        int   search_panel_width;
        int   search_panel_dock_direction;
        bool  show_layer_manager;
        bool  show_properties;
        bool  show_search;
        bool  show_net_inspector;
        bool  design_blocks_show;
        int   design_blocks_panel_docked_width;
        int   design_blocks_panel_float_width;
        int   design_blocks_panel_float_height;
    };

    struct DIALOG_EXPORT_D356
    {
        // Export D356 uses wxFileDialog, so there's no DIALOG_SHIM to save/restore control state
        bool doNotExportUnconnectedPads;
    };

    struct DIALOG_DRC
    {
        bool report_all_track_errors;
        bool crossprobe;
        bool scroll_on_crossprobe;
    };

    struct FOOTPRINT_CHOOSER
    {
        // Footprint chooser is a FRAME, so there's no DIALOG_SHIM to save/restore control state
        int  width;
        int  height;
        int  sash_h;
        int  sash_v;
        int  sort_mode;
        bool use_fp_filters;
        bool filter_on_pin_count;
    };

    struct DISPLAY_OPTIONS
    {
        // Note: Display options common to  Cvpcb and Pcbnew are stored in
        // VIEWERS_DISPLAY_OPTIONS m_ViewersDisplay, because the section DISPLAY_OPTIONS
        // exists only for Pcbnew
        bool                 m_DisplayViaFill;
        bool                 m_DisplayPcbTrackFill;

        TRACK_CLEARANCE_MODE m_TrackClearance;
        bool                 m_PadClearance;
        bool                 m_UseViaColorForNormalTHPadstacks;

        int                  m_NetNames;

        RATSNEST_MODE        m_RatsnestMode;

        int                  m_MaxLinksShowed;
        bool                 m_ShowModuleRatsnest;
        bool                 m_ShowGlobalRatsnest;
        bool                 m_DisplayRatsnestLinesCurved;
        double               m_RatsnestThickness;

        PCB_DISPLAY_ORIGIN   m_DisplayOrigin;
        bool                 m_DisplayInvertXAxis;
        bool                 m_DisplayInvertYAxis;

        bool                 m_ForceShowFieldsWhenFPSelected;
        bool                 m_Live3DRefresh;
    };

    PCBNEW_SETTINGS();
    virtual ~PCBNEW_SETTINGS();

    virtual bool MigrateFromLegacy( wxConfigBase* aLegacyConfig ) override;

protected:
    virtual std::string getLegacyFrameName() const override { return "PcbFrame"; }

public:
    AUI_PANELS         m_AuiPanels;

    DIALOG_EXPORT_D356 m_ExportD356;
    DIALOG_DRC         m_DRCDialog;
    FOOTPRINT_CHOOSER  m_FootprintChooser;

    WINDOW_SETTINGS    m_FootprintViewer;
    WINDOW_SETTINGS    m_FootprintWizard;

    DISPLAY_OPTIONS    m_Display;

    MAGNETIC_SETTINGS  m_MagneticItems;
    TRACK_DRAG_ACTION  m_TrackDragAction;
    ARC_EDIT_MODE      m_ArcEditMode;

    bool               m_CtrlClickHighlight;

    LEADER_MODE        m_AngleSnapMode;        // Constrain tool actions to horizontal/vertical or 45°/90°
    FLIP_DIRECTION     m_FlipDirection;

    bool      m_ESCClearsNetHighlight;

    bool      m_PolarCoords;

    EDA_ANGLE m_RotationAngle;

    bool      m_ShowPageLimits;
    bool      m_ShowCourtyardCollisions;

    ///<@todo Implement real auto zone filling (not just after zone properties are edited)
    bool      m_AutoRefillZones; // Fill zones after editing the zone using the Zone Properties dialog

    bool      m_AllowFreePads;  // True: unlocked pads can be moved freely with respect to the footprint.
                                // False (default): all pads are treated as locked for the purposes of
                                // movement and any attempt to move them will move the footprint instead.

    std::unique_ptr<PNS::ROUTING_SETTINGS> m_PnsSettings;

    int       m_FootprintViewerLibListWidth;
    int       m_FootprintViewerFPListWidth;

    wxString  m_LastFootprintLibDir;
    wxString  m_LastFootprint3dDir;

    ACTION_PLUGIN_SETTINGS_LIST m_VisibleActionPlugins;
};

