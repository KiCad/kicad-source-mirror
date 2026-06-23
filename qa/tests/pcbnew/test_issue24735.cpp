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
#include <pcbnew_utils/board_test_utils.h>
#include <settings/settings_manager.h>
#include <board.h>
#include <footprint.h>
#include <zone.h>

struct ISSUE24735_FIXTURE
{
    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


BOOST_FIXTURE_TEST_SUITE( Issue24735, ISSUE24735_FIXTURE )

// A v10 board stores footprint-embedded zone outlines in board frame.  On load
// the parser converts them to the footprint lib frame, but the border hatch
// lines were generated from the board-frame outline beforehand.  Without a
// regeneration GetHatchLines() applies the footprint transform a second time
// and the rule area's hatched border renders far from the footprint.
BOOST_AUTO_TEST_CASE( EmbeddedZoneHatchFollowsFootprint )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue24735/issue24735", m_board );

    int checkedZones = 0;
    int checkedHatches = 0;

    for( const FOOTPRINT* fp : m_board->Footprints() )
    {
        for( const ZONE* zone : fp->Zones() )
        {
            BOX2I zoneBBox = zone->GetBoundingBox();

            BOOST_CHECK_MESSAGE( fp->GetBoundingBox().Intersects( zoneBBox ),
                                 "Embedded zone of " << fp->GetReference()
                                 << " is separated from its footprint" );

            for( const SEG& hatch : zone->GetHatchLines() )
            {
                checkedHatches++;

                BOOST_CHECK_MESSAGE( zoneBBox.Contains( hatch.A ) && zoneBBox.Contains( hatch.B ),
                                     "Hatch line of " << fp->GetReference()
                                     << " lies outside the zone outline" );
            }

            checkedZones++;
        }
    }

    BOOST_CHECK_GT( checkedZones, 0 );
    BOOST_CHECK_GT( checkedHatches, 0 );
}

BOOST_AUTO_TEST_SUITE_END()
