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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <footprint.h>
#include <layer_ids.h>
#include <layer_utils.h>
#include <lset.h>
#include <pad.h>
#include <pcb_shape.h>
#include <pcbnew_utils/board_file_utils.h>

/**
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/24045
 *
 * A footprint that contains items on the internal Rescue pseudo-layer must not cause
 * the Footprint Properties dialog validator to report "You are trying to remove layers..."
 * with an empty list. Rescue is an internal fallback layer with no UI control, so the
 * orphaned-layer check must treat it as non-blocking.
 */
BOOST_AUTO_TEST_SUITE( FootprintRescueLayer )


BOOST_AUTO_TEST_CASE( OrphanLayerCheckIgnoresRescue )
{
    // Default allowed custom user layer set used by the Footprint Properties dialog when
    // "Custom Layers" is off.
    const LSET defaultCustomLayers =
            LSET{ F_Cu, B_Cu } | LSET::InternalCuMask() | LSET::UserDefinedLayersMask( 4 );

    // A footprint with only "clean" items on copper/tech layers must report no orphans.
    {
        FOOTPRINT footprint( nullptr );

        PCB_SHAPE* shape = new PCB_SHAPE( &footprint );
        shape->SetLayer( F_SilkS );
        footprint.Add( shape );

        LSET orphans = LAYER_UTILS::GetOrphanedFootprintLayers( footprint, defaultCustomLayers );
        BOOST_TEST( orphans.none() );
    }

    // A footprint with an item on Rescue must NOT be reported as an orphan (it is handled
    // by library-parity DRC, not by the properties dialog).
    {
        FOOTPRINT footprint( nullptr );

        PCB_SHAPE* rescueShape = new PCB_SHAPE( &footprint );
        rescueShape->SetLayer( Rescue );
        footprint.Add( rescueShape );

        LSET allLayers = LAYER_UTILS::GetAllFootprintLayers( footprint );
        BOOST_TEST( allLayers.test( Rescue ) );

        LSET orphans = LAYER_UTILS::GetOrphanedFootprintLayers( footprint, defaultCustomLayers );
        BOOST_TEST( orphans.none() );
    }

    // An inner copper layer used by a footprint item is still correctly reported as an
    // orphan when the custom user layer set excludes it.
    {
        FOOTPRINT footprint( nullptr );

        PCB_SHAPE* copperShape = new PCB_SHAPE( &footprint );
        copperShape->SetLayer( In1_Cu );
        footprint.Add( copperShape );

        LSET restrictive = LSET{ F_Cu, B_Cu };
        LSET orphans = LAYER_UTILS::GetOrphanedFootprintLayers( footprint, restrictive );

        BOOST_TEST( orphans.test( In1_Cu ) );
    }

    // Multi-layer items (PTH pads span all copper layers plus their masks) must be
    // fully aggregated by GetAllFootprintLayers rather than collapsed to a single
    // layer.
    {
        FOOTPRINT footprint( nullptr );

        PAD* pthPad = new PAD( &footprint );
        pthPad->SetAttribute( PAD_ATTRIB::PTH );
        footprint.Add( pthPad );

        LSET padLayers = pthPad->GetLayerSet();
        LSET allLayers = LAYER_UTILS::GetAllFootprintLayers( footprint );

        BOOST_TEST( ( allLayers & padLayers ) == padLayers );
    }
}


BOOST_AUTO_TEST_CASE( Issue24045Footprint )
{
    // Load the exact footprint from the issue report. It contains graphic items on the
    // Rescue layer. Loading must succeed and the orphan-layer validator must pass.
    const std::string fpPath = KI_TEST::GetPcbnewTestDataDir() + "issue24045/"
                               + "QFN-24_L4.0-W4.0-P0.50-BL-EP2.6.kicad_mod";

    std::unique_ptr<FOOTPRINT> fp = KI_TEST::ReadFootprintFromFileOrStream( fpPath );
    BOOST_REQUIRE( fp );

    LSET allLayers = LAYER_UTILS::GetAllFootprintLayers( *fp );
    BOOST_TEST( allLayers.test( Rescue ),
                "Issue footprint must exercise the Rescue layer for this test to be meaningful" );

    const LSET defaultCustomLayers =
            LSET{ F_Cu, B_Cu } | LSET::InternalCuMask() | LSET::UserDefinedLayersMask( 4 );

    LSET orphans = LAYER_UTILS::GetOrphanedFootprintLayers( *fp, defaultCustomLayers );
    BOOST_TEST( orphans.none(),
                "Footprint properties dialog must not block saving when only Rescue items orphan" );
}


BOOST_AUTO_TEST_SUITE_END()
