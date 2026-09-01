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
#include <tool/tool_event.h>
#include <tool/tool_interactive.h>
#include <tool/tool_manager.h>

/**
 * Tests for TOOL_MANAGER::PrimeTool. A primed tool must see a left click carrying the primed
 * position. The router main loop starts routing from such a click, and the microvia stack
 * router handoff primes the router at the stack's last via to continue routing from it.
 */

namespace
{
// Not the tool name: for TC_COMMAND events matching is by name alone, so a start event named
// like the tool would also match the activation event Activate() sends and re-enter Main.
const TOOL_EVENT START_EVENT( TC_COMMAND, TA_ACTION, std::string( "test.primeRecorder.start" ) );


class PRIME_RECORDER_TOOL : public TOOL_INTERACTIVE
{
public:
    PRIME_RECORDER_TOOL() :
            TOOL_INTERACTIVE( "test.primeRecorder" ),
            m_gotClick( false ),
            m_hadPosition( false )
    {
    }

    void Reset( RESET_REASON aReason ) override {}

    int Main( const TOOL_EVENT& aEvent )
    {
        Activate();

        while( TOOL_EVENT* evt = Wait() )
        {
            // The same test the router main loop uses to start routing from a click.
            if( evt->IsClick( BUT_LEFT ) )
            {
                m_gotClick = true;
                m_hadPosition = evt->HasPosition();

                if( m_hadPosition )
                    m_clickPos = evt->Position();

                break;
            }
        }

        return 0;
    }

    bool     m_gotClick;
    bool     m_hadPosition;
    VECTOR2D m_clickPos;

private:
    void setTransitions() override { Go( &PRIME_RECORDER_TOOL::Main, START_EVENT ); }
};
} // namespace


BOOST_AUTO_TEST_SUITE( ToolManagerPriming )

BOOST_AUTO_TEST_CASE( PrimeActsAsPositionedLeftClick )
{
    TOOL_MANAGER         mgr;
    PRIME_RECORDER_TOOL* tool = new PRIME_RECORDER_TOOL();

    mgr.SetEnvironment( nullptr, nullptr, nullptr, nullptr, nullptr );
    mgr.RegisterTool( tool );
    mgr.ResetTools( TOOL_BASE::RUN );

    mgr.ProcessEvent( START_EVENT );
    BOOST_REQUIRE( !tool->m_gotClick );

    const VECTOR2D primePos( 1250000, -730000 );
    mgr.PrimeTool( primePos );

    // Priming only queues the click, so it arrives on the next processing pass.
    BOOST_REQUIRE( !tool->m_gotClick );
    mgr.ProcessEvent( TOOL_EVENT( TC_MOUSE, TA_MOUSE_MOTION ) );

    BOOST_REQUIRE( tool->m_gotClick );
    BOOST_CHECK( tool->m_hadPosition );
    BOOST_CHECK_EQUAL( tool->m_clickPos.x, primePos.x );
    BOOST_CHECK_EQUAL( tool->m_clickPos.y, primePos.y );
}

BOOST_AUTO_TEST_SUITE_END()
