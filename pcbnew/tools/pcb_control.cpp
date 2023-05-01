/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2016 CERN
 * Copyright (C) 2019-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <tools/edit_tool.h>
#include <tools/board_inspection_tool.h>
#include <router/router_tool.h>
#include <pgm_base.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_control.h>
#include <tools/pcb_picker_tool.h>
#include <tools/pcb_selection_tool.h>
#include <tools/board_reannotate_tool.h>
#include <3d_viewer/eda_3d_viewer_frame.h>
#include <board_commit.h>
#include <board.h>
#include <board_design_settings.h>
#include <board_item.h>
#include <dialogs/dialog_paste_special.h>
#include <pcb_dimension.h>
#include <footprint.h>
#include <pcb_group.h>
#include <pcb_textbox.h>
#include <pcb_track.h>
#include <wildcards_and_files_ext.h>
#include <zone.h>
#include <confirm.h>
#include <connectivity/connectivity_data.h>
#include <core/kicad_algo.h>
#include <kicad_clipboard.h>
#include <origin_viewitem.h>
#include <pcb_edit_frame.h>
#include <pcb_painter.h>
#include <string_utf8_map.h>
#include <settings/color_settings.h>
#include <string>
#include <tool/tool_manager.h>
#include <footprint_edit_frame.h>
#include <widgets/wx_progress_reporters.h>
#include <widgets/wx_infobar.h>
#include <wx/hyperlink.h>

using namespace std::placeholders;


// files.cpp
extern bool AskLoadBoardFileName( PCB_EDIT_FRAME* aParent, int* aCtl, wxString* aFileName,
                                  bool aKicadFilesOnly = false );
extern IO_MGR::PCB_FILE_T plugin_type( const wxString& aFileName, int aCtl );


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
        m_gridOrigin->SetColor( m_frame->GetGridColor() );
        getView()->Remove( m_gridOrigin.get() );
        getView()->Add( m_gridOrigin.get() );
    }
}


int PCB_CONTROL::AddLibrary( const TOOL_EVENT& aEvent )
{
    if( m_frame->IsType( FRAME_FOOTPRINT_EDITOR ) || m_frame->IsType( FRAME_PCB_EDITOR ) )
    {
        if( aEvent.IsAction( &ACTIONS::newLibrary ) )
            static_cast<PCB_BASE_EDIT_FRAME*>( m_frame )->CreateNewLibrary();
        else if( aEvent.IsAction( &ACTIONS::addLibrary ) )
            static_cast<PCB_BASE_EDIT_FRAME*>( m_frame )->AddLibrary();
    }

    return 0;
}


int PCB_CONTROL::DdAddLibrary( const TOOL_EVENT& aEvent )
{
    const wxString fn = *aEvent.Parameter<wxString*>();
    static_cast<PCB_BASE_EDIT_FRAME*>( m_frame )->AddLibrary( fn );
    return 0;
}


int PCB_CONTROL::DdImportFootprint( const TOOL_EVENT& aEvent )
{
    const wxString fn = *aEvent.Parameter<wxString*>();
    static_cast<FOOTPRINT_EDIT_FRAME*>( m_frame )->ImportFootprint( fn );
    m_frame->Zoom_Automatique( false );
    return 0;
}


int PCB_CONTROL::Quit( const TOOL_EVENT& aEvent )
{
    m_frame->Close( false );
    return 0;
}


template<class T> void Flip( T& aValue )
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

    canvas()->Refresh();

    return 0;
}


int PCB_CONTROL::ToggleRatsnest( const TOOL_EVENT& aEvent )
{
    if( aEvent.IsAction( &PCB_ACTIONS::showRatsnest ) )
    {
        // N.B. Do not disable the Ratsnest layer here.  We use it for local ratsnest
        Flip( displayOptions().m_ShowGlobalRatsnest );
        getEditFrame<PCB_EDIT_FRAME>()->SetElementVisibility( LAYER_RATSNEST,
                                                              displayOptions().m_ShowGlobalRatsnest );

    }
    else if( aEvent.IsAction( &PCB_ACTIONS::ratsnestLineMode ) )
    {
        Flip( displayOptions().m_DisplayRatsnestLinesCurved );
    }

    frame()->OnDisplayOptionsChanged();

    canvas()->RedrawRatsnest();
    canvas()->Refresh();

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
        WX_INFOBAR*      infobar = frame()->GetInfoBar();
        wxHyperlinkCtrl* button = new wxHyperlinkCtrl( infobar, wxID_ANY, _( "Don't show again" ),
                                                       wxEmptyString );

        button->Bind( wxEVT_COMMAND_HYPERLINK, std::function<void( wxHyperlinkEvent& aEvent )>(
                [&]( wxHyperlinkEvent& aEvent )
                {
                    Pgm().GetCommonSettings()->m_DoNotShowAgain.zone_fill_warning = true;
                    frame()->GetInfoBar()->Dismiss();
                } ) );

        infobar->RemoveAllButtons();
        infobar->AddButton( button );

        wxString msg;
        msg.Printf( _( "Not all zones are filled. Use Edit > Fill All Zones (%s) "
                       "if you wish to see all fills." ),
                    KeyNameFromKeyCode( PCB_ACTIONS::zoneFillAll.GetHotKey() ) );

        infobar->ShowMessageFor( msg, 5000, wxICON_WARNING  );
    }
}


int PCB_CONTROL::ZoneDisplayMode( const TOOL_EVENT& aEvent )
{
    PCB_DISPLAY_OPTIONS opts = frame()->GetDisplayOptions();

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
    PCB_DISPLAY_OPTIONS opts = frame()->GetDisplayOptions();

    opts.m_ContrastModeDisplay = opts.m_ContrastModeDisplay == HIGH_CONTRAST_MODE::NORMAL
                                        ? HIGH_CONTRAST_MODE::DIMMED
                                        : HIGH_CONTRAST_MODE::NORMAL;

    m_frame->SetDisplayOptions( opts );

    return 0;
}


int PCB_CONTROL::HighContrastModeCycle( const TOOL_EVENT& aEvent )
{
    PCB_DISPLAY_OPTIONS opts = frame()->GetDisplayOptions();

    switch( opts.m_ContrastModeDisplay )
    {
    case HIGH_CONTRAST_MODE::NORMAL: opts.m_ContrastModeDisplay = HIGH_CONTRAST_MODE::DIMMED; break;
    case HIGH_CONTRAST_MODE::DIMMED: opts.m_ContrastModeDisplay = HIGH_CONTRAST_MODE::HIDDEN; break;
    case HIGH_CONTRAST_MODE::HIDDEN: opts.m_ContrastModeDisplay = HIGH_CONTRAST_MODE::NORMAL; break;
    }

    m_frame->SetDisplayOptions( opts );

    return 0;
}


int PCB_CONTROL::NetColorModeCycle( const TOOL_EVENT& aEvent )
{
    PCB_DISPLAY_OPTIONS opts = frame()->GetDisplayOptions();

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

    getEditFrame<PCB_EDIT_FRAME>()->SetElementVisibility( LAYER_RATSNEST,
                                                          displayOptions().m_ShowGlobalRatsnest );

    frame()->OnDisplayOptionsChanged();

    canvas()->RedrawRatsnest();
    canvas()->Refresh();
    return 0;
}


int PCB_CONTROL::LayerSwitch( const TOOL_EVENT& aEvent )
{
    m_frame->SwitchLayer( aEvent.Parameter<PCB_LAYER_ID>() );

    return 0;
}


int PCB_CONTROL::LayerNext( const TOOL_EVENT& aEvent )
{
    PCB_BASE_FRAME* editFrame  = m_frame;
    BOARD*          brd        = board();
    int             layer      = editFrame->GetActiveLayer();
    int             startLayer = layer;

    while( startLayer != ++layer )
    {
        if( brd->IsLayerVisible( static_cast<PCB_LAYER_ID>( layer ) ) && IsCopperLayer( layer ) )
            break;

        if( layer >= B_Cu )
            layer = F_Cu - 1;
    }

    wxCHECK( IsCopperLayer( layer ), 0 );
    editFrame->SwitchLayer( ToLAYER_ID( layer ) );

    return 0;
}


int PCB_CONTROL::LayerPrev( const TOOL_EVENT& aEvent )
{
    PCB_BASE_FRAME* editFrame  = m_frame;
    BOARD*          brd        = board();
    int             layer      = editFrame->GetActiveLayer();
    int             startLayer = layer;

    while( startLayer != --layer )
    {
        if( IsCopperLayer( layer )       // also test for valid layer id (layer >= F_Cu)
                && brd->IsLayerVisible( static_cast<PCB_LAYER_ID>( layer ) ) )
        {
            break;
        }

        if( layer <= F_Cu )
            layer = B_Cu + 1;
    }

    wxCHECK( IsCopperLayer( layer ), 0 );
    editFrame->SwitchLayer( ToLAYER_ID( layer ) );

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
    }
    else
    {
        wxBell();
    }

    return 0;
}


void PCB_CONTROL::DoSetGridOrigin( KIGFX::VIEW* aView, PCB_BASE_FRAME* aFrame,
                                   EDA_ITEM* originViewItem, const VECTOR2D& aPoint )
{
    aFrame->GetDesignSettings().SetGridOrigin( VECTOR2I( aPoint ) );
    aView->GetGAL()->SetGridOrigin( aPoint );
    originViewItem->SetPosition( aPoint );
    aView->MarkDirty();
    aFrame->OnModify();
}


int PCB_CONTROL::GridSetOrigin( const TOOL_EVENT& aEvent )
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

        picker->SetClickHandler(
                [this]( const VECTOR2D& pt ) -> bool
                {
                    m_frame->SaveCopyInUndoList( m_gridOrigin.get(), UNDO_REDO::GRIDORIGIN );
                    DoSetGridOrigin( getView(), m_frame, m_gridOrigin.get(), pt );
                    return false;   // drill origin is a one-shot; don't continue with tool
                } );

        m_toolMgr->RunAction( ACTIONS::pickerTool, true, (void*) &aEvent );
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


int PCB_CONTROL::DeleteItemCursor( const TOOL_EVENT& aEvent )
{
    if( m_isFootprintEditor && !m_frame->GetBoard()->GetFirstFootprint() )
        return 0;

    PCB_PICKER_TOOL* picker = m_toolMgr->GetTool<PCB_PICKER_TOOL>();

    m_pickerItem = nullptr;
    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    // Deactivate other tools; particularly important if another PICKER is currently running
    Activate();

    picker->SetCursor( KICURSOR::REMOVE );

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
                        m_statusPopup->Move( wxGetMousePosition() + wxPoint( 20, 20 ) );
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
                    if( !selectionTool->Selectable( collector[ i ] ) )
                        collector.Remove( i );
                }

                if( collector.GetCount() > 1 )
                    selectionTool->GuessSelectionCandidates( collector, aPos );

                BOARD_ITEM* item = collector.GetCount() == 1 ? collector[ 0 ] : nullptr;

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

    m_toolMgr->RunAction( ACTIONS::pickerTool, true, (void*) &aEvent );

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

    // Not all graphic items can be added to the current footprint:
    // Reference and value are already existing in the current footprint, and must be unique.
    // So they will be skipped
    for( BOARD_ITEM* item : aClipFootprint->GraphicalItems() )
    {
        if( item->Type() == PCB_TEXT_T )
        {
            PCB_TEXT* text = static_cast<PCB_TEXT*>( item );

            if( text->GetType() != PCB_TEXT::TEXT_is_DIVERS )
                continue;

            text->SetTextAngle( text->GetTextAngle() - aClipFootprint->GetOrientation() );
            text->SetTextAngle( text->GetTextAngle() + editorFootprint->GetOrientation() );
        }

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
    if( m_isFootprintEditor || frame()->IsType( FRAME_FOOTPRINT_EDITOR ) )
        return;

    LSET                     enabledLayers = board()->GetEnabledLayers();
    std::vector<BOARD_ITEM*> returnItems;
    bool                     fpItemDeleted = false;

    auto processFPItem =
            [&]( FOOTPRINT* aFootprint, BOARD_ITEM* aItem )
            {
                LSET allowed = aItem->GetLayerSet() & enabledLayers;

                if( allowed.any() )
                {
                    // Don't prune internal copper layers on items with holes
                    if( aItem->HasHole() && aItem->IsOnCopperLayer() )
                        allowed |= LSET::InternalCuMask();

                    aItem->SetLayerSet( allowed );
                }
                else
                {
                    aFootprint->Remove( aItem );
                    fpItemDeleted = true;
                }
            };

    for( BOARD_ITEM* item : aItems )
    {

        if( item->Type() == PCB_FOOTPRINT_T )
        {
            FOOTPRINT* fp = static_cast<FOOTPRINT*>( item );

            if( !enabledLayers.test( fp->Reference().GetLayer() ) )
                fp->Reference().SetLayer( fp->IsFlipped() ? B_SilkS : F_SilkS );

            if( !enabledLayers.test( fp->Value().GetLayer() ) )
                fp->Value().SetLayer( fp->IsFlipped() ? B_Fab : F_Fab );

            // NOTE: all traversals from the back as processFPItem() might delete the item

            for( int ii = static_cast<int>( fp->Pads().size() ) - 1; ii >= 0; ii-- )
                processFPItem( fp, fp->Pads()[ii] );

            for( int ii = static_cast<int>( fp->Zones().size() ) - 1; ii >= 0; ii-- )
                processFPItem( fp, fp->Zones()[ii] );

            for( int ii = static_cast<int>( fp->GraphicalItems().size() ) - 1; ii >= 0; ii-- )
                processFPItem( fp, fp->GraphicalItems()[ii] );

            if( fp->GraphicalItems().size() || fp->Pads().size() || fp->Zones().size() )
                returnItems.push_back( fp );
        }
        else if( item->Type() == PCB_GROUP_T )
        {
            returnItems.push_back( item );
        }
        else
        {
            LSET allowed = item->GetLayerSet() & enabledLayers;

            if( allowed.any() )
            {
                item->SetLayerSet( allowed );
                returnItems.push_back( item );
            }
        }
    }

    if( ( returnItems.size() < aItems.size() ) || fpItemDeleted )
    {
        DisplayError( m_frame, _( "Warning: some pasted items were on layers which are not "
                                  "present in the current board.\n"
                                  "These items could not be pasted.\n" )  );
    }

    aItems = returnItems;
}


int PCB_CONTROL::Paste( const TOOL_EVENT& aEvent )
{
    CLIPBOARD_IO pi;
    BOARD_ITEM* clipItem = pi.Parse();

    if( !clipItem )
        return 0;

    // The viewer frames cannot paste
    if( !frame()->IsType( FRAME_FOOTPRINT_EDITOR ) && !frame()->IsType( FRAME_PCB_EDITOR ) )
        return 0;

    PASTE_MODE     mode = PASTE_MODE::KEEP_ANNOTATIONS;
    const wxString defaultRef = wxT( "REF**" );

    if( aEvent.IsAction( &ACTIONS::pasteSpecial ) )
    {
        DIALOG_PASTE_SPECIAL dlg( m_frame, &mode, defaultRef );

        if( dlg.ShowModal() == wxID_CANCEL )
            return 0;
    }

    bool isFootprintEditor = m_isFootprintEditor || frame()->IsType( FRAME_FOOTPRINT_EDITOR );

    if( clipItem->Type() == PCB_T )
    {
        if( isFootprintEditor )
        {
            for( BOARD_CONNECTED_ITEM* item : static_cast<BOARD*>( clipItem )->AllConnectedItems() )
                item->SetNet( NETINFO_LIST::OrphanedItem() );
        }
        else
        {
            static_cast<BOARD*>( clipItem )->MapNets( m_frame->GetBoard() );
        }
    }

    // The clipboard can contain two different things, an entire kicad_pcb or a single footprint
    if( isFootprintEditor && ( !board() || !footprint() ) )
    {
        return 0;
    }

    switch( clipItem->Type() )
    {
        case PCB_T:
        {
            BOARD* clipBoard = static_cast<BOARD*>( clipItem );

            if( isFootprintEditor )
            {
                FOOTPRINT* editorFootprint = board()->GetFirstFootprint();
                std::vector<BOARD_ITEM*> pastedItems;

                for( PCB_GROUP* group : clipBoard->Groups() )
                {
                    group->SetParent( editorFootprint );
                    pastedItems.push_back( group );
                }

                clipBoard->Groups().clear();

                for( FOOTPRINT* clipFootprint : clipBoard->Footprints() )
                    pasteFootprintItemsToFootprintEditor( clipFootprint, board(), pastedItems );

                for( BOARD_ITEM* clipDrawItem : clipBoard->Drawings() )
                {
                    switch( clipDrawItem->Type() )
                    {
                    case PCB_TEXT_T:
                    case PCB_TEXTBOX_T:
                    case PCB_SHAPE_T:
                    case PCB_DIM_ALIGNED_T:
                    case PCB_DIM_CENTER_T:
                    case PCB_DIM_LEADER_T:
                    case PCB_DIM_ORTHOGONAL_T:
                    case PCB_DIM_RADIAL_T:
                        clipDrawItem->SetParent( editorFootprint );
                        pastedItems.push_back( clipDrawItem );
                        break;

                    default:
                        if( PCB_GROUP* parentGroup = clipDrawItem->GetParentGroup() )
                            parentGroup->RemoveItem( clipDrawItem );

                        break;
                    }
                }

                clipBoard->Drawings().clear();

                clipBoard->Visit(
                        [&]( EDA_ITEM* item, void* testData )
                        {
                            // Anything still on the clipboard didn't get copied and needs to be
                            // removed from the pasted groups.
                            BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( item );
                            PCB_GROUP*  parentGroup = boardItem->GetParentGroup();

                            if( parentGroup )
                                parentGroup->RemoveItem( boardItem );

                            return INSPECT_RESULT::CONTINUE;
                        },
                        nullptr, GENERAL_COLLECTOR::AllBoardItems );

                delete clipBoard;

                pruneItemLayers( pastedItems );

                placeBoardItems( pastedItems, true, true, mode == PASTE_MODE::UNIQUE_ANNOTATIONS );
            }
            else
            {
                if( mode == PASTE_MODE::REMOVE_ANNOTATIONS )
                {
                    for( FOOTPRINT* clipFootprint : clipBoard->Footprints() )
                        clipFootprint->SetReference( defaultRef );
                }

                placeBoardItems( clipBoard, true, mode == PASTE_MODE::UNIQUE_ANNOTATIONS );
            }

            break;
        }

        case PCB_FOOTPRINT_T:
        {
            FOOTPRINT* clipFootprint = static_cast<FOOTPRINT*>( clipItem );
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
                pastedItems.push_back( clipFootprint );
            }

            pruneItemLayers( pastedItems );

            placeBoardItems( pastedItems, true, true, mode == PASTE_MODE::UNIQUE_ANNOTATIONS );
            break;
        }

        default:
            m_frame->DisplayToolMsg( _( "Invalid clipboard contents" ) );
            break;
    }

    return 1;
}


int PCB_CONTROL::AppendBoardFromFile( const TOOL_EVENT& aEvent )
{
    int open_ctl;
    wxString fileName;

    PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>( m_frame );

    if( !editFrame )
        return 1;

    // Pick a file to append
    if( !AskLoadBoardFileName( editFrame, &open_ctl, &fileName, true ) )
        return 1;

    IO_MGR::PCB_FILE_T pluginType = plugin_type( fileName, open_ctl );
    PLUGIN::RELEASER pi( IO_MGR::PluginFind( pluginType ) );

    return AppendBoard( *pi, fileName );
}


// Helper function for PCB_CONTROL::placeBoardItems()
template<typename T>
static void moveUnflaggedItems( std::deque<T>& aList, std::vector<BOARD_ITEM*>& aTarget,
                                bool aIsNew )
{
    std::copy_if( aList.begin(), aList.end(), std::back_inserter( aTarget ),
            [aIsNew]( T aItem )
            {
                bool doCopy = ( aItem->GetFlags() & SKIP_STRUCT ) == 0;

                aItem->ClearFlags( SKIP_STRUCT );
                aItem->SetFlags( aIsNew ? IS_NEW : 0 );

                return doCopy;
            } );

    if( aIsNew )
        aList.clear();
}


static void moveUnflaggedItems( ZONES& aList, std::vector<BOARD_ITEM*>& aTarget, bool aIsNew )
{
    if( aList.size() == 0 )
        return;

    auto obj = aList.front();
    int idx = 0;

    if( aIsNew )
    {
        obj = aList.back();
        aList.pop_back();
    }

    for( ; obj ; )
    {
        if( obj->HasFlag( SKIP_STRUCT ) )
            obj->ClearFlags( SKIP_STRUCT );
        else
            aTarget.push_back( obj );

        if( aIsNew )
        {
            if( aList.size() )
            {
                obj = aList.back();
                aList.pop_back();
            }
            else
            {
                obj = nullptr;
            }
        }
        else
        {
            obj = idx < int(aList.size()-1) ? aList[++idx] : nullptr;
        }
    }
}



int PCB_CONTROL::placeBoardItems( BOARD* aBoard, bool aAnchorAtOrigin, bool aReannotateDuplicates )
{
    // items are new if the current board is not the board source
    bool isNew = board() != aBoard;
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

    pruneItemLayers( items );

    return placeBoardItems( items, isNew, aAnchorAtOrigin, aReannotateDuplicates );
}


int PCB_CONTROL::placeBoardItems( std::vector<BOARD_ITEM*>& aItems, bool aIsNew,
                                  bool aAnchorAtOrigin, bool aReannotateDuplicates )
{
    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    PCB_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    EDIT_TOOL*          editTool = m_toolMgr->GetTool<EDIT_TOOL>();

    std::vector<BOARD_ITEM*> itemsToSel;
    itemsToSel.reserve( aItems.size() );

    for( BOARD_ITEM* item : aItems )
    {
        if( aIsNew )
        {
            const_cast<KIID&>( item->m_Uuid ) = KIID();

            // Even though BOARD_COMMIT::Push() will add any new items to the group, we're
            // going to run PCB_ACTIONS::move first, and the move tool will throw out any
            // items that aren't in the entered group.
            if( selectionTool->GetEnteredGroup() && !item->GetParentGroup() )
                selectionTool->GetEnteredGroup()->AddItem( item );
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
        if( !item->GetParentGroup() || !alg::contains( aItems, item->GetParentGroup() ) )
            itemsToSel.push_back( item );
    }

    // Select the items that should be selected
    m_toolMgr->RunAction( PCB_ACTIONS::selectItems, true, &itemsToSel );

    // Reannotate duplicate footprints (make sense only in board editor )
    if( aReannotateDuplicates && m_isBoardEditor )
        m_toolMgr->GetTool<BOARD_REANNOTATE_TOOL>()->ReannotateDuplicatesInSelection();

    for( BOARD_ITEM* item : aItems )
    {
        // Commit after reannotation
        if( aIsNew )
            editTool->GetCurrentCommit()->Add( item );
        else
            editTool->GetCurrentCommit()->Added( item );
    }

    PCB_SELECTION& selection = selectionTool->GetSelection();

    if( selection.Size() > 0 )
    {
        if( aAnchorAtOrigin )
        {
            selection.SetReferencePoint( VECTOR2I( 0, 0 ) );
        }
        else
        {
            BOARD_ITEM* item = static_cast<BOARD_ITEM*>( selection.GetTopLeftItem() );
            selection.SetReferencePoint( item->GetPosition() );
        }

        getViewControls()->SetCursorPosition( getViewControls()->GetMousePosition(), false );

        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );
        m_toolMgr->RunAction( PCB_ACTIONS::move, true );
    }

    return 0;
}


int PCB_CONTROL::AppendBoard( PLUGIN& pi, wxString& fileName )
{
    PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>( m_frame );

    if( !editFrame )
        return 1;

    BOARD* brd = board();

    if( !brd )
        return 1;

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

    std::map<wxString, wxString> oldProperties = brd->GetProperties();
    std::map<wxString, wxString> newProperties;

    PAGE_INFO   oldPageInfo = brd->GetPageSettings();
    TITLE_BLOCK oldTitleBlock = brd->GetTitleBlock();

    // Keep also the count of copper layers, to adjust if necessary
    int initialCopperLayerCount = brd->GetCopperLayerCount();
    LSET initialEnabledLayers = brd->GetEnabledLayers();

    // Load the data
    try
    {
        STRING_UTF8_MAP props;

        // EAGLE_PLUGIN can use this info to center the BOARD, but it does not yet.

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

        WX_PROGRESS_REPORTER progressReporter( editFrame, _( "Loading PCB" ), 1 );

        editFrame->GetDesignSettings().m_NetSettings->m_NetClasses.clear();
        pi.Load( fileName, brd, &props, nullptr, &progressReporter );
    }
    catch( const IO_ERROR& ioe )
    {
        wxString msg = wxString::Format( _( "Error loading board.\n%s" ), ioe.What() );
        DisplayError( editFrame, msg );

        return 0;
    }

    newProperties = brd->GetProperties();

    for( const std::pair<const wxString, wxString>& prop : oldProperties )
        newProperties[ prop.first ] = prop.second;

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

    return placeBoardItems( brd, false, false ); // Do not reannotate duplicates on Append Board
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
        EDA_ITEM*                   item = selection.Front();

        item->GetMsgPanelInfo( m_frame, msgItems );
    }
    else if( pcbFrame && selection.GetSize() == 2 )
    {
        // Pair selection broken into multiple, optional data, starting with the selected item
        // names

        BOARD_ITEM* a = static_cast<BOARD_ITEM*>( selection[0] );
        BOARD_ITEM* b = static_cast<BOARD_ITEM*>( selection[1] );

        msgItems.emplace_back( MSG_PANEL_ITEM( a->GetItemDescription( m_frame ),
                                               b->GetItemDescription( m_frame ) ) );

        BOARD_CONNECTED_ITEM* a_conn = dyn_cast<BOARD_CONNECTED_ITEM*>( a );
        BOARD_CONNECTED_ITEM* b_conn = dyn_cast<BOARD_CONNECTED_ITEM*>( b );

        if( a_conn && b_conn )
        {
            LSET overlap = a_conn->GetLayerSet() & b_conn->GetLayerSet() & LSET::AllCuMask();
            int  a_netcode = a_conn->GetNetCode();
            int  b_netcode = b_conn->GetNetCode();

            if( overlap.count() > 0
                    && ( a_netcode != b_netcode || a_netcode < 0 || b_netcode < 0 ) )
            {
                PCB_LAYER_ID layer = overlap.CuStack().front();

                constraint = drcEngine->EvalRules( CLEARANCE_CONSTRAINT, a, b, layer );

                std::shared_ptr<SHAPE> a_shape( a_conn->GetEffectiveShape( layer ) );
                std::shared_ptr<SHAPE> b_shape( b_conn->GetEffectiveShape( layer ) );

                int actual_clearance = a_shape->GetClearance( b_shape.get() );

                msgItems.emplace_back( _( "Resolved clearance" ),
                                       m_frame->MessageTextFromValue( constraint.m_Value.Min() ) );

                if( actual_clearance > -1 && actual_clearance < std::numeric_limits<int>::max() )
                {
                    msgItems.emplace_back( _( "Actual clearance" ),
                                           m_frame->MessageTextFromValue( actual_clearance ) );
                }
            }
        }

        if( ( a->HasHole() || b->HasHole() ) )
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
                    msgItems.emplace_back( _( "Resolved hole clearance" ),
                            m_frame->MessageTextFromValue( constraint.m_Value.Min() ) );

                    if( actual > -1 && actual < std::numeric_limits<int>::max() )
                    {
                        msgItems.emplace_back( _( "Actual hole clearance" ),
                                m_frame->MessageTextFromValue( actual ) );
                    }
                }
            }
        }

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
                    msgItems.emplace_back( _( "Resolved edge clearance" ),
                                           m_frame->MessageTextFromValue( constraint.m_Value.Min() ) );
                }
                else
                {
                    msgItems.emplace_back( _( "Resolved margin clearance" ),
                                           m_frame->MessageTextFromValue( constraint.m_Value.Min() ) );
                }
            }
        }
    }

    if( msgItems.empty() )
    {
        if( selection.GetSize() )
        {
            msgItems.emplace_back( _( "Selected Items" ),
                                   wxString::Format( wxT( "%d" ), selection.GetSize() ) );

            std::set<wxString> netNames;
            std::set<wxString> netClasses;

            for( EDA_ITEM* item : selection )
            {
                if( BOARD_CONNECTED_ITEM* bci = dynamic_cast<BOARD_CONNECTED_ITEM*>( item ) )
                {
                    netNames.insert( UnescapeString( bci->GetNetname() ) );
                    netClasses.insert( UnescapeString( bci->GetEffectiveNetClass()->GetName() ) );

                    if( netNames.size() > 1 && netClasses.size() > 1 )
                        break;
                }
            }

            if( netNames.size() == 1 )
                msgItems.emplace_back( _( "Net" ), *netNames.begin() );

            if( netClasses.size() == 1 )
                msgItems.emplace_back( _( "Resolved Netclass" ), *netClasses.begin() );
        }
        else
        {
            m_frame->GetBoard()->GetMsgPanelInfo( m_frame, msgItems );
        }
    }

    m_frame->SetMsgPanel( msgItems );

    return 0;
}


int PCB_CONTROL::DdAppendBoard( const TOOL_EVENT& aEvent )
{
    wxFileName fileName = wxFileName( *aEvent.Parameter<wxString*>() );

    int open_ctl = fileName.GetExt() == KiCadPcbFileExtension ? 0 : KICTL_EAGLE_BRD;

    PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>( m_frame );

    if( !editFrame )
        return 1;

    wxString filePath = fileName.GetFullPath();
    IO_MGR::PCB_FILE_T pluginType = plugin_type( filePath, open_ctl );
    PLUGIN::RELEASER pi( IO_MGR::PluginFind( pluginType ) );

    return AppendBoard( *pi, filePath );
}


int PCB_CONTROL::FlipPcbView( const TOOL_EVENT& aEvent )
{
    view()->SetMirror( !view()->IsMirroredX(), false );
    view()->RecacheAllItems();
    frame()->GetCanvas()->ForceRefresh();
    frame()->OnDisplayOptionsChanged();
    return 0;
}


void PCB_CONTROL::setTransitions()
{
    Go( &PCB_CONTROL::AddLibrary,           ACTIONS::newLibrary.MakeEvent() );
    Go( &PCB_CONTROL::AddLibrary,           ACTIONS::addLibrary.MakeEvent() );
    Go( &PCB_CONTROL::Print,                ACTIONS::print.MakeEvent() );
    Go( &PCB_CONTROL::Quit,                 ACTIONS::quit.MakeEvent() );

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
    Go( &PCB_CONTROL::NetColorModeCycle,     PCB_ACTIONS::netColorModeCycle.MakeEvent() );
    Go( &PCB_CONTROL::RatsnestModeCycle,     PCB_ACTIONS::ratsnestModeCycle.MakeEvent() );
    Go( &PCB_CONTROL::FlipPcbView,           PCB_ACTIONS::flipBoard.MakeEvent() );

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

    // Grid control
    Go( &PCB_CONTROL::GridSetOrigin,        ACTIONS::gridSetOrigin.MakeEvent() );
    Go( &PCB_CONTROL::GridResetOrigin,      ACTIONS::gridResetOrigin.MakeEvent() );

    Go( &PCB_CONTROL::Undo,                 ACTIONS::undo.MakeEvent() );
    Go( &PCB_CONTROL::Redo,                 ACTIONS::redo.MakeEvent() );

    // Miscellaneous
    Go( &PCB_CONTROL::DeleteItemCursor,     ACTIONS::deleteTool.MakeEvent() );

    // Append control
    Go( &PCB_CONTROL::AppendBoardFromFile,  PCB_ACTIONS::appendBoard.MakeEvent() );
    Go( &PCB_CONTROL::DdAppendBoard,        PCB_ACTIONS::ddAppendBoard.MakeEvent() );

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
