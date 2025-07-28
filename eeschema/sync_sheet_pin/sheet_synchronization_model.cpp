/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Ethan Chien <liangtie.qian@gmail.com>
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

#include "sheet_synchronization_model.h"
#include "sheet_synchronization_item.h"
#include "sheet_synchronization_notifier.h"
#include "sheet_synchronization_agent.h"

#include <sch_label.h>
#include <sch_sheet_pin.h>
#include <wx/colour.h>
#include <wx/variant.h>


// sch_label.cpp
extern wxString getElectricalTypeLabel( LABEL_FLAG_SHAPE aType );


SHEET_SYNCHRONIZATION_MODEL::SHEET_SYNCHRONIZATION_MODEL( SHEET_SYNCHRONIZATION_AGENT& aAgent,
                                                          SCH_SHEET*                   aSheet,
                                                          const SCH_SHEET_PATH&        aPath ) :
        m_selectedIndex( std::optional<unsigned>() ),
        m_agent( aAgent ),
        m_sheet( aSheet ),
        m_path( aPath )
{
}


SHEET_SYNCHRONIZATION_MODEL::~SHEET_SYNCHRONIZATION_MODEL() = default;


void SHEET_SYNCHRONIZATION_MODEL::GetValueByRow( wxVariant& aVariant, unsigned row,
                                                 unsigned col ) const
{
    const std::shared_ptr<SHEET_SYNCHRONIZATION_ITEM>& item = m_items[row];

    switch( col )
    {
    case NAME:
        aVariant << wxDataViewIconText( item->GetName(), item->GetBitmap() );
        break;
    case SHAPE:
        aVariant = getElectricalTypeLabel( static_cast<LABEL_FLAG_SHAPE>( item->GetShape() ) );
        break;
    }
}


bool SHEET_SYNCHRONIZATION_MODEL::SetValueByRow( const wxVariant& aVariant, unsigned row,
                                                 unsigned col )
{
    WXUNUSED( aVariant )
    WXUNUSED( row )
    WXUNUSED( col )

    return {};
}


bool SHEET_SYNCHRONIZATION_MODEL::GetAttrByRow( unsigned row, unsigned int col,
                                                wxDataViewItemAttr& attr ) const
{
    if( m_selectedIndex.has_value() && row == m_selectedIndex )
    {
        attr.SetBold( true );
        return true;
    }

    return false;
}


void SHEET_SYNCHRONIZATION_MODEL::RemoveItems( wxDataViewItemArray const& aItems )
{
    if( aItems.empty() )
        return;

    for( const auto& item : TakeItems( aItems ) )
        m_agent.RemoveItem( *item, m_sheet, m_path );

    DoNotify();
}


bool SHEET_SYNCHRONIZATION_MODEL::AppendNewItem( std::shared_ptr<SHEET_SYNCHRONIZATION_ITEM> aItem )
{
    m_items.push_back( std::move( aItem ) );
    Reset( GetCount() );
    DoNotify();
    return true;
}


bool SHEET_SYNCHRONIZATION_MODEL::AppendItem( std::shared_ptr<SHEET_SYNCHRONIZATION_ITEM> aItem )
{
    m_items.push_back( std::move( aItem ) );
    Reset( GetCount() );
    return true;
}


SHEET_SYNCHRONIZATION_ITEM_LIST
SHEET_SYNCHRONIZATION_MODEL::TakeItems( wxDataViewItemArray const& aItems )
{
    if( aItems.size() == 1 )
        return { TakeItem( aItems[0] ) };

    std::set<unsigned>              rowsToBeRemove;
    SHEET_SYNCHRONIZATION_ITEM_LIST items_remain;
    SHEET_SYNCHRONIZATION_ITEM_LIST items_token;

    for( const auto& item : aItems )
    {
        if( item.IsOk() )
        {
            unsigned int idx = GetRow( item );
            rowsToBeRemove.insert( idx );
        }
    }

    for( unsigned i = 0; i < m_items.size(); i++ )
    {
        if( rowsToBeRemove.find( i ) == rowsToBeRemove.end() )
        {
            items_remain.push_back( m_items[i] );
        }
        else
        {
            items_token.push_back( m_items[i] );
        }
    }

    UpdateItems( std::move( items_remain ) );
    OnRowSelected( {} );
    return items_token;
}


SHEET_SYNCHRONIZATION_ITE_PTR SHEET_SYNCHRONIZATION_MODEL::TakeItem( wxDataViewItem const& aItem )
{
    const unsigned int row = GetRow( aItem );

    if( row + 1 > m_items.size() )
        return {};

    std::shared_ptr<SHEET_SYNCHRONIZATION_ITEM> item = m_items[row];
    m_items.erase( m_items.begin() + row );
    OnRowSelected( {} );
    Reset( GetCount() );
    return item;
}


SHEET_SYNCHRONIZATION_ITE_PTR
SHEET_SYNCHRONIZATION_MODEL::GetSynchronizationItem( unsigned aIndex ) const
{
    if( aIndex < m_items.size() )
        return m_items[aIndex];

    return {};
}


SHEET_SYNCHRONIZATION_ITE_PTR
SHEET_SYNCHRONIZATION_MODEL::GetSynchronizationItem( wxDataViewItem const& aItem ) const
{
    return GetSynchronizationItem( GetRow( aItem ) );
}


void SHEET_SYNCHRONIZATION_MODEL::OnRowSelected( std::optional<unsigned> aRow )
{
    m_selectedIndex = aRow;

    if( aRow.has_value() && m_items.size() > *aRow )
    {
        if( wxDataViewItem item = GetItem( *aRow ); item.IsOk() )
            ItemChanged( item );
    }
}


void SHEET_SYNCHRONIZATION_MODEL::UpdateItems( SHEET_SYNCHRONIZATION_ITEM_LIST aItems )
{
    m_items = std::move( aItems );
    Reset( GetCount() );
}


void SHEET_SYNCHRONIZATION_MODEL::AddNotifier(
        std::shared_ptr<SHEET_SYNCHRONIZATION_NOTIFIER> aNotifier )
{
    m_notifiers.push_back( std::move( aNotifier ) );
}


void SHEET_SYNCHRONIZATION_MODEL::DoNotify()
{
    for( const auto& notifier : m_notifiers )
        notifier->Notify();
}

unsigned int SHEET_SYNCHRONIZATION_MODEL::GetCount() const
{
    return m_items.size();
}
