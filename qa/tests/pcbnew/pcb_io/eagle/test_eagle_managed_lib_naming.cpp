/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
 * @file test_eagle_managed_lib_naming.cpp
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/18515
 *
 * Eagle 9.x managed libraries carry a URN such as
 * "urn:adsk.eagle:library:38243636". Older importer code appended the
 * library URN ordinal to every package name, producing footprints with
 * names like "LED-0603_38243636" on the board even though the matching
 * schematic importer wrote bare "LED-0603" into the symbol's footprint
 * field. The mismatch broke "Update PCB from Schematic" because the
 * netlist updater could no longer link symbols to their footprints.
 */

#include <pcbnew_utils/board_file_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <pcbnew/pcb_io/eagle/pcb_io_eagle.h>

#include <board.h>
#include <footprint.h>
#include <lib_id.h>

#include <wx/filefn.h>
#include <wx/filename.h>


BOOST_AUTO_TEST_SUITE( EagleManagedLibNaming )


BOOST_AUTO_TEST_CASE( FootprintNamesHaveNoUrnSuffix )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir()
                           + "plugins/eagle/issue18515_managed_lib.brd";

    BOOST_REQUIRE_MESSAGE( wxFileName::FileExists( dataPath ),
                           "Test board file not found: " + dataPath );

    // This board carries no Eagle clearance matrix, so loading it must not leave an
    // empty .kicad_dru sidecar behind in the source tree.
    wxFileName rulesFn( dataPath );
    rulesFn.SetExt( wxT( "kicad_dru" ) );

    BOOST_REQUIRE_MESSAGE( !rulesFn.FileExists(),
                           "Stale rules sidecar present before load: "
                                   + rulesFn.GetFullPath().ToStdString() );

    PCB_IO_EAGLE eaglePlugin;
    BOARD*       rawBoard = nullptr;

    try
    {
        rawBoard = eaglePlugin.LoadBoard( dataPath, nullptr, nullptr );
    }
    catch( const IO_ERROR& e )
    {
        BOOST_FAIL( "IO_ERROR loading Eagle board: " + e.What().ToStdString() );
    }
    catch( const std::exception& e )
    {
        BOOST_FAIL( std::string( "Exception loading Eagle board: " ) + e.what() );
    }

    std::unique_ptr<BOARD> board( rawBoard );

    BOOST_REQUIRE( board );

    // The demo project has two elements: D1 (LED-0603) and R1 (R0603).
    BOOST_REQUIRE_EQUAL( board->Footprints().size(), 2 );

    bool sawLed   = false;
    bool sawRes   = false;

    for( FOOTPRINT* fp : board->Footprints() )
    {
        const wxString itemName = fp->GetFPID().GetUniStringLibItemName();

        // The fix's invariant: bare Eagle package names with no
        // "_<urn-ordinal>" suffix appended.
        BOOST_CHECK_MESSAGE(
                !itemName.Contains( wxT( "_38243636" ) ),
                "Footprint name '" + itemName.ToStdString()
                        + "' should not carry the Eagle library URN ordinal." );

        if( itemName == wxT( "LED-0603" ) )
            sawLed = true;
        else if( itemName == wxT( "R0603" ) )
            sawRes = true;
    }

    BOOST_CHECK_MESSAGE( sawLed, "Expected footprint named 'LED-0603' (no URN suffix)" );
    BOOST_CHECK_MESSAGE( sawRes, "Expected footprint named 'R0603' (no URN suffix)" );

    // With no clearance matrix to convert, the importer must not write an empty
    // rules sidecar into the source tree.
    if( rulesFn.FileExists() )
    {
        wxRemoveFile( rulesFn.GetFullPath() );
        BOOST_ERROR( "Importer wrote an empty rules sidecar: "
                     + rulesFn.GetFullPath().ToStdString() );
    }
}


BOOST_AUTO_TEST_SUITE_END()
