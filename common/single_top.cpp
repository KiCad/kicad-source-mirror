/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2014-2020 KiCad Developers, see CHANGELOG.TXT for contributors.
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
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/snglinst.h>
#include <wx/html/htmlwin.h>

#include <kiway.h>
#include <pgm_base.h>
#include <kiway_player.h>
#include <macros.h>
#include <confirm.h>
#include <settings/settings_manager.h>

#include <kiplatform/app.h>


// Only a single KIWAY is supported in this single_top top level component,
// which is dedicated to loading only a single DSO.
KIWAY    Kiway( &Pgm(), KFCTL_STANDALONE );


// implement a PGM_BASE and a wxApp side by side:

/**
 * Struct PGM_SINGLE_TOP
 * implements PGM_BASE with its own OnPgmInit() and OnPgmExit().
 */
static struct PGM_SINGLE_TOP : public PGM_BASE
{
    bool OnPgmInit();

    void OnPgmExit()
    {
        Kiway.OnKiwayEnd();

        if( m_settings_manager && m_settings_manager->IsOK() )
        {
            SaveCommonSettings();
            m_settings_manager->Save();
        }

        // Destroy everything in PGM_BASE, especially wxSingleInstanceCheckerImpl
        // earlier than wxApp and earlier than static destruction would.
        PGM_BASE::Destroy();
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
                frame->OpenProjectFiles( std::vector<wxString>( 1, aFileName ) );
        }
    }

} program;


PGM_BASE& Pgm()
{
    return program;
}

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

/**
 * Struct APP_SINGLE_TOP
 * implements a bare naked wxApp (so that we don't become dependent on
 * functionality in a wxApp derivative that we cannot deliver under wxPython).
 */
struct APP_SINGLE_TOP : public wxApp
{
    bool OnInit() override
    {
        // Init the platform-specific parts
        if( !KIPLATFORM::APP::PlatformInit() )
            return false;

        // Force wxHtmlWinParser initialization when a wxHtmlWindow is used only
        // in a shared library (.so or .dll file)
        // Otherwise the Html text is displayed as plain text.
        HtmlModule html_init;

        try
        {
            return program.OnPgmInit();
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

        program.OnPgmExit();

        return false;
    }

    int  OnExit() override
    {
        // Fixes segfault when wxPython scripting is enabled.
#if defined( KICAD_SCRIPTING_WXPYTHON )
        program.OnPgmExit();
#endif
        return wxApp::OnExit();
    }

    int OnRun() override
    {
        int ret = -1;

        try
        {
            ret = wxApp::OnRun();
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

        // Works properly when wxPython scripting is disabled.
#if !defined( KICAD_SCRIPTING_WXPYTHON )
        program.OnPgmExit();
#endif
        return ret;
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

#ifdef __WXMAC__

    /**
     * Function MacOpenFile
     * is specific to MacOSX (not used under Linux or Windows).
     * MacOSX requires it for file association.
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

    if( !InitPgm() )
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
    KIFACE_GETTER_FUNC* getter = &KIFACE_GETTER;

    int  kiface_version;

    // Get the KIFACE.
    KIFACE* kiface = getter( &kiface_version, KIFACE_VERSION, this );

    // Trick the KIWAY into thinking it loaded a KIFACE, by recording the KIFACE
    // in the KIWAY.  It needs to be there for KIWAY::OnKiwayEnd() anyways.
    Kiway.set_kiface( KIWAY::KifaceType( TOP_FRAME ), kiface );
#endif

    static const wxCmdLineEntryDesc desc[] = {
        { wxCMD_LINE_OPTION, "f", "frame", "Frame to load", wxCMD_LINE_VAL_STRING, 0 },
        { wxCMD_LINE_PARAM, nullptr, nullptr, "File to load", wxCMD_LINE_VAL_STRING,
                wxCMD_LINE_PARAM_MULTIPLE | wxCMD_LINE_PARAM_OPTIONAL },
        { wxCMD_LINE_NONE, nullptr, nullptr, nullptr, wxCMD_LINE_VAL_NONE, 0 }
    };

    wxCmdLineParser parser( App().argc, App().argv );
    parser.SetDesc( desc );
    parser.Parse( false );

    FRAME_T appType = TOP_FRAME;

    const struct
    {
        wxString name;
        FRAME_T type;
    } frameTypes[] = {
        { wxT( "pcb" ),    FRAME_PCB_EDITOR },
        { wxT( "fpedit" ), FRAME_FOOTPRINT_EDITOR },
        { wxT( "" ),       FRAME_T_COUNT }
    };

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

    // Tell the settings manager about the current Kiway
    GetSettingsManager().SetKiway( &Kiway );

    // Use KIWAY to create a top window, which registers its existence also.
    // "TOP_FRAME" is a macro that is passed on compiler command line from CMake,
    // and is one of the types in FRAME_T.
    KIWAY_PLAYER* frame = Kiway.Player( appType, true );

    if( frame == nullptr )
    {
        return false;
    }

    Kiway.SetTop( frame );

    App().SetTopWindow( frame );      // wxApp gets a face.
    App().SetAppDisplayName( frame->GetAboutTitle() );

    // Allocate a slice of time to show the frame and update wxWidgets widgets
    // (especially setting valid sizes) after creating frame and before calling
    // OpenProjectFiles() that can update/use some widgets.
    // The 2 calls to wxSafeYield are needed on wxGTK for best results.
    wxSafeYield();
    frame->Show();
    wxSafeYield();

    // Individual frames may provide additional option/switch processing, but for compatibility,
    // any positional arguments are treated as a list of files to pass to OpenProjectFiles
    frame->ParseArgs( parser );

    // Now after the frame processing, the rest of the positional args are files
    std::vector<wxString> fileArgs;

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

        // Use the KIWAY_PLAYER::OpenProjectFiles() API function:
        if( !frame->OpenProjectFiles( fileArgs ) )
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

    return true;
}
