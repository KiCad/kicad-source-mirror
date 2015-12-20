/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2013 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file class_netlist_object.h
 * @brief Definition of the NETLIST_OBJECT class.
 */

#ifndef _CLASS_NETLIST_OBJECT_H_
#define _CLASS_NETLIST_OBJECT_H_


#include <sch_sheet_path.h>
#include <lib_pin.h>      // LIB_PIN::PinStringNum( m_PinNum )
#include <sch_item_struct.h>

class NETLIST_OBJECT_LIST;
class SCH_COMPONENT;


/* Type of Net objects (wires, labels, pins...) */
enum NETLIST_ITEM_T
{
    NET_ITEM_UNSPECIFIED,           // only for not yet initialized instances
    NET_SEGMENT,                    // connection by wire
    NET_BUS,                        // connection by bus
    NET_JUNCTION,                   // connection by junction: can connect to
                                    // or more crossing wires
    NET_LABEL,                      // this is a local label
    NET_GLOBLABEL,                  // this is a global label that connect all
                                    // others global label in whole hierarchy
    NET_HIERLABEL,                  // element to indicate connection to a
                                    // higher-level sheet
    NET_SHEETLABEL,                 // element to indicate connection to a
                                    // lower-level sheet.
    NET_BUSLABELMEMBER,             /* created when a bus label is found:
                                     * the bus label (like DATA[0..7] is
                                     * converted to n single labels like
                                     * DATA0, DATA1 ...
                                     */
    NET_GLOBBUSLABELMEMBER,         // see NET_BUSLABELMEMBER, used when a
                                    // global bus label is found
    NET_HIERBUSLABELMEMBER,         // see NET_BUSLABELMEMBER, used when a
                                    // hierarchical bus label is found
    NET_SHEETBUSLABELMEMBER,        // see NET_BUSLABELMEMBER, used when a
                                    // pin sheet label using bus notation
                                    // is found
    NET_PINLABEL,                   /* created when a pin is POWER (IN or
                                     * OUT) with invisible attribute is found:
                                     * these pins are equivalent to a global
                                     * label and are automatically connected
                                     */
    NET_PIN,                        // this is an usual pin
    NET_NOCONNECT                   // this is a no connect symbol
};


/* Values for .m_FlagOfConnection member */
enum NET_CONNECTION_T
{
    UNCONNECTED = 0,            /* Pin or Label not connected (error) */
    NOCONNECT_SYMBOL_PRESENT,   /* Pin not connected but have a  NoConnect
                                 * symbol on it (no error) */
    PAD_CONNECT                 /* Normal connection (no error) */
};


class NETLIST_OBJECT
{
public:
    NETLIST_ITEM_T m_Type;              /* Type of item (see NETLIST_ITEM_T enum) */
    EDA_ITEM* m_Comp;                   /* Pointer to the library item that
                                         * created this net object (the parent)
                                         */
    SCH_ITEM* m_Link;                   /* For SCH_SHEET_PIN:
                                         * Pointer to the hierarchy sheet that
                                         * contains this SCH_SHEET_PIN
                                         * For Pins: pointer to the schematic component
                                         * that contains this pin
                                         */
    int m_Flag;                         /* flag used in calculations */
    SCH_SHEET_PATH  m_SheetPath;        // the sheet path which contains this item
    SCH_SHEET_PATH  m_SheetPathInclude; // sheet path which contains the hierarchical label
    int m_ElectricalType;               /* Has meaning only for Pins and
                                         * hierarchical pins: electrical type */
    int m_BusNetCode;                   /* Used for BUS connections */
    int m_Member;                       /* for labels type NET_BUSLABELMEMBER ( bus member
                                         * created from the BUS label ) member number.
                                         */
    NET_CONNECTION_T m_ConnectionType;  // Used to store the connection type
    long m_PinNum;                      // pin number ( 1 long = 4 bytes -> 4 ascii codes)
    wxString    m_Label;                // Label text (for labels) or Pin name (for pins)
    wxPoint     m_Start;                // Position of object or for segments: starting point
    wxPoint     m_End;                  // For segments (wire and buses): ending point

private:
    int m_netCode;                      /* net code for all items except BUS
                                         * labels because a BUS label has
                                         * as many net codes as bus members
                                         */
    NETLIST_OBJECT* m_netNameCandidate; /* a pointer to a label connected to the net,
                                         * that can be used to give a name to the net
                                         * or a pin if there is no label in net
                                         * When no label, the pin is used to build
                                         * default net name.
                                         */

public:

#if defined(DEBUG)
    void Show( std::ostream& out, int ndx ) const;      // override

#endif

    NETLIST_OBJECT();
    NETLIST_OBJECT( NETLIST_OBJECT& aSource );       // Copy constructor

    ~NETLIST_OBJECT();

    // Accessors:
    void SetNet( int aNetCode ) { m_netCode = aNetCode; }
    int GetNet() const { return m_netCode; }

    /**
     * Set the item connection type:
     * UNCONNECTED                 Pin or Label not connected (error)
     * NOCONNECT_SYMBOL_PRESENT    Pin not connected but have a  NoConnect
     *                             symbol on it (no error)
     * PAD_CONNECT                 Normal connection (no error)
     */
    void SetConnectionType( NET_CONNECTION_T aFlg = UNCONNECTED )
    {
        m_ConnectionType = aFlg;
    }

    NET_CONNECTION_T GetConnectionType()
    {
        return m_ConnectionType;
    }

    /**
     * Set m_netNameCandidate to a connected item which will
     * be used to calcule the net name of the item
     * Obviously the candidate can be only a label
     * when there is no label on the net a pad which will
     * used to build a net name (something like Cmp<REF>_Pad<PAD_NAME>
     * @param aCandidate = the connected item candidate
     */
    void SetNetNameCandidate( NETLIST_OBJECT* aCandidate );

    /**
     * @return true if an item has already a net name candidate
     * and false if not ( m_netNameCandidate == NULL )
     */
    bool HasNetNameCandidate() { return m_netNameCandidate != NULL; }

    /**
     * Function GetPinNum
     * returns a pin number in wxString form.  Pin numbers are not always
     * numbers.  \"A23\" would be a valid pin number.
     */
    wxString GetPinNumText()
    {
        // hide the ugliness in here, but do it inline.
        return LIB_PIN::PinStringNum( m_PinNum );
    }

    /**  For Pins (NET_PINS):
     * @return the schematic component which contains this pin
     * (Note: this is the schematic component, not the library component
     * for others items: return NULL
     */
    SCH_COMPONENT* GetComponentParent() const
    {
        if( m_Link && m_Link->Type() == SCH_COMPONENT_T )
            return (SCH_COMPONENT*) m_Link;

        return NULL;
    }

    /**
     * Function IsLabelConnected
     * tests if the net list object is a hierarchical label or sheet label and is
     * connected to an associated hierarchical label or sheet label of \a aNetItem.
     *
     * @param aNetItem A pointer to a NETLIST_OBJECT to test against.
     * @return A bool value of true if there is a connection with \a aNetItem or false
     *         if no connection to \a aNetItem.
     */
    bool IsLabelConnected( NETLIST_OBJECT* aNetItem );

    /**
     * Function IsLabelGlobal
     * @return true if the object is a global label
     * (i.e. an real global label or a pin label coming
     * from a power pin invisible
     */
    bool IsLabelGlobal() const
    {
        return ( m_Type == NET_PINLABEL ) || ( m_Type == NET_GLOBLABEL );
    }

    /**
     * Function IsLabelType
     * @return true if the object is a label of any type
     */
    bool IsLabelType() const;

    /**
     * Function GetNetName
     * @return the full net name of the item, i.e. the net name
     * from the "best" label, prefixed by the sheet path
     */
    wxString GetNetName() const;

    /**
     * Function GetShortNetName
     * @return the short net name of the item i.e. the net name
     * from the "best" label without any prefix.
     * 2 different nets can have the same short name
     */
    wxString GetShortNetName() const;

    /**
     * Function ConvertBusToNetListItems
     * breaks the text of a bus label type net list object into as many members as
     * it contains and creates a #NETLIST_OBJECT for each label and adds it to \a
     * aNetListItems.
     *
     * @param aNetListItems A reference to vector of #NETLIST_OBJECT pointers to add
     *                      the bus label NETLIST_OBJECTs.
     */
    void ConvertBusToNetListItems( NETLIST_OBJECT_LIST& aNetListItems );
};


/**
 * Type NETLIST_OBJECTS
 * is a container referring to (not owning) NETLIST_OBJECTs, which are connected items
 * in a full schematic hierarchy.  It is useful when referring to NETLIST_OBJECTs
 * actually owned by some other container.
 */
typedef std::vector<NETLIST_OBJECT*>    NETLIST_OBJECTS;


/**
 * Class NETLIST_OBJECT_LIST
 * is a container holding and _owning_ NETLIST_OBJECTs, which are connected items
 * in a full schematic hierarchy.  It is helpful for netlist and ERC calculations.
 */
class NETLIST_OBJECT_LIST : public NETLIST_OBJECTS
{
    int m_lastNetCode;      // Used in intermediate calculation: last net code created
    int m_lastBusNetCode;   // Used in intermediate calculation:
                            // last net code created for bus members

public:
    /**
     * Constructor.
     * NETLIST_OBJECT_LIST handle a list of connected items.
     * the instance can be owner of items or not.
     * If it is the owner, the items are freeed by the destructor
     * @param aIsOwner true if the instance is the owner of item list
     * (default = false)
     */
    NETLIST_OBJECT_LIST()
    {
        // Do not leave some members uninitialized:
        m_lastNetCode = 0;
        m_lastBusNetCode = 0;
    }

    ~NETLIST_OBJECT_LIST();

    /**
     * Function BuildNetListInfo
     * the master function of tgis class.
     * Build the list of connected objects (pins, labels ...) and
     * all info to generate netlists or run ERC diags
     * @param aSheets = the flattened sheet list
     * @return true if OK, false is not item found
     */
    bool BuildNetListInfo( SCH_SHEET_LIST& aSheets );

    /**
     * Acces to an item in list
     */
    NETLIST_OBJECT* GetItem( unsigned aIdx ) const
    {
        return *( this->begin() + aIdx );
    }

    /**
     * Acces to an item type
     */
    NETLIST_ITEM_T GetItemType( unsigned aIdx ) const
    {
        return GetItem( aIdx )->m_Type;
    }

    /**
     * Acces to an item net code
     */
    int GetItemNet( unsigned aIdx ) const
    {
        return GetItem( aIdx )->GetNet();
    }

    NET_CONNECTION_T GetConnectionType( unsigned aIdx )
    {
        return GetItem( aIdx )->GetConnectionType();
    }

    /**
     * Set the item connection type:
     * UNCONNECTED                 Pin or Label not connected (error)
     * NOCONNECT_SYMBOL_PRESENT    Pin not connected but have a  NoConnect
     *                             symbol on it (no error)
     * PAD_CONNECT                 Normal connection (no error)
     */
    void SetConnectionType( unsigned aIdx, NET_CONNECTION_T aFlg = UNCONNECTED )
    {
        GetItem( aIdx )->SetConnectionType( aFlg );
    }

    /** Delete all objects in list and clear list */
    void Clear();

    /**
     * Reset the connection type of all items to UNCONNECTED type
     */
    void ResetConnectionsType()
    {
        for( unsigned ii = 0; ii < size(); ii++ )
            GetItem( ii )->SetConnectionType( UNCONNECTED );
    }

    /*
     * Sorts the list of connected items by net code
     */
    void SortListbyNetcode();

    /*
     * Sorts the list of connected items by sheet.
     * This sorting is used when searching "physical" connection between items
     * because obviously only items inside the same sheet can be connected
     */
    void SortListbySheet();

    /**
     * Counts number of pins connected on the same net.
     * Used to count all pins connected to a no connect symbol
     * @return the pin count of the net starting at aNetStart
     * @param aNetStart = index in list of net objects of the first item
     */
    int CountPinsInNet( unsigned aNetStart );

    /**
     * Function TestforNonOrphanLabel
     * Sheet labels are expected to be connected to a hierarchical label.
     * Hierarchical labels are expected to be connected to a sheet label.
     * Global labels are expected to be not orphan (connected to at least one other global label.
     * this function tests the connection to an other suitable label
     */
    void TestforNonOrphanLabel( unsigned aNetItemRef, unsigned aStartNet );

    /**
     * Function TestforSimilarLabels
     * detects labels which are different when using case sensitive comparisons
     * but are equal when using case insensitive comparisons
     * It can be due to a mistake from designer, so this kind of labels
     * is reported by TestforSimilarLabels
     */
    void TestforSimilarLabels();


    #if defined(DEBUG)
    void DumpNetTable()
    {
        for( unsigned idx = 0; idx < size(); ++idx )
        {
            GetItem( idx )->Show( std::cout, idx );
        }
    }

    #endif

private:
    /*
     * Propagate aNewNetCode to items having an internal netcode aOldNetCode
     * used to interconnect group of items already physically connected,
     * when a new connection is found between aOldNetCode and aNewNetCode
     */
    void propageNetCode( int aOldNetCode, int aNewNetCode, bool aIsBus );

    /*
     * This function merges the net codes of groups of objects already connected
     * to labels (wires, bus, pins ... ) when 2 labels are equivalents
     * (i.e. group objects connected by labels)
     */
    void labelConnect( NETLIST_OBJECT* aLabelRef );

    /* Comparison function to sort by increasing Netcode the list of connected items
     */
    static bool sortItemsbyNetcode( const NETLIST_OBJECT* Objet1, const NETLIST_OBJECT* Objet2 )
    {
        return Objet1->GetNet() < Objet2->GetNet();
    }

    /* Comparison routine to sort items by Sheet path
     */
    static bool sortItemsBySheet( const NETLIST_OBJECT* Objet1, const NETLIST_OBJECT* Objet2 )
    {
        return Objet1->m_SheetPath.Cmp( Objet2->m_SheetPath ) < 0;
    }

    /*
     * Propagate net codes from a parent sheet to an include sheet,
     * from a pin sheet connection
     */
    void sheetLabelConnect( NETLIST_OBJECT* aSheetLabel );

    void pointToPointConnect( NETLIST_OBJECT* aRef, bool aIsBus, int start );

    /*
     * Search connections betweena junction and segments
     * Propagate the junction net code to objects connected by this junction.
     * The junction must have a valid net code
     * The list of objects is expected sorted by sheets.
     * Search is done from index aIdxStart to the last element of list
     */
    void segmentToPointConnect( NETLIST_OBJECT* aJonction, bool aIsBus, int aIdxStart );

    void connectBusLabels();

    /*
     * Set the m_FlagOfConnection member of items in list
     * depending on the connection type:
     * UNCONNECTED, PAD_CONNECT or NOCONNECT_SYMBOL_PRESENT
     * The list is expected sorted by order of net code,
     * i.e. items having the same net code are grouped
     */
    void setUnconnectedFlag();

    /**
     * Function findBestNetNameForEachNet
     * fill the .m_NetNameCandidate member of each item of aNetItemBuffer
     * with a reference to the "best" NETLIST_OBJECT usable to give a name to the net
     * If no suitable object found, .m_NetNameCandidate is filled with 0.
     * The "best" NETLIST_OBJECT is a NETLIST_OBJECT that have the type label
     * and by priority order:
     * the label is global or local
     * the label is in the first sheet in a hierarchy (the root sheet has the most priority)
     * alphabetic order.
     */
    void findBestNetNameForEachNet();
};


/**
 * Function IsBusLabel
 * test if \a aLabel has a bus notation.
 *
 * @param aLabel A wxString object containing the label to test.
 * @return true if text is a bus notation format otherwise false is returned.
 */
extern bool IsBusLabel( const wxString& aLabel );

#endif    // _CLASS_NETLIST_OBJECT_H_
