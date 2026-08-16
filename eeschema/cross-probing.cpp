/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <wx/tokenzr.h>
#include <api/api_utils.h>
#include <api/common/commands/cross_probe_commands.pb.h>
#include <api/cross_probe_client.h>
#include <fmt.h>
#include <kiface_base.h>
#include <kiway.h>
#include <kiway_mail.h>
#include <connection_graph.h>
#include <sch_netchain.h>
#include <sch_sheet.h>
#include <sch_symbol.h>
#include <sch_reference_list.h>
#include <string_utils.h>
#include <netlist_exporters/netlist_exporter_kicad.h>
#include <project/project_file.h>
#include <project/net_settings.h>
#include <project_sch.h>
#include <richio.h>
#include <tools/sch_actions.h>
#include <tools/sch_editor_control.h>
#include <advanced_config.h>
#include <api/api_handler_sch.h>

#include <pgm_base.h>
#include <libraries/symbol_library_adapter.h>
#include <widgets/sch_design_block_pane.h>
#include <widgets/kistatusbar.h>
#include <wx/filefn.h>
#include <wx/log.h>
#include <trace_helpers.h>

SCH_ITEM* SCH_EDITOR_CONTROL::FindSymbolAndItem( const wxString* aPath, const wxString* aReference,
                                                 bool aSearchHierarchy, SCH_SEARCH_T aSearchType,
                                                 const wxString& aSearchText )
{
    SCH_SHEET_PATH* sheetWithSymbolFound = nullptr;
    SCH_SYMBOL*     symbol = nullptr;
    SCH_PIN*        pin = nullptr;
    SCH_SHEET_LIST  sheetList;
    SCH_ITEM*       foundItem = nullptr;

    if( !aSearchHierarchy )
        sheetList.push_back( m_frame->GetCurrentSheet() );
    else
        sheetList = m_frame->Schematic().Hierarchy();

    for( SCH_SHEET_PATH& sheet : sheetList )
    {
        SCH_SCREEN* screen = sheet.LastScreen();

        for( EDA_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* candidate = static_cast<SCH_SYMBOL*>( item );

            // Search by path if specified, otherwise search by reference
            bool found = false;

            if( aPath )
            {
                wxString path = sheet.PathAsString() + candidate->m_Uuid.AsString();
                found = ( *aPath == path );
            }
            else
            {
                found = ( aReference && aReference->CmpNoCase( candidate->GetRef( &sheet ) ) == 0 );
            }

            if( found )
            {
                symbol = candidate;
                sheetWithSymbolFound = &sheet;

                if( aSearchType == HIGHLIGHT_PIN )
                {
                    pin = symbol->GetPin( aSearchText );

                    // Fall back to reverse pin-to-pad resolution so a remapped pad highlights its
                    // owning pin (issue #2282); the search text from pcbnew is a pad number.
                    if( !pin )
                        pin = symbol->GetPinByEffectivePadNumber( aSearchText, &sheet,
                                                                  m_frame->Schematic().GetCurrentVariant() );

                    // Ensure we have found the right unit in case of multi-units symbol
                    if( pin )
                    {
                        int unit = pin->GetLibPin()->GetUnit();

                        if( unit != 0 && unit != symbol->GetUnit() )
                        {
                            pin = nullptr;
                            continue;
                        }

                        // Get pin position in true schematic coordinate
                        foundItem = pin;
                        break;
                    }
                }
                else
                {
                    foundItem = symbol;
                    break;
                }
            }
        }

        if( foundItem )
            break;
    }

    CROSS_PROBING_SETTINGS& crossProbingSettings = m_frame->eeconfig()->m_CrossProbing;

    if( symbol )
    {
        if( *sheetWithSymbolFound != m_frame->GetCurrentSheet() )
        {
            m_frame->GetToolManager()->RunAction<SCH_SHEET_PATH*>( SCH_ACTIONS::changeSheet,
                                                                   sheetWithSymbolFound );
        }

        if( crossProbingSettings.center_on_items )
        {
            if( crossProbingSettings.zoom_to_fit )
            {
                BOX2I bbox = symbol->GetBoundingBox();

                m_toolMgr->GetTool<SCH_SELECTION_TOOL>()->ZoomFitCrossProbeBBox( bbox );
            }

            if( pin )
                m_frame->FocusOnItem( pin );
            else
                m_frame->FocusOnItem( symbol );
        }
    }

    /* Print diag */
    wxString msg;
    wxString displayRef;

    if( aReference )
        displayRef = *aReference;
    else if( aPath )
        displayRef = *aPath;

    if( symbol )
    {
        if( aSearchType == HIGHLIGHT_PIN )
        {
            if( foundItem )
                msg.Printf( _( "%s pin %s found" ), displayRef, aSearchText );
            else
                msg.Printf( _( "%s found but pin %s not found" ), displayRef, aSearchText );
        }
        else
        {
            msg.Printf( _( "%s found" ), displayRef );
        }
    }
    else
    {
        msg.Printf( _( "%s not found" ), displayRef );
    }

    m_frame->SetStatusText( msg );
    m_frame->GetCanvas()->Refresh();

    return foundItem;
}


/* Execute a remote command sent via a socket on port KICAD_PCB_PORT_SERVICE_NUMBER
 *
 * Commands are:
 *
 * $PART: "reference"                  Put cursor on symbol.
 * $PART: "reference" $REF: "ref"      Put cursor on symbol reference.
 * $PART: "reference" $VAL: "value"    Put cursor on symbol value.
 * $PART: "reference" $PAD: "pin name" Put cursor on the symbol pin.
 * $NET: "netname"                     Highlight a specified net
 * $CLEAR: "HIGHLIGHTED"               Clear symbols highlight
 *
 * $CONFIG     Show the Manage Symbol Libraries dialog
 * $ERC        Show the ERC dialog
 */
void SCH_EDIT_FRAME::ExecuteRemoteCommand( const char* cmdline )
{
    // A remote command can arrive over the cross-probe socket before tools are registered
    // or after the tool manager has been torn down while the frame is closing.
    if( !m_toolManager )
        return;

    SCH_EDITOR_CONTROL* editor = m_toolManager->GetTool<SCH_EDITOR_CONTROL>();

    if( !editor )
        return;

    char line[1024];

    strncpy( line, cmdline, sizeof( line ) - 1 );
    line[ sizeof( line ) - 1 ] = '\0';

    char* idcmd = strtok( line, " \n\r" );
    char* text  = strtok( nullptr, "\"\n\r" );

    if( idcmd == nullptr )
        return;

    CROSS_PROBING_SETTINGS& crossProbingSettings = eeconfig()->m_CrossProbing;

    if( strcmp( idcmd, "$CONFIG" ) == 0 )
    {
        GetToolManager()->RunAction( ACTIONS::showSymbolLibTable );
        return;
    }
    else if( strcmp( idcmd, "$ERC" ) == 0 )
    {
        GetToolManager()->RunAction( SCH_ACTIONS::runERC );
        return;
    }
    else if( strcmp( idcmd, "$NET:" ) == 0 )
    {
        if( !crossProbingSettings.auto_highlight )
            return;

        wxString netName = From_UTF8( text );

        if( auto sg = Schematic().ConnectionGraph()->FindFirstSubgraphByName( netName ) )
            m_highlightedConn = sg->GetDriverConnection()->Name();
        else
            m_highlightedConn = wxEmptyString;

        // If the incoming net belongs to a net chain, also turn on chain
        // highlight so the schematic mirrors what the PCB editor is doing.
        if( CONNECTION_GRAPH* graph = Schematic().ConnectionGraph() )
        {
            if( SCH_NETCHAIN* chain = graph->GetNetChainForNet( m_highlightedConn ) )
                SetHighlightedNetChain( chain->GetName() );
            else
                SetHighlightedNetChain( wxEmptyString );
        }

        GetToolManager()->RunAction( SCH_ACTIONS::updateNetHighlighting );
        RefreshNetNavigator();

        SetStatusText( _( "Highlighted net:" ) + wxS( " " ) + UnescapeString( netName ) );
        return;
    }
    else if( strcmp( idcmd, "$CLEAR:" ) == 0 )
    {
        // Cross-probing is now done through selection so we no longer need a clear command
        return;
    }

    if( !crossProbingSettings.on_selection )
        return;

    if( text == nullptr )
        return;

    if( strcmp( idcmd, "$PART:" ) != 0 )
        return;

    wxString part_ref = From_UTF8( text );

    /* look for a complement */
    idcmd = strtok( nullptr, " \n\r" );

    if( idcmd == nullptr )    // Highlight symbol only (from CvPcb or Pcbnew)
    {
        // Highlight symbol part_ref, or clear Highlight, if part_ref is not existing
        editor->FindSymbolAndItem( nullptr, &part_ref, true, HIGHLIGHT_SYMBOL, wxEmptyString );
        return;
    }

    text = strtok( nullptr, "\"\n\r" );

    if( text == nullptr )
        return;

    wxString msg = From_UTF8( text );

    if( strcmp( idcmd, "$REF:" ) == 0 )
    {
        // Highlighting the reference itself isn't actually that useful, and it's harder to
        // see.  Highlight the parent and display the message.
        editor->FindSymbolAndItem( nullptr, &part_ref, true, HIGHLIGHT_SYMBOL, msg );
    }
    else if( strcmp( idcmd, "$VAL:" ) == 0 )
    {
        // Highlighting the value itself isn't actually that useful, and it's harder to see.
        // Highlight the parent and display the message.
        editor->FindSymbolAndItem( nullptr, &part_ref, true, HIGHLIGHT_SYMBOL, msg );
    }
    else if( strcmp( idcmd, "$PAD:" ) == 0 )
    {
        editor->FindSymbolAndItem( nullptr, &part_ref, true, HIGHLIGHT_PIN, msg );
    }
    else
    {
        editor->FindSymbolAndItem( nullptr, &part_ref, true, HIGHLIGHT_SYMBOL, wxEmptyString );
    }
}


void SCH_EDIT_FRAME::HandleRemoteNetHighlight( const wxString& aNetName )
{
    if( auto sg = Schematic().ConnectionGraph()->FindFirstSubgraphByName( aNetName ) )
        m_highlightedConn = sg->GetDriverConnection()->Name();
    else
        m_highlightedConn = wxEmptyString;

    // If the incoming net belongs to a net chain, also turn on chain
    // highlight so the schematic mirrors what the PCB editor is doing.
    if( CONNECTION_GRAPH* graph = Schematic().ConnectionGraph() )
    {
        if( SCH_NETCHAIN* chain = graph->GetNetChainForNet( m_highlightedConn ) )
            SetHighlightedNetChain( chain->GetName() );
        else
            SetHighlightedNetChain( wxEmptyString );
    }

    GetToolManager()->RunAction( SCH_ACTIONS::updateNetHighlighting );
    RefreshNetNavigator();

    SetStatusText( _( "Highlighted net:" ) + wxS( " " ) + UnescapeString( aNetName ) );
}


void SCH_EDIT_FRAME::SendSelectItemsToPcb( const std::vector<EDA_ITEM*>& aItems, bool aForce )
{
    kiapi::common::commands::SyncSelection sync;

    for( EDA_ITEM* item : aItems )
    {
        switch( item->Type() )
        {
        case SCH_SYMBOL_T:
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            kiapi::common::commands::SelectionSpec* spec = sync.add_items();
            spec->mutable_footprint()->set_reference( symbol->GetField( FIELD_T::REFERENCE )->GetText().ToUTF8() );
            break;
        }

        case SCH_SHEET_T:
        {
            // For cross probing, we need the full path of the sheet, because
            // we search by the footprint path prefix in the PCB editor.
            KIID_PATH path = GetCurrentSheet().Path();
            path.push_back( item->m_Uuid );

            kiapi::common::commands::SelectionSpec* spec = sync.add_items();
            kiapi::common::PackSheetPath( *spec->mutable_sheet_path(), path );
            break;
        }

        case SCH_PIN_T:
        {
            SCH_PIN* pin = static_cast<SCH_PIN*>( item );
            SYMBOL*  symbol = pin->GetParentSymbol();
            wxString ref = symbol->GetRef( &GetCurrentSheet(), false );

            // Highlight the resolved pad(s) in pcbnew (issue #2282); a mapped pin may target more
            // than one pad via stacked notation, so highlight all of them.
            wxString effective = pin->GetEffectivePadNumber( GetCurrentSheet(), Schematic().GetCurrentVariant() );

            for( const wxString& pad : ExpandStackedPinNotation( effective ) )
            {
                kiapi::common::commands::SelectionSpec* spec = sync.add_items();
                spec->mutable_pad()->set_reference( ref.ToUTF8() );
                spec->mutable_pad()->set_number( pad.ToUTF8() );
            }

            break;
        }

        default:
            break;
        }
    }

    if( sync.items_size() == 0 )
        return;

    sync.set_context( aForce ? commands::SyncSelectionContext::SSC_EXPLICIT
                             : commands::SyncSelectionContext::SSC_IMPLICIT );

    if( Kiface().IsSingle() )
    {
        CROSS_PROBE_CLIENT::SendToFrame( FRAME_PCB_EDITOR, sync );
    }
    else
    {
        std::string payload;
        kiapi::common::PackKiwayApiMessage( sync, payload );
        Kiway().ExpressMail( FRAME_PCB_EDITOR, MAIL_SELECTION, payload, this );
    }
}


void SCH_EDIT_FRAME::SendCrossProbeNetName( const wxString& aNetName )
{
    kiapi::common::commands::HighlightNets message;

    message.add_net_name( aNetName.ToUTF8() );

    if( Kiface().IsSingle() )
    {
        CROSS_PROBE_CLIENT::SendToFrame( FRAME_PCB_EDITOR, message );
    }
    else
    {
        std::string payload;
        kiapi::common::PackKiwayApiMessage( message, payload );
        Kiway().ExpressMail( FRAME_PCB_EDITOR, MAIL_CROSS_PROBE, payload, this );
    }
}


void SCH_EDIT_FRAME::SetCrossProbeConnection( const SCH_CONNECTION* aConnection )
{
    if( !aConnection )
    {
        SendCrossProbeClearHighlight();
        return;
    }

    if( aConnection->IsNet() )
    {
        SendCrossProbeNetName( aConnection->Name() );
        return;
    }

    if( aConnection->Members().empty() )
        return;

    kiapi::common::commands::HighlightNets message;

    auto all_members = aConnection->AllMembers();

    if( all_members.size() == 1 )
    {
        SendCrossProbeNetName( all_members[0]->Name() );
        return;
    }

    message.add_net_name( all_members[0]->Name().ToUTF8() );

    // TODO: This could be replaced by just sending the bus name once we have bus contents
    // included as part of the netlist sent from Eeschema to Pcbnew (and thus Pcbnew can
    // natively keep track of bus membership)

    for( size_t i = 1; i < all_members.size(); i++ )
        message.add_net_name( all_members[i]->Name().ToUTF8() );

    if( Kiface().IsSingle() )
    {
        CROSS_PROBE_CLIENT::SendToFrame( FRAME_PCB_EDITOR, message );
    }
    else
    {
        std::string data = message.SerializeAsString();
        Kiway().ExpressMail( FRAME_PCB_EDITOR, MAIL_CROSS_PROBE, data, this );
    }
}


void SCH_EDIT_FRAME::SendCrossProbeClearHighlight()
{
    SendCrossProbeNetName( wxEmptyString );
}


void SCH_EDIT_FRAME::KiwayMailIn( KIWAY_MAIL_EVENT& mail )
{
    std::string& payload = mail.GetPayload();

    switch( mail.Command() )
    {
    case MAIL_ADD_LOCAL_LIB:
    {
        std::stringstream ss( payload );
        std::string       file;

        LIBRARY_MANAGER&              manager = Pgm().GetLibraryManager();
        SYMBOL_LIBRARY_ADAPTER*       adapter = PROJECT_SCH::SymbolLibAdapter( &Prj() );
        std::optional<LIBRARY_TABLE*> optTable = manager.Table( LIBRARY_TABLE_TYPE::SYMBOL,
                                                                LIBRARY_TABLE_SCOPE::PROJECT );

        wxCHECK_RET( optTable.has_value(), "Could not load symbol lib table." );
        LIBRARY_TABLE* table = optTable.value();

        wxString projectPath = Prj().GetProjectPath();

        // First line of payload is the source project directory.
        std::string srcProjDir;
        std::getline( ss, srcProjDir, '\n' );

        std::vector<wxString> toLoad;

        while( std::getline( ss, file, '\n' ) )
        {
            if( file.empty() )
                continue;

            wxFileName             fn( file );
            IO_RELEASER<SCH_IO>    pi;
            SCH_IO_MGR::SCH_FILE_T type = SCH_IO_MGR::GuessPluginTypeFromLibPath( fn.GetFullPath() );

            if( type == SCH_IO_MGR::SCH_FILE_UNKNOWN )
            {
                wxLogTrace( "KIWAY", "Unknown file type: %s", fn.GetFullPath() );
                continue;
            }

            pi.reset( SCH_IO_MGR::FindPlugin( type ) );

            wxString libTableUri;
            bool     isProjectLocal = fn.GetFullPath().StartsWith( wxString( srcProjDir ) );

            if( isProjectLocal )
            {
                // Project-local library: copy into the KiCad project directory and use a
                // project-relative path so the sym-lib-table stays portable.
                if( !fn.FileExists() )
                    continue;

                wxFileName projectFn( projectPath, fn.GetFullName() );

                if( fn.GetFullPath() != projectFn.GetFullPath() && !projectFn.FileExists() )
                    wxCopyFile( fn.GetFullPath(), projectFn.GetFullPath() );

                libTableUri = wxS( "${KIPRJMOD}/" ) + fn.GetFullName();
            }
            else
            {
                // External library referenced by absolute path. Preserve the original path.
                libTableUri = fn.GetFullPath();
            }

            if( !table->HasRow( fn.GetName() ) )
            {
                LIBRARY_TABLE_ROW& row = table->InsertRow();
                row.SetNickname( fn.GetName() );
                row.SetURI( libTableUri );
                row.SetType( SCH_IO_MGR::ShowType( type ) );
                toLoad.emplace_back( fn.GetName() );
            }
        }

        if( !toLoad.empty() )
        {
            bool success = true;

            table->Save().map_error(
                        [&]( const LIBRARY_ERROR& aError )
                        {
                            wxLogError( wxT( "Error saving project library table:\n\n" ) + aError.message );
                            success = false;
                        } );

            if( success )
            {
                manager.AbortAsyncLoads();
                manager.LoadProjectTables( { LIBRARY_TABLE_TYPE::SYMBOL } );

                std::ranges::for_each( toLoad,
                                       [adapter]( const wxString& aNick )
                                       {
                                           adapter->LoadOne( aNick );
                                       } );
            }
        }

        Kiway().ExpressMail( FRAME_CVPCB, MAIL_RELOAD_LIB, payload );
        Kiway().ExpressMail( FRAME_SCH_SYMBOL_EDITOR, MAIL_RELOAD_LIB, payload );
        Kiway().ExpressMail( FRAME_SCH_VIEWER, MAIL_RELOAD_LIB, payload );

        break;
    }

    // Handled as API commands
    case MAIL_SELECTION:
    case MAIL_CROSS_PROBE:
        if( ApiRequest request; request.ParseFromString( payload.c_str() ) )
            m_apiHandler->Handle( request );

        break;

    case MAIL_SCH_GET_NETLIST:
    {
        if( !payload.empty() )
        {
            wxString annotationMessage( payload );

            // Ensure schematic is OK for netlist creation (especially that it is fully annotated):
            bool userCancelled = false;

            if( !ReadyToNetlist( annotationMessage, &userCancelled ) )
            {
                // Cancel replies with a sentinel so the caller aborts silently; an unannotated
                // schematic echoes the annotation message so the caller shows its own error
                if( userCancelled )
                    payload = MAIL_SCH_GET_NETLIST_CANCELLED;

                return;
            }
        }

        if( ADVANCED_CFG::GetCfg().m_IncrementalConnectivity )
            RecalculateConnections( nullptr, GLOBAL_CLEANUP );

        NETLIST_EXPORTER_KICAD exporter( &Schematic() );
        STRING_FORMATTER formatter;

        exporter.SetKiway( &Kiway() );
        exporter.Format( &formatter, GNL_ALL | GNL_OPT_KICAD );

        payload = formatter.GetString();
        break;
    }

    case MAIL_SCH_GET_ITEM:
    {
        KIID           uuid( payload );
        SCH_SHEET_PATH path;

        if( SCH_ITEM* item = m_schematic->ResolveItem( uuid, &path, true ) )
        {
            if( item->Type() == SCH_SHEET_T )
                payload = static_cast<SCH_SHEET*>( item )->GetShownName( false );
            else if( item->Type() == SCH_SYMBOL_T )
                payload = static_cast<SCH_SYMBOL*>( item )->GetRef( &path, true );
            else
                payload = item->GetFriendlyName();
        }

        break;
    }

    case MAIL_ASSIGN_FOOTPRINTS:
        try
        {
            SCH_EDITOR_CONTROL* controlTool = m_toolManager->GetTool<SCH_EDITOR_CONTROL>();
            controlTool->AssignFootprints( payload );
        }
        catch( const IO_ERROR& )
        {
        }
        break;

    case MAIL_SCH_REFRESH:
    {
        TestDanglingEnds();

        GetCanvas()->GetView()->UpdateAllItems( KIGFX::ALL );
        GetCanvas()->Refresh();
        break;
    }

    case MAIL_IMPORT_FILE:
    {
        // Extract file format type and path (plugin type, path and properties keys, values
        // separated with \n)
        std::stringstream ss( payload );
        char              delim = '\n';

        std::string formatStr;
        wxCHECK( std::getline( ss, formatStr, delim ), /* void */ );

        std::string fnameStr;
        wxCHECK( std::getline( ss, fnameStr, delim ), /* void */ );

        int importFormat;

        try
        {
            importFormat = std::stoi( formatStr );
        }
        catch( std::invalid_argument& )
        {
            wxFAIL;
            importFormat = -1;
        }

        std::map<std::string, UTF8> props;

        do
        {
            std::string key, value;

            if( !std::getline( ss, key, delim ) )
                break;

            std::getline( ss, value, delim ); // We may want an empty string as value

            props.emplace( key, value );

        } while( true );

        if( importFormat >= 0 )
            importFile( fnameStr, importFormat, props.empty() ? nullptr : &props );

        break;
    }

    case MAIL_SCH_SAVE:
        if( SaveProject() )
            payload = "success";

        break;

    case MAIL_SCH_UPDATE:
        m_toolManager->RunAction( ACTIONS::updateSchematicFromPcb );
        break;

    case MAIL_RELOAD_LIB:
    {
        if( m_designBlocksPane && m_designBlocksPane->IsShown() )
        {
            m_designBlocksPane->RefreshLibs();
            SyncView();
        }

        // Show any symbol library load errors in the status bar
        if( KISTATUSBAR* statusBar = dynamic_cast<KISTATUSBAR*>( GetStatusBar() ) )
        {
            SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( &Prj() );
            wxString errors = adapter->GetLibraryLoadErrors();

            if( !errors.IsEmpty() )
                statusBar->AddWarningMessages( "load", errors );
        }

        break;
    }

    case MAIL_SCH_NAVIGATE_TO_SHEET:
    {
        wxString targetFile( payload );

        for( SCH_SHEET_PATH& sheetPath : m_schematic->Hierarchy() )
        {
            SCH_SCREEN* screen = sheetPath.LastScreen();

            if( screen && screen->GetFileName() == targetFile )
            {
                m_toolManager->RunAction<SCH_SHEET_PATH*>( SCH_ACTIONS::changeSheet, &sheetPath );
                payload = "success";
                Raise();
                return;
            }
        }

        payload.clear();
        break;
    }

    default:;

    }
}
