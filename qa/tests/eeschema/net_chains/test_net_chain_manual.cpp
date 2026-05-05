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
#include <schematic_utils/schematic_file_util.h>

#include <connection_graph.h>
#include <kiid.h>
#include <sch_netchain.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <locale_io.h>

#include <fstream>
#include <set>

#include <wx/filename.h>
#include <wx/filefn.h>


struct NETCHAIN_MANUAL_FIXTURE
{
    NETCHAIN_MANUAL_FIXTURE() : m_settingsManager() {}

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


// Force-create a manual net chain via the same backend path the dialog uses when
// no passthrough connectivity exists between the two terminals.  Validates that:
//  - the chain is inserted into m_committedNetChains (not the legacy KIID-keyed map)
//  - it is queryable by GetNetChainByName / GetNetChainForNet
//  - it survives a Recalculate via the terminal-ref restore pass
//  - duplicate names and IsValidName violations are rejected
BOOST_FIXTURE_TEST_CASE( NetChain_ManualForceCreate_CommitsAndQueryable, NETCHAIN_MANUAL_FIXTURE )
{
    LOCALE_IO dummy;
    KI_TEST::LoadSchematic( m_settingsManager, wxString( "net_chains_four_nets" ), m_schematic );

    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();
    BOOST_REQUIRE( graph );

    graph->Recalculate( m_schematic->BuildSheetListSortedByPageNumbers(), true );

    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();
    BOOST_REQUIRE( !sheets.empty() );

    // Find two distinct symbols and one connected pin from each to use as terminals.
    SCH_SYMBOL* symA = nullptr;
    SCH_SYMBOL* symB = nullptr;
    SCH_PIN*    pinA = nullptr;
    SCH_PIN*    pinB = nullptr;
    wxString    refA, refB, pinNumA, pinNumB;
    wxString    netA, netB;

    for( const SCH_SHEET_PATH& sp : sheets )
    {
        SCH_SCREEN* sc = sp.LastScreen();

        if( !sc )
            continue;

        for( SCH_ITEM* item : sc->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );

            for( SCH_PIN* p : sym->GetPins( &sp ) )
            {
                if( !p->Connection() || p->Connection()->Name().IsEmpty() )
                    continue;

                if( !symA )
                {
                    symA = sym;
                    pinA = p;
                    refA = sym->GetRef( &sp );
                    pinNumA = p->GetNumber();
                    netA = p->Connection()->Name();
                }
                else if( sym != symA && p->Connection()->Name() != netA )
                {
                    symB = sym;
                    pinB = p;
                    refB = sym->GetRef( &sp );
                    pinNumB = p->GetNumber();
                    netB = p->Connection()->Name();
                    break;
                }
            }

            if( symB )
                break;
        }

        if( symB )
            break;
    }

    BOOST_REQUIRE( symA );
    BOOST_REQUIRE( symB );
    BOOST_REQUIRE( pinA );
    BOOST_REQUIRE( pinB );

    std::set<SCH_SYMBOL*> symbols { symA, symB };
    std::set<wxString>    nets    { netA, netB };

    // IsValidName rejects spaces — verify the API enforces this.
    SCH_NETCHAIN* rejectedSpace = graph->CreateManualNetChain(
            wxT( "BAD NAME" ), symbols, nets, pinA->m_Uuid, pinB->m_Uuid, refA, pinNumA, refB, pinNumB );
    BOOST_CHECK( rejectedSpace == nullptr );

    // Empty names are rejected.
    SCH_NETCHAIN* rejectedEmpty = graph->CreateManualNetChain(
            wxEmptyString, symbols, nets, pinA->m_Uuid, pinB->m_Uuid, refA, pinNumA, refB, pinNumB );
    BOOST_CHECK( rejectedEmpty == nullptr );

    SCH_NETCHAIN* committed = graph->CreateManualNetChain(
            wxT( "MANUAL_CHAIN" ), symbols, nets, pinA->m_Uuid, pinB->m_Uuid, refA, pinNumA, refB, pinNumB );

    BOOST_REQUIRE( committed );
    BOOST_CHECK_EQUAL( committed->GetName(), wxString( wxT( "MANUAL_CHAIN" ) ) );
    BOOST_CHECK_EQUAL( committed->GetNets().count( netA ), 1u );
    BOOST_CHECK_EQUAL( committed->GetNets().count( netB ), 1u );
    BOOST_CHECK_EQUAL( committed->GetSymbols().count( symA ), 1u );
    BOOST_CHECK_EQUAL( committed->GetSymbols().count( symB ), 1u );
    BOOST_CHECK_EQUAL( committed->GetTerminalRef( 0 ), refA );
    BOOST_CHECK_EQUAL( committed->GetTerminalPinNum( 0 ), pinNumA );
    BOOST_CHECK_EQUAL( committed->GetTerminalRef( 1 ), refB );
    BOOST_CHECK_EQUAL( committed->GetTerminalPinNum( 1 ), pinNumB );

    // Chain must be reachable through the public accessors that IO save, netlist
    // export, and the Setup panel use.
    BOOST_CHECK( graph->GetNetChainByName( wxT( "MANUAL_CHAIN" ) ) == committed );

    bool foundInCommittedList = false;

    for( const std::unique_ptr<SCH_NETCHAIN>& chain : graph->GetCommittedNetChains() )
    {
        if( chain && chain->GetName() == wxT( "MANUAL_CHAIN" ) )
        {
            foundInCommittedList = true;
            break;
        }
    }

    BOOST_CHECK( foundInCommittedList );

    // GetNetChainForNet should resolve member nets to the manual chain.
    BOOST_CHECK( graph->GetNetChainForNet( netA ) == committed );
    BOOST_CHECK( graph->GetNetChainForNet( netB ) == committed );

    // Duplicate name must be rejected.
    SCH_NETCHAIN* duplicate = graph->CreateManualNetChain(
            wxT( "MANUAL_CHAIN" ), symbols, nets, pinA->m_Uuid, pinB->m_Uuid, refA, pinNumA, refB, pinNumB );
    BOOST_CHECK( duplicate == nullptr );

    // Member symbols carry the chain marker so the post-Recalculate restore pass
    // does not reapply a different name.
    BOOST_CHECK_EQUAL( symA->GetNetChainName(), wxString( wxT( "MANUAL_CHAIN" ) ) );
    BOOST_CHECK_EQUAL( symB->GetNetChainName(), wxString( wxT( "MANUAL_CHAIN" ) ) );
}


// Reject an attempt to claim a net already owned by a committed chain.  Without this
// guard, GetNetChainForNet() (which returns the first match) silently picks one chain
// over the other based on iteration order, producing ambiguous netclass / colour
// resolution.
BOOST_FIXTURE_TEST_CASE( NetChain_ManualForceCreate_RejectsNetCollision, NETCHAIN_MANUAL_FIXTURE )
{
    LOCALE_IO dummy;
    KI_TEST::LoadSchematic( m_settingsManager, wxString( "net_chains_four_nets" ), m_schematic );

    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();
    BOOST_REQUIRE( graph );

    graph->Recalculate( m_schematic->BuildSheetListSortedByPageNumbers(), true );

    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();

    SCH_SYMBOL* symA = nullptr;
    SCH_SYMBOL* symB = nullptr;
    SCH_PIN*    pinA = nullptr;
    SCH_PIN*    pinB = nullptr;
    wxString    refA, refB, pinNumA, pinNumB;
    wxString    netA, netB;

    for( const SCH_SHEET_PATH& sp : sheets )
    {
        SCH_SCREEN* sc = sp.LastScreen();

        if( !sc )
            continue;

        for( SCH_ITEM* item : sc->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );

            for( SCH_PIN* p : sym->GetPins( &sp ) )
            {
                if( !p->Connection() || p->Connection()->Name().IsEmpty() )
                    continue;

                if( !symA )
                {
                    symA = sym;
                    pinA = p;
                    refA = sym->GetRef( &sp );
                    pinNumA = p->GetNumber();
                    netA = p->Connection()->Name();
                }
                else if( sym != symA && p->Connection()->Name() != netA )
                {
                    symB = sym;
                    pinB = p;
                    refB = sym->GetRef( &sp );
                    pinNumB = p->GetNumber();
                    netB = p->Connection()->Name();
                    break;
                }
            }

            if( symB )
                break;
        }

        if( symB )
            break;
    }

    BOOST_REQUIRE( symA && symB && pinA && pinB );

    std::set<SCH_SYMBOL*> symbols { symA, symB };
    std::set<wxString>    nets    { netA, netB };

    SCH_NETCHAIN* first = graph->CreateManualNetChain(
            wxT( "FIRST" ), symbols, nets, pinA->m_Uuid, pinB->m_Uuid, refA, pinNumA, refB, pinNumB );
    BOOST_REQUIRE( first );

    // Attempt a second chain with overlapping membership.  Must be rejected.
    std::set<wxString> overlapNets { netA };
    SCH_NETCHAIN*      collision = graph->CreateManualNetChain(
            wxT( "SECOND" ), symbols, overlapNets, pinA->m_Uuid, pinB->m_Uuid, refA, pinNumA, refB,
            pinNumB );

    BOOST_CHECK( collision == nullptr );
    BOOST_CHECK( graph->GetNetChainByName( wxT( "SECOND" ) ) == nullptr );

    // The first chain still owns the contested net.
    BOOST_CHECK( graph->GetNetChainForNet( netA ) == first );
}


// A manual chain has no underlying inferred potential, so the existing terminal-ref
// restore path can't reconstruct it.  Verify that the (nets ...) sexpr field combined
// with the manual fallback in RebuildNetChains restores it after a save->reload cycle.
BOOST_FIXTURE_TEST_CASE( NetChain_ManualForceCreate_SurvivesSaveReload, NETCHAIN_MANUAL_FIXTURE )
{
    LOCALE_IO dummy;
    KI_TEST::LoadSchematic( m_settingsManager, wxString( "net_chains_four_nets" ), m_schematic );

    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();
    BOOST_REQUIRE( graph );

    graph->Recalculate( m_schematic->BuildSheetListSortedByPageNumbers(), true );

    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();
    BOOST_REQUIRE( !sheets.empty() );

    SCH_SYMBOL* symA = nullptr;
    SCH_SYMBOL* symB = nullptr;
    SCH_PIN*    pinA = nullptr;
    SCH_PIN*    pinB = nullptr;
    wxString    refA, refB, pinNumA, pinNumB;
    wxString    netA, netB;

    for( const SCH_SHEET_PATH& sp : sheets )
    {
        SCH_SCREEN* sc = sp.LastScreen();

        if( !sc )
            continue;

        for( SCH_ITEM* item : sc->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );

            for( SCH_PIN* p : sym->GetPins( &sp ) )
            {
                if( !p->Connection() || p->Connection()->Name().IsEmpty() )
                    continue;

                if( !symA )
                {
                    symA = sym;
                    pinA = p;
                    refA = sym->GetRef( &sp );
                    pinNumA = p->GetNumber();
                    netA = p->Connection()->Name();
                }
                else if( sym != symA && p->Connection()->Name() != netA )
                {
                    symB = sym;
                    pinB = p;
                    refB = sym->GetRef( &sp );
                    pinNumB = p->GetNumber();
                    netB = p->Connection()->Name();
                    break;
                }
            }

            if( symB )
                break;
        }

        if( symB )
            break;
    }

    BOOST_REQUIRE( symA && symB && pinA && pinB );

    std::set<SCH_SYMBOL*> symbols { symA, symB };
    std::set<wxString>    nets    { netA, netB };

    SCH_NETCHAIN* committed = graph->CreateManualNetChain(
            wxT( "ROUNDTRIP" ), symbols, nets, pinA->m_Uuid, pinB->m_Uuid, refA, pinNumA, refB,
            pinNumB );
    BOOST_REQUIRE( committed );
    committed->SetNetClass( wxT( "rt_class" ) );

    // Round-trip via the kicad_sexpr IO.
    wxString tmpDir = wxFileName::CreateTempFileName( wxT( "kicad_qa_netchain_" ) );
    wxRemoveFile( tmpDir );
    wxMkdir( tmpDir );
    wxString tmpFile = tmpDir + wxFileName::GetPathSeparator() + wxT( "out.kicad_sch" );

    SCH_SHEET* root = m_schematic->GetTopLevelSheet( 0 );
    BOOST_REQUIRE( root );

    KI_TEST::DumpSchematicToFile( *m_schematic, *root, tmpFile.ToStdString() );

    // Reload from the dumped file.  ReadSchematicFromStream pipes the net-chain
    // override maps into the new connection graph for us.
    std::ifstream              stream( tmpFile.ToStdString() );
    BOOST_REQUIRE( stream.good() );

    PROJECT*                   prj = &m_schematic->Project();
    std::unique_ptr<SCHEMATIC> reloaded = KI_TEST::ReadSchematicFromStream( stream, prj );
    BOOST_REQUIRE( reloaded );

    reloaded->ConnectionGraph()->Recalculate( reloaded->BuildSheetListSortedByPageNumbers(), true );

    SCH_NETCHAIN* restored = reloaded->ConnectionGraph()->GetNetChainByName( wxT( "ROUNDTRIP" ) );
    BOOST_REQUIRE_MESSAGE( restored, "Manual net chain failed to survive save/reload" );

    BOOST_CHECK_EQUAL( restored->GetName(), wxString( wxT( "ROUNDTRIP" ) ) );
    BOOST_CHECK_EQUAL( restored->GetNetClass(), wxString( wxT( "rt_class" ) ) );
    BOOST_CHECK_EQUAL( restored->GetNets().count( netA ), 1u );
    BOOST_CHECK_EQUAL( restored->GetNets().count( netB ), 1u );

    wxRemoveFile( tmpFile );
    wxRmdir( tmpDir );
}


// Exercises the panel rename path against a manual chain.  Member-net overrides must
// be rekeyed alongside the netclass / colour / terminal-ref maps; without it, a
// subsequent Recalculate would either lose the manual chain (override missing under
// the new name) or resurrect a stale ghost under the old name.
BOOST_FIXTURE_TEST_CASE( NetChain_ManualForceCreate_RenameRekeysMemberNetOverrides,
                         NETCHAIN_MANUAL_FIXTURE )
{
    LOCALE_IO dummy;
    KI_TEST::LoadSchematic( m_settingsManager, wxString( "net_chains_four_nets" ), m_schematic );

    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();
    BOOST_REQUIRE( graph );

    graph->Recalculate( m_schematic->BuildSheetListSortedByPageNumbers(), true );

    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();
    BOOST_REQUIRE( !sheets.empty() );

    SCH_SYMBOL* symA = nullptr;
    SCH_SYMBOL* symB = nullptr;
    SCH_PIN*    pinA = nullptr;
    SCH_PIN*    pinB = nullptr;
    wxString    refA, refB, pinNumA, pinNumB;
    wxString    netA, netB;

    for( const SCH_SHEET_PATH& sp : sheets )
    {
        SCH_SCREEN* sc = sp.LastScreen();

        if( !sc )
            continue;

        for( SCH_ITEM* item : sc->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );

            for( SCH_PIN* p : sym->GetPins( &sp ) )
            {
                if( !p->Connection() || p->Connection()->Name().IsEmpty() )
                    continue;

                if( !symA )
                {
                    symA = sym;
                    pinA = p;
                    refA = sym->GetRef( &sp );
                    pinNumA = p->GetNumber();
                    netA = p->Connection()->Name();
                }
                else if( sym != symA && p->Connection()->Name() != netA )
                {
                    symB = sym;
                    pinB = p;
                    refB = sym->GetRef( &sp );
                    pinNumB = p->GetNumber();
                    netB = p->Connection()->Name();
                    break;
                }
            }

            if( symB )
                break;
        }

        if( symB )
            break;
    }

    BOOST_REQUIRE( symA && symB && pinA && pinB );

    std::set<SCH_SYMBOL*> symbols { symA, symB };
    std::set<wxString>    nets    { netA, netB };

    // Seed the member-net override map under the original name, exactly as the
    // sexpr parser would after reading a (nets ...) field.
    std::map<wxString, std::set<wxString>> seed;
    seed[wxT( "ORIG" )] = nets;
    graph->SetNetChainMemberNetOverrides( seed );

    SCH_NETCHAIN* committed = graph->CreateManualNetChain(
            wxT( "ORIG" ), symbols, nets, pinA->m_Uuid, pinB->m_Uuid, refA, pinNumA, refB,
            pinNumB );
    BOOST_REQUIRE( committed );

    // Rename through the same API the panel uses on Apply.
    BOOST_REQUIRE( graph->RenameCommittedNetChain( wxT( "ORIG" ), wxT( "NEW_NAME" ) ) );

    const auto& memberOverrides = graph->GetNetChainMemberNetOverrides();
    BOOST_CHECK_EQUAL( memberOverrides.count( wxT( "ORIG" ) ), 0u );
    BOOST_CHECK_EQUAL( memberOverrides.count( wxT( "NEW_NAME" ) ), 1u );

    auto it = memberOverrides.find( wxT( "NEW_NAME" ) );
    BOOST_REQUIRE( it != memberOverrides.end() );
    BOOST_CHECK_EQUAL( it->second.count( netA ), 1u );
    BOOST_CHECK_EQUAL( it->second.count( netB ), 1u );

    BOOST_CHECK( graph->GetNetChainByName( wxT( "ORIG" ) ) == nullptr );
    BOOST_CHECK( graph->GetNetChainByName( wxT( "NEW_NAME" ) ) == committed );

    // DeleteCommittedNetChain must drop the rekeyed override entry along with the chain.
    BOOST_REQUIRE( graph->DeleteCommittedNetChain( wxT( "NEW_NAME" ) ) );
    BOOST_CHECK_EQUAL( graph->GetNetChainMemberNetOverrides().count( wxT( "NEW_NAME" ) ), 0u );
}
