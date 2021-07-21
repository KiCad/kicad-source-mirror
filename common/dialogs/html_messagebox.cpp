/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/clipbrd.h>
#include <wx/log.h>
#include <kicad_string.h>
#include <dialogs/html_messagebox.h>


HTML_MESSAGE_BOX::HTML_MESSAGE_BOX( wxWindow* aParent, const wxString& aTitle,
                                    const wxPoint& aPosition, const wxSize& aSize ) :
    DIALOG_DISPLAY_HTML_TEXT_BASE( aParent, wxID_ANY, aTitle, aPosition, aSize )
{
    m_htmlWindow->SetLayoutDirection( wxLayout_LeftToRight );
    ListClear();

    // Gives a default logical size (the actual size depends on the display definition)
    if( aSize != wxDefaultSize )
        setSizeInDU( aSize.x, aSize.y );

    Center();

    m_sdbSizer1OK->SetDefault();
}


HTML_MESSAGE_BOX::~HTML_MESSAGE_BOX()
{
    // Prevent wxWidgets bug which fails to release when closing the window on an <esc>.
    if( m_htmlWindow->HasCapture() )
        m_htmlWindow->ReleaseMouse();
}


void HTML_MESSAGE_BOX::ListClear()
{
    m_htmlWindow->SetPage( wxEmptyString );
}


void HTML_MESSAGE_BOX::ListSet( const wxString& aList )
{
    wxArrayString strings_list;
    wxStringSplit( aList, strings_list, wxChar( '\n' ) );

    wxString msg = wxT( "<ul>" );

    for ( unsigned ii = 0; ii < strings_list.GetCount(); ii++ )
    {
        msg += wxT( "<li>" );
        msg += strings_list.Item( ii ) + wxT( "</li>" );
    }

    msg += wxT( "</ul>" );

    m_htmlWindow->AppendToPage( msg );
}


void HTML_MESSAGE_BOX::ListSet( const wxArrayString& aList )
{
    wxString msg = wxT( "<ul>" );

    for( unsigned ii = 0; ii < aList.GetCount(); ii++ )
    {
        msg += wxT( "<li>" );
        msg += aList.Item( ii ) + wxT( "</li>" );
    }

    msg += wxT( "</ul>" );

    m_htmlWindow->AppendToPage( msg );
}


void HTML_MESSAGE_BOX::MessageSet( const wxString& message )
{
    wxString message_value = wxString::Format(
                wxT( "<b>%s</b><br>" ), message );

    m_htmlWindow->AppendToPage( message_value );
}


void HTML_MESSAGE_BOX::AddHTML_Text( const wxString& message )
{
    m_htmlWindow->AppendToPage( message );
}


void HTML_MESSAGE_BOX::ShowModeless()
{
    m_sdbSizer1->Show( false );
    Layout();

    Show( true );
}


void HTML_MESSAGE_BOX::OnCharHook( wxKeyEvent& aEvent )
{
    // shift-return (Mac default) or Ctrl-Return (GTK) for OK
    if( aEvent.GetKeyCode() == WXK_ESCAPE )
    {
        wxPostEvent( this, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK ) );
        return;
    }
    else if( aEvent.GetModifiers() == wxMOD_CONTROL && aEvent.GetKeyCode() == 'A' )
    {
        m_htmlWindow->SelectAll();
        return;
    }
    else if( aEvent.GetModifiers() == wxMOD_CONTROL && aEvent.GetKeyCode() == 'C' )
    {
        wxLogNull doNotLog; // disable logging of failed clipboard actions

        if( wxTheClipboard->Open() )
        {
            wxTheClipboard->SetData( new wxTextDataObject( m_htmlWindow->SelectionToText() ) );
            wxTheClipboard->Flush(); // Allow data to be available after closing KiCad
            wxTheClipboard->Close();
        }

        return;
    }

    aEvent.Skip();
}
