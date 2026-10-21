/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <filesystem>
#include <qa_utils/file_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>
#include <boost/test/unit_test.hpp>

#include <memory>

#include <base_units.h>
#include <footprint.h>
#include <pad.h>
#include <pcbnew_utils/board_file_utils.h>


/**
 * Sets up a footprint and a single pad, and provides a save/reload round trip.
 */
struct PAD_ROUNDTRIP_FIXTURE
{
    PAD_ROUNDTRIP_FIXTURE() :
            m_Footprint( nullptr ),
            m_Pad( new PAD( &m_Footprint ) )
    {
        m_Footprint.SetReference( "U1" );

        // Set some default pad properties.
        // Override them if necessary for specific test cases.
        m_Pad->SetPadstackMode( PADSTACK::MODE::NORMAL );
        m_Pad->SetNumber( "1" );
        m_Pad->SetAttribute( PAD_ATTRIB::SMD );
        m_Pad->SetLayerSet( LSET( { F_Cu } ) );
        m_Pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );

        const VECTOR2I padSize( pcbIUScale.mmToIU( 1.0 ), pcbIUScale.mmToIU( 2.0 ) );
        m_Pad->SetSize( PADSTACK::ALL_LAYERS, padSize );

        m_Footprint.Add( m_Pad );
    }

    /**
     * Save the fixture footprint to a temporary library and load it back.
     *
     * @return the reloaded footprint, or nullptr if the file could not be read.
     */
    std::unique_ptr<FOOTPRINT> RoundTrip() const
    {
        KI_TEST::SCOPED_TEMP_DIR tempDir( "kicad_qa_pad_roundtrip" );
        std::filesystem::path    savePath = tempDir.CreateChildDir( "pad.pretty" ) / "pad.kicad_mod";

        KI_TEST::DumpFootprintToFile( m_Footprint, savePath );

        return KI_TEST::ReadFootprintFromFileOrStream( savePath.string() );
    }

    FOOTPRINT m_Footprint;
    PAD*      m_Pad;
};


BOOST_AUTO_TEST_SUITE( PadRoundTrip )


/**
 * A copper-only pad must survive a footprint save/reload intact.
 */
BOOST_FIXTURE_TEST_CASE( CopperOnlyPad, PAD_ROUNDTRIP_FIXTURE )
{
    m_Pad->SetAttribute( PAD_ATTRIB::NON_COMPONENT );
    m_Pad->SetLayerSet( LSET( { In1_Cu } ) );

    // Inner-layer pads need a custom stackup to be properly represented.
    m_Footprint.SetStackupMode( FOOTPRINT_STACKUP::CUSTOM_LAYERS );
    m_Footprint.SetStackupLayers( LSET::AllCuMask( 4 ) );

    std::unique_ptr<FOOTPRINT> loaded = RoundTrip();

    BOOST_REQUIRE( loaded );
    BOOST_REQUIRE_EQUAL( loaded->Pads().size(), 1 );

    const PAD* loadedPad = loaded->Pads().front();

    BOOST_CHECK( loadedPad->GetAttribute() == PAD_ATTRIB::NON_COMPONENT );
    BOOST_CHECK( loadedPad->GetLayerSet() == LSET( { In1_Cu } ) );
    BOOST_CHECK_EQUAL( loadedPad->GetDrillSize().x, 0 );
    BOOST_CHECK_EQUAL( loadedPad->GetDrillSize().y, 0 );
}


BOOST_AUTO_TEST_SUITE_END()
