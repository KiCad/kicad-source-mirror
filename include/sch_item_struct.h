/*****************************************************************************/
/*  sch_item_struct.h :  Basic classes for most eeschema items descriptions  */
/*****************************************************************************/

#ifndef SCH_ITEM_STRUCT_H
#define SCH_ITEM_STRUCT_H


class WinEDA_SchematicFrame;
class wxFindReplaceData;


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
    SCH_ITEM* Back() { return (SCH_ITEM*) Pback; }

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

    /** Function GetPenSize virtual pure
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize( ) = 0;

    /**
     * Function Draw
     */
    virtual void    Draw( WinEDA_DrawPanel* panel,
                          wxDC*             DC,
                          const wxPoint&    offset,
                          int               draw_mode,
                          int               Color = -1 ) = 0;


    /* Place function */
    virtual void    Place( WinEDA_SchematicFrame* frame, wxDC* DC );

    // Geometric transforms (used in block operations):
    /** virtual function Move
     * move item to a new position.
     * @param aMoveVector = the deplacement vector
     */
    virtual void Move(const wxPoint& aMoveVector) = 0;

    /** virtual function Mirror_Y
     * mirror item relative to an Y axis
     * @param aYaxis_position = the y axis position
     */
    virtual void Mirror_Y(int aYaxis_position) = 0;

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.sch"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    virtual bool    Save( FILE* aFile ) const = 0;

    /**
     * Compare schematic item against search string.
     *
     * The base class returns false since many of the objects derived from
     * SCH_ITEM do not have any text to search.
     *
     * @todo - This should probably be pushed down to EDA_BaseStruct so that
     *         searches can be done on all of the Kicad applications that use
     *         objects derived from EDA_BaseStruct.
     *
     * @param aSearchData - The search criteria.
     * @return True if this schematic text item matches the search criteria.
     */
    virtual bool Matches( wxFindReplaceData& aSearchData ) { return false; }

    /**
     * Compare schematic item against search string.
     *
     * @param aText - String test.
     * @param aSearchData - The criteria to search against.
     * @return True if this item matches the search criteria.
     */
    bool Matches( const wxString& aText, wxFindReplaceData& aSearchData );
};

#endif /* SCH_ITEM_STRUCT_H */
