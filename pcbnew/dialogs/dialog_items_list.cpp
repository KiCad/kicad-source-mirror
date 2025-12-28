/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
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

#include "dialog_items_list.h"
#include <wx/sizer.h>
#include <wx/button.h>

DIALOG_ITEMS_LIST::DIALOG_ITEMS_LIST( wxWindow* aParent, const wxString& aTitle,
                                      const wxString& aMessage, const wxString& aDetailsLabel ) :
    DIALOG_SHIM( aParent, wxID_ANY, aTitle, wxDefaultPosition, wxDefaultSize,
                 wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER )
{
    wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );

    m_message = new wxStaticText( this, wxID_ANY, aMessage );
    mainSizer->Add( m_message, 0, wxALL | wxEXPAND, 10 );

    m_collapsiblePane = new wxCollapsiblePane( this, wxID_ANY, aDetailsLabel );
    mainSizer->Add( m_collapsiblePane, 1, wxALL | wxEXPAND, 10 );

    wxWindow* pane = m_collapsiblePane->GetPane();
    wxBoxSizer* paneSizer = new wxBoxSizer( wxVERTICAL );

    m_listCtrl = new wxListCtrl( pane, wxID_ANY, wxDefaultPosition, wxSize( 400, 200 ),
                                 wxLC_REPORT | wxLC_NO_HEADER | wxLC_SINGLE_SEL );
    m_listCtrl->InsertColumn( 0, wxEmptyString );

    paneSizer->Add( m_listCtrl, 1, wxEXPAND | wxALL, 5 );
    pane->SetSizer( paneSizer );

    m_collapsiblePane->Bind( wxEVT_COLLAPSIBLEPANE_CHANGED, &DIALOG_ITEMS_LIST::onCollapse, this );
    m_listCtrl->Bind( wxEVT_LIST_ITEM_SELECTED, &DIALOG_ITEMS_LIST::onSelectionChanged, this );

    wxSizer* buttonsSizer = CreateSeparatedButtonSizer( wxOK | wxCANCEL );
    mainSizer->Add( buttonsSizer, 0, wxEXPAND | wxALL, 10 );

    SetSizer( mainSizer );
    Layout();
    GetSizer()->Fit( this );
    Center();
}

DIALOG_ITEMS_LIST::~DIALOG_ITEMS_LIST()
{
}

void DIALOG_ITEMS_LIST::AddItems( const std::vector<wxString>& aItems )
{
    for( const wxString& item : aItems )
        m_listCtrl->InsertItem( m_listCtrl->GetItemCount(), item );

    m_listCtrl->SetColumnWidth( 0, wxLIST_AUTOSIZE_USEHEADER );
}

void DIALOG_ITEMS_LIST::SetSelectionCallback( std::function<void(int)> aCallback )
{
    m_callback = aCallback;
}

void DIALOG_ITEMS_LIST::onSelectionChanged( wxListEvent& aEvent )
{
    if( m_callback )
        m_callback( aEvent.GetIndex() );
}

void DIALOG_ITEMS_LIST::onCollapse( wxCollapsiblePaneEvent& aEvent )
{
    Layout();
    GetSizer()->Fit( this );
}
