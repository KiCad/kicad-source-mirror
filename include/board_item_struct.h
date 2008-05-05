/*********************************************************************/
/*  board_item_struct.h :  Basic classes for BOARD_ITEM descriptions */
/*********************************************************************/

#ifndef BOARD_ITEM_STRUCT_H
#define BOARD_ITEM_STRUCT_H


/* Forme des segments (pistes, contours ..) ( parametre .shape ) */
enum Track_Shapes {
    S_SEGMENT = 0,      /* segment rectiligne */
    S_RECT,             /* segment forme rect (i.e. bouts non arrondis) */
    S_ARC,              /* segment en arc de cercle (bouts arrondis)*/
    S_CIRCLE,           /* segment en cercle (anneau)*/
    S_ARC_RECT,         /* segment en arc de cercle (bouts droits) (GERBER)*/
    S_SPOT_OVALE,       /* spot ovale (for GERBER)*/
    S_SPOT_CIRCLE,      /* spot rond (for GERBER)*/
    S_SPOT_RECT,        /* spot rect (for GERBER)*/
    S_POLYGON           /* polygon shape */
};


/**
 * Class BOARD_ITEM
 * is a base class for any item which can be embedded within the BOARD
 * container class, and therefore instances of derived classes should only be
 * found in PCBNEW or other programs that use class BOARD and its contents.
 * The corresponding class in EESCHEMA is SCH_ITEM.
 */
class BOARD_ITEM : public EDA_BaseStruct
{
protected:
    int m_Layer;

public:

    BOARD_ITEM( BOARD_ITEM* StructFather, KICAD_T idtype ) :
        EDA_BaseStruct( StructFather, idtype )
        , m_Layer( 0 )
    {
    }


    BOARD_ITEM( const BOARD_ITEM& src ) :
        EDA_BaseStruct( src.m_Parent, src.Type() )
        , m_Layer( src.m_Layer )
    {
    }


    /**
     * A value of wxPoint(0,0) which can be passed to the Draw() functions.
     */
    static wxPoint  ZeroOffset;

    BOARD_ITEM* Next() const { return (BOARD_ITEM*) Pnext; }
    BOARD_ITEM* Back() const { return (BOARD_ITEM*) Pback; }
    BOARD_ITEM* GetParent() const { return (BOARD_ITEM*) m_Parent; }

    /**
     * Function GetPosition
     * returns the position of this object.
     * @return wxPoint& - The position of this object, non-const so it
     *          can be changed
     */
    virtual wxPoint& GetPosition() = 0;

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
     * BOARD_ITEMs have their own color information.
     */
    virtual void Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                      int aDrawMode, const wxPoint& offset = ZeroOffset ) = 0;


    /**
     * Function IsOnLayer
     * tests to see if this object is on the given layer.  Is virtual so
     * objects like D_PAD, which reside on multiple layers can do their own
     * form of testing.
     * @param aLayer The layer to test for.
     * @return bool - true if on given layer, else false.
     */
    virtual bool IsOnLayer( int aLayer ) const
    {
        return m_Layer == aLayer;
    }


    /**
     * Function IsLocked
     * @return bool - true if the object is locked, else false
     */
    virtual bool IsLocked() const
    {
        return false;   // only MODULEs can be locked at this time.
    }


    /**
     * Function UnLink
     * detaches this object from its owner.
     */
    virtual void UnLink() = 0;


    /**
     * Function DeleteStructure
     * deletes this object after UnLink()ing it from its owner.
     */
    void DeleteStructure()
    {
        UnLink();
        delete this;
    }


    /**
     * Function MenuText
     * returns the text to use in any menu type UI control which must uniquely
     * identify this item.
     * @param aBoard The PCB in which this item resides, needed for Net lookup.
     * @return wxString
     * @todo: maybe: make this virtual and split into each derived class
     */
    wxString        MenuText( const BOARD* aBoard ) const;


    /**
     * Function MenuIcon
     * @return const char** - The XPM to use in any UI control which can help
     *  identify this item.
     * @todo: make this virtual and split into each derived class
     */
    const char**    MenuIcon() const;


    /**
     * Function ShowShape
     * converts the enum Track_Shapes integer value to a wxString.
     */
    static wxString ShowShape( Track_Shapes aShape );


    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    virtual bool    Save( FILE* aFile ) const = 0;
};

#endif /* BOARD_ITEM_STRUCT_H */

