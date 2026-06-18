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
#include <set>

#include <pin_map.h>
#include <lib_symbol.h>
#include <sch_pin.h>
#include <sch_symbol.h>
#include <sch_sheet_path.h>


using PAD_RESOLUTION = SCH_PIN::PAD_RESOLUTION;


namespace
{
LIB_ID makeFp( const wxString& aLibNick, const wxString& aName )
{
    LIB_ID id;
    id.SetLibNickname( aLibNick );
    id.SetLibItemName( aName );
    return id;
}


SCH_PIN* addPin( LIB_SYMBOL& aSymbol, const wxString& aNumber )
{
    SCH_PIN* pin = new SCH_PIN( &aSymbol );
    pin->SetNumber( aNumber );
    pin->SetType( ELECTRICAL_PINTYPE::PT_PASSIVE );
    aSymbol.AddDrawItem( pin );
    return pin;
}
} // namespace


struct PIN_MAP_RESOLVER_FIXTURE
{
    PIN_MAP_RESOLVER_FIXTURE()
    {
        m_dfn = makeFp( wxS( "Package_DFN_QFN" ), wxS( "DFN-8-1EP" ) );

        m_lib = std::make_unique<LIB_SYMBOL>( wxS( "LM358" ), nullptr );
        addPin( *m_lib, wxS( "1" ) );   // identity in DFN
        addPin( *m_lib, wxS( "4" ) );   // mapped to [4,9]
        addPin( *m_lib, wxS( "8" ) );   // not mapped, no pad -> UNMAPPED

        PIN_MAP map( wxS( "DFN-8-EP" ) );
        map.SetEntry( wxS( "4" ), wxS( "[4,9]" ) );
        m_lib->PinMaps().AddOrReplace( map );
        m_lib->SetAssociatedFootprints( { { m_dfn, wxS( "DFN-8-EP" ) } } );

        m_symbol = std::make_unique<SCH_SYMBOL>( *m_lib, m_lib->GetLibId(), nullptr, 0, 0,
                                                 VECTOR2I( 0, 0 ) );
        m_symbol->UpdatePins();
    }

    SCH_PIN* pin( const wxString& aNumber ) const { return m_symbol->GetPin( aNumber ); }

    LIB_ID                      m_dfn;
    std::unique_ptr<LIB_SYMBOL>  m_lib;
    std::unique_ptr<SCH_SYMBOL>  m_symbol;
    SCH_SHEET_PATH               m_sheet;   // empty path; no variant means base override
};


BOOST_FIXTURE_TEST_SUITE( PinMapResolver, PIN_MAP_RESOLVER_FIXTURE )


BOOST_AUTO_TEST_CASE( ThreeResolutionStates )
{
    std::set<wxString> dfnPads = { wxS( "1" ), wxS( "2" ), wxS( "3" ), wxS( "4" ),
                                   wxS( "5" ), wxS( "6" ), wxS( "7" ), wxS( "9" ) };

    PAD_RESOLUTION state;

    // MAPPED: pin 4 -> [4,9] via the library map (no footprint needed for MAPPED).
    BOOST_CHECK_EQUAL( pin( wxS( "4" ) )->GetEffectivePadNumber( m_sheet, wxEmptyString, m_dfn,
                                                                &dfnPads, &state ),
                       wxS( "[4,9]" ) );
    BOOST_CHECK( state == PAD_RESOLUTION::MAPPED );

    // IDENTITY: pin 1 has no entry but pad 1 exists on the footprint.
    BOOST_CHECK_EQUAL( pin( wxS( "1" ) )->GetEffectivePadNumber( m_sheet, wxEmptyString, m_dfn,
                                                                &dfnPads, &state ),
                       wxS( "1" ) );
    BOOST_CHECK( state == PAD_RESOLUTION::IDENTITY );

    // UNMAPPED: pin 8 has no entry and pad 8 is absent (set has 1-7 and 9).
    BOOST_CHECK( pin( wxS( "8" ) )->GetEffectivePadNumber( m_sheet, wxEmptyString, m_dfn, &dfnPads,
                                                          &state ).IsEmpty() );
    BOOST_CHECK( state == PAD_RESOLUTION::UNMAPPED );
}


BOOST_AUTO_TEST_CASE( NoFootprintPathAssumesIdentity )
{
    PAD_RESOLUTION state;

    // MAPPED entries still resolve with no footprint.
    BOOST_CHECK_EQUAL( pin( wxS( "4" ) )->GetEffectivePadNumber( m_sheet, wxEmptyString, m_dfn,
                                                                nullptr, &state ),
                       wxS( "[4,9]" ) );
    BOOST_CHECK( state == PAD_RESOLUTION::MAPPED );

    // Everything unmapped is assumed identity (returns the pin number) on the painter path.
    BOOST_CHECK_EQUAL( pin( wxS( "8" ) )->GetEffectivePadNumber( m_sheet, wxEmptyString, m_dfn,
                                                                nullptr, &state ),
                       wxS( "8" ) );
    BOOST_CHECK( state == PAD_RESOLUTION::IDENTITY );
}


BOOST_AUTO_TEST_CASE( OverridePrecedence )
{
    PAD_RESOLUTION state;

    // FORCE_IDENTITY ignores the library map: pin 4 falls through to identity/unmapped.
    PIN_MAP_INSTANCE_OVERRIDE forceIdentity;
    forceIdentity.m_Mode = PIN_MAP_OVERRIDE_MODE::FORCE_IDENTITY;
    m_symbol->SetPinMapOverride( forceIdentity );

    std::set<wxString> pads = { wxS( "4" ) };
    BOOST_CHECK_EQUAL( pin( wxS( "4" ) )->GetEffectivePadNumber( m_sheet, wxEmptyString, m_dfn,
                                                                &pads, &state ),
                       wxS( "4" ) );
    BOOST_CHECK( state == PAD_RESOLUTION::IDENTITY );

    // Sparse edits are ignored under forced identity (PIN_MAP_INSTANCE_OVERRIDE contract).
    forceIdentity.m_Edits.push_back( { wxS( "4" ), wxS( "99" ) } );
    m_symbol->SetPinMapOverride( forceIdentity );
    BOOST_CHECK_EQUAL( pin( wxS( "4" ) )->GetEffectivePadNumber( m_sheet, wxEmptyString, m_dfn,
                                                                &pads, &state ),
                       wxS( "4" ) );
    BOOST_CHECK( state == PAD_RESOLUTION::IDENTITY );

    // A sparse instance edit beats the library map.
    PIN_MAP_INSTANCE_OVERRIDE edited;
    edited.m_Mode = PIN_MAP_OVERRIDE_MODE::USE_LIBRARY_DEFAULT;
    edited.m_Edits.push_back( { wxS( "4" ), wxS( "99" ) } );
    m_symbol->SetPinMapOverride( edited );

    BOOST_CHECK_EQUAL( pin( wxS( "4" ) )->GetEffectivePadNumber( m_sheet, wxEmptyString, m_dfn,
                                                                nullptr, &state ),
                       wxS( "99" ) );
    BOOST_CHECK( state == PAD_RESOLUTION::MAPPED );
}


BOOST_AUTO_TEST_CASE( NamedMapOverride )
{
    PAD_RESOLUTION state;

    // USE_NAMED_MAP forces a specific map by name regardless of footprint association.
    m_lib->PinMaps().AddOrReplace( PIN_MAP( wxS( "SWAP" ) ) );
    m_lib->PinMaps().FindByName( wxS( "SWAP" ) )->SetEntry( wxS( "1" ), wxS( "8" ) );

    // Rebuild the symbol so its flattened library reference carries the new map.
    m_symbol = std::make_unique<SCH_SYMBOL>( *m_lib, m_lib->GetLibId(), nullptr, 0, 0,
                                             VECTOR2I( 0, 0 ) );
    m_symbol->UpdatePins();

    PIN_MAP_INSTANCE_OVERRIDE named;
    named.m_Mode = PIN_MAP_OVERRIDE_MODE::USE_NAMED_MAP;
    named.m_ActiveMapName = wxS( "SWAP" );
    m_symbol->SetPinMapOverride( named );

    BOOST_CHECK_EQUAL( pin( wxS( "1" ) )->GetEffectivePadNumber( m_sheet, wxEmptyString, m_dfn,
                                                                nullptr, &state ),
                       wxS( "8" ) );
    BOOST_CHECK( state == PAD_RESOLUTION::MAPPED );
}


BOOST_AUTO_TEST_SUITE_END()
