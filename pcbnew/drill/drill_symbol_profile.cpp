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

#include <drill/drill_symbol_profile.h>

#include <base_units.h>
#include <drill/drill_operation.h>
#include <fmt/format.h>


DRILL_SYMBOL_PROFILE::DRILL_SYMBOL_PROFILE() :
        m_markPolicy( DRILL_MARK_POLICY::SHAPES_THEN_LETTERS ),
        m_symbolSize( pcbIUScale.mmToIU( 1.5 ) ),
        m_symbolWidth( pcbIUScale.mmToIU( 0.15 ) ),
        m_freezeAssignments( true )
{
    m_groupBy = { DRILL_GROUP_KEY::SIZE,    DRILL_GROUP_KEY::SLOT,
                  DRILL_GROUP_KEY::PLATING, DRILL_GROUP_KEY::SPAN,
                  DRILL_GROUP_KEY::OPERATION, DRILL_GROUP_KEY::POST_MACHINING };
}


void DRILL_SYMBOL_PROFILE::SetGroupedBy( DRILL_GROUP_KEY aKey, bool aOn )
{
    if( aOn )
        m_groupBy.insert( aKey );
    else
        m_groupBy.erase( aKey );
}


const DRILL_SYMBOL_ASSIGNMENT* DRILL_SYMBOL_PROFILE::GetAssignment( const std::string& aKey ) const
{
    auto it = m_assignments.find( aKey );

    return it == m_assignments.end() ? nullptr : &it->second;
}


void DRILL_SYMBOL_PROFILE::SetAssignment( const std::string& aKey,
                                          const DRILL_SYMBOL_ASSIGNMENT& aAssignment )
{
    m_assignments[aKey] = aAssignment;
}


std::string DRILL_SYMBOL_PROFILE::GroupKeyString( const DRILL_OPERATION& aOperation ) const
{
    std::string key = "v1";

    if( IsGroupedBy( DRILL_GROUP_KEY::SIZE ) )
        key += fmt::format( "|d{}", aOperation.m_Diameter );

    if( IsGroupedBy( DRILL_GROUP_KEY::SLOT ) )
    {
        key += aOperation.m_IsSlot ? fmt::format( "|s{}x{}", aOperation.m_SizeXY.x, aOperation.m_SizeXY.y )
                                   : "|s-";
    }

    if( IsGroupedBy( DRILL_GROUP_KEY::PLATING ) )
        key += aOperation.m_NotPlated ? "|p0" : "|p1";

    if( IsGroupedBy( DRILL_GROUP_KEY::SPAN ) )
    {
        // Copper ordinals, not layer names, so a stackup rename cannot invalidate the key
        key += fmt::format( "|L{}-{}", CopperLayerToOrdinal( aOperation.m_TopLayer ),
                            CopperLayerToOrdinal( aOperation.m_BottomLayer ) );
    }

    if( IsGroupedBy( DRILL_GROUP_KEY::OPERATION ) )
        key += fmt::format( "|k{}", static_cast<int>( aOperation.m_Kind ) );

    if( IsGroupedBy( DRILL_GROUP_KEY::HOLE_FUNCTION ) )
        key += fmt::format( "|a{}", static_cast<int>( aOperation.m_Attribute ) );

    if( IsGroupedBy( DRILL_GROUP_KEY::PROTECTION ) )
    {
        key += fmt::format( "|r{}{}{}{}{}{}{}{}", aOperation.m_Filled ? 1 : 0,
                            aOperation.m_Capped ? 1 : 0, aOperation.m_TopCovered ? 1 : 0,
                            aOperation.m_BottomCovered ? 1 : 0, aOperation.m_TopPlugged ? 1 : 0,
                            aOperation.m_BottomPlugged ? 1 : 0, aOperation.m_TopTented ? 1 : 0,
                            aOperation.m_BottomTented ? 1 : 0 );
    }

    if( IsGroupedBy( DRILL_GROUP_KEY::POST_MACHINING ) )
    {
        const DRILL_POST_MACHINING& front = aOperation.m_FrontPostMachining;
        const DRILL_POST_MACHINING& back = aOperation.m_BackPostMachining;

        key += fmt::format( "|m{}:{}:{}:{}/{}:{}:{}:{}", static_cast<int>( front.m_Mode ),
                            front.m_Size, front.m_Depth, front.m_Angle,
                            static_cast<int>( back.m_Mode ), back.m_Size, back.m_Depth,
                            back.m_Angle );
    }

    return key;
}


uint64_t DRILL_SYMBOL_PROFILE::Fingerprint() const
{
    uint64_t hash = 1469598103934665603ULL;

    auto mix =
            [&]( uint64_t aValue )
            {
                hash ^= aValue;
                hash *= 1099511628211ULL;
            };

    for( DRILL_GROUP_KEY key : m_groupBy )
        mix( static_cast<uint64_t>( key ) );

    mix( static_cast<uint64_t>( m_markPolicy ) );
    mix( static_cast<uint64_t>( m_symbolSize ) );
    mix( static_cast<uint64_t>( m_symbolWidth ) );
    mix( m_freezeAssignments ? 1 : 0 );

    for( const auto& [key, assignment] : m_assignments )
    {
        for( const char ch : key )
            mix( static_cast<unsigned char>( ch ) );

        mix( static_cast<uint64_t>( assignment.m_MarkMode ) );
        mix( static_cast<uint64_t>( assignment.m_ShapeIndex ) );

        // Letter and description are drawn, so a change to either has to invalidate the
        // caches keyed on this value
        for( const wxUniChar ch : assignment.m_Letter )
            mix( static_cast<uint64_t>( ch.GetValue() ) );

        mix( assignment.m_Letter.length() );

        for( const wxUniChar ch : assignment.m_Description )
            mix( static_cast<uint64_t>( ch.GetValue() ) );

        mix( assignment.m_Description.length() );
    }

    return hash;
}


bool DRILL_SYMBOL_PROFILE::operator==( const DRILL_SYMBOL_PROFILE& aOther ) const
{
    if( m_name != aOther.m_name || m_groupBy != aOther.m_groupBy
        || m_markPolicy != aOther.m_markPolicy || m_symbolSize != aOther.m_symbolSize
        || m_symbolWidth != aOther.m_symbolWidth
        || m_freezeAssignments != aOther.m_freezeAssignments )
    {
        return false;
    }

    if( m_assignments.size() != aOther.m_assignments.size() )
        return false;

    for( const auto& [key, assignment] : m_assignments )
    {
        const DRILL_SYMBOL_ASSIGNMENT* other = aOther.GetAssignment( key );

        if( !other || other->m_MarkMode != assignment.m_MarkMode
            || other->m_ShapeIndex != assignment.m_ShapeIndex
            || other->m_Letter != assignment.m_Letter
            || other->m_Description != assignment.m_Description )
        {
            return false;
        }
    }

    return true;
}


namespace
{
struct TOKEN_MAP
{
    const char* token;
    int         value;
};

const TOKEN_MAP groupKeyTokens[] = {
    { "size", (int) DRILL_GROUP_KEY::SIZE },
    { "slot", (int) DRILL_GROUP_KEY::SLOT },
    { "plating", (int) DRILL_GROUP_KEY::PLATING },
    { "span", (int) DRILL_GROUP_KEY::SPAN },
    { "operation", (int) DRILL_GROUP_KEY::OPERATION },
    { "hole_function", (int) DRILL_GROUP_KEY::HOLE_FUNCTION },
    { "protection", (int) DRILL_GROUP_KEY::PROTECTION },
    { "post_machining", (int) DRILL_GROUP_KEY::POST_MACHINING },
};

const TOKEN_MAP markPolicyTokens[] = {
    { "shapes", (int) DRILL_MARK_POLICY::SHAPES },
    { "letters", (int) DRILL_MARK_POLICY::LETTERS },
    { "shapes_then_letters", (int) DRILL_MARK_POLICY::SHAPES_THEN_LETTERS },
    { "size_text", (int) DRILL_MARK_POLICY::SIZE_TEXT },
};

const TOKEN_MAP markModeTokens[] = {
    { "shape", (int) DRILL_MARK_MODE::SHAPE },
    { "letter", (int) DRILL_MARK_MODE::LETTER },
    { "size_text", (int) DRILL_MARK_MODE::SIZE_TEXT },
};

template <size_t N>
const char* tokenFor( const TOKEN_MAP ( &aMap )[N], int aValue )
{
    for( const TOKEN_MAP& entry : aMap )
    {
        if( entry.value == aValue )
            return entry.token;
    }

    return aMap[0].token;
}

template <size_t N>
bool valueFor( const TOKEN_MAP ( &aMap )[N], const wxString& aToken, int& aValue )
{
    for( const TOKEN_MAP& entry : aMap )
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


const char* DrillGroupKeyToken( DRILL_GROUP_KEY aKey )
{
    return tokenFor( groupKeyTokens, (int) aKey );
}


const char* DrillMarkPolicyToken( DRILL_MARK_POLICY aPolicy )
{
    return tokenFor( markPolicyTokens, (int) aPolicy );
}


const char* DrillMarkModeToken( DRILL_MARK_MODE aMode )
{
    return tokenFor( markModeTokens, (int) aMode );
}


bool DrillGroupKeyFromToken( const wxString& aToken, DRILL_GROUP_KEY& aKey )
{
    int value = 0;

    if( !valueFor( groupKeyTokens, aToken, value ) )
        return false;

    aKey = (DRILL_GROUP_KEY) value;
    return true;
}


bool DrillMarkPolicyFromToken( const wxString& aToken, DRILL_MARK_POLICY& aPolicy )
{
    int value = 0;

    if( !valueFor( markPolicyTokens, aToken, value ) )
        return false;

    aPolicy = (DRILL_MARK_POLICY) value;
    return true;
}


bool DrillMarkModeFromToken( const wxString& aToken, DRILL_MARK_MODE& aMode )
{
    int value = 0;

    if( !valueFor( markModeTokens, aToken, value ) )
        return false;

    aMode = (DRILL_MARK_MODE) value;
    return true;
}
