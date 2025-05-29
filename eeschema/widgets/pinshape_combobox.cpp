/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Simon Richter <Simon.Richter@hogyros.de>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include "pinshape_combobox.h"
#include <bitmaps.h>


PINSHAPE_COMBOBOX::PINSHAPE_COMBOBOX( wxWindow* parent, wxWindowID id, const wxString& value,
                                     const wxPoint& pos, const wxSize& size, int n,
                                     const wxString choices[], long style,
                                     const wxValidator& validator, const wxString& name ) :
        WX_BITMAP_COMBOBOX( parent, id, value, pos, size, n, choices, style, validator, name )
{
    for( unsigned ii = 0; ii < GRAPHIC_PINSHAPES_TOTAL; ++ii )
    {
        GRAPHIC_PINSHAPE shape = static_cast<GRAPHIC_PINSHAPE>( ii );

        wxString text = PinShapeGetText( shape );
        BITMAPS bitmap = PinShapeGetBitmap( shape );

        if( bitmap == BITMAPS::INVALID_BITMAP )
            Append( text );
        else
            Insert( text, KiBitmapBundle( bitmap ), ii );
    }
}


GRAPHIC_PINSHAPE PINSHAPE_COMBOBOX::GetPinShapeSelection()
{
    return static_cast<GRAPHIC_PINSHAPE>( GetSelection() );
}


void PINSHAPE_COMBOBOX::SetSelection( GRAPHIC_PINSHAPE aShape )
{
    WX_BITMAP_COMBOBOX::SetSelection( static_cast<int>( aShape ) );
}
