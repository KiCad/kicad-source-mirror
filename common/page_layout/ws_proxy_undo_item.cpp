/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2019 Kicad Developers, see AUTHORS.txt for contributors.
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

#include <ws_proxy_undo_item.h>
#include <ws_data_model.h>
#include <eda_draw_frame.h>
#include <macros.h>

using namespace KIGFX;

WS_PROXY_UNDO_ITEM::WS_PROXY_UNDO_ITEM( const EDA_DRAW_FRAME* aFrame ) :
        EDA_ITEM( aFrame ? WS_PROXY_UNDO_ITEM_PLUS_T : WS_PROXY_UNDO_ITEM_T ),
        m_selectedDataItem( INT_MAX ),
        m_selectedDrawItem( INT_MAX )
{
    if( aFrame )
    {
        m_pageInfo = aFrame->GetPageSettings();
        m_titleBlock = aFrame->GetTitleBlock();
    }

    WS_DATA_MODEL& model = WS_DATA_MODEL::GetTheInstance();
    model.SaveInString( m_layoutSerialization );

    for( size_t ii = 0; ii < model.GetItems().size(); ++ii )
    {
        WS_DATA_ITEM* dataItem = model.GetItem( ii );

        for( size_t jj = 0; jj < dataItem->GetDrawItems().size(); ++jj )
        {
            WS_DRAW_ITEM_BASE* drawItem = dataItem->GetDrawItems()[ jj ];

            if( drawItem->IsSelected() )
            {
                m_selectedDataItem = ii;
                m_selectedDrawItem = jj;
                break;
            }
        }
    }
}


void WS_PROXY_UNDO_ITEM::Restore( EDA_DRAW_FRAME* aFrame, KIGFX::VIEW* aView )
{
    if( Type() == WS_PROXY_UNDO_ITEM_PLUS_T )
    {
        aFrame->SetPageSettings( m_pageInfo );
        aFrame->SetTitleBlock( m_titleBlock );
    }

    WS_DATA_MODEL::GetTheInstance().SetPageLayout( TO_UTF8( m_layoutSerialization ) );

    if( aView )
    {
        aView->Clear();

        for( int ii = 0; ii < (int)WS_DATA_MODEL::GetTheInstance().GetItems().size(); ++ii )
        {
            WS_DATA_ITEM* dataItem = WS_DATA_MODEL::GetTheInstance().GetItem( ii );

            dataItem->SyncDrawItems( nullptr, aView );

            if( ii == m_selectedDataItem && m_selectedDrawItem < (int)dataItem->GetDrawItems().size() )
            {
                WS_DRAW_ITEM_BASE* drawItem = dataItem->GetDrawItems()[ m_selectedDrawItem ];
                drawItem->SetSelected();
            }
        }
    }
}

