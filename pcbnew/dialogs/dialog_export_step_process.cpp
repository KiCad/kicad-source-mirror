/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2020 New Pagodi(https://stackoverflow.com/users/6846682/new-pagodi)
 *                    from https://stackoverflow.com/a/63289812/1522001
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

#include "dialog_export_step_process.h"
#include <wx/textctrl.h>
#include <wx/process.h>
#include <wx/log.h>
#include <wx/timer.h>
#include <wx/txtstrm.h>
#include <wx/msgdlg.h>

wxDEFINE_EVENT( wxEVT_THREAD_STDIN, wxThreadEvent );
wxDEFINE_EVENT( wxEVT_THREAD_STDERR, wxThreadEvent );

/**
 * This thread handles consuming the input streams from the launched process.
 * And generates ui events on the main thread with the content
 */
class STDSTREAM_THREAD : public wxThread
{
public:
    STDSTREAM_THREAD( wxEvtHandler* aEventHandler, wxProcess* aProcess,
                      wxMessageQueue<DIALOG_EXPORT_STEP_LOG::STATE_MESSAGE>& aMsgQueue ) :
            wxThread( wxTHREAD_JOINABLE ),
            m_queue( aMsgQueue )
    {
        m_process = aProcess;
        m_handler = aEventHandler;
        m_bufferSize = 1024 * 1024;
        m_buffer = new char[m_bufferSize];
    }

    ~STDSTREAM_THREAD()
    {
        delete[] m_buffer;
    }

private:
    ExitCode Entry() override;
    void     DrainInput();

    wxMessageQueue<DIALOG_EXPORT_STEP_LOG::STATE_MESSAGE>& m_queue;
    wxEvtHandler*                  m_handler;
    wxProcess*                     m_process;
    char*                          m_buffer;
    size_t                         m_bufferSize;
};


wxThread::ExitCode STDSTREAM_THREAD::Entry()
{
    ExitCode c;

    while( 1 )
    {
        // Check if termination was requested.
        if( TestDestroy() )
        {
            wxProcess::Kill( m_process->GetPid(), wxSIGKILL );
            c = reinterpret_cast<ExitCode>( 1 );
            break;
        }

        DIALOG_EXPORT_STEP_LOG::STATE_MESSAGE m = DIALOG_EXPORT_STEP_LOG::STATE_MESSAGE::SENTINEL;
        wxMessageQueueError e = m_queue.ReceiveTimeout( 10, m );

        // Check if a message was received or we timed out.
        if( e == wxMSGQUEUE_NO_ERROR )
        {
            if( m == DIALOG_EXPORT_STEP_LOG::STATE_MESSAGE::PROCESS_COMPLETE )
            {
                DrainInput();
                c = reinterpret_cast<ExitCode>( 0 );
                break;
            }
            else if( m == DIALOG_EXPORT_STEP_LOG::STATE_MESSAGE::REQUEST_EXIT )
            {
                wxProcess::Kill( m_process->GetPid(), wxSIGKILL );
                c = reinterpret_cast<ExitCode>( 1 );
                break;
            }
        }
        else if( e == wxMSGQUEUE_TIMEOUT )
        {
            DrainInput();
        }
    }

    return c;
}


void STDSTREAM_THREAD::DrainInput()
{
    if( !m_process->IsInputOpened() )
    {
        return;
    }

    wxString       fromInputStream, fromErrorStream;
    wxInputStream* stream;

    while( m_process->IsInputAvailable() )
    {
        stream = m_process->GetInputStream();
        stream->Read( m_buffer, m_bufferSize );
        fromInputStream << wxString( m_buffer, stream->LastRead() );
    }

    while( m_process->IsErrorAvailable() )
    {
        stream = m_process->GetErrorStream();
        stream->Read( m_buffer, m_bufferSize );
        fromErrorStream << wxString( m_buffer, stream->LastRead() );
    }

    if( !fromInputStream.IsEmpty() )
    {
        wxThreadEvent* event = new wxThreadEvent( wxEVT_THREAD_STDIN );
        event->SetString( fromInputStream );
        m_handler->QueueEvent( event );
    }

    if( !fromErrorStream.IsEmpty() )
    {
        wxThreadEvent* event = new wxThreadEvent( wxEVT_THREAD_STDERR );
        event->SetString( fromErrorStream );
        m_handler->QueueEvent( event );
    }
}


bool DIALOG_EXPORT_STEP_LOG::TransferDataToWindow()
{
    // Ensure we start with a clean log window message
    m_textCtrlLog->Clear();
    m_textCtrlLog->AppendText( m_startMessage );
    return true;
}


void DIALOG_EXPORT_STEP_LOG::appendMessage( const wxString& aMessage )
{
    m_textCtrlLog->AppendText( aMessage );
}


void DIALOG_EXPORT_STEP_LOG::onProcessTerminate( wxProcessEvent& aEvent )
{
    // We need to inform the thread that the process has died
    // Since it can't receive the event from wx
    if( m_stdioThread && m_stdioThread->IsRunning() )
    {
        m_msgQueue.Post( STATE_MESSAGE::PROCESS_COMPLETE );
        m_stdioThread->Wait();
        delete m_stdioThread;
        m_stdioThread = nullptr;
        m_sdbSizerOK->Enable( true );

        int exitCode = aEvent.GetExitCode();

        // set the progress bar to complete/incomplete base don status
        m_activityGauge->SetRange( 1 );

        if( exitCode != 0 )
        {
            m_textCtrlLog->SetForegroundColour( *wxRED );

            m_textCtrlLog->AppendText( wxS( "\n*** " ) );
            m_textCtrlLog->AppendText(
                    wxString::Format( _( "Process failed with exit code %d" ), exitCode ) );
            m_textCtrlLog->AppendText( wxS( " ***\n" ) );

            m_activityGauge->SetValue( 0 );
        }
        else
        {
            m_textCtrlLog->AppendText( wxS( "\n*** " ) );
            m_textCtrlLog->AppendText( wxString::Format( _( "Success" ) ) );
            m_textCtrlLog->AppendText( wxS( " ***\n" ) );

            m_activityGauge->SetValue( 1 );
        }
    }
}

void DIALOG_EXPORT_STEP_LOG::onThreadInput( wxThreadEvent& aEvent )
{
    m_textCtrlLog->AppendText( aEvent.GetString() );
    m_activityGauge->Pulse();
}


void DIALOG_EXPORT_STEP_LOG::onClose( wxCloseEvent& aEvent )
{
    if( m_stdioThread && m_stdioThread->IsRunning() )
    {
        if( aEvent.CanVeto() )
        {
            wxMessageDialog dlg( this, _( "Do you want to cancel the export process?" ),
                                 _( "Cancel Export" ), wxYES_NO );

            if( dlg.ShowModal() == wxID_NO )
            {
                aEvent.Veto();
                return;
            }
        }

        m_msgQueue.Post( STATE_MESSAGE::REQUEST_EXIT );
        m_stdioThread->Delete();

        m_process->DeletePendingEvents();
        m_process->Unlink();
        m_process->CloseOutput();
        m_process->Detach();
    }

    // Clear log window message, storing the log data in config has no interest.
    m_textCtrlLog->Clear();

    aEvent.Skip();
}

DIALOG_EXPORT_STEP_LOG::~DIALOG_EXPORT_STEP_LOG()
{
    delete m_stdioThread;
}


DIALOG_EXPORT_STEP_LOG::DIALOG_EXPORT_STEP_LOG( wxWindow* aParent, const wxString& aStepCmd ) :
        DIALOG_EXPORT_STEP_PROCESS_BASE( aParent )
{
    m_sdbSizerOK->Enable( false );

    m_process = new wxProcess( this );
    m_process->Redirect();

    Bind( wxEVT_END_PROCESS, &DIALOG_EXPORT_STEP_LOG::onProcessTerminate, this );

    Bind( wxEVT_THREAD_STDIN, &DIALOG_EXPORT_STEP_LOG::onThreadInput, this );
    Bind( wxEVT_THREAD_STDERR, &DIALOG_EXPORT_STEP_LOG::onThreadInput, this );
    Bind( wxEVT_CLOSE_WINDOW, &DIALOG_EXPORT_STEP_LOG::onClose, this );

    // Print the command line used to run kicad-cli.
    // it can be useful if kicad-cli as a problem.
    // However it cannot be printed in the Ctor, but only in TransferDataToWindow(),
    // after DIALOG_SHIM initializations
    m_startMessage.Append( _( "Command line:\n" ) );
    m_startMessage.Append( aStepCmd );
    m_startMessage.Append( wxT( "\n\n" ) );

    m_stdioThread = new STDSTREAM_THREAD( this, m_process, m_msgQueue );
    m_stdioThread->Run();

    if( !m_stdioThread->IsRunning() )
    {
        m_startMessage.Append( "Unable to launch stdstream thread.\n" );
        delete m_stdioThread;
        return;
    }

    m_activityGauge->Pulse();

    wxExecute( aStepCmd, wxEXEC_ASYNC, m_process );
}