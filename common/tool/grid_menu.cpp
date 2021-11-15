/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
 * Copyright (C) 2015-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
    SetTitle( _( "Grid" ) );
    SetIcon( BITMAPS::grid_select );

    APP_SETTINGS_BASE* settings = m_parent->config();
    wxArrayString      gridsList;
    int                i = ID_POPUP_GRID_START;

    BuildChoiceList( &gridsList, settings, m_parent );

    for( const wxString& grid : gridsList )
        Append( i++, grid, wxEmptyString, wxITEM_CHECK );
}


OPT_TOOL_EVENT GRID_MENU::eventHandler( const wxMenuEvent& aEvent )
{
    OPT_TOOL_EVENT event( ACTIONS::gridPreset.MakeEvent() );
    event->SetParameter( (intptr_t) aEvent.GetId() - ID_POPUP_GRID_START );
    return event;
}


void GRID_MENU::update()
{
    APP_SETTINGS_BASE* settings = m_parent->config();
    unsigned int       current = settings->m_Window.grid.last_size_idx;
    wxArrayString      gridsList;

    GRID_MENU::BuildChoiceList( &gridsList, settings, m_parent );

    for( unsigned int i = 0; i < GetMenuItemCount(); ++i )
    {
        wxMenuItem* menuItem = FindItemByPosition( i );

        menuItem->SetItemLabel( gridsList[ i ] );  // Refresh label in case units have changed
        menuItem->Check( i == current );           // Refresh checkmark
    }
}

void GRID_MENU::BuildChoiceList( wxArrayString* aGridsList, APP_SETTINGS_BASE* aCfg,
                                 EDA_DRAW_FRAME* aParent )
{
    wxString msg;

    EDA_UNITS primaryUnit;
    EDA_UNITS secondaryUnit;

    aParent->GetUnitPair( primaryUnit, secondaryUnit );

    for( const wxString& gridSize : aCfg->m_Window.grid.sizes )
    {
        int val = (int) ValueFromString( EDA_UNITS::MILLIMETRES, gridSize );

        msg = _( "Grid" ) + wxString::Format( wxT( ": %s (%s)" ),
                                              MessageTextFromValue( primaryUnit, val ),
                                              MessageTextFromValue( secondaryUnit, val ) );

        aGridsList->Add( msg );
    }

    if( !aCfg->m_Window.grid.user_grid_x.empty() )
    {
        int val = (int) ValueFromString( EDA_UNITS::INCHES, aCfg->m_Window.grid.user_grid_x );

        msg.Printf( _( "User grid: %s (%s)" ),
                    MessageTextFromValue( primaryUnit, val ),
                    MessageTextFromValue( secondaryUnit, val ) );

        aGridsList->Add( msg );
    }
}
