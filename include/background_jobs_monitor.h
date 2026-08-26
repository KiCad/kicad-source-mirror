/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mark Roszko <mark.roszko@gmail.com>
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

#ifndef BACKGROUND_JOBS_MONITOR_H
#define BACKGROUND_JOBS_MONITOR_H

#include <kicommon.h>
#include <widgets/progress_reporter_base.h>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <vector>

class PROGRESS_REPORTER;
class wxString;
class KISTATUSBAR;
struct BACKGROUND_JOB;
class BACKGROUND_JOB_REPORTER;
class BACKGROUND_JOB_LIST;
class BACKGROUND_JOBS_MONITOR;
class wxWindow;
class wxCloseEvent;
class wxWindowDestroyEvent;

class KICOMMON_API BACKGROUND_JOB_REPORTER : public PROGRESS_REPORTER_BASE
{
public:
    BACKGROUND_JOB_REPORTER( BACKGROUND_JOBS_MONITOR*               aMonitor,
                             const std::shared_ptr<BACKGROUND_JOB>& aJob );

    void SetTitle( const wxString& aTitle ) override
    {
    }

    void Report( const wxString& aMessage ) override;

    void Cancel() { m_cancelled.store( true ); }

    void AdvancePhase() override;

    void SetNumPhases( int aNumPhases ) override;

    void SetCurrentProgress( double aProgress ) override;

private:
    bool updateUI() override;

    BACKGROUND_JOBS_MONITOR* m_monitor;
    std::shared_ptr<BACKGROUND_JOB> m_job;
    wxString m_title;
    wxString m_report;
};


struct KICOMMON_API BACKGROUND_JOB
{
public:
    /**
     * Worker threads write the status and the UI reads it so it needs a lock
     */
    wxString GetStatus() const
    {
        std::lock_guard<std::mutex> lock( m_statusMutex );
        return m_status;
    }

    void SetStatus( const wxString& aStatus )
    {
        std::lock_guard<std::mutex> lock( m_statusMutex );
        m_status = aStatus;
    }

    wxString m_name;
    std::shared_ptr<BACKGROUND_JOB_REPORTER> m_reporter;

    std::atomic<int> m_maxProgress;
    std::atomic<int> m_currentProgress;

private:
    mutable std::mutex m_statusMutex;
    wxString           m_status;
};


class KICOMMON_API BACKGROUND_JOBS_MONITOR
{
    friend class BACKGROUND_JOB_REPORTER;
    friend class BACKGROUND_JOB_LIST;

public:
    BACKGROUND_JOBS_MONITOR();

    /**
     * Creates a background job with the given name
     *
     * @param aName is the displayed title for the event
     */
    std::shared_ptr<BACKGROUND_JOB> Create( const wxString& aName );

    /**
     * Removes the given background job from any lists and frees it
     */
    void Remove( std::shared_ptr<BACKGROUND_JOB> job );

    /**
     * Shows the background job list
     */
    void ShowList( wxWindow* aParent, wxPoint aPos );

    /**
     * Add a status bar for handling
     */
    void RegisterStatusBar( KISTATUSBAR* aStatusBar );

    /**
     * Removes status bar from handling
     */
    void UnregisterStatusBar( KISTATUSBAR* aStatusBar );

private:
    /**
     * Handles removing the shown list window from our list of shown windows
     */
    void onListWindowClosed( wxCloseEvent& aEvent );

    /**
     * Handles a list window that its parent destroys without a close event
     */
    void onListWindowDestroyed( wxWindowDestroyEvent& aEvent );

    /**
     * Removes a list window from m_shownDialogs
     *
     * @param aWindow can be partly destroyed so only its address is used
     */
    void removeShownDialog( wxObject* aWindow );

    /**
     * Handles job status updates, intended to be called by BACKGROUND_JOB_REPORTER only
     */
    void jobUpdated( std::shared_ptr<BACKGROUND_JOB> aJob );

    /**
     * Holds a reference to all active background jobs
     * Access to this vector should be protected by locks since threads may Create or Remove at will
     * to register their activity
     */
    std::vector<std::shared_ptr<BACKGROUND_JOB>> m_jobs;
    std::vector<BACKGROUND_JOB_LIST*> m_shownDialogs;

    std::vector<KISTATUSBAR*> m_statusBars;

    /// Mutex to protect access to m_jobs, m_shownDialogs and m_statusBars
    mutable std::shared_mutex m_mutex;
};

#endif
