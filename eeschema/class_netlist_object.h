/***********************************************************************/
/* Class NETLIST_OBJECT to handle 1 item connected (in netlist and erc */
/* calculations)                                                       */
/***********************************************************************/

#ifndef _CLASS_NETLIST_OBJECT_H_
#define _CLASS_NETLIST_OBJECT_H_

#include "sch_sheet_path.h"

#include "lib_pin.h"      // LIB_PIN::ReturnPinStringNum( m_PinNum )


/* Type of Net objects (wires, labels, pins...) */
enum NetObjetType {
    NET_ITEM_UNSPECIFIED,           // only for not yet initialized instances
    NET_SEGMENT,                    // connection by wire
    NET_BUS,                        // connection by bus
    NET_JONCTION,                   // connection by junction: can connect to
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
enum  ConnectType {
    UNCONNECTED = 0,            /* Pin or Label not connected (error) */
    NOCONNECT_SYMBOL_PRESENT,   /* Pin not connected but have a  NoConnect
                                 * symbol on it (no error) */
    PAD_CONNECT                 /* Normal connection (no error) */
};


class NETLIST_OBJECT
{
public:
    NetObjetType    m_Type;             /* Type of item (see NetObjetType
                                         * enum) */
    EDA_ITEM      * m_Comp;             /* Pointer on the library item that
                                         * created this net object (the parent)
                                         */
    SCH_ITEM*       m_Link;             /* For SCH_SHEET_PIN:
                                         *  Pointer to the hierarchy sheet that
                                         *  contains this SCH_SHEET_PIN
                                         * For Pins: pointer to the component
                                         *   that contains this pin
                                         */
    int            m_Flag;              /* flag used in calculations */
    SCH_SHEET_PATH m_SheetList;
    int            m_ElectricalType;    /* Has meaning only for Pins and
                                         * hierarchical pins: electrical type */
private:
    int            m_NetCode;           /* net code for all items except BUS
                                         * labels because a BUS label has
                                         * as many net codes as bus members
                                         */
public:
    int m_BusNetCode;                   /* Used for BUS connections */
    int m_Member;                       /* for labels type NET_BUSLABELMEMBER
                                         * ( bus member created from the BUS
                                         * label ) member number
                                         */
    ConnectType     m_FlagOfConnection;
    SCH_SHEET_PATH  m_SheetListInclude; /* sheet that the hierarchical label
                                         * connects to.*/
    long            m_PinNum;           /* pin number ( 1 long = 4 bytes ->
                                         * 4 ascii codes) */
    wxString        m_Label;            /* Label text. */
    wxPoint         m_Start;            // Position of object or for segments:
                                        // starting point
    wxPoint         m_End;              // For segments (wire and buses):
                                        // ending point
    NETLIST_OBJECT* m_NetNameCandidate; /* a pointer to a label connected to the net,
                                         * that can be used to give a name to the net
                                         * NULL if no usable label
                                         */

#if defined(DEBUG)
    void Show( std::ostream& out, int ndx );

#endif
    NETLIST_OBJECT();
    NETLIST_OBJECT( NETLIST_OBJECT& aSource );       // Copy constructor

    ~NETLIST_OBJECT();

    void SetNet( int aNetCode ) { m_NetCode = aNetCode; }
    int GetNet() const { return m_NetCode; }

    /**
     * Function GetPinNum
     * returns a pin number in wxString form.  Pin numbers are not always
     * numbers.  "A23" would be a valid pin number.
     */
    wxString GetPinNumText()
    {
        // hide the ugliness in here, but do it inline.
        return  LIB_PIN::ReturnPinStringNum( m_PinNum );
    }
};

// Buffer to build the list of items used in netlist and erc calculations
typedef std::vector <NETLIST_OBJECT*> NETLIST_OBJECT_LIST;


#endif  // _CLASS_NETLIST_OBJECT_H_
