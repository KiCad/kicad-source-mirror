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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef BACKGROUND_JOBS_MONITOR_H
#define BACKGROUND_JOBS_MONITOR_H

#include <kicommon.h>
#include <widgets/progress_reporter_base.h>
#include <functional>
#include <memory>
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
    wxString m_name;
    wxString m_status;
    std::shared_ptr<BACKGROUND_JOB_REPORTER> m_reporter;

    int m_maxProgress;
    int m_currentProgress;
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

    /// Mutex to protect access to the m_jobs vector
    mutable std::shared_mutex m_mutex;
};

#endif
