/********************************************/
/* Definitions for the EESchema program:	*/
/********************************************/

#ifndef CLASS_DRAWSHEET_PATH_H
#define CLASS_DRAWSHEET_PATH_H

#ifndef eda_global
#define eda_global extern
#endif

#include "base_struct.h"


/**********************************************/
/* class to handle a series of sheets *********/
/* a 'path' so to speak.. *********************/
/**********************************************/
class DrawSheetPath
{
public:
    int m_numSheets;

#define DSLSZ 32          // Max number of levels for a sheet path
    DrawSheetStruct* m_sheets[DSLSZ];

    DrawSheetPath();
    ~DrawSheetPath() { };
    void                Clear() { m_numSheets = 0; }
    int              Cmp( const DrawSheetPath& d ) const;
    DrawSheetStruct* Last();
    SCH_SCREEN*      LastScreen();
    EDA_BaseStruct*  LastDrawList();
    void             Push( DrawSheetStruct* sheet );
    DrawSheetStruct* Pop();

    /** Function Path
     * the path uses the time stamps which do not changes even when editing sheet parameters
     * a path is something like / (root) or /34005677 or /34005677/00AE4523
     */
    wxString         Path();

    /** Function PathHumanReadable
     * Return the sheet path in a readable form, i.e.
     * as a path made from sheet names.
     * (the "normal" path uses the time stamps which do not changes even when editing sheet parameters)
     */
    wxString         PathHumanReadable();

    /**
     * Function UpdateAllScreenReferences
     * updates the reference and the m_Multi parameter (part selection) for all
     * components on a screen depending on the actual sheet path.
     * Mandatory in complex hierarchies because sheets use the same screen (basic schematic)
     * but with different references and part selection according to the displayed sheet
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
                                 * starting at the given sheet in constructor . the given sheet is counted
                                 */
    int            m_index;
    DrawSheetPath  m_currList;

public:
    EDA_SheetList( DrawSheetStruct* sheet )
    {
        m_index = 0;
        m_count = 0;
        m_List  = NULL;
        if( sheet == NULL )
            sheet = g_RootSheet;
        BuildSheetList( sheet );
    }


    ~EDA_SheetList()
    {
        if( m_List )
        {
            free( m_List );
        }
        m_List = NULL;
    }


    int GetCount() { return m_count; }
    DrawSheetPath* GetFirst();
    DrawSheetPath* GetNext();
    DrawSheetPath* GetSheet( int index );

private:
    void           BuildSheetList( DrawSheetStruct* sheet );
};

#endif /* CLASS_DRAWSHEET_PATH_H */
