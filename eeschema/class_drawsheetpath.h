/********************************************/
/* Definitions for the EESchema program:    */
/********************************************/

#ifndef CLASS_DRAWSHEET_PATH_H
#define CLASS_DRAWSHEET_PATH_H

#include "base_struct.h"

/** Info about complex hierarchies handling:
 * A hierarchical schematic uses sheets (hierarchical sheets) included in a
 * given sheet.  Rach sheet corresponds to a schematic drawing handled by a
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
 * The class DrawSheetPath handles paths used to access a sheet.  The class
 * EDA_SheetList allows to handle the full (or partial) list of sheets and
 * their paths in a complex hierarchy.  The class EDA_ScreenList allow to
 * handle the list of SCH_SCREEN. It is useful to clear or save data,
 * but is not suitable to handle the full complex hierarchy possibilities
 * (usable in flat and simple hierarchies).
 */


/****************************************/
/* class to handle and access to a sheet */
/* a 'path' so to speak..               */
/****************************************/

/*
 * The member m_sheets stores the list of sheets from the first (usually
 * g_RootSheet)
 * to a given sheet in last position.
 * The last sheet is usually the sheet we want to select or reach. So Last()
 * return this last sheet
 * Others sheets are the "path" from the first to the last sheet
 */
class DrawSheetPath
{
private:
    unsigned m_numSheets;

public:
#define DSLSZ 32          // Max number of levels for a sheet path
    SCH_SHEET * m_sheets[DSLSZ];

public: DrawSheetPath();
    ~DrawSheetPath() { };
    void                Clear()
    {
        m_numSheets = 0;
    }


    unsigned GetSheetsCount()
    {
        return m_numSheets;
    }


    /** Function Cmp
     * Compare if this is the same sheet path as aSheetPathToTest
     * @param aSheetPathToTest = sheet path to compare
     * @return -1 if different, 0 if same
     */
    int              Cmp( const DrawSheetPath& aSheetPathToTest ) const;

    /** Function Last
     * returns a pointer to the last sheet of the list
     * One can see the others sheet as the "path" to reach this last sheet
     */
    SCH_SHEET* Last();

    /** Function LastScreen
     * @return the SCH_SCREEN relative to the last sheet in list
     */
    SCH_SCREEN*      LastScreen();

    /** Function LastScreen
     * @return a pointer to the first schematic item handled by the
     * SCH_SCREEN relative to the last sheet in list
     */
    SCH_ITEM*        LastDrawList();

    /** Function Push
     * store (push) aSheet in list
     * @param aSheet = pointer to the SCH_SHEET to store in list
     * Push is used when entered a sheet to select or analyze it
     * This is like cd <directory> in directories navigation
     */
    void             Push( SCH_SHEET* aSheet );

    /** Function Pop
     * retrieves (pop) the last entered sheet and remove it from list
     * @return a SCH_SHEET* pointer to the removed sheet in list
     * Pop is used when leaving a sheet after a selection or analyze
     * This is like cd .. in directories navigation
     */
    SCH_SHEET* Pop();

    /** Function Path
     * the path uses the time stamps which do not changes even when editing
     * sheet parameters
     * a path is something like / (root) or /34005677 or /34005677/00AE4523
     */
    wxString         Path();

    /** Function PathHumanReadable
     * Return the sheet path in a readable form, i.e.
     * as a path made from sheet names.
     * (the "normal" path uses the time stamps which do not changes even when
     * editing sheet parameters)
     */
    wxString         PathHumanReadable();

    /** Function BuildSheetPathInfoFromSheetPathValue
     * Fill this with data to access to the hierarchical sheet known by its
     * path aPath
     * @param aPath = path of the sheet to reach (in non human readable format)
     * @return true if success else false
     */
    bool             BuildSheetPathInfoFromSheetPathValue(
        const wxString& aPath,
        bool            aFound = false );

    /**
     * Function UpdateAllScreenReferences
     * updates the reference and the m_Multi parameter (part selection) for all
     * components on a screen depending on the actual sheet path.
     * Mandatory in complex hierarchies because sheets use the same screen
     * (basic schematic)
     * but with different references and part selections according to the
     * displayed sheet
     */
    void             UpdateAllScreenReferences();

    bool operator    =( const DrawSheetPath& d1 );

    bool operator    ==( const DrawSheetPath& d1 );

    bool operator    !=( const DrawSheetPath& d1 );
};


/*******************************************************/
/* Class to handle the list of *Sheets* in a hierarchy */
/*******************************************************/

/* sheets are not unique - can have many sheets with the same
 * filename and the same SCH_SCREEN reference.
 * the schematic (SCH_SCREEN) is shared between these sheets,
 * and component references are specific to a sheet path.
 * When a sheet is entered, component references and sheet number are updated
 */
class EDA_SheetList
{
private:
    DrawSheetPath* m_List;
    int            m_count;     /* Number of sheets included in hierarchy,
                                 * starting at the given sheet in constructor .
                                 * the given sheet is counted
                                 */
    int            m_index;     /* internal variable to handle GetNext():
                                 * cleared by GetFirst()
                                 *  and incremented by GetNext() after
                                 * returning the next item in m_List
                                 * Also used for internal calculations in
                                 * BuildSheetList()
                                 */
    DrawSheetPath m_currList;

public:
    /* The constructor: build the list of sheets from aSheet.
     * If aSheet == NULL (default) build the whole list of sheets in hierarchy
     * So usually call it with no param.
     */
    EDA_SheetList( SCH_SHEET* aSheet = NULL );

    ~EDA_SheetList()
    {
        if( m_List )
            free( m_List );
        m_List = NULL;
    }


    /** Function GetCount()
     * @return the number of sheets in list:
     * usually the number of sheets found in the whole hierarchy
     */
    int GetCount() { return m_count; }

    /** Function GetFirst
     *  @return the first item (sheet) in m_List and prepare calls to GetNext()
     */
    DrawSheetPath* GetFirst();

    /** Function GetNext
     *  @return the next item (sheet) in m_List or NULL if no more item in
     * sheet list
     */
    DrawSheetPath* GetNext();

    /** Function GetSheet
     *  @return the item (sheet) in aIndex position in m_List or NULL if less
     * than index items
     * @param aIndex = index in sheet list to get the sheet
     */
    DrawSheetPath* GetSheet( int aIndex );

private:

    /** Function BuildSheetList
     * Build the list of sheets and their sheet path from the aSheet sheet
     * if aSheet = g_RootSheet, the full sheet path and sheet list is built
     * @param aSheet = the starting sheet from the built is made
     */
    void           BuildSheetList( SCH_SHEET* sheet );
};

#endif /* CLASS_DRAWSHEET_PATH_H */
