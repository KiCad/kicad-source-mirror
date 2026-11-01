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
 * with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <memory>
#include <stdexcept>
#include <string>

#include <boost/test/unit_test.hpp>

#include <api/api_handler_common.h>
#include <api/api_task_scheduler.h>
#include <api/common/commands/editor_commands.pb.h>
#include <api/common/envelope.pb.h>
#include <api/common/tasks.pb.h>


namespace
{

/**
 * A task whose behaviour is selected up front, so the scheduler's state machine can be
 * exercised without a board or schematic.
 */
class TEST_TASK : public API_TASK
{
public:
    enum class BEHAVIOR
    {
        COMPLETE,
        REPORT_FAILED,
        REPORT_CANCELLED,
        THROW_STD,
        THROW_UNKNOWN
    };

    explicit TEST_TASK( BEHAVIOR aBehavior ) :
            m_behavior( aBehavior )
    {
    }

    void Run() override
    {
        ++m_runCount;

        switch( m_behavior )
        {
        case BEHAVIOR::COMPLETE:
            break;

        case BEHAVIOR::REPORT_FAILED:
            m_state = kiapi::common::tasks::TS_FAILED;
            m_errorMessage = "reported failure";
            break;

        case BEHAVIOR::REPORT_CANCELLED:
            m_state = kiapi::common::tasks::TS_CANCELLED;
            m_errorMessage = "reported cancellation";
            break;

        case BEHAVIOR::THROW_STD:
            throw std::runtime_error( "boom" );

        case BEHAVIOR::THROW_UNKNOWN:
            throw 42;
        }
    }

    void FillStatus( kiapi::common::tasks::TaskStatusResponse& aStatus ) const override
    {
        if( m_state != kiapi::common::tasks::TS_UNKNOWN )
            aStatus.set_state( m_state );

        if( !m_errorMessage.empty() )
            aStatus.set_error_message( m_errorMessage );
    }

    int RunCount() const { return m_runCount; }

private:
    BEHAVIOR                          m_behavior;
    int                               m_runCount = 0;
    kiapi::common::tasks::TaskState   m_state = kiapi::common::tasks::TS_UNKNOWN;
    std::string                       m_errorMessage;
};


kiapi::common::ApiRequest makeGetTaskStatusRequest( const std::string& aToken )
{
    kiapi::common::tasks::GetTaskStatus command;
    command.mutable_token()->set_value( aToken );

    kiapi::common::ApiRequest request;
    request.mutable_header()->set_client_name( "kicad.qa" );
    BOOST_REQUIRE( request.mutable_message()->PackFrom( command ) );

    return request;
}

} // namespace


BOOST_AUTO_TEST_SUITE( ApiTasks )


BOOST_AUTO_TEST_CASE( StartTaskEnforcesSingleFlight )
{
    API_TASK_SCHEDULER& scheduler = API_TASK_SCHEDULER::Instance();

    // Run anything left over by an earlier test so the first StartTask cannot be rejected
    scheduler.Pump();

    auto first = std::make_shared<TEST_TASK>( TEST_TASK::BEHAVIOR::COMPLETE );
    std::optional<std::string> firstToken = scheduler.StartTask( first, "first" );
    BOOST_REQUIRE( firstToken.has_value() );
    BOOST_CHECK( scheduler.AnyTaskRunning() );

    // The run event has not been pumped, so the first task is still running
    auto second = std::make_shared<TEST_TASK>( TEST_TASK::BEHAVIOR::COMPLETE );
    BOOST_CHECK( !scheduler.StartTask( second, "second" ).has_value() );

    scheduler.Pump();
    BOOST_CHECK( !scheduler.AnyTaskRunning() );
    BOOST_CHECK_EQUAL( second->RunCount(), 0 );

    auto status = scheduler.GetStatus( *firstToken );
    BOOST_REQUIRE( status.has_value() );
    BOOST_CHECK_EQUAL( status->state(), kiapi::common::tasks::TS_COMPLETED );
}


BOOST_AUTO_TEST_CASE( TaskExceptionIsReportedAndDoesNotBlockScheduler )
{
    API_TASK_SCHEDULER& scheduler = API_TASK_SCHEDULER::Instance();
    scheduler.Pump();

    auto throwing = std::make_shared<TEST_TASK>( TEST_TASK::BEHAVIOR::THROW_STD );
    std::optional<std::string> token = scheduler.StartTask( throwing, "throwing" );
    BOOST_REQUIRE( token.has_value() );

    scheduler.Pump();

    auto status = scheduler.GetStatus( *token );
    BOOST_REQUIRE( status.has_value() );
    BOOST_CHECK_EQUAL( status->state(), kiapi::common::tasks::TS_FAILED );
    BOOST_CHECK_EQUAL( status->error_message(), "boom" );
    BOOST_CHECK_EQUAL( throwing->RunCount(), 1 );
    BOOST_CHECK( !scheduler.AnyTaskRunning() );

    // A failed task must not leave the scheduler permanently busy
    auto following = std::make_shared<TEST_TASK>( TEST_TASK::BEHAVIOR::COMPLETE );
    std::optional<std::string> followingToken = scheduler.StartTask( following, "following" );
    BOOST_REQUIRE( followingToken.has_value() );

    scheduler.Pump();

    auto followingStatus = scheduler.GetStatus( *followingToken );
    BOOST_REQUIRE( followingStatus.has_value() );
    BOOST_CHECK_EQUAL( followingStatus->state(), kiapi::common::tasks::TS_COMPLETED );
}


BOOST_AUTO_TEST_CASE( TaskUnknownExceptionIsReportedAsFailure )
{
    API_TASK_SCHEDULER& scheduler = API_TASK_SCHEDULER::Instance();
    scheduler.Pump();

    auto throwing = std::make_shared<TEST_TASK>( TEST_TASK::BEHAVIOR::THROW_UNKNOWN );
    std::optional<std::string> token = scheduler.StartTask( throwing, "throwing" );
    BOOST_REQUIRE( token.has_value() );

    scheduler.Pump();

    auto status = scheduler.GetStatus( *token );
    BOOST_REQUIRE( status.has_value() );
    BOOST_CHECK_EQUAL( status->state(), kiapi::common::tasks::TS_FAILED );
    BOOST_CHECK( !status->error_message().empty() );
}


BOOST_AUTO_TEST_CASE( TaskReportedFailureIsPreserved )
{
    API_TASK_SCHEDULER& scheduler = API_TASK_SCHEDULER::Instance();
    scheduler.Pump();

    auto task = std::make_shared<TEST_TASK>( TEST_TASK::BEHAVIOR::REPORT_FAILED );
    std::optional<std::string> token = scheduler.StartTask( task, "reported" );
    BOOST_REQUIRE( token.has_value() );

    scheduler.Pump();

    auto status = scheduler.GetStatus( *token );
    BOOST_REQUIRE( status.has_value() );
    BOOST_CHECK_EQUAL( status->state(), kiapi::common::tasks::TS_FAILED );
    BOOST_CHECK_EQUAL( status->error_message(), "reported failure" );
}


BOOST_AUTO_TEST_CASE( TaskReportedCancellationIsPreserved )
{
    API_TASK_SCHEDULER& scheduler = API_TASK_SCHEDULER::Instance();
    scheduler.Pump();

    auto task = std::make_shared<TEST_TASK>( TEST_TASK::BEHAVIOR::REPORT_CANCELLED );
    std::optional<std::string> token = scheduler.StartTask( task, "cancelled" );
    BOOST_REQUIRE( token.has_value() );

    scheduler.Pump();

    auto status = scheduler.GetStatus( *token );
    BOOST_REQUIRE( status.has_value() );
    BOOST_CHECK_EQUAL( status->state(), kiapi::common::tasks::TS_CANCELLED );
    BOOST_CHECK_EQUAL( status->error_message(), "reported cancellation" );
}


BOOST_AUTO_TEST_CASE( FinishedTaskRecordsAreEvicted )
{
    API_TASK_SCHEDULER& scheduler = API_TASK_SCHEDULER::Instance();
    scheduler.Pump();

    std::string firstToken;
    std::string lastToken;

    for( size_t i = 0; i < API_TASK_SCHEDULER::MAX_FINISHED_TASKS + 1; ++i )
    {
        auto task = std::make_shared<TEST_TASK>( TEST_TASK::BEHAVIOR::COMPLETE );
        std::optional<std::string> token = scheduler.StartTask( task, "eviction" );
        BOOST_REQUIRE( token.has_value() );

        if( firstToken.empty() )
            firstToken = *token;

        lastToken = *token;
        scheduler.Pump();
    }

    // The oldest record has been evicted; the newest is still retrievable
    BOOST_CHECK( !scheduler.GetStatus( firstToken ).has_value() );
    BOOST_CHECK( scheduler.GetFinishedTask( firstToken ) == nullptr );

    auto lastStatus = scheduler.GetStatus( lastToken );
    BOOST_REQUIRE( lastStatus.has_value() );
    BOOST_CHECK_EQUAL( lastStatus->state(), kiapi::common::tasks::TS_COMPLETED );
    BOOST_CHECK( scheduler.GetFinishedTask( lastToken ) != nullptr );
}


BOOST_AUTO_TEST_CASE( GetTaskStatusHandlerReportsLifecycle )
{
    API_TASK_SCHEDULER& scheduler = API_TASK_SCHEDULER::Instance();
    scheduler.Pump();

    API_HANDLER_COMMON handler;
    BOOST_REQUIRE( handler.CanHandle( std::string( kiapi::common::tasks::GetTaskStatus().GetTypeName() ) ) );

    auto task = std::make_shared<TEST_TASK>( TEST_TASK::BEHAVIOR::COMPLETE );
    std::optional<std::string> token = scheduler.StartTask( task, "status" );
    BOOST_REQUIRE( token.has_value() );

    kiapi::common::ApiRequest runningRequest = makeGetTaskStatusRequest( *token );
    API_RESULT runningResult = handler.Handle( runningRequest );
    BOOST_REQUIRE( runningResult.has_value() );

    kiapi::common::tasks::TaskStatusResponse runningStatus;
    BOOST_REQUIRE( runningResult->message().UnpackTo( &runningStatus ) );
    BOOST_CHECK_EQUAL( runningStatus.state(), kiapi::common::tasks::TS_RUNNING );

    scheduler.Pump();

    kiapi::common::ApiRequest completedRequest = makeGetTaskStatusRequest( *token );
    API_RESULT completedResult = handler.Handle( completedRequest );
    BOOST_REQUIRE( completedResult.has_value() );

    kiapi::common::tasks::TaskStatusResponse completedStatus;
    BOOST_REQUIRE( completedResult->message().UnpackTo( &completedStatus ) );
    BOOST_CHECK_EQUAL( completedStatus.state(), kiapi::common::tasks::TS_COMPLETED );
}


BOOST_AUTO_TEST_CASE( GetTaskStatusHandlerRejectsUnknownToken )
{
    API_HANDLER_COMMON handler;

    kiapi::common::ApiRequest request =
            makeGetTaskStatusRequest( "deadbeef-0000-0000-0000-000000000000" );
    API_RESULT result = handler.Handle( request );

    BOOST_REQUIRE( !result.has_value() );
    BOOST_CHECK_EQUAL( result.error().status(), kiapi::common::ApiStatusCode::AS_BAD_REQUEST );
}


BOOST_AUTO_TEST_SUITE_END()
