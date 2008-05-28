/**********************************************/
/* Module de calcul de la Netliste: netlist.h */
/**********************************************/

#ifndef _NETLIST_H_
#define _NETLIST_H_

#ifndef eda_global
#define eda_global extern
#endif

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

enum NetObjetType {      /* Type des objets de Net */
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


enum  IsConnectType {       /* Valeur du Flag de connection */
    UNCONNECT,              /* Pin ou Label non connecte */
    NOCONNECT,              /* Pin volontairement non connectee (Symb. NoConnect utilise) */
    PAD_CONNECT             /* connexion normale */
};


/* Structure decrivant 1 element de connexion (pour netlist ) */
class ObjetNetListStruct
{
public:
    EDA_BaseStruct* m_Comp;         /* Pointeur sur la definition de l'objet */
    void*           m_Link;      /* Pour SheetLabelStruct: Pointeur sur la feuille de hierarchie
                                  *  Pour les Pins: pointeur sur le composant */
    int             m_Flag;         /* flag pour calculs internes */
    DrawSheetPath   m_SheetList;
    NetObjetType    m_Type;
    int             m_ElectricalType;   /* Pour Pins et sheet labels: type electrique */
private:
    int             m_NetCode;          /* pour elements simples */
public:
    int             m_BusNetCode;       /* pour connexions type bus */
    int             m_Member;        /* pour les labels type BUSWIRE ( labels de bus eclate )
                                      *  numero de membre */
    IsConnectType   m_FlagOfConnection;
    DrawSheetPath   m_SheetListInclude;     /* sheet that the hierarchal label connects to.*/
    long            m_PinNum;               /* numero de pin( 4 octets -> 4 codes ascii) */
    const wxString* m_Label;                /* Tous types Labels:pointeur sur la wxString definissant le label */
    wxPoint         m_Start, m_End;

#if defined (DEBUG)
    void Show( std::ostream& out, int ndx );

#endif

    void SetNet( int aNetCode ) { m_NetCode = aNetCode; }
    int GetNet() const { return m_NetCode; }
};


/* Structures pour memo et liste des elements */
struct ListLabel
{
    int   m_LabelType;
    void* m_Label;
    char  m_SheetPath[256];
};


// Used to create lists of components BOM, netlist generation)
struct ListComponent
{
    SCH_COMPONENT* m_Comp;      // pointer on the component in schematic
    char           m_Ref[32];   // component reference
    int            m_Unit;      // Unit value, for multiple parts per package
    //have to store it here since the object references will be duplicated.
    DrawSheetPath m_SheetList; //composed of UIDs
};


/* Structure decrivant 1 composant de la schematique (for annotation ) */
struct CmpListStruct
{
public:
    SCH_COMPONENT* m_Cmp;                               /* Pointeur sur le composant */
    int            m_NbParts;                           /* Nombre de parts par boitier */
    bool           m_PartsLocked;                       // For multi part components: True if the part cannot be changed
    int            m_Unit;                              /* Numero de part */
    DrawSheetPath  m_SheetList;
    unsigned long  m_TimeStamp;                         /* unique identification number */
    int            m_IsNew;                             /* != 0 pour composants non annotes */
    char           m_TextValue[32];                     /* Valeur */
    char           m_TextRef[32];                       /* Reference ( hors numero ) */
    int            m_NumRef;                            /* Numero de reference */
    int            m_Flag;                              /* flag pour calculs internes */
    wxPoint        m_Pos;                               /* position components */
    char           m_Path[256];                         // the 'path' of the object in the sheet hierarchy.
};


/* Global Variables */
eda_global int g_NbrObjNet;
eda_global ObjetNetListStruct* g_TabObjNet;

/* Prototypes: */
void        WriteNetList( WinEDA_SchematicFrame* frame,
                          const wxString&        FileNameNL,
                          bool                   use_netnames );
void        FreeTabNetList( ObjetNetListStruct* TabNetItems, int NbrNetItems );

/** Function ReturnUserNetlistTypeName
 * to retrieve user netlist type names
 * @param first = true: return first name of the list, false = return next
 * @return a wxString : name of the type netlist or empty string
 * this function must be called first with "first_item" = true
 * and after with "first_item" = false to get all the other existing netlist names
 */
wxString    ReturnUserNetlistTypeName( bool first_item );


#endif
