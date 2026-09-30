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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <pcbnew_utils/board_file_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>
#include <boost/test/unit_test.hpp>
#include <json_common.h>

#include <array>
#include <memory>
#include <set>
#include <string>

#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <pcbnew/exporters/fab_model/fab_pin.h>


BOOST_AUTO_TEST_CASE( FabPinRoles )
{
    const std::array<const char*, 3> boardNames = { "connect/connect.kicad_pcb",
                                                   "issue19325/issue19325.kicad_pcb",
                                                   "issue14130.kicad_pcb" };
    size_t fiducials = 0;
    size_t castellated = 0;
    size_t tooling = 0;
    size_t issue14130NpthTht = 0;
    size_t issue14130NpthHole = 0;
    size_t plated = 0;
    size_t padCount = 0;
    std::set<wxString> issue14130NpthNames;

    for( const char* boardName : boardNames )
    {
        std::unique_ptr<BOARD> board = KI_TEST::ReadBoardFromFileOrStream( KI_TEST::GetPcbnewTestDataDir()
                                                                            + boardName );
        BOOST_REQUIRE_MESSAGE( board, boardName );

        for( const FOOTPRINT* footprint : board->Footprints() )
        {
            for( size_t index = 0; index < footprint->Pads().size(); ++index )
            {
                const PAD* pad = footprint->Pads()[index];
                FAB_PIN fabPin = MakeFabPin( *pad, index );
                std::string location = std::string( boardName ) + "/" + footprint->GetReference().ToStdString() + "/"
                                       + std::to_string( index );
                ++padCount;

                if( pad->GetProperty() == PAD_PROP::FIDUCIAL_GLBL )
                {
                    ++fiducials;
                    BOOST_CHECK( GetFabPadRole( *pad ) == FAB_PAD_ROLE::FIDUCIAL_GLOBAL );
                }

                if( pad->GetProperty() == PAD_PROP::CASTELLATED )
                {
                    ++castellated;
                    BOOST_CHECK( GetFabPadRole( *pad ) == FAB_PAD_ROLE::CASTELLATED );
                }

                if( pad->GetAttribute() == PAD_ATTRIB::NPTH && pad->GetNumber().IsEmpty()
                    && pad->GetProperty() == PAD_PROP::NONE )
                {
                    ++tooling;
                    BOOST_CHECK( GetFabPadRole( *pad ) == FAB_PAD_ROLE::TOOLING_HOLE );
                    BOOST_CHECK( pad->HasHole() );
                    BOOST_CHECK( fabPin.m_mount == FAB_MOUNT::THT || fabPin.m_mount == FAB_MOUNT::HOLE );
                    BOOST_CHECK( fabPin.m_electrical == FAB_ELECTRICAL::MECHANICAL );
                }

                if( std::string( boardName ) == "issue14130.kicad_pcb"
                    && footprint->GetReference() == wxS( "P2" ) && pad->GetAttribute() == PAD_ATTRIB::NPTH )
                {
                    BOOST_CHECK_MESSAGE( issue14130NpthNames.insert( fabPin.m_name ).second,
                                         location << " repeats an NPTH pin name" );

                    if( fabPin.m_mount == FAB_MOUNT::THT )
                    {
                        ++issue14130NpthTht;
                    }
                    else if( fabPin.m_mount == FAB_MOUNT::HOLE )
                    {
                        ++issue14130NpthHole;
                    }
                }

                if( std::string( boardName ) == "connect/connect.kicad_pcb"
                    && pad->GetAttribute() != PAD_ATTRIB::NPTH && !pad->GetNumber().IsEmpty()
                    && pad->IsOnCopperLayer() )
                {
                    ++plated;
                    BOOST_CHECK( fabPin.m_electrical == FAB_ELECTRICAL::ELECTRICAL );
                }
            }
        }
    }

    BOOST_CHECK_GT( fiducials, 0 );
    BOOST_CHECK_GT( castellated, 0 );
    BOOST_CHECK_GT( tooling, 0 );
    BOOST_CHECK_GT( plated, 0 );
    BOOST_CHECK_EQUAL( issue14130NpthNames.size(), 7 );
    BOOST_CHECK_EQUAL( issue14130NpthTht, 1 );
    BOOST_CHECK_EQUAL( issue14130NpthHole, 6 );
    BOOST_CHECK_GT( padCount, 0 );
}
