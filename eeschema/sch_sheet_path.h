/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file sch_sheet_path.h
 * @brief Definition of the SCH_SHEET_PATH and SCH_SHEET_LIST classes for Eeschema.
 */

#ifndef CLASS_DRAWSHEET_PATH_H
#define CLASS_DRAWSHEET_PATH_H

#include <base_struct.h>


/** Info about complex hierarchies handling:
 * A hierarchical schematic uses sheets (hierarchical sheets) included in a
 * given sheet.  Each sheet corresponds to a schematic drawing handled by a
 * SCH_SCREEN structure.  A SCH_SCREEN structure contains drawings, and have
 * a filename to write it's data.  Also a SCH_SCREEN display a sheet number
 * and the name of the sheet.
 *
 * In simple (and flat) hierarchies a sheet is linked to a SCH_SCREEN,
 * and a SCH_SCREEN is used by only one hierarchical sheet.
 *
 * In complex hierarchies the same SCH_SCREEN (and its data) is shared between
 * more than one sheet.  Therefore subsheets (like subsheets in a SCH_SCREEN
 * shared by many sheets) can be also shared.  So the same SCH_SCREEN must
 * handle different components references and parts selection depending on
 * which sheet is currently selected, and how a given subsheet is selected.
 * 2 sheets share the same SCH_SCREEN (the same drawings) if they have the
 * same filename.
 *
 * In KiCad each component and sheet receives (when created) an unique
 * identification called Time Stamp.  So each sheet has 2 ids: its time stamp
 * (that cannot change) and its name ( that can be edited and therefore is
 * not reliable for strong identification).  KiCad uses Time Stamp ( a unique
 * 32 bit id), to identify sheets in hierarchies.
 * A given sheet in a hierarchy is fully labeled by its path (or sheet path)
 * that is the list of time stamp found to access it through the hierarchy
 * the root sheet is /.  All  other sheets have a path like /1234ABCD or
 * /4567FEDC/AA2233DD/.  This path can be displayed as human readable sheet
 * name like: / or /sheet1/include_sheet/ or /sheet2/include_sheet/
 *
 * So to know for a given SCH_SCREEN (a given schematic drawings) we must:
 *   1) Handle all references possibilities.
 *   2) When acceded by a given selected sheet, display (update) the
 *      corresponding references and sheet path
 *
 * The class SCH_SHEET_PATH handles paths used to access a sheet.  The class
 * SCH_SHEET_LIST allows to handle the full (or partial) list of sheets and
 * their paths in a complex hierarchy.  The class EDA_ScreenList allow to
 * handle the list of SCH_SCREEN. It is useful to clear or save data,
 * but is not suitable to handle the full complex hierarchy possibilities
 * (usable in flat and simple hierarchies).
 */


class wxFindReplaceData;
class SCH_SCREEN;
class SCH_MARKER;
class SCH_SHEET;
class SCH_ITEM;
class SCH_REFERENCE_LIST;
class PART_LIBS;


/**
 * Class SCH_SHEET_PATH
 * handles access to a sheet by way of a path.
 * <p>
 * The member m_sheets stores the list of sheets from the first (usually
 * g_RootSheet) to a given sheet in last position.
 * The _last_ sheet is usually the sheet we want to select or reach (which is
 * what the function Last() returns).
 * Others sheets constitute the "path" from the first to the last sheet.
 * </p>
 */
class SCH_SHEET_PATH
{
#define DSLSZ 32          // Max number of levels for a sheet path

    SCH_SHEET* m_sheets[ DSLSZ ];
    unsigned   m_numSheets;

public:
    SCH_SHEET_PATH();

    void Clear()
    {
        m_numSheets = 0;
    }

    unsigned GetSheetsCount()
    {
        return m_numSheets;
    }

    /**
     * Function Cmp
     * Compare if this is the same sheet path as aSheetPathToTest
     * @param aSheetPathToTest = sheet path to compare
     * @return -1 if different, 0 if same
     */
    int Cmp( const SCH_SHEET_PATH& aSheetPathToTest ) const;

    /**
     * Function Last
     * returns a pointer to the last sheet of the list
     * One can see the others sheet as the "path" to reach this last sheet
     */
    SCH_SHEET* Last() const;

    /**
     * Function LastScreen
     * @return the SCH_SCREEN relative to the last sheet in list
     */
    SCH_SCREEN* LastScreen() const;

    /**
     * Function LastDrawList
     * @return a pointer to the first schematic item handled by the
     * SCH_SCREEN relative to the last sheet in list
     */
    SCH_ITEM* LastDrawList() const;

    /**
     * Get the last schematic item relative to the first sheet in the list.
     *
     * @return Last schematic item relative to the first sheet in the list if list
     *         is not empty.  Otherwise NULL.
     */
    SCH_ITEM* FirstDrawList() const;

    /**
     * Function Push
     * store (push) aSheet in list
     * @param aSheet = pointer to the SCH_SHEET to store in list
     * Push is used when entered a sheet to select or analyze it
     * This is like cd &ltdirectory&gt in directories navigation
     */
    void Push( SCH_SHEET* aSheet );

    /**
     * Function Pop
     * retrieves (pop) the last entered sheet and remove it from list
     * @return a SCH_SHEET* pointer to the removed sheet in list
     * Pop is used when leaving a sheet after a selection or analyze
     * This is like cd .. in directories navigation
     */
    SCH_SHEET* Pop();

    /**
     * Function Path
     * the path uses the time stamps which do not changes even when editing
     * sheet parameters
     * a path is something like / (root) or /34005677 or /34005677/00AE4523
     */
    wxString Path() const;

    /**
     * Function PathHumanReadable
     * returns the sheet path in a human readable form, i.e. as a path made
     * from sheet names.  The the "normal" path instead uses the time
     * stamps in the path.  (Time stamps do not change even when editing
     * sheet parameters).
     */
    wxString PathHumanReadable() const;

    /**
     * Function BuildSheetPathInfoFromSheetPathValue
     * Fill this with data to access to the hierarchical sheet known by its path \a aPath
     * @param aPath = path of the sheet to reach (in non human readable format)
     * @param aFound - Please document me.
     * @return true if success else false
     */
    bool BuildSheetPathInfoFromSheetPathValue( const wxString& aPath, bool aFound = false );

    /**
     * Function UpdateAllScreenReferences
     * updates the reference and the m_Multi parameter (part selection) for all
     * components on a screen depending on the actual sheet path.
     * Mandatory in complex hierarchies because sheets use the same screen
     * (basic schematic)
     * but with different references and part selections according to the
     * displayed sheet
     */
    void UpdateAllScreenReferences();

    /**
     * Function AnnotatePowerSymbols
     * annotates the power symbols only starting at \a aReference in the sheet path.
     * @param aReference A pointer to the number for the reference designator of the
     *                   first power symbol to be annotated.  If the pointer is NULL
     *                   the annotation starts at 1.  The number is incremented for
     *                   each power symbol annotated.
     */
    void AnnotatePowerSymbols( PART_LIBS* aLibs, int* aReference );

    /**
     * Function GetComponents
     * adds a SCH_REFERENCE() object to \a aReferences for each component in the sheet.
     * @param aReferences List of references to populate.
     * @param aIncludePowerSymbols Set to false to only get normal components.
     */
    void GetComponents( PART_LIBS* aLibs, SCH_REFERENCE_LIST& aReferences, bool aIncludePowerSymbols = true  );

    /**
     * Function SetFootprintField
     * searches last sheet in the path for a component with \a aReference and set the footprint
     * field to \a aFootPrint if found.
     *
     * @param aReference The reference designator of the component.
     * @param aFootPrint The value to set the footprint field.
     * @param aSetVisible The value to set the field visibility flag.
     * @return True if \a aReference was found otherwise false.
     */
    bool SetComponentFootprint( const wxString& aReference, const wxString& aFootPrint,
                                bool aSetVisible );

    /**
     * Find the next schematic item in this sheet object.
     *
     * @param aType - The type of schematic item object to search for.
     * @param aLastItem - Start search from aLastItem.  If no aLastItem, search from
     *                    the beginning of the list.
     * @param aWrap - Wrap around the end of the list to find the next item if aLastItem
     *                is defined.
     * @return - The next schematic item if found.  Otherwise, NULL is returned.
     */
    SCH_ITEM* FindNextItem( KICAD_T aType, SCH_ITEM* aLastItem = NULL, bool aWrap = false ) const;

    /**
     * Find the previous schematic item in this sheet path object.
     *
     * @param aType - The type of schematic item object to search for.
     * @param aLastItem - Start search from aLastItem.  If no aLastItem, search from
     *                    the end of the list.
     * @param aWrap - Wrap around the beginning of the list to find the next item if aLastItem
     *                is defined.
     * @return - The previous schematic item if found.  Otherwise, NULL is returned.
     */
    SCH_ITEM* FindPreviousItem( KICAD_T aType, SCH_ITEM* aLastItem = NULL, bool aWrap = false ) const;

    SCH_SHEET_PATH& operator=( const SCH_SHEET_PATH& d1 );

    bool operator==( const SCH_SHEET_PATH& d1 ) const;

    bool operator!=( const SCH_SHEET_PATH& d1 ) const { return !( *this == d1 ) ; }
};


/**
 * Class SCH_SHEET_LIST
 * handles the list of Sheets in a hierarchy.
 * Sheets are not unique, there can be many sheets with the same
 * filename and the same SCH_SCREEN reference.
 * The schematic (SCH_SCREEN) is shared between these sheets,
 * and component references are specific to a sheet path.
 * When a sheet is entered, component references and sheet number are updated.
 */
class SCH_SHEET_LIST
{
private:
    SCH_SHEET_PATH* m_List;
    int             m_count;     /* Number of sheets included in hierarchy,
                                  * starting at the given sheet in constructor .
                                  * the given sheet is counted
                                 */
    int             m_index;     /* internal variable to handle GetNext(): cleared by
                                  * GetFirst() and incremented by GetNext() after
                                  * returning the next item in m_List.  Also used for
                                  * internal calculations in BuildSheetList()
                                  */
    bool            m_isRootSheet;
    SCH_SHEET_PATH  m_currList;

public:

    /**
     * Constructor
     * builds the list of sheets from aSheet.
     * If aSheet == NULL (default) build the whole list of sheets in hierarchy.
     * So usually call it with no parameter.
     */
    SCH_SHEET_LIST( SCH_SHEET* aSheet = NULL );

    ~SCH_SHEET_LIST()
    {
        if( m_List )
            delete[] m_List;

        m_List = NULL;
    }

    /**
     * Function GetCount
     * @return the number of sheets in list:
     * usually the number of sheets found in the whole hierarchy
     */
    int GetCount() const { return m_count; }

    /**
     * Function GetFirst
     * @return the first item (sheet) in m_List and prepare calls to GetNext()
     */
    SCH_SHEET_PATH* GetFirst();

    /**
     * Function GetNext
     * @return the next item (sheet) in m_List or NULL if no more item in
     * sheet list
     */
    SCH_SHEET_PATH* GetNext();

    /**
     * Function GetLast
     * returns the last sheet in the sheet list.
     *
     * @return Last sheet in the list or NULL if sheet list is empty.
     */
    SCH_SHEET_PATH* GetLast();

    /**
     * Function GetPrevious
     * returns the previous sheet in the sheet list.
     *
     * @return The previous sheet in the sheet list or NULL if already at the
     *         beginning of the list.
     */
    SCH_SHEET_PATH* GetPrevious();

    /**
     * Function GetSheet
     *
     * @param aIndex A index in sheet list to get the sheet.
     * @return the sheet at \a aIndex position in m_List or NULL if \a aIndex is
     *         outside the bounds of the index list.
     */
    SCH_SHEET_PATH* GetSheet( int aIndex );

    /**
     * Function GetSheet
     * returns a sheet matching the path name in \a aPath.
     *
     * @param aPath A wxString object containing path of the sheet to get.
     * @param aHumanReadable True uses the human readable path for comparison.
     *                       False uses the timestamp generated path.
     * @return The sheet that matches \a aPath or NULL if no sheet matching
     *         \a aPath is found.
     */
    SCH_SHEET_PATH* GetSheet( const wxString aPath, bool aHumanReadable = true );

    /**
     * Function IsModified
     * checks the entire hierarchy for any modifications.
     * @returns True if the hierarchy is modified otherwise false.
     */
    bool IsModified();

    /**
     * Function IsAutoSaveRequired
     * checks the entire hierarchy for any modifications that require auto save.
     * @return True if the hierarchy is modified otherwise false.
     */
    bool IsAutoSaveRequired();

    void ClearModifyStatus();

    /**
     * Function AnnotatePowerSymbols
     * clear and annotates the entire hierarchy of the sheet path list.
     */
    void AnnotatePowerSymbols( PART_LIBS* aLib );

    /**
     * Function GetComponents
     * adds a SCH_REFERENCE() object to \a aReferences for each component in the list
     * of sheets.
     * @param aReferences List of references to populate.
     * @param aIncludePowerSymbols Set to false to only get normal components.
     */
    void GetComponents( PART_LIBS* aLibs, SCH_REFERENCE_LIST& aReferences, bool aIncludePowerSymbols = true  );

    /**
     * Function FindNextItem
     * searches the entire schematic for the next schematic object.
     *
     * @param aType - The type of schematic item to find.
     * @param aSheetFound - The sheet the item was found in.  NULL if the next item
     *                      is not found.
     * @param aLastItem - Find next item after aLastItem if not NULL.
     * @param aWrap - Wrap past around the end of the list of sheets.
     * @return If found, Returns the next schematic item.  Otherwise, returns NULL.
     */
    SCH_ITEM* FindNextItem( KICAD_T aType, SCH_SHEET_PATH** aSheetFound = NULL,
                            SCH_ITEM* aLastItem = NULL, bool aWrap = true );

    /**
     * Function FindPreviousItem
     * searches the entire schematic for the previous schematic item.
     *
     * @param aType - The type of schematic item to find.
     * @param aSheetFound - The sheet the item was found in.  NULL if the previous item
     *                      is not found.
     * @param aLastItem - Find the previous item before aLastItem if not NULL.
     * @param aWrap - Wrap past around the beginning of the list of sheets.
     * @return If found, the previous schematic item.  Otherwise, NULL.
     */
    SCH_ITEM* FindPreviousItem( KICAD_T aType, SCH_SHEET_PATH** aSheetFound = NULL,
                                SCH_ITEM* aLastItem = NULL, bool aWrap = true );

    /**
     * Function SetFootprintField
     * searches all the sheets for a component with \a aReference and set the footprint
     * field to \a aFootPrint if found.
     *
     * @param aReference The reference designator of the component.
     * @param aFootPrint The value to set the footprint field.
     * @param aSetVisible The value to set the field visibility flag.
     * @return True if \a aReference was found otherwise false.
     */
    bool SetComponentFootprint( const wxString& aReference, const wxString& aFootPrint,
                                bool aSetVisible );

    /**
     * Function IsComplexHierarchy
     * searches all of the sheets for duplicate files names which indicates a complex
     * hierarchy.
     *
     * @return true if the #SCH_SHEET_LIST is a complex hierarchy.
     */
    bool IsComplexHierarchy();

private:

    /**
     * Function BuildSheetList
     * builds the list of sheets and their sheet path from \a aSheet.
     * If \a aSheet is the root sheet, the full sheet path and sheet list are built.
     *
     * @param aSheet is the starting sheet from which the list is built, or NULL
     *               indicating that g_RootSheet should be used.
     * @throw std::bad_alloc if the memory for the sheet path list could not be allocated.
     */
    void BuildSheetList( SCH_SHEET* aSheet );
};

#endif // CLASS_DRAWSHEET_PATH_H
