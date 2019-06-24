/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2014-2017 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <string.h>

#include <macros.h>
#include <kiway.h>
#include <kiway_player.h>
#include <kiway_express.h>
#include <pgm_base.h>
#include <config.h>
#include <id.h>

#include <wx/stdpaths.h>
#include <wx/debug.h>
#include <wx/utils.h>


KIFACE* KIWAY::m_kiface[KIWAY_FACE_COUNT];
int     KIWAY::m_kiface_version[KIWAY_FACE_COUNT];



KIWAY::KIWAY( PGM_BASE* aProgram, int aCtlBits, wxFrame* aTop ):
    m_program( aProgram ), m_ctl( aCtlBits ), m_top( 0 )
{
    SetTop( aTop );     // hook player_destroy_handler() into aTop.


    // Prepare the room to store the frame names, once they will be created
    // with FRAME_T type as index in this table.
    // (note this is a list of frame names, but a non empty entry
    // does not mean the frame still exists. It means only the frame was created
    // at least once. It can be destroyed after. These entries are not cleared.
    // the purpose is just to allow a call to wxWindow::FindWindowByName(), from
    // a FRAME_T frame type
    m_playerFrameName.Add( wxEmptyString, KIWAY_PLAYER_COUNT );
}


#if 0
// Any event types derived from wxCommandEvt, like wxWindowDestroyEvent, are
// propagated upwards to parent windows if not handled below.  Therefore the
// m_top window should receive all wxWindowDestroyEvents originating from
// KIWAY_PLAYERs.  It does anyways, but now player_destroy_handler eavesdrops
// on that event stream looking for KIWAY_PLAYERs being closed.

void KIWAY::player_destroy_handler( wxWindowDestroyEvent& event )
{
    // Currently : do nothing
    event.Skip();  // skip to who, the wxApp?  I'm the top window.
}
#endif


void KIWAY::SetTop( wxFrame* aTop )
{
#if 0
    if( m_top )
    {
        m_top->Disconnect( wxEVT_DESTROY, wxWindowDestroyEventHandler( KIWAY::player_destroy_handler ), NULL, this );
    }

    if( aTop )
    {
        aTop->Connect( wxEVT_DESTROY, wxWindowDestroyEventHandler( KIWAY::player_destroy_handler ), NULL, this );
    }
#endif

    m_top = aTop;
}


const wxString KIWAY::dso_search_path( FACE_T aFaceId )
{
    const char*   name;

    switch( aFaceId )
    {
    case FACE_SCH:              name = KIFACE_PREFIX "eeschema";            break;
    case FACE_PCB:              name = KIFACE_PREFIX "pcbnew";              break;
    case FACE_CVPCB:            name = KIFACE_PREFIX "cvpcb";               break;
    case FACE_GERBVIEW:         name = KIFACE_PREFIX "gerbview";            break;
    case FACE_PL_EDITOR:        name = KIFACE_PREFIX "pl_editor";           break;
    case FACE_PCB_CALCULATOR:   name = KIFACE_PREFIX "pcb_calculator";      break;
    case FACE_BMP2CMP:          name = KIFACE_PREFIX "bitmap2component";    break;

    default:
        wxASSERT_MSG( 0, wxT( "caller has a bug, passed a bad aFaceId" ) );
        return wxEmptyString;
    }

#ifndef __WXMAC__
    wxString path;

    if( m_ctl & (KFCTL_STANDALONE | KFCTL_CPP_PROJECT_SUITE) )
    {
        // The 2 *.cpp program launchers: single_top.cpp and kicad.cpp expect
        // the *.kiface's to reside in same diretory as their binaries do.
        // Not so for python launcher, identified by KFCTL_PY_PROJECT_SUITE
        path = wxStandardPaths::Get().GetExecutablePath();
    }

    wxFileName fn = path;
#else
    // we have the dso's in main OSX bundle kicad.app/Contents/PlugIns
    wxFileName fn = Pgm().GetExecutablePath();
    fn.AppendDir( wxT( "Contents" ) );
    fn.AppendDir( wxT( "PlugIns" ) );
#endif

    fn.SetName( name );

#ifdef DEBUG
    // To speed up development, it's sometimes nice to run kicad from inside
    // the build path.  In that case, each program will be in a subdirectory.
    // To find the DSOs, we need to go up one directory and then enter a subdirectory
    // with the same name as the DSO (without the prefix).

    if( wxGetEnv( wxT( "KICAD_RUN_FROM_BUILD_DIR" ), nullptr ) )
    {
#ifdef __WXMAC__
        fn = wxStandardPaths::Get().GetExecutablePath();
        fn.RemoveLastDir();
        fn.AppendDir( wxT( "PlugIns" ) );
        fn.SetName( name );
#else
        fn.RemoveLastDir();
        fn.AppendDir( name + 1 );
#endif
    }
#endif

    // Here a "suffix" == an extension with a preceding '.',
    // so skip the preceding '.' to get an extension
    fn.SetExt( KIFACE_SUFFIX + 1 );         // + 1 => &KIFACE_SUFFIX[1]

    return fn.GetFullPath();
}


PROJECT& KIWAY::Prj() const
{
    return *(PROJECT*) &m_project;      // strip const-ness, function really is const.
}


KIFACE*  KIWAY::KiFACE( FACE_T aFaceId, bool doLoad )
{
    // Since this will be called from python, cannot assume that code will
    // not pass a bad aFaceId.
    if( (unsigned) aFaceId >= arrayDim( m_kiface ) )
    {
        // @todo : throw an exception here for python's benefit, at least that
        // way it gets some explanatory text.

        wxASSERT_MSG( 0, wxT( "caller has a bug, passed a bad aFaceId" ) );
        return NULL;
    }

    // return the previously loaded KIFACE, if it was.
    if( m_kiface[aFaceId] )
        return m_kiface[aFaceId];

    wxString msg;

    // DSO with KIFACE has not been loaded yet, does caller want to load it?
    if( doLoad  )
    {
        wxString dname = dso_search_path( aFaceId );

        wxDynamicLibrary dso;

        void*   addr = NULL;

        if( !dso.Load( dname, wxDL_VERBATIM | wxDL_NOW | wxDL_GLOBAL ) )
        {
            // Failure: error reporting UI was done via wxLogSysError().
            // No further reporting required here.  Apparently this is not true on all
            // platforms and/or wxWidgets builds and KiCad will crash.  Throwing the exception
            // here and catching it in the KiCad launcher resolves the crash issue.  See bug
            // report https://bugs.launchpad.net/kicad/+bug/1577786.

            msg.Printf( _( "Failed to load kiface library \"%s\"." ), GetChars( dname ) );
            THROW_IO_ERROR( msg );
        }
        else if( ( addr = dso.GetSymbol( wxT( KIFACE_INSTANCE_NAME_AND_VERSION ) ) ) == NULL )
        {
            // Failure: error reporting UI was done via wxLogSysError().
            // No further reporting required here.  Assume the same thing applies here as
            // above with the Load() call.  This has not been tested.
            msg.Printf(
                _( "Could not read instance name and version symbol form kiface library \"%s\"." ),
                GetChars( dname ) );
            THROW_IO_ERROR( msg );
        }
        else
        {
            KIFACE_GETTER_FUNC* getter = (KIFACE_GETTER_FUNC*) addr;

            KIFACE* kiface = getter( &m_kiface_version[aFaceId], KIFACE_VERSION, m_program );

            // KIFACE_GETTER_FUNC function comment (API) says the non-NULL is unconditional.
            wxASSERT_MSG( kiface,
                          wxT( "attempted DSO has a bug, failed to return a KIFACE*" ) );

            // Give the DSO a single chance to do its "process level" initialization.
            // "Process level" specifically means stay away from any projects in there.
            if( kiface->OnKifaceStart( m_program, m_ctl ) )
            {
                // Tell dso's wxDynamicLibrary destructor not to Unload() the program image.
                (void) dso.Detach();

                return m_kiface[aFaceId] = kiface;
            }
        }

        // In any of the failure cases above, dso.Unload() should be called here
        // by dso destructor.
        // However:

        // There is a file installation bug. We only look for KIFACE_I's which we know
        // to exist, and we did not find one.  If we do not find one, this is an
        // installation bug.

        msg = wxString::Format( _(
            "Fatal Installation Bug. File:\n"
            "\"%s\"\ncould not be loaded\n" ), GetChars( dname ) );

        if( ! wxFileExists( dname ) )
            msg << _( "It is missing.\n" );
        else
            msg << _( "Perhaps a shared library (.dll or .so) file is missing.\n" );

        msg << _( "From command line: argv[0]:\n'" );
        msg << wxStandardPaths::Get().GetExecutablePath() << wxT( "'\n" );

        // This is a fatal error, one from which we cannot recover, nor do we want
        // to protect against in client code which would require numerous noisy
        // tests in numerous places.  So we inform the user that the installation
        // is bad.  This exception will likely not get caught until way up in the
        // wxApp derivative, at which point the process will exit gracefully.
        THROW_IO_ERROR( msg );
    }

    return NULL;
}


KIWAY::FACE_T KIWAY::KifaceType( FRAME_T aFrameType )
{
    switch( aFrameType )
    {
    case FRAME_SCH:
    case FRAME_SCH_LIB_EDITOR:
    case FRAME_SCH_VIEWER:
    case FRAME_SCH_VIEWER_MODAL:
    case FRAME_SIMULATOR:
        return FACE_SCH;

    case FRAME_PCB:
    case FRAME_PCB_MODULE_EDITOR:
    case FRAME_PCB_MODULE_VIEWER:
    case FRAME_PCB_MODULE_VIEWER_MODAL:
    case FRAME_PCB_FOOTPRINT_WIZARD:
    case FRAME_PCB_DISPLAY3D:
        return FACE_PCB;

    case FRAME_CVPCB:
    case FRAME_CVPCB_DISPLAY:
        return FACE_CVPCB;

    case FRAME_GERBER:
        return FACE_GERBVIEW;

    case FRAME_PL_EDITOR:
        return FACE_PL_EDITOR;

    case FRAME_CALC:
        return FACE_PCB_CALCULATOR;

    case FRAME_BM2CMP:
        return FACE_BMP2CMP;

    default:
        return FACE_T( -1 );
    }
}


KIWAY_PLAYER* KIWAY::GetPlayerFrame( FRAME_T aFrameType )
{
    if( m_playerFrameName[aFrameType].IsEmpty() )
        return NULL;

    return static_cast<KIWAY_PLAYER*>( wxWindow::FindWindowByName( m_playerFrameName[aFrameType] ) );
}


KIWAY_PLAYER* KIWAY::Player( FRAME_T aFrameType, bool doCreate, wxTopLevelWindow* aParent )
{
    // Since this will be called from python, cannot assume that code will
    // not pass a bad aFrameType.
    if( (unsigned) aFrameType >= KIWAY_PLAYER_COUNT )
    {
        // @todo : throw an exception here for python's benefit, at least that
        // way it gets some explanatory text.

        wxASSERT_MSG( 0, wxT( "caller has a bug, passed a bad aFrameType" ) );
        return NULL;
    }

    // return the previously opened window
    KIWAY_PLAYER* frame = GetPlayerFrame( aFrameType );

    if( frame )
        return frame;

    if( doCreate )
    {
        FACE_T face_type = KifaceType( aFrameType );
        wxASSERT( face_type != FACE_T(-1) );

        KIFACE* kiface = KiFACE( face_type );
        wxASSERT( kiface );

        if( kiface )
        {
            frame = (KIWAY_PLAYER*) kiface->CreateWindow(
                    aParent,    // Parent window of frame in modal mode, NULL in non modal mode
                    aFrameType,
                    this,
                    m_ctl       // questionable need, these same flags where passed
                                // to the KIFACE::OnKifaceStart()
                    );
            wxASSERT( frame );

            m_playerFrameName[aFrameType] = frame->GetName();

            return frame;
        }
    }

    return NULL;
}


bool KIWAY::PlayerClose( FRAME_T aFrameType, bool doForce )
{
    // Since this will be called from python, cannot assume that code will
    // not pass a bad aFrameType.
    if( (unsigned) aFrameType >= KIWAY_PLAYER_COUNT )
    {
        // @todo : throw an exception here for python's benefit, at least that
        // way it gets some explanatory text.

        wxASSERT_MSG( 0, wxT( "caller has a bug, passed a bad aFrameType" ) );
        return false;
    }

    KIWAY_PLAYER* frame =  GetPlayerFrame( aFrameType );

    if( frame == NULL ) // Already closed
        return true;

    if( frame->Close( doForce ) )
        return true;

    return false;
}


bool KIWAY::PlayersClose( bool doForce )
{
    bool ret = true;

    for( unsigned i=0; i < KIWAY_PLAYER_COUNT;  ++i )
    {
        ret = ret && PlayerClose( FRAME_T( i ), doForce );
    }

    return ret;
}


void KIWAY::ExpressMail( FRAME_T aDestination, MAIL_T aCommand, std::string& aPayload, wxWindow* aSource )
{
    KIWAY_EXPRESS   mail( aDestination, aCommand, aPayload, aSource );

    ProcessEvent( mail );
}


void KIWAY::SetLanguage( int aLanguage )
{
    Pgm().SetLanguageIdentifier( aLanguage );
    Pgm().SetLanguage();

#if 1
    // This is a risky hack that goes away if we allow the language to be
    // set only from the top most frame if !Kiface.IsSingle()

    // Only for the C++ project manager, and not for the python one and not for
    // single_top do we look for the EDA_BASE_FRAME as the top level window.
    // For single_top this is not needed because that window is registered in
    // the array below.
    if( m_ctl & KFCTL_CPP_PROJECT_SUITE )
    {
        // A dynamic_cast could be better, but creates link issues
        // (some basic_frame functions not found) on some platforms,
        // so a static_cast is used.
        EDA_BASE_FRAME* top = static_cast<EDA_BASE_FRAME*>( m_top );

        if( top )
            top->ShowChangedLanguage();
    }
#endif

    for( unsigned i=0;  i < KIWAY_PLAYER_COUNT;  ++i )
    {
        KIWAY_PLAYER* frame = GetPlayerFrame( ( FRAME_T )i );

        if( frame )
        {
            frame->ShowChangedLanguage();
        }
    }
}

void KIWAY::CommonSettingsChanged( bool aEnvVarsChanged )
{
#if 1
    if( m_ctl & KFCTL_CPP_PROJECT_SUITE )
    {
        // A dynamic_cast could be better, but creates link issues
        // (some basic_frame functions not found) on some platforms,
        // so a static_cast is used.
        EDA_BASE_FRAME* top = static_cast<EDA_BASE_FRAME*>( m_top );

        if( top )
            top->CommonSettingsChanged( aEnvVarsChanged );
    }
#endif

    for( unsigned i=0;  i < KIWAY_PLAYER_COUNT;  ++i )
    {
        KIWAY_PLAYER* frame = GetPlayerFrame( ( FRAME_T )i );

        if( frame )
            frame->CommonSettingsChanged( aEnvVarsChanged );
    }
}


bool KIWAY::ProcessEvent( wxEvent& aEvent )
{
    KIWAY_EXPRESS* mail = dynamic_cast<KIWAY_EXPRESS*>( &aEvent );

    if( mail )
    {
        FRAME_T dest = mail->Dest();

        // see if recipient is alive
        KIWAY_PLAYER* alive = Player( dest, false );

        if( alive )
        {
#if 1
            return alive->ProcessEvent( aEvent );
#else
            alive->KiwayMailIn( *mail );
            return true;
#endif
        }
    }

    return false;
}


void KIWAY::OnKiCadExit()
{
    if( m_ctl & KFCTL_CPP_PROJECT_SUITE )
    {
        // A dynamic_cast could be better, but creates link issues
        // (some basic_frame functions not found) on some platforms,
        // so a static_cast is used.
        EDA_BASE_FRAME* top = static_cast<EDA_BASE_FRAME*>( m_top );

        if( top )
            top->Close( false );
    }
}


void KIWAY::OnKiwayEnd()
{
    for( KIFACE* i : m_kiface )
    {
        if( i )
            i->OnKifaceEnd();
    }
}
