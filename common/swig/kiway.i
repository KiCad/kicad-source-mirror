
//%module(directors="1") kiway
%module kiway


%include ki_exception.i     // affects all that follow it

/*

By default we do not translate exceptions for EVERY C++ function since not every
C++ function throws, and that would be unused and very bulky mapping code.
Therefore please help gather the subset of C++ functions for this class that do
throw and add them here, before the class declarations.

*/
HANDLE_EXCEPTIONS(KIWAY::Player)

%ignore PgmOrNull;

%include pgm_base.h
%include frame_type.h
%include mail_type.h
%include project.h
%include kiway.h
%include kiway_express.h

%include kiway_player.i



%constant KIWAY    Kiway;


%{
#include <kiway.h>
#include <kiway_express.h>
#include <pgm_base.h>

#include <wx/stdpaths.h>


// Only a single KIWAY is supported in this single_top top level component,
// which is dedicated to loading only a single DSO.
KIWAY    Kiway( &Pgm(), KFCTL_PY_PROJECT_SUITE );


/**
 * Struct PGM_PYTHON
 * implements PGM_BASE with its own OnPgmInit() and OnPgmExit().
 */
static struct PGM_PYTHON : public PGM_BASE
{
    bool OnPgmInit()
    {

        if( !InitPgmSettings() )
            return false;
        return true;
    }

    void OnPgmExit()
    {
        Kiway.OnKiwayEnd();
    }

    virtual void MacOpenFile( const wxString& aFileName ) override { }
} program;

%}

%init %{
    int kiface_version = KIFACE_VERSION;
    auto kf = KIFACE_GETTER( &kiface_version, KIFACE_VERSION, &program );
    program.OnPgmInit();
    kf->OnKifaceStart( &program, KFCTL_PY_PROJECT_SUITE );
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
        print("hereA")

        if not self.InitPgm():
            return False;

        print("hereB")

        try:
            # A KIWAY_PLAYER is a wx.Window
            frame = Kiway.Player( FRAME_SCH, True )

            print("here0")

        except IOError as e:
            print('Player()', e)
            return None

        print("here1")

        Kiway.SetTop(frame)

        print("here2")

        return frame
    %}
};
