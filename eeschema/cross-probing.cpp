/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
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

/**
 * @file eeschema/cross-probing.cpp
 */

#include <fctsys.h>
#include <pgm_base.h>
#include <kiface_i.h>
#include <kiway_express.h>
#include <macros.h>
#include <eda_dde.h>
#include <schframe.h>

#include <general.h>
#include <eeschema_id.h>
#include <lib_draw_item.h>
#include <lib_pin.h>
#include <sch_component.h>


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
 * <p>
 * @param cmdline = received command from Pcbnew
 */
void SCH_EDIT_FRAME::ExecuteRemoteCommand( const char* cmdline )
{
    char     line[1024];

    strncpy( line, cmdline, sizeof(line) - 1 );
    line[ sizeof(line) - 1 ] = '\0';

    char* idcmd = strtok( line, " \n\r" );
    char* text  = strtok( NULL, "\"\n\r" );

    if( (idcmd == NULL) || (text == NULL) )
        return;

    if( strcmp( idcmd, "$PART:" ) != 0 )
        return;

    wxString part_ref = FROM_UTF8( text );

    /* look for a complement */
    idcmd = strtok( NULL, " \n\r" );

    if( idcmd == NULL )    // component only
    {
        FindComponentAndItem( part_ref, true, FIND_COMPONENT_ONLY, wxEmptyString, false );
        return;
    }

    text = strtok( NULL, "\"\n\r" );

    if( text == NULL )
        return;

    wxString msg = FROM_UTF8( text );

    if( strcmp( idcmd, "$REF:" ) == 0 )
    {
        FindComponentAndItem( part_ref, true, FIND_REFERENCE, msg, false );
    }
    else if( strcmp( idcmd, "$VAL:" ) == 0 )
    {
        FindComponentAndItem( part_ref, true, FIND_VALUE, msg, false );
    }
    else if( strcmp( idcmd, "$PAD:" ) == 0 )
    {
        FindComponentAndItem( part_ref, true, FIND_PIN, msg, false );
    }
    else
    {
        FindComponentAndItem( part_ref, true, FIND_COMPONENT_ONLY, wxEmptyString, false );
    }
}


std::string FormatProbeItem( EDA_ITEM* aComponent, SCH_COMPONENT* aPart )
{
    // Cross probing to Pcbnew if a pin or a component is found
    switch( aComponent->Type() )
    {
    case SCH_FIELD_T:
    case LIB_FIELD_T:
        {
            if( !aPart )
                break;

            return StrPrintf( "$PART: %s", TO_UTF8( aPart->GetField( REFERENCE )->GetText() ) );
        }
        break;

    case SCH_COMPONENT_T:
        aPart = (SCH_COMPONENT*) aComponent;
        return StrPrintf( "$PART: %s", TO_UTF8( aPart->GetField( REFERENCE )->GetText() ) );

    case LIB_PIN_T:
        {
            if( !aPart )
                break;

            LIB_PIN* pin = (LIB_PIN*) aComponent;

            if( pin->GetNumber() )
            {
                wxString pinnum;

                pin->PinStringNum( pinnum );

                return StrPrintf( "$PIN: %s $PART: %s", TO_UTF8( pinnum ),
                         TO_UTF8( aPart->GetField( REFERENCE )->GetText() ) );
            }
            else
            {
                return StrPrintf( "$PART: %s", TO_UTF8( aPart->GetField( REFERENCE )->GetText() ) );
            }
        }
        break;

    default:
        break;
    }

    return "";
}


void SCH_EDIT_FRAME::SendMessageToPCBNEW( EDA_ITEM* aComponent, SCH_COMPONENT* aPart )
{
#if 1
    wxASSERT( aComponent );     // fix the caller

#else  // WTF?
    if( !aComponent )           // caller remains eternally stupid.
        return;
#endif

    std::string packet = FormatProbeItem( aComponent, aPart );

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


void SCH_EDIT_FRAME::KiwayMailIn( KIWAY_EXPRESS& mail )
{
    const std::string& payload = mail.GetPayload();

    switch( mail.Command() )
    {
    case MAIL_CROSS_PROBE:
        ExecuteRemoteCommand( payload.c_str() );
        break;

    case MAIL_BACKANNOTATE_FOOTPRINTS:
        try
        {
            backAnnotateFootprints( payload );
        }
        catch( const IO_ERROR& ioe )
        {
            DBG( printf( "%s: ioe:%s\n", __func__, TO_UTF8( ioe.errorText ) );)
        }
        break;

    // many many others.

    }
}


