/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

/**
 * @file kicad.cpp
 * Main KiCad project manager file.
 */


#include <wx/filename.h>
#include <wx/log.h>
#include <wx/app.h>
#include <wx/stdpaths.h>
#include <wx/msgdlg.h>
#include <wx/cmdline.h>

#include <env_vars.h>
#include <file_history.h>
#include <hotkeys_basic.h>
#include <kiway.h>
#include <macros.h>
#include <paths.h>
#include <richio.h>
#include <settings/settings_manager.h>
#include <settings/kicad_settings.h>
#include <../include/startwizard/startwizard.h>
#include <systemdirsappend.h>
#include <thread_pool.h>
#include <trace_helpers.h>
#include <wildcards_and_files_ext.h>
#include <confirm.h>

#include <git/git_backend.h>
#include <git/libgit_backend.h>
#include <stdexcept>

#include "pgm_kicad.h"
#include "kicad_manager_frame.h"

#include <kiplatform/app.h>
#include <kiplatform/environment.h>

#ifdef KICAD_IPC_API
#include <api/api_server.h>
#endif

// a dummy to quiet linking with EDA_BASE_FRAME::config();
#include <kiface_base.h>

#include <libraries/library_manager.h>


KIFACE_BASE& Kiface()
{
    // This function should never be called.  It is only referenced from
    // EDA_BASE_FRAME::config() and this is only provided to satisfy the linker,
    // not to be actually called.
    wxLogFatalError( wxT( "Unexpected call to Kiface() in kicad/kicad.cpp" ) );

    throw std::logic_error( "Unexpected call to Kiface() in kicad/kicad.cpp" );
}


static PGM_KICAD program;

PGM_KICAD& PgmTop()
{
    return program;
}


bool PGM_KICAD::OnPgmInit()
{
    App().SetAppDisplayName( wxT( "KiCad" ) );

#if defined(DEBUG)
    wxString absoluteArgv0 = wxStandardPaths::Get().GetExecutablePath();

    if( !wxIsAbsolutePath( absoluteArgv0 ) )
    {
        wxLogError( wxT( "No meaningful argv[0]" ) );
        return false;
    }
#endif

    // Initialize the git backend before trying to initialize individual programs
    SetGitBackend( new LIBGIT_BACKEND() );
    GetGitBackend()->Init();

    static const wxCmdLineEntryDesc desc[] = {
        { wxCMD_LINE_OPTION, "f", "frame", "Frame to load", wxCMD_LINE_VAL_STRING, 0 },
        { wxCMD_LINE_SWITCH, "n", "new", "New instance of KiCad, does not attempt to load previously open files",
          wxCMD_LINE_VAL_NONE, 0 },
#ifndef __WXOSX__
        { wxCMD_LINE_SWITCH, nullptr, "software-rendering", "Use software rendering instead of OpenGL",
          wxCMD_LINE_VAL_NONE, 0 },
#endif
        { wxCMD_LINE_PARAM, nullptr, nullptr, "File to load", wxCMD_LINE_VAL_STRING,
          wxCMD_LINE_PARAM_MULTIPLE | wxCMD_LINE_PARAM_OPTIONAL },
        { wxCMD_LINE_NONE, nullptr, nullptr, nullptr, wxCMD_LINE_VAL_NONE, 0 }
    };

    wxCmdLineParser parser( App().argc, App().argv );
    parser.SetDesc( desc );
    parser.Parse( false );

    FRAME_T appType = KICAD_MAIN_FRAME_T;

    const struct
    {
        wxString name;
        FRAME_T  type;
    } frameTypes[] = { { wxT( "pcb" ), FRAME_PCB_EDITOR },
                       { wxT( "fpedit" ), FRAME_FOOTPRINT_EDITOR },
                       { wxT( "sch" ), FRAME_SCH },
                       { wxT( "calc" ), FRAME_CALC },
                       { wxT( "bm2cmp" ), FRAME_BM2CMP },
                       { wxT( "ds" ), FRAME_PL_EDITOR },
                       { wxT( "gerb" ), FRAME_GERBER },
                       { wxT( "" ), FRAME_T_COUNT } };

    wxString frameName;

    if( parser.Found( "frame", &frameName ) )
    {
        appType = FRAME_T_COUNT;

        for( const auto& it : frameTypes )
        {
            if( it.name == frameName )
                appType = it.type;
        }

        if( appType == FRAME_T_COUNT )
        {
            wxLogError( wxT( "Unknown frame: %s" ), frameName );
            // Clean up
            OnPgmExit();
            return false;
        }
    }

    if( appType == KICAD_MAIN_FRAME_T )
    {
        Kiway.SetCtlBits( KFCTL_CPP_PROJECT_SUITE );
    }
    else
    {
        Kiway.SetCtlBits( KFCTL_STANDALONE );
    }

#ifndef __WXMAC__
    if( parser.Found( "software-rendering" ) )
    {
        wxSetEnv( "KICAD_SOFTWARE_RENDERING", "1" );
    }
#endif

    bool skipPythonInit = false;

    if( appType == FRAME_BM2CMP || appType == FRAME_PL_EDITOR || appType == FRAME_GERBER
        || appType == FRAME_CALC )
        skipPythonInit = true;

    if( !InitPgm( false, skipPythonInit ) )
        return false;


    m_bm.InitSettings( new KICAD_SETTINGS );
    GetSettingsManager().RegisterSettings( PgmSettings() );
    GetSettingsManager().SetKiway( &Kiway );
    m_bm.Init();


    // Add search paths to feed the PGM_KICAD::SysSearch() function,
    // currently limited in support to only look for project templates
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

        // The versioned TEMPLATE_DIR takes precedence over the search stack template path.
        if( std::optional<wxString> v = ENV_VAR::GetVersionedEnvVarValue( GetLocalEnvVariables(),
                                                                          wxT( "TEMPLATE_DIR" ) ) )
        {
            if( !v->IsEmpty() )
                m_bm.m_search.Insert( *v, 0 );
        }

        // We've been adding system (installed default) search paths so far, now for user paths
        // The default user search path is inside KIPLATFORM::ENV::GetDocumentsPath()
        m_bm.m_search.Insert( PATHS::GetUserTemplatesPath(), 0 );

        // ...but the user can override that default with the KICAD_USER_TEMPLATE_DIR env var
        ENV_VAR_MAP_CITER it = GetLocalEnvVariables().find( "KICAD_USER_TEMPLATE_DIR" );

        if( it != GetLocalEnvVariables().end() && it->second.GetValue() != wxEmptyString )
            m_bm.m_search.Insert( it->second.GetValue(), 0 );
    }

    wxFrame*      frame = nullptr;
    KIWAY_PLAYER* playerFrame = nullptr;
    KICAD_MANAGER_FRAME* managerFrame = nullptr;

    if( appType == KICAD_MAIN_FRAME_T )
    {
        managerFrame = new KICAD_MANAGER_FRAME( nullptr, wxT( "KiCad" ), wxDefaultPosition,
                                                wxWindow::FromDIP( wxSize( 775, -1 ), NULL ) );
        frame = managerFrame;

        STARTWIZARD startWizard;
        startWizard.CheckAndRun( frame );
    }
    else
    {
        // Use KIWAY to create a top window, which registers its existence also.
        // "TOP_FRAME" is a macro that is passed on compiler command line from CMake,
        // and is one of the types in FRAME_T.
        playerFrame = Kiway.Player( appType, true );
        frame = playerFrame;

        if( frame == nullptr )
        {
            return false;
        }
    }

    App().SetTopWindow( frame );

    if( playerFrame )
        App().SetAppDisplayName( playerFrame->GetAboutTitle() );

    Kiway.SetTop( frame );

    KIPLATFORM::ENV::SetAppDetailsForWindow( frame, '"' + wxStandardPaths::Get().GetExecutablePath() + '"' + " -n",
                                             frame->GetTitle() );

    KICAD_SETTINGS* settings = static_cast<KICAD_SETTINGS*>( PgmSettings() );

    GetLibraryManager().LoadGlobalTables();

#ifdef KICAD_IPC_API
    m_api_server = std::make_unique<KICAD_API_SERVER>();
    m_api_common_handler = std::make_unique<API_HANDLER_COMMON>();
    m_api_server->RegisterHandler( m_api_common_handler.get() );
#endif

    wxString projToLoad;

    HideSplash();

    if( playerFrame && parser.GetParamCount() )
    {
        // Now after the frame processing, the rest of the positional args are files
        std::vector<wxString> fileArgs;
        /*
            gerbview handles multiple project data files, i.e. gerber files on
            cmd line. Others currently do not, they handle only one. For common
            code simplicity we simply pass all the arguments in however, each
            program module can do with them what they want, ignore, complain
            whatever.  We don't establish policy here, as this is a multi-purpose
            launcher.
        */

        for( size_t i = 0; i < parser.GetParamCount(); i++ )
            fileArgs.push_back( parser.GetParam( i ) );

        // special attention to a single argument: argv[1] (==argSet[0])
        if( fileArgs.size() == 1 )
        {
            wxFileName argv1( fileArgs[0] );

#if defined( PGM_DATA_FILE_EXT )
            // PGM_DATA_FILE_EXT, if present, may be different for each compile,
            // it may come from CMake on the compiler command line, but often does not.
            // This facility is mostly useful for those program footprints
            // supporting a single argv[1].
            if( !argv1.GetExt() )
                argv1.SetExt( wxT( PGM_DATA_FILE_EXT ) );
#endif
            argv1.MakeAbsolute();

            fileArgs[0] = argv1.GetFullPath();
        }

        // Use the KIWAY_PLAYER::OpenProjectFiles() API function:
        if( !playerFrame->OpenProjectFiles( fileArgs ) )
        {
            // OpenProjectFiles() API asks that it report failure to the UI.
            // Nothing further to say here.

            // We've already initialized things at this point, but wx won't call OnExit if
            // we fail out. Call our own cleanup routine here to ensure the relevant resources
            // are freed at the right time (if they aren't, segfaults will occur).
            OnPgmExit();

            // Fail the process startup if the file could not be opened,
            // although this is an optional choice, one that can be reversed
            // also in the KIFACE specific OpenProjectFiles() return value.
            return false;
        }
    }
    else if( managerFrame )
    {
        if( parser.GetParamCount() > 0 )
        {
            wxFileName tmp = parser.GetParam( 0 );

            if( tmp.GetExt() != FILEEXT::ProjectFileExtension && tmp.GetExt() != FILEEXT::LegacyProjectFileExtension )
            {
                DisplayErrorMessage( nullptr, wxString::Format( _( "File '%s'\n"
                                                                   "does not appear to be a KiCad project file." ),
                                                                tmp.GetFullPath() ) );
            }
            else
            {
                projToLoad = tmp.GetFullPath();
            }
        }

        // If no file was given as an argument, check that there was a file open.
        if( projToLoad.IsEmpty() && settings->m_OpenProjects.size() && !parser.FoundSwitch( "new" ) )
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

            if( fn.Exists() && (   fn.GetExt() == FILEEXT::ProjectFileExtension
                                || fn.GetExt() == FILEEXT::LegacyProjectFileExtension ) )
            {
                fn.MakeAbsolute();

                if( appType == KICAD_MAIN_FRAME_T )
                    managerFrame->LoadProject( fn );
            }
        }
    }

    frame->Show( true );
    frame->Raise();

#ifdef KICAD_IPC_API
    m_api_server->SetReadyToReply();
#endif

    return true;
}


int PGM_KICAD::OnPgmRun()
{
    return 0;
}


void PGM_KICAD::OnPgmExit()
{
    // Abort and wait on any background jobs
    GetKiCadThreadPool().purge();
    GetKiCadThreadPool().wait();

    Kiway.OnKiwayEnd();

#ifdef KICAD_IPC_API
    m_api_server.reset();
#endif

    if( m_settings_manager && m_settings_manager->IsOK() )
    {
        SaveCommonSettings();
        m_settings_manager->Save();
    }

    // Destroy everything in PGM_KICAD,
    // especially wxSingleInstanceCheckerImpl earlier than wxApp and earlier
    // than static destruction would.
    Destroy();
    GetGitBackend()->Shutdown();
    delete GetGitBackend();
    SetGitBackend( nullptr );
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


KIWAY  Kiway( KFCTL_CPP_PROJECT_SUITE );

#ifdef NDEBUG
// Define a custom assertion handler
void CustomAssertHandler(const wxString& file,
                         int line,
                         const wxString& func,
                         const wxString& cond,
                         const wxString& msg)
{
    Pgm().HandleAssert( file, line, func, cond, msg );
}
#endif

/**
 * Not publicly visible because most of the action is in #PGM_KICAD these days.
 */
struct APP_KICAD : public wxApp
{
    APP_KICAD() : wxApp()
    {
        SetPgm( &program );

        // Init the environment each platform wants
        KIPLATFORM::ENV::Init();
    }


    bool OnInit()           override
    {
#ifdef NDEBUG
        // These checks generate extra assert noise
        wxSizerFlags::DisableConsistencyChecks();
        wxDISABLE_DEBUG_SUPPORT();
        wxSetAssertHandler( CustomAssertHandler );
#endif

        // Perform platform-specific init tasks
        if( !KIPLATFORM::APP::Init() )
            return false;

#ifndef DEBUG
        // Enable logging traces to the console in release build.
        // This is usually disabled, but it can be useful for users to run to help
        // debug issues and other problems.
        if( wxGetEnv( wxS( "KICAD_ENABLE_WXTRACE" ), nullptr ) )
        {
            wxLog::EnableLogging( true );
            wxLog::SetLogLevel( wxLOG_Trace );
        }
#endif

        if( !program.OnPgmInit() )
        {
            program.OnPgmExit();
            return false;
        }

        return true;
    }

    int OnExit() override
    {
        program.OnPgmExit();

        // Avoid wxLog crashing when used in destructors.
        wxLog::EnableLogging( false );

        return wxApp::OnExit();
    }


    int OnRun() override
    {
        try
        {
            return wxApp::OnRun();
        }
        catch(...)
        {
            Pgm().HandleException( std::current_exception() );
        }

        return -1;
    }


    void OnUnhandledException() override
    {
        Pgm().HandleException( std::current_exception(), true );
    }


    int FilterEvent( wxEvent& aEvent ) override
    {
        if( aEvent.GetEventType() == wxEVT_SHOW )
        {
            wxShowEvent& event = static_cast<wxShowEvent&>( aEvent );
            wxDialog*    dialog = dynamic_cast<wxDialog*>( event.GetEventObject() );

            std::vector<void*>& dlgs = Pgm().m_ModalDialogs;

            if( dialog )
            {
                if( event.IsShown() && dialog->IsModal() )
                {
                    dlgs.push_back( dialog );
                }
                // Under GTK, sometimes the modal flag is cleared before hiding
                else if( !event.IsShown() && !dlgs.empty() )
                {
                    // If we close the expected dialog, remove it from our stack
                    if( dlgs.back() == dialog )
                        dlgs.pop_back();
                    // If an out-of-order, remove all dialogs added after the closed one
                    else if( auto it = std::find( dlgs.begin(), dlgs.end(), dialog ) ; it != dlgs.end() )
                        dlgs.erase( it, dlgs.end() );
                }
            }
        }

        return Event_Skip;
    }

#if defined( DEBUG )
    /**
     * Process any unhandled events at the application level.
     */
    bool ProcessEvent( wxEvent& aEvent ) override
    {
        if( aEvent.GetEventType() == wxEVT_CHAR || aEvent.GetEventType() == wxEVT_CHAR_HOOK )
        {
            wxKeyEvent* keyEvent = static_cast<wxKeyEvent*>( &aEvent );

            if( keyEvent )
            {
                wxLogTrace( kicadTraceKeyEvent, "APP_KICAD::ProcessEvent %s", dump( *keyEvent ) );
            }
        }

        aEvent.Skip();
        return false;
    }

    /**
     * Override main loop exception handling on debug builds.
     *
     * It can be painfully difficult to debug exceptions that happen in wxUpdateUIEvent
     * handlers.  The override provides a bit more useful information about the exception
     * and a breakpoint can be set to pin point the event where the exception was thrown.
     */
    bool OnExceptionInMainLoop() override
    {
        try
        {
            throw;
        }
        catch(...)
        {
            Pgm().HandleException( std::current_exception() );
        }

        return false;   // continue on. Return false to abort program
    }
#endif

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

