/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <widgets/search_pane_tab.h>
#include <widgets/search_pane.h>
#include <kiway.h>

SEARCH_PANE_LISTVIEW::SEARCH_PANE_LISTVIEW( SEARCH_HANDLER* handler, wxWindow* parent,
                                            wxWindowID winid, const wxPoint& pos,
                                            const wxSize& size ) :
        wxListView( parent, winid, pos, size, wxLC_REPORT | wxLC_VIRTUAL ),
        m_handler( handler )
{
    SetItemCount( 0 );

    RefreshColumnNames();

    Bind( wxEVT_LIST_ITEM_SELECTED, &SEARCH_PANE_LISTVIEW::OnItemSelected, this );
}


void SEARCH_PANE_LISTVIEW::OnItemSelected( wxListEvent& aEvent )
{
    long idx = aEvent.GetIndex();

    m_handler->SelectItem( idx );
}


void SEARCH_PANE_LISTVIEW::RefreshColumnNames()
{
    Freeze();
    DeleteAllColumns();

    std::vector<wxString> columns = m_handler->GetColumnNames();
    for( wxString& columnName : columns )
    {
        AppendColumn( _( columnName ) );
    }

    Thaw();
}


wxString SEARCH_PANE_LISTVIEW::OnGetItemText( long item, long column ) const
{
    return m_handler->GetResultCell( item, column );
}


SEARCH_PANE_TAB::SEARCH_PANE_TAB( SEARCH_HANDLER* handler, wxWindow* parent, wxWindowID aId,
                                  const wxPoint& aLocation, const wxSize& aSize ) :
        wxPanel( parent, aId, aLocation, aSize ),
        m_handler( handler )
{
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

    m_listView = new SEARCH_PANE_LISTVIEW( handler, this );
    sizer->Add( m_listView, 5, wxRIGHT | wxBOTTOM | wxEXPAND, 1 );

    SetSizer( sizer );

    Layout();
    sizer->Fit( this );
}


void SEARCH_PANE_TAB::Search( wxString& query )
{
    int results = m_handler->Search( query );
    m_listView->SetItemCount( results );
    m_listView->Refresh();
}


void SEARCH_PANE_TAB::Clear()
{
    m_listView->SetItemCount( 0 );
    m_listView->Refresh();
}


void SEARCH_PANE_TAB::RefreshColumnNames()
{
    m_listView->RefreshColumnNames();
}