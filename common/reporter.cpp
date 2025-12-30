/**
 * @file reporter.cpp
 */
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
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

#include <mutex>
#include <macros.h>
#include <reporter.h>
#include <font/fontconfig.h>
#include <string_utils.h>
#include <widgets/wx_infobar.h>
#include <wx/crt.h>
#include <wx/log.h>
#include <wx/textctrl.h>
#include <wx/statusbr.h>


/**
 * Flag to enable reporter debugging output.
 *
 * @ingroup trace_env_vars
 */
static const wxChar traceReporter[] = wxT( "KICAD_REPORTER" );

static std::mutex g_logReporterMutex;


REPORTER& REPORTER::Report( const char* aText, SEVERITY aSeverity )
{
    Report( From_UTF8( aText ) );
    return *this;
}


REPORTER& WX_TEXT_CTRL_REPORTER::Report( const wxString& aText, SEVERITY aSeverity )
{
    REPORTER::Report( aText, aSeverity );

    wxCHECK_MSG( m_textCtrl != nullptr, *this,
                 wxT( "No wxTextCtrl object defined in WX_TEXT_CTRL_REPORTER." ) );

    m_textCtrl->AppendText( aText + wxS( "\n" ) );
    return *this;
}


REPORTER& WX_STRING_REPORTER::Report( const wxString& aText, SEVERITY aSeverity )
{
    REPORTER::Report( aText, aSeverity );

    m_string << aText << wxS( "\n" );
    return *this;
}


const wxString& WX_STRING_REPORTER::GetMessages() const
{
    return m_string;
}


void WX_STRING_REPORTER::Clear()
{
    REPORTER::Clear();
    m_string.clear();
}


REPORTER& NULL_REPORTER::Report( const wxString& aText, SEVERITY aSeverity )
{
    return REPORTER::Report( aText, aSeverity );
}


REPORTER& NULL_REPORTER::GetInstance()
{
    static REPORTER* s_nullReporter = nullptr;

    if( !s_nullReporter )
        s_nullReporter = new NULL_REPORTER();

    return *s_nullReporter;
}


REPORTER& CLI_REPORTER::Report( const wxString& aMsg, SEVERITY aSeverity )
{
    REPORTER::Report( aMsg, aSeverity );

    FILE* target = stdout;

    if( aSeverity == RPT_SEVERITY_ERROR )
        target = stderr;

    if( aMsg.EndsWith( wxS( "\n" ) ) )
        wxFprintf( target, aMsg );
    else
        wxFprintf( target, aMsg + wxS( "\n" ) );

    // Needed  after wxPrintf (or printf) to be sure the message is immediately printed
    // (i.e. not stored in some i/o buffer)
    fflush( target );

    return *this;
}


REPORTER& CLI_REPORTER::GetInstance()
{
    static CLI_REPORTER s_cliReporter;

    return s_cliReporter;
}


REPORTER& STDOUT_REPORTER::Report( const wxString& aMsg, SEVERITY aSeverity )
{
    REPORTER::Report( aMsg, aSeverity );

    switch( aSeverity )
    {
    case RPT_SEVERITY_UNDEFINED: std::cout << "SEVERITY_UNDEFINED: "; break;
    case RPT_SEVERITY_INFO:      std::cout << "SEVERITY_INFO: ";      break;
    case RPT_SEVERITY_WARNING:   std::cout << "SEVERITY_WARNING: ";   break;
    case RPT_SEVERITY_ERROR:     std::cout << "SEVERITY_ERROR: ";     break;
    case RPT_SEVERITY_ACTION:    std::cout << "SEVERITY_ACTION: ";    break;
    case RPT_SEVERITY_DEBUG:     std::cout << "SEVERITY_DEBUG: ";    break;
    case RPT_SEVERITY_EXCLUSION:
    case RPT_SEVERITY_IGNORE:    break;
    }

    std::cout << aMsg << std::endl;

    return *this;
}


REPORTER& STDOUT_REPORTER::GetInstance()
{
    static REPORTER* s_stdoutReporter = nullptr;

    if( !s_stdoutReporter )
        s_stdoutReporter = new STDOUT_REPORTER();

    return *s_stdoutReporter;
}


REPORTER& WXLOG_REPORTER::Report( const wxString& aMsg, SEVERITY aSeverity )
{
    REPORTER::Report( aMsg, aSeverity );

    switch( aSeverity )
    {
    case RPT_SEVERITY_ERROR:     wxLogError( aMsg );                  break;
    case RPT_SEVERITY_WARNING:   wxLogWarning( aMsg );                break;
    case RPT_SEVERITY_UNDEFINED: wxLogMessage( aMsg );                break;
    case RPT_SEVERITY_INFO:      wxLogInfo( aMsg );                   break;
    case RPT_SEVERITY_ACTION:    wxLogInfo( aMsg );                   break;
    case RPT_SEVERITY_DEBUG:     wxLogTrace( traceReporter, aMsg );   break;
    case RPT_SEVERITY_EXCLUSION:                                      break;
    case RPT_SEVERITY_IGNORE:                                         break;
    }

    return *this;
}


REPORTER& WXLOG_REPORTER::GetInstance()
{
    static REPORTER* s_wxLogReporter = nullptr;
    std::lock_guard lock( g_logReporterMutex );

    if( !s_wxLogReporter )
        s_wxLogReporter = new WXLOG_REPORTER();

    return *s_wxLogReporter;
}


REPORTER& LOAD_INFO_REPORTER::Report( const wxString& aMsg, SEVERITY aSeverity )
{
    REPORTER::Report( aMsg, aSeverity );

    REPORTER* target = m_redirectTarget;

    if( !target )
        target = &WXLOG_REPORTER::GetInstance();

    target->Report( aMsg, aSeverity );

    return *this;
}


LOAD_INFO_REPORTER& LOAD_INFO_REPORTER::GetInstance()
{
    static LOAD_INFO_REPORTER s_loadInfoReporter;
    std::lock_guard lock( g_logReporterMutex );

    return s_loadInfoReporter;
}


void LOAD_INFO_REPORTER::SetRedirectTarget( REPORTER* aReporter )
{
    std::lock_guard lock( g_logReporterMutex );
    m_redirectTarget = aReporter;
}


REPORTER* LOAD_INFO_REPORTER::GetRedirectTarget() const
{
    std::lock_guard lock( g_logReporterMutex );
    return m_redirectTarget;
}


LOAD_INFO_REPORTER_SCOPE::LOAD_INFO_REPORTER_SCOPE( REPORTER* aReporter ) :
        m_reporter( LOAD_INFO_REPORTER::GetInstance() ),
        m_previousReporter( m_reporter.GetRedirectTarget() )
{
    m_reporter.SetRedirectTarget( aReporter );
}


LOAD_INFO_REPORTER_SCOPE::~LOAD_INFO_REPORTER_SCOPE()
{
    m_reporter.SetRedirectTarget( m_previousReporter );
}


FONTCONFIG_REPORTER_SCOPE::FONTCONFIG_REPORTER_SCOPE( REPORTER* aReporter ) :
        m_previousReporter( fontconfig::FONTCONFIG::GetReporter() )
{
    fontconfig::FONTCONFIG::SetReporter( aReporter );
}


FONTCONFIG_REPORTER_SCOPE::~FONTCONFIG_REPORTER_SCOPE()
{
    fontconfig::FONTCONFIG::SetReporter( m_previousReporter );
}


REPORTER& REDIRECT_REPORTER::Report( const wxString& aText, SEVERITY aSeverity )
{
    REPORTER::Report( aText, aSeverity );

    if( m_redirectTarget )
        m_redirectTarget->Report( aText, aSeverity );

    return *this;
}


REPORTER& STATUSBAR_REPORTER::Report( const wxString& aText, SEVERITY aSeverity )
{
    REPORTER::Report( aText, aSeverity );

    if( m_statusBar )
        m_statusBar->SetStatusText( aText, m_position );

    return *this;
}
