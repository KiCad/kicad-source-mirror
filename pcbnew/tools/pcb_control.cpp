/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2016 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include "pcb_control.h"
#include "convert_basic_shapes_to_polygon.h"

#include <kiplatform/ui.h>
#include <kiway.h>
#include <tools/edit_tool.h>
#include <tools/board_inspection_tool.h>
#include <router/router_tool.h>
#include <pgm_base.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_picker_tool.h>
#include <tools/pcb_selection_tool.h>
#include <tools/board_reannotate_tool.h>
#include <3d_viewer/eda_3d_viewer_frame.h>
#include <board_commit.h>
#include <board.h>
#include <board_design_settings.h>
#include <board_item.h>
#include <board_stackup_manager/stackup_predefined_prms.h>
#include <clipboard.h>
#include <design_block.h>
#include <dialogs/dialog_paste_special.h>
#include <pcb_dimension.h>
#include <geometry/convex_hull.h>
#include <geometry/shape_utils.h>
#include <gal/graphics_abstraction_layer.h>
#include <footprint.h>
#include <pad.h>
#include <layer_pairs.h>
#include <pcb_group.h>
#include <pcb_layer_presentation.h>
#include <pcb_reference_image.h>
#include <pcb_textbox.h>
#include <pcb_table.h>
#include <pcb_tablecell.h>
#include <pcb_track.h>
#include <pcb_generator.h>
#include <tools/pcb_edit_table_tool.h>
#include <project_pcb.h>
#include <wildcards_and_files_ext.h>
#include <filename_resolver.h>
#include <3d_cache/3d_cache.h>
#include <embedded_files.h>
#include <wx/filename.h>
#include <zone.h>
#include <confirm.h>
#include <kidialog.h>
#include <connectivity/connectivity_data.h>
#include <core/kicad_algo.h>
#include <dialogs/hotkey_cycle_popup.h>
#include <kicad_clipboard.h>
#include <origin_viewitem.h>
#include <pcb_edit_frame.h>
#include <pcb_painter.h>
#include <settings/color_settings.h>
#include <string>
#include <tool/tool_manager.h>
#include <tools/multichannel_tool.h>
#include <footprint_edit_frame.h>
#include <footprint_editor_settings.h>
#include <footprint_viewer_frame.h>
#include <widgets/appearance_controls.h>
#include <widgets/pcb_design_block_pane.h>
#include <widgets/wx_progress_reporters.h>
#include <widgets/wx_infobar.h>
#include <wx/hyperlink.h>


using namespace std::placeholders;


// files.cpp
extern bool AskLoadBoardFileName( PCB_EDIT_FRAME* aParent, wxString* aFileName, int aCtl = 0 );

// board_tables/board_stackup_table.cpp
extern PCB_TABLE* Build_Board_Stackup_Table( BOARD* aBoard, EDA_UNITS aDisplayUnits );
// board_tables/board_characteristics_table.cpp
extern PCB_TABLE* Build_Board_Characteristics_Table( BOARD* aBoard, EDA_UNITS aDisplayUnits );


PCB_CONTROL::PCB_CONTROL() :
        PCB_TOOL_BASE( "pcbnew.Control" ),
        m_frame( nullptr ),
        m_pickerItem( nullptr )
{
    m_gridOrigin.reset( new KIGFX::ORIGIN_VIEWITEM() );
}


PCB_CONTROL::~PCB_CONTROL()
{
}


void PCB_CONTROL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<PCB_BASE_FRAME>();

    if( aReason == MODEL_RELOAD || aReason == GAL_SWITCH || aReason == REDRAW )
    {
        m_gridOrigin->SetPosition( board()->GetDesignSettings().GetGridOrigin() );

        double  backgroundBrightness = m_frame->GetCanvas()->GetGAL()->GetClearColor().GetBrightness();
        COLOR4D color = m_frame->GetGridColor();

        if( backgroundBrightness > 0.5 )
            color.Darken( 0.25 );
        else
            color.Brighten( 0.25 );

        m_gridOrigin->SetColor( color );

        getView()->Remove( m_gridOrigin.get() );
        getView()->Add( m_gridOrigin.get() );
    }
}


int PCB_CONTROL::AddLibrary( const TOOL_EVENT& aEvent )
{
    if( m_frame->IsType( FRAME_FOOTPRINT_EDITOR ) || m_frame->IsType( FRAME_PCB_EDITOR ) )
    {
        if( aEvent.IsAction( &ACTIONS::newLibrary ) )
            static_cast<PCB_BASE_EDIT_FRAME*>( m_frame )->CreateNewLibrary( _( "New Footprint Library" ) );
        else if( aEvent.IsAction( &ACTIONS::addLibrary ) )
            static_cast<PCB_BASE_EDIT_FRAME*>( m_frame )->AddLibrary( _( "Add Footprint Library" ) );
    }

    return 0;
}


int PCB_CONTROL::LoadFpFromBoard( const TOOL_EVENT& aEvent )
{
    if( m_frame->IsType( FRAME_FOOTPRINT_EDITOR ) )
        static_cast<FOOTPRINT_EDIT_FRAME*>( m_frame )->LoadFootprintFromBoard( nullptr );

    return 0;
}


int PCB_CONTROL::SaveFpToBoard( const TOOL_EVENT& aEvent )
{
    if( m_frame->IsType( FRAME_FOOTPRINT_EDITOR ) )
        static_cast<FOOTPRINT_EDIT_FRAME*>( m_frame )->SaveFootprintToBoard( true );
    else if( m_frame->IsType( FRAME_FOOTPRINT_VIEWER ) )
        static_cast<FOOTPRINT_VIEWER_FRAME*>( m_frame )->AddFootprintToPCB();

    return 0;
}


int PCB_CONTROL::DdAddLibrary( const TOOL_EVENT& aEvent )
{
    const wxString fn = *aEvent.Parameter<wxString*>();
    static_cast<PCB_BASE_EDIT_FRAME*>( m_frame )->AddLibrary( _( "Add Footprint Library" ), fn,
                                                              LIBRARY_TABLE_SCOPE::PROJECT );
    return 0;
}


int PCB_CONTROL::DdImportFootprint( const TOOL_EVENT& aEvent )
{
    const wxString fn = *aEvent.Parameter<wxString*>();
    static_cast<FOOTPRINT_EDIT_FRAME*>( m_frame )->ImportFootprint( fn );
    m_frame->Zoom_Automatique( false );
    return 0;
}


int PCB_CONTROL::IterateFootprint( const TOOL_EVENT& aEvent )
{
    if( m_frame->IsType( FRAME_FOOTPRINT_VIEWER ) )
        static_cast<FOOTPRINT_VIEWER_FRAME*>( m_frame )->SelectAndViewFootprint( aEvent.Parameter<FPVIEWER_CONSTANTS>() );

    return 0;
}


template<class T>
void Flip( T& aValue )
{
    aValue = !aValue;
}


int PCB_CONTROL::TrackDisplayMode( const TOOL_EVENT& aEvent )
{
    Flip( displayOptions().m_DisplayPcbTrackFill );

    for( PCB_TRACK* track : board()->Tracks() )
    {
        if( track->Type() == PCB_TRACE_T || track->Type() == PCB_ARC_T )
            view()->Update( track, KIGFX::REPAINT );
    }

    for( BOARD_ITEM* shape : board()->Drawings() )
    {
        if( shape->Type() == PCB_SHAPE_T && static_cast<PCB_SHAPE*>( shape )->IsOnCopperLayer() )
            view()->Update( shape, KIGFX::REPAINT );
    }

    canvas()->Refresh();

    return 0;
}


int PCB_CONTROL::ToggleRatsnest( const TOOL_EVENT& aEvent )
{
    if( PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>( m_frame ) )
    {
        if( aEvent.IsAction( &PCB_ACTIONS::showRatsnest ) )
        {
            // N.B. Do not disable the Ratsnest layer here.  We use it for local ratsnest
            Flip( displayOptions().m_ShowGlobalRatsnest );
            editFrame->SetElementVisibility( LAYER_RATSNEST, displayOptions().m_ShowGlobalRatsnest );
        }
        else if( aEvent.IsAction( &PCB_ACTIONS::ratsnestLineMode ) )
        {
            Flip( displayOptions().m_DisplayRatsnestLinesCurved );
        }

        editFrame->OnDisplayOptionsChanged();

        canvas()->RedrawRatsnest();
        canvas()->Refresh();
    }

    return 0;
}


int PCB_CONTROL::ViaDisplayMode( const TOOL_EVENT& aEvent )
{
    Flip( displayOptions().m_DisplayViaFill );

    for( PCB_TRACK* track : board()->Tracks() )
    {
        if( track->Type() == PCB_VIA_T )
            view()->Update( track, KIGFX::REPAINT );
    }

    canvas()->Refresh();
    return 0;
}


/**
 * We have bug reports indicating that some new users confuse zone filling/unfilling with the
 * display modes.  This will put up a warning if they show zone fills when one or more zones
 * are unfilled.
 */
void PCB_CONTROL::unfilledZoneCheck()
{
    if( Pgm().GetCommonSettings()->m_DoNotShowAgain.zone_fill_warning )
        return;

    bool unfilledZones = false;

    for( const ZONE* zone : board()->Zones() )
    {
        if( !zone->GetIsRuleArea() && !zone->IsFilled() )
        {
            unfilledZones = true;
            break;
        }
    }

    if( unfilledZones )
    {
        WX_INFOBAR*      infobar = m_frame->GetInfoBar();
        wxHyperlinkCtrl* button = new wxHyperlinkCtrl( infobar, wxID_ANY, _( "Don't show again" ), wxEmptyString );

        button->Bind( wxEVT_COMMAND_HYPERLINK, std::function<void( wxHyperlinkEvent& aEvent )>(
                [&]( wxHyperlinkEvent& aEvent )
                {
                    Pgm().GetCommonSettings()->m_DoNotShowAgain.zone_fill_warning = true;
                    m_frame->GetInfoBar()->Dismiss();
                } ) );

        infobar->RemoveAllButtons();
        infobar->AddButton( button );

        wxString msg;
        msg.Printf( _( "Not all zones are filled. Use Edit > Fill All Zones (%s) "
                       "if you wish to see all fills." ),
                    KeyNameFromKeyCode( PCB_ACTIONS::zoneFillAll.GetHotKey() ) );

        infobar->ShowMessageFor( msg, 5000, wxICON_WARNING );
    }
}


int PCB_CONTROL::ZoneDisplayMode( const TOOL_EVENT& aEvent )
{
    PCB_DISPLAY_OPTIONS opts = m_frame->GetDisplayOptions();

    // Apply new display options to the GAL canvas
    if( aEvent.IsAction( &PCB_ACTIONS::zoneDisplayFilled ) )
    {
        unfilledZoneCheck();

        opts.m_ZoneDisplayMode = ZONE_DISPLAY_MODE::SHOW_FILLED;
    }
    else if( aEvent.IsAction( &PCB_ACTIONS::zoneDisplayOutline ) )
    {
        opts.m_ZoneDisplayMode = ZONE_DISPLAY_MODE::SHOW_ZONE_OUTLINE;
    }
    else if( aEvent.IsAction( &PCB_ACTIONS::zoneDisplayFractured ) )
    {
        opts.m_ZoneDisplayMode = ZONE_DISPLAY_MODE::SHOW_FRACTURE_BORDERS;
    }
    else if( aEvent.IsAction( &PCB_ACTIONS::zoneDisplayTriangulated ) )
    {
        opts.m_ZoneDisplayMode = ZONE_DISPLAY_MODE::SHOW_TRIANGULATION;
    }
    else if( aEvent.IsAction( &PCB_ACTIONS::zoneDisplayToggle ) )
    {
        if( opts.m_ZoneDisplayMode == ZONE_DISPLAY_MODE::SHOW_FILLED )
            opts.m_ZoneDisplayMode = ZONE_DISPLAY_MODE::SHOW_ZONE_OUTLINE;
        else
            opts.m_ZoneDisplayMode = ZONE_DISPLAY_MODE::SHOW_FILLED;
    }
    else
    {
        wxFAIL;
    }

    m_frame->SetDisplayOptions( opts );

    for( ZONE* zone : board()->Zones() )
        view()->Update( zone, KIGFX::REPAINT );

    canvas()->Refresh();

    return 0;
}


int PCB_CONTROL::HighContrastMode( const TOOL_EVENT& aEvent )
{
    PCB_DISPLAY_OPTIONS opts = m_frame->GetDisplayOptions();

    opts.m_ContrastModeDisplay = opts.m_ContrastModeDisplay == HIGH_CONTRAST_MODE::NORMAL ? HIGH_CONTRAST_MODE::DIMMED
                                                                                          : HIGH_CONTRAST_MODE::NORMAL;

    m_frame->SetDisplayOptions( opts );
    return 0;
}


int PCB_CONTROL::HighContrastModeCycle( const TOOL_EVENT& aEvent )
{
    PCB_DISPLAY_OPTIONS opts = m_frame->GetDisplayOptions();

    switch( opts.m_ContrastModeDisplay )
    {
    case HIGH_CONTRAST_MODE::NORMAL: opts.m_ContrastModeDisplay = HIGH_CONTRAST_MODE::DIMMED; break;
    case HIGH_CONTRAST_MODE::DIMMED: opts.m_ContrastModeDisplay = HIGH_CONTRAST_MODE::HIDDEN; break;
    case HIGH_CONTRAST_MODE::HIDDEN: opts.m_ContrastModeDisplay = HIGH_CONTRAST_MODE::NORMAL; break;
    }

    m_frame->SetDisplayOptions( opts );

    m_toolMgr->PostEvent( EVENTS::ContrastModeChangedByKeyEvent );
    return 0;
}


int PCB_CONTROL::ContrastModeFeedback( const TOOL_EVENT& aEvent )
{
    if( !Pgm().GetCommonSettings()->m_Input.hotkey_feedback )
        return 0;

    PCB_DISPLAY_OPTIONS opts = m_frame->GetDisplayOptions();

    wxArrayString labels;
    labels.Add( _( "Normal" ) );
    labels.Add( _( "Dimmed" ) );
    labels.Add( _( "Hidden" ) );

    if( !m_frame->GetHotkeyPopup() )
        m_frame->CreateHotkeyPopup();

    HOTKEY_CYCLE_POPUP* popup = m_frame->GetHotkeyPopup();

    if( popup )
    {
        popup->Popup( _( "Inactive Layer Display" ), labels, static_cast<int>( opts.m_ContrastModeDisplay ) );
    }

    return 0;
}


int PCB_CONTROL::NetColorModeCycle( const TOOL_EVENT& aEvent )
{
    PCB_DISPLAY_OPTIONS opts = m_frame->GetDisplayOptions();

    switch( opts.m_NetColorMode )
    {
    case NET_COLOR_MODE::ALL:      opts.m_NetColorMode = NET_COLOR_MODE::RATSNEST; break;
    case NET_COLOR_MODE::RATSNEST: opts.m_NetColorMode = NET_COLOR_MODE::OFF;      break;
    case NET_COLOR_MODE::OFF:      opts.m_NetColorMode = NET_COLOR_MODE::ALL;      break;
    }

    m_frame->SetDisplayOptions( opts );
    return 0;
}


int PCB_CONTROL::RatsnestModeCycle( const TOOL_EVENT& aEvent )
{
    if( PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>( m_frame ) )
    {
        if( !displayOptions().m_ShowGlobalRatsnest )
        {
            displayOptions().m_ShowGlobalRatsnest = true;
            displayOptions().m_RatsnestMode = RATSNEST_MODE::ALL;
        }
        else if( displayOptions().m_RatsnestMode == RATSNEST_MODE::ALL )
        {
            displayOptions().m_RatsnestMode = RATSNEST_MODE::VISIBLE;
        }
        else
        {
            displayOptions().m_ShowGlobalRatsnest = false;
        }

        editFrame->SetElementVisibility( LAYER_RATSNEST, displayOptions().m_ShowGlobalRatsnest );

        editFrame->OnDisplayOptionsChanged();

        canvas()->RedrawRatsnest();
        canvas()->Refresh();
    }

    return 0;
}


int PCB_CONTROL::LayerSwitch( const TOOL_EVENT& aEvent )
{
    m_frame->SwitchLayer( aEvent.Parameter<PCB_LAYER_ID>() );

    return 0;
}


int PCB_CONTROL::LayerNext( const TOOL_EVENT& aEvent )
{
    BOARD*       brd        = board();
    PCB_LAYER_ID layer      = m_frame->GetActiveLayer();
    bool         wraparound = false;

    if( !IsCopperLayer( layer ) )
    {
        m_frame->SwitchLayer( B_Cu );
        return 0;
    }

    LSET cuMask = LSET::AllCuMask( brd->GetCopperLayerCount() );
    LSEQ layerStack = cuMask.UIOrder();

    int ii = 0;

    // Find the active layer in list
    for( ; ii < (int) layerStack.size(); ii++ )
    {
        if( layer == layerStack[ii] )
            break;
    }

    // Find the next visible layer in list
    for( ; ii < (int) layerStack.size(); ii++ )
    {
        int jj = ii + 1;

        if( jj >= (int) layerStack.size() )
            jj = 0;

        layer = layerStack[jj];

        if( brd->IsLayerVisible( layer ) )
            break;

        if( jj == 0 )   // the end of list is reached. Try from the beginning
        {
            if( wraparound )
            {
                wxBell();
                return 0;
            }
            else
            {
                wraparound = true;
                ii = -1;
            }
        }
    }

    wxCHECK( IsCopperLayer( layer ), 0 );
    m_frame->SwitchLayer( layer );

    return 0;
}


int PCB_CONTROL::LayerPrev( const TOOL_EVENT& aEvent )
{
    BOARD*       brd        = board();
    PCB_LAYER_ID layer      = m_frame->GetActiveLayer();
    bool         wraparound = false;

    if( !IsCopperLayer( layer ) )
    {
        m_frame->SwitchLayer( F_Cu );
        return 0;
    }

    LSET cuMask = LSET::AllCuMask( brd->GetCopperLayerCount() );
    LSEQ layerStack = cuMask.UIOrder();

    int ii = 0;

    // Find the active layer in list
    for( ; ii < (int) layerStack.size(); ii++ )
    {
        if( layer == layerStack[ii] )
            break;
    }

    // Find the previous visible layer in list
    for( ; ii >= 0; ii-- )
    {
        int jj = ii - 1;

        if( jj < 0 )
            jj = (int) layerStack.size() - 1;

        layer = layerStack[jj];

        if( brd->IsLayerVisible( layer ) )
            break;

        if( ii == 0 )   // the start of list is reached. Try from the last
        {
            if( wraparound )
            {
                wxBell();
                return 0;
            }
            else
            {
                wraparound = true;
                ii = 1;
            }
        }
    }

    wxCHECK( IsCopperLayer( layer ), 0 );
    m_frame->SwitchLayer( layer );

    return 0;
}


int PCB_CONTROL::LayerToggle( const TOOL_EVENT& aEvent )
{
    int         currentLayer = m_frame->GetActiveLayer();
    PCB_SCREEN* screen = m_frame->GetScreen();

    if( currentLayer == screen->m_Route_Layer_TOP )
        m_frame->SwitchLayer( screen->m_Route_Layer_BOTTOM );
    else
        m_frame->SwitchLayer( screen->m_Route_Layer_TOP );

    return 0;
}


// It'd be nice to share the min/max with the DIALOG_COLOR_PICKER, but those are
// set in wxFormBuilder.
#define ALPHA_MIN 0.20
#define ALPHA_MAX 1.00
#define ALPHA_STEP 0.05


int PCB_CONTROL::LayerAlphaInc( const TOOL_EVENT& aEvent )
{
    COLOR_SETTINGS* settings = m_frame->GetColorSettings();
    int             currentLayer = m_frame->GetActiveLayer();
    KIGFX::COLOR4D  currentColor = settings->GetColor( currentLayer );

    if( currentColor.a <= ALPHA_MAX - ALPHA_STEP )
    {
        currentColor.a += ALPHA_STEP;
        settings->SetColor( currentLayer, currentColor );
        m_frame->GetCanvas()->UpdateColors();

        KIGFX::VIEW* view = m_frame->GetCanvas()->GetView();
        view->UpdateLayerColor( currentLayer );
        view->UpdateLayerColor( GetNetnameLayer( currentLayer ) );

        if( IsCopperLayer( currentLayer ) )
            view->UpdateLayerColor( ZONE_LAYER_FOR( currentLayer ) );

        m_frame->GetCanvas()->ForceRefresh();
    }
    else
    {
        wxBell();
    }

    return 0;
}


int PCB_CONTROL::LayerAlphaDec( const TOOL_EVENT& aEvent )
{
    COLOR_SETTINGS* settings = m_frame->GetColorSettings();
    int             currentLayer = m_frame->GetActiveLayer();
    KIGFX::COLOR4D  currentColor = settings->GetColor( currentLayer );

    if( currentColor.a >= ALPHA_MIN + ALPHA_STEP )
    {
        currentColor.a -= ALPHA_STEP;
        settings->SetColor( currentLayer, currentColor );
        m_frame->GetCanvas()->UpdateColors();

        KIGFX::VIEW* view = m_frame->GetCanvas()->GetView();
        view->UpdateLayerColor( currentLayer );
        view->UpdateLayerColor( GetNetnameLayer( currentLayer ) );

        if( IsCopperLayer( currentLayer ) )
            view->UpdateLayerColor( ZONE_LAYER_FOR( currentLayer ) );

        m_frame->GetCanvas()->ForceRefresh();
    }
    else
    {
        wxBell();
    }

    return 0;
}


int PCB_CONTROL::CycleLayerPresets( const TOOL_EVENT& aEvent )
{
    if( PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>( m_frame ) )
    {
        LAYER_PAIR_SETTINGS* settings = editFrame->GetLayerPairSettings();

        if( !settings )
            return 0;

        int                          currentIndex;
        std::vector<LAYER_PAIR_INFO> presets = settings->GetEnabledLayerPairs( currentIndex );

        if( presets.size() < 2 )
            return 0;

        if( currentIndex < 0 )
        {
            wxASSERT_MSG( false, "Current layer pair not found in layer settings" );
            currentIndex = 0;
        }

        const int         nextIndex = ( currentIndex + 1 ) % presets.size();
        const LAYER_PAIR& nextPair = presets[nextIndex].GetLayerPair();

        settings->SetCurrentLayerPair( nextPair );

        m_toolMgr->PostEvent( PCB_EVENTS::LayerPairPresetChangedByKeyEvent() );
    }

    return 0;
}


int PCB_CONTROL::LayerPresetFeedback( const TOOL_EVENT& aEvent )
{
    if( !Pgm().GetCommonSettings()->m_Input.hotkey_feedback )
        return 0;

    if( PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>( m_frame ) )
    {
        LAYER_PAIR_SETTINGS* settings = editFrame->GetLayerPairSettings();

        if( !settings )
            return 0;

        PCB_LAYER_PRESENTATION layerPresentation( editFrame );

        int                          currentIndex;
        std::vector<LAYER_PAIR_INFO> presets = settings->GetEnabledLayerPairs( currentIndex );

        wxArrayString labels;
        for( const LAYER_PAIR_INFO& layerPairInfo : presets )
        {
            wxString label = layerPresentation.getLayerPairName( layerPairInfo.GetLayerPair() );

            if( layerPairInfo.GetName() )
                label += wxT( " (" ) + *layerPairInfo.GetName() + wxT( ")" );

            labels.Add( label );
        }

        if( !editFrame->GetHotkeyPopup() )
            editFrame->CreateHotkeyPopup();

        HOTKEY_CYCLE_POPUP* popup = editFrame->GetHotkeyPopup();

        if( popup )
        {
            int selection = currentIndex;
            popup->Popup( _( "Preset Layer Pairs" ), labels, selection );
        }
    }

    return 0;
}


void PCB_CONTROL::DoSetGridOrigin( KIGFX::VIEW* aView, PCB_BASE_FRAME* aFrame, EDA_ITEM* originViewItem,
                                   const VECTOR2D& aPoint )
{
    aFrame->GetDesignSettings().SetGridOrigin( VECTOR2I( aPoint ) );
    aView->GetGAL()->SetGridOrigin( aPoint );
    originViewItem->SetPosition( aPoint );
    aView->MarkDirty();
    aFrame->OnModify();
}


int PCB_CONTROL::GridPlaceOrigin( const TOOL_EVENT& aEvent )
{
    VECTOR2D* origin = aEvent.Parameter<VECTOR2D*>();

    if( origin )
    {
        // We can't undo the other grid dialog settings, so no sense undoing just the origin
        DoSetGridOrigin( getView(), m_frame, m_gridOrigin.get(), *origin );
        delete origin;
    }
    else
    {
        if( m_isFootprintEditor && !getEditFrame<PCB_BASE_EDIT_FRAME>()->GetModel() )
            return 0;

        PCB_PICKER_TOOL* picker = m_toolMgr->GetTool<PCB_PICKER_TOOL>();

        if( !picker )   // Happens in footprint wizard
            return 0;

        // Deactivate other tools; particularly important if another PICKER is currently running
        Activate();

        picker->SetCursor( KICURSOR::PLACE );
        picker->ClearHandlers();

        picker->SetClickHandler(
                [this]( const VECTOR2D& pt ) -> bool
                {
                    m_frame->SaveCopyInUndoList( m_gridOrigin.get(), UNDO_REDO::GRIDORIGIN );
                    DoSetGridOrigin( getView(), m_frame, m_gridOrigin.get(), pt );
                    return false;   // drill origin is a one-shot; don't continue with tool
                } );

        m_toolMgr->RunAction( ACTIONS::pickerTool, &aEvent );
    }

    return 0;
}


int PCB_CONTROL::GridResetOrigin( const TOOL_EVENT& aEvent )
{
    m_frame->SaveCopyInUndoList( m_gridOrigin.get(), UNDO_REDO::GRIDORIGIN );
    DoSetGridOrigin( getView(), m_frame, m_gridOrigin.get(), VECTOR2D( 0, 0 ) );
    return 0;
}


#define HITTEST_THRESHOLD_PIXELS 5


int PCB_CONTROL::InteractiveDelete( const TOOL_EVENT& aEvent )
{
    if( m_isFootprintEditor && !m_frame->GetBoard()->GetFirstFootprint() )
        return 0;

    PCB_PICKER_TOOL* picker = m_toolMgr->GetTool<PCB_PICKER_TOOL>();

    m_pickerItem = nullptr;
    m_toolMgr->RunAction( ACTIONS::selectionClear );

    // Deactivate other tools; particularly important if another PICKER is currently running
    Activate();

    picker->SetCursor( KICURSOR::REMOVE );
    picker->SetSnapping( false );
    picker->ClearHandlers();

    picker->SetClickHandler(
            [this]( const VECTOR2D& aPosition ) -> bool
            {
                if( m_pickerItem )
                {
                    if( m_pickerItem && m_pickerItem->IsLocked() )
                    {
                        m_statusPopup.reset( new STATUS_TEXT_POPUP( m_frame ) );
                        m_statusPopup->SetText( _( "Item locked." ) );
                        m_statusPopup->PopupFor( 2000 );
                        m_statusPopup->Move( KIPLATFORM::UI::GetMousePosition() + wxPoint( 20, 20 ) );
                        return true;
                    }

                    PCB_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
                    selectionTool->UnbrightenItem( m_pickerItem );

                    PCB_SELECTION items;
                    items.Add( m_pickerItem );

                    EDIT_TOOL* editTool = m_toolMgr->GetTool<EDIT_TOOL>();
                    editTool->DeleteItems( items, false );

                    m_pickerItem = nullptr;
                }

                return true;
            } );

    picker->SetMotionHandler(
            [this]( const VECTOR2D& aPos )
            {
                BOARD*                   board = m_frame->GetBoard();
                PCB_SELECTION_TOOL*      selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
                GENERAL_COLLECTORS_GUIDE guide = m_frame->GetCollectorsGuide();
                GENERAL_COLLECTOR        collector;
                collector.m_Threshold = KiROUND( getView()->ToWorld( HITTEST_THRESHOLD_PIXELS ) );

                if( m_isFootprintEditor )
                    collector.Collect( board, GENERAL_COLLECTOR::FootprintItems, aPos, guide );
                else
                    collector.Collect( board, GENERAL_COLLECTOR::BoardLevelItems, aPos, guide );

                // Remove unselectable items
                for( int i = collector.GetCount() - 1; i >= 0; --i )
                {
                    if( !selectionTool->Selectable( collector[i] ) )
                        collector.Remove( i );
                }

                selectionTool->FilterCollectorForHierarchy( collector, false );
                selectionTool->FilterCollectedItems( collector, false, nullptr );

                if( collector.GetCount() > 1 )
                    selectionTool->GuessSelectionCandidates( collector, aPos );

                BOARD_ITEM* item = collector.GetCount() == 1 ? collector[0] : nullptr;

                if( m_pickerItem != item )
                {
                    if( m_pickerItem )
                        selectionTool->UnbrightenItem( m_pickerItem );

                    m_pickerItem = item;

                    if( m_pickerItem )
                        selectionTool->BrightenItem( m_pickerItem );
                }
            } );

    picker->SetFinalizeHandler(
            [this]( const int& aFinalState )
            {
                if( m_pickerItem )
                    m_toolMgr->GetTool<PCB_SELECTION_TOOL>()->UnbrightenItem( m_pickerItem );

                m_statusPopup.reset();

                // Ensure the cursor gets changed&updated
                m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
                m_frame->GetCanvas()->Refresh();
            } );

    m_toolMgr->RunAction( ACTIONS::pickerTool, &aEvent );

    return 0;
}


static void pasteFootprintItemsToFootprintEditor( FOOTPRINT* aClipFootprint, BOARD* aBoard,
                                                  std::vector<BOARD_ITEM*>& aPastedItems )
{
    FOOTPRINT* editorFootprint = aBoard->GetFirstFootprint();

    aClipFootprint->SetParent( aBoard );

    for( PAD* pad : aClipFootprint->Pads() )
    {
        pad->SetParent( editorFootprint );
        aPastedItems.push_back( pad );
    }

    aClipFootprint->Pads().clear();

    // Not all items can be added to the current footprint: mandatory fields are already existing
    // in the current footprint.
    //
    for( PCB_FIELD* field : aClipFootprint->GetFields() )
    {
        wxCHECK2( field, continue );

        if( field->IsMandatory() )
        {
            if( EDA_GROUP* parentGroup = field->GetParentGroup() )
                parentGroup->RemoveItem( field );
        }
        else
        {
            PCB_TEXT* text = static_cast<PCB_TEXT*>( field );

            text->SetTextAngle( text->GetTextAngle() - aClipFootprint->GetOrientation() );
            text->SetTextAngle( text->GetTextAngle() + editorFootprint->GetOrientation() );

            VECTOR2I pos = field->GetFPRelativePosition();
            field->SetParent( editorFootprint );
            field->SetFPRelativePosition( pos );

            aPastedItems.push_back( field );
        }
    }

    aClipFootprint->GetFields().clear();

    for( BOARD_ITEM* item : aClipFootprint->GraphicalItems() )
    {
        if( item->Type() == PCB_TEXT_T )
        {
            PCB_TEXT* text = static_cast<PCB_TEXT*>( item );

            text->SetTextAngle( text->GetTextAngle() - aClipFootprint->GetOrientation() );
            text->SetTextAngle( text->GetTextAngle() + editorFootprint->GetOrientation() );
        }

        item->Rotate( item->GetPosition(), -aClipFootprint->GetOrientation() );
        item->Rotate( item->GetPosition(), editorFootprint->GetOrientation() );

        VECTOR2I pos = item->GetFPRelativePosition();
        item->SetParent( editorFootprint );
        item->SetFPRelativePosition( pos );

        aPastedItems.push_back( item );
    }

    aClipFootprint->GraphicalItems().clear();

    for( ZONE* zone : aClipFootprint->Zones() )
    {
        zone->SetParent( editorFootprint );
        aPastedItems.push_back( zone );
    }

    aClipFootprint->Zones().clear();

    for( PCB_GROUP* group : aClipFootprint->Groups() )
    {
        group->SetParent( editorFootprint );
        aPastedItems.push_back( group );
    }

    aClipFootprint->Groups().clear();
}


void PCB_CONTROL::pruneItemLayers( std::vector<BOARD_ITEM*>& aItems )
{
    // Do not prune items or layers when copying to the FP editor, because all
    // layers are accepted, even if they are not enabled in the dummy board
    // This is mainly true for internal copper layers: all are allowed but only one
    // (In1.cu) is enabled for the GUI.
    if( m_isFootprintEditor || m_frame->IsType( FRAME_FOOTPRINT_EDITOR ) )
        return;

    LSET                     enabledLayers = board()->GetEnabledLayers();
    std::vector<BOARD_ITEM*> returnItems;
    bool                     fpItemDeleted = false;

    for( BOARD_ITEM* item : aItems )
    {
        if( item->Type() == PCB_FOOTPRINT_T )
        {
            FOOTPRINT* fp = static_cast<FOOTPRINT*>( item );

            // Items living in a parent footprint are never removed, even if their
            // layer does not exist in the board editor
            // Otherwise the parent footprint could be seriously broken especially
            // if some layers are later re-enabled.
            // Moreover a fp lives in a fp library, that does not know the enabled
            // layers of a given board, so fp items are just ignored when on not
            // enabled layers in board editor
            returnItems.push_back( fp );
        }
        else if( item->Type() == PCB_GROUP_T || item->Type() == PCB_GENERATOR_T )
        {
            returnItems.push_back( item );
        }
        else
        {
            LSET allowed = item->GetLayerSet() & enabledLayers;
            bool item_valid = true;

            // Ensure, for vias, the top and bottom layers are compatible with
            // the current board copper layers.
            // Otherwise they must be skipped, even is one layer is valid
            if( item->Type() == PCB_VIA_T )
                item_valid = static_cast<PCB_VIA*>( item )->HasValidLayerPair( board()->GetCopperLayerCount() );

            if( allowed.any() && item_valid )
            {
                item->SetLayerSet( allowed );
                returnItems.push_back( item );
            }
            else
            {
                if( EDA_GROUP* parentGroup = item->GetParentGroup() )
                    parentGroup->RemoveItem( item );
            }
        }
    }

    if( ( returnItems.size() < aItems.size() ) || fpItemDeleted )
    {
        DisplayError( m_frame, _( "Warning: some pasted items were on layers which are not "
                                  "present in the current board.\n"
                                  "These items could not be pasted.\n" ) );
    }

    aItems = returnItems;
}


int PCB_CONTROL::Paste( const TOOL_EVENT& aEvent )
{
    // The viewer frames cannot paste
    if( !m_frame->IsType( FRAME_FOOTPRINT_EDITOR ) && !m_frame->IsType( FRAME_PCB_EDITOR ) )
        return 0;

    bool isFootprintEditor = m_isFootprintEditor || m_frame->IsType( FRAME_FOOTPRINT_EDITOR );

    // The clipboard can contain two different things, an entire kicad_pcb or a single footprint
    if( isFootprintEditor && ( !board() || !footprint() ) )
        return 0;

    // We should never get here if a modal dialog is up... but we do on MacOS.
    // https://gitlab.com/kicad/code/kicad/-/issues/18912
#ifdef __WXMAC__
    if( wxDialog::OSXHasModalDialogsOpen() )
    {
        wxBell();
        return 0;
    }
#endif

    BOARD_COMMIT commit( m_frame );

    CLIPBOARD_IO pi;
    BOARD_ITEM*  clipItem = pi.Parse();

    PCB_SELECTION_TOOL* selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();

    if( selTool && clipItem )
    {
        PCB_SELECTION& selection = selTool->GetSelection();

        bool hasTableCells = false;

        for( EDA_ITEM* item : selection )
        {
            if( item->Type() == PCB_TABLECELL_T )
            {
                hasTableCells = true;
                break;
            }
        }

        if( hasTableCells )
        {
            PCB_TABLE* clipboardTable = nullptr;

            if( clipItem->Type() == PCB_T )
            {
                BOARD* clipBoard = static_cast<BOARD*>( clipItem );

                for( BOARD_ITEM* item : clipBoard->Drawings() )
                {
                    if( item->Type() == PCB_TABLE_T )
                    {
                        clipboardTable = static_cast<PCB_TABLE*>( item );
                        break;
                    }
                }
            }

            if( clipboardTable )
            {
                PCB_EDIT_TABLE_TOOL* tableEditTool = m_toolMgr->GetTool<PCB_EDIT_TABLE_TOOL>();

                if( tableEditTool )
                {
                    wxString errorMsg;

                    if( !tableEditTool->validatePasteIntoSelection( selection, errorMsg ) )
                    {
                        DisplayError( m_frame, errorMsg );
                        return 0;
                    }

                    if( tableEditTool->pasteCellsIntoSelection( selection, clipboardTable, commit ) )
                    {
                        commit.Push( _( "Paste Cells" ) );
                        return 0;
                    }
                    else
                    {
                        DisplayError( m_frame, _( "Failed to paste cells" ) );
                        return 0;
                    }
                }
            }
        }
    }

    if( !clipItem )
    {
        // When the clipboard doesn't parse, create a PCB item with the clipboard contents
        std::vector<BOARD_ITEM*> newItems;

        if( std::unique_ptr<wxImage> clipImg = GetImageFromClipboard() )
        {
            auto refImg = std::make_unique<PCB_REFERENCE_IMAGE>( m_frame->GetModel() );

            if( refImg->GetReferenceImage().SetImage( *clipImg ) )
                newItems.push_back( refImg.release() );
        }
        else
        {
            const wxString clipText = GetClipboardUTF8();

            if( clipText.empty() )
                return 0;

            // If it wasn't content, then paste as a text object.
            if( clipText.size() > static_cast<size_t>( ADVANCED_CFG::GetCfg().m_MaxPastedTextLength ) )
            {
                int result = IsOK( m_frame, _( "Pasting a long text text string may be very slow.  "
                                               "Do you want to continue?" ) );
                if( !result )
                    return 0;
            }

            std::unique_ptr<PCB_TEXT> item = std::make_unique<PCB_TEXT>( m_frame->GetModel() );
            item->SetText( clipText );
            item->SetLayer( m_frame->GetActiveLayer() );

            newItems.push_back( item.release() );
        }

        bool cancelled = !placeBoardItems( &commit, newItems, true, false, false, false );

        if( cancelled )
            commit.Revert();
        else
            commit.Push( _( "Paste Text" ) );
        return 0;
    }

    // If we get here, we have a parsed board/FP to paste

    PASTE_MODE     mode = PASTE_MODE::KEEP_ANNOTATIONS;
    bool           clear_nets = false;
    const wxString defaultRef = wxT( "REF**" );

    if( aEvent.IsAction( &ACTIONS::pasteSpecial ) )
    {
        DIALOG_PASTE_SPECIAL dlg( m_frame, &mode, defaultRef );

        if( clipItem->Type() != PCB_T )
            dlg.HideClearNets();

        if( dlg.ShowModal() == wxID_CANCEL )
            return 0;

        clear_nets = dlg.GetClearNets();
    }

    if( clipItem->Type() == PCB_T )
    {
        BOARD* clipBoard = static_cast<BOARD*>( clipItem );

        if( isFootprintEditor || clear_nets )
        {
            for( BOARD_CONNECTED_ITEM* item : clipBoard->AllConnectedItems() )
                item->SetNet( NETINFO_LIST::OrphanedItem() );
        }
        else
        {
            clipBoard->MapNets( m_frame->GetBoard() );
        }
    }

    bool cancelled = false;

    switch( clipItem->Type() )
    {
    case PCB_T:
    {
        BOARD* clipBoard = static_cast<BOARD*>( clipItem );

        if( isFootprintEditor )
        {
            FOOTPRINT*               editorFootprint = board()->GetFirstFootprint();
            std::vector<BOARD_ITEM*> pastedItems;

            for( PCB_GROUP* group : clipBoard->Groups() )
            {
                group->SetParent( editorFootprint );
                pastedItems.push_back( group );
            }

            clipBoard->RemoveAll( { PCB_GROUP_T } );

            for( FOOTPRINT* clipFootprint : clipBoard->Footprints() )
                pasteFootprintItemsToFootprintEditor( clipFootprint, board(), pastedItems );

            for( BOARD_ITEM* clipDrawItem : clipBoard->Drawings() )
            {
                switch( clipDrawItem->Type() )
                {
                case PCB_TEXT_T:
                case PCB_TEXTBOX_T:
                case PCB_TABLE_T:
                case PCB_SHAPE_T:
                case PCB_BARCODE_T:
                case PCB_DIM_ALIGNED_T:
                case PCB_DIM_CENTER_T:
                case PCB_DIM_LEADER_T:
                case PCB_DIM_ORTHOGONAL_T:
                case PCB_DIM_RADIAL_T:
                    clipDrawItem->SetParent( editorFootprint );
                    pastedItems.push_back( clipDrawItem );
                    break;

                default:
                    // Everything we *didn't* put into pastedItems is going to get nuked, so
                    // make sure it's not still included in its parent group.
                    if( EDA_GROUP* parentGroup = clipDrawItem->GetParentGroup() )
                        parentGroup->RemoveItem( clipDrawItem );

                    break;
                }
            }

            // NB: PCB_SHAPE_T actually removes everything in Drawings() (including PCB_TEXTs,
            // PCB_TABLEs, PCB_BARCODEs, dimensions, etc.), not just PCB_SHAPEs.)
            clipBoard->RemoveAll( { PCB_SHAPE_T } );

            clipBoard->Visit(
                    [&]( EDA_ITEM* item, void* testData )
                    {
                        if( item->IsBOARD_ITEM() )
                        {
                            // Anything still on the clipboard didn't get copied and needs to be
                            // removed from the pasted groups.
                            BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( item );
                            EDA_GROUP*  parentGroup = boardItem->GetParentGroup();

                            if( parentGroup )
                                parentGroup->RemoveItem( boardItem );
                        }

                        return INSPECT_RESULT::CONTINUE;
                    },
                    nullptr, GENERAL_COLLECTOR::AllBoardItems );

            delete clipBoard;

            pruneItemLayers( pastedItems );

            cancelled = !placeBoardItems( &commit, pastedItems, true, true, mode == PASTE_MODE::UNIQUE_ANNOTATIONS,
                                          false );
        }
        else    // isBoardEditor
        {
            // Fixup footprint component classes
            for( FOOTPRINT* fp : clipBoard->Footprints() )
            {
                fp->ResolveComponentClassNames( board(), fp->GetTransientComponentClassNames() );
                fp->ClearTransientComponentClassNames();
            }

            if( mode == PASTE_MODE::REMOVE_ANNOTATIONS )
            {
                for( FOOTPRINT* fp : clipBoard->Footprints() )
                    fp->SetReference( defaultRef );
            }

            cancelled = !placeBoardItems( &commit, clipBoard, true, mode == PASTE_MODE::UNIQUE_ANNOTATIONS, false );
        }

        break;
    }

    case PCB_FOOTPRINT_T:
    {
        FOOTPRINT*               clipFootprint = static_cast<FOOTPRINT*>( clipItem );
        std::vector<BOARD_ITEM*> pastedItems;

        if( isFootprintEditor )
        {
            pasteFootprintItemsToFootprintEditor( clipFootprint, board(), pastedItems );
            delete clipFootprint;
        }
        else
        {
            if( mode == PASTE_MODE::REMOVE_ANNOTATIONS )
                clipFootprint->SetReference( defaultRef );

            clipFootprint->SetParent( board() );
            clipFootprint->ResolveComponentClassNames( board(), clipFootprint->GetTransientComponentClassNames() );
            clipFootprint->ClearTransientComponentClassNames();
            pastedItems.push_back( clipFootprint );
        }

        pruneItemLayers( pastedItems );

        cancelled = !placeBoardItems( &commit, pastedItems, true, true, mode == PASTE_MODE::UNIQUE_ANNOTATIONS, false );
        break;
    }

    default:
        m_frame->DisplayToolMsg( _( "Invalid clipboard contents" ) );
        break;
    }

    if( cancelled )
        commit.Revert();
    else
        commit.Push( _( "Paste" ) );

    return 1;
}


int PCB_CONTROL::AppendBoardFromFile( const TOOL_EVENT& aEvent )
{
    wxString fileName;

    PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>( m_frame );

    if( !editFrame )
        return 1;

    // Pick a file to append
    if( !AskLoadBoardFileName( editFrame, &fileName, KICTL_KICAD_ONLY ) )
        return 1;

    PCB_IO_MGR::PCB_FILE_T pluginType = PCB_IO_MGR::FindPluginTypeFromBoardPath( fileName, KICTL_KICAD_ONLY );
    IO_RELEASER<PCB_IO>    pi( PCB_IO_MGR::FindPlugin( pluginType ) );

    if( !pi )
        return 1;

    return AppendBoard( *pi, fileName );
}


int PCB_CONTROL::AppendDesignBlock( const TOOL_EVENT& aEvent )
{
    PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>( m_frame );

    if( !editFrame )
        return 1;

    if( !editFrame->GetDesignBlockPane()->GetSelectedLibId().IsValid() )
        return 1;

    DESIGN_BLOCK_PANE*            designBlockPane = editFrame->GetDesignBlockPane();
    const LIB_ID                  selectedLibId = designBlockPane->GetSelectedLibId();
    std::unique_ptr<DESIGN_BLOCK> designBlock( designBlockPane->GetDesignBlock( selectedLibId, true, true ) );

    if( !designBlock )
    {
        wxString msg;
        msg.Printf( _( "Could not find design block %s." ), selectedLibId.GetUniStringLibId() );
        editFrame->ShowInfoBarError( msg, true );
        return 1;
    }

    if( designBlock->GetBoardFile().IsEmpty() || !wxFileName::FileExists( designBlock->GetBoardFile() ) )
    {
        editFrame->ShowInfoBarError( _( "Design block has no layout to place." ), true );
        return 1;
    }

    PCB_IO_MGR::PCB_FILE_T pluginType = PCB_IO_MGR::KICAD_SEXP;
    IO_RELEASER<PCB_IO>    pi( PCB_IO_MGR::FindPlugin( pluginType ) );

    if( !pi )
        return 1;

    bool repeatPlacement = false;

    if( APP_SETTINGS_BASE* cfg = editFrame->config() )
        repeatPlacement = cfg->m_DesignBlockChooserPanel.repeated_placement;

    int ret = 0;

    do
    {
        ret = AppendBoard( *pi, designBlock->GetBoardFile(), designBlock.get() );
    } while( repeatPlacement && ret == 0 );

    return ret;
}

int PCB_CONTROL::ApplyDesignBlockLayout( const TOOL_EVENT& aEvent )
{
    PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>( m_frame );

    if( !editFrame )
        return 1;

    BOARD* brd = board();

    if( !brd )
        return 1;

    // Need to have a group selected and it needs to have a linked design block
    PCB_SELECTION_TOOL* selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    PCB_SELECTION       selection = selTool->GetSelection();

    if( selection.Size() != 1 || selection[0]->Type() != PCB_GROUP_T )
        return 1;

    PCB_GROUP* group = static_cast<PCB_GROUP*>( selection[0] );

    if( !group->HasDesignBlockLink() )
        return 1;

    // Get the associated design block
    DESIGN_BLOCK_PANE*            designBlockPane = editFrame->GetDesignBlockPane();
    std::unique_ptr<DESIGN_BLOCK> designBlock( designBlockPane->GetDesignBlock( group->GetDesignBlockLibId(),
                                                                                true, true ) );

    if( !designBlock )
    {
        wxString msg;
        msg.Printf( _( "Could not find design block %s." ), group->GetDesignBlockLibId().GetUniStringLibId() );
        m_frame->GetInfoBar()->ShowMessageFor( msg, 5000, wxICON_WARNING );
        return 1;
    }

    if( designBlock->GetBoardFile().IsEmpty() )
    {
        wxString msg;
        msg.Printf( _( "Design block %s does not have a board file." ),
                    group->GetDesignBlockLibId().GetUniStringLibId() );
        m_frame->GetInfoBar()->ShowMessageFor( msg, 5000, wxICON_WARNING );
        return 1;
    }

    BOARD_COMMIT tempCommit( m_frame );

    std::set<EDA_ITEM*> originalItems;
    // Apply MCT_SKIP_STRUCT to every EDA_ITEM on the board so we know what is not part of the design block
    // Can't use SKIP_STRUCT as that is used and cleared by the temporary board appending
    brd->Visit(
            []( EDA_ITEM* item, void* )
            {
                item->SetFlags( MCT_SKIP_STRUCT );
                return INSPECT_RESULT::CONTINUE;
            },
            nullptr, GENERAL_COLLECTOR::AllBoardItems );

    int ret = 1;

    bool skipMove = true;

    // If we succeeded in placing the linked design block, we're ready to apply the multichannel tool
    if( m_toolMgr->RunSynchronousAction( PCB_ACTIONS::placeLinkedDesignBlock, &tempCommit, &skipMove ) )
    {
        // Lambda for the bounding box of all the components
        auto generateBoundingBox =
                [&]( std::unordered_set<EDA_ITEM*> aItems )
                {
                    std::vector<VECTOR2I> bbCorners;
                    bbCorners.reserve( aItems.size() * 4 );

                    for( auto item : aItems )
                    {
                        const BOX2I bb = item->GetBoundingBox().GetInflated( 100000 );
                        KIGEOM::CollectBoxCorners( bb, bbCorners );
                    }

                    std::vector<VECTOR2I> hullVertices;
                    BuildConvexHull( hullVertices, bbCorners );

                    SHAPE_LINE_CHAIN hull( hullVertices );

                    // Make the newly computed convex hull use only 90 degree segments
                    return KIGEOM::RectifyPolygon( hull );
                };

        // Build a rule area that contains all the components in the design block,
        // meaning all items without SKIP_STRUCT set.
        RULE_AREA dbRA;

        dbRA.m_sourceType = PLACEMENT_SOURCE_T::DESIGN_BLOCK;
        dbRA.m_generateEnabled = true;

        // Add all components that aren't marked MCT_SKIP_STRUCT to ra.m_components
        brd->Visit(
                [&]( EDA_ITEM* item, void* data )
                {
                    if( !item->HasFlag( MCT_SKIP_STRUCT ) )
                    {
                        dbRA.m_designBlockItems.insert( item );

                        if( item->Type() == PCB_FOOTPRINT_T )
                            dbRA.m_components.insert( static_cast<FOOTPRINT*>( item ) );
                    }
                    return INSPECT_RESULT::CONTINUE;
                },
                nullptr, GENERAL_COLLECTOR::AllBoardItems );

        dbRA.m_zone = new ZONE( board() );
        //dbRA.m_area->SetZoneName( wxString::Format( wxT( "design-block-source-%s" ), group->GetDesignBlockLibId().GetUniStringLibId() ) );
        dbRA.m_zone->SetIsRuleArea( true );
        dbRA.m_zone->SetLayerSet( LSET::AllCuMask() );
        dbRA.m_zone->SetPlacementAreaEnabled( true );
        dbRA.m_zone->SetDoNotAllowZoneFills( false );
        dbRA.m_zone->SetDoNotAllowVias( false );
        dbRA.m_zone->SetDoNotAllowTracks( false );
        dbRA.m_zone->SetDoNotAllowPads( false );
        dbRA.m_zone->SetDoNotAllowFootprints( false );
        dbRA.m_zone->SetPlacementAreaSourceType( dbRA.m_sourceType );
        dbRA.m_zone->SetPlacementAreaSource( group->GetDesignBlockLibId().GetUniStringLibId() );
        dbRA.m_zone->SetHatchStyle( ZONE_BORDER_DISPLAY_STYLE::NO_HATCH );
        dbRA.m_zone->AddPolygon( generateBoundingBox( dbRA.m_designBlockItems ) );
        dbRA.m_center = dbRA.m_zone->Outline()->COutline( 0 ).Centre();

        // Create the destination rule area for the group
        RULE_AREA destRA;

        destRA.m_sourceType = PLACEMENT_SOURCE_T::GROUP_PLACEMENT;

        // Add all the design block group footprints to the destination rule area
        for( EDA_ITEM* item : group->GetItems() )
        {
            if( item->Type() == PCB_FOOTPRINT_T )
            {
                FOOTPRINT* fp = static_cast<FOOTPRINT*>( item );

                // If the footprint is locked, we can't place it
                if( fp->IsLocked() )
                {
                    wxString msg;
                    msg.Printf( _( "Footprint %s is locked and cannot be placed." ), fp->GetReference() );
                    m_frame->GetInfoBar()->ShowMessageFor( msg, 5000, wxICON_WARNING );
                    return 1;
                }

                destRA.m_components.insert( fp );
            }
        }

        destRA.m_zone = new ZONE( board() );
        destRA.m_zone->SetZoneName( wxString::Format( wxT( "design-block-dest-%s" ),
                                                      group->GetDesignBlockLibId().GetUniStringLibId() ) );
        destRA.m_zone->SetIsRuleArea( true );
        destRA.m_zone->SetLayerSet( LSET::AllCuMask() );
        destRA.m_zone->SetPlacementAreaEnabled( true );
        destRA.m_zone->SetDoNotAllowZoneFills( false );
        destRA.m_zone->SetDoNotAllowVias( false );
        destRA.m_zone->SetDoNotAllowTracks( false );
        destRA.m_zone->SetDoNotAllowPads( false );
        destRA.m_zone->SetDoNotAllowFootprints( false );
        destRA.m_zone->SetPlacementAreaSourceType( destRA.m_sourceType );
        destRA.m_zone->SetPlacementAreaSource( group->GetName() );
        destRA.m_zone->SetHatchStyle( ZONE_BORDER_DISPLAY_STYLE::NO_HATCH );
        destRA.m_zone->AddPolygon( generateBoundingBox( group->GetItems() ) );
        destRA.m_center = destRA.m_zone->Outline()->COutline( 0 ).Centre();

        // Use the multichannel tool to repeat the layout
        MULTICHANNEL_TOOL* mct = m_toolMgr->GetTool<MULTICHANNEL_TOOL>();

        ret = mct->RepeatLayout( aEvent, dbRA, destRA );

        // Get rid of the temporary design blocks and rule areas
        tempCommit.Revert();

        delete dbRA.m_zone;
        delete destRA.m_zone;
    }

    // We're done, remove SKIP_STRUCT
    brd->Visit(
            []( EDA_ITEM* item, void* )
            {
                item->ClearFlags( MCT_SKIP_STRUCT );
                return INSPECT_RESULT::CONTINUE;
            },
            nullptr, GENERAL_COLLECTOR::AllBoardItems );

    return ret;
}

int PCB_CONTROL::PlaceLinkedDesignBlock( const TOOL_EVENT& aEvent )
{
    PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>( m_frame );

    if( !editFrame )
        return 1;

    // Need to have a group selected and it needs to have a linked design block
    PCB_SELECTION_TOOL* selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    PCB_SELECTION       selection = selTool->GetSelection();

    if( selection.Size() != 1 || selection[0]->Type() != PCB_GROUP_T )
        return 1;

    PCB_GROUP* group = static_cast<PCB_GROUP*>( selection[0] );

    if( !group->HasDesignBlockLink() )
        return 1;

    // Get the associated design block
    DESIGN_BLOCK_PANE*            designBlockPane = editFrame->GetDesignBlockPane();
    std::unique_ptr<DESIGN_BLOCK> designBlock( designBlockPane->GetDesignBlock( group->GetDesignBlockLibId(),
                                                                                true, true ) );

    if( !designBlock )
    {
        wxString msg;
        msg.Printf( _( "Could not find design block %s." ), group->GetDesignBlockLibId().GetUniStringLibId() );
        m_frame->GetInfoBar()->ShowMessageFor( msg, 5000, wxICON_WARNING );
        return 1;
    }

    if( designBlock->GetBoardFile().IsEmpty() )
    {
        wxString msg;
        msg.Printf( _( "Design block %s does not have a board file." ),
                    group->GetDesignBlockLibId().GetUniStringLibId() );
        m_frame->GetInfoBar()->ShowMessageFor( msg, 5000, wxICON_WARNING );
        return 1;
    }


    PCB_IO_MGR::PCB_FILE_T pluginType = PCB_IO_MGR::KICAD_SEXP;
    IO_RELEASER<PCB_IO>    pi( PCB_IO_MGR::FindPlugin( pluginType ) );

    if( !pi )
        return 1;

    if( aEvent.Parameter<bool*>() != nullptr )
        return AppendBoard( *pi, designBlock->GetBoardFile(), designBlock.get(),
                            static_cast<BOARD_COMMIT*>( aEvent.Commit() ), *aEvent.Parameter<bool*>() );
    else
        return AppendBoard( *pi, designBlock->GetBoardFile(), designBlock.get() );
}


int PCB_CONTROL::SaveToLinkedDesignBlock( const TOOL_EVENT& aEvent )
{
    PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>( m_frame );

    if( !editFrame )
        return 1;

    // Need to have a group selected and it needs to have a linked design block
    PCB_SELECTION_TOOL* selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    PCB_SELECTION       selection = selTool->GetSelection();

    if( selection.Size() != 1 || selection[0]->Type() != PCB_GROUP_T )
        return 1;

    PCB_GROUP* group = static_cast<PCB_GROUP*>( selection[0] );

    if( !group->HasDesignBlockLink() )
        return 1;

    // Get the associated design block
    DESIGN_BLOCK_PANE*            designBlockPane = editFrame->GetDesignBlockPane();
    std::unique_ptr<DESIGN_BLOCK> designBlock( designBlockPane->GetDesignBlock( group->GetDesignBlockLibId(),
                                                                                true, true ) );

    if( !designBlock )
    {
        wxString msg;
        msg.Printf( _( "Could not find design block %s." ), group->GetDesignBlockLibId().GetUniStringLibId() );
        m_frame->GetInfoBar()->ShowMessageFor( msg, 5000, wxICON_WARNING );
        return 1;
    }

    editFrame->GetDesignBlockPane()->SelectLibId( group->GetDesignBlockLibId() );

    return m_toolMgr->RunAction( PCB_ACTIONS::updateDesignBlockFromSelection ) ? 1 : 0;
}


template<typename T>
static void moveUnflaggedItems( const std::deque<T>& aList, std::vector<BOARD_ITEM*>& aTarget, bool aIsNew )
{
    std::copy_if( aList.begin(), aList.end(), std::back_inserter( aTarget ),
            [aIsNew]( T aItem )
            {
                bool doCopy = ( aItem->GetFlags() & SKIP_STRUCT ) == 0;

                aItem->ClearFlags( SKIP_STRUCT );
                aItem->SetFlags( aIsNew ? IS_NEW : 0 );

                return doCopy;
            } );
}


template<typename T>
static void moveUnflaggedItems( const std::vector<T>& aList, std::vector<BOARD_ITEM*>& aTarget, bool aIsNew )
{
    std::copy_if( aList.begin(), aList.end(), std::back_inserter( aTarget ),
            [aIsNew]( T aItem )
            {
                bool doCopy = ( aItem->GetFlags() & SKIP_STRUCT ) == 0;

                aItem->ClearFlags( SKIP_STRUCT );
                aItem->SetFlags( aIsNew ? IS_NEW : 0 );

                return doCopy;
            } );
}


bool PCB_CONTROL::placeBoardItems( BOARD_COMMIT* aCommit, BOARD* aBoard, bool aAnchorAtOrigin,
                                   bool aReannotateDuplicates, bool aSkipMove )
{
    // items are new if the current board is not the board source
    bool                     isNew = board() != aBoard;
    std::vector<BOARD_ITEM*> items;

    moveUnflaggedItems( aBoard->Tracks(), items, isNew );
    moveUnflaggedItems( aBoard->Footprints(), items, isNew );
    moveUnflaggedItems( aBoard->Drawings(), items, isNew );
    moveUnflaggedItems( aBoard->Zones(), items, isNew );

    // Subtlety: When selecting a group via the mouse,
    // PCB_SELECTION_TOOL::highlightInternal runs, which does a SetSelected() on all
    // descendants. In PCB_CONTROL::placeBoardItems, below, we skip that and
    // mark items non-recursively.  That works because the saving of the
    // selection created aBoard that has the group and all descendants in it.
    moveUnflaggedItems( aBoard->Groups(), items, isNew );

    moveUnflaggedItems( aBoard->Generators(), items, isNew );

    if( isNew )
        aBoard->RemoveAll();

    // Reparent before calling pruneItemLayers, as SetLayer can have a dependence on the
    // item's parent board being set correctly.
    if( isNew )
    {
        for( BOARD_ITEM* item : items )
            item->SetParent( board() );
    }

    pruneItemLayers( items );

    return placeBoardItems( aCommit, items, isNew, aAnchorAtOrigin, aReannotateDuplicates, aSkipMove );
}


bool PCB_CONTROL::placeBoardItems( BOARD_COMMIT* aCommit, std::vector<BOARD_ITEM*>& aItems, bool aIsNew,
                                   bool aAnchorAtOrigin, bool aReannotateDuplicates, bool aSkipMove )
{
    m_toolMgr->RunAction( ACTIONS::selectionClear );

    PCB_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();

    std::vector<BOARD_ITEM*> itemsToSel;
    itemsToSel.reserve( aItems.size() );

    for( BOARD_ITEM* item : aItems )
    {
        if( aIsNew )
        {
            const_cast<KIID&>( item->m_Uuid ) = KIID();

            item->RunOnChildren(
                    []( BOARD_ITEM* aChild )
                    {
                        const_cast<KIID&>( aChild->m_Uuid ) = KIID();
                    },
                    RECURSE_MODE::RECURSE );

            // While BOARD_COMMIT::Push() will add any new items to the entered group,
            // we need to do it earlier so that the previews while moving are correct.
            if( PCB_GROUP* enteredGroup = selectionTool->GetEnteredGroup() )
            {
                if( item->IsGroupableType() && !item->GetParentGroup() )
                {
                    aCommit->Modify( enteredGroup, nullptr, RECURSE_MODE::NO_RECURSE );
                    enteredGroup->AddItem( item );
                }
            }

            item->SetParent( board() );
        }

        // Update item attributes if needed
        if( BaseType( item->Type() ) == PCB_DIMENSION_T )
        {
            static_cast<PCB_DIMENSION_BASE*>( item )->UpdateUnits();
        }
        else if( item->Type() == PCB_FOOTPRINT_T )
        {
            FOOTPRINT* footprint = static_cast<FOOTPRINT*>( item );

            // Update the footprint path with the new KIID path if the footprint is new
            if( aIsNew )
                footprint->SetPath( KIID_PATH() );

            for( BOARD_ITEM* dwg : footprint->GraphicalItems() )
            {
                if( BaseType( dwg->Type() ) == PCB_DIMENSION_T )
                    static_cast<PCB_DIMENSION_BASE*>( dwg )->UpdateUnits();
            }
        }

        // We only need to add the items that aren't inside a group currently selected
        // to the selection. If an item is inside a group and that group is selected,
        // then the selection tool will select it for us.
        if( !item->GetParentGroup() || !alg::contains( aItems, item->GetParentGroup()->AsEdaItem() ) )
            itemsToSel.push_back( item );
    }

    // Select the items that should be selected
    EDA_ITEMS toSel( itemsToSel.begin(), itemsToSel.end() );
    m_toolMgr->RunAction<EDA_ITEMS*>( ACTIONS::selectItems, &toSel );

    // Reannotate duplicate footprints (make sense only in board editor )
    if( aReannotateDuplicates && m_isBoardEditor )
        m_toolMgr->GetTool<BOARD_REANNOTATE_TOOL>()->ReannotateDuplicatesInSelection();

    for( BOARD_ITEM* item : aItems )
    {
        if( aIsNew )
            aCommit->Add( item );
        else
            aCommit->Added( item );
    }

    PCB_SELECTION& selection = selectionTool->GetSelection();

    if( selection.Size() > 0 )
    {
        if( aAnchorAtOrigin )
        {
            selection.SetReferencePoint( VECTOR2I( 0, 0 ) );
        }
        else if( BOARD_ITEM* item = dynamic_cast<BOARD_ITEM*>( selection.GetTopLeftItem() ) )
        {
            selection.SetReferencePoint( item->GetPosition() );
        }

        getViewControls()->SetCursorPosition( getViewControls()->GetMousePosition(), false );

        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

        if( !aSkipMove )
            return m_toolMgr->RunSynchronousAction( PCB_ACTIONS::move, aCommit );
    }

    return true;
}


int PCB_CONTROL::AppendBoard( PCB_IO& pi, const wxString& fileName, DESIGN_BLOCK* aDesignBlock, BOARD_COMMIT* aCommit,
                              bool aSkipMove )
{
    PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>( m_frame );

    if( !editFrame )
        return 1;

    BOARD* brd = board();

    if( !brd )
        return 1;

    // Give ourselves a commit to work with if we weren't provided one
    std::unique_ptr<BOARD_COMMIT> tempCommit;
    BOARD_COMMIT*                 commit = aCommit;

    if( !commit )
    {
        tempCommit = std::make_unique<BOARD_COMMIT>( editFrame );
        commit = tempCommit.get();
    }

    // Mark existing items, in order to know what are the new items so we can select only
    // the new items after loading
    for( PCB_TRACK* track : brd->Tracks() )
        track->SetFlags( SKIP_STRUCT );

    for( FOOTPRINT* footprint : brd->Footprints() )
        footprint->SetFlags( SKIP_STRUCT );

    for( PCB_GROUP* group : brd->Groups() )
        group->SetFlags( SKIP_STRUCT );

    for( BOARD_ITEM* drawing : brd->Drawings() )
        drawing->SetFlags( SKIP_STRUCT );

    for( ZONE* zone : brd->Zones() )
        zone->SetFlags( SKIP_STRUCT );

    for( PCB_GENERATOR* generator : brd->Generators() )
        generator->SetFlags( SKIP_STRUCT );

    std::map<wxString, wxString> oldProperties = brd->GetProperties();
    std::map<wxString, wxString> newProperties;

    PAGE_INFO   oldPageInfo = brd->GetPageSettings();
    TITLE_BLOCK oldTitleBlock = brd->GetTitleBlock();

    // Keep also the count of copper layers, to adjust if necessary
    int  initialCopperLayerCount = brd->GetCopperLayerCount();
    LSET initialEnabledLayers = brd->GetEnabledLayers();

    // Load the data
    try
    {
        std::map<std::string, UTF8> props;

        // PCB_IO_EAGLE can use this info to center the BOARD, but it does not yet.

        props["page_width"] = std::to_string( editFrame->GetPageSizeIU().x );
        props["page_height"] = std::to_string( editFrame->GetPageSizeIU().y );

        pi.SetQueryUserCallback(
                [&]( wxString aTitle, int aIcon, wxString aMessage, wxString aAction ) -> bool
                {
                    KIDIALOG dlg( editFrame, aMessage, aTitle, wxOK | wxCANCEL | aIcon );

                    if( !aAction.IsEmpty() )
                        dlg.SetOKLabel( aAction );

                    dlg.DoNotShowCheckbox( aMessage, 0 );

                    return dlg.ShowModal() == wxID_OK;
                } );

        WX_PROGRESS_REPORTER progressReporter( editFrame, _( "Load PCB" ), 1, PR_CAN_ABORT );

        pi.SetProgressReporter( &progressReporter );
        pi.LoadBoard( fileName, brd, &props, nullptr );
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayErrorMessage( editFrame, _( "Error loading board." ), ioe.What() );

        return 0;
    }

    newProperties = brd->GetProperties();

    for( const std::pair<const wxString, wxString>& prop : oldProperties )
        newProperties[prop.first] = prop.second;

    brd->SetProperties( newProperties );

    brd->SetPageSettings( oldPageInfo );
    brd->SetTitleBlock( oldTitleBlock );

    // rebuild nets and ratsnest before any use of nets
    brd->BuildListOfNets();
    brd->SynchronizeNetsAndNetClasses( true );
    brd->BuildConnectivity();

    // Synchronize layers
    // we should not ask PLUGINs to do these items:
    int copperLayerCount = brd->GetCopperLayerCount();

    if( copperLayerCount > initialCopperLayerCount )
        brd->SetCopperLayerCount( copperLayerCount );

    // Enable all used layers, and make them visible:
    LSET enabledLayers = brd->GetEnabledLayers();
    enabledLayers |= initialEnabledLayers;
    brd->SetEnabledLayers( enabledLayers );
    brd->SetVisibleLayers( enabledLayers );

    int ret = 0;

    bool placeAsGroup = false;

    if( APP_SETTINGS_BASE* cfg = editFrame->config() )
        placeAsGroup = cfg->m_DesignBlockChooserPanel.place_as_group;

    if( placeBoardItems( commit, brd, false, false /* Don't reannotate dupes on Append Board */, aSkipMove ) )
    {
        if( placeAsGroup )
        {
            PCB_GROUP* group = new PCB_GROUP( brd );

            if( aDesignBlock )
            {
                group->SetName( aDesignBlock->GetLibId().GetLibItemName() );
                group->SetDesignBlockLibId( aDesignBlock->GetLibId() );
            }
            else
            {
                group->SetName( wxFileName( fileName ).GetName() );
            }

            // Get the selection tool selection
            PCB_SELECTION_TOOL* selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
            PCB_SELECTION       selection = selTool->GetSelection();

            for( EDA_ITEM* eda_item : selection )
            {
                if( eda_item->IsBOARD_ITEM() )
                {
                    if( static_cast<BOARD_ITEM*>( eda_item )->IsLocked() )
                        group->SetLocked( true );
                }
            }

            commit->Add( group );

            for( EDA_ITEM* eda_item : selection )
            {
                if( eda_item->IsBOARD_ITEM() && !static_cast<BOARD_ITEM*>( eda_item )->GetParentFootprint() )
                {
                    commit->Modify( eda_item );
                    group->AddItem( eda_item );
                }
            }

            selTool->ClearSelection();
            selTool->select( group );

            m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );
            m_frame->OnModify();
            m_frame->Refresh();
        }

        // If we were provided a commit, let the caller control when to push it
        if( !aCommit )
            commit->Push( aDesignBlock ? _( "Place Design Block" ) : _( "Append Board" ) );

        editFrame->GetBoard()->BuildConnectivity();
        ret = 0;
    }
    else
    {
        // If we were provided a commit, let the caller control when to revert it
        if( !aCommit )
            commit->Revert();

        ret = 1;
    }

    // Refresh the UI for the updated board properties
    editFrame->GetAppearancePanel()->OnBoardChanged();

    return ret;
}


int PCB_CONTROL::Undo( const TOOL_EVENT& aEvent )
{
    PCB_BASE_EDIT_FRAME* editFrame = dynamic_cast<PCB_BASE_EDIT_FRAME*>( m_frame );
    wxCommandEvent       dummy;

    if( editFrame )
        editFrame->RestoreCopyFromUndoList( dummy );

    return 0;
}


int PCB_CONTROL::Redo( const TOOL_EVENT& aEvent )
{
    PCB_BASE_EDIT_FRAME* editFrame = dynamic_cast<PCB_BASE_EDIT_FRAME*>( m_frame );
    wxCommandEvent       dummy;

    if( editFrame )
        editFrame->RestoreCopyFromRedoList( dummy );

    return 0;
}


int PCB_CONTROL::SnapMode( const TOOL_EVENT& aEvent )
{
    MAGNETIC_SETTINGS& settings = m_isFootprintEditor ? m_frame->GetFootprintEditorSettings()->m_MagneticItems
                                                      : m_frame->GetPcbNewSettings()->m_MagneticItems;
    bool&              snapMode = settings.allLayers;

    if( aEvent.IsAction( &PCB_ACTIONS::magneticSnapActiveLayer ) )
        snapMode = false;
    else if( aEvent.IsAction( &PCB_ACTIONS::magneticSnapAllLayers ) )
        snapMode = true;
    else
        snapMode = !snapMode;

    m_toolMgr->PostEvent( PCB_EVENTS::SnappingModeChangedByKeyEvent() );

    return 0;
}


int PCB_CONTROL::SnapModeFeedback( const TOOL_EVENT& aEvent )
{
    if( !Pgm().GetCommonSettings()->m_Input.hotkey_feedback )
        return 0;

    wxArrayString labels;
    labels.Add( _( "Active Layer" ) );
    labels.Add( _( "All Layers" ) );

    if( !m_frame->GetHotkeyPopup() )
        m_frame->CreateHotkeyPopup();

    HOTKEY_CYCLE_POPUP* popup = m_frame->GetHotkeyPopup();

    MAGNETIC_SETTINGS& settings = m_isFootprintEditor ? m_frame->GetFootprintEditorSettings()->m_MagneticItems
                                                      : m_frame->GetPcbNewSettings()->m_MagneticItems;

    if( popup )
        popup->Popup( _( "Object Snapping" ), labels, static_cast<int>( settings.allLayers ) );

    return 0;
}


int PCB_CONTROL::UpdateMessagePanel( const TOOL_EVENT& aEvent )
{
    PCB_SELECTION_TOOL*         selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    ROUTER_TOOL*                routerTool = m_toolMgr->GetTool<ROUTER_TOOL>();
    PCB_SELECTION&              selection = selTool->GetSelection();
    PCB_EDIT_FRAME*             pcbFrame = dynamic_cast<PCB_EDIT_FRAME*>( m_frame );
    std::shared_ptr<DRC_ENGINE> drcEngine = m_frame->GetBoard()->GetDesignSettings().m_DRCEngine;
    DRC_CONSTRAINT              constraint;

    std::vector<MSG_PANEL_ITEM> msgItems;

    if( routerTool && routerTool->RoutingInProgress() )
    {
        routerTool->UpdateMessagePanel();
        return 0;
    }

    if( !pcbFrame && !m_frame->GetModel() )
        return 0;

    if( selection.Empty() )
    {
        if( !pcbFrame )
        {
            FOOTPRINT* fp = static_cast<FOOTPRINT*>( m_frame->GetModel() );
            fp->GetMsgPanelInfo( m_frame, msgItems );
        }
        else
        {
            m_frame->SetMsgPanel( m_frame->GetBoard() );
        }
    }
    else if( selection.GetSize() == 1 )
    {
        EDA_ITEM* item = selection.Front();

        if( std::optional<wxString> uuid = GetMsgPanelDisplayUuid( item->m_Uuid ) )
            msgItems.emplace_back( _( "UUID" ), *uuid );

        item->GetMsgPanelInfo( m_frame, msgItems );

        PCB_TRACK*    track = dynamic_cast<PCB_TRACK*>( item );
        NETINFO_ITEM* net = track ? track->GetNet() : nullptr;
        NETINFO_ITEM* coupledNet = net ? m_frame->GetBoard()->DpCoupledNet( net ) : nullptr;

        if( coupledNet )
        {
            SEG         trackSeg( track->GetStart(), track->GetEnd() );
            PCB_TRACK*  coupledItem = nullptr;
            SEG::ecoord closestDist_sq = VECTOR2I::ECOORD_MAX;

            for( PCB_TRACK* candidate : m_frame->GetBoard()->Tracks() )
            {
                if( candidate->GetNet() != coupledNet )
                    continue;

                SEG::ecoord dist_sq = trackSeg.SquaredDistance( SEG( candidate->GetStart(), candidate->GetEnd() ) );

                if( !coupledItem || dist_sq < closestDist_sq )
                {
                    coupledItem = candidate;
                    closestDist_sq = dist_sq;
                }
            }

            constraint = drcEngine->EvalRules( DIFF_PAIR_GAP_CONSTRAINT, track, coupledItem, track->GetLayer() );

            wxString msg = m_frame->MessageTextFromMinOptMax( constraint.Value() );

            if( !msg.IsEmpty() )
            {
                msgItems.emplace_back( wxString::Format( _( "DP Gap Constraints: %s" ), msg ),
                                       wxString::Format( _( "(from %s)" ), constraint.GetName() ) );
            }

            constraint = drcEngine->EvalRules( MAX_UNCOUPLED_CONSTRAINT, track, coupledItem, track->GetLayer() );

            if( constraint.Value().HasMax() )
            {
                msg = m_frame->MessageTextFromValue( constraint.Value().Max() );
                msgItems.emplace_back( wxString::Format( _( "DP Max Uncoupled-length: %s" ), msg ),
                                       wxString::Format( _( "(from %s)" ), constraint.GetName() ) );
            }
        }
    }
    else if( pcbFrame && selection.GetSize() == 2 )
    {
        // Pair selection broken into multiple, optional data, starting with the selected item
        // names

        BOARD_ITEM* a = dynamic_cast<BOARD_ITEM*>( selection[0] );
        BOARD_ITEM* b = dynamic_cast<BOARD_ITEM*>( selection[1] );

        if( a && b )
        {
            msgItems.emplace_back( MSG_PANEL_ITEM( a->GetItemDescription( m_frame, false ),
                                                   b->GetItemDescription( m_frame, false ) ) );
        }

        BOARD_CONNECTED_ITEM* a_conn = dynamic_cast<BOARD_CONNECTED_ITEM*>( a );
        BOARD_CONNECTED_ITEM* b_conn = dynamic_cast<BOARD_CONNECTED_ITEM*>( b );

        if( a_conn && b_conn )
        {
            LSET overlap = a_conn->GetLayerSet() & b_conn->GetLayerSet() & LSET::AllCuMask();
            int  a_netcode = a_conn->GetNetCode();
            int  b_netcode = b_conn->GetNetCode();

            if( overlap.count() > 0 )
            {
                PCB_LAYER_ID layer = overlap.CuStack().front();

                if( a_netcode != b_netcode || a_netcode < 0 || b_netcode < 0 )
                {
                    constraint = drcEngine->EvalRules( CLEARANCE_CONSTRAINT, a, b, layer );
                    msgItems.emplace_back( _( "Resolved Clearance" ),
                                           m_frame->MessageTextFromValue( constraint.m_Value.Min() ) );
                }

                std::shared_ptr<SHAPE> a_shape( a_conn->GetEffectiveShape( layer ) );
                std::shared_ptr<SHAPE> b_shape( b_conn->GetEffectiveShape( layer ) );

                int actual_clearance = a_shape->GetClearance( b_shape.get() );

                if( actual_clearance > -1 && actual_clearance < std::numeric_limits<int>::max() )
                {
                    msgItems.emplace_back( _( "Actual Clearance" ),
                                           m_frame->MessageTextFromValue( actual_clearance ) );
                }
            }
        }

        if( a && b && ( a->HasHole() || b->HasHole() ) )
        {
            PCB_LAYER_ID active = m_frame->GetActiveLayer();
            PCB_LAYER_ID layer = UNDEFINED_LAYER;

            if( b->IsOnLayer( active ) && IsCopperLayer( active ) )
                layer = active;
            else if( b->HasHole() && a->IsOnLayer( active ) && IsCopperLayer( active ) )
                layer = active;
            else if( a->HasHole() && b->IsOnCopperLayer() )
                layer = b->GetLayer();
            else if( b->HasHole() && a->IsOnCopperLayer() )
                layer = a->GetLayer();

            if( IsCopperLayer( layer ) )
            {
                int actual = std::numeric_limits<int>::max();

                if( a->HasHole() && b->IsOnCopperLayer() )
                {
                    std::shared_ptr<SHAPE_SEGMENT> hole = a->GetEffectiveHoleShape();
                    std::shared_ptr<SHAPE>         other( b->GetEffectiveShape( layer ) );

                    actual = std::min( actual, hole->GetClearance( other.get() ) );
                }

                if( b->HasHole() && a->IsOnCopperLayer() )
                {
                    std::shared_ptr<SHAPE_SEGMENT> hole = b->GetEffectiveHoleShape();
                    std::shared_ptr<SHAPE>         other( a->GetEffectiveShape( layer ) );

                    actual = std::min( actual, hole->GetClearance( other.get() ) );
                }

                if( actual < std::numeric_limits<int>::max() )
                {
                    constraint = drcEngine->EvalRules( HOLE_CLEARANCE_CONSTRAINT, a, b, layer );
                    msgItems.emplace_back( _( "Resolved Hole Clearance" ),
                                           m_frame->MessageTextFromValue( constraint.m_Value.Min() ) );

                    if( actual > -1 && actual < std::numeric_limits<int>::max() )
                    {
                        msgItems.emplace_back( _( "Actual Hole Clearance" ),
                                               m_frame->MessageTextFromValue( actual ) );
                    }
                }
            }
        }

        if( a && b )
        {
            for( PCB_LAYER_ID edgeLayer : { Edge_Cuts, Margin } )
            {
                PCB_LAYER_ID active = m_frame->GetActiveLayer();
                PCB_LAYER_ID layer = UNDEFINED_LAYER;

                if( a->IsOnLayer( edgeLayer ) && b->Type() != PCB_FOOTPRINT_T )
                {
                    if( b->IsOnLayer( active ) && IsCopperLayer( active ) )
                        layer = active;
                    else if( IsCopperLayer( b->GetLayer() ) )
                        layer = b->GetLayer();
                }
                else if( b->IsOnLayer( edgeLayer ) && a->Type() != PCB_FOOTPRINT_T )
                {
                    if( a->IsOnLayer( active ) && IsCopperLayer( active ) )
                        layer = active;
                    else if( IsCopperLayer( a->GetLayer() ) )
                        layer = a->GetLayer();
                }

                if( layer >= 0 )
                {
                    constraint = drcEngine->EvalRules( EDGE_CLEARANCE_CONSTRAINT, a, b, layer );

                    if( edgeLayer == Edge_Cuts )
                    {
                        msgItems.emplace_back( _( "Resolved Edge Clearance" ),
                                               m_frame->MessageTextFromValue( constraint.m_Value.Min() ) );
                    }
                    else
                    {
                        msgItems.emplace_back( _( "Resolved Margin Clearance" ),
                                               m_frame->MessageTextFromValue( constraint.m_Value.Min() ) );
                    }
                }
            }
        }
    }

    if( selection.GetSize() )
    {
        if( msgItems.empty() )
        {
            // Count items by type
            std::map<KICAD_T, int> typeCounts;

            for( EDA_ITEM* item : selection )
                typeCounts[item->Type()]++;

            // Check if all items are the same type
            bool allSameType = ( typeCounts.size() == 1 );
            KICAD_T commonType = allSameType ? typeCounts.begin()->first : NOT_USED;

            if( allSameType )
            {
                // Show "Type: N" for homogeneous selections
                wxString typeName = selection.Front()->GetFriendlyName();
                msgItems.emplace_back( typeName,
                                       wxString::Format( wxT( "%d" ), selection.GetSize() ) );

                // For pads, show common properties
                if( commonType == PCB_PAD_T )
                {
                    std::set<wxString> layers;
                    std::set<PAD_SHAPE> shapes;
                    std::set<VECTOR2I>  sizes;

                    for( EDA_ITEM* item : selection )
                    {
                        PAD* pad = static_cast<PAD*>( item );
                        layers.insert( pad->LayerMaskDescribe() );
                        shapes.insert( pad->GetShape( PADSTACK::ALL_LAYERS ) );
                        sizes.insert( pad->GetSize( PADSTACK::ALL_LAYERS ) );
                    }

                    if( layers.size() == 1 )
                        msgItems.emplace_back( _( "Layer" ), *layers.begin() );

                    if( shapes.size() == 1 )
                    {
                        PAD* firstPad = static_cast<PAD*>( selection.Front() );
                        msgItems.emplace_back( _( "Pad Shape" ),
                                               firstPad->ShowPadShape( PADSTACK::ALL_LAYERS ) );
                    }

                    if( sizes.size() == 1 )
                    {
                        VECTOR2I size = *sizes.begin();
                        msgItems.emplace_back( _( "Pad Size" ),
                            wxString::Format( wxT( "%s x %s" ),
                                              m_frame->MessageTextFromValue( size.x ),
                                              m_frame->MessageTextFromValue( size.y ) ) );
                    }
                }
            }
            else
            {
                // Show type breakdown for mixed selections
                wxString breakdown;

                for( const auto& [type, count] : typeCounts )
                {
                    if( !breakdown.IsEmpty() )
                        breakdown += wxT( ", " );

                    // Get friendly name from first item of this type
                    wxString typeName;

                    for( EDA_ITEM* item : selection )
                    {
                        if( item->Type() == type )
                        {
                            typeName = item->GetFriendlyName();
                            break;
                        }
                    }

                    breakdown += wxString::Format( wxT( "%s: %d" ), typeName, count );
                }

                msgItems.emplace_back( _( "Selected Items" ),
                                       wxString::Format( wxT( "%d (%s)" ),
                                                         selection.GetSize(), breakdown ) );
            }

            if( m_isBoardEditor )
            {
                std::set<wxString> netNames;
                std::set<wxString> netClasses;

                for( EDA_ITEM* item : selection )
                {
                    if( BOARD_CONNECTED_ITEM* bci = dynamic_cast<BOARD_CONNECTED_ITEM*>( item ) )
                    {
                        if( !bci->GetNet() || bci->GetNetCode() <= NETINFO_LIST::UNCONNECTED )
                            continue;

                        netNames.insert( UnescapeString( bci->GetNetname() ) );
                        netClasses.insert( UnescapeString( bci->GetEffectiveNetClass()->GetHumanReadableName() ) );

                        if( netNames.size() > 1 && netClasses.size() > 1 )
                            break;
                    }
                }

                if( netNames.size() == 1 )
                    msgItems.emplace_back( _( "Net" ), *netNames.begin() );

                if( netClasses.size() == 1 )
                    msgItems.emplace_back( _( "Resolved Netclass" ), *netClasses.begin() );
            }
        }

        if( selection.GetSize() >= 2 )
        {
            bool   lengthValid = true;
            double selectedLength = 0;

            // Lambda to accumulate track length if item is a track or arc, otherwise mark invalid
            std::function<void( EDA_ITEM* )> accumulateTrackLength;

            accumulateTrackLength =
                    [&]( EDA_ITEM* aItem )
                    {
                        if( aItem->Type() == PCB_TRACE_T || aItem->Type() == PCB_ARC_T )
                        {
                            selectedLength += static_cast<PCB_TRACK*>( aItem )->GetLength();
                        }
                        else if( aItem->Type() == PCB_VIA_T )
                        {
                            // zero 2D length
                        }
                        else if( aItem->Type() == PCB_SHAPE_T )
                        {
                            PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( aItem );

                            if( shape->GetShape() == SHAPE_T::SEGMENT
                                    || shape->GetShape() == SHAPE_T::ARC
                                    || shape->GetShape() == SHAPE_T::BEZIER )
                            {
                                selectedLength += shape->GetLength();
                            }
                            else
                            {
                                lengthValid = false;
                            }
                        }
                        // Use dynamic_cast to include PCB_GENERATORs.
                        else if( PCB_GROUP* group = dynamic_cast<PCB_GROUP*>( aItem ) )
                        {
                            group->RunOnChildren( accumulateTrackLength, RECURSE_MODE::RECURSE );
                        }
                        else
                        {
                            lengthValid = false;
                        }
                    };

            for( EDA_ITEM* item : selection )
            {
                if( lengthValid )
                    accumulateTrackLength( item );
            }

            if( lengthValid )
            {
                msgItems.emplace_back( _( "Selected 2D Length" ),
                                       m_frame->MessageTextFromValue( selectedLength ) );
            }
        }

        if( selection.GetSize() >= 2 && selection.GetSize() < 100 )
        {
            LSET enabledLayers = m_frame->GetBoard()->GetEnabledLayers();
            LSET enabledCopper = LSET::AllCuMask( m_frame->GetBoard()->GetCopperLayerCount() );
            bool areaValid = true;
            bool hasCopper = false;
            bool hasNonCopper = false;

            std::map<PCB_LAYER_ID, SHAPE_POLY_SET> layerPolys;
            SHAPE_POLY_SET                         holes;

            std::function<void( EDA_ITEM* )> accumulateArea;

            accumulateArea =
                    [&]( EDA_ITEM* aItem )
                    {
                        if( aItem->Type() == PCB_FOOTPRINT_T || aItem->Type() == PCB_MARKER_T )
                        {
                            areaValid = false;
                            return;
                        }

                        if( PCB_GROUP* group = dynamic_cast<PCB_GROUP*>( aItem ) )
                        {
                            group->RunOnChildren( accumulateArea, RECURSE_MODE::RECURSE );
                            return;
                        }

                        if( BOARD_ITEM* boardItem = dynamic_cast<BOARD_ITEM*>( aItem ) )
                        {
                            boardItem->RunOnChildren( accumulateArea, RECURSE_MODE::NO_RECURSE );

                            LSET itemLayers = boardItem->GetLayerSet() & enabledLayers;

                            for( PCB_LAYER_ID layer : itemLayers )
                            {
                                boardItem->TransformShapeToPolySet( layerPolys[layer], layer, 0,
                                                                    ARC_LOW_DEF, ERROR_INSIDE );

                                if( enabledCopper.Contains( layer ) )
                                    hasCopper = true;
                                else
                                    hasNonCopper = true;
                            }

                            if( aItem->Type() == PCB_PAD_T && static_cast<PAD*>( aItem )->HasHole() )
                            {
                                static_cast<PAD*>( aItem )->TransformHoleToPolygon( holes, 0, ARC_LOW_DEF,
                                                                                    ERROR_OUTSIDE );
                            }
                            else if( aItem->Type() == PCB_VIA_T )
                            {
                                PCB_VIA* via = static_cast<PCB_VIA*>( aItem );
                                VECTOR2I center = via->GetPosition();
                                int      R = via->GetDrillValue() / 2;

                                TransformCircleToPolygon( holes, center, R, ARC_LOW_DEF, ERROR_OUTSIDE );
                            }
                        }
                    };

            for( EDA_ITEM* item : selection )
            {
                if( areaValid )
                    accumulateArea( item );
            }

            if( areaValid )
            {
                double area = 0.0;

                for( auto& [layer, layerPoly] : layerPolys )
                {
                    // Only subtract holes from copper layers
                    if( enabledCopper.Contains( layer ) )
                        layerPoly.BooleanSubtract( holes );

                    area += layerPoly.Area();
                }

                // Choose appropriate label based on what layers are involved
                wxString areaLabel;

                if( hasCopper && !hasNonCopper )
                    areaLabel = _( "Selected 2D Copper Area" );
                else if( !hasCopper && hasNonCopper )
                    areaLabel = _( "Selected 2D Area" );
                else
                    areaLabel = _( "Selected 2D Total Area" );

                msgItems.emplace_back( areaLabel,
                                       m_frame->MessageTextFromValue( area, true, EDA_DATA_TYPE::AREA ) );
            }
        }
    }
    else
    {
        m_frame->GetBoard()->GetMsgPanelInfo( m_frame, msgItems );
    }

    m_frame->SetMsgPanel( msgItems );

    // Update vertex editor if it exists
    PCB_BASE_EDIT_FRAME* editFrame = dynamic_cast<PCB_BASE_EDIT_FRAME*>( m_frame );
    if( editFrame )
    {
        BOARD_ITEM* selectedItem = ( selection.GetSize() == 1 ) ? dynamic_cast<BOARD_ITEM*>( selection.Front() )
                                                                : nullptr;
        editFrame->UpdateVertexEditorSelection( selectedItem );
    }

    return 0;
}


int PCB_CONTROL::DdAppendBoard( const TOOL_EVENT& aEvent )
{
    wxFileName fileName = wxFileName( *aEvent.Parameter<wxString*>() );

    PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>( m_frame );

    if( !editFrame )
        return 1;

    wxString               filePath = fileName.GetFullPath();
    PCB_IO_MGR::PCB_FILE_T pluginType = PCB_IO_MGR::FindPluginTypeFromBoardPath( filePath );
    IO_RELEASER<PCB_IO>    pi( PCB_IO_MGR::FindPlugin( pluginType ) );

    if( !pi )
        return 1;

    return AppendBoard( *pi, filePath );
}


int PCB_CONTROL::PlaceCharacteristics( const TOOL_EVENT& aEvent )
{
    BOARD_COMMIT commit( this );
    EDA_UNITS    displayUnit = m_frame->GetUserUnits();
    PCB_TABLE*   table = Build_Board_Characteristics_Table( m_frame->GetBoard(), displayUnit );
    table->SetLayer( m_frame->GetActiveLayer() );

    std::vector<BOARD_ITEM*> items;
    items.push_back( table );

    if( placeBoardItems( &commit, items, true, true, false, false ) )
        commit.Push( _( "Place Board Characteristics" ) );
    else
        delete table;

    return 0;
}


int PCB_CONTROL::PlaceStackup( const TOOL_EVENT& aEvent )
{
    BOARD_COMMIT commit( this );
    EDA_UNITS    displayUnit = m_frame->GetUserUnits();

    PCB_TABLE* table = Build_Board_Stackup_Table( m_frame->GetBoard(), displayUnit );
    table->SetLayer( m_frame->GetActiveLayer() );

    std::vector<BOARD_ITEM*> items;
    items.push_back( table );

    if( placeBoardItems( &commit, items, true, true, false, false ) )
        commit.Push( _( "Place Board Stackup Table" ) );
    else
        delete table;

    return 0;
}


int PCB_CONTROL::FlipPcbView( const TOOL_EVENT& aEvent )
{
    view()->SetMirror( !view()->IsMirroredX(), false );
    view()->RecacheAllItems();
    m_frame->GetCanvas()->ForceRefresh();
    m_frame->OnDisplayOptionsChanged();
    return 0;
}


void PCB_CONTROL::rehatchBoardItem( BOARD_ITEM* aItem )
{
    if( aItem->Type() == PCB_SHAPE_T )
    {
        static_cast<PCB_SHAPE*>( aItem )->UpdateHatching();

        if( view() )
            view()->Update( aItem );
    }
}


int PCB_CONTROL::RehatchShapes( const TOOL_EVENT& aEvent )
{
    for( FOOTPRINT* footprint : board()->Footprints() )
        footprint->RunOnChildren( std::bind( &PCB_CONTROL::rehatchBoardItem, this, _1 ), NO_RECURSE );

    for( BOARD_ITEM* item : board()->Drawings() )
        rehatchBoardItem( item );

    return 0;
}


int PCB_CONTROL::CollectAndEmbed3DModels( const TOOL_EVENT& aEvent )
{
    BOARD* brd = board();

    if( !brd )
        return 0;

    PROJECT&           prj = m_frame->Prj();
    S3D_CACHE*         cache = PROJECT_PCB::Get3DCacheManager( &prj );
    FILENAME_RESOLVER* resolver = cache ? cache->GetResolver() : nullptr;

    wxString                           workingPath = prj.GetProjectPath();
    std::vector<const EMBEDDED_FILES*> stack;
    stack.push_back( brd->GetEmbeddedFiles() );

    BOARD_COMMIT commit( m_frame );
    int          embeddedCount = 0;

    for( FOOTPRINT* fp : brd->Footprints() )
    {
        bool fpModified = false;

        for( FP_3DMODEL& model : fp->Models() )
        {
            if( model.m_Filename.StartsWith( FILEEXT::KiCadUriPrefix ) )
                continue;

            wxString fullPath =
                    resolver ? resolver->ResolvePath( model.m_Filename, workingPath, stack )
                             : model.m_Filename;

            wxFileName fname( fullPath );
            wxString   ext = fname.GetExt().Upper();

            if( fname.Exists() )
            {
                if( EMBEDDED_FILES::EMBEDDED_FILE* file =
                            brd->GetEmbeddedFiles()->AddFile( fname, false ) )
                {
                    model.m_Filename = file->GetLink();
                    fpModified = true;
                    embeddedCount++;

                    // Store STEP along with WRL for the OCCT(STEP) exporter.
                    if( ext == "WRL" || ext == "WRZ" )
                    {
                        wxArrayString alts;

                        // Step files
                        alts.Add( wxT( "stp" ) );
                        alts.Add( wxT( "step" ) );
                        alts.Add( wxT( "STP" ) );
                        alts.Add( wxT( "STEP" ) );
                        alts.Add( wxT( "Stp" ) );
                        alts.Add( wxT( "Step" ) );
                        alts.Add( wxT( "stpz" ) );
                        alts.Add( wxT( "stpZ" ) );
                        alts.Add( wxT( "STPZ" ) );

                        for( const auto& alt : alts )
                        {
                            wxFileName altFile( fname.GetPath(),
                                                fname.GetName() + wxT( "." ) + alt );

                            if( altFile.IsOk() && altFile.FileExists() )
                            {
                                brd->GetEmbeddedFiles()->AddFile( altFile, false );
                                break;
                            }
                        }
                    }
                }
            }
        }

        if( fpModified )
            commit.Modify( fp );
    }

    if( embeddedCount > 0 )
    {
        commit.Push( _( "Embed 3D Models" ) );
        wxString msg = wxString::Format( _( "%d 3D model(s) successfully embedded." ), embeddedCount );
        m_frame->GetInfoBar()->ShowMessageFor( msg, 5000 );
    }

    return 0;
}


// clang-format off
void PCB_CONTROL::setTransitions()
{
    Go( &PCB_CONTROL::AddLibrary,           ACTIONS::newLibrary.MakeEvent() );
    Go( &PCB_CONTROL::AddLibrary,           ACTIONS::addLibrary.MakeEvent() );
    Go( &PCB_CONTROL::Print,                ACTIONS::print.MakeEvent() );

    // Footprint library actions
    Go( &PCB_CONTROL::SaveFpToBoard,        PCB_ACTIONS::saveFpToBoard.MakeEvent() );
    Go( &PCB_CONTROL::LoadFpFromBoard,      PCB_ACTIONS::loadFpFromBoard.MakeEvent() );
    Go( &PCB_CONTROL::IterateFootprint,     PCB_ACTIONS::nextFootprint.MakeEvent() );
    Go( &PCB_CONTROL::IterateFootprint,     PCB_ACTIONS::previousFootprint.MakeEvent() );

    // Display modes
    Go( &PCB_CONTROL::TrackDisplayMode,      PCB_ACTIONS::trackDisplayMode.MakeEvent() );
    Go( &PCB_CONTROL::ToggleRatsnest,        PCB_ACTIONS::showRatsnest.MakeEvent() );
    Go( &PCB_CONTROL::ToggleRatsnest,        PCB_ACTIONS::ratsnestLineMode.MakeEvent() );
    Go( &PCB_CONTROL::ViaDisplayMode,        PCB_ACTIONS::viaDisplayMode.MakeEvent() );
    Go( &PCB_CONTROL::ZoneDisplayMode,       PCB_ACTIONS::zoneDisplayFilled.MakeEvent() );
    Go( &PCB_CONTROL::ZoneDisplayMode,       PCB_ACTIONS::zoneDisplayOutline.MakeEvent() );
    Go( &PCB_CONTROL::ZoneDisplayMode,       PCB_ACTIONS::zoneDisplayFractured.MakeEvent() );
    Go( &PCB_CONTROL::ZoneDisplayMode,       PCB_ACTIONS::zoneDisplayTriangulated.MakeEvent() );
    Go( &PCB_CONTROL::ZoneDisplayMode,       PCB_ACTIONS::zoneDisplayToggle.MakeEvent() );
    Go( &PCB_CONTROL::HighContrastMode,      ACTIONS::highContrastMode.MakeEvent() );
    Go( &PCB_CONTROL::HighContrastModeCycle, ACTIONS::highContrastModeCycle.MakeEvent() );
    Go( &PCB_CONTROL::ContrastModeFeedback,  EVENTS::ContrastModeChangedByKeyEvent );
    Go( &PCB_CONTROL::NetColorModeCycle,     PCB_ACTIONS::netColorModeCycle.MakeEvent() );
    Go( &PCB_CONTROL::RatsnestModeCycle,     PCB_ACTIONS::ratsnestModeCycle.MakeEvent() );
    Go( &PCB_CONTROL::FlipPcbView,           PCB_ACTIONS::flipBoard.MakeEvent() );
    Go( &PCB_CONTROL::RehatchShapes,         PCB_ACTIONS::rehatchShapes.MakeEvent() );

    // Layer control
    Go( &PCB_CONTROL::LayerSwitch,          PCB_ACTIONS::layerTop.MakeEvent() );
    Go( &PCB_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner1.MakeEvent() );
    Go( &PCB_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner2.MakeEvent() );
    Go( &PCB_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner3.MakeEvent() );
    Go( &PCB_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner4.MakeEvent() );
    Go( &PCB_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner5.MakeEvent() );
    Go( &PCB_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner6.MakeEvent() );
    Go( &PCB_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner7.MakeEvent() );
    Go( &PCB_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner8.MakeEvent() );
    Go( &PCB_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner9.MakeEvent() );
    Go( &PCB_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner10.MakeEvent() );
    Go( &PCB_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner11.MakeEvent() );
    Go( &PCB_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner12.MakeEvent() );
    Go( &PCB_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner13.MakeEvent() );
    Go( &PCB_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner14.MakeEvent() );
    Go( &PCB_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner15.MakeEvent() );
    Go( &PCB_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner16.MakeEvent() );
    Go( &PCB_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner17.MakeEvent() );
    Go( &PCB_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner18.MakeEvent() );
    Go( &PCB_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner19.MakeEvent() );
    Go( &PCB_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner20.MakeEvent() );
    Go( &PCB_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner21.MakeEvent() );
    Go( &PCB_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner22.MakeEvent() );
    Go( &PCB_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner23.MakeEvent() );
    Go( &PCB_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner24.MakeEvent() );
    Go( &PCB_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner25.MakeEvent() );
    Go( &PCB_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner26.MakeEvent() );
    Go( &PCB_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner27.MakeEvent() );
    Go( &PCB_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner28.MakeEvent() );
    Go( &PCB_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner29.MakeEvent() );
    Go( &PCB_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner30.MakeEvent() );
    Go( &PCB_CONTROL::LayerSwitch,          PCB_ACTIONS::layerBottom.MakeEvent() );
    Go( &PCB_CONTROL::LayerNext,            PCB_ACTIONS::layerNext.MakeEvent() );
    Go( &PCB_CONTROL::LayerPrev,            PCB_ACTIONS::layerPrev.MakeEvent() );
    Go( &PCB_CONTROL::LayerToggle,          PCB_ACTIONS::layerToggle.MakeEvent() );
    Go( &PCB_CONTROL::LayerAlphaInc,        PCB_ACTIONS::layerAlphaInc.MakeEvent() );
    Go( &PCB_CONTROL::LayerAlphaDec,        PCB_ACTIONS::layerAlphaDec.MakeEvent() );

    Go( &PCB_CONTROL::CycleLayerPresets,    PCB_ACTIONS::layerPairPresetsCycle.MakeEvent() );
    Go( &PCB_CONTROL::LayerPresetFeedback,  PCB_EVENTS::LayerPairPresetChangedByKeyEvent() );

    // Grid control
    Go( &PCB_CONTROL::GridPlaceOrigin,      ACTIONS::gridSetOrigin.MakeEvent() );
    Go( &PCB_CONTROL::GridResetOrigin,      ACTIONS::gridResetOrigin.MakeEvent() );

    Go( &PCB_CONTROL::Undo,                 ACTIONS::undo.MakeEvent() );
    Go( &PCB_CONTROL::Redo,                 ACTIONS::redo.MakeEvent() );

    // Snapping control
    Go( &PCB_CONTROL::SnapMode,             PCB_ACTIONS::magneticSnapActiveLayer.MakeEvent() );
    Go( &PCB_CONTROL::SnapMode,             PCB_ACTIONS::magneticSnapAllLayers.MakeEvent() );
    Go( &PCB_CONTROL::SnapMode,             PCB_ACTIONS::magneticSnapToggle.MakeEvent() );
    Go( &PCB_CONTROL::SnapModeFeedback,     PCB_EVENTS::SnappingModeChangedByKeyEvent() );

    // Miscellaneous
    Go( &PCB_CONTROL::InteractiveDelete,       ACTIONS::deleteTool.MakeEvent() );
    Go( &PCB_CONTROL::CollectAndEmbed3DModels, PCB_ACTIONS::collect3DModels.MakeEvent() );

    // Append control
    Go( &PCB_CONTROL::AppendDesignBlock,    PCB_ACTIONS::placeDesignBlock.MakeEvent() );
    Go( &PCB_CONTROL::ApplyDesignBlockLayout, PCB_ACTIONS::applyDesignBlockLayout.MakeEvent() );
    Go( &PCB_CONTROL::PlaceLinkedDesignBlock, PCB_ACTIONS::placeLinkedDesignBlock.MakeEvent() );
    Go( &PCB_CONTROL::SaveToLinkedDesignBlock, PCB_ACTIONS::saveToLinkedDesignBlock.MakeEvent() );
    Go( &PCB_CONTROL::AppendBoardFromFile,  PCB_ACTIONS::appendBoard.MakeEvent() );
    Go( &PCB_CONTROL::DdAppendBoard,        PCB_ACTIONS::ddAppendBoard.MakeEvent() );
    Go( &PCB_CONTROL::PlaceCharacteristics, PCB_ACTIONS::placeCharacteristics.MakeEvent() );
    Go( &PCB_CONTROL::PlaceStackup,         PCB_ACTIONS::placeStackup.MakeEvent() );

    Go( &PCB_CONTROL::Paste,                ACTIONS::paste.MakeEvent() );
    Go( &PCB_CONTROL::Paste,                ACTIONS::pasteSpecial.MakeEvent() );

    Go( &PCB_CONTROL::UpdateMessagePanel,   EVENTS::PointSelectedEvent );
    Go( &PCB_CONTROL::UpdateMessagePanel,   EVENTS::SelectedEvent );
    Go( &PCB_CONTROL::UpdateMessagePanel,   EVENTS::UnselectedEvent );
    Go( &PCB_CONTROL::UpdateMessagePanel,   EVENTS::ClearedEvent );
    Go( &PCB_CONTROL::UpdateMessagePanel,   EVENTS::SelectedItemsModified );
    Go( &PCB_CONTROL::UpdateMessagePanel,   EVENTS::ConnectivityChangedEvent );

    // Add library by dropping file
    Go( &PCB_CONTROL::DdAddLibrary,         ACTIONS::ddAddLibrary.MakeEvent() );
    Go( &PCB_CONTROL::DdImportFootprint,    PCB_ACTIONS::ddImportFootprint.MakeEvent() );
}
// clang-format on
