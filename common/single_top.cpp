/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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


/*

    This is a program launcher for a single KIFACE DSO. It only mimics a KIWAY,
    not actually implements one, since only a single DSO is supported by it.

    It is compiled multiple times, once for each standalone program and as such
    gets different compiler command line supplied #defines from CMake.

*/


#include <typeinfo>
#include <wx/cmdline.h>
#include <wx/dialog.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/snglinst.h>
#include <wx/html/htmlwin.h>

#include <kiway.h>
#include <build_version.h>
#include <pgm_base.h>
#include <kiway_player.h>
#include <macros.h>
#include <confirm.h>
#include <settings/kicad_settings.h>
#include <settings/settings_manager.h>
#include <paths.h>

#include <kiplatform/app.h>
#include <kiplatform/environment.h>

#include <git2.h>
#include <libraries/library_manager.h>

#ifdef KICAD_USE_SENTRY
#include <sentry.h>
#endif

#ifdef KICAD_IPC_API
#include <api/api_server.h>
#endif

// Only a single KIWAY is supported in this single_top top level component,
// which is dedicated to loading only a single DSO.
KIWAY    Kiway( KFCTL_STANDALONE );


// implement a PGM_BASE and a wxApp side by side:

/**
 * Implement PGM_BASE with its own OnPgmInit() and OnPgmExit().
 */
static struct PGM_SINGLE_TOP : public PGM_BASE
{
    bool OnPgmInit();

    void OnPgmExit()
    {

        Kiway.OnKiwayEnd();

#ifdef KICAD_IPC_API
        m_api_server.reset();
#endif

        if( m_settings_manager && m_settings_manager->IsOK() )
        {
            SaveCommonSettings();
            m_settings_manager->Save();
        }

        // Destroy everything in PGM_BASE, especially wxSingleInstanceCheckerImpl
        // earlier than wxApp and earlier than static destruction would.
        PGM_BASE::Destroy();
        git_libgit2_shutdown();
    }

    void MacOpenFile( const wxString& aFileName )   override
    {
        wxFileName  filename( aFileName );

        if( filename.FileExists() )
        {
    #if 0
            // this pulls in EDA_DRAW_FRAME type info, which we don't want in
            // the single_top link image.
            KIWAY_PLAYER* frame = dynamic_cast<KIWAY_PLAYER*>( App().GetTopWindow() );
    #else
            KIWAY_PLAYER* frame = (KIWAY_PLAYER*) App().GetTopWindow();
    #endif
            if( frame )
            {
                if( wxWindow* blocking_win = frame->Kiway().GetBlockingDialog() )
                    blocking_win->Close( true );

                frame->OpenProjectFiles( std::vector<wxString>( 1, aFileName ) );
            }
        }
    }

} program;


// A module to allow Html module initialization/cleanup
// When a wxHtmlWindow is used *only* in a dll/so module, the Html text is displayed
// as plain text.
// This helper class is just used to force wxHtmlWinParser initialization
// see https://groups.google.com/forum/#!topic/wx-users/FF0zv5qGAT0
class HtmlModule: public wxModule
{
public:
    HtmlModule() { }
    virtual bool OnInit() override { AddDependency( CLASSINFO( wxHtmlWinParser ) ); return true; };
    virtual void OnExit() override {};

private:
    wxDECLARE_DYNAMIC_CLASS( HtmlModule );
};

wxIMPLEMENT_DYNAMIC_CLASS(HtmlModule, wxModule);


#ifdef NDEBUG
// Define a custom assertion handler
void CustomAssertHandler( const wxString& file,
                          int line,
                          const wxString& func,
                          const wxString& cond,
                          const wxString& msg )
{
    Pgm().HandleAssert( file, line, func, cond, msg );
}
#endif


/**
 * Implement a bare naked wxApp (so that we don't become dependent on
 * functionality in a wxApp derivative that we cannot deliver under wxPython).
 */
struct APP_SINGLE_TOP : public wxApp
{
    APP_SINGLE_TOP() : wxApp()
    {
        SetPgm( &program );

        // Init the environment each platform wants
        KIPLATFORM::ENV::Init();
    }


    bool OnInit() override
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

        // Force wxHtmlWinParser initialization when a wxHtmlWindow is used only
        // in a shared library (.so or .dll file)
        // Otherwise the Html text is displayed as plain text.
        HtmlModule html_init;

        try
        {
            return program.OnPgmInit();
        }
        catch( ... )
        {
            Pgm().HandleException( std::current_exception() );
        }

        program.OnPgmExit();

        return false;
    }

    int  OnExit() override
    {
        program.OnPgmExit();
        return wxApp::OnExit();
    }

    int OnRun() override
    {
        int ret = -1;

        try
        {
            ret = wxApp::OnRun();
        }
        catch(...)
        {
            Pgm().HandleException( std::current_exception() );
        }

        return ret;
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
                else if( !event.IsShown() && !dlgs.empty()  )
                {
                    // If we close the expected dialog, remove it from our stack
                    if( dlgs.back() == dialog )
                        dlgs.pop_back();
                    // If an out-of-order, remove all dialogs added after the closed one
                    else if( auto it = std::find( dlgs.begin(), dlgs.end(), dialog ); it != dlgs.end() )
                        dlgs.erase( it, dlgs.end() );
                }
            }
        }

        return Event_Skip;
    }

#if defined( DEBUG )
    /**
     * Override main loop exception handling on debug builds.
     *
     * It can be painfully difficult to debug exceptions that happen in wxUpdateUIEvent
     * handlers.  The override provides a bit more useful information about the exception
     * and a breakpoint can be set to pin point the event where the exception was thrown.
     */
    virtual bool OnExceptionInMainLoop() override
    {
        try
        {
            throw;
        }
        catch( ... )
        {
            Pgm().HandleException( std::current_exception() );
        }

        return false;   // continue on. Return false to abort program
    }
#endif

#ifdef __WXMAC__

    /**
     * Specific to MacOSX (not used under Linux or Windows).
     *
     * MacOSX requires it for file association.
     *
     * @see http://wiki.wxwidgets.org/WxMac-specific_topics
     */
    void MacOpenFile( const wxString& aFileName ) override
    {
        Pgm().MacOpenFile( aFileName );
    }

#endif
};

IMPLEMENT_APP( APP_SINGLE_TOP )


bool PGM_SINGLE_TOP::OnPgmInit()
{
#if defined(DEBUG)
    wxString absoluteArgv0 = wxStandardPaths::Get().GetExecutablePath();

    if( !wxIsAbsolutePath( absoluteArgv0 ) )
    {
        wxLogError( wxT( "No meaningful argv[0]" ) );
        return false;
    }
#endif

    // Initialize the git library before trying to initialize individual programs
    int gitInit = git_libgit2_init();

    if( gitInit < 0 )
    {
        const git_error* err = git_error_last();
        wxString         msg = wxS( "Failed to initialize git library" );

        if( err && err->message )
            msg += wxS( ": " ) + wxString::FromUTF8( err->message );

        wxLogError( msg );
        return false;
    }

    // Not all KiCad applications use the python stuff. skip python init
    // for these apps.
    bool skip_python_initialization = false;

#if defined( BITMAP_2_CMP ) || defined( PL_EDITOR ) || defined( GERBVIEW ) || \
    defined( PCB_CALCULATOR_BUILD )
    skip_python_initialization = true;
#endif

    if( !InitPgm( false, skip_python_initialization ) )
    {
        // Clean up
        OnPgmExit();
        return false;
    }

#if !defined(BUILD_KIWAY_DLL)

    // Only bitmap2component and pcb_calculator use this code currently, as they
    // are not split to use single_top as a link image separate from a *.kiface.
    // i.e. they are single part link images so don't need to load a *.kiface.

    // Get the getter, it is statically linked into this binary image.
    KIFACE_GETTER_FUNC* ki_getter = &KIFACE_GETTER;

    int  kiface_version;

    // Get the KIFACE.
    KIFACE* kiface = ki_getter( &kiface_version, KIFACE_VERSION, this );

    // Trick the KIWAY into thinking it loaded a KIFACE, by recording the KIFACE
    // in the KIWAY.  It needs to be there for KIWAY::OnKiwayEnd() anyways.
    Kiway.set_kiface( KIWAY::KifaceType( TOP_FRAME ), kiface );
#endif

    // Tell the settings manager about the current Kiway
    GetSettingsManager().SetKiway( &Kiway );

    GetSettingsManager().RegisterSettings( new KICAD_SETTINGS );

    GetLibraryManager().LoadGlobalTables();

#ifdef KICAD_IPC_API
    // Create the API server thread once the app event loop exists
    m_api_server = std::make_unique<KICAD_API_SERVER>();
#endif

    // Use KIWAY to create a top window, which registers its existence also.
    // "TOP_FRAME" is a macro that is passed on compiler command line from CMake,
    // and is one of the types in FRAME_T.
    KIWAY_PLAYER* frame = Kiway.Player( TOP_FRAME, true );

    if( frame == nullptr )
    {
        // Clean up
        OnPgmExit();
        return false;
    }

    Kiway.SetTop( frame );

    App().SetTopWindow( frame );      // wxApp gets a face.
    App().SetAppDisplayName( frame->GetAboutTitle() );

    wxString relaunchDisplayName = frame->GetAboutTitle() + " " + GetMajorMinorVersion();
    KIPLATFORM::ENV::SetAppDetailsForWindow( frame, PATHS::GetExecutablePath(), relaunchDisplayName );

    // Allocate a slice of time to show the frame and update wxWidgets widgets
    // (especially setting valid sizes) after creating frame and before calling
    // OpenProjectFiles() that can update/use some widgets.
    // The 2 calls to wxSafeYield are needed on wxGTK for best results.
    wxSafeYield();
    HideSplash();
    frame->Show();
    wxSafeYield();

    // Now after the frame processing, the rest of the positional args are files
    std::vector<wxString> fileArgs;


    static const wxCmdLineEntryDesc desc[] = {
        { wxCMD_LINE_PARAM, nullptr, nullptr, "File to load", wxCMD_LINE_VAL_STRING,
          wxCMD_LINE_PARAM_MULTIPLE | wxCMD_LINE_PARAM_OPTIONAL },
        { wxCMD_LINE_NONE, nullptr, nullptr, nullptr, wxCMD_LINE_VAL_NONE, 0 }
    };

    wxCmdLineParser parser( App().argc, App().argv );
    parser.SetDesc( desc );
    parser.Parse( false );

    if( parser.GetParamCount() )
    {
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

#if defined(PGM_DATA_FILE_EXT)
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

        frame->OpenProjectFiles( fileArgs );
    }

#ifdef KICAD_IPC_API
    m_api_server->SetReadyToReply();
#endif

    return true;
}
