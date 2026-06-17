/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <sim/sim_model_multiunit.h>
#include <sim/sim_model_subckt.h>
#include <sim/spice_generator.h>
#include <ki_exception.h>

#include <memory>
#include <set>

#include <wx/tokenzr.h>


// Builds a 5-pin single-unit opamp base model (+IN -IN VCC VEE OUT) for wrapper tests.
static std::unique_ptr<SIM_MODEL_SUBCKT> makeBaseOpamp()
{
    auto base = std::make_unique<SIM_MODEL_SUBCKT>();
    base->AddPin( { "+IN", "1" } );
    base->AddPin( { "-IN", "2" } );
    base->AddPin( { "VCC", "3" } );
    base->AddPin( { "VEE", "4" } );
    base->AddPin( { "OUT", "5" } );
    return base;
}


// Dual opamp: units 1 and 2 are functional gates, unit 3 carries the shared supply pins.
static std::vector<UNIT_PIN_MAP> dualOpampMaps()
{
    return {
        { 1, { { wxS( "3" ), wxS( "+IN" ) }, { wxS( "2" ), wxS( "-IN" ) }, { wxS( "1" ), wxS( "OUT" ) } } },
        { 2, { { wxS( "5" ), wxS( "+IN" ) }, { wxS( "6" ), wxS( "-IN" ) }, { wxS( "7" ), wxS( "OUT" ) } } },
        { 3, { { wxS( "4" ), wxS( "VEE" ) }, { wxS( "8" ), wxS( "VCC" ) } } },
    };
}


static std::vector<wxString> splitTokens( const wxString& aLine )
{
    std::vector<wxString> tokens;
    wxStringTokenizer     tokenizer( aLine, wxS( " \t" ), wxTOKEN_STRTOK );

    while( tokenizer.HasMoreTokens() )
        tokens.push_back( tokenizer.GetNextToken() );

    return tokens;
}


BOOST_AUTO_TEST_SUITE( SimModelMultiunit )


BOOST_AUTO_TEST_CASE( ParsePinsPreservesOrder )
{
    std::vector<std::pair<wxString, wxString>> pairs =
            ParseSimPinsTokens( wxS( "3=+IN 2=-IN 1=OUT" ), wxS( "U1" ) );

    const std::vector<std::pair<wxString, wxString>> expected = {
        { wxS( "3" ), wxS( "+IN" ) }, { wxS( "2" ), wxS( "-IN" ) }, { wxS( "1" ), wxS( "OUT" ) } };

    BOOST_CHECK( pairs == expected );
}


BOOST_AUTO_TEST_CASE( ParsePinsAllowsSharedModelPinAcrossSymbolPins )
{
    // Two different symbol pins mapping to the same model pin within one unit is legal.
    BOOST_CHECK_NO_THROW( ParseSimPinsTokens( wxS( "3=+IN 5=+IN" ), wxS( "U1" ) ) );
}


BOOST_AUTO_TEST_CASE( ParsePinsDuplicateIdenticalIsDeduped )
{
    std::vector<std::pair<wxString, wxString>> pairs =
            ParseSimPinsTokens( wxS( "3=+IN 3=+IN" ), wxS( "U1" ) );

    BOOST_REQUIRE_EQUAL( pairs.size(), 1 );
    BOOST_CHECK_EQUAL( pairs[0].first, wxS( "3" ) );
    BOOST_CHECK_EQUAL( pairs[0].second, wxS( "+IN" ) );
}


BOOST_AUTO_TEST_CASE( ParsePinsMalformedMissingValue )
{
    BOOST_CHECK_THROW( ParseSimPinsTokens( wxS( "5=" ), wxS( "U1" ) ), IO_ERROR );
}


BOOST_AUTO_TEST_CASE( ParsePinsMalformedMissingNumber )
{
    BOOST_CHECK_THROW( ParseSimPinsTokens( wxS( "=OUT" ), wxS( "U1" ) ), IO_ERROR );
}


BOOST_AUTO_TEST_CASE( ParsePinsMalformedNoEquals )
{
    BOOST_CHECK_THROW( ParseSimPinsTokens( wxS( "OUT" ), wxS( "U1" ) ), IO_ERROR );
}


BOOST_AUTO_TEST_CASE( ParsePinsConflictThrows )
{
    // Same symbol pin mapped to two different model pins within one unit is an error.
    BOOST_CHECK_THROW( ParseSimPinsTokens( wxS( "3=+IN 3=-IN" ), wxS( "U1" ) ), IO_ERROR );
}


BOOST_AUTO_TEST_CASE( WrapperSyntheticPinList )
{
    std::unique_ptr<SIM_MODEL_SUBCKT> base = makeBaseOpamp();
    SIM_MODEL_MULTIUNIT wrapper( *base, wxS( "uopamp_single" ), dualOpampMaps(),
                                 { wxS( "VCC" ), wxS( "VEE" ) } );

    BOOST_CHECK_EQUAL( wrapper.GetInstanceCount(), 2 );
    BOOST_REQUIRE_EQUAL( wrapper.GetPinCount(), 8 );

    std::set<wxString>    symbolPins;
    std::set<std::string> nodeNames;

    for( const SIM_MODEL_PIN& pin : wrapper.GetPins() )
    {
        symbolPins.insert( pin.symbolPinNumber );
        nodeNames.insert( pin.modelPinName );
    }

    BOOST_CHECK_EQUAL( symbolPins.size(), 8 );
    BOOST_CHECK_EQUAL( nodeNames.size(), 8 );

    for( int ii = 1; ii <= 8; ++ii )
        BOOST_CHECK_MESSAGE( symbolPins.count( wxString::Format( wxS( "%d" ), ii ) ),
                             "missing outer pin for symbol pin " << ii );
}


BOOST_AUTO_TEST_CASE( WrapperModelLine )
{
    std::unique_ptr<SIM_MODEL_SUBCKT> base = makeBaseOpamp();
    SIM_MODEL_MULTIUNIT wrapper( *base, wxS( "uopamp_single" ), dualOpampMaps(),
                                 { wxS( "VCC" ), wxS( "VEE" ) } );

    SPICE_ITEM item;
    item.refName = "U1";

    wxString          modelLine( wrapper.SpiceGenerator().ModelLine( item ) );
    wxArrayString     lines = wxStringTokenize( modelLine, wxS( "\n" ), wxTOKEN_STRTOK );

    BOOST_REQUIRE_EQUAL( lines.size(), 4 );

    // .subckt header: node order matches the synthetic outer pin order.
    std::vector<wxString> header = splitTokens( lines[0] );
    BOOST_REQUIRE_GE( header.size(), 2 );
    BOOST_CHECK_EQUAL( header[0], wxS( ".subckt" ) );
    BOOST_CHECK_EQUAL( header[1], wrapper.GetSignature() );

    const std::vector<wxString> expectedNodes = { wxS( "n3" ), wxS( "n2" ), wxS( "n1" ),
                                                  wxS( "n5" ), wxS( "n6" ), wxS( "n7" ),
                                                  wxS( "n8" ), wxS( "n4" ) };
    std::vector<wxString>       headerNodes( header.begin() + 2, header.end() );
    BOOST_CHECK( headerNodes == expectedNodes );

    // Inner instances follow base pin order (+IN -IN VCC VEE OUT); shared VCC/VEE nodes are
    // identical across X1 and X2.
    BOOST_CHECK_EQUAL( lines[1], wxS( "X1 n3 n2 n8 n4 n1 uopamp_single" ) );
    BOOST_CHECK_EQUAL( lines[2], wxS( "X2 n5 n6 n8 n4 n7 uopamp_single" ) );
    BOOST_CHECK_EQUAL( lines[3], wxS( ".ends" ) );
}


BOOST_AUTO_TEST_CASE( WrapperDedupSignatureStable )
{
    std::unique_ptr<SIM_MODEL_SUBCKT> base1 = makeBaseOpamp();
    std::unique_ptr<SIM_MODEL_SUBCKT> base2 = makeBaseOpamp();

    SIM_MODEL_MULTIUNIT a( *base1, wxS( "uopamp_single" ), dualOpampMaps(),
                           { wxS( "VCC" ), wxS( "VEE" ) } );
    SIM_MODEL_MULTIUNIT b( *base2, wxS( "uopamp_single" ), dualOpampMaps(),
                           { wxS( "VCC" ), wxS( "VEE" ) } );

    // Identical components must share one wrapper definition.
    BOOST_CHECK_EQUAL( a.GetSignature(), b.GetSignature() );

    // A different base model must produce a different wrapper.
    SIM_MODEL_MULTIUNIT c( *base1, wxS( "other_opamp" ), dualOpampMaps(),
                           { wxS( "VCC" ), wxS( "VEE" ) } );
    BOOST_CHECK( a.GetSignature() != c.GetSignature() );
}


BOOST_AUTO_TEST_CASE( WrapperCurrentNames )
{
    std::unique_ptr<SIM_MODEL_SUBCKT> base = makeBaseOpamp();
    SIM_MODEL_MULTIUNIT wrapper( *base, wxS( "uopamp_single" ), dualOpampMaps(),
                                 { wxS( "VCC" ), wxS( "VEE" ) } );

    SPICE_ITEM item;
    item.refName = "U1";

    std::vector<std::string> names = wrapper.SpiceGenerator().CurrentNames( item );

    // One outer-pin current per synthetic pin; no inner-instance currents.
    BOOST_CHECK_EQUAL( names.size(), 8 );

    for( const std::string& name : names )
        BOOST_CHECK_MESSAGE( name.rfind( "I(XU1:", 0 ) == 0, "unexpected current name " << name );
}


BOOST_AUTO_TEST_CASE( WrapperUnknownSharedPinThrows )
{
    std::unique_ptr<SIM_MODEL_SUBCKT> base = makeBaseOpamp();

    BOOST_CHECK_THROW( SIM_MODEL_MULTIUNIT( *base, wxS( "uopamp_single" ), dualOpampMaps(),
                                            { wxS( "VGG" ) } ),
                       IO_ERROR );
}


BOOST_AUTO_TEST_CASE( WrapperUnassignedBasePinThrows )
{
    std::unique_ptr<SIM_MODEL_SUBCKT> base = makeBaseOpamp();

    // OUT is mapped by no unit and is not shared.
    std::vector<UNIT_PIN_MAP> maps = {
        { 1, { { wxS( "3" ), wxS( "+IN" ) }, { wxS( "2" ), wxS( "-IN" ) } } },
        { 2, { { wxS( "5" ), wxS( "+IN" ) }, { wxS( "6" ), wxS( "-IN" ) } } },
        { 3, { { wxS( "4" ), wxS( "VEE" ) }, { wxS( "8" ), wxS( "VCC" ) } } },
    };

    BOOST_CHECK_THROW( SIM_MODEL_MULTIUNIT( *base, wxS( "uopamp_single" ), maps,
                                            { wxS( "VCC" ), wxS( "VEE" ) } ),
                       IO_ERROR );
}


BOOST_AUTO_TEST_CASE( WrapperSharedPinMissingNetThrows )
{
    std::unique_ptr<SIM_MODEL_SUBCKT> base = makeBaseOpamp();

    // VCC is declared shared but no unit maps it.
    std::vector<UNIT_PIN_MAP> maps = {
        { 1, { { wxS( "3" ), wxS( "+IN" ) }, { wxS( "2" ), wxS( "-IN" ) }, { wxS( "1" ), wxS( "OUT" ) } } },
        { 2, { { wxS( "5" ), wxS( "+IN" ) }, { wxS( "6" ), wxS( "-IN" ) }, { wxS( "7" ), wxS( "OUT" ) } } },
        { 3, { { wxS( "4" ), wxS( "VEE" ) } } },
    };

    BOOST_CHECK_THROW( SIM_MODEL_MULTIUNIT( *base, wxS( "uopamp_single" ), maps,
                                            { wxS( "VCC" ), wxS( "VEE" ) } ),
                       IO_ERROR );
}


BOOST_AUTO_TEST_CASE( WrapperSharedPinMultipleNetsThrows )
{
    std::unique_ptr<SIM_MODEL_SUBCKT> base = makeBaseOpamp();

    // VCC is mapped to two different symbol pins (4 and 8), so it cannot resolve to one net.
    std::vector<UNIT_PIN_MAP> maps = {
        { 1, { { wxS( "3" ), wxS( "+IN" ) }, { wxS( "2" ), wxS( "-IN" ) }, { wxS( "1" ), wxS( "OUT" ) },
               { wxS( "4" ), wxS( "VCC" ) } } },
        { 2, { { wxS( "5" ), wxS( "+IN" ) }, { wxS( "6" ), wxS( "-IN" ) }, { wxS( "7" ), wxS( "OUT" ) } } },
        { 3, { { wxS( "9" ), wxS( "VEE" ) }, { wxS( "8" ), wxS( "VCC" ) } } },
    };

    BOOST_CHECK_THROW( SIM_MODEL_MULTIUNIT( *base, wxS( "uopamp_single" ), maps,
                                            { wxS( "VCC" ), wxS( "VEE" ) } ),
                       IO_ERROR );
}


BOOST_AUTO_TEST_CASE( WrapperFourInstancesSharedRails )
{
    // Quad gate: a single-gate model (A B Y VCC GND) repeated four times, sharing VCC/GND.
    auto base = std::make_unique<SIM_MODEL_SUBCKT>();
    base->AddPin( { "A", "1" } );
    base->AddPin( { "B", "2" } );
    base->AddPin( { "Y", "3" } );
    base->AddPin( { "VCC", "4" } );
    base->AddPin( { "GND", "5" } );

    std::vector<UNIT_PIN_MAP> maps = {
        { 1, { { wxS( "1" ), wxS( "A" ) }, { wxS( "2" ), wxS( "B" ) }, { wxS( "3" ), wxS( "Y" ) } } },
        { 2, { { wxS( "4" ), wxS( "A" ) }, { wxS( "5" ), wxS( "B" ) }, { wxS( "6" ), wxS( "Y" ) } } },
        { 3, { { wxS( "9" ), wxS( "A" ) }, { wxS( "10" ), wxS( "B" ) }, { wxS( "8" ), wxS( "Y" ) } } },
        { 4, { { wxS( "12" ), wxS( "A" ) }, { wxS( "13" ), wxS( "B" ) }, { wxS( "11" ), wxS( "Y" ) } } },
        { 5, { { wxS( "14" ), wxS( "VCC" ) }, { wxS( "7" ), wxS( "GND" ) } } },
    };

    SIM_MODEL_MULTIUNIT wrapper( *base, wxS( "nand_single" ), maps, { wxS( "VCC" ), wxS( "GND" ) } );

    BOOST_CHECK_EQUAL( wrapper.GetInstanceCount(), 4 );

    SPICE_ITEM item;
    item.refName = "U1";

    wxArrayString lines = wxStringTokenize( wxString( wrapper.SpiceGenerator().ModelLine( item ) ),
                                            wxS( "\n" ), wxTOKEN_STRTOK );

    std::vector<std::vector<wxString>> innerLines;

    for( const wxString& line : lines )
    {
        if( line.EndsWith( wxS( "nand_single" ) ) )
            innerLines.push_back( splitTokens( line ) );
    }

    BOOST_REQUIRE_EQUAL( innerLines.size(), 4u );

    // Inner node order is A B Y VCC GND; shared VCC (index 4) and GND (index 5) are identical
    // across all four instances.
    for( const std::vector<wxString>& inner : innerLines )
    {
        BOOST_REQUIRE_EQUAL( inner.size(), 7u );
        BOOST_CHECK_EQUAL( inner[4], innerLines[0][4] );
        BOOST_CHECK_EQUAL( inner[5], innerLines[0][5] );
    }
}


BOOST_AUTO_TEST_CASE( WrapperPrivatePerInstancePinIsNotConnected )
{
    // A sixth base pin EN mapped by only one instance is that instance's private pin and
    // not-connected on the other instance (not an error).
    auto base = std::make_unique<SIM_MODEL_SUBCKT>();
    base->AddPin( { "+IN", "1" } );
    base->AddPin( { "-IN", "2" } );
    base->AddPin( { "VCC", "3" } );
    base->AddPin( { "VEE", "4" } );
    base->AddPin( { "OUT", "5" } );
    base->AddPin( { "EN", "6" } );

    std::vector<UNIT_PIN_MAP> maps = {
        { 1, { { wxS( "3" ), wxS( "+IN" ) }, { wxS( "2" ), wxS( "-IN" ) }, { wxS( "1" ), wxS( "OUT" ) },
               { wxS( "9" ), wxS( "EN" ) } } },
        { 2, { { wxS( "5" ), wxS( "+IN" ) }, { wxS( "6" ), wxS( "-IN" ) }, { wxS( "7" ), wxS( "OUT" ) } } },
        { 3, { { wxS( "4" ), wxS( "VEE" ) }, { wxS( "8" ), wxS( "VCC" ) } } },
    };

    SIM_MODEL_MULTIUNIT wrapper( *base, wxS( "uopamp_single" ), maps,
                                 { wxS( "VCC" ), wxS( "VEE" ) } );

    BOOST_CHECK_EQUAL( wrapper.GetInstanceCount(), 2 );

    SPICE_ITEM item;
    item.refName = "U1";
    wxString    modelLine( wrapper.SpiceGenerator().ModelLine( item ) );

    // Instance 2 has no EN mapping, so its EN node is an internal not-connected node.
    BOOST_CHECK_MESSAGE( modelLine.Contains( wxS( "nc_" ) ),
                         "expected a not-connected node for the unmapped private pin" );
}


BOOST_AUTO_TEST_SUITE_END()
