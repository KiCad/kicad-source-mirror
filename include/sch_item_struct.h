/*****************************************************************************/
/*  sch_item_struct.h :  Basic classes for most eeschema items descriptions  */
/*****************************************************************************/

#ifndef SCH_ITEM_STRUCT_H
#define SCH_ITEM_STRUCT_H

#include <vector>
#include <class_base_screen.h>

using namespace std;


class SCH_ITEM;
class LINE_READER;
class SCH_EDIT_FRAME;
class wxFindReplaceData;


// Schematic item filter mask for hit test objects in schematic editor.
enum SCH_FILTER_T {
    COMPONENT_T         = 0x0001,
    WIRE_T              = 0X0002,
    BUS_T               = 0x0004,
    BUS_ENTRY_T         = 0x0008,
    JUNCTION_T          = 0x0010,
    DRAW_ITEM_T         = 0x0020,
    TEXT_T              = 0x0040,
    LABEL_T             = 0x0080,
    SHEET_T             = 0x0100,
    MARKER_T            = 0x0200,
    NO_CONNECT_T        = 0x0400,
    SHEET_LABEL_T       = 0x0800,
    FIELD_T             = 0x1000,
    EXCLUDE_ENDPOINTS_T = 0x2000,
    ENDPOINTS_ONLY_T    = 0x4000,
    NO_FILTER_T         = 0xFFFF
};


enum DANGLING_END_T {
    UNKNOWN = 0,
    WIRE_START_END,
    WIRE_END_END,
    BUS_START_END,
    BUS_END_END,
    JUNCTION_END,
    PIN_END,
    LABEL_END,
    ENTRY_END,
    SHEET_LABEL_END
};

// A helper class to store a list of items that can be connected to something:
class DANGLING_END_ITEM
{
public:
    const void*    m_Item;         // a pointer to the parent
    wxPoint        m_Pos;          // the position of the connecting point
    DANGLING_END_T m_Type;         // type of parent

    DANGLING_END_ITEM( DANGLING_END_T type, const void* aItem )
    {
        m_Item = aItem;
        m_Type = type;
    }
};


/**
 * Class SCH_ITEM
 * is a base class for any item which can be embedded within the SCHEMATIC
 * container class, and therefore instances of derived classes should only be
 * found in EESCHEMA or other programs that use class SCHEMATIC and its contents.
 * The corresponding class in PCBNEW is BOARD_ITEM.
 */
class SCH_ITEM : public EDA_ITEM
{
protected:
    int            m_Layer;
    EDA_ITEMS      m_connections;   ///< List of items connected to this item.

public:
    SCH_ITEM( EDA_ITEM* aParent, KICAD_T aType );

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
    void SetLayer( int aLayer )  { m_Layer = aLayer; }

    /**
     * Function GetPenSize virtual pure
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize() const { return 0; }

    /**
     * Function Draw
     */
    virtual void Draw( WinEDA_DrawPanel* aPanel,
                       wxDC*             aDC,
                       const wxPoint&    aOffset,
                       int               aDrawMode,
                       int               aColor = -1 ) = 0;


    /* Place function */
    virtual void Place( SCH_EDIT_FRAME* aFrame, wxDC* aDC );

    // Geometric transforms (used in block operations):
    /** virtual function Move
     * move item to a new position.
     * @param aMoveVector = the deplacement vector
     */
    virtual void Move( const wxPoint& aMoveVector ) = 0;

    /** virtual function Mirror_Y
     * mirror item relative to an Y axis
     * @param aYaxis_position = the y axis position
     */
    virtual void Mirror_Y( int aYaxis_position ) = 0;
    virtual void Mirror_X( int aXaxis_position ) = 0;
    virtual void Rotate( wxPoint rotationPoint ) = 0;


    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.sch"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    virtual bool Save( FILE* aFile ) const = 0;

    /**
     * Load schematic item from \a aLine in a .sch file.
     *
     * @param aLine - Essentially this is file to read the object from.
     * @param aErrorMsg - Description of the error if an error occurs while loading the object.
     * @return True if the object loaded successfully.
     */
    virtual bool Load( LINE_READER& aLine, wxString& aErrorMsg ) { return false; }

    /**
     * Compare schematic item against search string.
     *
     * The base class returns false since many of the objects derived from
     * SCH_ITEM do not have any text to search.
     *
     * @todo - This should probably be pushed down to EDA_ITEM so that
     *         searches can be done on all of the Kicad applications that use
     *         objects derived from EDA_ITEM.
     *
     * @param aSearchData - The search criteria.
     * @param aAuxData - a pointer on auxiliary data, if needed (NULL if not used).
     *        When searching string in REFERENCE field we must know the sheet path
     *          This param is used in such cases
     * @param aFindLocation - a wxPoint where to put the location of matched item. can be NULL.
     * @return True if this schematic text item matches the search criteria.
     */
    virtual bool Matches( wxFindReplaceData& aSearchData, void* aAuxData, wxPoint* aFindLocation )
        { return false; }

    /**
     * Compare schematic item against search string.
     *
     * @param aText - String test.
     * @param aSearchData - The criteria to search against.
     * @return True if this item matches the search criteria.
     */
    bool Matches( const wxString& aText, wxFindReplaceData& aSearchData );

    /**
     * Add schematic item end points to \a aItemList if the item has endpoints.
     *
     * The default version doesn't do anything since many of the schematic object cannot
     * be tested for dangling ends.  If you add a new schematic item that can have a
     * dangling end ( no connect ), override this method to provide the correct end
     * points.
     *
     * @param aItemList - List of DANGLING_END_ITEMS to add to.
     */
    virtual void GetEndPoints( vector< DANGLING_END_ITEM >& aItemList ) {}

    /**
     * Test the schematic item to \a aItemList to check if it's dangling state has changed.
     *
     * Note that the return value only true when the state of the test has changed.  Use
     * the IsDangling() method to get the current dangling state of the item.  Some of
     * the schematic objects cannot be tested for a dangling state, the default method
     * always returns false.  Only override the method if the item can be tested for a
     * dangling state.
     *
     * @param aItemList - List of items to test item against.
     * @return True if the dangling state has changed from it's current setting.
     */
    virtual bool IsDanglingStateChanged( vector< DANGLING_END_ITEM >& aItemList ) { return false; }

    virtual bool IsDangling() const { return false; }

    /**
     * Check if the selection state of an item  inside \a aRect has changed.
     *
     * The is used by the block selection code to verify if an item is selected or not.
     * True is be return anytime the select state changes.  If you need to know the
     * the current selection state, use the IsSelected() method.
     *
     * @param aRect - Rectange to test against.
     */
    virtual bool IsSelectStateChanged( const wxRect& aRect ) { return false; }

    /**
     * Get a list of connection points for this item.
     *
     * Not all schematic items have connection points so the default method does nothing.
     *
     * @param aPoints - List of connection points to add to.
     */
    virtual void GetConnectionPoints( vector< wxPoint >& aPoints ) const { }

    /**
     * Clear all of the connection items from the list.
     *
     * The vector release method is used to prevent the item pointers from being deleted.
     * Do not use the vector erase method on the connection list.
     */
    void ClearConnections() { m_connections.release(); }

    /**
     * Function IsConnected().
     * Test \a aPosition to see if it is connected to this schematic object.
     *
     * @param aPosition - Position to test for connection.
     * @return True if connection to \a aPosition exists.
     */
    bool IsConnected( const wxPoint& aPoint ) const;

    /**
     * Function HitTest().
     * Test if \a aPoint is contained within the bounding box or on an item.
     *
     * @param aPoint - Point to test.
     * @param aAccuracy - Increase the item bounding box by this amount.
     * @param aFilter - Mask to provide more granular hit testing.  See enum SCH_FILTER_T.
     * @return True if \aPoint is within the item and meets the filter criteria.
     */
    bool HitTest( const wxPoint& aPoint, int aAccuracy = 0,
                  SCH_FILTER_T aFilter = NO_FILTER_T ) const
    {
        return DoHitTest( aPoint, aAccuracy, aFilter );
    }

    /**
     * Function HitTest().
     * Test if \a aRect intersects or is contained within the bounding box of an item.
     *
     * @param aRect - Rectangle to test.
     * @param aContained - Set to true to test for containment instead of an intersection.
     * @param aAccuracy - Increase the item bounding box by this amount.
     * @return True if \aRect contains or intersects the item bounding box.
     */
    bool HitTest( const EDA_Rect& aRect, bool aContained = false, int aAccuracy = 0 ) const
    {
        return DoHitTest( aRect, aContained, aAccuracy );
    }

    /**
     * @note - The DoXXX() functions below are used to enforce the interface while retaining
     *         the ability of change the implementation behavior of derived classes.  See
     *         Herb Sutters explanation of virtuality as to why you might want to do this at:
     *         http://www.gotw.ca/publications/mill18.htm.
     */
private:
    virtual bool DoHitTest( const wxPoint& aPoint, int aAccuracy, SCH_FILTER_T aFilter ) const
    {
        return false;
    }

    virtual bool DoHitTest( const EDA_Rect& aRect, bool aContained, int aAccuracy ) const
    {
        return false;
    }

    virtual bool DoIsConnected( const wxPoint& aPosition ) const { return false; }
};

#endif /* SCH_ITEM_STRUCT_H */
