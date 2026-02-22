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

#include <sch_io/sch_io.h>
#include <sch_io/sch_io_mgr.h>


static wxString getEasyEdaProV3ArchivePath()
{
    return wxString::FromUTF8(
            KI_TEST::GetTestDataRootDir()
            + "pcbnew/plugins/easyedapro/ProProject_LS2K0300Core_2025-11-14.epro2" );
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
