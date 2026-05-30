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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 */

/**
 * @file test_altium_custom_pad_issue23933.cpp
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/23933
 */

#include <pcbnew_utils/board_test_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <pcbnew/pcb_io/altium/pcb_io_altium_designer.h>

#include <footprint.h>
#include <pad.h>
#include <padstack.h>
#include <base_units.h>
#include <geometry/shape_poly_set.h>

#include <map>
#include <memory>


struct ALTIUM_CUSTOM_PAD_FIXTURE
{
    ALTIUM_CUSTOM_PAD_FIXTURE() {}

    PCB_IO_ALTIUM_DESIGNER altiumPlugin;
};


BOOST_FIXTURE_TEST_SUITE( AltiumCustomPadIssue23933, ALTIUM_CUSTOM_PAD_FIXTURE )


BOOST_AUTO_TEST_CASE( CustomShapePadIsSinglePad )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "plugins/altium/pcblib/";
    wxString    libraryPath = dataPath + "GW_JTLMS1_CM.PcbLib";

    std::unique_ptr<FOOTPRINT> footprint(
            altiumPlugin.FootprintLoad( libraryPath, wxT( "GW JTLMS1_CM" ), false, nullptr ) );
    BOOST_REQUIRE( footprint );

    std::map<wxString, std::vector<PAD*>> padsByNumber;

    for( PAD* pad : footprint->Pads() )
    {
        if( !pad->GetNumber().IsEmpty() )
            padsByNumber[pad->GetNumber()].push_back( pad );
    }

    // Anchors from the Altium source: pad 1 at (1.31mm, 0), pad 2 at (-0.69mm, 0)
    std::map<wxString, VECTOR2I> expectedAnchors = {
            { wxT( "1" ), VECTOR2I( pcbIUScale.mmToIU( 1.31 ), 0 ) },
            { wxT( "2" ), VECTOR2I( pcbIUScale.mmToIU( -0.69 ), 0 ) }
    };

    for( const auto& [number, anchor] : expectedAnchors )
    {
        BOOST_TEST_CONTEXT( wxString::Format( wxT( "Pad number '%s'" ), number ) )
        {
            BOOST_REQUIRE_EQUAL( padsByNumber[number].size(), 1u );

            PAD* pad = padsByNumber[number].front();

            BOOST_CHECK_EQUAL( static_cast<int>( pad->GetShape( PADSTACK::ALL_LAYERS ) ),
                               static_cast<int>( PAD_SHAPE::CUSTOM ) );
            BOOST_CHECK_EQUAL( pad->GetPosition().x, anchor.x );
            BOOST_CHECK_EQUAL( pad->GetPosition().y, anchor.y );

            bool hasPrimitive = false;

            for( PCB_LAYER_ID layer : { PADSTACK::ALL_LAYERS, F_Cu, B_Cu } )
            {
                if( !pad->GetPrimitives( layer ).empty() )
                {
                    hasPrimitive = true;
                    break;
                }
            }

            BOOST_CHECK( hasPrimitive );

            // Pre-fix the outline was anchored at a polygon corner, ~1mm from the pad anchor
            std::shared_ptr<SHAPE_POLY_SET> poly =
                    pad->GetEffectivePolygon( PADSTACK::ALL_LAYERS, ERROR_INSIDE );
            BOOST_REQUIRE( poly && poly->OutlineCount() > 0 );

            VECTOR2I outlineCenter = poly->BBox().GetCenter();
            int      tol = pcbIUScale.mmToIU( 0.5 );

            BOOST_CHECK_LE( std::abs( outlineCenter.x - anchor.x ), tol );
            BOOST_CHECK_LE( std::abs( outlineCenter.y - anchor.y ), tol );
        }
    }
}


BOOST_AUTO_TEST_SUITE_END()
