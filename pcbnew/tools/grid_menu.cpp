/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include "grid_menu.h"
#include <id.h>
#include <draw_frame.h>
#include <class_base_screen.h>
#include <tools/common_actions.h>

#include <boost/bind.hpp>

GRID_MENU::GRID_MENU( EDA_DRAW_FRAME* aParent ) : m_parent( aParent )
{
    BASE_SCREEN* screen = aParent->GetScreen();

    SetIcon( grid_select_xpm );
    SetMenuHandler( boost::bind( &GRID_MENU::EventHandler, this, _1 ) );
    SetUpdateHandler( boost::bind( &GRID_MENU::Update, this ) );

    wxArrayString gridsList;
    screen->BuildGridsChoiceList( gridsList, g_UserUnit != INCHES );

    for( unsigned int i = 0; i < gridsList.GetCount(); ++i )
    {
        GRID_TYPE& grid = screen->GetGrid( i );
        Append( grid.m_Id, gridsList[i], wxEmptyString, true );
    }
}


OPT_TOOL_EVENT GRID_MENU::EventHandler( const wxMenuEvent& aEvent )
{
    OPT_TOOL_EVENT event( COMMON_ACTIONS::gridPreset.MakeEvent() );
    long idx = aEvent.GetId() - ID_POPUP_GRID_SELECT - 1;
    event->SetParameter( idx );

    return event;
}


void GRID_MENU::Update()
{
    for( unsigned int i = 0; i < GetMenuItemCount(); ++i )
        Check( ID_POPUP_GRID_SELECT + 1 + i, false );

    Check( m_parent->GetScreen()->GetGridId(), true );
}
