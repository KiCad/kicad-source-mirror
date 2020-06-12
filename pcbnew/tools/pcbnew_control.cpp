/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2016 CERN
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "edit_tool.h"
#include "pcb_actions.h"
#include "pcbnew_control.h"
#include "pcbnew_picker_tool.h"
#include "selection_tool.h"
#include <3d_viewer/eda_3d_viewer.h>
#include <bitmaps.h>
#include <board_commit.h>
#include <class_board.h>
#include <class_board_item.h>
#include <class_module.h>
#include <class_track.h>
#include <class_zone.h>
#include <class_edge_mod.h>
#include <confirm.h>
#include <connectivity/connectivity_data.h>
#include <gal/graphics_abstraction_layer.h>
#include <io_mgr.h>
#include <kicad_clipboard.h>
#include <kiway.h>
#include <origin_viewitem.h>
#include <pcb_edit_frame.h>
#include <pcb_painter.h>
#include <pcb_screen.h>
#include <properties.h>
#include <settings/color_settings.h>
#include <tool/tool_manager.h>
#include <view/view_controls.h>
#include <functional>
#include <footprint_viewer_frame.h>
#include <footprint_edit_frame.h>
#include <math/util.h>      // for KiROUND

using namespace std::placeholders;


// files.cpp
extern bool AskLoadBoardFileName( wxWindow* aParent, int* aCtl, wxString* aFileName,
                                    bool aKicadFilesOnly = false );
extern IO_MGR::PCB_FILE_T plugin_type( const wxString& aFileName, int aCtl );


PCBNEW_CONTROL::PCBNEW_CONTROL() :
    PCB_TOOL_BASE( "pcbnew.Control" ),
    m_frame( nullptr ),
    m_pickerItem( nullptr )
{
    m_gridOrigin.reset( new KIGFX::ORIGIN_VIEWITEM() );
}


PCBNEW_CONTROL::~PCBNEW_CONTROL()
{
}


void PCBNEW_CONTROL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<PCB_BASE_FRAME>();

    if( aReason == MODEL_RELOAD || aReason == GAL_SWITCH )
    {
        m_gridOrigin->SetPosition( board()->GetGridOrigin() );
        m_gridOrigin->SetColor( m_frame->GetGridColor() );
        getView()->Remove( m_gridOrigin.get() );
        getView()->Add( m_gridOrigin.get() );
    }
}


int PCBNEW_CONTROL::AddLibrary( const TOOL_EVENT& aEvent )
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


int PCBNEW_CONTROL::Quit( const TOOL_EVENT& aEvent )
{
    m_frame->Close( false );
    return 0;
}


template<class T> void Flip( T& aValue )
{
    aValue = !aValue;
}


int PCBNEW_CONTROL::TrackDisplayMode( const TOOL_EVENT& aEvent )
{
    auto opts = displayOptions();

    Flip( opts.m_DisplayPcbTrackFill );
    m_frame->SetDisplayOptions( opts );
    view()->UpdateDisplayOptions( opts );

    for( auto track : board()->Tracks() )
    {
        if( track->Type() == PCB_TRACE_T )
            view()->Update( track, KIGFX::GEOMETRY );
    }

    canvas()->Refresh();

    return 0;
}


int PCBNEW_CONTROL::ToggleRatsnest( const TOOL_EVENT& aEvent )
{
    auto opts = displayOptions();

    if( aEvent.IsAction( &PCB_ACTIONS::showRatsnest ) )
    {
        // N.B. Do not disable the Ratsnest layer here.  We use it for local ratsnest
        Flip( opts.m_ShowGlobalRatsnest );
        m_frame->SetDisplayOptions( opts );
        view()->UpdateDisplayOptions( opts );
        getEditFrame<PCB_EDIT_FRAME>()->SetElementVisibility( LAYER_RATSNEST,
                opts.m_ShowGlobalRatsnest );

    }
    else if( aEvent.IsAction( &PCB_ACTIONS::ratsnestLineMode ) )
    {
        Flip( opts.m_DisplayRatsnestLinesCurved );
        m_frame->SetDisplayOptions( opts );
        view()->UpdateDisplayOptions( opts );
    }

    canvas()->RedrawRatsnest();
    canvas()->Refresh();

    return 0;
}


int PCBNEW_CONTROL::ViaDisplayMode( const TOOL_EVENT& aEvent )
{
    auto opts = displayOptions();

    Flip( opts.m_DisplayViaFill );
    view()->UpdateDisplayOptions( opts );
    m_frame->SetDisplayOptions( opts );

    for( auto track : board()->Tracks() )
    {
        if( track->Type() == PCB_TRACE_T || track->Type() == PCB_VIA_T )
            view()->Update( track, KIGFX::GEOMETRY );
    }

    canvas()->Refresh();

    return 0;
}


int PCBNEW_CONTROL::ZoneDisplayMode( const TOOL_EVENT& aEvent )
{
    auto opts = displayOptions();

    // Apply new display options to the GAL canvas
    if( aEvent.IsAction( &PCB_ACTIONS::zoneDisplayEnable ) )
        opts.m_DisplayZonesMode = 0;
    else if( aEvent.IsAction( &PCB_ACTIONS::zoneDisplayDisable ) )
        opts.m_DisplayZonesMode = 1;
    else if( aEvent.IsAction( &PCB_ACTIONS::zoneDisplayOutlines ) )
        opts.m_DisplayZonesMode = 2;
    else if( aEvent.IsAction( &PCB_ACTIONS::zoneDisplayToggle ) )
        opts.m_DisplayZonesMode = ( opts.m_DisplayZonesMode + 1 ) % 3;
    else
        wxFAIL;

    m_frame->SetDisplayOptions( opts );
    view()->UpdateDisplayOptions( opts );

    for( int i = 0; i < board()->GetAreaCount(); ++i )
        view()->Update( board()->GetArea( i ), KIGFX::GEOMETRY );

    canvas()->Refresh();

    return 0;
}


int PCBNEW_CONTROL::HighContrastMode( const TOOL_EVENT& aEvent )
{
    auto opts = displayOptions();

    Flip( opts.m_ContrastModeDisplay );
    m_frame->SetDisplayOptions( opts );
    view()->UpdateDisplayOptions( opts );
    canvas()->SetHighContrastLayer( m_frame->GetActiveLayer() );

    return 0;
}


// Layer control
int PCBNEW_CONTROL::LayerSwitch( const TOOL_EVENT& aEvent )
{
    m_frame->SwitchLayer( NULL, aEvent.Parameter<PCB_LAYER_ID>() );

    return 0;
}


int PCBNEW_CONTROL::LayerNext( const TOOL_EVENT& aEvent )
{
    PCB_BASE_FRAME* editFrame = m_frame;
    BOARD*          brd        = board();
    LAYER_NUM layer = editFrame->GetActiveLayer();
    LAYER_NUM       startLayer = layer;

    if( layer < F_Cu || layer > B_Cu )
        return 0;

    while( startLayer != ++layer )
    {
        if( brd->IsLayerVisible( static_cast<PCB_LAYER_ID>( layer ) ) && IsCopperLayer( layer ) )
            break;

        if( layer >= B_Cu )
            layer = F_Cu - 1;
    }

    wxCHECK( IsCopperLayer( layer ), 0 );
    editFrame->SwitchLayer( NULL, ToLAYER_ID( layer ) );

    return 0;
}


int PCBNEW_CONTROL::LayerPrev( const TOOL_EVENT& aEvent )
{
    PCB_BASE_FRAME* editFrame = m_frame;
    BOARD*          brd        = board();
    LAYER_NUM layer = editFrame->GetActiveLayer();
    LAYER_NUM       startLayer = layer;

    if( layer < F_Cu || layer > B_Cu )
        return 0;

    while( startLayer != --layer )
    {
        if( IsCopperLayer( layer )       // also test for valid layer id (layer >= F_Cu)
            && brd->IsLayerVisible( static_cast<PCB_LAYER_ID>( layer ) ) )
            break;

        if( layer <= F_Cu )
            layer = B_Cu + 1;
    }


    wxCHECK( IsCopperLayer( layer ), 0 );
    editFrame->SwitchLayer( NULL, ToLAYER_ID( layer ) );

    return 0;
}


int PCBNEW_CONTROL::LayerToggle( const TOOL_EVENT& aEvent )
{
    LAYER_NUM currentLayer = m_frame->GetActiveLayer();
    PCB_SCREEN* screen = m_frame->GetScreen();

    if( currentLayer == screen->m_Route_Layer_TOP )
        m_frame->SwitchLayer( NULL, screen->m_Route_Layer_BOTTOM );
    else
        m_frame->SwitchLayer( NULL, screen->m_Route_Layer_TOP );

    return 0;
}


// It'd be nice to share the min/max with the DIALOG_COLOR_PICKER, but those are
// set in wxFormBuilder.
#define ALPHA_MIN 0.20
#define ALPHA_MAX 1.00
#define ALPHA_STEP 0.05

int PCBNEW_CONTROL::LayerAlphaInc( const TOOL_EVENT& aEvent )
{
    auto settings = m_frame->GetColorSettings();

    LAYER_NUM currentLayer = m_frame->GetActiveLayer();
    KIGFX::COLOR4D currentColor = settings->GetColor( currentLayer );

    if( currentColor.a <= ALPHA_MAX - ALPHA_STEP )
    {
        currentColor.a += ALPHA_STEP;
        settings->SetColor( currentLayer, currentColor );
        m_frame->GetCanvas()->UpdateColors();

        KIGFX::VIEW* view = m_frame->GetCanvas()->GetView();
        view->UpdateLayerColor( currentLayer );

        wxUpdateUIEvent dummy;
        static_cast<PCB_EDIT_FRAME*>( m_frame )->OnUpdateLayerAlpha( dummy );
    }
    else
        wxBell();

    return 0;
}


int PCBNEW_CONTROL::LayerAlphaDec( const TOOL_EVENT& aEvent )
{
    auto settings = m_frame->GetColorSettings();

    LAYER_NUM currentLayer = m_frame->GetActiveLayer();
    KIGFX::COLOR4D currentColor = settings->GetColor( currentLayer );

    if( currentColor.a >= ALPHA_MIN + ALPHA_STEP )
    {
        currentColor.a -= ALPHA_STEP;
        settings->SetColor( currentLayer, currentColor );
        m_frame->GetCanvas()->UpdateColors();

        KIGFX::VIEW* view = m_frame->GetCanvas()->GetView();
        view->UpdateLayerColor( currentLayer );

        wxUpdateUIEvent dummy;
        static_cast<PCB_BASE_FRAME*>( m_frame )->OnUpdateLayerAlpha( dummy );
    }
    else
        wxBell();

    return 0;
}


// Grid control
int PCBNEW_CONTROL::GridFast1( const TOOL_EVENT& aEvent )
{
    m_toolMgr->RunAction( "common.Control.gridPreset", true, m_frame->Settings().m_FastGrid1 );
    return 0;
}


int PCBNEW_CONTROL::GridFast2( const TOOL_EVENT& aEvent )
{
    m_toolMgr->RunAction( "common.Control.gridPreset", true, m_frame->Settings().m_FastGrid2 );
    return 0;
}


void PCBNEW_CONTROL::DoSetGridOrigin( KIGFX::VIEW* aView, PCB_BASE_FRAME* aFrame,
                                      BOARD_ITEM* originViewItem, const VECTOR2D& aPoint )
{
    aFrame->SetGridOrigin( wxPoint( aPoint.x, aPoint.y ) );
    aView->GetGAL()->SetGridOrigin( aPoint );
    originViewItem->SetPosition( wxPoint( aPoint.x, aPoint.y ) );
    aView->MarkDirty();
    aFrame->OnModify();
}


int PCBNEW_CONTROL::GridSetOrigin( const TOOL_EVENT& aEvent )
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
        if( m_editModules && !getEditFrame<PCB_BASE_EDIT_FRAME>()->GetModel() )
            return 0;

        std::string         tool = aEvent.GetCommandStr().get();
        PCBNEW_PICKER_TOOL* picker = m_toolMgr->GetTool<PCBNEW_PICKER_TOOL>();

        // Deactivate other tools; particularly important if another PICKER is currently running
        Activate();

        picker->SetClickHandler(
            [this] ( const VECTOR2D& pt ) -> bool
            {
                m_frame->SaveCopyInUndoList( m_gridOrigin.get(), UR_GRIDORIGIN );
                DoSetGridOrigin( getView(), m_frame, m_gridOrigin.get(), pt );
                return false;   // drill origin is a one-shot; don't continue with tool
            } );

        m_toolMgr->RunAction( ACTIONS::pickerTool, true, &tool );
    }

    return 0;
}


int PCBNEW_CONTROL::GridResetOrigin( const TOOL_EVENT& aEvent )
{
    m_frame->SaveCopyInUndoList( m_gridOrigin.get(), UR_GRIDORIGIN );
    DoSetGridOrigin( getView(), m_frame, m_gridOrigin.get(), VECTOR2D( 0, 0 ) );
    return 0;
}


#define HITTEST_THRESHOLD_PIXELS 5


int PCBNEW_CONTROL::DeleteItemCursor( const TOOL_EVENT& aEvent )
{
    if( m_editModules && !m_frame->GetBoard()->GetFirstModule() )
        return 0;

    std::string         tool = aEvent.GetCommandStr().get();
    PCBNEW_PICKER_TOOL* picker = m_toolMgr->GetTool<PCBNEW_PICKER_TOOL>();

    m_pickerItem = nullptr;
    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    // Deactivate other tools; particularly important if another PICKER is currently running
    Activate();

    picker->SetCursor( wxStockCursor( wxCURSOR_BULLSEYE ) );

    picker->SetClickHandler(
        [this] ( const VECTOR2D& aPosition ) -> bool
        {
            if( m_pickerItem )
            {
                if( m_pickerItem && m_pickerItem->IsLocked() )
                {
                    STATUS_TEXT_POPUP statusPopup( m_frame );
                    statusPopup.SetText( _( "Item locked." ) );
                    statusPopup.PopupFor( 2000 );
                    statusPopup.Move( wxGetMousePosition() + wxPoint( 20, 20 ) );
                    return true;
                }

                SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<SELECTION_TOOL>();
                selectionTool->UnbrightenItem( m_pickerItem );
                selectionTool->AddItemToSel( m_pickerItem, true /*quiet mode*/ );
                m_toolMgr->RunAction( ACTIONS::doDelete, true );
                m_pickerItem = nullptr;
            }

            return true;
        } );

    picker->SetMotionHandler(
        [this] ( const VECTOR2D& aPos )
        {
            BOARD* board = m_frame->GetBoard();
            SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<SELECTION_TOOL>();
            GENERAL_COLLECTORS_GUIDE guide = m_frame->GetCollectorsGuide();
            GENERAL_COLLECTOR collector;
            collector.m_Threshold = KiROUND( getView()->ToWorld( HITTEST_THRESHOLD_PIXELS ) );

            if( m_editModules )
                collector.Collect( board, GENERAL_COLLECTOR::ModuleItems, (wxPoint) aPos, guide );
            else
                collector.Collect( board, GENERAL_COLLECTOR::BoardLevelItems, (wxPoint) aPos, guide );

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
        [this] ( const int& aFinalState )
        {
            if( m_pickerItem )
                m_toolMgr->GetTool<SELECTION_TOOL>()->UnbrightenItem( m_pickerItem );
        } );

    m_toolMgr->RunAction( ACTIONS::pickerTool, true, &tool );

    return 0;
}


void pasteModuleItemsToModEdit( MODULE* aClipModule, BOARD* aBoard,
                                std::vector<BOARD_ITEM*>& aPastedItems )
{
    MODULE* editModule = aBoard->GetFirstModule();

    aClipModule->SetParent( aBoard );

    for( D_PAD* pad : aClipModule->Pads() )
    {
        pad->SetParent( editModule );
        aPastedItems.push_back( pad );
    }

    aClipModule->Pads().clear();

    for( BOARD_ITEM* item : aClipModule->GraphicalItems() )
    {
        if( item->Type() == PCB_MODULE_EDGE_T )
        {
            EDGE_MODULE* edge = static_cast<EDGE_MODULE*>( item );

            edge->SetParent( nullptr );
            edge->SetLocalCoord();
        }
        else if( item->Type() == PCB_MODULE_TEXT_T )
        {
            TEXTE_MODULE* text = static_cast<TEXTE_MODULE*>( item );

            if( text->GetType() != TEXTE_MODULE::TEXT_is_DIVERS )
                text->SetType( TEXTE_MODULE::TEXT_is_DIVERS );

            if( text->GetText() == "${VALUE}" )
                text->SetText( aClipModule->GetValue() );
            else if( text->GetText() == "${REFERENCE}" )
                text->SetText( aClipModule->GetReference() );

            text->SetTextAngle( aClipModule->GetOrientation() );

            text->SetParent( nullptr );
            text->SetLocalCoord();
        }

        item->SetParent( editModule );
        aPastedItems.push_back( item );
    }

    aClipModule->GraphicalItems().clear();

    if( !aClipModule->GetReference().IsEmpty() )
    {
        TEXTE_MODULE* text = new TEXTE_MODULE( aClipModule->Reference() );
        text->SetType( TEXTE_MODULE::TEXT_is_DIVERS );
        text->SetTextAngle( aClipModule->GetOrientation() );

        text->SetParent( nullptr );
        text->SetLocalCoord();

        text->SetParent( editModule );
        aPastedItems.push_back( text );
    }

    if( !aClipModule->GetValue().IsEmpty() )
    {
        TEXTE_MODULE* text = new TEXTE_MODULE( aClipModule->Value() );
        text->SetType( TEXTE_MODULE::TEXT_is_DIVERS );
        text->SetTextAngle( aClipModule->GetOrientation() );

        text->SetParent( nullptr );
        text->SetLocalCoord();

        text->SetParent( editModule );
        aPastedItems.push_back( text );
    }
}


int PCBNEW_CONTROL::Paste( const TOOL_EVENT& aEvent )
{
    CLIPBOARD_IO pi;
    BOARD_ITEM* clipItem = pi.Parse();

    if( !clipItem )
        return 0;

    bool editModules = m_editModules || frame()->IsType( FRAME_FOOTPRINT_EDITOR );

    if( clipItem->Type() == PCB_T )
    {
        if( editModules )
        {
            for( BOARD_CONNECTED_ITEM* item : static_cast<BOARD*>( clipItem )->AllConnectedItems() )
                item->SetNet( NETINFO_LIST::OrphanedItem() );
        }
        else
            static_cast<BOARD*>( clipItem )->MapNets( m_frame->GetBoard() );
    }

    // The clipboard can contain two different things, an entire kicad_pcb
    // or a single module

    if( editModules && ( !board() || !module() ) )
    {
        wxLogDebug( wxT( "Attempting to paste to empty module editor window\n") );
        return 0;
    }

    switch( clipItem->Type() )
    {
        case PCB_T:
        {
            BOARD* clipBoard = static_cast<BOARD*>( clipItem );

            if( editModules )
            {
                MODULE* editModule = board()->GetFirstModule();
                std::vector<BOARD_ITEM*> pastedItems;

                for( MODULE* clipModule : clipBoard->Modules() )
                    pasteModuleItemsToModEdit( clipModule, board(), pastedItems );

                for( BOARD_ITEM* clipDrawItem : clipBoard->Drawings() )
                {
                    if( clipDrawItem->Type() == PCB_LINE_T )
                    {
                        DRAWSEGMENT* clipDrawSeg = static_cast<DRAWSEGMENT*>( clipDrawItem );

                        // Convert to PCB_MODULE_EDGE_T
                        EDGE_MODULE* pastedDrawSeg = new EDGE_MODULE( editModule );
                        static_cast<DRAWSEGMENT*>( pastedDrawSeg )->SwapData( clipDrawSeg );
                        pastedDrawSeg->SetLocalCoord();

                        pastedItems.push_back( pastedDrawSeg );
                    }
                    else if( clipDrawItem->Type() == PCB_TEXT_T )
                    {
                        TEXTE_PCB* clipTextItem = static_cast<TEXTE_PCB*>( clipDrawItem );

                        // Convert to PCB_MODULE_TEXT_T
                        TEXTE_MODULE* pastedTextItem = new TEXTE_MODULE( editModule );
                        static_cast<EDA_TEXT*>( pastedTextItem )->SwapText( *clipTextItem );
                        static_cast<EDA_TEXT*>( pastedTextItem )->SwapEffects( *clipTextItem );

                        pastedItems.push_back( pastedTextItem );
                    }
                }

                delete clipBoard;

                placeBoardItems( pastedItems, true, true );
            }
            else
            {
                placeBoardItems( clipBoard, true );

                m_frame->Compile_Ratsnest( true );
                m_frame->GetBoard()->BuildConnectivity();
            }

            break;
        }

        case PCB_MODULE_T:
        {
            MODULE* clipModule = static_cast<MODULE*>( clipItem );
            std::vector<BOARD_ITEM*> pastedItems;

            if( editModules )
            {
                pasteModuleItemsToModEdit( clipModule, board(), pastedItems );
                delete clipModule;
            }
            else
            {
                clipModule->SetParent( board() );
                pastedItems.push_back( clipModule );
            }

            placeBoardItems( pastedItems, true, true );
            break;
        }

        default:
            m_frame->DisplayToolMsg( _( "Invalid clipboard contents" ) );
            break;
    }

    return 1;
}


int PCBNEW_CONTROL::AppendBoardFromFile( const TOOL_EVENT& aEvent )
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


// Helper function for PCBNEW_CONTROL::placeBoardItems()
template<typename T>
static void moveNoFlagToVector( std::deque<T>& aList, std::vector<BOARD_ITEM*>& aTarget, bool aIsNew )
{
    std::copy_if( aList.begin(), aList.end(), std::back_inserter( aTarget ),
            [aIsNew]( T aItem )
            {
                bool doCopy = ( aItem->GetFlags() & FLAG0 ) == 0;

                aItem->ClearFlags( FLAG0 );
                aItem->SetFlags( aIsNew ? IS_NEW : 0 );

                return doCopy;
            } );

    if( aIsNew )
        aList.clear();
}


static void moveNoFlagToVector(  ZONE_CONTAINERS& aList, std::vector<BOARD_ITEM*>& aTarget, bool aIsNew )
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
        if( obj->HasFlag( FLAG0 ) )
            obj->ClearFlags( FLAG0 );
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
                obj = nullptr;
        }
        else
            obj = idx < int(aList.size()-1) ? aList[++idx] : nullptr;
    }
}



int PCBNEW_CONTROL::placeBoardItems( BOARD* aBoard, bool aAnchorAtOrigin  )
{
    // items are new if the current board is not the board source
    bool isNew = board() != aBoard;
    std::vector<BOARD_ITEM*> items;

    moveNoFlagToVector( aBoard->Tracks(), items, isNew );
    moveNoFlagToVector( aBoard->Modules(), items, isNew );
    moveNoFlagToVector( aBoard->Drawings(), items, isNew );
    moveNoFlagToVector( aBoard->Zones(), items, isNew );

    return placeBoardItems( items, isNew, aAnchorAtOrigin );
}


int PCBNEW_CONTROL::placeBoardItems( std::vector<BOARD_ITEM*>& aItems, bool aIsNew,
                                     bool aAnchorAtOrigin )
{
    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    auto selectionTool = m_toolMgr->GetTool<SELECTION_TOOL>();
    auto editTool = m_toolMgr->GetTool<EDIT_TOOL>();

    PCBNEW_SELECTION& selection = selectionTool->GetSelection();

    for( auto item : aItems )
    {
        item->SetSelected();
        selection.Add( item );

        // Add or just select items for the move/place command
        if( aIsNew )
            editTool->GetCurrentCommit()->Add( item );
        else
            editTool->GetCurrentCommit()->Added( item );
    }

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


int PCBNEW_CONTROL::AppendBoard( PLUGIN& pi, wxString& fileName )
{
    PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>( m_frame );

    if( !editFrame )
        return 1;

    BOARD* brd = board();

    if( !brd )
        return 1;

    // Mark existing items, in order to know what are the new items so we can select only
    // the new items after loading
    for( auto track : brd->Tracks() )
        track->SetFlags( FLAG0 );

    for( auto module : brd->Modules() )
        module->SetFlags( FLAG0 );

    for( auto drawing : brd->Drawings() )
        drawing->SetFlags( FLAG0 );

    for( auto zone : brd->Zones() )
        zone->SetFlags( FLAG0 );

    // Keep also the count of copper layers, to adjust if necessary
    int initialCopperLayerCount = brd->GetCopperLayerCount();
    LSET initialEnabledLayers = brd->GetEnabledLayers();

    // Load the data
    try
    {
        PROPERTIES  props;
        char        xbuf[30];
        char        ybuf[30];

        // EAGLE_PLUGIN can use this info to center the BOARD, but it does not yet.
        sprintf( xbuf, "%d", editFrame->GetPageSizeIU().x );
        sprintf( ybuf, "%d", editFrame->GetPageSizeIU().y );

        props["page_width"]  = xbuf;
        props["page_height"] = ybuf;

        editFrame->GetDesignSettings().m_NetClasses.Clear();
        pi.Load( fileName, brd, &props );
    }
    catch( const IO_ERROR& ioe )
    {
        wxString msg = wxString::Format( _( "Error loading board.\n%s" ), GetChars( ioe.What() ));
        DisplayError( editFrame, msg );

        return 0;
    }

    // rebuild nets and ratsnest before any use of nets
    brd->BuildListOfNets();
    brd->SynchronizeNetsAndNetClasses();
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

    return placeBoardItems( brd, false );
}


int PCBNEW_CONTROL::Undo( const TOOL_EVENT& aEvent )
{
    PCB_BASE_EDIT_FRAME* editFrame = dynamic_cast<PCB_BASE_EDIT_FRAME*>( m_frame );
    wxCommandEvent       dummy;

    if( editFrame )
        editFrame->RestoreCopyFromUndoList( dummy );

    return 0;
}


int PCBNEW_CONTROL::Redo( const TOOL_EVENT& aEvent )
{
    PCB_BASE_EDIT_FRAME* editFrame = dynamic_cast<PCB_BASE_EDIT_FRAME*>( m_frame );
    wxCommandEvent       dummy;

    if( editFrame )
        editFrame->RestoreCopyFromRedoList( dummy );

    return 0;
}


int PCBNEW_CONTROL::UpdateMessagePanel( const TOOL_EVENT& aEvent )
{
    SELECTION_TOOL*   selTool = m_toolMgr->GetTool<SELECTION_TOOL>();
    PCBNEW_SELECTION& selection = selTool->GetSelection();

    if( selection.GetSize() == 1 )
    {
        EDA_ITEM*       item = selection.Front();
        MSG_PANEL_ITEMS msgItems;

        item->GetMsgPanelInfo( m_frame, msgItems );
        m_frame->SetMsgPanel( msgItems );
    }
    else if( selection.GetSize() > 1 )
    {
        MSG_PANEL_ITEMS msgItems;
        wxString        msg = wxString::Format( wxT( "%d" ), selection.GetSize() );

        msgItems.emplace_back( MSG_PANEL_ITEM( _( "Selected Items" ), msg, DARKCYAN ) );
        m_frame->SetMsgPanel( msgItems );
    }
    else if( auto editFrame = dynamic_cast<FOOTPRINT_EDIT_FRAME*>( m_frame ) )
    {
        MODULE* footprint = static_cast<MODULE*>( editFrame->GetModel() );

        if( !footprint )
            return 0;

        MSG_PANEL_ITEMS msgItems;
        wxString        msg;

        msg = footprint->GetFPID().GetLibNickname().wx_str();
        msgItems.emplace_back( MSG_PANEL_ITEM( _( "Library" ), msg, DARKCYAN ) );

        msg = footprint->GetFPID().GetLibItemName().wx_str();
        msgItems.emplace_back( MSG_PANEL_ITEM( _( "Footprint Name" ), msg, DARKCYAN ) );

        wxDateTime date( static_cast<time_t>( footprint->GetLastEditTime() ) );

        if( footprint->GetLastEditTime() && date.IsValid() )
        // Date format: see http://www.cplusplus.com/reference/ctime/strftime
            msg = date.Format( wxT( "%b %d, %Y" ) ); // Abbreviated_month_name Day, Year
        else
            msg = _( "Unknown" );

        msgItems.emplace_back( MSG_PANEL_ITEM( _( "Last Change" ), msg, BROWN ) );

        msg.Printf( wxT( "%zu" ), (size_t) footprint->GetPadCount( DO_NOT_INCLUDE_NPTH ) );
        msgItems.emplace_back( MSG_PANEL_ITEM( _( "Pads" ), msg, BLUE ) );

        wxString doc, keyword;
        doc.Printf( _( "Doc: %s" ), footprint->GetDescription() );
        keyword.Printf( _( "Key Words: %s" ), footprint->GetKeywords() );
        msgItems.emplace_back( MSG_PANEL_ITEM( doc, keyword, BLACK ) );

        m_frame->SetMsgPanel( msgItems );
    }
    else
    {
        m_frame->SetMsgPanel( m_frame->GetBoard() );
    }

    return 0;
}


void PCBNEW_CONTROL::setTransitions()
{
    Go( &PCBNEW_CONTROL::AddLibrary,           ACTIONS::newLibrary.MakeEvent() );
    Go( &PCBNEW_CONTROL::AddLibrary,           ACTIONS::addLibrary.MakeEvent() );
    Go( &PCBNEW_CONTROL::Print,                ACTIONS::print.MakeEvent() );
    Go( &PCBNEW_CONTROL::Quit,                 ACTIONS::quit.MakeEvent() );

    // Display modes
    Go( &PCBNEW_CONTROL::TrackDisplayMode,     PCB_ACTIONS::trackDisplayMode.MakeEvent() );
    Go( &PCBNEW_CONTROL::ToggleRatsnest,       PCB_ACTIONS::showRatsnest.MakeEvent() );
    Go( &PCBNEW_CONTROL::ToggleRatsnest,       PCB_ACTIONS::ratsnestLineMode.MakeEvent() );
    Go( &PCBNEW_CONTROL::ViaDisplayMode,       PCB_ACTIONS::viaDisplayMode.MakeEvent() );
    Go( &PCBNEW_CONTROL::ZoneDisplayMode,      PCB_ACTIONS::zoneDisplayEnable.MakeEvent() );
    Go( &PCBNEW_CONTROL::ZoneDisplayMode,      PCB_ACTIONS::zoneDisplayDisable.MakeEvent() );
    Go( &PCBNEW_CONTROL::ZoneDisplayMode,      PCB_ACTIONS::zoneDisplayOutlines.MakeEvent() );
    Go( &PCBNEW_CONTROL::ZoneDisplayMode,      PCB_ACTIONS::zoneDisplayToggle.MakeEvent() );
    Go( &PCBNEW_CONTROL::HighContrastMode,     ACTIONS::highContrastMode.MakeEvent() );

    // Layer control
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerTop.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner1.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner2.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner3.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner4.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner5.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner6.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner7.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner8.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner9.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner10.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner11.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner12.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner13.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner14.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner15.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner16.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner17.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner18.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner19.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner20.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner21.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner22.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner23.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner24.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner25.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner26.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner27.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner28.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner29.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner30.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerBottom.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerNext,            PCB_ACTIONS::layerNext.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerPrev,            PCB_ACTIONS::layerPrev.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerToggle,          PCB_ACTIONS::layerToggle.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerAlphaInc,        PCB_ACTIONS::layerAlphaInc.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerAlphaDec,        PCB_ACTIONS::layerAlphaDec.MakeEvent() );

    // Grid control
    Go( &PCBNEW_CONTROL::GridFast1,            ACTIONS::gridFast1.MakeEvent() );
    Go( &PCBNEW_CONTROL::GridFast2,            ACTIONS::gridFast2.MakeEvent() );
    Go( &PCBNEW_CONTROL::GridSetOrigin,        ACTIONS::gridSetOrigin.MakeEvent() );
    Go( &PCBNEW_CONTROL::GridResetOrigin,      ACTIONS::gridResetOrigin.MakeEvent() );

    Go( &PCBNEW_CONTROL::Undo,                 ACTIONS::undo.MakeEvent() );
    Go( &PCBNEW_CONTROL::Redo,                 ACTIONS::redo.MakeEvent() );

    // Miscellaneous
    Go( &PCBNEW_CONTROL::DeleteItemCursor,     ACTIONS::deleteTool.MakeEvent() );

    // Append control
    Go( &PCBNEW_CONTROL::AppendBoardFromFile,  PCB_ACTIONS::appendBoard.MakeEvent() );

    Go( &PCBNEW_CONTROL::Paste,                ACTIONS::paste.MakeEvent() );

    Go( &PCBNEW_CONTROL::UpdateMessagePanel,   EVENTS::SelectedEvent );
    Go( &PCBNEW_CONTROL::UpdateMessagePanel,   EVENTS::UnselectedEvent );
    Go( &PCBNEW_CONTROL::UpdateMessagePanel,   EVENTS::ClearedEvent );
    Go( &PCBNEW_CONTROL::UpdateMessagePanel,   EVENTS::SelectedItemsModified );
}


