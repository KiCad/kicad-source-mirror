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


#define BOOST_TEST_NO_MAIN
#include <boost/test/unit_test.hpp>
#include <lset.h>
#include <lseq.h>

// Macros for easier test specification
#define PCB_LAYER_COUNT PCB_LAYER_ID_COUNT

BOOST_AUTO_TEST_SUITE(LSETTests)

// Initialize an empty LSET
BOOST_AUTO_TEST_CASE(LSETConstructorEmpty)
{
    LSET set;
    BOOST_CHECK_EQUAL(set.count(), 0);
}

// Initialize LSET from another BASE_SET
BOOST_AUTO_TEST_CASE(LSETConstructorFromBaseSet)
{
    BASE_SET base( PCB_LAYER_ID_COUNT );
    base.set(F_Cu);
    base.set(In1_Cu);

    LSET set(base);
    BOOST_CHECK_EQUAL(set.count(), 2);
    BOOST_CHECK(set.test(F_Cu));
    BOOST_CHECK(set.test(In1_Cu));
}

// Initialize LSET from a specific PCB_LAYER_ID
BOOST_AUTO_TEST_CASE(LSETConstructorFromLayer)
{
    LSET set( { F_Cu } );
    BOOST_CHECK_EQUAL(set.count(), 1);
    BOOST_CHECK(set.test(F_Cu));
}

// Initialize LSET from an initializer list
BOOST_AUTO_TEST_CASE(LSETConstructorFromList)
{
    LSET set({F_Cu, In1_Cu, In2_Cu});
    BOOST_CHECK_EQUAL(set.count(), 3);
    BOOST_CHECK(set.test(F_Cu));
    BOOST_CHECK(set.test(In1_Cu));
    BOOST_CHECK(set.test(In2_Cu));
}

// Initialize LSET from LSEQ
BOOST_AUTO_TEST_CASE(LSETConstructorFromSequence)
{
    LSEQ seq = {F_Cu, In1_Cu, In2_Cu};
    LSET set(seq);
    BOOST_CHECK_EQUAL(set.count(), 3);
    BOOST_CHECK(set.test(F_Cu));
    BOOST_CHECK(set.test(In1_Cu));
    BOOST_CHECK(set.test(In2_Cu));
}

// Test Containment Check
BOOST_AUTO_TEST_CASE(LSETContains)
{
    LSET set({F_Cu, In1_Cu, In2_Cu});
    BOOST_CHECK(set.Contains(F_Cu));
    BOOST_CHECK(set.Contains(In1_Cu));
    BOOST_CHECK(!set.Contains(In30_Cu));
}

// Test Sequence Generation
BOOST_AUTO_TEST_CASE(LSETSequenceGeneration)
{
    LSET set({F_Cu, In1_Cu, In2_Cu});

    LSEQ sequence = set.Seq();
    BOOST_CHECK_EQUAL(sequence.size(), 3);
    BOOST_CHECK_EQUAL(sequence[0], F_Cu);
    BOOST_CHECK_EQUAL(sequence[1], In1_Cu);
    BOOST_CHECK_EQUAL(sequence[2], In2_Cu);
}

// Test Hex and Binary Formatting
BOOST_AUTO_TEST_CASE(LSETFormatting)
{
    LSET set({F_Cu, In1_Cu, In2_Cu});

    std::string hexString = set.FmtHex();
    std::string expectedHexString = "00000000_00000000_00000000_00000051"; // depends on bit ordering

    BOOST_CHECK_EQUAL(hexString, expectedHexString);

    std::string binString = set.FmtBin();
    std::string expectedBinString = "0000_0000|0000_0000|0000_0000|0000_0000|0000_0000|0000_0000|0000_0000|0000_0000|"
                                    "0000_0000|0000_0000|0000_0000|0000_0000|0000_0000|0000_0000|0000_0000|0101_0001"; // depends on bit ordering

    BOOST_CHECK_EQUAL(binString, expectedBinString);
}
// Test ExtractLayer and Flip
BOOST_AUTO_TEST_CASE(LSETManipulations)
{
    LSET set({F_Cu, In1_Cu, In2_Cu});

    // Test ExtractLayer: should extract the layer set or undefined if more than one
    PCB_LAYER_ID extractedLayer = set.ExtractLayer();
    BOOST_CHECK_EQUAL(extractedLayer, UNDEFINED_LAYER);

    // Test Flip: should swap front and back layers
    set.FlipStandardLayers( 4 );
    BOOST_CHECK(set.Contains(B_Cu));
    BOOST_CHECK(set.Contains(In1_Cu)); // Internal layers remain unchanged

    // Test setting a single layer
    set = {F_Cu};
    extractedLayer = set.ExtractLayer();
    BOOST_CHECK_EQUAL(extractedLayer, F_Cu);
    set.FlipStandardLayers();
    extractedLayer = set.ExtractLayer();
    BOOST_CHECK_EQUAL(extractedLayer, B_Cu);
}

// Test Static Mask Methods
BOOST_AUTO_TEST_CASE(LSETStaticMasks)
{
    LSET internalCuMask = LSET::InternalCuMask();
    BOOST_CHECK(internalCuMask.Contains(PCB_LAYER_ID::In1_Cu));
    BOOST_CHECK(internalCuMask.Contains(PCB_LAYER_ID::In30_Cu));
    BOOST_CHECK(!internalCuMask.Contains(PCB_LAYER_ID::F_Cu));
    BOOST_CHECK(!internalCuMask.Contains(PCB_LAYER_ID::B_Cu));
}

// Auxiliary function to compare LSEQ objects
bool compareLSEQ( const LSEQ& seq1, const LSEQ& seq2 )
{
    if( seq1.size() != seq2.size() )
        return false;
    for( size_t i = 0; i < seq1.size(); ++i )
    {
        if( seq1[i] != seq2[i] )
            return false;
    }
    return true;
}


// Test LSET::Seq base case
BOOST_AUTO_TEST_CASE( LSETSeqBaseCase )
{
    LSET lset( { F_Cu, B_Cu, In1_Cu, In2_Cu } );
    LSEQ expected = { F_Cu, B_Cu, In1_Cu, In2_Cu };

    LSEQ result = lset.Seq();
    BOOST_CHECK( compareLSEQ( result, expected ) );
}

// Test LSET::Seq empty case
BOOST_AUTO_TEST_CASE( LSETSeqEmptyCase )
{
    LSET lset;
    LSEQ expected = {};

    LSEQ result = lset.Seq();
    BOOST_CHECK( compareLSEQ( result, expected ) );
}

// Test LSET::SeqStackupTop2Bottom base case
BOOST_AUTO_TEST_CASE( LSETSeqStackupTop2BottomBaseCase )
{
    LSET lset( { F_Cu, B_Cu, In1_Cu, In2_Cu, F_SilkS, B_SilkS, Edge_Cuts, Margin, Dwgs_User } );
    LSEQ expected = { Edge_Cuts, Margin, Dwgs_User, F_SilkS, F_Cu,
                      In1_Cu,    In2_Cu, B_Cu,      B_SilkS };

    LSEQ result = lset.SeqStackupTop2Bottom( UNDEFINED_LAYER );
    BOOST_CHECK( compareLSEQ( result, expected ) );
}

// Test LSET::SeqStackupTop2Bottom prioritizing selected layer
BOOST_AUTO_TEST_CASE( LSETSeqStackupTop2BottomWithSelection )
{
    LSET lset( { F_Cu, B_Cu, In1_Cu, In2_Cu, F_SilkS, B_SilkS, Edge_Cuts, Margin, Dwgs_User } );
    LSEQ expected = { F_SilkS, Edge_Cuts, Margin, Dwgs_User, F_Cu, In1_Cu, In2_Cu, B_Cu, B_SilkS };

    LSEQ result = lset.SeqStackupTop2Bottom( F_SilkS );
    BOOST_CHECK( compareLSEQ( result, expected ) );
}

// Test LSET::SeqStackupForPlotting base case
BOOST_AUTO_TEST_CASE( LSETSeqStackupForPlottingBaseCase )
{
    LSET lset( { F_Cu, B_Cu, In1_Cu, In2_Cu, F_SilkS, B_SilkS, Edge_Cuts, Margin } );
    LSEQ expected = { B_Cu, B_SilkS, In2_Cu, In1_Cu, F_Cu, F_SilkS, Margin, Edge_Cuts };

    LSEQ result = lset.SeqStackupForPlotting();
    BOOST_CHECK( compareLSEQ( result, expected ) );
}


BOOST_AUTO_TEST_SUITE_END()
