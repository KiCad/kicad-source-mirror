/**
 * @file class_board_item.cpp
 * @brief Class BOARD_ITEM definition and  some basic functions.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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
#include <common.h>
#include <pcbnew.h>

#include <class_board.h>


wxString BOARD_ITEM::ShowShape( STROKE_T aShape )
{
    switch( aShape )
    {
    case S_SEGMENT:         return _( "Line" );
    case S_RECT:            return _( "Rect" );
    case S_ARC:             return _( "Arc" );
    case S_CIRCLE:          return _( "Circle" );
    case S_CURVE:           return _( "Bezier Curve" );
    case S_POLYGON:         return _( "Polygon" );
    default:                return wxT( "??" );
    }
}


void BOARD_ITEM::UnLink()
{
    DLIST<BOARD_ITEM>* list = (DLIST<BOARD_ITEM>*) GetList();
    wxASSERT( list );

    if( list )
        list->Remove( this );
}


BOARD* BOARD_ITEM::GetBoard() const
{
    if( Type() == PCB_T )
        return (BOARD*) this;

    BOARD_ITEM* parent = GetParent();

    if( parent )
        return parent->GetBoard();

    return NULL;
}


wxString BOARD_ITEM::GetLayerName() const
{
    BOARD*  board = GetBoard();

    if( board )
        return board->GetLayerName( m_Layer ).Trim();

    // If no parent, return the untranslated layer name.
    return BOARD::GetDefaultLayerName( m_Layer, false );
}


std::string BOARD_ITEM::FormatInternalUnits( int aValue )
{
    char buf[50];

    double  mm = aValue / IU_PER_MM;

    int     len;

    if( mm != 0.0 && fabs( mm ) <= 0.0001 )
    {
        len = sprintf( buf, "%.10f", mm );

        while( --len > 0 && buf[len] == '0' )
            buf[len] = '\0';

        ++len;
    }
    else
    {
        len = sprintf( buf, "%.10g", mm );
    }

    return std::string( buf, len );
}


std::string BOARD_ITEM::FormatAngle( double aAngle )
{
    char temp[50];

    int len = snprintf( temp, sizeof(temp), "%.10g", aAngle / 10.0 );

    return std::string( temp, len );
}


std::string BOARD_ITEM::FormatInternalUnits( const wxPoint& aPoint )
{
    return FormatInternalUnits( aPoint.x ) + " " + FormatInternalUnits( aPoint.y );
}


std::string BOARD_ITEM::FormatInternalUnits( const wxSize& aSize )
{
    return FormatInternalUnits( aSize.GetWidth() ) + " " + FormatInternalUnits( aSize.GetHeight() );
}
