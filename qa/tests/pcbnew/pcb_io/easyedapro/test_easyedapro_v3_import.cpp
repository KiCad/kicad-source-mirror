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

/**
 * @file test_easyedapro_v3_import.cpp
 * Test suite for import of EasyEDA Pro v3 PCB files
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>

#include <pcbnew/pcb_io/pcb_io.h>
#include <pcbnew/pcb_io/pcb_io_mgr.h>

#include <board.h>
#include <footprint.h>

#include <memory>


BOOST_AUTO_TEST_SUITE( EasyedaproV3Import )


static wxString getEasyEdaProV3SymbolLibPath()
{
    return wxString::FromUTF8( KI_TEST::GetTestDataRootDir() + "eeschema/plugins/easyedapro/LS2K0300_Symbol.elibz2" );
}


static wxString getEasyEdaProV3FootprintLibPath()
{
    return wxString::FromUTF8( KI_TEST::GetPcbnewTestDataDir()
                               + "plugins/easyedapro/LS2K0300_Footprint_2025-11-14.elibz2" );
}


BOOST_AUTO_TEST_CASE( FootprintLibraryCanReadOnlyFootprintElibz2 )
{
    IO_RELEASER<PCB_IO> plugin( PCB_IO_MGR::FindPlugin( PCB_IO_MGR::EASYEDAPRO_V3 ) );
    BOOST_REQUIRE( plugin );

    BOOST_CHECK( plugin->CanReadLibrary( getEasyEdaProV3FootprintLibPath() ) );
    BOOST_CHECK( !plugin->CanReadLibrary( getEasyEdaProV3SymbolLibPath() ) );
}


BOOST_AUTO_TEST_CASE( FootprintLibraryEnumeratesAndLoadsElibz2 )
{
    IO_RELEASER<PCB_IO> plugin( PCB_IO_MGR::FindPlugin( PCB_IO_MGR::EASYEDAPRO_V3 ) );
    BOOST_REQUIRE( plugin );

    wxArrayString footprintNames;
    BOOST_REQUIRE_NO_THROW( plugin->FootprintEnumerate( footprintNames, getEasyEdaProV3FootprintLibPath(), false ) );

    BOOST_REQUIRE_EQUAL( footprintNames.GetCount(), 1 );
    BOOST_CHECK_EQUAL( footprintNames[0], wxString( wxS( "BGA-286_17x17_12.0x12.0mm" ) ) );

    std::unique_ptr<FOOTPRINT> footprint(
            plugin->FootprintLoad( getEasyEdaProV3FootprintLibPath(), wxS( "BGA-286_17x17_12.0x12.0mm" ) ) );

    BOOST_REQUIRE( footprint );
    BOOST_CHECK_EQUAL( footprint->GetFPID().GetLibItemName(), UTF8( "BGA-286_17x17_12.0x12.0mm" ) );
    BOOST_CHECK_EQUAL( footprint->Pads().size(), 286 );
}


BOOST_AUTO_TEST_CASE( BoardLoadImportsInnerLayers )
{
    wxString dataPath = wxString::FromUTF8(
            KI_TEST::GetPcbnewTestDataDir()
            + "plugins/easyedapro/ProProject_LS2K0300Core_2025-11-14.epro2" );

    std::map<std::string, UTF8> properties;
    properties["pcb_id"] = "eb9fbfba682940f7a002816e66fbb3d7";

    IO_RELEASER<PCB_IO> plugin( PCB_IO_MGR::FindPlugin( PCB_IO_MGR::EASYEDAPRO_V3 ) );
    BOOST_REQUIRE( plugin );

    std::unique_ptr<BOARD> board( plugin->LoadBoard( dataPath, nullptr, &properties ) );
    BOOST_REQUIRE( board );

    BOOST_CHECK( board->Footprints().size() > 0 );
    BOOST_CHECK( board->GetNetCount() > 0 );

    // A 4-layer board whose inner and back copper carry their EasyEDA names.
    BOOST_CHECK_EQUAL( board->GetCopperLayerCount(), 4 );
    BOOST_CHECK_EQUAL( board->GetLayerName( In1_Cu ), wxString( wxS( "Inner1" ) ) );
    BOOST_CHECK_EQUAL( board->GetLayerName( In2_Cu ), wxString( wxS( "Inner2" ) ) );
    BOOST_CHECK_EQUAL( board->GetLayerName( B_Cu ), wxString( wxS( "Bottom Layer" ) ) );
}


BOOST_AUTO_TEST_SUITE_END()
