/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <kiface_base.h>
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
#include <tools/ee_actions.h>
#include <tools/sch_editor_control.h>
#include <advanced_config.h>
#include <wx/log.h>

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
        sheetList = m_frame->Schematic().GetSheets();

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
            m_frame->Schematic().SetCurrentSheet( *sheetWithSymbolFound );
            m_frame->DisplayCurrentSheet();
        }

        if( crossProbingSettings.center_on_items )
        {
            if( crossProbingSettings.zoom_to_fit )
            {
                BOX2I bbox = symbol->GetBoundingBox();

                m_toolMgr->GetTool<EE_SELECTION_TOOL>()->ZoomFitCrossProbeBBox( bbox );
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
        GetToolManager()->RunAction( EE_ACTIONS::runERC );
        return;
    }
    else if( strcmp( idcmd, "$NET:" ) == 0 )
    {
        if( !crossProbingSettings.auto_highlight )
            return;

        wxString netName = FROM_UTF8( text );

        if( auto sg = Schematic().ConnectionGraph()->FindFirstSubgraphByName( netName ) )
            m_highlightedConn = sg->GetDriverConnection()->Name();
        else
            m_highlightedConn = wxEmptyString;

        GetToolManager()->RunAction( EE_ACTIONS::updateNetHighlighting );

        SetStatusText( _( "Selected net:" ) + wxS( " " ) + UnescapeString( netName ) );
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

    wxString part_ref = FROM_UTF8( text );

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

    wxString msg = FROM_UTF8( text );

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
            wxString    ref = symbol->GetField( REFERENCE_FIELD )->GetText();

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
            SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
            SCH_SYMBOL* symbol = pin->GetParentSymbol();
            wxString    ref = symbol->GetField( REFERENCE_FIELD )->GetText();

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

    std::string packet = StrPrintf( "$NET: \"%s\"", TO_UTF8( aNetName ) );

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

    std::string packet = StrPrintf( "$NETS: \"%s\"", TO_UTF8( nets ) );

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
        const SCHEMATIC& aSchematic, const SCH_SHEET_PATH& aSheetPath,
        std::unordered_map<wxString, std::vector<SCH_REFERENCE>>&             aSyncSymMap,
        std::unordered_map<wxString, std::unordered_map<wxString, SCH_PIN*>>& aSyncPinMap,
        bool                                                                  aRecursive = false )
{
    if( aRecursive )
    {
        // Iterate over children
        for( const SCH_SHEET_PATH& candidate : aSchematic.GetSheets() )
        {
            if( candidate == aSheetPath || !candidate.IsContainedWithin( aSheetPath ) )
                continue;

            findSymbolsAndPins( aSchematic, candidate, aSyncSymMap, aSyncPinMap, aRecursive );
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
        const SCHEMATIC& aSchematic, const SCH_SHEET_PATH& aSheetPath,
        std::unordered_map<wxString, std::vector<SCH_REFERENCE>>&             aSyncSymMap,
        std::unordered_map<wxString, std::unordered_map<wxString, SCH_PIN*>>& aSyncPinMap,
        std::unordered_map<SCH_SHEET_PATH, bool>&                             aCache )
{
    auto cacheIt = aCache.find( aSheetPath );

    if( cacheIt != aCache.end() )
        return cacheIt->second;

    // Iterate over children
    for( const SCH_SHEET_PATH& candidate : aSchematic.GetSheets() )
    {
        if( candidate == aSheetPath || !candidate.IsContainedWithin( aSheetPath ) )
            continue;

        bool childRet = sheetContainsOnlyWantedItems( aSchematic, candidate, aSyncSymMap,
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

    const SCH_SHEET_LIST allSheetsList = aSchematic.GetSheets();

    // In orderedSheets, the current sheet comes first.
    SCH_SHEET_PATHS orderedSheets;
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
        default: break;
        }
    }

    // Lambda definitions
    auto flattenSyncMaps = [&syncSymMap, &syncPinMap]() -> std::vector<SCH_ITEM*>
    {
        std::vector<SCH_ITEM*> allVec;

        for( auto const& pairSym : syncSymMap )
        {
            for( const SCH_REFERENCE& ref : pairSym.second )
            {
                allVec.push_back( ref.GetSymbol() );
            }
        }

        for( auto const& pairSym : syncPinMap )
        {
            for( auto const& pairPin : pairSym.second )
            {
                if( pairPin.second )
                    allVec.push_back( pairPin.second );
            }
        }

        return allVec;
    };

    auto clearSyncMaps = [&syncSymMap, &syncPinMap]()
    {
        for( auto& pairSym : syncSymMap )
        {
            pairSym.second.clear();
        }

        for( auto& pairSym : syncPinMap )
        {
            for( auto& pairPin : pairSym.second )
            {
                pairPin.second = nullptr;
            }
        }
    };

    auto syncMapsValuesEmpty = [&syncSymMap, &syncPinMap]() -> bool
    {
        for( auto const& pairSym : syncSymMap )
        {
            if( pairSym.second.size() > 0 )
                return false;
        }

        for( auto const& pairSym : syncPinMap )
        {
            for( auto const& pairPin : pairSym.second )
            {
                if( pairPin.second )
                    return false;
            }
        }

        return true;
    };

    auto checkFocusItems = [&]( const SCH_SHEET_PATH& aSheetPath )
    {
        if( focusSymbol )
        {
            auto findIt = syncSymMap.find( *focusSymbol );
            if( findIt != syncSymMap.end() )
            {
                if( findIt->second.size() > 0 )
                {
                    focusItemResults[aSheetPath].push_back( findIt->second.front().GetSymbol() );
                }
            }
        }
        else if( focusPin )
        {
            auto findIt = syncPinMap.find( focusPin->first );
            if( findIt != syncPinMap.end() )
            {
                if( findIt->second[focusPin->second] )
                {
                    focusItemResults[aSheetPath].push_back( findIt->second[focusPin->second] );
                }
            }
        }
    };

    auto makeRetForSheet = [&]( const SCH_SHEET_PATH& aSheet, SCH_ITEM* aFocusItem )
    {
        clearSyncMaps();

        // Fill sync maps
        findSymbolsAndPins( aSchematic, aSheet, syncSymMap, syncPinMap );
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

            if( sheetContainsOnlyWantedItems( aSchematic, *subsheetPath, syncSymMap, syncPinMap,
                                              fullyWantedCache ) )
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

            findSymbolsAndPins( aSchematic, sheetPath, syncSymMap, syncPinMap );

            checkFocusItems( sheetPath );
        }

        if( focusItemResults.size() > 0 )
        {
            for( const SCH_SHEET_PATH& sheetPath : orderedSheets )
            {
                auto vec = focusItemResults[sheetPath];

                if( !vec.empty() )
                    return makeRetForSheet( sheetPath, vec.front() );
            }
        }
    }
    else
    {
        for( const SCH_SHEET_PATH& sheetPath : orderedSheets )
        {
            clearSyncMaps();

            findSymbolsAndPins( aSchematic, sheetPath, syncSymMap, syncPinMap );

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
        // Select and focus on <spec1> item, select other specified items that are on the same sheet.

        std::string prefix = "$SELECT: ";

        std::string paramStr = payload.substr( prefix.size() );

        if( paramStr.size() < 2 )   // Empty/broken command: we need at least 2 chars for sync string.
            break;

        std::string syncStr = paramStr.substr( 2 );

        bool focusOnFirst = ( paramStr[0] == '1' );

        std::optional<std::tuple<SCH_SHEET_PATH, SCH_ITEM*, std::vector<SCH_ITEM*>>> findRet =
                findItemsFromSyncSelection( Schematic(), syncStr, focusOnFirst );

        if( findRet )
        {
            auto& [sheetPath, focusItem, items] = *findRet;

            m_syncingPcbToSchSelection = true; // recursion guard

            GetToolManager()->GetTool<EE_SELECTION_TOOL>()->SyncSelection( sheetPath, focusItem,
                                                                           items );

            m_syncingPcbToSchSelection = false;
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

        if( SCH_ITEM* item = m_schematic->GetSheets().GetItem( uuid, &path ) )
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
        // Extract file format type and path (plugin type and path separated with \n)
        size_t split = payload.find( '\n' );
        wxCHECK( split != std::string::npos, /*void*/ );
        int importFormat;

        try
        {
            importFormat = std::stoi( payload.substr( 0, split ) );
        }
        catch( std::invalid_argument& )
        {
            wxFAIL;
            importFormat = -1;
        }

        std::string path = payload.substr( split + 1 );
        wxASSERT( !path.empty() );

        if( importFormat >= 0 )
            importFile( path, importFormat );

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
        SyncView();
        break;

    default:;

    }
}
