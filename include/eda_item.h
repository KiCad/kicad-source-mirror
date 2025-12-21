/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef EDA_ITEM_H
#define EDA_ITEM_H

#include <deque>

#include <geometry/shape_line_chain.h>
#include <api/serializable.h>
#include <core/typeinfo.h>
#include <eda_item_flags.h>
#include <eda_search_data.h>
#include <view/view_item.h>
#include <kiid.h>

enum class BITMAPS : unsigned int;


enum class INSPECT_RESULT
{
    QUIT,
    CONTINUE
};

enum RECURSE_MODE
{
    RECURSE,
    NO_RECURSE,
};

#define IGNORE_PARENT_GROUP false

/**
 * Additional flag values wxFindReplaceData::m_Flags
 */
class UNITS_PROVIDER;
class EDA_DRAW_FRAME;
class EDA_GROUP;
class MSG_PANEL_ITEM;
class EMBEDDED_FILES;

namespace google { namespace protobuf { class Any; } }


/**
 * Used to inspect and possibly collect the (search) results of iterating over a list or
 * tree of #KICAD_T objects.
 *
 * Provide an implementation as needed to inspect EDA_ITEMs visited via #EDA_ITEM::Visit()
 * and #EDA_ITEM::IterateForward().
 *
 * FYI the std::function may hold a lambda, std::bind, pointer to func, or ptr to member
 * function, per modern C++. It is used primarily for searching, but not limited to that.
 * It can also collect or modify the scanned objects.  'Capturing' lambdas are particularly
 * convenient because they can use context and this often means @a aTestData is not used.
 *
 * @param aItem An #EDA_ITEM to examine.
 * @param aTestData is arbitrary data needed by the inspector to determine
 *  if the EDA_ITEM under test meets its match criteria, and is often NULL
 *  with the advent of capturing lambdas.
 * @return A #SEARCH_RESULT type #SEARCH_QUIT if the iterator function is to
 *          stop the scan, else #SEARCH_CONTINUE;
 */
typedef std::function< INSPECT_RESULT ( EDA_ITEM* aItem, void* aTestData ) > INSPECTOR_FUNC;

/// std::function passed to nested users by ref, avoids copying std::function.
typedef const INSPECTOR_FUNC& INSPECTOR;


/**
 * A base class for most all the KiCad significant classes used in schematics and boards.
 */
class EDA_ITEM : public KIGFX::VIEW_ITEM, public SERIALIZABLE
{
public:
    virtual ~EDA_ITEM() = default;

    /**
     * Returns the type of object.
     *
     * This attribute should never be changed after a ctor sets it, so there is no public
     * "setter" method.
     *
     * @return the type of object.
     */
    inline KICAD_T Type() const { return m_structType; }

    EDA_ITEM* GetParent() const { return m_parent; }
    virtual void SetParent( EDA_ITEM* aParent )   { m_parent = aParent; }

    virtual void SetParentGroup( EDA_GROUP* aGroup ) { m_group = aGroup; }
    virtual EDA_GROUP* GetParentGroup() const { return m_group; }

    KIID GetParentGroupId() const;

    virtual bool IsLocked() const { return false; }
    virtual void SetLocked( bool aLocked ) {}

    inline bool IsModified() const { return m_flags & IS_CHANGED; }
    inline bool IsNew() const { return m_flags & IS_NEW; }
    inline bool IsMoving() const { return m_flags & IS_MOVING; }

    inline bool IsSelected() const { return m_flags & SELECTED; }
    inline bool IsEntered() const { return m_flags & ENTERED; }
    inline bool IsBrightened() const { return m_flags & BRIGHTENED; }

    inline bool IsRollover() const { return m_isRollover; }
    inline VECTOR2I GetRolloverPos() const { return m_rolloverPos; }
    inline void SetIsRollover( bool aIsRollover, const VECTOR2I& aMousePos )
    {
        m_isRollover = aIsRollover;
        m_rolloverPos = aMousePos;
    }
    inline void SetActiveUrl( const wxString& aUrl ) const { m_activeUrl = aUrl; }

    inline void SetSelected() { SetFlags( SELECTED ); }
    inline void SetBrightened() { SetFlags( BRIGHTENED ); }

    inline void ClearSelected() { ClearFlags( SELECTED ); }
    inline void ClearBrightened() { ClearFlags( BRIGHTENED ); }

    void SetModified();

    void           SetFlags( EDA_ITEM_FLAGS aMask ) { m_flags |= aMask; }
    void           XorFlags( EDA_ITEM_FLAGS aMask ) { m_flags ^= aMask; }
    void           ClearFlags( EDA_ITEM_FLAGS aMask = EDA_ITEM_ALL_FLAGS ) { m_flags &= ~aMask; }
    EDA_ITEM_FLAGS GetFlags() const { return m_flags; }
    bool           HasFlag( EDA_ITEM_FLAGS aFlag ) const { return ( m_flags & aFlag ) == aFlag; }

    EDA_ITEM_FLAGS GetEditFlags() const
    {
        constexpr int mask =
                ( IS_NEW | IS_PASTED | IS_MOVING | IS_BROKEN | IS_CHANGED | STRUCT_DELETED );

        return m_flags & mask;
    }

    virtual void ClearEditFlags()
    {
        ClearFlags( GetEditFlags() );
    }

    EDA_ITEM_FLAGS GetTempFlags() const
    {
        constexpr int mask = ( CANDIDATE | SELECTED_BY_DRAG | IS_LINKED | SKIP_STRUCT | SELECTION_CANDIDATE
                               | CONNECTIVITY_CANDIDATE );

        return m_flags & mask;
    }

    virtual void ClearTempFlags()
    {
        ClearFlags( GetTempFlags() );
    }

    virtual bool RenderAsBitmap( double aWorldScale ) const { return false; }

    void SetIsShownAsBitmap( bool aBitmap )
    {
        if( aBitmap )
            SetFlags( IS_SHOWN_AS_BITMAP );
        else
            ClearFlags( IS_SHOWN_AS_BITMAP );
    }

    inline bool IsShownAsBitmap() const { return m_flags & IS_SHOWN_AS_BITMAP; }

    /**
     * Check whether the item is one of the listed types.
     *
     * @param aScanTypes List of item types
     * @return true if the item type is contained in the list aScanTypes
     */
    virtual bool IsType( const std::vector<KICAD_T>& aScanTypes ) const
    {
        for( KICAD_T scanType : aScanTypes )
        {
            if( scanType == SCH_LOCATE_ANY_T || scanType == m_structType )
                return true;
        }

        return false;
    }

    /**
     * Set and clear force visible flag used to force the item to be drawn even if it's draw
     * attribute is set to not visible.
     *
     * @param aEnable True forces the item to be drawn.  False uses the item's visibility
     *                setting to determine if the item is to be drawn.
     */
    void SetForceVisible( bool aEnable ) { m_forceVisible = aEnable; }
    bool IsForceVisible() const { return m_forceVisible; }

    /**
     * Populate \a aList of #MSG_PANEL_ITEM objects with it's internal state for display
     * purposes.
     *
     * @param aFrame is the EDA_DRAW_FRAME that displays the message panel
     * @param aList is the list to populate.
     */
    virtual void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
    {
    }

    virtual wxString GetFriendlyName() const;

    /**
     * Test if \a aPosition is inside or on the boundary of this item.
     *
     * @param aPosition A reference to a VECTOR2I object containing the coordinates to test.
     * @param aAccuracy Increase the item bounding box by this amount.
     * @return True if \a aPosition is within the item bounding box.
     */
    virtual bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const
    {
        return false;   // derived classes should override this function
    }

    /**
     * Test if \a aRect intersects this item.
     *
     * @param aRect A reference to a #BOX2I object containing the rectangle to test.
     * @param aContained Set to true to test for containment instead of an intersection.
     * @param aAccuracy Increase \a aRect by this amount.
     * @return True if \a aRect contains or intersects the item bounding box.
     */
    virtual bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy = 0 ) const
    {
        return false;   // derived classes should override this function
    }

    /**
     * Test if \a aPoly intersects this item.
     *
     * @param aPoly A reference to a #SHAPE_LINE_CHAIN object containing the polygon or polyline to test.
     * @param aContained Set to true to test for containment instead of an intersection.
     * @return True if \a aPoly contains or intersects the item.
     */
    virtual bool HitTest( const SHAPE_LINE_CHAIN& aPoly, bool aContained ) const
    {
        return false; // derived classes should override this function
    }

    /**
     * Return the orthogonal bounding box of this object for display purposes.
     *
     * This box should be an enclosing perimeter for visible components of this
     * object, and the units should be in the pcb or schematic coordinate
     * system.  It is OK to overestimate the size by a few counts.
     */
    virtual const BOX2I GetBoundingBox() const;

    virtual VECTOR2I GetPosition() const { return VECTOR2I(); }
    virtual void     SetPosition( const VECTOR2I& aPos ){};

    /**
     * Similar to GetPosition() but allows items to return their visual center rather
     * than their anchor.
     */
    virtual const VECTOR2I GetFocusPosition() const { return GetPosition(); }

    /**
     * Return the coordinates that should be used for sorting this element
     * visually compared to other elements. For instance, for lines the midpoint
     * might be a better sorting point than either end.
     *
     * @return X,Y coordinate of the sort point
     */
    virtual VECTOR2I GetSortPosition() const { return GetPosition(); }

    /**
     * Create a duplicate of this item with linked list members set to NULL.
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
    virtual EDA_ITEM* Clone() const;

    /**
     * May be re-implemented for each derived class in order to handle all the types given
     * by its member data.
     *
     * Implementations should call inspector->Inspect() on types in aScanTypes, and may use
     * #IterateForward() to do so on lists of such data.
     *
     * @param inspector An #INSPECTOR instance to use in the inspection.
     * @param testData Arbitrary data used by the inspector.
     * @param aScanTypes Which #KICAD_T types are of interest and the order in which they should
     *                   be processed.
     * @return #SEARCH_RESULT SEARCH_QUIT if the Iterator is to stop the scan,
     *         else #SCAN_CONTINUE, and determined by the inspector.
     */
    virtual INSPECT_RESULT Visit( INSPECTOR inspector, void* testData,
                                  const std::vector<KICAD_T>& aScanTypes );

    /**
     * This changes first parameter to avoid the DList and use the main queue instead.
     */
    template< class T >
    static INSPECT_RESULT IterateForward( std::deque<T>& aList, INSPECTOR inspector, void* testData,
                                          const std::vector<KICAD_T>& scanTypes )
    {
        for( const auto& it : aList )
        {
            EDA_ITEM* item = static_cast<EDA_ITEM*>( it );

            if( item && item->Visit( inspector, testData, scanTypes ) == INSPECT_RESULT::QUIT )
            {
                return INSPECT_RESULT::QUIT;
            }
        }

        return INSPECT_RESULT::CONTINUE;
    }

    /**
     * Change first parameter to avoid the DList and use std::vector instead.
     */
    template <class T>
    static INSPECT_RESULT IterateForward( std::vector<T>& aList, INSPECTOR inspector,
                                          void* testData, const std::vector<KICAD_T>& scanTypes )
    {
        for( const auto& it : aList )
        {
            EDA_ITEM* item = static_cast<EDA_ITEM*>( it );

            if( item && item->Visit( inspector, testData, scanTypes ) == INSPECT_RESULT::QUIT )
            {
                return INSPECT_RESULT::QUIT;
            }
        }

        return INSPECT_RESULT::CONTINUE;
    }

    /**
     * Return a translated description of the type for this EDA_ITEM for display in user facing
     * messages.
     */
    wxString GetTypeDesc() const;

    /**
     * Return a user-visible description string of this item.  This description is used in
     * disambiguation menus, the message panel, ERC/DRC reports, etc.
     *
     * The default version of this function raises an assertion in the debug mode and
     * returns a string to indicate that it was not overridden to provide the object
     * specific text.
     *
     * @param aLong indicates a long string is acceptable
     * @return The menu text string.
     */
    virtual wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const;

    /**
     * Return a pointer to an image to be used in menus.
     *
     * The default version returns the right arrow image.  Override this function to provide
     * object specific menu images.
     *
     * @return The menu image associated with the item.
     */
    virtual BITMAPS GetMenuImage() const;

    /**
     * Compare the item against the search criteria in \a aSearchData.
     *
     * The base class returns false since many of the objects derived from EDA_ITEM
     * do not have any text to search.
     *
     * @param aSearchData A reference to a wxFindReplaceData object containing the
     *                    search criteria.
     * @param aAuxData A pointer to optional data required for the search or NULL if not used.
     * @return True if the item's text matches the search criteria in \a aSearchData.
     */
    virtual bool Matches( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) const
    {
        return false;
    }

    /**
     * Perform a text replace on \a aText using the find and replace criteria in
     * \a aSearchData on items that support text find and replace.
     *
     * @param aSearchData A reference to a wxFindReplaceData object containing the
     *                    search and replace criteria.
     * @param aText A reference to a wxString object containing the text to be replaced.
     * @return True if \a aText was modified, otherwise false.
     */
    static bool Replace( const EDA_SEARCH_DATA& aSearchData, wxString& aText );

    /**
     * Perform a text replace using the find and replace criteria in \a aSearchData
     * on items that support text find and replace.
     *
     * This function must be overridden for items that support text replace.
     *
     * @param aSearchData A reference to a wxFindReplaceData object containing the search and
     *                    replace criteria.
     * @param aAuxData A pointer to optional data required for the search or NULL if not used.
     * @return True if the item text was modified, otherwise false.
     */
    virtual bool Replace( const EDA_SEARCH_DATA& aSearchData, void* aAuxData = nullptr )
    {
        return false;
    }

    /**
     * Override this method in any derived object that supports test find and replace.
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
     * Helper function to be used by the C++ STL sort algorithm for sorting a STL
     * container of #EDA_ITEM pointers.
     *
     * @param aLeft The left hand item to compare.
     * @param aRight The right hand item to compare.
     * @return True if \a aLeft is less than \a aRight.
     */
    static bool Sort( const EDA_ITEM* aLeft, const EDA_ITEM* aRight ) { return *aLeft < *aRight; }

    /**
     * Assign the members of \a aItem to another object.
     */
    EDA_ITEM& operator=( const EDA_ITEM& aItem );

    virtual const BOX2I ViewBBox() const override;

    virtual std::vector<int> ViewGetLayers() const override;

    virtual EMBEDDED_FILES* GetEmbeddedFiles() { return nullptr; }
    virtual const std::vector<wxString>* GetEmbeddedFonts() { return nullptr; }

#if defined(DEBUG)

    /**
     * Output the object tree, currently for debugging only.
     *
     * This is pure virtual so compiler warns if somebody mucks up a derived declaration.
     *
     * @param nestLevel An aid to prettier tree indenting, and is the level
     *                  of nesting of this object within the overall tree.
     * @param os The ostream& to output to.
     */
    virtual void Show( int nestLevel, std::ostream& os ) const { ShowDummy( os ); };

    void ShowDummy( std::ostream& os ) const;  ///< call this if you are a lazy developer

    /**
     * Output nested space for pretty indenting.
     *
     * @param nestLevel The nest count.
     * @param os The ostream&, where to output
     * @return The std::ostream& for continuation.
     **/
    static std::ostream& NestedSpace( int nestLevel, std::ostream& os );

#endif

protected:
    EDA_ITEM( EDA_ITEM* parent, KICAD_T idType, bool isSCH_ITEM = false, bool isBOARD_ITEM = false );
    EDA_ITEM( KICAD_T idType, bool isSCH_ITEM = false, bool isBOARD_ITEM = false );
    EDA_ITEM( const EDA_ITEM& base );

    /**
     * Compare \a aText against search criteria in \a aSearchData.
     *
     * This is a helper function for simplify derived class logic.
     *
     * @param aText A reference to a wxString object containing the string to test.
     * @param aSearchData The criteria to search against.
     * @return True if \a aText matches the search criteria in \a aSearchData.
     */
    bool Matches( const wxString& aText, const EDA_SEARCH_DATA& aSearchData ) const;

    EDA_ITEM* findParent( KICAD_T aType ) const;

public:
    const KIID  m_Uuid;

private:
    /**
     * Run time identification, _keep private_ so it can never be changed after a ctor sets it.
     *
     * See comment near SetType() regarding virtual functions.
     */
    KICAD_T        m_structType;

protected:
    EDA_ITEM_FLAGS m_flags;
    EDA_ITEM*      m_parent;        ///< Owner.
    EDA_GROUP*     m_group;         ///< The group this item belongs to, if any.  No ownership implied.
    bool           m_forceVisible;

    bool             m_isRollover;
    VECTOR2I         m_rolloverPos;
    mutable wxString m_activeUrl;
};


/**
 * Provide cloning capabilities for all Boost pointer containers of #EDA_ITEM pointers.
 *
 * @param aItem EDA_ITEM to clone.
 * @return Clone of \a aItem.
 */
inline EDA_ITEM* new_clone( const EDA_ITEM& aItem ) { return aItem.Clone(); }

/**
 * Comparison functor for sorting EDA_ITEM pointers by their UUID.
 */
struct CompareByUuid
{
    bool operator()(const EDA_ITEM* item1, const EDA_ITEM* item2) const
    {
        assert( item1 != nullptr && item2 != nullptr );

        if( item1->m_Uuid == item2->m_Uuid )
            return item1 < item2;

        return item1->m_Uuid < item2->m_Uuid;
    }
};


/**
 * Define list of drawing items for screens.
 *
 * The standard C++ container was chosen so the pointer can be removed  from a list without
 * it being destroyed.
 */
typedef std::vector< EDA_ITEM* > EDA_ITEMS;

typedef std::set< EDA_ITEM*, CompareByUuid > EDA_ITEM_SET;

#endif // EDA_ITEM_H
