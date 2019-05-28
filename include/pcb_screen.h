/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file pcb_screen.h
 */

#ifndef PCB_SCREEN_H
#define PCB_SCREEN_H


#include <base_screen.h>
#include <class_board_item.h>


class UNDO_REDO_CONTAINER;


/* Handle info to display a board */
class PCB_SCREEN : public BASE_SCREEN
{
public:
    PCB_LAYER_ID m_Active_Layer;
    PCB_LAYER_ID m_Route_Layer_TOP;
    PCB_LAYER_ID m_Route_Layer_BOTTOM;

public:

    /**
     * Constructor
     * @param aPageSizeIU is the size of the initial paper page in internal units.
     */
    PCB_SCREEN( const wxSize& aPageSizeIU );

    ~PCB_SCREEN();

    PCB_SCREEN* Next() const { return static_cast<PCB_SCREEN*>( Pnext ); }

    /* full undo redo management : */

    // use BASE_SCREEN::ClearUndoRedoList()
    // use BASE_SCREEN::PushCommandToUndoList( PICKED_ITEMS_LIST* aItem )
    // use BASE_SCREEN::PushCommandToRedoList( PICKED_ITEMS_LIST* aItem )

    /**
     * Function ClearUndoORRedoList
     * free the undo or redo list from List element
     *  Wrappers are deleted.
     *  datas pointed by wrappers are deleted if not in use in schematic
     *  i.e. when they are copy of a schematic item or they are no more in use
     *  (DELETED)
     * @param aList = the UNDO_REDO_CONTAINER to clear
     * @param aItemCount = the count of items to remove. < 0 for all items
     * items are removed from the beginning of the list.
     * So this function can be called to remove old commands
     */
    void ClearUndoORRedoList( UNDO_REDO_CONTAINER& aList, int aItemCount = -1 ) override;
};

#endif  // PCB_SCREEN_H
