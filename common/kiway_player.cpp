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


#include <kiway_player.h>
#include <kiway_express.h>
#include <kiway.h>
#include <id.h>
#include <macros.h>
#include <typeinfo>
#include <wx/utils.h>
#include <wx/evtloop.h>


BEGIN_EVENT_TABLE( KIWAY_PLAYER, EDA_BASE_FRAME )
    EVT_KIWAY_EXPRESS( KIWAY_PLAYER::kiway_express )
    EVT_MENU_RANGE( ID_LANGUAGE_CHOICE, ID_LANGUAGE_CHOICE_END, KIWAY_PLAYER::language_change )
END_EVENT_TABLE()


KIWAY_PLAYER::KIWAY_PLAYER( KIWAY* aKiway, wxWindow* aParent, FRAME_T aFrameType,
        const wxString& aTitle, const wxPoint& aPos, const wxSize& aSize,
        long aStyle, const wxString& aWdoName ) :
    EDA_BASE_FRAME( aParent, aFrameType, aTitle, aPos, aSize, aStyle, aWdoName ),
    KIWAY_HOLDER( aKiway ),
    m_modal( false ),
    m_modal_loop( 0 ), m_modal_resultant_parent( 0 )
{
    // DBG( printf("KIWAY_EXPRESS::wxEVENT_ID:%d\n", KIWAY_EXPRESS::wxEVENT_ID );)
}


KIWAY_PLAYER::KIWAY_PLAYER( wxWindow* aParent, wxWindowID aId, const wxString& aTitle,
        const wxPoint& aPos, const wxSize& aSize, long aStyle,
        const wxString& aWdoName ) :
    EDA_BASE_FRAME( aParent, (FRAME_T) aId, aTitle, aPos, aSize, aStyle, aWdoName ),
    KIWAY_HOLDER( 0 ),
    m_modal( false ),
    m_modal_loop( 0 ), m_modal_resultant_parent( 0 )
{
    // DBG( printf("KIWAY_EXPRESS::wxEVENT_ID:%d\n", KIWAY_EXPRESS::wxEVENT_ID );)
}


KIWAY_PLAYER::~KIWAY_PLAYER(){}


void KIWAY_PLAYER::KiwayMailIn( KIWAY_EXPRESS& aEvent )
{
    // override this in derived classes.
}


bool KIWAY_PLAYER::ShowModal( wxString* aResult, wxWindow* aResultantFocusWindow )
{
    wxASSERT_MSG( IsModal(), wxT( "ShowModal() shouldn't be called on non-modal frame" ) );

    /*
        This function has a nice interface but a necessarily unsightly implementation.
        Now the implementation is encapsulated, localizing future changes.

        It works in tandem with DismissModal().  But only ShowModal() is in the
        vtable and therefore cross-module capable.
   */

    // This is an exception safe way to zero a pointer before returning.
    // Yes, even though DismissModal() clears this first normally, this is
    // here in case there's an exception before the dialog is dismissed.
    struct NULLER
    {
        void*&  m_what;
        NULLER( void*& aPtr ) : m_what( aPtr ) {}
        ~NULLER() { m_what = 0; }   // indeed, set it to NULL on destruction
    } clear_this( (void*&) m_modal_loop );


    m_modal_resultant_parent = aResultantFocusWindow;
    Show( true );
    SetFocus();

    {
        // exception safe way to disable all frames except the modal one,
        // re-enables only those that were disabled on exit
        wxWindowDisabler    toggle( this );

        WX_EVENT_LOOP           event_loop;

#if wxCHECK_VERSION( 2, 9, 4 )  // 2.9.4 is only approximate.
        // new code needs this, old code does it in wxEventLoop::Run() and cannot
        // tolerate it here. Where that boundary is as a version number, I don't know.
        // A closer look at the subversion repo for wx would tell.
        wxEventLoopActivator    event_loop_stacker( &event_loop );
#endif

        m_modal_loop = &event_loop;

        event_loop.Run();

    }   // End of scop for some variables.
        // End nesting before setting focus below.

    if( aResult )
        *aResult = m_modal_string;

    DBG(printf( "~%s: aResult:'%s'  ret:%d\n",
            __func__, TO_UTF8( m_modal_string ), m_modal_ret_val );)

    if( aResultantFocusWindow )
    {
        aResultantFocusWindow->Raise();

        // have the final say, after wxWindowDisabler reenables my parent and
        // the events settle down, set the focus
        wxYield();
        aResultantFocusWindow->SetFocus();
    }

    return m_modal_ret_val;
}

bool KIWAY_PLAYER::Destroy()
{
    // Reparent is needed on Windows to leave the modal parent on top with focus
    // However it works only if the caller is a main frame, not a dialog.
    // (application crashes if the new parent is a wxDialog
#ifdef __WINDOWS__
    if( m_modal_resultant_parent && GetParent() != m_modal_resultant_parent )
    {
        EDA_BASE_FRAME* parent = dynamic_cast<EDA_BASE_FRAME*>(m_modal_resultant_parent);
        if( parent )
            Reparent( m_modal_resultant_parent );
    }
#endif

    return EDA_BASE_FRAME::Destroy();
}

bool KIWAY_PLAYER::IsDismissed()
{
    bool ret = !m_modal_loop;

    DBG(printf( "%s: ret:%d\n", __func__, ret );)

    return ret;
}


void KIWAY_PLAYER::DismissModal( bool aRetVal, const wxString& aResult )
{
    m_modal_ret_val = aRetVal;
    m_modal_string  = aResult;

    if( m_modal_loop )
    {
        m_modal_loop->Exit();
        m_modal_loop = 0;      // this marks it as dismissed.
    }

    Show( false );
}


void KIWAY_PLAYER::kiway_express( KIWAY_EXPRESS& aEvent )
{
    // logging support
#if defined(DEBUG)
    const char* class_name = typeid( this ).name();

    printf( "%s: received cmd:%d  pay:'%s'\n", class_name,
        aEvent.Command(), aEvent.GetPayload().c_str() );
#endif

    KiwayMailIn( aEvent );     // call the virtual, override in derived.
}


void KIWAY_PLAYER::language_change( wxCommandEvent& event )
{
    int id = event.GetId();

    // tell all the KIWAY_PLAYERs about the language change.
    Kiway().SetLanguage( id );
}
