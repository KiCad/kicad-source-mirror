/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file class_library_listbox.cpp
 * class to display used library and selecting it
 */

#include <fctsys.h>
#include <wxstruct.h>
#include <macros.h>

#include <cvpcb.h>
#include <cvpcb_mainframe.h>
#include <cvstruct.h>


/***************************************/
/* ListBox handling the library list */
/***************************************/

LIBRARY_LISTBOX::LIBRARY_LISTBOX( CVPCB_MAINFRAME* parent,
                                        wxWindowID id, const wxPoint& loc,
                                        const wxSize& size,
                                        int nbitems, wxString choice[] ) :
    ITEMS_LISTBOX_BASE( parent, id, loc, size )
{
    //ListLibraries();
}


LIBRARY_LISTBOX::~LIBRARY_LISTBOX()
{
}


/*
 * Return number of items
 */
int LIBRARY_LISTBOX::GetCount()
{
    return m_LibraryList.Count();
}


/*
 * Change an item text
 */
void LIBRARY_LISTBOX::SetString( unsigned linecount, const wxString& text )
{
    if( linecount >= m_LibraryList.Count() )
        linecount = m_LibraryList.Count() - 1;
    if( linecount >= 0 )
        m_LibraryList[linecount] = text;
}


wxString LIBRARY_LISTBOX::GetSelectedLibrary()
{
    wxString libraryName;
    int      ii = GetFirstSelected();

    if( ii >= 0 )
    {
        libraryName = m_LibraryList[ii];
    }

    return libraryName;
}


void LIBRARY_LISTBOX::AppendLine( const wxString& text )
{
    m_LibraryList.Add( text );
    SetItemCount( m_LibraryList.Count() );
}


/*
 * Overlaid function: MUST be provided in wxLC_VIRTUAL mode
 * because real data is not handled by ITEMS_LISTBOX_BASE
 */
wxString LIBRARY_LISTBOX::OnGetItemText( long item, long column ) const
{
    return m_LibraryList.Item( item );
}

/*
 * Enable or disable an item
 */
void LIBRARY_LISTBOX::SetSelection( unsigned index, bool State )
{
    if( (int) index >= GetCount() )
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


void LIBRARY_LISTBOX::SetLibraryList( wxArrayString list )
{
    wxString msg;
    int      oldSelection = GetSelection();

    m_LibraryList.Clear();

    for( unsigned ii = 0; ii < list.GetCount(); ii++ )
    {
        msg = list.Item(ii);
        m_LibraryList.Add( msg );
    }

    SetItemCount(list.GetCount());

    if(  GetCount() == 0 || oldSelection < 0 || oldSelection >= GetCount() )
        SetSelection( 0, true );
    Refresh();
}


/**************************************/
/* Event table for the library list */
/**************************************/

BEGIN_EVENT_TABLE( LIBRARY_LISTBOX, ITEMS_LISTBOX_BASE )
    EVT_SIZE( ITEMS_LISTBOX_BASE::OnSize )
    EVT_CHAR( LIBRARY_LISTBOX::OnChar )
END_EVENT_TABLE()


/**
 * Function OnChar
 * called on a key pressed
 * Call default handler for some special keys,
 * and for "ascii" keys, select the first footprint
 * that the name starts by the letter.
 * This is the defaut behaviour of a listbox, but because we use
 * virtual lists, the listbox does not know anything to what is displayed,
 * we must handle this behaviour here.
 * Furthermore the footprint name is not at the beginning of
 * displayed lines (the first word is the line number)
 */
void LIBRARY_LISTBOX::OnChar( wxKeyEvent& event )
{
    int key = event.GetKeyCode();
    switch( key )
    {
        case WXK_RIGHT:
        case WXK_NUMPAD_RIGHT:
            GetParent()->m_ListCmp->SetFocus();
            return;

        case WXK_HOME:
        case WXK_END:
        case WXK_UP:
        case WXK_DOWN:
        case WXK_PAGEUP:
        case WXK_PAGEDOWN:
        case WXK_LEFT:
        case WXK_NUMPAD_LEFT:
            event.Skip();
            return;

        default:
            break;
    }
    // Search for an item name starting by the key code:
    key = toupper(key);
    for( unsigned ii = 0; ii < m_LibraryList.GetCount(); ii++ )
    {
        wxString text = m_LibraryList.Item(ii);
        /* search for the start char of the footprint name.
         * we must skip the line number
        */
        text.Trim(false);      // Remove leading spaces in line
        unsigned jj = 0;
        for( ; jj < text.Len(); jj++ )
        {
            // skip line number
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
            Focus( ii );
            SetSelection( ii, true );   // Ensure visible
            break;
        }
    }
}
