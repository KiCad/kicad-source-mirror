/**********************************************/
/* Module de calcul de la Netliste: netlist.h */
/**********************************************/

#ifndef _NETLIST_H_
#define _NETLIST_H_

#define NETLIST_HEAD_STRING "EESchema Netlist Version 1.1"

#define ISBUS 1

#define CUSTOMPANEL_COUNTMAX 8  // Max number of netlist plugins

/* Id to select netlist type */
enum  TypeNetForm {
    NET_TYPE_UNINIT = 0,
    NET_TYPE_PCBNEW,
    NET_TYPE_ORCADPCB2,
    NET_TYPE_CADSTAR,
    NET_TYPE_SPICE,
    NET_TYPE_CUSTOM1,   /* NET_TYPE_CUSTOM1
                         * is the first id for user netlist format
                         * NET_TYPE_CUSTOM1+CUSTOMPANEL_COUNTMAX-1
                         * is the last id for user netlist format
                         */
    NET_TYPE_CUSTOM_MAX = NET_TYPE_CUSTOM1 + CUSTOMPANEL_COUNTMAX - 1
};


/* Max pin number per component and footprint */
#define MAXPIN 5000

enum NetObjetType {
    /* Type des objets de Net */
    NET_SEGMENT,
    NET_BUS,
    NET_JONCTION,
    NET_LABEL,
    NET_GLOBLABEL,
    NET_HIERLABEL,  //on a screen to indicate connection to a higher-level sheet
    NET_SHEETLABEL, //on a drawscreen element to indicate connection to a lower-level sheet.
    NET_BUSLABELMEMBER,
    NET_GLOBBUSLABELMEMBER,
    NET_HIERBUSLABELMEMBER,
    NET_SHEETBUSLABELMEMBER,
    NET_PINLABEL,
    NET_PIN,
    NET_NOCONNECT
};


enum  ConnectType {
    /* Valeur du Flag de connection */
    UNCONNECTED = 0,        /* Pin ou Label non connecte */
    NOCONNECT,              /* Pin volontairement non connectee (Symb. NoConnect utilise) */
    PAD_CONNECT             /* connexion normale */
};


/* Structure decrivant 1 element de connexion (pour netlist ) */
class ObjetNetListStruct
{
public:
    NetObjetType    m_Type;             // Type of this item (see NetObjetType enum)
    EDA_BaseStruct* m_Comp;             /* Pointer on the schematic item that created this net object (the parent)*/
    void*           m_Link;      /* For Hierarchical_PIN_Sheet_Struct:
                                  * Pointer to the hierarchy sheet that contains this Hierarchical_PIN_Sheet_Struct
                                  *  For Pins: pointer to the component that contains this pin
                                  */
    int             m_Flag;             /* flag used in calculations */
    DrawSheetPath   m_SheetList;
    int             m_ElectricalType;   /* Has meaning only for Pins and hierachical pins: electrical type */
private:
    int             m_NetCode;          /* net code for all items except BUS labels because a BUS label has
                                         *  as many net codes as bus members
                                         */
public:
    int             m_BusNetCode;       /* Used for BUS connections */
    int             m_Member;        /* for labels type NET_BUSLABELMEMBER ( bus member created from the BUS label )
                                      *  member number
                                      */
    ConnectType     m_FlagOfConnection;
    DrawSheetPath   m_SheetListInclude;     /* sheet that the hierarchal label connects to.*/
    long            m_PinNum;               /* numero de pin( 4 octets -> 4 codes ascii) */
    const wxString* m_Label;                /* For all labels:pointer on the text label */
    wxPoint         m_Start, m_End;

#if defined(DEBUG)
    void Show( std::ostream& out, int ndx );

#endif

    void SetNet( int aNetCode ) { m_NetCode = aNetCode; }
    int GetNet() const { return m_NetCode; }
};


/* object used in annotation to handle a list of components in schematic
 * because in a complex hierarchy, a component is used more than once,
 * and its reference is depending on the sheet path
 * for the same component, we must create a flat list of components
 * used in nelist generation, BOM generation and annotation
 */
class OBJ_CMP_TO_LIST
{
public:
    SCH_COMPONENT*          m_RootCmp;                  // the component in schematic
    EDA_LibComponentStruct* m_Entry;                    // the source component in library
    int m_Unit;                                         /* Selected part (For multi parts per package) depending on sheet path */
    DrawSheetPath           m_SheetPath;                /* the sheet path for this component */
    unsigned long           m_TimeStamp;                /* unique identification number depending on sheet path */
    bool      m_IsNew;                                  /* true for not yet annotated components */
    wxString* m_Value;                                  /* Component value (same for all instances) */
    char      m_Reference[32];                          /* Component reference prefix, without number (for IC1, this is IC) ) */
    int       m_NumRef;                                 /* Reference number (for IC1, this is 1) ) depending on sheet path*/
    int       m_Flag;                                   /* flag for computations */
public:

    OBJ_CMP_TO_LIST()
    {
        m_RootCmp      = NULL;
        m_Entry        = NULL;
        m_Unit         = 0;
        m_TimeStamp    = 0;
        m_IsNew        = false;
        m_Value        = NULL;
        m_Reference[0] = 0;
        m_NumRef       = 0;
        m_Flag         = 0;
    }


    int CompareValue( const OBJ_CMP_TO_LIST& item ) const
    {
        return m_Value->CmpNoCase( *item.m_Value );
    }


    int CompareRef( const OBJ_CMP_TO_LIST& item ) const
    {
        return strnicmp( m_Reference, item.m_Reference, 32 );
    }


    bool IsPartsLocked()
    {
        return m_Entry->m_UnitSelectionLocked;
    }
};


/* Global Variables */
extern int g_NbrObjNet;
extern ObjetNetListStruct* g_TabObjNet;


/* Prototypes: */
void     WriteNetList( WinEDA_SchematicFrame* frame,
                       const wxString&        FileNameNL,
                       bool                   use_netnames );
void     FreeTabNetList( ObjetNetListStruct* TabNetItems, int NbrNetItems );

/** Function ReturnUserNetlistTypeName
 * to retrieve user netlist type names
 * @param first = true: return first name of the list, false = return next
 * @return a wxString : name of the type netlist or empty string
 * this function must be called first with "first_item" = true
 * and after with "first_item" = false to get all the other existing netlist names
 */
wxString ReturnUserNetlistTypeName( bool first_item );


#endif
