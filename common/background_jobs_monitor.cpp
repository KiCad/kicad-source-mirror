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

#include <unordered_map>

#include <wx/gauge.h>
#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/settings.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/string.h>

#include <background_jobs_monitor.h>
#include <widgets/kistatusbar.h>


class BACKGROUND_JOB_PANEL : public wxPanel
{
public:
    BACKGROUND_JOB_PANEL( wxWindow* aParent, std::shared_ptr<BACKGROUND_JOB> aJob ) :
            wxPanel( aParent, wxID_ANY, wxDefaultPosition, wxSize( -1, 75 ),
                     wxBORDER_SIMPLE ),
            m_job( aJob )
    {
        SetSizeHints( wxDefaultSize, wxDefaultSize );

        wxBoxSizer* mainSizer;
        mainSizer = new wxBoxSizer( wxVERTICAL );

        SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_3DLIGHT ) );

        m_stName = new wxStaticText( this, wxID_ANY, aJob->m_name );
        m_stName->Wrap( -1 );
        m_stName->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT,
                                   wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false,
                                   wxEmptyString ) );
        mainSizer->Add( m_stName, 0, wxALL | wxEXPAND, 1 );

        m_stStatus = new wxStaticText( this, wxID_ANY, aJob->m_status, wxDefaultPosition,
                                       wxDefaultSize, 0 );
        m_stStatus->Wrap( -1 );
        mainSizer->Add( m_stStatus, 0, wxALL | wxEXPAND, 1 );

        m_progress = new wxGauge( this, wxID_ANY, aJob->m_maxProgress, wxDefaultPosition,
                                  wxDefaultSize, wxGA_HORIZONTAL );
        m_progress->SetValue( 0 );
        mainSizer->Add( m_progress, 0, wxALL | wxEXPAND, 1 );

        SetSizer( mainSizer );
        Layout();

        UpdateFromJob();
    }


    void UpdateFromJob()
    {
        m_stStatus->SetLabelText( m_job->m_status );
        m_progress->SetValue( m_job->m_currentProgress );
        m_progress->SetRange( m_job->m_maxProgress );
    }

private:
    wxGauge* m_progress;
    wxStaticText* m_stName;
    wxStaticText* m_stStatus;
    std::shared_ptr<BACKGROUND_JOB> m_job;
};


class BACKGROUND_JOB_LIST : public wxFrame
{
public:
    BACKGROUND_JOB_LIST( wxWindow* parent, const wxPoint& pos ) :
            wxFrame( parent, wxID_ANY, _( "Background Jobs" ), pos, wxSize( 300, 150 ),
                     wxFRAME_NO_TASKBAR | wxBORDER_SIMPLE )
    {
        SetSizeHints( wxDefaultSize, wxDefaultSize );

	    wxBoxSizer* bSizer1;
        bSizer1 = new wxBoxSizer( wxVERTICAL );

        m_scrolledWindow = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition,
                                                 wxSize( -1, -1 ), wxVSCROLL );
        m_scrolledWindow->SetScrollRate( 5, 5 );
        m_contentSizer = new wxBoxSizer( wxVERTICAL );

        m_scrolledWindow->SetSizer( m_contentSizer );
        m_scrolledWindow->Layout();
        m_contentSizer->Fit( m_scrolledWindow );
        bSizer1->Add( m_scrolledWindow, 1, wxEXPAND | wxALL, 0 );

        Bind( wxEVT_KILL_FOCUS, &BACKGROUND_JOB_LIST::onFocusLoss, this );

	    SetSizer( bSizer1 );
        Layout();

        SetFocus();
    }

    void onFocusLoss( wxFocusEvent& aEvent )
    {
        Close( true );
        aEvent.Skip();
    }


    void Add( std::shared_ptr<BACKGROUND_JOB> aJob )
    {
        BACKGROUND_JOB_PANEL* panel = new BACKGROUND_JOB_PANEL( m_scrolledWindow, aJob );
        m_contentSizer->Add( panel, 0, wxEXPAND | wxALL, 2 );
        m_scrolledWindow->Layout();
        m_contentSizer->Fit( m_scrolledWindow );

        // call this at this window otherwise the child panels don't resize width properly
        Layout();

        m_jobPanels[aJob] = panel;
    }


    void Remove( std::shared_ptr<BACKGROUND_JOB> aJob )
    {
        auto it = m_jobPanels.find( aJob );
        if( it != m_jobPanels.end() )
        {
            BACKGROUND_JOB_PANEL* panel = m_jobPanels[aJob];
            m_contentSizer->Detach( panel );
            panel->Destroy();

            m_jobPanels.erase( it );
        }
    }

    void UpdateJob( std::shared_ptr<BACKGROUND_JOB> aJob )
    {
        auto it = m_jobPanels.find( aJob );
        if( it != m_jobPanels.end() )
        {
            BACKGROUND_JOB_PANEL* panel = m_jobPanels[aJob];
            panel->UpdateFromJob();
        }
    }

private:
    wxScrolledWindow* m_scrolledWindow;
    wxBoxSizer*       m_contentSizer;
    std::unordered_map<std::shared_ptr<BACKGROUND_JOB>, BACKGROUND_JOB_PANEL*> m_jobPanels;
};


BACKGROUND_JOB_REPORTER::BACKGROUND_JOB_REPORTER( BACKGROUND_JOBS_MONITOR* aMonitor,
                                                  const std::shared_ptr<BACKGROUND_JOB>& aJob ) :
        PROGRESS_REPORTER_BASE( 1 ),
        m_monitor( aMonitor ),
        m_job( aJob )
{

}


bool BACKGROUND_JOB_REPORTER::updateUI()
{
    return !m_cancelled;
}


void BACKGROUND_JOB_REPORTER::Report( const wxString& aMessage )
{
    m_job->m_status = aMessage;
    m_monitor->jobUpdated( m_job );
}


void BACKGROUND_JOB_REPORTER::SetNumPhases( int aNumPhases )
{
    PROGRESS_REPORTER_BASE::SetNumPhases( aNumPhases );
    m_job->m_maxProgress = m_numPhases;
    m_monitor->jobUpdated( m_job );
}


void BACKGROUND_JOB_REPORTER::AdvancePhase()
{
    PROGRESS_REPORTER_BASE::AdvancePhase();
    m_job->m_currentProgress = m_phase;
    m_monitor->jobUpdated( m_job );
}


void BACKGROUND_JOB_REPORTER::SetCurrentProgress( double aProgress )
{
    PROGRESS_REPORTER_BASE::SetCurrentProgress( aProgress );
    m_job->m_maxProgress = 1000;
    m_job->m_currentProgress = ( 1000 * aProgress );
    m_monitor->jobUpdated( m_job );
}


BACKGROUND_JOBS_MONITOR::BACKGROUND_JOBS_MONITOR()
{

}


std::shared_ptr<BACKGROUND_JOB> BACKGROUND_JOBS_MONITOR::Create( const wxString& aName )
{
    std::shared_ptr<BACKGROUND_JOB> job = std::make_shared<BACKGROUND_JOB>();

    job->m_name = aName;
    job->m_reporter = std::make_shared<BACKGROUND_JOB_REPORTER>( this, job );

    std::lock_guard<std::shared_mutex> lock( m_mutex );
    m_jobs.push_back( job );

    if( m_shownDialogs.size() > 0 )
    {
        // update dialogs
        for( BACKGROUND_JOB_LIST* list : m_shownDialogs )
        {
            list->CallAfter(
                    [=]()
                    {
                        list->Add( job );
                    } );
        }
    }

    return job;
}


void BACKGROUND_JOBS_MONITOR::Remove( std::shared_ptr<BACKGROUND_JOB> aJob )
{
    if( m_shownDialogs.size() > 0 )
    {
        // update dialogs

        for( BACKGROUND_JOB_LIST* list : m_shownDialogs )
        {
            list->CallAfter(
                    [=]()
                    {
                        list->Remove( aJob );
                    } );
        }
    }

    std::lock_guard<std::shared_mutex> lock( m_mutex );
    m_jobs.erase( std::remove_if( m_jobs.begin(), m_jobs.end(),
                                  [&]( std::shared_ptr<BACKGROUND_JOB> job )
                                  {
                                      return job == aJob;
                                  } ) );

    if( m_jobs.size() > 0 )
    {
        jobUpdated( m_jobs.front() );
    }
    else
    {
        for( KISTATUSBAR* statusBar : m_statusBars )
        {
            statusBar->CallAfter(
                    [=]()
                    {
                        statusBar->HideBackgroundProgressBar();
                        statusBar->SetBackgroundStatusText( wxT( "" ) );
                    } );
        }
    }
}


void BACKGROUND_JOBS_MONITOR::onListWindowClosed( wxCloseEvent& aEvent )
{
    BACKGROUND_JOB_LIST* evtWindow = dynamic_cast<BACKGROUND_JOB_LIST*>( aEvent.GetEventObject() );

    m_shownDialogs.erase( std::remove_if( m_shownDialogs.begin(), m_shownDialogs.end(),
                                          [&]( BACKGROUND_JOB_LIST* dialog )
                                          {
                                              return dialog == evtWindow;
                                          } ) );

    aEvent.Skip();
}


void BACKGROUND_JOBS_MONITOR::ShowList( wxWindow* aParent, wxPoint aPos )
{
    BACKGROUND_JOB_LIST* list = new BACKGROUND_JOB_LIST( aParent, aPos );

    std::shared_lock<std::shared_mutex> lock( m_mutex, std::try_to_lock );

    for( const std::shared_ptr<BACKGROUND_JOB>& job : m_jobs )
        list->Add( job );

    lock.unlock();

    m_shownDialogs.push_back( list );

    list->Bind( wxEVT_CLOSE_WINDOW, &BACKGROUND_JOBS_MONITOR::onListWindowClosed, this );

    // correct the position
    wxSize windowSize = list->GetSize();
    list->SetPosition( aPos - windowSize );

    list->Show();
}


void BACKGROUND_JOBS_MONITOR::jobUpdated( std::shared_ptr<BACKGROUND_JOB> aJob )
{
    std::shared_lock<std::shared_mutex> lock( m_mutex, std::try_to_lock );

    // this method is called from the reporters from potentially other threads
    // we have to guard ui calls with CallAfter
    if( m_jobs.size() > 0 )
    {
        //for now, we go and update the status bar if its the first job in the vector
        if( m_jobs.front() == aJob )
        {
            // update all status bar entries
            for( KISTATUSBAR* statusBar : m_statusBars )
            {
                statusBar->CallAfter(
                        [=]()
                        {
                            statusBar->ShowBackgroundProgressBar();
                            statusBar->SetBackgroundProgressMax( aJob->m_maxProgress );
                            statusBar->SetBackgroundProgress( aJob->m_currentProgress );
                            statusBar->SetBackgroundStatusText( aJob->m_status );
                        } );
            }
        }
    }


    for( BACKGROUND_JOB_LIST* list : m_shownDialogs )
    {
        list->CallAfter(
                [=]()
                {
                    list->UpdateJob( aJob );
                } );
    }
}


void BACKGROUND_JOBS_MONITOR::RegisterStatusBar( KISTATUSBAR* aStatusBar )
{
    m_statusBars.push_back( aStatusBar );
}


void BACKGROUND_JOBS_MONITOR::UnregisterStatusBar( KISTATUSBAR* aStatusBar )
{
    m_statusBars.erase( std::remove_if( m_statusBars.begin(), m_statusBars.end(),
                                        [&]( KISTATUSBAR* statusBar )
                                        {
                                            return statusBar == aStatusBar;
                                        } ) );
}
