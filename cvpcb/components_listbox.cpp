/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file class_components_listbox.cpp
 */

#include <trace_helpers.h>

#include <cvpcb_mainframe.h>
#include <listboxes.h>
#include <cvpcb_id.h>


COMPONENTS_LISTBOX::COMPONENTS_LISTBOX( CVPCB_MAINFRAME* parent, wxWindowID id,
                                        const wxPoint& loc, const wxSize& size ) :
    ITEMS_LISTBOX_BASE( parent, id, loc, size, 0 )
{
}


COMPONENTS_LISTBOX::~COMPONENTS_LISTBOX()
{
}


BEGIN_EVENT_TABLE( COMPONENTS_LISTBOX, ITEMS_LISTBOX_BASE )
    EVT_CHAR( COMPONENTS_LISTBOX::OnChar )
    EVT_LIST_ITEM_SELECTED( ID_CVPCB_COMPONENT_LIST, COMPONENTS_LISTBOX::OnSelectComponent )
END_EVENT_TABLE()


void COMPONENTS_LISTBOX::Clear()
{
    m_ComponentList.Clear();
    SetItemCount( 0 );
}


int COMPONENTS_LISTBOX::GetCount()
{
    return m_ComponentList.Count();
}


void COMPONENTS_LISTBOX::SetString( unsigned linecount, const wxString& text )
{
    if( linecount >= m_ComponentList.Count() )
        linecount = m_ComponentList.Count() - 1;

    if( m_ComponentList.Count() > 0 )
    {
        m_ComponentList[linecount] = text;
        UpdateWidth( linecount );
    }
}


void COMPONENTS_LISTBOX::AppendLine( const wxString& text )
{
    m_ComponentList.Add( text );
    int lines = m_ComponentList.Count();
    SetItemCount( lines );
    UpdateWidth( lines - 1 );
}


wxString COMPONENTS_LISTBOX::OnGetItemText( long item, long column ) const
{
    return m_ComponentList.Item( item );
}


void COMPONENTS_LISTBOX::SetSelection( int index, bool State )
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


void COMPONENTS_LISTBOX::OnChar( wxKeyEvent& event )
{
    wxLogTrace( kicadTraceKeyEvent, "COMPONENTS_LISTBOX::OnChar %s", dump( event ) );

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

    for( unsigned ii = 0; ii < m_ComponentList.GetCount(); ii++ )
    {
        wxString text = m_ComponentList.Item( ii );

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


void COMPONENTS_LISTBOX::OnSelectComponent( wxListEvent& event )
{
    SetFocus();
    GetParent()->OnSelectComponent( event );
}
