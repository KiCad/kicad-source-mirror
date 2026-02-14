/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 * or you may search the http://www.gnu.org website for the version 32 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * Test for issue #23058: Jumper pin groups with out-of-bounds pin numbers cause crash.
 *
 * When a jumper pin group references a pin number that doesn't exist in the symbol,
 * the connection graph would dereference a null pointer from GetPin() and crash.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <connection_graph.h>
#include <schematic.h>
#include <sch_sheet.h>
#include <sch_screen.h>
#include <settings/settings_manager.h>
#include <lib_symbol.h>
#include <sch_pin.h>
#include <sch_symbol.h>
#include <locale_io.h>

struct JUMPER_PIN_GROUP_FIXTURE
{
    JUMPER_PIN_GROUP_FIXTURE()
    {
        m_settingsManager.LoadProject( "" );
        m_schematic = std::make_unique<SCHEMATIC>( &m_settingsManager.Prj() );
        m_schematic->Reset();
        SCH_SHEET* defaultSheet = m_schematic->GetTopLevelSheet( 0 );

        SCH_SHEET* root = new SCH_SHEET( m_schematic.get() );
        SCH_SCREEN* screen = new SCH_SCREEN( m_schematic.get() );
        root->SetScreen( screen );

        m_schematic->AddTopLevelSheet( root );
        m_schematic->RemoveTopLevelSheet( defaultSheet );
        delete defaultSheet;
    }

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


BOOST_FIXTURE_TEST_CASE( JumperPinGroupOutOfBounds, JUMPER_PIN_GROUP_FIXTURE )
{
    SCH_SCREEN* screen = m_schematic->RootScreen();
    SCH_SHEET_PATH path;
    path.push_back( &m_schematic->Root() );

    LIB_SYMBOL* libSym = new LIB_SYMBOL( "SW_SPST", nullptr );

    SCH_PIN* pin1 = new SCH_PIN( libSym );
    pin1->SetNumber( "1" );
    pin1->SetType( ELECTRICAL_PINTYPE::PT_PASSIVE );
    pin1->SetPosition( VECTOR2I( -508000, 0 ) );
    libSym->AddDrawItem( pin1 );

    SCH_PIN* pin2 = new SCH_PIN( libSym );
    pin2->SetNumber( "2" );
    pin2->SetType( ELECTRICAL_PINTYPE::PT_PASSIVE );
    pin2->SetPosition( VECTOR2I( 508000, 0 ) );
    libSym->AddDrawItem( pin2 );

    // Add a jumper pin group that references non-existent pin "3"
    std::set<wxString> group;
    group.insert( wxT( "1" ) );
    group.insert( wxT( "3" ) );
    libSym->JumperPinGroups().push_back( group );

    SCH_SYMBOL* sym = new SCH_SYMBOL( *libSym, libSym->GetLibId(), &path, 0, 0,
                                       VECTOR2I( 15621000, 6223000 ) );
    sym->UpdatePins();
    screen->Append( sym );

    // This must not crash even though pin "3" doesn't exist
    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();
    BOOST_CHECK_NO_THROW( m_schematic->ConnectionGraph()->Recalculate( sheets, true ) );

    delete libSym;
}


BOOST_FIXTURE_TEST_CASE( JumperPinGroupAllInvalid, JUMPER_PIN_GROUP_FIXTURE )
{
    SCH_SCREEN* screen = m_schematic->RootScreen();
    SCH_SHEET_PATH path;
    path.push_back( &m_schematic->Root() );

    LIB_SYMBOL* libSym = new LIB_SYMBOL( "SW_SPST", nullptr );

    SCH_PIN* pin1 = new SCH_PIN( libSym );
    pin1->SetNumber( "1" );
    pin1->SetType( ELECTRICAL_PINTYPE::PT_PASSIVE );
    pin1->SetPosition( VECTOR2I( -508000, 0 ) );
    libSym->AddDrawItem( pin1 );

    // Add a jumper pin group where ALL pin numbers are invalid
    std::set<wxString> group;
    group.insert( wxT( "5" ) );
    group.insert( wxT( "6" ) );
    libSym->JumperPinGroups().push_back( group );

    SCH_SYMBOL* sym = new SCH_SYMBOL( *libSym, libSym->GetLibId(), &path, 0, 0,
                                       VECTOR2I( 15621000, 6223000 ) );
    sym->UpdatePins();
    screen->Append( sym );

    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();
    BOOST_CHECK_NO_THROW( m_schematic->ConnectionGraph()->Recalculate( sheets, true ) );

    delete libSym;
}


BOOST_FIXTURE_TEST_CASE( JumperPinGroupValidPins, JUMPER_PIN_GROUP_FIXTURE )
{
    SCH_SCREEN* screen = m_schematic->RootScreen();
    SCH_SHEET_PATH path;
    path.push_back( &m_schematic->Root() );

    LIB_SYMBOL* libSym = new LIB_SYMBOL( "SW_SPDT", nullptr );

    SCH_PIN* pin1 = new SCH_PIN( libSym );
    pin1->SetNumber( "1" );
    pin1->SetType( ELECTRICAL_PINTYPE::PT_PASSIVE );
    pin1->SetPosition( VECTOR2I( -508000, 0 ) );
    libSym->AddDrawItem( pin1 );

    SCH_PIN* pin2 = new SCH_PIN( libSym );
    pin2->SetNumber( "2" );
    pin2->SetType( ELECTRICAL_PINTYPE::PT_PASSIVE );
    pin2->SetPosition( VECTOR2I( 508000, 0 ) );
    libSym->AddDrawItem( pin2 );

    SCH_PIN* pin3 = new SCH_PIN( libSym );
    pin3->SetNumber( "3" );
    pin3->SetType( ELECTRICAL_PINTYPE::PT_PASSIVE );
    pin3->SetPosition( VECTOR2I( 508000, 254000 ) );
    libSym->AddDrawItem( pin3 );

    // Add a valid jumper pin group
    std::set<wxString> group;
    group.insert( wxT( "1" ) );
    group.insert( wxT( "3" ) );
    libSym->JumperPinGroups().push_back( group );

    SCH_SYMBOL* sym = new SCH_SYMBOL( *libSym, libSym->GetLibId(), &path, 0, 0,
                                       VECTOR2I( 15621000, 6223000 ) );
    sym->UpdatePins();
    screen->Append( sym );

    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();
    m_schematic->ConnectionGraph()->Recalculate( sheets, true );

    path = sheets[0];

    // Verify that pins 1 and 3 are connected via the jumper group
    SCH_PIN* schPin1 = sym->GetPin( wxT( "1" ) );
    SCH_PIN* schPin3 = sym->GetPin( wxT( "3" ) );

    BOOST_REQUIRE( schPin1 );
    BOOST_REQUIRE( schPin3 );

    bool connected = false;

    for( SCH_ITEM* item : schPin1->ConnectedItems( path ) )
    {
        if( item == schPin3 )
        {
            connected = true;
            break;
        }
    }

    BOOST_CHECK_MESSAGE( connected, "Pins 1 and 3 should be connected via jumper pin group" );

    delete libSym;
}


BOOST_FIXTURE_TEST_CASE( Issue23058FileLoad, JUMPER_PIN_GROUP_FIXTURE )
{
    LOCALE_IO dummy;

    KI_TEST::LoadSchematic( m_settingsManager, "issue23058/issue23058", m_schematic );

    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();

    // This must not crash. The schematic has a jumper pin group referencing
    // pin "3" but the symbol only has pins "1" and "2".
    BOOST_CHECK_NO_THROW( m_schematic->ConnectionGraph()->Recalculate( sheets, true ) );
}
