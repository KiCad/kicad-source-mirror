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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <api/api_task_scheduler.h>

#include <wx/log.h>

#include <api/api_utils.h>
#include <api/common/tasks.pb.h>
#include <kiid.h>

#include <exception>
#include <optional>


wxDEFINE_EVENT( API_TASK_RUN_EVENT, wxCommandEvent );


static wxCommandEvent* makeRunEvent( const std::string& aToken )
{
    wxCommandEvent* evt = new wxCommandEvent( API_TASK_RUN_EVENT );
    evt->SetClientData( new std::string( aToken ) );
    return evt;
}


API_TASK_SCHEDULER& API_TASK_SCHEDULER::Instance()
{
    static API_TASK_SCHEDULER s_instance;
    return s_instance;
}


API_TASK_SCHEDULER::API_TASK_SCHEDULER()
{
    Bind( API_TASK_RUN_EVENT, &API_TASK_SCHEDULER::onTaskRunEvent, this );
}


API_TASK_SCHEDULER::~API_TASK_SCHEDULER()
{
    Unbind( API_TASK_RUN_EVENT, &API_TASK_SCHEDULER::onTaskRunEvent, this );
}


std::optional<std::string> API_TASK_SCHEDULER::StartTask( const std::shared_ptr<API_TASK>& aTask,
                                                          const wxString& aDescription )
{
    std::string token = KIID().AsStdString();
    auto entry = std::make_shared<TASK_ENTRY>();
    entry->token = token;
    entry->description = aDescription;
    entry->task = aTask;

    {
        std::lock_guard<std::mutex> lock( m_mutex );

        for( const auto& [existingToken, existingEntry] : m_tasks )
        {
            if( !existingEntry->finished )
                return std::nullopt;
        }

        m_tasks[ token ] = entry;
    }

    wxLogTrace( traceApi, wxString::Format( wxS( "TaskScheduler: started task %s (%s)" ), token, aDescription ) );

    QueueEvent( makeRunEvent( token ) );

    return token;
}


std::optional<kiapi::common::tasks::TaskStatusResponse> API_TASK_SCHEDULER::GetStatus( const std::string& aToken ) const
{
    std::shared_ptr<TASK_ENTRY> entry;
    bool                        finished = false;
    bool                        failed = false;
    std::string                 failureMessage;

    {
        std::lock_guard<std::mutex> lock( m_mutex );

        auto it = m_tasks.find( aToken );

        if( it == m_tasks.end() )
            return std::nullopt;

        entry = it->second;
        finished = entry->finished;
        failed = entry->failed;
        failureMessage = entry->failureMessage;
    }

    kiapi::common::tasks::TaskStatusResponse status;

    if( !finished )
    {
        status.set_state( kiapi::common::tasks::TS_RUNNING );
    }
    else if( failed )
    {
        status.set_state( kiapi::common::tasks::TS_FAILED );
        status.set_error_message( failureMessage );
    }
    else
    {
        entry->task->FillStatus( status );

        // In case the task itself didn't update the state, we assume it's completed
        if( status.state() == kiapi::common::tasks::TS_UNKNOWN )
            status.set_state( kiapi::common::tasks::TS_COMPLETED );
    }

    return status;
}


std::shared_ptr<API_TASK> API_TASK_SCHEDULER::GetFinishedTask( const std::string& aToken ) const
{
    std::lock_guard<std::mutex> lock( m_mutex );

    auto it = m_tasks.find( aToken );

    if( it == m_tasks.end() || !it->second->finished )
        return nullptr;

    return it->second->task;
}


bool API_TASK_SCHEDULER::AnyTaskRunning() const
{
    std::lock_guard<std::mutex> lock( m_mutex );

    for( const auto& [token, entry] : m_tasks )
    {
        if( !entry->finished )
            return true;
    }

    return false;
}


void API_TASK_SCHEDULER::onTaskRunEvent( wxCommandEvent& aEvent )
{
    std::unique_ptr<std::string> token( static_cast<std::string*>( aEvent.GetClientData() ) );

    if( token )
        runTask( *token );
}


void API_TASK_SCHEDULER::runTask( const std::string& aToken )
{
    std::shared_ptr<TASK_ENTRY> entry;

    {
        std::lock_guard<std::mutex> lock( m_mutex );

        auto it = m_tasks.find( aToken );

        if( it == m_tasks.end() || it->second->finished )
            return;

        entry = it->second;
    }

    bool        failed = false;
    std::string failureMessage;

    try
    {
        entry->task->Run();
    }
    catch( const std::exception& aError )
    {
        failed = true;
        failureMessage = aError.what();

        wxLogTrace( traceApi, wxString::Format( wxS( "TaskScheduler: task %s failed: %s" ),
                                                wxString::FromUTF8( entry->token ),
                                                wxString::FromUTF8( aError.what() ) ) );
    }
    catch( ... )
    {
        failed = true;
        failureMessage = "task failed with an unknown error";

        wxLogTrace( traceApi, wxString::Format( wxS( "TaskScheduler: task %s failed" ),
                                                wxString::FromUTF8( entry->token ) ) );
    }

    {
        std::lock_guard<std::mutex> lock( m_mutex );

        if( entry->finished )
            return;

        entry->failed = failed;
        entry->failureMessage = failureMessage;
        entry->finished = true;

        m_finishedTokens.push_back( entry->token );

        while( m_finishedTokens.size() > MAX_FINISHED_TASKS )
        {
            const std::string& oldest = m_finishedTokens.front();
            auto oldestIt = m_tasks.find( oldest );

            if( oldestIt != m_tasks.end() && oldestIt->second->finished )
                m_tasks.erase( oldestIt );

            m_finishedTokens.pop_front();
        }
    }
}


void API_TASK_SCHEDULER::Pump()
{
    std::vector<std::string> pending;

    {
        std::lock_guard<std::mutex> lock( m_mutex );

        for( const auto& [token, entry] : m_tasks )
        {
            if( !entry->finished )
                pending.push_back( token );
        }
    }

    for( const std::string& token : pending )
        runTask( token );
}
