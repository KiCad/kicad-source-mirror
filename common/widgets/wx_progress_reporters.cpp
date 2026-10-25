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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <thread>
#include <widgets/wx_event_utils.h>
#include <widgets/wx_progress_reporters.h>
#include <wx/dcclient.h>


static wxString getPlaceholderText()
{
    const int c_numChars = 80;

    return wxString( L'\u2002', c_numChars ); // EN SPACE
}


WX_PROGRESS_REPORTER::WX_PROGRESS_REPORTER( wxWindow* aParent, const wxString& aTitle, int aNumPhases, int aCanAbort,
                                            bool aReserveSpaceForMessage ) :
        PROGRESS_REPORTER_BASE( aNumPhases ),
        WX_PROGRESS_REPORTER_BASE( aTitle, aReserveSpaceForMessage ? getPlaceholderText() : wxString( wxEmptyString ),
                                   1, aParent,
                                   // wxPD_APP_MODAL |   // Don't use; messes up OSX when called from
                                   // quasi-modal dialog
                                   wxPD_AUTO_HIDE | // *MUST* use; otherwise wxWidgets will spin
                                                    // up another event loop on completion which
                                                    // causes all sorts of grief
                                           aCanAbort | wxPD_ELAPSED_TIME ),
        m_appProgressIndicator( aParent ),
        m_reservedSize( aReserveSpaceForMessage ),
        m_updateThrottle( std::chrono::milliseconds( 100 ) ),
        m_lastUpdateResult( true ),
        m_indeterminate( false ),
        m_shownPhase( -1 )
{
    // wxAppProgressIndicator doesn't like value > max, ever. However there are some risks
    // with multithreaded setting of those values making a mess
    // the cop out is just to set the progress to "indeterminate"
    m_appProgressIndicator.Pulse();
}


WX_PROGRESS_REPORTER::~WX_PROGRESS_REPORTER()
{
}


void WX_PROGRESS_REPORTER::PinMessage( const wxString& aMessage )
{
    std::lock_guard<std::mutex> guard( m_mutex );

    m_messageChanged = m_rptMessage != aMessage;
    m_rptMessage = aMessage;
    m_messagePinned = true;
}


void WX_PROGRESS_REPORTER::UnpinMessage()
{
    std::lock_guard<std::mutex> guard( m_mutex );

    m_messagePinned = false;
}


void WX_PROGRESS_REPORTER::Report( const wxString& aMessage )
{
    // Checked and written under one lock, so a worker's report cannot land after a pin
    std::lock_guard<std::mutex> guard( m_mutex );

    if( m_messagePinned )
        return;

    m_messageChanged = m_rptMessage != aMessage;
    m_rptMessage = aMessage;
}


bool WX_PROGRESS_REPORTER::updateUI()
{
    const int phase = m_phase.load();

    if( !shouldRefresh( m_progress.load(), m_maxProgress.load(), phase, m_numPhases.load(), m_shownPhase.load(),
                        m_refreshOnPhaseChange, m_updateThrottle ) )
    {
        return !m_cancelled.load() && m_lastUpdateResult.load();
    }

    m_shownPhase.store( phase );

    int cur = CurrentProgress();

    if( cur < 0 || cur > 1000 )
        cur = 0;

    SetRange( 1000 );

    wxString message; // Empty message means unchanged
    {
        std::lock_guard<std::mutex> guard( m_mutex );
        bool                        messageChanged = m_messageChanged.exchange( false );

        if( messageChanged )
            message = m_rptMessage;
    }

    // Remove trailing newline
    if( !message.empty() && message.Last() == '\n' )
        message.RemoveLast();

    if( m_reservedSize && !message.empty() )
    {
        wxSize maxSize = GetTextExtent( getPlaceholderText() );

        wxClientDC dc( this );
        message = wxControl::Ellipsize( message, dc, wxELLIPSIZE_MIDDLE, maxSize.x );
    }

    // Allowing interaction with other windows has unintended consequences
    wxWindowDisabler ed( this );

    // Past the declared phases the gauge maximum would finish the dialog early and hide its Cancel button
    bool pulse = m_indeterminate.load() || phase >= m_numPhases.load();

    // Returns false when cancelled (if it's a cancellable dialog). A phase whose total is not
    // known yet pulses instead, so the gauge shows that work is still going on.
    bool diag = pulse ? WX_PROGRESS_REPORTER_BASE::Pulse( message )
                      : WX_PROGRESS_REPORTER_BASE::Update( cur, message );

    if( !message.empty() )
        Fit();

    DrainPendingEvents( drainCategories() );

    m_lastUpdateResult.store( diag );

    // Time the interval from the end of the refresh so a slow display cannot starve the work
    m_updateThrottle.Restart();

    return diag;
}


GAUGE_PROGRESS_REPORTER::GAUGE_PROGRESS_REPORTER( wxWindow* aParent, int aNumPhases ) :
        PROGRESS_REPORTER_BASE( aNumPhases ),
        wxGauge( aParent, wxID_ANY, 1000, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL,
                 wxDefaultValidator, wxGaugeNameStr )
{
}


bool GAUGE_PROGRESS_REPORTER::updateUI()
{
    int cur = CurrentProgress();

    if( cur < 0 || cur > 1000 )
        cur = 0;

    wxGauge::SetValue( cur );

    DrainPendingEvents( wxEVT_CATEGORY_UI );

    return true;  // No cancel button on a wxGauge
}
