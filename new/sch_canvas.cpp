/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2011 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2011 Kicad Developers, see change_log.txt for contributors.
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

namespace SCH {


CANVAS::CANVAS( wxWindow* aParent ) :
    OPENGL_GAL( aParent, wxDefaultSize )
{
    // Set the world unit length
    SetWorldUnitLength( 0.01 );

    SetScreenDPI( 100 );

    // SetLookAtPoint( VECTOR2D( size.x / 2, size.y / 2 ) );

    ComputeWorldScreenMatrix();

/*
    // Compute the world size
    m_worldSize = VECTOR2D( m_screenSize.x, m_screenSize.y );

    m_isReady   = true;
    m_isPanning = false;
    m_isGroupStarted = true;

    m_offset = 0.333;

    // Load Font
    if( !m_font.LoadNewStrokeFont( newstroke_font, newstroke_font_bufsize ) )
    {
        cout << "Loading of the font failed." << endl;
    }

    m_font.SetGraphicsAbstractionLayer( this );
*/
}


void CANVAS::Paint()
{
/*
    BeginDrawing();

    SetBackgroundColor( COLOR4D( 0, 0, 0, 1.0 ) );

    ClearScreen();

    SetLayerDepth( -10 );
    SetGridSize( VECTOR2D( 50, 50 ) );
    DrawGrid();

    SetLayerDepth( 0 );

    Flush();
    EndDrawing();
*/
}


}   // namespace SCH
