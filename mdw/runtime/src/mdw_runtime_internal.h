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

#ifndef MDW_RUNTIME_INTERNAL_H
#define MDW_RUNTIME_INTERNAL_H

#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>

class MDW_EVENT_PUBLISHER;

/// Where the progress of one request goes, and how recently it went
struct MDW_REQUEST_PROGRESS
{
    MDW_EVENT_PUBLISHER*                  publisher = nullptr;
    std::string                           workspaceId;
    std::string                           operationId;
    std::mutex                            mutex;
    std::chrono::steady_clock::time_point last;
    std::string                           lastPhase;
    bool                                  lastFinished = false;
};

/// The progress sink of the request the calling thread serves, or null when it serves none or none can be reported
std::shared_ptr<MDW_REQUEST_PROGRESS> MdwCurrentRequestProgress();

/// While alive, MdwReportProgress() on the thread that made it reports to aProgress
class MDW_REQUEST_PROGRESS_SCOPE
{
public:
    explicit MDW_REQUEST_PROGRESS_SCOPE( std::shared_ptr<MDW_REQUEST_PROGRESS> aProgress );
    ~MDW_REQUEST_PROGRESS_SCOPE();

    MDW_REQUEST_PROGRESS_SCOPE( const MDW_REQUEST_PROGRESS_SCOPE& ) = delete;
    MDW_REQUEST_PROGRESS_SCOPE& operator=( const MDW_REQUEST_PROGRESS_SCOPE& ) = delete;

private:
    std::shared_ptr<MDW_REQUEST_PROGRESS> m_outer;
};

/// While alive, MdwCurrentRequestCancelled() on the thread that made it reads aFlag
class MDW_REQUEST_CANCEL_SCOPE
{
public:
    explicit MDW_REQUEST_CANCEL_SCOPE( std::shared_ptr<const std::atomic<bool>> aFlag );
    ~MDW_REQUEST_CANCEL_SCOPE();

    MDW_REQUEST_CANCEL_SCOPE( const MDW_REQUEST_CANCEL_SCOPE& ) = delete;
    MDW_REQUEST_CANCEL_SCOPE& operator=( const MDW_REQUEST_CANCEL_SCOPE& ) = delete;

private:
    std::shared_ptr<const std::atomic<bool>> m_outer;
};

/// Wait for every fill MdwFillStagedFolder left running when its request was cancelled
void MdwJoinAbandonedFills();

/// Make a rename or removal in aDir survive power loss. Windows has no directory sync, so it reports success
bool MDW_SyncDirectory( const std::filesystem::path& aDir );

#endif // MDW_RUNTIME_INTERNAL_H
