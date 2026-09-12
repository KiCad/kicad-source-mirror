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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file test_issue25469_empty_sheet_pin_name.cpp
 *
 * Test for issue #25469: a sheet pin saved with an empty name made the whole schematic
 * unopenable.  Loading must keep the pin so the user can rename or delete it.
 */

#include <boost/test/unit_test.hpp>
#include <eeschema_test_utils.h>

#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <qa_utils/wx_utils/unit_test_utils.h>
#include <locale_io.h>
#include <wildcards_and_files_ext.h>

#include <wx/filename.h>


struct ISSUE25469_FIXTURE
{
    ISSUE25469_FIXTURE()
    {
        wxFileName projectPath( KI_TEST::GetEeschemaTestDataDir() );
        projectPath.AppendDir( "issue25469" );
        projectPath.SetName( "issue25469" );
        projectPath.SetExt( FILEEXT::ProjectFileExtension );

        m_settingsManager.LoadProject( projectPath.GetFullPath().ToStdString() );
        m_schematic = std::make_unique<SCHEMATIC>( nullptr );
        m_schematic->SetProject( &m_settingsManager.Prj() );
    }

    ~ISSUE25469_FIXTURE()
    {
        m_schematic.reset();
    }

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


BOOST_FIXTURE_TEST_SUITE( Issue25469EmptySheetPinName, ISSUE25469_FIXTURE )


BOOST_AUTO_TEST_CASE( LoadsSheetWithUnnamedPin )
{
    LOCALE_IO dummy;

    wxFileName dataDir( KI_TEST::GetEeschemaTestDataDir() );
    dataDir.AppendDir( "issue25469" );

    wxFileName rootFile( dataDir.GetPath(), "issue25469", FILEEXT::KiCadSchematicFileExtension );

    BOOST_REQUIRE_MESSAGE( rootFile.FileExists(),
                           "Test data missing: " + rootFile.GetFullPath().ToStdString() );

    SCH_IO_KICAD_SEXPR io;
    SCH_SHEET*         root = nullptr;

    BOOST_REQUIRE_NO_THROW( root = io.LoadSchematicFile( rootFile.GetFullPath(),
                                                         m_schematic.get() ) );
    BOOST_REQUIRE( root != nullptr );
    BOOST_REQUIRE( root->GetScreen() != nullptr );

    int pinCount = 0;
    int unnamedPinCount = 0;

    for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_SHEET_T ) )
    {
        for( SCH_SHEET_PIN* pin : static_cast<SCH_SHEET*>( item )->GetPins() )
        {
            pinCount++;

            if( pin->GetText().IsEmpty() )
                unnamedPinCount++;
        }
    }

    BOOST_CHECK_EQUAL( pinCount, 3 );
    BOOST_CHECK_EQUAL( unnamedPinCount, 1 );

    delete root;
}


BOOST_AUTO_TEST_SUITE_END()
