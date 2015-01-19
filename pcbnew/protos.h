/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jp.charras@wanadoo.fr
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

/**
 * @file pcbnew/protos.h
 */

#ifndef PROTO_H
#define PROTO_H
#include <gr_basic.h>

class wxDC;
class wxPoint;
class EDA_DRAW_PANEL;
class BOARD_ITEM;
class TRACK;
class MODULE;


/***************/
/* TRPISTE.CPP */
/***************/

/**
 * Function DrawTraces
 * Draws n consecutive track segments in list.
 * Useful to show a track when it is a chain of segments
 * (for instance when creating a new track)
 *
 * @param panel A EDA_DRAW_ITEM pointer to the canvas.
 * @param DC A wxDC pointer of the device context used for drawing.
 * @param aStartTrace First segment
 * @param nbsegment Number of segments in list
 * @param mode_color Drawing mode (GRXOR, GROR ..)
 */

void DrawTraces( EDA_DRAW_PANEL* panel,
                 wxDC*           DC,
                 TRACK*          aStartTrace,
                 int             nbsegment,
                 GR_DRAWMODE     mode_color );

void ShowNewTrackWhenMovingCursor( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                                   bool aErase );

/**
 * Determine coordinate for a segment direction of 0, 90, or 45 degrees
 * depending on it's position from the origin (ox, oy) and \a aPosiition.
 */
void CalculateSegmentEndPoint( const wxPoint& aPosition, int ox, int oy, int* fx, int* fy );

/**
 * Finds the projection of a grid point on a track. This is the point
 * from where we want to draw new orthogonal tracks when starting on a track.
 */
bool Project( wxPoint* res, wxPoint on_grid, const TRACK* track );
TRACK* LocateIntrusion( TRACK* listStart, TRACK* aTrack, LAYER_NUM aLayer, const wxPoint& aRef );



#endif  /* #define PROTO_H */
