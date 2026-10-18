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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <board.h>
#include <board_design_settings.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <netclass.h>
#include <pcb_track.h>
#include <project/net_settings.h>
#include <settings/settings_manager.h>


// An expression string cannot end in a backslash, so such a class loses only its own rules and is reported
BOOST_AUTO_TEST_CASE( DRCNetclassNameEndingInBackslash )
{
    SETTINGS_MANAGER       settingsManager;
    std::unique_ptr<BOARD> board;

    KI_TEST::LoadBoard( settingsManager, "connection_width_rules", board );

    BOARD_DESIGN_SETTINGS& bds = board->GetDesignSettings();
    bds.m_NetSettings->GetNetclasses().at( wxS( "High_current" ) )->SetName( wxS( "High_current\\" ) );

    std::vector<wxString> warnings;

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos, int aLayer,
                 const std::function<void( PCB_MARKER* )>& aPathGenerator )
            {
                if( aItem->GetErrorCode() == DRCE_GENERIC_WARNING )
                    warnings.push_back( aItem->GetErrorMessage( false ) );
            } );

    BOOST_REQUIRE_NO_THROW( bds.m_DRCEngine->InitEngine( board->GetDesignRulesPath() ) );
    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    BOOST_CHECK_EQUAL( warnings.size(), 1 );
    BOOST_CHECK( !warnings.empty() && warnings[0].Contains( wxS( "'High_current\\'" ) ) );

    PCB_TRACK* netA = nullptr;
    PCB_TRACK* netB = nullptr;

    for( PCB_TRACK* track : board->Tracks() )
    {
        if( track->GetNetname() == wxS( "net_1" ) )
            netA = track;
        else if( track->GetNetname() == wxS( "net_3" ) )
            netB = track;
    }

    BOOST_REQUIRE( netA && netB );

    DRC_CONSTRAINT constraint = bds.m_DRCEngine->EvalRules( CLEARANCE_CONSTRAINT, netA, netB, F_Cu );

    BOOST_REQUIRE( constraint.GetParentRule() );
    BOOST_CHECK_EQUAL( constraint.GetParentRule()->m_ImplicitNetclass, wxS( "Default" ) );
}
