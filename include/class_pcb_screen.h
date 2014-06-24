/**
 * @file class_pcb_screen.h
 */

#ifndef CLASS_PCB_SCREEN_H_
#define CLASS_PCB_SCREEN_H_


#include <class_base_screen.h>
#include <class_board_item.h>


class UNDO_REDO_CONTAINER;


/* Handle info to display a board */
class PCB_SCREEN : public BASE_SCREEN
{
public:
    LAYER_ID m_Active_Layer;
    LAYER_ID m_Route_Layer_TOP;
    LAYER_ID m_Route_Layer_BOTTOM;

public:

    /**
     * Constructor
     * @param aPageSizeIU is the size of the initial paper page in internal units.
     */
    PCB_SCREEN( const wxSize& aPageSizeIU );

    ~PCB_SCREEN();

    PCB_SCREEN* Next() const { return static_cast<PCB_SCREEN*>( Pnext ); }

    void        SetNextZoom();
    void        SetPreviousZoom();
    void        SetLastZoom();

    virtual int MilsToIuScalar();

    /**
     * Function GetCurItem
     * returns the currently selected BOARD_ITEM, overriding
     * BASE_SCREEN::GetCurItem().
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
    void SetCurItem( BOARD_ITEM* aItem ) { BASE_SCREEN::SetCurItem( (EDA_ITEM*)aItem ); }

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

#endif  // CLASS_PCB_SCREEN_H_
