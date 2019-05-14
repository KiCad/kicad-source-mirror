/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <pgm_base.h>
#include <kiface_i.h>
#include <kiway_express.h>
#include <macros.h>
#include <eda_dde.h>
#include <sch_draw_panel.h>
#include <tool/tool_manager.h>
#include <sch_edit_frame.h>
#include <general.h>
#include <eeschema_id.h>
#include <lib_draw_item.h>
#include <lib_pin.h>
#include <sch_component.h>
#include <sch_sheet.h>
#include <sch_view.h>
#include <reporter.h>
#include <netlist_exporters/netlist_exporter_kicad.h>
#include <tools/ee_actions.h>

/**
 * Execute a remote command sent by Pcbnew via a socket connection.
 * <p>
 * When user selects a module or pin in Pcbnew, Eeschema shows that same
 * component or pin and moves cursor on the item.  The socket port used
 * is #KICAD_SCH_PORT_SERVICE_NUMBER which defaults to 4243.
 *
 * Valid commands are:
 * \li \c \$PART: \c "reference" Put cursor on component.
 * \li \c \$PART: \c "reference" \c \$REF: \c "ref" Put cursor on component reference.
 * \li \c \$PART: \c "reference" \c \$VAL: \c "value" Put cursor on component value.
 * \li \c \$PART: \c "reference" \c \$PAD: \c "pin name" Put cursor on the component pin.
 * \li \c \$NET: \c "netname" Highlight a specified net
 * \li \c \$CLEAR: \c "HIGHLIGHTED" Clear components highlight
 * <p>
 * They are a keyword followed by a quoted string.
 * @param cmdline = received command from Pcbnew
 */
void SCH_EDIT_FRAME::ExecuteRemoteCommand( const char* cmdline )
{
    char     line[1024];

    strncpy( line, cmdline, sizeof(line) - 1 );
    line[ sizeof(line) - 1 ] = '\0';

    char* idcmd = strtok( line, " \n\r" );
    char* text  = strtok( NULL, "\"\n\r" );

    if( idcmd == NULL )
        return;

    if( strcmp( idcmd, "$NET:" ) == 0 )
    {
        if( GetToolId() == ID_HIGHLIGHT_TOOL )
        {
            m_SelectedNetName = FROM_UTF8( text );

            SetStatusText( _( "Selected net: " ) + UnescapeString( m_SelectedNetName ) );

            GetToolManager()->RunAction( EE_ACTIONS::updateNetHighlighting, true );
        }

        return;
    }

    if( strcmp( idcmd, "$CLEAR:" ) == 0 )
    {
        if( text && strcmp( text, "HIGHLIGHTED" ) == 0 )
        {
            GetCanvas()->GetView()->HighlightItem( nullptr, nullptr );
            GetCanvas()->Refresh();
        }

        return;
    }

    if( text == NULL )
        return;

    if( strcmp( idcmd, "$PART:" ) != 0 )
        return;

    wxString part_ref = FROM_UTF8( text );

    /* look for a complement */
    idcmd = strtok( NULL, " \n\r" );

    if( idcmd == NULL )    // Highlight component only (from Cvpcb or Pcbnew)
    {
        // Highlight component part_ref, or clear Highlight, if part_ref is not existing
        FindComponentAndItem( part_ref, true, FIND_COMPONENT_ONLY, wxEmptyString );
        return;
    }

    text = strtok( NULL, "\"\n\r" );

    if( text == NULL )
        return;

    wxString msg = FROM_UTF8( text );

    if( strcmp( idcmd, "$REF:" ) == 0 )
    {
        FindComponentAndItem( part_ref, true, FIND_REFERENCE, msg );
    }
    else if( strcmp( idcmd, "$VAL:" ) == 0 )
    {
        FindComponentAndItem( part_ref, true, FIND_VALUE, msg );
    }
    else if( strcmp( idcmd, "$PAD:" ) == 0 )
    {
        FindComponentAndItem( part_ref, true, FIND_PIN, msg );
    }
    else
    {
        FindComponentAndItem( part_ref, true, FIND_COMPONENT_ONLY, wxEmptyString );
    }
}


std::string FormatProbeItem( EDA_ITEM* aItem, SCH_COMPONENT* aComp )
{
    // This is a keyword followed by a quoted string.

    // Cross probing to Pcbnew if a pin or a component is found
    switch( aItem->Type() )
    {
    case LIB_PIN_T:
        wxFAIL_MSG( "What are we doing with LIB_* items here?" );
        break;

    case LIB_FIELD_T:
        wxFAIL_MSG( "What are we doing with LIB_* items here?" );
        // fall through to SCH_FIELD_T:

    case SCH_FIELD_T:
        if( aComp )
            return StrPrintf( "$PART: \"%s\"", TO_UTF8( aComp->GetField( REFERENCE )->GetText() ) );
        break;

    case SCH_COMPONENT_T:
        aComp = (SCH_COMPONENT*) aItem;
        return StrPrintf( "$PART: \"%s\"", TO_UTF8( aComp->GetField( REFERENCE )->GetText() ) );

    case SCH_SHEET_T:
        {
        SCH_SHEET* sheet = (SCH_SHEET*)aItem;
        return StrPrintf( "$SHEET: \"%8.8lX\"", (unsigned long) sheet->GetTimeStamp() );
        }

    case SCH_PIN_T:
        {
            SCH_PIN* pin = (SCH_PIN*) aItem;
            aComp = pin->GetParentComponent();

            if( !pin->GetNumber().IsEmpty() )
            {
                return StrPrintf( "$PIN: \"%s\" $PART: \"%s\"",
                                  TO_UTF8( pin->GetNumber() ),
                                  TO_UTF8( aComp->GetField( REFERENCE )->GetText() ) );
            }
            else
            {
                return StrPrintf( "$PART: \"%s\"",
                                  TO_UTF8( aComp->GetField( REFERENCE )->GetText() ) );
            }
        }

    default:
        break;
    }

    return "";
}


void SCH_EDIT_FRAME::SendMessageToPCBNEW( EDA_ITEM* aObjectToSync, SCH_COMPONENT* aLibItem )
{
    wxASSERT( aObjectToSync );     // fix the caller

    if( !aObjectToSync )
        return;

    std::string packet = FormatProbeItem( aObjectToSync, aLibItem );

    if( packet.size() )
    {
        if( Kiface().IsSingle() )
            SendCommand( MSG_TO_PCB, packet.c_str() );
        else
        {
            // Typically ExpressMail is going to be s-expression packets, but since
            // we have existing interpreter of the cross probe packet on the other
            // side in place, we use that here.
            Kiway().ExpressMail( FRAME_PCB, MAIL_CROSS_PROBE, packet, this );
        }
    }
}


void SCH_EDIT_FRAME::SendCrossProbeNetName( const wxString& aNetName )
{
    // The command is a keyword followed by a quoted string.

    std::string packet = StrPrintf( "$NET: \"%s\"", TO_UTF8( aNetName ) );

    if( packet.size() )
    {
        if( Kiface().IsSingle() )
            SendCommand( MSG_TO_PCB, packet.c_str() );
        else
        {
            // Typically ExpressMail is going to be s-expression packets, but since
            // we have existing interpreter of the cross probe packet on the other
            // side in place, we use that here.
            Kiway().ExpressMail( FRAME_PCB, MAIL_CROSS_PROBE, packet, this );
        }
    }
}


void SCH_EDIT_FRAME::SendCrossProbeClearHighlight()
{
    std::string packet = "$CLEAR\n";

    if( Kiface().IsSingle() )
        SendCommand( MSG_TO_PCB, packet.c_str() );
    else
    {
        // Typically ExpressMail is going to be s-expression packets, but since
        // we have existing interpreter of the cross probe packet on the other
        // side in place, we use that here.
        Kiway().ExpressMail( FRAME_PCB, MAIL_CROSS_PROBE, packet, this );
    }
}


void SCH_EDIT_FRAME::KiwayMailIn( KIWAY_EXPRESS& mail )
{
    std::string& payload = mail.GetPayload();

    switch( mail.Command() )
    {
    case MAIL_CROSS_PROBE:
        ExecuteRemoteCommand( payload.c_str() );
        break;

    case MAIL_SCH_GET_NETLIST:
        if( payload.find( "quiet-annotate" ) != std::string::npos )
        {
            SCH_SCREENS schematic;
            schematic.UpdateSymbolLinks();
            SCH_SHEET_LIST sheets( g_RootSheet );
            sheets.AnnotatePowerSymbols();
            AnnotateComponents( true, UNSORTED, INCREMENTAL_BY_REF, 0, false, false, true,
                                NULL_REPORTER::GetInstance() );
        }

        if( payload.find( "no-annotate" ) == std::string::npos )
        {
            // Ensure schematic is OK for netlist creation (especially that it is fully annotated):
            if( !prepareForNetlist() )
                return;
        }

        {
            NETLIST_OBJECT_LIST* net_atoms = BuildNetListBase();
            NETLIST_EXPORTER_KICAD exporter( this, net_atoms, g_ConnectionGraph );
            STRING_FORMATTER formatter;

            exporter.Format( &formatter, GNL_ALL );

            payload = formatter.GetString();
        }
        break;

    case MAIL_BACKANNOTATE_FOOTPRINTS:
        try
        {
            backAnnotateFootprints( payload );
        }
        catch( const IO_ERROR& DBG( ioe ) )
        {
            DBG( printf( "%s: ioe:%s\n", __func__, TO_UTF8( ioe.What() ) );)
        }
        break;

    case MAIL_SCH_REFRESH:
    {
        SCH_SCREENS schematic;
        schematic.UpdateSymbolLinks();
        schematic.TestDanglingEnds();

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
    }
        break;

    case MAIL_SCH_SAVE:
        if( SaveProject() )
            payload = "success";
        break;

    default:
        ;
    }
}
