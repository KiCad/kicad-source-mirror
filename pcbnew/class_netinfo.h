/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file class_netinfo.h
 */

/*
 *  Classes to handle info on nets
 */

#ifndef CLASS_NETINFO_
#define CLASS_NETINFO_

#include <macros.h>
#include <gr_basic.h>
#include <class_netclass.h>
#include <class_board_item.h>



class wxDC;
class wxPoint;
class LINE_READER;
class EDA_DRAW_PANEL;
class EDA_DRAW_FRAME;
class D_PAD;
class BOARD;
class BOARD_ITEM;
class MSG_PANEL_ITEM;


/*****************************/
/* flags for a RATSNEST_ITEM */
/*****************************/
#define CH_VISIBLE          1        /* Visible */
#define CH_UNROUTABLE       2        /* Don't use autorouter. */
#define CH_ROUTE_REQ        4        /* Must be routed by the autorouter. */
#define CH_ACTIF            8        /* Not routed. */
#define LOCAL_RATSNEST_ITEM 0x8000   /* Line between two pads of a single module. */


/**
 * Class RATSNEST_ITEM
 * describes a ratsnest line: a straight line connecting 2 pads
 */
class RATSNEST_ITEM
{
private:
    int m_NetCode;      // netcode ( = 1.. n ,  0 is the value used for not connected items)

public:
    int    m_Status;    // State: see previous defines (CH_ ...)
    D_PAD* m_PadStart;  // pointer to the starting pad
    D_PAD* m_PadEnd;    // pointer to ending pad
    int    m_Length;    // length of the line (used in some calculations)

    RATSNEST_ITEM();

    /**
     * Function GetNet
     * @return int - the net code.
     */
    int GetNet() const
    {
        return m_NetCode;
    }

    void SetNet( int aNetCode )
    {
        m_NetCode = aNetCode;
    }

    bool IsVisible()
    {
        return (m_Status & CH_VISIBLE) != 0;
    }

    bool IsActive()
    {
        return (m_Status & CH_ACTIF) != 0;
    }

    bool IsLocal()
    {
        return (m_Status & LOCAL_RATSNEST_ITEM) != 0;
    }

    /**
     * Function Draw
     */
    void Draw( EDA_DRAW_PANEL* panel, wxDC* DC, GR_DRAWMODE aDrawMode,
               const wxPoint& offset );
};


DECL_VEC_FOR_SWIG( D_PADS, D_PAD* )

/**
 * Class NETINFO_ITEM
 * handles the data for a net
 */
class NETINFO_ITEM : public BOARD_ITEM
{
    friend class NETINFO_LIST;

private:
    int m_NetCode;              ///< A number equivalent to the net name.
                                ///< Used for fast comparisons in ratsnest and DRC computations.

    wxString m_Netname;         ///< Full net name like /mysheet/mysubsheet/vout used by Eeschema

    wxString m_ShortNetname;    ///< short net name, like vout from /mysheet/mysubsheet/vout

    wxString  m_NetClassName;   // Net Class name. if void this is equivalent
                                // to "default" (the first
                                // item of the net classes list
    NETCLASSPTR m_NetClass;

    BOARD*  m_parent;           ///< The parent board the net belongs to.

public:

    D_PADS& Pads()              { return m_PadInNetList; }

    /**
     * Function GetNodesCount
     * @return int - number of pad nodes in the net
     */
    int GetNodesCount() const   { return m_PadInNetList.size(); }


    D_PADS  m_PadInNetList;     ///< List of pads connected to this net

    unsigned m_RatsnestStartIdx;       /* Starting point of ratsnests of this
                                        * net (included) in a general buffer of
                                        * ratsnest (a vector<RATSNEST_ITEM*>
                                        * buffer) */

    unsigned m_RatsnestEndIdx;         // Ending point of ratsnests of this net
                                       // (excluded) in this buffer

    NETINFO_ITEM( BOARD* aParent, const wxString& aNetName = wxEmptyString, int aNetCode = -1 );
    ~NETINFO_ITEM();

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && PCB_T == aItem->Type();
    }

    wxString GetClass() const override
    {
        return wxT( "NETINFO_ITEM" );
    }

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override
    {
    }
#endif

    const wxPoint& GetPosition() const override
    {
        static wxPoint dummy(0, 0);
        return dummy;
    }

    void SetPosition( const wxPoint& aPos ) override
    {
    }

    /**
     * Function SetClass
     * sets \a aNetclass into this NET
     */
    void SetClass( NETCLASSPTR aNetClass )
    {
        m_NetClass = aNetClass;

        if( aNetClass )
            m_NetClassName = aNetClass->GetName();
        else
            m_NetClassName = NETCLASS::Default;
    }

    NETCLASSPTR GetNetClass()
    {
        return m_NetClass;
    }

    /**
     * Function GetClassName
     * returns the class name
     */
    const wxString& GetClassName() const
    {
        return m_NetClassName;
    }

#if 1

    /**
     * Function GetTrackWidth
     * returns the width of tracks used to route this net.
     */
    int GetTrackWidth()
    {
        wxASSERT( m_NetClass );
        return m_NetClass->GetTrackWidth();
    }

    /**
     * Function GetViaSize
     * returns the size of vias used to route this net
     */
    int GetViaSize()
    {
        wxASSERT( m_NetClass );
        return m_NetClass->GetViaDiameter();
    }

    /**
     * Function GetMicroViaSize
     * returns the size of vias used to route this net
     */
    int GetMicroViaSize()
    {
        wxASSERT( m_NetClass );
        return m_NetClass->GetuViaDiameter();
    }

    /**
     * Function GetViaDrillSize
     * returns the size of via drills used to route this net
     */
    int GetViaDrillSize()
    {
        wxASSERT( m_NetClass );
        return m_NetClass->GetViaDrill();
    }

    /**
     * Function GetViaDrillSize
     * returns the size of via drills used to route this net
     */
    int GetMicroViaDrillSize()
    {
        wxASSERT( m_NetClass );
        return m_NetClass->GetuViaDrill();
    }


#if 0

    /**
     * Function GetViaMinSize
     * returns the Minimum value for via sizes (used in DRC)
     */
    int GetViaMinSize()
    {
        wxASSERT( m_NetClass );
        return m_NetClass->GetViaMinSize();
    }

#endif

    /**
     * Function GetClearance
     * returns the clearance when routing near aBoardItem
     */
    int GetClearance( BOARD_ITEM* aBoardItem )
    {
        wxASSERT( m_NetClass );
        return m_NetClass->GetClearance();
    }

#endif

    /**
     * Function Draw
     * @todo we actually could show a NET, simply show all the tracks and
     *       a pads or net name on pad and vias
     */
    void Draw( EDA_DRAW_PANEL* panel, wxDC* DC, GR_DRAWMODE aDrawMode,
               const wxPoint& offset ) override;

    /**
     * Function GetNet
     * @return int - the netcode
     */
    int GetNet() const { return m_NetCode; }

    void SetNetCode( int aNetCode ) { m_NetCode = aNetCode; }

    /**
     * Function GetNetname
     * @return const wxString&, a reference to the full netname
     */
    const wxString& GetNetname() const { return m_Netname; }

    /**
     * Function GetShortNetname
     * @return const wxString &, a reference to the short netname
     */
    const wxString& GetShortNetname() const { return m_ShortNetname; }

    /**
     * Function GetMsgPanelInfo
     * returns the information about the #NETINFO_ITEM in \a aList to display in the
     * message panel.
     *
     * @param aList is the list in which to place the  status information.
     */
    void GetMsgPanelInfo( std::vector< MSG_PANEL_ITEM >& aList ) override;

    /**
     * Function Clear
     * sets all fields to their defaults values.
     */
    void Clear()
    {
        m_PadInNetList.clear();

        m_RatsnestStartIdx  = 0;     // Starting point of ratsnests of this net in a
                                     // general buffer of ratsnest
        m_RatsnestEndIdx    = 0;     // Ending point of ratsnests of this net

        SetClass( NETCLASSPTR() );
    }

    BOARD* GetParent() const
    {
        return m_parent;
    }
};


class NETINFO_MAPPING
{
public:
    NETINFO_MAPPING()
    {
        m_board = NULL;
    }


    /**
     * Function SetBoard
     * Sets a BOARD object that is used to prepare the net code map.
     */
    void SetBoard( const BOARD* aBoard )
    {
        m_board = aBoard;
        Update();
    }

    /**
     * Function Update
     * Prepares a mapping for net codes so they can be saved as consecutive numbers.
     * To retrieve a mapped net code, use translateNet() function after calling this.
     */
    void Update();

    /**
     * Function Translate
     * Translates net number according to the map prepared by Update() function. It
     * allows to have items stored with consecutive net codes.
     * @param aNetCode is an old net code.
     * @return Net code that follows the mapping.
     */
    int Translate( int aNetCode ) const;

    ///> Wrapper class, so you can iterate through NETINFO_ITEM*s, not
    ///> std::pair<int/wxString, NETINFO_ITEM*>
    class iterator
    {
    public:
        iterator( std::map<int, int>::const_iterator aIter, const NETINFO_MAPPING* aMapping ) :
            m_iterator( aIter ), m_mapping( aMapping )
        {
        }

        /// pre-increment operator
        const iterator& operator++()
        {
            ++m_iterator;

            return *this;
        }

        /// post-increment operator
        iterator operator++( int )
        {
            iterator ret = *this;
            ++m_iterator;

            return ret;
        }

        NETINFO_ITEM* operator*() const;

        NETINFO_ITEM* operator->() const;

        bool operator!=( const iterator& aOther ) const
        {
            return m_iterator != aOther.m_iterator;
        }

        bool operator==( const iterator& aOther ) const
        {
            return m_iterator == aOther.m_iterator;
        }

    private:
        std::map<int, int>::const_iterator m_iterator;
        const NETINFO_MAPPING* m_mapping;
    };

    /**
     * Function begin()
     * Returns iterator to the first entry in the mapping.
     * NOTE: The entry is a pointer to the original NETINFO_ITEM object, this it contains
     * not mapped net code.
     */
    iterator begin() const
    {
        return iterator( m_netMapping.begin(), this );
    }

    /**
     * Function end()
     * Returns iterator to the last entry in the mapping.
     * NOTE: The entry is a pointer to the original NETINFO_ITEM object, this it contains
     * not mapped net code.
     */
    iterator end() const
    {
        return iterator( m_netMapping.end(), this );
    }

    /**
     * Function GetSize
     * @return Number of mapped nets (i.e. not empty nets for a given BOARD object).
     */
    int GetSize() const
    {
        return m_netMapping.size();
    }

private:
    ///> Board for which mapping is prepared
    const BOARD* m_board;

    ///> Map that allows saving net codes with consecutive numbers (for compatibility reasons)
    std::map<int, int> m_netMapping;
};


#if 0
// waiting for swig to support std::unordered_map, see
// http://www.swig.org/Doc3.0/CPlusPlus11.html
// section 7.3.3
#include <hashtables.h>
DECL_HASH_FOR_SWIG( NETNAMES_MAP, wxString,  NETINFO_ITEM* )
DECL_HASH_FOR_SWIG( NETCODES_MAP, int,       NETINFO_ITEM* )
#else
// use std::map for now
DECL_MAP_FOR_SWIG( NETNAMES_MAP, wxString,  NETINFO_ITEM* )
DECL_MAP_FOR_SWIG( NETCODES_MAP, int,       NETINFO_ITEM* )
#endif

/**
 * Class NETINFO_LIST
 * is a container class for NETINFO_ITEM elements, which are the nets.  That makes
 * this class a container for the nets.
 */
class NETINFO_LIST
{
    friend class BOARD;

public:
    NETINFO_LIST( BOARD* aParent );
    ~NETINFO_LIST();

    /**
     * Function GetItem
     * @param aNetCode = netcode to identify a given NETINFO_ITEM
     * @return NETINFO_ITEM* - by \a aNetCode, or NULL if not found
     */
    NETINFO_ITEM* GetNetItem( int aNetCode ) const;

    /**
     * Function GetItem
     * @param aNetName = net name to identify a given NETINFO_ITEM
     * @return NETINFO_ITEM* - by \a aNetName, or NULL if not found
     */
    NETINFO_ITEM* GetNetItem( const wxString& aNetName ) const;

    /**
     * Function GetNetCount
     * @return the number of nets ( always >= 1 )
     * because the first net is the "not connected" net and always exists
     */
    unsigned GetNetCount() const { return m_netNames.size(); }

    /**
     * Function AppendNet
     * adds \a aNewElement to the end of the net list. Negative net code means it is going to be
     * auto-assigned.
     */
    void AppendNet( NETINFO_ITEM* aNewElement );

    /**
     * Function RemoveNet
     * Removes a new from the net list.
     */
    void RemoveNet( NETINFO_ITEM* aNet );
    /**
     * Function GetPadCount
     * @return the number of pads in board
     */
    unsigned GetPadCount() const                { return m_PadsFullList.size(); }

    /**
     * Function GetPads
     * returns a list of all the pads (so long as buildPadsFullList() has
     * been recently called).  Returned list contains non-owning pointers.
     * @return D_PADS& - a full list of pads
     */
    const D_PADS& GetPads() const  { return m_PadsFullList; }

    /// Return the name map, at least for python.
    const NETNAMES_MAP& NetsByName() const      { return  m_netNames; }

    /// Return the netcode map, at least for python.
    const NETCODES_MAP& NetsByNetcode() const   { return m_netCodes; }

    /**
     * Function GetPad
     * @return D_PAD* - the pad from m_PadsFullList or nullptr if bad @a aIdx
     */
    D_PAD* GetPad( unsigned aIdx ) const;

    bool DeletePad( D_PAD* aPad );

    ///> Constant that holds the "unconnected net" number (typically 0)
    ///> all items "connected" to this net are actually not connected items
    static const int UNCONNECTED;

    ///> Constant that forces initialization of a netinfo item to the NETINFO_ITEM ORPHANED
    ///> (typically -1) when calling SetNetCode od board connected items
    static const int ORPHANED;

    ///> NETINFO_ITEM meaning that there was no net assigned for an item, as there was no
    ///> board storing net list available.
    static NETINFO_ITEM ORPHANED_ITEM;

#if defined(DEBUG)
    void Show() const;
#endif

#ifndef SWIG
    ///> Wrapper class, so you can iterate through NETINFO_ITEM*s, not
    ///> std::pair<int/wxString, NETINFO_ITEM*>
    class iterator
    {
    public:
        iterator( NETNAMES_MAP::const_iterator aIter ) : m_iterator( aIter )
        {
        }

        /// pre-increment operator
        const iterator& operator++()
        {
            ++m_iterator;
            return *this;
        }

        /// post-increment operator
        iterator operator++( int )
        {
            iterator ret = *this;
            ++m_iterator;
            return ret;
        }

        NETINFO_ITEM* operator*() const
        {
            return m_iterator->second;
        }

        NETINFO_ITEM* operator->() const
        {
            return m_iterator->second;
        }

        bool operator!=( const iterator& aOther ) const
        {
            return m_iterator != aOther.m_iterator;
        }

        bool operator==( const iterator& aOther ) const
        {
            return m_iterator == aOther.m_iterator;
        }

    private:
        NETNAMES_MAP::const_iterator m_iterator;
    };

    iterator begin() const
    {
        return iterator( m_netNames.begin() );
    }

    iterator end() const
    {
        return iterator( m_netNames.end() );
    }
#endif

    BOARD* GetParent() const
    {
        return m_Parent;
    }

private:
    /**
     * Function clear
     * deletes the list of nets (and free memory)
     */
    void clear();

    /**
     * Function buildListOfNets
     * builds or rebuilds the list of NETINFO_ITEMs
     * The list is sorted by names.
     */
    void buildListOfNets();

    /**
     * Function buildPadsFullList
     * creates the pad list, and initializes:
     *   m_Pads (list of pads)
     * set m_Status_Pcb = LISTE_PAD_OK;
     * and clear for all pads in list the m_SubRatsnest member;
     * clear m_Pcb->m_FullRatsnest
     */
    void buildPadsFullList();

    /**
     * Function getFreeNetCode
     * returns the first available net code that is not used by any other net.
     */
    int getFreeNetCode();

    BOARD* m_Parent;

    NETNAMES_MAP m_netNames;        ///< map of <wxString, NETINFO_ITEM*>, is NETINFO_ITEM owner
    NETCODES_MAP m_netCodes;        ///< map of <int, NETINFO_ITEM*> is NOT owner

    D_PADS  m_PadsFullList;         ///< contains all pads, sorted by pad's netname.
                                    ///< can be used in ratsnest calculations.

    int m_newNetCode;               ///< possible value for new net code assignment
};


/***********************************************************/
/* Description of a trace point for monitoring connections */
/***********************************************************/
#define START_ON_PAD   0x10
#define END_ON_PAD     0x20
#define START_ON_TRACK 0x40
#define END_ON_TRACK   0x80


/* Status bit (OR'ed bits) for class BOARD member .m_Status_Pcb */
enum StatusPcbFlags {
    LISTE_PAD_OK = 1,                    /* Pad list is Ok */
    LISTE_RATSNEST_ITEM_OK = 2,          /* General Ratsnest is Ok */
    RATSNEST_ITEM_LOCAL_OK = 4,          /* current MODULE ratsnest is Ok */
    CONNEXION_OK = 8,                    /* List of connections exists. */
    NET_CODES_OK = 0x10,                 /* Bit indicating that Netcode is OK,
                                          * do not change net name.  */
    DO_NOT_SHOW_GENERAL_RASTNEST = 0x20  /* Do not display the general
                                          * ratsnest (used in module moves) */
};

#endif  // CLASS_NETINFO_
