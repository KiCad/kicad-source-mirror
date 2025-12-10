/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialogs/eda_view_switcher.h>


#ifdef __WXGTK__
#define LIST_BOX_H_PADDING 20
#define LIST_BOX_V_PADDING 8
#else
#define LIST_BOX_H_PADDING 10
#define LIST_BOX_V_PADDING 5
#endif


EDA_VIEW_SWITCHER::EDA_VIEW_SWITCHER( wxWindow* aParent, const wxArrayString& aItems,
                                      wxKeyCode aCtrlKey ) :
        EDA_VIEW_SWITCHER_BASE( aParent ),
        // Start with the tab marked as "up" so the initial ctrl-tab press advances selection.
        m_tabState( false ),
        m_receivingEvents( false ),
        m_ctrlKey( aCtrlKey )
{
    m_listBox->InsertItems( aItems, 0 );
    m_listBox->SetSelection( std::min( 0, (int) m_listBox->GetCount() - 1 ) );

    int width = 0;
    int height = 0;

    for( const wxString& item : aItems )
    {
        wxSize extents = m_listBox->GetTextExtent( item );
        width = std::max( width, extents.x );
        height += extents.y + LIST_BOX_V_PADDING;
    }

    m_listBox->SetMinSize( wxSize( width + LIST_BOX_H_PADDING,
                                   height + ( LIST_BOX_V_PADDING * 2 ) ) );
    SetInitialFocus( m_listBox );

    // this line fixes an issue on Linux Ubuntu using Unity (dialog not shown),
    // and works fine on all systems
    GetSizer()->Fit( this );

    Centre();
}


bool EDA_VIEW_SWITCHER::Show( bool aShow )
{
    if( !aShow )
        m_receivingEvents = false;

    bool ret = DIALOG_SHIM::Show( aShow );

    if( aShow )
    {
        m_receivingEvents = true;

        // Force the dialog to always be centered over the window
        Centre();
    }

    return ret;
}


// OK, this is *really* annoying, but wxWidgets doesn't give us key-down events while the
// control key is being held down.  So we can't use OnKeyDown() or OnCharHook() and instead
// must rely on watching key states in TryBefore().
//
// Just checking the state of the tab key is tempting, but then we'll think it's been hit
// several times when it's actually just a key-down followed by a redraw or idle event.
//
// So we have to keep a state machine of the tab key.
//
bool EDA_VIEW_SWITCHER::TryBefore( wxEvent& aEvent )
{
    if( !m_receivingEvents )
    {
        return DIALOG_SHIM::TryBefore( aEvent );
    }

    // Check for tab key leading edge
    if( !m_tabState && wxGetKeyState( WXK_TAB ) )
    {
        m_tabState = true;

        int idx = m_listBox->GetSelection();

        if( wxGetKeyState( WXK_SHIFT ) && m_ctrlKey != WXK_SHIFT )
        {
            if( --idx < 0 )
                m_listBox->SetSelection( (int) m_listBox->GetCount() - 1 );
            else
                m_listBox->SetSelection( idx );
        }
        else
        {
            if( ++idx >= (int) m_listBox->GetCount() )
                m_listBox->SetSelection( 0 );
            else
                m_listBox->SetSelection( idx );
        }

        return true;
    }

    // Check for tab key trailing edge
    if( m_tabState && !wxGetKeyState( WXK_TAB ) )
    {
        m_tabState = false;
    }

    // Check for control key trailing edge
    if( !wxGetKeyState( m_ctrlKey ) )
    {
        wxPostEvent( this, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK ) );
    }
    else if( wxGetKeyState( WXK_ESCAPE ) )
    {
        wxPostEvent( this, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_CANCEL ) );
    }

    return DIALOG_SHIM::TryBefore( aEvent );
}
