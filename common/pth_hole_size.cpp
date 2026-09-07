/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <pth_hole_size.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>

#include <base_units.h>
#include <math/util.h>

#include <wx/intl.h>


namespace
{

/**
 * @return the effective minimum and maximum lead size.
 */
std::pair<int, int> effectiveLeadSize( const PTH_LEAD_DEF& aLead )
{
    switch( aLead.m_shape )
    {
    case PTH_LEAD_SHAPE::ROUND: return { aLead.m_minX, aLead.m_maxX };

    case PTH_LEAD_SHAPE::SQUARE:
        return { KiROUND( std::hypot( static_cast<double>( aLead.m_minX ), aLead.m_minX ) ),
                 KiROUND( std::hypot( static_cast<double>( aLead.m_maxX ), aLead.m_maxX ) ) };

    case PTH_LEAD_SHAPE::RECTANGULAR:
        return { KiROUND( std::hypot( static_cast<double>( aLead.m_minX ), aLead.m_minY ) ),
                 KiROUND( std::hypot( static_cast<double>( aLead.m_maxX ), aLead.m_maxY ) ) };
    }

    return { 0, 0 };
}

/**
 * IPC-2222B hole size rules
 *
 * There are three density levels, A, B, C.
 *
 * The standard defines a minimum hole size allowance and a maximum hole size allowed for
 * each density level.  The minimum hole size allowance is based on the largest lead size,
 * and the maximum hole size allowance is based on the smallest lead size.
 *
 * The standard defines only the allowable hole size range; choosing a recommended hole within
 * it is left to the caller.
 */
class IPC2222_HOLE_SIZE_STANDARD : public PTH_HOLE_SIZE_STANDARD
{
    // Smallest-hole allowances per density level A, B, C, added to the largest lead size.
    static constexpr int MIN_HOLE_ADDED[3] = { pcbIUScale.mmToIU( 0.25 ), pcbIUScale.mmToIU( 0.20 ),
                                               pcbIUScale.mmToIU( 0.15 ) };

    // Largest-hole allowances per density level A, B, C, added to the smallest lead size.
    static constexpr int MAX_HOLE_ADDED[3] = { pcbIUScale.mmToIU( 0.70 ), pcbIUScale.mmToIU( 0.70 ),
                                               pcbIUScale.mmToIU( 0.60 ) };

public:
    const wxString& GetID() const override
    {
        static const wxString id = "IPC-2222B";
        return id;
    }

    const wxString& GetDisplayName() const override { return GetID(); }

    int GetLevelCount() const override { return 3; }

    wxString GetLevelName( int aLevel ) const override
    {
        switch( std::clamp( aLevel, 0, GetLevelCount() - 1 ) )
        {
        case 0: return _( "A (General)" );
        case 1: return _( "B (Moderate)" );
        default: return _( "C (High density)" );
        }
    }

    int GetMinHoleAddend( int aLevel ) const override
    {
        return MIN_HOLE_ADDED[std::clamp( aLevel, 0, GetLevelCount() - 1 )];
    }

    int GetMaxHoleAddend( int aLevel ) const override
    {
        return MAX_HOLE_ADDED[std::clamp( aLevel, 0, GetLevelCount() - 1 )];
    }

    PTH_HOLE_SIZE_RESULT ComputeHoleSize( int aLevel, const PTH_LEAD_DEF& aLead ) const override
    {
        const int level = std::clamp( aLevel, 0, GetLevelCount() - 1 );

        const auto [leadMin, leadMax] = effectiveLeadSize( aLead );

        PTH_HOLE_SIZE_RESULT result;
        result.m_leadMin = leadMin;
        result.m_leadMax = leadMax;
        result.m_holeMin = leadMax + GetMinHoleAddend( level );
        result.m_holeMax = leadMin + GetMaxHoleAddend( level );

        return result;
    }
};

} // namespace


int ComputeRecommendedPthHoleSize( const PTH_HOLE_SIZE_RESULT& aRange, int aRoundingStep, PTH_HOLE_ROUNDING aRounding )
{
    // Midpoint of the allowable range.  Integer arithmetic keeps the rounding exact.
    const int64_t mid = ( static_cast<int64_t>( aRange.m_holeMin ) + aRange.m_holeMax ) / 2;

    if( aRoundingStep <= 0 )
        return static_cast<int>( mid );

    int64_t hole = 0;

    if( aRounding == PTH_HOLE_ROUNDING::UP )
        hole = ( mid + aRoundingStep - 1 ) / aRoundingStep * aRoundingStep;
    else
        hole = ( mid + aRoundingStep / 2 ) / aRoundingStep * aRoundingStep;

    // Rounding must not take the recommended hole below the smallest allowable hole.
    if( hole < aRange.m_holeMin )
        hole = ( static_cast<int64_t>( aRange.m_holeMin ) + aRoundingStep - 1 ) / aRoundingStep * aRoundingStep;

    return static_cast<int>( hole );
}


const PTH_HOLE_SIZE_STANDARD* FindPthHoleSizeStandard( const wxString& aId )
{
    for( const PTH_HOLE_SIZE_STANDARD* standard : GetPthHoleSizeStandards() )
    {
        if( aId == standard->GetID() )
            return standard;
    }

    return nullptr;
}


const std::vector<const PTH_HOLE_SIZE_STANDARD*>& GetPthHoleSizeStandards()
{
    static const IPC2222_HOLE_SIZE_STANDARD ipc2222b_std;

    static const std::vector<const PTH_HOLE_SIZE_STANDARD*> standards = {
        &ipc2222b_std,
    };

    return standards;
}
