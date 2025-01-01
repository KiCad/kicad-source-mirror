/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef GENERIC_PROGRESS_REPORTER_H
#define GENERIC_PROGRESS_REPORTER_H

#include <mutex>
#include <atomic>
#include <progress_reporter.h>

/**
 * This implements all the tricky bits for thread safety, but the GUI is left to derived classes.
 */
class KICOMMON_API PROGRESS_REPORTER_BASE : public PROGRESS_REPORTER
{
public:

    PROGRESS_REPORTER_BASE( int aNumPhases );
    PROGRESS_REPORTER_BASE( const PROGRESS_REPORTER_BASE& ) = delete;

    virtual ~PROGRESS_REPORTER_BASE()
    {
    }

    /**
     * Set the number of phases.
     */
    void SetNumPhases( int aNumPhases ) override;
    void AddPhases( int aNumPhases ) override;

    /**
     * Initialize the \a aPhase virtual zone of the dialog progress bar.
     */
    virtual void BeginPhase( int aPhase ) override;

    /**
     * Use the next available virtual zone of the dialog progress bar.
     */
    virtual void AdvancePhase() override;

    /**
     * Use the next available virtual zone of the dialog progress bar and updates the message.
     */
    virtual void AdvancePhase( const wxString& aMessage ) override;

    /**
     * Display \a aMessage in the progress bar dialog.
     */
    virtual void Report( const wxString& aMessage ) override;

    /**
     * Set the progress value to aProgress (0..1).
     */
    virtual void SetCurrentProgress( double aProgress ) override;

    /**
     * Fix the value that gives the 100 percent progress bar length
     * (inside the current virtual zone).
     */
    void SetMaxProgress( int aMaxProgress ) override;

    /**
     * Increment the progress bar length (inside the current virtual zone).
     */
    void AdvanceProgress() override;

    /**
     * Update the UI dialog.
     *
     * @warning This should  only be called from the main thread.
     *
     * @return false if the user clicked Cancel.
     */
    bool KeepRefreshing( bool aWait = false ) override;

    /**
     * Change the title displayed on the window caption.
     *
     * Has meaning only for some reporters.  Does nothing for others.
     *
     * @warning This should only be called from the main thread.
     */
    void SetTitle( const wxString& aTitle ) override { }

    bool IsCancelled() const override { return m_cancelled; }

    int CurrentProgress() const;

protected:

    virtual bool updateUI() = 0;

    wxString           m_rptMessage;

    mutable std::mutex m_mutex;
    std::atomic_int    m_phase;
    std::atomic_int    m_numPhases;
    std::atomic_int    m_progress;
    std::atomic_int    m_maxProgress;
    std::atomic_bool   m_cancelled;

    // True if the displayed message has changed,
    // so perhaps there is a need to resize the window
    // Note the resize is made only if the size of the new message
    // is bigger than the old message
    std::atomic_bool   m_messageChanged;
};


#endif
