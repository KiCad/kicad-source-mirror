/*
 * This program source code file is part of KICAD, a free EDA CAD application.
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

#include <drawing_sheet/ds_proxy_undo_item.h>
#include <drawing_sheet/ds_data_item.h>
#include <drawing_sheet/ds_data_model.h>
#include <view/view.h>
#include <eda_draw_frame.h>
#include <string_utils.h>

using namespace KIGFX;

DS_PROXY_UNDO_ITEM::DS_PROXY_UNDO_ITEM( const EDA_DRAW_FRAME* aFrame ) :
        EDA_ITEM( aFrame ? WS_PROXY_UNDO_ITEM_PLUS_T : WS_PROXY_UNDO_ITEM_T ),
        m_selectedDataItem( INT_MAX ),
        m_selectedDrawItem( INT_MAX )
{
    if( aFrame )
    {
        m_pageInfo = aFrame->GetPageSettings();
        m_titleBlock = aFrame->GetTitleBlock();
    }

    DS_DATA_MODEL& model = DS_DATA_MODEL::GetTheInstance();
    model.SaveInString( &m_layoutSerialization );

    for( size_t ii = 0; ii < model.GetItems().size(); ++ii )
    {
        DS_DATA_ITEM* dataItem = model.GetItem( ii );

        for( size_t jj = 0; jj < dataItem->GetDrawItems().size(); ++jj )
        {
            DS_DRAW_ITEM_BASE* drawItem = dataItem->GetDrawItems()[ jj ];

            if( drawItem->IsSelected() )
            {
                m_selectedDataItem = ii;
                m_selectedDrawItem = jj;
                break;
            }
        }
    }
}


void DS_PROXY_UNDO_ITEM::Restore( EDA_DRAW_FRAME* aFrame, KIGFX::VIEW* aView )
{
    if( Type() == WS_PROXY_UNDO_ITEM_PLUS_T )
    {
        aFrame->SetPageSettings( m_pageInfo );
        aFrame->SetTitleBlock( m_titleBlock );
    }

    DS_DATA_MODEL::GetTheInstance().SetPageLayout( TO_UTF8( m_layoutSerialization ) );

    if( aView )
    {
        aView->Clear();

        for( int ii = 0; ii < (int)DS_DATA_MODEL::GetTheInstance().GetItems().size(); ++ii )
        {
            DS_DATA_ITEM* dataItem = DS_DATA_MODEL::GetTheInstance().GetItem( ii );

            dataItem->SyncDrawItems( nullptr, aView );

            if( ii == m_selectedDataItem
              && m_selectedDrawItem < (int)dataItem->GetDrawItems().size() )
            {
                DS_DRAW_ITEM_BASE* drawItem = dataItem->GetDrawItems()[ m_selectedDrawItem ];
                drawItem->SetSelected();
            }
        }
    }
}

