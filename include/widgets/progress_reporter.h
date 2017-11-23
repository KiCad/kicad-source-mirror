/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
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

#ifndef __PROGRESS_REPORTER
#define __PROGRESS_REPORTER

#include <mutex>

#include <wx/progdlg.h>

class PROGRESS_REPORTER
{
    public:

        PROGRESS_REPORTER( int aNumPhases );
        PROGRESS_REPORTER( const PROGRESS_REPORTER& ) = delete;

        void BeginPhase( int aPhase );
        void AdvancePhase( );
        void Report ( const wxString& aMessage );
        void SetMaxProgress ( int aMaxProgress );
        void AdvanceProgress( );

    protected:

        int currentProgress() const;
        virtual void updateUI() = 0;

        wxString m_message;
        int m_phase, m_numPhases;
        int m_progress, m_maxProgress;

        std::mutex m_lock;
};

class WX_PROGRESS_REPORTER : public PROGRESS_REPORTER, public wxProgressDialog
{
public:

    WX_PROGRESS_REPORTER( wxWindow *aParent, const wxString &aTitle, int aNumPhases );
    ~WX_PROGRESS_REPORTER();

private:

    virtual void updateUI() override;
};

#endif
