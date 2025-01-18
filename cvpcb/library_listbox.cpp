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

#include <trace_helpers.h>

#include <cvpcb_mainframe.h>
#include <cvpcb_id.h>
#include <wx/log.h>
#include "lib_tree_model_adapter.h"


/***************************************/
/* ListBox handling the library list */
/***************************************/

LIBRARY_LISTBOX::LIBRARY_LISTBOX( CVPCB_MAINFRAME* parent, wxWindowID id ) :
        ITEMS_LISTBOX_BASE( parent, id, wxDefaultPosition, wxDefaultSize, wxLC_SINGLE_SEL )
{
}


int LIBRARY_LISTBOX::GetCount()
{
    return (int) m_libraryList.Count();
}


void LIBRARY_LISTBOX::SetString( unsigned linecount, const wxString& text )
{
    unsigned count = m_libraryList.Count();

    if( count > 0 )
    {
        if( linecount >= count )
            linecount = count - 1;

        m_libraryList[linecount] = text;
        UpdateWidth( linecount );
    }
}


wxString LIBRARY_LISTBOX::GetSelectedLibrary()
{
    wxString libName;
    int      ii = GetFirstSelected();

    if( ii >= 0 )
    {
        libName = m_libraryList[ii];
        libName.Trim( false );

        if( libName.StartsWith( LIB_TREE_MODEL_ADAPTER::GetPinningSymbol() ) )
            libName = libName.substr( LIB_TREE_MODEL_ADAPTER::GetPinningSymbol().length() );
    }

    return libName;
}


void LIBRARY_LISTBOX::AppendLine( const wxString& text )
{
    m_libraryList.Add( wxT( " " ) + text );
    int lines = m_libraryList.Count();
    SetItemCount( lines );
}


wxString LIBRARY_LISTBOX::OnGetItemText( long item, long column ) const
{
    return m_libraryList.Item( item );
}


void LIBRARY_LISTBOX::SetSelection( int index, bool State )
{
    if( index >= GetCount() )
        index = GetCount() - 1;

    if( (index >= 0)  && (GetCount() > 0) )
    {
#ifndef __WXMAC__
        Select( index, State );
#endif
        EnsureVisible( index );
#ifdef __WXMAC__
        Refresh();
#endif
    }
}


void LIBRARY_LISTBOX::Finish()
{
    if( m_libraryList.Count() )
    {
        RefreshItems( 0L, m_libraryList.Count() - 1 );
        UpdateWidth();
    }
}


void LIBRARY_LISTBOX::ClearList()
{
    m_libraryList.clear();
}


BEGIN_EVENT_TABLE( LIBRARY_LISTBOX, ITEMS_LISTBOX_BASE )
    EVT_CHAR( LIBRARY_LISTBOX::OnChar )
    EVT_LIST_ITEM_SELECTED( ID_CVPCB_LIBRARY_LIST, LIBRARY_LISTBOX::OnSelectLibrary )
END_EVENT_TABLE()


void LIBRARY_LISTBOX::OnChar( wxKeyEvent& event )
{
    wxLogTrace( kicadTraceKeyEvent, wxS( "LIBRARY_LISTBOX::OnChar %s" ), dump( event ) );

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
    key = toupper(key);

    for( unsigned ii = 0; ii < m_libraryList.GetCount(); ii++ )
    {
        wxString text = m_libraryList.Item( ii );

        // Search for the start char of the footprint name.  Skip the line number.
        text.Trim( false );      // Remove leading spaces in line
        unsigned jj = 0;

        for( ; jj < text.Len(); jj++ )
        {
            // skip line number
            if( text[jj] == ' ' )
                break;
        }

        for( ; jj < text.Len(); jj++ )
        {
            // skip blanks
            if( text[jj] != ' ' )
                break;
        }

        int start_char = toupper( text[jj] );

        if( key == start_char )
        {
            SetSelection( ii, true );   // Ensure visible
            break;
        }
    }

    event.Skip();
}


void LIBRARY_LISTBOX::OnSelectLibrary( wxListEvent& event )
{
    // Apply the filter
    GetParent()->SetFootprintFilter( FOOTPRINTS_LISTBOX::FILTERING_BY_LIBRARY,
                                     CVPCB_MAINFRAME::FILTER_ENABLE );

    SetFocus();
    GetParent()->OnSelectComponent( event );
}
