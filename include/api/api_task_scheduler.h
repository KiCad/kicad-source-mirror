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

#ifndef KICAD_API_TASK_SCHEDULER_H
#define KICAD_API_TASK_SCHEDULER_H

#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

#include <wx/event.h>

#include <kicommon.h>

namespace google::protobuf
{
class Any;
}

namespace kiapi::common::tasks
{
class TaskStatusResponse;
}

class KICOMMON_API API_TASK
{
public:
    virtual ~API_TASK() = default;

    /// The task runner; called on the main thread
    virtual void Run() = 0;

    /// Called after Run() returns; should fill in the result of the run
    virtual void FillStatus( kiapi::common::tasks::TaskStatusResponse& aStatus ) const = 0;
};

/**
 * For now, a single process-wide task runner, and one task at a time.
 */
class KICOMMON_API API_TASK_SCHEDULER : public wxEvtHandler
{
public:
    static constexpr size_t MAX_FINISHED_TASKS = 32;

    static API_TASK_SCHEDULER& Instance();

    ~API_TASK_SCHEDULER() override;

    API_TASK_SCHEDULER( const API_TASK_SCHEDULER& ) = delete;
    API_TASK_SCHEDULER& operator=( const API_TASK_SCHEDULER& ) = delete;

    /**
     * Register a new task and begin running it.
     * @param aTask is the task to run
     * @param aDescription is used for trace logging
     * @return the new task's token, or nullopt if another task is already running
     */
    std::optional<std::string> StartTask( const std::shared_ptr<API_TASK>& aTask, const wxString& aDescription );

    /**
     * Get a snapshot of the task's current status.
     * @return nullopt if the token is unknown (expired or never existed)
     */
    std::optional<kiapi::common::tasks::TaskStatusResponse> GetStatus( const std::string& aToken ) const;

    /**
     * Look up a task that has finished, for result retrieval.
     * The most recent MAX_FINISHED_TASKS results are available.
     */
    std::shared_ptr<API_TASK> GetFinishedTask( const std::string& aToken ) const;

    /// @return true if a task is currently running
    bool AnyTaskRunning() const;

    /**
     * Run the current task to completion, synchronously.  The scheduler normally pumps
     * itself via QueueEvent; hosts without a running event loop (unit tests) use this.
     */
    void Pump();

private:
    struct TASK_ENTRY
    {
        std::string               token;
        wxString                  description;
        std::shared_ptr<API_TASK> task;
        bool                      finished = false;
        bool                      failed = false;
        std::string               failureMessage;
    };

    API_TASK_SCHEDULER();

    void onTaskRunEvent( wxCommandEvent& aEvent );
    void runTask( const std::string& aToken );

    mutable std::mutex m_mutex;
    std::map<std::string, std::shared_ptr<TASK_ENTRY>> m_tasks;
    std::deque<std::string> m_finishedTokens;
};

#endif //KICAD_API_TASK_SCHEDULER_H
