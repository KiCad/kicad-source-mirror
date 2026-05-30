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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>
#include <algorithm>
#include <base_units.h>
#include <eda_text.h>
#include <gr_text.h>
#include <text_var_dependency.h>


BOOST_AUTO_TEST_SUITE( EdaText )


// Common starting point for the Bold/thickness decoupling tests (#12911); text size doesn't
// affect their outcomes, so a fixed 200x200 keeps each case focused on what it's testing.
static EDA_TEXT makeText200()
{
    EDA_TEXT t( unityScale );
    t.SetTextSize( VECTOR2I( 200, 200 ), false );
    return t;
}


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


BOOST_AUTO_TEST_CASE( BoldFlagLeavesStoredThicknessAlone )
{
    // Neither SetBold nor SetBoldFlag may mutate the stored width (#12911).
    EDA_TEXT t = makeText200();
    t.SetTextThickness( 30 );

    t.SetBoldFlag( true );
    BOOST_CHECK( t.IsBold() );
    BOOST_CHECK_EQUAL( t.GetTextThickness(), 30 );

    t.SetBold( false );
    BOOST_CHECK( !t.IsBold() );
    BOOST_CHECK_EQUAL( t.GetTextThickness(), 30 );
}


BOOST_AUTO_TEST_CASE( EffectiveWidthAppliesBoldMultiplierForStroke )
{
    // Bold scales the explicit base width by 1.6; 10 and 16 stay under the clamp.
    EDA_TEXT t = makeText200();
    t.SetTextThickness( 10 );

    BOOST_CHECK_EQUAL( t.GetEffectiveTextPenWidth(), 10 );

    t.SetBoldFlag( true );
    BOOST_CHECK_EQUAL( t.GetEffectiveTextPenWidth(), 16 );
}


BOOST_AUTO_TEST_CASE( EffectiveWidthAutoBoldMatchesLegacy )
{
    EDA_TEXT t = makeText200();
    t.SetTextThickness( 0 );
    t.SetBoldFlag( true );

    BOOST_CHECK_EQUAL( t.GetEffectiveTextPenWidth(), GetPenSizeForBold( t.GetTextWidth() ) );
}


BOOST_AUTO_TEST_CASE( MigrateLegacyBoldStrokeWidthRecoversBase )
{
    // Migration divides the baked bold width back to base; auto/non-bold untouched.
    EDA_TEXT bold = makeText200();
    bold.SetTextThickness( 32 );
    bold.SetBoldFlag( true );
    bold.MigrateLegacyBoldStrokeWidth();
    BOOST_CHECK_EQUAL( bold.GetTextThickness(), 20 );
    BOOST_CHECK_EQUAL( bold.GetEffectiveTextPenWidth(), 32 );  // rendered width preserved

    EDA_TEXT normal = makeText200();
    normal.SetTextThickness( 32 );
    normal.MigrateLegacyBoldStrokeWidth();
    BOOST_CHECK_EQUAL( normal.GetTextThickness(), 32 );

    EDA_TEXT autoWidth = makeText200();
    autoWidth.SetTextThickness( 0 );
    autoWidth.SetBoldFlag( true );
    autoWidth.MigrateLegacyBoldStrokeWidth();
    BOOST_CHECK_EQUAL( autoWidth.GetTextThickness(), 0 );

    // A width dividing to <= 1 must not collapse into the auto/"near-zero" special case in
    // GetEffectiveTextPenWidth(), so the floor holds it at 2. That floor means sub-3-IU legacy
    // widths (a scale no real board or schematic uses) don't round-trip exactly; document the
    // actual post-migration effective width rather than leaving it unchecked.
    EDA_TEXT tiny = makeText200();
    tiny.SetTextThickness( 2 );
    tiny.SetBoldFlag( true );
    tiny.MigrateLegacyBoldStrokeWidth();
    BOOST_CHECK_EQUAL( tiny.GetTextThickness(), 2 );
    BOOST_CHECK_EQUAL( tiny.GetEffectiveTextPenWidth(), 3 );
}


BOOST_AUTO_TEST_CASE( AutoThicknessOffFreezesBaseNotEffective )
{
    // Unchecking auto freezes the base, not the rendered width, so Bold doesn't double it.
    EDA_TEXT t = makeText200();
    t.SetTextThickness( 0 );
    t.SetBoldFlag( true );

    int autoEffective = t.GetEffectiveTextPenWidth();

    t.SetAutoThickness( false );
    BOOST_CHECK_EQUAL( t.GetTextThickness(), GetPenSizeForNormal( t.GetTextWidth() ) );
    BOOST_CHECK_EQUAL( t.GetEffectiveTextPenWidth(), autoEffective );
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
