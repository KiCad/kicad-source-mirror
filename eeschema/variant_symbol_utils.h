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

#ifndef VARIANT_SYMBOL_UTILS_H
#define VARIANT_SYMBOL_UTILS_H

#include <functional>
#include <vector>
#include <wx/string.h>

class LIB_ID;
class LIB_SYMBOL;
class SCH_PIN;


/**
 * Describes a single pin compatibility problem found when comparing a candidate symbol
 * against a base symbol for use as a variant symbol override.
 */
enum class VARIANT_COMPAT_ERROR
{
    NONE,
    MISSING_PIN_NUMBER,    ///< A base pin number is absent from the candidate for a given unit/body style
    EXTRA_PIN_NUMBER,      ///< Candidate has a pin occurrence that is absent from the base
    PIN_POSITION_MISMATCH, ///< A matching pin number has a different position in library coordinates
    PIN_TYPE_MISMATCH,     ///< A matching pin number has a different electrical type
    INSUFFICIENT_UNITS,    ///< Candidate has fewer units than the base
    MISSING_BODY_STYLE,    ///< Base uses alternate body styles but candidate does not
};


/**
 * Check whether two graphical pins occupy the same variant-symbol connection slot.
 */
bool VariantSymbolPinsMatch( const SCH_PIN& aBase, const SCH_PIN& aCandidate );


struct VARIANT_COMPAT_RESULT
{
    VARIANT_COMPAT_ERROR error = VARIANT_COMPAT_ERROR::NONE;
    wxString             detail;     ///< human-readable description
    wxString             pinNumber;  ///< pin that failed, if applicable
    int                  unit = 0;      ///< unit that failed, if applicable
    int                  bodyStyle = 0; ///< body style that failed, if applicable
};


/**
 * Callback that evaluates variant symbol compatibility for a given candidate LIB_ID.
 * Returns an empty vector when the candidate is fully compatible with the base symbol.
 */
using SYMBOL_COMPAT_FUNC = std::function<std::vector<VARIANT_COMPAT_RESULT>( const LIB_ID& )>;


/**
 * Check whether @a aCandidate can be used as a variant symbol override for @a aBase.
 *
 * The returned vector lists every issue found and an empty vector means fully compatible.
 * Every graphical pin occurrence must have a one-to-one match with the same number, unit,
 * body style, position, and electrical type.  The candidate must have at least as many units
 * as the base and must provide alternate body styles when the base has them.  Pin names,
 * symbol body graphics, field names and counts are not checked.
 *
 * @param aBase      the base symbol whose pin layout must be preserved.
 * @param aCandidate the proposed alternate symbol.
 * @return a vector of all compatibility issues found; empty if the symbols are compatible.
 */
std::vector<VARIANT_COMPAT_RESULT> ValidateVariantSymbolCompatibility( const LIB_SYMBOL& aBase,
                                                                        const LIB_SYMBOL& aCandidate );

#endif // VARIANT_SYMBOL_UTILS_H
