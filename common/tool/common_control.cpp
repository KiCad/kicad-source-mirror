/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2016 CERN
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

#include <bitmaps.h>
#include <build_version.h>
#include <common.h>     // for SearchHelpFileFullPath
#include <tool/actions.h>
#include <tool/tool_manager.h>
#include <eda_draw_frame.h>
#include <view/view.h>
#include <gal/graphics_abstraction_layer.h>
#include <base_screen.h>
#include <tool/common_control.h>
#include <id.h>
#include <kiface_i.h>
#include <kicad_curl/kicad_curl_easy.h>
#include <dialogs/dialog_configure_paths.h>
#include <eda_doc.h>

#define URL_GET_INVOLVED "https://kicad.org/contribute/"
#define URL_DONATE "https://go.kicad.org/app-donate"


/// URL to launch a new issue with pre-populated description
wxString COMMON_CONTROL::m_bugReportUrl =
        "https://gitlab.com/kicad/code/kicad/issues/new?issue[description]=%s";

/// Issue template to use for reporting bugs (this should not be translated)
wxString COMMON_CONTROL::m_bugReportTemplate =
        "<!-- Before Creating a New Issue:\n"
        "* Search the issue tracker to verify the issue has not already been reported.\n"
        "* Only report one problem per issue. -->\n"
        "\n"
        "# Description\n"
        "<!-- What is the current behavior and what is the expected behavior? -->\n"
        "<!-- Please attach screenshots if they will help explain the problem. -->\n"
        "\n"
        "# Steps to reproduce\n"
        "<!-- Please include a screen recording if it will help explain how to reproduce. -->\n"
        "<!-- If this issue is specific to a project, please attach it. -->\n"
        "1.\n"
        "2.\n"
        "# KiCad Version\n"
        "\n"
        "```\n"
        "%s\n"
        "```";


void COMMON_CONTROL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<EDA_BASE_FRAME>();
}


int COMMON_CONTROL::ConfigurePaths( const TOOL_EVENT& aEvent )
{
    // If _pcbnew.kiface is running have it put up the dialog so the 3D paths can also
    // be edited
    KIFACE* pcbnew = m_frame->Kiway().KiFACE( KIWAY::FACE_PCB, false );

    if( pcbnew )
    {
        try
        {
            pcbnew->CreateWindow( m_frame, DIALOG_CONFIGUREPATHS, &m_frame->Kiway() );
        }
        catch( ... )
        {
            // Do nothing here.
            // A error message is displayed after trying to load _pcbnew.kiface.
        }
    }
    else
    {
        DIALOG_CONFIGURE_PATHS dlg( m_frame, nullptr );

        // Use QuasiModal so that HTML help window will work
        if( dlg.ShowQuasiModal() == wxID_OK )
            m_frame->Kiway().CommonSettingsChanged( true, false );
    }

    return 0;
}


int COMMON_CONTROL::ShowLibraryTable( const TOOL_EVENT& aEvent )
{
    if( aEvent.IsAction( &ACTIONS::showSymbolLibTable ) )
    {
        try     // Sch frame was not available, try to start it
        {
            KIFACE* kiface = m_frame->Kiway().KiFACE( KIWAY::FACE_SCH );
            kiface->CreateWindow( m_frame, DIALOG_SCH_LIBRARY_TABLE, &m_frame->Kiway() );
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
            KIFACE* kiface = m_frame->Kiway().KiFACE( KIWAY::FACE_PCB );
            kiface->CreateWindow( m_frame, DIALOG_PCB_LIBRARY_TABLE, &m_frame->Kiway() );
        }
        catch( ... )
        {
            // _pcbnew.kiface is not available: it contains the library table dialog.
            // Do nothing here.
            // A error message is displayed after trying to load _pcbnew.kiface.
        }
    }

    return 0;
}


int COMMON_CONTROL::ShowPlayer( const TOOL_EVENT& aEvent )
{
    FRAME_T       playerType = aEvent.Parameter<FRAME_T>();
    KIWAY_PLAYER* editor = m_frame->Kiway().Player( playerType, true );

    // editor can be null if Player() fails:
    wxCHECK_MSG( editor != nullptr, 0, "Cannot open/create the editor frame" );

    // Needed on Windows, other platforms do not use it, but it creates no issue
    if( editor->IsIconized() )
        editor->Iconize( false );

    editor->Raise();

    // Raising the window does not set the focus on Linux.  This should work on
    // any platform.
    if( wxWindow::FindFocus() != editor )
        editor->SetFocus();

    return 0;
}


int COMMON_CONTROL::ShowHelp( const TOOL_EVENT& aEvent )
{
    const SEARCH_STACK& search = m_frame->sys_search();
    wxString            helpFile;
    wxString            msg;

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
            helpFile = SearchHelpFileFullPath( search, name );

            if( !helpFile.IsEmpty() )
                break;
        }

        if( !helpFile )
        {
            msg = wxString::Format( _( "Html or pdf help file \n%s\nor\n%s could not be found." ),
                                    names[0], names[1] );
            wxMessageBox( msg );
            return -1;
        }
    }
    else
    {
        wxString base_name = m_frame->help_name();

        helpFile = SearchHelpFileFullPath( search, base_name );

        if( !helpFile )
        {
            msg = wxString::Format( _( "Help file \"%s\" could not be found." ), base_name );
            wxMessageBox( msg );
            return -1;
        }
    }

    GetAssociatedDocument( m_frame, helpFile, &m_frame->Prj() );
    return 0;
}


int COMMON_CONTROL::ListHotKeys( const TOOL_EVENT& aEvent )
{
    DisplayHotkeyList( m_frame, m_toolMgr );
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
    wxString version = GetVersionInfoData( m_frame->GetAboutTitle(), false, true );

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
    Go( &COMMON_CONTROL::ConfigurePaths,     ACTIONS::configurePaths.MakeEvent() );
    Go( &COMMON_CONTROL::ShowLibraryTable,   ACTIONS::showSymbolLibTable.MakeEvent() );
    Go( &COMMON_CONTROL::ShowLibraryTable,   ACTIONS::showFootprintLibTable.MakeEvent() );
    Go( &COMMON_CONTROL::ShowPlayer,         ACTIONS::showSymbolBrowser.MakeEvent() );
    Go( &COMMON_CONTROL::ShowPlayer,         ACTIONS::showSymbolEditor.MakeEvent() );
    Go( &COMMON_CONTROL::ShowPlayer,         ACTIONS::showFootprintBrowser.MakeEvent() );
    Go( &COMMON_CONTROL::ShowPlayer,         ACTIONS::showFootprintEditor.MakeEvent() );

    Go( &COMMON_CONTROL::ShowHelp,           ACTIONS::gettingStarted.MakeEvent() );
    Go( &COMMON_CONTROL::ShowHelp,           ACTIONS::help.MakeEvent() );
    Go( &COMMON_CONTROL::ListHotKeys,        ACTIONS::listHotKeys.MakeEvent() );
    Go( &COMMON_CONTROL::GetInvolved,        ACTIONS::getInvolved.MakeEvent() );
    Go( &COMMON_CONTROL::Donate,             ACTIONS::donate.MakeEvent() );
    Go( &COMMON_CONTROL::ReportBug,          ACTIONS::reportBug.MakeEvent() );
}


