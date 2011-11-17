/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2006 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file sch_item_struct.cpp
 */

#include "fctsys.h"
#include "common.h"
#include "gr_basic.h"
#include "base_struct.h"
#include "sch_item_struct.h"
#include "class_sch_screen.h"
#include "class_drawpanel.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "protos.h"

#include "../eeschema/dialogs/dialog_schematic_find.h"


bool sort_schematic_items( const SCH_ITEM* aItem1, const SCH_ITEM* aItem2 )
{
    return *aItem1 < *aItem2;
}


/* Constructor and destructor for SCH_ITEM */
/* They are not inline because this creates problems with gcc at linking time
 * in debug mode
 */

SCH_ITEM::SCH_ITEM( EDA_ITEM* aParent, KICAD_T aType ) :
    EDA_ITEM( aParent, aType )
{
    m_Layer = 0;
}


SCH_ITEM::SCH_ITEM( const SCH_ITEM& aItem ) :
    EDA_ITEM( aItem )
{
    m_Layer = aItem.m_Layer;
}


SCH_ITEM::~SCH_ITEM()
{
    // Do not let the connections container go out of scope with any objects or they
    // will be deleted by the container will cause the Eeschema to crash.  These objects
    // are owned by the sheet object container.
    if( !m_connections.empty() )
        m_connections.clear();
}


void SCH_ITEM::Place( SCH_EDIT_FRAME* aFrame, wxDC* aDC )
{
    SCH_SCREEN* screen = aFrame->GetScreen();

    if( IsNew() )
    {
        if( !screen->CheckIfOnDrawList( this ) )  // don't want a loop!
            screen->AddToDrawList( this );

        aFrame->SetRepeatItem( this );
        aFrame->SaveCopyInUndoList( this, UR_NEW );
    }
    else
    {
        aFrame->SaveUndoItemInUndoList( this );
    }

    m_Flags = 0;
    screen->SetModify();
    screen->SetCurItem( NULL );
    aFrame->DrawPanel->SetMouseCapture( NULL, NULL );
    aFrame->DrawPanel->EndMouseCapture();

    if( aDC )
    {
        EDA_CROSS_HAIR_MANAGER( aFrame->DrawPanel, aDC );  // Erase schematic cursor
        Draw( aFrame->DrawPanel, aDC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
    }
}


bool SCH_ITEM::IsConnected( const wxPoint& aPosition ) const
{
    if( m_Flags & STRUCT_DELETED || m_Flags & SKIP_STRUCT )
        return false;

    return doIsConnected( aPosition );
}


void SCH_ITEM::SwapData( SCH_ITEM* aItem )
{
    wxFAIL_MSG( wxT( "SwapData() method not implemented for class " ) + GetClass() );
}


bool SCH_ITEM::operator < ( const SCH_ITEM& aItem ) const
{
    wxCHECK_MSG( false, this->Type() < aItem.Type(),
                 wxT( "Less than operator not defined for " ) + GetClass() );
}


void SCH_ITEM::doPlot( PLOTTER* aPlotter )
{
    wxFAIL_MSG( wxT( "doPlot() method not implemented for class " ) + GetClass() );
}
