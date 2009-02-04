/*****************************************************************************/
/*  sch_item_struct.h :  Basic classes for most eeschema items descriptions  */
/*****************************************************************************/

#ifndef SCH_ITEM_STRUCT_H
#define SCH_ITEM_STRUCT_H


class WinEDA_SchematicFrame;


/**
 * Class SCH_ITEM
 * is a base class for any item which can be embedded within the SCHEMATIC
 * container class, and therefore instances of derived classes should only be
 * found in EESCHEMA or other programs that use class SCHEMATIC and its contents.
 * The corresponding class in PCBNEW is BOARD_ITEM.
 */
class SCH_ITEM : public EDA_BaseStruct
{
protected:
    int            m_Layer;


public:
    SCH_ITEM( EDA_BaseStruct* aParent,  KICAD_T aType );

    ~SCH_ITEM();

    virtual wxString GetClass() const
    {
        return wxT( "SCH_ITEM" );
    }

    SCH_ITEM* Next() { return (SCH_ITEM*) Pnext; }

    /**
     * Function GetLayer
     * returns the layer this item is on.
     */
    int GetLayer() const { return m_Layer; }

    /**
     * Function SetLayer
     * sets the layer this item is on.
     * @param aLayer The layer number.
     */
    void  SetLayer( int aLayer )  { m_Layer = aLayer; }

    /**
     * Function Draw
     */
    virtual void    Draw( WinEDA_DrawPanel* panel,
                          wxDC*             DC,
                          const wxPoint&    offset,
                          int               draw_mode,
                          int               Color = -1 ) = 0;


    /* fonction de placement */
    virtual void    Place( WinEDA_SchematicFrame* frame, wxDC* DC );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.sch" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    virtual bool    Save( FILE* aFile ) const = 0;
};


/**
 * Class DrawPickedStruct
 * holds structures picked by pick events (like block selection).
 * This class has only one useful member: .m_PickedStruct, used as a link.
 * It is used to create a linked list of selected items (in block selection).
 * Each DrawPickedStruct item has is member: .m_PickedStruct pointing the
 * real selected item.
 */
class DrawPickedStruct : public SCH_ITEM
{
public:
    SCH_ITEM * m_PickedStruct;

public:
    DrawPickedStruct( SCH_ITEM * pickedstruct = NULL );
    ~DrawPickedStruct();
    void Place( WinEDA_SchematicFrame* frame, wxDC* DC ) { };
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

#endif /* SCH_ITEM_STRUCT_H */
