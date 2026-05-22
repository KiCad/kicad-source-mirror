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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <base_units.h>
#include <board.h>
#include <board_design_settings.h>
#include <drawing_sheet/ds_proxy_view_item.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <pcb_marker.h>
#include <python/scripting/pcbnew_scripting_helpers.h>
#include <settings/settings_manager.h>

#include <wx/filefn.h>
#include <wx/filename.h>


// Regression test for https://gitlab.com/kicad/code/kicad/-/issues/24124
//
// WriteDRCReport must not destroy markers already present on the board. Action
// plugins that call this helper may hold raw pointers to those markers in undo
// snapshots; deleting the markers leaves the host frame with dangling pointers
// and crashes on the next event loop iteration.

struct WRITE_DRC_REPORT_FIXTURE
{
    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


BOOST_FIXTURE_TEST_CASE( WriteDRCReportPreservesMarkers, WRITE_DRC_REPORT_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue9870", m_board );

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    // Seed a saved DRC exclusion. The previous WriteDRCReport implementation
    // silently wiped m_DrcExclusions via ResolveDRCExclusions(false); guard
    // against that regression.
    const wxString sentinelExclusion( wxT( "sentinel-drc-exclusion" ) );
    const wxString sentinelComment( wxT( "sentinel-drc-comment" ) );

    bds.m_DrcExclusions.insert( sentinelExclusion );
    bds.m_DrcExclusionComments[sentinelExclusion] = sentinelComment;

    // Seed the board with a synthetic marker, mimicking a prior interactive DRC run.
    std::shared_ptr<DRC_ITEM> seedItem = DRC_ITEM::Create( DRCE_CLEARANCE );
    PCB_MARKER* seedMarker = new PCB_MARKER( seedItem, VECTOR2I( 0, 0 ), F_Cu );
    m_board->Add( seedMarker );

    PCB_MARKER* preservedAddress = seedMarker;

    BOOST_REQUIRE_EQUAL( m_board->Markers().size(), 1u );

    wxString outPath = wxFileName::CreateTempFileName( wxT( "kicad-drc-report" ) );
    BOOST_REQUIRE( !outPath.IsEmpty() );

    bool ok = WriteDRCReport( m_board.get(), outPath, EDA_UNITS::MM, true );

    BOOST_CHECK_MESSAGE( ok, "WriteDRCReport returned false" );

    // Marker list must be intact.
    BOOST_REQUIRE_EQUAL( m_board->Markers().size(), 1u );
    BOOST_CHECK_EQUAL( m_board->Markers().front(), preservedAddress );

    // Saved exclusion data must also survive.
    BOOST_CHECK_EQUAL( bds.m_DrcExclusions.count( sentinelExclusion ), 1u );
    BOOST_REQUIRE_EQUAL( bds.m_DrcExclusionComments.count( sentinelExclusion ), 1u );
    BOOST_CHECK_EQUAL( bds.m_DrcExclusionComments.at( sentinelExclusion ),
                       sentinelComment );

    if( wxFileExists( outPath ) )
        wxRemoveFile( outPath );
}


// Second regression test for https://gitlab.com/kicad/code/kicad/-/issues/24124
//
// Distinct mechanism from the marker-preservation bug fixed in 6f4dbf133e:
// the DRC engine on BOARD_DESIGN_SETTINGS is shared across consumers, and
// DIALOG_DRC leaves a borrowed DS_PROXY_VIEW_ITEM* on it pointing at a proxy
// owned by PCB_DRAW_PANEL_GAL via unique_ptr. When an action plugin runs,
// PCB_EDIT_FRAME's post-plugin reload calls SetPageSettings, which destroys
// the canvas's proxy and installs a fresh one without notifying the engine.
// A subsequent WriteDRCReport invocation from another plugin would then
// dereference the stale pointer in DRC_TEST_PROVIDER_MISC::testTextVars and
// crash. WriteDRCReport must own the proxy used for its run and reset the
// engine's borrowed pointer before that proxy is destroyed.

BOOST_FIXTURE_TEST_CASE( WriteDRCReportClearsStaleDrawingSheet, WRITE_DRC_REPORT_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue9870", m_board );

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    // Pre-create the shared engine and seed a drawing-sheet pointer on it, the
    // way DIALOG_DRC would leave it after a GUI DRC run.
    bds.m_DRCEngine = std::make_shared<DRC_ENGINE>( m_board.get(), &bds );

    auto staleProxy = std::make_unique<DS_PROXY_VIEW_ITEM>( pcbIUScale, &m_board->GetPageSettings(),
                                                            m_board->GetProject(), &m_board->GetTitleBlock(), nullptr );

    DS_PROXY_VIEW_ITEM* staleAddress = staleProxy.get();
    bds.m_DRCEngine->SetDrawingSheet( staleAddress );
    BOOST_REQUIRE_EQUAL( bds.m_DRCEngine->GetDrawingSheet(), staleAddress );

    // Destroy the owner of the proxy AFTER the engine has captured its pointer.
    // This is the exact lifetime that PCB_EDIT_FRAME::SetPageSettings produces
    // post-plugin: the canvas's DS_PROXY_VIEW_ITEM is destroyed while the
    // engine still holds the borrowed pointer. The engine's GetDrawingSheet()
    // address-compare confirms the engine has not been notified.
    staleProxy.reset();
    BOOST_REQUIRE_EQUAL( bds.m_DRCEngine->GetDrawingSheet(), staleAddress );

    wxString outPath = wxFileName::CreateTempFileName( wxT( "kicad-drc-report" ) );
    BOOST_REQUIRE( !outPath.IsEmpty() );

    // Without the fix, WriteDRCReport reaches testTextVars while the engine
    // still holds the dangling pointer above, and BuildDrawItemsList crashes
    // on PAGE_INFO::GetTypeAsString with a null this. With the fix, the
    // engine's pointer is overwritten with a fresh proxy bound to this board
    // before RunTests runs.
    bool ok = WriteDRCReport( m_board.get(), outPath, EDA_UNITS::MM, false );
    BOOST_CHECK_MESSAGE( ok, "WriteDRCReport returned false" );

    // The fix: WriteDRCReport replaces the engine's drawing-sheet pointer with
    // a fresh proxy bound to this board's state for the duration of the run
    // (mirroring PCBNEW_JOBS_HANDLER::getDrawingSheetProxyView for kicad-cli),
    // then clears it before that local proxy is destroyed. After this call
    // the engine must not be left holding a borrowed pointer to anything,
    // whether the caller's pre-seeded proxy or WriteDRCReport's own.
    BOOST_CHECK_MESSAGE( bds.m_DRCEngine->GetDrawingSheet() == nullptr,
                         "WriteDRCReport must reset DRC_ENGINE drawing-sheet pointer" );

    if( wxFileExists( outPath ) )
        wxRemoveFile( outPath );
}
