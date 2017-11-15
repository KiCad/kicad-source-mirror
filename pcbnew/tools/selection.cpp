/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 * Copyright (C) 2017 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <limits>

#include <functional>
using namespace std::placeholders;

#include <class_board.h>
#include <class_board_item.h>
#include <class_track.h>
#include <class_module.h>
#include <class_pcb_text.h>
#include <class_drawsegment.h>
#include <class_zone.h>

#include <wxPcbStruct.h>

#include <class_draw_panel_gal.h>
#include <view/view_controls.h>
#include <view/view_group.h>
#include <preview_items/selection_area.h>
#include <painter.h>
#include <bitmaps.h>
#include <hotkeys.h>

#include <tool/tool_event.h>
#include <tool/tool_manager.h>
#include <connectivity_data.h>

#include "selection_tool.h"
#include "pcb_bright_box.h"
#include "pcb_actions.h"

#include "kicad_plugin.h"

// TODO(JE) Only works for BOARD_ITEM
VECTOR2I SELECTION::GetPosition() const
{
    return static_cast<VECTOR2I>( GetBoundingBox().GetPosition() );
}


VECTOR2I SELECTION::GetCenter() const
{
    return static_cast<VECTOR2I>( GetBoundingBox().Centre() );
}


EDA_RECT SELECTION::GetBoundingBox() const
{
    EDA_RECT bbox;

    bbox = Front()->GetBoundingBox();
    auto i = m_items.begin();
    ++i;

    for( ; i != m_items.end(); ++i )
    {
        bbox.Merge( (*i)->GetBoundingBox() );
    }

    return bbox;
}


EDA_ITEM* SELECTION::GetTopLeftItem( bool onlyModules ) const
{
    BOARD_ITEM* topLeftItem = nullptr;
    BOARD_ITEM* currentItem;

    wxPoint pnt;

    // find the leftmost (smallest x coord) and highest (smallest y with the smallest x) item in the selection
    for( auto item : m_items )
    {
        currentItem = static_cast<BOARD_ITEM*>( item );
        pnt = currentItem->GetPosition();

        if( ( currentItem->Type() != PCB_MODULE_T ) && onlyModules )
        {
            continue;
        }
        else
        {
            if( topLeftItem == nullptr )
            {
                topLeftItem = currentItem;
            }
            else if( ( pnt.x < topLeftItem->GetPosition().x ) ||
                     ( ( topLeftItem->GetPosition().x == pnt.x ) &&
                     ( pnt.y < topLeftItem->GetPosition().y ) ) )
            {
                topLeftItem = currentItem;
            }
        }
    }

    return static_cast<EDA_ITEM*>( topLeftItem );
}


EDA_ITEM* SELECTION::GetTopLeftModule() const
{
    return GetTopLeftItem( true );
}


const BOX2I SELECTION::ViewBBox() const
{
    BOX2I r;
    r.SetMaximum();
    return r;
    EDA_RECT eda_bbox;

    if( Size() == 1 )
    {
        eda_bbox = Front()->GetBoundingBox();
    }
    else if( Size() > 1 )
    {
        eda_bbox = Front()->GetBoundingBox();
        auto i = m_items.begin();
        ++i;

        for( ; i != m_items.end(); ++i )
        {
            eda_bbox.Merge( (*i)->GetBoundingBox() );
        }
    }

    return BOX2I( eda_bbox.GetOrigin(), eda_bbox.GetSize() );
}


const KIGFX::VIEW_GROUP::ITEMS SELECTION::updateDrawList() const
{
    std::vector<VIEW_ITEM*> items;

    for( auto item : m_items )
    {
        items.push_back( item );

        if( item->Type() == PCB_MODULE_T )
        {
            MODULE* module = static_cast<MODULE*>( item );
            module->RunOnChildren( [&] ( BOARD_ITEM* bitem ) { items.push_back( bitem ); } );
        }
    }

    return items;
}
