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

// high contrast + colour/alpha display changes used to recache the whole board
// displayOptionsRequireRecache limits recache to real geometry changes

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <pcb_edit_frame.h>
#include <pcb_display_options.h>
#include <project/board_project_settings.h>


BOOST_AUTO_TEST_SUITE( DisplayOptionsRecache )


BOOST_AUTO_TEST_CASE( ColorAndAlphaChangesSkipRecache )
{
    PCB_DISPLAY_OPTIONS base;

    // same options no recache
    BOOST_CHECK( !PCB_BASE_FRAME::displayOptionsRequireRecache( base, base ) );

    // high contrast is a colour-lookup flag geometry unchanged
    PCB_DISPLAY_OPTIONS contrast = base;
    contrast.m_ContrastModeDisplay = HIGH_CONTRAST_MODE::HIDDEN;
    BOOST_CHECK( !PCB_BASE_FRAME::displayOptionsRequireRecache( base, contrast ) );

    // opacity is a group colour so recolour suffices
    PCB_DISPLAY_OPTIONS opacity = base;
    opacity.m_TrackOpacity = 0.5;
    BOOST_CHECK( !PCB_BASE_FRAME::displayOptionsRequireRecache( base, opacity ) );

    // net colouring re-tints existing groups
    PCB_DISPLAY_OPTIONS netColor = base;
    netColor.m_NetColorMode = NET_COLOR_MODE::ALL;
    BOOST_CHECK( !PCB_BASE_FRAME::displayOptionsRequireRecache( base, netColor ) );
}


BOOST_AUTO_TEST_CASE( GeometryChangesRequireRecache )
{
    PCB_DISPLAY_OPTIONS base;

    // filled vs outline zones differ in geometry
    PCB_DISPLAY_OPTIONS zoneMode = base;
    zoneMode.m_ZoneDisplayMode = ZONE_DISPLAY_MODE::SHOW_ZONE_OUTLINE;
    BOOST_CHECK( PCB_BASE_FRAME::displayOptionsRequireRecache( base, zoneMode ) );

    // board flip regenerates mirrored geometry
    PCB_DISPLAY_OPTIONS flip = base;
    flip.m_FlipBoardView = true;
    BOOST_CHECK( PCB_BASE_FRAME::displayOptionsRequireRecache( base, flip ) );
}


BOOST_AUTO_TEST_SUITE_END()
