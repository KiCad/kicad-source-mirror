/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
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
 * @file class_footprints_listbox.cpp
 * class to display the list of available footprints
 */

#include <fctsys.h>
#include <wxstruct.h>
#include <macros.h>
#include <pgm_base.h>
#include <wildcards_and_files_ext.h>

#include <cvpcb.h>
#include <cvpcb_mainframe.h>
#include <cvstruct.h>
#include <cvpcb_id.h>


FOOTPRINTS_LISTBOX::FOOTPRINTS_LISTBOX( CVPCB_MAINFRAME* parent,
                                        wxWindowID id, const wxPoint& loc,
                                        const wxSize& size ) :
    ITEMS_LISTBOX_BASE( parent, id, loc, size )
{
}


FOOTPRINTS_LISTBOX::~FOOTPRINTS_LISTBOX()
{
}


int FOOTPRINTS_LISTBOX::GetCount()
{
    return m_footprintList.Count();
}


void FOOTPRINTS_LISTBOX::SetString( unsigned linecount, const wxString& text )
{
    if( linecount >= m_footprintList.Count() )
        linecount = m_footprintList.Count() - 1;

    if( linecount >= 0 )
        m_footprintList[linecount] = text;
}


wxString FOOTPRINTS_LISTBOX::GetSelectedFootprint()
{
    wxString footprintName;
    int      ii = GetFirstSelected();

    if( ii >= 0 )
    {
        wxString msg = m_footprintList[ii];
        msg.Trim( true );
        msg.Trim( false );
        footprintName = msg.AfterFirst( wxChar( ' ' ) );
    }

    return footprintName;
}


void FOOTPRINTS_LISTBOX::AppendLine( const wxString& text )
{
    m_footprintList.Add( text );
    SetItemCount( m_footprintList.Count() );
}


wxString FOOTPRINTS_LISTBOX::OnGetItemText( long item, long column ) const
{
    if( item < 0 || item >= (long)m_footprintList.GetCount() )
        return wxEmptyString;

    return m_footprintList.Item( item );
}


void FOOTPRINTS_LISTBOX::SetSelection( int index, bool State )
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


void FOOTPRINTS_LISTBOX::SetFootprints( FOOTPRINT_LIST& aList, const wxString& aLibName,
                                        COMPONENT* aComponent, int aFilterType )
{
    wxArrayString   newList;
    wxString        msg;
    wxString        oldSelection;

    if( GetSelection() >= 0 && GetSelection() < (int)m_footprintList.GetCount() )
        oldSelection = m_footprintList[ GetSelection() ];

    for( unsigned ii = 0; ii < aList.GetCount(); ii++ )
    {
        if( aFilterType == UNFILTERED )
        {
            msg.Printf( wxT( "%3zu %s:%s" ), newList.GetCount() + 1,
                        GetChars( aList.GetItem( ii ).GetNickname() ),
                        GetChars( aList.GetItem( ii ).GetFootprintName() ) );
            newList.Add( msg );
            continue;
        }

        if( (aFilterType & BY_LIBRARY) && !aLibName.IsEmpty()
          && !aList.GetItem( ii ).InLibrary( aLibName ) )
            continue;

        if( (aFilterType & BY_COMPONENT) && aComponent
          && !aComponent->MatchesFootprintFilters( aList.GetItem( ii ).GetFootprintName() ) )
            continue;

        if( (aFilterType & BY_PIN_COUNT) && aComponent
          && aComponent->GetNetCount() != aList.GetItem( ii ).GetPadCount() )
            continue;

        msg.Printf( wxT( "%3zu %s:%s" ), newList.GetCount() + 1,
                    GetChars( aList.GetItem( ii ).GetNickname() ),
                    GetChars( aList.GetItem( ii ).GetFootprintName() ) );
        newList.Add( msg );
    }

    if( newList == m_footprintList )
        return;

    m_footprintList = newList;

    int selection = m_footprintList.Index( oldSelection );

    if( selection == wxNOT_FOUND )
        selection = 0;

    DeleteAllItems();

    if( m_footprintList.GetCount() )
    {
        SetItemCount( m_footprintList.GetCount() );
        SetSelection( selection, true );
        RefreshItems( 0L, m_footprintList.GetCount()-1 );

#if defined (__WXGTK__ ) //&& wxMINOR_VERSION == 8
        // @bug On GTK and wxWidgets 2.8.x, this will assert in debug builds because the

        //      column parameter is -1.  This was the only way to prevent GTK3 from
        //      ellipsizing long strings down to a few characters.  It still doesn't set
        //      the scroll bars correctly (too short) but it's better than any of the
        //      other alternatives.  If someone knows how to fix this, please do.
        SetColumnWidth( -1, wxLIST_AUTOSIZE );
#else
        SetColumnWidth( 0, wxLIST_AUTOSIZE );
#endif
    }
}


BEGIN_EVENT_TABLE( FOOTPRINTS_LISTBOX, ITEMS_LISTBOX_BASE )
    EVT_SIZE( ITEMS_LISTBOX_BASE::OnSize )
    EVT_CHAR( FOOTPRINTS_LISTBOX::OnChar )
    EVT_LIST_ITEM_SELECTED( ID_CVPCB_FOOTPRINT_LIST, FOOTPRINTS_LISTBOX::OnLeftClick )
    EVT_LIST_ITEM_ACTIVATED( ID_CVPCB_FOOTPRINT_LIST, FOOTPRINTS_LISTBOX::OnLeftDClick )
END_EVENT_TABLE()


void FOOTPRINTS_LISTBOX::OnLeftClick( wxListEvent& event )
{
    if( m_footprintList.IsEmpty() )
        return;

    // If the footprint view window is displayed, update the footprint.
    if( GetParent()->GetFpViewerFrame() )
        GetParent()->CreateScreenCmp();

    GetParent()->DisplayStatus();
}


void FOOTPRINTS_LISTBOX::OnLeftDClick( wxListEvent& event )
{
    wxString footprintName = GetSelectedFootprint();

    GetParent()->SetNewPkg( footprintName );
}


void FOOTPRINTS_LISTBOX::OnChar( wxKeyEvent& event )
{
    int key = event.GetKeyCode();

    switch( key )
    {
    case WXK_TAB:
    case WXK_RIGHT:
    case WXK_NUMPAD_RIGHT:
        GetParent()->ChangeFocus( true );
        return;

    case WXK_LEFT:
    case WXK_NUMPAD_LEFT:
        GetParent()->ChangeFocus( false );
        return;

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

    for( unsigned ii = 0; ii < m_footprintList.GetCount(); ii++ )
    {
        wxString text = m_footprintList.Item( ii );

        // Search for the start char of the footprint name. Skip the line number.
        text.Trim( false );      // Remove leading spaces in line
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
            SetSelection( ii, true );   // Ensure visible
            break;
        }
    }
}
