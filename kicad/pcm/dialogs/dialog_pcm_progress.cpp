/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Andrew Lutsenko, anlutsenko at gmail dot com
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "dialog_pcm_progress.h"
#include <wx/msgdlg.h>


#define GAUGE_RANGE 1000

DIALOG_PCM_PROGRESS::DIALOG_PCM_PROGRESS( wxWindow* parent, bool aShowDownloadSection ) :
        DIALOG_PCM_PROGRESS_BASE( parent ),
        PROGRESS_REPORTER_BASE( 1 ),
        m_downloaded( 0 ),
        m_downloadTotal( 0 ),
        m_currentProgress( 0 ),
        m_currentProgressTotal( 0 ),
        m_finished( false ),
        m_disabler( this ),
        m_appProgressIndicator( parent->GetParent(), GAUGE_RANGE )
{
    m_appProgressIndicator.Pulse();

    m_reporter->SetImmediateMode();
    m_downloadGauge->SetRange( GAUGE_RANGE );
    m_overallGauge->SetRange( GAUGE_RANGE );

    if( !aShowDownloadSection )
        m_panelDownload->Hide();
}


void DIALOG_PCM_PROGRESS::OnCancelClicked( wxCommandEvent& event )
{
    SetNumPhases( 1 );
    SetPackageProgress( 1, 1 );
    m_reporter->Report( _( "Aborting remaining tasks." ) );

    m_cancelled.store( true );
    m_finished.store( true );
}


void DIALOG_PCM_PROGRESS::OnCloseClicked( wxCommandEvent& event )
{
    m_progress.store( m_maxProgress );
}


void DIALOG_PCM_PROGRESS::PCMReport( const wxString& aText, SEVERITY aSeverity )
{
    std::lock_guard<std::mutex> guard( m_mutex );
    m_reports.push_back( std::make_pair( aText, aSeverity ) );
}


void DIALOG_PCM_PROGRESS::SetDownloadProgress( uint64_t aDownloaded, uint64_t aTotal )
{
    m_downloaded.store( std::min( aDownloaded, aTotal ) );
    m_downloadTotal.store( aTotal );
}


uint64_t DIALOG_PCM_PROGRESS::toKb( uint64_t aValue )
{
    return ( aValue + 999 ) / 1000;
}


void DIALOG_PCM_PROGRESS::SetPackageProgress( uint64_t aProgress, uint64_t aTotal )
{
    m_currentProgress.store( std::min( aProgress, aTotal ) );
    m_currentProgressTotal.store( aTotal );
}


void DIALOG_PCM_PROGRESS::AdvancePhase()
{
    PROGRESS_REPORTER_BASE::AdvancePhase();
    m_currentProgress.store( 0 );
}


void DIALOG_PCM_PROGRESS::SetFinished()
{
    m_finished.store( true );
}


bool DIALOG_PCM_PROGRESS::updateUI()
{
    bool   finished = m_finished.load();
    int    phase    = m_phase.load();
    int    phases   = m_numPhases.load();
    long   cp       = m_currentProgress.load();
    long   total    = m_currentProgressTotal.load();
    double current  = ( total > 0 ) ? ( double( cp ) / double( total ) ) : 0;

    if( phases > 0 )
        current = ( phase + current ) / phases;

    if( current > 1.0 || finished )
        current = 1.0;

    m_overallGauge->SetValue( current * GAUGE_RANGE );
    m_appProgressIndicator.SetValue( current * GAUGE_RANGE );

    if( m_downloadTotal.load() == 0 )
    {
        m_downloadText->SetLabel( wxEmptyString );
        m_downloadGauge->SetValue( 0 );
    }
    else
    {
        m_downloadText->SetLabel( wxString::Format( _( "Downloaded %lld/%lld kB" ),
                                                    toKb( m_downloaded.load() ),
                                                    toKb( m_downloadTotal.load() ) ) );

        current = m_downloaded.load() / (double) m_downloadTotal.load();

        if( current > 1.0 || finished )
            current = 1.0;

        m_downloadGauge->SetValue( current * GAUGE_RANGE );
    }

    std::lock_guard<std::mutex> guard( m_mutex );

    for( const std::pair<wxString, SEVERITY>& pair : m_reports )
        m_reporter->Report( pair.first, pair.second );

    m_reports.clear();

    if( finished )
    {
        m_buttonCancel->Disable();
        m_buttonClose->Enable();
    }

    wxSafeYield();

    return true;
}


