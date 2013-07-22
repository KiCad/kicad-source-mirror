/**
 * @file class_pl_editor_layout.h
 */

#ifndef CLASS_PL_EDITOR_SCREEN_H_
#define CLASS_PL_EDITOR_SCREEN_H_


#include <base_units.h>
#include <class_base_screen.h>

class WORKSHEET_DATAITEM;


/* Handle info to display a board */
class PL_EDITOR_SCREEN : public BASE_SCREEN
{
public:
    /**
     * Constructor
     * @param aPageSizeIU is the size of the initial paper page in internal units.
     */
    PL_EDITOR_SCREEN( const wxSize& aPageSizeIU );

    ~PL_EDITOR_SCREEN();

    virtual int MilsToIuScalar();

    /**
     * Function ClearUndoORRedoList
     * virtual pure in BASE_SCREEN, so it must be defined here
     */

    void ClearUndoORRedoList( UNDO_REDO_CONTAINER& aList, int aItemCount = -1 );
    /**
     * Function GetCurItem
     * returns the currently selected WORKSHEET_DATAITEM, overriding
     * BASE_SCREEN::GetCurItem().
     * @return WORKSHEET_DATAITEM* - the one selected, or NULL.
     */

    WORKSHEET_DATAITEM* GetCurItem() const
    {
        return (WORKSHEET_DATAITEM*) BASE_SCREEN::GetCurItem();
    }

    /**
     * Function SetCurItem
     * sets the currently selected object, m_CurrentItem.
     * @param aItem Any object derived from WORKSHEET_DATAITEM
     */
    void SetCurItem( WORKSHEET_DATAITEM* aItem ) { BASE_SCREEN::SetCurItem( (EDA_ITEM*)aItem ); }
};


#endif  // CLASS_PL_EDITOR_SCREEN_H_
