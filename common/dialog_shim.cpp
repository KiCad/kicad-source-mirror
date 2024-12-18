/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2023 CERN
 * Copyright (C) 2012-2023, 2024 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <core/ignore.h>
#include <kiway_player.h>
#include <kiway.h>
#include <pgm_base.h>
#include <tool/tool_manager.h>
#include <kiplatform/ui.h>

#include <wx/display.h>
#include <wx/evtloop.h>
#include <wx/app.h>
#include <wx/event.h>
#include <wx/grid.h>
#include <wx/bmpbuttn.h>
#include <wx/textctrl.h>
#include <wx/stc/stc.h>

#include <algorithm>

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
            m_win->Raise(); // let's focus back on the parent window
        }
    }
};


BEGIN_EVENT_TABLE( DIALOG_SHIM, wxDialog )
    EVT_CHAR_HOOK( DIALOG_SHIM::OnCharHook )
END_EVENT_TABLE()


DIALOG_SHIM::DIALOG_SHIM( wxWindow* aParent, wxWindowID id, const wxString& title,
                          const wxPoint& pos, const wxSize& size, long style,
                          const wxString& name ) :
            wxDialog( aParent, id, title, pos, size, style, name ),
            KIWAY_HOLDER( nullptr, KIWAY_HOLDER::DIALOG ),
            m_units( EDA_UNITS::MILLIMETRES ),
            m_useCalculatedSize( false ),
            m_firstPaintEvent( true ),
            m_initialFocusTarget( nullptr ),
            m_isClosing( false ),
            m_qmodal_loop( nullptr ),
            m_qmodal_showing( false ),
            m_qmodal_parent_disabler( nullptr ),
            m_parentFrame( nullptr )
{
    KIWAY_HOLDER* kiwayHolder = nullptr;
    m_initialSize = size;

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
        m_parentFrame = static_cast<EDA_BASE_FRAME*>( kiwayHolder );
        TOOL_MANAGER* toolMgr = m_parentFrame->GetToolManager();

        if( toolMgr && toolMgr->IsContextMenuActive() )
            toolMgr->VetoContextMenuMouseWarp();
    }

    // Set up the message bus
    if( kiwayHolder )
        SetKiway( this, &kiwayHolder->Kiway() );

    if( HasKiway() )
        Kiway().SetBlockingDialog( this );

    Bind( wxEVT_CLOSE_WINDOW, &DIALOG_SHIM::OnCloseWindow, this );
    Bind( wxEVT_BUTTON, &DIALOG_SHIM::OnButton, this );

#ifdef __WINDOWS__
    // On Windows, the app top windows can be brought to the foreground (at least temporarily)
    // in certain circumstances such as when calling an external tool in Eeschema BOM generation.
    // So set the parent frame (if exists) to top window to avoid this annoying behavior.
    if( kiwayHolder && kiwayHolder->GetType() == KIWAY_HOLDER::FRAME )
        Pgm().App().SetTopWindow( (EDA_BASE_FRAME*) kiwayHolder );
#endif

    Bind( wxEVT_PAINT, &DIALOG_SHIM::OnPaint, this );
}


DIALOG_SHIM::~DIALOG_SHIM()
{
    m_isClosing = true;

    Unbind( wxEVT_CLOSE_WINDOW, &DIALOG_SHIM::OnCloseWindow, this );
    Unbind( wxEVT_BUTTON, &DIALOG_SHIM::OnButton, this );
    Unbind( wxEVT_PAINT, &DIALOG_SHIM::OnPaint, this );

    std::function<void( wxWindowList& )> disconnectFocusHandlers = [&]( wxWindowList& children )
    {
        for( wxWindow* child : children )
        {
            if( wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( child ) )
            {
                textCtrl->Disconnect( wxEVT_SET_FOCUS,
                                      wxFocusEventHandler( DIALOG_SHIM::onChildSetFocus ),
                                      nullptr, this );
            }
            else if( wxStyledTextCtrl* scintilla = dynamic_cast<wxStyledTextCtrl*>( child ) )
            {
                scintilla->Disconnect( wxEVT_SET_FOCUS,
                                       wxFocusEventHandler( DIALOG_SHIM::onChildSetFocus ),
                                       nullptr, this );
            }
            else
            {
                disconnectFocusHandlers( child->GetChildren() );
            }
        }
    };

    disconnectFocusHandlers( GetChildren() );

    // if the dialog is quasi-modal, this will end its event loop
    if( IsQuasiModal() )
        EndQuasiModal( wxID_CANCEL );

    if( HasKiway() )
        Kiway().SetBlockingDialog( nullptr );

    delete m_qmodal_parent_disabler;
}


void DIALOG_SHIM::finishDialogSettings()
{
    // must be called from the constructor of derived classes,
    // when all widgets are initialized, and therefore their size fixed

    // SetSizeHints fixes the minimal size of sizers in the dialog
    // (SetSizeHints calls Fit(), so no need to call it)
    GetSizer()->SetSizeHints( this );
}


void DIALOG_SHIM::setSizeInDU( int x, int y )
{
    wxSize sz( x, y );
    SetSize( ConvertDialogToPixels( sz ) );
}


int DIALOG_SHIM::horizPixelsFromDU( int x ) const
{
    wxSize sz( x, 0 );
    return ConvertDialogToPixels( sz ).x;
}


int DIALOG_SHIM::vertPixelsFromDU( int y ) const
{
    wxSize sz( 0, y );
    return ConvertDialogToPixels( sz ).y;
}


// our hashtable is an implementation secret, don't need or want it in a header file
#include <hashtables.h>
#include <typeinfo>

static std::unordered_map<std::string, wxRect> class_map;


void DIALOG_SHIM::SetPosition( const wxPoint& aNewPosition )
{
    wxDialog::SetPosition( aNewPosition );

    // Now update the stored position:
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

    std::unordered_map<std::string, wxRect>::iterator it = class_map.find( hash_key );

    if( it == class_map.end() )
        return;

    wxRect rect = it->second;
    rect.SetPosition( aNewPosition );

    class_map[ hash_key ] = rect;
}


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

        // classname is key, returns a zeroed-out default wxRect if none existed before.
        wxRect savedDialogRect = class_map[ hash_key ];

        if( savedDialogRect.GetSize().x != 0 && savedDialogRect.GetSize().y != 0 )
        {
            if( m_useCalculatedSize )
            {
                SetSize( savedDialogRect.GetPosition().x, savedDialogRect.GetPosition().y,
                         wxDialog::GetSize().x, wxDialog::GetSize().y, 0 );
            }
            else
            {
                SetSize( savedDialogRect.GetPosition().x, savedDialogRect.GetPosition().y,
                         std::max( wxDialog::GetSize().x, savedDialogRect.GetSize().x ),
                         std::max( wxDialog::GetSize().y, savedDialogRect.GetSize().y ),
                         0 );
            }
#ifdef __WXMAC__
            if( m_parent != nullptr )
            {
                if( wxDisplay::GetFromPoint( m_parent->GetPosition() )
                    != wxDisplay::GetFromPoint( savedDialogRect.GetPosition() ) )
                    Centre();
            }
#endif
        }
        else if( m_initialSize != wxDefaultSize )
        {
            SetSize( m_initialSize );
            Centre();
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
        class_map[ hash_key ] = wxRect( wxDialog::GetPosition(), wxDialog::GetSize() );

#ifdef __WXMAC__
        if ( m_eventLoop )
            m_eventLoop->Exit( GetReturnCode() );   // Needed for APP-MODAL dlgs on OSX
#endif

        ret = wxDialog::Show( show );

        if( m_parent )
            m_parent->SetFocus();
    }

    return ret;
}


void DIALOG_SHIM::resetSize()
{
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

    std::unordered_map<std::string, wxRect>::iterator it = class_map.find( hash_key );

    if( it == class_map.end() )
        return;

    wxRect rect = it->second;
    rect.SetSize( wxSize( 0, 0 ) );
    class_map[ hash_key ] = rect;
}


bool DIALOG_SHIM::Enable( bool enable )
{
    // so we can do logging of this state change:
    return wxDialog::Enable( enable );
}


// Recursive descent doing a SelectAll() in wxTextCtrls.
// MacOS User Interface Guidelines state that when tabbing to a text control all its
// text should be selected.  Since wxWidgets fails to implement this, we do it here.
void DIALOG_SHIM::SelectAllInTextCtrls( wxWindowList& children )
{
    for( wxWindow* child : children )
    {
        if( wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( child ) )
        {
            m_beforeEditValues[ textCtrl ] = textCtrl->GetValue();
            textCtrl->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( DIALOG_SHIM::onChildSetFocus ),
                               nullptr, this );

            // We don't currently run this on GTK because some window managers don't hide the
            // selection in non-active controls, and other window managers do the selection
            // automatically anyway.
#if defined( __WXMAC__ ) || defined( __WXMSW__ )
            if( !textCtrl->GetStringSelection().IsEmpty() )
            {
                // Respect an existing selection
            }
            else if( textCtrl->IsEditable() )
            {
                textCtrl->SelectAll();
            }
#else
            ignore_unused( textCtrl );
#endif
        }
        else if( wxStyledTextCtrl* scintilla = dynamic_cast<wxStyledTextCtrl*>( child ) )
        {
            m_beforeEditValues[ scintilla ] = scintilla->GetText();
            scintilla->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( DIALOG_SHIM::onChildSetFocus ),
                                nullptr, this );

            if( !scintilla->GetSelectedText().IsEmpty() )
            {
                // Respect an existing selection
            }
            else if( scintilla->GetMarginWidth( 0 ) > 0 )
            {
                // Don't select-all in Custom Rules, etc.
            }
            else if( scintilla->IsEditable() )
            {
                scintilla->SelectAll();
            }
        }
#ifdef __WXMAC__
        // Temp hack for square (looking) buttons on OSX.  Will likely be made redundant
        // by the image store....
        else if( dynamic_cast<wxBitmapButton*>( child ) != nullptr )
        {
            wxSize minSize( 29, 27 );
            wxRect rect = child->GetRect();

            child->ConvertDialogToPixels( minSize );

            rect.Inflate( std::max( 0, minSize.x - rect.GetWidth() ),
                          std::max( 0, minSize.y - rect.GetHeight() ) );

            child->SetMinSize( rect.GetSize() );
            child->SetSize( rect );
        }
#endif
        else
        {
            SelectAllInTextCtrls( child->GetChildren() );
        }
    }
}


void DIALOG_SHIM::OnPaint( wxPaintEvent &event )
{
    if( m_firstPaintEvent )
    {
        KIPLATFORM::UI::FixupCancelButtonCmdKeyCollision( this );

        SelectAllInTextCtrls( GetChildren() );

        if( m_initialFocusTarget )
            KIPLATFORM::UI::ForceFocus( m_initialFocusTarget );
        else
            KIPLATFORM::UI::ForceFocus( this );     // Focus the dialog itself

        m_firstPaintEvent = false;
    }

    event.Skip();
}


void DIALOG_SHIM::OnModify()
{
    if( !GetTitle().StartsWith( wxS( "*" ) ) )
        SetTitle( wxS( "*" ) + GetTitle() );
}


int DIALOG_SHIM::ShowModal()
{
    // Apple in its infinite wisdom will raise a disabled window before even passing
    // us the event, so we have no way to stop it.  Instead, we must set an order on
    // the windows so that the modal will be pushed in front of the disabled
    // window when it is raised.
    KIPLATFORM::UI::ReparentModal( this );

    // Call the base class ShowModal() method
    return wxDialog::ShowModal();
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
        ~NULLER() { m_what = nullptr; }   // indeed, set it to NULL on destruction
    } clear_this( (void*&) m_qmodal_loop );

    // release the mouse if it's currently captured as the window having it
    // will be disabled when this dialog is shown -- but will still keep the
    // capture making it impossible to do anything in the modal dialog itself
    wxWindow* win = wxWindow::GetCapture();
    if( win )
        win->ReleaseMouse();

    // Get the optimal parent
    wxWindow* parent = GetParentForModalDialog( GetParent(), GetWindowStyle() );

    wxASSERT_MSG( !m_qmodal_parent_disabler, wxT( "Caller using ShowQuasiModal() twice on same "
                                                  "window?" ) );

    // quasi-modal: disable only my "optimal" parent
    m_qmodal_parent_disabler = new WDO_ENABLE_DISABLE( parent );

    // Apple in its infinite wisdom will raise a disabled window before even passing
    // us the event, so we have no way to stop it.  Instead, we must set an order on
    // the windows so that the quasi-modal will be pushed in front of the disabled
    // window when it is raised.
    KIPLATFORM::UI::ReparentModal( this );

    Show( true );

    m_qmodal_showing = true;

    WX_EVENT_LOOP event_loop;

    m_qmodal_loop = &event_loop;

    event_loop.Run();

    m_qmodal_showing = false;

    if( parent )
        parent->SetFocus();

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
        wxFAIL_MSG( wxT( "Either DIALOG_SHIM::EndQuasiModal was called twice, or ShowQuasiModal"
                         "wasn't called" ) );
        return;
    }

    TearDownQuasiModal();

    if( m_qmodal_loop )
    {
        if( m_qmodal_loop->IsRunning() )
            m_qmodal_loop->Exit( 0 );
        else
            m_qmodal_loop->ScheduleExit( 0 );

        m_qmodal_loop = nullptr;
    }

    delete m_qmodal_parent_disabler;
    m_qmodal_parent_disabler = nullptr;

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
                ignore_unused( TransferDataFromWindow() );
            }
        }
        else if( id == wxID_CANCEL )
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


void DIALOG_SHIM::onChildSetFocus( wxFocusEvent& aEvent )
{
    // When setting focus to a text control reset the before-edit value.

    if( !m_isClosing )
    {
        if( wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( aEvent.GetEventObject() ) )
            m_beforeEditValues[ textCtrl ] = textCtrl->GetValue();
        else if( wxStyledTextCtrl* scintilla = dynamic_cast<wxStyledTextCtrl*>( aEvent.GetEventObject() ) )
            m_beforeEditValues[ scintilla ] = scintilla->GetText();
    }

    aEvent.Skip();
}


void DIALOG_SHIM::OnCharHook( wxKeyEvent& aEvt )
{
    if( aEvt.GetKeyCode() == 'U' && aEvt.GetModifiers() == wxMOD_CONTROL )
    {
        if( m_parentFrame )
        {
            m_parentFrame->ToggleUserUnits();
            return;
        }
    }
    // shift-return (Mac default) or Ctrl-Return (GTK) for OK
    else if( ( aEvt.GetKeyCode() == WXK_RETURN || aEvt.GetKeyCode() == WXK_NUMPAD_ENTER )
             && ( aEvt.ShiftDown() || aEvt.ControlDown() ) )
    {
        wxPostEvent( this, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK ) );
        return;
    }
    else if( aEvt.GetKeyCode() == WXK_TAB && !aEvt.ControlDown() )
    {
        wxWindow* currentWindow = wxWindow::FindFocus();
        int       currentIdx = -1;
        int       delta = aEvt.ShiftDown() ? -1 : 1;

        auto advance =
                [&]( int& idx )
                {
                    // Wrap-around modulus
                    int size = (int) m_tabOrder.size();
                    idx = ( ( idx + delta ) % size + size ) % size;
                };

        for( size_t i = 0; i < m_tabOrder.size(); ++i )
        {
            if( m_tabOrder[i] == currentWindow )
            {
                currentIdx = (int) i;
                break;
            }
        }

        if( currentIdx >= 0 )
        {
            advance( currentIdx );

            //todo: We don't currently have non-textentry dialog boxes but this will break if
            // we add them.
#ifdef __APPLE__
            while( dynamic_cast<wxTextEntry*>( m_tabOrder[ currentIdx ] ) == nullptr )
                advance( currentIdx );
#endif

            m_tabOrder[ currentIdx ]->SetFocus();
            return;
        }
    }
    else if( aEvt.GetKeyCode() == WXK_ESCAPE )
    {
        wxObject* eventSource = aEvt.GetEventObject();

        if( wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( eventSource ) )
        {
            // First escape after an edit cancels edit
            if( textCtrl->GetValue() != m_beforeEditValues[ textCtrl ] )
            {
                textCtrl->SetValue( m_beforeEditValues[ textCtrl ] );
                textCtrl->SelectAll();
                return;
            }
        }
        else if( wxStyledTextCtrl* scintilla = dynamic_cast<wxStyledTextCtrl*>( eventSource ) )
        {
            // First escape after an edit cancels edit
            if( scintilla->GetText() != m_beforeEditValues[ scintilla ] )
            {
                scintilla->SetText( m_beforeEditValues[ scintilla ] );
                scintilla->SelectAll();
                return;
            }
        }
    }

    aEvt.Skip();
}


static void recursiveDescent( wxSizer* aSizer, std::map<int, wxString>& aLabels )
{
    wxStdDialogButtonSizer* sdbSizer = dynamic_cast<wxStdDialogButtonSizer*>( aSizer );

    auto setupButton =
            [&]( wxButton* aButton )
            {
                if( aLabels.count( aButton->GetId() ) > 0 )
                {
                    aButton->SetLabel( aLabels[ aButton->GetId() ] );
                }
                else
                {
                    // wxWidgets has an uneven track record when the language is changed on
                    // the fly so we set them even when they don't appear in the label map
                    switch( aButton->GetId() )
                    {
                    case wxID_OK:           aButton->SetLabel( _( "&OK" ) );     break;
                    case wxID_CANCEL:       aButton->SetLabel( _( "&Cancel" ) ); break;
                    case wxID_YES:          aButton->SetLabel( _( "&Yes" ) );    break;
                    case wxID_NO:           aButton->SetLabel( _( "&No" ) );     break;
                    case wxID_APPLY:        aButton->SetLabel( _( "&Apply" ) );  break;
                    case wxID_SAVE:         aButton->SetLabel( _( "&Save" ) );   break;
                    case wxID_HELP:         aButton->SetLabel( _( "&Help" ) );   break;
                    case wxID_CONTEXT_HELP: aButton->SetLabel( _( "&Help" ) );   break;
                    }
                }
            };

    if( sdbSizer )
    {
        if( sdbSizer->GetAffirmativeButton() )
            setupButton( sdbSizer->GetAffirmativeButton() );

        if( sdbSizer->GetApplyButton() )
            setupButton( sdbSizer->GetApplyButton() );

        if( sdbSizer->GetNegativeButton() )
            setupButton( sdbSizer->GetNegativeButton() );

        if( sdbSizer->GetCancelButton() )
            setupButton( sdbSizer->GetCancelButton() );

        if( sdbSizer->GetHelpButton() )
            setupButton( sdbSizer->GetHelpButton() );

        sdbSizer->Layout();

        if( sdbSizer->GetAffirmativeButton() )
            sdbSizer->GetAffirmativeButton()->SetDefault();
    }

    for( wxSizerItem* item : aSizer->GetChildren() )
    {
        if( item->GetSizer() )
            recursiveDescent( item->GetSizer(), aLabels );
    }
}


void DIALOG_SHIM::SetupStandardButtons( std::map<int, wxString> aLabels )
{
    recursiveDescent( GetSizer(), aLabels );
}
