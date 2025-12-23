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
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file
 * Test suite for SCH_SHEET
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

// Code under test
#include <schematic.h>
#include <sch_sheet.h>
#include <sch_screen.h>
#include <sch_group.h>
#include <sch_symbol.h>
#include <sch_pin.h>
#include <lib_symbol.h>

#include <qa_utils/uuid_test_utils.h>
#include <qa_utils/wx_utils/wx_assert.h>

#include "eeschema_test_utils.h"

class TEST_SCH_GROUP_FIXTURE : public KI_TEST::SCHEMATIC_TEST_FIXTURE
{
public:
    TEST_SCH_GROUP_FIXTURE()
    {
        //m_schematic = SCHEMATIC( nullptr );
        //m_screen = SCH_SCREEN( &m_schematic );
    }

    ~TEST_SCH_GROUP_FIXTURE() {}

    void CreateTestSchematic()
    {
        m_schematic.reset();

        m_manager.LoadProject( "" );
        m_schematic = std::make_unique<SCHEMATIC>( &m_manager.Prj() );
        m_schematic->Reset();

        m_sheet = m_schematic->GetTopLevelSheet( 0 );
        m_screen = m_sheet->GetScreen();

        m_parent_part = new LIB_SYMBOL( "parent_part", nullptr );

        m_lib_pin = new SCH_PIN( m_parent_part );
        m_parent_part->AddDrawItem( m_lib_pin );

        // give the pin some kind of data we can use to test
        m_lib_pin->SetNumber( "42" );
        m_lib_pin->SetName( "pinname" );
        m_lib_pin->SetType( ELECTRICAL_PINTYPE::PT_INPUT );
        m_lib_pin->SetPosition( VECTOR2I( 1, 2 ) );

        SCH_SHEET_PATH path;
        m_parent_symbol = new SCH_SYMBOL( *m_parent_part, m_parent_part->GetLibId(), &path, 0, 0, VECTOR2I( 1, 2 ) );
        m_parent_symbol->SetRef( &path, "U2" );
        m_parent_symbol->UpdatePins();

        m_sch_pin = m_parent_symbol->GetPins( &path )[0];

        m_screen->Append( m_parent_symbol );
    }

    wxFileName SchematicQAPath( const wxString& aRelativePath ) override
    {
        wxFileName fn( KI_TEST::GetEeschemaTestDataDir() );

        wxString path = fn.GetFullPath();
        path += aRelativePath + wxT( "." ) + FILEEXT::KiCadSchematicFileExtension;

        return wxFileName( path );
    }

    SCH_SCREEN* m_screen;

    SCH_SHEET* m_sheet;

    LIB_SYMBOL* m_parent_part;
    SCH_PIN*    m_lib_pin;

    SCH_SYMBOL* m_parent_symbol;
    SCH_PIN*    m_sch_pin; // owned by m_parent_symbol, not us

    void CreateGroup()
    {
        SCH_GROUP* group = new SCH_GROUP( m_screen );
        group->AddItem( m_parent_symbol );

        m_screen->Append( group );
    }
};

/**
 * Declare the test suite
 */
BOOST_FIXTURE_TEST_SUITE( SchGroup, TEST_SCH_GROUP_FIXTURE )


/**
 * Check default properties
 */
BOOST_AUTO_TEST_CASE( Default )
{
    CreateTestSchematic();

    BOOST_CHECK_EQUAL( m_sheet->IsTopLevelSheet(), true );
    BOOST_CHECK_EQUAL( m_sheet->GetPosition(), VECTOR2I( 0, 0 ) );
    BOOST_CHECK_EQUAL( m_sheet->CountSheets(), 1 );
    BOOST_CHECK_EQUAL( m_sheet->SymbolCount(), 1 );

    BOOST_CHECK_EQUAL( m_sheet->GetScreenCount(), 1 );
}

/**
 * Check create group (and deletion clearing group parent properly)
 */
BOOST_AUTO_TEST_CASE( CreateGroup )
{
    CreateTestSchematic();

    SCH_GROUP* group = new SCH_GROUP( m_screen );
    group->AddItem( m_parent_symbol );

    m_screen->Append( group );

    EE_RTREE::EE_TYPE groups = m_screen->Items().OfType( SCH_GROUP_T );
    BOOST_CHECK_EQUAL( std::distance( groups.begin(), groups.end() ), 1 );
}

BOOST_AUTO_TEST_CASE( LoadSchGroups )
{
    LoadSchematic( SchematicQAPath( "groups_load_save" ) );

    EE_RTREE::EE_TYPE groups = m_schematic->RootScreen()->Items().OfType( SCH_GROUP_T );

    BOOST_CHECK_EQUAL( std::distance( groups.begin(), groups.end() ), 1 );

    SCH_GROUP* group = static_cast<SCH_GROUP*>( *groups.begin() );
    BOOST_CHECK_EQUAL( group->GetName(), "GroupName" );

    BOOST_CHECK_EQUAL( group->GetItems().size(), 2 );
}

BOOST_AUTO_TEST_SUITE_END()
