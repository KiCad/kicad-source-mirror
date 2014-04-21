/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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
#include <config.h>
#include <wx/debug.h>
#include <wx/stdpaths.h>


KIFACE* KIWAY::m_kiface[KIWAY_FACE_COUNT];
int     KIWAY::m_kiface_version[KIWAY_FACE_COUNT];


KIWAY::KIWAY( PGM_BASE* aProgram, wxFrame* aTop ):
    m_program( aProgram ),
    m_top( 0 )
{
    SetTop( aTop );     // hook playerDestroyHandler() into aTop.

    memset( m_player, 0, sizeof( m_player ) );
}

// Any event types derived from wxCommandEvt, like wxWindowDestroyEvent, are
// propogated upwards to parent windows if not handled below.  Therefor the
// m_top window should receive all wxWindowDestroyEvents originating from
// KIWAY_PLAYERs.  It does anyways, but now playerDestroyHandler eavesdrops
// on that event stream looking for KIWAY_PLAYERs being closed.

void KIWAY::playerDestroyHandler( wxWindowDestroyEvent& event )
{
    wxWindow* w = event.GetWindow();

    for( unsigned i=0; i<DIM(m_player);  ++i )
    {
        // if destroying one of our flock, then mark it as deceased.
        if( (wxWindow*) m_player[i] == w )
        {
            // DBG(printf( "%s: marking m_player[%d] as destroyed\n", __func__, i );)
            m_player[i] = 0;
        }
    }
}


void KIWAY::SetTop( wxFrame* aTop )
{
    if( m_top )
    {
        m_top->Disconnect( wxEVT_DESTROY, wxWindowDestroyEventHandler( KIWAY::playerDestroyHandler ), NULL, this );
    }

    if( aTop )
    {
        aTop->Connect( wxEVT_DESTROY, wxWindowDestroyEventHandler( KIWAY::playerDestroyHandler ), NULL, this );
    }

    m_top = aTop;
}


const wxString KIWAY::dso_full_path( FACE_T aFaceId )
{
    const wxChar*   name;

    switch( aFaceId )
    {
    case FACE_SCH:          name = KIFACE_PREFIX wxT( "eeschema" );     break;
    case FACE_PCB:          name = KIFACE_PREFIX wxT( "pcbnew" );       break;
    case FACE_CVPCB:        name = KIFACE_PREFIX wxT( "cvpcb" );        break;
    case FACE_GERBVIEW:     name = KIFACE_PREFIX wxT( "gerbview" );     break;
    case FACE_PL_EDITOR:    name = KIFACE_PREFIX wxT( "pl_editor" );    break;

    // case FACE_PCB_CALCULATOR:  who knows.

    default:
        wxASSERT_MSG( 0, wxT( "caller has a bug, passed a bad aFaceId" ) );
        return wxEmptyString;
    }

    wxFileName fn = wxStandardPaths::Get().GetExecutablePath();

    fn.SetName( name );

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
    if( unsigned( aFaceId ) >= DIM( m_kiface ) )
    {
        // @todo : throw an exception here for python's benefit, at least that
        // way it gets some explanatory text.

        wxASSERT_MSG( 0, wxT( "caller has a bug, passed a bad aFaceId" ) );
        return NULL;
    }

    // return the previously loaded KIFACE, if it was.
    if( m_kiface[aFaceId] )
        return m_kiface[aFaceId];

    // DSO with KIFACE has not been loaded yet, does caller want to load it?
    if( doLoad  )
    {
        wxString dname = dso_full_path( aFaceId );

        wxDynamicLibrary dso;

        void*   addr = NULL;

        if( !dso.Load( dname, wxDL_VERBATIM | wxDL_NOW ) )
        {
            // Failure: error reporting UI was done via wxLogSysError().
            // No further reporting required here.
        }

        else if( ( addr = dso.GetSymbol( wxT( KIFACE_INSTANCE_NAME_AND_VERSION ) ) ) == NULL )
        {
            // Failure: error reporting UI was done via wxLogSysError().
            // No further reporting required here.
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
            if( kiface->OnKifaceStart( m_program, KFCTL_PROJECT_SUITE ) )
            {
                // Tell dso's wxDynamicLibrary destructor not to Unload() the program image.
                (void) dso.Detach();

                return m_kiface[aFaceId] = kiface;
            }
        }

        // In any of the failure cases above, dso.Unload() should be called here
        // by dso destructor.
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
        return FACE_SCH;

    case FRAME_PCB:
    case FRAME_PCB_MODULE_EDITOR:
    case FRAME_PCB_MODULE_VIEWER:
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

    default:
        return FACE_T( -1 );
    }
}


KIWAY_PLAYER* KIWAY::Player( FRAME_T aFrameType, bool doCreate )
{
    // Since this will be called from python, cannot assume that code will
    // not pass a bad aFrameType.
    if( unsigned( aFrameType ) >= DIM( m_player ) )
    {
        // @todo : throw an exception here for python's benefit, at least that
        // way it gets some explanatory text.

        wxASSERT_MSG( 0, wxT( "caller has a bug, passed a bad aFrameType" ) );
        return NULL;
    }

    // return the previously opened window
    if( m_player[aFrameType] )
        return m_player[aFrameType];

    if( doCreate )
    {
        FACE_T face_type = KifaceType( aFrameType );

        wxASSERT( face_type != FACE_T(-1) );

        KIFACE* kiface = KiFACE( face_type );

        KIWAY_PLAYER* frame = (KIWAY_PLAYER*) kiface->CreateWindow( m_top, aFrameType, this, KFCTL_PROJECT_SUITE );

        return m_player[aFrameType] = frame;
    }

    return NULL;
}


bool KIWAY::PlayerClose( FRAME_T aFrameType, bool doForce )
{
    // Since this will be called from python, cannot assume that code will
    // not pass a bad aFrameType.
    if( unsigned( aFrameType ) >= DIM( m_player ) )
    {
        // @todo : throw an exception here for python's benefit, at least that
        // way it gets some explanatory text.

        wxASSERT_MSG( 0, wxT( "caller has a bug, passed a bad aFrameType" ) );
        return false;
    }

    if( m_player[aFrameType] )
    {
        if( m_player[aFrameType]->Close( doForce ) )
        {
            m_player[aFrameType] = 0;
            return true;
        }

        return false;
    }

    return true;    // window is closed already.
}


bool KIWAY::PlayersClose( bool doForce )
{
    bool ret = true;

    for( unsigned i=0; i < DIM( m_player );  ++i )
    {
        ret = ret && PlayerClose( FRAME_T( i ), doForce );
    }

    return ret;
}


void KIWAY::ExpressMail( FRAME_T aDestination,
                int aCommand, const std::string& aPayload, wxWindow* aSource )
{
    KIWAY_EXPRESS   mail( aDestination, aCommand, aPayload, aSource );

    ProcessEvent( mail );
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
            return alive->ProcessEvent( aEvent );
    }

    return false;
}
