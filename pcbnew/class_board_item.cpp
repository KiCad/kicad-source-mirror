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
#include <string>

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
        return board->GetLayerName( m_Layer );

    // If no parent, return standard name
    return BOARD::GetStandardLayerName( m_Layer );
}


std::string BOARD_ITEM::FormatInternalUnits( int aValue )
{
#if 1

    char    buf[50];
    int     len;
    double  mm = aValue / IU_PER_MM;

    if( mm != 0.0 && fabs( mm ) <= 0.0001 )
    {
        len = sprintf( buf, "%.10f", mm );

        while( --len > 0 && buf[len] == '0' )
            buf[len] = '\0';

        if( buf[len] == '.' )
            buf[len] = '\0';
        else
            ++len;
    }
    else
    {
        len = sprintf( buf, "%.10g", mm );
    }

    return std::string( buf, len );

#else

    // Assume aValue is in nanometers, and that we want the result in millimeters,
    // and that int is 32 bits wide.  Then perform an alternative algorithm.
    // Can be used to verify that the above algorithm is correctly generating text.
    // Convert aValue into an integer string, then insert a decimal point manually.
    // Results are the same as above general purpose algorithm.

    wxASSERT( sizeof(int) == 4 );

    if( aValue == 0 )
        return std::string( 1, '0' );
    else
    {
        char    buf[50];
        int     len = sprintf( buf, aValue > 0 ? "%06d" : "%07d", aValue );     // optionally pad w/leading zeros

        std::string ret( buf, len );

        std::string::iterator it = ret.end() - 1;           // last byte

        // insert '.' at 6 positions from end, dividing by 10e6 (a million), nm => mm
        std::string::iterator decpoint = ret.end() - 6;

        // truncate trailing zeros, up to decimal point position
        for(  ; *it=='0' && it >= decpoint;  --it )
            ret.erase( it );    // does not invalidate iterators it or decpoint

        if( it >= decpoint )
        {
            ret.insert( decpoint, '.' );

            // decpoint is invalidated here, after insert()

#if 1       // want a leading zero when decimal point is in first position?
            if( ret[0] == '.' )
            {
                // insert leading zero ahead of decimal point.
                ret.insert( ret.begin(), '0' );
            }
            else if( ret[0]=='-' && ret[1]=='.' )
            {
                ret.insert( ret.begin() + 1, '0' );
            }
#endif
        }

        return ret;
    }

#endif
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


void BOARD_ITEM::ViewGetLayers( int aLayers[], int& aCount ) const
{
    // Basic fallback
    aCount = 1;
    aLayers[0] = m_Layer;
}


int BOARD_ITEM::getTrailingInt( wxString aStr )
{
    int number = 0;
    int base = 1;

    // Trim and extract the trailing numeric part
    int index = aStr.Len() - 1;
    while( index >= 0 )
    {
        const char chr = aStr.GetChar( index );

        if( chr < '0' || chr > '9' )
            break;

        number += ( chr - '0' ) * base;
        base *= 10;
        index--;
    }

    return number;
}

int BOARD_ITEM::getNextNumberInSequence( std::set<int> aSeq, bool aFillSequenceGaps)
{
    // By default go to the end of the sequence
    int candidate = *aSeq.end();

    // Filling in gaps in pad numbering
    if( aFillSequenceGaps )
    {
        // start at the beginning
        candidate = *aSeq.begin();

        for( std::set<int>::iterator it = aSeq.begin(),
            itEnd = aSeq.end(); it != itEnd; ++it )
        {
            if( *it - candidate > 1 )
                break;

            candidate = *it;
        }
    }

    candidate++;
    return candidate;
}
