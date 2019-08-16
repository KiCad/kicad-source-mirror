/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <kiway.h>
#include <sch_view.h>
#include <sch_edit_frame.h>
#include <sch_sheet.h>
#include <sch_line.h>
#include <connection_graph.h>
#include <erc.h>
#include <eeschema_id.h>
#include <netlist_object.h>
#include <tool/tool_manager.h>
#include <tool/picker_tool.h>
#include <tools/ee_actions.h>
#include <tools/sch_editor_control.h>
#include <tools/ee_selection.h>
#include <tools/ee_selection_tool.h>
#include <advanced_config.h>
#include <simulation_cursors.h>
#include <sim/sim_plot_frame.h>
#include <sch_legacy_plugin.h>
#include <class_library.h>
#include <confirm.h>
#include <sch_painter.h>
#include <status_popup.h>
#include <ws_proxy_undo_item.h>
#include <dialogs/dialog_page_settings.h>
#include <dialogs/dialog_fields_editor_global.h>
#include <invoke_sch_dialog.h>


int SCH_EDITOR_CONTROL::New( const TOOL_EVENT& aEvent )
{
    m_frame->NewProject();
    return 0;
}


int SCH_EDITOR_CONTROL::Open( const TOOL_EVENT& aEvent )
{
    m_frame->LoadProject();
    return 0;
}


int SCH_EDITOR_CONTROL::Save( const TOOL_EVENT& aEvent )
{
    m_frame->Save_File();
    return 0;
}


int SCH_EDITOR_CONTROL::SaveAs( const TOOL_EVENT& aEvent )
{
    m_frame->Save_File( true );
    return 0;
}


int SCH_EDITOR_CONTROL::SaveAll( const TOOL_EVENT& aEvent )
{
    m_frame->SaveProject();
    return 0;
}


int SCH_EDITOR_CONTROL::PageSetup( const TOOL_EVENT& aEvent )
{
    PICKED_ITEMS_LIST   undoCmd;
    WS_PROXY_UNDO_ITEM* undoItem = new WS_PROXY_UNDO_ITEM( m_frame );
    ITEM_PICKER         wrapper( undoItem, UR_PAGESETTINGS );

    undoCmd.PushItem( wrapper );
    m_frame->SaveCopyInUndoList( undoCmd, UR_PAGESETTINGS );

    DIALOG_PAGES_SETTINGS dlg( m_frame, wxSize( MAX_PAGE_SIZE_MILS, MAX_PAGE_SIZE_MILS ) );
    dlg.SetWksFileName( BASE_SCREEN::m_PageLayoutDescrFileName );

    if( dlg.ShowModal() != wxID_OK )
        m_frame->RollbackSchematicFromUndo();

    return 0;
}


int SCH_EDITOR_CONTROL::Print( const TOOL_EVENT& aEvent )
{
    m_frame->Print();
    return 0;
}


int SCH_EDITOR_CONTROL::Plot( const TOOL_EVENT& aEvent )
{
    m_frame->PlotSchematic();
    return 0;
}


int SCH_EDITOR_CONTROL::Quit( const TOOL_EVENT& aEvent )
{
    m_frame->Close( false );
    return 0;
}


// A dummy wxFindReplaceData signalling any marker should be found
static wxFindReplaceData g_markersOnly;


int SCH_EDITOR_CONTROL::FindAndReplace( const TOOL_EVENT& aEvent )
{
    m_frame->ShowFindReplaceDialog( aEvent.IsAction( &ACTIONS::findAndReplace ));
    return UpdateFind( aEvent );
}


int SCH_EDITOR_CONTROL::UpdateFind( const TOOL_EVENT& aEvent )
{
    wxFindReplaceData* data = m_frame->GetFindReplaceData();

    if( aEvent.IsAction( &ACTIONS::find ) || aEvent.IsAction( &ACTIONS::findAndReplace )
        || aEvent.IsAction( &ACTIONS::updateFind ) )
    {
        m_selectionTool->ClearSelection();

        INSPECTOR_FUNC inspector = [&] ( EDA_ITEM* item, void* )
        {
            if( data && item->Matches( *data, nullptr ) )
                m_selectionTool->BrightenItem( item );
            else if( item->IsBrightened() )
                m_selectionTool->UnbrightenItem( item );

            return SEARCH_CONTINUE;
        };

        EDA_ITEM* start = m_frame->GetScreen()->GetDrawItems();
        EDA_ITEM::IterateForward( start, inspector, nullptr, EE_COLLECTOR::AllItems );
    }
    else if( aEvent.Matches( EVENTS::SelectedItemsModified ) )
    {
        for( EDA_ITEM* item : m_selectionTool->GetSelection() )
        {
            if( data && item->Matches( *data, nullptr ) )
                m_selectionTool->BrightenItem( item );
            else if( item->IsBrightened() )
                m_selectionTool->UnbrightenItem( item );
        }
    }

    getView()->UpdateItems();
    m_frame->GetCanvas()->Refresh();

    return 0;
}


EDA_ITEM* nextMatch( SCH_SCREEN* aScreen, EDA_ITEM* after, wxFindReplaceData* data )
{
    EDA_ITEM* found = nullptr;

    INSPECTOR_FUNC inspector = [&] ( EDA_ITEM* item, void* testData )
    {
        if( after )
        {
            if( after == item )
                after = nullptr;

            return SEARCH_CONTINUE;
        }

        if( ( data == &g_markersOnly && item->Type() == SCH_MARKER_T )
                || item->Matches( *data, nullptr ) )
        {
            found = item;
            return SEARCH_QUIT;
        }

        return SEARCH_CONTINUE;
    };

    EDA_ITEM::IterateForward( aScreen->GetDrawItems(), inspector, nullptr, EE_COLLECTOR::AllItems );

    return found;
}


int SCH_EDITOR_CONTROL::FindNext( const TOOL_EVENT& aEvent )
{
    // A timer during which a subsequent FindNext will result in a wrap-around
    static wxTimer wrapAroundTimer;

    wxFindReplaceData* data = m_frame->GetFindReplaceData();

    if( aEvent.IsAction( &ACTIONS::findNextMarker ) )
    {
        if( data )
            g_markersOnly.SetFlags( data->GetFlags() );

        data = &g_markersOnly;
    }
    else if( !data )
    {
        return FindAndReplace( ACTIONS::find.MakeEvent() );
    }

    bool          searchAllSheets = !( data->GetFlags() & FR_CURRENT_SHEET_ONLY );
    EE_SELECTION& selection = m_selectionTool->GetSelection();
    SCH_SCREEN*   afterScreen = m_frame->GetScreen();
    EDA_ITEM*     afterItem = selection.Front();
    EDA_ITEM*     item = nullptr;

    if( wrapAroundTimer.IsRunning() )
    {
        afterScreen = nullptr;
        afterItem = nullptr;
        wrapAroundTimer.Stop();
        m_frame->ClearFindReplaceStatus();
    }

    m_selectionTool->ClearSelection();

    if( afterScreen || !searchAllSheets )
        item = nextMatch( m_frame->GetScreen(), afterItem, data );

    if( !item && searchAllSheets )
    {
        SCH_SHEET_LIST schematic( g_RootSheet );
        SCH_SCREENS    screens;

        for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
        {
            if( afterScreen )
            {
                if( afterScreen == screen )
                    afterScreen = nullptr;

                continue;
            }

            item = nextMatch( screen, nullptr, data );

            if( item )
            {
                SCH_SHEET_PATH* sheet = schematic.FindSheetForScreen( screen );
                wxCHECK_MSG( sheet, 0, "Sheet not found for " + screen->GetFileName() );

                *g_CurrentSheet = *sheet;
                g_CurrentSheet->UpdateAllScreenReferences();

                screen->SetZoom( m_frame->GetScreen()->GetZoom() );
                screen->TestDanglingEnds();

                m_frame->SetScreen( screen );
                UpdateFind( ACTIONS::updateFind.MakeEvent() );

                break;
            }
        }
    }

    if( item )
    {
        m_selectionTool->AddItemToSel( item );
        m_frame->FocusOnLocation( item->GetBoundingBox().GetCenter(), true );
        m_frame->GetCanvas()->Refresh();
    }
    else
    {
        wxString msg = searchAllSheets ? _( "Reached end of schematic." )
                                       : _( "Reached end of sheet." );

        m_frame->ShowFindReplaceStatus( msg + _( "\nFind again to wrap around to the start." ) );
        wrapAroundTimer.StartOnce( 4000 );
    }

    return 0;
}


bool SCH_EDITOR_CONTROL::HasMatch()
{
    wxFindReplaceData* data = m_frame->GetFindReplaceData();
    EDA_ITEM*          item = m_selectionTool->GetSelection().Front();

    return data && item && item->Matches( *data, nullptr );
}


int SCH_EDITOR_CONTROL::ReplaceAndFindNext( const TOOL_EVENT& aEvent )
{
    wxFindReplaceData* data = m_frame->GetFindReplaceData();
    EDA_ITEM*          item = m_selectionTool->GetSelection().Front();

    if( !data )
        return FindAndReplace( ACTIONS::find.MakeEvent() );

    if( item && item->Matches( *data, nullptr ) )
    {
        item->Replace( *data, g_CurrentSheet );
        FindNext( ACTIONS::findNext.MakeEvent() );
    }

    return 0;
}


int SCH_EDITOR_CONTROL::ReplaceAll( const TOOL_EVENT& aEvent )
{
    wxFindReplaceData* data = m_frame->GetFindReplaceData();

    if( !data )
        return FindAndReplace( ACTIONS::find.MakeEvent() );

    SCH_SHEET_LIST schematic( g_RootSheet );
    SCH_SCREENS    screens;

    for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
    {
        for( EDA_ITEM* item = nextMatch( screen, nullptr, data ); item;
             item = nextMatch( screen, item, data ) )
        {
            item->Replace( *data, schematic.FindSheetForScreen( screen ) );
        }
    }

    return 0;
}


int SCH_EDITOR_CONTROL::CrossProbeToPcb( const TOOL_EVENT& aEvent )
{
    doCrossProbeSchToPcb( aEvent, false );
    return 0;
}


int SCH_EDITOR_CONTROL::ExplicitCrossProbeToPcb( const TOOL_EVENT& aEvent )
{
    doCrossProbeSchToPcb( aEvent, true );
    return 0;
}


void SCH_EDITOR_CONTROL::doCrossProbeSchToPcb( const TOOL_EVENT& aEvent, bool aForce )
{
    // Don't get in an infinite loop SCH -> PCB -> SCH -> PCB -> SCH -> ...
    if( m_probingPcbToSch )
        return;

    EE_SELECTION_TOOL* selTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
    SCH_ITEM*          item = nullptr;
    SCH_COMPONENT*     component = nullptr;

    if( aForce )
    {
        EE_SELECTION& selection = selTool->RequestSelection();

        if( selection.GetSize() >= 1 )
            item = (SCH_ITEM*) selection.Front();
    }
    else
    {
        EE_SELECTION& selection = selTool->GetSelection();

        if( selection.GetSize() >= 1 )
            item = (SCH_ITEM*) selection.Front();
    }

    if( !item )
    {
        if( aForce )
            m_frame->SendMessageToPCBNEW( nullptr, nullptr );

        return;
    }


    switch( item->Type() )
    {
    case SCH_FIELD_T:
    case LIB_FIELD_T:
        component = (SCH_COMPONENT*) item->GetParent();
        m_frame->SendMessageToPCBNEW( item, component );
        break;

    case SCH_COMPONENT_T:
        component = (SCH_COMPONENT*) item;
        m_frame->SendMessageToPCBNEW( item, component );
        break;

    case SCH_PIN_T:
        component = (SCH_COMPONENT*) item->GetParent();
        m_frame->SendMessageToPCBNEW( static_cast<SCH_PIN*>( item ), component );
        break;

    case SCH_SHEET_T:
        if( aForce )
            m_frame->SendMessageToPCBNEW( item, nullptr );
        break;

    default:
        break;
    }
}


#ifdef KICAD_SPICE

static KICAD_T wires[] = { SCH_LINE_LOCATE_WIRE_T, EOT };
static KICAD_T wiresAndPins[] = { SCH_LINE_LOCATE_WIRE_T, SCH_PIN_T, SCH_SHEET_PIN_T, EOT };
static KICAD_T fieldsAndComponents[] = { SCH_COMPONENT_T, SCH_FIELD_T, EOT };

#define HITTEST_THRESHOLD_PIXELS 5


int SCH_EDITOR_CONTROL::SimProbe( const TOOL_EVENT& aEvent )
{
    auto picker = m_toolMgr->GetTool<PICKER_TOOL>();
    auto simFrame = (SIM_PLOT_FRAME*) m_frame->Kiway().Player( FRAME_SIMULATOR, false );

    if( !simFrame )     // Defensive coding; shouldn't happen.
        return 0;

    // Deactivate other tools; particularly important if another PICKER is currently running
    Activate();

    picker->SetCursor( SIM_CURSORS::GetCursor( SIM_CURSORS::VOLTAGE_PROBE ) );

    picker->SetClickHandler(
        [this, simFrame] ( const VECTOR2D& aPosition )
        {
            EE_SELECTION_TOOL* selTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
            EDA_ITEM*          item = selTool->SelectPoint( aPosition, wiresAndPins );

            if( !item )
                return false;

            if( item->IsType( wires ) )
            {
                std::unique_ptr<NETLIST_OBJECT_LIST> netlist( m_frame->BuildNetListBase() );

                for( NETLIST_OBJECT* obj : *netlist )
                {
                    if( obj->m_Comp == item )
                    {
                        simFrame->AddVoltagePlot( UnescapeString( obj->GetNetName() ) );
                        break;
                    }
                }
            }
            else if( item->Type() == SCH_PIN_T )
            {
                SCH_PIN*       pin = (SCH_PIN*) item;
                SCH_COMPONENT* comp = (SCH_COMPONENT*) item->GetParent();
                wxString       param = wxString::Format( _T( "I%s" ), pin->GetName().Lower() );

                simFrame->AddCurrentPlot( comp->GetRef( g_CurrentSheet ), param );
            }

            return true;
        } );

    picker->SetMotionHandler(
        [this, picker] ( const VECTOR2D& aPos )
        {
            EE_COLLECTOR collector;
            collector.m_Threshold = KiROUND( getView()->ToWorld( HITTEST_THRESHOLD_PIXELS ) );
            collector.Collect( m_frame->GetScreen()->GetDrawItems(), wiresAndPins, (wxPoint) aPos );

            EE_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
            selectionTool->GuessSelectionCandidates( collector, aPos );

            EDA_ITEM* item = collector.GetCount() == 1 ? collector[ 0 ] : nullptr;
            SCH_LINE* wire = dynamic_cast<SCH_LINE*>( item );
            wxString  netName;

            if( wire )
            {
                item = nullptr;

                if( wire->Connection( *g_CurrentSheet ) )
                    netName = wire->Connection( *g_CurrentSheet )->Name();
            }

            if( item && item->Type() == SCH_PIN_T )
                picker->SetCursor( SIM_CURSORS::GetCursor( SIM_CURSORS::CURRENT_PROBE ) );
            else
                picker->SetCursor( SIM_CURSORS::GetCursor( SIM_CURSORS::VOLTAGE_PROBE ) );

            if( m_pickerItem != item )
            {

                if( m_pickerItem )
                    selectionTool->UnbrightenItem( m_pickerItem );

                m_pickerItem = item;

                if( m_pickerItem )
                    selectionTool->BrightenItem( m_pickerItem );
            }

            if( m_frame->GetSelectedNetName() != netName )
            {
                m_frame->SetSelectedNetName( netName );

                TOOL_EVENT dummyEvent;
                UpdateNetHighlighting( dummyEvent );
            }
        } );

    picker->SetFinalizeHandler(
        [this] ( const int& aFinalState )
        {
            if( m_pickerItem )
                m_toolMgr->GetTool<EE_SELECTION_TOOL>()->UnbrightenItem( m_pickerItem );

            if( !m_frame->GetSelectedNetName().IsEmpty() )
            {
                m_frame->SetSelectedNetName( wxEmptyString );

                TOOL_EVENT dummyEvent;
                UpdateNetHighlighting( dummyEvent );
            }
        } );

    std::string tool = aEvent.GetCommandStr().get();
    m_toolMgr->RunAction( ACTIONS::pickerTool, true, &tool );

    return 0;
}


int SCH_EDITOR_CONTROL::SimTune( const TOOL_EVENT& aEvent )
{
    PICKER_TOOL* picker = m_toolMgr->GetTool<PICKER_TOOL>();

    // Deactivate other tools; particularly important if another PICKER is currently running
    Activate();

    picker->SetCursor( SIM_CURSORS::GetCursor( SIM_CURSORS::CURSOR::TUNE ) );

    picker->SetClickHandler(
        [this] ( const VECTOR2D& aPosition )
        {
            EE_SELECTION_TOOL* selTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
            EDA_ITEM*          item = selTool->SelectPoint( aPosition, fieldsAndComponents );

            if( !item )
                return false;

            if( item->Type() != SCH_COMPONENT_T )
            {
                item = item->GetParent();

                if( item->Type() != SCH_COMPONENT_T )
                    return false;
            }

            SIM_PLOT_FRAME* simFrame =
                    (SIM_PLOT_FRAME*) m_frame->Kiway().Player( FRAME_SIMULATOR, false );

            if( simFrame )
                simFrame->AddTuner( static_cast<SCH_COMPONENT*>( item ) );

            return true;
        } );

    picker->SetMotionHandler(
        [this] ( const VECTOR2D& aPos )
        {
            EE_COLLECTOR collector;
            collector.m_Threshold = KiROUND( getView()->ToWorld( HITTEST_THRESHOLD_PIXELS ) );
            collector.Collect( m_frame->GetScreen()->GetDrawItems(), fieldsAndComponents, (wxPoint) aPos );

            EE_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
            selectionTool->GuessSelectionCandidates( collector, aPos );

            EDA_ITEM* item = collector.GetCount() == 1 ? collector[ 0 ] : nullptr;

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
                m_toolMgr->GetTool<EE_SELECTION_TOOL>()->UnbrightenItem( m_pickerItem );
        } );

    std::string tool = aEvent.GetCommandStr().get();
    m_toolMgr->RunAction( ACTIONS::pickerTool, true, &tool );

    return 0;
}
#endif /* KICAD_SPICE */


// A singleton reference for clearing the highlight
static VECTOR2D CLEAR;


// TODO(JE) Probably use netcode rather than connection name here eventually
static bool highlightNet( TOOL_MANAGER* aToolMgr, const VECTOR2D& aPosition )
{
    SCH_EDIT_FRAME*     editFrame = static_cast<SCH_EDIT_FRAME*>( aToolMgr->GetEditFrame() );
    EE_SELECTION_TOOL*  selTool = aToolMgr->GetTool<EE_SELECTION_TOOL>();
    SCH_EDITOR_CONTROL* editorControl = aToolMgr->GetTool<SCH_EDITOR_CONTROL>();
    wxString            netName;
    bool                retVal = true;

    if( aPosition != CLEAR )
    {
        if( TestDuplicateSheetNames( false ) > 0 )
        {
            wxMessageBox( _( "Error: duplicate sub-sheet names found in current sheet." ) );
            retVal = false;
        }
        else
        {
            SCH_ITEM* item = (SCH_ITEM*) selTool->GetNode( aPosition );

            if( item && item->Connection( *g_CurrentSheet ) )
                netName = item->Connection( *g_CurrentSheet )->Name();
        }
    }

    if( netName.empty() )
    {
        editFrame->SetStatusText( wxT( "" ) );
        editFrame->SendCrossProbeClearHighlight();
    }
    else
    {
        editFrame->SendCrossProbeNetName( netName );
        editFrame->SetStatusText( wxString::Format( _( "Highlighted net: %s" ),
                                                    UnescapeString( netName ) ) );
    }

    editFrame->SetSelectedNetName( netName );
    TOOL_EVENT dummy;
    editorControl->UpdateNetHighlighting( dummy );

    return retVal;
}


int SCH_EDITOR_CONTROL::HighlightNet( const TOOL_EVENT& aEvent )
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    VECTOR2D              cursorPos = controls->GetCursorPosition( !aEvent.Modifier( MD_ALT ) );

    highlightNet( m_toolMgr, cursorPos );

    return 0;
}


int SCH_EDITOR_CONTROL::ClearHighlight( const TOOL_EVENT& aEvent )
{
    highlightNet( m_toolMgr, CLEAR );

    return 0;
}


int SCH_EDITOR_CONTROL::UpdateNetHighlighting( const TOOL_EVENT& aEvent )
{
    SCH_SCREEN*            screen = g_CurrentSheet->LastScreen();
    std::vector<EDA_ITEM*> itemsToRedraw;
    wxString               selectedNetName = m_frame->GetSelectedNetName();

    if( !screen )
        return 0;

    for( SCH_ITEM* item = screen->GetDrawItems(); item; item = item->Next() )
    {
        SCH_CONNECTION* conn = item->Connection( *g_CurrentSheet );
        bool redraw = item->IsBrightened();

        if( conn && conn->Name() == selectedNetName )
            item->SetBrightened();
        else
            item->ClearBrightened();

        redraw |= item->IsBrightened();

        if( item->Type() == SCH_COMPONENT_T )
        {
            SCH_COMPONENT* comp = static_cast<SCH_COMPONENT*>( item );

            redraw |= comp->HasBrightenedPins();

            comp->ClearBrightenedPins();
            std::vector<LIB_PIN*> pins;
            comp->GetPins( pins );

            for( LIB_PIN* pin : pins )
            {
                auto pin_conn = comp->GetConnectionForPin( pin, *g_CurrentSheet );

                if( pin_conn && pin_conn->Name( false ) == selectedNetName )
                {
                    comp->BrightenPin( pin );
                    redraw = true;
                }
            }
        }
        else if( item->Type() == SCH_SHEET_T )
        {
            for( SCH_SHEET_PIN& pin : static_cast<SCH_SHEET*>( item )->GetPins() )
            {
                auto pin_conn = pin.Connection( *g_CurrentSheet );
                bool redrawPin = pin.IsBrightened();

                if( pin_conn && pin_conn->Name() == selectedNetName )
                    pin.SetBrightened();
                else
                    pin.ClearBrightened();

                redrawPin |= pin.IsBrightened();

                if( redrawPin )
                    itemsToRedraw.push_back( &pin );
            }
        }

        if( redraw )
            itemsToRedraw.push_back( item );
    }

    // Be sure hightlight change will be redrawn
    KIGFX::VIEW* view = getView();

    for( auto redrawItem : itemsToRedraw )
        view->Update( (KIGFX::VIEW_ITEM*)redrawItem, KIGFX::VIEW_UPDATE_FLAGS::REPAINT );

    m_frame->GetCanvas()->Refresh();

    return 0;
}


int SCH_EDITOR_CONTROL::HighlightNetCursor( const TOOL_EVENT& aEvent )
{
    // TODO(JE) remove once real-time connectivity is a given
    if( !ADVANCED_CFG::GetCfg().m_realTimeConnectivity || !CONNECTION_GRAPH::m_allowRealTime )
        m_frame->RecalculateConnections();

    std::string  tool = aEvent.GetCommandStr().get();
    PICKER_TOOL* picker = m_toolMgr->GetTool<PICKER_TOOL>();

    // Deactivate other tools; particularly important if another PICKER is currently running
    Activate();

    picker->SetCursor( wxStockCursor( wxCURSOR_BULLSEYE ) );

    picker->SetClickHandler(
        [this] ( const VECTOR2D& aPos )
        {
            return highlightNet( m_toolMgr, aPos );
        } );

    m_toolMgr->RunAction( ACTIONS::pickerTool, true, &tool );

    return 0;
}


int SCH_EDITOR_CONTROL::Undo( const TOOL_EVENT& aEvent )
{
    if( m_frame->GetScreen()->GetUndoCommandCount() <= 0 )
        return 0;

    // Inform tools that undo command was issued
    m_toolMgr->ProcessEvent( { TC_MESSAGE, TA_UNDO_REDO_PRE, AS_GLOBAL } );

    /* Get the old list */
    PICKED_ITEMS_LIST* List = m_frame->GetScreen()->PopCommandFromUndoList();

    /* Undo the command */
    m_frame->PutDataInPreviousState( List, false );

    /* Put the old list in RedoList */
    List->ReversePickersListOrder();
    m_frame->GetScreen()->PushCommandToRedoList( List );

    EE_SELECTION_TOOL* selTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
    selTool->RebuildSelection();

    m_frame->SetSheetNumberAndCount();
    m_frame->TestDanglingEnds();

    m_frame->SyncView();
    m_frame->GetCanvas()->Refresh();
    m_frame->OnModify();

    return 0;
}


int SCH_EDITOR_CONTROL::Redo( const TOOL_EVENT& aEvent )
{
    if( m_frame->GetScreen()->GetRedoCommandCount() == 0 )
        return 0;

    // Inform tools that undo command was issued
    m_toolMgr->ProcessEvent( { TC_MESSAGE, TA_UNDO_REDO_PRE, AS_GLOBAL } );

    /* Get the old list */
    PICKED_ITEMS_LIST* List = m_frame->GetScreen()->PopCommandFromRedoList();

    /* Redo the command: */
    m_frame->PutDataInPreviousState( List, true );

    /* Put the old list in UndoList */
    List->ReversePickersListOrder();
    m_frame->GetScreen()->PushCommandToUndoList( List );

    EE_SELECTION_TOOL* selTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
    selTool->RebuildSelection();

    m_frame->SetSheetNumberAndCount();
    m_frame->TestDanglingEnds();

    m_frame->SyncView();
    m_frame->GetCanvas()->Refresh();
    m_frame->OnModify();

    return 0;
}


bool SCH_EDITOR_CONTROL::doCopy()
{
    EE_SELECTION_TOOL* selTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
    EE_SELECTION&      selection = selTool->RequestSelection();

    if( !selection.GetSize() )
        return false;

    STRING_FORMATTER formatter;
    SCH_LEGACY_PLUGIN plugin;

    plugin.Format( &selection, &formatter );

    return m_toolMgr->SaveClipboard( formatter.GetString() );
}


int SCH_EDITOR_CONTROL::Cut( const TOOL_EVENT& aEvent )
{
    wxTextEntry* textEntry = dynamic_cast<wxTextEntry*>( wxWindow::FindFocus() );

    if( textEntry )
    {
        textEntry->Cut();
        return 0;
    }

    if( doCopy() )
        m_toolMgr->RunAction( ACTIONS::doDelete, true );

    return 0;
}


int SCH_EDITOR_CONTROL::Copy( const TOOL_EVENT& aEvent )
{
    wxTextEntry* textEntry = dynamic_cast<wxTextEntry*>( wxWindow::FindFocus() );

    if( textEntry )
    {
        textEntry->Copy();
        return 0;
    }

    doCopy();

    return 0;
}


int SCH_EDITOR_CONTROL::Paste( const TOOL_EVENT& aEvent )
{
    wxTextEntry* textEntry = dynamic_cast<wxTextEntry*>( wxWindow::FindFocus() );

    if( textEntry )
    {
        textEntry->Paste();
        return 0;
    }

    EE_SELECTION_TOOL* selTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();

    DLIST<SCH_ITEM>&   dlist = m_frame->GetScreen()->GetDrawList();
    SCH_ITEM*          last = dlist.GetLast();

    std::string        text = m_toolMgr->GetClipboard();

    if( text.empty() )
        return 0;

    STRING_LINE_READER reader( text, "Clipboard" );
    SCH_LEGACY_PLUGIN  plugin;

    try
    {
        plugin.LoadContent( reader, m_frame->GetScreen() );
    }
    catch( IO_ERROR& e )
    {
        // If it wasn't content, then paste as text
        dlist.Append( new SCH_TEXT( wxPoint( 0, 0 ), text ) );
    }

    // SCH_LEGACY_PLUGIN added the items to the DLIST, but not to the view or anything
    // else.  Pull them back out to start with.
    //
    EDA_ITEMS loadedItems;
    SCH_ITEM* next = nullptr;

    // We also make sure any pasted sheets will not cause recursion in the destination.
    // Moreover new sheets create new sheetpaths, and component alternate references must
    // be created and cleared
    //
    bool           sheetsPasted = false;
    SCH_SHEET_LIST hierarchy( g_RootSheet );
    wxFileName     destFn = g_CurrentSheet->Last()->GetFileName();

    if( destFn.IsRelative() )
        destFn.MakeAbsolute( m_frame->Prj().GetProjectPath() );

    for( SCH_ITEM* item = last ? last->Next() : dlist.GetFirst(); item; item = next )
    {
        next = item->Next();
        dlist.Remove( item );

        loadedItems.push_back( item );

        if( item->Type() == SCH_COMPONENT_T )
        {
            SCH_COMPONENT* component = (SCH_COMPONENT*) item;

            component->SetTimeStamp( GetNewTimeStamp() );

            // clear the annotation, but preserve the selected unit
            int unit = component->GetUnit();
            component->ClearAnnotation( nullptr );
            component->SetUnit( unit );
        }
        if( item->Type() == SCH_SHEET_T )
        {
            SCH_SHEET* sheet = (SCH_SHEET*) item;
            wxFileName srcFn = sheet->GetFileName();

            if( srcFn.IsRelative() )
                srcFn.MakeAbsolute( m_frame->Prj().GetProjectPath() );

            SCH_SHEET_LIST sheetHierarchy( sheet );

            if( hierarchy.TestForRecursion( sheetHierarchy, destFn.GetFullPath( wxPATH_UNIX ) ) )
            {
                auto msg = wxString::Format( _( "The pasted sheet \"%s\"\n"
                                                "was dropped because the destination already has "
                                                "the sheet or one of its subsheets as a parent." ),
                                             sheet->GetFileName() );
                DisplayError( m_frame, msg );
                loadedItems.pop_back();
            }
            else
            {
                // Duplicate sheet names and sheet time stamps are not valid.  Use a time stamp
                // based sheet name and update the time stamp for each sheet in the block.
                timestamp_t uid = GetNewTimeStamp();

                sheet->SetName( wxString::Format( wxT( "sheet%8.8lX" ), (unsigned long)uid ) );
                sheet->SetTimeStamp( uid );
                sheet->SetParent( g_CurrentSheet->Last() );
                sheet->SetScreen( nullptr );
                sheetsPasted = true;
            }
        }
    }

    // Now we can resolve the components and add everything to the screen, view, etc.
    //
    SYMBOL_LIB_TABLE* symLibTable = m_frame->Prj().SchSymbolLibTable();
    PART_LIB*         partLib = m_frame->Prj().SchLibs()->GetCacheLibrary();

    for( unsigned i = 0; i < loadedItems.size(); ++i )
    {
        EDA_ITEM* item = loadedItems[i];

        if( item->Type() == SCH_COMPONENT_T )
        {
            SCH_COMPONENT* component = (SCH_COMPONENT*) item;
            component->Resolve( *symLibTable, partLib );
            component->UpdatePins();
        }
        else if( item->Type() == SCH_SHEET_T )
        {
            SCH_SHEET*  sheet = (SCH_SHEET*) item;
            wxFileName  fn = sheet->GetFileName();
            SCH_SCREEN* existingScreen = nullptr;

            if( !fn.IsAbsolute() )
            {
                wxFileName currentSheetFileName = g_CurrentSheet->LastScreen()->GetFileName();
                fn.Normalize( wxPATH_NORM_ALL, currentSheetFileName.GetPath() );
            }

            if( g_RootSheet->SearchHierarchy( fn.GetFullPath( wxPATH_UNIX ), &existingScreen ) )
            {
                sheet->SetScreen( existingScreen );

                SCH_SHEET_PATH sheetpath = *g_CurrentSheet;
                sheetpath.push_back( sheet );

                // Clear annotation and create the AR for this path, if not exists,
                // when the screen is shared by sheet paths.
                // Otherwise ClearAnnotation do nothing, because the F1 field is used as
                // reference default value and takes the latest displayed value
                existingScreen->EnsureAlternateReferencesExist();
                existingScreen->ClearAnnotation( &sheetpath );
            }
            else
            {
                m_frame->LoadSheetFromFile( sheet, g_CurrentSheet, sheet->GetFileName() );
            }
        }

        item->SetFlags( IS_NEW | IS_PASTED | IS_MOVED );
        m_frame->AddItemToScreenAndUndoList( (SCH_ITEM*) item, i > 0 );

        // Reset flags for subsequent move operation
        item->SetFlags( IS_NEW | IS_PASTED | IS_MOVED );
    }

    if( sheetsPasted )
        m_frame->SetSheetNumberAndCount();

    // Now clear the previous selection, select the pasted items, and fire up the "move"
    // tool.
    //
    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
    m_toolMgr->RunAction( EE_ACTIONS::addItemsToSel, true, &loadedItems );

    EE_SELECTION& selection = selTool->GetSelection();

    if( !selection.Empty() )
    {
        SCH_ITEM* item = (SCH_ITEM*) selection.GetTopLeftItem();

        selection.SetReferencePoint( item->GetPosition() );
        m_toolMgr->RunAction( EE_ACTIONS::move, false );
    }

    return 0;
}


int SCH_EDITOR_CONTROL::EditWithLibEdit( const TOOL_EVENT& aEvent )
{
    EE_SELECTION_TOOL* selTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
    EE_SELECTION&      selection = selTool->RequestSelection( EE_COLLECTOR::ComponentsOnly );
    SCH_COMPONENT*     sym = nullptr;
    LIB_EDIT_FRAME*    libEdit;

    if( selection.GetSize() >= 1 )
        sym = (SCH_COMPONENT*) selection.Front();

    if( !sym || sym->GetEditFlags() != 0 )
        return 0;

    m_toolMgr->RunAction( ACTIONS::showSymbolEditor, true );
    libEdit = (LIB_EDIT_FRAME*) m_frame->Kiway().Player( FRAME_SCH_LIB_EDITOR, false );

    if( libEdit )
        libEdit->LoadComponentAndSelectLib( sym->GetLibId(), sym->GetUnit(), sym->GetConvert() );

    return 0;
}


int SCH_EDITOR_CONTROL::Annotate( const TOOL_EVENT& aEvent )
{
    wxCommandEvent dummy;
    m_frame->OnAnnotate( dummy );
    return 0;
}


int SCH_EDITOR_CONTROL::ShowCvpcb( const TOOL_EVENT& aEvent )
{
    wxCommandEvent dummy;
    m_frame->OnOpenCvpcb( dummy );
    return 0;
}


int SCH_EDITOR_CONTROL::EditSymbolFields( const TOOL_EVENT& aEvent )
{
    DIALOG_FIELDS_EDITOR_GLOBAL dlg( m_frame );
    dlg.ShowQuasiModal();
    return 0;
}


int SCH_EDITOR_CONTROL::EditSymbolLibraryLinks( const TOOL_EVENT& aEvent )
{
    if( InvokeDialogEditComponentsLibId( m_frame ) )
        m_frame->HardRedraw();

    return 0;
}


int SCH_EDITOR_CONTROL::ShowPcbNew( const TOOL_EVENT& aEvent )
{
    wxCommandEvent dummy;
    m_frame->OnOpenPcbnew( dummy );
    return 0;
}


int SCH_EDITOR_CONTROL::UpdatePCB( const TOOL_EVENT& aEvent )
{
    wxCommandEvent dummy;
    m_frame->OnUpdatePCB( dummy );
    return 0;
}


int SCH_EDITOR_CONTROL::ExportNetlist( const TOOL_EVENT& aEvent )
{
    int result = NET_PLUGIN_CHANGE;

    // If a plugin is removed or added, rebuild and reopen the new dialog
    while( result == NET_PLUGIN_CHANGE )
        result = InvokeDialogNetList( m_frame );

    return 0;
}


int SCH_EDITOR_CONTROL::GenerateBOM( const TOOL_EVENT& aEvent )
{
    InvokeDialogCreateBOM( m_frame );
    return 0;
}


int SCH_EDITOR_CONTROL::DrawSheetOnClipboard( const TOOL_EVENT& aEvent )
{
    m_frame->DrawCurrentSheetToClipboard();
    return 0;
}


int SCH_EDITOR_CONTROL::ShowBusManager( const TOOL_EVENT& aEvent )
{
    InvokeDialogBusManager( m_frame );
    return 0;
}


int SCH_EDITOR_CONTROL::EnterSheet( const TOOL_EVENT& aEvent )
{
    EE_SELECTION_TOOL*  selTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
    const EE_SELECTION& selection = selTool->RequestSelection( EE_COLLECTOR::SheetsOnly );

    if( selection.GetSize() == 1 )
    {
        SCH_SHEET* sheet = (SCH_SHEET*) selection.Front();

        m_toolMgr->RunAction( ACTIONS::cancelInteractive, true );
        m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

        g_CurrentSheet->push_back( sheet );
        m_frame->DisplayCurrentSheet();
    }

    return 0;
}


int SCH_EDITOR_CONTROL::LeaveSheet( const TOOL_EVENT& aEvent )
{
    if( g_CurrentSheet->Last() != g_RootSheet )
    {
        m_toolMgr->RunAction( ACTIONS::cancelInteractive, true );
        m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

        g_CurrentSheet->pop_back();
        m_frame->DisplayCurrentSheet();
    }

    return 0;
}


int SCH_EDITOR_CONTROL::ToggleHiddenPins( const TOOL_EVENT& aEvent )
{
    m_frame->SetShowAllPins( !m_frame->GetShowAllPins() );

    auto painter = static_cast<KIGFX::SCH_PAINTER*>( getView()->GetPainter() );
    painter->GetSettings()->m_ShowHiddenPins = m_frame->GetShowAllPins();

    getView()->UpdateAllItems( KIGFX::REPAINT );
    m_frame->GetCanvas()->Refresh();

    return 0;
}


int SCH_EDITOR_CONTROL::ToggleForceHV( const TOOL_EVENT& aEvent )
{
    m_frame->SetForceHVLines( !m_frame->GetForceHVLines() );

    return 0;
}


void SCH_EDITOR_CONTROL::setTransitions()
{
    Go( &SCH_EDITOR_CONTROL::New,                   ACTIONS::doNew.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::Open,                  ACTIONS::open.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::Save,                  ACTIONS::save.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::SaveAs,                ACTIONS::saveAs.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::SaveAll,               ACTIONS::saveAll.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::PageSetup,             ACTIONS::pageSettings.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::Print,                 ACTIONS::print.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::Plot,                  ACTIONS::plot.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::Quit,                  ACTIONS::quit.MakeEvent() );

    Go( &SCH_EDITOR_CONTROL::FindAndReplace,        ACTIONS::find.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::FindAndReplace,        ACTIONS::findAndReplace.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::FindNext,              ACTIONS::findNext.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::FindNext,              ACTIONS::findNextMarker.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ReplaceAndFindNext,    ACTIONS::replaceAndFindNext.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ReplaceAll,            ACTIONS::replaceAll.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::UpdateFind,            ACTIONS::updateFind.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::UpdateFind,            EVENTS::SelectedItemsModified );

    Go( &SCH_EDITOR_CONTROL::CrossProbeToPcb,       EVENTS::SelectedEvent );
    Go( &SCH_EDITOR_CONTROL::CrossProbeToPcb,       EVENTS::UnselectedEvent );
    Go( &SCH_EDITOR_CONTROL::CrossProbeToPcb,       EVENTS::ClearedEvent );
    Go( &SCH_EDITOR_CONTROL::ExplicitCrossProbeToPcb, EE_ACTIONS::explicitCrossProbe.MakeEvent() );

#ifdef KICAD_SPICE
    Go( &SCH_EDITOR_CONTROL::SimProbe,              EE_ACTIONS::simProbe.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::SimTune,               EE_ACTIONS::simTune.MakeEvent() );
#endif /* KICAD_SPICE */

    Go( &SCH_EDITOR_CONTROL::HighlightNet,          EE_ACTIONS::highlightNet.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ClearHighlight,        EE_ACTIONS::clearHighlight.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::HighlightNetCursor,    EE_ACTIONS::highlightNetTool.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::UpdateNetHighlighting, EVENTS::SelectedItemsModified );
    Go( &SCH_EDITOR_CONTROL::UpdateNetHighlighting, EE_ACTIONS::updateNetHighlighting.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ClearHighlight,        ACTIONS::cancelInteractive.MakeEvent() );

    Go( &SCH_EDITOR_CONTROL::Undo,                  ACTIONS::undo.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::Redo,                  ACTIONS::redo.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::Cut,                   ACTIONS::cut.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::Copy,                  ACTIONS::copy.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::Paste,                 ACTIONS::paste.MakeEvent() );

    Go( &SCH_EDITOR_CONTROL::EditWithLibEdit,       EE_ACTIONS::editWithLibEdit.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ShowCvpcb,             EE_ACTIONS::assignFootprints.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ImportFPAssignments,   EE_ACTIONS::importFPAssignments.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::Annotate,              EE_ACTIONS::annotate.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::EditSymbolFields,      EE_ACTIONS::editSymbolFields.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::EditSymbolLibraryLinks,EE_ACTIONS::editSymbolLibraryLinks.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ShowPcbNew,            EE_ACTIONS::showPcbNew.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::UpdatePCB,             ACTIONS::updatePcbFromSchematic.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ExportNetlist,         EE_ACTIONS::exportNetlist.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::GenerateBOM,           EE_ACTIONS::generateBOM.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::DrawSheetOnClipboard,  EE_ACTIONS::drawSheetOnClipboard.MakeEvent() );

    Go( &SCH_EDITOR_CONTROL::ShowBusManager,        EE_ACTIONS::showBusManager.MakeEvent() );

    Go( &SCH_EDITOR_CONTROL::EnterSheet,            EE_ACTIONS::enterSheet.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::LeaveSheet,            EE_ACTIONS::leaveSheet.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::NavigateHierarchy,     EE_ACTIONS::navigateHierarchy.MakeEvent() );

    Go( &SCH_EDITOR_CONTROL::ToggleHiddenPins,      EE_ACTIONS::toggleHiddenPins.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ToggleForceHV,         EE_ACTIONS::toggleForceHV.MakeEvent() );
}
