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

#include "variant_symbol_utils.h"

#include <lib_symbol.h>
#include <pin_type.h>
#include <sch_pin.h>

#include <algorithm>


bool VariantSymbolPinsMatch( const SCH_PIN& aBase, const SCH_PIN& aCandidate )
{
    return aBase.GetNumber() == aCandidate.GetNumber()
           && aBase.GetUnit() == aCandidate.GetUnit()
           && aBase.GetBodyStyle() == aCandidate.GetBodyStyle()
           && aBase.GetPosition() == aCandidate.GetPosition();
}


std::vector<VARIANT_COMPAT_RESULT> ValidateVariantSymbolCompatibility( const LIB_SYMBOL& aBase,
                                                                        const LIB_SYMBOL& aCandidate )
{
    std::vector<VARIANT_COMPAT_RESULT> results;

    int baseUnits = std::max( aBase.GetUnitCount(), 1 );
    int candUnits = std::max( aCandidate.GetUnitCount(), 1 );

    if( candUnits < baseUnits )
    {
        VARIANT_COMPAT_RESULT result;
        result.error  = VARIANT_COMPAT_ERROR::INSUFFICIENT_UNITS;
        result.detail = wxString::Format( _( "Candidate has %d unit(s), base requires %d" ),
                                          candUnits, baseUnits );
        results.push_back( result );
    }

    int baseBodyStyles = aBase.GetBodyStyleCount();

    if( baseBodyStyles > 1 && aCandidate.GetBodyStyleCount() < baseBodyStyles )
    {
        VARIANT_COMPAT_RESULT result;
        result.error  = VARIANT_COMPAT_ERROR::MISSING_BODY_STYLE;
        result.detail = wxString::Format( _( "Candidate has %d body style(s), base requires %d" ),
                                          aCandidate.GetBodyStyleCount(), baseBodyStyles );
        results.push_back( result );
    }

    std::vector<const SCH_PIN*> basePins = aBase.GetGraphicalPins();
    std::vector<const SCH_PIN*> candidatePins = aCandidate.GetGraphicalPins();
    std::vector<bool>           matched( candidatePins.size(), false );

    for( const SCH_PIN* basePin : basePins )
    {
        auto sameSlot = [&]( const SCH_PIN* aCandidatePin )
        {
            return basePin->GetNumber() == aCandidatePin->GetNumber()
                   && basePin->GetUnit() == aCandidatePin->GetUnit()
                   && basePin->GetBodyStyle() == aCandidatePin->GetBodyStyle();
        };

        auto findCandidate = [&]( bool aRequirePosition, bool aRequireType )
        {
            for( size_t i = 0; i < candidatePins.size(); ++i )
            {
                if( matched[i] || !sameSlot( candidatePins[i] ) )
                    continue;

                if( aRequirePosition && candidatePins[i]->GetPosition() != basePin->GetPosition() )
                    continue;

                if( aRequireType && candidatePins[i]->GetType() != basePin->GetType() )
                    continue;

                return i;
            }

            return candidatePins.size();
        };

        size_t candidateIndex = findCandidate( true, true );

        if( candidateIndex == candidatePins.size() )
            candidateIndex = findCandidate( true, false );

        if( candidateIndex == candidatePins.size() )
            candidateIndex = findCandidate( false, false );

        if( candidateIndex == candidatePins.size() )
        {
            VARIANT_COMPAT_RESULT result;
            result.error     = VARIANT_COMPAT_ERROR::MISSING_PIN_NUMBER;
            result.pinNumber = basePin->GetNumber();
            result.unit      = basePin->GetUnit();
            result.bodyStyle = basePin->GetBodyStyle();
            result.detail    = wxString::Format(
                    _( "Unit %d, body style %d: pin '%s' not found in candidate" ),
                    result.unit, result.bodyStyle, result.pinNumber );
            results.push_back( result );
            continue;
        }

        matched[candidateIndex] = true;
        const SCH_PIN* candidatePin = candidatePins[candidateIndex];

        if( candidatePin->GetPosition() != basePin->GetPosition() )
        {
            VARIANT_COMPAT_RESULT result;
            result.error     = VARIANT_COMPAT_ERROR::PIN_POSITION_MISMATCH;
            result.pinNumber = basePin->GetNumber();
            result.unit      = basePin->GetUnit();
            result.bodyStyle = basePin->GetBodyStyle();
            result.detail    = wxString::Format(
                    _( "Unit %d, body style %d: pin '%s' position mismatch "
                       "(%d,%d) vs (%d,%d)" ),
                    result.unit, result.bodyStyle, result.pinNumber,
                    basePin->GetPosition().x, basePin->GetPosition().y,
                    candidatePin->GetPosition().x, candidatePin->GetPosition().y );
            results.push_back( result );
        }

        if( candidatePin->GetType() != basePin->GetType() )
        {
            VARIANT_COMPAT_RESULT result;
            result.error     = VARIANT_COMPAT_ERROR::PIN_TYPE_MISMATCH;
            result.pinNumber = basePin->GetNumber();
            result.unit      = basePin->GetUnit();
            result.bodyStyle = basePin->GetBodyStyle();
            result.detail    = wxString::Format(
                    _( "Unit %d, body style %d: pin '%s' type mismatch ('%s' vs '%s')" ),
                    result.unit, result.bodyStyle, result.pinNumber,
                    GetCanonicalElectricalTypeName( basePin->GetType() ),
                    GetCanonicalElectricalTypeName( candidatePin->GetType() ) );
            results.push_back( result );
        }
    }

    for( size_t i = 0; i < candidatePins.size(); ++i )
    {
        if( !matched[i] )
        {
            const SCH_PIN* candidatePin = candidatePins[i];
            VARIANT_COMPAT_RESULT result;
            result.error     = VARIANT_COMPAT_ERROR::EXTRA_PIN_NUMBER;
            result.pinNumber = candidatePin->GetNumber();
            result.unit      = candidatePin->GetUnit();
            result.bodyStyle = candidatePin->GetBodyStyle();
            result.detail    = wxString::Format(
                    _( "Unit %d, body style %d: candidate has extra pin '%s'" ),
                    result.unit, result.bodyStyle, result.pinNumber );
            results.push_back( result );
        }
    }

    return results;
}
