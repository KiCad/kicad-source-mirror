/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2012 Kicad Developers, see change_log.txt for contributors.
 *
 * Graphics Abstraction Layer (GAL) - base class
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

#include <wx/log.h>

#include <gal/graphics_abstraction_layer.h>
#include <gal/definitions.h>
#include <gal/color4d.h>


using namespace KiGfx;

const wxEventType KiGfx::EVT_GAL_REDRAW = wxNewEventType();

GAL::GAL()
{
    // Set the default values for the internal variables
    SetIsFill( false );
    SetIsStroke( true );
    SetLineJoin( LINE_JOIN_ROUND );
    SetLineCap( LINE_CAP_ROUND );
    SetIsCursorEnabled( false );
    SetZoomFactor( 1.0 );
    SetFillColor( COLOR4D( 0.0, 0.0, 0.0, 0.0 ) );
    SetStrokeColor( COLOR4D( 1.0, 1.0, 1.0, 1.0 ) );
    SetGridColor( COLOR4D( 1, 1, 1, 0.1 ) );
    SetCoarseGrid( 5 );
    SetLineWidth( 1.0 );
    SetDepthRange( VECTOR2D( -2048, 2047 ) );
}


GAL::~GAL()
{
}


void GAL::DrawGrid()
{
    // The grid consists of lines
    // For the drawing the start points, end points and increments have to be calculated in world coordinates
    VECTOR2D    screenStartPoint( 0, 0 );
    VECTOR2D    screenEndPoint( screenSize.x, screenSize.y );
    MATRIX3x3D  inverseMatrix   = worldScreenMatrix.Inverse();
    VECTOR2D    worldStartPoint = inverseMatrix * screenStartPoint;
    VECTOR2D    worldEndPoint   = inverseMatrix * screenEndPoint;

    // Compute grid variables
    int gridStartX = round( worldStartPoint.x / gridSize.x );
    int gridEndX = round( worldEndPoint.x / gridSize.x );
    int gridStartY = round( worldStartPoint.y / gridSize.y );
    int gridEndY = round( worldEndPoint.y / gridSize.y );

    int gridScreenSizeDense  = round( gridSize.x * worldScale );
    int gridScreenSizeCoarse = round( gridSize.x * (double) gridTick * worldScale );

    // Swap the coordinates, if they have not the right order
    SWAP( gridEndX, <, gridStartX );
    SWAP( gridEndY, <, gridStartY );

    // Correct the index, else some lines are not correctly painted
    gridStartX  -= 1;
    gridStartY  -= 1;
    gridEndX    += 1;
    gridEndY    += 1;

    double  savedLineWidth  = GetLineWidth();
    COLOR4D savedColor      = GetStrokeColor();

    // Compute the line width of the grid
    ComputeWorldScale();
    double  width = gridLineWidth / worldScale;
    double  doubleWidth = 2 * width;

    // Set line width & color
    SetLineWidth( width );

    double origSize = (double) gridOriginMarkerSize / worldScale;

    SetStrokeColor( COLOR4D( 1.0, 1.0, 1.0, 1.0 ) );
    SetIsFill( false );
    DrawLine( gridOrigin + VECTOR2D( -origSize, -origSize ), gridOrigin + VECTOR2D( origSize, origSize ) );
    DrawLine( gridOrigin + VECTOR2D( -origSize, origSize ),  gridOrigin + VECTOR2D( origSize, -origSize ) );
    DrawCircle( gridOrigin, origSize * 0.7 );

    SetStrokeColor( gridColor );

    if( std::max( gridScreenSizeDense, gridScreenSizeCoarse ) < gridDrawThreshold )
        return;

    // Now draw the grid, every coarse grid line gets the double width
    for( int j = gridStartY; j < gridEndY; j += 1 )
    {
        if( j % gridTick == 0 && gridScreenSizeDense > gridDrawThreshold )
        {
            SetLineWidth( doubleWidth );
        }
        else
        {
            SetLineWidth( width );
        }

        if( ( j % gridTick == 0 && gridScreenSizeCoarse > gridDrawThreshold )
            || gridScreenSizeDense > gridDrawThreshold )
        {
            DrawGridLine( VECTOR2D( gridStartX * gridSize.x, j * gridSize.y ),
                          VECTOR2D( gridEndX * gridSize.x,   j * gridSize.y ) );
        }
    }

    for( int i = gridStartX; i < gridEndX; i += 1 )
    {
        if( i % gridTick == 0 && gridScreenSizeDense > gridDrawThreshold )
        {
            SetLineWidth( doubleWidth );
        }
        else
        {
            SetLineWidth( width );
        }

        if( ( i % gridTick == 0 && gridScreenSizeCoarse > gridDrawThreshold )
            || gridScreenSizeDense > gridDrawThreshold )
        {
            DrawGridLine( VECTOR2D( i * gridSize.x, gridStartY * gridSize.y ),
                          VECTOR2D( i * gridSize.x, gridEndY * gridSize.y ) );
        }
    }

    // Restore old values
    SetLineWidth( savedLineWidth );
    SetStrokeColor( savedColor );
}

