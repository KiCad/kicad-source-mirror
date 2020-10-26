/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Andrew Lutsenko, anlutsenko at gmail dot com
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
        DIALOG_PCM_PROGRESS_BASE( parent )
#if wxCHECK_VERSION( 3, 1, 0 )
        ,
        m_appProgressIndicator( parent->GetParent(), GAUGE_RANGE )
#endif
{
#if wxCHECK_VERSION( 3, 1, 0 )
    m_appProgressIndicator.Pulse();
#endif
    m_cancelled.store( false );

    m_reporter->SetImmediateMode();
    m_downloadGauge->SetRange( GAUGE_RANGE );
    m_overallGauge->SetRange( GAUGE_RANGE );

    m_overallPhases = 1;

    if( !aShowDownloadSection )
        m_panelDownload->Hide();
}


void DIALOG_PCM_PROGRESS::OnCancelClicked( wxCommandEvent& event )
{
    m_cancelled.store( true );
    m_buttonCancel->Disable();
}


void DIALOG_PCM_PROGRESS::OnCloseClicked( wxCommandEvent& event )
{
    EndModal( wxID_OK );
}


void DIALOG_PCM_PROGRESS::Report( const wxString& aText, SEVERITY aSeverity )
{
    CallAfter(
            [=]
            {
                m_reporter->Report( aText, aSeverity );
            } );
}


void DIALOG_PCM_PROGRESS::SetDownloadProgress( uint64_t aDownloaded, uint64_t aTotal )
{
    if( aDownloaded > aTotal )
        aDownloaded = aTotal;

    int value = 0;

    if( aTotal > 0 )
        value = aDownloaded * GAUGE_RANGE / aTotal;

    CallAfter(
            [=]
            {
                m_downloadText->SetLabel( wxString::Format( _( "Downloaded %lld/%lld Kb" ),
                                                            toKb( aDownloaded ), toKb( aTotal ) ) );

                m_downloadGauge->SetValue( value );
            } );
}


uint64_t DIALOG_PCM_PROGRESS::toKb( uint64_t aValue )
{
    return ( aValue + 1023 ) / 1024;
}


void DIALOG_PCM_PROGRESS::SetOverallProgress( uint64_t aProgress, uint64_t aTotal )
{
    double current = ( m_currentPhase + aProgress / (double) aTotal ) / m_overallPhases;

    if( current > 1.0 )
        current = 1.0;

    int value = current * GAUGE_RANGE;

    CallAfter(
            [=]
            {
                m_overallGauge->SetValue( value );
#if wxCHECK_VERSION( 3, 1, 0 )
                m_appProgressIndicator.SetValue( value );
#endif
            } );
}


void DIALOG_PCM_PROGRESS::SetOverallProgressPhases( int aPhases )
{
    m_currentPhase = 0;
    m_overallPhases = aPhases;
}


void DIALOG_PCM_PROGRESS::AdvanceOverallProgressPhase()
{
    m_currentPhase++;
    SetOverallProgress( 0, 1 );
}


void DIALOG_PCM_PROGRESS::SetFinished()
{
    CallAfter(
            [this]
            {
                m_buttonCancel->Disable();
                m_buttonClose->Enable();
            } );
}


void DIALOG_PCM_PROGRESS::SetDownloadsFinished()
{
    CallAfter(
            [this]
            {
                m_downloadText->SetLabel( _( "All downloads finished" ) );
            } );
}

bool DIALOG_PCM_PROGRESS::IsCancelled()
{
    return m_cancelled.load();
}
