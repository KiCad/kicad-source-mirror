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

#include "widgets/listbox_tricks.h"

#include <wx/clipbrd.h>
#include <wx/listbox.h>
#include <wx/menu.h>
#include <wx/window.h>

#include <bitmaps.h>
#include <bitmaps/bitmaps_list.h>
#include <widgets/ui_common.h>


wxDEFINE_EVENT( EDA_EVT_LISTBOX_COPY, wxCommandEvent );
wxDEFINE_EVENT( EDA_EVT_LISTBOX_CUT, wxCommandEvent );
wxDEFINE_EVENT( EDA_EVT_LISTBOX_PASTE, wxCommandEvent );
wxDEFINE_EVENT( EDA_EVT_LISTBOX_DELETE, wxCommandEvent );
wxDEFINE_EVENT( EDA_EVT_LISTBOX_DUPLICATE, wxCommandEvent );

wxDEFINE_EVENT( EDA_EVT_LISTBOX_CHANGED, wxCommandEvent );


LISTBOX_TRICKS::LISTBOX_TRICKS( wxWindow& aParent, wxListBox& aListBox ) :
        m_parent( aParent ),
        m_listBox( aListBox )
{
    m_listBox.Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( LISTBOX_TRICKS::OnListBoxRDown ),
                       nullptr, this );
    m_listBox.Connect( wxEVT_KEY_DOWN, wxKeyEventHandler( LISTBOX_TRICKS::OnListBoxKeyDown ),
                       nullptr, this );

    Connect( EDA_EVT_LISTBOX_DELETE, wxCommandEventHandler( LISTBOX_TRICKS::OnListBoxDelete ) );
    Connect( EDA_EVT_LISTBOX_COPY, wxCommandEventHandler( LISTBOX_TRICKS::OnListBoxCopy ) );
    Connect( EDA_EVT_LISTBOX_CUT, wxCommandEventHandler( LISTBOX_TRICKS::OnListBoxCut ) );
    Connect( EDA_EVT_LISTBOX_PASTE, wxCommandEventHandler( LISTBOX_TRICKS::OnListBoxPaste ) );
    Connect( EDA_EVT_LISTBOX_DUPLICATE, wxCommandEventHandler( LISTBOX_TRICKS::OnListBoxDuplicate ) );
}


LISTBOX_TRICKS::~LISTBOX_TRICKS()
{
    m_listBox.Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( LISTBOX_TRICKS::OnListBoxRDown ),
                          nullptr, this );
    m_listBox.Disconnect( wxEVT_KEY_DOWN, wxKeyEventHandler( LISTBOX_TRICKS::OnListBoxKeyDown ),
                          nullptr, this );

    Disconnect( EDA_EVT_LISTBOX_DELETE, wxCommandEventHandler( LISTBOX_TRICKS::OnListBoxDelete ) );
    Disconnect( EDA_EVT_LISTBOX_COPY, wxCommandEventHandler( LISTBOX_TRICKS::OnListBoxCopy ) );
    Disconnect( EDA_EVT_LISTBOX_CUT, wxCommandEventHandler( LISTBOX_TRICKS::OnListBoxCut ) );
    Disconnect( EDA_EVT_LISTBOX_PASTE, wxCommandEventHandler( LISTBOX_TRICKS::OnListBoxPaste ) );
    Disconnect( EDA_EVT_LISTBOX_DUPLICATE, wxCommandEventHandler( LISTBOX_TRICKS::OnListBoxDuplicate ) );
}


wxArrayInt LISTBOX_TRICKS::listBoxDeleteSelected()
{
    wxArrayInt selections;
    m_listBox.GetSelections( selections );

    if( selections.GetCount() == 0 )    // Nothing to remove
        return selections;

    std::sort( selections.begin(), selections.end() );

    for( int ii = selections.GetCount() - 1; ii >= 0; ii-- )
        m_listBox.Delete( selections[ii] );

    m_listBox.SetSelection( wxNOT_FOUND );

    if( m_listBox.GetCount() > 0 )
        m_listBox.SetSelection( std::max( 0, selections[0] - 1 ) );

    wxPostEvent( &m_listBox, wxCommandEvent( EDA_EVT_LISTBOX_CHANGED ) );
    return selections;
}


wxArrayString LISTBOX_TRICKS::listBoxGetSelected() const
{
    wxArrayInt selections;
    m_listBox.GetSelections( selections );

    wxArrayString result;

    for( size_t ii = 0; ii < selections.GetCount(); ii++ )
        result.Add( m_listBox.GetString( selections[ii] ) );

    return result;
}


void LISTBOX_TRICKS::listBoxDuplicateSelected()
{
    wxArrayInt selections;
    m_listBox.GetSelections( selections );

    int insertAt = selections.GetCount() > 0 ? selections.back() + 1 : m_listBox.GetCount();

    m_listBox.SetSelection( wxNOT_FOUND );

    for( size_t ii = 0; ii < selections.GetCount(); ii++ )
    {
        wxString filter = m_listBox.GetString( selections[ii] );
        m_listBox.Insert( filter, insertAt );
        m_listBox.SetSelection( insertAt );
        insertAt++;
    }

    wxPostEvent( &m_listBox, wxCommandEvent( EDA_EVT_LISTBOX_CHANGED ) );
}


void LISTBOX_TRICKS::listBoxCopy()
{
    wxArrayString filters = listBoxGetSelected();
    wxString      result;

    for( const wxString& filter : filters )
        result += filter + wxT( "\n" );

    if( wxTheClipboard->Open() )
    {
        wxTheClipboard->SetData( new wxTextDataObject( result ) );
        wxTheClipboard->Close();
    }
}


void LISTBOX_TRICKS::listBoxPaste()
{
    wxArrayString lines;

    if( wxTheClipboard->Open() )
    {
        wxTextDataObject data;
        wxTheClipboard->GetData( data );

        wxString text = data.GetText();
        text.Trim( false );
        text.Trim( true );
        lines = wxSplit( text, '\n' );

        wxTheClipboard->Close();
    }

    if( lines.IsEmpty() )
        return;

    wxArrayInt selections;
    m_listBox.GetSelections( selections );
    int insertAt = selections.GetCount() > 0 ? (int) selections.back() + 1 : (int) m_listBox.GetCount();

    for( wxString& line : lines )
    {
        line.Trim( false );
        line.Trim( true );
    }

    m_listBox.InsertItems( lines, insertAt );

    m_listBox.SetSelection( wxNOT_FOUND );

    for( size_t ii = insertAt; ii < insertAt + lines.GetCount(); ii++ )
        m_listBox.SetSelection( ii );

    wxPostEvent( &m_listBox, wxCommandEvent( EDA_EVT_LISTBOX_CHANGED ) );
}


void LISTBOX_TRICKS::listBoxCut()
{
    listBoxCopy();
    wxArrayInt deleted = listBoxDeleteSelected();

    size_t select = deleted.GetCount() > 0 ? deleted[0] : m_listBox.GetCount();

    m_listBox.SetSelection( wxNOT_FOUND );
    m_listBox.SetSelection( std::min( select, (size_t) m_listBox.GetCount() - 1 ) );
}


void LISTBOX_TRICKS::OnListBoxRDown( wxMouseEvent& aEvent )
{
    wxMenu menu;

    KIUI::AddMenuItem( &menu, ID_COPY,      _( "Copy" )      + "\tCtrl+C", KiBitmap( BITMAPS::copy ) );
    KIUI::AddMenuItem( &menu, ID_CUT,       _( "Cut" )       + "\tCtrl+X", KiBitmap( BITMAPS::cut ) );
    KIUI::AddMenuItem( &menu, ID_PASTE,     _( "Paste" )     + "\tCtrl+V", KiBitmap( BITMAPS::paste ) );
    KIUI::AddMenuItem( &menu, ID_DUPLICATE, _( "Duplicate" ) + "\tCtrl+D", KiBitmap( BITMAPS::duplicate ) );
    KIUI::AddMenuItem( &menu, ID_DELETE,    _( "Delete" )    + "\tDel",    KiBitmap( BITMAPS::trash ) );

    menu.Bind( wxEVT_COMMAND_MENU_SELECTED,
               [&]( wxCommandEvent& aCmd )
               {
                   switch( aEvent.GetId() )
                   {
                   case ID_COPY:      listBoxCopy();              break;
                   case ID_PASTE:     listBoxPaste();             break;
                   case ID_CUT:       listBoxCut();               break;
                   case ID_DELETE:    listBoxDeleteSelected();    break;
                   case ID_DUPLICATE: listBoxDuplicateSelected(); break;
                   default:           aEvent.Skip();
                   }
               } );

    m_parent.PopupMenu( &menu );
}


void LISTBOX_TRICKS::OnListBoxKeyDown( wxKeyEvent& aEvent )
{
    if( aEvent.GetKeyCode() == WXK_DELETE )
    {
        listBoxDeleteSelected();
    }
    else
    {
        if( aEvent.ControlDown() )
        {
            switch( aEvent.GetKeyCode() )
            {
            case 'C': listBoxCopy();              break;
            case 'V': listBoxPaste();             break;
            case 'X': listBoxCut();               break;
            case 'D': listBoxDuplicateSelected(); break;
            default:  aEvent.Skip();
            }
        }
        else
        {
            aEvent.Skip();
        }
    }
}


void LISTBOX_TRICKS::OnListBoxDelete( wxCommandEvent& aEvent )
{
    listBoxDeleteSelected();
}


void LISTBOX_TRICKS::OnListBoxCopy( wxCommandEvent& aEvent )
{
    listBoxCopy();
}


void LISTBOX_TRICKS::OnListBoxCut( wxCommandEvent& aEvent )
{
    listBoxCut();
}


void LISTBOX_TRICKS::OnListBoxPaste( wxCommandEvent& aEvent )
{
    listBoxPaste();
}


void LISTBOX_TRICKS::OnListBoxDuplicate( wxCommandEvent& aEvent )
{
    listBoxDuplicateSelected();
}
