/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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


/**
 * @file pcbnew/cross-probing.cpp
 * @brief Cross probing functions to handle communication to and from Eeschema.
 * Handle messages between Pcbnew and Eeschema via a socket, the port numbers are
 * KICAD_PCB_PORT_SERVICE_NUMBER (currently 4242) (Eeschema to Pcbnew)
 * KICAD_SCH_PORT_SERVICE_NUMBER (currently 4243) (Pcbnew to Eeschema)
 * Note: these ports must be enabled for firewall protection
 */

#include <api/api_handler_pcb.h>
#include <wx/tokenzr.h>
#include <api/api_utils.h>
#include <api/common/commands/cross_probe_commands.pb.h>
#include <api/cross_probe_client.h>
#include <board.h>
#include <board_design_settings.h>
#include <fmt.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <pcb_group.h>
#include <zone.h>
#include <collectors.h>
#include <kiface_base.h>
#include <kiway_mail.h>
#include <string_utils.h>
#include <netlist_reader/netlist_reader.h>
#include <netlist_reader/pcb_netlist.h>
#include <netlist_reader/board_netlist_updater.h>
#include <gal/painter.h>
#include <pcb_painter.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <render_settings.h>
#include <richio.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <trace_helpers.h>
#include <netlist_reader/netlist_reader.h>
#include <widgets/pcb_design_block_pane.h>
#include <widgets/kistatusbar.h>
#include <project_pcb.h>
#include <footprint_import_reconciler.h>
#include <import_local_footprint_libs.h>
#include <import_net_reconciler.h>
#include <import_proj_properties.h>
#include <netlist_reader/pcb_netlist.h>
#include <reporter.h>
#include <footprint_library_adapter.h>
#include <pcb_io/pcb_io_mgr.h>
#include <pgm_base.h>
#include <libraries/library_manager.h>
#include <libraries/library_table.h>
#include <unordered_set>

#include <wx/filename.h>
#include <wx/log.h>
#include <wx/msgdlg.h>


using namespace kiapi::common::commands;

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
    char*       idcmd;
    char*       text;
    int         netcode = -1;
    bool        multiHighlight = false;
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
        GetToolManager()->RunAction( ACTIONS::showFootprintLibTable );
        return;
    }
    else if( strcmp( idcmd, "$CUSTOM_RULES" ) == 0 )
    {
        ShowBoardSetupDialog( _( "Custom Rules" ) );
        return;
    }
    else if( strcmp( idcmd, "$DRC" ) == 0 )
    {
        GetToolManager()->RunAction( PCB_ACTIONS::runDRC );
        return;
    }
    else if( strcmp( idcmd, "$CLEAR" ) == 0 )
    {
        auto* pcbRender = dynamic_cast<KIGFX::PCB_RENDER_SETTINGS*>( renderSettings );

        bool hadHighlight = renderSettings->IsHighlightEnabled();
        bool hadChain = pcbRender && !pcbRender->GetHighlightedNetChain().IsEmpty();

        if( hadHighlight )
            renderSettings->SetHighlight( false );

        if( hadChain )
            pcbRender->SetHighlightedNetChain( wxString() );

        if( hadHighlight || hadChain )
            view->UpdateAllLayersColor();

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

        wxString net_name = From_UTF8( text );

        NETINFO_ITEM* netinfo = pcb->FindNet( net_name );

        if( netinfo )
        {
            netcode = netinfo->GetNetCode();

            std::vector<MSG_PANEL_ITEM> items;
            netinfo->GetMsgPanelInfo( this, items );
            SetMsgPanel( items );

            // If the incoming net belongs to a net chain, promote the single-net
            // highlight into a multi-net highlight covering every chain member so
            // the PCB mirrors the chain highlight happening on the schematic side.
            const wxString& chainName = netinfo->GetNetChain();

            if( !chainName.IsEmpty() )
            {
                pcb->SetHighLightNet( netcode );
                renderSettings->SetHighlight( true, netcode );
                multiHighlight = true;

                for( NETINFO_ITEM* candidate : pcb->GetNetInfo() )
                {
                    if( !candidate || candidate == netinfo )
                        continue;

                    if( candidate->GetNetChain() == chainName )
                    {
                        pcb->SetHighLightNet( candidate->GetNetCode(), true );
                        renderSettings->SetHighlight( true, candidate->GetNetCode(), true );
                    }
                }

                if( auto* pcbRender = dynamic_cast<KIGFX::PCB_RENDER_SETTINGS*>( renderSettings ) )
                    pcbRender->SetHighlightedNetChain( chainName );

                netcode = -1;
            }
        }

        // fall through to highlighting section
    }
    else if( strcmp( idcmd, "$NETS:" ) == 0 )
    {
        if( !crossProbingSettings.auto_highlight )
            return;

        wxStringTokenizer netsTok = wxStringTokenizer( From_UTF8( text ), ",", wxTOKEN_STRTOK );
        bool first = true;

        while( netsTok.HasMoreTokens() )
        {
            NETINFO_ITEM* netinfo = pcb->FindNet( netsTok.GetNextToken().Trim( true ).Trim( false ) );

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

    if( netcode > 0 || multiHighlight )
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


void PCB_EDIT_FRAME::HandleRemoteNetHighlight( const std::vector<wxString>& aNetNames )
{
    NETINFO_ITEM* netinfo;
    int    netcode = -1;
    bool   multiHighlight = false;
    BOARD* pcb = GetBoard();

    CROSS_PROBING_SETTINGS& crossProbingSettings = GetPcbNewSettings()->m_CrossProbing;
    KIGFX::VIEW*            view = m_toolManager->GetView();
    KIGFX::RENDER_SETTINGS* renderSettings = view->GetPainter()->GetSettings();

    if( aNetNames.empty() || ( aNetNames.size() == 1 && aNetNames[0].empty() ) )
    {
        auto* pcbRender = dynamic_cast<KIGFX::PCB_RENDER_SETTINGS*>( renderSettings );

        bool hadHighlight = renderSettings->IsHighlightEnabled();
        bool hadChain = pcbRender && !pcbRender->GetHighlightedNetChain().IsEmpty();

        if( hadHighlight )
            renderSettings->SetHighlight( false );

        if( hadChain )
            pcbRender->SetHighlightedNetChain( wxString() );

        if( hadHighlight || hadChain )
            view->UpdateAllLayersColor();

        if( pcb->IsHighLightNetON() )
        {
            pcb->ResetNetHighLight();
            SetMsgPanel( pcb );
        }

        GetCanvas()->Refresh();
        return;
    }

    if( aNetNames.size() == 1 && ( netinfo = pcb->FindNet( aNetNames[0] ) ) != nullptr )
    {
        netcode = netinfo->GetNetCode();

        std::vector<MSG_PANEL_ITEM> items;
        netinfo->GetMsgPanelInfo( this, items );
        SetMsgPanel( items );

        // If the incoming net belongs to a net chain, promote the single-net
        // highlight into a multi-net highlight covering every chain member so
        // the PCB mirrors the chain highlight happening on the schematic side.
        const wxString& chainName = netinfo->GetNetChain();

        if( !chainName.IsEmpty() )
        {
            pcb->SetHighLightNet( netcode );
            renderSettings->SetHighlight( true, netcode );
            multiHighlight = true;

            for( NETINFO_ITEM* candidate : pcb->GetNetInfo() )
            {
                if( !candidate || candidate == netinfo )
                    continue;

                if( candidate->GetNetChain() == chainName )
                {
                    pcb->SetHighLightNet( candidate->GetNetCode(), true );
                    renderSettings->SetHighlight( true, candidate->GetNetCode(), true );
                }
            }

            if( auto* pcbRender = dynamic_cast<KIGFX::PCB_RENDER_SETTINGS*>( renderSettings ) )
                pcbRender->SetHighlightedNetChain( chainName );

            netcode = -1;
        }
    }
    else
    {
        bool first = true;

        for( const wxString& netName : aNetNames )
        {
            netinfo = pcb->FindNet( netName );

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
    }

    BOX2I bbox;

    if( netcode > 0 || multiHighlight )
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


static bool selectionSpecFromItem( const EDA_ITEM* aItem, SelectionSpec& aSpec )
{
    switch( aItem->Type() )
    {
    case PCB_FOOTPRINT_T:
    {
        const FOOTPRINT* footprint = static_cast<const FOOTPRINT*>( aItem );
        aSpec.mutable_footprint()->set_reference( footprint->GetReference().ToUTF8() );
        return true;
    }

    case PCB_PAD_T:
    {
        const PAD* pad = static_cast<const PAD*>( aItem );

        if( const FOOTPRINT* footprint = pad->GetParentFootprint() )
        {
            aSpec.mutable_pad()->set_reference( footprint->GetReference().ToUTF8() );
            aSpec.mutable_pad()->set_number( pad->GetNumber().ToUTF8() );
            return true;
        }

        break;
    }

    default:
        break;
    }

    return false;
}


void PCB_EDIT_FRAME::SendSelectItemsToSch( const std::deque<EDA_ITEM*>& aItems, EDA_ITEM* aFocusItem, bool aForce )
{
    SyncSelection sync;

    if( aFocusItem )
    {
        SelectionSpec focusSpec;

        if( selectionSpecFromItem( aFocusItem, focusSpec ) )
        {
            sync.set_mode( SyncSelectionMode::SSM_ITEMS_AND_NETS );
            sync.mutable_focus_item()->CopyFrom( focusSpec );
            sync.mutable_items()->Add()->CopyFrom( focusSpec );
        }
    }

    std::vector<EDA_ITEM*> collectedItems;

    for( EDA_ITEM* item : aItems )
    {
        if( item->Type() == PCB_GROUP_T )
        {
            static_cast<const PCB_GROUP*>( item )->RunOnChildren(
                    [&]( BOARD_ITEM* aChild )
                    {
                        if( aChild->Type() == PCB_FOOTPRINT_T || aChild->Type() == PCB_PAD_T )
                            collectedItems.push_back( aChild );
                    }, RECURSE_MODE::RECURSE );
        }
        else if( item->Type() == PCB_FOOTPRINT_T || item->Type() == PCB_PAD_T )
        {
            collectedItems.push_back( item );
        }
    }

    for( EDA_ITEM* item : collectedItems )
        selectionSpecFromItem( item, *sync.add_items() );

    if( sync.items_size() == 0 )
        return;

    sync.set_context( aForce ? SyncSelectionContext::SSC_EXPLICIT : SyncSelectionContext::SSC_IMPLICIT );

    if( Kiface().IsSingle() )
    {
        CROSS_PROBE_CLIENT::SendToFrame( FRAME_SCH, sync );
    }
    else
    {
        std::string payload;
        kiapi::common::PackKiwayApiMessage( sync, payload );
        Kiway().ExpressMail( FRAME_SCH, MAIL_SELECTION, payload, this );
    }
}


void PCB_EDIT_FRAME::SendCrossProbeNetName( const wxString& aNetName )
{
    kiapi::common::commands::HighlightNets message;

    message.add_net_name( aNetName.ToUTF8() );

    if( Kiface().IsSingle() )
    {
        CROSS_PROBE_CLIENT::SendToFrame( FRAME_SCH, message );
    }
    else
    {
        std::string payload;
        kiapi::common::PackKiwayApiMessage( message, payload );
        Kiway().ExpressMail( FRAME_SCH, MAIL_CROSS_PROBE, payload, this );
    }
}


void PCB_EDIT_FRAME::SendCrossProbeItem( BOARD_ITEM* aSyncItem )
{
    if( !aSyncItem )
    {
        SendCrossProbeNetName( wxEmptyString );
        return;
    }

    kiapi::common::commands::FocusOnItem message;
    SelectionSpec* spec = message.mutable_focus_item();

    switch( aSyncItem->Type() )
    {
    case PCB_FOOTPRINT_T:
    {
        FOOTPRINT* footprint = static_cast<FOOTPRINT*>( aSyncItem );
        spec->mutable_footprint()->set_reference( footprint->GetReference().ToUTF8() );
        break;
    }

    case PCB_PAD_T:
    {
        PAD*       pad = static_cast<PAD*>( aSyncItem );
        FOOTPRINT* footprint = pad->GetParentFootprint();

        spec->mutable_pad()->set_reference( footprint->GetReference().ToUTF8() );
        spec->mutable_pad()->set_number( pad->GetNumber().ToUTF8() );
        break;
    }

    case PCB_FIELD_T:
    {
        PCB_FIELD*  field = static_cast<PCB_FIELD*>( aSyncItem );
        FOOTPRINT*  footprint = field->GetParentFootprint();
        spec->mutable_footprint()->set_reference( footprint->GetReference().ToUTF8() );
        break;
    }

    default:
        break;
    }

    if( Kiface().IsSingle() )
    {
        CROSS_PROBE_CLIENT::SendToFrame( FRAME_SCH, message );
    }
    else
    {
        std::string payload;
        kiapi::common::PackKiwayApiMessage( message, payload );
        Kiway().ExpressMail( FRAME_SCH, MAIL_CROSS_PROBE, payload, this );
    }
}


void PCB_EDIT_FRAME::SetLastSchematicSheetPath( const KIID_PATH& aPath )
{
    if( m_lastSchematicSheetPath == aPath )
        return;

    m_lastSchematicSheetPath = aPath;

    wxCommandEvent event( EDA_EVT_PCB_LAST_SCH_SHEET_CHANGED, GetId() );
    event.SetEventObject( this );
    wxPostEvent( this, event );
}


void PCB_EDIT_FRAME::KiwayMailIn( KIWAY_MAIL_EVENT& mail )
{
    std::string& payload = mail.GetPayload();

    switch( mail.Command() )
    {
    case MAIL_SCH_SHEET_CHANGED:
        SetLastSchematicSheetPath( KIID_PATH( wxString::FromUTF8( payload.c_str() ) ) );
        break;

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
                    component->AddNet( pad->GetNumber(), netname, pad->GetPinFunction(), pad->GetPinType() );
            }

            nlohmann::ordered_map<wxString, wxString> fields;

            for( PCB_FIELD* field : footprint->GetFields() )
            {
                wxCHECK2( field, continue );

                fields[field->GetUntranslatedName()] = field->GetText();
            }

            component->SetFields( fields );

            // Add DNP and exclusion properties
            std::map<wxString, wxString> properties;

            if( footprint->GetAttributes() & FP_DNP )
                properties.emplace( "dnp", "" );

            if( footprint->GetAttributes() & FP_EXCLUDE_FROM_BOM )
                properties.emplace( "exclude_from_bom", "" );

            if( footprint->GetAttributes() & FP_EXCLUDE_FROM_SIM )
                properties.emplace( "exclude_from_sim", "" );

            if( footprint->GetAttributes() & FP_EXCLUDE_FROM_POS_FILES )
                properties.emplace( "exclude_from_pos_files", "" );

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
            updater.SetWillDrag( false );
            updater.SetLookupByTimestamp( false );
            updater.SetDeleteUnusedFootprints( false );
            updater.SetReplaceFootprints( false );
            updater.SetTransferGroups( false );
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

    case MAIL_ADD_LOCAL_LIB:
    {
        std::stringstream ss( payload );
        std::string       file;

        // First line of payload is the source project directory.
        std::string srcProjDir;
        std::getline( ss, srcProjDir, '\n' );

        std::vector<wxString> libraries;

        while( std::getline( ss, file, '\n' ) )
            libraries.push_back( wxString::FromUTF8( file ) );

        RegisterLocalFootprintLibraries( Prj(), wxString::FromUTF8( srcProjDir ), libraries,
                                         WXLOG_REPORTER::GetInstance() );

        Kiway().ExpressMail( FRAME_CVPCB, MAIL_RELOAD_LIB, payload );
        Kiway().ExpressMail( FRAME_FOOTPRINT_EDITOR, MAIL_RELOAD_LIB, payload );
        Kiway().ExpressMail( FRAME_FOOTPRINT_VIEWER, MAIL_RELOAD_LIB, payload );

        break;
    }

    // Handled as API messages
    case MAIL_CROSS_PROBE:
    case MAIL_SELECTION:
        if( ApiRequest request; request.ParseFromString( payload ) )
            m_apiHandler->Handle( request );

        break;

    case MAIL_PCB_UPDATE:
        m_toolManager->RunAction( ACTIONS::updatePcbFromSchematic );
        break;

    case MAIL_PCB_RECONCILE_NETS:
    {
        // an empty payload skips the schematic's annotation prompt, and reading the reply here avoids the
        // dialogs of the update command, so an import never waits on the user
        std::string netlistText;

        Kiway().ExpressMail( FRAME_SCH, MAIL_SCH_GET_NETLIST, netlistText, this );

        if( netlistText.empty() )
        {
            WXLOG_REPORTER::GetInstance().Report( _( "The schematic gave no netlist, so the board nets keep their "
                                                     "names.\n" ),
                                                  RPT_SEVERITY_INFO );
            break;
        }

        NETLIST netlist;

        try
        {
            KICAD_NETLIST_READER reader( new STRING_LINE_READER( netlistText, _( "Eeschema netlist" ) ), &netlist );
            reader.LoadNetlist();
        }
        catch( const IO_ERROR& ioe )
        {
            WXLOG_REPORTER::GetInstance().Report( wxString::Format( _( "The schematic netlist could not be read, so "
                                                                        "the board nets keep their names: %s\n" ),
                                                                     ioe.What() ),
                                                  RPT_SEVERITY_INFO );
            break;
        }

        int renamed = ReconcileImportedReferences( *GetBoard(), netlist );

        NET_RECONCILE_RESULT result = ReconcileImportedNetNames( *GetBoard(), netlist, WXLOG_REPORTER::GetInstance() );

        int filled = FillImportedFootprintFields( *GetBoard(), netlist );

        if( renamed > 0 || result.m_renamed > 0 || result.m_unconnectedAssigned > 0 || filled > 0 )
            OnModify();

        // the fields were changed behind the view's back, so it is rebuilt from the board
        if( filled > 0 )
        {
            for( FOOTPRINT* footprint : GetBoard()->Footprints() )
                footprint->InvalidateGeometryCaches();

            GetCanvas()->DisplayBoard( GetBoard() );
            GetCanvas()->Refresh();
        }

        break;
    }

    case MAIL_PCB_GET_FOOTPRINT_IDS:
        payload = TO_UTF8( IMPORT_PROJ_PROPS::JoinTaggedMap( IMPORT_PROJ_PROPS::FOOTPRINT_ID_MAP_TAG,
                                                             ImportedFootprintIds( *GetBoard() ) ) );
        break;

    case MAIL_IMPORT_FILE:
    {
        // Extract file format type and path (plugin type, path and properties keys, values separated with \n)
        std::stringstream ss( payload );
        char              delim = '\n';

        std::string formatStr;
        wxCHECK( std::getline( ss, formatStr, delim ), /* void */ );

        std::string fnameStr;
        wxCHECK( std::getline( ss, fnameStr, delim ), /* void */ );
        wxASSERT( !fnameStr.empty() );

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

        std::string key, value;

        do
        {
            if( !std::getline( ss, key, delim ) )
                break;

            if( !std::getline( ss, value, delim ) )
                break;

            props.emplace( key, value );

        } while( true );

        payload.clear();

        if( importFormat >= 0 && importFile( fnameStr, importFormat, props.empty() ? nullptr : &props ) )
            payload = "success";

        break;
    }

    case MAIL_RELOAD_PLUGINS:
        GetToolManager()->RunAction( ACTIONS::pluginsReload );
        break;

    case MAIL_PCB_SAVE:
        if( SavePcbFile( Prj().AbsolutePath( GetBoard()->GetFileName() ) ) )
            payload = "success";

        break;

    case MAIL_PCB_RELOAD:
    {
        // A git operation rewrote files on disk. Reload the board so the editor matches.
        wxString file = Prj().AbsolutePath( GetBoard()->GetFileName() );

        if( file.IsEmpty() )
            break;

        // A non-empty payload lists the files the operation changed. When the board is not
        // among them, there is nothing to reload.
        if( !payload.empty() )
        {
            wxArrayString changed = wxSplit( wxString::FromUTF8( payload.c_str() ), '\n' );
            bool          mine = false;
            wxString      boardFile = wxFileName( file ).GetFullPath();

            for( const wxString& changedFile : changed )
            {
                if( !changedFile.IsEmpty() && boardFile == wxFileName( changedFile ).GetFullPath() )
                {
                    mine = true;
                    break;
                }
            }

            if( !mine )
                break;
        }

        // Neither choice is safe without asking: reloading discards the unsaved edits, keeping
        // them means the next save overwrites what arrived.
        if( IsContentModified() )
        {
            wxMessageDialog dlg( this,
                                 _( "The board changed on disk while you were editing it.\n\n"
                                    "Reloading loses your unsaved edits. Keeping them means your "
                                    "next save overwrites the changes that just arrived. To get those "
                                    "back later, use Discard Changes on the file in the Version Control "
                                    "panel, which drops your edits to it." ),
                                 _( "Board Changed on Disk" ), wxYES_NO | wxNO_DEFAULT | wxICON_WARNING | wxCENTER );
            dlg.SetYesNoLabels( _( "&Reload" ), _( "&Keep My Edits" ) );

            if( dlg.ShowModal() != wxID_YES )
                break;
        }

        ReloadFromDisk();
        break;
    }

    case MAIL_PCB_PROJECT_SETTINGS_CHANGED:
        // The settings were reread, but every net caches its resolved netclass, so routing would
        // keep the old widths and clearances until they resynchronize.
        GetBoard()->SynchronizeTuningProfileProperties();
        GetBoard()->SynchronizeNetsAndNetClasses( true );

        if( !GetBoard()->SynchronizeComponentClasses( std::unordered_set<wxString>() ) )
            wxLogTrace( traceGit, wxS( "Could not load component class assignment rules" ) );

        Prj().IncrementNetclassesTicker();
        Prj().IncrementTextVarsTicker();

        GetCanvas()->GetView()->UpdateAllItems( KIGFX::ALL );
        GetCanvas()->Refresh();
        break;

    case MAIL_RELOAD_LIB:
    {
        m_designBlocksPane->RefreshLibs();

        // Show any footprint library load errors in the status bar
        if( KISTATUSBAR* statusBar = dynamic_cast<KISTATUSBAR*>( GetStatusBar() ) )
        {
            FOOTPRINT_LIBRARY_ADAPTER* adapter = PROJECT_PCB::FootprintLibAdapter( &Prj() );
            statusBar->AddWarningMessages( "load", adapter->GetLibraryLoadErrors() );
        }

        break;
    }

    // many many others.
    default:
        ;
    }
}
