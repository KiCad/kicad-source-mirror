/**********************************************************************************************/
/*  board_item_struct.h :  Classes BOARD_ITEM and BOARD_CONNECTED_ITEM                        */
/**********************************************************************************************/

#ifndef BOARD_ITEM_STRUCT_H
#define BOARD_ITEM_STRUCT_H


#include <boost/ptr_container/ptr_vector.hpp>


/* Shapes for segments (graphic segments and tracks) ( .m_Shape member ) */
enum Track_Shapes {
    S_SEGMENT = 0,      /* usual segment : line with rounded ends */
    S_RECT,             /* segment with non rounded ends */
    S_ARC,              /* Arcs (with rounded ends)*/
    S_CIRCLE,           /* ring*/
    S_POLYGON,          /* polygonal shape (not yet used for tracks, but could be in microwave apps) */
    S_CURVE,            /* Bezier Curve*/
    S_LAST              /* last value for this list */
};


/**
 * Class BOARD_ITEM
 * is a base class for any item which can be embedded within the BOARD
 * container class, and therefore instances of derived classes should only be
 * found in PCBNEW or other programs that use class BOARD and its contents.
 * The corresponding class in EESCHEMA is SCH_ITEM.
 */
class BOARD_ITEM : public EDA_ITEM
{
    // These are made private here so they may not be used.
    // Instead everything derived from BOARD_ITEM is handled via DLIST<>'s
    // use of DHEAD's member functions.
    void SetNext( EDA_ITEM* aNext )       { Pnext = aNext; }
    void SetBack( EDA_ITEM* aBack )       { Pback = aBack; }


protected:
    int m_Layer;

public:

    BOARD_ITEM( BOARD_ITEM* aParent, KICAD_T idtype ) :
        EDA_ITEM( aParent, idtype )
        , m_Layer( 0 )
    {
    }


    BOARD_ITEM( const BOARD_ITEM& src ) :
        EDA_ITEM( src.m_Parent, src.Type() )
        , m_Layer( src.m_Layer )
    {
        m_Flags = src.m_Flags;
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
     * is virtual because some items (in fact: class DIMENSION)
     * have a slightly different initialisation
     */
    virtual void  SetLayer( int aLayer )  { m_Layer = aLayer; }


    /**
     * Function Draw
     * BOARD_ITEMs have their own color information.
     */
    virtual void Draw( EDA_DRAW_PANEL* panel, wxDC* DC,
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
     * Function IsTrack
     * tests to see if this object is a track or via (or microvia).
     * form of testing.
     * @return bool - true if a track or via, else false.
     */
    bool IsTrack( ) const
    {
        return (Type() == TYPE_TRACK) || (Type() == TYPE_VIA);
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
     * detaches this object from its owner.  This base class implementation
     * should work for all derived classes which are held in a DLIST<>.
     */
    virtual void UnLink();


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

    // Some geometric transforms, that must be rewrittem for derived classes
    /**
     * Function Move
     * move this object.
     * @param aMoveVector - the move vector for this object.
     */
    virtual void Move(const wxPoint& aMoveVector)
    {
        wxMessageBox(wxT("virtual BOARD_ITEM::Move used, should not occur"), GetClass());
    }

    /**
     * Function Rotate
     * Rotate this object.
     * @param aRotCentre - the rotation point.
     * @param aAngle - the rotation angle in 0.1 degree.
     */
    virtual void Rotate(const wxPoint& aRotCentre, int aAngle)
    {
        wxMessageBox(wxT("virtual BOARD_ITEM::Rotate used, should not occur"), GetClass());
    }

    /**
     * Function Flip
     * Flip this object, i.e. change the board side for this object
     * @param aCentre - the rotation point.
     */
    virtual void Flip(const wxPoint& aCentre )
    {
        wxMessageBox(wxT("virtual BOARD_ITEM::Flip used, should not occur"), GetClass());
    }


    /**
     * Function GetBoard
     * returns the BOARD in which this BOARD_ITEM resides, or NULL if none.
     */
    virtual BOARD* GetBoard() const;

    /**
     * Function GetLayerName
     * returns the name of the PCB layer on which the item resides.
     *
     * @return wxString containing the layer name associated with this item.
     */
    wxString GetLayerName() const;
};


class NETCLASS;

/**
 * Class BOARD_CONNECTED_ITEM
 * This is a base class derived from BOARD_ITEM for items that can be connected
 * and have a net, a netname, a clearance ...
 * mainly: tracks, pads and zones
 * Handle connection info
 */
class BOARD_CONNECTED_ITEM : public BOARD_ITEM
{
protected:
    int         m_NetCode;          // Net number

    int         m_Subnet;           /* In rastnest routines : for the current net,
                                     *  block number (number common to the current connected items found)
                                     */

    int         m_ZoneSubnet;   	   // variable used in rastnest computations : for the current net,
                                    // handle block number in zone connection

public:
    BOARD_CONNECTED_ITEM( BOARD_ITEM* aParent, KICAD_T idtype );
    BOARD_CONNECTED_ITEM( const BOARD_CONNECTED_ITEM& src );

    /**
     * Function GetNet
     * @return int - the net code.
     */
    int GetNet() const;
    virtual void SetNet( int aNetCode );

    /**
     * Function GetSubNet
     * @return int - the sub net code.
     */
    int GetSubNet() const;
    void SetSubNet( int aSubNetCode );

    /**
     * Function GetZoneSubNet
     * @return int - the sub net code in zone connections.
     */
    int GetZoneSubNet() const;
    void SetZoneSubNet( int aSubNetCode );


    /**
     * Function GetClearance
     * returns the clearance in 1/10000 inches.  If \a aItem is not NULL then the
     * returned clearance is the greater of this object's NETCLASS clearance and
     * aItem's NETCLASS clearance.  If \a aItem is NULL, then this objects clearance
     * is returned.
     * @param aItem is another BOARD_CONNECTED_ITEM or NULL
     * @return int - the clearance in 1/10000 inches.
     */
    virtual int GetClearance( BOARD_CONNECTED_ITEM* aItem = NULL ) const;

     /**
      * Function GetNetClass
      * returns the NETCLASS for this item.
      */
     NETCLASS* GetNetClass() const;

    /**
     * Function GetNetClassName
     * @return the Net Class name of this item
     */
    wxString GetNetClassName( ) const;
};


/*
 * class BOARD_ITEM_LIST
 * Handles a collection of BOARD_ITEM elements
*/
class BOARD_ITEM_LIST : public BOARD_ITEM
{
    typedef boost::ptr_vector<BOARD_ITEM>   ITEM_ARRAY;

    ITEM_ARRAY   myItems;

    BOARD_ITEM_LIST( const BOARD_ITEM_LIST& other ) :
        BOARD_ITEM( NULL, TYPE_BOARD_ITEM_LIST )
    {
        // copy constructor is not supported, is private to cause compiler error
    }

public:

    BOARD_ITEM_LIST( BOARD_ITEM* aParent = NULL ) :
        BOARD_ITEM( aParent, TYPE_BOARD_ITEM_LIST )
    {}

    //-----< satisfy some virtual functions >------------------------------
    wxPoint& GetPosition()
    {
        static wxPoint dummy;
        return dummy;
    }

    void Draw( EDA_DRAW_PANEL* DrawPanel, wxDC* DC,
               int aDrawMode, const wxPoint& offset = ZeroOffset )
    {
    }

    void UnLink()
    {
        /* if it were needed:
        DHEAD* list = GetList();

        wxASSERT( list );

        list->remove( this );
        */
    }

    bool Save( FILE* aFile ) const
    {
        return true;
    }

    //-----</ satisfy some virtual functions >-----------------------------


    /**
     * Function GetCount
     * returns the number of BOARD_ITEMs.
     */
    int GetCount() const
    {
        return myItems.size();
    }

    void    Append( BOARD_ITEM* aItem )
    {
        myItems.push_back( aItem );
    }

    BOARD_ITEM*   Replace( int aIndex, BOARD_ITEM* aItem )
    {
        ITEM_ARRAY::auto_type ret = myItems.replace( aIndex, aItem );
        return ret.release();
    }

    BOARD_ITEM*   Remove( int aIndex )
    {
        ITEM_ARRAY::auto_type ret = myItems.release( myItems.begin()+aIndex );
        return ret.release();
    }

    void    Insert( int aIndex, BOARD_ITEM* aItem )
    {
        myItems.insert( myItems.begin()+aIndex, aItem );
    }

    BOARD_ITEM*   At( int aIndex ) const
    {
        // we have varying sized objects and are using polymorphism, so we
        // must return a pointer not a reference.
        return (BOARD_ITEM*) &myItems[aIndex];
    }

    BOARD_ITEM* operator[]( int aIndex ) const
    {
        return At( aIndex );
    }

    void    Delete( int aIndex )
    {
        myItems.erase( myItems.begin()+aIndex );
    }

    void PushBack( BOARD_ITEM* aItem )
    {
        Append( aItem );
    }

    BOARD_ITEM* PopBack()
    {
        if( GetCount() )
            return Remove( GetCount()-1 );
        return NULL;
    }
};


#endif /* BOARD_ITEM_STRUCT_H */

