/*******************************************************/
/* class_module.h : module description (excepted pads) */
/*******************************************************/


class Pcb3D_GLCanvas;
class S3D_MASTER;

/************************************/
/* Modules (footprints) description */
/* pad are in class_pad.xx          */
/************************************/

/* Format des modules:
 *  Description generale
 *  Description segments contour
 *  Description textes
 *  Description pastilles
 */

/* Flags :*/

enum Mod_Attribut       /* Attributs used for modules */
{
    MOD_DEFAULT = 0,    /* Type default */
    MOD_CMS = 1,        /* Set for modules listed in the automatic insertion list
                         *  (usually SMD footprints) */
    MOD_VIRTUAL = 2     /* Virtuel component: when created by copper shapes on board
                         *  (Like edge card connectors, mounting hole...) */
};


/* flags for autoplace and autoroute (.m_ModuleStatus member) */
#define MODULE_is_LOCKED    0x01    /* module LOCKED: no autoplace allowed */
#define MODULE_is_PLACED    0x02    /* In autoplace: module automatically placed */
#define MODULE_to_PLACE     0x04    /* In autoplace: module waiting for autoplace */

class MODULE : public BOARD_ITEM
{
public:
    wxPoint             m_Pos;             // Real coord on board
    DLIST<D_PAD>        m_Pads;            /* Pad list (linked list) */
    DLIST<BOARD_ITEM>   m_Drawings;        /* Graphic items list (linked list) */
    DLIST<S3D_MASTER>   m_3D_Drawings;     /* First item of the 3D shapes (linked list)*/
    TEXTE_MODULE*       m_Reference;       // Component reference (U34, R18..)
    TEXTE_MODULE*       m_Value;           // Component value (74LS00, 22K..)
    wxString            m_LibRef;          /* Name of the module in library (and the default value when loading amodule from the library) */
    wxString            m_AlternateReference;  /* Used when m_Reference cannot be used to
                                              * identify the footprint ( after a full reannotation of the schematic */

    int           m_Attributs;          /* Flags(ORed bits) ( see Mod_Attribut ) */
    int           m_Orient;             /* orientation in 0.1 degrees */
    int           flag;                 /* flag utilise en trace rastnest et routage auto */
    int           m_ModuleStatus;       /* For autoplace: flags (LOCKED, AUTOPLACED) */
    EDA_Rect      m_BoundaryBox;        /* Bounding box coordinates relatives to the anchor, orient 0*/
    EDA_Rect      m_RealBoundaryBox;    /* Bounding box : coordinates on board, real orientation */
    int           m_PadNum;             // Pad count
    int           m_AltPadNum;          // Pad with netcode > 0 (active pads)count

    int           m_CntRot90;           // Automatic placement : cost ( 0..10 ) for 90 degrees rotaion (Horiz<->Vertical)
    int           m_CntRot180;          // Automatic placement : cost ( 0..10 ) for 180 degrees rotaion (UP <->Down)
    wxSize        m_Ext;                // Automatic placement margin around the module
    float         m_Surface;            // Bounding box area

    unsigned long m_Link;               // Temporary variable ( used in editions, ...)
    long          m_LastEdit_Time;      // Date de la derniere modification du module (gestion de librairies)
    wxString      m_Path;

    wxString      m_Doc;                // Module Description (info for users)
    wxString      m_KeyWord;            // Keywords to select the module in lib

public:
    MODULE( BOARD* parent );
    MODULE( MODULE* module );
    ~MODULE();

    MODULE*    Next() const { return (MODULE*) Pnext; }
    MODULE*    Back() const { return (MODULE*) Pback; }

    void    Copy( MODULE* Module );     // Copy structure


    /**
     * Function Add
     * adds the given item to this MODULE and takes ownership of its memory.
     * @param aBoardItem The item to add to this board.
     * @param doInsert If true, then insert, else append
    void    Add( BOARD_ITEM* aBoardItem, bool doInsert = true );
     */


    /**
     * Function Set_Rectangle_Encadrement()
     * calculates the bounding box for orient 0 et origin = module anchor)
     */
    void    Set_Rectangle_Encadrement();

    /** function SetRectangleExinscrit()
     * Calculates the real bounding box accordint to theboard position, and real orientaion
     *  and also calculates the area value (used in automatic placement)
     */
    void    SetRectangleExinscrit();

    /**
     * Function GetBoundingBox
     * returns the bounding box of this Footprint
     * Mainly used to redraw the screen area occuped by the footprint
     */
    EDA_Rect GetBoundingBox();

    /**
     * Function GetPosition
     * returns the position of this object.
     * Required by pure virtual BOARD_ITEM::GetPosition()
     * @return const wxPoint& - The position of this object.
     */
    wxPoint& GetPosition()
    {
        return m_Pos;
    }


    // Moves
    void    SetPosition( const wxPoint& newpos );
    void    SetOrientation( int newangle );


    /**
     * Function IsLocked
     * (virtual from BOARD_ITEM )
     * @returns bool - true if the MODULE is locked, else false
     */
    bool IsLocked() const
    {
        return (m_ModuleStatus & MODULE_is_LOCKED) != 0;
    }


    /**
     * Function SetLocked
     * sets the MODULE_is_LOCKED bit in the m_ModuleStatus
     * @param setLocked When true means turn on locked status, else unlock
     */
    void SetLocked( bool setLocked )
    {
        if( setLocked )
            m_ModuleStatus |= MODULE_is_LOCKED;
        else
            m_ModuleStatus &= ~MODULE_is_LOCKED;
    }


    /* Reading and writing data on files */

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool    Save( FILE* aFile ) const;

    int     Write_3D_Descr( FILE* File ) const;
    int     ReadDescr( FILE* File, int* LineNum = NULL );

    /**
     * Function Read_GPCB_Descr
     * reads a footprint description in GPCB format (newlib version)
     * @param CmpFullFileName = Full file name (there is one footprint per file.
     * this is also the footprint name
     * @return bool - true if success reading else false.
     */
    bool    Read_GPCB_Descr(const wxString & CmpFullFileName);
    int     Read_3D_Descr( FILE* File, int* LineNum = NULL );

    /* drawing functions */
    /** Function Draw
     * Draw the text accordint to the footprint pos and orient
     * @param panel = draw panel, Used to know the clip box
     * @param DC = Current Device Context
     * @param offset = draw offset (usually wxPoint(0,0)
     * @param aDrawMode = GR_OR, GR_XOR..
     */
    void    Draw( WinEDA_DrawPanel* panel, wxDC* DC, int aDrawMode, const wxPoint& offset = ZeroOffset );

    void    Draw3D( Pcb3D_GLCanvas* glcanvas );
    void    DrawEdgesOnly( WinEDA_DrawPanel* panel, wxDC* DC,
                           const wxPoint& offset, int draw_mode );
    void    DrawAncre( WinEDA_DrawPanel* panel, wxDC* DC,
                       const wxPoint& offset, int dim_ancre, int draw_mode );

    /**
     * Function Display_Infos
     * has knowledge about the frame and how and where to put status information
     * about this object into the frame's message panel.
     * @param frame A WinEDA_DrawFrame in which to print status information.
     */
    void    Display_Infos( WinEDA_DrawFrame* frame );


    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param refPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool    HitTest( const wxPoint& refPos );


    /**
     * Function HitTest (overlayed)
     * tests if the given EDA_Rect intersect the bounds of this object.
     * @param refArea : the given EDA_Rect
     * @return bool - true if a hit, else false
     */
    bool    HitTest( EDA_Rect& refArea );

    /**
     * Function GetReference
     * @return const wxString& - the reference designator text.
     */
    const wxString& GetReference()
    {
        return m_Reference->m_Text;
    }

    /**
     * Function GetValue
     * @return const wxString& - the value text.
     */
    const wxString& GetValue()
    {
        return m_Value->m_Text;
    }


    /**
     * Function FindPadByName
     * returns a D_PAD* with a matching name.  Note that names may not be
     * unique, depending on how the foot print was created.
     * @param
     * @return D_PAD* - The first matching name is returned, or NULL if not found.
     */
    D_PAD* FindPadByName( const wxString& aPadName ) const;


    /**
     * Function Visit
     * should be re-implemented for each derived class in order to handle
     * all the types given by its member data.  Implementations should call
     * inspector->Inspect() on types in scanTypes[], and may use IterateForward()
     * to do so on lists of such data.
     * @param inspector An INSPECTOR instance to use in the inspection.
     * @param testData Arbitrary data used by the inspector.
     * @param scanTypes Which KICAD_T types are of interest and the order
     *  is significant too, terminated by EOT.
     * @return SEARCH_RESULT - SEARCH_QUIT if the Iterator is to stop the scan,
     *   else SCAN_CONTINUE;
     */
    SEARCH_RESULT Visit( INSPECTOR* inspector, const void* testData,
                         const KICAD_T scanTypes[] );


    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    virtual wxString GetClass() const
    {
        return wxT( "MODULE" );
    }


 #if defined (DEBUG)

    /**
     * Function Show
     * is used to output the object tree, currently for debugging only.
     * @param nestLevel An aid to prettier tree indenting, and is the level
     *          of nesting of this object within the overall tree.
     * @param os The ostream& to output to.
     */
    virtual void Show( int nestLevel, std::ostream& os );

#endif
};
