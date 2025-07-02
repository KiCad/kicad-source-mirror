/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <array_axis.h>

#include <increment.h>


/**
 * @return False for schemes like 0,1...9,10
 *         True for schemes like A,B..Z,AA (where the tens column starts with char 0)
 */
static bool schemeNonUnitColsStartAt0( ARRAY_AXIS::NUMBERING_TYPE type )
{
    return type == ARRAY_AXIS::NUMBERING_TYPE::NUMBERING_ALPHA_FULL
           || type == ARRAY_AXIS::NUMBERING_TYPE::NUMBERING_ALPHA_NO_IOSQXZ;
}


ARRAY_AXIS::ARRAY_AXIS() : m_type( NUMBERING_TYPE::NUMBERING_NUMERIC ), m_offset( 0 ), m_step( 1 )
{
}


const wxString& ARRAY_AXIS::GetAlphabet() const
{
    static const wxString alphaNumeric = wxS( "0123456789" );
    static const wxString alphaHex = wxS( "0123456789ABCDEF" );
    static const wxString alphaFull = wxS( "ABCDEFGHIJKLMNOPQRSTUVWXYZ" );
    static const wxString alphaNoIOSQXZ = wxS( "ABCDEFGHJKLMNPRTUVWY" );

    switch( m_type )
    {
    default:
    case NUMBERING_NUMERIC:
        return alphaNumeric;
    case NUMBERING_HEX:
        return alphaHex;
    case NUMBERING_ALPHA_NO_IOSQXZ:
        return alphaNoIOSQXZ;
    case NUMBERING_ALPHA_FULL:
        return alphaFull;
    }
}


std::optional<int> ARRAY_AXIS::getNumberingOffset( const wxString& str ) const
{
    if( str.length() == 0 )
        return std::optional<int>{};

    const wxString& alphabet = GetAlphabet();

    int       offset = 0;
    const int radix = alphabet.length();

    for( unsigned i = 0; i < str.length(); i++ )
    {
        int chIndex = alphabet.Find( str[i], false );

        if( chIndex == wxNOT_FOUND )
            return std::optional<int>{};

        const bool start0 = schemeNonUnitColsStartAt0( m_type );

        // eg "AA" is actually index 27, not 26
        if( start0 && i < str.length() - 1 )
            chIndex++;

        offset *= radix;
        offset += chIndex;
    }

    return std::optional<int>{ offset };
}


void ARRAY_AXIS::SetAxisType( NUMBERING_TYPE aType )
{
    m_type = aType;
}


bool ARRAY_AXIS::SetOffset( const wxString& aOffsetName )
{
    std::optional<int> offset = getNumberingOffset( aOffsetName );

    // The string does not decode to a valid offset
    if( !offset )
        return false;

    SetOffset( *offset );
    return true;
}


void ARRAY_AXIS::SetOffset( int aOffset )
{
    m_offset = aOffset;
}


int ARRAY_AXIS::GetOffset() const
{
    return m_offset;
}


void ARRAY_AXIS::SetStep( int aStep )
{
    m_step = aStep;
}


wxString ARRAY_AXIS::GetItemNumber( int n ) const
{
    const wxString& alphabet = GetAlphabet();
    const bool      nonUnitColsStartAt0 = schemeNonUnitColsStartAt0( m_type );

    n = m_offset + m_step * n;

    return AlphabeticFromIndex( n, alphabet, nonUnitColsStartAt0 );
}