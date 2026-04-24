/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Wayne Stambaugh <stambaughw@gmail.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>
#include <algorithm>
#include <base_units.h>
#include <eda_text.h>
#include <text_var_dependency.h>


BOOST_AUTO_TEST_SUITE( EdaText )


BOOST_AUTO_TEST_CASE( Compare )
{
    std::hash<EDA_TEXT> hasher;
    EDA_TEXT a( unityScale );
    EDA_TEXT b( unityScale );

    BOOST_CHECK_EQUAL( a, b );
    BOOST_CHECK_EQUAL( hasher( a ), hasher( b ) );

    a.SetText( wxS( "A" ) );
    BOOST_CHECK_GT( a, b );
    BOOST_CHECK_NE( hasher( a ), hasher( b ) );

    b.SetText( wxS( "B" ) );
    BOOST_CHECK_LT( a, b );
    BOOST_CHECK_NE( hasher( a ), hasher( b ) );

    a.SetText( wxS( "B" ) );
    a.SetTextPos( VECTOR2I( 1, 0 ) );
    BOOST_CHECK_GT( a, b );

    a.SetTextPos( VECTOR2I( -1, 0 ) );
    BOOST_CHECK_LT( a, b );

    a.SetTextPos( VECTOR2I( 0, 0 ) );
    b.SetTextPos( VECTOR2I( 0, 1 ) );
    BOOST_CHECK_LT( a, b );

    b.SetTextPos( VECTOR2I( 0, -1 ) );
    BOOST_CHECK_GT( a, b );

    // Text attributes are tested in the TEXT_ATTRIBUTES unit tests.
}


BOOST_AUTO_TEST_CASE( TextVarReferences_EmptyWhenNoVars )
{
    EDA_TEXT t( unityScale );
    t.SetText( wxS( "plain text" ) );
    BOOST_CHECK( t.GetTextVarReferences().empty() );
    BOOST_CHECK( !t.HasTextVars() );
}


BOOST_AUTO_TEST_CASE( TextVarReferences_CapturedAfterSetText )
{
    EDA_TEXT t( unityScale );
    t.SetText( wxS( "${VALUE} - ${U1:MPN}" ) );

    BOOST_CHECK( t.HasTextVars() );

    const auto& refs = t.GetTextVarReferences();
    BOOST_REQUIRE_EQUAL( refs.size(), 2u );

    auto hasKey = [&]( TEXT_VAR_REF_KEY::KIND k, const wxString& p, const wxString& s )
    {
        return std::any_of( refs.begin(), refs.end(),
                            [&]( const TEXT_VAR_REF_KEY& ref )
                            { return ref.kind == k && ref.primary == p && ref.secondary == s; } );
    };

    BOOST_CHECK( hasKey( TEXT_VAR_REF_KEY::KIND::LOCAL, wxS( "VALUE" ), wxS( "" ) ) );
    BOOST_CHECK( hasKey( TEXT_VAR_REF_KEY::KIND::CROSS_REF, wxS( "U1" ), wxS( "MPN" ) ) );
}


BOOST_AUTO_TEST_CASE( TextVarReferences_InvalidatedOnRetext )
{
    EDA_TEXT t( unityScale );
    t.SetText( wxS( "${VALUE}" ) );
    BOOST_REQUIRE_EQUAL( t.GetTextVarReferences().size(), 1u );

    t.SetText( wxS( "${REFERENCE}" ) );
    const auto& refs = t.GetTextVarReferences();
    BOOST_REQUIRE_EQUAL( refs.size(), 1u );
    BOOST_CHECK( refs[0].primary == wxS( "REFERENCE" ) );
}


BOOST_AUTO_TEST_CASE( TextVarReferences_StableReferenceBetweenReads )
{
    // Refs are populated eagerly in cacheShownText() and stored as a plain
    // member; two successive const reads must return the same storage address
    // so listeners can hold the reference without copying.
    EDA_TEXT t( unityScale );
    t.SetText( wxS( "${X} and ${Y}" ) );

    const auto& first = t.GetTextVarReferences();
    const auto& second = t.GetTextVarReferences();

    BOOST_REQUIRE_EQUAL( first.size(), 2u );
    BOOST_CHECK( &first == &second );
}


BOOST_AUTO_TEST_CASE( TextVarReferences_EscapedLiteralNotEdge )
{
    // Finding 6 from codex review: extraction must run on raw m_text, not
    // m_shown_text (post-UnescapeString). A backslash-escaped `\${VAR}` is a
    // user literal and must NOT produce a dependency edge.
    EDA_TEXT t( unityScale );
    t.SetText( wxS( "literal: \\${VAR}" ) );

    BOOST_CHECK( t.GetTextVarReferences().empty() );
}


BOOST_AUTO_TEST_SUITE_END()
