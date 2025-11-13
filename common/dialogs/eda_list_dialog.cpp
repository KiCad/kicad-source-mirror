/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#include <eda_list_dialog.h>
#include <eda_draw_frame.h>
#include <string_utils.h>
#include <macros.h>
#include <wx/checkbox.h>
#include "lib_tree_model_adapter.h"

// wxWidgets spends *far* too long calculating column widths (most of it, believe it or
// not, in repeatedly creating/destroying a wxDC to do the measurement in).
// Use default column widths instead.
// Note, these are "standard DPI" widths
static int DEFAULT_SINGLE_COL_WIDTH = 260;
static int DEFAULT_COL_WIDTHS[] = { 200, 300 };


EDA_LIST_DIALOG::EDA_LIST_DIALOG( wxWindow* aParent, const wxString& aTitle, const wxArrayString& aItemHeaders,
                                  const std::vector<wxArrayString>& aItemList, const wxString& aPreselectText,
                                  bool aSortList ) :
        EDA_LIST_DIALOG_BASE( aParent, wxID_ANY, aTitle ),
        m_sortList( aSortList )
{
    // correct for wxfb not supporting FromDIP
    m_listBox->SetMinSize( FromDIP( m_listBox->GetMinSize() ) );
    m_filterBox->SetHint( _( "Filter" ) );

    initDialog( aItemHeaders, aItemList, aPreselectText );

    if( aItemList.size() > 16 )
        m_listBox->SetMinSize( wxSize( m_listBox->GetMinWidth(), KiROUND( m_listBox->GetMinHeight() * 1.66 ) ) );

    // DIALOG_SHIM needs a unique hash_key because classname is not sufficient
    // because so many dialogs share this same class, with different numbers of
    // columns, different column names, and column widths.
    m_hash_key = TO_UTF8( aTitle );

    SetupStandardButtons();

    Layout();
    GetSizer()->Fit( this );

    Centre();
}


EDA_LIST_DIALOG::EDA_LIST_DIALOG( wxWindow* aParent, const wxString& aTitle, bool aSortList ) :
        EDA_LIST_DIALOG_BASE( aParent, wxID_ANY, aTitle ),
        m_sortList( aSortList )
{
    m_filterBox->SetHint( _( "Filter" ) );
}


void EDA_LIST_DIALOG::AddExtraCheckbox( const wxString& aLabel, bool* aValuePtr )
{
    wxCHECK2_MSG( aValuePtr, return, wxT( "Null pointer for checkbox value." ) );

    int flags = wxBOTTOM;

    if( m_ExtrasSizer->GetItemCount() > 0 )
        flags |= wxTOP;

    wxCheckBox* cb = new wxCheckBox( this, wxID_ANY, aLabel );
    cb->SetValue( *aValuePtr );
    m_ExtrasSizer->Add( cb, 0, flags, 5 );
    m_extraCheckboxMap[cb] = aValuePtr;
}


bool EDA_LIST_DIALOG::Show( bool show )
{
    bool retVal = DIALOG_SHIM::Show( show );

    if( show )
    {
        wxSizeEvent dummy;
        onSize( dummy );
    }

    return retVal;
}


void EDA_LIST_DIALOG::initDialog( const wxArrayString& aItemHeaders, const std::vector<wxArrayString>& aItemList,
                                  const wxString&  aPreselectText )
{
    if( aItemHeaders.Count() == 1 )
    {
        m_listBox->InsertColumn( 0, aItemHeaders.Item( 0 ), wxLIST_FORMAT_LEFT,
                                 FromDIP( DEFAULT_SINGLE_COL_WIDTH ) );

        m_listBox->SetMinClientSize( FromDIP( wxSize( DEFAULT_SINGLE_COL_WIDTH, 200 ) ) );
        SetMinClientSize( FromDIP( wxSize( DEFAULT_COL_WIDTHS[0], 220 ) ) );
    }
    else if( aItemHeaders.Count() == 2 )
    {
        for( unsigned i = 0; i < aItemHeaders.Count(); i++ )
        {
            m_listBox->InsertColumn( i, aItemHeaders.Item( i ), wxLIST_FORMAT_LEFT,
                                     FromDIP( DEFAULT_COL_WIDTHS[ i ] ) );
        }

        m_listBox->SetMinClientSize( FromDIP( wxSize( DEFAULT_COL_WIDTHS[0] * 3, 200 ) ) );
        SetMinClientSize( FromDIP( wxSize( DEFAULT_COL_WIDTHS[0] * 2, 220 ) ) );
    }

    m_itemsList = aItemList;
    InsertItems( m_itemsList, 0 );

    if( !aPreselectText.IsEmpty() )
    {
        // If a preselect is specified, disable loading of previously-saved state
        OptOut( m_listBox );

        long sel = m_listBox->FindItem( -1, aPreselectText );

        if( sel != wxNOT_FOUND )
        {
            m_listBox->SetItemState( sel, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );

            // Set to a small size so EnsureVisible() won't be foiled by later additions.
            // ListBox will expand to fit later.
            m_listBox->SetSize( m_listBox->GetSize().GetX(), FromDIP( 100 ) );
            m_listBox->EnsureVisible( sel );
        }
    }
}


void EDA_LIST_DIALOG::SetListLabel( const wxString& aLabel )
{
    m_listLabel->SetLabel( aLabel );
    m_listBox->SetSingleStyle( wxLC_NO_HEADER, true );
}


void EDA_LIST_DIALOG::SetOKLabel( const wxString& aLabel )
{
    m_sdbSizerOK->SetLabel( aLabel );
}


void EDA_LIST_DIALOG::HideFilter()
{
    m_filterBox->Hide();
}


void EDA_LIST_DIALOG::textChangeInFilterBox( wxCommandEvent& event )
{
    wxString filter;
    wxString itemName;

    filter = wxT( "*" ) + m_filterBox->GetLineText( 0 ).MakeLower() + wxT( "*" );

    m_listBox->DeleteAllItems();

    for( const wxArrayString& row : m_itemsList )
    {
        itemName = row.Item( 0 );

        if( itemName.MakeLower().Matches( filter ) )
            Append( row );
    }

    sortList();
}


long EDA_LIST_DIALOG::GetSelection()
{
    return m_listBox->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
}


wxString EDA_LIST_DIALOG::GetTextSelection( int aColumn )
{
    wxCHECK_MSG( unsigned( aColumn ) < unsigned( m_listBox->GetColumnCount() ), wxEmptyString,
                 wxT( "Invalid list control column." ) );

    long     item = m_listBox->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
    wxString text;

    if( item >= 0 )     // if something is selected.
    {
        wxListItem info;

        info.m_mask = wxLIST_MASK_TEXT;
        info.m_itemId = item;
        info.m_col = aColumn;

        if( m_listBox->GetItem( info ) )
            text = info.m_text;

        if( text.StartsWith( LIB_TREE_MODEL_ADAPTER::GetPinningSymbol() ) )
            text = text.substr( LIB_TREE_MODEL_ADAPTER::GetPinningSymbol().length() );
    }

    return text;
}


void EDA_LIST_DIALOG::GetExtraCheckboxValues()
{
    for( const auto& [checkbox, valuePtr] : m_extraCheckboxMap )
        *valuePtr = checkbox->GetValue();
}


void EDA_LIST_DIALOG::Append( const wxArrayString& itemList )
{
    long itemIndex = m_listBox->InsertItem( m_listBox->GetItemCount(), itemList[0] );

    m_listBox->SetItemPtrData( itemIndex, wxUIntPtr( &itemList[0] ) );

    // Adding the next columns content
    for( unsigned i = 1; i < itemList.size(); i++ )
        m_listBox->SetItem( itemIndex, i, itemList[i] );
}


void EDA_LIST_DIALOG::InsertItems( const std::vector< wxArrayString >& itemList, int position )
{
    for( unsigned row = 0; row < itemList.size(); row++ )
    {
        wxASSERT( (int) itemList[row].GetCount() == m_listBox->GetColumnCount() );

        for( unsigned col = 0; col < itemList[row].GetCount(); col++ )
        {
            wxListItem info;
            info.m_itemId = row + position;
            info.m_col = col;
            info.m_text = itemList[row].Item( col );
            info.m_width = FromDIP( DEFAULT_COL_WIDTHS[ col ] );
            info.m_mask = wxLIST_MASK_TEXT | wxLIST_MASK_WIDTH;

            if( col == 0 )
            {
                info.m_data = wxUIntPtr( &itemList[row].Item( col ) );
                info.m_mask |= wxLIST_MASK_DATA;

                m_listBox->InsertItem( info );
            }
            else
            {
                m_listBox->SetItem( info );
            }
        }
    }

    sortList();
}


void EDA_LIST_DIALOG::onListItemActivated( wxListEvent& event )
{
    wxPostEvent( this, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK ) );
}


void EDA_LIST_DIALOG::onSize( wxSizeEvent& event )
{
    if( m_listBox->GetColumnCount() == 1 )
    {
        m_listBox->SetColumnWidth( 0, m_listBox->GetClientSize().x );
    }
    else if( m_listBox->GetColumnCount() == 2 )
    {
        int first = KiROUND( m_listBox->GetClientSize().x * 0.42 );
        m_listBox->SetColumnWidth( 0, first );
        m_listBox->SetColumnWidth( 1, m_listBox->GetClientSize().x - first );
    }

    event.Skip();
}


/*
 * Sort alphabetically, case insensitive.
 */
static int wxCALLBACK myCompareFunction( wxIntPtr aItem1, wxIntPtr aItem2, wxIntPtr WXUNUSED( aSortData ) )
{
    wxString* component1Name = (wxString*) aItem1;
    wxString* component2Name = (wxString*) aItem2;

    return StrNumCmp( *component1Name, *component2Name, true );
}


void EDA_LIST_DIALOG::sortList()
{
    if( m_sortList )
        m_listBox->SortItems( myCompareFunction, 0 );
}
