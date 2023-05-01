/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
 * http://www.gnu.org/licenses/old-licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


/**
 * @file pcbnew/cross-probing.cpp
 * @brief Cross probing functions to handle communication to and from Eeschema.
 * Handle messages between Pcbnew and Eeschema via a socket, the port numbers are
 * KICAD_PCB_PORT_SERVICE_NUMBER (currently 4242) (Eeschema to Pcbnew)
 * KICAD_SCH_PORT_SERVICE_NUMBER (currently 4243) (Pcbnew to Eeschema)
 * Note: these ports must be enabled for firewall protection
 */

#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <pcb_group.h>
#include <zone.h>
#include <collectors.h>
#include <eda_dde.h>
#include <kiface_base.h>
#include <kiway_express.h>
#include <string_utils.h>
#include <netlist_reader/pcb_netlist.h>
#include <netlist_reader/board_netlist_updater.h>
#include <painter.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <render_settings.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <netlist_reader/netlist_reader.h>
#include <wx/log.h>

/* Execute a remote command sent via a socket on port KICAD_PCB_PORT_SERVICE_NUMBER
 *
 * Commands are:
 *
 * $NET: "net name"               Highlight the given net
 * $NETS: "net name 1,net name 2" Highlight all given nets
 * $CLEAR                         Clear existing highlight
 *
 * $CONFIG       Show the Manage Footprint Libraries dialog
 * $CUSTOM_RULES Show the "Custom Rules" page of the Board Setup dialog
 * $DRC          Show the DRC dialog
 */
void PCB_EDIT_FRAME::ExecuteRemoteCommand( const char* cmdline )
{
    char        line[1024];
    wxString    msg;
    wxString    modName;
    char*       idcmd;
    char*       text;
    int         netcode = -1;
    bool        multiHighlight = false;
    FOOTPRINT*  footprint = nullptr;
    PAD*        pad = nullptr;
    BOARD*      pcb = GetBoard();

    CROSS_PROBING_SETTINGS& crossProbingSettings = GetPcbNewSettings()->m_CrossProbing;

    KIGFX::VIEW*            view = m_toolManager->GetView();
    KIGFX::RENDER_SETTINGS* renderSettings = view->GetPainter()->GetSettings();

    strncpy( line, cmdline, sizeof(line) - 1 );
    line[sizeof(line) - 1] = 0;

    idcmd = strtok( line, " \n\r" );
    text  = strtok( nullptr, "\"\n\r" );

    if( idcmd == nullptr )
        return;

    if( strcmp( idcmd, "$CONFIG" ) == 0 )
        {
            GetToolManager()->RunAction( ACTIONS::showSymbolLibTable, true );
            return;
        }
    else if( strcmp( idcmd, "$CUSTOM_RULES" ) == 0 )
    {
        ShowBoardSetupDialog( _( "Custom Rules" ) );
        return;
    }
    else if( strcmp( idcmd, "$DRC" ) == 0 )
    {
        GetToolManager()->RunAction( PCB_ACTIONS::runDRC, true );
        return;
    }
    else if( strcmp( idcmd, "$CLEAR" ) == 0 )
    {
        if( renderSettings->IsHighlightEnabled() )
        {
            renderSettings->SetHighlight( false );
            view->UpdateAllLayersColor();
        }

        if( pcb->IsHighLightNetON() )
        {
            pcb->ResetNetHighLight();
            SetMsgPanel( pcb );
        }

        GetCanvas()->Refresh();
        return;
    }
    else if( strcmp( idcmd, "$NET:" ) == 0 )
    {
        if( !crossProbingSettings.auto_highlight )
            return;

        wxString net_name = FROM_UTF8( text );

        NETINFO_ITEM* netinfo = pcb->FindNet( net_name );

        if( netinfo )
        {
            netcode = netinfo->GetNetCode();

            std::vector<MSG_PANEL_ITEM> items;
            netinfo->GetMsgPanelInfo( this, items );
            SetMsgPanel( items );
        }

        // fall through to hihglighting section
    }
    else if( strcmp( idcmd, "$NETS:" ) == 0 )
    {
        if( !crossProbingSettings.auto_highlight )
            return;

        wxStringTokenizer netsTok = wxStringTokenizer( FROM_UTF8( text ), wxT( "," ) );
        bool first = true;

        while( netsTok.HasMoreTokens() )
        {
            NETINFO_ITEM* netinfo = pcb->FindNet( netsTok.GetNextToken() );

            if( netinfo )
            {
                if( first )
                {
                    // TODO: Once buses are included in netlist, show bus name
                    std::vector<MSG_PANEL_ITEM> items;
                    netinfo->GetMsgPanelInfo( this, items );
                    SetMsgPanel( items );
                    first = false;

                    pcb->SetHighLightNet( netinfo->GetNetCode() );
                    renderSettings->SetHighlight( true, netinfo->GetNetCode() );
                    multiHighlight = true;
                }
                else
                {
                    pcb->SetHighLightNet( netinfo->GetNetCode(), true );
                    renderSettings->SetHighlight( true, netinfo->GetNetCode(), true );
                }
            }
        }

        netcode = -1;

        // fall through to highlighting section
    }

    BOX2I bbox;

    if( footprint )
    {
        bbox = footprint->GetBoundingBox( true, false ); // No invisible text in bbox calc

        if( pad )
            m_toolManager->RunAction( PCB_ACTIONS::highlightItem, true, (void*) pad );
        else
            m_toolManager->RunAction( PCB_ACTIONS::highlightItem, true, (void*) footprint );
    }
    else if( netcode > 0 || multiHighlight )
    {
        if( !multiHighlight )
        {
            renderSettings->SetHighlight( ( netcode >= 0 ), netcode );
            pcb->SetHighLightNet( netcode );
        }
        else
        {
            // Just pick the first one for area calculation
            netcode = *pcb->GetHighLightNetCodes().begin();
        }

        pcb->HighLightON();

        auto merge_area =
                [netcode, &bbox]( BOARD_CONNECTED_ITEM* aItem )
                {
                    if( aItem->GetNetCode() == netcode )
                        bbox.Merge( aItem->GetBoundingBox() );
                };

        if( crossProbingSettings.center_on_items )
        {
            for( ZONE* zone : pcb->Zones() )
                merge_area( zone );

            for( PCB_TRACK* track : pcb->Tracks() )
                merge_area( track );

            for( FOOTPRINT* fp : pcb->Footprints() )
            {
                for( PAD* p : fp->Pads() )
                    merge_area( p );
            }
        }
    }
    else
    {
        renderSettings->SetHighlight( false );
    }

    if( crossProbingSettings.center_on_items && bbox.GetWidth() != 0 && bbox.GetHeight() != 0 )
    {
        if( crossProbingSettings.zoom_to_fit )
            GetToolManager()->GetTool<PCB_SELECTION_TOOL>()->ZoomFitCrossProbeBBox( bbox );

        FocusOnLocation( bbox.Centre() );
    }

    view->UpdateAllLayersColor();

    // Ensure the display is refreshed, because in some installs the refresh is done only
    // when the gal canvas has the focus, and that is not the case when crossprobing from
    // Eeschema:
    GetCanvas()->Refresh();
}


std::string FormatProbeItem( BOARD_ITEM* aItem )
{
    if( !aItem )
        return "$CLEAR: \"HIGHLIGHTED\""; // message to clear highlight state

    switch( aItem->Type() )
    {
    case PCB_FOOTPRINT_T:
    {
        FOOTPRINT* footprint = static_cast<FOOTPRINT*>( aItem );
        return StrPrintf( "$PART: \"%s\"", TO_UTF8( footprint->GetReference() ) );
    }

    case PCB_PAD_T:
    {
        PAD*       pad = static_cast<PAD*>( aItem );
        FOOTPRINT* footprint = pad->GetParentFootprint();

        return StrPrintf( "$PART: \"%s\" $PAD: \"%s\"",
                          TO_UTF8( footprint->GetReference() ),
                          TO_UTF8( pad->GetNumber() ) );
    }

    case PCB_TEXT_T:
    {
        PCB_TEXT*   text = static_cast<PCB_TEXT*>( aItem );
        FOOTPRINT*  footprint = text->GetParentFootprint();
        const char* text_key;

        /* This can't be a switch since the break need to pull out
         * from the outer switch! */
        if( text->GetType() == PCB_TEXT::TEXT_is_REFERENCE )
            text_key = "$REF:";
        else if( text->GetType() == PCB_TEXT::TEXT_is_VALUE )
            text_key = "$VAL:";
        else
            break;

        return StrPrintf( "$PART: \"%s\" %s \"%s\"",
                          TO_UTF8( footprint->GetReference() ),
                          text_key,
                          TO_UTF8( text->GetText() ) );
    }

    default:
        break;
    }

    return "";
}


template <typename ItemContainer>
void collectItemsForSyncParts( ItemContainer& aItems, std::set<wxString>& parts )
{
    for( EDA_ITEM* item : aItems )
    {
        switch( item->Type() )
        {
        case PCB_GROUP_T:
        {
            PCB_GROUP* group = static_cast<PCB_GROUP*>( item );

            collectItemsForSyncParts( group->GetItems(), parts );

            break;
        }
        case PCB_FOOTPRINT_T:
        {
            FOOTPRINT* footprint = static_cast<FOOTPRINT*>( item );
            wxString   ref = footprint->GetReference();

            parts.emplace( wxT( "F" ) + EscapeString( ref, CTX_IPC ) );

            break;
        }

        case PCB_PAD_T:
        {
            PAD*      pad = static_cast<PAD*>( item );
            wxString  ref = pad->GetParentFootprint()->GetReference();

            parts.emplace( wxT( "P" ) + EscapeString( ref, CTX_IPC ) + wxT( "/" )
                           + EscapeString( pad->GetNumber(), CTX_IPC ) );

            break;
        }

        default: break;
        }
    }
}


void PCB_EDIT_FRAME::SendSelectItemsToSch( const std::deque<EDA_ITEM*>& aItems,
                                           EDA_ITEM* aFocusItem, bool aForce )
{
    std::string command = "$SELECT: ";

    if( aFocusItem )
    {
        std::deque<EDA_ITEM*> focusItems = { aFocusItem };
        std::set<wxString>    focusParts;
        collectItemsForSyncParts( focusItems, focusParts );

        if( focusParts.size() > 0 )
        {
            command += "1,";
            command += *focusParts.begin();
            command += ",";
        }
        else
        {
            command += "0,";
        }
    }
    else
    {
        command += "0,";
    }

    std::set<wxString> parts;
    collectItemsForSyncParts( aItems, parts );

    if( parts.empty() )
        return;

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
        Kiway().ExpressMail( FRAME_SCH, aForce ? MAIL_SELECTION_FORCE : MAIL_SELECTION, command,
                             this );
    }
}


void PCB_EDIT_FRAME::SendCrossProbeNetName( const wxString& aNetName )
{
    std::string packet = StrPrintf( "$NET: \"%s\"", TO_UTF8( aNetName ) );

    if( !packet.empty() )
    {
        if( Kiface().IsSingle() )
        {
            SendCommand( MSG_TO_SCH, packet );
        }
        else
        {
            // Typically ExpressMail is going to be s-expression packets, but since
            // we have existing interpreter of the cross probe packet on the other
            // side in place, we use that here.
            Kiway().ExpressMail( FRAME_SCH, MAIL_CROSS_PROBE, packet, this );
        }
    }
}


void PCB_EDIT_FRAME::SendCrossProbeItem( BOARD_ITEM* aSyncItem )
{
    std::string packet = FormatProbeItem( aSyncItem );

    if( !packet.empty() )
    {
        if( Kiface().IsSingle() )
        {
            SendCommand( MSG_TO_SCH, packet );
        }
        else
        {
            // Typically ExpressMail is going to be s-expression packets, but since
            // we have existing interpreter of the cross probe packet on the other
            // side in place, we use that here.
            Kiway().ExpressMail( FRAME_SCH, MAIL_CROSS_PROBE, packet, this );
        }
    }
}


std::vector<BOARD_ITEM*> PCB_EDIT_FRAME::FindItemsFromSyncSelection( std::string syncStr )
{
    wxArrayString syncArray = wxStringTokenize( syncStr, "," );

    std::vector<std::pair<int, BOARD_ITEM*>> orderPairs;

    for( FOOTPRINT* footprint : GetBoard()->Footprints() )
    {
        if( footprint == nullptr )
            continue;

        wxString fpSheetPath = footprint->GetPath().AsString().BeforeLast( '/' );
        wxString fpUUID = footprint->m_Uuid.AsString();

        if( fpSheetPath.IsEmpty() )
            fpSheetPath += '/';

        if( fpUUID.empty() )
            continue;

        wxString fpRefEscaped = EscapeString( footprint->GetReference(), CTX_IPC );

        for( unsigned index = 0; index < syncArray.size(); ++index )
        {
            wxString syncEntry = syncArray[index];

            if( syncEntry.empty() )
                continue;

            wxString syncData = syncEntry.substr( 1 );

            switch( syncEntry.GetChar( 0 ).GetValue() )
            {
            case 'S': // Select sheet with subsheets: S<Sheet path>
                if( fpSheetPath.StartsWith( syncData ) )
                {
                    orderPairs.emplace_back( index, footprint );
                }
                break;
            case 'F': // Select footprint: F<Reference>
                if( syncData == fpRefEscaped )
                {
                    orderPairs.emplace_back( index, footprint );
                }
                break;
            case 'P': // Select pad: P<Footprint reference>/<Pad number>
            {
                if( syncData.StartsWith( fpRefEscaped ) )
                {
                    wxString selectPadNumberEscaped =
                            syncData.substr( fpRefEscaped.size() + 1 ); // Skips the slash

                    wxString selectPadNumber = UnescapeString( selectPadNumberEscaped );

                    for( PAD* pad : footprint->Pads() )
                    {
                        if( selectPadNumber == pad->GetNumber() )
                        {
                            orderPairs.emplace_back( index, pad );
                        }
                    }
                }
                break;
            }
            default: break;
            }
        }
    }

    std::sort(
            orderPairs.begin(), orderPairs.end(),
            []( const std::pair<int, BOARD_ITEM*>& a, const std::pair<int, BOARD_ITEM*>& b ) -> bool
            {
                return a.first < b.first;
            } );

    std::vector<BOARD_ITEM*> items;
    items.reserve( orderPairs.size() );

    for( const std::pair<int, BOARD_ITEM*>& pair : orderPairs )
        items.push_back( pair.second );

    return items;
}


void PCB_EDIT_FRAME::KiwayMailIn( KIWAY_EXPRESS& mail )
{
    std::string& payload = mail.GetPayload();

    switch( mail.Command() )
    {
    case MAIL_PCB_GET_NETLIST:
    {
        NETLIST          netlist;
        STRING_FORMATTER sf;

        for( FOOTPRINT* footprint : GetBoard()->Footprints() )
        {
            if( footprint->GetAttributes() & FP_BOARD_ONLY )
                continue; // Don't add board-only footprints to the netlist

            COMPONENT* component = new COMPONENT( footprint->GetFPID(), footprint->GetReference(),
                                                  footprint->GetValue(), footprint->GetPath(), {} );

            for( PAD* pad : footprint->Pads() )
            {
                const wxString& netname = pad->GetShortNetname();

                if( !netname.IsEmpty() )
                {
                    component->AddNet( pad->GetNumber(), netname, pad->GetPinFunction(),
                                       pad->GetPinType() );
                }
            }

            // Add DNP and Exclude from BOM properties
            std::map<wxString, wxString> properties;

            if( footprint->GetAttributes() & FP_DNP )
                properties.emplace( "dnp", "" );

            if( footprint->GetAttributes() & FP_EXCLUDE_FROM_BOM )
                properties.emplace( "exclude_from_bom", "" );

            component->SetProperties( properties );

            netlist.AddComponent( component );
        }

        netlist.Format( "pcb_netlist", &sf, 0, CTL_OMIT_FILTERS );
        payload = sf.GetString();
        break;
    }

    case MAIL_PCB_UPDATE_LINKS:
        try
        {
            NETLIST netlist;
            FetchNetlistFromSchematic( netlist, wxEmptyString );

            BOARD_NETLIST_UPDATER updater( this, GetBoard() );
            updater.SetLookupByTimestamp( false );
            updater.SetDeleteUnusedFootprints( false );
            updater.SetReplaceFootprints( false );
            updater.UpdateNetlist( netlist );

            bool dummy;
            OnNetlistChanged( updater, &dummy );
        }
        catch( const IO_ERROR& )
        {
            assert( false ); // should never happen
            return;
        }

        break;

    case MAIL_CROSS_PROBE:
        ExecuteRemoteCommand( payload.c_str() );
        break;


    case MAIL_SELECTION:
        if( !GetPcbNewSettings()->m_CrossProbing.on_selection )
            break;

        KI_FALLTHROUGH;

    case MAIL_SELECTION_FORCE:
    {
        // $SELECT: <mode 0 - only footprints, 1 - with connections>,<spec1>,<spec2>,<spec3>
        std::string prefix = "$SELECT: ";

        if( !payload.compare( 0, prefix.size(), prefix ) )
        {
            std::string del = ",";
            std::string paramStr = payload.substr( prefix.size() );
            int         modeEnd = paramStr.find( del );
            bool        selectConnections = false;

            try
            {
                if( std::stoi( paramStr.substr( 0, modeEnd ) ) == 1 )
                    selectConnections = true;
            }
            catch( std::invalid_argument& )
            {
                wxFAIL;
            }

            std::vector<BOARD_ITEM*> items =
                    FindItemsFromSyncSelection( paramStr.substr( modeEnd + 1 ) );

            m_probingSchToPcb = true; // recursion guard

            if( selectConnections )
            {
                GetToolManager()->RunAction( PCB_ACTIONS::syncSelectionWithNets, true,
                                             static_cast<void*>( &items ) );
            }
            else
            {
                GetToolManager()->RunAction( PCB_ACTIONS::syncSelection, true,
                                             static_cast<void*>( &items ) );
            }

            // Update 3D viewer highlighting
            Update3DView( false, GetPcbNewSettings()->m_Display.m_Live3DRefresh );

            m_probingSchToPcb = false;
        }

        break;
    }

    case MAIL_PCB_UPDATE:
        m_toolManager->RunAction( ACTIONS::updatePcbFromSchematic, true );
        break;

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

    case MAIL_RELOAD_PLUGINS:
        GetToolManager()->RunAction( PCB_ACTIONS::pluginsReload, true );
        break;

    // many many others.
    default:
        ;
    }
}

