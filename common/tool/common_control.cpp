/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2016 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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
// kicad_curl_easy.h must be included before wxWidgets because on Windows (msys2), there are
// collision with wx headers and curl.h defs
#include <kicad_curl/kicad_curl_easy.h>

#include <bitmaps.h>
#include <build_version.h>
#include <common.h>     // for SearchHelpFileFullPath
#include <pgm_base.h>
#include <tool/actions.h>
#include <tool/tool_manager.h>
#include <eda_draw_frame.h>
#include <view/view.h>
#include <gal/graphics_abstraction_layer.h>
#include <base_screen.h>
#include <tool/common_control.h>
#include <id.h>
#include <kiface_base.h>
#include <dialogs/dialog_configure_paths.h>
#include <eda_doc.h>
#include <wx/msgdlg.h>
#include <executable_names.h>
#include <gestfich.h>
#include <tools/kicad_manager_actions.h>

#define URL_GET_INVOLVED wxS( "https://go.kicad.org/contribute/" )
#define URL_DONATE wxS( "https://go.kicad.org/app-donate" )
#define URL_DOCUMENTATION wxS( "https://go.kicad.org/docs/" )


/// URL to launch a new issue with pre-populated description
wxString COMMON_CONTROL::m_bugReportUrl =
        wxS( "https://gitlab.com/kicad/code/kicad/-/issues/new?issuable_template=bare&issue"
             "[description]=%s" );


/// Issue template to use for reporting bugs (this should not be translated)
wxString COMMON_CONTROL::m_bugReportTemplate = wxS(
        "```\n"
        "%s\n"
        "```" );


void COMMON_CONTROL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<EDA_BASE_FRAME>();
}


int COMMON_CONTROL::OpenPreferences( const TOOL_EVENT& aEvent )
{
    m_frame->ShowPreferences( wxEmptyString, wxEmptyString );
    return 0;
}


int COMMON_CONTROL::ConfigurePaths( const TOOL_EVENT& aEvent )
{
    // If _pcbnew.kiface is running have it put up the dialog so the 3D paths can also
    // be edited
    if( KIFACE* pcbnew = m_frame->Kiway().KiFACE( KIWAY::FACE_PCB, false ) )
    {
        try
        {
            pcbnew->CreateKiWindow( m_frame, DIALOG_CONFIGUREPATHS, &m_frame->Kiway() );
        }
        catch( ... )
        {
            // Do nothing here.
            // A error message is displayed after trying to load _pcbnew.kiface.
        }
    }
    else
    {
        DIALOG_CONFIGURE_PATHS dlg( m_frame );

        if( dlg.ShowModal() == wxID_OK )
            m_frame->Kiway().CommonSettingsChanged( ENVVARS_CHANGED );
    }

    return 0;
}


int COMMON_CONTROL::ShowLibraryTable( const TOOL_EVENT& aEvent )
{
    if( aEvent.IsAction( &ACTIONS::showSymbolLibTable ) )
    {
        try     // Sch frame was not available, try to start it
        {
            if( KIFACE* kiface = m_frame->Kiway().KiFACE( KIWAY::FACE_SCH ) )
                kiface->CreateKiWindow( m_frame, DIALOG_SCH_LIBRARY_TABLE, &m_frame->Kiway() );
        }
        catch( ... )
        {
            // _eeschema.kiface is not available: it contains the library table dialog.
            // Do nothing here.
            // A error message is displayed after trying to load _eeschema.kiface.
        }
    }
    else if( aEvent.IsAction( &ACTIONS::showFootprintLibTable ) )
    {
        try     // Pcb frame was not available, try to start it
        {
            if( KIFACE* kiface = m_frame->Kiway().KiFACE( KIWAY::FACE_PCB ) )
                kiface->CreateKiWindow( m_frame, DIALOG_PCB_LIBRARY_TABLE, &m_frame->Kiway() );
        }
        catch( ... )
        {
            // _pcbnew.kiface is not available: it contains the library table dialog.
            // Do nothing here.
            // A error message is displayed after trying to load _pcbnew.kiface.
        }
    }
    else if( aEvent.IsAction( &ACTIONS::showDesignBlockLibTable ) )
    {
        try     // Kicad frame was not available, try to start it
        {
            if( KIFACE* kiface = m_frame->Kiway().KiFACE( KIWAY::FACE_SCH ) )
                kiface->CreateKiWindow( m_frame, DIALOG_DESIGN_BLOCK_LIBRARY_TABLE, &m_frame->Kiway() );
        }
        catch( ... )
        {
            // _eeschema.kiface is not available: it contains the library table dialog.
            // Do nothing here.
            // A error message is displayed after trying to load _eeschema.kiface.
        }
    }

    return 0;
}


void showFrame( EDA_BASE_FRAME* aFrame )
{
    // Needed on Windows, other platforms do not use it, but it creates no issue
    if( aFrame->IsIconized() )
        aFrame->Iconize( false );

    aFrame->Raise();

    // Raising the window does not set the focus on Linux.  This should work on
    // any platform.
    if( wxWindow::FindFocus() != aFrame )
        aFrame->SetFocus();

    // If the player is currently blocked, focus the user attention on the correct window
    if( wxWindow* blocking_win = aFrame->Kiway().GetBlockingDialog() )
    {
        blocking_win->Raise();
        blocking_win->SetFocus();
    }
}


int COMMON_CONTROL::ShowPlayer( const TOOL_EVENT& aEvent )
{
    FRAME_T       playerType = aEvent.Parameter<FRAME_T>();
    KIWAY_PLAYER* editor = m_frame->Kiway().Player( playerType, true );

    // editor can be null if Player() fails:
    wxCHECK_MSG( editor != nullptr, 0, wxT( "Cannot open/create the editor frame" ) );

    showFrame( editor );

    return 0;
}


int COMMON_CONTROL::Quit( const TOOL_EVENT& aEvent )
{
    m_frame->CallAfter(
            [this]()
            {
                m_frame->Kiway().OnKiCadExit();
            } );

    return 0;
}


class TERMINATE_HANDLER : public wxProcess
{
public:
    TERMINATE_HANDLER( const wxString& appName )
    { }

    void OnTerminate( int pid, int status ) override
    {
        delete this;
    }
};


int COMMON_CONTROL::Execute( const wxString& aExecutible, const wxString& aParam )
{
    TERMINATE_HANDLER* callback = new TERMINATE_HANDLER( aExecutible );

    bool isKicadBinary = false;

    if( aExecutible == GERBVIEW_EXE || aExecutible == BITMAPCONVERTER_EXE
        || aExecutible == PCB_CALCULATOR_EXE || aExecutible == PL_EDITOR_EXE
        || aExecutible == EESCHEMA_EXE || aExecutible == PCBNEW_EXE )
    {
        isKicadBinary = true;
    }

    long pid = ExecuteFile( aExecutible, aParam, callback, isKicadBinary );

    if( pid > 0 )
    {
#ifdef __WXMAC__
        wxString script;

        script.Printf( wxS( "tell application \"System Events\" to tell application process \"%s\"\n"
                            "   set frontmost to true\n"
                            "end tell" ),
                       aExecutible );

        // This non-parameterized use of wxExecute is fine because script is not derived
        // from user input.
        wxExecute( wxString::Format( "osascript -e '%s'", script ) );
#endif
    }
    else
    {
        delete callback;
    }

    return 0;
}


int COMMON_CONTROL::Execute( const TOOL_EVENT& aEvent )
{
    wxString execFile;
    wxString param;

    if( aEvent.IsAction( &ACTIONS::showCalculatorTools ) )
        execFile = PCB_CALCULATOR_EXE;
    else
        wxFAIL_MSG( "Execute(): unexpected request" );

    if( execFile.IsEmpty() )
        return 0;

    return Execute( execFile, param );
}


int COMMON_CONTROL::ShowProjectManager( const TOOL_EVENT& aEvent )
{
    // Note: dynamic_cast doesn't work over the Kiway() on MacOS.  We have to use static_cast
    // here.
    EDA_BASE_FRAME* top = static_cast<EDA_BASE_FRAME*>( m_frame->Kiway().GetTop() );

    if( top && top->GetFrameType() == KICAD_MAIN_FRAME_T )
        showFrame( top );
    else
        wxMessageDialog( m_frame, _( "Can not switch to project manager in stand-alone mode." ) );

    return 0;
}


int COMMON_CONTROL::ShowHelp( const TOOL_EVENT& aEvent )
{
    wxString helpFile;
    wxString msg;

    // the URL of help files is "https://go.kicad.org/docs/<version>/<language>/<name>/"
    const wxString baseUrl = URL_DOCUMENTATION + GetMajorMinorVersion() + wxT( "/" )
                             + Pgm().GetLocale()->GetName().BeforeLast( '_' ) + wxT( "/" );

    /* We have to get document for beginners,
     * or the full specific doc
     * if event id is wxID_INDEX, we want the document for beginners.
     * else the specific doc file (its name is in Kiface().GetHelpFileName())
     * The document for beginners is the same for all KiCad utilities
     */
    if( aEvent.IsAction( &ACTIONS::gettingStarted ) )
    {
        // List of possible names for Getting Started in KiCad
        const wxChar* names[2] = {
                wxT( "getting_started_in_kicad" ),
                wxT( "Getting_Started_in_KiCad" )
        };

        // Search for "getting_started_in_kicad.html" or "getting_started_in_kicad.pdf"
        // or "Getting_Started_in_KiCad.html" or "Getting_Started_in_KiCad.pdf"
        for( auto& name : names )
        {
            helpFile = SearchHelpFileFullPath( name );

            if( !helpFile.IsEmpty() )
                break;
        }

        if( !helpFile )
        {
            msg = wxString::Format( _( "Help file '%s' or\n'%s' could not be found.\n"
                                       "Do you want to access the KiCad online help?" ),
                                    names[0], names[1] );
            wxMessageDialog dlg( nullptr, msg, _( "File Not Found" ),
                                 wxYES_NO | wxNO_DEFAULT | wxCANCEL );

            if( dlg.ShowModal() != wxID_YES )
                return -1;

            helpFile = baseUrl + names[0] + wxS( "/" );
        }
    }
    else
    {
        wxString base_name = m_frame->help_name();

        helpFile = SearchHelpFileFullPath( base_name );

        if( !helpFile )
        {
            msg = wxString::Format( _( "Help file '%s' could not be found.\n"
                                       "Do you want to access the KiCad online help?" ),
                                    base_name );
            wxMessageDialog dlg( nullptr, msg, _( "File Not Found" ),
                                 wxYES_NO | wxNO_DEFAULT | wxCANCEL );

            if( dlg.ShowModal() != wxID_YES )
                return -1;

            helpFile = baseUrl + base_name + wxS( "/" );
        }
    }

    GetAssociatedDocument( m_frame, helpFile, &m_frame->Prj() );
    return 0;
}


int COMMON_CONTROL::About( const TOOL_EVENT& aEvent )
{
    void ShowAboutDialog( EDA_BASE_FRAME * aParent ); // See AboutDialog_main.cpp
    ShowAboutDialog( m_frame );
    return 0;
}


int COMMON_CONTROL::ListHotKeys( const TOOL_EVENT& aEvent )
{
    DisplayHotkeyList( m_frame );
    return 0;
}


int COMMON_CONTROL::GetInvolved( const TOOL_EVENT& aEvent )
{
    if( !wxLaunchDefaultBrowser( URL_GET_INVOLVED ) )
    {
        wxString msg;
        msg.Printf( _( "Could not launch the default browser.\n"
                       "For information on how to help the KiCad project, visit %s" ),
                    URL_GET_INVOLVED );
        wxMessageBox( msg, _( "Get involved with KiCad" ), wxOK, m_frame );
    }

    return 0;
}


int COMMON_CONTROL::Donate( const TOOL_EVENT& aEvent )
{
    if( !wxLaunchDefaultBrowser( URL_DONATE ) )
    {
        wxString msg;
        msg.Printf( _( "Could not launch the default browser.\n"
                       "To donate to the KiCad project, visit %s" ),
                    URL_DONATE );
        wxMessageBox( msg, _( "Donate to KiCad" ), wxOK, m_frame );
    }

    return 0;
}


int COMMON_CONTROL::ReportBug( const TOOL_EVENT& aEvent )
{
    if( WarnUserIfOperatingSystemUnsupported() )
        return 0;

    wxString version = GetVersionInfoData( m_frame->GetUntranslatedAboutTitle(), false, true );

    wxString message;
    message.Printf( m_bugReportTemplate, version );

    KICAD_CURL_EASY kcurl;
    wxString url_string;
    url_string.Printf( m_bugReportUrl, kcurl.Escape( std::string( message.utf8_str() ) ) );

    wxLaunchDefaultBrowser( url_string );

    return 0;
}


void COMMON_CONTROL::setTransitions()
{
    Go( &COMMON_CONTROL::Quit,               ACTIONS::quit.MakeEvent() );

    Go( &COMMON_CONTROL::OpenPreferences,    ACTIONS::openPreferences.MakeEvent() );
    Go( &COMMON_CONTROL::ConfigurePaths,     ACTIONS::configurePaths.MakeEvent() );
    Go( &COMMON_CONTROL::ShowLibraryTable,   ACTIONS::showSymbolLibTable.MakeEvent() );
    Go( &COMMON_CONTROL::ShowLibraryTable,   ACTIONS::showFootprintLibTable.MakeEvent() );
    Go( &COMMON_CONTROL::ShowLibraryTable,   ACTIONS::showDesignBlockLibTable.MakeEvent() );
    Go( &COMMON_CONTROL::ShowPlayer,         ACTIONS::showSymbolBrowser.MakeEvent() );
    Go( &COMMON_CONTROL::ShowPlayer,         ACTIONS::showSymbolEditor.MakeEvent() );
    Go( &COMMON_CONTROL::ShowPlayer,         ACTIONS::showFootprintBrowser.MakeEvent() );
    Go( &COMMON_CONTROL::ShowPlayer,         ACTIONS::showFootprintEditor.MakeEvent() );
    Go( &COMMON_CONTROL::Execute,            ACTIONS::showCalculatorTools.MakeEvent() );
    Go( &COMMON_CONTROL::ShowProjectManager, ACTIONS::showProjectManager.MakeEvent() );

    Go( &COMMON_CONTROL::ShowHelp,           ACTIONS::gettingStarted.MakeEvent() );
    Go( &COMMON_CONTROL::ShowHelp,           ACTIONS::help.MakeEvent() );
    Go( &COMMON_CONTROL::ListHotKeys,        ACTIONS::listHotKeys.MakeEvent() );
    Go( &COMMON_CONTROL::GetInvolved,        ACTIONS::getInvolved.MakeEvent() );
    Go( &COMMON_CONTROL::Donate,             ACTIONS::donate.MakeEvent() );
    Go( &COMMON_CONTROL::ReportBug,          ACTIONS::reportBug.MakeEvent() );
    Go( &COMMON_CONTROL::About,              ACTIONS::about.MakeEvent() );
}


