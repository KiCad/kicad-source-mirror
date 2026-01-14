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
 * @file
 * Test for issue #22286 - Pin alternate type not being applied after schematic load.
 *
 * When a pin has an alternate function selected (like "8.pow" with type power_in),
 * the pin TYPE should switch from the default (no_connect) to the alternate's type.
 * This test verifies the fix by loading the actual reproduction schematic.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include "eeschema_test_utils.h"

#include <lib_symbol.h>
#include <pin_type.h>
#include <sch_pin.h>
#include <sch_symbol.h>
#include <wildcards_and_files_ext.h>


class TEST_ISSUE22286_FIXTURE : public KI_TEST::SCHEMATIC_TEST_FIXTURE
{
public:
    SCH_SYMBOL* FindSymbolByRef( const wxString& aRef )
    {
        if( !m_schematic )
            return nullptr;

        SCH_SCREEN* screen = m_schematic->RootScreen();

        if( !screen )
            return nullptr;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol && symbol->GetRef( &m_schematic->Hierarchy()[0], false ) == aRef )
                return symbol;
        }

        return nullptr;
    }

    SCH_PIN* FindPinByNumber( SCH_SYMBOL* aSymbol, const wxString& aNumber )
    {
        if( !aSymbol )
            return nullptr;

        for( SCH_PIN* pin : aSymbol->GetPins( &m_schematic->Hierarchy()[0] ) )
        {
            if( pin->GetNumber() == aNumber )
                return pin;
        }

        return nullptr;
    }
};


BOOST_FIXTURE_TEST_SUITE( Issue22286, TEST_ISSUE22286_FIXTURE )


/**
 * Test that pin alternates are correctly applied after loading a schematic from disk.
 *
 * This test loads the actual reproduction schematic for issue #22286 and verifies that:
 * 1. Pin 8 on symbol J1 has the alternate "8.pow" set
 * 2. Pin 8's GetType() returns PT_POWER_IN (not PT_NC)
 * 3. The library pin has the alternates map populated
 */
BOOST_AUTO_TEST_CASE( PinAlternateTypeAfterSchematicLoad )
{
    wxFileName fn;
    fn.SetPath( KI_TEST::GetEeschemaTestDataDir() );
    fn.AppendDir( wxS( "issue22286" ) );
    fn.SetName( wxS( "bugtest" ) );
    fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

    LoadSchematic( fn.GetFullPath() );

    // Find symbol J1 (the MOLEX connector)
    SCH_SYMBOL* j1 = FindSymbolByRef( wxS( "J1" ) );
    BOOST_REQUIRE_MESSAGE( j1 != nullptr, "Symbol J1 not found in schematic" );

    // Find pin 8
    SCH_PIN* pin8 = FindPinByNumber( j1, wxS( "8" ) );
    BOOST_REQUIRE_MESSAGE( pin8 != nullptr, "Pin 8 not found on symbol J1" );

    // Check that the alternate is set
    BOOST_CHECK_MESSAGE( pin8->GetAlt() == wxS( "8.pow" ),
                         "Pin 8 should have alternate '8.pow' set, but has '"
                         + pin8->GetAlt() + "'" );

    // Check that m_libPin is set
    BOOST_CHECK_MESSAGE( pin8->GetLibPin() != nullptr,
                         "Pin 8 should have m_libPin set after schematic load" );

    if( pin8->GetLibPin() )
    {
        // Check that the library pin has alternates
        const auto& alternates = pin8->GetLibPin()->GetAlternates();
        BOOST_CHECK_MESSAGE( !alternates.empty(),
                             "Library pin should have alternates populated" );
        BOOST_CHECK_MESSAGE( alternates.count( wxS( "8.pow" ) ) > 0,
                             "Library pin should have '8.pow' in alternates map" );

        // Check the alternate's type in the library pin
        if( alternates.count( wxS( "8.pow" ) ) > 0 )
        {
            const SCH_PIN::ALT& alt = alternates.at( wxS( "8.pow" ) );
            BOOST_CHECK_MESSAGE( alt.m_Type == ELECTRICAL_PINTYPE::PT_POWER_IN,
                                 "Library pin alternate '8.pow' should have type PT_POWER_IN" );
        }
    }

    // The critical test: GetType() should return the alternate's type, not the default
    ELECTRICAL_PINTYPE pinType = pin8->GetType();
    BOOST_CHECK_MESSAGE( pinType == ELECTRICAL_PINTYPE::PT_POWER_IN,
                         "Pin 8 GetType() should return PT_POWER_IN (7) for alternate '8.pow', "
                         "but returned " + std::to_string( static_cast<int>( pinType ) )
                         + " (" + ElectricalPinTypeGetText( pinType ) + ")" );

    // Verify the shown name is the alternate name
    BOOST_CHECK_MESSAGE( pin8->GetShownName() == wxS( "8.pow" ),
                         "Pin 8 GetShownName() should return '8.pow', but returned '"
                         + pin8->GetShownName() + "'" );
}


/**
 * Verify the connection graph does NOT treat the pin as no-connect.
 *
 * When pin 8 has type PT_POWER_IN (from alternate), it should NOT be marked
 * as a no-connect in the connection graph. It will still get a "Net-(...)" name
 * because it's on a regular connector (not a power symbol), but importantly it
 * should NOT be prefixed with "unconnected-".
 */
BOOST_AUTO_TEST_CASE( ConnectionGraphHandlesPinAlternateType )
{
    wxFileName fn;
    fn.SetPath( KI_TEST::GetEeschemaTestDataDir() );
    fn.AppendDir( wxS( "issue22286" ) );
    fn.SetName( wxS( "bugtest" ) );
    fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

    LoadSchematic( fn.GetFullPath() );

    SCH_SYMBOL* j1 = FindSymbolByRef( wxS( "J1" ) );
    BOOST_REQUIRE( j1 != nullptr );

    SCH_PIN* pin8 = FindPinByNumber( j1, wxS( "8" ) );
    BOOST_REQUIRE( pin8 != nullptr );

    // Get the net name for pin 8
    SCH_SHEET_PATH path = m_schematic->Hierarchy()[0];
    wxString netName = pin8->GetDefaultNetName( path );

    // With the fix, pin 8 should NOT get an "unconnected-(...)" name because
    // it's a power pin (PT_POWER_IN), not a no-connect pin (PT_NC).
    // It will still get a "Net-(...)" name because it's on a regular connector.
    BOOST_CHECK_MESSAGE( !netName.StartsWith( wxS( "unconnected-(" ) ),
                         "Pin 8 (power_in) should not get an 'unconnected-(...)' name. "
                         "Got: '" + netName + "'" );
}


BOOST_AUTO_TEST_SUITE_END()
