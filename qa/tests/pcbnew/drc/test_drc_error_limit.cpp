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

#include <atomic>
#include <functional>

#include <math/vector2d.h>
#include <pcb_marker.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>


BOOST_AUTO_TEST_SUITE( DRCErrorLimit )


// ReportViolation enforces the per-code cap under its own lock, so a code can never report more
// violations than its limit no matter how many times (or from how many threads) it is hit.  With
// the cap check removed the handler fires on every call and the count runs past the limit.
BOOST_AUTO_TEST_CASE( CapIsExact )
{
    DRC_ENGINE engine;

    std::atomic<int> reported( 0 );

    engine.SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>&, const VECTOR2I&, int,
                 const std::function<void( PCB_MARKER* )>& )
            {
                reported.fetch_add( 1 );
            } );

    const int code = DRCE_ASSERTION_FAILURE;

    auto emit =
            [&]()
            {
                engine.ReportViolation( DRC_ITEM::Create( code ), VECTOR2I(), 0,
                                        []( PCB_MARKER* ) {} );
            };

    while( !engine.IsErrorLimitExceeded( code ) )
        emit();

    const int capped = reported.load();

    BOOST_TEST( capped > 0 );

    for( int ii = 0; ii < 500; ++ii )
        emit();

    BOOST_TEST( reported.load() == capped );
}


BOOST_AUTO_TEST_SUITE_END()
