/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
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
 * This file is part of the common library.
 * @file  block_commande.h
 * @see   common.h
 */

#ifndef __INCLUDE__BLOCK_COMMANDE_H__
#define __INCLUDE__BLOCK_COMMANDE_H__


#include <base_struct.h>
#include <class_undoredo_container.h>
#include <gr_basic.h>

/* Block state codes. */
typedef enum {
    STATE_NO_BLOCK,
    STATE_BLOCK_INIT,
    STATE_BLOCK_END,
    STATE_BLOCK_MOVE,
    STATE_BLOCK_STOP
} BLOCK_STATE_T;


/* Block command codes. */
typedef enum {
    BLOCK_IDLE,
    BLOCK_MOVE,
    BLOCK_COPY,
    BLOCK_COPY_AND_INCREMENT,
    BLOCK_SAVE,
    BLOCK_DELETE,
    BLOCK_PASTE,
    BLOCK_DRAG,
    BLOCK_DRAG_ITEM,    // like BLOCK_DRAG, when used to drag a selected component
                        // and not using an area defined by a mouse drag
    BLOCK_ROTATE,
    BLOCK_FLIP,
    BLOCK_ZOOM,
    BLOCK_ABORT,
    BLOCK_PRESELECT_MOVE,
    BLOCK_MOVE_EXACT,
    BLOCK_SELECT_ITEMS_ONLY,
    BLOCK_MIRROR_X,
    BLOCK_MIRROR_Y
} BLOCK_COMMAND_T;


class BLOCK_SELECTOR : public EDA_RECT
{
    BLOCK_STATE_T     m_state;                    //< State (enum BLOCK_STATE_T) of the block.
    BLOCK_COMMAND_T   m_command;                  //< Command (enum BLOCK_COMMAND_T) operation.
    PICKED_ITEMS_LIST m_items;                    //< List of items selected in this block.
    EDA_COLOR_T       m_color;                    //< Block Color (for drawings).
    wxPoint           m_moveVector;               //< Move distance to move the block.
    wxPoint           m_lastCursorPosition;       //< Last Mouse position in block command
                                                  //< last cursor position in move commands
                                                  //< 0,0 in paste command.

public:
    BLOCK_SELECTOR();
    ~BLOCK_SELECTOR();

    void SetState( BLOCK_STATE_T aState ) { m_state = aState; }

    BLOCK_STATE_T GetState() const { return m_state; }

    void SetCommand( BLOCK_COMMAND_T aCommand ) { m_command = aCommand; }

    BLOCK_COMMAND_T GetCommand() const { return m_command; }

    void SetColor( EDA_COLOR_T aColor ) { m_color = aColor; }

    EDA_COLOR_T GetColor() const { return m_color; }

    /**
     * Function SetLastCursorPosition
     * sets the last cursor position to \a aPosition.
     *
     * @param aPosition The current cursor position.
     */
    void SetLastCursorPosition( const wxPoint& aPosition ) { m_lastCursorPosition = aPosition; }

    wxPoint GetLastCursorPosition() const { return m_lastCursorPosition; }

    void SetMoveVector( const wxPoint& aMoveVector ) { m_moveVector = aMoveVector; }

    wxPoint GetMoveVector() const { return m_moveVector; }

    /**
     * Function InitData
     * sets the initial values of a BLOCK_SELECTOR, before starting a block
     * command
     */
    void InitData( EDA_DRAW_PANEL* Panel, const wxPoint& startpos );

    /**
     * Function SetMessageBlock
     * Displays the type of block command in the status bar of the window
     */
    void SetMessageBlock( EDA_DRAW_FRAME* frame );

    void Draw( EDA_DRAW_PANEL* aPanel,
               wxDC*           aDC,
               const wxPoint&  aOffset,
               GR_DRAWMODE     aDrawMode,
               EDA_COLOR_T     aColor );

    /**
     * Function PushItem
     * adds \a aItem to the list of items.
     * @param aItem = an ITEM_PICKER to add to the list
     */
    void PushItem( ITEM_PICKER& aItem );

    /**
     * Function ClearListAndDeleteItems
     * deletes only the list of EDA_ITEM * pointers, AND the data printed
     * by m_Item
     */
    void ClearListAndDeleteItems();

    /**
     * Function ClearItemsList
     * clear only the list of #EDA_ITEM pointers, it does _NOT_ delete the #EDA_ITEM object
     * itself
     */
    void ClearItemsList();

    unsigned GetCount() const
    {
        return m_items.GetCount();
    }

    PICKED_ITEMS_LIST& GetItems() { return m_items; }

    EDA_ITEM* GetItem( unsigned aIndex )
    {
        if( aIndex < m_items.GetCount() )
            return m_items.GetPickedItem( aIndex );

        return NULL;
    }

    /**
     * Function IsDragging
     * returns true if the current block command is a drag operation.
     */
    bool IsDragging() const
    {
        return m_command == BLOCK_DRAG || m_command == BLOCK_DRAG_ITEM;
    }

    /**
     * Function IsIdle
     * returns true if there is currently no block operation in progress.
     */
    inline bool IsIdle() const { return m_command == BLOCK_IDLE; }

    /**
     * Function Clear
     * clears the block selector by setting the command to idle, the state to no block,
     * and clears the selected item list.
     */
    void Clear();
};


/**
 * Function AbortBlockCurrentCommand
 * cancels the current block operation.
 */
void AbortBlockCurrentCommand( EDA_DRAW_PANEL* aPanel, wxDC* aDC );


/**
 * Function DrawAndSizingBlockOutlines
 * redraws the outlines of the block which shows the search area for block commands.
 *
 * The first point of the rectangle showing the area is initialized by InitBlockLocateDatas().
 * The other point of the rectangle is the mouse cursor position.
 */
void DrawAndSizingBlockOutlines( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                                 bool aErase );


#endif /* __INCLUDE__BLOCK_COMMANDE_H__ */
