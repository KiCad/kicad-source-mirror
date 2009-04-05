/*****************************************************************************/
/*  sch_item_struct.h :  Basic classes for most eeschema items descriptions  */
/*****************************************************************************/

#ifndef __CLASS_DRAWPICKEDSTRUCT_H__
#define __CLASS_DRAWPICKEDSTRUCT_H__


#include "base_struct.h"


/**
 * Class DrawPickedStruct
 * holds structures picked by pick events (like block selection).
 * This class has only one useful member: .m_PickedStruct, used as a link.
 * It is used to create a linked list of selected items (in block selection).
 * Each DrawPickedStruct item has is member: .m_PickedStruct pointing the
 * real selected item.
 */
class DrawPickedStruct : public EDA_BaseStruct
{
public:
    EDA_BaseStruct * m_PickedStruct;

public:
    DrawPickedStruct( EDA_BaseStruct * pickedstruct = NULL );
    ~DrawPickedStruct();
    void Place( WinEDA_DrawFrame* frame, wxDC* DC ) { };
    void DeleteWrapperList();

    DrawPickedStruct* Next() { return (DrawPickedStruct*) Pnext; }

    EDA_Rect GetBoundingBox();

    /**
     * Function GetBoundingBoxUnion
     * returns the union of all the BoundingBox rectangles of all held items
     * in the picklist whose list head is this DrawPickedStruct.
     * @return EDA_Rect - The combined, composite, bounding box.
     */
    EDA_Rect GetBoundingBoxUnion();

    wxString GetClass() const { return wxT( "DrawPickedStruct" ); }

    /**
     * Function Draw
     * Do nothing, needed for SCH_ITEM compat.
    */
    void    Draw( WinEDA_DrawPanel* panel,
                  wxDC*             DC,
                  const wxPoint&    offset,
                  int               draw_mode,
                  int               Color = -1 )
    {
    }

    /**
     * Function Save
     * Do nothing, needed for SCH_ITEM compat.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool    Save( FILE* aFile ) const
    {
        return false;
    }

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os );
#endif
};

#endif /* __CLASS_DRAWPICKEDSTRUCT_H__ */
