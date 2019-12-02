/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <macros.h>
#include <eda_draw_frame.h>
#include <kicad_string.h>
#include <dialog_helpers.h>


// wxWidgets spends *far* too long calcuating column widths (most of it, believe it or
// not, in repeatedly creating/destroying a wxDC to do the measurement in).
// Use default column widths instead.
static int DEFAULT_COL_WIDTHS[] = { 200, 600 };



EDA_LIST_DIALOG::EDA_LIST_DIALOG( EDA_DRAW_FRAME* aParent, const wxString& aTitle,
                                  const wxArrayString& aItemHeaders,
                                  const std::vector<wxArrayString>& aItemList,
                                  const wxString& aSelection,
                                  void( *aCallBackFunction )( wxString&, void* ),
                                  void* aCallBackFunctionData ) :
    EDA_LIST_DIALOG_BASE( aParent, wxID_ANY, aTitle )
{
    m_cb_func     = aCallBackFunction;
    m_cb_data     = aCallBackFunctionData;
    m_itemsListCp = &aItemList;

    m_filterBox->SetHint( _( "Filter" ) );

    initDialog( aItemHeaders, aSelection );

    // DIALOG_SHIM needs a unique hash_key because classname is not sufficient
    // because so many dialogs share this same class, with different numbers of
    // columns, different column names, and column widths.
    m_hash_key = TO_UTF8( aTitle );

    m_sdbSizerOK->SetDefault();

    // this line fixes an issue on Linux Ubuntu using Unity (dialog not shown),
    // and works fine on all systems
    GetSizer()->Fit(  this );

    Centre();
}


void EDA_LIST_DIALOG::initDialog( const wxArrayString& aItemHeaders, const wxString& aSelection)
{
    for( unsigned i = 0; i < aItemHeaders.Count(); i++ )
        m_listBox->InsertColumn( i, aItemHeaders.Item( i ),
                                 wxLIST_FORMAT_LEFT, DEFAULT_COL_WIDTHS[ i ] );

    InsertItems( *m_itemsListCp, 0 );

    if( m_cb_func == NULL )
    {
        m_messages->Show( false );
        m_staticTextMsg->Show( false );
    }

    if( !aSelection.IsEmpty() )
    {
        for( unsigned row = 0; row < m_itemsListCp->size(); ++row )
        {
            if( (*m_itemsListCp)[row][0] == aSelection )
            {
                m_listBox->SetItemState( row, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );

                // Set to a small size so EnsureVisible() won't be foiled by later additions.
                // ListBox will expand to fit later.
                m_listBox->SetSize( m_listBox->GetSize().GetX(), 100 );
                m_listBox->EnsureVisible( row );

                break;
            }
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


void EDA_LIST_DIALOG::textChangeInFilterBox( wxCommandEvent& event )
{
    wxString filter;
    wxString itemName;

    filter = wxT( "*" ) + m_filterBox->GetLineText( 0 ).MakeLower() + wxT( "*" );

    m_listBox->DeleteAllItems();

    for( unsigned i = 0; i < m_itemsListCp->size(); i++ )
    {
        itemName = (*m_itemsListCp)[i].Item( 0 );

        if( itemName.MakeLower().Matches( filter ) )
        {
            Append( (*m_itemsListCp)[i] );
        }
    }

    sortList();
}


wxString EDA_LIST_DIALOG::GetTextSelection( int aColumn )
{
    wxCHECK_MSG( unsigned( aColumn ) < unsigned( m_listBox->GetColumnCount() ), wxEmptyString,
                 wxT( "Invalid list control column." ) );

    long    item = m_listBox->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );

    if( item >= 0 )     // if something is selected.
    {
        wxListItem info;

        info.m_mask = wxLIST_MASK_TEXT;
        info.m_itemId = item;
        info.m_col = aColumn;

        if( m_listBox->GetItem( info ) )
            return info.m_text;
    }

    return wxEmptyString;
}


void EDA_LIST_DIALOG::Append( const wxArrayString& itemList )
{
    long itemIndex = m_listBox->InsertItem( m_listBox->GetItemCount(), itemList[0] );

    m_listBox->SetItemPtrData( itemIndex, wxUIntPtr( &itemList[0] ) );

    // Adding the next columns content
    for( unsigned i = 1; i < itemList.size(); i++ )
    {
        m_listBox->SetItem( itemIndex, i, itemList[i] );
    }
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
            info.m_width = DEFAULT_COL_WIDTHS[ col ];
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


void EDA_LIST_DIALOG::onListItemSelected( wxListEvent& event )
{
    if( m_cb_func )
    {
        m_messages->Clear();
        wxString text = GetTextSelection();
        m_cb_func( text, m_cb_data );
        m_messages->WriteText( text );
    }
}


void EDA_LIST_DIALOG::onListItemActivated( wxListEvent& event )
{
    EndModal( wxID_OK );
}


/* Sort alphabetically, case insensitive.
 */
static int wxCALLBACK myCompareFunction( wxIntPtr aItem1, wxIntPtr aItem2,
                                         wxIntPtr WXUNUSED( aSortData ) )
{
    wxString* component1Name = (wxString*) aItem1;
    wxString* component2Name = (wxString*) aItem2;

    return StrNumCmp( *component1Name, *component2Name, true );
}


void EDA_LIST_DIALOG::sortList()
{
    m_listBox->SortItems( myCompareFunction, 0 );
}
