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

#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <padstack.h>
#include <pcbnew_utils/board_file_utils.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>

#include <filesystem>
#include <fstream>
#include <sstream>


BOOST_AUTO_TEST_SUITE( PadTenting )


static std::string SaveBoardToString( BOARD& aBoard )
{
    std::filesystem::path tempPath = std::filesystem::temp_directory_path() / "pad_tenting_test.kicad_pcb";
    std::stringstream     buf;

    {
        KI_TEST::DumpBoardToFile( aBoard, tempPath.string() );

        std::ifstream ifs( tempPath );
        buf << ifs.rdbuf();
    }

    std::filesystem::remove( tempPath );

    return buf.str();
}


/**
 * Pads with no explicit tenting set should not have a (tenting ...) block in the output.
 */
BOOST_AUTO_TEST_CASE( DefaultTentingNotSerialized )
{
    BOARD board;
    board.SetBoardUse( BOARD_USE::FPHOLDER );

    auto fp = std::make_unique<FOOTPRINT>( &board );
    fp->SetPosition( VECTOR2I( 0, 0 ) );

    auto* pad = new PAD( fp.get() );
    pad->SetAttribute( PAD_ATTRIB::PTH );
    pad->SetLayerSet( PAD::PTHMask() );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
    pad->SetSize( PADSTACK::ALL_LAYERS,
                  VECTOR2I( pcbIUScale.mmToIU( 1.5 ), pcbIUScale.mmToIU( 1.5 ) ) );
    pad->SetDrillSize( VECTOR2I( pcbIUScale.mmToIU( 0.8 ), pcbIUScale.mmToIU( 0.8 ) ) );
    pad->SetNumber( wxT( "1" ) );
    pad->SetPosition( VECTOR2I( 0, 0 ) );

    BOOST_CHECK( !pad->Padstack().FrontOuterLayers().has_solder_mask.has_value() );
    BOOST_CHECK( !pad->Padstack().BackOuterLayers().has_solder_mask.has_value() );

    fp->Add( pad );
    board.Add( fp.release() );

    std::string output = SaveBoardToString( board );

    BOOST_CHECK_MESSAGE( output.find( "(tenting" ) == std::string::npos
                                 || output.find( "(tenting" ) < output.find( "(footprint" ),
                         "PTH pad with default tenting should not have a (tenting) block. "
                         "Board-level tenting in setup section is expected." );
}


/**
 * Pads with explicit tenting should have the (tenting ...) block in the output.
 */
BOOST_AUTO_TEST_CASE( ExplicitTentingSerialized )
{
    BOARD board;
    board.SetBoardUse( BOARD_USE::FPHOLDER );

    auto fp = std::make_unique<FOOTPRINT>( &board );
    fp->SetPosition( VECTOR2I( 0, 0 ) );

    auto* pad = new PAD( fp.get() );
    pad->SetAttribute( PAD_ATTRIB::PTH );
    pad->SetLayerSet( PAD::PTHMask() );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
    pad->SetSize( PADSTACK::ALL_LAYERS,
                  VECTOR2I( pcbIUScale.mmToIU( 1.5 ), pcbIUScale.mmToIU( 1.5 ) ) );
    pad->SetDrillSize( VECTOR2I( pcbIUScale.mmToIU( 0.8 ), pcbIUScale.mmToIU( 0.8 ) ) );
    pad->SetNumber( wxT( "1" ) );
    pad->SetPosition( VECTOR2I( 0, 0 ) );

    pad->Padstack().FrontOuterLayers().has_solder_mask = true;

    BOOST_CHECK( pad->Padstack().FrontOuterLayers().has_solder_mask.has_value() );

    fp->Add( pad );
    board.Add( fp.release() );

    std::string output = SaveBoardToString( board );

    size_t fpStart = output.find( "(footprint" );
    BOOST_REQUIRE( fpStart != std::string::npos );

    size_t tentingPos = output.find( "(tenting", fpStart );
    BOOST_CHECK_MESSAGE( tentingPos != std::string::npos,
                         "PTH pad with explicit tenting should have a (tenting) block" );
}


/**
 * NPTH pads with no explicit tenting should not have the (tenting ...) block.
 */
BOOST_AUTO_TEST_CASE( NpthDefaultTentingNotSerialized )
{
    BOARD board;
    board.SetBoardUse( BOARD_USE::FPHOLDER );

    auto fp = std::make_unique<FOOTPRINT>( &board );
    fp->SetPosition( VECTOR2I( 0, 0 ) );

    auto* pad = new PAD( fp.get() );
    pad->SetAttribute( PAD_ATTRIB::NPTH );
    pad->SetLayerSet( PAD::UnplatedHoleMask() );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
    pad->SetSize( PADSTACK::ALL_LAYERS,
                  VECTOR2I( pcbIUScale.mmToIU( 1.0 ), pcbIUScale.mmToIU( 1.0 ) ) );
    pad->SetDrillSize( VECTOR2I( pcbIUScale.mmToIU( 1.0 ), pcbIUScale.mmToIU( 1.0 ) ) );
    pad->SetNumber( wxT( "" ) );
    pad->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 5 ), 0 ) );

    BOOST_CHECK( !pad->Padstack().FrontOuterLayers().has_solder_mask.has_value() );
    BOOST_CHECK( !pad->Padstack().BackOuterLayers().has_solder_mask.has_value() );

    fp->Add( pad );
    board.Add( fp.release() );

    std::string output = SaveBoardToString( board );

    size_t fpStart = output.find( "(footprint" );
    BOOST_REQUIRE( fpStart != std::string::npos );

    size_t tentingPos = output.find( "(tenting", fpStart );
    BOOST_CHECK_MESSAGE( tentingPos == std::string::npos,
                         "NPTH pad with default tenting should not have a (tenting) block" );
}


/**
 * Vias with no explicit tenting should not have the (tenting ...) block.
 */
BOOST_AUTO_TEST_CASE( ViaDefaultTentingNotSerialized )
{
    BOARD board;
    board.SetBoardUse( BOARD_USE::FPHOLDER );

    auto* via = new PCB_VIA( &board );
    via->SetPosition( VECTOR2I( 0, 0 ) );
    via->SetWidth( PADSTACK::ALL_LAYERS, pcbIUScale.mmToIU( 0.8 ) );
    via->SetDrill( pcbIUScale.mmToIU( 0.4 ) );
    via->SetViaType( VIATYPE::THROUGH );

    BOOST_CHECK( !via->Padstack().FrontOuterLayers().has_solder_mask.has_value() );
    BOOST_CHECK( !via->Padstack().BackOuterLayers().has_solder_mask.has_value() );

    board.Add( via );

    std::string output = SaveBoardToString( board );

    size_t viaStart = output.find( "(via" );
    BOOST_REQUIRE( viaStart != std::string::npos );

    size_t tentingPos = output.find( "(tenting", viaStart );
    BOOST_CHECK_MESSAGE( tentingPos == std::string::npos,
                         "Via with default tenting should not have a (tenting) block" );
}


/**
 * Vias with explicit tenting should have the (tenting ...) block.
 */
BOOST_AUTO_TEST_CASE( ViaExplicitTentingSerialized )
{
    BOARD board;
    board.SetBoardUse( BOARD_USE::FPHOLDER );

    auto* via = new PCB_VIA( &board );
    via->SetPosition( VECTOR2I( 0, 0 ) );
    via->SetWidth( PADSTACK::ALL_LAYERS, pcbIUScale.mmToIU( 0.8 ) );
    via->SetDrill( pcbIUScale.mmToIU( 0.4 ) );
    via->SetViaType( VIATYPE::THROUGH );

    via->Padstack().FrontOuterLayers().has_solder_mask = false;
    via->Padstack().BackOuterLayers().has_solder_mask = false;

    board.Add( via );

    std::string output = SaveBoardToString( board );

    size_t viaStart = output.find( "(via" );
    BOOST_REQUIRE( viaStart != std::string::npos );

    size_t tentingPos = output.find( "(tenting", viaStart );
    BOOST_CHECK_MESSAGE( tentingPos != std::string::npos,
                         "Via with explicit tenting should have a (tenting) block" );
}


BOOST_AUTO_TEST_SUITE_END()
