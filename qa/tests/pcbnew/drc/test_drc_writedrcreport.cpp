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
#include <board.h>
#include <board_design_settings.h>
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
