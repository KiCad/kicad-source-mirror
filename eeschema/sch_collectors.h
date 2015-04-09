/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2011-2015 KiCad Developers, see change_log.txt for contributors.
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
 * @file sch_collectors.h
 */

#ifndef _SCH_COLLECTORS_H_
#define _SCH_COLLECTORS_H_


#include <class_collector.h>
#include <sch_item_struct.h>
#include <dialogs/dialog_schematic_find.h>


/**
 * Class SCH_COLLECTOR
 */
class SCH_COLLECTOR : public COLLECTOR
{
public:

    /**
     * A scan list for all schematic items.
     */
    static const KICAD_T AllItems[];

    /**
     * A scan list for all editable schematic items.
     */
    static const KICAD_T EditableItems[];

    /**
     * A scan list for a specific editable field: Value.
     */
    static const KICAD_T CmpFieldValueOnly[];

    /**
     * A scan list for a specific editable field: Reference.
     */
    static const KICAD_T CmpFieldReferenceOnly[];

    /**
     * A scan list for a specific editable field: Footprint.
     */
    static const KICAD_T CmpFieldFootprintOnly[];

    /**
     * A scan list for all movable schematic items.
     */
    static const KICAD_T MovableItems[];

    /**
     * A scan list for all draggable schematic items.
     */
    static const KICAD_T DraggableItems[];

    /**
     * A scan list for all rotatable schematic items.
     */
    static const KICAD_T RotatableItems[];

    /**
     * A scan list for only parent schematic items.
     */
    static const KICAD_T ParentItems[];

    /**
     * A scan list for all schematic items except pins.
     */
    static const KICAD_T AllItemsButPins[];

    /**
     * A scan list for schematic component items only.
     */
    static const KICAD_T ComponentsOnly[];

    /**
     * A scan list for schematic sheet items only.
     */
    static const KICAD_T SheetsOnly[];

    /**
     * A scan list for schematic sheet and sheet label items.
     */
    static const KICAD_T SheetsAndSheetLabels[];

    /**
     * A scan list for schematic items that can be mirrored.
     */
    static const KICAD_T OrientableItems[];

    /**
     * Constructor SCH_COLLECTOR
     */
    SCH_COLLECTOR( const KICAD_T* aScanTypes = SCH_COLLECTOR::AllItems )
    {
        SetScanTypes( aScanTypes );
    }

    /**
     * Operator []
     * overloads COLLECTOR::operator[](int) to return a SCH_ITEM* instead of
     * an EDA_ITEM* type.
     * @param aIndex The index into the list.
     * @return SCH_ITEM* at \a aIndex or NULL.
     */
    SCH_ITEM* operator[]( int aIndex ) const
    {
        if( (unsigned)aIndex < (unsigned)GetCount() )
            return (SCH_ITEM*) m_List[ aIndex ];

        return NULL;
    }

    SEARCH_RESULT Inspect( EDA_ITEM* aItem, const void* aTestData = NULL );

    /**
     * Function Collect
     * scans a SCH_ITEM using this class's Inspector method, which does the collection.
     * @param aItem A SCH_ITEM to scan.
     * @param aFilterList A list of #KICAD_T types with a terminating #EOT, that determines
     *                    what is to be collected and the priority order of the resulting
     *                    collection.
     * @param aPosition A wxPoint to use in hit-testing.
     */
    void Collect( SCH_ITEM* aItem, const KICAD_T aFilterList[], const wxPoint& aPosition );

    /**
     * Function IsCorner
     * tests if the collected items forms as corner of two line segments.
     * @return True if the collected items form a corner of two line segments.
     */
    bool IsCorner() const;

    /**
     * Function IsNode
     * tests if the collected items form a node.
     *
     * @param aIncludePins Indicate if component pin items should be included in the test.
     * @return True if the collected items form a node.
     */
    bool IsNode( bool aIncludePins = true ) const;

    /**
     * Function IsDraggableJunction
     * tests to see if the collected items form a draggable junction.
     * <p>
     * Daggable junctions are defined as:
     * <ul>
     * <li> The intersection of three or more wire end points. </li>
     * <li> The intersection of one or more wire end point and one wire mid point. </li>
     * <li> The crossing of two or more wire mid points and a junction. </li>
     * </ul>
     * </p>
     * @return True if the collection is a draggable junction.
     */
    bool IsDraggableJunction() const;
};


/**
 * Class SCH_FIND_COLLECTOR_DATA
 * is used as a data container for the associated item found by the #SCH_FIND_COLLECTOR
 * object.
 */
class SCH_FIND_COLLECTOR_DATA
{
    /// The position in drawing units of the found item.
    wxPoint m_position;

    /// The human readable sheet path @see SCH_SHEET_PATH::PathHumanReadable() of the found item.
    wxString m_sheetPath;

    /// The parent object if the item found is a child object.
    SCH_ITEM* m_parent;

public:
    SCH_FIND_COLLECTOR_DATA( const wxPoint& aPosition = wxDefaultPosition,
                             const wxString& aSheetPath = wxEmptyString,
                             SCH_ITEM* aParent = NULL )
        : m_position( aPosition )
        , m_sheetPath( aSheetPath )
        , m_parent( aParent )
    { }

    wxPoint GetPosition() const { return m_position; }

    wxString GetSheetPath() const { return m_sheetPath; }

    SCH_ITEM* GetParent() const { return m_parent; }
};


/**
 * Class SCH_FIND_COLLECTOR
 * is used to iterate over all of the items in a schematic or sheet and collect all
 * the items that match the given search criteria.
 */
class SCH_FIND_COLLECTOR : public COLLECTOR
{
    /// Data associated with each found item.
    std::vector< SCH_FIND_COLLECTOR_DATA > m_data;

    /// The criteria used to test for matching items.
    SCH_FIND_REPLACE_DATA m_findReplaceData;

    /// The path of the sheet currently being iterated over.
    SCH_SHEET_PATH* m_sheetPath;

    /// The current found item list index.
    int     m_foundIndex;

    /// A flag to indicate that the schemtic has been modified and a new search must be
    /// performed even if the search criteria hasn't changed.
    bool    m_forceSearch;

    /// last known library change hash, used to detect library changes which
    /// should trigger cache obsolescence.
    int     m_lib_hash;

    /**
     * Function dump
     * is a helper to dump the items in the find list for debugging purposes.
     */
#if defined(DEBUG)
    void dump();
#endif

public:

    /**
     * Constructor SCH_FIND_COLLECTOR
     */
    SCH_FIND_COLLECTOR( const KICAD_T* aScanTypes = SCH_COLLECTOR::AllItems )
    {
        SetScanTypes( aScanTypes );
        m_foundIndex = 0;
        SetForceSearch( false );
        m_sheetPath = NULL;
    }

    void Empty()
    {
        m_foundIndex = 0;
        COLLECTOR::Empty();
        m_data.clear();
    }

    void SetForceSearch( bool doSearch = true ) { m_forceSearch = doSearch; }

    int GetLibHash() const          { return m_lib_hash; }
    void SetLibHash( int aHash )    { m_lib_hash = aHash; }

    /**
     * Function PassedEnd
     * tests if #m_foundIndex is beyond the end of the list give the current
     * find/replace criterial in #m_findReplaceData.
     *
     * @return True if #m_foundIndex has crossed the end of the found item list.
     */
    bool PassedEnd() const;

    /**
     * Function UpdateIndex
     * updates the list index according to the current find and replace criteria.
     */
    void UpdateIndex();

    /**
     * Function GetFindData
     * returns the data associated with the item found at \a aIndex.
     *
     * @param aIndex The list index of the data to return.
     * @return The associated found item data at \a aIndex if \a aIndex is within the
     *         list limits.  Otherwise an empty data item will be returned.
     */
    SCH_FIND_COLLECTOR_DATA GetFindData( int aIndex );

    /**
     * Function IsSearchRequired
     * checks the current collector state against \a aFindReplaceData to see if a new search
     * needs to be performed to update the collector.
     *
     * @param aFindReplaceData A #SCH_FIND_REPLACE_DATA object containing the search criteria
     *                         to test for changes against the current search criteria.
     * @return True if \a aFindReplaceData would require a new search to be performaed or
     *         the force search flag is true.  Otherwise, false is returned.
     */
    bool IsSearchRequired( const SCH_FIND_REPLACE_DATA& aFindReplaceData )
    {
        return m_findReplaceData.ChangesCompare( aFindReplaceData ) || m_forceSearch ||
               (m_findReplaceData.IsWrapping() != aFindReplaceData.IsWrapping());
    }

    /**
     * Function GetText()
     * @return A wxString object containing the description of the item found at the
     *         current index or a wxEmptyString if the list is empty or the index is
     *         invalid.
     */
    wxString GetText();

    /**
     * Function GetItem
     * returns the item and associated data of the current index.
     *
     * @param aFindData A reference to a #SCH_FIND_COLLECTOR_DATA object to place the
     *                  associated data for the current item into if the current item
     *                  index is valid.
     * @return A pointer to the current #EDA_ITEM in the list if the list index is valid
     *         Otherwise NULL is returned and the \a aFindData object is not updated.
     */
    EDA_ITEM* GetItem( SCH_FIND_COLLECTOR_DATA& aFindData );

    /**
     * Function ReplaceItem
     * performs a string replace of the item at the current index.
     *
     * @return True if the text replace occurred otherwise false.
     */
    bool ReplaceItem( SCH_SHEET_PATH* aSheetPath = NULL );

    SEARCH_RESULT Inspect( EDA_ITEM* aItem, const void* aTestData = NULL );

    /**
     * Function Collect
     * scans \a aSheetPath using this class's Inspector method for items matching
     * \a aFindReplaceData.
     *
     * @param aFindReplaceData A #SCH_FIND_REPLACE_DATA object containing the search criteria.
     * @param aSheetPath A pointer to a #SCH_SHEET_PATH object to test for matches.  A NULL
     *                   value searches the entire schematic hierarchy.
     */
    void Collect( SCH_FIND_REPLACE_DATA& aFindReplaceData, SCH_SHEET_PATH* aSheetPath = NULL );

    void IncrementIndex() { m_foundIndex += 1; }
};


/**
 * Class TYPE_COLLECTOR
 * merely gathers up all SCH_ITEMs of a given set of KICAD_T type(s).  It does
 * no hit-testing.
 *
 * @see class COLLECTOR
 */
class SCH_TYPE_COLLECTOR : public SCH_COLLECTOR
{
public:
    /**
     * Function Inspect
     * is the examining function within the INSPECTOR which is passed to the
     * Iterate function.
     *
     * @param testItem An EDA_ITEM to examine.
     * @param testData is not used in this class.
     * @return SEARCH_RESULT - SEARCH_QUIT if the Iterator is to stop the scan,
     *   else SCAN_CONTINUE;
     */
    SEARCH_RESULT Inspect( EDA_ITEM* testItem, const void* testData );

    /**
     * Function Collect
     * scans a BOARD_ITEM using this class's Inspector method, which does
     * the collection.
     * @param aBoard The BOARD_ITEM to scan.
     * @param aScanList The KICAD_Ts to gather up.
     */
    void Collect( SCH_ITEM* aBoard, const KICAD_T aScanList[] );
};


#endif // _SCH_COLLECTORS_H_
