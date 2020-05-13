/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <class_library.h>
#include <confirm.h>
#include <connection_graph.h>
#include <dialogs/dialog_fields_editor_global.h>
#include <dialogs/dialog_page_settings.h>
#include <dialogs/dialog_paste_special.h>
#include <dialogs/dialog_plot_schematic.h>
#include <dialogs/dialog_symbol_remap.h>
#include <project_rescue.h>
#include <erc.h>
#include <fctsys.h>
#include <invoke_sch_dialog.h>
#include <kiway.h>
#include <netlist_exporters/netlist_exporter_pspice.h>
#include <netlist_object.h>
#include <sch_edit_frame.h>
#include <sch_sexpr_plugin.h>
#include <sch_line.h>
#include <sch_painter.h>
#include <sch_sheet.h>
#include <sch_view.h>
#include <schematic.h>
#include <advanced_config.h>
#include <sim/sim_plot_frame.h>
#include <simulation_cursors.h>
#include <lib_view_frame.h>
#include <status_popup.h>
#include <tool/picker_tool.h>
#include <tool/tool_manager.h>
#include <tools/ee_actions.h>
#include <tools/ee_selection.h>
#include <tools/ee_selection_tool.h>
#include <tools/sch_editor_control.h>
#include <ws_proxy_undo_item.h>
#include <math/util.h>      // for KiROUND
#include <dialog_update_from_pcb.h>


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
    m_frame->SaveProject();
    return 0;
}


int SCH_EDITOR_CONTROL::SaveAs( const TOOL_EVENT& aEvent )
{
    m_frame->Save_File( true );
    return 0;
}


int SCH_EDITOR_CONTROL::ShowSchematicSetup( const TOOL_EVENT& aEvent )
{
    m_frame->DoShowSchematicSetupDialog();
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


int SCH_EDITOR_CONTROL::RescueSymbols( const TOOL_EVENT& aEvent )
{
    SCH_SCREENS schematic( m_frame->Schematic().Root() );

    if( schematic.HasNoFullyDefinedLibIds() )
        RescueLegacyProject( true );
    else
        RescueSymbolLibTableProject( true );

    return 0;
}


bool SCH_EDITOR_CONTROL::RescueLegacyProject( bool aRunningOnDemand )
{
    LEGACY_RESCUER rescuer( m_frame->Prj(), &m_frame->Schematic(), &m_frame->GetCurrentSheet(),
                            m_frame->GetCanvas()->GetBackend() );

    return rescueProject( rescuer, aRunningOnDemand );
}


bool SCH_EDITOR_CONTROL::RescueSymbolLibTableProject( bool aRunningOnDemand )
{
    SYMBOL_LIB_TABLE_RESCUER rescuer( m_frame->Prj(), &m_frame->Schematic(),
                                      &m_frame->GetCurrentSheet(),
                                      m_frame->GetCanvas()->GetBackend() );

    return rescueProject( rescuer, aRunningOnDemand );
}


bool SCH_EDITOR_CONTROL::rescueProject( RESCUER& aRescuer, bool aRunningOnDemand )
{
    if( !RESCUER::RescueProject( m_frame, aRescuer, aRunningOnDemand ) )
        return false;

    if( aRescuer.GetCandidateCount() )
    {
        KIWAY_PLAYER* viewer = m_frame->Kiway().Player( FRAME_SCH_VIEWER, false );

        if( viewer )
            static_cast<LIB_VIEW_FRAME*>( viewer )->ReCreateListLib();

        if( aRunningOnDemand )
        {
            SCH_SCREENS schematic( m_frame->Schematic().Root() );

            schematic.UpdateSymbolLinks();
            m_frame->RecalculateConnections( GLOBAL_CLEANUP );
        }

        m_frame->GetScreen()->ClearUndoORRedoList( m_frame->GetScreen()->m_UndoList, 1 );
        m_frame->SyncView();
        m_frame->GetCanvas()->Refresh();
        m_frame->OnModify();
    }

    return true;
}


int SCH_EDITOR_CONTROL::RemapSymbols( const TOOL_EVENT& aEvent )
{
    DIALOG_SYMBOL_REMAP dlgRemap( m_frame );

    dlgRemap.ShowQuasiModal();

    m_frame->GetCanvas()->Refresh( true );

    return 0;
}


int SCH_EDITOR_CONTROL::Print( const TOOL_EVENT& aEvent )
{
    InvokeDialogPrintUsingPrinter( m_frame );

    wxFileName fn = m_frame->Prj().AbsolutePath( m_frame->Schematic().RootScreen()->GetFileName() );

    if( fn.GetName() != NAMELESS_PROJECT )
        m_frame->SaveProjectSettings();

    return 0;
}


int SCH_EDITOR_CONTROL::Plot( const TOOL_EVENT& aEvent )
{
    DIALOG_PLOT_SCHEMATIC dlg( m_frame );

    dlg.ShowModal();

    // save project config if the prj config has changed:
    if( dlg.PrjConfigChanged() )
        m_frame->SaveProjectSettings();

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

int SCH_EDITOR_CONTROL::NavigateHierarchy( const TOOL_EVENT& aEvent )
{
    m_frame->UpdateHierarchyNavigator( true );
    return 0;
}

int SCH_EDITOR_CONTROL::UpdateFind( const TOOL_EVENT& aEvent )
{
    wxFindReplaceData* data = m_frame->GetFindReplaceData();

    auto visit = [&] ( EDA_ITEM* aItem )
                 {
                     if( data && aItem->Matches( *data, nullptr ) )
                     {
                         aItem->SetForceVisible( true );
                         m_selectionTool->BrightenItem( aItem );
                     }
                     else if( aItem->IsBrightened() )
                     {
                         aItem->SetForceVisible( false );
                         m_selectionTool->UnbrightenItem( aItem );
                     }
                 };

    if( aEvent.IsAction( &ACTIONS::find ) || aEvent.IsAction( &ACTIONS::findAndReplace )
        || aEvent.IsAction( &ACTIONS::updateFind ) )
    {
        m_selectionTool->ClearSelection();

        for( SCH_ITEM* item : m_frame->GetScreen()->Items() )
        {
            visit( item );

            if( item->Type() == SCH_COMPONENT_T )
            {
                SCH_COMPONENT* cmp = static_cast<SCH_COMPONENT*>( item );

                for( SCH_FIELD& field : cmp->GetFields() )
                    visit( &field );

                for( SCH_PIN* pin : cmp->GetSchPins() )
                    visit( pin );
            }
            else if( item->Type() == SCH_SHEET_T )
            {
                SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );

                for( SCH_FIELD& field : sheet->GetFields() )
                    visit( &field );

                for( SCH_SHEET_PIN* pin : sheet->GetPins() )
                    visit( pin );
            }

        }
    }
    else if( aEvent.Matches( EVENTS::SelectedItemsModified ) )
    {
        for( EDA_ITEM* item : m_selectionTool->GetSelection() )
            visit( item );
    }

    getView()->UpdateItems();
    m_frame->GetCanvas()->Refresh();
    m_frame->UpdateTitle();

    return 0;
}


SCH_ITEM* SCH_EDITOR_CONTROL::nextMatch( SCH_SCREEN* aScreen, SCH_ITEM* aAfter,
                                         wxFindReplaceData* aData )
{
    bool past_item = true;

    if( aAfter != nullptr )
    {
        past_item = false;

        if( aAfter->Type() == SCH_PIN_T || aAfter->Type() == SCH_FIELD_T )
            aAfter = static_cast<SCH_ITEM*>( aAfter->GetParent() );
    }


    for( SCH_ITEM* item : aScreen->Items() )
    {
        if( item == aAfter )
        {
            past_item = true;
        }
        else if( past_item )
        {
            if( aData == &g_markersOnly && item->Type() == SCH_MARKER_T )
                return item;

            if( item->Matches( *aData, nullptr ) )
                return item;

            if( item->Type() == SCH_COMPONENT_T )
            {
                SCH_COMPONENT* cmp = static_cast<SCH_COMPONENT*>( item );

                for( SCH_FIELD& field : cmp->GetFields() )
                {
                    if( field.Matches( *aData, nullptr ) )
                        return &field;
                }

                for( SCH_PIN* pin : cmp->GetSchPins() )
                {
                    if( pin->Matches( *aData, nullptr ) )
                        return pin;
                }
            }

            if( item->Type() == SCH_SHEET_T )
            {
                SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );

                for( SCH_FIELD& field : sheet->GetFields() )
                {
                    if( field.Matches( *aData, nullptr ) )
                        return &field;
                }

                for( SCH_SHEET_PIN* pin : sheet->GetPins() )
                {
                    if( pin->Matches( *aData, nullptr ) )
                        return pin;
                }
            }
        }
    }

    return nullptr;
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
    SCH_ITEM*     afterItem       = dynamic_cast<SCH_ITEM*>( selection.Front() );
    SCH_ITEM*     item            = nullptr;

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
        SCH_SHEET_LIST schematic = m_frame->Schematic().GetSheets();
        SCH_SCREENS    screens( m_frame->Schematic().Root() );

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

                m_frame->Schematic().SetCurrentSheet( *sheet );
                m_frame->GetCurrentSheet().UpdateAllScreenReferences();

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
        m_frame->FocusOnLocation( item->GetBoundingBox().GetCenter() );
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
        if( item->Replace( *data, &m_frame->GetCurrentSheet() ) )
        {
            m_frame->RefreshItem( item );
            m_frame->OnModify();
        }

        FindNext( ACTIONS::findNext.MakeEvent() );
    }

    return 0;
}


int SCH_EDITOR_CONTROL::ReplaceAll( const TOOL_EVENT& aEvent )
{
    wxFindReplaceData* data = m_frame->GetFindReplaceData();

    if( !data )
        return FindAndReplace( ACTIONS::find.MakeEvent() );

    SCH_SHEET_LIST schematic = m_frame->Schematic().GetSheets();
    SCH_SCREENS    screens( m_frame->Schematic().Root() );

    for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
    {
        for( EDA_ITEM* item = nextMatch( screen, nullptr, data ); item;  )
        {
            if( item->Replace( *data, schematic.FindSheetForScreen( screen ) ) )
            {
                m_frame->RefreshItem( item );
                m_frame->OnModify();
            }

            item = nextMatch( screen, dynamic_cast<SCH_ITEM*>( item ), data );
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
        if( item->GetParent() && item->GetParent()->Type() == SCH_COMPONENT_T )
        {
            component = (SCH_COMPONENT*) item->GetParent();
            m_frame->SendMessageToPCBNEW( item, component );
        }
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
                wxString       param;
                wxString       primitive;

                primitive = NETLIST_EXPORTER_PSPICE::GetSpiceField( SF_PRIMITIVE, comp, 0 );
                primitive.LowerCase();

                if( primitive == "c" || primitive == "l" || primitive == "r" )
                    param = wxT( "I" );
                else if( primitive == "d" )
                    param = wxT( "Id" );
                else
                    param = wxString::Format( wxT( "I%s" ), pin->GetName().Lower() );

                simFrame->AddCurrentPlot( comp->GetRef( &m_frame->GetCurrentSheet() ), param );
            }

            return true;
        } );

    picker->SetMotionHandler(
        [this, picker] ( const VECTOR2D& aPos )
        {
            EE_COLLECTOR collector;
            collector.m_Threshold = KiROUND( getView()->ToWorld( HITTEST_THRESHOLD_PIXELS ) );
            collector.Collect( m_frame->GetScreen(), wiresAndPins, (wxPoint) aPos );

            EE_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
            selectionTool->GuessSelectionCandidates( collector, aPos );

            EDA_ITEM* item = collector.GetCount() == 1 ? collector[ 0 ] : nullptr;
            SCH_LINE* wire = dynamic_cast<SCH_LINE*>( item );
            wxString  netName;

            if( wire )
            {
                item = nullptr;

                if( wire->Connection( m_frame->GetCurrentSheet() ) )
                    netName = wire->Connection( m_frame->GetCurrentSheet() )->Name();
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
            collector.Collect( m_frame->GetScreen(), fieldsAndComponents, (wxPoint) aPos );

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
    SCH_EDIT_FRAME*     editFrame = static_cast<SCH_EDIT_FRAME*>( aToolMgr->GetToolHolder() );
    EE_SELECTION_TOOL*  selTool = aToolMgr->GetTool<EE_SELECTION_TOOL>();
    SCH_EDITOR_CONTROL* editorControl = aToolMgr->GetTool<SCH_EDITOR_CONTROL>();
    wxString            netName;
    bool                retVal = true;

    if( aPosition != CLEAR )
    {
        if( TestDuplicateSheetNames( &editFrame->Schematic(), false ) > 0 )
        {
            wxMessageBox( _( "Error: duplicate sub-sheet names found in current sheet." ) );
            retVal = false;
        }
        else
        {
            SCH_ITEM*      item = static_cast<SCH_ITEM*>( selTool->GetNode( aPosition ) );
            SCH_COMPONENT* comp = nullptr;

            if( item )
            {
                if( item->Type() == SCH_FIELD_T )
                    comp = dynamic_cast<SCH_COMPONENT*>( item->GetParent() );
                else
                    comp = dynamic_cast<SCH_COMPONENT*>( item );
            }

            if( comp && comp->GetPartRef() && comp->GetPartRef()->IsPower() )
                netName = comp->GetPartRef()->GetName();
            else if( item && item->Connection( editFrame->GetCurrentSheet() ) )
                netName = item->Connection( editFrame->GetCurrentSheet() )->Name();
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
    SCH_SCREEN*            screen = m_frame->GetCurrentSheet().LastScreen();
    std::vector<EDA_ITEM*> itemsToRedraw;
    wxString               selectedNetName = m_frame->GetSelectedNetName();

    if( !screen )
        return 0;

    for( SCH_ITEM* item : screen->Items() )
    {
        wxString        itemConnectionName;
        SCH_COMPONENT*  comp = nullptr;
        bool            redraw = item->IsBrightened();

        if( item->Type() == SCH_COMPONENT_T )
            comp = static_cast<SCH_COMPONENT*>( item );

        if( comp && comp->GetPartRef() && comp->GetPartRef()->IsPower() )
        {
            itemConnectionName = comp->GetPartRef()->GetName();
        }
        else
        {
            SCH_CONNECTION* connection = item->Connection( m_frame->GetCurrentSheet() );

            if( connection )
                itemConnectionName = connection->Name();
        }

        if( !selectedNetName.IsEmpty() && itemConnectionName == selectedNetName )
            item->SetBrightened();
        else
            item->ClearBrightened();

        redraw |= item->IsBrightened();

        if( item->Type() == SCH_COMPONENT_T )
        {
            redraw |= comp->HasBrightenedPins();

            comp->ClearBrightenedPins();
            std::vector<LIB_PIN*> pins;
            comp->GetPins( pins );

            for( LIB_PIN* pin : pins )
            {
                SCH_CONNECTION* pin_conn =
                        comp->GetConnectionForPin( pin, m_frame->GetCurrentSheet() );

                if( comp && pin_conn && pin_conn->Name( false ) == selectedNetName )
                {
                    comp->BrightenPin( pin );
                    redraw = true;
                }
            }

            if( comp->GetPartRef() && comp->GetPartRef()->IsPower() )
            {
                std::vector<SCH_FIELD>& fields = comp->GetFields();

                for( int id : { REFERENCE, VALUE } )
                {
                    if( item->IsBrightened() && fields[id].IsVisible() )
                        fields[id].SetBrightened();
                    else
                        fields[id].ClearBrightened();
                }
            }
        }
        else if( item->Type() == SCH_SHEET_T )
        {
            for( SCH_SHEET_PIN* pin : static_cast<SCH_SHEET*>( item )->GetPins() )
            {
                SCH_CONNECTION* pin_conn = pin->Connection( m_frame->GetCurrentSheet() );
                bool            redrawPin = pin->IsBrightened();

                if( pin_conn && pin_conn->Name() == selectedNetName )
                    pin->SetBrightened();
                else
                    pin->ClearBrightened();

                redrawPin |= pin->IsBrightened();

                if( redrawPin )
                    itemsToRedraw.push_back( pin );
            }
        }

        if( redraw )
            itemsToRedraw.push_back( item );
    }

    // Be sure highlight change will be redrawn
    KIGFX::VIEW* view = getView();

    for( EDA_ITEM* redrawItem : itemsToRedraw )
        view->Update( (KIGFX::VIEW_ITEM*)redrawItem, KIGFX::VIEW_UPDATE_FLAGS::REPAINT );

    m_frame->GetCanvas()->Refresh();

    return 0;
}


int SCH_EDITOR_CONTROL::HighlightNetCursor( const TOOL_EVENT& aEvent )
{
    // TODO(JE) remove once real-time connectivity is a given
    if( !ADVANCED_CFG::GetCfg().m_realTimeConnectivity || !CONNECTION_GRAPH::m_allowRealTime )
        m_frame->RecalculateConnections( NO_CLEANUP );

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

    selection.SetScreen( m_frame->GetScreen() );
    m_supplementaryClipboard.clear();

    for( EDA_ITEM* item : selection )
    {
        if( item->Type() == SCH_SHEET_T )
        {
            SCH_SHEET* sheet = (SCH_SHEET*) item;
            m_supplementaryClipboard[ sheet->GetFileName() ] = sheet->GetScreen();
        }
    }

    STRING_FORMATTER formatter;
    SCH_SEXPR_PLUGIN plugin;

    plugin.Format( &selection, &formatter );

    return m_toolMgr->SaveClipboard( formatter.GetString() );
}


bool SCH_EDITOR_CONTROL::searchSupplementaryClipboard( const wxString& aSheetFilename,
                                                       SCH_SCREEN** aScreen )
{
    if( m_supplementaryClipboard.count( aSheetFilename ) > 0 )
    {
        *aScreen = m_supplementaryClipboard[ aSheetFilename ];
        return true;
    }

    return false;
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
    std::string        text = m_toolMgr->GetClipboard();

    if( text.empty() )
        return 0;

    STRING_LINE_READER reader( text, "Clipboard" );
    SCH_SEXPR_PLUGIN  plugin;

    SCH_SHEET paste_sheet;
    SCH_SCREEN* paste_screen = new SCH_SCREEN( &m_frame->GetScreen()->Kiway() );

    // Screen object on heap is owned by the sheet.
    paste_sheet.SetScreen( paste_screen );

    try
    {
        plugin.LoadContent( reader, &paste_sheet );
    }
    catch( IO_ERROR& ioe )
    {
        // If it wasn't content, then paste as text
        paste_screen->Append( new SCH_TEXT( wxPoint( 0, 0 ), text ) );
    }

    bool forceKeepAnnotations = false;
    bool forceDropAnnotations = false;
    bool dropAnnotations = false;

    if( aEvent.IsAction( &ACTIONS::pasteSpecial ) )
    {
        DIALOG_PASTE_SPECIAL dlg( m_frame, &forceKeepAnnotations, &forceDropAnnotations );

        if( dlg.ShowModal() == wxID_CANCEL )
            return 0;
    }

    if( forceDropAnnotations )
        dropAnnotations = true;

    // SCH_SEXP_PLUGIN added the items to the paste screen, but not to the view or anything
    // else.  Pull them back out to start with.
    //
    EDA_ITEMS       loadedItems;
    bool            sheetsPasted = false;
    SCH_SHEET_LIST  hierarchy    = m_frame->Schematic().GetSheets();
    SCH_SHEET_PATH& currentSheet = m_frame->GetCurrentSheet();
    wxFileName      destFn       = currentSheet.Last()->GetFileName();

    if( destFn.IsRelative() )
        destFn.MakeAbsolute( m_frame->Prj().GetProjectPath() );

    for( SCH_ITEM* item : paste_screen->Items() )
    {
        loadedItems.push_back( item );

        if( item->Type() == SCH_COMPONENT_T )
        {
            if( !dropAnnotations && !forceKeepAnnotations )
            {
                for( SCH_ITEM* existingItem : m_frame->GetScreen()->Items() )
                {
                    if( item->m_Uuid == existingItem->m_Uuid )
                    {
                        dropAnnotations = true;
                        break;
                    }
                }
            }
        }
        else if( item->Type() == SCH_SHEET_T )
        {
            SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );
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
        }
    }

    // Remove the references from our temporary screen to prevent freeing on the DTOR
    paste_screen->Clear( false );

    for( unsigned i = 0; i < loadedItems.size(); ++i )
    {
        EDA_ITEM* item = loadedItems[i];

        if( item->Type() == SCH_COMPONENT_T )
        {
            SCH_COMPONENT* component = (SCH_COMPONENT*) item;

            // The library symbol gets set from the cached library symbols in the current
            // schematic not the symbol libraries.  The cached library symbol may have
            // changed from the original library symbol which would cause the copy to
            // be incorrect.
            SCH_SCREEN* currentScreen = m_frame->GetScreen();

            wxCHECK2( currentScreen, continue );

            auto it = currentScreen->GetLibSymbols().find( component->GetSchSymbolLibraryName() );

            if( it != currentScreen->GetLibSymbols().end() )
                component->SetLibSymbol( new LIB_PART( *it->second ) );

            if( dropAnnotations )
            {
                const_cast<KIID&>( component->m_Uuid ) = KIID();

                // clear the annotation, but preserve the selected unit
                int unit = component->GetUnit();
                component->ClearAnnotation( nullptr );
                component->SetUnit( unit );
            }
        }
        else
        {
            // Everything else gets a new UUID
            const_cast<KIID&>( item->m_Uuid ) = KIID();
        }

        if( item->Type() == SCH_SHEET_T )
        {
            SCH_SHEET*      sheet                = (SCH_SHEET*) item;
            SCH_FIELD&      nameField            = sheet->GetFields()[SHEETNAME];
            wxFileName      fn                   = sheet->GetFileName();
            SCH_SCREEN*     existingScreen       = nullptr;
            bool            dropSheetAnnotations = false;
            wxString        baseName             = nameField.GetText();
            wxString        candidateName        = baseName;
            int             uniquifier           = 1;

            while( hierarchy.NameExists( candidateName ) )
                candidateName = wxString::Format( wxT( "%s%d" ), baseName, uniquifier++ );

            nameField.SetText( candidateName );

            sheet->SetParent( currentSheet.Last() );
            sheet->SetScreen( nullptr );
            sheetsPasted = true;

            if( !fn.IsAbsolute() )
            {
                wxFileName currentSheetFileName = currentSheet.LastScreen()->GetFileName();
                fn.Normalize( wxPATH_NORM_ALL, currentSheetFileName.GetPath() );
            }

            if( m_frame->Schematic().Root().SearchHierarchy(
                        fn.GetFullPath( wxPATH_UNIX ), &existingScreen ) )
                dropSheetAnnotations = !forceKeepAnnotations;
            else
                searchSupplementaryClipboard( sheet->GetFileName(), &existingScreen );

            if( existingScreen )
            {
                sheet->SetScreen( existingScreen );

                SCH_SHEET_PATH sheetpath = currentSheet;
                sheetpath.push_back( sheet );

                // Clear annotation and create the AR for this path, if not exists,
                // when the screen is shared by sheet paths.
                // Otherwise ClearAnnotation do nothing, because the F1 field is used as
                // reference default value and takes the latest displayed value
                existingScreen->EnsureAlternateReferencesExist();

                if( forceDropAnnotations || dropSheetAnnotations )
                    existingScreen->ClearAnnotation( &sheetpath );
            }
            else
            {
                if( !m_frame->LoadSheetFromFile( sheet, &currentSheet, fn.GetFullPath() ) )
                    m_frame->InitSheet( sheet, sheet->GetFileName() );
            }
        }

        item->SetFlags( IS_NEW | IS_PASTED | IS_MOVED );
        m_frame->AddItemToScreenAndUndoList( (SCH_ITEM*) item, i > 0 );

        // Reset flags for subsequent move operation
        item->SetFlags( IS_NEW | IS_PASTED | IS_MOVED );
        // Start out hidden so the pasted items aren't "ghosted" in their original location
        // before being moved to the current location.
        getView()->Hide( item, true );
    }

    if( sheetsPasted )
    {
        m_frame->SetSheetNumberAndCount();
        m_frame->UpdateHierarchyNavigator();
    }

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


int SCH_EDITOR_CONTROL::UpdateFromPCB( const TOOL_EVENT& aEvent )
{
    DIALOG_UPDATE_FROM_PCB dlg( m_frame );
    dlg.ShowModal();
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

        m_frame->GetCurrentSheet().push_back( sheet );
        m_frame->DisplayCurrentSheet();
        m_frame->UpdateHierarchyNavigator();
    }

    return 0;
}


int SCH_EDITOR_CONTROL::LeaveSheet( const TOOL_EVENT& aEvent )
{
    if( m_frame->GetCurrentSheet().Last() != &m_frame->Schematic().Root() )
    {
        m_toolMgr->RunAction( ACTIONS::cancelInteractive, true );
        m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

        m_frame->GetCurrentSheet().pop_back();
        m_frame->DisplayCurrentSheet();
        m_frame->UpdateHierarchyNavigator();
    }

    return 0;
}


int SCH_EDITOR_CONTROL::ToggleHiddenPins( const TOOL_EVENT& aEvent )
{
    EESCHEMA_SETTINGS* cfg = m_frame->eeconfig();
    cfg->m_Appearance.show_hidden_pins = !cfg->m_Appearance.show_hidden_pins;

    KIGFX::SCH_PAINTER* painter = static_cast<KIGFX::SCH_PAINTER*>( getView()->GetPainter() );
    painter->GetSettings()->m_ShowHiddenPins = m_frame->GetShowAllPins();

    getView()->UpdateAllItems( KIGFX::REPAINT );
    m_frame->GetCanvas()->Refresh();

    return 0;
}


int SCH_EDITOR_CONTROL::ToggleHiddenFields( const TOOL_EVENT& aEvent )
{
    EESCHEMA_SETTINGS* cfg = m_frame->eeconfig();
    cfg->m_Appearance.show_hidden_fields = !cfg->m_Appearance.show_hidden_fields;

    KIGFX::SCH_PAINTER* painter = static_cast<KIGFX::SCH_PAINTER*>( getView()->GetPainter() );
    painter->GetSettings()->m_ShowHiddenText = cfg->m_Appearance.show_hidden_fields;

    getView()->UpdateAllItems( KIGFX::REPAINT );
    m_frame->GetCanvas()->Refresh();

    return 0;
}


int SCH_EDITOR_CONTROL::ToggleForceHV( const TOOL_EVENT& aEvent )
{
    m_frame->eeconfig()->m_Drawing.hv_lines_only = !m_frame->eeconfig()->m_Drawing.hv_lines_only;

    return 0;
}


void SCH_EDITOR_CONTROL::setTransitions()
{
    Go( &SCH_EDITOR_CONTROL::New,                   ACTIONS::doNew.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::Open,                  ACTIONS::open.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::Save,                  ACTIONS::save.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::SaveAs,                ACTIONS::saveAs.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ShowSchematicSetup,    EE_ACTIONS::schematicSetup.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::PageSetup,             ACTIONS::pageSettings.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::Print,                 ACTIONS::print.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::Plot,                  ACTIONS::plot.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::Quit,                  ACTIONS::quit.MakeEvent() );

    Go( &SCH_EDITOR_CONTROL::RescueSymbols,         EE_ACTIONS::rescueSymbols.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::RemapSymbols,          EE_ACTIONS::remapSymbols.MakeEvent() );

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
    Go( &SCH_EDITOR_CONTROL::Paste,                 ACTIONS::pasteSpecial.MakeEvent() );

    Go( &SCH_EDITOR_CONTROL::EditWithLibEdit,       EE_ACTIONS::editWithLibEdit.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ShowCvpcb,             EE_ACTIONS::assignFootprints.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ImportFPAssignments,   EE_ACTIONS::importFPAssignments.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::Annotate,              EE_ACTIONS::annotate.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::EditSymbolFields,      EE_ACTIONS::editSymbolFields.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::EditSymbolLibraryLinks,EE_ACTIONS::editSymbolLibraryLinks.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ShowPcbNew,            EE_ACTIONS::showPcbNew.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::UpdatePCB,             ACTIONS::updatePcbFromSchematic.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::UpdateFromPCB,         ACTIONS::updateSchematicFromPcb.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ExportNetlist,         EE_ACTIONS::exportNetlist.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::GenerateBOM,           EE_ACTIONS::generateBOM.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::DrawSheetOnClipboard,  EE_ACTIONS::drawSheetOnClipboard.MakeEvent() );

    Go( &SCH_EDITOR_CONTROL::ShowBusManager,        EE_ACTIONS::showBusManager.MakeEvent() );

    Go( &SCH_EDITOR_CONTROL::EnterSheet,            EE_ACTIONS::enterSheet.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::LeaveSheet,            EE_ACTIONS::leaveSheet.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::NavigateHierarchy,     EE_ACTIONS::navigateHierarchy.MakeEvent() );

    Go( &SCH_EDITOR_CONTROL::ToggleHiddenPins,      EE_ACTIONS::toggleHiddenPins.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ToggleHiddenFields,    EE_ACTIONS::toggleHiddenFields.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ToggleForceHV,         EE_ACTIONS::toggleForceHV.MakeEvent() );
}
