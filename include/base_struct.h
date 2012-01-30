/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
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
 * @file  base_struct.h
 * @brief Basic classes for most KiCad items.
 */

#ifndef BASE_STRUCT_H_
#define BASE_STRUCT_H_

#include <colors.h>
#include <bitmaps.h>

#include <boost/ptr_container/ptr_vector.hpp>

#if defined(DEBUG)
#include <iostream>         // needed for Show()
extern std::ostream& operator <<( std::ostream& out, const wxSize& size );

extern std::ostream& operator <<( std::ostream& out, const wxPoint& pt );
#endif


/**
 * Enum KICAD_T
 * is the set of class identification values, stored in EDA_ITEM::m_StructType
 */
enum KICAD_T {
    NOT_USED = -1,          ///< the 3d code uses this value

    EOT = 0,                ///< search types array terminator (End Of Types)

    TYPE_NOT_INIT = 0,
    PCB_T,
    SCREEN_T,               ///< not really an item, used to identify a screen

    // Items in pcb
    PCB_MODULE_T,           ///< class MODULE, a footprint
    PCB_PAD_T,              ///< class D_PAD, a pad in a footprint
    PCB_LINE_T,             ///< class DRAWSEGMENT, a segment not on copper layers
    PCB_TEXT_T,             ///< class TEXTE_PCB, text on a layer
    PCB_MODULE_TEXT_T,      ///< class TEXTE_MODULE, text in a footprint
    PCB_MODULE_EDGE_T,      ///< class EDGE_MODULE, a footprint edge
    PCB_TRACE_T,            ///< class TRACKE, a track segment (segment on a copper layer)
    PCB_VIA_T,              ///< class VIA, a via (like a track segment on a copper layer)
    PCB_ZONE_T,             ///< class SEGZONE, a segment used to fill a zone area (segment on a
                            ///< copper layer)
    PCB_MARKER_T,           ///< class MARKER_PCB, a marker used to show something
    PCB_DIMENSION_T,        ///< class DIMENSION, a dimension (graphic item)
    PCB_TARGET_T,           ///< class PCB_TARGET, a target (graphic item)
    PCB_ZONE_AREA_T,        ///< class ZONE_CONTAINER, a zone area
    PCB_ITEM_LIST_T,        ///< class BOARD_ITEM_LIST, a list of board items

    // Schematic draw Items.  The order of these items effects the sort order.
    // It is currently ordered to mimic the old Eeschema locate behavior where
    // the smallest item is the selected item.
    SCH_MARKER_T,
    SCH_JUNCTION_T,
    SCH_NO_CONNECT_T,
    SCH_BUS_ENTRY_T,
    SCH_LINE_T,
    SCH_POLYLINE_T,
    SCH_BITMAP_T,
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
     * For GerbView: items type:
     */
    TYPE_GERBER_DRAW_ITEM,

    // End value
    MAX_STRUCT_TYPE_ID
};


enum SEARCH_RESULT {
    SEARCH_QUIT,
    SEARCH_CONTINUE
};


class wxFindReplaceData;
class EDA_ITEM;
class EDA_DRAW_FRAME;
class EDA_RECT;
class EDA_DRAW_PANEL;
class DHEAD;


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
     * EDA_ITEM::Iterate() function.  It is used primarily for searching, but
     * not limited to that.  It can also collect or modify the scanned objects.
     *
     * @param aItem An EDA_ITEM to examine.
     * @param aTestData is arbitrary data needed by the inspector to determine
     *                  if the EDA_ITEM under test meets its match criteria.
     * @return A #SEARCH_RESULT type #SEARCH_QUIT if the iterator function is to
     *          stop the scan, else #SEARCH_CONTINUE;
     */
    SEARCH_RESULT virtual Inspect( EDA_ITEM* aItem, const void* aTestData ) = 0;
};


/**
 * Class EDA_RECT
 * handles the component boundary box.
 * This class is similar to wxRect, but some wxRect functions are very curious,
 * and are working only if dimensions are >= 0 (not always the case in KiCad)
 * and also KiCad needs some specific method.
 * so I prefer this more suitable class
 */
class EDA_RECT
{
private:
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
     * ensures that the height ant width are positive.
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
                                   ///< already been edited, in some functions
#define EDA_ITEM_ALL_FLAGS -1


/**
 * Class EDA_ITEM
 * is a base class for most all the KiCad significant classes, used in
 * schematics and boards.
 */
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
    EDA_ITEM*     Pnext;          ///< next in linked list
    EDA_ITEM*     Pback;          ///< previous in linked list
    DHEAD*        m_List;         ///< which DLIST I am on.

    EDA_ITEM*     m_Parent;       /* Linked list: Link (parent struct) */
    EDA_ITEM*     m_Son;          /* Linked list: Link (son struct) */
    unsigned long m_TimeStamp;    ///< Time stamp used for logical links

    /// Set to true to override the visibility setting of the item.
    bool          m_forceVisible;

    /// Flag bits for editing and other uses.
    int           m_Flags;

    // Link to an copy of the item use to save the item's state for undo/redo feature.
    EDA_ITEM*     m_Image;

private:
    void InitVars();

    /**
     * Function doClone
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

    void SetTimeStamp( unsigned long aNewTimeStamp ) { m_TimeStamp = aNewTimeStamp; }
    unsigned long GetTimeStamp() const { return m_TimeStamp; }

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

    int GetStatus() const           { return m_Status; }
    void SetStatus( int aStatus )   { m_Status = aStatus; }

    void SetFlags( int aMask ) { m_Flags |= aMask; }
    void ClearFlags( int aMask = EDA_ITEM_ALL_FLAGS ) { m_Flags &= ~aMask; }
    int GetFlags() const { return m_Flags; }

    void SetImage( EDA_ITEM* aItem ) { m_Image = aItem; }

    /**
     * Function SetForceVisible
     * is used to set and cleag force visible flag used to force the item to be drawn
     * even if it's draw attribute is set to not visible.
     *
     * @param aEnable True forces the item to be drawn.  False uses the item's visibility
     *                setting to determine if the item is to be drawn.
     */
    void SetForceVisible( bool aEnable ) { m_forceVisible = aEnable; }

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
     * Function Clone
     * creates a duplicate of this item with linked list members set to NULL.
     *
     * The Clone() function only calls the private virtual doClone() which actually
     * does the cloning for the derived object.
     *
     * @todo: use this instead of Copy() everywhere, then kill Copy().
     *
     * @return A clone of the item.
     */
    EDA_ITEM* Clone() const;    // should not be inline, to save the ~ 6 bytes per call site.

    /**
     * Function IterateForward
     * walks through the object tree calling the inspector() on each object
     * type requested in scanTypes.
     *
     * @param listStart The first in a list of EDA_ITEMs to iterate over.
     * @param inspector Is an INSPECTOR to call on each object that is one of
     *                  the requested scanTypes.
     * @param testData Is an aid to testFunc, and should be sufficient to allow
     *                 it to fully determine if an item meets the match criteria,
     *                 but it may also be used to collect output.
     * @param scanTypes A KICAD_T array that is EOT terminated, and provides both
     *                  the order and interest level of of the types of objects to
     *                  be iterated over.
     * @return SEARCH_RESULT SEARCH_QUIT if the called INSPECTOR returned
     *                       SEARCH_QUIT, else SCAN_CONTINUE;
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
     *                  is significant too, terminated by EOT.
     * @return SEARCH_RESULT SEARCH_QUIT if the Iterator is to stop the scan,
     *                       else SCAN_CONTINUE, and determined by the inspector.
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
     * the right arrow image.  Override this function to provide object specific menu
     * images.
     * @return The menu image associated with the item.
     */
    virtual BITMAP_DEF GetMenuImage() const { return right_xpm; }

    /**
     * Function Matches
     * compares the item against the search criteria in \a aSearchData.
     *
     * The base class returns false since many of the objects derived from EDA_ITEM
     * do not have any text to search.
     *
     * @param aSearchData A reference to a wxFindReplaceData object containing the
     *                    search criteria.
     * @param aAuxData A pointer to optional data required for the search or NULL
     *                 if not used.
     * @param aFindLocation A pointer to a wxPoint object to store the location of
     *                      matched item.  The pointer can be NULL if it is not used.
     * @return True if the item's text matches the search criteria in \a aSearchData.
     */
    virtual bool Matches( wxFindReplaceData& aSearchData, void* aAuxData, wxPoint* aFindLocation )
    {
        return false;
    }

    /**
     * Function Matches
     * compares \a aText against search criteria in \a aSearchData.
     *
     * @param aText A reference to a wxString object containing the string to test.
     * @param aSearchData The criteria to search against.
     * @return True if \a aText matches the search criteria in \a aSearchData.
     */
    bool Matches( const wxString& aText, wxFindReplaceData& aSearchData );

    /**
     * Function Replace
     * performs a text replace on \a aText using the find and replace criteria in
     * \a aSearchData on items that support text find and replace.
     *
     * @param aSearchData A reference to a wxFindReplaceData object containing the
     *                    search and replace criteria.
     * @param aText A reference to a wxString object containing the text to be
     *              replaced.
     * @return True if \a aText was modified, otherwise false.
     */
    bool Replace( wxFindReplaceData& aSearchData, wxString& aText );

    /**
     * Function Replace
     * performs a text replace using the find and replace criteria in \a aSearchData
     * on items that support text find and replace.
     *
     * This function must be overridden for items that support text replace.
     *
     * @param aSearchData A reference to a wxFindReplaceData object containing the
     *                    search and replace criteria.
     * @param aAuxData A pointer to optional data required for the search or NULL
     *                 if not used.
     * @return True if the item text was modified, otherwise false.
     */
    virtual bool Replace( wxFindReplaceData& aSearchData, void* aAuxData = NULL ) { return false; }

    /**
     * Function IsReplaceable
     * <p>
     * Override this method in any derived object that supports test find and
     * replace.
     * </p>
     *
     * @return True if the item has replaceable text that can be modified using
     *         the find and replace dialog.
     */
    virtual bool IsReplaceable() const { return false; }

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

    /**
     * Operator assignment
     * is used to assign the members of \a aItem to another object.
     *
     * @warning This is still a work in progress and not ready for prime time.  Do not use
     *          as there is a known issue with wxString buffers.
     */
    virtual EDA_ITEM& operator=( const EDA_ITEM& aItem );

#if defined(DEBUG)

    /**
     * Function Show
     * is used to output the object tree, currently for debugging only.
     * @param nestLevel An aid to prettier tree indenting, and is the level
     *                  of nesting of this object within the overall tree.
     * @param os The ostream& to output to.
     */
    virtual void Show( int nestLevel, std::ostream& os ) const = 0;
    // pure virtual so compiler warns if somebody mucks up a derived declaration

    void ShowDummy( std::ostream& os ) const;  ///< call this if you are a lazy developer

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
 * The standard C++ container was chosen so the pointer can be removed  from a list without
 * it being destroyed.
 */
typedef std::vector< EDA_ITEM* > EDA_ITEMS;


// Graphic Text justify:
// Values -1,0,1 are used in computations, do not change them
enum EDA_TEXT_HJUSTIFY_T {
    GR_TEXT_HJUSTIFY_LEFT   = -1,
    GR_TEXT_HJUSTIFY_CENTER = 0,
    GR_TEXT_HJUSTIFY_RIGHT  = 1
};


enum EDA_TEXT_VJUSTIFY_T {
    GR_TEXT_VJUSTIFY_TOP    = -1,
    GR_TEXT_VJUSTIFY_CENTER = 0,
    GR_TEXT_VJUSTIFY_BOTTOM = 1
};

/* Options to show solid segments (segments, texts...) */
enum EDA_DRAW_MODE_T {
    LINE = 0,           // segments are drawn as lines
    FILLED,             // normal mode: segments have thickness
    SKETCH              // sketch mode: segments have thickness, but are not filled
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
 * ..) not used directly. The "used" text classes are derived from EDA_ITEM and
 * EDA_TEXT using multiple inheritance.
 */
class EDA_TEXT
{
public:
    wxString m_Text;
    int      m_Thickness;               ///< pen size used to draw this text
    double   m_Orient;                  ///< Orient in 0.1 degrees
    wxPoint  m_Pos;                     ///< XY position of anchor text.
    wxSize   m_Size;                    ///< XY size of text
    bool     m_Mirror;                  ///< true iff mirrored
    int      m_Attributs;               ///< bit flags such as visible, etc.
    bool     m_Italic;                  ///< should be italic font (if available)
    bool     m_Bold;                    ///< should be bold font (if available)
    EDA_TEXT_HJUSTIFY_T m_HJustify;     ///< horizontal justification
    EDA_TEXT_VJUSTIFY_T m_VJustify;     ///< vertical justification

    bool     m_MultilineAllowed;        /**< true to use multiline option, false
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

    void SetOrientation( double aOrientation ) { m_Orient = aOrientation; }
    double GetOrientation() const { return m_Orient; }

    void SetItalic( bool isItalic ) { m_Italic = isItalic; }
    bool IsItalic() const { return m_Italic; }

    void SetMirrored( bool isMirrored ) { m_Mirror = isMirrored; }
    bool IsMirrored() const { return m_Mirror; }

    /**
     * Function SetSize
     * sets text size.
     * @param aNewSize is the new text size.
     */
    void SetSize( const wxSize& aNewSize ) { m_Size = aNewSize; };

    /**
     * Function GetSize
     * returns text size.
     * @return wxSize - text size.
     */
    const wxSize GetSize() const { return m_Size; };

    /// named differently than the ones using multiple inheritance and including this class
    void SetPos( const wxPoint& aPoint ) { m_Pos = aPoint; }
    const wxPoint GetPos() const { return m_Pos; }

    int GetLength() const { return m_Text.Length(); };

    /**
     * Function Draw
     * @param aPanel = the current DrawPanel
     * @param aDC = the current Device Context
     * @param aOffset = draw offset (usually (0,0))
     * @param aColor = text color
     * @param aDrawMode = GR_OR, GR_XOR.., -1 to use the current mode.
     * @param aDisplay_mode = LINE, FILLED or SKETCH
     * @param aAnchor_color = anchor color ( UNSPECIFIED_COLOR = do not draw anchor ).
     */
    void Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
               const wxPoint& aOffset, EDA_Colors aColor,
               int aDrawMode, EDA_DRAW_MODE_T aDisplay_mode = LINE,
               EDA_Colors aAnchor_color = UNSPECIFIED_COLOR );

private:

    /**
     * Function DrawOneLineOfText
     * Draw a single text line.
     * Used to draw each line of this EDA_TEXT, that can be multiline
     * @param aPanel = the current DrawPanel
     * @param aDC = the current Device Context
     * @param aOffset = draw offset (usually (0,0))
     * @param aColor = text color
     * @param aDrawMode = GR_OR, GR_XOR.., -1 to use the current mode.
     * @param aFillMode = LINE, FILLED or SKETCH
     * @param aAnchor_color = anchor color ( UNSPECIFIED_COLOR = do not draw anchor ).
     * @param aText = the single line of text to draw.
     * @param aPos = the position of this line ).
     */
    void DrawOneLineOfText( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
                            const wxPoint& aOffset, EDA_Colors aColor,
                            int aDrawMode, EDA_DRAW_MODE_T aFillMode,
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
     *         size of one line) this rectangle is calculated for 0 orient text.
     *         If orientation is not 0 the rect must be rotated to match the
     *         physical area
     * @param aLine The line of text to consider.
     * for single line text, aLine is unused
     * If aLine == -1, the full area (considering all lines) is returned
     * @param aThickness Overrides the current thickness when greater than 0.
     * @param aInvertY Invert the Y axis when calculating bounding box.
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
     * @return a wxString with the style name( Normal, Italic, Bold, Bold+Italic)
     */
    wxString GetTextStyleName();

    void SetText( const wxString& aText ) { m_Text = aText; }

    /**
     * Function GetText
     * returns the string associated with the text object.
     * <p>
     * This function is virtual to allow derived classes to override getting the
     * string to provide a way for modifying the base string by adding a suffix or
     * prefix to the base string.
     * </p>
     * @return a const wxString object containing the string of the item.
     */
    virtual const wxString GetText() const { return m_Text; }

    EDA_TEXT_HJUSTIFY_T GetHorizJustify() const { return m_HJustify; };
    EDA_TEXT_VJUSTIFY_T GetVertJustify() const { return m_VJustify; };
    void SetHorizJustify( EDA_TEXT_HJUSTIFY_T aType ) { m_HJustify = aType; };
    void SetVertJustify( EDA_TEXT_VJUSTIFY_T aType ) { m_VJustify = aType; };
};

#endif // BASE_STRUCT_H_
