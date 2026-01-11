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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <fmt.h>
#include <kiface_base.h>
#include <kiway.h>
#include <kiway_express.h>
#include <eda_dde.h>
#include <connection_graph.h>
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

#include <pgm_base.h>
#include <libraries/symbol_library_adapter.h>
#include <widgets/sch_design_block_pane.h>
#include <widgets/kistatusbar.h>
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
    SCH_EDITOR_CONTROL* editor = m_toolManager->GetTool<SCH_EDITOR_CONTROL>();
    char                line[1024];

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


void SCH_EDIT_FRAME::SendSelectItemsToPcb( const std::vector<EDA_ITEM*>& aItems, bool aForce )
{
    std::vector<wxString> parts;

    for( EDA_ITEM* item : aItems )
    {
        switch( item->Type() )
        {
        case SCH_SYMBOL_T:
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            wxString    ref = symbol->GetField( FIELD_T::REFERENCE )->GetText();

            parts.push_back( wxT( "F" ) + EscapeString( ref, CTX_IPC ) );
            break;
        }

        case SCH_SHEET_T:
        {
            // For cross probing, we need the full path of the sheet, because
            // we search by the footprint path prefix in the PCB editor
            wxString full_path = GetCurrentSheet().PathAsString() + item->m_Uuid.AsString();

            parts.push_back( wxT( "S" ) + full_path );
            break;
        }

        case SCH_PIN_T:
        {
            SCH_PIN* pin = static_cast<SCH_PIN*>( item );
            SYMBOL*  symbol = pin->GetParentSymbol();
            wxString ref = symbol->GetRef( &GetCurrentSheet(), false );

            parts.push_back( wxT( "P" ) + EscapeString( ref, CTX_IPC ) + wxT( "/" )
                             + EscapeString( pin->GetShownNumber(), CTX_IPC ) );
            break;
        }

        default:
            break;
        }
    }

    if( parts.empty() )
        return;

    std::string command = "$SELECT: 0,";

    for( wxString part : parts )
    {
        command += part;
        command += ",";
    }

    command.pop_back();

    if( Kiface().IsSingle() )
    {
        SendCommand( MSG_TO_PCB, command );
    }
    else
    {
        // Typically ExpressMail is going to be s-expression packets, but since
        // we have existing interpreter of the selection packet on the other
        // side in place, we use that here.
        Kiway().ExpressMail( FRAME_PCB_EDITOR, aForce ? MAIL_SELECTION_FORCE : MAIL_SELECTION,
                             command, this );
    }
}


void SCH_EDIT_FRAME::SendCrossProbeNetName( const wxString& aNetName )
{
    // The command is a keyword followed by a quoted string.

    std::string packet = fmt::format( "$NET: \"{}\"", TO_UTF8( aNetName ) );

    if( !packet.empty() )
    {
        if( Kiface().IsSingle() )
        {
            SendCommand( MSG_TO_PCB, packet );
        }
        else
        {
            // Typically ExpressMail is going to be s-expression packets, but since
            // we have existing interpreter of the cross probe packet on the other
            // side in place, we use that here.
            Kiway().ExpressMail( FRAME_PCB_EDITOR, MAIL_CROSS_PROBE, packet, this );
        }
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

    auto all_members = aConnection->AllMembers();

    wxString nets = all_members[0]->Name();

    if( all_members.size() == 1 )
    {
        SendCrossProbeNetName( nets );
        return;
    }

    // TODO: This could be replaced by just sending the bus name once we have bus contents
    // included as part of the netlist sent from Eeschema to Pcbnew (and thus Pcbnew can
    // natively keep track of bus membership)

    for( size_t i = 1; i < all_members.size(); i++ )
        nets << "," << all_members[i]->Name();

    std::string packet = fmt::format( "$NETS: \"{}\"", TO_UTF8( nets ) );

    if( !packet.empty() )
    {
        if( Kiface().IsSingle() )
            SendCommand( MSG_TO_PCB, packet );
        else
        {
            // Typically ExpressMail is going to be s-expression packets, but since
            // we have existing interpreter of the cross probe packet on the other
            // side in place, we use that here.
            Kiway().ExpressMail( FRAME_PCB_EDITOR, MAIL_CROSS_PROBE, packet, this );
        }
    }
}


void SCH_EDIT_FRAME::SendCrossProbeClearHighlight()
{
    std::string packet = "$CLEAR\n";

    if( Kiface().IsSingle() )
    {
        SendCommand( MSG_TO_PCB, packet );
    }
    else
    {
        // Typically ExpressMail is going to be s-expression packets, but since
        // we have existing interpreter of the cross probe packet on the other
        // side in place, we use that here.
        Kiway().ExpressMail( FRAME_PCB_EDITOR, MAIL_CROSS_PROBE, packet, this );
    }
}


bool findSymbolsAndPins(
        const SCH_SHEET_LIST& aSchematicSheetList, const SCH_SHEET_PATH& aSheetPath,
        std::unordered_map<wxString, std::vector<SCH_REFERENCE>>&             aSyncSymMap,
        std::unordered_map<wxString, std::unordered_map<wxString, SCH_PIN*>>& aSyncPinMap,
        bool                                                                  aRecursive = false )
{
    if( aRecursive )
    {
        // Iterate over children
        for( const SCH_SHEET_PATH& candidate : aSchematicSheetList )
        {
            if( candidate == aSheetPath || !candidate.IsContainedWithin( aSheetPath ) )
                continue;

            findSymbolsAndPins( aSchematicSheetList, candidate, aSyncSymMap, aSyncPinMap,
                                aRecursive );
        }
    }

    SCH_REFERENCE_LIST references;

    aSheetPath.GetSymbols( references, false, true );

    for( unsigned ii = 0; ii < references.GetCount(); ii++ )
    {
        SCH_REFERENCE& schRef = references[ii];

        if( schRef.IsSplitNeeded() )
            schRef.Split();

        SCH_SYMBOL* symbol = schRef.GetSymbol();
        wxString    refNum = schRef.GetRefNumber();
        wxString    fullRef = schRef.GetRef() + refNum;

        // Skip power symbols
        if( fullRef.StartsWith( wxS( "#" ) ) )
            continue;

        // Unannotated symbols are not supported
        if( refNum.compare( wxS( "?" ) ) == 0 )
            continue;

        // Look for whole footprint
        auto symMatchIt = aSyncSymMap.find( fullRef );

        if( symMatchIt != aSyncSymMap.end() )
        {
            symMatchIt->second.emplace_back( schRef );

            // Whole footprint was selected, no need to select pins
            continue;
        }

        // Look for pins
        auto symPinMatchIt = aSyncPinMap.find( fullRef );

        if( symPinMatchIt != aSyncPinMap.end() )
        {
            std::unordered_map<wxString, SCH_PIN*>& pinMap = symPinMatchIt->second;
            std::vector<SCH_PIN*>                   pinsOnSheet = symbol->GetPins( &aSheetPath );

            for( SCH_PIN* pin : pinsOnSheet )
            {
                int pinUnit = pin->GetLibPin()->GetUnit();

                if( pinUnit > 0 && pinUnit != schRef.GetUnit() )
                    continue;

                auto pinIt = pinMap.find( pin->GetNumber() );

                if( pinIt != pinMap.end() )
                    pinIt->second = pin;
            }
        }
    }

    return false;
}


bool sheetContainsOnlyWantedItems(
        const SCH_SHEET_LIST& aSchematicSheetList, const SCH_SHEET_PATH& aSheetPath,
        std::unordered_map<wxString, std::vector<SCH_REFERENCE>>&             aSyncSymMap,
        std::unordered_map<wxString, std::unordered_map<wxString, SCH_PIN*>>& aSyncPinMap,
        std::unordered_map<SCH_SHEET_PATH, bool>&                             aCache )
{
    auto cacheIt = aCache.find( aSheetPath );

    if( cacheIt != aCache.end() )
        return cacheIt->second;

    // Iterate over children
    for( const SCH_SHEET_PATH& candidate : aSchematicSheetList )
    {
        if( candidate == aSheetPath || !candidate.IsContainedWithin( aSheetPath ) )
            continue;

        bool childRet = sheetContainsOnlyWantedItems( aSchematicSheetList, candidate, aSyncSymMap,
                                                      aSyncPinMap, aCache );

        if( !childRet )
        {
            aCache.emplace( aSheetPath, false );
            return false;
        }
    }

    SCH_REFERENCE_LIST references;
    aSheetPath.GetSymbols( references, false, true );

    if( references.GetCount() == 0 )    // Empty sheet, obviously do not contain wanted items
    {
        aCache.emplace( aSheetPath, false );
        return false;
    }

    for( unsigned ii = 0; ii < references.GetCount(); ii++ )
    {
        SCH_REFERENCE& schRef = references[ii];

        if( schRef.IsSplitNeeded() )
            schRef.Split();

        wxString refNum = schRef.GetRefNumber();
        wxString fullRef = schRef.GetRef() + refNum;

        // Skip power symbols
        if( fullRef.StartsWith( wxS( "#" ) ) )
            continue;

        // Unannotated symbols are not supported
        if( refNum.compare( wxS( "?" ) ) == 0 )
            continue;

        if( aSyncSymMap.find( fullRef ) == aSyncSymMap.end() )
        {
            aCache.emplace( aSheetPath, false );
            return false; // Some symbol is not wanted.
        }

        if( aSyncPinMap.find( fullRef ) != aSyncPinMap.end() )
        {
            aCache.emplace( aSheetPath, false );
            return false; // Looking for specific pins, so can't be mapped
        }
    }

    aCache.emplace( aSheetPath, true );
    return true;
}


std::optional<std::tuple<SCH_SHEET_PATH, SCH_ITEM*, std::vector<SCH_ITEM*>>>
findItemsFromSyncSelection( const SCHEMATIC& aSchematic, const std::string aSyncStr,
                            bool aFocusOnFirst )
{
    wxArrayString syncArray = wxStringTokenize( aSyncStr, wxS( "," ) );

    std::unordered_map<wxString, std::vector<SCH_REFERENCE>>             syncSymMap;
    std::unordered_map<wxString, std::unordered_map<wxString, SCH_PIN*>> syncPinMap;
    std::unordered_map<SCH_SHEET_PATH, double>                           symScores;
    std::unordered_map<SCH_SHEET_PATH, bool>                             fullyWantedCache;

    std::optional<wxString>                                    focusSymbol;
    std::optional<std::pair<wxString, wxString>>               focusPin;
    std::unordered_map<SCH_SHEET_PATH, std::vector<SCH_ITEM*>> focusItemResults;

    const SCH_SHEET_LIST allSheetsList = aSchematic.Hierarchy();

    // In orderedSheets, the current sheet comes first.
    std::vector<SCH_SHEET_PATH> orderedSheets;
    orderedSheets.reserve( allSheetsList.size() );
    orderedSheets.push_back( aSchematic.CurrentSheet() );

    for( const SCH_SHEET_PATH& sheetPath : allSheetsList )
    {
        if( sheetPath != aSchematic.CurrentSheet() )
            orderedSheets.push_back( sheetPath );
    }

    // Init sync maps from the sync string
    for( size_t i = 0; i < syncArray.size(); i++ )
    {
        wxString syncEntry = syncArray[i];

        if( syncEntry.empty() )
            continue;

        wxString syncData = syncEntry.substr( 1 );

        switch( syncEntry.GetChar( 0 ).GetValue() )
        {
        case 'F': // Select by footprint: F<Reference>
        {
            wxString symRef = UnescapeString( syncData );

            if( aFocusOnFirst && ( i == 0 ) )
                focusSymbol = symRef;

            syncSymMap[symRef] = std::vector<SCH_REFERENCE>();
            break;
        }

        case 'P': // Select by pad: P<Footprint reference>/<Pad number>
        {
            wxString symRef = UnescapeString( syncData.BeforeFirst( '/' ) );
            wxString padNum = UnescapeString( syncData.AfterFirst( '/' ) );

            if( aFocusOnFirst && ( i == 0 ) )
                focusPin = std::make_pair( symRef, padNum );

            syncPinMap[symRef][padNum] = nullptr;
            break;
        }

        default:
            break;
        }
    }

    // Lambda definitions
    auto flattenSyncMaps =
            [&syncSymMap, &syncPinMap]() -> std::vector<SCH_ITEM*>
            {
                std::vector<SCH_ITEM*> allVec;

                for( const auto& [symRef, symbols] : syncSymMap )
                {
                    for( const SCH_REFERENCE& ref : symbols )
                        allVec.push_back( ref.GetSymbol() );
                }

                for( const auto& [symRef, pinMap] : syncPinMap )
                {
                    for( const auto& [padNum, pin] : pinMap )
                    {
                        if( pin )
                            allVec.push_back( pin );
                    }
                }

                return allVec;
            };

    auto clearSyncMaps =
            [&syncSymMap, &syncPinMap]()
            {
                for( auto& [symRef, symbols] : syncSymMap )
                    symbols.clear();

                for( auto& [reference, pins] : syncPinMap )
                {
                    for( auto& [number, pin] : pins )
                        pin = nullptr;
                }
            };

    auto syncMapsValuesEmpty =
            [&syncSymMap, &syncPinMap]() -> bool
            {
                for( const auto& [symRef, symbols] : syncSymMap )
                {
                    if( symbols.size() > 0 )
                        return false;
                }

                for( const auto& [symRef, pins] : syncPinMap )
                {
                    for( const auto& [padNum, pin] : pins )
                    {
                        if( pin )
                            return false;
                    }
                }

                return true;
            };

    auto checkFocusItems =
            [&]( const SCH_SHEET_PATH& aSheet )
            {
                if( focusSymbol )
                {
                    auto findIt = syncSymMap.find( *focusSymbol );

                    if( findIt != syncSymMap.end() )
                    {
                        if( findIt->second.size() > 0 )
                            focusItemResults[aSheet].push_back( findIt->second.front().GetSymbol() );
                    }
                }
                else if( focusPin )
                {
                    auto findIt = syncPinMap.find( focusPin->first );

                    if( findIt != syncPinMap.end() )
                    {
                        if( findIt->second[focusPin->second] )
                            focusItemResults[aSheet].push_back( findIt->second[focusPin->second] );
                    }
                }
            };

    auto makeRetForSheet =
            [&]( const SCH_SHEET_PATH& aSheet, SCH_ITEM* aFocusItem )
            {
                clearSyncMaps();

                // Fill sync maps
                findSymbolsAndPins( allSheetsList, aSheet, syncSymMap, syncPinMap );
                std::vector<SCH_ITEM*> itemsVector = flattenSyncMaps();

                // Add fully wanted sheets to vector
                for( SCH_ITEM* item : aSheet.LastScreen()->Items().OfType( SCH_SHEET_T ) )
                {
                    KIID_PATH kiidPath = aSheet.Path();
                    kiidPath.push_back( item->m_Uuid );

                    std::optional<SCH_SHEET_PATH> subsheetPath =
                            allSheetsList.GetSheetPathByKIIDPath( kiidPath );

                    if( !subsheetPath )
                        continue;

                    if( sheetContainsOnlyWantedItems( allSheetsList, *subsheetPath, syncSymMap,
                                                      syncPinMap, fullyWantedCache ) )
                    {
                        itemsVector.push_back( item );
                    }
                }

                return std::make_tuple( aSheet, aFocusItem, itemsVector );
            };

    if( aFocusOnFirst )
    {
        for( const SCH_SHEET_PATH& sheetPath : orderedSheets )
        {
            clearSyncMaps();

            findSymbolsAndPins( allSheetsList, sheetPath, syncSymMap, syncPinMap );

            checkFocusItems( sheetPath );
        }

        if( focusItemResults.size() > 0 )
        {
            for( const SCH_SHEET_PATH& sheetPath : orderedSheets )
            {
                const std::vector<SCH_ITEM*>& items = focusItemResults[sheetPath];

                if( !items.empty() )
                    return makeRetForSheet( sheetPath, items.front() );
            }
        }
    }
    else
    {
        for( const SCH_SHEET_PATH& sheetPath : orderedSheets )
        {
            clearSyncMaps();

            findSymbolsAndPins( allSheetsList, sheetPath, syncSymMap, syncPinMap );

            if( !syncMapsValuesEmpty() )
            {
                // Something found on sheet
                return makeRetForSheet( sheetPath, nullptr );
            }
        }
    }

    return std::nullopt;
}


void SCH_EDIT_FRAME::KiwayMailIn( KIWAY_EXPRESS& mail )
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

        while( std::getline( ss, file, '\n' ) )
        {
            if( file.empty() )
                continue;

            wxFileName             fn( file );
            IO_RELEASER<SCH_IO>    pi;
            SCH_IO_MGR::SCH_FILE_T type = SCH_IO_MGR::GuessPluginTypeFromLibPath( fn.GetFullPath() );
            bool                   success = true;

            if( type == SCH_IO_MGR::SCH_FILE_UNKNOWN )
            {
                wxLogTrace( "KIWAY", "Unknown file type: %s", fn.GetFullPath() );
                continue;
            }

            pi.reset( SCH_IO_MGR::FindPlugin( type ) );

            if( !table->HasRow( fn.GetName() ) )
            {
                LIBRARY_TABLE_ROW& row = table->InsertRow();
                row.SetNickname( fn.GetName() );
                row.SetURI( fn.GetFullPath() );
                row.SetType( SCH_IO_MGR::ShowType( type ) );

                table->Save().map_error(
                        [&]( const LIBRARY_ERROR& aError )
                        {
                            wxLogError( wxT( "Error saving project library table:\n\n" ) + aError.message );
                            success = false;
                        } );

                if( success )
                {
                    manager.LoadProjectTables( { LIBRARY_TABLE_TYPE::SYMBOL } );
                    adapter->LoadOne( fn.GetName() );
                }
            }
        }

        Kiway().ExpressMail( FRAME_CVPCB, MAIL_RELOAD_LIB, payload );
        Kiway().ExpressMail( FRAME_SCH_SYMBOL_EDITOR, MAIL_RELOAD_LIB, payload );
        Kiway().ExpressMail( FRAME_SCH_VIEWER, MAIL_RELOAD_LIB, payload );

        break;
    }

    case MAIL_CROSS_PROBE:
        ExecuteRemoteCommand( payload.c_str() );
        break;

    case MAIL_SELECTION:
        if( !eeconfig()->m_CrossProbing.on_selection )
            break;

        KI_FALLTHROUGH;

    case MAIL_SELECTION_FORCE:
    {
        // $SELECT: 0,<spec1>,<spec2>,<spec3>
        // Try to select specified items.

        // $SELECT: 1,<spec1>,<spec2>,<spec3>
        // Select and focus on <spec1> item, select other specified items that are on the
        // same sheet.

        std::string prefix = "$SELECT: ";

        std::string paramStr = payload.substr( prefix.size() );

        // Empty/broken command: we need at least 2 chars for sync string.
        if( paramStr.size() < 2 )
            break;

        std::string syncStr = paramStr.substr( 2 );

        bool focusOnFirst = ( paramStr[0] == '1' );

        std::optional<std::tuple<SCH_SHEET_PATH, SCH_ITEM*, std::vector<SCH_ITEM*>>> findRet =
                findItemsFromSyncSelection( Schematic(), syncStr, focusOnFirst );

        if( findRet )
        {
            auto& [sheetPath, focusItem, items] = *findRet;

            m_syncingPcbToSchSelection = true; // recursion guard

            GetToolManager()->GetTool<SCH_SELECTION_TOOL>()->SyncSelection( sheetPath, focusItem,
                                                                            items );

            m_syncingPcbToSchSelection = false;

            if( eeconfig()->m_CrossProbing.flash_selection )
            {
                wxLogTrace( traceCrossProbeFlash, "MAIL_SELECTION(_FORCE): flash enabled, items=%zu", items.size() );
                if( items.empty() )
                {
                    wxLogTrace( traceCrossProbeFlash, "MAIL_SELECTION(_FORCE): nothing to flash" );
                }
                else
                {
                    std::vector<SCH_ITEM*> itemPtrs;
                    std::copy( items.begin(), items.end(), std::back_inserter( itemPtrs ) );

                    StartCrossProbeFlash( itemPtrs );
                }
            }
            else
            {
                wxLogTrace( traceCrossProbeFlash, "MAIL_SELECTION(_FORCE): flash disabled" );
            }
        }

        break;
    }

    case MAIL_SCH_GET_NETLIST:
    {
        if( !payload.empty() )
        {
            wxString annotationMessage( payload );

            // Ensure schematic is OK for netlist creation (especially that it is fully annotated):
            if( !ReadyToNetlist( annotationMessage ) )
                return;
        }

        if( ADVANCED_CFG::GetCfg().m_IncrementalConnectivity )
            RecalculateConnections( nullptr, GLOBAL_CLEANUP );

        NETLIST_EXPORTER_KICAD exporter( &Schematic() );
        STRING_FORMATTER formatter;

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
                statusBar->SetLoadWarningMessages( errors );
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
