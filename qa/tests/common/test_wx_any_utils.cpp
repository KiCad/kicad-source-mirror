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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <properties/wx_any_utils.h>
#include <properties/property.h>

#include <geometry/eda_angle.h>
#include <gal/color4d.h>
#include <kiid.h>
#include <layer_ids.h>
#include <math/box2.h>
#include <math/vector2d.h>

#include <wx/any.h>
#include <wx/string.h>

#include <optional>


// Enum-backed properties store the enum type in the wxAny, not int, so the
// comparator needs the property's wxPGChoices to compare via the integer
// payload. We register a small enum to exercise that path. The enum and its
// wxAny adapter must live at global scope (outside the Boost test suite
// namespace) so the wxAnyValueTypeImpl specialization is well-formed.

enum class WX_ANY_TEST_ENUM
{
    ALPHA = 0,
    BETA  = 1,
    GAMMA = 2
};


ENUM_TO_WXANY( WX_ANY_TEST_ENUM );


BOOST_AUTO_TEST_SUITE( WxAnyUtils )


BOOST_AUTO_TEST_CASE( SameType_SameValue )
{
    BOOST_CHECK( KiWxAnyEquals( wxAny( 42 ), wxAny( 42 ) ) );
    BOOST_CHECK( KiWxAnyEquals( wxAny( wxString( "x" ) ), wxAny( wxString( "x" ) ) ) );
    BOOST_CHECK( KiWxAnyEquals( wxAny( true ), wxAny( true ) ) );
    BOOST_CHECK( KiWxAnyEquals( wxAny( std::string( "y" ) ), wxAny( std::string( "y" ) ) ) );
}


BOOST_AUTO_TEST_CASE( SameType_DifferentValue )
{
    BOOST_CHECK( !KiWxAnyEquals( wxAny( 42 ), wxAny( 43 ) ) );
    BOOST_CHECK( !KiWxAnyEquals( wxAny( true ), wxAny( false ) ) );
    BOOST_CHECK( !KiWxAnyEquals( wxAny( wxString( "a" ) ), wxAny( wxString( "b" ) ) ) );
    BOOST_CHECK( !KiWxAnyEquals( wxAny( std::string( "a" ) ), wxAny( std::string( "b" ) ) ) );
}


BOOST_AUTO_TEST_CASE( DifferentTypes )
{
    // Mixed types should not compare equal; the comparator treats a type
    // mismatch as inequality.
    BOOST_CHECK( !KiWxAnyEquals( wxAny( 42 ), wxAny( 42.0 ) ) );
    BOOST_CHECK( !KiWxAnyEquals( wxAny( true ), wxAny( 1 ) ) );
}


BOOST_AUTO_TEST_CASE( NumericTypes )
{
    // The comparator must cover long, long long, unsigned, and float in
    // addition to int and double; otherwise a property carrying e.g. a float
    // silently produces a false delta on every comparison.
    BOOST_CHECK( KiWxAnyEquals( wxAny( 1.5 ), wxAny( 1.5 ) ) );
    BOOST_CHECK( !KiWxAnyEquals( wxAny( 1.5 ), wxAny( 2.5 ) ) );

    BOOST_CHECK( KiWxAnyEquals( wxAny( 2.5f ), wxAny( 2.5f ) ) );
    BOOST_CHECK( !KiWxAnyEquals( wxAny( 2.5f ), wxAny( 3.5f ) ) );

    BOOST_CHECK( KiWxAnyEquals( wxAny( 42L ), wxAny( 42L ) ) );
    BOOST_CHECK( KiWxAnyEquals( wxAny( 42LL ), wxAny( 42LL ) ) );
    BOOST_CHECK( !KiWxAnyEquals( wxAny( 42LL ), wxAny( 43LL ) ) );

    BOOST_CHECK( KiWxAnyEquals( wxAny( 42u ), wxAny( 42u ) ) );
    BOOST_CHECK( !KiWxAnyEquals( wxAny( 42u ), wxAny( 43u ) ) );
}


BOOST_AUTO_TEST_CASE( OptionalInt )
{
    std::optional<int> a = 5, b = 5, c = 6, e;

    BOOST_CHECK( KiWxAnyEquals( wxAny( a ), wxAny( b ) ) );
    BOOST_CHECK( !KiWxAnyEquals( wxAny( a ), wxAny( c ) ) );
    BOOST_CHECK( !KiWxAnyEquals( wxAny( a ), wxAny( e ) ) );
    BOOST_CHECK( KiWxAnyEquals( wxAny( e ), wxAny( std::optional<int>{} ) ) );
}


BOOST_AUTO_TEST_CASE( OptionalDouble )
{
    std::optional<double> a = 5.0, b = 5.0, c = 6.0, e;

    BOOST_CHECK( KiWxAnyEquals( wxAny( a ), wxAny( b ) ) );
    BOOST_CHECK( !KiWxAnyEquals( wxAny( a ), wxAny( c ) ) );
    BOOST_CHECK( !KiWxAnyEquals( wxAny( a ), wxAny( e ) ) );
}


BOOST_AUTO_TEST_CASE( EdaAngle )
{
    EDA_ANGLE a( 90.0, DEGREES_T ), b( 90.0, DEGREES_T ), c( 45.0, DEGREES_T );

    BOOST_CHECK( KiWxAnyEquals( wxAny( a ), wxAny( b ) ) );
    BOOST_CHECK( !KiWxAnyEquals( wxAny( a ), wxAny( c ) ) );
}


BOOST_AUTO_TEST_CASE( Vector2I )
{
    VECTOR2I a( 1, 2 ), b( 1, 2 ), c( 3, 4 );

    BOOST_CHECK( KiWxAnyEquals( wxAny( a ), wxAny( b ) ) );
    BOOST_CHECK( !KiWxAnyEquals( wxAny( a ), wxAny( c ) ) );
}


BOOST_AUTO_TEST_CASE( Box2I )
{
    BOX2I a( VECTOR2I( 0, 0 ), VECTOR2I( 10, 10 ) );
    BOX2I b( VECTOR2I( 0, 0 ), VECTOR2I( 10, 10 ) );
    BOX2I c( VECTOR2I( 1, 0 ), VECTOR2I( 10, 10 ) );

    BOOST_CHECK( KiWxAnyEquals( wxAny( a ), wxAny( b ) ) );
    BOOST_CHECK( !KiWxAnyEquals( wxAny( a ), wxAny( c ) ) );
}


BOOST_AUTO_TEST_CASE( Color )
{
    KIGFX::COLOR4D a( 1.0, 0.5, 0.0, 1.0 );
    KIGFX::COLOR4D b( 1.0, 0.5, 0.0, 1.0 );
    KIGFX::COLOR4D c( 0.5, 0.5, 0.0, 1.0 );

    BOOST_CHECK( KiWxAnyEquals( wxAny( a ), wxAny( b ) ) );
    BOOST_CHECK( !KiWxAnyEquals( wxAny( a ), wxAny( c ) ) );
}


BOOST_AUTO_TEST_CASE( Kiid )
{
    KIID a, b;

    BOOST_CHECK( KiWxAnyEquals( wxAny( a ), wxAny( a ) ) );
    BOOST_CHECK( !KiWxAnyEquals( wxAny( a ), wxAny( b ) ) );
}


BOOST_AUTO_TEST_CASE( PcbLayerId )
{
    BOOST_CHECK( KiWxAnyEquals( wxAny( F_Cu ), wxAny( F_Cu ) ) );
    BOOST_CHECK( !KiWxAnyEquals( wxAny( F_Cu ), wxAny( B_Cu ) ) );
}


BOOST_AUTO_TEST_CASE( UnsupportedReturnsFalse )
{
    // Unsupported types conservatively report "not equal" so a property delta
    // is generated rather than silently passing.
    struct Custom
    {
        int n;
    };

    BOOST_CHECK( !KiWxAnyEquals( wxAny( Custom{ 1 } ), wxAny( Custom{ 1 } ) ) );
}


namespace
{

struct ENUM_OWNER
{
    WX_ANY_TEST_ENUM getValue() const { return m_value; }
    void             setValue( WX_ANY_TEST_ENUM aValue ) { m_value = aValue; }

    WX_ANY_TEST_ENUM m_value = WX_ANY_TEST_ENUM::ALPHA;
};


struct ENUM_TEST_FIXTURE
{
    ENUM_TEST_FIXTURE()
    {
        ENUM_MAP<WX_ANY_TEST_ENUM>::Instance()
                .Map( WX_ANY_TEST_ENUM::ALPHA, wxT( "ALPHA" ) )
                .Map( WX_ANY_TEST_ENUM::BETA, wxT( "BETA" ) )
                .Map( WX_ANY_TEST_ENUM::GAMMA, wxT( "GAMMA" ) );
    }

    PROPERTY_ENUM<ENUM_OWNER, WX_ANY_TEST_ENUM> prop{ wxT( "Value" ), &ENUM_OWNER::setValue,
                                                      &ENUM_OWNER::getValue };
};

} // namespace


BOOST_FIXTURE_TEST_CASE( EnumViaProperty, ENUM_TEST_FIXTURE )
{
    BOOST_REQUIRE( prop.HasChoices() );

    wxAny alpha = WX_ANY_TEST_ENUM::ALPHA;
    wxAny alpha2 = WX_ANY_TEST_ENUM::ALPHA;
    wxAny beta = WX_ANY_TEST_ENUM::BETA;

    BOOST_CHECK( KiWxAnyEquals( alpha, alpha2, &prop ) );
    BOOST_CHECK( !KiWxAnyEquals( alpha, beta, &prop ) );
}


BOOST_AUTO_TEST_SUITE_END()
