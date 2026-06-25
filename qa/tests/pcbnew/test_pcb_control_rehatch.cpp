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

// rehatchBoardItem() runs on every board item on every edit (BOARD_COMMIT::Push posts
// rehatchShapes), so re-caching anything but hatch-filled shapes stalls commits on dense boards.

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <board.h>
#include <pcb_shape.h>
#include <pcb_track.h>
#include <tools/pcb_control.h>
#include <view/view.h>

namespace
{

// Records which items get queued for redraw without needing a GAL backend.
class RECORDING_VIEW : public KIGFX::VIEW
{
public:
    void Update( const KIGFX::VIEW_ITEM* aItem, int aUpdateFlags ) const override
    {
        m_updated.push_back( aItem );
    }

    void Update( const KIGFX::VIEW_ITEM* aItem ) const override { m_updated.push_back( aItem ); }

    bool WasUpdated( const KIGFX::VIEW_ITEM* aItem ) const
    {
        return std::find( m_updated.begin(), m_updated.end(), aItem ) != m_updated.end();
    }

    mutable std::vector<const KIGFX::VIEW_ITEM*> m_updated;
};

} // namespace


BOOST_AUTO_TEST_SUITE( PcbControlRehatch )


BOOST_AUTO_TEST_CASE( RehatchOnlyTouchesHatchedShapes )
{
    BOARD board;

    PCB_SHAPE solidRect( &board, SHAPE_T::RECTANGLE );
    solidRect.SetFillMode( FILL_T::NO_FILL );

    PCB_SHAPE filledRect( &board, SHAPE_T::RECTANGLE );
    filledRect.SetFillMode( FILL_T::FILLED_SHAPE );

    PCB_SHAPE hatchedRect( &board, SHAPE_T::RECTANGLE );
    hatchedRect.SetFillMode( FILL_T::HATCH );

    PCB_TRACK track( &board );

    BOOST_REQUIRE( !solidRect.IsHatchedFill() );
    BOOST_REQUIRE( !filledRect.IsHatchedFill() );
    BOOST_REQUIRE( hatchedRect.IsHatchedFill() );

    RECORDING_VIEW view;

    PCB_CONTROL::rehatchBoardItem( &view, &solidRect );
    PCB_CONTROL::rehatchBoardItem( &view, &filledRect );
    PCB_CONTROL::rehatchBoardItem( &view, &track );
    PCB_CONTROL::rehatchBoardItem( &view, &hatchedRect );

    BOOST_CHECK( view.WasUpdated( &hatchedRect ) );
    BOOST_CHECK( !view.WasUpdated( &solidRect ) );
    BOOST_CHECK( !view.WasUpdated( &filledRect ) );
    BOOST_CHECK( !view.WasUpdated( &track ) );
    BOOST_CHECK_EQUAL( view.m_updated.size(), 1u );
}


BOOST_AUTO_TEST_SUITE_END()
