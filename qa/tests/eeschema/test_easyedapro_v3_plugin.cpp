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

#include <lib_symbol.h>
#include <sch_io/sch_io.h>
#include <sch_io/sch_io_mgr.h>
#include <sch_pin.h>

#include <memory>


static wxString getEasyEdaProV3ArchivePath()
{
    return wxString::FromUTF8(
            KI_TEST::GetTestDataRootDir()
            + "pcbnew/plugins/easyedapro/ProProject_LS2K0300Core_2025-11-14.epro2" );
}


static wxString getEasyEdaProV3SymbolLibPath()
{
    return wxString::FromUTF8( KI_TEST::GetTestDataRootDir() + "eeschema/plugins/easyedapro/LS2K0300_Symbol.elibz2" );
}


static wxString getEasyEdaProV3FootprintLibPath()
{
    return wxString::FromUTF8( KI_TEST::GetTestDataRootDir()
                               + "pcbnew/plugins/easyedapro/LS2K0300_Footprint_2025-11-14.elibz2" );
}


BOOST_AUTO_TEST_CASE( EasyEdaProV3FindPlugin )
{
    IO_RELEASER<SCH_IO> plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_EASYEDAPRO_V3 ) );
    BOOST_CHECK_NE( plugin.get(), nullptr );
}


BOOST_AUTO_TEST_CASE( EasyEdaProV3CanReadSchematicFile )
{
    IO_RELEASER<SCH_IO> plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_EASYEDAPRO_V3 ) );
    BOOST_REQUIRE( plugin );

    BOOST_CHECK( plugin->CanReadSchematicFile( getEasyEdaProV3ArchivePath() ) );
}


BOOST_AUTO_TEST_CASE( EasyEdaProV3GuessPluginType )
{
    BOOST_CHECK_EQUAL(
            SCH_IO_MGR::GuessPluginTypeFromSchPath( getEasyEdaProV3ArchivePath() ),
            SCH_IO_MGR::SCH_EASYEDAPRO_V3 );
}


BOOST_AUTO_TEST_CASE( EasyEdaProV3CanReadSymbolLibrary )
{
    IO_RELEASER<SCH_IO> plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_EASYEDAPRO_V3 ) );
    BOOST_REQUIRE( plugin );

    BOOST_CHECK( plugin->CanReadLibrary( getEasyEdaProV3SymbolLibPath() ) );
    BOOST_CHECK( !plugin->CanReadLibrary( getEasyEdaProV3FootprintLibPath() ) );
}


BOOST_AUTO_TEST_CASE( EasyEdaProV3EnumeratesAndLoadsSymbolLibrary )
{
    IO_RELEASER<SCH_IO> plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_EASYEDAPRO_V3 ) );
    BOOST_REQUIRE( plugin );

    wxArrayString symbolNames;
    BOOST_REQUIRE_NO_THROW( plugin->EnumerateSymbolLib( symbolNames, getEasyEdaProV3SymbolLibPath() ) );

    BOOST_REQUIRE_EQUAL( symbolNames.GetCount(), 1 );
    BOOST_CHECK_EQUAL( symbolNames[0], wxString( wxS( "LS2K0300" ) ) );

    std::unique_ptr<LIB_SYMBOL> symbol( plugin->LoadSymbol( getEasyEdaProV3SymbolLibPath(), wxS( "LS2K0300" ) ) );

    BOOST_REQUIRE( symbol );
    BOOST_CHECK_EQUAL( symbol->GetName(), wxString( wxS( "LS2K0300" ) ) );
    BOOST_CHECK_EQUAL( symbol->GetUnitCount(), 5 );
    BOOST_CHECK( symbol->GetPins().size() > 200 );
}
