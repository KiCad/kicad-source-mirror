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

/**
 * @file symbols_listbox.cpp
 */

#include <trace_helpers.h>

#include <kiplatform/ui.h>
#include <cvpcb_mainframe.h>
#include <listboxes.h>
#include <cvpcb_id.h>
#include <wx/log.h>


SYMBOLS_LISTBOX::SYMBOLS_LISTBOX( CVPCB_MAINFRAME* parent, wxWindowID id ) :
        ITEMS_LISTBOX_BASE( parent, id ),
        m_warningAttr( std::make_unique<wxListItemAttr>() )
{
    m_warningAttr->SetBackgroundColour( KIPLATFORM::UI::IsDarkTheme() ? wxColour( 112, 96, 32 )
                                                                      : wxColour( 255, 248, 212 ) );
}


BEGIN_EVENT_TABLE( SYMBOLS_LISTBOX, ITEMS_LISTBOX_BASE )
    EVT_CHAR( SYMBOLS_LISTBOX::OnChar )
    EVT_LIST_ITEM_SELECTED( ID_CVPCB_COMPONENT_LIST, SYMBOLS_LISTBOX::OnSelectComponent )
END_EVENT_TABLE()


void SYMBOLS_LISTBOX::Clear()
{
    m_SymbolList.Clear();
    SetItemCount( 0 );
}


int SYMBOLS_LISTBOX::GetCount()
{
    return m_SymbolList.Count();
}


void SYMBOLS_LISTBOX::SetString( unsigned linecount, const wxString& text )
{
    if( linecount >= m_SymbolList.Count() )
        linecount = m_SymbolList.Count() - 1;

    if( m_SymbolList.Count() > 0 )
    {
        m_SymbolList[linecount] = text;
        UpdateWidth( linecount );
    }
}


void SYMBOLS_LISTBOX::AppendLine( const wxString& text )
{
    m_SymbolList.Add( text );
    int lines = m_SymbolList.Count();
    SetItemCount( lines );
    UpdateWidth( lines - 1 );
}


void SYMBOLS_LISTBOX::AppendWarning( int index )
{
    if( !std::count( m_symbolWarning.begin(), m_symbolWarning.end(), index ) )
    {
        m_symbolWarning.emplace_back( index );
    }
}


void SYMBOLS_LISTBOX::RemoveWarning( int index )
{
    if( auto const found{ std::find( m_symbolWarning.begin(), m_symbolWarning.end(), index ) };
        found != m_symbolWarning.end() )
    {
        m_symbolWarning.erase( found );
    }
}


wxString SYMBOLS_LISTBOX::OnGetItemText( long item, long column ) const
{
    return m_SymbolList.Item( item );
}


wxListItemAttr* SYMBOLS_LISTBOX::OnGetItemAttr( long item ) const
{
    if( std::count( m_symbolWarning.begin(), m_symbolWarning.end(), item ) )
        return m_warningAttr.get();

    return nullptr;
}


void SYMBOLS_LISTBOX::SetSelection( int index, bool State )
{
    if( index >= GetCount() )
        index = GetCount() - 1;

    if( (index >= 0) && (GetCount() > 0) )
    {
        Select( index, State );
        EnsureVisible( index );

#ifdef __WXMAC__
        Update();
#endif
    }
}


void SYMBOLS_LISTBOX::OnChar( wxKeyEvent& event )
{
    if( m_isClosing )
        return;

    wxLogTrace( kicadTraceKeyEvent, wxS( "SYMBOLS_LISTBOX::OnChar %s" ), dump( event ) );

    int key = event.GetKeyCode();

    switch( key )
    {
    case WXK_HOME:
    case WXK_END:
    case WXK_UP:
    case WXK_DOWN:
    case WXK_PAGEUP:
    case WXK_PAGEDOWN:
        event.Skip();
        return;


    default:
        break;
    }

    // Search for an item name starting by the key code:
    key = toupper( key );

    for( unsigned ii = 0; ii < m_SymbolList.GetCount(); ii++ )
    {
        wxString text = m_SymbolList.Item( ii );

        // Search for the start char of the footprint name.  Skip the line number.
        text.Trim( false );      // Remove leading spaces in line
        unsigned jj = 0;

        for( ; jj < text.Len(); jj++ )
        {   // skip line number
            if( text[jj] == ' ' )
                break;
        }

        for( ; jj < text.Len(); jj++ )
        {   // skip blanks
            if( text[jj] != ' ' )
                break;
        }

        int start_char = toupper( text[jj] );

        if( key == start_char )
        {
            SetSelection( (int) ii, true );   // Ensure visible
            break;
        }
    }

    event.Skip();
}


void SYMBOLS_LISTBOX::OnSelectComponent( wxListEvent& event )
{
    if( m_isClosing )
        return;

    SetFocus();
    GetParent()->OnSelectComponent( event );
}
