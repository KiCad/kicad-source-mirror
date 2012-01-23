/**
 * @file  class_board_connected_item.h
 * @brief Class BOARD_CONNECTED_ITEM.
 */

#ifndef BOARD_CONNECTED_ITEM_H
#define BOARD_CONNECTED_ITEM_H


#include <class_board_item.h>

class NETCLASS;
class TRACK;
class D_PAD;

/**
 * Class BOARD_CONNECTED_ITEM
 * is a base class derived from BOARD_ITEM for items that can be connected
 * and have a net, a netname, a clearance ...
 * mainly: tracks, pads and zones
 * Handle connection info
 */
class BOARD_CONNECTED_ITEM : public BOARD_ITEM
{
    friend class CONNECTIONS;

public:
    // These 2 members are used for temporary storage during connections calculations:
    std::vector<TRACK*> m_TracksConnected;      // list of other tracks connected to me
    std::vector<D_PAD*> m_PadsConnected;        // list of other pads connected to me

private:
    int         m_NetCode;      // Net number

    int         m_Subnet;       /* In rastnest routines : for the current net, block number
                                 * (number common to the current connected items found)
                                 */

    int         m_ZoneSubnet;   // used in rastnest computations : for the current net,
                                // handle cluster number in zone connection

public:
    BOARD_CONNECTED_ITEM( BOARD_ITEM* aParent, KICAD_T idtype );

    BOARD_CONNECTED_ITEM( const BOARD_CONNECTED_ITEM& aItem );

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
    wxString GetNetClassName() const;
};


/**
 * Class BOARD_ITEM_LIST
 * is a container for a list of BOARD_ITEMs.
 */
class BOARD_ITEM_LIST : public BOARD_ITEM
{
    typedef boost::ptr_vector<BOARD_ITEM>   ITEM_ARRAY;

    ITEM_ARRAY   myItems;

    BOARD_ITEM_LIST( const BOARD_ITEM_LIST& other ) :
        BOARD_ITEM( NULL, PCB_ITEM_LIST_T )
    {
        // copy constructor is not supported, is private to cause compiler error
    }

public:

    BOARD_ITEM_LIST( BOARD_ITEM* aParent = NULL ) :
        BOARD_ITEM( aParent, PCB_ITEM_LIST_T )
    {}

    //-----< satisfy some virtual functions >------------------------------
    const wxPoint GetPosition()
    {
        return wxPoint(0, 0);   // dummy
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

    void Append( BOARD_ITEM* aItem )
    {
        myItems.push_back( aItem );
    }

    BOARD_ITEM* Replace( int aIndex, BOARD_ITEM* aItem )
    {
        ITEM_ARRAY::auto_type ret = myItems.replace( aIndex, aItem );
        return ret.release();
    }

    BOARD_ITEM* Remove( int aIndex )
    {
        ITEM_ARRAY::auto_type ret = myItems.release( myItems.begin()+aIndex );
        return ret.release();
    }

    void Insert( int aIndex, BOARD_ITEM* aItem )
    {
        myItems.insert( myItems.begin()+aIndex, aItem );
    }

    BOARD_ITEM* At( int aIndex ) const
    {
        // we have varying sized objects and are using polymorphism, so we
        // must return a pointer not a reference.
        return (BOARD_ITEM*) &myItems[aIndex];
    }

    BOARD_ITEM* operator[]( int aIndex ) const
    {
        return At( aIndex );
    }

    void Delete( int aIndex )
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


#endif // BOARD_CONNECTED_ITEM_H
