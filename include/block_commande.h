/**
 * This file is part of the common libary.
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
typedef enum {
    /* definition de l'etat du block */
    STATE_NO_BLOCK,             /* Block non initialise */
    STATE_BLOCK_INIT,           /* Block initialise: 1er point defini */
    STATE_BLOCK_END,            /* Block initialise: 2eme point defini */
    STATE_BLOCK_MOVE,           /* Block en deplacement */
    STATE_BLOCK_STOP            /* Block fixe (fin de deplacement) */
} BlockState;

/* codes des differentes commandes sur block: */
typedef enum {
    BLOCK_IDLE,
    BLOCK_MOVE,
    BLOCK_COPY,
    BLOCK_SAVE,
    BLOCK_DELETE,
    BLOCK_PASTE,
    BLOCK_DRAG,
    BLOCK_ROTATE,
    BLOCK_INVERT,
    BLOCK_ZOOM,
    BLOCK_ABORT,
    BLOCK_PRESELECT_MOVE,
    BLOCK_SELECT_ITEMS_ONLY,
    BLOCK_MIRROR_X,
    BLOCK_MIRROR_Y
} CmdBlockType;


class BLOCK_SELECTOR : public EDA_BaseStruct, public EDA_Rect
{
public:
    BlockState        m_State;                  /* Stae (enum BlockState) of the block */
    CmdBlockType      m_Command;                /* Type (enum CmdBlockType) d'operation */
    PICKED_ITEMS_LIST m_ItemsSelection;         /* list of items selected in this block */
    int m_Color;                                /* Block Color (for drawings) */
    wxPoint           m_MoveVector;             /* Move distance in move, drag, copy ... command */
    wxPoint           m_BlockLastCursorPosition; /* Last Mouse position in block command
                                                  *  = last cursor position in move commands
                                                  *  = 0,0 in block paste */

public:
    BLOCK_SELECTOR();
    ~BLOCK_SELECTOR();

    /** function InitData
     *  Init the initial values of a BLOCK_SELECTOR, before starting a block command
     */
    void InitData( WinEDA_DrawPanel* Panel, const wxPoint& startpos );
    /** Function SetMessageBlock
     * Displays the type of block command in the status bar of the window
    */
    void        SetMessageBlock( WinEDA_DrawFrame* frame );

    void        Draw( WinEDA_DrawPanel*      aPanel,
                      wxDC* aDC, const wxPoint& aOffset,
                      int                    aDrawMode,
                      int                    aColor );

    /** Function PushItem
     * Add aItem to the list of items
     * @param aItem = an ITEM_PICKER to add to the list
     */
    void        PushItem( ITEM_PICKER& aItem );

    /** Function ClearListAndDeleteItems
     * delete only the list of EDA_BaseStruct * pointers, AND the data pinted by m_Item
     */
    void           ClearListAndDeleteItems();

    void        ClearItemsList();
    unsigned        GetCount()
    {
        return m_ItemsSelection.GetCount();
    }
};


/* Cancel Current block operation.
*/
void    AbortBlockCurrentCommand( WinEDA_DrawPanel* Panel, wxDC* DC );


/* Redraw the outlines of the block which shows the search area for block commands
 *  The first point of the rectangle showing the area is initialised
 *  by InitBlockLocateDatas().
 *  The other point of the rectangle is the mouse cursor
 */
void    DrawAndSizingBlockOutlines( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );


#endif /* __INCLUDE__BLOCK_COMMANDE_H__ */

