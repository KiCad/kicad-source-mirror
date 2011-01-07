/**
 * This file is part of the common library.
 * @file  block_commande.h
 * @see   common.h
 */

#ifndef __INCLUDE__BLOCK_COMMANDE_H__
#define __INCLUDE__BLOCK_COMMANDE_H__ 1


#include "base_struct.h"
#include "class_undoredo_container.h"


// Forward declarations:


/**************************/
/*  class BLOCK_SELECTOR */
/**************************/

/**
 *  class BLOCK_SELECTOR is used to handle block selection and commands
 */

/* Block state codes. */
typedef enum {
    STATE_NO_BLOCK,
    STATE_BLOCK_INIT,
    STATE_BLOCK_END,
    STATE_BLOCK_MOVE,
    STATE_BLOCK_STOP
} BlockState;


/* Block command codes. */
typedef enum {
    BLOCK_IDLE,
    BLOCK_MOVE,
    BLOCK_COPY,
    BLOCK_SAVE,
    BLOCK_DELETE,
    BLOCK_PASTE,
    BLOCK_DRAG,
    BLOCK_ROTATE,
    BLOCK_FLIP,
    BLOCK_ZOOM,
    BLOCK_ABORT,
    BLOCK_PRESELECT_MOVE,
    BLOCK_SELECT_ITEMS_ONLY,
    BLOCK_MIRROR_X,
    BLOCK_MIRROR_Y
} CmdBlockType;


class BLOCK_SELECTOR : public EDA_ITEM, public EDA_Rect
{
public:
    BlockState        m_State;                    /* State (enum BlockState)
                                                   * of the block */
    CmdBlockType      m_Command;                  /* Type (enum CmdBlockType)
                                                   * operation */
    PICKED_ITEMS_LIST m_ItemsSelection;           /* list of items selected
                                                   * in this block */
    int m_Color;                                  /* Block Color (for
                                                   * drawings) */
    wxPoint           m_MoveVector;               /* Move distance in move,
                                                   * drag, copy ... command */
    wxPoint           m_BlockLastCursorPosition;  /* Last Mouse position in
                                                   * block command
                                                   *  = last cursor position in
                                                   * move commands
                                                   *  = 0,0 in block paste */

public:
    BLOCK_SELECTOR();
    ~BLOCK_SELECTOR();

    /**
     * Function InitData
     * sets the initial values of a BLOCK_SELECTOR, before starting a block
     * command
     */
    void InitData( WinEDA_DrawPanel* Panel, const wxPoint& startpos );

    /**
     * Function SetMessageBlock
     * Displays the type of block command in the status bar of the window
     */
    void SetMessageBlock( WinEDA_DrawFrame* frame );

    void Draw( WinEDA_DrawPanel* aPanel,
               wxDC* aDC, const wxPoint& aOffset,
               int aDrawMode,
               int aColor );

    /**
     * Function PushItem
     * adds aItem to the list of items
     * @param aItem = an ITEM_PICKER to add to the list
     */
    void PushItem( ITEM_PICKER& aItem );

    /**
     * Function ClearListAndDeleteItems
     * deletes only the list of EDA_ITEM * pointers, AND the data printed
     * by m_Item
     */
    void ClearListAndDeleteItems();

    void ClearItemsList();

    unsigned        GetCount()
    {
        return m_ItemsSelection.GetCount();
    }

    /**
     * Function SetLastCursorPosition
     * sets m_BlockLastCursorPosition
     * @param aPosition = new position
     **/
    void SetLastCursorPosition( wxPoint aPosition )
    {
        m_BlockLastCursorPosition = aPosition;
    }

    /**
     * Function IsDragging
     * returns true if the current block command is a drag operation.
     */
    bool IsDragging() const { return m_Command == BLOCK_DRAG; }
};


/* Cancel Current block operation.
 */
void AbortBlockCurrentCommand( WinEDA_DrawPanel* Panel, wxDC* DC );


/* Redraw the outlines of the block which shows the search area for block
 * commands
 *  The first point of the rectangle showing the area is initialized
 *  by InitBlockLocateDatas().
 *  The other point of the rectangle is the mouse cursor
 */
void DrawAndSizingBlockOutlines( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );


#endif /* __INCLUDE__BLOCK_COMMANDE_H__ */
