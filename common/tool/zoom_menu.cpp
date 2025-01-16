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

#include <tool/zoom_menu.h>
#include <id.h>
#include <eda_draw_frame.h>
#include <settings/app_settings.h>
#include <tool/actions.h>
#include <gal/graphics_abstraction_layer.h>
#include <bitmaps.h>
#include <functional>

using namespace std::placeholders;

ZOOM_MENU::ZOOM_MENU( EDA_DRAW_FRAME* aParent ) :
        ACTION_MENU( true ),
        m_parent( aParent )
{
    UpdateTitle();
    SetIcon( BITMAPS::zoom_selection );
}


OPT_TOOL_EVENT ZOOM_MENU::eventHandler( const wxMenuEvent& aEvent )
{
    OPT_TOOL_EVENT event( ACTIONS::zoomPreset.MakeEvent() );
    event->SetParameter<int>( aEvent.GetId() - ID_POPUP_ZOOM_LEVEL_START );
    return event;
}


void ZOOM_MENU::UpdateTitle()
{
    SetTitle( _( "Zoom" ) );
}


void ZOOM_MENU::update()
{
    Clear();

    int ii = ID_POPUP_ZOOM_LEVEL_START + 1;  // 0 reserved for menus which support auto-zoom

    for( double factor : m_parent->config()->m_Window.zoom_factors )
        Append( ii++, wxString::Format( _( "Zoom: %.2f" ), factor ), wxEmptyString, wxITEM_CHECK );

    double zoom = m_parent->GetCanvas()->GetGAL()->GetZoomFactor();

    const std::vector<double>& zoomList = m_parent->config()->m_Window.zoom_factors;

    for( size_t jj = 0; jj < zoomList.size(); ++jj )
    {
        // Search for a value near the current zoom setting:
        double rel_error = std::fabs( zoomList[jj] - zoom ) / zoom;

        // IDs start with 1 (leaving 0 for auto-zoom)
        Check( ID_POPUP_ZOOM_LEVEL_START + jj + 1, rel_error < 0.1 );
    }
}
