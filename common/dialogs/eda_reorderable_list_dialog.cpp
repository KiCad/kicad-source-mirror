/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <algorithm>
#include <set>
#include <bitmaps.h>
#include <string_utils.h>
#include <dialogs/eda_reorderable_list_dialog.h>
#include <widgets/std_bitmap_button.h>


static int DEFAULT_SINGLE_COL_WIDTH = 260;


EDA_REORDERABLE_LIST_DIALOG::EDA_REORDERABLE_LIST_DIALOG( wxWindow* aParent, const wxString& aTitle,
                                                          const std::vector<wxString>& aAllItems,
                                                          const std::vector<wxString>& aEnabledItems ) :
        EDA_REORDERABLE_LIST_DIALOG_BASE( aParent, wxID_ANY, aTitle ),
        m_availableItems( aAllItems ),
        m_enabledItems( aEnabledItems ),
        m_selectedAvailable( 0 ),
        m_selectedEnabled( 0 )
{
    m_btnUp->SetBitmap( KiBitmapBundle( BITMAPS::small_up ) );
    m_btnDown->SetBitmap( KiBitmapBundle( BITMAPS::small_down ) );

    // DIALOG_SHIM needs a unique hash_key because classname is not sufficient
    // because so many dialogs share this same class, with different numbers of
    // columns, different column names, and column widths.
    m_hash_key = TO_UTF8( aTitle );

    m_availableListBox->InsertColumn( 0, wxEmptyString, wxLIST_FORMAT_LEFT,
                                      DEFAULT_SINGLE_COL_WIDTH );
    m_enabledListBox->InsertColumn( 0, wxEmptyString, wxLIST_FORMAT_LEFT,
                                    DEFAULT_SINGLE_COL_WIDTH );

    updateItems();

    SetupStandardButtons();

    // this line fixes an issue on Linux Ubuntu using Unity (dialog not shown),
    // and works fine on all systems
    GetSizer()->Fit(  this );

    Centre();
}


void EDA_REORDERABLE_LIST_DIALOG::updateItems()
{
    m_availableListBox->DeleteAllItems();
    m_enabledListBox->DeleteAllItems();

    std::set<wxString> enabledSet;

    for( size_t idx = 0; idx < m_enabledItems.size(); ++idx )
    {
        wxListItem info;
        info.m_itemId = idx;
        info.m_col    = 0;
        info.m_text   = m_enabledItems[idx];
        info.m_width  = DEFAULT_SINGLE_COL_WIDTH;
        info.m_mask   = wxLIST_MASK_TEXT | wxLIST_MASK_WIDTH;

        m_enabledListBox->InsertItem( info );

        if( m_selectedEnabled == static_cast<long>( idx ) )
            m_enabledListBox->SetItemState( idx, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );

        enabledSet.insert( m_enabledItems[idx] );
    }

    m_availableItems.erase( std::remove_if( m_availableItems.begin(), m_availableItems.end(),
                                            [&]( const wxString& aItem ) -> bool
                                            {
                                                return enabledSet.count( aItem );
                                            } ),
                            m_availableItems.end() );

    for( size_t idx = 0; idx < m_availableItems.size(); ++idx )
    {
        wxListItem info;
        info.m_itemId = idx;
        info.m_col    = 0;
        info.m_text   = m_availableItems[idx];
        info.m_width  = DEFAULT_SINGLE_COL_WIDTH;
        info.m_mask   = wxLIST_MASK_TEXT | wxLIST_MASK_WIDTH;

        m_availableListBox->InsertItem( info );

        if( m_selectedAvailable == static_cast<long>( idx ) )
            m_availableListBox->SetItemState( idx, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
    }

    if( !m_availableItems.empty() )
        m_availableListBox->EnsureVisible( m_selectedAvailable );

    if( !m_enabledItems.empty() )
        m_enabledListBox->EnsureVisible( m_selectedEnabled );

    m_btnAdd->Enable( !m_availableItems.empty() );
    m_btnRemove->Enable( !m_enabledItems.empty() );
}


void EDA_REORDERABLE_LIST_DIALOG::onAddItem( wxCommandEvent& aEvent )
{
    wxListItem info;

    if( !getSelectedItem( m_availableListBox, info ) )
    {
        wxBell();
        return;
    }

    m_availableItems.erase( m_availableItems.begin() + info.m_itemId );
    m_availableListBox->DeleteItem( m_selectedAvailable );

    long pos = std::min( m_selectedEnabled + 1, static_cast<long>( m_enabledItems.size() ) );

    info.m_itemId = pos;
    info.m_mask   = wxLIST_MASK_TEXT;

    m_enabledItems.insert( m_enabledItems.begin() + pos, info.m_text );
    m_enabledListBox->InsertItem( info );
}


void EDA_REORDERABLE_LIST_DIALOG::onRemoveItem( wxCommandEvent& aEvent )
{
    wxListItem info;

    if( !getSelectedItem( m_enabledListBox, info ) || info.m_itemId == 0 )
    {
        wxBell();
        return;
    }

    m_enabledItems.erase( m_enabledItems.begin() + info.m_itemId );
    m_enabledListBox->DeleteItem( m_selectedEnabled );

    m_selectedEnabled = std::min( m_selectedEnabled,
                                  static_cast<long>( m_enabledItems.size() - 1 ) );

    m_enabledListBox->SetItemState( m_selectedEnabled, wxLIST_STATE_SELECTED,
                                    wxLIST_STATE_SELECTED );

    long pos = std::max( long( 0 ), m_selectedAvailable );
    info.m_itemId = pos;

    m_availableItems.insert( m_availableItems.begin() + pos, info.m_text );
    m_availableListBox->InsertItem( info );
}


void EDA_REORDERABLE_LIST_DIALOG::onMoveUp( wxCommandEvent& aEvent )
{
    wxListItem info;

    if( !getSelectedItem( m_enabledListBox, info ) || info.m_itemId == 0 )
    {
        wxBell();
        return;
    }

    auto current = m_enabledItems.begin() + info.m_itemId;
    auto prev    = m_enabledItems.begin() + info.m_itemId - 1;

    std::iter_swap( current, prev );

    m_selectedEnabled--;

    updateItems();
}


void EDA_REORDERABLE_LIST_DIALOG::onMoveDown( wxCommandEvent& aEvent )
{
    wxListItem info;

    if( !getSelectedItem( m_enabledListBox, info )
        || info.m_itemId == static_cast<long>( m_enabledItems.size() ) - 1 )
    {
        wxBell();
        return;
    }

    auto current = m_enabledItems.begin() + info.m_itemId;
    auto prev    = m_enabledItems.begin() + info.m_itemId + 1;

    std::iter_swap( current, prev );

    m_selectedEnabled++;

    updateItems();
}


bool EDA_REORDERABLE_LIST_DIALOG::getSelectedItem( wxListCtrl* aList, wxListItem& aInfo )
{
    long item = aList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );

    if( item < 0 )
        return false;

    aInfo.m_mask   = wxLIST_MASK_DATA | wxLIST_MASK_STATE | wxLIST_MASK_TEXT;
    aInfo.m_itemId = item;
    aInfo.m_col    = 0;

    if( !aList->GetItem( aInfo ) )
        return false;

    return true;
}


void EDA_REORDERABLE_LIST_DIALOG::onEnabledListItemSelected( wxListEvent& event )
{
    wxListItem info;

    if( !getSelectedItem( m_enabledListBox, info ) )
    {
        m_selectedEnabled = -1;
        return;
    }

    m_selectedEnabled = info.m_itemId;
}


void EDA_REORDERABLE_LIST_DIALOG::onAvailableListItemSelected( wxListEvent& event )
{
    wxListItem info;

    if( !getSelectedItem( m_availableListBox, info ) )
    {
        m_selectedAvailable = -1;
        return;
    }

    m_selectedAvailable = info.m_itemId;
}

