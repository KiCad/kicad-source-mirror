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
 */

#include <boost/test/unit_test.hpp>

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <connection_graph.h>
#include <sch_netchain.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <locale_io.h>

#include <wx/debug.h>

#include <functional>
#include <stdexcept>


// Test backdoor declared in connection_graph.h.
void boost_test_inject_committed_net_chain( CONNECTION_GRAPH& aGraph,
                                            std::unique_ptr<SCH_NETCHAIN> aChain );


// Silent no-op assert handler.  wxFAIL_MSG routes through wxTheAssertHandler;
// returning without side effects swallows the assertion so execution continues.
static void swallowWxAssert( const wxString&, int, const wxString&, const wxString&,
                             const wxString& )
{
}


// RAII swap of the wx assert handler.  The eeschema test_module installs
// KI_TEST::wxAssertThrower, which converts every wxFAIL_MSG into a thrown
// WX_ASSERT_ERROR.  RebuildNetChains()'s catch block opens with wxFAIL_MSG;
// under the throwing handler that fires before the rollback statements run,
// hiding the very behaviour this test is trying to validate.  Install a
// guaranteed-silent handler for the duration of the throw test, then put the
// throwing handler back.
//
// Do not use wxSetDefaultAssertHandler() here.  The default handler is not
// silent across environments: headless CI builds trap/abort on the wxFAIL_MSG,
// which Boost's signal monitor reports as a failed test, while interactive
// builds merely log and continue.
struct ASSERT_HANDLER_SCOPE
{
    ASSERT_HANDLER_SCOPE()
    {
        wxSetAssertHandler( &swallowWxAssert );
    }

    ~ASSERT_HANDLER_SCOPE()
    {
        wxSetAssertHandler( &KI_TEST::wxAssertThrower );
    }
};


struct NETCHAIN_ROLLBACK_FIXTURE
{
    NETCHAIN_ROLLBACK_FIXTURE() : m_settingsManager() {}

    ~NETCHAIN_ROLLBACK_FIXTURE()
    {
        // Defensive cleanup so a failing test never leaks a hook into a sibling test.
        CONNECTION_GRAPH::RebuildNetChainsTestHook() = nullptr;
    }

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


// Regression for H-12.  RebuildNetChains() snapshots m_committedNetChains.size() and
// m_netChainsBuilt at entry and rolls them back if any of the restore passes throws.
// A regression that moved the snapshot inside the try, dropped the resize, or forgot to
// restore the built flag would leak partial chains and report stale readiness.  Inject a
// throw via the test hook and verify the catch handler truncates the vector and restores
// the flag.
BOOST_FIXTURE_TEST_CASE( NetChain_RebuildRollback_TruncatesAndRestoresOnThrow,
                         NETCHAIN_ROLLBACK_FIXTURE )
{
    LOCALE_IO dummy;
    KI_TEST::LoadSchematic( m_settingsManager, wxString( "net_chains_four_nets" ),
                            m_schematic );

    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();
    BOOST_REQUIRE( graph );

    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();

    // Initial pass leaves m_netChainsBuilt = true and the snapshot baseline at zero
    // committed chains (the fixture has none persisted).
    graph->Recalculate( sheets, /*aUnconditional=*/true );
    BOOST_REQUIRE( graph->NetChainsBuilt() );

    const std::size_t baselineCount = graph->GetCommittedNetChains().size();

    // Note: Recalculate(true) -> Reset() flips m_netChainsBuilt to false BEFORE
    // RebuildNetChains() runs, so the snapshot captured at RebuildNetChains entry is
    // always false on the unconditional path.  After the catch block restores the
    // snapshot value, NetChainsBuilt() must therefore read false.
    const bool expectedBuiltAfterRollback = false;

    // Hook fires inside the try block, after the restore loops have run.  It mimics a
    // partial-rebuild failure: push a fresh chain onto m_committedNetChains so the
    // container has grown beyond the snapshot, then throw.  The catch block must
    // truncate back to baselineCount and restore m_netChainsBuilt to baselineBuilt.
    bool hookFired = false;
    CONNECTION_GRAPH::RebuildNetChainsTestHook() =
            [&]( CONNECTION_GRAPH& aGraph )
            {
                hookFired = true;

                auto stray = std::make_unique<SCH_NETCHAIN>();
                stray->SetName( wxT( "ROLLBACK_PARTIAL" ) );
                stray->AddNet( wxT( "/STRAY_NET" ) );
                boost_test_inject_committed_net_chain( aGraph, std::move( stray ) );

                BOOST_REQUIRE_GT( aGraph.GetCommittedNetChains().size(), 0u );

                throw std::runtime_error( "rollback test injected throw" );
            };

    {
        ASSERT_HANDLER_SCOPE silenceWxFail;
        graph->Recalculate( sheets, /*aUnconditional=*/true );
    }

    CONNECTION_GRAPH::RebuildNetChainsTestHook() = nullptr;

    BOOST_CHECK_MESSAGE( hookFired,
                         "Rollback hook never fired; snapshot/restore code path was not exercised" );

    BOOST_CHECK_MESSAGE( graph->GetCommittedNetChains().size() == baselineCount,
                         "Catch block did not resize m_committedNetChains back to the snapshot; "
                         "partial chain leaked into the committed list" );

    BOOST_CHECK_MESSAGE( graph->NetChainsBuilt() == expectedBuiltAfterRollback,
                         "Catch block did not restore m_netChainsBuilt; readiness flag is stale" );

    // The stray name we injected must not survive the rollback.  GetNetChainByName scans
    // the live committed list; if rollback worked the lookup misses.
    BOOST_CHECK_MESSAGE( graph->GetNetChainByName( wxT( "ROLLBACK_PARTIAL" ) ) == nullptr,
                         "Partially-built chain 'ROLLBACK_PARTIAL' survived the rollback" );
}


// Companion check.  When RebuildNetChains() throws WITHOUT any growth in
// m_committedNetChains, the catch block must still restore the built-flag snapshot
// and clear m_potentialNetChains, leaving the graph in a clean pre-rebuild state.
// Catches a regression that flipped m_netChainsBuilt to true before the hook call.
BOOST_FIXTURE_TEST_CASE( NetChain_RebuildRollback_NoGrowthStillRestoresFlagAndPotentials,
                         NETCHAIN_ROLLBACK_FIXTURE )
{
    LOCALE_IO dummy;
    KI_TEST::LoadSchematic( m_settingsManager, wxString( "net_chains_four_nets" ),
                            m_schematic );

    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();
    BOOST_REQUIRE( graph );

    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();

    // LoadSchematic already ran a Recalculate, so the graph has potentials and built==true.
    // We re-run with throw to confirm rollback returns the built flag to false (the
    // snapshot value captured AFTER Reset() runs at the top of Recalculate(true)).
    BOOST_REQUIRE( graph->NetChainsBuilt() );
    const std::size_t baselineCount = graph->GetCommittedNetChains().size();

    bool hookFired = false;
    CONNECTION_GRAPH::RebuildNetChainsTestHook() =
            [&]( CONNECTION_GRAPH& )
            {
                hookFired = true;
                throw std::runtime_error( "no-growth rollback test throw" );
            };

    {
        ASSERT_HANDLER_SCOPE silenceWxFail;
        graph->Recalculate( sheets, /*aUnconditional=*/true );
    }

    CONNECTION_GRAPH::RebuildNetChainsTestHook() = nullptr;

    BOOST_CHECK( hookFired );

    // Reset() inside Recalculate(true) clears m_netChainsBuilt to false BEFORE
    // RebuildNetChains() snapshots it; after rollback the flag must read false.
    BOOST_CHECK_MESSAGE( !graph->NetChainsBuilt(),
                         "Rollback left m_netChainsBuilt = true; the success-flag write "
                         "escaped the catch handler's snapshot restore" );

    BOOST_CHECK_EQUAL( graph->GetCommittedNetChains().size(), baselineCount );

    // The catch block also clears m_potentialNetChains so a partial pass cannot leak
    // half-populated inferred groupings into the next consumer.
    BOOST_CHECK( graph->GetPotentialNetChains().empty() );
}
