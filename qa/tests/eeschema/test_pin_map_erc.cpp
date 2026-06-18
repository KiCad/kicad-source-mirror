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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <memory>

#include <pin_map.h>
#include <lib_symbol.h>
#include <sch_pin.h>
#include <sch_symbol.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <erc/erc.h>


struct PIN_MAP_ERC_FIXTURE
{
    PIN_MAP_ERC_FIXTURE()
    {
        m_settingsManager.LoadProject( "" );
        m_schematic = std::make_unique<SCHEMATIC>( &m_settingsManager.Prj() );
        m_schematic->Reset();

        SCH_SHEET*  defaultSheet = m_schematic->GetTopLevelSheet( 0 );
        SCH_SHEET*  root = new SCH_SHEET( m_schematic.get() );
        SCH_SCREEN* screen = new SCH_SCREEN( m_schematic.get() );
        root->SetScreen( screen );

        m_schematic->AddTopLevelSheet( root );
        m_schematic->RemoveTopLevelSheet( defaultSheet );
        delete defaultSheet;
    }

    /// Build a two-pin symbol carrying @a aMap (+ optional symbol jumper group) and place it.
    void placeSymbol( PIN_MAP aMap, const std::set<wxString>& aJumperGroup = {} )
    {
        SCH_SHEET_PATH path;
        path.push_back( &m_schematic->Root() );

        m_libSym = std::make_unique<LIB_SYMBOL>( "DUT", nullptr );

        for( const wxString& number : { wxString( "1" ), wxString( "2" ) } )
        {
            SCH_PIN* pin = new SCH_PIN( m_libSym.get() );
            pin->SetNumber( number );
            pin->SetType( ELECTRICAL_PINTYPE::PT_PASSIVE );
            m_libSym->AddDrawItem( pin );
        }

        m_libSym->PinMaps().AddOrReplace( std::move( aMap ) );

        if( !aJumperGroup.empty() )
            m_libSym->JumperPinGroups().push_back( aJumperGroup );

        SCH_SYMBOL* sym = new SCH_SYMBOL( *m_libSym, m_libSym->GetLibId(), &path, 0, 0,
                                          VECTOR2I( 0, 0 ) );
        sym->UpdatePins();
        m_schematic->RootScreen()->Append( sym );
    }

    int runPinMapErc() { return ERC_TESTER( m_schematic.get() ).TestPinMap(); }

    SETTINGS_MANAGER            m_settingsManager;
    std::unique_ptr<SCHEMATIC>  m_schematic;
    std::unique_ptr<LIB_SYMBOL> m_libSym;
};


BOOST_FIXTURE_TEST_SUITE( PinMapErc, PIN_MAP_ERC_FIXTURE )


BOOST_AUTO_TEST_CASE( StalePinReferenceFires )
{
    PIN_MAP map( wxS( "M" ) );
    map.SetEntry( wxS( "1" ), wxS( "1" ) );
    map.SetEntry( wxS( "9" ), wxS( "9" ) );   // pin 9 does not exist on the symbol

    placeSymbol( std::move( map ) );

    BOOST_CHECK_EQUAL( runPinMapErc(), 1 );
}


BOOST_AUTO_TEST_CASE( DuplicatePadFires )
{
    PIN_MAP map( wxS( "M" ) );
    map.SetEntry( wxS( "1" ), wxS( "5" ) );
    map.SetEntry( wxS( "2" ), wxS( "5" ) );   // two different pins -> same pad

    placeSymbol( std::move( map ) );

    BOOST_CHECK_EQUAL( runPinMapErc(), 1 );
}


BOOST_AUTO_TEST_CASE( CleanMapStaysQuiet )
{
    PIN_MAP map( wxS( "M" ) );
    map.SetEntry( wxS( "1" ), wxS( "1" ) );
    map.SetEntry( wxS( "2" ), wxS( "2" ) );

    placeSymbol( std::move( map ) );

    BOOST_CHECK_EQUAL( runPinMapErc(), 0 );
}


BOOST_AUTO_TEST_CASE( JumperedDuplicateStaysQuiet )
{
    // Two pins deliberately jumpered (internally connected) may share a pad.
    PIN_MAP map( wxS( "M" ) );
    map.SetEntry( wxS( "1" ), wxS( "5" ) );
    map.SetEntry( wxS( "2" ), wxS( "5" ) );

    placeSymbol( std::move( map ), { wxS( "1" ), wxS( "2" ) } );

    BOOST_CHECK_EQUAL( runPinMapErc(), 0 );
}


BOOST_AUTO_TEST_CASE( StackedSamePinStaysQuiet )
{
    // One pin mapped to several pads (stacked) is not a duplicate-target collision.
    PIN_MAP map( wxS( "M" ) );
    map.SetEntry( wxS( "1" ), wxS( "[4,9]" ) );
    map.SetEntry( wxS( "2" ), wxS( "2" ) );

    placeSymbol( std::move( map ) );

    BOOST_CHECK_EQUAL( runPinMapErc(), 0 );
}


BOOST_AUTO_TEST_SUITE_END()
