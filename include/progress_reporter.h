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

#ifndef PROGRESS_REPORTER_H
#define PROGRESS_REPORTER_H

#include <kicommon.h>
#include <wx/wx.h>

/**
 * A progress reporter interface for use in multi-threaded environments.  The various advancement
 * and message methods can be called from sub-threads.  The KeepRefreshing method *MUST* be called
 * only from the main thread (primarily a MSW requirement, which won't allow access to UI objects
 * allocated from a separate thread).
 */
class KICOMMON_API PROGRESS_REPORTER
{
public:

    PROGRESS_REPORTER()
    { }

    PROGRESS_REPORTER( const PROGRESS_REPORTER& ) = delete;

    virtual ~PROGRESS_REPORTER()
    { }

    /**
     * Set the number of phases.
     */
    virtual void SetNumPhases( int aNumPhases ) = 0;
    virtual void AddPhases( int aNumPhases ) = 0;

    /**
     * Initialize the \a aPhase virtual zone of the dialog progress bar.
     */
    virtual void BeginPhase( int aPhase ) = 0;

    /**
     * Use the next available virtual zone of the dialog progress bar.
     */
    virtual void AdvancePhase() = 0;

    /**
     * Use the next available virtual zone of the dialog progress bar and updates the message.
     */
    virtual void AdvancePhase( const wxString& aMessage ) = 0;

    /**
     * Display \a aMessage in the progress bar dialog.
     */
    virtual void Report( const wxString& aMessage ) = 0;

    /**
     * Set the progress value to aProgress (0..1).
     */
    virtual void SetCurrentProgress( double aProgress ) = 0;

    /**
     * Fix the value that gives the 100 percent progress bar length
     * (inside the current virtual zone).
     */
    virtual void SetMaxProgress( int aMaxProgress ) = 0;

    /**
     * Increment the progress bar length (inside the current virtual zone).
     */
    virtual void AdvanceProgress() = 0;

    /**
     * Update the UI (if any).
     *
     * @warning This should  only be called from the main thread.
     *
     * @return false if the user cancelled.
     */
    virtual bool KeepRefreshing( bool aWait = false ) = 0;

    /**
     * Change the title displayed on the window caption.
     *
     * Has meaning only for some reporters.  Does nothing for others.
     *
     * @warning This should only be called from the main thread.
     */
    virtual void SetTitle( const wxString& aTitle ) = 0;

    virtual bool IsCancelled() const = 0;
};


#endif
