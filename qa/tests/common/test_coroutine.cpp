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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file test_coroutine.cpp
 * Test suite for coroutines.
 *
 * See also the coroutine utility in qa/common_tools for a command line
 * test utility.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <tool/coroutine.h>

#include <common.h>


/**
 * An event in a simple coroutine harness.
 */
struct COROUTINE_TEST_EVENT
{
    enum class TYPE
    {
        START,
        CALL,
        YIELD,
        RETURNED,
        END,
    };

    TYPE m_type;
    int  m_value;

    bool operator==( const COROUTINE_TEST_EVENT& aOther ) const
    {
        return m_type == aOther.m_type && m_value == aOther.m_value;
    }

    bool operator!=( const COROUTINE_TEST_EVENT& aOther ) const
    {
        return !operator==( aOther );
    }
};


/**
 * Define a stream function for logging this type.
 */
std::ostream& boost_test_print_type( std::ostream& os, const COROUTINE_TEST_EVENT& aObj )
{
    os << "COROUTINE_TEST_EVENT[ type: " << (int) aObj.m_type << ", value: " << aObj.m_value
       << " ]";
    return os;
}


/**
 * Simple coroutine harness that runs a coroutine that increments a number up
 * to a pre-set limit, spitting out coroutine events as it goes.
 *
 * This can then be used to ensure the events are occurring as expected.
 */
class COROUTINE_INCREMENTING_HARNESS
{
public:
    /**
     * The coroutine test take ints and returns them
     */
    using TEST_COROUTINE = COROUTINE<int, int>;

    using EVT_HANDLER = std::function<void( const COROUTINE_TEST_EVENT& )>;

    COROUTINE_INCREMENTING_HARNESS( EVT_HANDLER aHandler, int aCount )
            : m_handler( aHandler ), m_count( aCount )
    {
    }

    int CountTo( int n )
    {
        m_handler( { COROUTINE_TEST_EVENT::TYPE::START, 0 } );

        for( int i = 1; i <= n; i++ )
        {
            m_handler( { COROUTINE_TEST_EVENT::TYPE::YIELD, i } );
            m_cofunc->KiYield( i );
        }

        return 0;
    }

    void Run()
    {
        m_cofunc =
                std::make_unique<TEST_COROUTINE>( this, &COROUTINE_INCREMENTING_HARNESS::CountTo );
        m_handler( { COROUTINE_TEST_EVENT::TYPE::CALL, m_count } );
        m_cofunc->Call( m_count );

        int ret_val = 0;

        while( m_cofunc->Running() )
        {
            ret_val = m_cofunc->ReturnValue();
            m_handler( { COROUTINE_TEST_EVENT::TYPE::RETURNED, ret_val } );
            m_cofunc->Resume();
        }

        m_handler( { COROUTINE_TEST_EVENT::TYPE::END, ret_val } );
    }

    EVT_HANDLER                     m_handler;
    std::unique_ptr<TEST_COROUTINE> m_cofunc;
    int                             m_count;
};

/**
 * Declare the test suite
 */
BOOST_AUTO_TEST_SUITE( Coroutine )


/**
 * A basic test to repeatedly call a coroutine and check that it yields
 * values as expected.
 */
BOOST_AUTO_TEST_CASE( Increment )
{
    const int count = 2;

    const std::vector<COROUTINE_TEST_EVENT> exp_events = {
        { COROUTINE_TEST_EVENT::TYPE::CALL, count },
        { COROUTINE_TEST_EVENT::TYPE::START, 0 },
        { COROUTINE_TEST_EVENT::TYPE::YIELD, 1 },
        { COROUTINE_TEST_EVENT::TYPE::RETURNED, 1 },
        { COROUTINE_TEST_EVENT::TYPE::YIELD, 2 },
        { COROUTINE_TEST_EVENT::TYPE::RETURNED, 2 },
        { COROUTINE_TEST_EVENT::TYPE::END, 2 },
    };

    std::vector<COROUTINE_TEST_EVENT> received_events;

    auto handler = [&]( const COROUTINE_TEST_EVENT& aEvent ) {
        received_events.push_back( aEvent );
    };

    COROUTINE_INCREMENTING_HARNESS harness( handler, count );

    harness.Run();

    BOOST_CHECK_EQUAL_COLLECTIONS(
            received_events.begin(), received_events.end(), exp_events.begin(), exp_events.end() );
}

BOOST_AUTO_TEST_SUITE_END()
