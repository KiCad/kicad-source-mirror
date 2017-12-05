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

#include <widgets/progress_reporter.h>

#include <thread>

PROGRESS_REPORTER::PROGRESS_REPORTER( int aNumPhases ) :
    m_phase( 0 ),
    m_numPhases( aNumPhases ),
    m_progress( 0 ),
    m_maxProgress( 1 )
{
};

void PROGRESS_REPORTER::BeginPhase( int aPhase )
{
    m_phase = aPhase;
    m_progress = 0;
    updateUI();
}

void PROGRESS_REPORTER::AdvancePhase( )
{
    m_phase++;
    m_progress = 0;
    updateUI();
}

void PROGRESS_REPORTER::Report( const wxString& aMessage )
{
    m_rptMessage   = aMessage;
    updateUI();
}

void PROGRESS_REPORTER::SetMaxProgress ( int aMaxProgress )
{
    m_maxProgress = aMaxProgress;
    updateUI();
}

void PROGRESS_REPORTER::AdvanceProgress( )
{
    m_progress++;
}

int PROGRESS_REPORTER::currentProgress() const
{
    double current = (1.0 / (double)m_numPhases) * ( (double) m_phase + ( (double) m_progress.load() / (double) m_maxProgress ) );

    return (int)(current * 1000);
}

WX_PROGRESS_REPORTER::WX_PROGRESS_REPORTER( wxWindow* aParent,
        const wxString& aTitle,
        int aNumPhases ) :
    PROGRESS_REPORTER( aNumPhases ),
    wxProgressDialog( aTitle, wxT( "" ), 1, aParent, wxPD_AUTO_HIDE | wxPD_CAN_ABORT |
            wxPD_APP_MODAL | wxPD_ELAPSED_TIME )
{
}


WX_PROGRESS_REPORTER::~WX_PROGRESS_REPORTER()
{
    Destroy();
}


void WX_PROGRESS_REPORTER::updateUI()
{
    int cur = currentProgress();

    if( cur < 0 || cur > 1000 )
        cur = 0;

    SetRange( 1000 );
    wxProgressDialog::Update( cur, m_rptMessage );
}

void PROGRESS_REPORTER::KeepRefreshing()
{
    while ( m_progress < m_maxProgress && m_maxProgress > 0)
    {
        updateUI();
        #ifdef USE_OPENMP
            wxMilliSleep(10);
        #endif
    }
}
