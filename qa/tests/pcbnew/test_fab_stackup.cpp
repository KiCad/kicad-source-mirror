/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or
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

#include <board.h>
#include <board_design_settings.h>
#include <pcbnew/exporters/fab_model/fab_stackup.h>

#include <algorithm>


BOOST_AUTO_TEST_CASE( FabStackupSnapshotsRealBoard )
{
    std::unique_ptr<BOARD> board = KI_TEST::ReadBoardFromFileOrStream( KI_TEST::GetPcbnewTestDataDir()
                                                                       + "ipc2581/dielectric-sublayer.kicad_pcb" );
    BOOST_REQUIRE( board );

    BOARD_DESIGN_SETTINGS&           settings = board->GetDesignSettings();
    BOARD_STACKUP                    before = settings.GetStackupDescriptor();
    std::vector<BOARD_STACKUP_ITEM*> beforeItems = settings.GetStackupDescriptor().GetList();
    BOARD_STACKUP                    expected = before;
    expected.SynchronizeWithBoard( &settings );

    FAB_STACKUP snapshot( *board );
    BOOST_CHECK( snapshot.Stackup() == expected );

    for( const BOARD_STACKUP_ITEM* item : snapshot.Stackup().GetList() )
    {
        BOOST_CHECK( std::ranges::none_of( beforeItems,
                                           [&]( const BOARD_STACKUP_ITEM* aSource )
                                           {
                                               return aSource == item;
                                           } ) );
    }

    BOOST_CHECK_EQUAL( snapshot.Thickness(), expected.BuildBoardThicknessFromStackup() );
    BOOST_CHECK( settings.GetStackupDescriptor() == before );
    BOOST_CHECK( settings.GetStackupDescriptor().GetList() == beforeItems );

    for( const BOARD_STACKUP_ITEM* item : snapshot.Stackup().GetList() )
    {
        if( item->GetBrdLayerId() != UNDEFINED_LAYER )
            BOOST_CHECK( snapshot.ItemForLayer( item->GetBrdLayerId() ) != nullptr );
    }

    BOOST_CHECK( snapshot.ItemForLayer( UNDEFINED_LAYER ) == nullptr );
}
