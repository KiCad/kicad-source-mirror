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

#include <dialogs/hotkey_cycle_popup.h>
#include <eda_draw_frame.h>

#ifdef __WXGTK__
#define LIST_BOX_H_PADDING 20
#define LIST_BOX_V_PADDING 8
#elif __WXMAC__
#define LIST_BOX_H_PADDING 40
#define LIST_BOX_V_PADDING 5
#else
#define LIST_BOX_H_PADDING 10
#define LIST_BOX_V_PADDING 5
#endif

#define SHOW_TIME_MS 500


HOTKEY_CYCLE_POPUP::HOTKEY_CYCLE_POPUP( EDA_DRAW_FRAME* aParent ) :
        EDA_VIEW_SWITCHER_BASE( aParent ),
        m_drawFrame( aParent )
{
    m_showTimer = new wxTimer( this );
    Bind( wxEVT_TIMER, [&]( wxTimerEvent& ) { Show( false ); m_drawFrame->GetCanvas()->SetFocus(); },
          m_showTimer->GetId() );

    // On macOS, we can't change focus to the canvas before sending the event, so the key events
    // just get discarded via the "don't steal from an input control" logic.  So, set this input
    // control with a special flag because we really do want to steal from it.
    m_listBox->SetName( KIUI::s_FocusStealableInputName );

#ifdef __WXOSX__
    m_listBox->Bind( wxEVT_CHAR_HOOK,
                     [=, this]( wxKeyEvent& aEvent )
                     {
                         aEvent.SetEventType( wxEVT_CHAR );
                         m_drawFrame->GetCanvas()->SetFocus();
                         m_drawFrame->GetCanvas()->OnEvent( aEvent );
                     } );
#endif
}


HOTKEY_CYCLE_POPUP::~HOTKEY_CYCLE_POPUP()
{
    delete m_showTimer;
}


void HOTKEY_CYCLE_POPUP::Popup( const wxString& aTitle, const wxArrayString& aItems,
                                int aSelection )
{
    m_stTitle->SetLabel( aTitle );
    m_listBox->Clear();
    m_listBox->InsertItems( aItems, 0 );
    m_listBox->SetSelection( std::min( aSelection,
                                       static_cast<int>( m_listBox->GetCount() ) - 1 ) );

    int width = m_stTitle->GetTextExtent( aTitle ).x;
    int height = 0;

    for( const wxString& item : aItems )
    {
        wxSize extents = m_listBox->GetTextExtent( item );
        width = std::max( width, extents.x );
        height += extents.y + LIST_BOX_V_PADDING;
    }

    m_listBox->SetMinSize( wxSize( width + LIST_BOX_H_PADDING,
                                   height + ( LIST_BOX_V_PADDING * 2 ) ) );

    // this line fixes an issue on Linux Ubuntu using Unity (dialog not shown),
    // and works fine on all systems
    GetSizer()->Fit( this );

    if( m_showTimer->IsRunning() )
    {
        m_showTimer->StartOnce( SHOW_TIME_MS );
        SetFocus();
        return;
    }

    m_showTimer->StartOnce( SHOW_TIME_MS );

    Show( true );
    Centre();
    SetFocus();
}


bool HOTKEY_CYCLE_POPUP::TryBefore( wxEvent& aEvent )
{
    if( aEvent.GetEventType() == wxEVT_CHAR || aEvent.GetEventType() == wxEVT_CHAR_HOOK )
    {
        aEvent.SetEventType( wxEVT_CHAR );
        //m_drawFrame->GetCanvas()->SetFocus(); // on GTK causes focus flicker and is not needed
        m_drawFrame->GetCanvas()->OnEvent( aEvent );
        return true;
    }

    return EDA_VIEW_SWITCHER_BASE::TryBefore( aEvent );
}
