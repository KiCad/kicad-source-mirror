
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <dialog_shim.h>
#include <kiway_player.h>
#include <wx/evtloop.h>


/// Toggle a window's "enable" status to disabled, then enabled on destruction.
class WDO_ENABLE_DISABLE
{
    wxWindow* m_win;

public:

    WDO_ENABLE_DISABLE( wxWindow* aWindow ) :
        m_win( aWindow )
    {
        if( m_win )
            m_win->Disable();
    }

    ~WDO_ENABLE_DISABLE()
    {
        if( m_win )
        {
            m_win->Enable();
            m_win->SetFocus(); // let's focus back on the parent window
        }
    }
};


DIALOG_SHIM::DIALOG_SHIM( wxWindow* aParent, wxWindowID id, const wxString& title,
        const wxPoint& pos, const wxSize& size, long style, const wxString& name ) :
    wxDialog( aParent, id, title, pos, size, style, name ),
    KIWAY_HOLDER( 0 ),
    m_qmodal_loop( 0 ),
    m_qmodal_showing( false ),
    m_qmodal_parent_disabler( 0 )
{
    // pray that aParent is either a KIWAY_PLAYER or DIALOG_SHIM derivation.
    KIWAY_HOLDER* h = dynamic_cast<KIWAY_HOLDER*>( aParent );

    // wxASSERT_MSG( h, wxT( "DIALOG_SHIM's parent is NULL or not derived from KIWAY_PLAYER nor DIALOG_SHIM" ) );

    if( h )
        SetKiway( this, &h->Kiway() );

#if DLGSHIM_USE_SETFOCUS
    Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_SHIM::onInit ) );
#endif
}


DIALOG_SHIM::~DIALOG_SHIM()
{
    // if the dialog is quasi-modal, this will end its event loop
    if( IsQuasiModal() )
        EndQuasiModal( wxID_CANCEL );

    delete m_qmodal_parent_disabler;    // usually NULL by now
}


// our hashtable is an implementation secret, don't need or want it in a header file
#include <hashtables.h>
#include <base_struct.h>        // EDA_RECT
#include <typeinfo>

static RECT_MAP class_map;

bool DIALOG_SHIM::Show( bool show )
{
    bool        ret;
    const char* hash_key;

    if( m_hash_key.size() )
    {
        // a special case like EDA_LIST_DIALOG, which has multiple uses.
        hash_key = m_hash_key.c_str();
    }
    else
    {
        hash_key = typeid(*this).name();
    }

    // Show or hide the window.  If hiding, save current position and size.
    // If showing, use previous position and size.
    if( show )
    {
        ret = wxDialog::Show( show );

        // classname is key, returns a zeroed out default EDA_RECT if none existed before.
        EDA_RECT r = class_map[ hash_key ];

        if( r.GetSize().x != 0 && r.GetSize().y != 0 )
            SetSize( r.GetPosition().x, r.GetPosition().y, r.GetSize().x, r.GetSize().y, 0 );
    }
    else
    {
        // Save the dialog's position & size before hiding, using classname as key
        EDA_RECT  r( wxDialog::GetPosition(), wxDialog::GetSize() );
        class_map[ hash_key ] = r;

        ret = wxDialog::Show( show );
    }
    return ret;
}


bool DIALOG_SHIM::Enable( bool enable )
{
    // so we can do logging of this state change:

#if defined(DEBUG)
    const char* type_id = typeid( *this ).name();
    printf( "wxDialog %s: %s\n", type_id, enable ? "enabled" : "disabled" );
#endif

    return wxDialog::Enable( enable );
}


#if DLGSHIM_USE_SETFOCUS

static bool findWindowRecursively( const wxWindowList& children, const wxWindow* wanted )
{
    for( wxWindowList::const_iterator it = children.begin();  it != children.end();  ++it )
    {
        const wxWindow* child = *it;

        if( wanted == child )
            return true;
        else
        {
            if( findWindowRecursively( child->GetChildren(), wanted ) )
                return true;
        }
    }

    return false;
}


static bool findWindowRecursively( const wxWindow* topmost, const wxWindow* wanted )
{
    // wanted may be NULL and that is ok.

    if( wanted == topmost )
        return true;

    return findWindowRecursively( topmost->GetChildren(), wanted );
}


/// Set the focus if it is not already set in a derived constructor to a specific control.
void DIALOG_SHIM::onInit( wxInitDialogEvent& aEvent )
{
    wxWindow* focusWnd = wxWindow::FindFocus();

    // If focusWnd is not already this window or a child of it, then SetFocus().
    // Otherwise the derived class's constructor SetFocus() already to a specific
    // child control.

    if( !findWindowRecursively( this, focusWnd ) )
    {
        // Linux wxGTK needs this to allow the ESCAPE key to close a wxDialog window.
        SetFocus();
    }

    aEvent.Skip();     // derived class's handler should be called too
}
#endif


/*
    Quasi-Modal Mode Explained:

    The gtk calls in wxDialog::ShowModal() cause event routing problems if that
    modal dialog then tries to use KIWAY_PLAYER::ShowModal().  The latter shows up
    and mostly works but does not respond to the window decoration close button.
    There is no way to get around this without reversing the gtk calls temporarily.

    Quasi-Modal mode is our own almost modal mode which disables only the parent
    of the DIALOG_SHIM, leaving other frames operable and while staying captured in the
    nested event loop.  This avoids the gtk calls and leaves event routing pure
    and sufficient to operate the KIWAY_PLAYER::ShowModal() properly.  When using
    ShowQuasiModal() you have to use EndQuasiModal() in your dialogs and not
    EndModal().  There is also IsQuasiModal() but its value can only be true
    when the nested event loop is active.  Do not mix the modal and quasi-modal
    functions.  Use one set or the other.

    You might find this behavior preferable over a pure modal mode, and it was said
    that only the Mac has this natively, but now other platforms have something
    similar.  You CAN use it anywhere for any dialog.  But you MUST use it when
    you want to use KIWAY_PLAYER::ShowModal() from a dialog event.
*/


#if !wxCHECK_VERSION( 2, 9, 4 )
wxWindow* DIALOG_SHIM::CheckIfCanBeUsedAsParent( wxWindow* parent ) const
{
    if ( !parent )
        return NULL;

    extern WXDLLIMPEXP_DATA_BASE(wxList) wxPendingDelete;

    if ( wxPendingDelete.Member(parent) || parent->IsBeingDeleted() )
    {
        // this window is being deleted and we shouldn't create any children
        // under it
        return NULL;
    }

    if ( parent->GetExtraStyle() & wxWS_EX_TRANSIENT )
    {
        // this window is not being deleted yet but it's going to disappear
        // soon so still don't parent this window under it
        return NULL;
    }

    if ( !parent->IsShownOnScreen() )
    {
        // using hidden parent won't work correctly neither
        return NULL;
    }

    // FIXME-VC6: this compiler requires an explicit const cast or it fails
    //            with error C2446
    if ( const_cast<const wxWindow *>(parent) == this )
    {
        // not sure if this can really happen but it doesn't hurt to guard
        // against this clearly invalid situation
        return NULL;
    }

    return parent;
}


wxWindow* DIALOG_SHIM::GetParentForModalDialog(wxWindow *parent, long style) const
{
    // creating a parent-less modal dialog will result (under e.g. wxGTK2)
    // in an unfocused dialog, so try to find a valid parent for it unless we
    // were explicitly asked not to
    if ( style & wxDIALOG_NO_PARENT )
        return NULL;

    // first try the given parent
    if ( parent )
        parent = CheckIfCanBeUsedAsParent(wxGetTopLevelParent(parent));

    // then the currently active window
    if ( !parent )
        parent = CheckIfCanBeUsedAsParent(
                    wxGetTopLevelParent(wxGetActiveWindow()));

    // and finally the application main window
    if ( !parent )
        parent = CheckIfCanBeUsedAsParent(wxTheApp->GetTopWindow());

    return parent;
}
#endif


/*
/// wxEventLoopActivator but with a friend so it
/// has access to m_evtLoopOld, and it does not SetActive() as that is
/// done inside base class Run().
class ELOOP_ACTIVATOR
{
    friend class EVENT_LOOP;

public:
    ELOOP_ACTIVATOR( WX_EVENT_LOOP* evtLoop )
    {
        m_evtLoopOld = wxEventLoopBase::GetActive();
        // wxEventLoopBase::SetActive( evtLoop );
    }

    ~ELOOP_ACTIVATOR()
    {
        // restore the previously active event loop
        wxEventLoopBase::SetActive( m_evtLoopOld );
    }

private:
    WX_EVENT_LOOP* m_evtLoopOld;
};
*/


class EVENT_LOOP : public WX_EVENT_LOOP
{
public:

    EVENT_LOOP()
    {
        ;
    }

    ~EVENT_LOOP()
    {
    }

#if 0   // does not work any better than inherited wxGuiEventLoop functions:

    // sets the "should exit" flag and wakes up the loop so that it terminates
    // soon
    void ScheduleExit( int rc = 0 )
    {
        wxCHECK_RET( IsInsideRun(), wxT("can't call ScheduleExit() if not running") );

        m_exitcode = rc;
        m_shouldExit = true;

        OnExit();

        // all we have to do to exit from the loop is to (maybe) wake it up so that
        // it can notice that Exit() had been called
        //
        // in particular, do *not* use here calls such as PostQuitMessage() (under
        // MSW) which terminate the current event loop here because we're not sure
        // that it is going to be processed by the correct event loop: it would be
        // possible that another one is started and terminated by mistake if we do
        // this
        WakeUp();
    }

    int Run()
    {
        // event loops are not recursive, you need to create another loop!
        //wxCHECK_MSG( !IsInsideRun(), -1, wxT("can't reenter a message loop") );

        // ProcessIdle() and ProcessEvents() below may throw so the code here should
        // be exception-safe, hence we must use local objects for all actions we
        // should undo
        wxEventLoopActivator activate(this);

        // We might be called again, after a previous call to ScheduleExit(), so
        // reset this flag.
        m_shouldExit = false;

        // Set this variable to true for the duration of this method.
        setInsideRun( true );

        struct SET_FALSE
        {
            EVENT_LOOP* m_loop;
            SET_FALSE( EVENT_LOOP* aLoop ) : m_loop( aLoop ) {}
            ~SET_FALSE() { m_loop->setInsideRun( false ); }
        } t( this );

        // Finally really run the loop.
        return DoRun();
    }

    bool ProcessEvents()
    {
        // process pending wx events first as they correspond to low-level events
        // which happened before, i.e. typically pending events were queued by a
        // previous call to Dispatch() and if we didn't process them now the next
        // call to it might enqueue them again (as happens with e.g. socket events
        // which would be generated as long as there is input available on socket
        // and this input is only removed from it when pending event handlers are
        // executed)
        if( wxTheApp )
        {
            wxTheApp->ProcessPendingEvents();

            // One of the pending event handlers could have decided to exit the
            // loop so check for the flag before trying to dispatch more events
            // (which could block indefinitely if no more are coming).
            if( m_shouldExit )
                return false;
        }

        return Dispatch();
    }


    int DoRun()
    {
        // we must ensure that OnExit() is called even if an exception is thrown
        // from inside ProcessEvents() but we must call it from Exit() in normal
        // situations because it is supposed to be called synchronously,
        // wxModalEventLoop depends on this (so we can't just use ON_BLOCK_EXIT or
        // something similar here)
    #if wxUSE_EXCEPTIONS
        for ( ;; )
        {
            try
            {
    #endif // wxUSE_EXCEPTIONS

                // this is the event loop itself
                for ( ;; )
                {
                    // generate and process idle events for as long as we don't
                    // have anything else to do
                    while ( !m_shouldExit && !Pending() && ProcessIdle() )
                        ;

                    if ( m_shouldExit )
                        break;

                    // a message came or no more idle processing to do, dispatch
                    // all the pending events and call Dispatch() to wait for the
                    // next message
                    if ( !ProcessEvents() )
                    {
                        // we got WM_QUIT
                        break;
                    }
                }

                // Process the remaining queued messages, both at the level of the
                // underlying toolkit level (Pending/Dispatch()) and wx level
                // (Has/ProcessPendingEvents()).
                //
                // We do run the risk of never exiting this loop if pending event
                // handlers endlessly generate new events but they shouldn't do
                // this in a well-behaved program and we shouldn't just discard the
                // events we already have, they might be important.
                for ( ;; )
                {
                    bool hasMoreEvents = false;
                    if ( wxTheApp && wxTheApp->HasPendingEvents() )
                    {
                        wxTheApp->ProcessPendingEvents();
                        hasMoreEvents = true;
                    }

                    if ( Pending() )
                    {
                        Dispatch();
                        hasMoreEvents = true;
                    }

                    if ( !hasMoreEvents )
                        break;
                }

    #if wxUSE_EXCEPTIONS
                // exit the outer loop as well
                break;
            }
            catch ( ... )
            {
                try
                {
                    if ( !wxTheApp || !wxTheApp->OnExceptionInMainLoop() )
                    {
                        OnExit();
                        break;
                    }
                    //else: continue running the event loop
                }
                catch ( ... )
                {
                    // OnException() throwed, possibly rethrowing the same
                    // exception again: very good, but we still need OnExit() to
                    // be called
                    OnExit();
                    throw;
                }
            }
        }
    #endif // wxUSE_EXCEPTIONS

        return m_exitcode;
    }

protected:
    int     m_exitcode;

    /* this only works if you add
    friend class EVENT_LOOP
    to EventLoopBase
    */
    void setInsideRun( bool aValue )
    {
        m_isInsideRun = aValue;
    }
#endif
};


int DIALOG_SHIM::ShowQuasiModal()
{
    // This is an exception safe way to zero a pointer before returning.
    // Yes, even though DismissModal() clears this first normally, this is
    // here in case there's an exception before the dialog is dismissed.
    struct NULLER
    {
        void*&  m_what;
        NULLER( void*& aPtr ) : m_what( aPtr ) {}
        ~NULLER() { m_what = 0; }   // indeed, set it to NULL on destruction
    } clear_this( (void*&) m_qmodal_loop );

    // release the mouse if it's currently captured as the window having it
    // will be disabled when this dialog is shown -- but will still keep the
    // capture making it impossible to do anything in the modal dialog itself
    wxWindow* win = wxWindow::GetCapture();
    if( win )
        win->ReleaseMouse();

    // Get the optimal parent
    wxWindow* parent = GetParentForModalDialog( GetParent(), GetWindowStyle() );

    // Show the optimal parent
    DBG( if( parent ) printf( "%s: optimal parent: %s\n", __func__, typeid(*parent).name() );)

    wxASSERT_MSG( !m_qmodal_parent_disabler,
            wxT( "Caller using ShowQuasiModal() twice on same window?" ) );

    // quasi-modal: disable only my "optimal" parent
    m_qmodal_parent_disabler = new WDO_ENABLE_DISABLE( parent );

    Show( true );

    m_qmodal_showing = true;

    EVENT_LOOP event_loop;

    m_qmodal_loop = &event_loop;

    event_loop.Run();

    return GetReturnCode();
}


void DIALOG_SHIM::EndQuasiModal( int retCode )
{
    SetReturnCode( retCode );

    if( !IsQuasiModal() )
    {
        wxFAIL_MSG( wxT( "either DIALOG_SHIM::EndQuasiModal called twice or ShowQuasiModal wasn't called" ) );
        return;
    }

    m_qmodal_showing = false;

    if( m_qmodal_loop )
    {
#if wxCHECK_VERSION( 2, 9, 4 )  // 2.9.4 is only approximate, might be 3, 0, 0.
        if( m_qmodal_loop->IsRunning() )
            m_qmodal_loop->Exit( 0 );
        else
            m_qmodal_loop->ScheduleExit( 0 );
#else
        m_qmodal_loop->Exit( 0 );
#endif
        m_qmodal_loop = NULL;
    }

    delete m_qmodal_parent_disabler;
    m_qmodal_parent_disabler = 0;

    Show( false );
}
