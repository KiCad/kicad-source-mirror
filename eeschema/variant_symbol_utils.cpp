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
#include <connectivity/conn_facts.h>
#include <span>
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


static std::vector<VARIANT_COMPAT_RESULT> validateCompatibility(
        int aBaseUnits, int aBaseBodyStyles, std::span<const PIN_COMPARISON_DATA> basePins,
        int aCandidateUnits, int aCandidateBodyStyles, std::span<const PIN_COMPARISON_DATA> candidatePins )
{
    std::vector<VARIANT_COMPAT_RESULT> results;

    int baseUnits = std::max( aBaseUnits, 1 );
    int candUnits = std::max( aCandidateUnits, 1 );

    if( candUnits < baseUnits )
    {
        VARIANT_COMPAT_RESULT result;
        result.error  = VARIANT_COMPAT_ERROR::INSUFFICIENT_UNITS;
        result.detail = wxString::Format( _( "Candidate has %d unit(s), base requires %d" ),
                                          candUnits, baseUnits );
        results.push_back( result );
    }

    int baseBodyStyles = aBaseBodyStyles;

    if( baseBodyStyles > 1 && aCandidateBodyStyles < baseBodyStyles )
    {
        VARIANT_COMPAT_RESULT result;
        result.error  = VARIANT_COMPAT_ERROR::MISSING_BODY_STYLE;
        result.detail = wxString::Format( _( "Candidate has %d body style(s), base requires %d" ),
                                          aCandidateBodyStyles, baseBodyStyles );
        results.push_back( result );
    }

    // Library pins without a concrete type resolve to unspecified, as with SCH_PIN::GetType().
    const auto type = []( const PIN_COMPARISON_DATA& pin )
    {
        return pin.type == ELECTRICAL_PINTYPE::PT_INHERIT ? ELECTRICAL_PINTYPE::PT_UNSPECIFIED : pin.type;
    };

    std::vector<bool> matched( candidatePins.size(), false );

    for( const PIN_COMPARISON_DATA& basePin : basePins )
    {
        auto sameSlot = [&]( const PIN_COMPARISON_DATA& aCandidatePin )
        {
            return basePin.number == aCandidatePin.number
                   && basePin.unit == aCandidatePin.unit
                   && basePin.bodyStyle == aCandidatePin.bodyStyle;
        };

        auto findCandidate = [&]( bool aRequirePosition, bool aRequireType )
        {
            for( size_t i = 0; i < candidatePins.size(); ++i )
            {
                if( matched[i] || !sameSlot( candidatePins[i] ) )
                    continue;

                if( aRequirePosition && candidatePins[i].position != basePin.position )
                    continue;

                if( aRequireType && type( candidatePins[i] ) != type( basePin ) )
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
            result.pinNumber = basePin.number;
            result.unit      = basePin.unit;
            result.bodyStyle = basePin.bodyStyle;
            result.detail    = wxString::Format(
                    _( "Unit %d, body style %d: pin '%s' not found in candidate" ),
                    result.unit, result.bodyStyle, result.pinNumber );
            results.push_back( result );
            continue;
        }

        matched[candidateIndex] = true;
        const auto& candidatePin = candidatePins[candidateIndex];

        if( candidatePin.position != basePin.position )
        {
            VARIANT_COMPAT_RESULT result;
            result.error     = VARIANT_COMPAT_ERROR::PIN_POSITION_MISMATCH;
            result.pinNumber = basePin.number;
            result.unit      = basePin.unit;
            result.bodyStyle = basePin.bodyStyle;
            result.detail    = wxString::Format(
                    _( "Unit %d, body style %d: pin '%s' position mismatch "
                       "(%d,%d) vs (%d,%d)" ),
                    result.unit, result.bodyStyle, result.pinNumber,
                    basePin.position.x, basePin.position.y,
                    candidatePin.position.x, candidatePin.position.y );
            results.push_back( result );
        }

        if( type( candidatePin ) != type( basePin ) )
        {
            VARIANT_COMPAT_RESULT result;
            result.error     = VARIANT_COMPAT_ERROR::PIN_TYPE_MISMATCH;
            result.pinNumber = basePin.number;
            result.unit      = basePin.unit;
            result.bodyStyle = basePin.bodyStyle;
            result.detail    = wxString::Format(
                    _( "Unit %d, body style %d: pin '%s' type mismatch ('%s' vs '%s')" ),
                    result.unit, result.bodyStyle, result.pinNumber,
                    GetCanonicalElectricalTypeName( type( basePin ) ),
                    GetCanonicalElectricalTypeName( type( candidatePin ) ) );
            results.push_back( result );
        }
    }

    for( size_t i = 0; i < candidatePins.size(); ++i )
    {
        if( !matched[i] )
        {
            const auto& candidatePin = candidatePins[i];
            VARIANT_COMPAT_RESULT result;
            result.error     = VARIANT_COMPAT_ERROR::EXTRA_PIN_NUMBER;
            result.pinNumber = candidatePin.number;
            result.unit      = candidatePin.unit;
            result.bodyStyle = candidatePin.bodyStyle;
            result.detail    = wxString::Format(
                    _( "Unit %d, body style %d: candidate has extra pin '%s'" ),
                    result.unit, result.bodyStyle, result.pinNumber );
            results.push_back( result );
        }
    }

    return results;
}


static std::vector<PIN_COMPARISON_DATA> variantPins( const LIB_SYMBOL& aSymbol )
{
    std::vector<PIN_COMPARISON_DATA> result;

    for( const SCH_PIN* pin : aSymbol.GetGraphicalPins() )
        result.push_back( pin->ComparisonData() );

    return result;
}


std::vector<VARIANT_COMPAT_RESULT> ValidateVariantSymbolCompatibility( const LIB_SYMBOL& aBase,
                                                                        const LIB_SYMBOL& aCandidate )
{
    return validateCompatibility( aBase.GetUnitCount(), aBase.GetBodyStyleCount(), variantPins( aBase ),
                                 aCandidate.GetUnitCount(), aCandidate.GetBodyStyleCount(), variantPins( aCandidate ) );
}


std::vector<VARIANT_COMPAT_RESULT> ValidateVariantSymbolCompatibility(
        const SCH_CONNECTIVITY::LIBRARY_SYMBOL_FACT& aBase, const LIB_SYMBOL& aCandidate )
{
    return validateCompatibility( aBase.unitCount, aBase.bodyStyleCount,
                                  aBase.inheritedPins ? *aBase.inheritedPins : aBase.pins,
                                  aCandidate.GetUnitCount(), aCandidate.GetBodyStyleCount(),
                                  variantPins( aCandidate ) );
}
