/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#include <footprint_filter.h>
#include <tool/tool_manager.h>
#include <trace_helpers.h>
#include <wx/log.h>
#include <wx/wupdlock.h>

#include <cvpcb_id.h>
#include <cvpcb_mainframe.h>
#include <tools/cvpcb_actions.h>

FOOTPRINTS_LISTBOX::FOOTPRINTS_LISTBOX( CVPCB_MAINFRAME* parent, wxWindowID id ) :
        ITEMS_LISTBOX_BASE( parent, id, wxDefaultPosition, wxDefaultSize, wxLC_SINGLE_SEL | wxNO_BORDER )
{
}


int FOOTPRINTS_LISTBOX::GetCount()
{
    return (int) m_footprintList.Count();
}


void FOOTPRINTS_LISTBOX::SetString( unsigned linecount, const wxString& text )
{
    unsigned count = m_footprintList.Count();

    if( count > 0 )
    {
        if( linecount >= count )
            linecount = count - 1;

        m_footprintList[linecount] = text;
    }

    UpdateWidth( linecount );
}


wxString FOOTPRINTS_LISTBOX::GetSelectedFootprint()
{
    wxString footprintName;
    int      ii = GetFirstSelected();

    if( ii >= 0 && ii < (int)m_footprintList.size() )
    {
        wxString msg = m_footprintList[ii];
        msg.Trim( true );
        msg.Trim( false );
        footprintName = msg.AfterFirst( wxChar( ' ' ) );
    }

    return footprintName;
}


wxString FOOTPRINTS_LISTBOX::OnGetItemText( long item, long column ) const
{
    if( item < 0 || item >= (long)m_footprintList.GetCount() )
        return wxEmptyString;

    return m_footprintList.Item( item );
}


void FOOTPRINTS_LISTBOX::SetSelection( int aIndex, bool aState )
{
    if( aIndex >= GetCount() )
        aIndex = GetCount() - 1;

    if( aIndex >= 0 && GetCount() > 0)
    {
        Select( aIndex, aState );
        EnsureVisible( aIndex );
        Refresh();
    }
}


void FOOTPRINTS_LISTBOX::SetSelectedFootprint( const LIB_ID& aFPID )
{
    wxString id = aFPID.Format().wx_str();

    for( int i = 0; i < GetCount(); ++i )
    {
        wxString candidate = m_footprintList.Item( i ).substr( 4 );

        if( candidate.CmpNoCase( id ) == 0 )
        {
            SetSelection( i, true );
            return;
        }
    }
}


void FOOTPRINTS_LISTBOX::SetFootprints( FOOTPRINT_LIST& aList, const wxString& aLibName,
                                        COMPONENT* aComponent,
                                        const wxString &aFootPrintFilterPattern,
                                        int aFilterType )
{
    wxArrayString   newList;
    wxString        msg;
    wxString        oldSelection;

    FOOTPRINT_FILTER filter( aList );

    if( aFilterType & FILTERING_BY_COMPONENT_FP_FILTERS && aComponent )
        filter.FilterByFootprintFilters( aComponent->GetFootprintFilters() );

    if( aFilterType & FILTERING_BY_PIN_COUNT && aComponent )
    {
        int pc = aComponent->GetPinCount();
        wxLogTrace( "CVPCB_PINCOUNT",
                    wxT( "FOOTPRINTS_LISTBOX::SetFootprints: ref='%s' pinCount filter=%d" ),
                    aComponent->GetReference(), pc );
        filter.FilterByPinCount( pc );
    }

    if( aFilterType & FILTERING_BY_LIBRARY )
        filter.FilterByLibrary( aLibName );

    if( !aFootPrintFilterPattern.IsEmpty() )
        filter.FilterByTextPattern( aFootPrintFilterPattern );

    if( GetSelection() >= 0 && GetSelection() < (int)m_footprintList.GetCount() )
        oldSelection = m_footprintList[ GetSelection() ];

    for( const FOOTPRINT_INFO& i : filter )
    {
        msg.Printf( wxS( "%3d %s:%s" ),
                    int( newList.GetCount() + 1 ),
                    i.GetLibNickname(),
                    i.GetFootprintName() );
        newList.Add( msg );
    }

    if( newList == m_footprintList )
        return;

    m_footprintList = newList;

    int selection = m_footprintList.Index( oldSelection );

    if( selection == wxNOT_FOUND )
        selection = 0;

    DeselectAll();
    wxSafeYield();
    wxWindowUpdateLocker freeze( this );
    DeleteAllItems();

    if( m_footprintList.GetCount() )
    {
        SetItemCount( m_footprintList.GetCount() );
        SetSelection( selection, true );
        RefreshItems( 0L, m_footprintList.GetCount() - 1 );
        UpdateWidth();
    }
}


BEGIN_EVENT_TABLE( FOOTPRINTS_LISTBOX, ITEMS_LISTBOX_BASE )
    EVT_CHAR( FOOTPRINTS_LISTBOX::OnChar )
    EVT_LIST_ITEM_SELECTED( ID_CVPCB_FOOTPRINT_LIST, FOOTPRINTS_LISTBOX::OnLeftClick )
    EVT_LIST_ITEM_ACTIVATED( ID_CVPCB_FOOTPRINT_LIST, FOOTPRINTS_LISTBOX::OnLeftDClick )
END_EVENT_TABLE()


void FOOTPRINTS_LISTBOX::OnLeftClick( wxListEvent& event )
{
    if( m_isClosing )
        return;

    GetParent()->RefreshFootprintViewer();
}


void FOOTPRINTS_LISTBOX::OnLeftDClick( wxListEvent& event )
{
    if( m_isClosing )
        return;

    GetParent()->GetToolManager()->RunAction( CVPCB_ACTIONS::associate );
}


void FOOTPRINTS_LISTBOX::OnChar( wxKeyEvent& event )
{
    if( m_isClosing )
        return;

    wxLogTrace( kicadTraceKeyEvent, wxS( "FOOTPRINTS_LISTBOX::OnChar %s" ), dump( event ) );

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
