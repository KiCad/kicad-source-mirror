/////////////////////////////////////////////////////////////////////////////

// Name:        class_drawsheet.cpp
// Purpose:     member functions for SCH_SHEET
//              header = class_drawsheet.h
// Author:      jean-pierre Charras
// Modified by:
// License:     License GNU
/////////////////////////////////////////////////////////////////////////////

#include "fctsys.h"

#include "common.h"
#include "program.h"
#include "general.h"


/**********************************************/
/* class to handle a series of sheets *********/
/* a 'path' so to speak.. *********************/
/**********************************************/
SCH_SHEET_PATH::SCH_SHEET_PATH()
{
    for( int i = 0; i<DSLSZ; i++ )
        m_sheets[i] = NULL;

    m_numSheets = 0;
}


/** Function BuildSheetPathInfoFromSheetPathValue
 * Fill this with data to access to the hierarchical sheet known by its path
 * aPath
 * @param aPath = path of the sheet to reach (in non human readable format)
 * @return true if success else false
 */
bool SCH_SHEET_PATH::BuildSheetPathInfoFromSheetPathValue(
    const wxString& aPath,
    bool            aFound )
{
    if( aFound )
        return true;

    if(  GetSheetsCount() == 0 )
        Push( g_RootSheet );

    if( aPath == Path() )
        return true;

    SCH_ITEM* schitem = LastDrawList();
    while( schitem && GetSheetsCount() < NB_MAX_SHEET )
    {
        if( schitem->Type() == DRAW_SHEET_STRUCT_TYPE )
        {
            SCH_SHEET* sheet = (SCH_SHEET*) schitem;
            Push( sheet );
            if( aPath == Path() )
                return true;
            if( BuildSheetPathInfoFromSheetPathValue( aPath ) )
                return true;
            Pop();
        }
        schitem = schitem->Next();
    }

    return false;
}


/** Function Cmp
 * Compare if this is the same sheet path as aSheetPathToTest
 * @param aSheetPathToTest = sheet path to compare
 * @return -1 if different, 0 if same
 */
int SCH_SHEET_PATH::Cmp( const SCH_SHEET_PATH& aSheetPathToTest ) const
{
    if( m_numSheets > aSheetPathToTest.m_numSheets )
        return 1;
    if( m_numSheets < aSheetPathToTest.m_numSheets )
        return -1;

    //otherwise, same number of sheets.
    for( unsigned i = 0; i<m_numSheets; i++ )
    {
        if( m_sheets[i]->m_TimeStamp >
            aSheetPathToTest.m_sheets[i]->m_TimeStamp )
            return 1;
        if( m_sheets[i]->m_TimeStamp <
            aSheetPathToTest.m_sheets[i]->m_TimeStamp )
            return -1;
    }

    return 0;
}


/** Function Last
 * returns a pointer to the last sheet of the list
 * One can see the others sheet as the "path" to reach this last sheet
 */
SCH_SHEET* SCH_SHEET_PATH::Last()
{
    if( m_numSheets )
        return m_sheets[m_numSheets - 1];
    return NULL;
}


/** Function LastScreen
 * @return the SCH_SCREEN relative to the last sheet in list
 */
SCH_SCREEN* SCH_SHEET_PATH::LastScreen()
{
    if( m_numSheets )
        return m_sheets[m_numSheets - 1]->m_AssociatedScreen;
    return NULL;
}


/** Function LastScreen
 * @return a pointer to the first schematic item handled by the
 * SCH_SCREEN relative to the last sheet in list
 */
SCH_ITEM* SCH_SHEET_PATH::LastDrawList()
{
    if( m_numSheets && m_sheets[m_numSheets - 1]->m_AssociatedScreen )
        return m_sheets[m_numSheets - 1]->m_AssociatedScreen->EEDrawList;
    return NULL;
}


/** Function Push
 * store (push) aSheet in list
 * @param aSheet = pointer to the SCH_SHEET to store in list
 */
void SCH_SHEET_PATH::Push( SCH_SHEET* aSheet )
{
    if( m_numSheets > DSLSZ )
        wxMessageBox( wxT( "SCH_SHEET_PATH::Push() error: no room in buffer \
to store sheet" ) );

    if( m_numSheets < DSLSZ )
    {
        m_sheets[m_numSheets] = aSheet;
        m_numSheets++;
    }
}


/** Function Pop
 * retrieves (pop) the last entered sheet and remove it from list
 * @return a SCH_SHEET* pointer to the removed sheet in list
 */
SCH_SHEET* SCH_SHEET_PATH::Pop()
{
    if( m_numSheets > 0 )
    {
        m_numSheets--;
        return m_sheets[m_numSheets];
    }
    return NULL;
}


/** Function Path
 * the path uses the time stamps which do not changes even when editing sheet
 * parameters
 * a path is something like / (root) or /34005677 or /34005677/00AE4523
 */
wxString SCH_SHEET_PATH::Path()
{
    wxString s, t;

    s = wxT( "/" );     // This is the root path

    // start at 1 to avoid the root sheet,
    // which does not need to be added to the path
    // it's timestamp changes anyway.
    for( unsigned i = 1; i < m_numSheets; i++ )
    {
        t.Printf( _( "%8.8lX/" ), m_sheets[i]->m_TimeStamp );
        s = s + t;
    }

    return s;
}


/** Function PathHumanReadable
 * Return the sheet path in a readable form, i.e.
 * as a path made from sheet names.
 * (the "normal" path uses the time stamps which do not changes even when
 * editing sheet parameters)
 */
wxString SCH_SHEET_PATH::PathHumanReadable()
{
    wxString s, t;

    s = wxT( "/" );

    // start at 1 to avoid the root sheet, as above.
    for( unsigned i = 1; i< m_numSheets; i++ )
    {
        s = s + m_sheets[i]->m_SheetName + wxT( "/" );
    }

    return s;
}


void SCH_SHEET_PATH::UpdateAllScreenReferences()
{
    EDA_BaseStruct* t = LastDrawList();

    while( t )
    {
        if( t->Type() == TYPE_SCH_COMPONENT )
        {
            SCH_COMPONENT* component = (SCH_COMPONENT*) t;
            component->GetField( REFERENCE )->m_Text = component->GetRef( this );
            component->m_Multi = component->GetUnitSelection( this );
        }
        t = t->Next();
    }
}


bool SCH_SHEET_PATH::operator=( const SCH_SHEET_PATH& d1 )
{
    m_numSheets = d1.m_numSheets;
    unsigned i;
    for( i = 0; i < m_numSheets; i++ )
    {
        m_sheets[i] = d1.m_sheets[i];
    }

    for( ; i < DSLSZ; i++ )
    {
        m_sheets[i] = 0;
    }

    return true;
}


bool SCH_SHEET_PATH::operator==( const SCH_SHEET_PATH& d1 )
{
    if( m_numSheets != d1.m_numSheets )
        return false;
    for( unsigned i = 0; i < m_numSheets; i++ )
    {
        if( m_sheets[i] != d1.m_sheets[i] )
            return false;
    }

    return true;
}


bool SCH_SHEET_PATH::operator!=( const SCH_SHEET_PATH& d1 )
{
    if( m_numSheets != d1.m_numSheets )
        return true;
    for( unsigned i = 0; i < m_numSheets; i++ )
    {
        if( m_sheets[i] != d1.m_sheets[i] )
            return true;
    }

    return false;
}


/*********************************************************************/
/* Class SCH_SHEET_LIST to handle the list of Sheets in a hierarchy */
/*********************************************************************/


/* The constructor: build the list of sheets from aSheet.
 * If aSheet == NULL (default) build the whole list of sheets in hierarchy
 * So usually call it with no param.
 */
SCH_SHEET_LIST::SCH_SHEET_LIST( SCH_SHEET* aSheet )
{
    m_index = 0;
    m_count = 0;
    m_List  = NULL;
    if( aSheet == NULL )
        aSheet = g_RootSheet;
    BuildSheetList( aSheet );
}


/** Function GetFirst
 *  @return the first item (sheet) in m_List and prepare calls to GetNext()
 */
SCH_SHEET_PATH* SCH_SHEET_LIST::GetFirst()
{
    m_index = 0;
    if( GetCount() > 0 )
        return &( m_List[0] );
    return NULL;
}


/** Function GetNext
 *  @return the next item (sheet) in m_List or NULL if no more item in sheet
 * list
 */
SCH_SHEET_PATH* SCH_SHEET_LIST::GetNext()
{
    if( m_index < GetCount() )
        m_index++;
    return GetSheet( m_index );
}


/** Function GetSheet
 *  @return the item (sheet) in aIndex position in m_List or NULL if less than
 * index items
 * @param aIndex = index in sheet list to get the sheet
 */
SCH_SHEET_PATH* SCH_SHEET_LIST::GetSheet( int aIndex )
{
    if( aIndex < GetCount() )
        return &( m_List[aIndex] );
    return NULL;
}


/** Function BuildSheetList
 * Build the list of sheets and their sheet path from the aSheet sheet
 * if aSheet = g_RootSheet, the full sheet path list (and full sheet list) is
 * built
 * @param aSheet = the starting sheet to  build list
 */
void SCH_SHEET_LIST::BuildSheetList( SCH_SHEET* aSheet )
{
    if( m_List == NULL )
    {
        int count = aSheet->CountSheets();
        m_count = count;
        m_index = 0;
        count  *= sizeof(SCH_SHEET_PATH);

        /* @bug - MyZMalloc() can return a NULL pointer if there is not enough
         *        memory.  This code continues on it's merry way with out
         *        checking to see if the memory was actually allocated.
         */
        m_List  = (SCH_SHEET_PATH*) MyZMalloc( count );
        m_currList.Clear();
    }

    m_currList.Push( aSheet );
    m_List[m_index] = m_currList;
    m_index++;

    if( aSheet->m_AssociatedScreen != NULL )
    {
        EDA_BaseStruct* strct = m_currList.LastDrawList();
        while( strct )
        {
            if( strct->Type() == DRAW_SHEET_STRUCT_TYPE )
            {
                SCH_SHEET* sheet = (SCH_SHEET*) strct;
                BuildSheetList( sheet );
            }

            strct = strct->Next();
        }
    }

    m_currList.Pop();
}
