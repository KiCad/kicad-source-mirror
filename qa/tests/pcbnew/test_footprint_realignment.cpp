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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <boost/test/data/test_case.hpp>

#include <optional>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include <footprint_utils.h>

#include <footprint.h>
#include <pad.h>


/**
 * @brief Information needed to make a footprint to use for alignment tests
 */
struct ALIGN_FP_DEF
{
    std::string m_name;
    std::vector<std::pair<std::string, VECTOR2I>> m_pads;
};


std::ostream& operator<<( std::ostream& os, const ALIGN_FP_DEF& fpDef )
{
    os << "ALIGN_FP_DEF: " << fpDef.m_name;
    return os;
}


struct FP_ALIGNMENT_TEST_CASE
{
    std::string m_TestName;
    const ALIGN_FP_DEF& m_OrigPadPositions;
    const ALIGN_FP_DEF& m_NewPadPositions;
    std::optional<VECTOR2I> m_ExpectedPosShift;
    std::optional<EDA_ANGLE> m_ExpectedAngleShift;

    // Alls a data test case to print the name
    friend std::ostream& operator<<( std::ostream& os, const FP_ALIGNMENT_TEST_CASE& testCase )
    {
        return os << testCase.m_TestName;
    }
};


struct FP_REALIGNMENT_TEST_FIXTURE
{
    VECTOR2I m_PosShift;
    EDA_ANGLE m_AngleShift;

    std::unique_ptr<FOOTPRINT> CreateFootprint( const ALIGN_FP_DEF& padPositions )
    {
        std::unique_ptr<FOOTPRINT> fp = std::make_unique<FOOTPRINT>( nullptr );

        for( const auto& [number, pos] : padPositions.m_pads )
        {
            PAD* pad = new PAD( fp.get() );
            pad->SetNumber( number );
            pad->SetFPRelativePosition( pos );
            fp->Add( pad );
        }

        return fp;
    }


    void ExecuteTestCase( const FP_ALIGNMENT_TEST_CASE& testCase )
    {
        const std::unique_ptr<FOOTPRINT> fpOrig = CreateFootprint( testCase.m_OrigPadPositions );
        const std::unique_ptr<FOOTPRINT> fpNew = CreateFootprint( testCase.m_NewPadPositions );

        bool ok = ComputeFootprintShift( *fpOrig, *fpNew, m_PosShift, m_AngleShift );

        if( testCase.m_ExpectedPosShift.has_value() && testCase.m_ExpectedAngleShift.has_value() )
        {
            BOOST_REQUIRE( ok );
            BOOST_TEST( m_PosShift == testCase.m_ExpectedPosShift.value() );
            BOOST_TEST( m_AngleShift == testCase.m_ExpectedAngleShift.value() );
        }
        else
        {
            BOOST_REQUIRE( !ok );
        }

        // Check that when the original footprint is rotated, we compute the same
        // transform, but:
        //  - the move vector is rotated by the same angle
        //  - the angle shift is the same

        fpOrig->SetOrientation( ANGLE_45 );

        bool ok2 = ComputeFootprintShift( *fpOrig, *fpNew, m_PosShift, m_AngleShift );
        if( testCase.m_ExpectedPosShift.has_value() && testCase.m_ExpectedAngleShift.has_value() )
        {
            VECTOR2I expectedPosShift = testCase.m_ExpectedPosShift.value();
            RotatePoint( expectedPosShift, ANGLE_45 );

            BOOST_REQUIRE( ok2 );
            BOOST_TEST( m_PosShift == expectedPosShift );
            BOOST_TEST( m_AngleShift == testCase.m_ExpectedAngleShift.value() );
        }
        else
        {
            BOOST_REQUIRE( !ok2 );
        }
    }
};


static const ALIGN_FP_DEF FpDef_NoPins{
    "No pins",
    {},
};


static const ALIGN_FP_DEF FpDef_DIP4_OrigPin1{
    "DIP4, pin 1 at origin",
    {
        { "1", { 0, 0 } },
        { "2", { 0, 1000 } },
        { "3", { 2000, 0 } },
        { "4", { 2000, 1000 } },
    },
};


static const ALIGN_FP_DEF FpDef_DIP4_OrigBodyCenter{
    "DIP4, body center at origin",
    {
        { "1", { -1000, -500 } },
        { "2", { -1000, 500 } },
        { "3", { 1000, -500 } },
        { "4", { 1000, 500 } },
    },
};


// Pin 1 bottom left = Level B style
// Rotated 90 degrees CCW around origin
static const ALIGN_FP_DEF FpDef_DIP4_Rotated90DegCCW{
    "DIP4, rotated 90 deg CCW",
    {
        { "1", { 0, 0 } },
        { "2", { 1000, 0 } },
        { "3", { 1000, -2000 } },
        { "4", { 0, -2000 } },
    },
};


static const ALIGN_FP_DEF FpDef_DIP4_OrigBodyCenter_Rotated90DegCCW{
    "DIP4, body center at origin, rotated 90 deg CCW",
    {
        { "1", { -500, 1000 } },
        { "2", { 500, 1000 } },
        { "3", { 500, -1000 } },
        { "4", { -500, -1000 } },
    },
};


static const ALIGN_FP_DEF FpDef_DIP4_Pin4Deleted{
    "DIP4, pin 4 deleted",
    {
        { "1", { 0, 0 } },
        { "2", { 0, 1000 } },
        { "3", { 2000, 0 } },
    },
};


static const ALIGN_FP_DEF FpDef_DIP4_Pin4Moved{
    "DIP4, pin 4 moved",
    {
        { "1", { 0, 0 } },
        { "2", { 0, 1000 } },
        { "3", { 2000, 0 } },
        { "4", { 2100, 1000 } },
    },
};


static const ALIGN_FP_DEF FpDef_DIP4_Widened{
    "DIP4, widened",
    {
        { "1", { 0, 0 } },
        { "2", { 0, 1000 } },
        { "3", { 2400, 0 } },
        { "4", { 2400, 1000 } },
    },
};


static const ALIGN_FP_DEF FpDef_DIP4_WithMPs{
    "DIP4, with mechanical pins",
    {
        { "1", { 0, 0 } },
        { "2", { 0, 1000 } },
        { "3", { 2000, 0 } },
        { "4", { 2000, 1000 } },
        { "MP", { -1000, 500 } },
        { "MP", { 3000, 500 } },
    },
};


/**
 * Definition of a test case for self-to-self alignment.
 * This either succeeds (with zero shift and rotation) or fails
 * (if no baseline is available).
 */
struct SELF_ALIGNMENT_TEST_CASE
{
    const ALIGN_FP_DEF& m_FpDef;
    bool                m_ExpectSuccess;
};


static const std::vector<SELF_ALIGNMENT_TEST_CASE> FpSelfToSelfCases = {
    // No baseline, not enough information to align
    { FpDef_NoPins, false },
    { FpDef_DIP4_OrigPin1, true },
    { FpDef_DIP4_OrigBodyCenter, true },
    { FpDef_DIP4_Rotated90DegCCW, true },
    { FpDef_DIP4_OrigBodyCenter_Rotated90DegCCW, true },
    { FpDef_DIP4_Pin4Deleted, true },
    { FpDef_DIP4_Pin4Moved, true },
    { FpDef_DIP4_Widened, true },
    { FpDef_DIP4_WithMPs, true },
};


std::ostream& operator<<( std::ostream& os, const SELF_ALIGNMENT_TEST_CASE& testCase )
{
    os << "SELF_ALIGNMENT_TEST_CASE: " << testCase.m_FpDef.m_name;
    return os;
}


static const std::vector<FP_ALIGNMENT_TEST_CASE> FpAlignmentTestCases = {
    {
            "No pins",
            FpDef_NoPins,
            FpDef_NoPins,
            std::nullopt,
            std::nullopt,
    },
    {
            "Origin point change",
            FpDef_DIP4_OrigPin1,
            FpDef_DIP4_OrigBodyCenter,
            VECTOR2I( 1000, 500 ),
            ANGLE_0,
    },
    {
            "Rotation",
            FpDef_DIP4_OrigPin1,
            FpDef_DIP4_Rotated90DegCCW,
            VECTOR2I( 0, 0 ),
            ANGLE_270,
    },
    {
            "Rotation and origin point change",
            FpDef_DIP4_OrigPin1,
            FpDef_DIP4_OrigBodyCenter_Rotated90DegCCW,
            VECTOR2I( 1000, 500 ),
            ANGLE_270,
    },
    {
            "Pin deleted",
            FpDef_DIP4_OrigPin1,
            FpDef_DIP4_Pin4Deleted,
            VECTOR2I( 0, 0 ),
            ANGLE_0,
    },
    {
            "Pin moved",
            FpDef_DIP4_OrigPin1,
            FpDef_DIP4_Pin4Moved,
            VECTOR2I( 0, 0 ),
            ANGLE_0,
    },
    {
            "Footprint widened (all pins moved)",
            FpDef_DIP4_OrigPin1,
            FpDef_DIP4_Widened,
            VECTOR2I( -200, 0 ),
            ANGLE_0,
    },
    {
            "Ignore same named pads",
            FpDef_DIP4_OrigPin1,
            FpDef_DIP4_WithMPs,
            VECTOR2I( 0, 0 ),
            ANGLE_0,
    },
};


BOOST_FIXTURE_TEST_SUITE( FootprintRealignment, FP_REALIGNMENT_TEST_FIXTURE )


BOOST_DATA_TEST_CASE( TestSelfToSelfCases,
                      boost::unit_test::data::make( FpSelfToSelfCases ),
                      selfCase )
{
    std::optional<VECTOR2I>  expectedPosShift;
    std::optional<EDA_ANGLE> expectedAngleShift;

    // If the alignment can be computed, it's zero shift and zero rotation
    if( selfCase.m_ExpectSuccess )
    {
        expectedPosShift = VECTOR2I( 0, 0 );
        expectedAngleShift = ANGLE_0;
    }

    const FP_ALIGNMENT_TEST_CASE selfAlignTestCase = {
        selfCase.m_FpDef.m_name + "-> " + selfCase.m_FpDef.m_name,
        selfCase.m_FpDef,
        selfCase.m_FpDef,
        expectedPosShift,
        expectedAngleShift,
    };

    ExecuteTestCase( selfAlignTestCase );
}


BOOST_DATA_TEST_CASE( TestAllCases,
                      boost::unit_test::data::make( FpAlignmentTestCases ),
                      testCase )
{
    ExecuteTestCase( testCase );
}

BOOST_AUTO_TEST_SUITE_END()