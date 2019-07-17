/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <pgm_base.h>
#include <kiface_i.h>
#include <kiway_express.h>
#include <pcb_edit_frame.h>
#include <eda_dde.h>
#include <macros.h>
#include <pcbnew_id.h>
#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_zone.h>
#include <collectors.h>
#include <pcbnew.h>
#include <pcb_netlist.h>
#include <tools/pcb_actions.h>
#include <tool/tool_manager.h>
#include <tools/selection_tool.h>
#include <pcb_painter.h>

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
    MODULE*     module = NULL;
    D_PAD*      pad = NULL;
    BOARD*      pcb = GetBoard();

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
        wxString net_name = FROM_UTF8( text );

        NETINFO_ITEM* netinfo = pcb->FindNet( net_name );

        if( netinfo )
        {
            netcode = netinfo->GetNet();

            MSG_PANEL_ITEMS items;
            netinfo->GetMsgPanelInfo( GetUserUnits(), items );
            SetMsgPanel( items );
        }
    }
    else if( strcmp( idcmd, "$PIN:" ) == 0 )
    {
        wxString pinName = FROM_UTF8( text );

        text = strtok( NULL, " \n\r" );

        if( text && strcmp( text, "$PART:" ) == 0 )
            text = strtok( NULL, "\"\n\r" );

        modName = FROM_UTF8( text );

        module = pcb->FindModuleByReference( modName );

        if( module )
            pad = module->FindPadByName( pinName );

        if( pad )
            netcode = pad->GetNetCode();

        if( module == NULL )
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

        module = pcb->FindModuleByReference( modName );

        if( module )
            msg.Printf( _( "%s found" ), modName );
        else
            msg.Printf( _( "%s not found" ), modName );

        SetStatusText( msg );
    }
    else if( strcmp( idcmd, "$SHEET:" ) == 0 )
    {
        msg.Printf( _( "Selecting all from sheet \"%s\"" ), FROM_UTF8( text ) );
        wxString sheetStamp( FROM_UTF8( text ) );
        SetStatusText( msg );
        GetToolManager()->RunAction( PCB_ACTIONS::selectOnSheetFromEeschema, true,
                                     static_cast<void*>( &sheetStamp ) );
        return;
    }
    else if( strcmp( idcmd, "$CLEAR" ) == 0 )
    {
        renderSettings->SetHighlight( false );
        view->UpdateAllLayersColor();

        pcb->ResetNetHighLight();
        SetMsgPanel( pcb );

        GetCanvas()->Refresh();
        return;
    }

    BOX2I bbox = { { 0, 0 }, { 0, 0 } };

    if( module )
    {
        m_toolManager->RunAction( PCB_ACTIONS::highlightItem, true, (void*) module );
        bbox = module->GetBoundingBox();
    }
    else if( netcode > 0 )
    {
        renderSettings->SetHighlight( ( netcode >= 0 ), netcode );

        pcb->SetHighLightNet( netcode );

        auto merge_area = [netcode, &bbox]( BOARD_CONNECTED_ITEM* aItem )
        {
            if( aItem->GetNetCode() == netcode )
            {
                if( bbox.GetWidth() == 0 )
                    bbox = aItem->GetBoundingBox();
                else
                    bbox.Merge( aItem->GetBoundingBox() );
            }
        };

        for( auto zone : pcb->Zones() )
            merge_area( zone );

        for( auto track : pcb->Tracks() )
            merge_area( track );

        for( auto mod : pcb->Modules() )
            for ( auto mod_pad : mod->Pads() )
                merge_area( mod_pad );
    }
    else
    {
        renderSettings->SetHighlight( false );
    }

    if( bbox.GetWidth() > 0 && bbox.GetHeight() > 0 )
    {
        auto bbSize = bbox.Inflate( bbox.GetWidth() * 0.2f ).GetSize();
        auto screenSize = view->ToWorld( GetCanvas()->GetClientSize(), false );
        double ratio = std::max( fabs( bbSize.x / screenSize.x ),
                                 fabs( bbSize.y / screenSize.y ) );

        // Try not to zoom on every cross-probe; it gets very noisy
        if( ratio < 0.1 || ratio > 1.0 )
            view->SetScale( view->GetScale() / ratio );

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
    MODULE*     module;

    if( !aItem )
        return "$CLEAR: \"HIGHLIGHTED\""; // message to clear highlight state

    switch( aItem->Type() )
    {
    case PCB_MODULE_T:
        module = (MODULE*) aItem;
        return StrPrintf( "$PART: \"%s\"", TO_UTF8( module->GetReference() ) );

    case PCB_PAD_T:
        {
            module = (MODULE*) aItem->GetParent();
            wxString pad = ((D_PAD*)aItem)->GetName();

            return StrPrintf( "$PART: \"%s\" $PAD: \"%s\"",
                              TO_UTF8( module->GetReference() ),
                              TO_UTF8( pad ) );
        }

    case PCB_MODULE_TEXT_T:
        {
            module = static_cast<MODULE*>( aItem->GetParent() );

            TEXTE_MODULE*   text_mod = static_cast<TEXTE_MODULE*>( aItem );

            const char*     text_key;

            /* This can't be a switch since the break need to pull out
             * from the outer switch! */
            if( text_mod->GetType() == TEXTE_MODULE::TEXT_is_REFERENCE )
                text_key = "$REF:";
            else if( text_mod->GetType() == TEXTE_MODULE::TEXT_is_VALUE )
                text_key = "$VAL:";
            else
                break;

            return StrPrintf( "$PART: \"%s\" %s \"%s\"",
                              TO_UTF8( module->GetReference() ),
                              text_key,
                              TO_UTF8( text_mod->GetText() ) );
        }

    default:
        break;
    }

    return "";
}


/* Send a remote command to Eeschema via a socket,
 * aSyncItem = item to be located on schematic (module, pin or text)
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
            SendCommand( MSG_TO_SCH, packet.c_str() );
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
            SendCommand( MSG_TO_SCH, packet.c_str() );
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
    const std::string& payload = mail.GetPayload();

    switch( mail.Command() )
    {
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

    // many many others.
    default:
        ;
    }
}

