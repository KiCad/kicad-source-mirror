/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <properties/wx_any_utils.h>

#include <properties/property.h>
#include <geometry/eda_angle.h>
#include <gal/color4d.h>
#include <math/vector2d.h>
#include <math/box2.h>
#include <kiid.h>
#include <layer_ids.h>

#include <wx/string.h>

#include <optional>


bool KiWxAnyEquals( const wxAny& aA, const wxAny& aB, const PROPERTY_BASE* aProperty )
{
    // Enum-backed properties: compare via the integer choice value (or string
    // label) rather than the enum type itself, since wxAny equality on enums
    // requires knowing the exact enum type at compile time.
    if( aProperty && aProperty->HasChoices() )
    {
        int aInt = 0, bInt = 0;

        if( aA.GetAs<int>( &aInt ) && aB.GetAs<int>( &bInt ) )
            return aInt == bInt;

        wxString aStr, bStr;

        if( aA.GetAs<wxString>( &aStr ) && aB.GetAs<wxString>( &bStr ) )
            return aStr == bStr;

        // Couldn't extract — fall through to scalar checks below.
    }


    // Same type is the precondition for value equality. wxAny doesn't expose a
    // direct type_index, so we check via CheckType<>() for the types we support.
    // For an unsupported type we conservatively report "not equal" so that a
    // property delta is generated (the differ will then drop the property if
    // conversion to DIFF_VALUE also fails).

    auto cmp = [&]( auto sentinel ) -> std::optional<bool>
    {
        using T = decltype( sentinel );

        if( aA.CheckType<T>() && aB.CheckType<T>() )
            return aA.As<T>() == aB.As<T>();

        if( aA.CheckType<T>() != aB.CheckType<T>() )
            return false;

        return std::nullopt;
    };

    using llong = long long;

    if( auto r = cmp( bool{} ) )                    return *r;
    if( auto r = cmp( int{} ) )                     return *r;
    if( auto r = cmp( long{} ) )                    return *r;
    if( auto r = cmp( llong{} ) )                   return *r;
    if( auto r = cmp( unsigned{} ) )                return *r;
    if( auto r = cmp( float{} ) )                   return *r;
    if( auto r = cmp( double{} ) )                  return *r;
    if( auto r = cmp( wxString{} ) )                return *r;
    if( auto r = cmp( std::string{} ) )             return *r;

    if( aA.CheckType<std::optional<int>>() && aB.CheckType<std::optional<int>>() )
        return aA.As<std::optional<int>>() == aB.As<std::optional<int>>();

    if( aA.CheckType<std::optional<double>>() && aB.CheckType<std::optional<double>>() )
        return aA.As<std::optional<double>>() == aB.As<std::optional<double>>();

    if( aA.CheckType<EDA_ANGLE>() && aB.CheckType<EDA_ANGLE>() )
        return aA.As<EDA_ANGLE>() == aB.As<EDA_ANGLE>();

    if( aA.CheckType<VECTOR2I>() && aB.CheckType<VECTOR2I>() )
        return aA.As<VECTOR2I>() == aB.As<VECTOR2I>();

    if( aA.CheckType<BOX2I>() && aB.CheckType<BOX2I>() )
        return aA.As<BOX2I>() == aB.As<BOX2I>();

    if( aA.CheckType<KIGFX::COLOR4D>() && aB.CheckType<KIGFX::COLOR4D>() )
        return aA.As<KIGFX::COLOR4D>() == aB.As<KIGFX::COLOR4D>();

    if( aA.CheckType<KIID>() && aB.CheckType<KIID>() )
        return aA.As<KIID>() == aB.As<KIID>();

    if( aA.CheckType<PCB_LAYER_ID>() && aB.CheckType<PCB_LAYER_ID>() )
        return aA.As<PCB_LAYER_ID>() == aB.As<PCB_LAYER_ID>();

    return false; // unsupported type pair — conservatively report differing
}
