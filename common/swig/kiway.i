
//%module(directors="1") kiway
%module kiway


%import(module="wx")  wx_kiway_player_hierarchy.h

%include ki_exception.i     // affects all that follow it

/*

By default we do not translate exceptions for EVERY C++ function since not every
C++ function throws, and that would be unused and very bulky mapping code.
Therefore please help gather the subset of C++ functions for this class that do
throw and add them here, before the class declarations.

*/
HANDLE_EXCEPTIONS(KIWAY::Player)

%include pgm_base.h
%include frame_type.h
%include mail_type.h
%include project.h
%include kiway.h
%include kiway_express.h

%include kiway_player.i



%constant KIWAY    Kiway;

%pythoncode
%{
#import wx
%}


%{
#include <kiway.h>
#include <kiway_express.h>
#include <pgm_base.h>

#include <wx/app.h>
#include <wx/stdpaths.h>
#include <wx/wxPython/wxPython_int.h>


// Only a single KIWAY is supported in this single_top top level component,
// which is dedicated to loading only a single DSO.
KIWAY    Kiway( &Pgm(), KFCTL_PY_PROJECT_SUITE );


// a dummy to quiet linking with EDA_BASE_FRAME::config();
#include <kiface_i.h>
KIFACE_I& Kiface()
{
    // This function should never be called.  It is only referenced from
    // EDA_BASE_FRAME::config() and this is only provided to satisfy the linker,
    // not to be actually called.
    wxLogFatalError( wxT( "Unexpected call to Kiface() in kicad/kicad.cpp" ) );

    return (KIFACE_I&) *(KIFACE_I*) 0;
}


/**
 * Struct PGM_PYTHON
 * implements PGM_BASE with its own OnPgmInit() and OnPgmExit().
 */
static struct PGM_PYTHON : public PGM_BASE
{
#if 0
    bool OnPgmInit( wxApp* aWxApp )
    {
        // first thing: set m_wx_app
        SetApp( aWxApp );

        if( !initPgm() )
            return false;

        // Use KIWAY to create a top window, which registers its existence also.
        // "TOP_FRAME" is a macro that is passed on compiler command line from CMake,
        // and is one of the types in FRAME_T.
        KIWAY_PLAYER* frame = Kiway.Player( FRAME_PCB, true );

        Kiway.SetTop( frame );

        App().SetTopWindow( frame );      // wxApp gets a face.

        // Open project or file specified on the command line:
        int argc = App().argc;

        if( argc > 1 )
        {
            /*
                gerbview handles multiple project data files, i.e. gerber files on
                cmd line. Others currently do not, they handle only one. For common
                code simplicity we simply pass all the arguments in however, each
                program module can do with them what they want, ignore, complain
                whatever.  We don't establish policy here, as this is a multi-purpose
                launcher.
            */

            std::vector<wxString>   argSet;

            for( int i=1;  i<argc;  ++i )
            {
                argSet.push_back( App().argv[i] );
            }

            // special attention to the first argument: argv[1] (==argSet[0])
            wxFileName argv1( argSet[0] );

            if( argc == 2 )
            {
#if defined(PGM_DATA_FILE_EXT)
                // PGM_DATA_FILE_EXT, if present, may be different for each compile,
                // it may come from CMake on the compiler command line, but often does not.
                // This facillity is mostly useful for those program modules
                // supporting a single argv[1].
                if( !argv1.GetExt() )
                    argv1.SetExt( wxT( PGM_DATA_FILE_EXT ) );
#endif
                argv1.MakeAbsolute();

                argSet[0] = argv1.GetFullPath();
            }

            // Use the KIWAY_PLAYER::OpenProjectFiles() API function:
            if( !frame->OpenProjectFiles( argSet ) )
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

        frame->Show();

        return true;
    }

    void OnPgmExit()
    {
        Kiway.OnKiwayEnd();

        saveCommonSettings();

        // Destroy everything in PGM_BASE, especially wxSingleInstanceCheckerImpl
        // earlier than wxApp and earlier than static destruction would.
        PGM_BASE::destroy();
    }
#endif


#if 0   // multi-project
    void PGM_KICAD::MacOpenFile( const wxString& aFileName )
    {
    #if defined(__WXMAC__)

        KICAD_MANAGER_FRAME* frame = (KICAD_MANAGER_FRAME*) App().GetTopWindow();

        frame->SetProjectFileName( aFileName );

        wxCommandEvent loadEvent( 0, wxID_ANY );

        frame->OnLoadProject( loadEvent );
    #endif
    }

#else
    void MacOpenFile( const wxString& aFileName )   override
    {
        wxFileName  filename( aFileName );

        if( filename.FileExists() )
        {
#if 0
            // this pulls in EDA_DRAW_FRAME type info, which we don't want in
            // the link image.
            KIWAY_PLAYER* frame = dynamic_cast<KIWAY_PLAYER*>( App().GetTopWindow() );
#else
            KIWAY_PLAYER* frame = (KIWAY_PLAYER*) App().GetTopWindow();
#endif
            if( frame )
                frame->OpenProjectFiles( std::vector<wxString>( 1, aFileName ) );
        }
    }
#endif

} program;


PGM_BASE& Pgm()
{
    return program;
}

%}

/*
import ctypes, os, sys
libcef_so = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'libcef.so')
if os.path.exists(libcef_so):
*/

%extend PGM_BASE {

    %pythoncode
    %{

    def OnPgmInit(self):
        print "hereA"

        if not self.InitPgm():
            return False;

        print "hereB"

        try:
            # A KIWAY_PLAYER is a wx.Window
            frame = Kiway.Player( FRAME_SCH, True )

            print "here0"

        except IOError as e:
            print 'Player()', e
            return None

        print "here1"

        Kiway.SetTop(frame)

        print "here2"

        return frame
    %}
};

