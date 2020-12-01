/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#pragma once

/*  ZOOM LIMITS

    The largest distance that we (and Kicad) can support is INT_MAX, since it represents
    distance often in a wxCoord or wxSize. As a scalar, a distance is always
    positive. Because int is 32 bits and INT_MAX is
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

    The largest zoom factor allowed is therefore ~ 300 (which computes to 762000).
*/

#define MAX_ZOOM_FACTOR 300.0

// Adjusted to display zoom level ~ 1 when the screen shows a 1:1 image.
// Obviously depends on the monitor, but this is an acceptable value.
#define ZOOM_COEFF 1.1
