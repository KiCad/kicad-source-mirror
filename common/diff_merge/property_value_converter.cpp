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

#include <diff_merge/property_value_converter.h>

#include <properties/property.h>
#include <geometry/eda_angle.h>
#include <gal/color4d.h>
#include <math/vector2d.h>
#include <math/box2.h>
#include <kiid.h>
#include <layer_ids.h>

#include <wx/string.h>

#include <optional>


namespace KICAD_DIFF
{

namespace
{

/**
 * Attempt to extract a value of type T from a wxAny and produce a DIFF_VALUE.
 * Returns std::nullopt if the wxAny does not carry T.
 */
template <typename T, typename F>
std::optional<DIFF_VALUE> tryAs( const wxAny& aValue, F&& aMaker )
{
    if( aValue.CheckType<T>() )
        return aMaker( aValue.As<T>() );

    return std::nullopt;
}


/**
 * Map a property's PROPERTY_DISPLAY to the DIFF_VALUE display hint so distance,
 * coordinate and angle deltas render in user units. PROPERTY_DISPLAY is a plain
 * enum in property.h, so this stays inside kicommon without pulling in
 * PROPERTY_MANAGER.
 */
DISPLAY_HINT displayHintFor( const PROPERTY_BASE* aProperty )
{
    if( !aProperty )
        return DISPLAY_HINT::NONE;

    switch( aProperty->Display() )
    {
    case PT_SIZE:   return DISPLAY_HINT::DISTANCE;
    case PT_COORD:  return DISPLAY_HINT::COORD;
    case PT_DEGREE: return DISPLAY_HINT::ANGLE;
    default:        return DISPLAY_HINT::NONE;
    }
}


/// Apply a display hint to @p aValue only when one is meaningful, leaving plain
/// numbers untouched so non-distance counts never acquire a bogus unit.
DIFF_VALUE withHint( DIFF_VALUE aValue, DISPLAY_HINT aHint )
{
    if( aHint == DISPLAY_HINT::NONE )
        return aValue;

    return aValue.WithDisplayHint( aHint );
}

} // namespace


DIFF_VALUE WxAnyToDiffValue( const wxAny& aValue, PROPERTY_BASE* aProperty )
{
    // The order matters: more specific types first (so that std::optional<int>
    // is not mis-matched against int).

    // Distance / coordinate / angle hint, derived once from the property so the
    // numeric branches below render in user units instead of raw IU.
    const DISPLAY_HINT hint = displayHintFor( aProperty );

    if( auto v = tryAs<bool>( aValue, []( bool x ) { return DIFF_VALUE::FromBool( x ); } ) )
        return *v;

    // std::optional<int> and std::optional<double> are used heavily for soldermask
    // margins, drill diameters with fallback, etc.
    if( aValue.CheckType<std::optional<int>>() )
    {
        std::optional<int> opt = aValue.As<std::optional<int>>();

        if( !opt.has_value() )
            return DIFF_VALUE();   // T::NONE — distinguishes "unset" from "0"

        return withHint( DIFF_VALUE::FromInt( *opt ), hint );
    }

    if( aValue.CheckType<std::optional<double>>() )
    {
        std::optional<double> opt = aValue.As<std::optional<double>>();

        if( !opt.has_value() )
            return DIFF_VALUE();

        return withHint( DIFF_VALUE::FromDouble( *opt ), hint );
    }

    if( auto v = tryAs<int>( aValue, [&]( int x )
                             { return withHint( DIFF_VALUE::FromInt( x ), hint ); } ) )
        return *v;

    if( auto v = tryAs<long>( aValue, [&]( long x )
                              { return withHint( DIFF_VALUE::FromInt64( x ), hint ); } ) )
        return *v;

    if( auto v = tryAs<long long>( aValue, [&]( long long x )
                                   { return withHint( DIFF_VALUE::FromInt64( x ), hint ); } ) )
        return *v;

    if( auto v = tryAs<unsigned>( aValue, [&]( unsigned x )
                                  { return withHint( DIFF_VALUE::FromInt64( x ), hint ); } ) )
        return *v;

    if( auto v = tryAs<double>( aValue, [&]( double x )
                                { return withHint( DIFF_VALUE::FromDouble( x ), hint ); } ) )
        return *v;

    if( aValue.CheckType<float>() )
        return withHint( DIFF_VALUE::FromDouble( aValue.As<float>() ), hint );

    if( auto v = tryAs<wxString>( aValue, []( const wxString& x )
                                  { return DIFF_VALUE::FromString( x ); } ) )
        return *v;

    if( auto v = tryAs<std::string>( aValue, []( const std::string& x )
                                     { return DIFF_VALUE::FromString( x ); } ) )
        return *v;

    if( auto v = tryAs<KIID>( aValue, []( const KIID& x )
                              { return DIFF_VALUE::FromKiid( x ); } ) )
        return *v;

    if( auto v = tryAs<EDA_ANGLE>( aValue, []( const EDA_ANGLE& x )
                                   { return DIFF_VALUE::FromDouble( x.AsDegrees() )
                                                    .WithDisplayHint( DISPLAY_HINT::ANGLE ); } ) )
        return *v;

    if( auto v = tryAs<VECTOR2I>( aValue, [&]( const VECTOR2I& x )
                                  { return withHint( DIFF_VALUE::FromVector2I( x ), hint ); } ) )
        return *v;

    if( auto v = tryAs<BOX2I>( aValue, []( const BOX2I& x )
                               { return DIFF_VALUE::FromBox2I( x ); } ) )
        return *v;

    if( auto v = tryAs<KIGFX::COLOR4D>( aValue, []( const KIGFX::COLOR4D& x )
                                        { return DIFF_VALUE::FromColor( x ); } ) )
        return *v;

    if( auto v = tryAs<PCB_LAYER_ID>( aValue, []( PCB_LAYER_ID x )
                                      { return DIFF_VALUE::FromLayer( x ); } ) )
        return *v;

    // Enum-backed properties: PROPERTY_ENUM<Owner, T> stores the enum type T
    // in the wxAny, so CheckType<int>() is false. wxAny::GetAs<int>() asks
    // the wxAnyValueType for a conversion; for enums whose value type
    // registers int compatibility this succeeds. When it does, we pair the
    // integer with the choice label so JSON output is readable.
    if( aProperty && aProperty->HasChoices() )
    {
        int enumInt = 0;

        if( aValue.GetAs<int>( &enumInt ) )
        {
            const wxPGChoices& choices = aProperty->Choices();
            std::string        label;

            for( unsigned ii = 0; ii < choices.GetCount(); ++ii )
            {
                if( choices.GetValue( ii ) == enumInt )
                {
                    label = std::string( choices.GetLabel( ii ).ToUTF8() );
                    break;
                }
            }

            return DIFF_VALUE::FromEnum( enumInt, label );
        }

        // Fallback: print the value as a label string. Most wxAnyValueType
        // implementations register a wxString conversion for display.
        wxString s;

        if( aValue.GetAs<wxString>( &s ) )
            return DIFF_VALUE::FromEnum( 0, std::string( s.ToUTF8() ) );
    }

    // Unknown type: produce NONE rather than throwing. The caller decides how
    // to handle an unsupported property (typically by skipping it).
    return DIFF_VALUE();
}


bool DiffValueToWxAny( const DIFF_VALUE& aValue, wxAny& aOut )
{
    switch( aValue.GetType() )
    {
    case DIFF_VALUE::T::NONE:                                      return false;
    case DIFF_VALUE::T::BOOL:     aOut = aValue.AsBool();          break;
    case DIFF_VALUE::T::INT:      aOut = aValue.AsInt();           break;
    case DIFF_VALUE::T::INT64:    aOut = aValue.AsInt64();         break;
    case DIFF_VALUE::T::DOUBLE:   aOut = aValue.AsDouble();        break;
    case DIFF_VALUE::T::STRING:   aOut = aValue.AsString();        break;
    case DIFF_VALUE::T::KIID:     aOut = aValue.AsKiid();          break;
    case DIFF_VALUE::T::VECTOR2I: aOut = aValue.AsVector2I();      break;
    case DIFF_VALUE::T::BOX2I:    aOut = aValue.AsBox2I();         break;
    case DIFF_VALUE::T::COLOR:    aOut = aValue.AsColor();         break;
    case DIFF_VALUE::T::LAYER:    aOut = aValue.AsLayer();         break;
    case DIFF_VALUE::T::ENUM:     aOut = aValue.AsEnum().first;    break;
    case DIFF_VALUE::T::POLYGON_SET: return false;
    }

    return true;
}

} // namespace KICAD_DIFF
