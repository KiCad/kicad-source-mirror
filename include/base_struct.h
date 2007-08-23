/*********************************************************************/
/*	base_struct.h :  Basic classes for most kicad item descriptions  */
/*********************************************************************/

#ifndef BASE_STRUCT_H
#define BASE_STRUCT_H


#if defined(DEBUG)
#include <iostream>         // needed for Show()
extern std::ostream& operator<<( std::ostream& out, const wxSize& size );
extern std::ostream& operator<<( std::ostream& out, const wxPoint& pt );
#endif


/* Id for class identification, at run time */
enum DrawStructureType {

    EOT = 0,                // search types array terminator (End Of Types)
    
    TYPE_NOT_INIT = 0,
    TYPEPCB,

    // Items in pcb
    PCB_EQUIPOT_STRUCT_TYPE,
    TYPEMODULE,
    TYPEPAD,
    TYPEDRAWSEGMENT,
    TYPETEXTE,
    TYPETEXTEMODULE,
    TYPEEDGEMODULE,
    TYPETRACK,
    TYPEZONE,
    TYPEVIA,
    TYPEMARQUEUR,
    TYPECOTATION,
    TYPEMIRE,
    TYPESCREEN,
    TYPEBLOCK,
    TYPEEDGEZONE,

    // Draw Items in schematic
    DRAW_POLYLINE_STRUCT_TYPE,
    DRAW_JUNCTION_STRUCT_TYPE,
    DRAW_TEXT_STRUCT_TYPE,
    DRAW_LABEL_STRUCT_TYPE,
    DRAW_GLOBAL_LABEL_STRUCT_TYPE,
    DRAW_LIB_ITEM_STRUCT_TYPE,
    DRAW_PICK_ITEM_STRUCT_TYPE,
    DRAW_SEGMENT_STRUCT_TYPE,
    DRAW_BUSENTRY_STRUCT_TYPE,
    DRAW_SHEET_STRUCT_TYPE,
    DRAW_SHEETLABEL_STRUCT_TYPE,
    DRAW_MARKER_STRUCT_TYPE,
    DRAW_NOCONNECT_STRUCT_TYPE,
    DRAW_PART_TEXT_STRUCT_TYPE,

    // General
    SCREEN_STRUCT_TYPE,
    BLOCK_LOCATE_STRUCT_TYPE,

    // Draw Items in library component
    LIBCOMPONENT_STRUCT_TYPE,
    COMPONENT_ARC_DRAW_TYPE,
    COMPONENT_CIRCLE_DRAW_TYPE,
    COMPONENT_GRAPHIC_TEXT_DRAW_TYPE,
    COMPONENT_RECT_DRAW_TYPE,
    COMPONENT_POLYLINE_DRAW_TYPE,
    COMPONENT_LINE_DRAW_TYPE,
    COMPONENT_PIN_DRAW_TYPE,
    COMPONENT_FIELD_DRAW_TYPE,

    // End value
    MAX_STRUCT_TYPE_ID
};


enum SEARCH_RESULT {
    SEARCH_QUIT,
    SEARCH_CONTINUE
};    

typedef DrawStructureType   KICAD_T;    // shorter name

class EDA_BaseStruct;
class WinEDA_DrawFrame;

/**
 * Class INSPECTOR
 * is an abstract class that is used to inspect and possibly collect the 
 * (search) results of Iterating over a list or tree of KICAD_T objects.
 * Extend from this class and implement the Inspect function and provide for
 * a way for the extension to collect the results of the search/scan data and
 * provide them to the caller.
 */
class INSPECTOR
{
    
public:
    virtual ~INSPECTOR()
    {
    }

    
    /**
     * Function Inspect
     * is the examining function within the INSPECTOR which is passed to the 
     * Iterate function.  It is used primarily for searching, but not limited to
     * that.  It can also collect or modify the scanned objects.
     *
     * @param testItem An EDA_BaseStruct to examine.
     * @param testData is arbitrary data needed by the inspector to determine
     *   if the BOARD_ITEM under test meets its match criteria.
     * @return SEARCH_RESULT - SEARCH_QUIT if the Iterator is to stop the scan,
     *   else SCAN_CONTINUE;
     */ 
    SEARCH_RESULT virtual Inspect( EDA_BaseStruct* testItem, 
        const void* testData ) = 0;
};


/********************************************************************/
/* Classes de base: servent a deriver les classes reellement utiles */
/********************************************************************/

class EDA_BaseStruct        /* Basic class, not directly used */
{
public:
    int             m_StructType;       /* Struct ident for run time identification */
    EDA_BaseStruct* Pnext;              /* Linked list: Link (next struct) */
    EDA_BaseStruct* Pback;              /* Linked list: Link (previous struct) */
    EDA_BaseStruct* m_Parent;           /* Linked list: Link (parent struct) */
    EDA_BaseStruct* m_Son;              /* Linked list: Link (son struct) */
    EDA_BaseStruct* m_Image;            /* Link to an image copy for undelete or abort command */

    int             m_Flags;            // flags for editions and other
#define IS_CHANGED      (1<<0)    
#define IS_LINKED       (1<<1)
#define IN_EDIT         (1<<2)
#define IS_MOVED        (1<<3)
#define IS_NEW          (1<<4)
#define IS_RESIZED      (1<<5)
#define IS_DRAGGED      (1<<6)
#define IS_DELETED      (1<<7)
#define IS_WIRE_IMAGE   (1<<8)
#define STARTPOINT      (1<<9)
#define ENDPOINT        (1<<10)
#define SELECTED        (1<<11)
#define SELECTEDNODE    (1<<12)         ///< flag indiquant que la structure a deja selectionnee
#define STRUCT_DELETED  (1<<13)         ///< Bit flag de Status pour structures effacee
#define CANDIDATE       (1<<14)         ///< flag indiquant que la structure est connectee
#define SKIP_STRUCT     (1<<15)         ///< flag indiquant que la structure ne doit pas etre traitee


    unsigned long   m_TimeStamp;        // Time stamp used for logical links
    int             m_Selected;         /* Used by block commands, and selective editing */
//    int             m_Layer;            ///< used by many derived classes, so make common

private:
    int             m_Status;

private:
    void InitVars( void );

public:

    EDA_BaseStruct( EDA_BaseStruct* parent, int idType );
    EDA_BaseStruct( int struct_type );
    virtual ~EDA_BaseStruct() { };
    
    EDA_BaseStruct* Next( void ) { return Pnext; }
    
    /* Gestion de l'etat (status) de la structure (active, deleted..) */
    int     GetState( int type );
    void    SetState( int type, int state );

    int ReturnStatus( void ) const
    {
        return m_Status;
    }

    void SetStatus( int new_status )
    {
        m_Status = new_status;
    }

    wxString        ReturnClassName() const;

    /* addition d'une nouvelle struct a la liste chain� */
    void            AddToChain( EDA_BaseStruct* laststruct );

    /* fonction de placement */
    virtual void    Place( WinEDA_DrawFrame* frame, wxDC* DC );

    virtual void    Draw( WinEDA_DrawPanel* panel,
                          wxDC*             DC,
                          const wxPoint&    offset,
                          int               draw_mode,
                          int               Color = -1 );

    
    /**
     * Function Display_Infos
     * has knowledge about the frame and how and where to put status information
     * about this object into the frame's message panel.
     * @param frame A WinEDA_DrawFrame in which to print status information.
     */ 
    virtual void    Display_Infos( WinEDA_DrawFrame* frame )
    {
        // derived classes may implement this 
    }
    
    
    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param refPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    virtual bool    HitTest( const wxPoint& refPos )
    {
        return false;   // derived classes should override this function
    }


    /**
     * Function IterateForward
     * walks through the object tree calling the testFunc on each object 
     * type requested in scanTypes.
     *
     * @param listStart The first in a list of EDA_BaseStructs to iterate over. 
     * @param inspector Is an INSPECTOR to call on each object that is one of 
     *  the requested scanTypes.
     * @param testData Is an aid to testFunc, and should be sufficient to 
     *  allow it to fully determine if an item meets the match criteria, but it
     *  may also be used to collect output.
     * @param scanTypes A KICAD_T array that is EOT 
     *  terminated, and provides both the order and interest level of of
     *  the types of objects to be iterated over.
     * @return SEARCH_RESULT - SEARCH_QUIT if the called INSPECTOR returned 
     *  SEARCH_QUIT, else SCAN_CONTINUE;
     */
    static SEARCH_RESULT IterateForward( EDA_BaseStruct* listStart, 
        INSPECTOR* inspector, const void* testData, const KICAD_T scanTypes[] );

    
    /**
     * Function Visit
     * may be re-implemented for each derived class in order to handle
     * all the types given by its member data.  Implementations should call
     * inspector->Inspect() on types in scanTypes[], and may use IterateForward()
     * to do so on lists of such data.
     * @param inspector An INSPECTOR instance to use in the inspection.
     * @param testData Arbitrary data used by the inspector.
     * @param scanTypes Which KICAD_T types are of interest and the order 
     *  is significant too, terminated by EOT.
     * @return SEARCH_RESULT - SEARCH_QUIT if the Iterator is to stop the scan,
     *  else SCAN_CONTINUE, and determined by the inspector.
     */
    virtual SEARCH_RESULT Visit( INSPECTOR* inspector, const void* testData, 
        const KICAD_T scanTypes[] );

    
#if defined(DEBUG)

    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    virtual wxString GetClass() const
    {
        // ReturnClassName() is too hard to maintain, coordinating the array 
        // with the enum.  It would be nice to migrate to virtual GetClass()
        // away from ReturnClassName().   Over time, derived classes should
        // simply return a wxString from their virtual GetClass() function.
        // Some classes do that now.
        return ReturnClassName();
    }

    
    /**
     * Function Show
     * is used to output the object tree, currently for debugging only.
     * @param nestLevel An aid to prettier tree indenting, and is the level 
     *          of nesting of this object within the overall tree.
     * @param os The ostream& to output to.
     */
    virtual void Show( int nestLevel, std::ostream& os );

    
    /** 
     * Function NestedSpace
     * outputs nested space for pretty indenting.
     * @param nestLevel The nest count
     * @param os The ostream&, where to output
     * @return std::ostream& - for continuation.
     **/
    static std::ostream& NestedSpace( int nestLevel, std::ostream& os );

#endif

};


// Text justify:
// Values -1,0,1 are used in computations, do not change them
typedef enum {
    GR_TEXT_HJUSTIFY_LEFT   = -1,
    GR_TEXT_HJUSTIFY_CENTER = 0,
    GR_TEXT_HJUSTIFY_RIGHT  = 1
} GRTextHorizJustifyType;


typedef enum {
    GR_TEXT_VJUSTIFY_TOP    = -1,
    GR_TEXT_VJUSTIFY_CENTER = 0,
    GR_TEXT_VJUSTIFY_BOTTOM = 1
} GRTextVertJustifyType;


/* controle des remplissages a l'ecran (Segments textes...)*/
#define FILAIRE  0
#define FILLED   1
#define SKETCH   2


#define DEFAULT_SIZE_TEXT 60        /* default text height (in mils or  1/1000") */

/* classe de gestion des textes (labels, textes composants ..)
 *  (Non utilisee seule) */
class EDA_TextStruct
{
public:
    wxString m_Text;                    /* text! */
    wxPoint  m_Pos;                     /* XY position of anchor text. */
    wxSize   m_Size;                    /* XY size of text */
    int      m_Width;                   /* epaisseur du trait */
    int      m_Orient;                  /* Orient in 0.1 degrees */
    int      m_Miroir;                  // Display Normal / mirror
    int      m_Attributs;               /* controle visibilite */
    int      m_CharType;                /* normal, bold, italic ... */
    int      m_HJustify, m_VJustify;    /* Justifications Horiz et Vert du texte */
    int      m_ZoomLevelDrawable;       /* Niveau de zoom acceptable pour affichage normal */
    int*     m_TextDrawings;            /* pointeur sur la liste des segments de dessin */
    int      m_TextDrawingsSize;        /* nombre de segments a dessiner */

public:
    EDA_TextStruct( const wxString& text = wxEmptyString );
    virtual ~EDA_TextStruct( void );
    void    CreateDrawData( void );

    int     GetLength( void ) { return m_Text.Length(); };
    int     Pitch( void );/* retourne le pas entre 2 caracteres */
    void    Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                  const wxPoint& offset, int color,
                  int draw_mode, int display_mode = FILAIRE, int anchor_color = -1 );

    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param ref_pos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool    HitTest( const wxPoint& ref_pos );
    
    int     Len_Size( void ); // Return the text lenght in internal units
};


/**
 * Class BOARD_ITEM
 * is an abstract base class for any item which can be embedded within the BOARD
 * container class, and therefore instances of derived classes should only be 
 * found in PCBNEW or other programs that use class BOARD and its contents. 
 * The corresponding class in EESCHEMA seems to be DrawPartStruct.
 */
class BOARD_ITEM : public EDA_BaseStruct
{
protected:
    int     m_Layer;

public:

    BOARD_ITEM( BOARD_ITEM* StructFather, DrawStructureType idtype ) :
        EDA_BaseStruct( StructFather, idtype ),
        m_Layer(0)
    {
    }

    BOARD_ITEM( const BOARD_ITEM& src ) :
        EDA_BaseStruct( src.m_Parent, src.m_StructType ),
        m_Layer( src.m_Layer )
    {
    }

    BOARD_ITEM* Next()      { return (BOARD_ITEM*) Pnext; }

    
    /**
     * Function GetLayer
     * returns the layer this item is on.
     */
    int         GetLayer() const  { return m_Layer; }
    
    /**
     * Function SetLayer
     * sets the layer this item is on.
     * @param aLayer The layer number.
     */
    void        SetLayer( int aLayer )  { m_Layer = aLayer; }
    
        
};


/* Base class for building items like lines, which have 1 start point and 1 end point.
 * Arc and circles can use this class.
 */
class EDA_BaseLineStruct : public EDA_BaseStruct
{
public:
    int     m_Layer;            // Layer number
    int     m_Width;            // 0 = line, > 0 = tracks, bus ...
    wxPoint m_Start;            // Line start point
    wxPoint m_End;              // Line end point

public:
    EDA_BaseLineStruct( EDA_BaseStruct* StructFather, DrawStructureType idtype );
};


/**************************/
/* class DrawPickedStruct */
/**************************/

/* Class to hold structures picked by pick events (like block selection)
 *  This class has only one useful member: .m_PickedStruct, used as a link.
 *  It does not describe really an item.
 *  It is used to create a linked list of selected items (in block selection).
 *  Each DrawPickedStruct item has is member: .m_PickedStruct pointing the
 *  real selected item
 */
class DrawPickedStruct : public EDA_BaseStruct
{
public:
    EDA_BaseStruct* m_PickedStruct;

public:
    DrawPickedStruct( EDA_BaseStruct* pickedstruct = NULL );
    ~DrawPickedStruct( void );
    void Place( WinEDA_DrawFrame* frame, wxDC* DC ) { };
    void DeleteWrapperList( void );

    DrawPickedStruct* Next( void ) { return (DrawPickedStruct*) Pnext; }
};


/**
 * Class EDA_Rect
 * handles the component boundary box.
 * This class is similar to wxRect, but some wxRect functions are very curious,
 * so I prefer this suitable class
 */
class EDA_Rect
{
public:
    wxPoint m_Pos;      // Rectangle Origin
    wxSize  m_Size;     // Rectangle Size

public:
    EDA_Rect( void ) { };
    wxPoint Centre( void )
    {
        return wxPoint( m_Pos.x + (m_Size.x >> 1), m_Pos.y + (m_Size.y >> 1) );
    }


    void    Normalize( void );              // Ensure the height ant width are >= 0
    bool    Inside( const wxPoint& point ); // Return TRUE if point is in Rect

    bool Inside( int x, int y ) { return Inside( wxPoint( x, y ) ); }
    wxSize GetSize( void ) { return m_Size; }
    int GetX( void ) { return m_Pos.x; }
    int GetY( void ) { return m_Pos.y; }
    wxPoint GetOrigin( void ) { return m_Pos; }
    wxPoint GetPosition( void ) { return m_Pos; }
    wxPoint GetEnd( void ) { return wxPoint( GetRight(), GetBottom() ); }
    int GetWidth( void ) { return m_Size.x; }
    int GetHeight( void ) { return m_Size.y; }
    int GetRight( void ) { return m_Pos.x + m_Size.x; }
    int GetBottom( void ) { return m_Pos.y + m_Size.y; }
    void SetOrigin( const wxPoint& pos ) { m_Pos = pos; }
    void SetOrigin( int x, int y ) { m_Pos.x = x; m_Pos.y = y; }
    void SetSize( const wxSize& size ) { m_Size = size; }
    void SetSize( int w, int h ) { m_Size.x = w; m_Size.y = h; }
    void Offset( int dx, int dy ) { m_Pos.x += dx; m_Pos.y += dy; }
    void Offset( const wxPoint& offset ) { m_Pos.x += offset.x; m_Pos.y += offset.y; }
    void SetX( int val ) { m_Pos.x = val; }
    void SetY( int val ) { m_Pos.y = val; }
    void SetWidth( int val ) { m_Size.x = val; }
    void SetHeight( int val ) { m_Size.y = val; }
    void SetEnd( const wxPoint& pos )
    {
        m_Size.x = pos.x - m_Pos.x; m_Size.y = pos.y - m_Pos.y;
    }


    EDA_Rect& Inflate( wxCoord dx, wxCoord dy );
};

#endif /* BASE_STRUCT_H */
