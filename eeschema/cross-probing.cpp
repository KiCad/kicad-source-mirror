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
#include <appl_wxstruct.h>
#include <macros.h>
#include <eda_dde.h>
#include <wxEeschemaStruct.h>

#include <general.h>
#include <eeschema_id.h>
#include <protos.h>
#include <lib_draw_item.h>
#include <lib_pin.h>
#include <sch_component.h>


/**
 * Function RemoteCommand
 * read a remote command sent by Pcbnew via a socket connection.
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
void RemoteCommand( const char* cmdline )
{
    char     line[1024];
    char*    idcmd;
    char*    text;
    wxString part_ref, msg;
    SCH_EDIT_FRAME* frame;

    frame = (SCH_EDIT_FRAME*)wxGetApp().GetTopWindow();

    strncpy( line, cmdline, sizeof(line) - 1 );

    idcmd = strtok( line, " \n\r" );
    text  = strtok( NULL, "\"\n\r" );

    if( (idcmd == NULL) || (text == NULL) )
        return;

    if( strcmp( idcmd, "$PART:" ) != 0 )
        return;

    part_ref = FROM_UTF8( text );

    /* look for a complement */
    idcmd = strtok( NULL, " \n\r" );

    if( idcmd == NULL )    // component only
    {
        frame->FindComponentAndItem( part_ref, true, FIND_COMPONENT_ONLY, wxEmptyString, false );
        return;
    }

    text = strtok( NULL, "\"\n\r" );

    if( text == NULL )
        return;

    msg = FROM_UTF8( text );

    if( strcmp( idcmd, "$REF:" ) == 0 )
    {
        frame->FindComponentAndItem( part_ref, true, FIND_REFERENCE, msg, false );
    }
    else if( strcmp( idcmd, "$VAL:" ) == 0 )
    {
        frame->FindComponentAndItem( part_ref, true, FIND_VALUE, msg, false );
    }
    else if( strcmp( idcmd, "$PAD:" ) == 0 )
    {
        frame->FindComponentAndItem( part_ref, true, FIND_PIN, msg, false );
    }
    else
    {
        frame->FindComponentAndItem( part_ref, true, FIND_COMPONENT_ONLY, wxEmptyString, false );
    }
}


void SCH_EDIT_FRAME::SendMessageToPCBNEW( EDA_ITEM* objectToSync, SCH_COMPONENT* LibItem )
{
    if( objectToSync == NULL )
        return;

    LIB_PIN* Pin = NULL;
    char     Line[1024];

    /* Cross probing to Pcbnew if a pin or a component is found */
    switch( objectToSync->Type() )
    {
    case SCH_FIELD_T:
    case LIB_FIELD_T:
    {
        if( LibItem == NULL )
            break;

        sprintf( Line, "$PART: %s", TO_UTF8( LibItem->GetField( REFERENCE )->m_Text ) );
        SendCommand( MSG_TO_PCB, Line );
    }
    break;

    case SCH_COMPONENT_T:
        LibItem = (SCH_COMPONENT*) objectToSync;
        sprintf( Line, "$PART: %s", TO_UTF8( LibItem->GetField( REFERENCE )->m_Text ) );
        SendCommand( MSG_TO_PCB, Line );
        break;

    case LIB_PIN_T:
        if( LibItem == NULL )
            break;

        Pin = (LIB_PIN*) objectToSync;

        if( Pin->GetNumber() )
        {
            wxString pinnum;
            Pin->ReturnPinStringNum( pinnum );
            sprintf( Line, "$PIN: %s $PART: %s", TO_UTF8( pinnum ),
                     TO_UTF8( LibItem->GetField( REFERENCE )->m_Text ) );
        }
        else
        {
            sprintf( Line, "$PART: %s", TO_UTF8( LibItem->GetField( REFERENCE )->m_Text ) );
        }

        SendCommand( MSG_TO_PCB, Line );
        break;

    default:
        break;
    }
}
