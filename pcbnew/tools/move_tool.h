/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author @author Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef __MOVE_TOOL_H
#define __MOVE_TOOL_H

#include <math/vector2d.h>
#include <tool/tool_interactive.h>

class BOARD_ITEM;
class SELECTION_TOOL;

namespace KiGfx
{
class VIEW_GROUP;
}

/**
 * Class MOVE_TOOL
 *                                                      /// TODO DOCS!!
 * Our sample move tool: currently supports:
 * - pick single objects (click LMB)
 * - add objects to existing move (Shift+LMB)
 * - draw move box (drag LMB)
 *
 * WORK IN PROGRESS. CONSIDER AS A DEMO!
 */

class MOVE_TOOL : public TOOL_INTERACTIVE
{
public:
    MOVE_TOOL();
    ~MOVE_TOOL();

    /**
     * Function Reset()
     *
     * Resets the tool and initializes it.
     */
    void Reset();

    /**
     * Function Main()
     *
     * Main loop in which events are handled.
     */
    int Main( TOOL_EVENT& aEvent );

private:
    void viewGroupAdd( BOARD_ITEM* aItem, KiGfx::VIEW_GROUP* aGroup );

    /// Structure for (re)storing BOARD_ITEM state
    typedef struct
    {
        BOARD_ITEM* item;       /// Pointer to the item
        VECTOR2D position;      /// Original position of the item
        bool visible;           /// Original visibility flag

        void Save( BOARD_ITEM* aItem )
        {
            wxPoint pos = aItem->GetPosition();

            item = aItem;
            position.x = pos.x;
            position.y = pos.y;
            visible = aItem->ViewIsVisible();
        }

        void RestorePosition()
        {
            wxPoint curPosition = item->GetPosition();
            item->Move( wxPoint( position.x - curPosition.x, position.y - curPosition.y ) );
        }

        void RestoreVisibility()
        {
            item->ViewSetVisible( visible );
        }

        void Restore()
        {
            RestorePosition();
            RestoreVisibility();
        }
    } ITEM_STATE;

    /// Selection tool used for obtaining selected items
    SELECTION_TOOL* m_selectionTool;

    std::deque<ITEM_STATE> m_itemsState;
};

#endif
