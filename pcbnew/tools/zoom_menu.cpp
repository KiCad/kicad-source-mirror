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

#include "zoom_menu.h"
#include <id.h>
#include <draw_frame.h>
#include <class_base_screen.h>
#include <tools/common_actions.h>

#include <boost/bind.hpp>

ZOOM_MENU::ZOOM_MENU( EDA_DRAW_FRAME* aParent ) : m_parent( aParent )
{
    BASE_SCREEN* screen = aParent->GetScreen();

    SetIcon( zoom_selection_xpm );
    SetMenuHandler( boost::bind( &ZOOM_MENU::EventHandler, this, _1 ) );
    SetUpdateHandler( boost::bind( &ZOOM_MENU::Update, this ) );

    //int zoom = screen->GetZoom();
    int maxZoomIds = std::min( ID_POPUP_ZOOM_LEVEL_END - ID_POPUP_ZOOM_LEVEL_START,
                               (int) screen->m_ZoomList.size() );

    for( int i = 0; i < maxZoomIds; ++i )
    {
        Append( ID_POPUP_ZOOM_LEVEL_START + i,
            wxString::Format( _( "Zoom: %.2f" ), aParent->GetZoomLevelCoeff() / screen->m_ZoomList[i] ),
            wxEmptyString, wxITEM_CHECK );
    }
}


OPT_TOOL_EVENT ZOOM_MENU::EventHandler( const wxMenuEvent& aEvent )
{
    OPT_TOOL_EVENT event( COMMON_ACTIONS::zoomPreset.MakeEvent() );
    long idx = aEvent.GetId() - ID_POPUP_ZOOM_LEVEL_START;
    event->SetParameter( idx );

    return event;
}


void ZOOM_MENU::Update()
{
    double zoom = m_parent->GetScreen()->GetZoom();
    const std::vector<double>& zoomList = m_parent->GetScreen()->m_ZoomList;

    for( unsigned int i = 0; i < GetMenuItemCount(); ++i )
        Check( ID_POPUP_ZOOM_LEVEL_START + i, zoomList[i] == zoom );
}
