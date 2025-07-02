/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <cstring>

#include <app_monitor.h>
#include <core/ignore.h>
#include <macros.h>
#include <kiway.h>
#include <kiway_player.h>
#include <kiway_express.h>
#include <pgm_base.h>
#include <config.h>
#include <core/arraydim.h>
#include <id.h>
#include <kiplatform/app.h>
#include <kiplatform/environment.h>
#include <settings/settings_manager.h>
#include <tool/action_manager.h>
#include <logging.h>

#include <wx/dynlib.h>
#include <wx/stdpaths.h>
#include <wx/debug.h>
#include <wx/utils.h>
#include <confirm.h>

KIFACE* KIWAY::m_kiface[KIWAY_FACE_COUNT];
int     KIWAY::m_kiface_version[KIWAY_FACE_COUNT];



KIWAY::KIWAY( int aCtlBits, wxFrame* aTop ):
     m_ctl( aCtlBits ), m_top( nullptr ), m_blockingDialog( wxID_NONE )
{
    SetTop( aTop );     // hook player_destroy_handler() into aTop.

    // Set the array of all known frame window IDs to empty = wxID_NONE,
    // once they are be created, they are added with FRAME_T as index to this array.
    // Note: A non empty entry does not mean the frame still exists.
    //   It means only the frame was created at least once. It can be destroyed after.
    //   These entries are not cleared automatically on window closing. The purpose is just
    //   to allow a call to wxWindow::FindWindowById() using a FRAME_T frame type
    for( int n = 0; n < KIWAY_PLAYER_COUNT; n++ )
        m_playerFrameId[n] = wxID_NONE;
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
        m_top->Disconnect( wxEVT_DESTROY,
                           wxWindowDestroyEventHandler( KIWAY::player_destroy_handler ),
                           nullptr, this );
    }

    if( aTop )
    {
        aTop->Connect( wxEVT_DESTROY,
                       wxWindowDestroyEventHandler( KIWAY::player_destroy_handler ),
                       nullptr, this );
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
    case FACE_PYTHON:           name = KIFACE_PREFIX "kipython";            break;

    default:
        wxASSERT_MSG( 0, wxT( "caller has a bug, passed a bad aFaceId" ) );
        return wxEmptyString;
    }

#ifndef __WXMAC__
    wxString path;

    if( m_ctl & (KFCTL_STANDALONE | KFCTL_CPP_PROJECT_SUITE) )
    {
        // The 2 *.cpp program launchers: single_top.cpp and kicad.cpp expect
        // the *.kiface's to reside in same directory as their binaries do.
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

    // To speed up development, it's sometimes nice to run kicad from inside
    // the build path.  In that case, each program will be in a subdirectory.
    // To find the DSOs, we need to go up one directory and then enter a subdirectory.
    if( wxGetEnv( wxT( "KICAD_RUN_FROM_BUILD_DIR" ), nullptr ) )
    {
#ifdef __WXMAC__
        // On Mac, all of the kifaces are placed in the kicad.app bundle, even though the individual
        // standalone binaries are placed in separate bundles before the make install step runs.
        // So, we have to jump up to the kicad directory, then the PlugIns section of the kicad
        // bundle.
        fn = wxStandardPaths::Get().GetExecutablePath();

        fn.RemoveLastDir();
        fn.RemoveLastDir();
        fn.RemoveLastDir();
        fn.RemoveLastDir();
        fn.AppendDir( wxT( "kicad" ) );
        fn.AppendDir( wxT( "kicad.app" ) );
        fn.AppendDir( wxT( "Contents" ) );
        fn.AppendDir( wxT( "PlugIns" ) );
        fn.SetName( name );
#else
        const char*   dirName;

        // The subdirectories usually have the same name as the kiface
        switch( aFaceId )
        {
            case FACE_PL_EDITOR: dirName = "pagelayout_editor";   break;
            case FACE_PYTHON:    dirName = "scripting";           break;
            default:             dirName = name + 1;              break;
        }

        fn.RemoveLastDir();
        fn.AppendDir( dirName );
#endif
    }

    // Here a "suffix" == an extension with a preceding '.',
    // so skip the preceding '.' to get an extension
    fn.SetExt( &KIFACE_SUFFIX[1] );

    return fn.GetFullPath();
}


PROJECT& KIWAY::Prj() const
{
    return Pgm().GetSettingsManager().Prj();
}


KIFACE* KIWAY::KiFACE( FACE_T aFaceId, bool doLoad )
{
    // Since this will be called from python, cannot assume that code will
    // not pass a bad aFaceId.
    if( (unsigned) aFaceId >= arrayDim( m_kiface ) )
    {
        // @todo : throw an exception here for python's benefit, at least that
        // way it gets some explanatory text.

        wxASSERT_MSG( 0, wxT( "caller has a bug, passed a bad aFaceId" ) );
        return nullptr;
    }

    // return the previously loaded KIFACE, if it was.
    if( m_kiface[aFaceId] )
        return m_kiface[aFaceId];

    // DSO with KIFACE has not been loaded yet, does caller want to load it?
    if( doLoad )
    {
        wxString dname = dso_search_path( aFaceId );

        // Insert DLL search path for kicad_3dsg from build dir
        if( wxGetEnv( wxT( "KICAD_RUN_FROM_BUILD_DIR" ), nullptr ) )
        {
            wxFileName myPath = wxStandardPaths::Get().GetExecutablePath();

            if( !myPath.GetPath().EndsWith( wxT( "pcbnew" ) ) )
            {
                myPath.RemoveLastDir();
                myPath.AppendDir( wxT( "pcbnew" ) );
                KIPLATFORM::APP::AddDynamicLibrarySearchPath( myPath.GetPath() );
            }
        }

        wxString msg;

#ifdef KICAD_WIN32_VERIFY_CODESIGN
        bool codeSignOk = KIPLATFORM::ENV::VerifyFileSignature( dname );
        if( !codeSignOk )
        {
            msg.Printf( _( "Failed to verify kiface library '%s' signature." ), dname );
            THROW_IO_ERROR( msg );
        }
#endif

        wxDynamicLibrary dso;

        void*   addr = nullptr;

        // For some reason wxDynamicLibrary::Load() crashes in some languages
        // (chinese for instance) when loading the dynamic library.
        // The crash happens for Eeschema.
        // So switch to "C" locale during loading (LC_COLLATE is enough).
        int lc_new_type = LC_COLLATE;
        std::string user_locale = setlocale( lc_new_type, nullptr );
        setlocale( lc_new_type, "C" );

        bool success = dso.Load( dname, wxDL_VERBATIM | wxDL_NOW | wxDL_GLOBAL );

        setlocale( lc_new_type, user_locale.c_str() );

        msg = wxString::Format( "Loading kiface %d", aFaceId );
        APP_MONITOR::AddNavigationBreadcrumb( msg, "kiway.kiface" );

        if( !success )
        {
            // Failure: error reporting UI was done via wxLogSysError().
            // No further reporting required here.  Apparently this is not true on all
            // platforms and/or wxWidgets builds and KiCad will crash.  Throwing the exception
            // here and catching it in the KiCad launcher resolves the crash issue.  See bug
            // report https://bugs.launchpad.net/kicad/+bug/1577786.

            msg.Printf( _( "Failed to load kiface library '%s'." ), dname );
            THROW_IO_ERROR( msg );
        }
        else if( ( addr = dso.GetSymbol( wxT( KIFACE_INSTANCE_NAME_AND_VERSION ) ) ) == nullptr )
        {
            // Failure: error reporting UI was done via wxLogSysError().
            // No further reporting required here.  Assume the same thing applies here as
            // above with the Load() call.  This has not been tested.
            msg.Printf( _( "Could not read instance name and version from kiface library '%s'." ),
                        dname );
            THROW_IO_ERROR( msg );
        }
        else
        {
            KIFACE_GETTER_FUNC* ki_getter = (KIFACE_GETTER_FUNC*) addr;

            KIFACE* kiface = ki_getter( &m_kiface_version[aFaceId], KIFACE_VERSION, &Pgm() );

            // KIFACE_GETTER_FUNC function comment (API) says the non-NULL is unconditional.
            wxASSERT_MSG( kiface,
                          wxT( "attempted DSO has a bug, failed to return a KIFACE*" ) );

            wxDllType dsoHandle = dso.Detach();

            bool startSuccess = false;

            // Give the DSO a single chance to do its "process level" initialization.
            // "Process level" specifically means stay away from any projects in there.

            try
            {
                startSuccess = kiface->OnKifaceStart( &Pgm(), m_ctl, this );
            }
            catch (...)
            {
                // OnKiFaceStart may generate an exception
                // Before we continue and ultimately unload our module to retry we need
                // to process the exception before we delete the free the memory space the
                // exception resides in
                Pgm().HandleException( std::current_exception() );
            }

            if( startSuccess )
            {
                return m_kiface[aFaceId] = kiface;
            }
            else
            {
                // Usually means canceled initial global library setup
                // But it could have been an exception/failure
                // Let the module go out of scope to unload
                dso.Attach( dsoHandle );

                return nullptr;
            }
        }
    }

    return nullptr;
}


KIWAY::FACE_T KIWAY::KifaceType( FRAME_T aFrameType )
{
    switch( aFrameType )
    {
    case FRAME_SCH:
    case FRAME_SCH_SYMBOL_EDITOR:
    case FRAME_SCH_VIEWER:
    case FRAME_SYMBOL_CHOOSER:
    case FRAME_SIMULATOR:
        return FACE_SCH;

    case FRAME_PCB_EDITOR:
    case FRAME_FOOTPRINT_EDITOR:
    case FRAME_FOOTPRINT_VIEWER:
    case FRAME_FOOTPRINT_CHOOSER:
    case FRAME_FOOTPRINT_WIZARD:
    case FRAME_PCB_DISPLAY3D:
        return FACE_PCB;

    case FRAME_CVPCB:
    case FRAME_CVPCB_DISPLAY:
        return FACE_CVPCB;

    case FRAME_PYTHON:
        return FACE_PYTHON;

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
    wxWindowID storedId = m_playerFrameId[aFrameType];

    if( storedId == wxID_NONE )
        return nullptr;

    wxWindow* frame = wxWindow::FindWindowById( storedId );

    // Since wxWindow::FindWindow*() is not cheap (especially if the window does not exist),
    // clear invalid entries to save CPU on repeated calls that do not lead to frame creation
    if( !frame )
        m_playerFrameId[aFrameType].compare_exchange_strong( storedId, wxID_NONE );

    return static_cast<KIWAY_PLAYER*>( frame );
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
        return nullptr;
    }

    // return the previously opened window
    KIWAY_PLAYER* frame = GetPlayerFrame( aFrameType );

    if( frame )
        return frame;

    if( doCreate )
    {
        try
        {
            wxString msg = wxString::Format( "Creating window type %d", aFrameType );
            APP_MONITOR::AddNavigationBreadcrumb( msg, "kiway.player" );

            FACE_T  face_type = KifaceType( aFrameType );
            KIFACE* kiface = KiFACE( face_type );

            if( !kiface )
                return nullptr;

            frame = (KIWAY_PLAYER*) kiface->CreateKiWindow(
                                            aParent,    // Parent window of frame in modal mode,
                                                        // NULL in non modal mode
                                            aFrameType,
                                            this,
                                            m_ctl       // questionable need, these same flags
                                                        // were passed to KIFACE::OnKifaceStart()
                                            );
            if( frame )
                m_playerFrameId[aFrameType].store( frame->GetId() );

            return frame;
        }
        catch( ... )
        {
            Pgm().HandleException( std::current_exception() );
            wxLogError( _( "Error loading editor." ) );
        }
    }

    return nullptr;
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

    KIWAY_PLAYER* frame = GetPlayerFrame( aFrameType );

    if( frame == nullptr ) // Already closed
        return true;

    wxString msg = wxString::Format( "Closing window type %d", aFrameType );
    APP_MONITOR::AddNavigationBreadcrumb( msg, "kiway.playerclose" );

    if( frame->NonUserClose( doForce ) )
    {
        m_playerFrameId[aFrameType] = wxID_NONE;
        return true;
    }

    return false;
}


bool KIWAY::PlayersClose( bool doForce )
{
    bool ret = true;

    for( unsigned i=0; i < KIWAY_PLAYER_COUNT;  ++i )
        ret = ret && PlayerClose( FRAME_T( i ), doForce );

    return ret;
}


void KIWAY::PlayerDidClose( FRAME_T aFrameType )
{
    m_playerFrameId[aFrameType] = wxID_NONE;
}


void KIWAY::ExpressMail( FRAME_T aDestination, MAIL_T aCommand, std::string& aPayload,
                         wxWindow* aSource )
{
    KIWAY_EXPRESS   mail( aDestination, aCommand, aPayload, aSource );

    ProcessEvent( mail );
}


void KIWAY::GetActions( std::vector<TOOL_ACTION*>& aActions ) const
{
    for( TOOL_ACTION* action : ACTION_MANAGER::GetActionList() )
        aActions.push_back( action );
}


void KIWAY::SetLanguage( int aLanguage )
{
    wxString errMsg;
    bool     ret = false;

    {
        // Only allow the traces to be logged by wx. We use our own system to log when the
        // OS doesn't support the language, so we want to hide the wx error.
        WX_LOG_TRACE_ONLY logtraceOnly;
        Pgm().SetLanguageIdentifier( aLanguage );
        ret = Pgm().SetLanguage( errMsg );
    }

    if( !ret )
    {
        wxString lang;

        for( unsigned ii = 0;  LanguagesList[ii].m_KI_Lang_Identifier != 0; ii++ )
        {
            if( aLanguage == LanguagesList[ii].m_KI_Lang_Identifier )
            {
                if( LanguagesList[ii].m_DoNotTranslate )
                    lang = LanguagesList[ii].m_Lang_Label;
                else
                    lang = wxGetTranslation( LanguagesList[ii].m_Lang_Label );

                break;
            }
        }

        DisplayErrorMessage( nullptr,
                             wxString::Format( _( "Unable to switch language to %s" ), lang ),
                             errMsg );
        return;
    }

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

        if ( top )
        {
            top->ShowChangedLanguage();
            wxCommandEvent e( EDA_LANG_CHANGED );
            top->GetEventHandler()->ProcessEvent( e );
        }
    }
#endif

    for( unsigned i=0;  i < KIWAY_PLAYER_COUNT;  ++i )
    {
        KIWAY_PLAYER* frame = GetPlayerFrame( ( FRAME_T )i );

        if( frame )
        {
            frame->ShowChangedLanguage();
            wxCommandEvent e( EDA_LANG_CHANGED );
            frame->GetEventHandler()->ProcessEvent( e );
        }
    }
}


void KIWAY::CommonSettingsChanged( int aFlags )
{
    if( m_ctl & KFCTL_CPP_PROJECT_SUITE )
    {
        // A dynamic_cast could be better, but creates link issues
        // (some basic_frame functions not found) on some platforms,
        // so a static_cast is used.
        EDA_BASE_FRAME* top = static_cast<EDA_BASE_FRAME*>( m_top );

        if( top )
            top->CommonSettingsChanged( aFlags );
    }

    for( unsigned i=0;  i < KIWAY_PLAYER_COUNT;  ++i )
    {
        KIWAY_PLAYER* frame = GetPlayerFrame( ( FRAME_T )i );

        if( frame )
            frame->CommonSettingsChanged( aFlags );
    }
}


void KIWAY::ClearFileHistory()
{
    if( m_ctl & KFCTL_CPP_PROJECT_SUITE )
    {
        // A dynamic_cast could be better, but creates link issues
        // (some basic_frame functions not found) on some platforms,
        // so a static_cast is used.
        EDA_BASE_FRAME* top = static_cast<EDA_BASE_FRAME*>( m_top );

        if( top )
            top->ClearFileHistory();
    }

    for( unsigned i=0;  i < KIWAY_PLAYER_COUNT;  ++i )
    {
        KIWAY_PLAYER* frame = GetPlayerFrame( ( FRAME_T )i );

        if( frame )
            frame->ClearFileHistory();
    }
}


void KIWAY::ProjectChanged()
{
    APP_MONITOR::AddNavigationBreadcrumb( "Changing project", "kiway.projectchanged" );

    if( m_ctl & KFCTL_CPP_PROJECT_SUITE )
    {
        // A dynamic_cast could be better, but creates link issues
        // (some basic_frame functions not found) on some platforms,
        // so a static_cast is used.
        EDA_BASE_FRAME* top = static_cast<EDA_BASE_FRAME*>( m_top );

        if( top )
            top->ProjectChanged();
    }

    // Cancel an in-progress load of libraries; handled through the schematic and PCB ifaces
    if ( KIFACE* schface = KiFACE( KIWAY::FACE_SCH ) )
        schface->ProjectChanged();

    if ( KIFACE* pcbface = KiFACE( KIWAY::FACE_PCB ) )
        pcbface->ProjectChanged();

    for( unsigned i=0;  i < KIWAY_PLAYER_COUNT;  ++i )
    {
        KIWAY_PLAYER* frame = GetPlayerFrame( ( FRAME_T )i );

        if( frame )
            frame->ProjectChanged();
    }
}


wxWindow* KIWAY::GetBlockingDialog()
{
    return wxWindow::FindWindowById( m_blockingDialog );
}


void KIWAY::SetBlockingDialog( wxWindow* aWin )
{
    if( !aWin )
        m_blockingDialog = wxID_NONE;
    else
        m_blockingDialog = aWin->GetId();
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


int KIWAY::ProcessJob( KIWAY::FACE_T aFace, JOB* job, REPORTER* aReporter, PROGRESS_REPORTER* aProgressReporter )
{
    KIFACE* kiface = KiFACE( aFace );

    return kiface->HandleJob( job, aReporter, aProgressReporter );
}


bool KIWAY::ProcessJobConfigDialog( KIWAY::FACE_T aFace, JOB* aJob, wxWindow* aWindow )
{
    KIFACE* kiface = KiFACE( aFace );

    return kiface->HandleJobConfig( aJob, aWindow );
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
