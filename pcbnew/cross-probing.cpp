/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <footprint.h>
#include <track.h>
#include <collectors.h>
#include <eda_dde.h>
#include <kiface_i.h>
#include <kiway_express.h>
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

/* Execute a remote command send by Eeschema via a socket,
 * port KICAD_PCB_PORT_SERVICE_NUMBER
 * cmdline = received command from Eeschema
 * Commands are
 * $PART: "reference"   put cursor on component
 * $PIN: "pin name"  $PART: "reference" put cursor on the footprint pin
 * $NET: "net name" highlight the given net (if highlight tool is active)
 * $CLEAR Clear existing highlight
 * They are a keyword followed by a quoted string.
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
    text  = strtok( NULL, "\"\n\r" );

    if( idcmd == NULL )
        return;

    if( strcmp( idcmd, "$NET:" ) == 0 )
    {
        if( !crossProbingSettings.auto_highlight )
            return;

        wxString net_name = FROM_UTF8( text );

        NETINFO_ITEM* netinfo = pcb->FindNet( net_name );

        if( netinfo )
        {
            netcode = netinfo->GetNetCode();

            MSG_PANEL_ITEMS items;
            netinfo->GetMsgPanelInfo( this, items );
            SetMsgPanel( items );
        }
    }
    if( strcmp( idcmd, "$NETS:" ) == 0 )
    {
        if( !crossProbingSettings.auto_highlight )
            return;

        wxStringTokenizer netsTok = wxStringTokenizer( FROM_UTF8( text ), "," );
        bool first = true;

        while( netsTok.HasMoreTokens() )
        {
            NETINFO_ITEM* netinfo = pcb->FindNet( netsTok.GetNextToken() );

            if( netinfo )
            {
                if( first )
                {
                    // TODO: Once buses are included in netlist, show bus name
                    MSG_PANEL_ITEMS items;
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
    else if( strcmp( idcmd, "$PIN:" ) == 0 )
    {
        wxString pinName = FROM_UTF8( text );

        text = strtok( NULL, " \n\r" );

        if( text && strcmp( text, "$PART:" ) == 0 )
            text = strtok( NULL, "\"\n\r" );

        modName = FROM_UTF8( text );

        footprint = pcb->FindFootprintByReference( modName );

        if( footprint )
            pad = footprint->FindPadByName( pinName );

        if( pad )
            netcode = pad->GetNetCode();

        if( footprint == NULL )
            msg.Printf( _( "%s not found" ), modName );
        else if( pad == NULL )
            msg.Printf( _( "%s pin %s not found" ), modName, pinName );
        else
            msg.Printf( _( "%s pin %s found" ), modName, pinName );

        SetStatusText( msg );
    }
    else if( strcmp( idcmd, "$PART:" ) == 0 )
    {
        pcb->ResetNetHighLight();

        modName = FROM_UTF8( text );

        footprint = pcb->FindFootprintByReference( modName );

        if( footprint )
            msg.Printf( _( "%s found" ), modName );
        else
            msg.Printf( _( "%s not found" ), modName );

        SetStatusText( msg );
    }
    else if( strcmp( idcmd, "$SHEET:" ) == 0 )
    {
        msg.Printf( _( "Selecting all from sheet \"%s\"" ), FROM_UTF8( text ) );
        wxString sheetUIID( FROM_UTF8( text ) );
        SetStatusText( msg );
        GetToolManager()->RunAction( PCB_ACTIONS::selectOnSheetFromEeschema, true,
                                     static_cast<void*>( &sheetUIID ) );
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

    BOX2I bbox = { { 0, 0 }, { 0, 0 } };

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
            {
                if( bbox.GetWidth() == 0 )
                    bbox = aItem->GetBoundingBox();
                else
                    bbox.Merge( aItem->GetBoundingBox() );
            }
        };

        if( crossProbingSettings.center_on_items )
        {
            for( ZONE* zone : pcb->Zones() )
                merge_area( zone );

            for( TRACK* track : pcb->Tracks() )
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

    if( crossProbingSettings.center_on_items && bbox.GetWidth() > 0 && bbox.GetHeight() > 0 )
    {
        if( crossProbingSettings.zoom_to_fit )
        {
//#define DEFAULT_PCBNEW_CODE // Un-comment for normal full zoom KiCad algorithm
 #ifdef DEFAULT_PCBNEW_CODE
            auto bbSize     = bbox.Inflate( bbox.GetWidth() * 0.2f ).GetSize();
            auto screenSize = view->ToWorld( GetCanvas()->GetClientSize(), false );

            // The "fabs" on x ensures the right answer when the view is flipped
            screenSize.x = std::max( 10.0, fabs( screenSize.x ) );
            screenSize.y = std::max( 10.0, screenSize.y );
            double ratio = std::max( fabs( bbSize.x / screenSize.x ), fabs( bbSize.y / screenSize.y ) );

            // Try not to zoom on every cross-probe; it gets very noisy
            if( crossProbingSettings.zoom_to_fit && ( ratio < 0.5 || ratio > 1.0 ) )
                view->SetScale( view->GetScale() / ratio );
 #endif // DEFAULT_PCBNEW_CODE

#ifndef DEFAULT_PCBNEW_CODE  // Do the scaled zoom
            auto bbSize     = bbox.Inflate( bbox.GetWidth() * 0.2f ).GetSize();
            auto screenSize = view->ToWorld( GetCanvas()->GetClientSize(), false );

            // This code tries to come up with a zoom factor that doesn't simply zoom in
            // to the cross probed component, but instead shows a reasonable amount of the
            // circuit around it to provide context.  This reduces or eliminates the need
            // to manually change the zoom because it's too close.

            // Using the default text height as a constant to compare against, use the
            // height of the bounding box of visible items for a footprint to figure out
            // if this is a big footprint (like a processor) or a small footprint (like a resistor).
            // This ratio is not useful by itself as a scaling factor.  It must be "bent" to
            // provide good scaling at varying component sizes.  Bigger components need less
            // scaling than small ones.
            double currTextHeight = Millimeter2iu( DEFAULT_TEXT_SIZE );

            double compRatio     = bbSize.y / currTextHeight; // Ratio of component to text height
            double compRatioBent = 1.0; // This will end up as the scaling factor we apply to "ratio"

            // This is similar to the original KiCad code that scaled the zoom to make sure components
            // were visible on screen.  It's simply a ratio of screen size to component size, and its
            // job is to zoom in to make the component fullscreen.  Earlier in the code the
            // component BBox is given a 20% margin to add some breathing room. We compare
            // the height of this enlarged component bbox to the default text height.  If a component
            // will end up with the sides clipped, we adjust later to make sure it fits on screen.
            //
            // The "fabs" on x ensures the right answer when the view is flipped
            screenSize.x = std::max( 10.0, fabs( screenSize.x ) );
            screenSize.y = std::max( 10.0, screenSize.y );
            double ratio = std::max( -1.0, fabs( bbSize.y / screenSize.y ) );
            // Original KiCad code for how much to scale the zoom
            double kicadRatio = std::max( fabs( bbSize.x / screenSize.x ),
                                          fabs( bbSize.y / screenSize.y ) );

            // LUT to scale zoom ratio to provide reasonable schematic context.  Must work
            // with footprints of varying sizes (e.g. 0402 package and 200 pin BGA).
            // "first" is used as the input and "second" as the output
            //
            // "first" = compRatio (footprint height / default text height)
            // "second" = Amount to scale ratio by
            std::vector<std::pair<double, double>> lut{
                { 1, 8 },
                { 1.5, 5 },
                { 3, 3 },
                { 4.5, 2.5 },
                { 8, 2.0 },
                { 12, 1.7 },
                { 16, 1.5 },
                { 24, 1.3 },
                { 32, 1.0 },
            };


            std::vector<std::pair<double, double>>::iterator it;

            compRatioBent = lut.back().second; // Large component default

            if( compRatio >= lut.front().first )
            {
                // Use LUT to do linear interpolation of "compRatio" within "first", then
                // use that result to linearly interpolate "second" which gives the scaling
                // factor needed.

                for( it = lut.begin(); it < lut.end() - 1; it++ )
                {
                    if( it->first <= compRatio && next( it )->first >= compRatio )
                    {
                        double diffx = compRatio - it->first;
                        double diffn = next( it )->first - it->first;

                        compRatioBent =
                                it->second + ( next( it )->second - it->second ) * diffx / diffn;
                        break; // We have our interpolated value
                    }
                }
            }
            else
                compRatioBent = lut.front().second; // Small component default

            // If the width of the part we're probing is bigger than what the screen width will be
            // after the zoom, then punt and use the KiCad zoom algorithm since it guarantees the
            // part's width will be encompassed within the screen.  This will apply to parts that are
            // much wider than they are tall.

            if( bbSize.x > screenSize.x * ratio * compRatioBent )
            {
                ratio = kicadRatio; // Use standard KiCad zoom algorithm for parts too wide to fit screen
                compRatioBent = 1.0; // Reset so we don't modify the "KiCad" ratio
                wxLogTrace( "CROSS_PROBE_SCALE",
                        "Part TOO WIDE for screen.  Using normal KiCad zoom ratio: %1.5f", ratio );
            }

            // Now that "compRatioBent" holds our final scaling factor we apply it to the original
            // fullscreen zoom ratio to arrive at the final ratio itself.
            ratio *= compRatioBent;

            bool alwaysZoom = false; // DEBUG - allows us to minimize zooming or not
            // Try not to zoom on every cross-probe; it gets very noisy
            if( ( ratio < 0.5 || ratio > 1.0 ) || alwaysZoom )
                view->SetScale( view->GetScale() / ratio );
#endif // ifndef DEFAULT_PCBNEW_CODE
        }
        view->SetCenter( bbox.Centre() );
    }

    view->UpdateAllLayersColor();
    // Ensure the display is refreshed, because in some installs the refresh is done only
    // when the gal canvas has the focus, and that is not the case when crossprobing from
    // Eeschema:
    GetCanvas()->Refresh();
}


std::string FormatProbeItem( BOARD_ITEM* aItem )
{
    FOOTPRINT* footprint;

    if( !aItem )
        return "$CLEAR: \"HIGHLIGHTED\""; // message to clear highlight state

    switch( aItem->Type() )
    {
    case PCB_FOOTPRINT_T:
        footprint = (FOOTPRINT*) aItem;
        return StrPrintf( "$PART: \"%s\"", TO_UTF8( footprint->GetReference() ) );

    case PCB_PAD_T:
        {
            footprint = static_cast<FOOTPRINT*>( aItem->GetParent() );
            wxString pad = static_cast<PAD*>( aItem )->GetName();

            return StrPrintf( "$PART: \"%s\" $PAD: \"%s\"",
                              TO_UTF8( footprint->GetReference() ),
                              TO_UTF8( pad ) );
        }

    case PCB_FP_TEXT_T:
        {
            footprint = static_cast<FOOTPRINT*>( aItem->GetParent() );

            FP_TEXT*    text = static_cast<FP_TEXT*>( aItem );
            const char* text_key;

            /* This can't be a switch since the break need to pull out
             * from the outer switch! */
            if( text->GetType() == FP_TEXT::TEXT_is_REFERENCE )
                text_key = "$REF:";
            else if( text->GetType() == FP_TEXT::TEXT_is_VALUE )
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


/* Send a remote command to Eeschema via a socket,
 * aSyncItem = item to be located on schematic (footprint, pin or text)
 * Commands are
 * $PART: "reference"   put cursor on component anchor
 * $PART: "reference" $PAD: "pad number" put cursor on the component pin
 * $PART: "reference" $REF: "reference" put cursor on the component ref
 * $PART: "reference" $VAL: "value" put cursor on the component value
 */
void PCB_EDIT_FRAME::SendMessageToEESCHEMA( BOARD_ITEM* aSyncItem )
{
    std::string packet = FormatProbeItem( aSyncItem );

    if( !packet.empty() )
    {
        if( Kiface().IsSingle() )
            SendCommand( MSG_TO_SCH, packet );
        else
        {
            // Typically ExpressMail is going to be s-expression packets, but since
            // we have existing interpreter of the cross probe packet on the other
            // side in place, we use that here.
            Kiway().ExpressMail( FRAME_SCH, MAIL_CROSS_PROBE, packet, this );
        }
    }
}


void PCB_EDIT_FRAME::SendCrossProbeNetName( const wxString& aNetName )
{
    std::string packet = StrPrintf( "$NET: \"%s\"", TO_UTF8( aNetName ) );

    if( !packet.empty() )
    {
        if( Kiface().IsSingle() )
            SendCommand( MSG_TO_SCH, packet );
        else
        {
            // Typically ExpressMail is going to be s-expression packets, but since
            // we have existing interpreter of the cross probe packet on the other
            // side in place, we use that here.
            Kiway().ExpressMail( FRAME_SCH, MAIL_CROSS_PROBE, packet, this );
        }
    }
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
                    component->AddNet( pad->GetName(), netname, pad->GetPinFunction(),
                                       pad->GetPinType() );
                }
            }

            netlist.AddComponent( component );
        }

        netlist.Format( "pcb_netlist", &sf, 0, CTL_OMIT_FILTERS );
        payload = sf.GetString();
    }
        break;

    case MAIL_PCB_UPDATE_LINKS:
        try
        {
            NETLIST netlist;
            FetchNetlistFromSchematic( netlist, wxEmptyString );

            BOARD_NETLIST_UPDATER updater( this, GetBoard() );
            updater.SetLookupByTimestamp( false );
            updater.SetDeleteUnusedComponents ( false );
            updater.SetReplaceFootprints( false );
            updater.SetDeleteSinglePadNets( false );
            updater.SetWarnPadNoNetInNetlist( false );
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
    }
        break;

    // many many others.
    default:
        ;
    }
}

