/********************************************/
/* Definitions for the EESchema program:	*/
/********************************************/

#ifndef CLASS_SCREEN_H
#define CLASS_SCREEN_H

#include "macros.h"
#include "base_struct.h"
#include "class_base_screen.h"

/* Max number of sheets in a hierarchy project: */
#define NB_MAX_SHEET 500


class SCH_SCREEN : public BASE_SCREEN
{
public:
    int m_RefCount;                             /*how many sheets reference this screen?
                                                  * delete when it goes to zero. */
    SCH_SCREEN( KICAD_T aType = SCREEN_STRUCT_TYPE );
    ~SCH_SCREEN();

    /**
     * Function GetCurItem
     * returns the currently selected SCH_ITEM, overriding BASE_SCREEN::GetCurItem().
     * @return SCH_ITEM* - the one selected, or NULL.
     */
    SCH_ITEM* GetCurItem() const {  return (SCH_ITEM*) BASE_SCREEN::GetCurItem(); }

    /**
     * Function SetCurItem
     * sets the currently selected object, m_CurrentItem.
     * @param current Any object derived from SCH_ITEM
     */
    void SetCurItem( SCH_ITEM* aItem )
    {
        BASE_SCREEN::SetCurItem( (BASE_SCREEN*) aItem );
    }


    virtual wxString GetClass() const
    {
        return wxT( "SCH_SCREEN" );
    }

    void         FreeDrawList();    // Free EESchema drawing list (does not delete the sub hierarchies)

    void         Place( WinEDA_SchematicFrame* frame, wxDC* DC ) { };

    void         RemoveFromDrawList( SCH_ITEM* DrawStruct );    /* remove DrawStruct from EEDrawList. */
    bool         CheckIfOnDrawList( SCH_ITEM* st );
    void         AddToDrawList( SCH_ITEM* DrawStruct );

    bool         SchematicCleanUp( wxDC* DC = NULL );
    SCH_ITEM*    ExtractWires( bool CreateCopy );

    /* full undo redo management : */
    virtual void ClearUndoRedoList();
    virtual void PushCommandToUndoList( PICKED_ITEMS_LIST* aItem );
    virtual void PushCommandToRedoList( PICKED_ITEMS_LIST* aItem );

    /** Function ClearUndoORRedoList
     * free the undo or redo list from List element
     *  Wrappers are deleted.
     *  datas pointed by wrappers are deleted if not flagged IS_NEW
     *  because they are copy of used data or they are not in use (DELETED)
     * @param aList = the UNDO_REDO_CONTAINER to clear
     * @param aItemCount = the count of items to remove. < 0 for all items
     * items are removed from the beginning of the list.
     * So this function can be called to remove old commands
     */
    void         ClearUndoORRedoList( UNDO_REDO_CONTAINER& aList, int aItemCount = -1 );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool         Save( FILE* aFile ) const;
};


/********************************************************/
/* Class to handle the list of *screens* in a hierarchy */
/********************************************************/

// screens are unique, and correspond to .sch files.
WX_DEFINE_ARRAY( SCH_SCREEN *, ScreenGrowArray );

class EDA_ScreenList
{
private:
    ScreenGrowArray m_List;
    unsigned int    m_Index;

public:
    EDA_ScreenList();
    ~EDA_ScreenList() { }
    int GetCount() { return m_List.GetCount(); }
    SCH_SCREEN* GetFirst();
    SCH_SCREEN* GetNext();
    SCH_SCREEN* GetScreen( unsigned int index );

private:
    void        AddScreenToList( SCH_SCREEN* testscreen );
    void        BuildScreenList( EDA_BaseStruct* sheet );
};

#endif /* CLASS_SCREEN_H */
