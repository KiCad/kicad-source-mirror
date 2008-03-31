/*******************************************************/
/* class_module.h : module description (excepted pads) */
/*******************************************************/


class Pcb3D_GLCanvas;
class Struct3D_Master;

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

enum Mod_Attribut       /* Attributs d'un module */
{
    MOD_DEFAULT = 0,    /* Type default */
    MOD_CMS = 1,        /* Pour module apparaissant dans les
                         *  fichiers de placement automatique (principalement modules CMS */
    MOD_VIRTUAL = 2     /* Module virtuel constitue par un dessin sur circuit
                         *  (connecteur, trou de percage..) */
};


/* flags for autoplace and autoroute (.m_ModuleStatus member) */
#define MODULE_is_LOCKED    0x01    /* module LOCKED: no autoplace allowed */
#define MODULE_is_PLACED    0x02    /* In autoplace: module automatically placed */
#define MODULE_to_PLACE     0x04    /* In autoplace: module waiting for autoplace */

class MODULE : public BOARD_ITEM
{
public:
    wxPoint          m_Pos;             // Real coord on board
    D_PAD*           m_Pads;            /* Pad list (linked list) */
    BOARD_ITEM*      m_Drawings;        /* Graphic items list (linked list) */
    Struct3D_Master* m_3D_Drawings;     /* Pointeur sur la liste des elements de trace 3D*/
    TEXTE_MODULE*    m_Reference;       // texte reference du composant (U34, R18..)
    TEXTE_MODULE*    m_Value;           // texte valeur du composant (74LS00, 22K..)
    wxString         m_LibRef;          /* nom du module en librairie */
    wxString         m_AlternateReference;  /* Used when m_Reference cannot be used to
                                              * identify the footprint ( after a full reannotation of the schematic */

    int           m_Attributs;          /* Flags bits a bit ( voir enum Mod_Attribut ) */
    int           m_Orient;             /* orientation en 1/10 degres */
    int           flag;                 /* flag utilise en trace rastnest et routage auto */
    int           m_ModuleStatus;       /* For autoplace: flags (LOCKED, AUTOPLACED) */
    EDA_Rect      m_BoundaryBox;        /* position/taille du cadre de reperage (coord locales)*/
    EDA_Rect      m_RealBoundaryBox;    /* position/taille du module (coord relles) */
    int           m_PadNum;             // Nombre total de pads
    int           m_AltPadNum;          // en placement auto Nombre de pads actifs pour
                                        // les calculs

    int           m_CntRot90;           // Placement auto: cout ( 0..10 ) de la rotation 90 degre
    int           m_CntRot180;          // Placement auto: cout ( 0..10 ) de la rotation 180 degre
    wxSize        m_Ext;                // marges de "garde": utilise en placement auto.
    float         m_Surface;            // surface du rectangle d'encadrement

    unsigned long m_Link;               // variable temporaire ( pour editions, ...)
    long          m_LastEdit_Time;      // Date de la derniere modification du module (gestion de librairies)
    wxString      m_Path;

    wxString      m_Doc;                // Texte de description du module
    wxString      m_KeyWord;            // Liste des mots cles relatifs au module

public:
    MODULE( BOARD* parent );
    MODULE( MODULE* module );
    ~MODULE();

    void    Copy( MODULE* Module );     // Copy structure

    MODULE* Next()  { return (MODULE*) Pnext; }

    void    Set_Rectangle_Encadrement(); /* mise a jour du rect d'encadrement
                                         *  en coord locales (orient 0 et origine = pos  module) */

    void    SetRectangleExinscrit(); /* mise a jour du rect d'encadrement
                                     *   et de la surface en coord reelles */
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


    // deplacements
    void    SetPosition( const wxPoint& newpos );
    void    SetOrientation( int newangle );

    /* supprime du chainage la structure Struct */
    void    UnLink();


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
     * Read a footprint description in GPCB format
     * @param CmpFullFileName = Full file name (there is one footprint per file.
     * this is also the footprint name
     * @return bool - true if success reading else false.
     */
    bool        Read_GPCB_Descr(const wxString & CmpFullFileName);
    int     Read_3D_Descr( FILE* File, int* LineNum = NULL );

    /* drawing functions */
    /** Function Draw
     * Draw the text accordint to the footprint pos and orient
     * @param panel = draw panel, Used to know the clip box
     * @param DC = Current Device Context
     * @param offset = draw offset (usually wxPoint(0,0)
     * @param draw_mode = GR_OR, GR_XOR..
     */
    void    Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                  const wxPoint& offset, int draw_mode );
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
