
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



DIALOG_SHIM::DIALOG_SHIM( wxWindow* aParent, wxWindowID id, const wxString& title,
        const wxPoint& pos, const wxSize& size, long style, const wxString& name ) :
    wxDialog( aParent, id, title, pos, size, style, name ),
    KIWAY_HOLDER( 0 ),
    m_qmodal_loop( 0 ),
    m_qmodal_showing( false )
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


int DIALOG_SHIM::ShowQuasiModal()
{
    // toggle a window's "enable" status to disabled, then enabled on exit.
    // exception safe.
    struct ENABLE_DISABLE
    {
        wxWindow* m_win;
        ENABLE_DISABLE( wxWindow* aWindow ) : m_win( aWindow ) { if( m_win ) m_win->Disable(); }
        ~ENABLE_DISABLE()
        {
            if( m_win )
            {
                m_win->Enable();
                m_win->SetFocus(); // let's focus back on the parent window
            }
        }
    };

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

    ENABLE_DISABLE  toggle( parent );       // quasi-modal: disable only my "optimal" parent

    Show( true );

    m_qmodal_showing = true;

    WX_EVENT_LOOP           event_loop;

#if wxCHECK_VERSION( 2, 9, 4 )  // 2.9.4 is only approximate.
    // new code needs this, old code does it in wxEventLoop::Run() and cannot
    // tolerate it here. Where that boundary is as a version number, I don't know.
    // A closer look at the subversion repo for wx would tell.
    wxEventLoopActivator    event_loop_stacker( &event_loop );
#endif

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
        m_qmodal_loop->Exit();
        m_qmodal_loop = NULL;
    }

    Show( false );
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
