/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef BOARD_ITEM_CONTAINER_H
#define BOARD_ITEM_CONTAINER_H

#include <class_board_item.h>

enum ADD_MODE { ADD_INSERT, ADD_APPEND };

/**
 * @brief Abstract interface for BOARD_ITEMs capable of storing other items inside.
 * @see MODULE
 * @see BOARD
 */
class BOARD_ITEM_CONTAINER : public BOARD_ITEM
{
public:
    BOARD_ITEM_CONTAINER( BOARD_ITEM* aParent, KICAD_T aType )
        : BOARD_ITEM( aParent, aType )
    {
    }

    virtual ~BOARD_ITEM_CONTAINER()
    {
    }

    /**
     * @brief Adds an item to the container.
     * @param aItem is an item to be added.
     * @param aMode decides whether the item is added in the beginning or at the end of the list.
     */
    virtual void Add( BOARD_ITEM* aItem, ADD_MODE aMode = ADD_INSERT ) = 0;

    /**
     * @brief Removes an item from the container.
     * @param aItem is an item to be removed.
     */
    virtual void Remove( BOARD_ITEM* aItem ) = 0;

    /**
     * @brief Removes an item from the containter and deletes it.
     * @param aItem is an item to be deleted.
     */
    virtual void Delete( BOARD_ITEM* aItem )
    {
        Remove( aItem );
        delete aItem;
    }
};

#endif /* BOARD_ITEM_CONTAINER_H */
