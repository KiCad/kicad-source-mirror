/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2011 KiCad Developers, see change_log.txt for contributors.
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


#include "sch_canvas.h"
#include "sch_part.h"
#include <gal/font/newstroke_font.h>


#define SWEET_UNIT      50      ///< world units comprizing a sweet grid cell



namespace SCH {


CANVAS::CANVAS( wxWindow* aParent ) :
    OPENGL_GAL( aParent, NULL, this )       // I am my own PaintListener
{
    Connect( EVT_GAL_REDRAW, wxCommandEventHandler( CANVAS::onRedraw ) );

    // Set the world unit length
    SetWorldUnitLength( 0.01 );

    SetScreenDPI( 100 );

    ComputeWorldScreenMatrix();

    // Set the world unit length
    // SetLookAtPoint( VECTOR2D( size.x / 2, size.y / 2 ) );

    // Load Font
    if( !m_font.LoadNewStrokeFont( newstroke_font, newstroke_font_bufsize ) )
    {
        printf( "Loading of the font failed.\n" );
    }

    m_font.SetGraphicsAbstractionLayer( this );
}


void CANVAS::onRedraw( wxCommandEvent& event )
{
    PaintScene();
}


void CANVAS::PaintScene()
{
    BeginDrawing();

    SetBackgroundColor( COLOR4D( 0x6c/255.0, 0x53/255.0, 0x53/255.0, 1.0 ) );

    ClearScreen();

    SetLayerDepth( -10 );

    SetGridSize( VECTOR2D( SWEET_UNIT, SWEET_UNIT ) );

    DrawGrid();

    PaintRectangle();

    SetLayerDepth( 0 );

    Flush();
    EndDrawing();
}


void CANVAS::PaintRectangle()
{
    VECTOR2D    worldSize = GetSize();

#if 1

    SetLineWidth( 1 );

    SetIsFill( true );
    SetIsStroke( true );

    SetFillColor( COLOR4D( 1, 1, 1, 1 ) );

    SetStrokeColor( COLOR4D( 0, 0, 0, 1 ) );

    DrawRectangle( VECTOR2D( worldSize.x/4, worldSize.y/4 ), VECTOR2D( 3*worldSize.x/4, 3*worldSize.y/4 ) );

#else

    D( cout << worldSize << endl; )

    int k = 0;
    double alpha = 0;
    for( double y = 0; y < worldSize.y - worldSize.y / 4; y += worldSize.y / 8 )
    {
        double index = 1;
        double y2 = y;

        for( double x = 0; x < worldSize.x - worldSize.x / 4; x += worldSize.x / 8 )
        {
            SetLineWidth( index + 1 );

            SetIsFill( ( k == 1 ) || ( k == 2 ) );

            SetIsStroke( ( k == 0 ) || ( k == 2 ) );

            SetFillColor( COLOR4D( 1 - alpha, 1, 1, 1 - alpha ) );
            SetStrokeColor( COLOR4D( alpha, alpha, 1 - alpha, alpha ) );

            DrawRectangle( VECTOR2D( x, y2 ), VECTOR2D( x + index * 10, y2 + index * 10 ) );

            x  += index * 5;
            y2 += index * 5;

            index += 1;
        }

        k = ( k + 1 ) % 3;
        alpha += 0.1;
    }
#endif

}



}   // namespace SCH
