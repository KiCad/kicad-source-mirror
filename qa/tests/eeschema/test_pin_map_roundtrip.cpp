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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <memory>

#include <pin_map.h>
#include <lib_symbol.h>
#include <richio.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr_parser.h>
#include <symbol_library_common.h>


namespace
{
LIB_ID makeFp( const wxString& aLibNick, const wxString& aName )
{
    LIB_ID id;
    id.SetLibNickname( aLibNick );
    id.SetLibItemName( aName );
    return id;
}


/// Format @a aSymbol to its S-expression text via the production formatter.
std::string format( LIB_SYMBOL& aSymbol )
{
    STRING_FORMATTER formatter;
    SCH_IO_KICAD_SEXPR::FormatLibSymbol( &aSymbol, formatter );
    return formatter.GetString();
}


/// Round-trip a single root symbol through format + parse and return the reloaded copy.
std::unique_ptr<LIB_SYMBOL> roundTrip( LIB_SYMBOL& aSymbol )
{
    std::string                text = format( aSymbol );
    std::vector<LIB_SYMBOL*>   loaded = SCH_IO_KICAD_SEXPR::ParseLibSymbols( text, "pin map test" );

    BOOST_REQUIRE_EQUAL( loaded.size(), 1u );
    return std::unique_ptr<LIB_SYMBOL>( loaded.front() );
}
} // namespace


BOOST_AUTO_TEST_SUITE( PinMapRoundTrip )


BOOST_AUTO_TEST_CASE( RootSymbolWithStackedAndSharedMaps )
{
    LIB_SYMBOL sym( wxS( "LM358" ) );

    PIN_MAP std8( wxS( "STD-8" ) );

    for( int i = 1; i <= 8; ++i )
        std8.SetEntry( wxString::Format( wxS( "%d" ), i ), wxString::Format( wxS( "%d" ), i ) );

    PIN_MAP dfn( wxS( "DFN-8-EP" ) );
    dfn.SetEntry( wxS( "4" ), wxS( "[4,9]" ) );   // V- on pad 4 AND the exposed pad

    sym.PinMaps().AddOrReplace( std8 );
    sym.PinMaps().AddOrReplace( dfn );

    // STD-8 is shared between two footprints; DFN uses its own map.
    sym.SetAssociatedFootprints( {
            { makeFp( wxS( "Package_SO" ), wxS( "SOIC-8" ) ), wxS( "STD-8" ) },
            { makeFp( wxS( "Package_DFN_QFN" ), wxS( "DFN-8-1EP" ) ), wxS( "DFN-8-EP" ) },
            { makeFp( wxS( "Package_SO" ), wxS( "VSSOP-8" ) ), wxS( "STD-8" ) },
    } );

    std::unique_ptr<LIB_SYMBOL> reloaded = roundTrip( sym );

    BOOST_CHECK( reloaded->GetPinMaps() == sym.GetPinMaps() );
    BOOST_CHECK( reloaded->GetAssociatedFootprints() == sym.GetAssociatedFootprints() );

    // The bracketed stacked pad survives verbatim.
    const PIN_MAP* reDfn = reloaded->GetPinMaps().FindByName( wxS( "DFN-8-EP" ) );
    BOOST_REQUIRE( reDfn );
    BOOST_CHECK_EQUAL( reDfn->GetPadNumber( wxS( "4" ) ), wxS( "[4,9]" ) );

    // STD-8 is written once and reused by both associations.
    BOOST_CHECK_EQUAL( reloaded->GetAssociatedFootprints()[0].m_MapName, wxS( "STD-8" ) );
    BOOST_CHECK_EQUAL( reloaded->GetAssociatedFootprints()[2].m_MapName, wxS( "STD-8" ) );
}


BOOST_AUTO_TEST_CASE( EmptyStateProducesNoTokens )
{
    LIB_SYMBOL sym( wxS( "Plain" ) );

    std::string text = format( sym );

    BOOST_CHECK( text.find( "pin_maps" ) == std::string::npos );
    BOOST_CHECK( text.find( "associated_footprints" ) == std::string::npos );
}


BOOST_AUTO_TEST_CASE( DerivedEmitsOnlyOwnSets )
{
    LIB_SYMBOL root( wxS( "root" ) );
    root.PinMaps().AddOrReplace( PIN_MAP( wxS( "ROOT-MAP" ) ) );
    root.SetAssociatedFootprints( { { makeFp( wxS( "L" ), wxS( "F" ) ), wxS( "ROOT-MAP" ) } } );

    // A derived symbol that inherits the bundle writes nothing of its own.
    LIB_SYMBOL inheritor( wxS( "inheritor" ) );
    inheritor.SetParent( &root );

    std::string inheritedText = format( inheritor );
    BOOST_CHECK( inheritedText.find( "pin_maps" ) == std::string::npos );
    BOOST_CHECK( inheritedText.find( "associated_footprints" ) == std::string::npos );

    // A derived symbol that overrides the bundle writes only its own sets.
    LIB_SYMBOL overrider( wxS( "overrider" ) );
    overrider.SetParent( &root );
    overrider.PinMaps().AddOrReplace( PIN_MAP( wxS( "CHILD-MAP" ) ) );

    std::string ownText = format( overrider );
    BOOST_CHECK( ownText.find( "CHILD-MAP" ) != std::string::npos );
    BOOST_CHECK( ownText.find( "ROOT-MAP" ) == std::string::npos );
}


BOOST_AUTO_TEST_CASE( MalformedInputIsRejected )
{
    // An entry missing its pad number must be rejected, not silently accepted.
    std::string bad = "(symbol \"Bad\" (pin_maps (pin_map \"M\" (entry \"1\"))))";

    BOOST_CHECK_THROW( SCH_IO_KICAD_SEXPR::ParseLibSymbols( bad, "bad" ), IO_ERROR );
}


BOOST_AUTO_TEST_CASE( NewerFormatVersionIsRefused )
{
    // The format-version bump means an older reader must refuse a newer file.  Prove the gate with
    // a version far above the supported maximum (issue #2282, plan verification step 4).
    std::string text =
            "(kicad_symbol_lib (version 99999999) (generator \"test\") (generator_version \"1\"))";

    STRING_LINE_READER        reader( text, "future" );
    SCH_IO_KICAD_SEXPR_PARSER parser( &reader );
    LIB_SYMBOL_MAP            map;

    BOOST_CHECK_THROW( parser.ParseLib( map ), IO_ERROR );
}


BOOST_AUTO_TEST_SUITE_END()
