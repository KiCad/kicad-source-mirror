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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <board_tables/generated_table_schema.h>

#include <algorithm>
#include <set>

#include <base_units.h>
#include <layer_ids.h>

#include <wx/translation.h>


GENERATED_TABLE_SCHEMA::GENERATED_TABLE_SCHEMA( std::vector<GENERATED_TABLE_COLUMN_DEF> aDefs,
                                                 GENERATED_TABLE_UNITS aDefaultUnits, int aDefaultPrecision ) :
        m_defs( std::move( aDefs ) ),
        m_defaultUnits( aDefaultUnits ),
        m_defaultPrecision( aDefaultPrecision )
{
}


const GENERATED_TABLE_COLUMN_DEF* GENERATED_TABLE_SCHEMA::Find( int aId ) const
{
    for( const GENERATED_TABLE_COLUMN_DEF& def : m_defs )
    {
        if( def.m_Id == aId )
            return &def;
    }

    return nullptr;
}


const GENERATED_TABLE_COLUMN_DEF* GENERATED_TABLE_SCHEMA::FindToken( const wxString& aToken ) const
{
    for( const GENERATED_TABLE_COLUMN_DEF& def : m_defs )
    {
        if( aToken.IsSameAs( wxString::FromUTF8( def.m_Token ) ) )
            return &def;
    }

    return nullptr;
}


const char* GENERATED_TABLE_SCHEMA::Token( int aId ) const
{
    if( const GENERATED_TABLE_COLUMN_DEF* def = Find( aId ) )
        return def->m_Token;

    return m_defs.front().m_Token;
}


GENERATED_TABLE_COLUMN GENERATED_TABLE_SCHEMA::DefaultColumn( int aId ) const
{
    GENERATED_TABLE_COLUMN col;
    col.m_Id = aId;

    if( const GENERATED_TABLE_COLUMN_DEF* def = Find( aId ) )
    {
        col.m_Heading = wxGetTranslation( wxString( def->m_Heading ) );
        col.m_Align = def->m_Align;
    }

    return col;
}


std::vector<GENERATED_TABLE_COLUMN> GENERATED_TABLE_SCHEMA::Defaults() const
{
    std::vector<GENERATED_TABLE_COLUMN> cols;

    for( const GENERATED_TABLE_COLUMN_DEF& def : m_defs )
    {
        if( def.m_DefaultShown )
            cols.push_back( DefaultColumn( def.m_Id ) );
    }

    return cols;
}


bool GENERATED_TABLE_SCHEMA::Validate( std::vector<GENERATED_TABLE_COLUMN>& aColumns ) const
{
    std::vector<GENERATED_TABLE_COLUMN> kept;
    std::set<int>                       seen;
    int64_t                             total = 0;

    for( GENERATED_TABLE_COLUMN& col : aColumns )
    {
        // Unknown to this schema, or a repeat that adds no information and multiplies the
        // generated cell count either way
        if( !Find( col.m_Id ) || !seen.insert( col.m_Id ).second )
            continue;

        col.m_Width = std::clamp( col.m_Width, 0, GENERATED_TABLE_MAX_COLUMN_WIDTH );
        total += col.m_Width;
        kept.push_back( col );
    }

    aColumns = std::move( kept );

    return !aColumns.empty() && total <= GENERATED_TABLE_MAX_TOTAL_WIDTH;
}


LSET DocumentationLayers()
{
    // Drops mask, paste, adhesive and silk, which print on the finished board, plus the
    // courtyard layers, which DRC reads as constraints
    LSET layers = LSET::AllNonCuMask() & ~LSET::AllBoardTechMask();

    layers.reset( Edge_Cuts );
    layers.reset( Margin );
    layers.reset( F_CrtYd );
    layers.reset( B_CrtYd );
    layers.reset( Rescue );

    return layers;
}


wxString FormatGeneratedTableLength( int aValue, GENERATED_TABLE_UNITS aUnits, int aPrecision )
{
    const double mm = pcbIUScale.IUTomm( aValue );

    switch( aUnits )
    {
    case GENERATED_TABLE_UNITS::INCH:
        return wxString::Format( wxT( "%.*f″" ), aPrecision + 1, mm / 25.4 );

    // A mil is three orders of magnitude finer than an inch, so its precision borrows from
    // the same setting rather than needing one of its own
    case GENERATED_TABLE_UNITS::MILS:
        return wxString::Format( wxT( "%.*f mil" ), std::max( aPrecision - 2, 0 ), mm / 25.4 * 1000.0 );

    case GENERATED_TABLE_UNITS::MM:
    default:
        return wxString::Format( wxT( "%.*f" ), aPrecision, mm );
    }
}


namespace
{

struct SCHEMA_TOKEN
{
    const char* token;
    int         value;
};

const SCHEMA_TOKEN unitsTokens[] = {
    { "mm", (int) GENERATED_TABLE_UNITS::MM },
    { "inch", (int) GENERATED_TABLE_UNITS::INCH },
    { "mils", (int) GENERATED_TABLE_UNITS::MILS },
};

const SCHEMA_TOKEN alignTokens[] = {
    { "left", (int) GENERATED_TABLE_ALIGN::LEFT },
    { "center", (int) GENERATED_TABLE_ALIGN::CENTER },
    { "right", (int) GENERATED_TABLE_ALIGN::RIGHT },
};

template <size_t N>
const char* tokenFor( const SCHEMA_TOKEN ( &aMap )[N], int aValue )
{
    for( const SCHEMA_TOKEN& entry : aMap )
    {
        if( entry.value == aValue )
            return entry.token;
    }

    return aMap[0].token;
}

template <size_t N>
bool valueFor( const SCHEMA_TOKEN ( &aMap )[N], const wxString& aToken, int& aValue )
{
    for( const SCHEMA_TOKEN& entry : aMap )
    {
        if( aToken.IsSameAs( wxString::FromUTF8( entry.token ) ) )
        {
            aValue = entry.value;
            return true;
        }
    }

    return false;
}

} // namespace


const char* GeneratedTableUnitsToken( GENERATED_TABLE_UNITS aUnits )
{
    return tokenFor( unitsTokens, (int) aUnits );
}


bool GeneratedTableUnitsFromToken( const wxString& aToken, GENERATED_TABLE_UNITS& aUnits )
{
    int v = 0;

    if( !valueFor( unitsTokens, aToken, v ) )
        return false;

    aUnits = (GENERATED_TABLE_UNITS) v;
    return true;
}


const char* GeneratedTableAlignToken( GENERATED_TABLE_ALIGN aAlign )
{
    return tokenFor( alignTokens, (int) aAlign );
}


bool GeneratedTableAlignFromToken( const wxString& aToken, GENERATED_TABLE_ALIGN& aAlign )
{
    int v = 0;

    if( !valueFor( alignTokens, aToken, v ) )
        return false;

    aAlign = (GENERATED_TABLE_ALIGN) v;
    return true;
}
