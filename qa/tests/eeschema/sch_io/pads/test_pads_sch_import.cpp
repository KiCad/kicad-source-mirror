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

#include <boost/test/unit_test.hpp>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <schematic.h>
#include <sch_io/pads/sch_io_pads.h>
#include <sch_io/sch_io_mgr.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <settings/settings_manager.h>


namespace
{

struct PADS_SCH_IMPORT_FIXTURE
{
    PADS_SCH_IMPORT_FIXTURE() : m_schematic( nullptr )
    {
        m_settingsManager.LoadProject( "" );
        m_schematic.SetProject( &m_settingsManager.Prj() );
        m_schematic.Reset();
    }

    ~PADS_SCH_IMPORT_FIXTURE()
    {
        m_schematic.Reset();
    }

    SETTINGS_MANAGER m_settingsManager;
    SCHEMATIC        m_schematic;
};

} // namespace


BOOST_FIXTURE_TEST_SUITE( PadsSchImport, PADS_SCH_IMPORT_FIXTURE )


BOOST_AUTO_TEST_CASE( CanReadSchematicFile )
{
    SCH_IO_PADS plugin;

    wxString padsFile = wxString::FromUTF8(
            KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/simple_schematic.txt" );

    BOOST_CHECK( plugin.CanReadSchematicFile( padsFile ) );
}


BOOST_AUTO_TEST_CASE( CanReadSchematicFile_RejectNonPads )
{
    SCH_IO_PADS plugin;

    wxString kicadFile = wxString::FromUTF8(
            KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/simple_schematic.txt" );

    BOOST_CHECK( plugin.CanReadSchematicFile( kicadFile ) );
}


BOOST_AUTO_TEST_CASE( FindPlugin )
{
    IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_PADS ) );
    BOOST_CHECK_NE( pi.get(), nullptr );
}


BOOST_AUTO_TEST_SUITE_END()
