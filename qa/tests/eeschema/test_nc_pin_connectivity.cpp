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
#include <erc/erc_settings.h>
#include <erc/erc.h>
#include <erc/erc_report.h>

struct NC_PIN_CONNECTIVITY_FIXTURE
{
    NC_PIN_CONNECTIVITY_FIXTURE()
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

BOOST_FIXTURE_TEST_CASE( StackedNCPins, NC_PIN_CONNECTIVITY_FIXTURE )
{
    SCH_SCREEN* screen = m_schematic->RootScreen();
    SCH_SHEET_PATH path;
    path.push_back( &m_schematic->Root() );

    // Create first symbol with NC pin
    LIB_SYMBOL* libSym1 = new LIB_SYMBOL( "sym1", nullptr );
    SCH_PIN* libPin1 = new SCH_PIN( libSym1 );
    libPin1->SetNumber( "1" );
    libPin1->SetType( ELECTRICAL_PINTYPE::PT_NC );
    libPin1->SetPosition( VECTOR2I( 0, 0 ) );
    libSym1->AddDrawItem( libPin1 );

    SCH_SYMBOL* sym1 = new SCH_SYMBOL( *libSym1, libSym1->GetLibId(), &path, 0, 0, VECTOR2I( 100, 100 ) );
    sym1->UpdatePins();
    screen->Append( sym1 );

    // Create second symbol with NC pin at same position
    LIB_SYMBOL* libSym2 = new LIB_SYMBOL( "sym2", nullptr );
    SCH_PIN* libPin2 = new SCH_PIN( libSym2 );
    libPin2->SetNumber( "1" );
    libPin2->SetType( ELECTRICAL_PINTYPE::PT_NC );
    libPin2->SetPosition( VECTOR2I( 0, 0 ) );
    libSym2->AddDrawItem( libPin2 );

    SCH_SYMBOL* sym2 = new SCH_SYMBOL( *libSym2, libSym2->GetLibId(), &path, 0, 0, VECTOR2I( 100, 100 ) );
    sym2->UpdatePins();
    screen->Append( sym2 );

    // Recalculate connectivity
    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();
    m_schematic->ConnectionGraph()->Recalculate( sheets, true );

    path = sheets[0];

    // Check connectivity
    SCH_PIN* pin1 = sym1->GetPins( &path )[0];
    SCH_PIN* pin2 = sym2->GetPins( &path )[0];

    // They should NOT be connected
    bool connected = false;
    for( SCH_ITEM* item : pin1->ConnectedItems( path ) )
    {
        if( item == pin2 )
        {
            connected = true;
            break;
        }
    }

    BOOST_CHECK_MESSAGE( !connected, "Stacked NC pins should not be connected" );

    // Check subgraph codes
    SCH_CONNECTION* conn1 = pin1->Connection( &path );
    SCH_CONNECTION* conn2 = pin2->Connection( &path );

    BOOST_CHECK( conn1 );
    BOOST_CHECK( conn2 );

    if( conn1 && conn2 )
    {
        BOOST_CHECK_MESSAGE( conn1->SubgraphCode() != conn2->SubgraphCode(), "Stacked NC pins should be in different subgraphs" );
    }

    delete libSym1;
    delete libSym2;
}

BOOST_FIXTURE_TEST_CASE( NCPinStackedWithNormalPin, NC_PIN_CONNECTIVITY_FIXTURE )
{
    SCH_SCREEN* screen = m_schematic->RootScreen();
    SCH_SHEET_PATH path;
    path.push_back( &m_schematic->Root() );

    // Create NC pin
    LIB_SYMBOL* libSym1 = new LIB_SYMBOL( "sym1", nullptr );
    SCH_PIN* libPin1 = new SCH_PIN( libSym1 );
    libPin1->SetNumber( "1" );
    libPin1->SetType( ELECTRICAL_PINTYPE::PT_NC );
    libPin1->SetPosition( VECTOR2I( 0, 0 ) );
    libSym1->AddDrawItem( libPin1 );

    SCH_SYMBOL* sym1 = new SCH_SYMBOL( *libSym1, libSym1->GetLibId(), &path, 0, 0, VECTOR2I( 100, 100 ) );
    sym1->UpdatePins();
    screen->Append( sym1 );

    // Create normal pin at same position
    LIB_SYMBOL* libSym2 = new LIB_SYMBOL( "sym2", nullptr );
    SCH_PIN* libPin2 = new SCH_PIN( libSym2 );
    libPin2->SetNumber( "1" );
    libPin2->SetType( ELECTRICAL_PINTYPE::PT_PASSIVE );
    libPin2->SetPosition( VECTOR2I( 0, 0 ) );
    libSym2->AddDrawItem( libPin2 );

    SCH_SYMBOL* sym2 = new SCH_SYMBOL( *libSym2, libSym2->GetLibId(), &path, 0, 0, VECTOR2I( 100, 100 ) );
    sym2->UpdatePins();
    screen->Append( sym2 );

    // Recalculate connectivity
    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();
    m_schematic->ConnectionGraph()->Recalculate( sheets, true );

    path = sheets[0];

    // Check connectivity
    SCH_PIN* pin1 = sym1->GetPins( &path )[0];
    SCH_PIN* pin2 = sym2->GetPins( &path )[0];

    SCH_CONNECTION* conn1 = pin1->Connection( &path );
    SCH_CONNECTION* conn2 = pin2->Connection( &path );

    BOOST_CHECK( conn1 );
    BOOST_CHECK( conn2 );

    if( conn1 && conn2 )
    {
        BOOST_CHECK_MESSAGE( conn1->SubgraphCode() != conn2->SubgraphCode(), "NC pin should not connect to normal pin" );
    }

    delete libSym1;
    delete libSym2;
}

BOOST_FIXTURE_TEST_CASE( StackedNCPinsOnSameSymbol, NC_PIN_CONNECTIVITY_FIXTURE )
{
    SCH_SCREEN* screen = m_schematic->RootScreen();
    SCH_SHEET_PATH path;
    path.push_back( &m_schematic->Root() );

    // Create symbol with two stacked NC pins
    LIB_SYMBOL* libSym1 = new LIB_SYMBOL( "sym1", nullptr );

    SCH_PIN* libPin1 = new SCH_PIN( libSym1 );
    libPin1->SetNumber( "1" );
    libPin1->SetType( ELECTRICAL_PINTYPE::PT_NC );
    libPin1->SetPosition( VECTOR2I( 0, 0 ) );
    libSym1->AddDrawItem( libPin1 );

    SCH_PIN* libPin2 = new SCH_PIN( libSym1 );
    libPin2->SetNumber( "2" ); // Different number
    libPin2->SetType( ELECTRICAL_PINTYPE::PT_NC );
    libPin2->SetPosition( VECTOR2I( 0, 0 ) );
    libSym1->AddDrawItem( libPin2 );

    SCH_SYMBOL* sym1 = new SCH_SYMBOL( *libSym1, libSym1->GetLibId(), &path, 0, 0, VECTOR2I( 100, 100 ) );
    sym1->UpdatePins();
    screen->Append( sym1 );

    // Recalculate connectivity
    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();
    m_schematic->ConnectionGraph()->Recalculate( sheets, true );

    path = sheets[0];

    // Check connectivity
    std::vector<SCH_PIN*> pins = sym1->GetPins( &path );
    SCH_PIN* pin1 = pins[0];
    SCH_PIN* pin2 = pins[1];

    SCH_CONNECTION* conn1 = pin1->Connection( &path );
    SCH_CONNECTION* conn2 = pin2->Connection( &path );

    BOOST_CHECK( conn1 );
    BOOST_CHECK( conn2 );

    if( conn1 && conn2 )
    {
        BOOST_CHECK_MESSAGE( conn1->SubgraphCode() != conn2->SubgraphCode(), "Stacked NC pins on same symbol should not be connected" );
    }

    delete libSym1;
}

BOOST_FIXTURE_TEST_CASE( StackedNCPinsERC, NC_PIN_CONNECTIVITY_FIXTURE )
{
    SCH_SCREEN* screen = m_schematic->RootScreen();
    SCH_SHEET_PATH path;
    path.push_back( &m_schematic->Root() );

    // Create first symbol with NC pin
    LIB_SYMBOL* libSym1 = new LIB_SYMBOL( "sym1", nullptr );
    SCH_PIN* libPin1 = new SCH_PIN( libSym1 );
    libPin1->SetNumber( "1" );
    libPin1->SetType( ELECTRICAL_PINTYPE::PT_NC );
    libPin1->SetPosition( VECTOR2I( 0, 0 ) );
    libSym1->AddDrawItem( libPin1 );

    SCH_SYMBOL* sym1 = new SCH_SYMBOL( *libSym1, libSym1->GetLibId(), &path, 0, 0, VECTOR2I( 100, 100 ) );
    sym1->UpdatePins();
    screen->Append( sym1 );

    // Create second symbol with NC pin at same position
    LIB_SYMBOL* libSym2 = new LIB_SYMBOL( "sym2", nullptr );
    SCH_PIN* libPin2 = new SCH_PIN( libSym2 );
    libPin2->SetNumber( "1" );
    libPin2->SetType( ELECTRICAL_PINTYPE::PT_NC );
    libPin2->SetPosition( VECTOR2I( 0, 0 ) );
    libSym2->AddDrawItem( libPin2 );

    SCH_SYMBOL* sym2 = new SCH_SYMBOL( *libSym2, libSym2->GetLibId(), &path, 0, 0, VECTOR2I( 100, 100 ) );
    sym2->UpdatePins();
    screen->Append( sym2 );

    // Run ERC
    ERC_SETTINGS& settings = m_schematic->ErcSettings();
    SHEETLIST_ERC_ITEMS_PROVIDER errors( m_schematic.get() );

    // Enable all errors
    settings.m_ERCSeverities[ERCE_PIN_TO_PIN_WARNING] = RPT_SEVERITY_ERROR;
    settings.m_ERCSeverities[ERCE_PIN_TO_PIN_ERROR] = RPT_SEVERITY_ERROR;
    settings.m_ERCSeverities[ERCE_NOCONNECT_CONNECTED] = RPT_SEVERITY_ERROR;

    m_schematic->ConnectionGraph()->Recalculate( m_schematic->BuildSheetListSortedByPageNumbers(), true );
    m_schematic->ConnectionGraph()->RunERC();

    ERC_TESTER tester( m_schematic.get() );
    tester.TestPinToPin();
    tester.TestNoConnectPins();

    // We expect NO errors for stacked NC pins on same symbol
    BOOST_CHECK_EQUAL( errors.GetCount(), 0 );

    delete libSym1;
}

BOOST_FIXTURE_TEST_CASE( StackedNCPinsOnSameSymbolERC, NC_PIN_CONNECTIVITY_FIXTURE )
{
    SCH_SCREEN* screen = m_schematic->RootScreen();
    SCH_SHEET_PATH path;
    path.push_back( &m_schematic->Root() );

    // Create symbol with two stacked NC pins
    LIB_SYMBOL* libSym1 = new LIB_SYMBOL( "sym1", nullptr );

    SCH_PIN* libPin1 = new SCH_PIN( libSym1 );
    libPin1->SetNumber( "1" );
    libPin1->SetType( ELECTRICAL_PINTYPE::PT_NC );
    libPin1->SetPosition( VECTOR2I( 0, 0 ) );
    libSym1->AddDrawItem( libPin1 );

    SCH_PIN* libPin2 = new SCH_PIN( libSym1 );
    libPin2->SetNumber( "2" ); // Different number
    libPin2->SetType( ELECTRICAL_PINTYPE::PT_NC );
    libPin2->SetPosition( VECTOR2I( 0, 0 ) );
    libSym1->AddDrawItem( libPin2 );

    SCH_SYMBOL* sym1 = new SCH_SYMBOL( *libSym1, libSym1->GetLibId(), &path, 0, 0, VECTOR2I( 100, 100 ) );
    sym1->UpdatePins();
    screen->Append( sym1 );

    // Run ERC
    ERC_SETTINGS& settings = m_schematic->ErcSettings();
    SHEETLIST_ERC_ITEMS_PROVIDER errors( m_schematic.get() );

    // Enable all errors
    settings.m_ERCSeverities[ERCE_PIN_TO_PIN_WARNING] = RPT_SEVERITY_ERROR;
    settings.m_ERCSeverities[ERCE_PIN_TO_PIN_ERROR] = RPT_SEVERITY_ERROR;
    settings.m_ERCSeverities[ERCE_NOCONNECT_CONNECTED] = RPT_SEVERITY_ERROR;

    m_schematic->ConnectionGraph()->Recalculate( m_schematic->BuildSheetListSortedByPageNumbers(), true );
    m_schematic->ConnectionGraph()->RunERC();

    ERC_TESTER tester( m_schematic.get() );
    tester.TestPinToPin();
    int ncErrors = tester.TestNoConnectPins();

    // We expect NO errors for stacked NC pins on same symbol
    BOOST_CHECK_EQUAL( ncErrors, 0 );
    BOOST_CHECK_EQUAL( errors.GetCount(), 0 );

    delete libSym1;
}

BOOST_FIXTURE_TEST_CASE( NCPinStackedWithNormalPinERC, NC_PIN_CONNECTIVITY_FIXTURE )
{
    SCH_SCREEN* screen = m_schematic->RootScreen();
    SCH_SHEET_PATH path;
    path.push_back( &m_schematic->Root() );

    // Create NC pin
    LIB_SYMBOL* libSym1 = new LIB_SYMBOL( "sym1", nullptr );
    SCH_PIN* libPin1 = new SCH_PIN( libSym1 );
    libPin1->SetNumber( "1" );
    libPin1->SetType( ELECTRICAL_PINTYPE::PT_NC );
    libPin1->SetPosition( VECTOR2I( 0, 0 ) );
    libSym1->AddDrawItem( libPin1 );

    SCH_SYMBOL* sym1 = new SCH_SYMBOL( *libSym1, libSym1->GetLibId(), &path, 0, 0, VECTOR2I( 100, 100 ) );
    sym1->UpdatePins();
    screen->Append( sym1 );

    // Create normal pin at same position
    LIB_SYMBOL* libSym2 = new LIB_SYMBOL( "sym2", nullptr );
    SCH_PIN* libPin2 = new SCH_PIN( libSym2 );
    libPin2->SetNumber( "1" );
    libPin2->SetType( ELECTRICAL_PINTYPE::PT_PASSIVE );
    libPin2->SetPosition( VECTOR2I( 0, 0 ) );
    libSym2->AddDrawItem( libPin2 );

    SCH_SYMBOL* sym2 = new SCH_SYMBOL( *libSym2, libSym2->GetLibId(), &path, 0, 0, VECTOR2I( 100, 100 ) );
    sym2->UpdatePins();
    screen->Append( sym2 );

    // Run ERC
    ERC_SETTINGS& settings = m_schematic->ErcSettings();

    // Enable all errors
    settings.m_ERCSeverities[ERCE_PIN_TO_PIN_WARNING] = RPT_SEVERITY_ERROR;
    settings.m_ERCSeverities[ERCE_PIN_TO_PIN_ERROR] = RPT_SEVERITY_ERROR;
    settings.m_ERCSeverities[ERCE_NOCONNECT_CONNECTED] = RPT_SEVERITY_ERROR;

    m_schematic->ConnectionGraph()->Recalculate( m_schematic->BuildSheetListSortedByPageNumbers(), true );
    m_schematic->ConnectionGraph()->RunERC();

    ERC_TESTER tester( m_schematic.get() );
    int ncErrors = tester.TestNoConnectPins();

    BOOST_CHECK_EQUAL( ncErrors, 1 );
}

BOOST_FIXTURE_TEST_CASE( Test1243, NC_PIN_CONNECTIVITY_FIXTURE )
{
    std::cout << "Data dir: " << KI_TEST::GetEeschemaTestDataDir() << std::endl;
    KI_TEST::LoadSchematic( m_settingsManager, "test1243/test1243", m_schematic );

    // Run ERC
    ERC_SETTINGS& settings = m_schematic->ErcSettings();
    SHEETLIST_ERC_ITEMS_PROVIDER errors( m_schematic.get() );

    // Enable all errors
    settings.m_ERCSeverities[ERCE_PIN_TO_PIN_WARNING] = RPT_SEVERITY_ERROR;
    settings.m_ERCSeverities[ERCE_PIN_TO_PIN_ERROR] = RPT_SEVERITY_ERROR;
    settings.m_ERCSeverities[ERCE_NOCONNECT_CONNECTED] = RPT_SEVERITY_ERROR;

    m_schematic->ConnectionGraph()->Recalculate( m_schematic->BuildSheetListSortedByPageNumbers(), true );
    m_schematic->ConnectionGraph()->RunERC();

    ERC_TESTER tester( m_schematic.get() );
    int ncErrors = tester.TestNoConnectPins();

    // We expect NO errors
    BOOST_CHECK_EQUAL( ncErrors, 0 );
    BOOST_CHECK_EQUAL( errors.GetCount(), 0 );
}
