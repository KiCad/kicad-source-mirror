/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <pcb_screen.h>
#include <base_units.h>
#include <layers_id_colors_and_visibility.h>


#define ZOOM_FACTOR( x )       ( x * IU_PER_MILS / 10 )

/**
    Default Pcbnew zoom values.
    Limited to 19 values to keep a decent size to menus.
    Roughly a 1.5 progression.
    The last 2 values are handy when somebody uses a library import of a module
    (or foreign data) which has a bad coordinate.
    Also useful in GerbView for this reason.
    Zoom 5 and 10 can create artefacts when drawing (integer overflow in low level graphic
    functions )
*/
static const double pcbZoomList[] =
{
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
    ZOOM_FACTOR( 220.0 ),
    ZOOM_FACTOR( 350.0 )
/*
    The largest distance that wx can support is INT_MAX, since it represents
    distance often in a wxCoord or wxSize. As a scalar, a distance is always
    positive. On most machines which run KiCad, int is 32 bits and INT_MAX is
    2147483647. The most difficult distance for a virtual (world) cartesian
    space is the hypotenuse, or diagonal measurement at a 45 degree angle. This
    puts the most stress on the distance magnitude within the bounded virtual
    space. So if we allow this distance to be our constraint of <= INT_MAX, this
    constraint then propagates to the maximum distance in X and in Y that can be
    supported on each axis. Remember that the hypotenuse of a 1x1 square is
    sqrt( 1x1 + 1x1 ) = sqrt(2) = 1.41421356.

    hypotenuse of any square = sqrt(2) * deltaX;

    Let maximum supported hypotenuse be INT_MAX, then:

    MAX_AXIS = INT_MAX / sqrt(2) = 2147483647 / 1.41421356 = 1518500251

    This maximum distance is imposed by wxWidgets, not by KiCad. The imposition
    comes in the form of the data structures used in the graphics API at the
    wxDC level. Obviously when we are not interacting with wx we can use double
    to compute distances larger than this. For example the computation of the
    total length of a net, can and should be done in double, since it might
    actually be longer than a single diagonal line.

    The next choice is what to use for internal units (IU), sometimes called
    world units.  If nanometers, then the virtual space must be limited to
    about 1.5 x 1.5 meters square.  This is 1518500251 divided by 1e9 nm/meter.

    The maximum zoom factor then depends on the client window size.  If we ask
    wx to handle something outside INT_MIN to INT_MAX, there are unreported
    problems in the non-Debug build because wxRound() goes silent.

    Let:
    const double MAX_AXIS = 1518500251;

    Then a maximum zoom factor for a screen of 1920 pixels wide is
        1518500251 / 1920 = 790885.

    The largest ZOOM_FACTOR in above table is ZOOM_FACTOR( 300 ), which computes
    out to 762000 just below 790885.
*/
};


PCB_SCREEN::PCB_SCREEN( const wxSize& aPageSizeIU ) :
    BASE_SCREEN( SCREEN_T )
{
    for( double zoom : pcbZoomList )
        m_ZoomList.push_back( zoom );

    m_Active_Layer       = F_Cu;     // default active layer = front layer
    m_Route_Layer_TOP    = F_Cu;     // default layers pair for vias (bottom to top)
    m_Route_Layer_BOTTOM = B_Cu;

    InitDataPoints( aPageSizeIU );
}


PCB_SCREEN::~PCB_SCREEN()
{
    ClearUndoRedoList();
}
