/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <filehistory.h>
#include <hotkeys_basic.h>
#include <kiway.h>
#include <macros.h>
#include <paths.h>
#include <richio.h>
#include <settings/settings_manager.h>
#include <settings/kicad_settings.h>
#include <systemdirsappend.h>
#include <trace_helpers.h>
#include <wildcards_and_files_ext.h>

#include <stdexcept>

#include "pgm_kicad.h"
#include "kicad_manager_frame.h"

#include <kicad_build_version.h>
#include <kiplatform/app.h>
#include <kiplatform/environment.h>

#include "cli/command_export_kicad_pcbnew.h"
#include "cli/command_export_step.h"
#include "cli/exit_codes.h"

// a dummy to quiet linking with EDA_BASE_FRAME::config();
#include <kiface_base.h>
KIFACE_BASE& Kiface()
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

struct COMMAND_ENTRY
{
    CLI::COMMAND* handler;

    std::vector<COMMAND_ENTRY> subCommands;

    COMMAND_ENTRY( CLI::COMMAND* aHandler ) : handler( aHandler ){};
    COMMAND_ENTRY( CLI::COMMAND* aHandler, std::vector<COMMAND_ENTRY> aSub ) :
            handler( aHandler ), subCommands( aSub ){};
};

static CLI::EXPORT_STEP_COMMAND   stepCmd{};
static CLI::EXPORT_KICAD_PCBNEW_COMMAND exportPcbCmd{};

static std::vector<COMMAND_ENTRY> commandStack = { { &exportPcbCmd, { &stepCmd } } };

bool PGM_KICAD::OnPgmInit()
{
    PGM_BASE::BuildArgvUtf8();
    App().SetAppDisplayName( wxT( "KiCad" ) );

#if defined(DEBUG)
    wxString absoluteArgv0 = wxStandardPaths::Get().GetExecutablePath();

    if( !wxIsAbsolutePath( absoluteArgv0 ) )
    {
        wxLogError( wxT( "No meaningful argv[0]" ) );
        return false;
    }
#endif

    argparse::ArgumentParser argParser( std::string( "kicad" ),
                                        KICAD_MAJOR_MINOR_VERSION );

    for( COMMAND_ENTRY& entry : commandStack )
    {
        argParser.add_subparser( entry.handler->GetArgParser() );

        for( COMMAND_ENTRY& subentry : entry.subCommands )
        {
            entry.handler->GetArgParser().add_subparser( subentry.handler->GetArgParser() );
        }
    }

    try
    {
        argParser.parse_args( m_argcUtf8, m_argvUtf8 );
    }
    catch( const std::runtime_error& err )
    {
        // Ignore any argParser "errors"
        // unforunately there are cases like the only arg being a file (double click open)
        // that we need to fall through
    }

    bool          cliCmdRequested = false;
    CLI::COMMAND* cliCmd = nullptr;
    for( COMMAND_ENTRY& entry : commandStack )
    {
        if( argParser.is_subcommand_used( entry.handler->GetName() ) )
        {
            for( COMMAND_ENTRY& subentry : entry.subCommands )
            {
                if( entry.handler->GetArgParser().is_subcommand_used(
                            subentry.handler->GetName() ) )
                {
                    cliCmd = subentry.handler;
                    cliCmdRequested = true;
                }
            }

            if( !cliCmdRequested )
            {
                cliCmd = entry.handler;
            }
        }
    }

    if( !InitPgm() )
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

        // The KICAD6_TEMPLATE_DIR takes precedence over the search stack template path.
        ENV_VAR_MAP_CITER it = GetLocalEnvVariables().find( "KICAD6_TEMPLATE_DIR" );

        if( it != GetLocalEnvVariables().end() && it->second.GetValue() != wxEmptyString )
            m_bm.m_search.Insert( it->second.GetValue(), 0 );

        // We've been adding system (installed default) search paths so far, now for user paths
        // The default user search path is inside KIPLATFORM::ENV::GetDocumentsPath()
        m_bm.m_search.Insert( PATHS::GetUserTemplatesPath(), 0 );

        // ...but the user can override that default with the KICAD_USER_TEMPLATE_DIR env var
        it = GetLocalEnvVariables().find( "KICAD_USER_TEMPLATE_DIR" );

        if( it != GetLocalEnvVariables().end() && it->second.GetValue() != wxEmptyString )
            m_bm.m_search.Insert( it->second.GetValue(), 0 );
    }

    if( cliCmdRequested )
    {
        int exitCode = CLI::EXIT_CODES::ERR_UNKNOWN;
        if( cliCmd )
        {
            exitCode = cliCmd->Perform( Kiway );
        }

        if( exitCode != CLI::EXIT_CODES::AVOID_CLOSING )
        {
            std::exit( exitCode );
        }
        else
        {
            return true;
        }
    }

    KICAD_MANAGER_FRAME* frame = new KICAD_MANAGER_FRAME( nullptr, wxT( "KiCad" ),
                                                          wxDefaultPosition, wxSize( 775, -1 ) );
    App().SetTopWindow( frame );

    Kiway.SetTop( frame );

    KICAD_SETTINGS* settings = static_cast<KICAD_SETTINGS*>( PgmSettings() );

    wxString projToLoad;

    if( App().argc > 1 )
    {
        wxFileName tmp = App().argv[1];

        if( tmp.GetExt() != ProjectFileExtension && tmp.GetExt() != LegacyProjectFileExtension )
        {
            wxString msg;

            msg.Printf( _( "File '%s'\ndoes not appear to be a valid KiCad project file." ),
                        tmp.GetFullPath() );
            wxMessageDialog dlg( nullptr, msg, _( "Error" ), wxOK | wxICON_EXCLAMATION );
            dlg.ShowModal();
        }
        else
        {
            projToLoad = tmp.GetFullPath();
        }
    }

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
 * Not publicly visible because most of the action is in #PGM_KICAD these days.
 */
struct APP_KICAD : public wxApp
{
    APP_KICAD() : wxApp()
    {
        // Init the environment each platform wants
        KIPLATFORM::ENV::Init();
    }


    bool OnInit()           override
    {
        // Perform platform-specific init tasks
        if( !KIPLATFORM::APP::Init() )
            return false;

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
        // Avoid wxLog crashing when used in destructors.
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
        catch( const std::exception& e )
        {
            wxLogError( "Unhandled exception class: %s  what: %s",
                        FROM_UTF8( typeid(e).name() ),
                        FROM_UTF8( e.what() ) );
        }
        catch( const IO_ERROR& ioe )
        {
            wxLogError( ioe.What() );
        }
        catch(...)
        {
            wxLogError( "Unhandled exception of unknown type" );
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

