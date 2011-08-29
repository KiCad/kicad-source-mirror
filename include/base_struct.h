/*********************************************************************/
/*  base_struct.h :  Basic classes for most kicad item descriptions  */
/*********************************************************************/

#ifndef BASE_STRUCT_H
#define BASE_STRUCT_H

#include "colors.h"
#include "bitmaps.h"

#include <boost/ptr_container/ptr_vector.hpp>

#if defined(DEBUG)
#include <iostream>         // needed for Show()
extern std::ostream& operator <<( std::ostream& out, const wxSize& size );

extern std::ostream& operator <<( std::ostream& out, const wxPoint& pt );
#endif


/* Id for class identification, at run time */
enum KICAD_T {
    NOT_USED = -1,          // the 3d code uses this value

    EOT = 0,                // search types array terminator (End Of Types)

    TYPE_NOT_INIT = 0,
    TYPE_PCB,
    TYPE_SCREEN,            // not really an item, used to identify a screen

    // Items in pcb
    TYPE_MODULE,            // a footprint
    TYPE_PAD,               // a pad in a footprint
    TYPE_DRAWSEGMENT,       // a segment not on copper layers
    TYPE_TEXTE,             // a text on a layer
    TYPE_TEXTE_MODULE,      // a text in a footprint
    TYPE_EDGE_MODULE,       // a footprint edge
    TYPE_TRACK,             // a track segment (segment on a copper layer)
    TYPE_VIA,               // a via (like atrack segment on a copper layer)
    TYPE_ZONE,              // a segment used to fill a zone area (segment on a
                            // copper layer)
    TYPE_MARKER_PCB,        // a marker used to show something
    TYPE_DIMENSION,         // a dimension (graphic item)
    TYPE_MIRE,              // a target (graphic item)
    TYPE_ZONE_EDGE_CORNER,  // in zone outline: a point to define an outline
    TYPE_ZONE_CONTAINER,    // a zone area
    TYPE_BOARD_ITEM_LIST,   // a list of board items

    // Schematic draw Items.  The order of these items effects the sort order.
    // It is currenlty ordered to mimic the old EESchema locate behavior where
    // the smallest item is the selected item.
    SCH_MARKER_T,
    SCH_JUNCTION_T,
    SCH_NO_CONNECT_T,
    SCH_BUS_ENTRY_T,
    SCH_LINE_T,
    SCH_POLYLINE_T,
    SCH_TEXT_T,
    SCH_LABEL_T,
    SCH_GLOBAL_LABEL_T,
    SCH_HIERARCHICAL_LABEL_T,
    SCH_FIELD_T,
    SCH_COMPONENT_T,
    SCH_SHEET_PIN_T,
    SCH_SHEET_T,

    // General
    SCH_SCREEN_T,
    BLOCK_LOCATE_STRUCT_TYPE,

    /*
     * Draw items in library component.
     *
     * The order of these items effects the sort order for items inside the
     * "DRAW/ENDDRAW" section of the component definition in a library file.
     * If you add a new draw item, type, please make sure you add it so the
     * sort order is logical.
     */
    LIB_COMPONENT_T,
    LIB_ALIAS_T,
    LIB_ARC_T,
    LIB_CIRCLE_T,
    LIB_TEXT_T,
    LIB_RECTANGLE_T,
    LIB_POLYLINE_T,
    LIB_BEZIER_T,
    LIB_PIN_T,

    /*
     * Fields are not saved inside the "DRAW/ENDDRAW".  Add new draw item
     * types before this line.
     */
    LIB_FIELD_T,

    /*
     * For Gerbview: items type:
     */
    TYPE_GERBER_DRAW_ITEM,

    // End value
    MAX_STRUCT_TYPE_ID
};


enum SEARCH_RESULT {
    SEARCH_QUIT,
    SEARCH_CONTINUE
};


class EDA_ITEM;
class EDA_DRAW_FRAME;
class BOARD;
class EDA_RECT;
class EDA_DRAW_PANEL;

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
     * Iterate function.  It is used primarily for searching, but not limited
     *to
     * that.  It can also collect or modify the scanned objects.
     *
     * @param testItem An EDA_ITEM to examine.
     * @param testData is arbitrary data needed by the inspector to determine
     *   if the BOARD_ITEM under test meets its match criteria.
     * @return SEARCH_RESULT - SEARCH_QUIT if the Iterator is to stop the scan,
     *   else SCAN_CONTINUE;
     */
    SEARCH_RESULT virtual Inspect( EDA_ITEM* testItem, const void* testData ) = 0;
};


/**
 * Class EDA_RECT
 * handles the component boundary box.
 * This class is similar to wxRect, but some wxRect functions are very curious,
 * and are working only if dimensions are >= 0 (not always the case in kicad)
 * and also kicad needs some specific method.
 * so I prefer this more suitable class
 */
class EDA_RECT
{
public:
    wxPoint m_Pos;      // Rectangle Origin
    wxSize  m_Size;     // Rectangle Size

public:
    EDA_RECT() { };

    EDA_RECT( const wxPoint& aPos, const wxSize& aSize ) :
        m_Pos( aPos ),
        m_Size( aSize )
    { }

    wxPoint Centre()
    {
        return wxPoint( m_Pos.x + ( m_Size.x >> 1 ),
                        m_Pos.y + ( m_Size.y >> 1 ) );
    }

    /**
     * Function Move
     * moves the rectangle by the \a aMoveVector.
     * @param aMoveVector A wxPoint that is the value to move this rectangle
     */
    void Move( const wxPoint& aMoveVector );

    /**
     * Function Normalize
     * ensures thatthe height ant width are positive.
     */
    void Normalize();

    /**
     * Function Contains
     * @param aPoint = the wxPoint to test
     * @return true if aPoint is inside the boundary box. A point on a edge is seen as inside
     */
    bool Contains( const wxPoint& aPoint ) const;
    /**
     * Function Contains
     * @param x = the x coordinate of the point to test
     * @param y = the x coordinate of the point to test
     * @return true if point is inside the boundary box. A point on a edge is seen as inside
     */
    bool Contains( int x, int y ) const { return Contains( wxPoint( x, y ) ); }

    /**
     * Function Contains
     * @param aRect = the EDA_RECT to test
     * @return true if aRect is Contained. A common edge is seen as contained
     */
    bool Contains( const EDA_RECT& aRect ) const;

    wxSize GetSize() const { return m_Size; }
    int GetX() const { return m_Pos.x; }
    int GetY() const { return m_Pos.y; }
    wxPoint GetOrigin() const { return m_Pos; }
    wxPoint GetPosition() const { return m_Pos; }
    wxPoint GetEnd() const { return wxPoint( GetRight(), GetBottom() ); }
    int GetWidth() const { return m_Size.x; }
    int GetHeight() const { return m_Size.y; }
    int GetRight() const { return m_Pos.x + m_Size.x; }
    int GetBottom() const { return m_Pos.y + m_Size.y; }
    void SetOrigin( const wxPoint& pos ) { m_Pos = pos; }
    void SetOrigin( int x, int y ) { m_Pos.x = x; m_Pos.y = y; }
    void SetSize( const wxSize& size ) { m_Size = size; }
    void SetSize( int w, int h ) { m_Size.x = w; m_Size.y = h; }
    void Offset( int dx, int dy ) { m_Pos.x += dx; m_Pos.y += dy; }
    void Offset( const wxPoint& offset ) { m_Pos.x += offset.x; m_Pos.y +=
                                               offset.y; }
    void SetX( int val ) { m_Pos.x = val; }
    void SetY( int val ) { m_Pos.y = val; }
    void SetWidth( int val ) { m_Size.x = val; }
    void SetHeight( int val ) { m_Size.y = val; }
    void SetEnd( int x, int y ) { SetEnd( wxPoint( x, y ) ); }
    void SetEnd( const wxPoint& pos )
    {
        m_Size.x = pos.x - m_Pos.x; m_Size.y = pos.y - m_Pos.y;
    }


    /**
     * Function Intersects
     * @return bool - true if the argument rectangle intersects this rectangle.
     * (i.e. if the 2 rectangles have at least a common point)
     */
    bool Intersects( const EDA_RECT& aRect ) const;

    /**
     * Function operator(wxRect)
     * overloads the cast operator to return a wxRect
     */
    operator wxRect() const { return wxRect( m_Pos, m_Size ); }

    /**
     * Function Inflate
     * inflates the rectangle horizontally by \a dx and vertically by \a dy. If \a dx
     * and/or \a dy is negative the rectangle is deflated.
     */
    EDA_RECT& Inflate( wxCoord dx, wxCoord dy );

    /**
     * Function Inflate
     * inflates the rectangle horizontally and vertically by \a aDelta. If \a aDelta
     * is negative the rectangle is deflated.
     */
    EDA_RECT& Inflate( int aDelta );

    /**
     * Function Merge
     * modifies the position and size of the rectangle in order to contain \a aRect.  It is
     * mainly used to calculate bounding boxes.
     * @param aRect  The rectangle to merge with this rectangle.
     */
    void Merge( const EDA_RECT& aRect );

    /**
     * Function Merge
     * modifies the position and size of the rectangle in order to contain the given point.
     * @param aPoint The point to merge with the rectangle.
     */
    void Merge( const wxPoint& aPoint );

    /**
     * Function GetArea
     * returns the area of the rectangle.
     * @return The area of the rectangle.
     */
    double GetArea() const;
};


/******************************************************/
/* Basic Classes : used classes are derived from them */
/******************************************************/

class DHEAD;

/**
 * Class EDA_ITEM
 * is a base class for most all the kicad significant classes, used in
 * schematics and boards.
 */

// These define are used for the .m_Flags and .m_UndoRedoStatus member of the
// class EDA_ITEM
#define IS_CHANGED     (1 << 0)   ///< Item was edited, and modified
#define IS_LINKED      (1 << 1)   ///< Used in calculation to mark linked items (temporary use)
#define IN_EDIT        (1 << 2)   ///< Item currently edited
#define IS_MOVED       (1 << 3)   ///< Item being moved
#define IS_NEW         (1 << 4)   ///< New item, just created
#define IS_RESIZED     (1 << 5)   ///< Item being resized
#define IS_DRAGGED     (1 << 6)   ///< Item being dragged
#define IS_DELETED     (1 << 7)
#define IS_WIRE_IMAGE  (1 << 8)
#define STARTPOINT     (1 << 9)
#define ENDPOINT       (1 << 10)
#define SELECTED       (1 << 11)
#define SELECTEDNODE   (1 << 12)   ///< flag indicating that the structure has already selected
#define STRUCT_DELETED (1 << 13)   ///< flag indication structures to be erased
#define CANDIDATE      (1 << 14)   ///< flag indicating that the structure is connected
#define SKIP_STRUCT    (1 << 15)   ///< flag indicating that the structure should be ignored
#define DO_NOT_DRAW    (1 << 16)   ///< Used to disable draw function
#define IS_CANCELLED   (1 << 17)   ///< flag set when edit dialogs are canceled when editing a
                                   ///< new object
#define TRACK_LOCKED   (1 << 18)   ///< Pcbnew: track locked: protected from global deletion
#define TRACK_AR       (1 << 19)   ///< Pcbnew: autorouted track
#define FLAG1          (1 << 20)   ///< Pcbnew: flag used in local computations
#define FLAG0          (1 << 21)   ///< Pcbnew: flag used in local computations
#define BEGIN_ONPAD    (1 << 22)   ///< Pcbnew: flag set for track segment starting on a pad
#define END_ONPAD      (1 << 23)   ///< Pcbnew: flag set for track segment ending on a pad
#define BUSY           (1 << 24)   ///< Pcbnew: flag indicating that the structure has
                                   // already been edited, in some functions
#define EDA_ITEM_ALL_FLAGS -1


class EDA_ITEM
{
private:

    /**
     * Run time identification, _keep private_ so it can never be changed after
     * a constructor sets it.  See comment near SetType() regarding virtual
     * functions.
     */
    KICAD_T       m_StructType;
    int           m_Status;

protected:
    EDA_ITEM*     Pnext;          /* Linked list: Link (next struct) */
    EDA_ITEM*     Pback;          /* Linked list: Link (previous struct) */
    EDA_ITEM*     m_Parent;       /* Linked list: Link (parent struct) */
    EDA_ITEM*     m_Son;          /* Linked list: Link (son struct) */
    DHEAD*        m_List;         ///< which DLIST I am on.

public:
    int           m_Flags;        // flags for editing and other uses.

    unsigned long m_TimeStamp;    // Time stamp used for logical links
    int           m_Selected;     /* Used by block commands, and selective
                                     * editing */

    // member used in undo/redo function
    EDA_ITEM*     m_Image;        // Link to an image copy to save a copy of
                                  // old parameters values
private:
    void InitVars();

    /**
     * @brief Function doClone
     * is used by the derived class to actually implement the cloning.
     *
     * The default version will return NULL in release builds and likely crash the
     * program.  In debug builds, an warning message indicating the derived class
     * has not implemented cloning.  This really should be a pure virtual function.
     * Due to the fact that there are so many objects derived from EDA_ITEM, the
     * decision was made to return NULL until all the objects derived from EDA_ITEM
     * implement cloning.  Once that happens, this function should be made pure.
     *
     * @return A clone of the item.
     */
    virtual EDA_ITEM* doClone() const;

public:

    EDA_ITEM( EDA_ITEM* parent, KICAD_T idType );
    EDA_ITEM( KICAD_T idType );
    EDA_ITEM( const EDA_ITEM& base );
    virtual ~EDA_ITEM() { };

    /**
     * Function Type
     * returns the type of object.  This attribute should never be changed
     * after a constructor sets it, so there is no public "setter" method.
     * @return KICAD_T - the type of object.
     */
    KICAD_T Type()  const { return m_StructType; }


    EDA_ITEM* Next() const { return (EDA_ITEM*) Pnext; }
    EDA_ITEM* Back() const { return (EDA_ITEM*) Pback; }
    EDA_ITEM* GetParent() const { return m_Parent; }
    EDA_ITEM* GetSon() const { return m_Son; }
    DHEAD* GetList() const { return m_List; }

    void SetNext( EDA_ITEM* aNext )       { Pnext = aNext; }
    void SetBack( EDA_ITEM* aBack )       { Pback = aBack; }
    void SetParent( EDA_ITEM* aParent )   { m_Parent = aParent; }
    void SetSon( EDA_ITEM* aSon )         { m_Son = aSon; }
    void SetList( DHEAD* aList )          { m_List = aList; }

    inline bool IsNew() const { return m_Flags & IS_NEW; }
    inline bool IsModified() const { return m_Flags & IS_CHANGED; }
    inline bool IsMoving() const { return m_Flags & IS_MOVED; }
    inline bool IsDragging() const { return m_Flags & IS_DRAGGED; }
    inline bool IsSelected() const { return m_Flags & SELECTED; }
    inline bool IsResized() const { return m_Flags & IS_RESIZED; }

    void SetModified();

    int GetState( int type ) const
    {
        return m_Status & type;
    }


    void SetState( int type, int state )
    {
        if( state )
            m_Status |= type; // state = ON or OFF
        else
            m_Status &= ~type;
    }


    int ReturnStatus() const {  return m_Status;  }

    void SetStatus( int new_status )
    {
        m_Status = new_status;
    }

    void SetFlags( int aMask ) { m_Flags |= aMask; }
    void ClearFlags( int aMask = EDA_ITEM_ALL_FLAGS ) { m_Flags &= ~aMask; }
    int GetFlags() const { return m_Flags; }

    /**
     * Function DisplayInfo
     * has knowledge about the frame and how and where to put status
     * information about this object into the frame's message panel.
     * @param frame A EDA_DRAW_FRAME in which to print status information.
     */
    virtual void DisplayInfo( EDA_DRAW_FRAME* frame )
    {
        // derived classes may implement this
    }

    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param refPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    virtual bool HitTest( const wxPoint& refPos )
    {
        return false;   // derived classes should override this function
    }

    /**
     * Function HitTest (overlaid)
     * tests if the given EDA_RECT intersect this object.
     * For now, an ending point must be inside this rect.
     * @param refArea : the given EDA_RECT
     * @return bool - true if a hit, else false
     */
    virtual bool HitTest( EDA_RECT& refArea )
    {
        return false;   // derived classes should override this function
    }

    /**
     * Function GetBoundingBox
     * returns the orthogonal, bounding box of this object for display
     * purposes.
     * This box should be an enclosing perimeter for visible components of this
     * object, and the units should be in the pcb or schematic coordinate
     * system.
     * It is OK to overestimate the size by a few counts.
     */
    virtual EDA_RECT GetBoundingBox() const
    {
#if defined(DEBUG)
        printf( "Missing GetBoundingBox()\n" );
        Show( 0, std::cout ); // tell me which classes still need GetBoundingBox support
#endif

        // return a zero-sized box per default. derived classes should override
        // this
        return EDA_RECT( wxPoint( 0, 0 ), wxSize( 0, 0 ) );
    }

    /**
     * @brief Function Clone
     * creates a duplicate of this item with linked list members set to NULL.
     *
     * The Clone() function only calls the private virtual doClone() which actually
     * does the cloning for the derived object.
     *
     * @return A clone of the item.
     */
    EDA_ITEM* Clone() const { return doClone(); }

    /**
     * Function IterateForward
     * walks through the object tree calling the inspector() on each object
     * type requested in scanTypes.
     *
     * @param listStart The first in a list of EDA_ITEMs to iterate over.
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
    static SEARCH_RESULT IterateForward( EDA_ITEM*     listStart,
                                         INSPECTOR*    inspector,
                                         const void*   testData,
                                         const KICAD_T scanTypes[] );

    /**
     * Function Visit
     * may be re-implemented for each derived class in order to handle
     * all the types given by its member data.  Implementations should call
     * inspector->Inspect() on types in scanTypes[], and may use
     * IterateForward()
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

    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    virtual wxString GetClass() const
    {
        return wxT( "EDA_ITEM" );
    }

    /**
     * Function GetSelectMenuText
     * returns the text to display to be used in the selection clarification context menu
     * when multiple items are found at the current cursor position.  The default version
     * of this function raises an assertion in the debug mode and returns a string to
     * indicate that it was not overridden to provide the object specific text.
     *
     * @return The menu text string.
     */
    virtual wxString GetSelectMenuText() const;

    /**
     * Function GetMenuImage
     * returns a pointer to an image to be used in menus.  The default version returns
     * the right arrow image.  Overide this function to provide object specific menu
     * images.
     * @return The menu image associated with the item.
     */
    virtual BITMAP_DEF GetMenuImage() const { return right_xpm; }

    /**
     * Test if another item is less than this object.
     *
     * @param aItem - Item to compare against.
     * @return - True if \a aItem is less than the item.
     */
    bool operator<( const EDA_ITEM& aItem ) const;

    /**
     * Function Sort
     * is a helper function to be used by the C++ STL sort algorithm for sorting a STL
     * container of EDA_ITEM pointers.
     *
     * @param aLeft The left hand item to compare.
     * @param aRight The right hand item to compare.
     * @return True if \a aLeft is less than \a aRight.
     */
    static bool Sort( const EDA_ITEM* aLeft, const EDA_ITEM* aRight ) { return *aLeft < *aRight; }

#if defined(DEBUG)

    /**
     * Function Show
     * is used to output the object tree, currently for debugging only.
     * @param nestLevel An aid to prettier tree indenting, and is the level
     *          of nesting of this object within the overall tree.
     * @param os The ostream& to output to.
     */
    virtual void Show( int nestLevel, std::ostream& os ) const;


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


/**
 * Function new_clone
 * provides cloning capabilities for all Boost pointer containers of EDA_ITEM pointers.
 *
 * @param aItem EDA_ITEM to clone.
 * @return Clone of \a aItem.
 */
inline EDA_ITEM* new_clone( const EDA_ITEM& aItem ) { return aItem.Clone(); }


/**
 * Define list of drawing items for screens.
 *
 * The standard C++ containter was choosen so the pointer can be removed  from a list without
 * it being destroyed.
 */
typedef std::vector< EDA_ITEM* > EDA_ITEMS;


// Graphic Text justify:
// Values -1,0,1 are used in computations, do not change them
enum  GRTextHorizJustifyType {
    GR_TEXT_HJUSTIFY_LEFT   = -1,
    GR_TEXT_HJUSTIFY_CENTER = 0,
    GR_TEXT_HJUSTIFY_RIGHT  = 1
};


enum GRTextVertJustifyType {
    GR_TEXT_VJUSTIFY_TOP = -1,
    GR_TEXT_VJUSTIFY_CENTER = 0,
    GR_TEXT_VJUSTIFY_BOTTOM = 1
};

/* Options to show solid segments (segments, texts...) */
enum GRTraceMode {
    FILAIRE = 0,        // segments are drawn as lines
    FILLED,             // normal mode: segments have thickness
    SKETCH              // sketcg mode: segments have thickness, but are not
                        // filled
};

/**
 * Enum FILL_T
 * is the set of fill types used in plotting or drawing enclosed areas.
 */
enum FILL_T {
    NO_FILL,                     // Poly, Square, Circle, Arc = option No Fill
    FILLED_SHAPE,                /* Poly, Square, Circle, Arc = option Fill
                                  * with current color ("Solid shape") */
    FILLED_WITH_BG_BODYCOLOR    /* Poly, Square, Circle, Arc = option Fill
                                  * with background body color, translucent
                                  * (texts inside this shape can be seen)
                                  * not filled in B&W mode when plotting or
                                  * printing */
};


#define DEFAULT_SIZE_TEXT 60    /* default text height (in mils or 1/1000") */

/**
 * Class EDA_TEXT
 * is a basic class to handle texts (labels, texts on components or footprints
 * ..) not used directly.
 * The text classes are derived from EDA_ITEM and EDA_TEXT
 */
class EDA_TEXT
{
public:
    wxString m_Text;                    /* text! */
    wxPoint  m_Pos;                     /* XY position of anchor text. */
    wxSize   m_Size;                    /* XY size of text */
    int      m_Thickness;               /* pen size used to draw this text */
    int      m_Orient;                  /* Orient in 0.1 degrees */
    bool     m_Mirror;                  /* Display Normal / mirror */
    int      m_Attributs;               /* flags (visible...) */
    bool     m_Italic;                  /* true to simulate (or use if exists)
                                         * an italic font... */
    bool     m_Bold;                    /* true to simulate a bold font ... */
    GRTextHorizJustifyType m_HJustify;  /* Horiz justification */
    GRTextVertJustifyType m_VJustify;   /* Vertical justification */
    bool     m_MultilineAllowed;        /* true to use multiline option, false
                                         * to use only single line text
                                         * Single line is faster in
                                         * calculations than multiline */

public:
    EDA_TEXT( const wxString& text = wxEmptyString );
    EDA_TEXT( const EDA_TEXT& aText );
    virtual ~EDA_TEXT();

    /**
     * Function SetThickness
     * sets text thickness.
     * @param aNewThickness is the new text thickness.
     */
    void SetThickness( int aNewThickness ) { m_Thickness = aNewThickness; };

    /**
     * Function GetThickness
     * returns text thickness.
     * @return int - text thickness.
     */
    int GetThickness() const { return m_Thickness; };

    /**
     * Function SetSize
     * sets text size.
     * @param aNewSize is the new text size.
     */
    void SetSize( wxSize aNewSize ) { m_Size = aNewSize; };

    /**
     * Function GetSize
     * returns text size.
     * @return wxSize - text size.
     */
    wxSize GetSize() const { return m_Size; };

    int GetLength() const { return m_Text.Length(); };

    /**
     * Function Draw
     *  @param aPanel = the current DrawPanel
     *  @param aDC = the current Device Context
     *  @param aOffset = draw offset (usually (0,0))
     *  @param aColor = text color
     *  @param aDrawMode = GR_OR, GR_XOR.., -1 to use the current mode.
     *  @param aDisplay_mode = FILAIRE, FILLED or SKETCH
     *  @param aAnchor_color = anchor color ( UNSPECIFIED_COLOR = do
     *                                    not draw anchor ).
     */
    void Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
               const wxPoint& aOffset, EDA_Colors aColor,
               int aDrawMode, GRTraceMode aDisplay_mode = FILAIRE,
               EDA_Colors aAnchor_color = UNSPECIFIED_COLOR );

private:

    /**
     * Function DrawOneLineOfText
     * Draw a single text line.
     * Used to draw each line of this EDA_TEXT, that can be multiline
     *  @param aPanel = the current DrawPanel
     *  @param aDC = the current Device Context
     *  @param aOffset = draw offset (usually (0,0))
     *  @param aColor = text color
     *  @param aDrawMode = GR_OR, GR_XOR.., -1 to use the current mode.
     *  @param aFillMode = FILAIRE, FILLED or SKETCH
     *  @param aAnchor_color = anchor color ( UNSPECIFIED_COLOR = do
     *    not draw anchor ).
     *  @param aText = the single line of text to draw.
     *  @param aPos = the position of this line ).
     */
    void DrawOneLineOfText( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
                            const wxPoint& aOffset, EDA_Colors aColor,
                            int aDrawMode, GRTraceMode aFillMode,
                            EDA_Colors aAnchor_color, wxString& aText,
                            wxPoint aPos );

public:

    /**
     * Function TextHitTest
     * Test if \a aPoint is within the bounds of this object.
     * @param aPoint- A wxPoint to test
     * @param aAccuracy - Amount to inflate the bounding box.
     * @return bool - true if a hit, else false
     */
    bool TextHitTest( const wxPoint& aPoint, int aAccuracy = 0 ) const;

    /**
     * Function TextHitTest (overloaded)
     * Tests if object bounding box is contained within or intersects \a aRect.
     *
     * @param aRect - Rect to test against.
     * @param aContains - Test for containment instead of intersection if true.
     * @param aAccuracy - Amount to inflate the bounding box.
     * @return bool - true if a hit, else false
     */
    bool TextHitTest( const EDA_RECT& aRect, bool aContains = false, int aAccuracy = 0 ) const;

    /**
     * Function LenSize
     * @return the text length in internal units
     * @param aLine : the line of text to consider.
     * For single line text, this parameter is always m_Text
     */
    int LenSize( const wxString& aLine ) const;

    /**
     * Function GetTextBox
     * useful in multiline texts to calculate the full text or a line area (for
     * zones filling, locate functions....)
     * @return the rect containing the line of text (i.e. the position and the
     *  size of one line)
     * this rectangle is calculated for 0 orient text. if orient is not 0 the
     * rect must be rotated to match the physical area
     * @param aLine : the line of text to consider.
     * for single line text, aLine is unused
     * If aLine == -1, the full area (considering all lines) is returned
     * @param aThickness - Overrides the current thickness when greater than 0.
     * @param aInvertY - Invert the Y axis when calculating bounding box.
     */
    EDA_RECT GetTextBox( int aLine = -1, int aThickness = -1, bool aInvertY = false ) const;

    /**
     * Function GetInterline
     * return the distance between 2 text lines
     * has meaning only for multiline texts
     */
    int GetInterline() const
    {
        return (( m_Size.y * 14 ) / 10) + m_Thickness;
    }

    /**
     * Function GetTextStyleName
     * @return a wwString withe the style name( Normal, Italic, Bold, Bold+Italic)
     */
    wxString GetTextStyleName();

    void SetText( const wxString& aText ) { m_Text = aText; }

    wxString GetText() const { return m_Text; }

    GRTextHorizJustifyType GetHorizJustify() const { return m_HJustify; };
    GRTextVertJustifyType GetVertJustify() const { return m_VJustify; };
    void SetHorizJustify( GRTextHorizJustifyType aType ) { m_HJustify = aType; };
    void SetVertJustify( GRTextVertJustifyType aType ) { m_VJustify = aType; };
};

#endif /* BASE_STRUCT_H */
