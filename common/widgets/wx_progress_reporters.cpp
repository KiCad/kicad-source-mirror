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

#include <wx/evtloop.h>
#include <thread>
#include <widgets/wx_progress_reporters.h>


WX_PROGRESS_REPORTER::WX_PROGRESS_REPORTER( wxWindow* aParent, const wxString& aTitle,
                                            int aNumPhases, int aCanAbort,
                                            bool aReserveSpaceForMessage ) :
        PROGRESS_REPORTER_BASE( aNumPhases ),
        WX_PROGRESS_REPORTER_BASE( aTitle,
                          ( aReserveSpaceForMessage ? wxT( " " ) : wxT( "" ) ),
                          1, aParent,
                          // wxPD_APP_MODAL |   // Don't use; messes up OSX when called from
                                                // quasi-modal dialog
                          wxPD_AUTO_HIDE |      // *MUST* use; otherwise wxWidgets will spin
                                                // up another event loop on completion which
                                                // causes all sorts of grief
                          aCanAbort | wxPD_ELAPSED_TIME ),
        m_appProgressIndicator( aParent ),
        m_messageWidth( 0 )
{
    // wxAppProgressIndicator doesn't like value > max, ever. However there are some risks
    // with multithreaded setting of those values making a mess
    // the cop out is just to set the progress to "indeterminate"
    m_appProgressIndicator.Pulse();
}


WX_PROGRESS_REPORTER::~WX_PROGRESS_REPORTER()
{
}


bool WX_PROGRESS_REPORTER::updateUI()
{
    int cur = CurrentProgress();

    if( cur < 0 || cur > 1000 )
        cur = 0;

    SetRange( 1000 );

    wxString message;

    {
        std::lock_guard<std::mutex> guard( m_mutex );
        message = m_rptMessage;
    }

    // Perhaps the window size is too small if the new message to display is bigger
    // than the previous message. in this case, resize the WX_PROGRESS_REPORTER window
    // GetTextExtent has probably bugs in wxWidgets < 3.1.6, so calling it only when
    // the message has changed is mandatory
    if( m_messageChanged )
    {
        int  newWidth = GetTextExtent( m_rptMessage ).x;

        if( newWidth > m_messageWidth )
        {
            m_messageWidth = newWidth;
            Fit();
        }

        Raise();

        m_messageChanged = false;
    }

    // Returns false when cancelled (if it's a cancellable dialog)
    bool diag = WX_PROGRESS_REPORTER_BASE::Update( cur, message );

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
    wxEventLoopBase::GetActive()->YieldFor( wxEVT_CATEGORY_UI );

    return true;  // No cancel button on a wxGauge
}
