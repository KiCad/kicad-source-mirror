/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Author Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "lib_symbol.h"
#include "sch_symbol.h"
#include "sch_pin.h"

struct ORIENT_MIRROR
{
    int flag;
    int n_rots;
    int mirror_x;
    int mirror_y;
};


// symbols_orientations_list is the list of possible orientation+mirror values
// like returned by SCH_SYMBOL::GetOrientation()
// Some transforms are equivalent, like rotation 180 + mirror X = mirror Y

static ORIENT_MIRROR symbols_orientations_list[] =
{
    { SYM_ORIENT_0,                  0, 0, 0 },
    { SYM_ORIENT_90,                 1, 0, 0 },
    { SYM_ORIENT_180,                2, 0, 0 },
    { SYM_ORIENT_270,                3, 0, 0 },
    { SYM_MIRROR_X + SYM_ORIENT_0,   0, 1, 0 },
    { SYM_MIRROR_X + SYM_ORIENT_90,  1, 1, 0 },
    { SYM_MIRROR_Y,                  0, 0, 1 },
    { SYM_MIRROR_X + SYM_ORIENT_270, 3, 1, 0 },
    { SYM_MIRROR_Y + SYM_ORIENT_0,   0, 0, 1 },
    { SYM_MIRROR_Y + SYM_ORIENT_90,  1, 0, 1 },
    { SYM_MIRROR_Y + SYM_ORIENT_180, 2, 0, 1 },
    { SYM_MIRROR_Y + SYM_ORIENT_270, 3, 0, 1 }
};


void OrientAndMirrorSymbolItems( LIB_SYMBOL* aSymbol, int aOrientation )
{
    ORIENT_MIRROR o = symbols_orientations_list[ 0 ];

    for( ORIENT_MIRROR& i : symbols_orientations_list )
    {
        if( i.flag == aOrientation )
        {
            o = i;
            break;
        }
    }

    for( SCH_ITEM& item : aSymbol->GetDrawItems() )
    {
        for( int i = 0; i < o.n_rots; i++ )
            item.Rotate( VECTOR2I( 0, 0 ), true );

        if( o.mirror_x )
            item.MirrorVertically( 0 );

        if( o.mirror_y )
            item.MirrorHorizontally( 0 );
    }
}



// Rotate and/or mirror a SCH_PIN according to aOrientMirror.
// aOrientMirror is the orientation/mirror of the parent symbol.
// The modified pin orientation is the actual pin orientation/mirror
// when the parent symbol is drawn.
void RotateAndMirrorPin( SCH_PIN& aPin, int aOrientMirror )
{
    ORIENT_MIRROR o = symbols_orientations_list[ 0 ];

    for( ORIENT_MIRROR& i : symbols_orientations_list )
    {
        if( i.flag == aOrientMirror )
        {
            o = i;
            break;
        }
    }

    for( int i = 0; i < o.n_rots; i++ )
        aPin.RotatePin( VECTOR2I( 0, 0 ), true );

    if( o.mirror_x )
        aPin.MirrorVerticallyPin( 0 );

    if( o.mirror_y )
        aPin.MirrorHorizontallyPin( 0 );
}
