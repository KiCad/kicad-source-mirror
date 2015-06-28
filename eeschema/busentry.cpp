/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
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
 * @file busentry.cpp
 * @brief Code to handle manipulation of bus entry objects.
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <class_drawpanel.h>
#include <eeschema_id.h>
#include <confirm.h>
#include <schframe.h>

#include <general.h>
#include <sch_bus_entry.h>


static int     s_LastShape = '\\';


SCH_BUS_BUS_ENTRY* SCH_EDIT_FRAME::CreateBusBusEntry()
{
    // Create and place a new bus entry at cursor position
    SCH_BUS_BUS_ENTRY* busEntry = new SCH_BUS_BUS_ENTRY( GetCrossHairPosition(), s_LastShape );

    busEntry->SetFlags( IS_NEW );
    GetScreen()->SetCurItem( busEntry );
    addCurrentItemToList();
    return busEntry;
}

SCH_BUS_WIRE_ENTRY* SCH_EDIT_FRAME::CreateBusWireEntry()
{
    // Create and place a new bus entry at cursor position
    SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( GetCrossHairPosition(), s_LastShape );

    busEntry->SetFlags( IS_NEW );
    GetScreen()->SetCurItem( busEntry );
    addCurrentItemToList();
    return busEntry;
}

/* set the shape of BusEntry (shape = / or \ )
 */
void SCH_EDIT_FRAME::SetBusEntryShape( wxDC* DC, SCH_BUS_ENTRY_BASE* BusEntry, char entry_shape )
{
    if( BusEntry == NULL )
        return;

    /* Put old item in undo list if it is not currently in edit */
    if( BusEntry->GetFlags() == 0 )
        SaveCopyInUndoList( BusEntry, UR_CHANGED );

    s_LastShape = entry_shape == '/' ? '/' : '\\';

    BusEntry->Draw( m_canvas, DC, wxPoint( 0, 0 ), g_XorMode );
    BusEntry->SetBusEntryShape( s_LastShape );
    GetScreen()->TestDanglingEnds();
    BusEntry->Draw( m_canvas, DC, wxPoint( 0, 0 ), g_XorMode );

    OnModify( );
}
