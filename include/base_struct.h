/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2019 KiCad Developers, see change_log.txt for contributors.
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

#ifndef BASE_STRUCT_H_
#define BASE_STRUCT_H_

#include <deque>

#include <core/typeinfo.h>
#include "common.h"
#include <wx/fdrepdlg.h>
#include <bitmap_types.h>
#include <view/view_item.h>

/**
 * Enum FILL_T
 * is the set of fill types used in plotting or drawing enclosed areas.
 */
enum FILL_T {
    NO_FILL,                     // Poly, Square, Circle, Arc = option No Fill
    FILLED_SHAPE,                /* Poly, Square, Circle, Arc = option Fill
                                  * with current color ("Solid shape") */
    FILLED_WITH_BG_BODYCOLOR     /* Poly, Square, Circle, Arc = option Fill
                                  * with background body color, translucent
                                  * (texts inside this shape can be seen)
                                  * not filled in B&W mode when plotting or
                                  * printing */
};


enum SEARCH_RESULT {
    SEARCH_QUIT,
    SEARCH_CONTINUE
};


/**
 * Additional flag values wxFindReplaceData::m_Flags
 */
enum FIND_REPLACE_FLAGS
{
    // The last wxFindReplaceFlag enum is wxFR_MATCHCASE = 0x4.
    FR_CURRENT_SHEET_ONLY = wxFR_MATCHCASE << 1,   // Search the current sheet only.
    FR_SEARCH_ALL_FIELDS  = wxFR_MATCHCASE << 2,   // Search user fields as well as ref and value.
    FR_SEARCH_ALL_PINS    = wxFR_MATCHCASE << 3,   // Search pin name and number.
    FR_MATCH_WILDCARD     = wxFR_MATCHCASE << 4,   // Use simple wild card matching (* & ?).
    FR_SEARCH_WRAP        = wxFR_MATCHCASE << 5,   // Wrap around the start or end of search.
    FR_SEARCH_REPLACE     = wxFR_MATCHCASE << 7,   // Search for a item that has replaceable text.
    FR_REPLACE_ITEM_FOUND = wxFR_MATCHCASE << 8,   // Indicates an item with replaceable text has
                                                   // been found.
    FR_REPLACE_REFERENCES = wxFR_MATCHCASE << 9    // Don't replace in references.
};


class wxFindReplaceData;
class EDA_ITEM;
class EDA_DRAW_FRAME;
class EDA_RECT;
class DHEAD;
class MSG_PANEL_ITEM;


/**
 * Typedef INSPECTOR
 * is used to inspect and possibly collect the
 * (search) results of iterating over a list or tree of KICAD_T objects.
 * Provide an implementation as needed to inspect EDA_ITEMs visited via
 * EDA_ITEM::Visit() and EDA_ITEM::IterateForward().
 * <p>
 * FYI the std::function may hold a lambda, std::bind, pointer to func, or
 * ptr to member function, per modern C++. It is used primarily for searching,
 * but not limited to that.  It can also collect or modify the scanned objects.
 * 'Capturing' lambdas are particularly convenient because they can use context
 * and this often means @a aTestData is not used.
 *
 * @param aItem An EDA_ITEM to examine.
 * @param aTestData is arbitrary data needed by the inspector to determine
 *  if the EDA_ITEM under test meets its match criteria, and is often NULL
 *  with the advent of capturing lambdas.
 * @return A #SEARCH_RESULT type #SEARCH_QUIT if the iterator function is to
 *          stop the scan, else #SEARCH_CONTINUE;
 */
typedef std::function< SEARCH_RESULT ( EDA_ITEM* aItem, void* aTestData ) >   INSPECTOR_FUNC;

typedef const INSPECTOR_FUNC& INSPECTOR;    /// std::function passed to nested users by ref, avoids copying std::function


// These define are used for the .m_Flags and .m_UndoRedoStatus member of the
// class EDA_ITEM
//
// NB: DO NOT ADD FLAGS ANYWHERE BUT AT THE END: THE FLAG-SET IS STORED AS AN INTEGER IN FILES.
//
#define IS_CHANGED     (1 << 0)    ///< Item was edited, and modified
#define IS_LINKED      (1 << 1)    ///< Used in calculation to mark linked items (temporary use)
#define IN_EDIT        (1 << 2)    ///< Item currently edited
#define IS_MOVED       (1 << 3)    ///< Item being moved
#define IS_NEW         (1 << 4)    ///< New item, just created
#define IS_RESIZED     (1 << 5)    ///< Item being resized
#define IS_DRAGGED     (1 << 6)    ///< Item being dragged
#define IS_DELETED     (1 << 7)
#define IS_WIRE_IMAGE  (1 << 8)    ///< Item to be drawn as wireframe while editing
#define STARTPOINT     (1 << 9)    ///< When a line is selected, these flags indicate which
#define ENDPOINT       (1 << 10)   ///< ends.  (Used to support dragging.)
#define SELECTED       (1 << 11)
#define SELECTEDNODE   (1 << 12)   ///< flag indicating that the structure has already selected
#define STRUCT_DELETED (1 << 13)   ///< flag indication structures to be erased
#define CANDIDATE      (1 << 14)   ///< flag indicating that the structure is connected
#define SKIP_STRUCT    (1 << 15)   ///< flag indicating that the structure should be ignored
#define DO_NOT_DRAW    (1 << 16)   ///< Used to disable draw function
#define IS_PASTED      (1 << 17)   ///< Modifier on IS_NEW which indicates it came from clipboard
#define TRACK_LOCKED   (1 << 18)   ///< Pcbnew: track locked: protected from global deletion
#define TRACK_AR       (1 << 19)   ///< Pcbnew: autorouted track
#define FLAG1          (1 << 20)   ///< Pcbnew: flag used in local computations
#define FLAG0          (1 << 21)   ///< Pcbnew: flag used in local computations
#define BEGIN_ONPAD    (1 << 22)   ///< Pcbnew: flag set for track segment starting on a pad
#define END_ONPAD      (1 << 23)   ///< Pcbnew: flag set for track segment ending on a pad
#define BUSY           (1 << 24)   ///< Pcbnew: flag indicating that the structure has
                                   ///< already been edited, in some functions
#define HIGHLIGHTED    (1 << 25)   ///< item is drawn in normal colors, when the rest is darkened
#define BRIGHTENED     (1 << 26)   ///< item is drawn with a bright contour

#define DP_COUPLED     (1 << 27)   ///< item is coupled with another item making a differential pair
                                   ///< (applies to segments only)
#define UR_TRANSIENT   (1 << 28)   ///< indicates the item is owned by the undo/redo stack

#define IS_DANGLING    (1 << 29)   ///< indicates a pin is dangling

// WARNING: if you add flags, you'll probably need to adjust the masks in GetEditFlags() and
// ClearTempFlags().

#define EDA_ITEM_ALL_FLAGS -1

typedef unsigned STATUS_FLAGS;

/**
 * Class EDA_ITEM
 * is a base class for most all the KiCad significant classes, used in
 * schematics and boards.
 */
class EDA_ITEM : public KIGFX::VIEW_ITEM
{
private:

    /**
     * Run time identification, _keep private_ so it can never be changed after
     * a constructor sets it.  See comment near SetType() regarding virtual
     * functions.
     */
    KICAD_T       m_StructType;
    STATUS_FLAGS  m_Status;

protected:
    EDA_ITEM*     Pnext;          ///< next in linked list
    EDA_ITEM*     Pback;          ///< previous in linked list
    DHEAD*        m_List;         ///< which DLIST I am on.

    EDA_ITEM*     m_Parent;       ///< Linked list: Link (parent struct)
    timestamp_t   m_TimeStamp;    ///< Time stamp used for logical links

    /// Set to true to override the visibility setting of the item.
    bool          m_forceVisible;

    /// Flag bits for editing and other uses.
    STATUS_FLAGS  m_Flags;

private:

    void initVars();

protected:

    EDA_ITEM( EDA_ITEM* parent, KICAD_T idType );
    EDA_ITEM( KICAD_T idType );
    EDA_ITEM( const EDA_ITEM& base );

public:

    virtual ~EDA_ITEM() { };

    /**
     * Function Type()
     *
     * returns the type of object.  This attribute should never be changed
     * after a constructor sets it, so there is no public "setter" method.
     * @return KICAD_T - the type of object.
     */
    inline KICAD_T Type() const
    {
        return m_StructType;
    }

    void SetTimeStamp( timestamp_t aNewTimeStamp ) { m_TimeStamp = aNewTimeStamp; }
    timestamp_t GetTimeStamp() const { return m_TimeStamp; }

    EDA_ITEM* Next() const { return Pnext; }
    EDA_ITEM* Back() const { return Pback; }
    EDA_ITEM* GetParent() const { return m_Parent; }
    DHEAD* GetList() const { return m_List; }

    void SetNext( EDA_ITEM* aNext )       { Pnext = aNext; }
    void SetBack( EDA_ITEM* aBack )       { Pback = aBack; }
    void SetParent( EDA_ITEM* aParent )   { m_Parent = aParent; }
    void SetList( DHEAD* aList )          { m_List = aList; }

    inline bool IsNew() const { return m_Flags & IS_NEW; }
    inline bool IsModified() const { return m_Flags & IS_CHANGED; }
    inline bool IsMoving() const { return m_Flags & IS_MOVED; }
    inline bool IsDragging() const { return m_Flags & IS_DRAGGED; }
    inline bool IsWireImage() const { return m_Flags & IS_WIRE_IMAGE; }
    inline bool IsSelected() const { return m_Flags & SELECTED; }
    inline bool IsResized() const { return m_Flags & IS_RESIZED; }
    inline bool IsHighlighted() const { return m_Flags & HIGHLIGHTED; }
    inline bool IsBrightened() const { return m_Flags & BRIGHTENED; }

    inline void SetWireImage() { SetFlags( IS_WIRE_IMAGE ); }
    inline void SetSelected() { SetFlags( SELECTED ); }
    inline void SetHighlighted() { SetFlags( HIGHLIGHTED ); }
    inline void SetBrightened() { SetFlags( BRIGHTENED ); }

    inline void ClearSelected() { ClearFlags( SELECTED ); }
    inline void ClearHighlighted() { ClearFlags( HIGHLIGHTED ); }
    inline void ClearBrightened() { ClearFlags( BRIGHTENED ); }

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

    STATUS_FLAGS GetStatus() const           { return m_Status; }
    void SetStatus( STATUS_FLAGS aStatus )   { m_Status = aStatus; }

    void SetFlags( STATUS_FLAGS aMask ) { m_Flags |= aMask; }
    void ClearFlags( STATUS_FLAGS aMask = EDA_ITEM_ALL_FLAGS ) { m_Flags &= ~aMask; }
    STATUS_FLAGS GetFlags() const { return m_Flags; }

    STATUS_FLAGS GetEditFlags() const
    {
        int mask = EDA_ITEM_ALL_FLAGS - ( SELECTED | HIGHLIGHTED | BRIGHTENED |
                                          STARTPOINT | ENDPOINT | IS_DANGLING |
                                          BEGIN_ONPAD | END_ONPAD | DP_COUPLED );
        return m_Flags & mask;
    }

    void ClearTempFlags()
    {
        ClearFlags( STARTPOINT | ENDPOINT | CANDIDATE | IS_LINKED | SKIP_STRUCT | DO_NOT_DRAW );
    }

    void ClearEditFlags()
    {
        ClearFlags( GetEditFlags() );
    }

    /**
     * Function IsType
     * Checks whether the item is one of the listed types
     * @param aScanTypes List of item types
     * @return true if the item type is contained in the list aScanTypes
     */
    virtual bool IsType( const KICAD_T aScanTypes[] )
    {
        if( aScanTypes[0] == SCH_LOCATE_ANY_T )
            return true;

        for( const KICAD_T* p = aScanTypes; *p != EOT; ++p )
        {
            if( m_StructType == *p )
                return true;
        }

        return false;
    }

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
     * Function GetMsgPanelInfo
     * populates \a aList of #MSG_PANEL_ITEM objects with it's internal state for display
     * purposes.
     *
     * @note This method replaces DisplayInfo() so that KiCad objects no longer have any
     *       knowledge of wxWidgets UI objects.
     *
     * @param aList is the list to populate.
     */
    virtual void GetMsgPanelInfo( EDA_UNITS_T aUnits, std::vector< MSG_PANEL_ITEM >& aList )
    {
    }

    /**
     * Function HitTest
     * tests if \a aPosition is contained within or on the bounding box of an item.
     *
     * @param aPosition A reference to a wxPoint object containing the coordinates to test.
     * @param aAccuracy Increase the item bounding box by this amount.
     * @return True if \a aPosition is within the item bounding box.
     */
    virtual bool HitTest( const wxPoint& aPosition, int aAccuracy = 0 ) const
    {
        return false;   // derived classes should override this function
    }

    /**
     * Function HitTest
     * tests if \a aRect intersects or is contained within the bounding box of an item.
     *
     * @param aRect A reference to a EDA_RECT object containing the rectangle to test.
     * @param aContained Set to true to test for containment instead of an intersection.
     * @param aAccuracy Increase \a aRect by this amount.
     * @return True if \a aRect contains or intersects the item bounding box.
     */
    virtual bool HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy = 0 ) const
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
    virtual const EDA_RECT GetBoundingBox() const;

    /**
     * Function Clone
     * creates a duplicate of this item with linked list members set to NULL.
     *
     * The default version will return NULL in release builds and likely crash the
     * program.  In debug builds, a warning message indicating the derived class
     * has not implemented cloning.  This really should be a pure virtual function.
     * Due to the fact that there are so many objects derived from EDA_ITEM, the
     * decision was made to return NULL until all the objects derived from EDA_ITEM
     * implement cloning.  Once that happens, this function should be made pure.
     *
     * @return A clone of the item.
     */
    virtual EDA_ITEM* Clone() const; // should not be inline, to save the ~ 6 bytes per call site.

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
    virtual SEARCH_RESULT Visit( INSPECTOR inspector, void* testData, const KICAD_T scanTypes[] );

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
    static SEARCH_RESULT IterateForward( EDA_ITEM*      listStart,
                                         INSPECTOR      inspector,
                                         void*          testData,
                                         const KICAD_T  scanTypes[] )

    {
        for( EDA_ITEM* p = listStart; p; p = p->Pnext )
        {
            if( SEARCH_QUIT == p->Visit( inspector, testData, scanTypes ) )
                return SEARCH_QUIT;
        }

        return SEARCH_CONTINUE;
    }

    /**
     * @copydoc SEARCH_RESULT IterateForward( EDA_ITEM*, INSPECTOR, void*, const KICAD_T )
     *
     * This changes first parameter to avoid the DList and use the main queue instead
     */
    template< class T >
    static SEARCH_RESULT IterateForward( std::deque<T>&  aList,
                                         INSPECTOR       inspector,
                                         void*           testData,
                                         const KICAD_T   scanTypes[] )
    {
        for( auto it : aList )
        {
            if( static_cast<EDA_ITEM*>( it )->Visit( inspector, testData, scanTypes ) == SEARCH_QUIT )
                return SEARCH_QUIT;
        }

        return SEARCH_CONTINUE;
    }

    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    virtual wxString GetClass() const = 0;

    /**
     * Function GetSelectMenuText
     * returns the text to display to be used in the selection clarification context menu
     * when multiple items are found at the current cursor position.  The default version
     * of this function raises an assertion in the debug mode and returns a string to
     * indicate that it was not overridden to provide the object specific text.
     *
     * @return The menu text string.
     */
    virtual wxString GetSelectMenuText( EDA_UNITS_T aUnits ) const;

    /**
     * Function GetMenuImage
     * returns a pointer to an image to be used in menus.  The default version returns
     * the right arrow image.  Override this function to provide object specific menu
     * images.
     * @return The menu image associated with the item.
     */
    virtual BITMAP_DEF GetMenuImage() const;

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
     * @return True if the item's text matches the search criteria in \a aSearchData.
     */
    virtual bool Matches( wxFindReplaceData& aSearchData, void* aAuxData )
    {
        return false;
    }

    /**
     * Helper function used in search and replace dialog
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
    static bool Replace( wxFindReplaceData& aSearchData, wxString& aText );

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
     */
    EDA_ITEM& operator=( const EDA_ITEM& aItem );

    virtual const BOX2I ViewBBox() const override;

    virtual void ViewGetLayers( int aLayers[], int& aCount ) const override;

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

protected:
    /**
     * Function Matches
     * compares \a aText against search criteria in \a aSearchData.
     * This is a helper function for simplify derived class logic.
     *
     * @param aText A reference to a wxString object containing the string to test.
     * @param aSearchData The criteria to search against.
     * @return True if \a aText matches the search criteria in \a aSearchData.
     */
    bool Matches( const wxString& aText, wxFindReplaceData& aSearchData );
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


#endif // BASE_STRUCT_H_
