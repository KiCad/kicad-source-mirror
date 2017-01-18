/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Simon Richter <Simon.Richter@hogyros.de>
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
 * @file pin_shape_combobox.cpp
 * @brief ComboBox widget for pin shape
 */

#include "pin_shape_combobox.h"

#include <lib_pin.h>

PinShapeComboBox::PinShapeComboBox( wxWindow* parent,
        wxWindowID id,
        const wxString& value,
        const wxPoint& pos,
        const wxSize& size,
        int n,
        const wxString choices[],
        long style,
        const wxValidator& validator,
        const wxString& name ) :
    wxBitmapComboBox( parent, id, value, pos, size, n, choices, style, validator, name )
{
    for( unsigned ii = 0; ii < PINSHAPE_COUNT; ++ii )
    {
        GRAPHIC_PINSHAPE shape = static_cast<GRAPHIC_PINSHAPE>( ii );

        wxString text = GetText( shape );
        BITMAP_DEF bitmap = GetBitmap( shape );

        if( bitmap == NULL )
            Append( text );
        else
            Insert( text, KiBitmap( bitmap ), ii );
    }
}


GRAPHIC_PINSHAPE PinShapeComboBox::GetPinShapeSelection()
{
    return static_cast<GRAPHIC_PINSHAPE>( GetSelection() );
}


void PinShapeComboBox::SetSelection( GRAPHIC_PINSHAPE aShape )
{
    wxBitmapComboBox::SetSelection( aShape );
}
