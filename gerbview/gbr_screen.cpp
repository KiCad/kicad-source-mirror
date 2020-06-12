/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2014 Jean-Pierre Charras  jp.charras at wanadoo.fr
 * Copyright (C) 1992-2020 KiCad Developers, see change_log.txt for contributors.
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
#include <macros.h>
#include <gbr_screen.h>


/**
    Default GerbView zoom values.
    Roughly a 1.5 progression.
*/
static const double gbrZoomList[] =
{
    ZOOM_FACTOR( 0.022 ),
    ZOOM_FACTOR( 0.035 ),
    ZOOM_FACTOR( 0.05 ),
    ZOOM_FACTOR( 0.08 ),
    ZOOM_FACTOR( 0.13 ),
    ZOOM_FACTOR( 0.22 ),
    ZOOM_FACTOR( 0.35 ),
    ZOOM_FACTOR( 0.6 ),
    ZOOM_FACTOR( 1.0 ),
    ZOOM_FACTOR( 1.5 ),
    ZOOM_FACTOR( 2.2 ),
    ZOOM_FACTOR( 3.5 ),
    ZOOM_FACTOR( 5.0 ),
    ZOOM_FACTOR( 8.0 ),
    ZOOM_FACTOR( 13.0 ),
    ZOOM_FACTOR( 20.0 ),
    ZOOM_FACTOR( 35.0 ),
    ZOOM_FACTOR( 50.0 ),
    ZOOM_FACTOR( 80.0 ),
    ZOOM_FACTOR( 130.0 ),
    ZOOM_FACTOR( 220.0 )
};


GBR_SCREEN::GBR_SCREEN( const wxSize& aPageSizeIU ) :
    BASE_SCREEN( SCREEN_T )
{
    for( unsigned i = 0; i < arrayDim( gbrZoomList );  ++i )
        m_ZoomList.push_back( gbrZoomList[i] );

    SetZoom( ZOOM_FACTOR( 350 ) );  // a default value for zoom

    m_Active_Layer       = 0;       // default active layer = first graphic layer

    InitDataPoints( aPageSizeIU );
}
