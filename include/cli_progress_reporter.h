/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef CLI_PROGRESS_REPORTER_H
#define CLI_PROGRESS_REPORTER_H

#include <wx/string.h>
#include <progress_reporter.h>
#include <kicommon.h>

/**
 * Reporter forwarding messages to stdout or stderr as appropriate
 */
class KICOMMON_API CLI_PROGRESS_REPORTER : public PROGRESS_REPORTER
{
public:
    CLI_PROGRESS_REPORTER() {}

    virtual ~CLI_PROGRESS_REPORTER() {}

    static PROGRESS_REPORTER& GetInstance();

    /**
     * Set the number of phases.
     */
    virtual void SetNumPhases( int aNumPhases ) override {}
    virtual void AddPhases( int aNumPhases ) override {};

    /**
     * Initialize the \a aPhase virtual zone of the dialog progress bar.
     */
    virtual void BeginPhase( int aPhase ) override {}

    /**
     * Use the next available virtual zone of the dialog progress bar.
     */
    virtual void AdvancePhase() override {}

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
    virtual void SetCurrentProgress( double aProgress ) override {}

    /**
     * Fix the value that gives the 100 percent progress bar length
     * (inside the current virtual zone).
     */
    virtual void SetMaxProgress( int aMaxProgress ) override {}

    /**
     * Increment the progress bar length (inside the current virtual zone).
     */
    virtual void AdvanceProgress() override {}

    /**
     * Update the UI (if any).
     *
     * @warning This should  only be called from the main thread.
     *
     * @return false if the user cancelled.
     */
    virtual bool KeepRefreshing( bool aWait = false ) override { return false; }

    /**
     * Change the title displayed on the window caption.
     *
     * Has meaning only for some reporters.  Does nothing for others.
     *
     * @warning This should only be called from the main thread.
     */
    virtual void SetTitle( const wxString& aTitle ) override {}

    virtual bool IsCancelled() const override { return false; }


private:
    void printLine( const wxString& aMessage );
};

#endif