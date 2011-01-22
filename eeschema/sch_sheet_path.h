/********************************************/
/* Definitions for the EESchema program:    */
/********************************************/

#ifndef CLASS_DRAWSHEET_PATH_H
#define CLASS_DRAWSHEET_PATH_H

#include "sch_sheet.h"

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
 * In Kicad each component and sheet receives (when created) an unique
 * identification called Time Stamp.  So each sheet has 2 ids: its time stamp
 * (that cannot change) and its name ( that can be edited and therefore is
 * not reliable for strong identification).  Kicad uses Time Stamp ( a unique
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
class SCH_ITEM;
class SCH_REFERENCE_LIST;


/**
 * Class SCH_SHEET_PATH
 * handles access to a hierarchical sheet by way of a path.
 * <p>
 * The sheets are stored from the first (usually the root sheet) to the last sheet in
 * the hierarchy.  The _last_ sheet is usually the sheet used to select or reach the
 * data for the sheet (which is what the function Last() returns).  The other sheets
 * constitute the "path" to the last sheet.
 */
class SCH_SHEET_PATH
{
    SCH_SHEETS m_sheets;  ///< The list of sheets used to create this sheet path.

public:
#define MAX_SHEET_PATH_DEPTH 32          // Max number of levels for a sheet path

    SCH_SHEET_PATH();

    ~SCH_SHEET_PATH();

    void Clear();

    unsigned GetSheetCount() const { return m_sheets.size(); }

    /**
     * Function Cmp
     * compares if the sheet path is the same as the sheet path of \a aSheetPathToTest
     * @param aSheetPathToTest Sheet path to compare.
     * @return -1 is less than, 0 if same as, or 1 if greater than.
     */
    int Cmp( const SCH_SHEET_PATH& aSheetPathToTest ) const;

    /**
     * Function Last
     * returns a pointer to the last sheet of the list.
     * One can see the others sheet as the "path" to reach this last sheet.
     */
    SCH_SHEET* Last();

    /**
     * Function LastScreen
     * @return The SCH_SCREEN relative to the last sheet in the hierarchy.
     */
    SCH_SCREEN* LastScreen();

    /**
     * Function LastDrawItem
     * @return The first item in the draw list of the screen associated with the last
     * sheet in the hierarchy.
     */
    SCH_ITEM* LastDrawList();

    /**
     * Get the last schematic item relative to the first sheet in the list.
     *
     * @return Last schematic item relative to the first sheet in the list if list
     *         is not empty.  Otherwise NULL.
     */
    SCH_ITEM* FirstDrawList();

    /**
     * Function Push
     * adds \a aSheet to the end of the hierarchy.
     * @param aSheet The SCH_SHEET to store in the hierarchy.
     * Push is used to enter a sheet to select or analyze it.  This is like using
     * cd &ltdirectory&gt to navigate a directory.
     */
    void Push( SCH_SHEET* aSheet );

    /**
     * Function Pop
     * removes and returns the last sheet in the hierarchy.
     * @return The last in the hierarchy or NULL if there are no sheets in the hierarchy.
     * <p>
     * Pop is used when leaving a sheet after a has been selected or analyzed.  This is
     * like using cd .. to navigate a directory.
     */
    SCH_SHEET* Pop();

    /**
     * Function Path
     * returns the path using the time stamps which do not changes even when editing
     * sheet parameters.
     * Sample paths include / (root) or /34005677 or /34005677/00AE4523
     */
    wxString Path() const;

    /**
     * Function PathHumanReadable
     * returns the sheet path in a human readable form, i.e. as a path made from sheet
     * names.  This is the "normal" path instead of the path that uses the time stamps
     * in the path.  Time stamps do not change even when the sheet name is changed.
     */
    wxString PathHumanReadable() const;

    /**
     * Function BuildSheetPathInfoFromSheetPathValue
     * creates the hierarchy to access the sheet known by \a aPath.
     * @param aPath = path of the sheet to reach (in non human readable format)
     * @param aFound - Please document me.
     * @return true if success else false
     */
    bool BuildSheetPathInfoFromSheetPathValue( const wxString& aPath, bool aFound = false );

    /**
     * Function UpdateAllScreenReferences
     * updates the reference and the part selection parameter for all components with
     * multiple parts on a screen depending on the actual sheet path.  This is required
     * in complex hierarchies because sheets use the same screen with different references
     * and part selections according to the displayed sheet.
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
    void AnnotatePowerSymbols( int* aReference );

    /**
     * Function GetComponents
     * adds a SCH_REFERENCE() object to \a aReferences for each component in the sheet.
     * @param aReferences List of references to populate.
     * @param aIncludePowerSymbols Set to false to only get normal components.
     */
    void GetComponents( SCH_REFERENCE_LIST& aReferences,
                        bool                aIncludePowerSymbols = true  );

    /**
     * Function FindNextItem
     * searches for the next schematic item of \a aType in the hierarchy.
     *
     * @param aType The type of schematic item object to search for.
     * @param aLastItem Start search from aLastItem.  If no aLastItem, search from
     *                  the beginning of the list.
     * @param aWrap Wrap around the end of the list to find the next item if aLastItem
     *                is defined.
     * @return The next schematic item if found.  Otherwise, NULL is returned.
     */
    SCH_ITEM* FindNextItem( KICAD_T aType, SCH_ITEM* aLastItem = NULL, bool aWrap = false );

    /**
     * Fucntion FindPreviousItem
     * searched for the previous schematic item of \a aType in the hierarchy.
     *
     * @param aType The type of schematic item object to search for.
     * @param aLastItem Start search from aLastItem.  If no aLastItem, search from
     *                  the end of the list.
     * @param aWrap Wrap around the beginning of the list to find the next item if aLastItem
     *              is defined.
     * @return The previous schematic item if found.  Otherwise, NULL is returned.
     */
    SCH_ITEM* FindPreviousItem( KICAD_T aType, SCH_ITEM* aLastItem = NULL, bool aWrap = false );

    /**
     * Funnction MatchNextItem
     * searches the hierarchy for the next item that matches the search criteria
     * \a aSeaechDate.
     *
     * @param aSearchData Criteria to search item against.
     * @param aLastItem Find next item after aLastItem if not NULL.
     * @param aFindLocation The location where to put the location of matched item.  Can be NULL.
     * @return The next schematic item if found.  Otherwise, returns NULL.
     */
    SCH_ITEM* MatchNextItem( wxFindReplaceData& aSearchData, SCH_ITEM* aLastItem,
                             wxPoint* aFindLocation );

    bool operator==( const SCH_SHEET_PATH& d1 ) const;

    bool operator!=( const SCH_SHEET_PATH& d1 ) const { return !( *this == d1 ); }
};


/**
 * Class SCH_SHEET_LIST
 * handles a list of hierarchies.
 * <p>
 * Sheets may not be unique.  There can be many sheets that share the same file name and
 * SCH_SCREEN reference.  When a file is shared between sheets the component references
 * are specific to where the sheet is in the hierarchy.   When a sheet is entered, the
 * component references and sheet number in the screen are updated to reflect the sheet
 * path.  If the hierarchy is created with the root sheet, the hierarchy represents the
 * entire schematic.
 * </p>
 */
class SCH_SHEET_LIST
{
private:
    SCH_SHEET_PATH* m_List;
    int             m_count;     /* Number of sheets included in hierarchy,
                                  * starting at the given sheet in constructor .
                                  * the given sheet is counted
                                 */
    int             m_index;     /* internal variable to handle GetNext():
                                  * cleared by GetFirst()
                                  *  and incremented by GetNext() after
                                  * returning the next item in m_List
                                  * Also used for internal calculations in
                                  * BuildSheetList()
                                  */
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
            free( m_List );
        m_List = NULL;
    }

    /**
     * Function GetCount
     * @return The number of sheets in the hierarchy.
     * usually the number of sheets found in the whole hierarchy
     */
    int GetCount() { return m_count; }

    /**
     * Function GetFirst
     * @return The first hierarchical path and prepare for calls to GetNext().
     */
    SCH_SHEET_PATH* GetFirst();

    /**
     * Function GetNext
     * @return The next hierarchical path or NULL if the end of the list.
     */
    SCH_SHEET_PATH* GetNext();

    /**
     * Function GetLast
     * returns the last sheet in the hierarchy.
     *
     * @return Last sheet in the hierarchy or NULL if the hierarchy is empty.
     */
    SCH_SHEET_PATH* GetLast();

    /**
     * Function GetPrevious
     * returns the previous sheet in the hierarchy.
     *
     * @return The previous sheet in the sheet list or NULL if already at the
     *         beginning of the list.
     */
    SCH_SHEET_PATH* GetPrevious();

    /**
     * Function GetSheet
     * @return The hierarchy at \a aIndex position or NULL if \a aIndex is out of range.
     * @param aIndex Index in the list of hierarchies.
     */
    SCH_SHEET_PATH* GetSheet( int aIndex );

    /**
     * Function IsModified
     * checks the entire hierachy for any modifications.
     * @returns True if the hierarchy is modified otherwise false.
     */
    bool IsModified();

    void ClearModifyStatus();

    /**
     * Function AnnotatePowerSymbols
     * clear and annotate the entire hierarchy of the sheet path list.
     */
    void AnnotatePowerSymbols();

    /**
     * Function GetComponents
     * adds a SCH_REFERENCE() object to \a aReferences for each component in the list
     * of sheets.
     * @param aReferences List of references to populate.
     * @param aIncludePowerSymbols Set to false to only get normal components.
     */
    void GetComponents( SCH_REFERENCE_LIST& aReferences,
                        bool                aIncludePowerSymbols = true  );

    /**
     * Function FindNextItem
     * searches the entire schematic for the next schematic object.
     *
     * @param aType The type of schematic item to find.
     * @param aSheetFound The sheet the item was found in.  NULL if the next item
     *                    is not found.
     * @param aLastItem Find next item after aLastItem if not NULL.
     * @param aWrap Wrap past around the end of the list of sheets.
     * @return If found, Returns the next schematic item.  Otherwise, returns NULL.
     */
    SCH_ITEM* FindNextItem( KICAD_T aType, SCH_SHEET_PATH** aSheetFound = NULL,
                            SCH_ITEM* aLastItem = NULL, bool aWrap = true );

    /**
     * Function FindPreviousItem
     * searches the entire schematic for the previous schematic item.
     *
     * @param aType The type of schematic item to find.
     * @param aSheetFound The sheet the item was found in.  NULL if the previous item
     *                    is not found.
     * @param aLastItem Find the previous item before aLastItem if not NULL.
     * @param aWrap Wrap past around the beginning of the list of sheets.
     * @return If found, the previous schematic item.  Otherwise, NULL.
     */
    SCH_ITEM* FindPreviousItem( KICAD_T aType, SCH_SHEET_PATH** aSheetFound = NULL,
                                SCH_ITEM* aLastItem = NULL, bool aWrap = true );

    /**
     * Function MatchNextItem
     * searches the entire schematic for the next item that matches the search criteria.
     *
     * @param aSearchData Criteria to search item against.
     * @param aSheetFound The sheet the item was found in.  NULL if the next item
     *                    is not found.
     * @param aLastItem Find next item after aLastItem if not NULL.
     * @param aFindLocation a wxPoint where to put the location of matched item. can be NULL.
     * @return If found, Returns the next schematic item.  Otherwise, returns NULL.
     */
    SCH_ITEM* MatchNextItem( wxFindReplaceData& aSearchData,
                             SCH_SHEET_PATH**   aSheetFound,
                             SCH_ITEM*          aLastItem,
                             wxPoint*           aFindLocation );

private:

    /**
     * Function BuildSheetList
     * builds the list of sheets and their sheet path from \a aSheet.  If \a aSheet is
     * the root sheet, the full sheet path and sheet list is built.
     *
     * @param aSheet The starting sheet from which the list is built, or NULL indicating
     *               that the root sheet should be used.
     */
    void           BuildSheetList( SCH_SHEET* aSheet );
};

#endif // CLASS_DRAWSHEET_PATH_H
