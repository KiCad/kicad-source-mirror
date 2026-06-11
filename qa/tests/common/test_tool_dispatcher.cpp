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

#include <boost/test/unit_test.hpp>
#include <tool/tool_dispatcher.h>

/**
 * Tests for TOOL_DISPATCHER::ShouldDropAutoRepeat, the pure decision that discards backlogged
 * OS key auto-repeat events delivered after a hotkey was released (issue 24540).
 */

BOOST_AUTO_TEST_SUITE( ToolDispatcherAutoRepeat )

// A single key press whose key has already been released (a quick tap) must always run.
BOOST_AUTO_TEST_CASE( LeadingEdgeAlwaysRuns )
{
    int        lastKey = 0;
    wxLongLong lastTime = 0;

    bool drop = TOOL_DISPATCHER::ShouldDropAutoRepeat( 'R', /*now*/ 1000, /*keyIsDown*/ false,
                                                       lastKey, lastTime );

    BOOST_CHECK( !drop );
    BOOST_CHECK_EQUAL( lastKey, 'R' );
    BOOST_CHECK( lastTime == 1000 );
}

// Repeats that arrive within the burst window while the key is still held keep running.
BOOST_AUTO_TEST_CASE( HeldKeyRepeatsRun )
{
    int        lastKey = 0;
    wxLongLong lastTime = 0;

    BOOST_CHECK( !TOOL_DISPATCHER::ShouldDropAutoRepeat( 'R', 1000, true, lastKey, lastTime ) );
    BOOST_CHECK( !TOOL_DISPATCHER::ShouldDropAutoRepeat( 'R', 1030, true, lastKey, lastTime ) );
    BOOST_CHECK( !TOOL_DISPATCHER::ShouldDropAutoRepeat( 'R', 1060, true, lastKey, lastTime ) );
}

// The defect: repeats from the same burst that arrive after release must be dropped.
BOOST_AUTO_TEST_CASE( StaleRepeatAfterReleaseDropped )
{
    int        lastKey = 0;
    wxLongLong lastTime = 0;

    BOOST_CHECK( !TOOL_DISPATCHER::ShouldDropAutoRepeat( 'R', 1000, true, lastKey, lastTime ) );

    // Backlogged events still in the queue after the key came up.
    BOOST_CHECK( TOOL_DISPATCHER::ShouldDropAutoRepeat( 'R', 1030, false, lastKey, lastTime ) );
    BOOST_CHECK( TOOL_DISPATCHER::ShouldDropAutoRepeat( 'R', 1060, false, lastKey, lastTime ) );
}

// A second deliberate tap after a human-scale gap is a new burst and must run again.
BOOST_AUTO_TEST_CASE( SecondDeliberateTapRuns )
{
    int        lastKey = 0;
    wxLongLong lastTime = 0;

    BOOST_CHECK( !TOOL_DISPATCHER::ShouldDropAutoRepeat( 'R', 1000, false, lastKey, lastTime ) );

    // Well beyond the auto-repeat window, so this is treated as a fresh press, not stale backlog.
    wxLongLong later = 1000 + TOOL_DISPATCHER::AutoRepeatWindowMs + 1;
    BOOST_CHECK( !TOOL_DISPATCHER::ShouldDropAutoRepeat( 'R', later, false, lastKey, lastTime ) );
}

// Switching to a different key starts a new burst whose leading edge runs.
BOOST_AUTO_TEST_CASE( DifferentKeyIsNewBurst )
{
    int        lastKey = 0;
    wxLongLong lastTime = 0;

    BOOST_CHECK( !TOOL_DISPATCHER::ShouldDropAutoRepeat( 'R', 1000, true, lastKey, lastTime ) );

    // A different hotkey arriving immediately is not part of the R burst.
    BOOST_CHECK( !TOOL_DISPATCHER::ShouldDropAutoRepeat( 'F', 1010, false, lastKey, lastTime ) );
}

BOOST_AUTO_TEST_SUITE_END()
