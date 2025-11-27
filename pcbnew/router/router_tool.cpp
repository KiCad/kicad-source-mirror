/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include <wx/filedlg.h>
#include <wx/hyperlink.h>
#include <advanced_config.h>

#include <functional>
#include <iomanip>
#include <utility>
#include <sstream>

using namespace std::placeholders;
#include <board.h>
#include <board_design_settings.h>
#include <board_item.h>
#include <footprint.h>
#include <geometry/geometry_utils.h>
#include <pad.h>
#include <zone.h>
#include <pcb_edit_frame.h>
#include <pcbnew_id.h>
#include <dialogs/dialog_pns_settings.h>
#include <dialogs/dialog_pns_diff_pair_dimensions.h>
#include <dialogs/dialog_track_via_size.h>
#include <math/vector2wx.h>
#include <paths.h>
#include <confirm.h>
#include <kidialog.h>
#include <widgets/wx_infobar.h>
#include <widgets/appearance_controls.h>
#include <connectivity/connectivity_data.h>
#include <connectivity/connectivity_algo.h>
#include <gal/graphics_abstraction_layer.h>
#include <view/view_controls.h>
#include <bitmaps.h>
#include <string_utils.h>
#include <gal/painter.h>
#include <tool/tool_action.h>
#include <tool/action_menu.h>
#include <tool/tool_manager.h>
#include <tool/tool_menu.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <tools/pcb_grid_helper.h>
#include <tools/drc_tool.h>
#include <tools/zone_filler_tool.h>
#include <drc/drc_interactive_courtyard_clearance.h>

#include <project.h>
#include <project/project_file.h>
#include <project/project_local_settings.h>

#include "router_tool.h"
#include "router_status_view_item.h"
#include "pns_router.h"
#include "pns_itemset.h"
#include "pns_logger.h"
#include "pns_placement_algo.h"
#include "pns_drag_algo.h"

#include "pns_kicad_iface.h"

#include <ratsnest/ratsnest_data.h>

#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>

using namespace KIGFX;

/**
 * Flags used by via tool actions
 */
enum VIA_ACTION_FLAGS
{
    // Via type
    VIA_MASK     = 0x07,
    VIA          = 0x00,     ///< Normal via
    BLIND_VIA    = 0x01,     ///< blind via
    BURIED_VIA   = 0x02,     ///< buried via
    MICROVIA     = 0x04,     ///< Microvia

    // Select layer
    SELECT_LAYER = VIA_MASK + 1,    ///< Ask user to select layer before adding via
};


// Actions, being statically-defined, require specialized I18N handling.  We continue to
// use the _() macro so that string harvesting by the I18N framework doesn't have to be
// specialized, but we don't translate on initialization and instead do it in the getters.

#undef _
#define _(s) s

// Pass all the parameters as int to allow combining flags
static const TOOL_ACTION ACT_PlaceThroughVia( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveRouter.PlaceVia" )
        .Scope( AS_CONTEXT )
        .DefaultHotkey( 'V' )
        .LegacyHotkeyName( "Add Through Via" )
        .FriendlyName( _( "Place Through Via" ) )
        .Tooltip( _( "Adds a through-hole via at the end of currently routed track." ) )
        .Icon( BITMAPS::via )
        .Flags( AF_NONE )
        .Parameter<int>( VIA_ACTION_FLAGS::VIA ) );

static const TOOL_ACTION ACT_PlaceBlindVia( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveRouter.PlaceBlindVia" )
        .Scope( AS_CONTEXT )
        .DefaultHotkey( MD_ALT + MD_SHIFT + 'V' )
        .LegacyHotkeyName( "Add Blind/Buried Via" )
        .FriendlyName( _( "Place Blind/Buried Via" ) )
        .Tooltip( _( "Adds a blind or buried via at the end of currently routed track.") )
        .Icon( BITMAPS::via_buried )
        .Flags( AF_NONE )
        .Parameter<int>( VIA_ACTION_FLAGS::BLIND_VIA ) );

static const TOOL_ACTION ACT_PlaceMicroVia( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveRouter.PlaceMicroVia" )
        .Scope( AS_CONTEXT )
        .DefaultHotkey( MD_CTRL + 'V' )
        .LegacyHotkeyName( "Add MicroVia" )
        .FriendlyName( _( "Place Microvia" ) )
        .Tooltip( _( "Adds a microvia at the end of currently routed track." ) )
        .Icon( BITMAPS::via_microvia )
        .Flags( AF_NONE )
        .Parameter<int>( VIA_ACTION_FLAGS::MICROVIA ) );

static const TOOL_ACTION ACT_SelLayerAndPlaceThroughVia( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveRouter.SelLayerAndPlaceVia" )
        .Scope( AS_CONTEXT )
        .DefaultHotkey( '<' )
        .LegacyHotkeyName( "Select Layer and Add Through Via" )
        .FriendlyName( _( "Select Layer and Place Through Via..." ) )
        .Tooltip( _( "Select a layer, then add a through-hole via at the end of currently routed track." ) )
        .Icon( BITMAPS::select_w_layer )
        .Flags( AF_NONE )
        .Parameter<int>( VIA_ACTION_FLAGS::VIA | VIA_ACTION_FLAGS::SELECT_LAYER ) );

static const TOOL_ACTION ACT_SelLayerAndPlaceBlindVia( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveRouter.SelLayerAndPlaceBlindVia" )
        .Scope( AS_CONTEXT )
        .DefaultHotkey( MD_ALT + '<' )
        .LegacyHotkeyName( "Select Layer and Add Blind/Buried Via" )
        .FriendlyName( _( "Select Layer and Place Blind/Buried Via..." ) )
        .Tooltip( _( "Select a layer, then add a blind or buried via at the end of currently routed track." ) )
        .Icon( BITMAPS::select_w_layer )
        .Flags( AF_NONE )
        .Parameter<int>( VIA_ACTION_FLAGS::BLIND_VIA | VIA_ACTION_FLAGS::SELECT_LAYER ) );

static const TOOL_ACTION ACT_SelLayerAndPlaceMicroVia( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveRouter.SelLayerAndPlaceMicroVia" )
        .Scope( AS_CONTEXT )
        .FriendlyName( _( "Select Layer and Place Micro Via..." ) )
        .Tooltip( _( "Select a layer, then add a micro via at the end of currently routed track." ) )
        .Icon( BITMAPS::select_w_layer )
        .Flags( AF_NONE )
        .Parameter<int>( VIA_ACTION_FLAGS::MICROVIA | VIA_ACTION_FLAGS::SELECT_LAYER ) );

static const TOOL_ACTION ACT_CustomTrackWidth( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveRouter.CustomTrackViaSize" )
        .Scope( AS_CONTEXT )
        .DefaultHotkey( 'Q' )
        .LegacyHotkeyName( "Custom Track/Via Size" )
        .FriendlyName( _( "Custom Track/Via Size..." ) )
        .Tooltip( _( "Shows a dialog for changing the track width and via size." ) )
        .Icon( BITMAPS::width_track ) );

static const TOOL_ACTION ACT_SwitchPosture( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveRouter.SwitchPosture" )
        .Scope( AS_CONTEXT )
        .DefaultHotkey( '/' )
        .LegacyHotkeyName( "Switch Track Posture" )
        .FriendlyName( _( "Switch Track Posture" ) )
        .Tooltip( _( "Switches posture of the currently routed track." ) )
        .Icon( BITMAPS::change_entry_orient ) );

        // This old command ( track corner switch mode) is now moved to a submenu with other corner mode options
static const TOOL_ACTION ACT_SwitchCornerModeToNext( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveRouter.SwitchRoundingToNext" )
        .Scope( AS_CONTEXT )
        .DefaultHotkey( MD_CTRL + '/' )
        .FriendlyName( _( "Track Corner Mode Switch" ) )
        .Tooltip( _( "Switches between sharp/rounded and 45°/90° corners when routing tracks." ) )
        .Icon( BITMAPS::switch_corner_rounding_shape ) );

// hotkeys W and Shift+W  are used to switch to track width changes
static const TOOL_ACTION ACT_SwitchCornerMode45( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveRouter.SwitchRounding45" )
        .Scope( AS_CONTEXT )
        .DefaultHotkey( MD_CTRL + 'W' )
        .FriendlyName( _( "Track Corner Mode 45" ) )
        .Tooltip( _( "Switch to 45° corner when routing tracks." ) ) );

static const TOOL_ACTION ACT_SwitchCornerMode90( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveRouter.SwitchRounding90" )
        .Scope( AS_CONTEXT )
        .DefaultHotkey(  MD_CTRL + MD_ALT + 'W' )
        .FriendlyName( _( "Track Corner Mode 90" ) )
        .Tooltip( _( "Switch to 90° corner when routing tracks." ) ) );

static const TOOL_ACTION ACT_SwitchCornerModeArc45( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveRouter.SwitchRoundingArc45" )
        .Scope( AS_CONTEXT )
        .DefaultHotkey( MD_CTRL + MD_SHIFT + 'W' )
        .FriendlyName( _( "Track Corner Mode Arc 45" ) )
        .Tooltip( _( "Switch to arc 45° corner when routing tracks." ) ) );

static const TOOL_ACTION ACT_SwitchCornerModeArc90( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveRouter.SwitchRoundingArc90" )
        .Scope( AS_CONTEXT )
        .DefaultHotkey( MD_ALT + 'W' )
        .FriendlyName( _( "Track Corner Mode Arc 90" ) )
        .Tooltip( _( "Switch to arc 90° corner when routing tracks." ) ) );

#undef _
#define _(s) wxGetTranslation((s))


ROUTER_TOOL::ROUTER_TOOL() :
        TOOL_BASE( "pcbnew.InteractiveRouter" ),
        m_lastTargetLayer( UNDEFINED_LAYER ),
        m_originalActiveLayer( UNDEFINED_LAYER ),
        m_inRouterTool( false )
{
}


class TRACK_WIDTH_MENU : public ACTION_MENU
{
public:
    TRACK_WIDTH_MENU( PCB_EDIT_FRAME& aFrame ) :
        ACTION_MENU( true ),
        m_frame( aFrame )
    {
        SetIcon( BITMAPS::width_track_via );
        SetTitle( _( "Select Track/Via Width" ) );
    }

protected:
    ACTION_MENU* create() const override
    {
        return new TRACK_WIDTH_MENU( m_frame );
    }

    void update() override
    {
        BOARD_DESIGN_SETTINGS& bds = m_frame.GetBoard()->GetDesignSettings();
        bool                   useIndex = !bds.m_UseConnectedTrackWidth &&
                                          !bds.UseCustomTrackViaSize();
        wxString               msg;

        Clear();

        Append( ID_POPUP_PCB_SELECT_AUTO_WIDTH, _( "Use Starting Track Width" ),
                _( "Route using the width of the starting track." ), wxITEM_CHECK );
        Check( ID_POPUP_PCB_SELECT_AUTO_WIDTH,
               bds.m_UseConnectedTrackWidth && !bds.UseCustomTrackViaSize() );

        Append( ID_POPUP_PCB_SELECT_USE_NETCLASS_VALUES, _( "Use Net Class Values" ),
                _( "Use track and via sizes from the net class" ), wxITEM_CHECK );
        Check( ID_POPUP_PCB_SELECT_USE_NETCLASS_VALUES,
               useIndex && bds.GetTrackWidthIndex() == 0 && bds.GetViaSizeIndex() == 0 );

        Append( ID_POPUP_PCB_SELECT_CUSTOM_WIDTH, _( "Use Custom Values..." ),
                _( "Specify custom track and via sizes" ), wxITEM_CHECK );
        Check( ID_POPUP_PCB_SELECT_CUSTOM_WIDTH, bds.UseCustomTrackViaSize() );

        AppendSeparator();

        // Append the list of tracks & via sizes
        for( unsigned i = 0; i < bds.m_TrackWidthList.size(); i++ )
        {
            int width = bds.m_TrackWidthList[i];

            if( i == 0 )
                msg = _( "Track netclass width" );
            else
                msg.Printf( _( "Track %s" ), m_frame.MessageTextFromValue( width ) );

            int menuIdx = ID_POPUP_PCB_SELECT_WIDTH1 + i;
            Append( menuIdx, msg, wxEmptyString, wxITEM_CHECK );
            Check( menuIdx, useIndex && bds.GetTrackWidthIndex() == i );
        }

        AppendSeparator();

        for( unsigned i = 0; i < bds.m_ViasDimensionsList.size(); i++ )
        {
            VIA_DIMENSION via = bds.m_ViasDimensionsList[i];

            if( i == 0 )
                msg = _( "Via netclass values" );
            else
            {
                if( via.m_Drill > 0 )
                {
                    msg.Printf( _("Via %s, hole %s" ),
                                m_frame.MessageTextFromValue( via.m_Diameter ),
                                m_frame.MessageTextFromValue( via.m_Drill ) );
                }
                else
                {
                    msg.Printf( _( "Via %s" ),
                                m_frame.MessageTextFromValue( via.m_Diameter ) );
                }
            }

            int menuIdx = ID_POPUP_PCB_SELECT_VIASIZE1 + i;
            Append( menuIdx, msg, wxEmptyString, wxITEM_CHECK );
            Check( menuIdx, useIndex && bds.GetViaSizeIndex() == i );
        }
    }

    OPT_TOOL_EVENT eventHandler( const wxMenuEvent& aEvent ) override
    {
        BOARD_DESIGN_SETTINGS &bds = m_frame.GetBoard()->GetDesignSettings();
        int id = aEvent.GetId();

        // On Windows, this handler can be called with an event ID not existing in any
        // menuitem, so only set flags when we have an ID match.

        if( id == ID_POPUP_PCB_SELECT_CUSTOM_WIDTH )
        {
            bds.UseCustomTrackViaSize( true );
            bds.m_TempOverrideTrackWidth = true;
            m_frame.GetToolManager()->RunAction( ACT_CustomTrackWidth );
        }
        else if( id == ID_POPUP_PCB_SELECT_AUTO_WIDTH )
        {
            bds.UseCustomTrackViaSize( false );
            bds.m_UseConnectedTrackWidth = true;
            bds.m_TempOverrideTrackWidth = false;
        }
        else if( id == ID_POPUP_PCB_SELECT_USE_NETCLASS_VALUES )
        {
            bds.UseCustomTrackViaSize( false );
            bds.m_UseConnectedTrackWidth = false;
            bds.SetViaSizeIndex( 0 );
            bds.SetTrackWidthIndex( 0 );
        }
        else if( id >= ID_POPUP_PCB_SELECT_VIASIZE1 && id <= ID_POPUP_PCB_SELECT_VIASIZE16 )
        {
            bds.UseCustomTrackViaSize( false );
            bds.SetViaSizeIndex( id - ID_POPUP_PCB_SELECT_VIASIZE1 );
        }
        else if( id >= ID_POPUP_PCB_SELECT_WIDTH1 && id <= ID_POPUP_PCB_SELECT_WIDTH16 )
        {
            bds.UseCustomTrackViaSize( false );
            bds.m_TempOverrideTrackWidth = true;
            bds.SetTrackWidthIndex( id - ID_POPUP_PCB_SELECT_WIDTH1 );
        }

        return OPT_TOOL_EVENT( PCB_ACTIONS::trackViaSizeChanged.MakeEvent() );
    }

private:
    PCB_EDIT_FRAME& m_frame;
};


class DIFF_PAIR_MENU : public ACTION_MENU
{
public:
    DIFF_PAIR_MENU( PCB_EDIT_FRAME& aFrame ) :
        ACTION_MENU( true ),
        m_frame( aFrame )
    {
        SetIcon( BITMAPS::width_track_via );
        SetTitle( _( "Select Differential Pair Dimensions" ) );
    }

protected:
    ACTION_MENU* create() const override
    {
        return new DIFF_PAIR_MENU( m_frame );
    }

    void update() override
    {
        const BOARD_DESIGN_SETTINGS& bds = m_frame.GetBoard()->GetDesignSettings();

        Clear();

        Append( ID_POPUP_PCB_SELECT_USE_NETCLASS_DIFFPAIR, _( "Use Net Class Values" ),
                _( "Use differential pair dimensions from the net class" ), wxITEM_CHECK );
        Check( ID_POPUP_PCB_SELECT_USE_NETCLASS_DIFFPAIR,
               !bds.UseCustomDiffPairDimensions() && bds.GetDiffPairIndex() == 0 );

        Append( ID_POPUP_PCB_SELECT_CUSTOM_DIFFPAIR, _( "Use Custom Values..." ),
                _( "Specify custom differential pair dimensions" ), wxITEM_CHECK );
        Check( ID_POPUP_PCB_SELECT_CUSTOM_DIFFPAIR, bds.UseCustomDiffPairDimensions() );

        AppendSeparator();

        // Append the list of differential pair dimensions

        // Drop index 0 which is the current netclass dimensions (which are handled above)
        for( unsigned i = 1; i < bds.m_DiffPairDimensionsList.size(); ++i )
        {
            DIFF_PAIR_DIMENSION diffPair = bds.m_DiffPairDimensionsList[i];
            wxString            msg;

            if( diffPair.m_Gap <= 0 )
            {
                if( diffPair.m_ViaGap <= 0 )
                {
                    msg.Printf( _( "Width %s" ),
                                m_frame.MessageTextFromValue( diffPair.m_Width ) );
                }
                else
                {
                    msg.Printf( _( "Width %s, via gap %s" ),
                                m_frame.MessageTextFromValue( diffPair.m_Width ),
                                m_frame.MessageTextFromValue( diffPair.m_ViaGap ) );
                }
            }
            else
            {
                if( diffPair.m_ViaGap <= 0 )
                {
                    msg.Printf( _( "Width %s, gap %s" ),
                                m_frame.MessageTextFromValue( diffPair.m_Width ),
                                m_frame.MessageTextFromValue( diffPair.m_Gap ) );
                }
                else
                {
                    msg.Printf( _( "Width %s, gap %s, via gap %s" ),
                                m_frame.MessageTextFromValue( diffPair.m_Width ),
                                m_frame.MessageTextFromValue( diffPair.m_Gap ),
                                m_frame.MessageTextFromValue( diffPair.m_ViaGap ) );
                }
            }

            int menuIdx = ID_POPUP_PCB_SELECT_DIFFPAIR1 + i - 1;
            Append( menuIdx, msg, wxEmptyString, wxITEM_CHECK );
            Check( menuIdx, !bds.UseCustomDiffPairDimensions() && bds.GetDiffPairIndex() == i );
        }
    }

    OPT_TOOL_EVENT eventHandler( const wxMenuEvent& aEvent ) override
    {
        BOARD_DESIGN_SETTINGS &bds = m_frame.GetBoard()->GetDesignSettings();
        int id = aEvent.GetId();

        // On Windows, this handler can be called with an event ID not existing in any
        // menuitem, so only set flags when we have an ID match.

        if( id == ID_POPUP_PCB_SELECT_CUSTOM_DIFFPAIR )
        {
            bds.UseCustomDiffPairDimensions( true );
            TOOL_MANAGER* toolManager = m_frame.GetToolManager();
            toolManager->RunAction( PCB_ACTIONS::routerDiffPairDialog );
        }
        else if( id == ID_POPUP_PCB_SELECT_USE_NETCLASS_DIFFPAIR )
        {
            bds.UseCustomDiffPairDimensions( false );
            bds.SetDiffPairIndex( 0 );
        }
        else if( id >= ID_POPUP_PCB_SELECT_DIFFPAIR1 && id <= ID_POPUP_PCB_SELECT_DIFFPAIR16 )
        {
            bds.UseCustomDiffPairDimensions( false );
            // remember that the menu doesn't contain index 0 (which is the netclass values)
            bds.SetDiffPairIndex( id - ID_POPUP_PCB_SELECT_DIFFPAIR1 + 1 );
        }

        return OPT_TOOL_EVENT( PCB_ACTIONS::trackViaSizeChanged.MakeEvent() );
    }

private:
    PCB_EDIT_FRAME& m_frame;
};


ROUTER_TOOL::~ROUTER_TOOL()
{
}


bool ROUTER_TOOL::Init()
{
    m_originalActiveLayer = UNDEFINED_LAYER;

    PCB_EDIT_FRAME* frame = getEditFrame<PCB_EDIT_FRAME>();

    wxASSERT( frame );

    auto& menu = m_menu->GetMenu();
    menu.SetUntranslatedTitle( _HKI( "Interactive Router" ) );

    m_trackViaMenu = std::make_shared<TRACK_WIDTH_MENU>( *frame );
    m_trackViaMenu->SetTool( this );
    m_menu->RegisterSubMenu( m_trackViaMenu );

    m_diffPairMenu = std::make_shared<DIFF_PAIR_MENU>( *frame );
    m_diffPairMenu->SetTool( this );
    m_menu->RegisterSubMenu( m_diffPairMenu );

    ACTION_MANAGER* mgr = frame->GetToolManager()->GetActionManager();

    auto haveHighlight =
            [this]( const SELECTION& sel )
            {
                KIGFX::RENDER_SETTINGS* cfg = m_toolMgr->GetView()->GetPainter()->GetSettings();

                return !cfg->GetHighlightNetCodes().empty();
            };

    auto notRoutingCond =
            [this]( const SELECTION& )
            {
                return !m_router->RoutingInProgress();
            };

    auto hasOtherEnd =
            [this]( const SELECTION& )
            {
                std::vector<PNS::NET_HANDLE> currentNets = m_router->GetCurrentNets();

                if( currentNets.empty() || currentNets[0] == nullptr )
                    return false;

                // Need to have something unconnected to finish to
                NETINFO_ITEM* netInfo = static_cast<NETINFO_ITEM*>( currentNets[0] );
                int           currentNet = netInfo->GetNetCode();
                BOARD*        board = getEditFrame<PCB_EDIT_FRAME>()->GetBoard();
                RN_NET*       ratsnest = board->GetConnectivity()->GetRatsnestForNet( currentNet );

                return ratsnest && !ratsnest->GetEdges().empty();
            };

    menu.AddItem( ACTIONS::cancelInteractive,         SELECTION_CONDITIONS::ShowAlways, 1 );
    menu.AddSeparator( 1 );

    menu.AddItem( PCB_ACTIONS::clearHighlight,        haveHighlight, 2 );
    menu.AddSeparator(                                haveHighlight, 2 );

    menu.AddItem( PCB_ACTIONS::routeSingleTrack,      notRoutingCond );
    menu.AddItem( PCB_ACTIONS::routeDiffPair,         notRoutingCond );
    menu.AddItem( ACTIONS::finishInteractive,         SELECTION_CONDITIONS::ShowAlways );
    menu.AddItem( PCB_ACTIONS::routerUndoLastSegment, SELECTION_CONDITIONS::ShowAlways );
    menu.AddItem( PCB_ACTIONS::routerContinueFromEnd, hasOtherEnd );
    menu.AddItem( PCB_ACTIONS::routerAttemptFinish,   hasOtherEnd );
    menu.AddItem( PCB_ACTIONS::routerAutorouteSelected, notRoutingCond
                                                            && SELECTION_CONDITIONS::NotEmpty );
    menu.AddItem( PCB_ACTIONS::breakTrack,            notRoutingCond );

    menu.AddItem( PCB_ACTIONS::drag45Degree,          notRoutingCond );
    menu.AddItem( PCB_ACTIONS::dragFreeAngle,         notRoutingCond );

    menu.AddItem( ACT_PlaceThroughVia,                SELECTION_CONDITIONS::ShowAlways );
    menu.AddItem( ACT_PlaceBlindVia,                  SELECTION_CONDITIONS::ShowAlways );
    menu.AddItem( ACT_PlaceMicroVia,                  SELECTION_CONDITIONS::ShowAlways );
    menu.AddItem( ACT_SelLayerAndPlaceThroughVia,     SELECTION_CONDITIONS::ShowAlways );
    menu.AddItem( ACT_SelLayerAndPlaceBlindVia,       SELECTION_CONDITIONS::ShowAlways );
    menu.AddItem( ACT_SelLayerAndPlaceMicroVia,       SELECTION_CONDITIONS::ShowAlways );
    menu.AddItem( ACT_SwitchPosture,                  SELECTION_CONDITIONS::ShowAlways );

    // Add submenu for track corner mode handling
    CONDITIONAL_MENU* submenuCornerMode = new CONDITIONAL_MENU( this );
    submenuCornerMode->SetTitle( _( "Track Corner Mode" ) );
    submenuCornerMode->SetIcon( BITMAPS::switch_corner_rounding_shape );

    submenuCornerMode->AddItem( ACT_SwitchCornerModeToNext, SELECTION_CONDITIONS::ShowAlways );
    submenuCornerMode->AddSeparator( 1 );
    submenuCornerMode->AddCheckItem( ACT_SwitchCornerMode45, SELECTION_CONDITIONS::ShowAlways );
    submenuCornerMode->AddCheckItem( ACT_SwitchCornerModeArc45, SELECTION_CONDITIONS::ShowAlways );
    submenuCornerMode->AddCheckItem( ACT_SwitchCornerMode90, SELECTION_CONDITIONS::ShowAlways );
    submenuCornerMode->AddCheckItem( ACT_SwitchCornerModeArc90, SELECTION_CONDITIONS::ShowAlways );

    menu.AddMenu( submenuCornerMode );

    // Manage check/uncheck marks in this submenu items
    auto cornerMode45Cond =
        [this]( const SELECTION& )
        {
            return m_router->Settings().GetCornerMode() == DIRECTION_45::CORNER_MODE::MITERED_45;
        };

    auto cornerMode90Cond =
        [this]( const SELECTION& )
        {
            return m_router->Settings().GetCornerMode() == DIRECTION_45::CORNER_MODE::MITERED_90;
        };

    auto cornerModeArc45Cond =
        [this]( const SELECTION& )
        {
            return m_router->Settings().GetCornerMode() == DIRECTION_45::CORNER_MODE::ROUNDED_45;
        };

    auto cornerModeArc90Cond =
        [this]( const SELECTION& )
        {
            return m_router->Settings().GetCornerMode() == DIRECTION_45::CORNER_MODE::ROUNDED_90;
        };

#define CHECK( x )  ACTION_CONDITIONS().Check( x )
    mgr->SetConditions( ACT_SwitchCornerMode45,       CHECK( cornerMode45Cond ) );
    mgr->SetConditions( ACT_SwitchCornerMode90,       CHECK( cornerMode90Cond ) );
    mgr->SetConditions( ACT_SwitchCornerModeArc45,    CHECK( cornerModeArc45Cond ) );
    mgr->SetConditions( ACT_SwitchCornerModeArc90,    CHECK( cornerModeArc90Cond ) );

    auto diffPairCond =
        [this]( const SELECTION& )
        {
            return m_router->Mode() == PNS::PNS_MODE_ROUTE_DIFF_PAIR;
        };

    menu.AddSeparator();

    menu.AddMenu( m_trackViaMenu.get(),               SELECTION_CONDITIONS::ShowAlways );
    menu.AddMenu( m_diffPairMenu.get(),               diffPairCond );

    menu.AddItem( PCB_ACTIONS::routerSettingsDialog,  SELECTION_CONDITIONS::ShowAlways );

    menu.AddSeparator();

    frame->AddStandardSubMenus( *m_menu.get() );

    return true;
}


void ROUTER_TOOL::Reset( RESET_REASON aReason )
{
    if( aReason == RUN )
        TOOL_BASE::Reset( aReason );
}

// Saves the complete event log and the dump of the PCB, allowing us to
// recreate hard-to-find P&S quirks and bugs.

void ROUTER_TOOL::saveRouterDebugLog()
{
    static wxString mruPath = PATHS::GetDefaultUserProjectsPath();
    static size_t   lastLoggerSize = 0;

    auto logger = m_router->Logger();

    if( !logger || logger->GetEvents().size() == 0
        || logger->GetEvents().size() == lastLoggerSize )
    {
        return;
    }

    wxFileDialog dlg( frame(), _( "Save router log" ), mruPath, "pns.log",
                      "PNS log files" + AddFileExtListToFilter( { "log" } ),
                      wxFD_OVERWRITE_PROMPT | wxFD_SAVE );

    if( dlg.ShowModal() != wxID_OK )
    {
        lastLoggerSize = logger->GetEvents().size(); // prevent re-entry
        return;
    }

    wxFileName fname_log( dlg.GetPath() );
    mruPath = fname_log.GetPath();

    wxFileName fname_dump( fname_log );
    fname_dump.SetExt( "dump" );

    wxFileName fname_settings( fname_log );
    fname_settings.SetExt( "settings" );

    FILE* settings_f = wxFopen( fname_settings.GetAbsolutePath(), "wb" );
    std::string settingsStr = m_router->Settings().FormatAsString();
    fprintf( settings_f, "%s\n", settingsStr.c_str() );
    fclose( settings_f );

    // Export as *.kicad_pcb format, using a strategy which is specifically chosen
    // as an example on how it could also be used to send it to the system clipboard.

    PCB_IO_KICAD_SEXPR  pcb_io;

    pcb_io.SaveBoard( fname_dump.GetAbsolutePath(), m_iface->GetBoard(), nullptr );

    PROJECT* prj = m_iface->GetBoard()->GetProject();
    prj->GetProjectFile().SaveAs( fname_dump.GetPath(), fname_dump.GetName() );
    prj->GetLocalSettings().SaveAs( fname_dump.GetPath(), fname_dump.GetName() );

    // Build log file:
    std::vector<PNS::ITEM*> added, removed, heads;
    m_router->GetUpdatedItems( removed, added, heads );

    std::set<KIID> removedKIIDs;

    for( auto item : removed )
    {
        wxASSERT_MSG( item->Parent() != nullptr, "removed an item with no parent uuid?" );

        if( item->Parent() )
            removedKIIDs.insert( item->Parent()->m_Uuid );
    }

    FILE*    log_f = wxFopen( fname_log.GetAbsolutePath(), "wb" );
    wxString logString = PNS::LOGGER::FormatLogFileAsString( m_router->Mode(),
                                                             added, removedKIIDs, heads,
                                                             logger->GetEvents() );

    if( !log_f )
    {
        DisplayError( frame(), wxString::Format( _( "Unable to write '%s'." ),
                                                 fname_log.GetAbsolutePath() ) );
        return;
    }

    fprintf( log_f, "%s\n", logString.c_str().AsChar() );
    fclose( log_f );

    logger->Clear(); // prevent re-entry
    lastLoggerSize = 0;
}


void ROUTER_TOOL::handleCommonEvents( TOOL_EVENT& aEvent )
{
    if( aEvent.Category() == TC_VIEW || aEvent.Category() == TC_MOUSE )
    {
        BOX2D viewAreaD = getView()->GetGAL()->GetVisibleWorldExtents();
        m_router->SetVisibleViewArea( BOX2ISafe( viewAreaD ) );
    }

    if( !ADVANCED_CFG::GetCfg().m_EnableRouterDump )
        return;

    if( !aEvent.IsKeyPressed() )
        return;

    switch( aEvent.KeyCode() )
    {
    case '0':
        saveRouterDebugLog();
        aEvent.SetPassEvent( false );
        break;

    default:
        break;
    }
}

int ROUTER_TOOL::handlePnSCornerModeChange( const TOOL_EVENT& aEvent )
{
    bool asChanged = false;

    if( aEvent.IsAction( &ACT_SwitchCornerModeToNext ) )
    {
        DIRECTION_45::CORNER_MODE curr_mode = m_router->Settings().GetCornerMode();

        if( curr_mode == DIRECTION_45::CORNER_MODE::MITERED_45 )
            m_router->Settings().SetCornerMode( DIRECTION_45::CORNER_MODE::ROUNDED_45 );
        else if( curr_mode == DIRECTION_45::CORNER_MODE::ROUNDED_45 )
            m_router->Settings().SetCornerMode( DIRECTION_45::CORNER_MODE::MITERED_90 );
        else if( curr_mode == DIRECTION_45::CORNER_MODE::MITERED_90 )
            m_router->Settings().SetCornerMode( DIRECTION_45::CORNER_MODE::ROUNDED_90 );
        else if( curr_mode == DIRECTION_45::CORNER_MODE::ROUNDED_90 )
            m_router->Settings().SetCornerMode( DIRECTION_45::CORNER_MODE::MITERED_45 );

        asChanged = true;
    }
    else if( aEvent.IsAction( &ACT_SwitchCornerMode45 ) )
    {
        m_router->Settings().SetCornerMode( DIRECTION_45::CORNER_MODE::MITERED_45 );
        asChanged = true;
    }
    else if( aEvent.IsAction( &ACT_SwitchCornerModeArc45 ) )
    {
        m_router->Settings().SetCornerMode( DIRECTION_45::CORNER_MODE::ROUNDED_45 );
        asChanged = true;
    }
    else if( aEvent.IsAction( &ACT_SwitchCornerMode90 ) )
    {
        m_router->Settings().SetCornerMode( DIRECTION_45::CORNER_MODE::MITERED_90 );
        asChanged = true;
    }
    else if( aEvent.IsAction( &ACT_SwitchCornerModeArc90 ) )
    {
        m_router->Settings().SetCornerMode( DIRECTION_45::CORNER_MODE::ROUNDED_90 );
        asChanged = true;
    }

    if( asChanged )
    {
        UpdateMessagePanel();
        updateEndItem( aEvent );
        m_router->Move( m_endSnapPoint, m_endItem );        // refresh
    }

    return 0;
}


PCB_LAYER_ID ROUTER_TOOL::getStartLayer( const PNS::ITEM* aItem )
{
    PCB_LAYER_ID tl = static_cast<PCB_LAYER_ID>( getView()->GetTopLayer() );

    if( m_startItem )
    {
        int startLayer = m_iface->GetPNSLayerFromBoardLayer( tl );
        const PNS_LAYER_RANGE& ls = m_startItem->Layers();

        if( ls.Overlaps( startLayer ) )
            return tl;
        else
            return m_iface->GetBoardLayerFromPNSLayer( ls.Start() );
    }

    return tl;
}


void ROUTER_TOOL::switchLayerOnViaPlacement()
{
    int activeLayer = m_iface->GetPNSLayerFromBoardLayer( frame()->GetActiveLayer() );
    int currentLayer = m_router->GetCurrentLayer();

    if( currentLayer != activeLayer )
        m_router->SwitchLayer( activeLayer );

    std::optional<int> newLayer = m_router->Sizes().PairedLayer( currentLayer );

    if( !newLayer )
        newLayer = m_router->Sizes().GetLayerTop();

    m_router->SwitchLayer( *newLayer );
    m_lastTargetLayer = m_iface->GetBoardLayerFromPNSLayer( *newLayer );

    updateSizesAfterRouterEvent( *newLayer, m_endSnapPoint );
    UpdateMessagePanel();
}


// N.B. aTargetLayer is a PNS layer, not a PCB_LAYER_ID
void ROUTER_TOOL::updateSizesAfterRouterEvent( int aTargetLayer, const VECTOR2I& aPos )
{
    std::vector<PNS::NET_HANDLE> nets = m_router->GetCurrentNets();

    PNS::SIZES_SETTINGS          sizes     = m_router->Sizes();
    BOARD_DESIGN_SETTINGS&       bds       = board()->GetDesignSettings();
    std::shared_ptr<DRC_ENGINE>& drcEngine = bds.m_DRCEngine;
    DRC_CONSTRAINT               constraint;
    PCB_LAYER_ID                 targetLayer = m_iface->GetBoardLayerFromPNSLayer( aTargetLayer );

    PCB_TRACK dummyTrack( board() );
    dummyTrack.SetFlags( ROUTER_TRANSIENT );
    dummyTrack.SetLayer( targetLayer );
    dummyTrack.SetNet( nets.empty() ? nullptr: static_cast<NETINFO_ITEM*>( nets[0] ) );
    dummyTrack.SetStart( aPos );
    dummyTrack.SetEnd( dummyTrack.GetStart() );

    constraint = drcEngine->EvalRules( CLEARANCE_CONSTRAINT, &dummyTrack, nullptr, targetLayer );

    if( constraint.m_Value.Min() >= bds.m_MinClearance )
    {
        sizes.SetClearance( constraint.m_Value.Min() );
        sizes.SetClearanceSource( constraint.GetName() );
    }
    else
    {
        sizes.SetClearance( bds.m_MinClearance );
        sizes.SetClearanceSource( _( "board minimum clearance" ) );
    }

    if( bds.UseNetClassTrack() || !sizes.TrackWidthIsExplicit() )
    {
        constraint = drcEngine->EvalRules( TRACK_WIDTH_CONSTRAINT, &dummyTrack, nullptr,
                                           targetLayer );

        if( !constraint.IsNull() )
        {
            int width = sizes.TrackWidth();

            // Only change the size if we're explicitly using the net class, or we're out of range
            // for our new constraints. Otherwise, just leave the track width alone so we don't
            // change for no reason.
            if( bds.UseNetClassTrack()
                    || ( width < bds.m_TrackMinWidth )
                    || ( width < constraint.m_Value.Min() )
                    || ( width > constraint.m_Value.Max() ) )
            {
                sizes.SetTrackWidth( std::max( bds.m_TrackMinWidth, constraint.m_Value.Opt() ) );
            }

            if( sizes.TrackWidth() == constraint.m_Value.Opt() )
                sizes.SetWidthSource( constraint.GetName() );
            else if( sizes.TrackWidth() == bds.m_TrackMinWidth )
                sizes.SetWidthSource( _( "board minimum track width" ) );
            else
                sizes.SetWidthSource( _( "existing track" ) );
        }
    }

    if( nets.size() >= 2 && ( bds.UseNetClassDiffPair() || !sizes.TrackWidthIsExplicit() ) )
    {
        PCB_TRACK dummyTrackB( board() );
        dummyTrackB.SetFlags( ROUTER_TRANSIENT );
        dummyTrackB.SetLayer( targetLayer );
        dummyTrackB.SetNet( static_cast<NETINFO_ITEM*>( nets[1] ) );
        dummyTrackB.SetStart( aPos );
        dummyTrackB.SetEnd( dummyTrackB.GetStart() );

        constraint = drcEngine->EvalRules( TRACK_WIDTH_CONSTRAINT, &dummyTrack, &dummyTrackB,
                                           targetLayer );

        if( !constraint.IsNull() )
        {
            if( bds.UseNetClassDiffPair()
                    || ( sizes.DiffPairWidth() < bds.m_TrackMinWidth )
                    || ( sizes.DiffPairWidth() < constraint.m_Value.Min() )
                    || ( sizes.DiffPairWidth() > constraint.m_Value.Max() ) )
            {
                sizes.SetDiffPairWidth( std::max( bds.m_TrackMinWidth, constraint.m_Value.Opt() ) );
            }

            if( sizes.DiffPairWidth() == constraint.m_Value.Opt() )
                sizes.SetDiffPairWidthSource( constraint.GetName() );
            else
                sizes.SetDiffPairWidthSource( _( "board minimum track width" ) );
        }

        constraint = drcEngine->EvalRules( DIFF_PAIR_GAP_CONSTRAINT, &dummyTrack, &dummyTrackB,
                                           targetLayer );

        if( !constraint.IsNull() )
        {
            if( bds.UseNetClassDiffPair()
                    || ( sizes.DiffPairGap() < bds.m_MinClearance )
                    || ( sizes.DiffPairGap() < constraint.m_Value.Min() )
                    || ( sizes.DiffPairGap() > constraint.m_Value.Max() ) )
            {
                sizes.SetDiffPairGap( std::max( bds.m_MinClearance, constraint.m_Value.Opt() ) );
            }

            if( sizes.DiffPairGap() == constraint.m_Value.Opt() )
                sizes.SetDiffPairGapSource( constraint.GetName() );
            else
                sizes.SetDiffPairGapSource( _( "board minimum clearance" ) );
        }
    }

    m_router->UpdateSizes( sizes );
}


static VIATYPE getViaTypeFromFlags( int aFlags )
{
    switch( aFlags & VIA_ACTION_FLAGS::VIA_MASK )
    {
    case VIA_ACTION_FLAGS::VIA:
        return VIATYPE::THROUGH;
    case VIA_ACTION_FLAGS::BLIND_VIA:
        return VIATYPE::BLIND;
    case VIA_ACTION_FLAGS::BURIED_VIA:
        return VIATYPE::BURIED;
    case VIA_ACTION_FLAGS::MICROVIA:
        return VIATYPE::MICROVIA;
    default:
        wxASSERT_MSG( false, wxT( "Unhandled via type" ) );
        return VIATYPE::THROUGH;
    }
}


int ROUTER_TOOL::onLayerCommand( const TOOL_EVENT& aEvent )
{
    handleLayerSwitch( aEvent, false );
    UpdateMessagePanel();

    return 0;
}


int ROUTER_TOOL::onViaCommand( const TOOL_EVENT& aEvent )
{
    if( !m_router->IsPlacingVia() )
    {
        return handleLayerSwitch( aEvent, true );
    }
    else
    {
        m_router->ToggleViaPlacement();
        frame()->SetActiveLayer(
                m_iface->GetBoardLayerFromPNSLayer( m_router->GetCurrentLayer() ) );
        updateEndItem( aEvent );
        m_router->Move( m_endSnapPoint, m_endItem );
    }

    UpdateMessagePanel();
    return 0;
}


int ROUTER_TOOL::handleLayerSwitch( const TOOL_EVENT& aEvent, bool aForceVia )
{
    wxCHECK( m_router, 0 );

    if( !IsToolActive() )
        return 0;

    // Ensure PNS_KICAD_IFACE (m_iface) m_board member is up to date
    // For some reason, this is not always the case
    m_iface->SetBoard( board() );

    // First see if this is one of the switch layer commands
    BOARD*       brd           = board();
    LSET         enabledLayers = LSET::AllCuMask( brd->GetDesignSettings().GetCopperLayerCount() );
    LSEQ         layers        = enabledLayers.UIOrder();

    // These layers are in Board Layer UI order not PNS layer order
    PCB_LAYER_ID currentLayer = m_iface->GetBoardLayerFromPNSLayer( m_router->GetCurrentLayer() );
    PCB_LAYER_ID targetLayer  = UNDEFINED_LAYER;

    if( aEvent.IsAction( &PCB_ACTIONS::layerNext ) )
    {
        size_t idx = 0;
        size_t target_idx = 0;
        PCB_LAYER_ID lastTargetLayer = m_lastTargetLayer;

        for( size_t i = 0; i < layers.size(); i++ )
        {
            if( layers[i] == currentLayer )
            {
                idx = i;
                break;
            }
        }

        target_idx = ( idx + 1 ) % layers.size();
        // issue: #14480
        // idx + 1 layer may be invisible, switches to next visible layer
        for( size_t i = 0; i < layers.size() - 1; i++ )
        {
            if( brd->IsLayerVisible( layers[target_idx] ) )
            {
                targetLayer = layers[target_idx];
                break;
            }
            target_idx += 1;

            if( target_idx >= layers.size() )
            {
                target_idx = 0;
            }
        }

        if( targetLayer == UNDEFINED_LAYER )
        {
            // if there is no visible layers
            return 0;
        }
    }
    else if( aEvent.IsAction( &PCB_ACTIONS::layerPrev ) )
    {
        size_t idx = 0;
        size_t target_idx = 0;

        for( size_t i = 0; i < layers.size(); i++ )
        {
            if( layers[i] == currentLayer )
            {
                idx = i;
                break;
            }
        }

        target_idx = ( idx > 0 ) ? ( idx - 1 ) : ( layers.size() - 1 );

        for( size_t i = 0; i < layers.size() - 1; i++ )
        {
            if( brd->IsLayerVisible( layers[target_idx] ) )
            {
                targetLayer = layers[target_idx];
                break;
            }

            if( target_idx > 0 )
                target_idx -= 1;
            else
                target_idx = layers.size() - 1;
        }

        if( targetLayer == UNDEFINED_LAYER )
        {
            // if there is no visible layers
            return 0;
        }
    }
    else if( aEvent.IsAction( &PCB_ACTIONS::layerToggle ) )
    {
        PCB_SCREEN* screen = frame()->GetScreen();

        if( currentLayer == screen->m_Route_Layer_TOP )
            targetLayer = screen->m_Route_Layer_BOTTOM;
        else
            targetLayer = screen->m_Route_Layer_TOP;
    }
    else if( aEvent.IsActionInGroup( PCB_ACTIONS::layerDirectSwitchActions() ) )
    {
        targetLayer = aEvent.Parameter<PCB_LAYER_ID>();

        if( !enabledLayers.test( targetLayer ) )
            return 0;
    }

    if( targetLayer != UNDEFINED_LAYER )
    {
        if( targetLayer == currentLayer )
            return 0;

        if( !aForceVia && m_router && m_router->SwitchLayer( m_iface->GetPNSLayerFromBoardLayer( targetLayer ) ) )
        {
            updateEndItem( aEvent );
            updateSizesAfterRouterEvent( m_iface->GetPNSLayerFromBoardLayer( targetLayer ), m_endSnapPoint );
            m_router->Move( m_endSnapPoint, m_endItem );        // refresh
            return 0;
        }
    }

    BOARD_DESIGN_SETTINGS& bds        = board()->GetDesignSettings();
    const int              layerCount = bds.GetCopperLayerCount();

    PCB_LAYER_ID pairTop    = frame()->GetScreen()->m_Route_Layer_TOP;
    PCB_LAYER_ID pairBottom = frame()->GetScreen()->m_Route_Layer_BOTTOM;

    PNS::SIZES_SETTINGS sizes = m_router->Sizes();

    VIATYPE viaType     = VIATYPE::THROUGH;
    bool    selectLayer = false;

    // Otherwise it is one of the router-specific via commands
    if( targetLayer == UNDEFINED_LAYER )
    {
        const int actViaFlags = aEvent.Parameter<int>();
        selectLayer           = actViaFlags & VIA_ACTION_FLAGS::SELECT_LAYER;

        viaType = getViaTypeFromFlags( actViaFlags );

        // ask the user for a target layer
        if( selectLayer )
        {
            // When the currentLayer is undefined, trying to place a via does not work
            // because it means there is no track in progress, and some other variables
            // values are not defined like m_endSnapPoint. So do not continue.
            if( currentLayer == UNDEFINED_LAYER )
                return 0;

            wxPoint endPoint = ToWxPoint( view()->ToScreen( m_endSnapPoint ) );
            endPoint = frame()->GetCanvas()->ClientToScreen( endPoint );

            // Build the list of not allowed layer for the target layer
            LSET not_allowed_ly = LSET::AllNonCuMask();

            if( viaType != VIATYPE::THROUGH )
                not_allowed_ly.set( currentLayer );

            targetLayer = frame()->SelectOneLayer( static_cast<PCB_LAYER_ID>( currentLayer ),
                                                   not_allowed_ly, endPoint );

            // Reset the cursor to the end of the track
            controls()->SetCursorPosition( m_endSnapPoint );

            if( targetLayer == UNDEFINED_LAYER )    // canceled by user
                return 0;

            // One cannot place a blind/buried via on only one layer:
            if( viaType != VIATYPE::THROUGH )
            {
                if( currentLayer == targetLayer )
                    return 0;
            }
        }
    }

    // fixme: P&S supports more than one fixed layer pair. Update the dialog?
    sizes.ClearLayerPairs();

    // Convert blind/buried via to a through hole one, if it goes through all layers
    if( viaType != VIATYPE::THROUGH
            && ( ( targetLayer == B_Cu && currentLayer == F_Cu )
                       || ( targetLayer == F_Cu && currentLayer == B_Cu ) ) )
    {
        viaType = VIATYPE::THROUGH;
    }

    if( targetLayer == UNDEFINED_LAYER )
    {
        // Implicit layer selection
        if( viaType == VIATYPE::THROUGH )
        {
            // Try to switch to the nearest ratnest item's layer if we have one
            VECTOR2I        otherEnd;
            PNS_LAYER_RANGE otherEndLayers;
            PNS::ITEM*      otherEndItem = nullptr;

            if( !m_router->GetNearestRatnestAnchor( otherEnd, otherEndLayers, otherEndItem ) )
            {
                // use the default layer pair
                currentLayer = pairTop;
                targetLayer = pairBottom;
            }
            else
            {
                // use the layer of the other end, unless it is the same layer as the currently active layer, in which
                // case use the layer pair (if applicable)
                PCB_LAYER_ID otherEndLayerPcbId = m_iface->GetBoardLayerFromPNSLayer( otherEndLayers.Start() );
                const std::optional<int> pairedLayerPns = m_router->Sizes().PairedLayer( m_router->GetCurrentLayer() );

                if( currentLayer == otherEndLayerPcbId && pairedLayerPns.has_value() )
                {
                    // Closest ratsnest layer is the same as the active layer - assume the via is being placed for
                    // other routing reasons and switch the layer
                    targetLayer = m_iface->GetBoardLayerFromPNSLayer( *pairedLayerPns );
                }
                else
                {
                    targetLayer = m_iface->GetBoardLayerFromPNSLayer( otherEndLayers.Start() );
                }
            }
        }
        else
        {
            if( currentLayer == pairTop || currentLayer == pairBottom )
            {
                // the current layer is on the defined layer pair,
                // swap to the other side
                currentLayer = pairTop;
                targetLayer = pairBottom;
            }
            else
            {
                // the current layer is not part of the current layer pair,
                // so fallback and swap to the top layer of the pair by default
                targetLayer = pairTop;
            }

            // Do not create a broken via (i.e. a via on only one copper layer)
            if( currentLayer == targetLayer )
            {
                WX_INFOBAR* infobar = frame()->GetInfoBar();
                infobar->ShowMessageFor( _( "Via needs 2 different layers." ),
                                         2000, wxICON_ERROR,
                                         WX_INFOBAR::MESSAGE_TYPE::DRC_VIOLATION );
                return 0;
            }
        }
    }

    sizes.SetViaDiameter( bds.m_ViasMinSize );
    sizes.SetViaDrill( bds.m_MinThroughDrill );

    if( bds.UseNetClassVia() || viaType == VIATYPE::MICROVIA )
    {
        PCB_VIA dummyVia( board() );
        dummyVia.SetViaType( viaType );
        dummyVia.SetLayerPair( currentLayer, targetLayer );

        if( !m_router->GetCurrentNets().empty() )
            dummyVia.SetNet( static_cast<NETINFO_ITEM*>( m_router->GetCurrentNets()[0] ) );

        DRC_CONSTRAINT constraint;

        constraint = bds.m_DRCEngine->EvalRules( VIA_DIAMETER_CONSTRAINT, &dummyVia, nullptr,
                                                 currentLayer );

        if( !constraint.IsNull() )
            sizes.SetViaDiameter( constraint.m_Value.Opt() );

        constraint = bds.m_DRCEngine->EvalRules( HOLE_SIZE_CONSTRAINT, &dummyVia, nullptr,
                                                 currentLayer );

        if( !constraint.IsNull() )
            sizes.SetViaDrill( constraint.m_Value.Opt() );
    }
    else
    {
        sizes.SetViaDiameter( bds.GetCurrentViaSize() );
        sizes.SetViaDrill( bds.GetCurrentViaDrill() );
    }

    sizes.SetViaType( viaType );
    sizes.AddLayerPair( m_iface->GetPNSLayerFromBoardLayer( currentLayer ),
                        m_iface->GetPNSLayerFromBoardLayer( targetLayer ) );

    m_router->UpdateSizes( sizes );

    if( !m_router->IsPlacingVia() )
        m_router->ToggleViaPlacement();

    if( m_router->RoutingInProgress() )
    {
        updateEndItem( aEvent );
        m_router->Move( m_endSnapPoint, m_endItem );
    }
    else
    {
        updateStartItem( aEvent );
    }

    return 0;
}


bool ROUTER_TOOL::prepareInteractive( VECTOR2D aStartPosition )
{
    PCB_EDIT_FRAME* editFrame = getEditFrame<PCB_EDIT_FRAME>();
    PCB_LAYER_ID    pcbLayer = getStartLayer( m_startItem );
    int             pnsLayer = m_iface->GetPNSLayerFromBoardLayer( pcbLayer );

    if( !::IsCopperLayer( pcbLayer ) )
    {
        editFrame->ShowInfoBarError( _( "Tracks on Copper layers only." ) );
        return false;
    }

    m_originalActiveLayer = editFrame->GetActiveLayer();
    editFrame->SetActiveLayer( pcbLayer );

    if( !getView()->IsLayerVisible( pcbLayer ) )
    {
        editFrame->GetAppearancePanel()->SetLayerVisible( pcbLayer, true );
        editFrame->GetCanvas()->Refresh();
    }

    PNS::SIZES_SETTINGS sizes( m_router->Sizes() );

    m_iface->SetStartLayerFromPCBNew( pcbLayer );

    frame()->GetBoard()->GetDesignSettings().m_TempOverrideTrackWidth = false;
    m_iface->ImportSizes( sizes, m_startItem, nullptr, aStartPosition );
    sizes.AddLayerPair( m_iface->GetPNSLayerFromBoardLayer( frame()->GetScreen()->m_Route_Layer_TOP ),
                        m_iface->GetPNSLayerFromBoardLayer( frame()->GetScreen()->m_Route_Layer_BOTTOM ) );

    m_router->UpdateSizes( sizes );

    if( m_startItem && m_startItem->Net() )
    {
        if( m_router->Mode() == PNS::PNS_MODE_ROUTE_DIFF_PAIR )
        {
            if( PNS::NET_HANDLE coupledNet = m_router->GetRuleResolver()->DpCoupledNet( m_startItem->Net() ) )
                highlightNets( true, { m_startItem->Net(), coupledNet } );
        }
        else
        {
            highlightNets( true, { m_startItem->Net() } );
        }
    }

    controls()->SetAutoPan( true );

    if( !m_router->StartRouting( m_startSnapPoint, m_startItem, pnsLayer ) )
    {
        // It would make more sense to leave the net highlighted as the higher-contrast mode
        // makes the router clearances more visible.  However, since we just started routing
        // the conversion of the screen from low contrast to high contrast is a bit jarring and
        // makes the infobar coming up less noticeable.
        highlightNets( false );

        frame()->ShowInfoBarError( m_router->FailureReason(), true,
                                   [&]()
                                   {
                                       m_router->ClearViewDecorations();
                                   } );

        controls()->SetAutoPan( false );
        return false;
    }

    m_endItem = nullptr;
    m_endSnapPoint = m_startSnapPoint;

    UpdateMessagePanel();
    frame()->UndoRedoBlock( true );

    return true;
}


bool ROUTER_TOOL::finishInteractive()
{
    m_router->StopRouting();

    m_startItem = nullptr;
    m_endItem   = nullptr;

    frame()->SetActiveLayer( m_originalActiveLayer );
    UpdateMessagePanel();
    frame()->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    controls()->SetAutoPan( false );
    controls()->ForceCursorPosition( false );
    frame()->UndoRedoBlock( false );
    highlightNets( false );

    return true;
}


void ROUTER_TOOL::performRouting( VECTOR2D aStartPosition )
{
    m_router->ClearViewDecorations();

    if( !prepareInteractive( aStartPosition ) )
        return;

    auto setCursor =
            [&]()
            {
                frame()->GetCanvas()->SetCurrentCursor( KICURSOR::PENCIL );
            };

    auto syncRouterAndFrameLayer =
            [&]()
            {
                int             pnsLayer = m_router->GetCurrentLayer();
                PCB_LAYER_ID    pcbLayer = m_iface->GetBoardLayerFromPNSLayer( pnsLayer );
                PCB_EDIT_FRAME* editFrame = getEditFrame<PCB_EDIT_FRAME>();

                editFrame->SetActiveLayer( pcbLayer );

                if( !getView()->IsLayerVisible( pcbLayer ) )
                {
                    editFrame->GetAppearancePanel()->SetLayerVisible( pcbLayer, true );
                    editFrame->GetCanvas()->Refresh();
                }
            };

    // Set initial cursor
    setCursor();

    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();

        // Don't crash if we missed an operation that canceled routing.
        if( !m_router->RoutingInProgress() )
        {
            if( evt->IsCancelInteractive() )
                m_cancelled = true;

            break;
        }

        handleCommonEvents( *evt );

        if( evt->IsMotion() )
        {
            updateEndItem( *evt );
            m_router->Move( m_endSnapPoint, m_endItem );
        }
        else if( evt->IsAction( &PCB_ACTIONS::routerUndoLastSegment )
                    || evt->IsAction( &ACTIONS::doDelete )
                    || evt->IsAction( &ACTIONS::undo ) )
        {
            if( std::optional<VECTOR2I> last = m_router->UndoLastSegment() )
            {
                getViewControls()->WarpMouseCursor( last.value(), true );
                evt->SetMousePosition( last.value() );
            }

            updateEndItem( *evt );
            m_router->Move( m_endSnapPoint, m_endItem );
        }
        else if( evt->IsAction( &PCB_ACTIONS::routerAttemptFinish ) )
        {
            if( m_toolMgr->IsContextMenuActive() )
                m_toolMgr->WarpAfterContextMenu();

            bool* autoRouted = evt->Parameter<bool*>();

            if( m_router->Finish() )
            {
                // When we're routing a group of signals automatically we want
                // to break up the undo stack every time we have to manually route
                // so the user gets nice checkpoints. Remove the APPEND_UNDO flag.
                if( autoRouted != nullptr )
                    *autoRouted = true;

                break;
            }
            else
            {
                // This acts as check if we were called by the autorouter; we don't want
                // to reset APPEND_UNDO if we're auto finishing after route-other-end
                if( autoRouted != nullptr )
                {
                    *autoRouted = false;
                    m_iface->SetCommitFlags( 0 );
                }

                // Warp the mouse so the user is at the point we managed to route to
                controls()->WarpMouseCursor( m_router->Placer()->CurrentEnd(), true, true );
            }
        }
        else if( evt->IsAction( &PCB_ACTIONS::routerContinueFromEnd ) )
        {
            bool needsAppend = m_router->Placer()->HasPlacedAnything();

            if( m_router->ContinueFromEnd( &m_startItem ) )
            {
                syncRouterAndFrameLayer();
                m_startSnapPoint = m_router->Placer()->CurrentStart();
                updateEndItem( *evt );

                // Warp the mouse to wherever we actually ended up routing to
                controls()->WarpMouseCursor( m_router->Placer()->CurrentEnd(), true, true );

                // We want the next router commit to be one undo at the UI layer
                m_iface->SetCommitFlags( needsAppend ? APPEND_UNDO : 0 );
            }
            else
            {
                frame()->ShowInfoBarError( m_router->FailureReason(), true );
            }
        }
        else if( evt->IsClick( BUT_LEFT )
                     || evt->IsDrag( BUT_LEFT )
                     || evt->IsAction( &PCB_ACTIONS::routeSingleTrack ) )
        {
            updateEndItem( *evt );
            bool needLayerSwitch = m_router->IsPlacingVia();
            bool forceFinish = evt->Modifier( MD_SHIFT );
            bool forceCommit = false;

            if( m_router->FixRoute( m_endSnapPoint, m_endItem, false, forceCommit ) )
                break;

            if( needLayerSwitch )
            {
                switchLayerOnViaPlacement();
            }
            else
            {
                updateSizesAfterRouterEvent( m_router->GetCurrentLayer(), m_endSnapPoint );
            }

            // Synchronize the indicated layer
            syncRouterAndFrameLayer();

            updateEndItem( *evt );
            m_router->Move( m_endSnapPoint, m_endItem );
            m_startItem = nullptr;
        }
        else if( evt->IsAction( &ACT_SwitchPosture ) )
        {
            m_router->FlipPosture();
            updateEndItem( *evt );
            m_router->Move( m_endSnapPoint, m_endItem );        // refresh
        }
        else if( evt->IsAction( &PCB_ACTIONS::properties ) )
        {
            frame()->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
            controls()->SetAutoPan( false );
            {
                m_toolMgr->RunAction( ACT_CustomTrackWidth );
            }
            controls()->SetAutoPan( true );
            setCursor();
            UpdateMessagePanel();
        }
        else if( evt->IsAction( &ACTIONS::finishInteractive ) || evt->IsDblClick( BUT_LEFT )  )
        {
            // Stop current routing:
            bool forceFinish = true;
            bool forceCommit = false;

            m_router->FixRoute( m_endSnapPoint, m_endItem, forceFinish, forceCommit );
            break;
        }
        else if( evt->IsCancelInteractive() || evt->IsActivate()
                 || evt->IsAction( &PCB_ACTIONS::routerInlineDrag ) )
        {
            if( evt->IsCancelInteractive() && !m_router->RoutingInProgress() )
                m_cancelled = true;

            if( evt->IsActivate() && !evt->IsMoveTool() )
                m_cancelled = true;

            break;
        }
        else if( evt->IsUndoRedo() )
        {
            // We're in an UndoRedoBlock.  If we get here, something's broken.
            wxFAIL;
            break;
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu->ShowContextMenu( selection() );
        }
        // TODO: It'd be nice to be able to say "don't allow any non-trivial editing actions",
        // but we don't at present have that, so we just knock out some of the egregious ones.
        else if( ZONE_FILLER_TOOL::IsZoneFillAction( evt ) )
        {
            wxBell();
        }
        else
        {
            evt->SetPassEvent();
        }
    }

    m_router->CommitRouting();
    // Reset to normal for next route
    m_iface->SetCommitFlags( 0 );

    finishInteractive();
}


int ROUTER_TOOL::DpDimensionsDialog( const TOOL_EVENT& aEvent )
{
    PNS::SIZES_SETTINGS sizes = m_router->Sizes();
    DIALOG_PNS_DIFF_PAIR_DIMENSIONS settingsDlg( frame(), sizes );

    if( settingsDlg.ShowModal() == wxID_OK )
    {
        m_router->UpdateSizes( sizes );
        m_savedSizes = sizes;

        BOARD_DESIGN_SETTINGS& bds = frame()->GetBoard()->GetDesignSettings();
        bds.SetCustomDiffPairWidth( sizes.DiffPairWidth() );
        bds.SetCustomDiffPairGap( sizes.DiffPairGap() );
        bds.SetCustomDiffPairViaGap( sizes.DiffPairViaGap() );
    }

    return 0;
}


int ROUTER_TOOL::SettingsDialog( const TOOL_EVENT& aEvent )
{
    DIALOG_PNS_SETTINGS settingsDlg( frame(), m_router->Settings() );

    settingsDlg.ShowModal();

    UpdateMessagePanel();

    return 0;
}


int ROUTER_TOOL::ChangeRouterMode( const TOOL_EVENT& aEvent )
{
    PNS::PNS_MODE mode = aEvent.Parameter<PNS::PNS_MODE>();
    PNS::ROUTING_SETTINGS& settings = m_router->Settings();

    settings.SetMode( mode );
    UpdateMessagePanel();

    return 0;
}


int ROUTER_TOOL::CycleRouterMode( const TOOL_EVENT& aEvent )
{
    PNS::ROUTING_SETTINGS& settings = m_router->Settings();
    PNS::PNS_MODE mode = settings.Mode();

    switch( mode )
    {
    case PNS::RM_MarkObstacles: mode = PNS::RM_Shove;         break;
    case PNS::RM_Shove:         mode = PNS::RM_Walkaround;    break;
    case PNS::RM_Walkaround:    mode = PNS::RM_MarkObstacles; break;
    }

    settings.SetMode( mode );
    UpdateMessagePanel();

    return 0;
}


PNS::PNS_MODE ROUTER_TOOL::GetRouterMode()
{
    return m_router->Settings().Mode();
}


bool ROUTER_TOOL::RoutingInProgress()
{
    return m_router->RoutingInProgress();
}


void ROUTER_TOOL::breakTrack()
{
    if( !m_startItem )
        return;

    if( m_startItem->OfKind( PNS::ITEM::SEGMENT_T | PNS::ITEM::ARC_T ) )
        m_router->BreakSegmentOrArc( m_startItem, m_startSnapPoint );
}


int ROUTER_TOOL::RouteSelected( const TOOL_EVENT& aEvent )
{
    PNS::ROUTER_MODE mode = aEvent.Parameter<PNS::ROUTER_MODE>();
    PCB_EDIT_FRAME*  frame = getEditFrame<PCB_EDIT_FRAME>();
    VIEW_CONTROLS*   controls = getViewControls();
    PCB_LAYER_ID     originalLayer = frame->GetActiveLayer();
    bool             autoRoute = aEvent.Matches( PCB_ACTIONS::routerAutorouteSelected.MakeEvent() );
    bool             otherEnd  = aEvent.Matches( PCB_ACTIONS::routerRouteSelectedFromEnd.MakeEvent() );

    if( m_router->RoutingInProgress() )
        return 0;

    // Save selection then clear it for interactive routing
    PCB_SELECTION selection = m_toolMgr->GetTool<PCB_SELECTION_TOOL>()->GetSelection();

    if( selection.Size() == 0 )
        return 0;

    m_toolMgr->RunAction( ACTIONS::selectionClear );

    TOOL_EVENT pushedEvent = aEvent;
    frame->PushTool( aEvent );

    auto setCursor =
            [&]()
            {
                frame->GetCanvas()->SetCurrentCursor( KICURSOR::PENCIL );
            };

    Activate();
    // Must be done after Activate() so that it gets set into the correct context
    controls->ShowCursor( true );
    controls->ForceCursorPosition( false );
    // Set initial cursor
    setCursor();

    // Get all connected board items, adding pads for any footprints selected
    std::vector<BOARD_CONNECTED_ITEM*> itemList;

    for( EDA_ITEM* item : selection.GetItemsSortedBySelectionOrder() )
    {
        if( item->Type() == PCB_FOOTPRINT_T )
        {
            for( PAD* pad : static_cast<FOOTPRINT*>( item )->Pads() )
                itemList.push_back( pad );
        }
        else if( dynamic_cast<BOARD_CONNECTED_ITEM*>( item ) != nullptr )
        {
            itemList.push_back( static_cast<BOARD_CONNECTED_ITEM*>( item ) );
        }
    }

    std::shared_ptr<CONNECTIVITY_DATA> connectivity = frame->GetBoard()->GetConnectivity();

    // For putting sequential tracks that successfully autoroute into one undo commit
    bool groupStart = true;

    for( BOARD_CONNECTED_ITEM* item : itemList )
    {
        // This code is similar to GetRatsnestForPad() but it only adds the anchor for
        // the side of the connectivity on this pad. It also checks for ratsnest points
        // inside the pad (like a trace end) and counts them.
        RN_NET* net = connectivity->GetRatsnestForNet( item->GetNetCode() );

        if( !net )
            continue;

        std::vector<std::shared_ptr<const CN_ANCHOR>> anchors;

        for( const CN_EDGE& edge : net->GetEdges() )
        {
            std::shared_ptr<const CN_ANCHOR> target = edge.GetTargetNode();
            std::shared_ptr<const CN_ANCHOR> source = edge.GetSourceNode();

            if( !source || source->Dirty() || !target || target->Dirty() )
                continue;

            if( source->Parent() == item )
                anchors.push_back( source );
            else if( target->Parent() == item )
                anchors.push_back( target );
        }

        // Route them
        for( std::shared_ptr<const CN_ANCHOR> anchor : anchors )
        {
            if( !anchor->Valid() )
                continue;

            // Try to return to the original layer as indicating the user's preferred
            // layer for autorouting tracks. The layer can be changed by the user to
            // finish tracks that can't complete automatically, but should be changed
            // back after.
            if( frame->GetActiveLayer() != originalLayer )
                frame->SetActiveLayer( originalLayer );

            VECTOR2I ignore;
            m_startItem = m_router->GetWorld()->FindItemByParent( anchor->Parent() );
            m_startSnapPoint = anchor->Pos();
            m_router->SetMode( mode );

            // Prime the interactive routing to attempt finish if we are autorouting
            bool autoRouted = false;

            if( autoRoute )
                m_toolMgr->PostAction( PCB_ACTIONS::routerAttemptFinish, &autoRouted );
            else if( otherEnd )
                m_toolMgr->PostAction( PCB_ACTIONS::routerContinueFromEnd );

            // We want autorouted tracks to all be in one undo group except for
            // any tracks that need to be manually finished.
            // The undo appending for manually finished tracks is handled in peformRouting()
            if( groupStart )
                groupStart = false;
            else
                m_iface->SetCommitFlags( APPEND_UNDO );

            // Start interactive routing. Will automatically finish if possible.
            performRouting( VECTOR2D() );

            // Route didn't complete automatically, need to a new undo commit
            // for the next line so those can group as far as they autoroute
            if( !autoRouted )
                groupStart = true;
        }
    }

    m_iface->SetCommitFlags( 0 );
    frame->PopTool( pushedEvent );
    return 0;
}


int ROUTER_TOOL::MainLoop( const TOOL_EVENT& aEvent )
{
    if( m_inRouterTool )
        return 0;

    REENTRANCY_GUARD guard( &m_inRouterTool );

    PNS::ROUTER_MODE mode = aEvent.Parameter<PNS::ROUTER_MODE>();
    PCB_EDIT_FRAME*  frame = getEditFrame<PCB_EDIT_FRAME>();
    VIEW_CONTROLS*   controls = getViewControls();

    if( m_router->RoutingInProgress() )
    {
        if( m_router->Mode() == mode )
            return 0;
        else
            m_router->StopRouting();
    }

    // Deselect all items
    m_toolMgr->RunAction( ACTIONS::selectionClear );

    TOOL_EVENT pushedEvent = aEvent;
    frame->PushTool( aEvent );

    auto setCursor =
            [&]()
            {
                frame->GetCanvas()->SetCurrentCursor( KICURSOR::PENCIL );
            };

    Activate();
    // Must be done after Activate() so that it gets set into the correct context
    controls->ShowCursor( true );
    controls->ForceCursorPosition( false );
    // Set initial cursor
    setCursor();

    m_router->SetMode( mode );
    m_cancelled = false;

    if( aEvent.HasPosition() )
        m_toolMgr->PrimeTool( aEvent.Position() );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        if( !evt->IsDrag() )
            setCursor();

        if( evt->IsCancelInteractive() )
        {
            frame->PopTool( pushedEvent );
            break;
        }
        else if( evt->IsActivate() )
        {
            if( evt->IsMoveTool() || evt->IsEditorTool() )
            {
                // leave ourselves on the stack so we come back after the move
                break;
            }
            else
            {
                frame->PopTool( pushedEvent );
                break;
            }
        }
        else if( evt->Action() == TA_UNDO_REDO_PRE )
        {
            m_router->ClearWorld();
        }
        else if( evt->Action() == TA_UNDO_REDO_POST || evt->Action() == TA_MODEL_CHANGE )
        {
            m_router->SyncWorld();
        }
        else if( evt->IsMotion() )
        {
            updateStartItem( *evt );
        }
        else if( evt->IsAction( &PCB_ACTIONS::dragFreeAngle ) )
        {
            updateStartItem( *evt, true );
            performDragging( PNS::DM_ANY | PNS::DM_FREE_ANGLE );
        }
        else if( evt->IsAction( &PCB_ACTIONS::drag45Degree ) )
        {
            updateStartItem( *evt, true );
            performDragging( PNS::DM_ANY );
        }
        else if( evt->IsAction( &PCB_ACTIONS::breakTrack ) )
        {
            updateStartItem( *evt, true );
            breakTrack( );
            evt->SetPassEvent( false );
        }
        else if( evt->IsClick( BUT_LEFT )
              || evt->IsAction( &PCB_ACTIONS::routeSingleTrack )
              || evt->IsAction( &PCB_ACTIONS::routeDiffPair ) )
        {
            updateStartItem( *evt );

            if( evt->HasPosition() )
                performRouting( evt->Position() );
        }
        else if( evt->IsAction( &ACT_PlaceThroughVia ) )
        {
            m_toolMgr->RunAction( PCB_ACTIONS::layerToggle );
        }
        else if( evt->IsAction( &PCB_ACTIONS::layerChanged ) )
        {
            m_router->SwitchLayer( m_iface->GetPNSLayerFromBoardLayer( frame->GetActiveLayer() ) );
            updateStartItem( *evt );
            updateSizesAfterRouterEvent( m_iface->GetPNSLayerFromBoardLayer( frame->GetActiveLayer() ), m_startSnapPoint );
        }
        else if( evt->IsKeyPressed() )
        {
            // wxWidgets fails to correctly translate shifted keycodes on the wxEVT_CHAR_HOOK
            // event so we need to process the wxEVT_CHAR event that will follow as long as we
            // pass the event.
            evt->SetPassEvent();
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu->ShowContextMenu( selection() );
        }
        else
        {
            evt->SetPassEvent();
        }

        if( m_cancelled )
        {
            frame->PopTool( pushedEvent );
            break;
        }
    }

    // Store routing settings till the next invocation
    m_savedSizes = m_router->Sizes();
    m_router->ClearViewDecorations();

    return 0;
}


void ROUTER_TOOL::performDragging( int aMode )
{
    m_router->ClearViewDecorations();

    view()->ClearPreview();
    view()->InitPreview();

    VIEW_CONTROLS* ctls = getViewControls();

    if( m_startItem && m_startItem->IsLocked() )
    {
        KIDIALOG dlg( frame(), _( "The selected item is locked." ), _( "Confirmation" ),
                      wxOK | wxCANCEL | wxICON_WARNING );
        dlg.SetOKLabel( _( "Drag Anyway" ) );
        dlg.DoNotShowCheckbox( __FILE__, __LINE__ );

        if( dlg.ShowModal() == wxID_CANCEL )
            return;
    }

    // We don't support dragging arcs inside the PNS right now
    if( m_startItem && m_startItem->Kind() == PNS::ITEM::ARC_T )
    {
        if( m_router->RoutingInProgress() )
            m_router->StopRouting();

        m_startItem = nullptr;

        m_gridHelper->SetAuxAxes( false );
        ctls->ForceCursorPosition( false );
        highlightNets( false );

        m_cancelled = true;

        m_toolMgr->PostAction( PCB_ACTIONS::drag45Degree );

        return;
    }

    bool dragStarted = m_router->StartDragging( m_startSnapPoint, m_startItem, aMode );

    if( !dragStarted )
        return;

    if( m_startItem && m_startItem->Net() )
        highlightNets( true, { m_startItem->Net() } );

    ctls->SetAutoPan( true );
    m_gridHelper->SetAuxAxes( true, m_startSnapPoint );
    frame()->UndoRedoBlock( true );

    while( TOOL_EVENT* evt = Wait() )
    {
        ctls->ForceCursorPosition( false );

        if( evt->IsMotion() )
        {
            updateEndItem( *evt );
            m_router->Move( m_endSnapPoint, m_endItem );

            if( PNS::DRAG_ALGO* dragger = m_router->GetDragger() )
            {
                bool dragStatus;

                if( dragger->GetForceMarkObstaclesMode( &dragStatus ) )
                {
                    view()->ClearPreview();

                    if( !dragStatus )
                    {
                        wxString hint;
                        hint.Printf( _( "(%s to commit anyway.)" ),
                                    KeyNameFromKeyCode( MD_CTRL + PSEUDO_WXK_CLICK ) );

                        ROUTER_STATUS_VIEW_ITEM* statusItem = new ROUTER_STATUS_VIEW_ITEM();
                        statusItem->SetMessage( _( "Track violates DRC." ) );
                        statusItem->SetHint( hint );
                        statusItem->SetPosition( frame()->GetToolManager()->GetMousePosition() );
                        view()->AddToPreview( statusItem );
                    }
                }
            }
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            bool forceFinish = false;
            bool forceCommit = evt->Modifier( MD_CTRL );

            if( m_router->FixRoute( m_endSnapPoint, m_endItem, forceFinish, forceCommit ) )
                break;
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu->ShowContextMenu( selection() );
        }
        else if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            if( evt->IsCancelInteractive() && !m_startItem )
                m_cancelled = true;

            if( evt->IsActivate() && !evt->IsMoveTool() )
                m_cancelled = true;

            break;
        }
        else if( evt->IsUndoRedo() )
        {
            // We're in an UndoRedoBlock.  If we get here, something's broken.
            wxFAIL;
            break;
        }
        else if( evt->Category() == TC_COMMAND )
        {
            // TODO: It'd be nice to be able to say "don't allow any non-trivial editing actions",
            // but we don't at present have that, so we just knock out some of the egregious ones.
            if( evt->IsAction( &ACTIONS::cut )
                || evt->IsAction( &ACTIONS::copy )
                || evt->IsAction( &ACTIONS::paste )
                || evt->IsAction( &ACTIONS::pasteSpecial )
                || ZONE_FILLER_TOOL::IsZoneFillAction( evt ) )
            {
                wxBell();
            }
            // treat an undo as an escape
            else if( evt->IsAction( &ACTIONS::undo ) )
            {
                if( m_startItem )
                    break;
                else
                    wxBell();
            }
            else
            {
                evt->SetPassEvent();
            }
        }
        else
        {
            evt->SetPassEvent();
        }

        handleCommonEvents( *evt );
    }

    view()->ClearPreview();
    view()->ShowPreview( false );

    if( m_router->RoutingInProgress() )
        m_router->StopRouting();

    m_startItem = nullptr;

    m_gridHelper->SetAuxAxes( false );
    frame()->UndoRedoBlock( false );
    ctls->SetAutoPan( false );
    ctls->ForceCursorPosition( false );
    highlightNets( false );
}


void ROUTER_TOOL::NeighboringSegmentFilter( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector,
                                            PCB_SELECTION_TOOL* aSelTool )
{
    /*
     * If the collection contains a trivial line corner (two connected segments)
     * or a non-fanout-via (a via with no more than two connected segments), then
     * trim the collection down to a single item (which one won't matter since
     * they're all connected).
     */

    // First make sure we've got something that *might* match.
    int vias = aCollector.CountType( PCB_VIA_T );
    int traces = aCollector.CountType( PCB_TRACE_T );
    int arcs = aCollector.CountType( PCB_ARC_T );

    // We eliminate arcs because they are not supported in the inline drag code.
    if( arcs > 0 )
        return;

    // We need to have at least 1 via or track
    if( vias + traces == 0 )
        return;

    // We cannot drag more than one via at a time
    if( vias > 1 )
        return;

    // We cannot drag more than two track segments at a time
    if( traces > 2 )
        return;

    // Fetch first PCB_TRACK (via or trace) as our reference
    PCB_TRACK* reference = nullptr;

    for( int i = 0; !reference && i < aCollector.GetCount(); i++ )
        reference = dynamic_cast<PCB_TRACK*>( aCollector[i] );

    // This should never happen, but just in case...
    if( !reference )
        return;

    int refNet = reference->GetNetCode();

    VECTOR2I       refPoint( aPt.x, aPt.y );
    EDA_ITEM_FLAGS flags = reference->IsPointOnEnds( refPoint, -1 );

    if( flags & STARTPOINT )
        refPoint = reference->GetStart();
    else if( flags & ENDPOINT )
        refPoint = reference->GetEnd();

    // Check all items to ensure that any TRACKs are co-terminus with the reference and on
    // the same net.
    for( int i = 0; i < aCollector.GetCount(); i++ )
    {
        PCB_TRACK* neighbor = dynamic_cast<PCB_TRACK*>( aCollector[i] );

        if( neighbor && neighbor != reference )
        {
            if( neighbor->GetNetCode() != refNet )
                return;

            if( neighbor->GetStart() != refPoint && neighbor->GetEnd() != refPoint )
                return;
        }
    }

    // Selection meets criteria; trim it to the reference item.
    aCollector.Empty();
    aCollector.Append( reference );
}


bool ROUTER_TOOL::CanInlineDrag( int aDragMode )
{
    m_toolMgr->RunAction<CLIENT_SELECTION_FILTER>( ACTIONS::selectionCursor, NeighboringSegmentFilter );
    const PCB_SELECTION& selection = m_toolMgr->GetTool<PCB_SELECTION_TOOL>()->GetSelection();

    if( selection.Size() == 1 )
    {
        return selection.Front()->IsType( GENERAL_COLLECTOR::DraggableItems );
    }
    else if( selection.CountType( PCB_FOOTPRINT_T ) == selection.Size() )
    {
        // Footprints cannot be dragged freely.
        return !( aDragMode & PNS::DM_FREE_ANGLE );
    }
    else if( selection.CountType( PCB_TRACE_T ) == selection.Size() )
    {
        return true;
    }

    return false;
}


void ROUTER_TOOL::restoreSelection( const PCB_SELECTION& aOriginalSelection )
{
    EDA_ITEMS selItems;
    std::copy( aOriginalSelection.Items().begin(), aOriginalSelection.Items().end(), std::back_inserter( selItems ) );
    m_toolMgr->RunAction<EDA_ITEMS*>( ACTIONS::selectItems, &selItems );
}


int ROUTER_TOOL::InlineDrag( const TOOL_EVENT& aEvent )
{
    const PCB_SELECTION selection = m_toolMgr->GetTool<PCB_SELECTION_TOOL>()->GetSelection();

    if( selection.Empty() )
        m_toolMgr->RunAction<CLIENT_SELECTION_FILTER>( ACTIONS::selectionCursor, NeighboringSegmentFilter );

    if( selection.Empty() || !selection.Front()->IsBOARD_ITEM() )
        return 0;

    // selection gets cleared in the next action, we need a copy of the selected items.
    std::deque<EDA_ITEM*> selectedItems = selection.GetItems();

    BOARD_ITEM* item = static_cast<BOARD_ITEM*>( selection.Front() );

    if( item->Type() != PCB_TRACE_T
         && item->Type() != PCB_VIA_T
         && item->Type() != PCB_ARC_T
         && item->Type() != PCB_FOOTPRINT_T )
    {
        return 0;
    }

    std::set<FOOTPRINT*> footprints;

    if( item->Type() == PCB_FOOTPRINT_T )
        footprints.insert( static_cast<FOOTPRINT*>( item ) );

    // We can drag multiple footprints, but not a grab-bag of items
    if( selection.Size() > 1 && item->Type() == PCB_FOOTPRINT_T )
    {
        for( int idx = 1; idx < selection.Size(); ++idx )
        {
            if( !selection.GetItem( idx )->IsBOARD_ITEM() )
                return 0;

            if( static_cast<BOARD_ITEM*>( selection.GetItem( idx ) )->Type() != PCB_FOOTPRINT_T )
                return 0;

            footprints.insert( static_cast<FOOTPRINT*>( selection.GetItem( idx ) ) );
        }
    }

    // If we overrode locks, we want to clear the flag from the source item before SyncWorld is
    // called so that virtual vias are not generated for the (now unlocked) track segment.  Note in
    // this case the lock can't be reliably re-applied, because there is no guarantee that the end
    // state of the drag results in the same number of segments so it's not clear which segment to
    // apply the lock state to.
    bool wasLocked = false;

    if( item->IsLocked() )
    {
        wasLocked = true;
        item->SetLocked( false );
    }

    m_toolMgr->RunAction( ACTIONS::selectionClear );

    TOOL_EVENT pushedEvent = aEvent;
    frame()->PushTool( aEvent );
    Activate();

    m_startItem = nullptr;

    PNS::ITEM*    startItem = nullptr;
    PNS::ITEM_SET itemsToDrag;

    bool showCourtyardConflicts = frame()->GetPcbNewSettings()->m_ShowCourtyardCollisions;

    std::shared_ptr<DRC_ENGINE>         drcEngine = m_toolMgr->GetTool<DRC_TOOL>()->GetDRCEngine();
    DRC_INTERACTIVE_COURTYARD_CLEARANCE courtyardClearanceDRC( drcEngine );

    std::shared_ptr<CONNECTIVITY_DATA>  connectivityData = board()->GetConnectivity();
    std::vector<BOARD_ITEM*>            dynamicItems;
    std::unique_ptr<CONNECTIVITY_DATA>  dynamicData = nullptr;
    VECTOR2I                            lastOffset;
    std::vector<PNS::ITEM*>             leaderSegments;
    bool                                singleFootprintDrag = false;

    if( !footprints.empty() )
    {
        if( footprints.size() == 1 )
            singleFootprintDrag = true;

        if( showCourtyardConflicts )
            courtyardClearanceDRC.Init( board() );

        for( FOOTPRINT* footprint : footprints )
        {
            for( PAD* pad : footprint->Pads() )
            {
                PNS::ITEM* solid = m_router->GetWorld()->FindItemByParent( pad );

                if( solid )
                    itemsToDrag.Add( solid );

                if( pad->GetLocalRatsnestVisible() || displayOptions().m_ShowModuleRatsnest )
                {
                    if( connectivityData->GetRatsnestForPad( pad ).size() > 0 )
                        dynamicItems.push_back( pad );
                }
            }

            for( ZONE* zone : footprint->Zones() )
            {
                for( PNS::ITEM* solid : m_router->GetWorld()->FindItemsByParent( zone ) )
                    itemsToDrag.Add( solid );
            }

            for( BOARD_ITEM* shape : footprint->GraphicalItems() )
            {
                if( shape->GetLayer() == Edge_Cuts
                    || shape->GetLayer() == Margin
                    || IsCopperLayer( shape->GetLayer() ) )
                {
                    for( PNS::ITEM* solid : m_router->GetWorld()->FindItemsByParent( shape ) )
                        itemsToDrag.Add( solid );
                }
            }

            if( showCourtyardConflicts )
                courtyardClearanceDRC.m_FpInMove.push_back( footprint );
        }

        dynamicData = std::make_unique<CONNECTIVITY_DATA>( board()->GetConnectivity(),
                                                           dynamicItems, true );
        connectivityData->BlockRatsnestItems( dynamicItems );
    }
    else
    {
        for( const EDA_ITEM* selItem : selectedItems )
        {
            if( !selItem->IsBOARD_ITEM() )
                continue;

            const BOARD_ITEM* boardItem = static_cast<const BOARD_ITEM*>( selItem );
            PNS::ITEM*        pnsItem = m_router->GetWorld()->FindItemByParent( boardItem );

            if( !pnsItem )
                continue;

            if( pnsItem->OfKind( PNS::ITEM::SEGMENT_T )
                || pnsItem->OfKind( PNS::ITEM::VIA_T )
                || pnsItem->OfKind( PNS::ITEM::ARC_T ) )
            {
                itemsToDrag.Add( pnsItem );
            }
        }
    }

    GAL*     gal = m_toolMgr->GetView()->GetGAL();
    VECTOR2I p0 = GetClampedCoords( controls()->GetCursorPosition( false ), COORDS_PADDING );
    VECTOR2I p = p0;

    m_gridHelper->SetUseGrid( gal->GetGridSnapping() && !aEvent.DisableGridSnapping()  );
    m_gridHelper->SetSnap( !aEvent.Modifier( MD_SHIFT ) );

    if( itemsToDrag.Count() >= 1 )
    {
        // Snap to closest item
        int         layer = m_iface->GetPNSLayerFromBoardLayer( m_originalActiveLayer );
        PNS::ITEM*  closestItem = nullptr;
        SEG::ecoord closestDistSq = std::numeric_limits<SEG::ecoord>::max();

        for( PNS::ITEM* pitem : itemsToDrag.Items() )
        {
            SEG::ecoord distSq = pitem->Shape( layer )->SquaredDistance( p0, 0 );

            if( distSq < closestDistSq )
            {
                closestDistSq = distSq;
                closestItem = pitem;
            }
        }

        if( closestItem )
        {
            p = snapToItem( closestItem, p0 );

            m_startItem = closestItem;

            if( closestItem->Net() )
                highlightNets( true, { closestItem->Net() } );
        }
    }

    if( !footprints.empty() && singleFootprintDrag )
    {
        FOOTPRINT* footprint = static_cast<FOOTPRINT*>( item );

        // The mouse is going to be moved on grid before dragging begins.
        VECTOR2I             tweakedMousePos;
        PCB_BASE_EDIT_FRAME* editFrame = getEditFrame<PCB_BASE_EDIT_FRAME>();

        // Check if user wants to warp the mouse to origin of moved object

        if( editFrame->GetMoveWarpsCursor() )
            tweakedMousePos = footprint->GetPosition(); // Use footprint anchor to warp mouse
        else
            tweakedMousePos = GetClampedCoords( controls()->GetCursorPosition(),
                                                COORDS_PADDING ); // Just use current mouse pos

        // We tweak the mouse position using the value from above, and then use that as the
        // start position to prevent the footprint from jumping when we start dragging.
        // First we move the visual cross hair cursor...
        controls()->ForceCursorPosition( true, tweakedMousePos );
        controls()->SetCursorPosition( tweakedMousePos ); // ...then the mouse pointer

        // Now that the mouse is in the right position, get a copy of the position to use later
        p = controls()->GetCursorPosition();
    }

    int dragMode = aEvent.Parameter<int> ();

    bool dragStarted = m_router->StartDragging( p, itemsToDrag, dragMode );

    if( !dragStarted )
    {
        if( wasLocked )
            item->SetLocked( true );

        if( !footprints.empty() )
            connectivityData->ClearLocalRatsnest();

        // Clear temporary COURTYARD_CONFLICT flag and ensure the conflict shadow is cleared
        courtyardClearanceDRC.ClearConflicts( getView() );

        restoreSelection( selection );
        controls()->ForceCursorPosition( false );
        frame()->PopTool( pushedEvent );
        highlightNets( false );
        return 0;
    }

    m_gridHelper->SetAuxAxes( true, p );
    controls()->ShowCursor( true );
    controls()->SetAutoPan( true );
    frame()->UndoRedoBlock( true );

    view()->ClearPreview();
    view()->InitPreview();

    auto setCursor =
            [&]()
            {
                frame()->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
            };

    // Set initial cursor
    setCursor();

    // Set the initial visible area
    BOX2D viewAreaD = getView()->GetGAL()->GetVisibleWorldExtents();
    m_router->SetVisibleViewArea( BOX2ISafe( viewAreaD ) );

    // Send an initial movement to prime the collision detection
    m_router->Move( p, nullptr );

    bool hasMouseMoved = false;
    bool hasMultidragCancelled = false;

    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();

        if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            if( wasLocked )
                item->SetLocked( true );

            hasMultidragCancelled = true;

            break;
        }
        else if( evt->IsMotion() || evt->IsDrag( BUT_LEFT ) )
        {
            hasMouseMoved = true;
            updateEndItem( *evt );
            m_router->Move( m_endSnapPoint, m_endItem );

            view()->ClearPreview();

            if( !footprints.empty() )
            {
                VECTOR2I offset = m_endSnapPoint - p;
                BOARD_ITEM* previewItem;

                for( FOOTPRINT* footprint : footprints )
                {
                    for( BOARD_ITEM* drawing : footprint->GraphicalItems() )
                    {
                        previewItem = static_cast<BOARD_ITEM*>( drawing->Clone() );
                        previewItem->Move( offset );

                        view()->AddToPreview( previewItem );
                        view()->Hide( drawing, true );
                    }

                    for( PAD* pad : footprint->Pads() )
                    {
                        if( ( pad->GetLayerSet() & LSET::AllCuMask() ).none()
                            && pad->GetDrillSize().x == 0 )
                        {
                            previewItem = static_cast<BOARD_ITEM*>( pad->Clone() );
                            previewItem->Move( offset );

                            view()->AddToPreview( previewItem );
                        }
                        else
                        {
                            // Pads with copper or holes are handled by the router
                        }

                        view()->Hide( pad, true );
                    }

                    previewItem = static_cast<BOARD_ITEM*>( footprint->Reference().Clone() );
                    previewItem->Move( offset );
                    view()->AddToPreview( previewItem );
                    view()->Hide( &footprint->Reference() );

                    previewItem = static_cast<BOARD_ITEM*>( footprint->Value().Clone() );
                    previewItem->Move( offset );
                    view()->AddToPreview( previewItem );
                    view()->Hide( &footprint->Value() );

                    if( showCourtyardConflicts )
                        footprint->Move( offset );
                }

                if( showCourtyardConflicts )
                {
                    courtyardClearanceDRC.Run();
                    courtyardClearanceDRC.UpdateConflicts( getView(), false );

                    for( FOOTPRINT* footprint : footprints )
                        footprint->Move( -offset );
                }

                // Update ratsnest
                dynamicData->Move( offset - lastOffset );
                lastOffset = offset;
                connectivityData->ComputeLocalRatsnest( dynamicItems, dynamicData.get(), offset );
            }

            if( PNS::DRAG_ALGO* dragger = m_router->GetDragger() )
            {
                bool dragStatus;

                if( dragger->GetForceMarkObstaclesMode( &dragStatus ) )
                {
                    if( !dragStatus )
                    {
                        wxString hint;
                        hint.Printf( _( "(%s to commit anyway.)" ),
                                    KeyNameFromKeyCode( MD_CTRL + PSEUDO_WXK_CLICK ) );

                        ROUTER_STATUS_VIEW_ITEM* statusItem = new ROUTER_STATUS_VIEW_ITEM();
                        statusItem->SetMessage( _( "Track violates DRC." ) );
                        statusItem->SetHint( hint );
                        statusItem->SetPosition( frame()->GetToolManager()->GetMousePosition() );
                        view()->AddToPreview( statusItem );
                    }
                }
            }
        }
        else if( hasMouseMoved && ( evt->IsMouseUp( BUT_LEFT ) || evt->IsClick( BUT_LEFT ) ) )
        {
            bool forceFinish = false;
            bool forceCommit = evt->Modifier( MD_CTRL );

            updateEndItem( *evt );
            m_router->FixRoute( m_endSnapPoint, m_endItem, forceFinish, forceCommit );
            leaderSegments = m_router->GetLastCommittedLeaderSegments();

            break;
        }
        else if( evt->IsUndoRedo() )
        {
            // We're in an UndoRedoBlock.  If we get here, something's broken.
            wxFAIL;
            break;
        }
        else if( evt->Category() == TC_COMMAND )
        {
            // TODO: It'd be nice to be able to say "don't allow any non-trivial editing actions",
            // but we don't at present have that, so we just knock out some of the egregious ones.
            if( evt->IsAction( &ACTIONS::cut )
                || evt->IsAction( &ACTIONS::copy )
                || evt->IsAction( &ACTIONS::paste )
                || evt->IsAction( &ACTIONS::pasteSpecial )
                || ZONE_FILLER_TOOL::IsZoneFillAction( evt ) )
            {
                wxBell();
            }
            // treat an undo as an escape
            else if( evt->IsAction( &ACTIONS::undo ) )
            {
                if( wasLocked )
                    item->SetLocked( true );

                break;
            }
            else
            {
                evt->SetPassEvent();
            }
        }
        else
        {
            evt->SetPassEvent();
        }

        handleCommonEvents( *evt );
    }

    if( !footprints.empty() )
    {
        for( FOOTPRINT* footprint : footprints )
        {
            for( BOARD_ITEM* drawing : footprint->GraphicalItems() )
                view()->Hide( drawing, false );

            view()->Hide( &footprint->Reference(), false );
            view()->Hide( &footprint->Value(), false );

            for( PAD* pad : footprint->Pads() )
                view()->Hide( pad, false );
        }

        view()->ClearPreview();
        view()->ShowPreview( false );

        connectivityData->ClearLocalRatsnest();
    }

    // Clear temporary COURTYARD_CONFLICT flag and ensure the conflict shadow is cleared
    courtyardClearanceDRC.ClearConflicts( getView() );

    if( m_router->RoutingInProgress() )
        m_router->StopRouting();


    if( itemsToDrag.Size() && hasMultidragCancelled )
    {
        restoreSelection( selection );
    }
    else if( leaderSegments.size() )
    {
        std::vector<EDA_ITEM*> newItems;

        for( auto lseg : leaderSegments )
            newItems.push_back( lseg->Parent() );

        m_toolMgr->RunAction<EDA_ITEMS*>( ACTIONS::selectItems, &newItems );
    }

    m_gridHelper->SetAuxAxes( false );
    controls()->SetAutoPan( false );
    controls()->ForceCursorPosition( false );
    frame()->UndoRedoBlock( false );
    frame()->PopTool( pushedEvent );
    highlightNets( false );
    view()->ClearPreview();
    view()->ShowPreview( false );

    return 0;
}


int ROUTER_TOOL::InlineBreakTrack( const TOOL_EVENT& aEvent )
{
    const SELECTION& selection = m_toolMgr->GetTool<PCB_SELECTION_TOOL>()->GetSelection();

    if( selection.Size() != 1 )
        return 0;

    const BOARD_CONNECTED_ITEM* item =
            static_cast<const BOARD_CONNECTED_ITEM*>( selection.Front() );

    if( item->Type() != PCB_TRACE_T && item->Type() != PCB_ARC_T )
        return 0;

    m_toolMgr->RunAction( ACTIONS::selectionClear );

    Activate();

    m_startItem = m_router->GetWorld()->FindItemByParent( item );

    TOOL_MANAGER* toolManager = frame()->GetToolManager();
    GAL*          gal = toolManager->GetView()->GetGAL();

    m_gridHelper->SetUseGrid( gal->GetGridSnapping() && !aEvent.DisableGridSnapping()  );
    m_gridHelper->SetSnap( !aEvent.Modifier( MD_SHIFT ) );

    controls()->ForceCursorPosition( false );

    if( toolManager->IsContextMenuActive() )
    {
        // If we're here from a context menu then we need to get the position of the
        // cursor when the context menu was invoked.  This is used to figure out the
        // break point on the track.
        m_startSnapPoint = snapToItem( m_startItem, toolManager->GetMenuCursorPos() );
    }
    else
    {
        // If we're here from a hotkey, then get the current mouse position so we know
        // where to break the track.
        m_startSnapPoint = snapToItem( m_startItem, controls()->GetCursorPosition() );
    }

    if( m_startItem && m_startItem->IsLocked() )
    {
        KIDIALOG dlg( frame(), _( "The selected item is locked." ), _( "Confirmation" ),
                      wxOK | wxCANCEL | wxICON_WARNING );
        dlg.SetOKLabel( _( "Break Track" ) );
        dlg.DoNotShowCheckbox( __FILE__, __LINE__ );

        if( dlg.ShowModal() == wxID_CANCEL )
            return 0;
    }

    frame()->UndoRedoBlock( true );
    breakTrack();

    if( m_router->RoutingInProgress() )
        m_router->StopRouting();

    frame()->UndoRedoBlock( false );

    return 0;
}


int ROUTER_TOOL::CustomTrackWidthDialog( const TOOL_EVENT& aEvent )
{
    BOARD_DESIGN_SETTINGS& bds = board()->GetDesignSettings();
    DIALOG_TRACK_VIA_SIZE sizeDlg( frame(), bds );

    if( sizeDlg.ShowModal() == wxID_OK )
    {
        bds.m_TempOverrideTrackWidth = true;
        bds.UseCustomTrackViaSize( true );

        TOOL_EVENT dummy;
        onTrackViaSizeChanged( dummy );
    }

    return 0;
}


int ROUTER_TOOL::onTrackViaSizeChanged( const TOOL_EVENT& aEvent )
{
    PNS::SIZES_SETTINGS sizes( m_router->Sizes() );

    if( !m_router->GetCurrentNets().empty() )
        m_iface->ImportSizes( sizes, m_startItem, m_router->GetCurrentNets()[0], VECTOR2D() );

    m_router->UpdateSizes( sizes );

    // Changing the track width can affect the placement, so call the
    // move routine without changing the destination
    // Update end item first to avoid moving to an invalid/missing item
    updateEndItem( aEvent );
    m_router->Move( m_endSnapPoint, m_endItem );

    UpdateMessagePanel();

    return 0;
}


void ROUTER_TOOL::UpdateMessagePanel()
{
    std::vector<MSG_PANEL_ITEM> items;

    if( m_router->GetState() == PNS::ROUTER::ROUTE_TRACK )
    {
        PNS::SIZES_SETTINGS          sizes( m_router->Sizes() );
        PNS::RULE_RESOLVER*          resolver = m_iface->GetRuleResolver();
        PNS::CONSTRAINT              constraint;
        std::vector<PNS::NET_HANDLE> nets = m_router->GetCurrentNets();
        wxString                     description;
        wxString                     secondary;
        wxString                     mode;

        if( m_router->Mode() == PNS::ROUTER_MODE::PNS_MODE_ROUTE_DIFF_PAIR )
        {
            wxASSERT( nets.size() >= 2 );

            NETINFO_ITEM* netA = static_cast<NETINFO_ITEM*>( nets[0] );
            NETINFO_ITEM* netB = static_cast<NETINFO_ITEM*>( nets[1] );
            wxASSERT( netA );
            wxASSERT( netB );

            description = wxString::Format( _( "Routing Diff Pair: %s" ),
                                            netA->GetNetname() + wxT( ", " ) + netB->GetNetname() );

            wxString  netclass;
            NETCLASS* netclassA = netA->GetNetClass();
            NETCLASS* netclassB = netB->GetNetClass();

            if( *netclassA == *netclassB )
                netclass = netclassA->GetHumanReadableName();
            else
                netclass = netclassA->GetHumanReadableName() + wxT( ", " )
                           + netclassB->GetHumanReadableName();

            secondary = wxString::Format( _( "Resolved Netclass: %s" ),
                                          UnescapeString( netclass ) );
        }
        else if( !nets.empty() && nets[0] )
        {
            NETINFO_ITEM* net = static_cast<NETINFO_ITEM*>( nets[0] );

            description = wxString::Format( _( "Routing Track: %s" ),
                                            net->GetNetname() );

            secondary = wxString::Format(
                    _( "Resolved Netclass: %s" ),
                    UnescapeString( net->GetNetClass()->GetHumanReadableName() ) );
        }
        else
        {
            description = _( "Routing Track" );
            secondary = _( "(no net)" );
        }

        items.emplace_back( description, secondary );

        wxString cornerMode;

        if( m_router->Settings().GetFreeAngleMode() )
        {
            cornerMode = _( "Free-angle" );
        }
        else
        {
            switch( m_router->Settings().GetCornerMode() )
            {
            case DIRECTION_45::CORNER_MODE::MITERED_45: cornerMode = _( "45-degree" );         break;
            case DIRECTION_45::CORNER_MODE::ROUNDED_45: cornerMode = _( "45-degree rounded" ); break;
            case DIRECTION_45::CORNER_MODE::MITERED_90: cornerMode = _( "90-degree" );         break;
            case DIRECTION_45::CORNER_MODE::ROUNDED_90: cornerMode = _( "90-degree rounded" ); break;
            default: break;
            }
        }

        items.emplace_back( _( "Corner Style" ), cornerMode );

        switch( m_router->Settings().Mode() )
        {
        case PNS::PNS_MODE::RM_MarkObstacles: mode = _( "Highlight collisions" ); break;
        case PNS::PNS_MODE::RM_Walkaround:    mode = _( "Walk around" );          break;
        case PNS::PNS_MODE::RM_Shove:         mode = _( "Shove" );                break;
        default: break;
        }

        items.emplace_back( _( "Mode" ), mode );

#define FORMAT_VALUE( x ) frame()->MessageTextFromValue( x )

        if( m_router->Mode() == PNS::ROUTER_MODE::PNS_MODE_ROUTE_DIFF_PAIR )
        {
            items.emplace_back( wxString::Format( _( "Track Width: %s" ),
                                                  FORMAT_VALUE( sizes.DiffPairWidth() ) ),
                                wxString::Format( _( "(from %s)" ),
                                                  sizes.GetDiffPairWidthSource() ) );

            items.emplace_back( wxString::Format( _( "Min Clearance: %s" ),
                                                  FORMAT_VALUE( sizes.Clearance() ) ),
                                wxString::Format( _( "(from %s)" ),
                                                  sizes.GetClearanceSource() ) );

            items.emplace_back( wxString::Format( _( "Diff Pair Gap: %s" ),
                                                  FORMAT_VALUE( sizes.DiffPairGap() ) ),
                                wxString::Format( _( "(from %s)" ),
                                                  sizes.GetDiffPairGapSource() ) );

            const PNS::ITEM_SET& traces = m_router->Placer()->Traces();
            wxASSERT( traces.Count() == 2 );

            if( resolver->QueryConstraint( PNS::CONSTRAINT_TYPE::CT_MAX_UNCOUPLED, traces[0],
                                           traces[1], m_router->GetCurrentLayer(), &constraint ) )
            {
                items.emplace_back( wxString::Format( _( "DP Max Uncoupled-length: %s" ),
                                                      FORMAT_VALUE( constraint.m_Value.Max() ) ),
                                    wxString::Format( _( "(from %s)" ),
                                                      constraint.m_RuleName ) );
            }
        }
        else
        {
            items.emplace_back( wxString::Format( _( "Track Width: %s" ),
                                                  FORMAT_VALUE( sizes.TrackWidth() ) ),
                                wxString::Format( _( "(from %s)" ),
                                                  sizes.GetWidthSource() ) );

            items.emplace_back( wxString::Format( _( "Min Clearance: %s" ),
                                                  FORMAT_VALUE( sizes.Clearance() ) ),
                                wxString::Format( _( "(from %s)" ),
                                                  sizes.GetClearanceSource() ) );
        }

#undef FORMAT_VALUE

        frame()->SetMsgPanel( items );
    }
    else
    {
        frame()->SetMsgPanel( board() );
        return;
    }
}


void ROUTER_TOOL::setTransitions()
{
    Go( &ROUTER_TOOL::SelectCopperLayerPair,  PCB_ACTIONS::selectLayerPair.MakeEvent() );

    Go( &ROUTER_TOOL::MainLoop,               PCB_ACTIONS::routeSingleTrack.MakeEvent() );
    Go( &ROUTER_TOOL::MainLoop,               PCB_ACTIONS::routeDiffPair.MakeEvent() );
    Go( &ROUTER_TOOL::RouteSelected,          PCB_ACTIONS::routerRouteSelected.MakeEvent() );
    Go( &ROUTER_TOOL::RouteSelected,          PCB_ACTIONS::routerRouteSelectedFromEnd.MakeEvent() );
    Go( &ROUTER_TOOL::RouteSelected,          PCB_ACTIONS::routerAutorouteSelected.MakeEvent() );
    Go( &ROUTER_TOOL::DpDimensionsDialog,     PCB_ACTIONS::routerDiffPairDialog.MakeEvent() );
    Go( &ROUTER_TOOL::SettingsDialog,         PCB_ACTIONS::routerSettingsDialog.MakeEvent() );
    Go( &ROUTER_TOOL::ChangeRouterMode,       PCB_ACTIONS::routerHighlightMode.MakeEvent() );
    Go( &ROUTER_TOOL::ChangeRouterMode,       PCB_ACTIONS::routerShoveMode.MakeEvent() );
    Go( &ROUTER_TOOL::ChangeRouterMode,       PCB_ACTIONS::routerWalkaroundMode.MakeEvent() );
    Go( &ROUTER_TOOL::CycleRouterMode,        PCB_ACTIONS::cycleRouterMode.MakeEvent() );
    Go( &ROUTER_TOOL::InlineDrag,             PCB_ACTIONS::routerInlineDrag.MakeEvent() );
    Go( &ROUTER_TOOL::InlineBreakTrack,       PCB_ACTIONS::breakTrack.MakeEvent() );

    Go( &ROUTER_TOOL::onViaCommand,           ACT_PlaceThroughVia.MakeEvent() );
    Go( &ROUTER_TOOL::onViaCommand,           ACT_PlaceBlindVia.MakeEvent() );
    Go( &ROUTER_TOOL::onViaCommand,           ACT_PlaceMicroVia.MakeEvent() );
    Go( &ROUTER_TOOL::onViaCommand,           ACT_SelLayerAndPlaceThroughVia.MakeEvent() );
    Go( &ROUTER_TOOL::onViaCommand,           ACT_SelLayerAndPlaceBlindVia.MakeEvent() );
    Go( &ROUTER_TOOL::onViaCommand,           ACT_SelLayerAndPlaceMicroVia.MakeEvent() );

    Go( &ROUTER_TOOL::onLayerCommand,         PCB_ACTIONS::layerTop.MakeEvent() );
    Go( &ROUTER_TOOL::onLayerCommand,         PCB_ACTIONS::layerInner1.MakeEvent() );
    Go( &ROUTER_TOOL::onLayerCommand,         PCB_ACTIONS::layerInner2.MakeEvent() );
    Go( &ROUTER_TOOL::onLayerCommand,         PCB_ACTIONS::layerInner3.MakeEvent() );
    Go( &ROUTER_TOOL::onLayerCommand,         PCB_ACTIONS::layerInner4.MakeEvent() );
    Go( &ROUTER_TOOL::onLayerCommand,         PCB_ACTIONS::layerInner5.MakeEvent() );
    Go( &ROUTER_TOOL::onLayerCommand,         PCB_ACTIONS::layerInner6.MakeEvent() );
    Go( &ROUTER_TOOL::onLayerCommand,         PCB_ACTIONS::layerInner7.MakeEvent() );
    Go( &ROUTER_TOOL::onLayerCommand,         PCB_ACTIONS::layerInner8.MakeEvent() );
    Go( &ROUTER_TOOL::onLayerCommand,         PCB_ACTIONS::layerInner9.MakeEvent() );
    Go( &ROUTER_TOOL::onLayerCommand,         PCB_ACTIONS::layerInner10.MakeEvent() );
    Go( &ROUTER_TOOL::onLayerCommand,         PCB_ACTIONS::layerInner11.MakeEvent() );
    Go( &ROUTER_TOOL::onLayerCommand,         PCB_ACTIONS::layerInner12.MakeEvent() );
    Go( &ROUTER_TOOL::onLayerCommand,         PCB_ACTIONS::layerInner13.MakeEvent() );
    Go( &ROUTER_TOOL::onLayerCommand,         PCB_ACTIONS::layerInner14.MakeEvent() );
    Go( &ROUTER_TOOL::onLayerCommand,         PCB_ACTIONS::layerInner15.MakeEvent() );
    Go( &ROUTER_TOOL::onLayerCommand,         PCB_ACTIONS::layerInner16.MakeEvent() );
    Go( &ROUTER_TOOL::onLayerCommand,         PCB_ACTIONS::layerInner17.MakeEvent() );
    Go( &ROUTER_TOOL::onLayerCommand,         PCB_ACTIONS::layerInner18.MakeEvent() );
    Go( &ROUTER_TOOL::onLayerCommand,         PCB_ACTIONS::layerInner19.MakeEvent() );
    Go( &ROUTER_TOOL::onLayerCommand,         PCB_ACTIONS::layerInner20.MakeEvent() );
    Go( &ROUTER_TOOL::onLayerCommand,         PCB_ACTIONS::layerInner21.MakeEvent() );
    Go( &ROUTER_TOOL::onLayerCommand,         PCB_ACTIONS::layerInner22.MakeEvent() );
    Go( &ROUTER_TOOL::onLayerCommand,         PCB_ACTIONS::layerInner23.MakeEvent() );
    Go( &ROUTER_TOOL::onLayerCommand,         PCB_ACTIONS::layerInner24.MakeEvent() );
    Go( &ROUTER_TOOL::onLayerCommand,         PCB_ACTIONS::layerInner25.MakeEvent() );
    Go( &ROUTER_TOOL::onLayerCommand,         PCB_ACTIONS::layerInner26.MakeEvent() );
    Go( &ROUTER_TOOL::onLayerCommand,         PCB_ACTIONS::layerInner27.MakeEvent() );
    Go( &ROUTER_TOOL::onLayerCommand,         PCB_ACTIONS::layerInner28.MakeEvent() );
    Go( &ROUTER_TOOL::onLayerCommand,         PCB_ACTIONS::layerInner29.MakeEvent() );
    Go( &ROUTER_TOOL::onLayerCommand,         PCB_ACTIONS::layerInner30.MakeEvent() );
    Go( &ROUTER_TOOL::onLayerCommand,         PCB_ACTIONS::layerBottom.MakeEvent() );
    Go( &ROUTER_TOOL::onLayerCommand,         PCB_ACTIONS::layerNext.MakeEvent() );
    Go( &ROUTER_TOOL::onLayerCommand,         PCB_ACTIONS::layerPrev.MakeEvent() );
    Go( &ROUTER_TOOL::onLayerCommand,         PCB_ACTIONS::layerToggle.MakeEvent() );

    Go( &ROUTER_TOOL::CustomTrackWidthDialog, ACT_CustomTrackWidth.MakeEvent() );
    Go( &ROUTER_TOOL::onTrackViaSizeChanged,  PCB_ACTIONS::trackViaSizeChanged.MakeEvent() );

    Go( &ROUTER_TOOL::handlePnSCornerModeChange, ACT_SwitchCornerModeToNext.MakeEvent() );
    Go( &ROUTER_TOOL::handlePnSCornerModeChange, ACT_SwitchCornerMode45.MakeEvent() );
    Go( &ROUTER_TOOL::handlePnSCornerModeChange, ACT_SwitchCornerMode90.MakeEvent() );
    Go( &ROUTER_TOOL::handlePnSCornerModeChange, ACT_SwitchCornerModeArc45.MakeEvent() );
    Go( &ROUTER_TOOL::handlePnSCornerModeChange, ACT_SwitchCornerModeArc90.MakeEvent() );
}
