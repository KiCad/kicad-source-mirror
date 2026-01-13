/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <tool/grid_menu.h>
#include <id.h>
#include <eda_draw_frame.h>
#include <settings/app_settings.h>
#include <tool/actions.h>
#include <bitmaps.h>
#include <base_units.h>

using namespace std::placeholders;

GRID_MENU::GRID_MENU( EDA_DRAW_FRAME* aParent ) :
        ACTION_MENU( true ),
        m_parent( aParent )
{
    UpdateTitle();
    SetIcon( BITMAPS::grid_select );
    update();
}


OPT_TOOL_EVENT GRID_MENU::eventHandler( const wxMenuEvent& aEvent )
{
    OPT_TOOL_EVENT event( ACTIONS::gridPreset.MakeEvent() );
    event->SetParameter<int>( aEvent.GetId() - ID_POPUP_GRID_START );
    return event;
}


void GRID_MENU::UpdateTitle()
{
    SetTitle( _( "Grid" ) );
}


void GRID_MENU::update()
{
    WINDOW_SETTINGS* cfg = m_parent->GetWindowSettings( m_parent->config() );
    unsigned int     current = cfg->grid.last_size_idx + ID_POPUP_GRID_START;
    wxArrayString    gridsList;
    int              i = ID_POPUP_GRID_START;

    GRID_MENU::BuildChoiceList( &gridsList, cfg, m_parent );

    while( GetMenuItemCount() > 0 )
        Delete( FindItemByPosition( 0 ) );

    Add( ACTIONS::gridOrigin );
    AppendSeparator();

    for( const wxString& grid : gridsList )
    {
        int idx = i++;
        Append( idx, grid, wxEmptyString, wxITEM_CHECK )->Check( idx == (int) current );
    }
}


void GRID_MENU::BuildChoiceList( wxArrayString* aGridsList, WINDOW_SETTINGS* aCfg, EDA_DRAW_FRAME* aParent )
{
    wxString     msg;
    EDA_IU_SCALE scale = aParent->GetIuScale();
    EDA_UNITS    primaryUnit;
    EDA_UNITS    secondaryUnit;

    aParent->GetUnitPair( primaryUnit, secondaryUnit );

    for( GRID& gridSize : aCfg->grid.grids )
    {
        wxString name;

        if( !gridSize.name.IsEmpty() )
            name = gridSize.name + ": ";

        msg.Printf( _( "%s%s (%s)" ), name, gridSize.MessageText( scale, primaryUnit, true ),
                    gridSize.MessageText( scale, secondaryUnit, true ) );

        aGridsList->Add( msg );
    }
}
