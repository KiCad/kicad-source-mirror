/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2023 CERN
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

#include "tools/sch_editor_control.h"

#include <clipboard.h>
#include <confirm.h>
#include <connection_graph.h>
#include <design_block.h>
#include <dialogs/dialog_symbol_fields_table.h>
#include <dialogs/dialog_eeschema_page_settings.h>
#include <dialogs/dialog_paste_special.h>
#include <dialogs/dialog_plot_schematic.h>
#include <dialogs/dialog_symbol_remap.h>
#include <dialogs/dialog_assign_netclass.h>
#include <dialogs/dialog_update_from_pcb.h>
#include <dialogs/hotkey_cycle_popup.h>
#include <dialogs/dialog_increment_annotations_base.h>
#include <project_rescue.h>
#include <erc/erc.h>
#include <invoke_sch_dialog.h>
#include <string_utils.h>
#include <kiway.h>
#include <netlist_exporters/netlist_exporter_spice.h>
#include <paths.h>
#include <pgm_base.h>
#include <project/project_file.h>
#include <project/net_settings.h>
#include <project_sch.h>
#include <sch_design_block_pane.h>
#include <sch_edit_frame.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <sch_bitmap.h>
#include <sch_group.h>
#include <sch_line.h>
#include <sch_junction.h>
#include <sch_bus_entry.h>
#include <sch_shape.h>
#include <sch_painter.h>
#include <sch_sheet_pin.h>
#include <sch_commit.h>
#include <sim/simulator_frame.h>
#include <symbol_lib_table.h>
#include <symbol_library.h>
#include <symbol_library_manager.h>
#include <symbol_viewer_frame.h>
#include <tool/picker_tool.h>
#include <tool/tool_manager.h>
#include <tools/sch_actions.h>
#include <tools/sch_selection.h>
#include <tools/sch_selection_tool.h>
#include <tools/sch_tool_utils.h>
#include <tools/sch_edit_table_tool.h>
#include <drawing_sheet/ds_proxy_undo_item.h>
#include <eda_list_dialog.h>
#include <view/view_controls.h>
#include <wildcards_and_files_ext.h>
#include <wx_filename.h>
#include <wx/filedlg.h>
#include <wx/log.h>
#include <wx/treectrl.h>
#include <wx/msgdlg.h>
#include <io/kicad/kicad_io_utils.h>
#include <printing/dialog_print.h>

#ifdef KICAD_IPC_API
#include <api/api_plugin_manager.h>
#endif


/**
 * Flag to enable schematic paste debugging output.
 *
 * @ingroup trace_env_vars
 */
static const wxChar traceSchPaste[] = wxT( "KICAD_SCH_PASTE" );


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
    m_frame->SaveProject( true );
    return 0;
}


int SCH_EDITOR_CONTROL::SaveCurrSheetCopyAs( const TOOL_EVENT& aEvent )
{
    SCH_SHEET* curr_sheet = m_frame->GetCurrentSheet().Last();
    wxFileName curr_fn = curr_sheet->GetFileName();
    wxFileDialog dlg( m_frame, _( "Schematic Files" ), curr_fn.GetPath(), curr_fn.GetFullName(),
                      FILEEXT::KiCadSchematicFileWildcard(),
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return false;

    wxString newFilename =
            EnsureFileExtension( dlg.GetPath(), FILEEXT::KiCadSchematicFileExtension );

    m_frame->saveSchematicFile( curr_sheet, newFilename );
    return 0;
}


int SCH_EDITOR_CONTROL::Revert( const TOOL_EVENT& aEvent )
{
    SCHEMATIC& schematic = m_frame->Schematic();
    SCH_SHEET& root = schematic.Root();

    if( m_frame->GetCurrentSheet().Last() != &root )
    {
        SCH_SHEET_PATH rootSheetPath;
        rootSheetPath.push_back( &root );

        m_frame->GetToolManager()->RunAction<SCH_SHEET_PATH*>( SCH_ACTIONS::changeSheet, &rootSheetPath );
        wxSafeYield();
    }

    wxString msg;
    msg.Printf( _( "Revert '%s' (and all sub-sheets) to last version saved?" ),
                schematic.GetFileName() );

    if( !IsOK( m_frame, msg ) )
        return false;

    SCH_SCREENS screenList( schematic.Root() );

    for( SCH_SCREEN* screen = screenList.GetFirst(); screen; screen = screenList.GetNext() )
        screen->SetContentModified( false );    // do not prompt the user for changes

    m_frame->ReleaseFile();
    m_frame->OpenProjectFiles( std::vector<wxString>( 1, schematic.GetFileName() ), KICTL_REVERT );

    return 0;
}


int SCH_EDITOR_CONTROL::ShowSchematicSetup( const TOOL_EVENT& aEvent )
{
    m_frame->ShowSchematicSetupDialog();
    return 0;
}


int SCH_EDITOR_CONTROL::PageSetup( const TOOL_EVENT& aEvent )
{
    PICKED_ITEMS_LIST   undoCmd;
    DS_PROXY_UNDO_ITEM* undoItem = new DS_PROXY_UNDO_ITEM( m_frame );
    ITEM_PICKER         wrapper( m_frame->GetScreen(), undoItem, UNDO_REDO::PAGESETTINGS );

    undoCmd.PushItem( wrapper );
    undoCmd.SetDescription( _( "Page Settings" ) );
    m_frame->SaveCopyInUndoList( undoCmd, UNDO_REDO::PAGESETTINGS, false, false );

    DIALOG_EESCHEMA_PAGE_SETTINGS dlg( m_frame, m_frame->Schematic().GetEmbeddedFiles(), VECTOR2I( MAX_PAGE_SIZE_EESCHEMA_MILS,
                                                          MAX_PAGE_SIZE_EESCHEMA_MILS ) );
    dlg.SetWksFileName( BASE_SCREEN::m_DrawingSheetFileName );

    if( dlg.ShowModal() == wxID_OK )
    {
        // Update text variables
        m_frame->GetCanvas()->GetView()->MarkDirty();
        m_frame->GetCanvas()->GetView()->UpdateAllItems( KIGFX::REPAINT );
        m_frame->GetCanvas()->Refresh();

        m_frame->OnModify();
    }
    else
    {
        m_frame->RollbackSchematicFromUndo();
    }

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
            static_cast<SYMBOL_VIEWER_FRAME*>( viewer )->ReCreateLibList();

        if( aRunningOnDemand )
        {
            SCH_SCREENS schematic( m_frame->Schematic().Root() );

            schematic.UpdateSymbolLinks();
            m_frame->RecalculateConnections( nullptr, GLOBAL_CLEANUP );
        }

        m_frame->ClearUndoRedoList();
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
    DIALOG_PRINT dlg( m_frame );

    dlg.ShowModal();

    return 0;
}


int SCH_EDITOR_CONTROL::Plot( const TOOL_EVENT& aEvent )
{
    DIALOG_PLOT_SCHEMATIC dlg( m_frame );

    dlg.ShowModal();

    // save project config if the prj config has changed:
    if( dlg.PrjConfigChanged() )
        m_frame->OnModify();

    return 0;
}


int SCH_EDITOR_CONTROL::Quit( const TOOL_EVENT& aEvent )
{
    m_frame->Close( false );
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
    if( m_probingPcbToSch || m_frame->IsSyncingSelection() )
        return;

    SCH_SELECTION_TOOL* selTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
    SCH_SELECTION&      selection = aForce ? selTool->RequestSelection() : selTool->GetSelection();

    m_frame->SendSelectItemsToPcb( selection.GetItemsSortedBySelectionOrder(), aForce );
}


int SCH_EDITOR_CONTROL::ExportSymbolsToLibrary( const TOOL_EVENT& aEvent )
{
    bool                   savePowerSymbols = false;
    bool                   map = false;
    SYMBOL_LIBRARY_MANAGER mgr( *m_frame );
    wxString               targetLib;

    targetLib = m_frame->SelectLibrary( _( "Export Symbols" ), _( "Export symbols to library:" ),
                                        { { _( "Include power symbols in export" ), &savePowerSymbols },
                                          { _( "Update schematic symbols to link to exported symbols" ), &map }
                                        } );

    if( targetLib.empty() )
        return 0;

    SCH_SHEET_LIST     sheets = m_frame->Schematic().BuildSheetListSortedByPageNumbers();
    SCH_REFERENCE_LIST symbols;
    sheets.GetSymbols( symbols, savePowerSymbols );

    std::map<LIB_ID, LIB_SYMBOL*> libSymbols;
    std::map<LIB_ID, std::vector<SCH_SYMBOL*>> symbolMap;

    for( size_t i = 0; i < symbols.GetCount(); ++i )
    {
        SCH_SYMBOL* symbol = symbols[i].GetSymbol();
        LIB_SYMBOL* libSymbol = symbol->GetLibSymbolRef().get();
        LIB_ID id = libSymbol->GetLibId();

        if( libSymbols.count( id ) )
        {
            wxASSERT_MSG( libSymbols[id]->Compare( *libSymbol, SCH_ITEM::COMPARE_FLAGS::ERC ) == 0,
                          "Two symbols have the same LIB_ID but are different!" );
        }
        else
        {
            libSymbols[id] = libSymbol;
        }

        symbolMap[id].emplace_back( symbol );
    }

    bool                   append = false;
    SCH_COMMIT             commit( m_frame );
    SYMBOL_LIB_TABLE_ROW*  row = mgr.GetLibrary( targetLib );
    SCH_IO_MGR::SCH_FILE_T type = SCH_IO_MGR::EnumFromStr( row->GetType() );
    IO_RELEASER<SCH_IO>    pi( SCH_IO_MGR::FindPlugin( type ) );

    wxFileName dest = row->GetFullURI( true );
    dest.Normalize( FN_NORMALIZE_FLAGS | wxPATH_NORM_ENV_VARS );

    for( const std::pair<const LIB_ID, LIB_SYMBOL*>& it : libSymbols )
    {
        LIB_SYMBOL* origSym = it.second;
        LIB_SYMBOL* newSym = origSym->Flatten().release();

        try
        {
            pi->SaveSymbol( dest.GetFullPath(), newSym );
        }
        catch( const IO_ERROR& ioe )
        {
            wxString msg;
            msg.Printf( _( "Error saving symbol %s to library '%s'." ),
                        newSym->GetName(), row->GetNickName() );
            msg += wxS( "\n\n" ) + ioe.What();
            wxLogWarning( msg );
            return 0;
        }

        if( map )
        {
            LIB_ID id = it.first;
            id.SetLibNickname( targetLib );

            for( SCH_SYMBOL* symbol : symbolMap[it.first] )
            {
                SCH_SCREEN* parentScreen = static_cast<SCH_SCREEN*>( symbol->GetParent() );

                wxCHECK2( parentScreen, continue );

                commit.Modify( symbol, parentScreen, RECURSE_MODE::NO_RECURSE );
                symbol->SetLibId( id );
                append = true;
            }
        }
    }

    // Save the modified symbol library table. We need to look this up by name in each table to find
    // whether the new library is a global or project entity as the code above to choose the library
    // returns a different type depending on whether a global or project library is chosen.
    SYMBOL_LIB_TABLE* globalTable = &SYMBOL_LIB_TABLE::GetGlobalLibTable();
    SYMBOL_LIB_TABLE* projectTable = nullptr;

    if( !m_frame->Prj().IsNullProject() )
        projectTable = PROJECT_SCH::SchSymbolLibTable( &m_frame->Prj() );

    if( globalTable->FindRow( targetLib ) )
    {
        try
        {
            wxString globalTablePath = SYMBOL_LIB_TABLE::GetGlobalTableFileName();
            globalTable->Save( globalTablePath );
        }
        catch( const IO_ERROR& ioe )
        {
            wxString msg;
            msg.Printf( _( "Error saving global library table:\n\n%s" ), ioe.What() );
            wxMessageBox( msg, _( "File Save Error" ), wxOK | wxICON_ERROR );
        }
    }
    else if( projectTable && projectTable->FindRow( targetLib ) )
    {
        try
        {
            wxString   projectPath = m_frame->Prj().GetProjectPath();
            wxFileName projectTableFn( projectPath, SYMBOL_LIB_TABLE::GetSymbolLibTableFileName() );
            projectTable->Save( projectTableFn.GetFullPath() );
        }
        catch( const IO_ERROR& ioe )
        {
            wxString msg;
            msg.Printf( _( "Error saving project-specific library table:\n\n%s" ), ioe.What() );
            wxMessageBox( msg, _( "File Save Error" ), wxOK | wxICON_ERROR );
        }
    }

    if( append )
    {
        std::set<SCH_SCREEN*> processedScreens;

        for( SCH_SHEET_PATH& sheet : sheets )
        {
            SCH_SCREEN* screen = sheet.LastScreen();

            if( processedScreens.find( ( screen ) ) == processedScreens.end() )
            {
                processedScreens.insert( screen );
                screen->UpdateSymbolLinks();
            }
        }

        commit.Push( wxS( "Update Library Identifiers" ) );
    }

    return 0;
}


#define HITTEST_THRESHOLD_PIXELS 5

int SCH_EDITOR_CONTROL::SimProbe( const TOOL_EVENT& aEvent )
{
    PICKER_TOOL*     picker = m_toolMgr->GetTool<PICKER_TOOL>();
    KIWAY_PLAYER*    player = m_frame->Kiway().Player( FRAME_SIMULATOR, false );
    SIMULATOR_FRAME* simFrame = static_cast<SIMULATOR_FRAME*>( player );

    if( !simFrame )     // Defensive coding; shouldn't happen.
        return 0;

    if( wxWindow* blocking_win = simFrame->Kiway().GetBlockingDialog() )
        blocking_win->Close( true );

    // Deactivate other tools; particularly important if another PICKER is currently running
    Activate();

    picker->SetCursor( KICURSOR::VOLTAGE_PROBE );
    picker->SetSnapping( false );
    picker->ClearHandlers();

    picker->SetClickHandler(
            [this, simFrame]( const VECTOR2D& aPosition )
            {
                SCH_SELECTION_TOOL* selTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();

                // We do not really want to keep an item selected in schematic,
                // so clear the current selection
                selTool->ClearSelection();

                EDA_ITEM*          item = selTool->GetNode( aPosition );
                SCH_SHEET_PATH&    sheet = m_frame->GetCurrentSheet();

                if( !item )
                    return false;

                if( item->Type() == SCH_PIN_T )
                {
                    try
                    {
                        SCH_PIN*    pin = static_cast<SCH_PIN*>( item )->GetLibPin();
                        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item->GetParent() );

                        WX_STRING_REPORTER reporter;
                        SIM_LIB_MGR        mgr( &m_frame->Prj() );

                        std::vector<EMBEDDED_FILES*> embeddedFilesStack;
                        embeddedFilesStack.push_back( m_frame->Schematic().GetEmbeddedFiles() );
                        embeddedFilesStack.push_back( symbol->GetEmbeddedFiles() );

                        mgr.SetFilesStack( std::move( embeddedFilesStack ) );

                        SIM_MODEL& model = mgr.CreateModel( &sheet, *symbol, true, 0, reporter ).model;

                        if( reporter.HasMessage() )
                            THROW_IO_ERROR( reporter.GetMessages() );

                        SPICE_ITEM spiceItem;
                        spiceItem.refName = symbol->GetRef( &sheet ).ToStdString();
                        std::vector<std::string> currentNames = model.SpiceGenerator().CurrentNames( spiceItem );

                        if( currentNames.size() == 0 )
                        {
                            return true;
                        }
                        else if( currentNames.size() == 1 )
                        {
                            simFrame->AddCurrentTrace( currentNames.at( 0 ) );
                            return true;
                        }

                        int modelPinIndex = model.FindModelPinIndex( pin->GetNumber().ToStdString() );

                        if( modelPinIndex != SIM_MODEL_PIN::NOT_CONNECTED )
                        {
                            wxString name = currentNames.at( modelPinIndex );
                            simFrame->AddCurrentTrace( name );
                        }
                    }
                    catch( const IO_ERROR& e )
                    {
                        DisplayErrorMessage( m_frame, e.What() );
                    }
                }
                else if( item->IsType( { SCH_ITEM_LOCATE_WIRE_T } )
                         || item->IsType( { SCH_JUNCTION_T } ) )
                {
                    if( SCH_CONNECTION* conn = static_cast<SCH_ITEM*>( item )->Connection() )
                    {
                        wxString spiceNet = UnescapeString( conn->Name() );
                        NETLIST_EXPORTER_SPICE::ConvertToSpiceMarkup( &spiceNet );

                        simFrame->AddVoltageTrace( wxString::Format( "V(%s)", spiceNet ) );
                    }
                }

                return true;
            } );

    picker->SetMotionHandler(
            [this, picker]( const VECTOR2D& aPos )
            {
                SCH_COLLECTOR collector;
                collector.m_Threshold = KiROUND( getView()->ToWorld( HITTEST_THRESHOLD_PIXELS ) );
                collector.Collect( m_frame->GetScreen(), { SCH_ITEM_LOCATE_WIRE_T,
                                                           SCH_PIN_T,
                                                           SCH_SHEET_PIN_T }, aPos );

                SCH_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
                selectionTool->GuessSelectionCandidates( collector, aPos );

                EDA_ITEM* item = collector.GetCount() == 1 ? collector[ 0 ] : nullptr;
                SCH_LINE* wire = dynamic_cast<SCH_LINE*>( item );

                const SCH_CONNECTION* conn = nullptr;

                if( wire )
                {
                    item = nullptr;
                    conn = wire->Connection();
                }

                if( item && item->Type() == SCH_PIN_T )
                    picker->SetCursor( KICURSOR::CURRENT_PROBE );
                else
                    picker->SetCursor( KICURSOR::VOLTAGE_PROBE );

                if( m_pickerItem != item )
                {
                    if( m_pickerItem )
                        selectionTool->UnbrightenItem( m_pickerItem );

                    m_pickerItem = item;

                    if( m_pickerItem )
                        selectionTool->BrightenItem( m_pickerItem );
                }

                wxString connectionName = ( conn ) ? conn->Name() : wxString( wxS( "" ) );

                if( m_frame->GetHighlightedConnection() != connectionName )
                {
                    m_frame->SetHighlightedConnection( connectionName );

                    TOOL_EVENT dummyEvent;
                    UpdateNetHighlighting( dummyEvent );
                }
            } );

    picker->SetFinalizeHandler(
            [this]( const int& aFinalState )
            {
                if( m_pickerItem )
                    m_toolMgr->GetTool<SCH_SELECTION_TOOL>()->UnbrightenItem( m_pickerItem );

                if( !m_frame->GetHighlightedConnection().IsEmpty() )
                {
                    m_frame->SetHighlightedConnection( wxEmptyString );

                    TOOL_EVENT dummyEvent;
                    UpdateNetHighlighting( dummyEvent );
                }

                // Wake the selection tool after exiting to ensure the cursor gets updated
                // and deselect previous selection from simulator to avoid any issue
                // ( avoid crash in some cases when the SimProbe tool is deselected )
                SCH_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
                selectionTool->ClearSelection();
                m_toolMgr->PostAction( ACTIONS::selectionActivate );
            } );

    m_toolMgr->RunAction( ACTIONS::pickerTool, &aEvent );

    return 0;
}


int SCH_EDITOR_CONTROL::SimTune( const TOOL_EVENT& aEvent )
{
    PICKER_TOOL* picker = m_toolMgr->GetTool<PICKER_TOOL>();

    // Deactivate other tools; particularly important if another PICKER is currently running
    Activate();

    picker->SetCursor( KICURSOR::TUNE );
    picker->SetSnapping( false );
    picker->ClearHandlers();

    picker->SetClickHandler(
            [this]( const VECTOR2D& aPosition )
            {
                SCH_SELECTION_TOOL* selTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
                EDA_ITEM*           item = nullptr;
                selTool->SelectPoint( aPosition, { SCH_SYMBOL_T, SCH_FIELD_T }, &item );

                if( !item )
                    return false;

                if( item->Type() != SCH_SYMBOL_T )
                {
                    item = item->GetParent();

                    if( item->Type() != SCH_SYMBOL_T )
                        return false;
                }

                SCH_SYMBOL*    symbol = static_cast<SCH_SYMBOL*>( item );
                SCH_SHEET_PATH sheetPath = symbol->Schematic()->CurrentSheet();
                KIWAY_PLAYER*  simFrame = m_frame->Kiway().Player( FRAME_SIMULATOR, false );

                if( simFrame )
                {
                    if( wxWindow* blocking_win = simFrame->Kiway().GetBlockingDialog() )
                        blocking_win->Close( true );

                    static_cast<SIMULATOR_FRAME*>( simFrame )->AddTuner( sheetPath, symbol );
                }

                // We do not really want to keep a symbol selected in schematic,
                // so clear the current selection
                selTool->ClearSelection();
                return true;
            } );

    picker->SetMotionHandler(
            [this]( const VECTOR2D& aPos )
            {
                SCH_COLLECTOR collector;
                collector.m_Threshold = KiROUND( getView()->ToWorld( HITTEST_THRESHOLD_PIXELS ) );
                collector.Collect( m_frame->GetScreen(), { SCH_SYMBOL_T, SCH_FIELD_T }, aPos );

                SCH_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
                selectionTool->GuessSelectionCandidates( collector, aPos );

                EDA_ITEM* item = collector.GetCount() == 1 ? collector[ 0 ] : nullptr;

                if( item && item->Type() == SCH_FIELD_T )
                    item = static_cast<SCH_FIELD*>( item )->GetParentSymbol();

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
                    m_toolMgr->GetTool<SCH_SELECTION_TOOL>()->UnbrightenItem( m_pickerItem );

                // Wake the selection tool after exiting to ensure the cursor gets updated
                // and deselect previous selection from simulator to avoid any issue
                // ( avoid crash in some cases when the SimTune tool is deselected )
                SCH_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
                selectionTool->ClearSelection();
                m_toolMgr->PostAction( ACTIONS::selectionActivate );
            } );

    m_toolMgr->RunAction( ACTIONS::pickerTool, &aEvent );

    return 0;
}


// A singleton reference for clearing the highlight
static VECTOR2D CLEAR;


static bool highlightNet( TOOL_MANAGER* aToolMgr, const VECTOR2D& aPosition )
{
    SCH_EDIT_FRAME*     editFrame     = static_cast<SCH_EDIT_FRAME*>( aToolMgr->GetToolHolder() );
    SCH_SELECTION_TOOL* selTool       = aToolMgr->GetTool<SCH_SELECTION_TOOL>();
    SCH_EDITOR_CONTROL* editorControl = aToolMgr->GetTool<SCH_EDITOR_CONTROL>();
    SCH_CONNECTION*     conn          = nullptr;
    SCH_ITEM*           item          = nullptr;
    bool                retVal        = true;

    if( aPosition != CLEAR )
    {
        ERC_TESTER erc( &editFrame->Schematic() );

        if( erc.TestDuplicateSheetNames( false ) > 0 )
        {
            wxMessageBox( _( "Error: duplicate sub-sheet names found in current sheet." ) );
            retVal = false;
        }
        else
        {
            item   = static_cast<SCH_ITEM*>( selTool->GetNode( aPosition ) );
            SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( item );

            if( item )
            {
                if( item->IsConnectivityDirty() )
                    editFrame->RecalculateConnections( nullptr, NO_CLEANUP );

                if( item->Type() == SCH_FIELD_T )
                    symbol = dynamic_cast<SCH_SYMBOL*>( item->GetParent() );

                if( symbol && symbol->GetLibSymbolRef() && symbol->GetLibSymbolRef()->IsPower() )
                {
                    std::vector<SCH_PIN*> pins = symbol->GetPins();

                    if( pins.size() == 1 )
                        conn = pins[0]->Connection();
                }
                else
                {
                    conn = item->Connection();
                }
            }
        }
    }

    wxString connName = ( conn ) ? conn->Name() : wxString( wxS( "" ) );

    if( !conn )
    {
        editFrame->SetStatusText( wxT( "" ) );
        editFrame->SendCrossProbeClearHighlight();
        editFrame->SetHighlightedConnection( wxEmptyString );
        editorControl->SetHighlightBusMembers( false );
    }
    else
    {
        NET_NAVIGATOR_ITEM_DATA itemData( editFrame->GetCurrentSheet(), item );

        if( connName != editFrame->GetHighlightedConnection() )
        {
            editorControl->SetHighlightBusMembers( false );
            editFrame->SetCrossProbeConnection( conn );
            editFrame->SetHighlightedConnection( connName, &itemData );
        }
        else
        {
            editorControl->SetHighlightBusMembers( !editorControl->GetHighlightBusMembers() );

            if( item != editFrame->GetSelectedNetNavigatorItem() )
                editFrame->SelectNetNavigatorItem( &itemData );
        }
    }

    editFrame->UpdateNetHighlightStatus();

    TOOL_EVENT dummy;
    editorControl->UpdateNetHighlighting( dummy );

    return retVal;
}


int SCH_EDITOR_CONTROL::HighlightNet( const TOOL_EVENT& aEvent )
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    VECTOR2D              cursorPos = controls->GetCursorPosition( !aEvent.DisableGridSnapping() );

    highlightNet( m_toolMgr, cursorPos );

    return 0;
}


int SCH_EDITOR_CONTROL::ClearHighlight( const TOOL_EVENT& aEvent )
{
    highlightNet( m_toolMgr, CLEAR );

    return 0;
}


int SCH_EDITOR_CONTROL::AssignNetclass( const TOOL_EVENT& aEvent )
{
    SCH_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
    SCHEMATIC&          schematic = m_frame->Schematic();
    SCH_SCREEN*         screen = m_frame->GetCurrentSheet().LastScreen();

    const SCH_CONNECTION* conn = nullptr;
    VECTOR2D connPos;

    for( EDA_ITEM* item : selectionTool->GetSelection() )
    {
        conn    = static_cast<SCH_ITEM*>( item )->Connection();
        connPos = item->GetPosition();

        if( conn )
            break;
    }

    if( !conn )
    {
        m_frame->ShowInfoBarError( _( "No net selected." ) );
        return 0;
    }

    // Remove selection in favor of highlighting so the whole net is highlighted
    selectionTool->ClearSelection();
    highlightNet( m_toolMgr, connPos );

    wxString netName = conn->Name();

    if( conn->IsBus() )
    {
        wxString prefix;

        if( NET_SETTINGS::ParseBusVector( netName, &prefix, nullptr ) )
        {
            netName = prefix + wxT( "*" );
        }
        else if( NET_SETTINGS::ParseBusGroup( netName, &prefix, nullptr ) )
        {
            netName = prefix + wxT( ".*" );
        }
    }
    else if( !conn->Driver() || CONNECTION_SUBGRAPH::GetDriverPriority( conn->Driver() )
                                                < CONNECTION_SUBGRAPH::PRIORITY::SHEET_PIN )
    {
        m_frame->ShowInfoBarError( _( "Net must be labeled to assign a netclass." ) );
        highlightNet( m_toolMgr, CLEAR );
        return 0;
    }

    DIALOG_ASSIGN_NETCLASS dlg( m_frame, netName, schematic.GetNetClassAssignmentCandidates(),
            [&]( const std::vector<wxString>& aNetNames )
            {
                for( SCH_ITEM* item : screen->Items() )
                {
                    bool            redraw   = item->IsBrightened();
                    SCH_CONNECTION* itemConn = item->Connection();

                    if( itemConn && alg::contains( aNetNames, itemConn->Name() ) )
                        item->SetBrightened();
                    else
                        item->ClearBrightened();

                    redraw |= item->IsBrightened();

                    if( item->Type() == SCH_SYMBOL_T )
                    {
                        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

                        redraw |= symbol->HasBrightenedPins();

                        symbol->ClearBrightenedPins();

                        for( SCH_PIN* pin : symbol->GetPins() )
                        {
                            SCH_CONNECTION* pin_conn = pin->Connection();

                            if( pin_conn && alg::contains( aNetNames, pin_conn->Name() ) )
                            {
                                pin->SetBrightened();
                                redraw = true;
                            }
                        }
                    }
                    else if( item->Type() == SCH_SHEET_T )
                    {
                        for( SCH_SHEET_PIN* pin : static_cast<SCH_SHEET*>( item )->GetPins() )
                        {
                            SCH_CONNECTION* pin_conn = pin->Connection();

                            redraw |= pin->IsBrightened();

                            if( pin_conn && alg::contains( aNetNames, pin_conn->Name() ) )
                                pin->SetBrightened();
                            else
                                pin->ClearBrightened();

                            redraw |= pin->IsBrightened();
                        }
                    }

                    if( redraw )
                        getView()->Update( item, KIGFX::VIEW_UPDATE_FLAGS::REPAINT );
                }

                m_frame->GetCanvas()->ForceRefresh();
            } );

    if( dlg.ShowModal() )
    {
        getView()->UpdateAllItemsConditionally(
                [&]( KIGFX::VIEW_ITEM* aItem ) -> int
                {
                    int flags = 0;

                    auto invalidateTextVars =
                            [&flags]( EDA_TEXT* text )
                            {
                                if( text->HasTextVars() )
                                {
                                    text->ClearRenderCache();
                                    text->ClearBoundingBoxCache();
                                    flags |= KIGFX::GEOMETRY | KIGFX::REPAINT;
                                }
                            };

                    // Netclass coloured items
                    //
                    if( dynamic_cast<SCH_LINE*>( aItem ) )
                        flags |= KIGFX::REPAINT;
                    else if( dynamic_cast<SCH_JUNCTION*>( aItem ) )
                        flags |= KIGFX::REPAINT;
                    else if( dynamic_cast<SCH_BUS_ENTRY_BASE*>( aItem ) )
                        flags |= KIGFX::REPAINT;

                    // Items that might reference an item's netclass name
                    //
                    if( SCH_ITEM* item = dynamic_cast<SCH_ITEM*>( aItem ) )
                    {
                        item->RunOnChildren(
                                [&invalidateTextVars]( SCH_ITEM* aChild )
                                {
                                    if( EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( aChild ) )
                                        invalidateTextVars( text );
                                },
                                RECURSE_MODE::NO_RECURSE );

                        if( flags & KIGFX::GEOMETRY )
                            m_frame->GetScreen()->Update( item, false );   // Refresh RTree
                    }

                    if( EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( aItem ) )
                        invalidateTextVars( text );

                    return flags;
                } );
    }

    highlightNet( m_toolMgr, CLEAR );
    return 0;
}


int SCH_EDITOR_CONTROL::UpdateNetHighlighting( const TOOL_EVENT& aEvent )
{
    wxCHECK( m_frame, 0 );

    const SCH_SHEET_PATH&  sheetPath = m_frame->GetCurrentSheet();
    SCH_SCREEN*            screen = m_frame->GetCurrentSheet().LastScreen();
    CONNECTION_GRAPH*      connectionGraph = m_frame->Schematic().ConnectionGraph();
    wxString               selectedName = m_frame->GetHighlightedConnection();

    std::set<wxString>     connNames;
    std::vector<EDA_ITEM*> itemsToRedraw;

    wxCHECK( screen && connectionGraph, 0 );

    if( !selectedName.IsEmpty() )
    {
        connNames.emplace( selectedName );

        if( CONNECTION_SUBGRAPH* sg = connectionGraph->FindSubgraphByName( selectedName, sheetPath ) )
        {
            if( m_highlightBusMembers )
            {
                for( const SCH_ITEM* item : sg->GetItems() )
                {
                    wxCHECK2( item, continue );

                    if( SCH_CONNECTION* connection = item->Connection() )
                    {
                        for( const std::shared_ptr<SCH_CONNECTION>& member : connection->AllMembers() )
                        {
                            if( member )
                                connNames.emplace( member->Name() );
                        }
                    }
                }
            }
        }

        // Place all bus names that are connected to the selected net in the set, regardless of
        // their sheet. This ensures that nets that are connected to a bus on a different sheet
        // get their buses highlighted as well.
        for( CONNECTION_SUBGRAPH* sg : connectionGraph->GetAllSubgraphs( selectedName ) )
        {
            for( const auto& [_, bus_sgs] : sg->GetBusParents() )
            {
                for( CONNECTION_SUBGRAPH* bus_sg : bus_sgs )
                    connNames.emplace( bus_sg->GetNetName() );
            }

        }
    }

    for( SCH_ITEM* item : screen->Items() )
    {
        if( !item || !item->IsConnectable() )
            continue;

        SCH_ITEM* redrawItem = nullptr;

        if( item->Type() == SCH_SYMBOL_T )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            for( SCH_PIN* pin : symbol->GetPins() )
            {
                if( SCH_CONNECTION* pin_conn = pin->Connection() )
                {
                    if( !pin->IsBrightened() && connNames.count( pin_conn->Name() ) )
                    {
                        pin->SetBrightened();
                        redrawItem = symbol;
                    }
                    else if( pin->IsBrightened() && !connNames.count( pin_conn->Name() ) )
                    {
                        pin->ClearBrightened();
                        redrawItem = symbol;
                    }
                }
            }

            if( symbol->IsPower() && symbol->GetPins().size() )
            {
                if( SCH_CONNECTION* pinConn = symbol->GetPins()[0]->Connection() )
                {
                    for( FIELD_T id : { FIELD_T::REFERENCE, FIELD_T::VALUE } )
                    {
                        SCH_FIELD* field = symbol->GetField( id );

                        if( !field->IsVisible() )
                            continue;

                        if( !field->IsBrightened() && connNames.count( pinConn->Name() ) )
                        {
                            field->SetBrightened();
                            redrawItem = symbol;
                        }
                        else if( field->IsBrightened() && !connNames.count( pinConn->Name() ) )
                        {
                            field->ClearBrightened();
                            redrawItem = symbol;
                        }
                    }
                }
            }
        }
        else if( item->Type() == SCH_SHEET_T )
        {
            SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );

            for( SCH_SHEET_PIN* pin : sheet->GetPins() )
            {
                wxCHECK2( pin, continue );

                if( SCH_CONNECTION* pin_conn = pin->Connection() )
                {
                    if( !pin->IsBrightened() && connNames.count( pin_conn->Name() ) )
                    {
                        pin->SetBrightened();
                        redrawItem = sheet;
                    }
                    else if( pin->IsBrightened() && !connNames.count( pin_conn->Name() ) )
                    {
                        pin->ClearBrightened();
                        redrawItem = sheet;
                    }
                }
            }
        }
        else
        {
            if( SCH_CONNECTION* itemConn = item->Connection() )
            {
                if( !item->IsBrightened() && connNames.count( itemConn->Name() ) )
                {
                    item->SetBrightened();
                    redrawItem = item;
                }
                else if( item->IsBrightened() && !connNames.count( itemConn->Name() ) )
                {
                    item->ClearBrightened();
                    redrawItem = item;
                }
            }
        }

        if( redrawItem )
            itemsToRedraw.push_back( redrawItem );
    }

    if( itemsToRedraw.size() )
    {
        // Be sure highlight change will be redrawn
        KIGFX::VIEW* view = getView();

        for( EDA_ITEM* redrawItem : itemsToRedraw )
            view->Update( (KIGFX::VIEW_ITEM*)redrawItem, KIGFX::VIEW_UPDATE_FLAGS::REPAINT );

        m_frame->GetCanvas()->Refresh();
    }

    return 0;
}


int SCH_EDITOR_CONTROL::HighlightNetCursor( const TOOL_EVENT& aEvent )
{
    PICKER_TOOL* picker = m_toolMgr->GetTool<PICKER_TOOL>();

    // Deactivate other tools; particularly important if another PICKER is currently running
    Activate();

    picker->SetCursor( KICURSOR::BULLSEYE );
    picker->SetSnapping( false );
    picker->ClearHandlers();

    picker->SetClickHandler(
        [this] ( const VECTOR2D& aPos )
        {
            return highlightNet( m_toolMgr, aPos );
        } );

    m_toolMgr->RunAction( ACTIONS::pickerTool, &aEvent );

    return 0;
}


int SCH_EDITOR_CONTROL::Undo( const TOOL_EVENT& aEvent )
{
    wxCHECK( m_frame, 0 );

    if( m_frame->GetUndoCommandCount() <= 0 )
        return 0;

    // Inform tools that undo command was issued
    m_toolMgr->ProcessEvent( { TC_MESSAGE, TA_UNDO_REDO_PRE, AS_GLOBAL } );

    // Get the old list
    PICKED_ITEMS_LIST* undo_list = m_frame->PopCommandFromUndoList();

    wxCHECK( undo_list, 0 );

    m_frame->PutDataInPreviousState( undo_list );

    // Now push the old command to the RedoList
    undo_list->ReversePickersListOrder();
    m_frame->PushCommandToRedoList( undo_list );

    m_toolMgr->GetTool<SCH_SELECTION_TOOL>()->RebuildSelection();

    m_frame->GetCanvas()->Refresh();
    m_frame->OnModify();

    return 0;
}


int SCH_EDITOR_CONTROL::Redo( const TOOL_EVENT& aEvent )
{
    wxCHECK( m_frame, 0 );

    if( m_frame->GetRedoCommandCount() == 0 )
        return 0;

    // Inform tools that undo command was issued
    m_toolMgr->ProcessEvent( { TC_MESSAGE, TA_UNDO_REDO_PRE, AS_GLOBAL } );

    /* Get the old list */
    PICKED_ITEMS_LIST* list = m_frame->PopCommandFromRedoList();

    wxCHECK( list, 0 );

    /* Redo the command: */
    m_frame->PutDataInPreviousState( list );

    /* Put the old list in UndoList */
    list->ReversePickersListOrder();
    m_frame->PushCommandToUndoList( list );

    m_toolMgr->GetTool<SCH_SELECTION_TOOL>()->RebuildSelection();

    m_frame->GetCanvas()->Refresh();
    m_frame->OnModify();

    return 0;
}


bool SCH_EDITOR_CONTROL::doCopy( bool aUseDuplicateClipboard )
{
    SCH_SELECTION_TOOL* selTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
    SCH_SELECTION&      selection = selTool->RequestSelection();
    SCHEMATIC&          schematic = m_frame->Schematic();

    if( selection.Empty() )
        return false;

    if( aUseDuplicateClipboard )
        m_duplicateIsHoverSelection = selection.IsHover();

    selection.SetScreen( m_frame->GetScreen() );
    m_supplementaryClipboard.clear();

    for( EDA_ITEM* item : selection.GetItems() )
    {
        if( item->Type() == SCH_SHEET_T )
        {
            SCH_SHEET* sheet = (SCH_SHEET*) item;
            m_supplementaryClipboard[ sheet->GetFileName() ] = sheet->GetScreen();
        }
        else if( item->Type() == SCH_FIELD_T && selection.IsHover() )
        {
            // Most of the time the user is trying to duplicate the parent symbol
            // and the field text is in it
            selection.Add( item->GetParent() );
        }
        else if( item->Type() == SCH_MARKER_T )
        {
            // Don't let the markers be copied
            selection.Remove( item );
        }
        else if( item->Type() == SCH_GROUP_T )
        {
            // Groups need to have all their items selected
            static_cast<SCH_ITEM*>( item )->RunOnChildren(
                    [&]( EDA_ITEM* aChild )
                    {
                        selection.Add( aChild );
                    },
                    RECURSE_MODE::RECURSE );
        }
    }

    STRING_FORMATTER   formatter;
    SCH_IO_KICAD_SEXPR plugin;
    SCH_SHEET_PATH     selPath = m_frame->GetCurrentSheet();

    plugin.Format( &selection, &selPath, schematic, &formatter, true );

    std::string prettyData = formatter.GetString();
    KICAD_FORMAT::Prettify( prettyData, true );

    if( selection.IsHover() )
        m_toolMgr->RunAction( ACTIONS::selectionClear );

    if( aUseDuplicateClipboard )
    {
        m_duplicateClipboard = prettyData;
        return true;
    }

    return SaveClipboard( prettyData );
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


int SCH_EDITOR_CONTROL::Duplicate( const TOOL_EVENT& aEvent )
{
    doCopy( true ); // Use the local clipboard
    Paste( aEvent );

    return 0;
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
        m_toolMgr->RunAction( ACTIONS::doDelete );

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


int SCH_EDITOR_CONTROL::CopyAsText( const TOOL_EVENT& aEvent )
{
    SCH_SELECTION_TOOL* selTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
    SCH_SELECTION&      selection = selTool->RequestSelection();

    if( selection.Empty() )
        return false;

    wxString itemsAsText = GetSelectedItemsAsText( selection );

    if( selection.IsHover() )
        m_toolMgr->RunAction( ACTIONS::selectionClear );

    return SaveClipboard( itemsAsText.ToStdString() );
}


void SCH_EDITOR_CONTROL::updatePastedSymbol( SCH_SYMBOL* aSymbol,
                                             const SCH_SHEET_PATH& aPastePath,
                                             const KIID_PATH& aClipPath,
                                             bool aForceKeepAnnotations )
{
    wxCHECK( m_frame && aSymbol, /* void */ );

    SCH_SYMBOL_INSTANCE newInstance;
    bool instanceFound = false;
    KIID_PATH pasteLookupPath = aClipPath;

    m_pastedSymbols.insert( aSymbol );

    for( const SCH_SYMBOL_INSTANCE& tmp : aSymbol->GetInstances() )
    {
        if( ( tmp.m_Path.empty() && aClipPath.empty() )
          || ( !aClipPath.empty() && tmp.m_Path.EndsWith( aClipPath ) ) )
        {
            newInstance = tmp;
            instanceFound = true;

            wxLogTrace( traceSchPaste,
                        wxS( "Pasting found symbol instance with reference %s, unit %d:"
                             "\n\tClipboard path: %s\n\tSymbol UUID: %s." ),
                        tmp.m_Reference, tmp.m_Unit,
                        aClipPath.AsString(), aSymbol->m_Uuid.AsString() );

            break;
        }
    }

    // The pasted symbol look up paths include the symbol UUID.
    pasteLookupPath.push_back( aSymbol->m_Uuid );

    if( !instanceFound )
    {
        wxLogTrace( traceSchPaste,
                    wxS( "Clipboard symbol instance **not** found:\n\tClipboard path: %s\n\t"
                         "Symbol UUID: %s." ),
                    aClipPath.AsString(), aSymbol->m_Uuid.AsString() );

        // Some legacy versions saved value fields escaped.  While we still do in the symbol
        // editor, we don't anymore in the schematic, so be sure to unescape them.
        SCH_FIELD* valueField = aSymbol->GetField( FIELD_T::VALUE );
        valueField->SetText( UnescapeString( valueField->GetText() ) );

        // Pasted from notepad or an older instance of eeschema.  Use the values in the fields
        // instead.
        newInstance.m_Reference = aSymbol->GetField( FIELD_T::REFERENCE )->GetText();
        newInstance.m_Unit = aSymbol->GetUnit();
    }

    newInstance.m_Path = aPastePath.Path();
    newInstance.m_ProjectName = m_frame->Prj().GetProjectName();

    aSymbol->AddHierarchicalReference( newInstance );

    if( !aForceKeepAnnotations )
        aSymbol->ClearAnnotation( &aPastePath, false );

    // We might clear annotations but always leave the original unit number from the paste.
    aSymbol->SetUnit( newInstance.m_Unit );
}


SCH_SHEET_PATH SCH_EDITOR_CONTROL::updatePastedSheet( SCH_SHEET* aSheet,
                                                      const SCH_SHEET_PATH& aPastePath,
                                                      const KIID_PATH& aClipPath,
                                                      bool aForceKeepAnnotations,
                                                      SCH_SHEET_LIST* aPastedSheets,
                                                      std::map<SCH_SHEET_PATH,
                                                      SCH_REFERENCE_LIST>& aPastedSymbols )
{
    wxCHECK( aSheet && aPastedSheets, aPastePath );

    SCH_SHEET_PATH sheetPath = aPastePath;
    sheetPath.push_back( aSheet );

    aPastedSheets->push_back( sheetPath );

    if( aSheet->GetScreen() == nullptr )
        return sheetPath; // We can only really set the page number but not load any items

    for( SCH_ITEM* item : aSheet->GetScreen()->Items() )
    {
        if( item->IsConnectable() )
            item->SetConnectivityDirty();

        if( item->Type() == SCH_SYMBOL_T )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            wxCHECK2( symbol, continue );

            // Only do this once if the symbol is shared across multiple sheets.
            if( !m_pastedSymbols.count( symbol ) )
            {
                for( SCH_PIN* pin : symbol->GetPins() )
                {
                    const_cast<KIID&>( pin->m_Uuid ) = KIID();
                    pin->SetConnectivityDirty();
                }
            }

            updatePastedSymbol( symbol, sheetPath, aClipPath, aForceKeepAnnotations );
        }
        else if( item->Type() == SCH_SHEET_T )
        {
            SCH_SHEET* subsheet = static_cast<SCH_SHEET*>( item );

            wxCHECK2( subsheet, continue );

            // Make sure pins get a new UUID and set the dirty connectivity flag.
            if( !aPastedSheets->ContainsSheet( subsheet ) )
            {
                for( SCH_SHEET_PIN* pin : subsheet->GetPins() )
                {
                    const_cast<KIID&>( pin->m_Uuid ) = KIID();
                    pin->SetConnectivityDirty();
                }
            }

            KIID_PATH newClipPath = aClipPath;
            newClipPath.push_back( subsheet->m_Uuid );

            updatePastedSheet( subsheet, sheetPath, newClipPath, aForceKeepAnnotations,
                               aPastedSheets, aPastedSymbols );
        }
    }

    sheetPath.GetSymbols( aPastedSymbols[aPastePath] );

    return sheetPath;
}


void SCH_EDITOR_CONTROL::setPastedSymbolInstances( const SCH_SCREEN* aScreen )
{
    wxCHECK( aScreen, /* void */ );

    for( const SCH_ITEM* item : aScreen->Items() )
    {
        if( item->Type() == SCH_SYMBOL_T )
        {
            const SCH_SYMBOL* symbol = static_cast<const SCH_SYMBOL*>( item );

            wxCHECK2( symbol, continue );

            for( const SCH_SYMBOL_INSTANCE& symbolInstance : symbol->GetInstances() )
            {
                KIID_PATH pathWithSymbol = symbolInstance.m_Path;

                pathWithSymbol.push_back( symbol->m_Uuid );

                m_clipboardSymbolInstances[pathWithSymbol] = symbolInstance;
            }
        }
    }
}


void SCH_EDITOR_CONTROL::prunePastedSymbolInstances()
{
    wxCHECK( m_frame, /* void */ );

    for( SCH_SYMBOL* symbol : m_pastedSymbols )
    {
        wxCHECK2( symbol, continue );

        std::vector<KIID_PATH> instancePathsToRemove;

        for( const SCH_SYMBOL_INSTANCE& instance : symbol->GetInstances() )
        {
            if( ( instance.m_ProjectName != m_frame->Prj().GetProjectName() )
              || instance.m_Path.empty() )
                instancePathsToRemove.emplace_back( instance.m_Path );
        }

        for( const KIID_PATH& path : instancePathsToRemove )
            symbol->RemoveInstance( path );
    }
}


int SCH_EDITOR_CONTROL::Paste( const TOOL_EVENT& aEvent )
{
    wxTextEntry* textEntry = dynamic_cast<wxTextEntry*>( wxWindow::FindFocus() );

    if( textEntry )
    {
        textEntry->Paste();
        return 0;
    }

    SCH_SELECTION_TOOL* selTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
    std::string         content;
    VECTOR2I            eventPos;

    SCH_SHEET   tempSheet;
    SCH_SCREEN* tempScreen = new SCH_SCREEN( &m_frame->Schematic() );
    std::unique_ptr<wxImage> clipImg = GetImageFromClipboard();

    if( !aEvent.IsAction( &ACTIONS::duplicate ) && clipImg )
    {
        // Just image data
        auto bitmap = std::make_unique<SCH_BITMAP>();

        bool ok = bitmap->GetReferenceImage().SetImage( *clipImg );

        if( !ok )
        {
            delete tempScreen;
            return 0;
        }

        tempScreen->Append( bitmap.release() );
    }
    else
    {
        if( aEvent.IsAction( &ACTIONS::duplicate ) )
            content = m_duplicateClipboard;
        else
            content = GetClipboardUTF8();

        if( content.empty() )
        {
            delete tempScreen;
            return 0;
        }

        if( aEvent.IsAction( &ACTIONS::duplicate ) )
            eventPos = getViewControls()->GetCursorPosition( false );

        STRING_LINE_READER reader( content, "Clipboard" );
        SCH_IO_KICAD_SEXPR plugin;

        // Screen object on heap is owned by the sheet.
        tempSheet.SetScreen( tempScreen );

        try
        {
            plugin.LoadContent( reader, &tempSheet );
        }
        catch( IO_ERROR& )
        {
            // If it wasn't content, then paste as a text object.
            if( content.size() > static_cast<size_t>( ADVANCED_CFG::GetCfg().m_MaxPastedTextLength ) )
            {
                int result = IsOK( m_frame, _( "Pasting a long text text string may be very slow.  "
                                       "Do you want to continue?" ) );
                if( !result )
                    return 0;
            }

            SCH_TEXT* text_item = new SCH_TEXT( VECTOR2I( 0, 0 ), content );
            tempScreen->Append( text_item );
        }
    }

    m_pastedSymbols.clear();
    m_clipboardSymbolInstances.clear();

    // Save pasted symbol instances in case the user chooses to keep existing symbol annotation.
    setPastedSymbolInstances( tempScreen );

    tempScreen->MigrateSimModels();

    EESCHEMA_SETTINGS::PANEL_ANNOTATE& annotate = m_frame->eeconfig()->m_AnnotatePanel;
    SCHEMATIC_SETTINGS& schematicSettings = m_frame->Schematic().Settings();
    int annotateStartNum = schematicSettings.m_AnnotateStartNum;

    PASTE_MODE pasteMode = annotate.automatic ? PASTE_MODE::RESPECT_OPTIONS : PASTE_MODE::REMOVE_ANNOTATIONS;
    bool       forceRemoveAnnotations = false;

    if( aEvent.IsAction( &ACTIONS::pasteSpecial ) )
    {
        PASTE_MODE           pasteModeSpecial = pasteMode;
        DIALOG_PASTE_SPECIAL dlg( m_frame, &pasteModeSpecial );

        if( dlg.ShowModal() == wxID_CANCEL )
            return 0;

        // We have to distinguish if removing was explicit
        forceRemoveAnnotations = ( pasteModeSpecial == PASTE_MODE::REMOVE_ANNOTATIONS );
        pasteMode = pasteModeSpecial;
    }

    bool forceKeepAnnotations = pasteMode != PASTE_MODE::REMOVE_ANNOTATIONS;

    // SCH_SEXP_PLUGIN added the items to the paste screen, but not to the view or anything
    // else.  Pull them back out to start with.
    SCH_COMMIT             commit( m_toolMgr );
    EDA_ITEMS              loadedItems;
    std::vector<SCH_ITEM*> sortedLoadedItems;
    bool                   sheetsPasted = false;
    SCH_SHEET_LIST         hierarchy = m_frame->Schematic().Hierarchy();
    SCH_SHEET_PATH&        pasteRoot = m_frame->GetCurrentSheet();
    wxFileName             destFn = pasteRoot.Last()->GetFileName();

    if( destFn.IsRelative() )
        destFn.MakeAbsolute( m_frame->Prj().GetProjectPath() );

    // List of paths in the hierarchy that refer to the destination sheet of the paste
    SCH_SHEET_LIST sheetPathsForScreen = hierarchy.FindAllSheetsForScreen( pasteRoot.LastScreen() );
    sheetPathsForScreen.SortByPageNumbers();

    // Build a list of screens from the current design (to avoid loading sheets that already exist)
    std::map<wxString, SCH_SCREEN*> loadedScreens;

    for( const SCH_SHEET_PATH& item : hierarchy )
    {
        if( item.LastScreen() )
            loadedScreens[item.Last()->GetFileName()] = item.LastScreen();
    }

    // Get set of sheet names in the current schematic to prevent duplicate sheet names on paste.
    std::set<wxString> existingSheetNames = pasteRoot.LastScreen()->GetSheetNames();

    // Build symbol list for reannotation of duplicates
    SCH_REFERENCE_LIST existingRefs;
    hierarchy.GetSymbols( existingRefs );
    existingRefs.SortByReferenceOnly();

    std::set<wxString> existingRefsSet;

    for( const SCH_REFERENCE& ref : existingRefs )
        existingRefsSet.insert( ref.GetRef() );

    // Build UUID map for fetching last-resolved-properties
    std::map<KIID, EDA_ITEM*> itemMap;
    hierarchy.FillItemMap( itemMap );

    // Keep track of pasted sheets and symbols for the different paths to the hierarchy.
    std::map<SCH_SHEET_PATH, SCH_REFERENCE_LIST> pastedSymbols;
    std::map<SCH_SHEET_PATH, SCH_SHEET_LIST>     pastedSheets;

    for( SCH_ITEM* item : tempScreen->Items() )
    {
        if( item->Type() == SCH_SHEET_T )
            sortedLoadedItems.push_back( item );
        else
            loadedItems.push_back( item );
    }

    sort( sortedLoadedItems.begin(), sortedLoadedItems.end(),
          []( SCH_ITEM* firstItem, SCH_ITEM* secondItem )
          {
              SCH_SHEET* firstSheet = static_cast<SCH_SHEET*>( firstItem );
              SCH_SHEET* secondSheet = static_cast<SCH_SHEET*>( secondItem );
              return StrNumCmp( firstSheet->GetName(), secondSheet->GetName(), false ) < 0;
          });


    for( SCH_ITEM* item : sortedLoadedItems )
    {
        loadedItems.push_back( item );

        if( item->Type() == SCH_SHEET_T )
        {
            SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );
            wxFileName srcFn = sheet->GetFileName();

            if( srcFn.IsRelative() )
                srcFn.MakeAbsolute( m_frame->Prj().GetProjectPath() );

            SCH_SHEET_LIST sheetHierarchy( sheet );

            if( hierarchy.TestForRecursion( sheetHierarchy, destFn.GetFullPath( wxPATH_UNIX ) ) )
            {
                auto msg = wxString::Format( _( "The pasted sheet '%s'\n"
                                                "was dropped because the destination already has "
                                                "the sheet or one of its subsheets as a parent." ),
                                             sheet->GetFileName() );
                DisplayError( m_frame, msg );
                loadedItems.pop_back();
            }
        }
    }

    // Remove the references from our temporary screen to prevent freeing on the DTOR
    tempScreen->Clear( false );

    for( EDA_ITEM* item : loadedItems )
    {
        KIID_PATH clipPath( wxT( "/" ) ); // clipboard is at root

        SCH_ITEM* schItem = static_cast<SCH_ITEM*>( item );

        wxCHECK2( schItem, continue );

        if( schItem->IsConnectable() )
            schItem->SetConnectivityDirty();

        if( item->Type() == SCH_SYMBOL_T )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            // The library symbol gets set from the cached library symbols in the current
            // schematic not the symbol libraries.  The cached library symbol may have
            // changed from the original library symbol which would cause the copy to
            // be incorrect.
            SCH_SCREEN* currentScreen = m_frame->GetScreen();

            wxCHECK2( currentScreen, continue );

            auto it = currentScreen->GetLibSymbols().find( symbol->GetSchSymbolLibraryName() );
            auto end = currentScreen->GetLibSymbols().end();

            if( it == end )
            {
                // If can't find library definition in the design, use the pasted library
                it = tempScreen->GetLibSymbols().find( symbol->GetSchSymbolLibraryName() );
                end = tempScreen->GetLibSymbols().end();
            }

            LIB_SYMBOL* libSymbol = nullptr;

            if( it != end )
            {
                libSymbol = new LIB_SYMBOL( *it->second );
                symbol->SetLibSymbol( libSymbol );
            }

            // If the symbol is already in the schematic we have to always keep the
            // annotations. The exception is if the user has chosen to remove them.
            for( const SCH_SYMBOL_INSTANCE& instance : symbol->GetInstances() )
            {
                if( !existingRefsSet.contains( instance.m_Reference ) )
                {
                    forceKeepAnnotations = !forceRemoveAnnotations;
                    break;
                }
            }

            for( SCH_SHEET_PATH& sheetPath : sheetPathsForScreen )
                updatePastedSymbol( symbol, sheetPath, clipPath, forceKeepAnnotations );

            // Most modes will need new KIIDs for the symbol and its pins
            // However, if we are pasting unique annotations, we need to check if the symbol
            // is not already in the hierarchy.  If we don't already have a copy of the
            // symbol, we just keep the existing KIID data as it is likely the same symbol
            // being moved around the schematic
            bool needsNewKiid = ( pasteMode == PASTE_MODE::UNIQUE_ANNOTATIONS );

            for( const SCH_SYMBOL_INSTANCE& instance : symbol->GetInstances() )
            {
                if( existingRefsSet.contains( instance.m_Reference ) )
                {
                    needsNewKiid = true;
                    break;
                }
            }

            if( needsNewKiid )
            {
                // Assign a new KIID
                const_cast<KIID&>( item->m_Uuid ) = KIID();

                // Make sure pins get a new UUID
                for( SCH_PIN* pin : symbol->GetPins() )
                {
                    const_cast<KIID&>( pin->m_Uuid ) = KIID();
                    pin->SetConnectivityDirty();
                }

                for( SCH_SHEET_PATH& sheetPath : sheetPathsForScreen )
                {
                    // Ignore symbols from a non-existant library.
                    if( libSymbol )
                    {
                        SCH_REFERENCE schReference( symbol, sheetPath );
                        schReference.SetSheetNumber( sheetPath.GetVirtualPageNumber() );
                        pastedSymbols[sheetPath].AddItem( schReference );
                    }
                }
            }
        }
        else if( item->Type() == SCH_SHEET_T )
        {
            SCH_SHEET*  sheet          = (SCH_SHEET*) item;
            SCH_FIELD*  nameField      = sheet->GetField( FIELD_T::SHEET_NAME );
            wxString    baseName       = nameField->GetText();
            wxString    candidateName  = baseName;
            wxString    number;

            while( !baseName.IsEmpty() && wxIsdigit( baseName.Last() ) )
            {
                number = baseName.Last() + number;
                baseName.RemoveLast();
            }

            // Update hierarchy to include any other sheets we already added, avoiding
            // duplicate sheet names
            hierarchy = m_frame->Schematic().Hierarchy();

            int uniquifier = std::max( 0, wxAtoi( number ) ) + 1;

            while( existingSheetNames.count( candidateName ) )
                candidateName = wxString::Format( wxT( "%s%d" ), baseName, uniquifier++ );

            nameField->SetText( candidateName );
            existingSheetNames.emplace( candidateName );

            wxFileName     fn = sheet->GetFileName();
            SCH_SCREEN*    existingScreen = nullptr;

            sheet->SetParent( pasteRoot.Last() );
            sheet->SetScreen( nullptr );

            if( !fn.IsAbsolute() )
            {
                wxFileName currentSheetFileName = pasteRoot.LastScreen()->GetFileName();
                fn.Normalize(  FN_NORMALIZE_FLAGS | wxPATH_NORM_ENV_VARS,
                               currentSheetFileName.GetPath() );
            }

            // Try to find the screen for the pasted sheet by several means
            if( !m_frame->Schematic().Root().SearchHierarchy( fn.GetFullPath( wxPATH_UNIX ),
                                                              &existingScreen ) )
            {
                if( loadedScreens.count( sheet->GetFileName() ) > 0 )
                    existingScreen = loadedScreens.at( sheet->GetFileName() );
                else
                    searchSupplementaryClipboard( sheet->GetFileName(), &existingScreen );
            }

            if( existingScreen )
            {
                sheet->SetScreen( existingScreen );
            }
            else
            {
                if( !m_frame->LoadSheetFromFile( sheet, &pasteRoot, fn.GetFullPath() ) )
                    m_frame->InitSheet( sheet, sheet->GetFileName() );
            }

            // Save the symbol instances in case the user chooses to keep the existing
            // symbol annotation.
            setPastedSymbolInstances( sheet->GetScreen() );
            sheetsPasted = true;

            // Push it to the clipboard path while it still has its old KIID
            clipPath.push_back( sheet->m_Uuid );

            // Assign a new KIID to the pasted sheet
            const_cast<KIID&>( sheet->m_Uuid ) = KIID();

            // Make sure pins get a new UUID
            for( SCH_SHEET_PIN* pin : sheet->GetPins() )
            {
                const_cast<KIID&>( pin->m_Uuid ) = KIID();
                pin->SetConnectivityDirty();
            }

            // Once we have our new KIID we can update all pasted instances. This will either
            // reset the annotations or copy "kept" annotations from the supplementary clipboard.
            for( SCH_SHEET_PATH& sheetPath : sheetPathsForScreen )
            {
                SCH_SHEET_PATH subPath = updatePastedSheet( sheet, sheetPath, clipPath,
                                                            ( forceKeepAnnotations && annotate.automatic ),
                                                            &pastedSheets[sheetPath],
                                                            pastedSymbols );
            }
        }
        else
        {
            SCH_ITEM* srcItem = dynamic_cast<SCH_ITEM*>( itemMap[ item->m_Uuid ] );
            SCH_ITEM* destItem = dynamic_cast<SCH_ITEM*>( item );

            // Everything gets a new KIID
            const_cast<KIID&>( item->m_Uuid ) = KIID();

            if( srcItem && destItem )
            {
                destItem->SetConnectivityDirty( true );
                destItem->SetLastResolvedState( srcItem );
            }
        }

        // Lines need both ends selected for a move after paste so the whole line moves.
        if( item->Type() == SCH_LINE_T )
            item->SetFlags( STARTPOINT | ENDPOINT );

        item->SetFlags( IS_NEW | IS_PASTED | IS_MOVING );

        if( !m_frame->GetScreen()->CheckIfOnDrawList( (SCH_ITEM*) item ) )  // don't want a loop!
            m_frame->AddToScreen( item, m_frame->GetScreen() );

        commit.Added( (SCH_ITEM*) item, m_frame->GetScreen() );

        // Start out hidden so the pasted items aren't "ghosted" in their original location
        // before being moved to the current location.
        getView()->Hide( item, true );
    }

    if( sheetsPasted )
    {
        // The full schematic hierarchy need to be update before assigning new annotation and
        // page numbers.
        m_frame->Schematic().RefreshHierarchy();

        // Update page numbers: Find next free numeric page number
        for( SCH_SHEET_PATH& sheetPath : sheetPathsForScreen )
        {
            for( SCH_SHEET_PATH& pastedSheet : pastedSheets[sheetPath] )
            {
                int      page = 1;
                wxString pageNum = wxString::Format( "%d", page );

                while( hierarchy.PageNumberExists( pageNum ) )
                    pageNum = wxString::Format( "%d", ++page );

                SCH_SHEET_INSTANCE sheetInstance;

                sheetInstance.m_Path = pastedSheet.Path();

                // Don't include the actual sheet in the instance path.
                sheetInstance.m_Path.pop_back();
                sheetInstance.m_PageNumber = pageNum;
                sheetInstance.m_ProjectName = m_frame->Prj().GetProjectName();

                SCH_SHEET* sheet = pastedSheet.Last();

                wxCHECK2( sheet, continue );

                sheet->AddInstance( sheetInstance );
                hierarchy.push_back( pastedSheet );

                // Remove all pasted sheet instance data that is not part of the current project.
                std::vector<KIID_PATH> instancesToRemove;

                for( const SCH_SHEET_INSTANCE& instance : sheet->GetInstances() )
                {
                    if( !hierarchy.HasPath( instance.m_Path ) )
                        instancesToRemove.push_back( instance.m_Path );
                }

                for( const KIID_PATH& instancePath : instancesToRemove )
                    sheet->RemoveInstance( instancePath );
            }
        }

        m_frame->SetSheetNumberAndCount();

        // Get a version with correct sheet numbers since we've pasted sheets,
        // we'll need this when annotating next
        hierarchy = m_frame->Schematic().Hierarchy();
    }

    std::map<SCH_SHEET_PATH, SCH_REFERENCE_LIST> annotatedSymbols;

    // Update the list of symbol instances that satisfy the annotation criteria.
    for( const SCH_SHEET_PATH& sheetPath : sheetPathsForScreen )
    {
        for( size_t i = 0; i < pastedSymbols[sheetPath].GetCount(); i++ )
        {
            if(  pasteMode == PASTE_MODE::UNIQUE_ANNOTATIONS
              || pasteMode == PASTE_MODE::RESPECT_OPTIONS
              || pastedSymbols[sheetPath][i].AlwaysAnnotate() )
            {
                annotatedSymbols[sheetPath].AddItem( pastedSymbols[sheetPath][i] );
            }
        }

        for( const SCH_SHEET_PATH& pastedSheetPath : pastedSheets[sheetPath] )
        {
            for( size_t i = 0; i < pastedSymbols[pastedSheetPath].GetCount(); i++ )
            {
                if(  pasteMode == PASTE_MODE::UNIQUE_ANNOTATIONS
                  || pasteMode == PASTE_MODE::RESPECT_OPTIONS
                  || pastedSymbols[pastedSheetPath][i].AlwaysAnnotate() )
                {
                    annotatedSymbols[pastedSheetPath].AddItem( pastedSymbols[pastedSheetPath][i] );
                }
            }
        }
    }

    if( !annotatedSymbols.empty() )
    {
        for( SCH_SHEET_PATH& path : sheetPathsForScreen )
        {
            annotatedSymbols[path].SortByReferenceOnly();
            annotatedSymbols[path].SetRefDesTracker( schematicSettings.m_refDesTracker );

            if( pasteMode == PASTE_MODE::UNIQUE_ANNOTATIONS )
            {
                annotatedSymbols[path].ReannotateDuplicates( existingRefs );
            }
            else
            {
                annotatedSymbols[path].ReannotateByOptions( (ANNOTATE_ORDER_T) annotate.sort_order,
                                                            (ANNOTATE_ALGO_T) annotate.method,
                                                            annotateStartNum, existingRefs, false,
                                                            &hierarchy );
            }

            annotatedSymbols[path].UpdateAnnotation();

            // Update existing refs for next iteration
            for( size_t i = 0; i < annotatedSymbols[path].GetCount(); i++ )
                existingRefs.AddItem( annotatedSymbols[path][i] );

            for( const SCH_SHEET_PATH& pastedSheetPath : pastedSheets[path] )
            {
                annotatedSymbols[pastedSheetPath].SortByReferenceOnly();
                annotatedSymbols[pastedSheetPath].SetRefDesTracker( schematicSettings.m_refDesTracker );

                if( pasteMode == PASTE_MODE::UNIQUE_ANNOTATIONS )
                {
                    annotatedSymbols[pastedSheetPath].ReannotateDuplicates( existingRefs );
                }
                else
                {
                    annotatedSymbols[pastedSheetPath].ReannotateByOptions( (ANNOTATE_ORDER_T) annotate.sort_order,
                                                                           (ANNOTATE_ALGO_T) annotate.method,
                                                                           annotateStartNum, existingRefs,
                                                                           false,
                                                                           &hierarchy );
                }

                annotatedSymbols[pastedSheetPath].UpdateAnnotation();

                // Update existing refs for next iteration
                for( size_t i = 0; i < annotatedSymbols[pastedSheetPath].GetCount(); i++ )
                    existingRefs.AddItem( annotatedSymbols[pastedSheetPath][i] );
            }
        }
    }

    m_frame->GetCurrentSheet().UpdateAllScreenReferences();

    // The copy operation creates instance paths that are not valid for the current project or
    // saved as part of another project.  Prune them now so they do not accumulate in the saved
    // schematic file.
    prunePastedSymbolInstances();

    SCH_SHEET_LIST sheets = m_frame->Schematic().Hierarchy();
    SCH_SCREENS    allScreens( m_frame->Schematic().Root() );

    allScreens.PruneOrphanedSymbolInstances( m_frame->Prj().GetProjectName(), sheets );
    allScreens.PruneOrphanedSheetInstances( m_frame->Prj().GetProjectName(), sheets );

    // Now clear the previous selection, select the pasted items, and fire up the "move" tool.
    m_toolMgr->RunAction( ACTIONS::selectionClear );

    // If the item has a parent group, it will be part of the loadedItems, and will handle
    // the move action. Iterate backwards to avoid invalidating the iterator.
    for( int i = loadedItems.size() - 1; i >= 0; i-- )
    {
        EDA_ITEM* item = loadedItems[i];

        if( item->GetParentGroup() )
        {
            loadedItems.erase( loadedItems.begin() + i );
            // These were hidden before because they would be added to the move preview,
            // but now they need to be shown as a preview so they appear to move when
            // the group moves.
            getView()->SetVisible( item );
            getView()->AddToPreview( item, false );
        }
    }

    m_toolMgr->RunAction<EDA_ITEMS*>( ACTIONS::selectItems, &loadedItems );

    SCH_SELECTION& selection = selTool->GetSelection();

    if( !selection.Empty() )
    {
        if( aEvent.IsAction( &ACTIONS::duplicate ) )
        {
            int closest_dist = INT_MAX;

            auto processPt =
                    [&]( const VECTOR2I& pt )
                    {
                        int dist = ( eventPos - pt ).EuclideanNorm();

                        if( dist < closest_dist )
                        {
                            selection.SetReferencePoint( pt );
                            closest_dist = dist;
                        }
                    };

            // Prefer connection points (which should remain on grid)
            for( EDA_ITEM* item : selection.Items() )
            {
                SCH_ITEM* sch_item = dynamic_cast<SCH_ITEM*>( item );
                SCH_PIN*  pin = dynamic_cast<SCH_PIN*>( item );

                if( sch_item && sch_item->IsConnectable() )
                {
                    for( const VECTOR2I& pt : sch_item->GetConnectionPoints() )
                        processPt( pt );
                }
                else if( pin )
                {
                    processPt( pin->GetPosition() );
                }

                // Symbols need to have their center point added since often users are trying to
                // move parts from their center.
                if( dynamic_cast<SCH_SYMBOL*>( item ) )
                    processPt( item->GetPosition() );
            }

            // Only process other points if we didn't find any connection points
            if( closest_dist == INT_MAX )
            {
                for( EDA_ITEM* item : selection.Items() )
                {
                    switch( item->Type() )
                    {
                    case SCH_LINE_T:
                        processPt( static_cast<SCH_LINE*>( item )->GetStartPoint() );
                        processPt( static_cast<SCH_LINE*>( item )->GetEndPoint() );
                        break;

                    case SCH_SHAPE_T:
                    {
                        SCH_SHAPE* shape = static_cast<SCH_SHAPE*>( item );

                        switch( shape->GetShape() )
                        {
                        case SHAPE_T::RECTANGLE:
                            for( const VECTOR2I& pt : shape->GetRectCorners() )
                                processPt( pt );

                            break;

                        case SHAPE_T::CIRCLE:
                            processPt( shape->GetCenter() );
                            break;

                        case SHAPE_T::POLY:
                            for( int ii = 0; ii < shape->GetPolyShape().TotalVertices(); ++ii )
                                processPt( shape->GetPolyShape().CVertex( ii ) );

                            break;

                        default:
                            processPt( shape->GetStart() );
                            processPt( shape->GetEnd() );
                            break;
                        }

                        break;
                    }

                    default:
                        processPt( item->GetPosition() );
                        break;
                    }
                }
            }

            selection.SetIsHover( m_duplicateIsHoverSelection );
        }
        // We want to the first non-group item in the selection to be the reference point.
        else if( selection.GetTopLeftItem()->Type() == SCH_GROUP_T )
        {
            SCH_GROUP* group = static_cast<SCH_GROUP*>( selection.GetTopLeftItem() );

            bool      found = false;
            SCH_ITEM* item = nullptr;

            group->RunOnChildren(
                    [&]( SCH_ITEM* schItem )
                    {
                        if( !found && schItem->Type() != SCH_GROUP_T )
                        {
                            item = schItem;
                            found = true;
                        }
                    },
                    RECURSE_MODE::RECURSE );

            if( found )
                selection.SetReferencePoint( item->GetPosition() );
            else
                selection.SetReferencePoint( group->GetPosition() );
        }
        else
        {
            SCH_ITEM* item = static_cast<SCH_ITEM*>( selection.GetTopLeftItem() );

            selection.SetReferencePoint( item->GetPosition() );
        }

        if( m_toolMgr->RunSynchronousAction( SCH_ACTIONS::move, &commit ) )
        {
            // Pushing the commit will update the connectivity.
            commit.Push( _( "Paste" ) );

            if( sheetsPasted )
            {
                m_frame->UpdateHierarchyNavigator();
                // UpdateHierarchyNavigator() will call RefreshNetNavigator()
            }
            else
            {
                m_frame->RefreshNetNavigator();
            }
        }
        else
        {
            commit.Revert();
        }

        getView()->ClearPreview();
    }

    return 0;
}


int SCH_EDITOR_CONTROL::EditWithSymbolEditor( const TOOL_EVENT& aEvent )
{
    SCH_SELECTION_TOOL* selTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
    SCH_SELECTION&      selection = selTool->RequestSelection( { SCH_SYMBOL_T } );
    SCH_SYMBOL*         symbol = nullptr;
    SYMBOL_EDIT_FRAME*  symbolEditor;

    if( selection.GetSize() >= 1 )
        symbol = (SCH_SYMBOL*) selection.Front();

    if( selection.IsHover() )
        m_toolMgr->RunAction( ACTIONS::selectionClear );

    if( !symbol )
    {
        // Giant hack: by default we assign Edit Table to the same hotkey, so give the table
        // tool a chance to handle it if we can't.
        if( SCH_EDIT_TABLE_TOOL* tableTool = m_toolMgr->GetTool<SCH_EDIT_TABLE_TOOL>() )
            tableTool->EditTable( aEvent );

        return 0;
    }

    if( symbol->GetEditFlags() != 0 )
        return 0;

    if( symbol->IsMissingLibSymbol() )
    {
        m_frame->ShowInfoBarError( _( "Symbols with broken library symbol links cannot "
                                      "be edited." ) );
        return 0;
    }

    m_toolMgr->RunAction( ACTIONS::showSymbolEditor );
    symbolEditor = (SYMBOL_EDIT_FRAME*) m_frame->Kiway().Player( FRAME_SCH_SYMBOL_EDITOR, false );

    if( symbolEditor )
    {
        if( wxWindow* blocking_win = symbolEditor->Kiway().GetBlockingDialog() )
            blocking_win->Close( true );

        if( aEvent.IsAction( &SCH_ACTIONS::editWithLibEdit ) )
        {
            symbolEditor->LoadSymbolFromSchematic( symbol );
        }
        else if( aEvent.IsAction( &SCH_ACTIONS::editLibSymbolWithLibEdit ) )
        {
            symbolEditor->LoadSymbol( symbol->GetLibId(), symbol->GetUnit(),
                                      symbol->GetBodyStyle() );

            if( !symbolEditor->IsLibraryTreeShown() )
                symbolEditor->ToggleLibraryTree();
        }
    }

    return 0;
}


int SCH_EDITOR_CONTROL::Annotate( const TOOL_EVENT& aEvent )
{
    m_frame->OnAnnotate();
    return 0;
}


int SCH_EDITOR_CONTROL::IncrementAnnotations( const TOOL_EVENT& aEvent )
{
    DIALOG_INCREMENT_ANNOTATIONS_BASE dlg( m_frame );
    dlg.m_FirstRefDes->SetValidator( wxTextValidator( wxFILTER_EMPTY ) );

    dlg.SetInitialFocus( dlg.m_FirstRefDes );

    if( dlg.ShowModal() == wxID_OK )
    {
        SCH_REFERENCE startRef;
        startRef.SetRef( dlg.m_FirstRefDes->GetValue() );

        if( startRef.IsSplitNeeded() )
            startRef.Split();
        else
            return 0;

        int startNum = atoi( startRef.GetRefNumber().utf8_string().c_str() );

        SCH_COMMIT         commit( m_frame );
        SCHEMATIC*         schematic = m_frame->m_schematic;
        SCH_REFERENCE_LIST references;

        if( dlg.m_AllSheets->GetValue() )
            schematic->Hierarchy().GetSymbols( references );
        else
            schematic->CurrentSheet().GetSymbols( references );

        references.SplitReferences();

        for( SCH_REFERENCE& ref : references )
        {
            if( ref.GetRef() == startRef.GetRef() )
            {
                int num = atoi( ref.GetRefNumber().utf8_string().c_str() );

                if( num >= startNum )
                {
                    const SCH_SHEET_PATH& sheet = ref.GetSheetPath();
                    wxString              fullRef = ref.GetRef();

                    num += dlg.m_Increment->GetValue();
                    fullRef << num;

                    commit.Modify( ref.GetSymbol(), sheet.LastScreen(), RECURSE_MODE::NO_RECURSE );
                    ref.GetSymbol()->SetRef( &sheet, From_UTF8( fullRef.c_str() ) );
                }
            }
        }

        if( !commit.Empty() )
            commit.Push( _( "Increment Annotations" ) );
    }

    return 0;
}


int SCH_EDITOR_CONTROL::ShowCvpcb( const TOOL_EVENT& aEvent )
{
    m_frame->OnOpenCvpcb();
    return 0;
}


int SCH_EDITOR_CONTROL::EditSymbolFields( const TOOL_EVENT& aEvent )
{
    DIALOG_SYMBOL_FIELDS_TABLE* dlg = m_frame->GetSymbolFieldsTableDialog();

    wxCHECK( dlg, 0 );

    // Needed at least on Windows. Raise() is not enough
    dlg->Show( true );

    // Bring it to the top if already open.  Dual monitor users need this.
    dlg->Raise();

    dlg->ShowEditTab();

    return 0;
}


int SCH_EDITOR_CONTROL::EditSymbolLibraryLinks( const TOOL_EVENT& aEvent )
{
    if( InvokeDialogEditSymbolsLibId( m_frame ) )
        m_frame->HardRedraw();

    return 0;
}


int SCH_EDITOR_CONTROL::ShowPcbNew( const TOOL_EVENT& aEvent )
{
    m_frame->OnOpenPcbnew();
    return 0;
}


int SCH_EDITOR_CONTROL::UpdatePCB( const TOOL_EVENT& aEvent )
{
    m_frame->OnUpdatePCB();
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
    DIALOG_SYMBOL_FIELDS_TABLE* dlg = m_frame->GetSymbolFieldsTableDialog();

    wxCHECK( dlg, 0 );

    // Needed at least on Windows. Raise() is not enough
    dlg->Show( true );

    // Bring it to the top if already open.  Dual monitor users need this.
    dlg->Raise();

    dlg->ShowExportTab();

    return 0;
}


int SCH_EDITOR_CONTROL::GenerateBOMLegacy( const TOOL_EVENT& aEvent )
{
    InvokeDialogCreateBOM( m_frame );
    return 0;
}


int SCH_EDITOR_CONTROL::DrawSheetOnClipboard( const TOOL_EVENT& aEvent )
{
    m_frame->RecalculateConnections( nullptr, LOCAL_CLEANUP );
    m_frame->DrawCurrentSheetToClipboard();
    return 0;
}


int SCH_EDITOR_CONTROL::ShowSearch( const TOOL_EVENT& aEvent )
{
    getEditFrame<SCH_EDIT_FRAME>()->ToggleSearch();
    return 0;
}


int SCH_EDITOR_CONTROL::ShowHierarchy( const TOOL_EVENT& aEvent )
{
    getEditFrame<SCH_EDIT_FRAME>()->ToggleSchematicHierarchy();
    return 0;
}


int SCH_EDITOR_CONTROL::ShowNetNavigator( const TOOL_EVENT& aEvent )
{
    getEditFrame<SCH_EDIT_FRAME>()->ToggleNetNavigator();
    return 0;
}


int SCH_EDITOR_CONTROL::ToggleProperties( const TOOL_EVENT& aEvent )
{
    getEditFrame<SCH_EDIT_FRAME>()->ToggleProperties();
    return 0;
}


int SCH_EDITOR_CONTROL::ToggleLibraryTree( const TOOL_EVENT& aEvent )
{
    getEditFrame<SCH_EDIT_FRAME>()->ToggleLibraryTree();
    return 0;
}


int SCH_EDITOR_CONTROL::ToggleHiddenPins( const TOOL_EVENT& aEvent )
{
    EESCHEMA_SETTINGS* cfg = m_frame->eeconfig();
    cfg->m_Appearance.show_hidden_pins = !cfg->m_Appearance.show_hidden_pins;

    getView()->UpdateAllItems( KIGFX::REPAINT );
    m_frame->GetCanvas()->Refresh();

    return 0;
}


int SCH_EDITOR_CONTROL::ToggleHiddenFields( const TOOL_EVENT& aEvent )
{
    EESCHEMA_SETTINGS* cfg = m_frame->eeconfig();
    cfg->m_Appearance.show_hidden_fields = !cfg->m_Appearance.show_hidden_fields;

    getView()->UpdateAllItems( KIGFX::REPAINT );
    m_frame->GetCanvas()->Refresh();

    return 0;
}


int SCH_EDITOR_CONTROL::ToggleDirectiveLabels( const TOOL_EVENT& aEvent )
{
    EESCHEMA_SETTINGS* cfg = m_frame->eeconfig();
    cfg->m_Appearance.show_directive_labels = !cfg->m_Appearance.show_directive_labels;

    getView()->UpdateAllItems( KIGFX::REPAINT );
    m_frame->GetCanvas()->Refresh();

    return 0;
}


int SCH_EDITOR_CONTROL::ToggleERCWarnings( const TOOL_EVENT& aEvent )
{
    EESCHEMA_SETTINGS* cfg = m_frame->eeconfig();
    cfg->m_Appearance.show_erc_warnings = !cfg->m_Appearance.show_erc_warnings;

    getView()->SetLayerVisible( LAYER_ERC_WARN, cfg->m_Appearance.show_erc_warnings );
    m_frame->GetCanvas()->Refresh();

    return 0;
}


int SCH_EDITOR_CONTROL::ToggleERCErrors( const TOOL_EVENT& aEvent )
{
    EESCHEMA_SETTINGS* cfg = m_frame->eeconfig();
    cfg->m_Appearance.show_erc_errors = !cfg->m_Appearance.show_erc_errors;

    getView()->SetLayerVisible( LAYER_ERC_ERR, cfg->m_Appearance.show_erc_errors );
    m_frame->GetCanvas()->Refresh();

    return 0;
}


int SCH_EDITOR_CONTROL::ToggleERCExclusions( const TOOL_EVENT& aEvent )
{
    EESCHEMA_SETTINGS* cfg = m_frame->eeconfig();
    cfg->m_Appearance.show_erc_exclusions = !cfg->m_Appearance.show_erc_exclusions;

    m_frame->GetCanvas()->Refresh();

    return 0;
}


int SCH_EDITOR_CONTROL::MarkSimExclusions( const TOOL_EVENT& aEvent )
{
    EESCHEMA_SETTINGS* cfg = m_frame->eeconfig();
    cfg->m_Appearance.mark_sim_exclusions = !cfg->m_Appearance.mark_sim_exclusions;

    m_frame->GetCanvas()->GetView()->UpdateAllItemsConditionally(
            [&]( KIGFX::VIEW_ITEM* aItem ) -> int
            {
                int flags = 0;

                auto invalidateTextVars =
                        [&flags]( EDA_TEXT* text )
                        {
                            if( text->HasTextVars() )
                            {
                                text->ClearRenderCache();
                                text->ClearBoundingBoxCache();
                                flags |= KIGFX::GEOMETRY | KIGFX::REPAINT;
                            }
                        };

                if( SCH_ITEM* item = dynamic_cast<SCH_ITEM*>( aItem ) )
                {
                    item->RunOnChildren(
                            [&invalidateTextVars]( SCH_ITEM* aChild )
                            {
                                if( EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( aChild ) )
                                    invalidateTextVars( text );
                            },
                            RECURSE_MODE::NO_RECURSE );

                    if( item->GetExcludedFromSim() )
                        flags |= KIGFX::GEOMETRY | KIGFX::REPAINT;
                }

                if( EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( aItem ) )
                    invalidateTextVars( text );

                return flags;
            } );

    m_frame->GetCanvas()->Refresh();

    return 0;
}


int SCH_EDITOR_CONTROL::ToggleOPVoltages( const TOOL_EVENT& aEvent )
{
    EESCHEMA_SETTINGS* cfg = m_frame->eeconfig();
    cfg->m_Appearance.show_op_voltages = !cfg->m_Appearance.show_op_voltages;

    getView()->SetLayerVisible( LAYER_OP_VOLTAGES, cfg->m_Appearance.show_op_voltages );
    m_frame->RefreshOperatingPointDisplay();
    m_frame->GetCanvas()->Refresh();

    return 0;
}


int SCH_EDITOR_CONTROL::ToggleOPCurrents( const TOOL_EVENT& aEvent )
{
    EESCHEMA_SETTINGS* cfg = m_frame->eeconfig();
    cfg->m_Appearance.show_op_currents = !cfg->m_Appearance.show_op_currents;

    getView()->SetLayerVisible( LAYER_OP_CURRENTS, cfg->m_Appearance.show_op_currents );
    m_frame->RefreshOperatingPointDisplay();
    m_frame->GetCanvas()->Refresh();

    return 0;
}


int SCH_EDITOR_CONTROL::TogglePinAltIcons( const TOOL_EVENT& aEvent )
{
    EESCHEMA_SETTINGS* cfg = m_frame->eeconfig();
    cfg->m_Appearance.show_pin_alt_icons = !cfg->m_Appearance.show_pin_alt_icons;

    m_frame->GetRenderSettings()->m_ShowPinAltIcons = cfg->m_Appearance.show_pin_alt_icons;

    getView()->UpdateAllItems( KIGFX::REPAINT );
    m_frame->GetCanvas()->Refresh();

    return 0;
}


int SCH_EDITOR_CONTROL::ChangeLineMode( const TOOL_EVENT& aEvent )
{
    m_frame->eeconfig()->m_Drawing.line_mode = aEvent.Parameter<LINE_MODE>();
    m_toolMgr->PostAction( ACTIONS::refreshPreview );
    return 0;
}


int SCH_EDITOR_CONTROL::NextLineMode( const TOOL_EVENT& aEvent )
{
    m_frame->eeconfig()->m_Drawing.line_mode++;
    m_frame->eeconfig()->m_Drawing.line_mode %= LINE_MODE::LINE_MODE_COUNT;
    m_toolMgr->PostAction( ACTIONS::refreshPreview );
    return 0;
}


int SCH_EDITOR_CONTROL::ToggleAnnotateAuto( const TOOL_EVENT& aEvent )
{
    EESCHEMA_SETTINGS* cfg = m_frame->eeconfig();
    cfg->m_AnnotatePanel.automatic = !cfg->m_AnnotatePanel.automatic;
    return 0;
}


int SCH_EDITOR_CONTROL::ToggleAnnotateRecursive( const TOOL_EVENT& aEvent )
{
    EESCHEMA_SETTINGS* cfg = m_frame->eeconfig();
    cfg->m_AnnotatePanel.recursive = !cfg->m_AnnotatePanel.recursive;
    return 0;
}


int SCH_EDITOR_CONTROL::TogglePythonConsole( const TOOL_EVENT& aEvent )
{

    m_frame->ScriptingConsoleEnableDisable();
    return 0;
}


int SCH_EDITOR_CONTROL::ReloadPlugins( const TOOL_EVENT& aEvent )
{
#ifdef KICAD_IPC_API
    if( Pgm().GetCommonSettings()->m_Api.enable_server )
        Pgm().GetPluginManager().ReloadPlugins();
#endif
    return 0;
}


int SCH_EDITOR_CONTROL::RepairSchematic( const TOOL_EVENT& aEvent )
{
    int      errors = 0;
    wxString details;
    bool     quiet = aEvent.Parameter<bool>();

    // Repair duplicate IDs.
    std::map<KIID, EDA_ITEM*> ids;
    int                       duplicates = 0;

    SCH_SHEET_LIST sheets = m_frame->Schematic().Hierarchy();

    auto processItem =
            [&]( EDA_ITEM* aItem )
            {
                auto it = ids.find( aItem->m_Uuid );

                if( it != ids.end() && it->second != aItem )
                {
                    duplicates++;
                    const_cast<KIID&>( aItem->m_Uuid ) = KIID();
                }

                ids[ aItem->m_Uuid ] = aItem;
            };

    // Symbol IDs are the most important, so give them the first crack at "claiming" a
    // particular KIID.

    for( const SCH_SHEET_PATH& sheet : sheets )
    {
        SCH_SCREEN* screen = sheet.LastScreen();

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            processItem( item );

            for( SCH_PIN* pin : static_cast<SCH_SYMBOL*>( item )->GetPins( &sheet ) )
                processItem( pin );
        }
    }

    for( const SCH_SHEET_PATH& sheet : sheets )
    {
        SCH_SCREEN* screen = sheet.LastScreen();

        for( SCH_ITEM* item : screen->Items() )
        {
            processItem( item );

            if( item->Type() != SCH_GROUP_T )
            {
                item->RunOnChildren(
                        [&]( SCH_ITEM* aChild )
                        {
                            processItem( item );
                        },
                        RECURSE_MODE::NO_RECURSE );
            }
        }
    }

    /*******************************
     * Your test here
     */

    /*******************************
     * Inform the user
     */

    if( duplicates )
    {
        errors += duplicates;
        details += wxString::Format( _( "%d duplicate IDs replaced.\n" ), duplicates );

        // Rehash sheetpaths as we may have changed their uuids.
        m_frame->Schematic().RefreshHierarchy();
    }

    if( errors )
    {
        m_frame->OnModify();

        wxString msg = wxString::Format( _( "%d potential problems repaired." ), errors );

        if( !quiet )
            DisplayInfoMessage( m_frame, msg, details );
    }
    else if( !quiet )
    {
        DisplayInfoMessage( m_frame, _( "No errors found." ) );
    }

    return 0;
}


int SCH_EDITOR_CONTROL::GridFeedback( const TOOL_EVENT& aEvent )
{
    if( !Pgm().GetCommonSettings()->m_Input.hotkey_feedback )
        return 0;

    GRID_SETTINGS& gridSettings = m_toolMgr->GetSettings()->m_Window.grid;
    int currentIdx = m_toolMgr->GetSettings()->m_Window.grid.last_size_idx;

    wxArrayString gridsLabels;

    for( const GRID& grid : gridSettings.grids )
        gridsLabels.Add( grid.UserUnitsMessageText( m_frame ) );

    if( !m_frame->GetHotkeyPopup() )
        m_frame->CreateHotkeyPopup();

    HOTKEY_CYCLE_POPUP* popup = m_frame->GetHotkeyPopup();

    if( popup )
        popup->Popup( _( "Grid" ), gridsLabels, currentIdx );

    return 0;
}


int SCH_EDITOR_CONTROL::PlaceLinkedDesignBlock( const TOOL_EVENT& aEvent )
{
    SCH_EDIT_FRAME* editFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );

    if( !editFrame )
        return 1;

    // Need to have a group selected and it needs to have a linked design block
    SCH_SELECTION_TOOL* selTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
    SCH_SELECTION       selection = selTool->GetSelection();

    if( selection.Size() != 1 || selection[0]->Type() != SCH_GROUP_T )
        return 1;

    SCH_GROUP* group = static_cast<SCH_GROUP*>( selection[0] );

    if( !group->HasDesignBlockLink() )
        return 1;

    // Get the associated design block
    DESIGN_BLOCK_PANE* designBlockPane = editFrame->GetDesignBlockPane();
    std::unique_ptr<DESIGN_BLOCK> designBlock( designBlockPane->GetDesignBlock( group->GetDesignBlockLibId(),
                                                                                true, true ) );

    if( !designBlock )
    {
        wxString msg;
        msg.Printf( _( "Could not find design block %s." ), group->GetDesignBlockLibId().GetUniStringLibId() );
        m_frame->GetInfoBar()->ShowMessageFor( msg, 5000, wxICON_WARNING );
        return 1;
    }

    if( designBlock->GetSchematicFile().IsEmpty() )
    {
        wxString msg;
        msg.Printf( _( "Design block %s does not have a schematic file." ),
                    group->GetDesignBlockLibId().GetUniStringLibId() );
        m_frame->GetInfoBar()->ShowMessageFor( msg, 5000, wxICON_WARNING );
        return 1;
    }

    editFrame->GetDesignBlockPane()->SelectLibId( group->GetDesignBlockLibId() );

    return m_toolMgr->RunAction( SCH_ACTIONS::placeDesignBlock, designBlock.release() );
}


int SCH_EDITOR_CONTROL::SaveToLinkedDesignBlock( const TOOL_EVENT& aEvent )
{
    SCH_EDIT_FRAME* editFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );

    if( !editFrame )
        return 1;

    // Need to have a group selected and it needs to have a linked design block
    SCH_SELECTION_TOOL* selTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
    SCH_SELECTION       selection = selTool->GetSelection();

    if( selection.Size() != 1 || selection[0]->Type() != SCH_GROUP_T )
        return 1;

    SCH_GROUP* group = static_cast<SCH_GROUP*>( selection[0] );

    if( !group->HasDesignBlockLink() )
        return 1;

    // Get the associated design block
    DESIGN_BLOCK_PANE* designBlockPane = editFrame->GetDesignBlockPane();
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

    return m_toolMgr->RunAction( SCH_ACTIONS::saveSelectionToDesignBlock ) ? 1 : 0;
}


void SCH_EDITOR_CONTROL::setTransitions()
{
    Go( &SCH_EDITOR_CONTROL::New,                   ACTIONS::doNew.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::Open,                  ACTIONS::open.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::Save,                  ACTIONS::save.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::SaveAs,                ACTIONS::saveAs.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::SaveCurrSheetCopyAs,   SCH_ACTIONS::saveCurrSheetCopyAs.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::Revert,                ACTIONS::revert.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ShowSchematicSetup,    SCH_ACTIONS::schematicSetup.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::PageSetup,             ACTIONS::pageSettings.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::Print,                 ACTIONS::print.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::Plot,                  ACTIONS::plot.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::Quit,                  ACTIONS::quit.MakeEvent() );

    Go( &SCH_EDITOR_CONTROL::RescueSymbols,         SCH_ACTIONS::rescueSymbols.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::RemapSymbols,          SCH_ACTIONS::remapSymbols.MakeEvent() );

    Go( &SCH_EDITOR_CONTROL::CrossProbeToPcb,         EVENTS::PointSelectedEvent );
    Go( &SCH_EDITOR_CONTROL::CrossProbeToPcb,         EVENTS::SelectedEvent );
    Go( &SCH_EDITOR_CONTROL::CrossProbeToPcb,         EVENTS::UnselectedEvent );
    Go( &SCH_EDITOR_CONTROL::CrossProbeToPcb,         EVENTS::ClearedEvent );
    Go( &SCH_EDITOR_CONTROL::ExplicitCrossProbeToPcb, SCH_ACTIONS::selectOnPCB.MakeEvent() );

    Go( &SCH_EDITOR_CONTROL::SimProbe,              SCH_ACTIONS::simProbe.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::SimTune,               SCH_ACTIONS::simTune.MakeEvent() );

    Go( &SCH_EDITOR_CONTROL::HighlightNet,          SCH_ACTIONS::highlightNet.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ClearHighlight,        SCH_ACTIONS::clearHighlight.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::HighlightNetCursor,    SCH_ACTIONS::highlightNetTool.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::UpdateNetHighlighting, EVENTS::SelectedItemsModified );
    Go( &SCH_EDITOR_CONTROL::UpdateNetHighlighting, SCH_ACTIONS::updateNetHighlighting.MakeEvent() );

    Go( &SCH_EDITOR_CONTROL::AssignNetclass,        SCH_ACTIONS::assignNetclass.MakeEvent() );

    Go( &SCH_EDITOR_CONTROL::Undo,                  ACTIONS::undo.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::Redo,                  ACTIONS::redo.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::Cut,                   ACTIONS::cut.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::Copy,                  ACTIONS::copy.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::CopyAsText,            ACTIONS::copyAsText.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::Paste,                 ACTIONS::paste.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::Paste,                 ACTIONS::pasteSpecial.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::Duplicate,             ACTIONS::duplicate.MakeEvent() );

    Go( &SCH_EDITOR_CONTROL::GridFeedback,          EVENTS::GridChangedByKeyEvent );

    Go( &SCH_EDITOR_CONTROL::EditWithSymbolEditor,   SCH_ACTIONS::editWithLibEdit.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::EditWithSymbolEditor,   SCH_ACTIONS::editLibSymbolWithLibEdit.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ShowCvpcb,              SCH_ACTIONS::assignFootprints.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ImportFPAssignments,    SCH_ACTIONS::importFPAssignments.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::Annotate,               SCH_ACTIONS::annotate.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::IncrementAnnotations,   SCH_ACTIONS::incrementAnnotations.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::EditSymbolFields,       SCH_ACTIONS::editSymbolFields.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::EditSymbolLibraryLinks, SCH_ACTIONS::editSymbolLibraryLinks.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ShowPcbNew,             SCH_ACTIONS::showPcbNew.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::UpdatePCB,              ACTIONS::updatePcbFromSchematic.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::UpdateFromPCB,          ACTIONS::updateSchematicFromPcb.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ExportNetlist,          SCH_ACTIONS::exportNetlist.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::GenerateBOM,            SCH_ACTIONS::generateBOM.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::GenerateBOMLegacy,      SCH_ACTIONS::generateBOMLegacy.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::DrawSheetOnClipboard,   SCH_ACTIONS::drawSheetOnClipboard.MakeEvent() );

    Go( &SCH_EDITOR_CONTROL::ShowSearch,             SCH_ACTIONS::showSearch.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ShowHierarchy,          SCH_ACTIONS::showHierarchy.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ShowNetNavigator,       SCH_ACTIONS::showNetNavigator.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ToggleProperties,       ACTIONS::showProperties.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ToggleLibraryTree,      SCH_ACTIONS::showDesignBlockPanel.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ToggleLibraryTree,      SCH_ACTIONS::showDesignBlockPanel.MakeEvent() );

    Go( &SCH_EDITOR_CONTROL::ToggleHiddenPins,       SCH_ACTIONS::toggleHiddenPins.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ToggleHiddenFields,     SCH_ACTIONS::toggleHiddenFields.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ToggleDirectiveLabels,  SCH_ACTIONS::toggleDirectiveLabels.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ToggleERCWarnings,      SCH_ACTIONS::toggleERCWarnings.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ToggleERCErrors,        SCH_ACTIONS::toggleERCErrors.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ToggleERCExclusions,    SCH_ACTIONS::toggleERCExclusions.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::MarkSimExclusions,      SCH_ACTIONS::markSimExclusions.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ToggleOPVoltages,       SCH_ACTIONS::toggleOPVoltages.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ToggleOPCurrents,       SCH_ACTIONS::toggleOPCurrents.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::TogglePinAltIcons,      SCH_ACTIONS::togglePinAltIcons.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ChangeLineMode,         SCH_ACTIONS::lineModeFree.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ChangeLineMode,         SCH_ACTIONS::lineMode90.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ChangeLineMode,         SCH_ACTIONS::lineMode45.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::NextLineMode,           SCH_ACTIONS::lineModeNext.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ToggleAnnotateAuto,     SCH_ACTIONS::toggleAnnotateAuto.MakeEvent() );

    Go( &SCH_EDITOR_CONTROL::ReloadPlugins,          ACTIONS::pluginsReload.MakeEvent() );

    Go( &SCH_EDITOR_CONTROL::RepairSchematic,        SCH_ACTIONS::repairSchematic.MakeEvent() );

    Go( &SCH_EDITOR_CONTROL::ExportSymbolsToLibrary, SCH_ACTIONS::exportSymbolsToLibrary.MakeEvent() );

    Go( &SCH_EDITOR_CONTROL::PlaceLinkedDesignBlock, SCH_ACTIONS::placeLinkedDesignBlock.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::SaveToLinkedDesignBlock, SCH_ACTIONS::saveToLinkedDesignBlock.MakeEvent() );
}
