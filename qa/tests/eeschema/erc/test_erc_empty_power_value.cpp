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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <erc/erc.h>
#include <erc/erc_settings.h>
#include <lib_symbol.h>
#include <sch_line.h>
#include <sch_marker.h>
#include <sch_pin.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <settings/settings_manager.h>


BOOST_AUTO_TEST_SUITE( ERCEmptyPowerValue )


struct EMPTY_POWER_VALUE_ERC_FIXTURE
{
    EMPTY_POWER_VALUE_ERC_FIXTURE()
    {
        m_mgr.LoadProject( "" );
        m_schematic = std::make_unique<SCHEMATIC>( &m_mgr.Prj() );
        m_schematic->Reset();
        SCH_SHEET* defaultSheet = m_schematic->GetTopLevelSheet( 0 );

        m_screen = new SCH_SCREEN( m_schematic.get() );
        m_sheet = new SCH_SHEET( m_schematic.get() );
        m_sheet->SetScreen( m_screen );
        m_schematic->AddTopLevelSheet( m_sheet );
        m_schematic->RemoveTopLevelSheet( defaultSheet );
        delete defaultSheet;
    }

    /// Build a single pin power symbol, the shape every rail symbol in the stock library has.
    LIB_SYMBOL* MakePowerLibSymbol( const wxString& aName, bool aLocal )
    {
        LIB_SYMBOL* part = new LIB_SYMBOL( aName );

        if( aLocal )
            part->SetLocalPower();
        else
            part->SetGlobalPower();

        SCH_PIN* pin = new SCH_PIN( part );
        pin->SetNumber( wxT( "1" ) );
        pin->SetName( aName );
        pin->SetType( ELECTRICAL_PINTYPE::PT_POWER_IN );
        pin->SetVisible( false );
        part->AddDrawItem( pin );

        part->GetValueField().SetText( aName );
        m_libSymbols.emplace_back( part );

        return part;
    }

    /// An ordinary symbol carrying a hidden power input, the legacy shape that names its net
    /// from the pin and never from the value.
    LIB_SYMBOL* MakeSymbolWithHiddenPowerPin( const wxString& aName, const wxString& aPinName )
    {
        LIB_SYMBOL* part = new LIB_SYMBOL( aName );

        SCH_PIN* pin = new SCH_PIN( part );
        pin->SetNumber( wxT( "1" ) );
        pin->SetName( aPinName );
        pin->SetType( ELECTRICAL_PINTYPE::PT_POWER_IN );
        pin->SetVisible( false );
        part->AddDrawItem( pin );

        m_libSymbols.emplace_back( part );

        return part;
    }

    SCH_SYMBOL* Place( LIB_SYMBOL* aPart, const VECTOR2I& aPos, const wxString& aValue )
    {
        SCH_SYMBOL* symbol = new SCH_SYMBOL( *aPart, aPart->GetLibId(), &m_schematic->CurrentSheet(), 1, 1, aPos );
        symbol->GetField( FIELD_T::VALUE )->SetText( aValue );
        m_screen->Append( symbol );

        return symbol;
    }

    int RunCheck()
    {
        m_schematic->RebuildConnectivity();

        ERC_TESTER tester( m_schematic.get() );
        return tester.TestEmptyPowerValues();
    }

    int CountMarkers()
    {
        int count = 0;

        for( SCH_ITEM* item : m_screen->Items().OfType( SCH_MARKER_T ) )
        {
            SCH_MARKER* marker = static_cast<SCH_MARKER*>( item );

            if( marker->GetRCItem()->GetErrorCode() == ERCE_EMPTY_POWER_VALUE )
                count++;
        }

        return count;
    }

    SETTINGS_MANAGER                         m_mgr;
    std::unique_ptr<SCHEMATIC>               m_schematic;
    SCH_SCREEN*                              m_screen;
    SCH_SHEET*                               m_sheet;
    std::vector<std::unique_ptr<LIB_SYMBOL>> m_libSymbols;
};


BOOST_FIXTURE_TEST_CASE( EmptyGlobalPowerValueFlagged, EMPTY_POWER_VALUE_ERC_FIXTURE )
{
    LIB_SYMBOL* rail = MakePowerLibSymbol( wxT( "+3V3" ), false );

    Place( rail, VECTOR2I( 1000000, 0 ), wxEmptyString );

    BOOST_CHECK_EQUAL( RunCheck(), 1 );
    BOOST_CHECK_EQUAL( CountMarkers(), 1 );
}


BOOST_FIXTURE_TEST_CASE( EmptyLocalPowerValueFlagged, EMPTY_POWER_VALUE_ERC_FIXTURE )
{
    LIB_SYMBOL* rail = MakePowerLibSymbol( wxT( "+3V3" ), true );

    Place( rail, VECTOR2I( 1000000, 0 ), wxEmptyString );

    BOOST_CHECK_EQUAL( RunCheck(), 1 );
    BOOST_CHECK_EQUAL( CountMarkers(), 1 );
}


BOOST_FIXTURE_TEST_CASE( WhitespaceOnlyPowerValueFlagged, EMPTY_POWER_VALUE_ERC_FIXTURE )
{
    LIB_SYMBOL* rail = MakePowerLibSymbol( wxT( "+3V3" ), false );

    Place( rail, VECTOR2I( 1000000, 0 ), wxT( "   " ) );

    BOOST_CHECK_EQUAL( RunCheck(), 1 );
    BOOST_CHECK_EQUAL( CountMarkers(), 1 );
}


BOOST_FIXTURE_TEST_CASE( NamedPowerValuePasses, EMPTY_POWER_VALUE_ERC_FIXTURE )
{
    LIB_SYMBOL* rail = MakePowerLibSymbol( wxT( "+3V3" ), false );
    LIB_SYMBOL* ground = MakePowerLibSymbol( wxT( "GND" ), false );

    Place( rail, VECTOR2I( 1000000, 0 ), wxT( "+3V3" ) );
    Place( ground, VECTOR2I( 3000000, 0 ), wxT( "GND" ) );

    BOOST_CHECK_EQUAL( RunCheck(), 0 );
    BOOST_CHECK_EQUAL( CountMarkers(), 0 );
}


BOOST_FIXTURE_TEST_CASE( HiddenPowerPinOnOrdinarySymbolPasses, EMPTY_POWER_VALUE_ERC_FIXTURE )
{
    LIB_SYMBOL* part = MakeSymbolWithHiddenPowerPin( wxT( "74LS00" ), wxT( "VCC" ) );

    Place( part, VECTOR2I( 1000000, 0 ), wxEmptyString );

    BOOST_CHECK_EQUAL( RunCheck(), 0 );
    BOOST_CHECK_EQUAL( CountMarkers(), 0 );
}


BOOST_AUTO_TEST_SUITE_END()
