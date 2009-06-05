/************************/
/* file class_equipot.h */
/************************/

/*
 *  Classes to handle info on nets
 */

#ifndef __CLASSES_NETINFO__
#define __CLASSES_NETINFO__

// Forward declaration:
class NETINFO_ITEM;


/* Class RATSNEST_ITEM: describes a ratsnest line: a straight line connecting 2 pads */
/*****************************/
/* flags for a RATSNEST_ITEM */
/*****************************/
#define CH_VISIBLE          1   /* affichage permanent demande */
#define CH_UNROUTABLE       2   /* non route par l'autorouteur */
#define CH_ROUTE_REQ        4   /* doit etre route par l'autorouteur */
#define CH_ACTIF            8   /* chevelu non encore routé */
#define LOCAL_RATSNEST_ITEM 0x8000    /* indique un chevelu reliant 2 pins d'un meme
                                       *  module pour le calcul des chevelus relatifs a 1 seul module */

class RATSNEST_ITEM
{
private:
    int    m_NetCode;   // netcode ( = 1.. n ,  0 is the value used for not connected items)

public:
    int    m_Status;        // State: see previous defines (CH_ ...)
    D_PAD* m_PadStart;      // pointer to the starting pad
    D_PAD* m_PadEnd;        // pointer to ending pad
    int    m_Lenght;        // lenght of the line (temporary used in some calculations)

    /* constructor */
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

    /** function Draw
     */
    void Draw( WinEDA_DrawPanel* panel, wxDC* DC, int aDrawMode, const wxPoint& offset );

};

/***************************************************************/
/******************* class NETINFO *****************************/
/***************************************************************/


class NETINFO_LIST
{
private:
    BOARD* m_Parent;

//    boost::ptr_vector<NETINFO_ITEM*>  m_NetBuffer;           // nets buffer list (name, design constraints ..
    std::vector<NETINFO_ITEM*> m_NetBuffer;            // nets buffer list (name, design constraints ..
public:
    NETINFO_LIST( BOARD* aParent );
    ~NETINFO_LIST();

    /** Function GetItem
     * @param aNetcode = netcode to identify a given NETINFO_ITEM
     * @return a NETINFO_ITEM pointer to the selected NETINFO_ITEM by its netcode, or NULL if not found
     */
    NETINFO_ITEM* GetItem( int aNetcode );

    /** Function GetCount()
     * @return the number of nets ( always >= 1 )
     * the first net is the "not connected" net
     */
    unsigned GetCount() { return m_NetBuffer.size(); }

    /**
     * Function Append
     * adds \a aNewElement to the end of the list.
     */
    void Append( NETINFO_ITEM* aNewElement );

    /** Function DeleteData
     * delete the list of nets (and free memory)
     */
    void DeleteData();

    /** Function BuildListOfNets
     * initialize the list of NETINFO_ITEM m_NetBuffer
     * The list is sorted by names.
     */
    void BuildListOfNets();
};

/** class NETINFO_ITEM
 * @info This class handle the data relative to a given net
 */

class NETINFO_ITEM
{
private:
    int      m_NetCode;         // this is a number equivalent to the net name
                                // Used for fast comparisons in rastnest and DRC computations.
    wxString m_Netname;         // Full net name like /mysheet/mysubsheet/vout used by eeschema
    wxString m_ShortNetname;    // short net name, like vout from /mysheet/mysubsheet/vout


public:
    int                          m_NbNodes;         // Pads count for this net
    int                          m_NbLink;          // Ratsnets count for this net
    int                          m_NbNoconn;        // Ratsnets remaining to route count
    int                          m_ForceWidth;      // specific width (O = default width)
    std::vector <D_PAD*>         m_ListPad;         // List of pads connected to this net
    unsigned                     m_RatsnestStart;   // debut de liste ratsnests du net (included)
    unsigned                     m_RatsnestEnd;     // fin de liste ratsnests du net (excluded)

    NETINFO_ITEM( BOARD_ITEM* aParent );
    ~NETINFO_ITEM();


    /* Readind and writing data on files */
    int  ReadDescr( FILE* File, int* LineNum );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;


    /** function Draw
     * @todo we actually could show a NET, simply show all the tracks and pads or net name on pad and vias
     */
    void Draw( WinEDA_DrawPanel* panel, wxDC* DC, int aDrawMode, const wxPoint& offset );


    /**
     * Function GetNet
     * @return int - the netcode
     */
    int GetNet() const { return m_NetCode; }
    void SetNet( int aNetCode ) { m_NetCode = aNetCode; }

    int GetNodesCount() const { return m_ListPad.size(); }

    /**
     * Function GetNetname
     * @return const wxString * , a pointer to the full netname
     */
    wxString GetNetname() const { return m_Netname; }

    /**
     * Function GetShortNetname
     * @return const wxString * , a pointer to the short netname
     */
    wxString GetShortNetname() const { return m_ShortNetname; }

    /**
     * Function SetNetname
     * @param const wxString : the new netname
     */
    void SetNetname( const wxString& aNetname );


/**
 * Function DisplayInfo
 * has knowledge about the frame and how and where to put status information
 * about this object into the frame's message panel.
 * Is virtual from EDA_BaseStruct.
 * @param frame A WinEDA_DrawFrame in which to print status information.
 */
    void DisplayInfo( WinEDA_DrawFrame* frame );
};




/****************************************************************/
/* description d'un point de piste pour le suivi des connexions */
/****************************************************************/
#define START_ON_PAD   0x10
#define END_ON_PAD     0x20
#define START_ON_TRACK 0x40
#define END_ON_TRACK   0x80


/* Status bit (OR'ed bits) for class BOARD member .m_Status_Pcb */
enum StatusPcbFlags {
    LISTE_PAD_OK = 1,                           /* Pad list is Ok */
    LISTE_RATSNEST_ITEM_OK = 2,                 /* General Rastnest is Ok */
    RATSNEST_ITEM_LOCAL_OK = 4,                 /* current MODULE rastnest is Ok */
    CONNEXION_OK = 8,                           /* Bit indicant que la liste des connexions existe */
    NET_CODES_OK = 0x10,    /* Bit indicant que les netcodes sont OK ( pas de modif
                             *  de noms de net */
    DO_NOT_SHOW_GENERAL_RASTNEST = 0x20         /* Do not display the general rastnest (used in module moves) */
};


#endif  // __CLASSES_NETINFO__
