/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file kicad.cpp
 * @brief Main KiCad Project manager file
 */


#include <wx/filename.h>
#include <wx/log.h>
#include <wx/app.h>
#include <wx/stdpaths.h>
#include <wx/msgdlg.h>

#include <filehistory.h>
#include <hotkeys_basic.h>
#include <kiway.h>
#include <settings/settings_manager.h>
#include <systemdirsappend.h>

#include <stdexcept>

#include "pgm_kicad.h"
#include "kicad_manager_frame.h"
#include "kicad_settings.h"

#if defined( _WIN32 )
#include <config.h>
#include <VersionHelpers.h>
#endif

// a dummy to quiet linking with EDA_BASE_FRAME::config();
#include <kiface_i.h>
KIFACE_I& Kiface()
{
    // This function should never be called.  It is only referenced from
    // EDA_BASE_FRAME::config() and this is only provided to satisfy the linker,
    // not to be actually called.
    wxLogFatalError( wxT( "Unexpected call to Kiface() in kicad/kicad.cpp" ) );

    throw std::logic_error( "Unexpected call to Kiface() in kicad/kicad.cpp" );
}


static PGM_KICAD program;


PGM_BASE& Pgm()
{
    return program;
}


// Similar to PGM_BASE& Pgm(), but return nullptr when a *.ki_face is run from a python script.
PGM_BASE* PgmOrNull()
{
    return &program;
}

PGM_KICAD& PgmTop()
{
    return program;
}


bool PGM_KICAD::OnPgmInit()
{
#if defined(DEBUG)
    wxString absoluteArgv0 = wxStandardPaths::Get().GetExecutablePath();

    if( !wxIsAbsolutePath( absoluteArgv0 ) )
    {
        wxLogError( wxT( "No meaningful argv[0]" ) );
        return false;
    }
#endif

    if( !InitPgm() )
        return false;

    m_bm.InitSettings( new KICAD_SETTINGS );
    GetSettingsManager().RegisterSettings( PgmSettings() );
    GetSettingsManager().SetKiway( &Kiway );
    m_bm.Init();

    // Add search paths to feed the PGM_KICAD::SysSearch() function,
    // currenly limited in support to only look for project templates
    {
        SEARCH_STACK bases;

        SystemDirsAppend( &bases );

        for( unsigned i = 0; i < bases.GetCount(); ++i )
        {
            wxFileName fn( bases[i], wxEmptyString );

            // Add KiCad template file path to search path list.
            fn.AppendDir( wxT( "template" ) );

            // Only add path if exists and can be read by the user.
            if( fn.DirExists() && fn.IsDirReadable() )
                m_bm.m_search.AddPaths( fn.GetPath() );
        }

        // The KICAD6_TEMPLATE_DIR takes precedence over the search stack template path.
        ENV_VAR_MAP_CITER it = GetLocalEnvVariables().find( "KICAD6_TEMPLATE_DIR" );

        if( it != GetLocalEnvVariables().end() && it->second.GetValue() != wxEmptyString )
            m_bm.m_search.Insert( it->second.GetValue(), 0 );

        // The KICAD_USER_TEMPLATE_DIR takes precedence over KICAD6_TEMPLATE_DIR and the search
        // stack template path.
        it = GetLocalEnvVariables().find( "KICAD_USER_TEMPLATE_DIR" );

        if( it != GetLocalEnvVariables().end() && it->second.GetValue() != wxEmptyString )
            m_bm.m_search.Insert( it->second.GetValue(), 0 );
    }

    KICAD_MANAGER_FRAME* frame = new KICAD_MANAGER_FRAME( NULL, wxT( "KiCad" ),
                                                          wxDefaultPosition, wxSize( 775, -1 ) );
    App().SetTopWindow( frame );

    Kiway.SetTop( frame );

    KICAD_SETTINGS* settings = static_cast<KICAD_SETTINGS*>( PgmSettings() );

    wxString projToLoad;

    if( App().argc > 1 )
        projToLoad = App().argv[1];

    // If no file was given as an argument, check that there was a file open.
    if( projToLoad.IsEmpty() && settings->m_OpenProjects.size() )
    {
        wxString last_pro = settings->m_OpenProjects.front();
        settings->m_OpenProjects.erase( settings->m_OpenProjects.begin() );

        if( wxFileExists( last_pro ) )
        {
            // Try to open the last opened project,
            // if a project name is not given when starting Kicad
            projToLoad = last_pro;
        }
    }

    // Do not attempt to load a non-existent project file.
    if( !projToLoad.empty() )
    {
        wxFileName fn( projToLoad );

        if( fn.Exists() )
        {
            fn.MakeAbsolute();
            frame->LoadProject( fn );
        }
    }

    frame->Show( true );
    frame->Raise();

    return true;
}


void PGM_KICAD::OnPgmExit()
{
    Kiway.OnKiwayEnd();

    if( m_settings_manager && m_settings_manager->IsOK() )
    {
        SaveCommonSettings();
        m_settings_manager->Save();
    }

    // Destroy everything in PGM_KICAD,
    // especially wxSingleInstanceCheckerImpl earlier than wxApp and earlier
    // than static destruction would.
    Destroy();
}


void PGM_KICAD::MacOpenFile( const wxString& aFileName )
{
#if defined(__WXMAC__)

    KICAD_MANAGER_FRAME* frame = (KICAD_MANAGER_FRAME*) App().GetTopWindow();

    if( !aFileName.empty() && wxFileExists( aFileName ) )
        frame->LoadProject( wxFileName( aFileName ) );

#endif
}


void PGM_KICAD::Destroy()
{
    // unlike a normal destructor, this is designed to be called more
    // than once safely:

    m_bm.End();

    PGM_BASE::Destroy();
}


KIWAY  Kiway( &Pgm(), KFCTL_CPP_PROJECT_SUITE );


/**
 * Struct APP_KICAD
 * is not publicly visible because most of the action is in PGM_KICAD these days.
 */
struct APP_KICAD : public wxApp
{
#if defined (__LINUX__)
    APP_KICAD(): wxApp()
    {
        // Disable proxy menu in Unity window manager. Only usual menubar works with
        // wxWidgets (at least <= 3.1).  When the proxy menu menubar is enable, some
        // important things for us do not work: menuitems UI events and shortcuts.
        wxString wm;

        if( wxGetEnv( wxT( "XDG_CURRENT_DESKTOP" ), &wm ) && wm.CmpNoCase( wxT( "Unity" ) ) == 0 )
        {
            wxSetEnv ( wxT("UBUNTU_MENUPROXY" ), wxT( "0" ) );
        }

        // Force the use of X11 backend (or wayland-x11 compatibilty layer).  This is required until wxWidgets
        // supports the Wayland compositors
        wxSetEnv( wxT( "GDK_BACKEND" ), wxT( "x11" ) );

        // Disable overlay scrollbars as they mess up wxWidgets window sizing and cause excessive redraw requests
        wxSetEnv( wxT( "GTK_OVERLAY_SCROLLING" ), wxT( "0" ) );

        // Set GTK2-style input instead of xinput2.  This disables touchscreen and smooth scrolling
        // Needed to ensure that we are not getting multiple mouse scroll events
        wxSetEnv( wxT( "GDK_CORE_DEVICE_EVENTS" ), wxT( "1" ) );
    }
#endif

    bool OnInit()           override
    {
#if defined( _MSC_VER ) && defined( DEBUG )
        // wxWidgets turns on leak dumping in debug but its "flawed" and will falsely dump for half a hour
        // _CRTDBG_ALLOC_MEM_DF is the usual default for MSVC
        _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF );
#endif

#if defined( _WIN32 ) && defined( PYTHON_VERSION_MAJOR )                                           \
        && ( ( PYTHON_VERSION_MAJOR == 3 && PYTHON_VERSION_MINOR >= 8 )                            \
             || PYTHON_VERSION_MAJOR > 3 )

        // Python 3.8 switched to Windows 8+ API, we do not support Windows 7 and will not attempt to hack around it
        // Gracefully inform the user and refuse to start (because python will crash us if we continue)
        if( !IsWindows8OrGreater() )
        {
            wxMessageBox( _( "Windows 7 and older is no longer supported by KiCad and it's dependencies." ),
                          _( "Unsupported Operating System" ), wxOK | wxICON_ERROR );
            return false;
        }
#endif

        if( !program.OnPgmInit() )
        {
            program.OnPgmExit();
            return false;
        }

        return true;
    }

    int  OnExit()           override
    {
        program.OnPgmExit();

#if defined(__FreeBSD__)
        /* Avoid wxLog crashing when used in destructors. */
        wxLog::EnableLogging( false );
#endif

        return wxApp::OnExit();
    }

    int OnRun()             override
    {
        try
        {
            return wxApp::OnRun();
        }
        catch( const std::exception& e )
        {
            wxLogError( wxT( "Unhandled exception class: %s  what: %s" ),
                    FROM_UTF8( typeid( e ).name() ), FROM_UTF8( e.what() ) );
        }
        catch( const IO_ERROR& ioe )
        {
            wxLogError( ioe.What() );
        }
        catch(...)
        {
            wxLogError( wxT( "Unhandled exception of unknown type" ) );
        }

        return -1;
    }

    int FilterEvent( wxEvent& aEvent ) override
    {
        if( aEvent.GetEventType() == wxEVT_SHOW )
        {
            wxShowEvent& event = static_cast<wxShowEvent&>( aEvent );
            wxDialog*    dialog = dynamic_cast<wxDialog*>( event.GetEventObject() );

            if( dialog && dialog->IsModal() )
                Pgm().m_ModalDialogCount += event.IsShown() ? 1 : -1;
        }

        return Event_Skip;
    }

    /**
     * Set MacOS file associations.
     *
     * @see http://wiki.wxwidgets.org/WxMac-specific_topics
     */
#if defined( __WXMAC__ )
    void MacOpenFile( const wxString& aFileName ) override
    {
        Pgm().MacOpenFile( aFileName );
    }
#endif
};

IMPLEMENT_APP( APP_KICAD )


// The C++ project manager supports one open PROJECT, so Prj() calls within
// this link image need this function.
PROJECT& Prj()
{
    return Kiway.Prj();
}

