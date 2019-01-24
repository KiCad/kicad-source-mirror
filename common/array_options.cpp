/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <array_options.h>

#include <trigo.h>

const wxString& ARRAY_OPTIONS::AlphabetFromNumberingScheme( NUMBERING_TYPE_T type )
{
    static const wxString alphaNumeric = "0123456789";
    static const wxString alphaHex = "0123456789ABCDEF";
    static const wxString alphaFull = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static const wxString alphaNoIOSQXZ = "ABCDEFGHJKLMNPRTUVWY";

    switch( type )
    {
    default:
    case NUMBERING_NUMERIC: return alphaNumeric;
    case NUMBERING_HEX: return alphaHex;
    case NUMBERING_ALPHA_NO_IOSQXZ: return alphaNoIOSQXZ;
    case NUMBERING_ALPHA_FULL: return alphaFull;
    }
}


bool ARRAY_OPTIONS::SchemeNonUnitColsStartAt0( NUMBERING_TYPE_T type )
{
    return type == NUMBERING_ALPHA_FULL || type == NUMBERING_ALPHA_NO_IOSQXZ;
}


bool ARRAY_OPTIONS::GetNumberingOffset(
        const wxString& str, ARRAY_OPTIONS::NUMBERING_TYPE_T type, int& offsetToFill )
{
    const wxString& alphabet = ARRAY_OPTIONS::AlphabetFromNumberingScheme( type );

    int       offset = 0;
    const int radix = alphabet.length();

    for( unsigned i = 0; i < str.length(); i++ )
    {
        int chIndex = alphabet.Find( str[i], false );

        if( chIndex == wxNOT_FOUND )
            return false;

        const bool start0 = ARRAY_OPTIONS::SchemeNonUnitColsStartAt0( type );

        // eg "AA" is actually index 27, not 26
        if( start0 && i < str.length() - 1 )
            chIndex++;

        offset *= radix;
        offset += chIndex;
    }

    offsetToFill = offset;
    return true;
}


wxString ARRAY_OPTIONS::getCoordinateNumber( int n, NUMBERING_TYPE_T type )
{
    wxString        itemNum;
    const wxString& alphabet = AlphabetFromNumberingScheme( type );

    const bool nonUnitColsStartAt0 = SchemeNonUnitColsStartAt0( type );

    bool firstRound = true;
    int  radix = alphabet.Length();

    do
    {
        int modN = n % radix;

        if( nonUnitColsStartAt0 && !firstRound )
            modN--; // Start the "tens/hundreds/etc column" at "Ax", not "Bx"

        itemNum.insert( 0, 1, alphabet[modN] );

        n /= radix;
        firstRound = false;
    } while( n );

    return itemNum;
}


int ARRAY_GRID_OPTIONS::GetArraySize() const
{
    return m_nx * m_ny;
}


VECTOR2I ARRAY_GRID_OPTIONS::getGridCoords( int n ) const
{
    const int axisSize = m_horizontalThenVertical ? m_nx : m_ny;

    int x = n % axisSize;
    int y = n / axisSize;

    // reverse on this row/col?
    if( m_reverseNumberingAlternate && ( y % 2 ) )
        x = axisSize - x - 1;

    wxPoint coords( x, y );

    return coords;
}


ARRAY_OPTIONS::TRANSFORM ARRAY_GRID_OPTIONS::GetTransform( int n, const VECTOR2I& aPos ) const
{
    VECTOR2I point;

    VECTOR2I coords = getGridCoords( n );

    // swap axes if needed
    if( !m_horizontalThenVertical )
        std::swap( coords.x, coords.y );

    point.x = coords.x * m_delta.x + coords.y * m_offset.x;
    point.y = coords.y * m_delta.y + coords.x * m_offset.y;

    if( std::abs( m_stagger ) > 1 )
    {
        const int  stagger = std::abs( m_stagger );
        const bool sr = m_stagger_rows;
        const int  stagger_idx = ( ( sr ? coords.y : coords.x ) % stagger );

        VECTOR2I stagger_delta( ( sr ? m_delta.x : m_offset.x ), ( sr ? m_offset.y : m_delta.y ) );

        // Stagger to the left/up if the sign of the stagger is negative
        point += stagger_delta * copysign( stagger_idx, m_stagger ) / stagger;
    }

    // this is already relative to the first array entry
    return { point, 0.0 };
}


wxString ARRAY_GRID_OPTIONS::GetItemNumber( int n ) const
{
    wxString itemNum;

    if( m_2dArrayNumbering )
    {
        VECTOR2I coords = getGridCoords( n );

        itemNum += getCoordinateNumber( coords.x + m_numberingOffsetX, m_priAxisNumType );
        itemNum += getCoordinateNumber( coords.y + m_numberingOffsetY, m_secAxisNumType );
    }
    else
    {
        itemNum += getCoordinateNumber( n + m_numberingOffsetX, m_priAxisNumType );
    }

    return itemNum;
}


int ARRAY_CIRCULAR_OPTIONS::GetArraySize() const
{
    return m_nPts;
}


ARRAY_OPTIONS::TRANSFORM ARRAY_CIRCULAR_OPTIONS::GetTransform( int n, const VECTOR2I& aPos ) const
{
    double angle;

    if( m_angle == 0 )
        // angle is zero, divide evenly into m_nPts
        angle = 10 * 360.0 * n / double( m_nPts );
    else
        // n'th step
        angle = m_angle * n;

    VECTOR2I new_pos = aPos;
    RotatePoint( new_pos, m_centre, angle );

    // take off the rotation (but not the translation) if needed
    if( !m_rotateItems )
        angle = 0;

    return { new_pos - aPos, angle / 10.0 };
}


wxString ARRAY_CIRCULAR_OPTIONS::GetItemNumber( int aN ) const
{
    return getCoordinateNumber( aN + m_numberingOffset, m_numberingType );
}
