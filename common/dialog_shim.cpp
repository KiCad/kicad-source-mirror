/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <pgm_base.h>
#include <tool/tool_manager.h>
#include <eda_rect.h>
#include <wx/display.h>
#include <wx/grid.h>

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


BEGIN_EVENT_TABLE( DIALOG_SHIM, wxDialog )
    // If dialog has a grid and the grid has an active cell editor
    // Esc key closes cell editor, otherwise Esc key closes the dialog.
    EVT_GRID_EDITOR_SHOWN( DIALOG_SHIM::OnGridEditorShown )
    EVT_GRID_EDITOR_HIDDEN( DIALOG_SHIM::OnGridEditorHidden )
    EVT_CHAR_HOOK( DIALOG_SHIM::OnCharHook )
END_EVENT_TABLE()


DIALOG_SHIM::DIALOG_SHIM( wxWindow* aParent, wxWindowID id, const wxString& title,
                          const wxPoint& pos, const wxSize& size, long style, 
                          const wxString& name ) :
        wxDialog( aParent, id, title, pos, size, style, name ),
        KIWAY_HOLDER( nullptr, KIWAY_HOLDER::DIALOG ),
        m_units( MILLIMETRES ),
        m_firstPaintEvent( true ),
        m_initialFocusTarget( nullptr ),
        m_qmodal_loop( nullptr ),
        m_qmodal_showing( false ),
        m_qmodal_parent_disabler( nullptr )
{
    KIWAY_HOLDER* kiwayHolder = nullptr;

    if( aParent )
    {
        kiwayHolder = dynamic_cast<KIWAY_HOLDER*>( aParent );

        while( !kiwayHolder && aParent->GetParent() )
        {
            aParent = aParent->GetParent();
            kiwayHolder = dynamic_cast<KIWAY_HOLDER*>( aParent );
        }
    }

    // Inherit units from parent
    if( kiwayHolder && kiwayHolder->GetType() == KIWAY_HOLDER::FRAME )
        m_units = static_cast<EDA_BASE_FRAME*>( kiwayHolder )->GetUserUnits();
    else if( kiwayHolder && kiwayHolder->GetType() == KIWAY_HOLDER::DIALOG )
        m_units = static_cast<DIALOG_SHIM*>( kiwayHolder )->GetUserUnits();

    // Don't mouse-warp after a dialog run from the context menu
    if( kiwayHolder && kiwayHolder->GetType() == KIWAY_HOLDER::FRAME )
    {
        TOOL_MANAGER* toolMgr = static_cast<EDA_BASE_FRAME*>( kiwayHolder )->GetToolManager();

        if( toolMgr && toolMgr->IsContextMenuActive() )
            toolMgr->VetoContextMenuMouseWarp();
    }

    // Set up the message bus
    if( kiwayHolder )
        SetKiway( this, &kiwayHolder->Kiway() );

    Bind( wxEVT_CLOSE_WINDOW, &DIALOG_SHIM::OnCloseWindow, this );
    Bind( wxEVT_BUTTON, &DIALOG_SHIM::OnButton, this );

#ifdef __WINDOWS__
    // On Windows, the app top windows can be brought to the foreground (at least temporarily) 
    // in certain circumstances such as when calling an external tool in Eeschema BOM generation.
    // So set the parent frame (if exists) to top window to avoid this annoying behavior.
    if( kiwayHolder && kiwayHolder->GetType() == KIWAY_HOLDER::FRAME )
        Pgm().App().SetTopWindow( (EDA_BASE_FRAME*) kiwayHolder );
#endif

    Connect( wxEVT_PAINT, wxPaintEventHandler( DIALOG_SHIM::OnPaint ) );
}


DIALOG_SHIM::~DIALOG_SHIM()
{
    // if the dialog is quasi-modal, this will end its event loop
    if( IsQuasiModal() )
        EndQuasiModal( wxID_CANCEL );

    if( m_qmodal_parent_disabler )
        delete m_qmodal_parent_disabler;    // usually NULL by now
}


void DIALOG_SHIM::FinishDialogSettings()
{
    // must be called from the constructor of derived classes,
    // when all widgets are initialized, and therefore their size fixed

    // SetSizeHints fixes the minimal size of sizers in the dialog
    // (SetSizeHints calls Fit(), so no need to call it)
    GetSizer()->SetSizeHints( this );

    // the default position, when calling the first time the dlg
    Center();
}


void DIALOG_SHIM::SetSizeInDU( int x, int y )
{
    wxSize sz( x, y );
    SetSize( ConvertDialogToPixels( sz ) );
}


int DIALOG_SHIM::HorizPixelsFromDU( int x )
{
    wxSize sz( x, 0 );
    return ConvertDialogToPixels( sz ).x;
}


int DIALOG_SHIM::VertPixelsFromDU( int y )
{
    wxSize sz( 0, y );
    return ConvertDialogToPixels( sz ).y;
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
#ifndef __WINDOWS__
        wxDialog::Raise();  // Needed on OS X and some other window managers (i.e. Unity)
#endif
        ret = wxDialog::Show( show );

        // classname is key, returns a zeroed out default EDA_RECT if none existed before.
        EDA_RECT savedDialogRect = class_map[ hash_key ];

        if( savedDialogRect.GetSize().x != 0 && savedDialogRect.GetSize().y != 0 )
        {
            SetSize( savedDialogRect.GetPosition().x,
                     savedDialogRect.GetPosition().y,
                     std::max( wxDialog::GetSize().x, savedDialogRect.GetSize().x ),
                     std::max( wxDialog::GetSize().y, savedDialogRect.GetSize().y ),
                     0 );
        }

        // Be sure that the dialog appears in a visible area
        // (the dialog position might have been stored at the time when it was
        // shown on another display)
        if( wxDisplay::GetFromWindow( this ) == wxNOT_FOUND )
            Centre();
    }
    else
    {
        // Save the dialog's position & size before hiding, using classname as key
        class_map[ hash_key ] = EDA_RECT( wxDialog::GetPosition(), wxDialog::GetSize() );

#ifdef __WXMAC__
        if ( m_eventLoop )
            m_eventLoop->Exit( GetReturnCode() );   // Needed for APP-MODAL dlgs on OSX
#endif

        ret = wxDialog::Show( show );
    }

    return ret;
}


bool DIALOG_SHIM::Enable( bool enable )
{
    // so we can do logging of this state change:

#if 0 && defined(DEBUG)
    const char* type_id = typeid( *this ).name();
    printf( "DIALOG_SHIM %s: %s\n", type_id, enable ? "enabled" : "disabled" );
    fflush(0);  //Needed on msys2 to immediately print the message
#endif

    return wxDialog::Enable( enable );
}


// Recursive descent doing a SelectAll() in wxTextCtrls.
// MacOS User Interface Guidelines state that when tabbing to a text control all its
// text should be selected.  Since wxWidgets fails to implement this, we do it here.
static void selectAllInTextCtrls( wxWindowList& children )
{
    for( wxWindow* child : children )
    {
        wxTextCtrl* childTextCtrl = dynamic_cast<wxTextCtrl*>( child );
        if( childTextCtrl )
        {
            wxTextEntry* asTextEntry = dynamic_cast<wxTextEntry*>( childTextCtrl );

            // Respect an existing selection
            if( asTextEntry->GetStringSelection().IsEmpty() )
                asTextEntry->SelectAll();
        }
        else
            selectAllInTextCtrls( child->GetChildren() );
    }
}


#ifdef  __WXMAC__
static void fixOSXCancelButtonIssue( wxWindow *aWindow )
{
    // A ugly hack to fix an issue on OSX: cmd+c closes the dialog instead of
    // copying the text if a button with wxID_CANCEL is used in a
    // wxStdDialogButtonSizer created by wxFormBuilder: the label is &Cancel, and
    // this accelerator key has priority over the standard copy accelerator.
    // Note: problem also exists in other languages; for instance cmd+a closes
    // dialogs in German because the button is &Abbrechen.
    wxButton* button = dynamic_cast<wxButton*>( wxWindow::FindWindowById( wxID_CANCEL, aWindow ) );

    if( button )
    {
        static const wxString placeholder = wxT( "{amp}" );

        wxString buttonLabel = button->GetLabel();
        buttonLabel.Replace( wxT( "&&" ), placeholder );
        buttonLabel.Replace( wxT( "&" ), wxEmptyString );
        buttonLabel.Replace( placeholder, wxT( "&" ) );
        button->SetLabel( buttonLabel );
    }
}
#endif


void DIALOG_SHIM::OnPaint( wxPaintEvent &event )
{
    if( m_firstPaintEvent )
    {
#ifdef __WXMAC__
        fixOSXCancelButtonIssue( this );
#endif

        selectAllInTextCtrls( GetChildren() );

        if( m_initialFocusTarget )
            m_initialFocusTarget->SetFocus();
        else
            SetFocus();     // Focus the dialog itself

        m_firstPaintEvent = false;
    }

    event.Skip();
}


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

#ifdef  __WXMAC__
    // Apple in its infinite wisdom will raise a disabled window before even passing
    // us the event, so we have no way to stop it.  Instead, we must set an order on
    // the windows so that the quasi-modal will be pushed in front of the disabled
    // window when it is raised.
    ReparentQuasiModal();
#endif
    Show( true );

    m_qmodal_showing = true;

    WX_EVENT_LOOP event_loop;

    m_qmodal_loop = &event_loop;

    event_loop.Run();

    m_qmodal_showing = false;

    return GetReturnCode();
}


void DIALOG_SHIM::EndQuasiModal( int retCode )
{
    // Hook up validator and transfer data from controls handling so quasi-modal dialogs
    // handle validation in the same way as other dialogs.
    if( ( retCode == wxID_OK ) && ( !Validate() || !TransferDataFromWindow() ) )
        return;

    SetReturnCode( retCode );

    if( !IsQuasiModal() )
    {
        wxFAIL_MSG( wxT( "either DIALOG_SHIM::EndQuasiModal called twice or ShowQuasiModal wasn't called" ) );
        return;
    }

    if( m_qmodal_loop )
    {
        if( m_qmodal_loop->IsRunning() )
            m_qmodal_loop->Exit( 0 );
        else
            m_qmodal_loop->ScheduleExit( 0 );

        m_qmodal_loop = NULL;
    }

    delete m_qmodal_parent_disabler;
    m_qmodal_parent_disabler = 0;

    Show( false );
}


void DIALOG_SHIM::OnCloseWindow( wxCloseEvent& aEvent )
{
    if( IsQuasiModal() )
    {
        EndQuasiModal( wxID_CANCEL );
        return;
    }

    // This is mandatory to allow wxDialogBase::OnCloseWindow() to be called.
    aEvent.Skip();
}


void DIALOG_SHIM::OnButton( wxCommandEvent& aEvent )
{
    const int id = aEvent.GetId();

    // If we are pressing a button to exit, we need to enable the escapeID
    // otherwise the dialog does not process cancel
    if( id == wxID_CANCEL )
        SetEscapeId( wxID_ANY );

    if( IsQuasiModal() )
    {
        if( id == GetAffirmativeId() )
        {
            EndQuasiModal( id );
        }
        else if( id == wxID_APPLY )
        {
            // Dialogs that provide Apply buttons should make sure data is valid before
            // allowing a transfer, as there is no other way to indicate failure
            // (i.e. the dialog can't refuse to close as it might with OK, because it
            // isn't closing anyway)
            if( Validate() )
            {
                bool success = TransferDataFromWindow();
                (void) success;
            }
        }
        else if( id == GetEscapeId() ||
                 (id == wxID_CANCEL && GetEscapeId() == wxID_ANY) )
        {
            EndQuasiModal( wxID_CANCEL );
        }
        else // not a standard button
        {
            aEvent.Skip();
        }

        return;
    }

    // This is mandatory to allow wxDialogBase::OnButton() to be called.
    aEvent.Skip();
}


void DIALOG_SHIM::OnCharHook( wxKeyEvent& aEvt )
{
    // shift-return (Mac default) or Ctrl-Return (GTK) for OK
    if( aEvt.GetKeyCode() == WXK_RETURN && ( aEvt.ShiftDown() || aEvt.ControlDown() ) )
        wxPostEvent( this, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK ) );
    else
        aEvt.Skip();
}


void DIALOG_SHIM::OnGridEditorShown( wxGridEvent& event )
{
    SetEscapeId( wxID_NONE );
    event.Skip();
}


void DIALOG_SHIM::OnGridEditorHidden( wxGridEvent& event )
{
    SetEscapeId( wxID_ANY );
    event.Skip();
}
