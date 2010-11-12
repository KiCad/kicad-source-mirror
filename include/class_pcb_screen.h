/****************/
/*  pcbstruct.h */
/****************/

#ifndef __CLASSPCB_SCREEN_H__
#define __CLASSPCB_SCREEN_H__

/* Handle info to display a board */
class PCB_SCREEN : public BASE_SCREEN
{
public:
    int m_Active_Layer;
    int m_Route_Layer_TOP;
    int m_Route_Layer_BOTTOM;

public:
    PCB_SCREEN();
    ~PCB_SCREEN();

    PCB_SCREEN* Next() { return (PCB_SCREEN*) Pnext; }
    void        Init();
    void        SetNextZoom();
    void        SetPreviousZoom();
    void        SetLastZoom();

    virtual int GetInternalUnits( void );

    /**
     * Function GetCurItem
     * returns the currently selected BOARD_ITEM, overriding
     *BASE_SCREEN::GetCurItem().
     * @return BOARD_ITEM* - the one selected, or NULL.
     */
    BOARD_ITEM* GetCurItem() const
    {
        return (BOARD_ITEM*) BASE_SCREEN::GetCurItem();
    }

    /**
     * Function SetCurItem
     * sets the currently selected object, m_CurrentItem.
     * @param aItem Any object derived from BOARD_ITEM
     */
    void SetCurItem( BOARD_ITEM* aItem ) { BASE_SCREEN::SetCurItem( aItem ); }


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
    void ClearUndoORRedoList( UNDO_REDO_CONTAINER& aList, int aItemCount = -1 );
};


#endif /* __CLASSPCB_SCREEN_H__ */
