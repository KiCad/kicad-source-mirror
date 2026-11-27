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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>

#include <board.h>
#include <board_design_settings.h>
#include <pcb_track.h>
#include <drc/drc_engine.h>
#include <settings/settings_manager.h>


struct DRC_COURTYARD_VIA_LAYERS_FIXTURE
{
    ~DRC_COURTYARD_VIA_LAYERS_FIXTURE()
    {
        if( m_board )
            m_board->SetProject( nullptr );
    }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


// The GND via's annular ring touches J10's fine_pitch courtyard but its hole does not, so the
// neckdown rule holds on F.Cu even after In2.Cu, where the ring is removed, was evaluated first
BOOST_FIXTURE_TEST_CASE( CourtyardViaLayers, DRC_COURTYARD_VIA_LAYERS_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "courtyard_via_layers/courtyard_via_layers", m_board );

    BOARD_ITEM* gndVia = m_board->ResolveItem( KIID( "3dabf713-a4fc-42a1-a89e-031a79420612" ), true );
    BOARD_ITEM* gpioVia = m_board->ResolveItem( KIID( "2cba258f-5364-4baa-aec7-aa46feab491d" ), true );

    BOOST_REQUIRE( gndVia && gpioVia );

    std::shared_ptr<DRC_ENGINE> drc = m_board->GetDesignSettings().m_DRCEngine;

    drc->EvalRules( CLEARANCE_CONSTRAINT, gndVia, gpioVia, In2_Cu );

    DRC_CONSTRAINT constraint = drc->EvalRules( CLEARANCE_CONSTRAINT, gndVia, gpioVia, F_Cu );

    BOOST_CHECK_EQUAL( constraint.GetValue().Min(), pcbIUScale.mmToIU( 0.1 ) );
}
