/////////////////////////////////////////////////////////////////////////////
// Name:        class_drawsheet.cpp
// Purpose:		member functions for DrawSheetStruct
//				header = class_drawsheet.h
// Author:      jean-pierre Charras
// Modified by:
// Licence:     License GNU
/////////////////////////////////////////////////////////////////////////////

#include "fctsys.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"


/**********************************************/
/* class to handle a serie of sheets *********/
/* a 'path' so to speak.. *********************/
/**********************************************/
DrawSheetPath::DrawSheetPath()
{
    for( int i = 0; i<DSLSZ; i++ )
        m_sheets[i] = NULL;

    m_numSheets = 0;
}

/*********************************************************************************************/
bool DrawSheetPath::BuildSheetPathInfoFromSheetPathValue(const wxString & aPath, bool aFound )
/*********************************************************************************************/
/** Function BuildSheetPathInfoFromSheetPathValue
 * Fill this with data to acces to the hierarchical sheet known by its path aPath
 * @param aPath = path of the sheet to reach (in non human readable format)
 * @return true if success else false
 */
{
    if ( aFound )
        return true;

    if (  GetSheetsCount() == 0 )
        Push( g_RootSheet );

    if  ( aPath == Path() )
        return true;

    SCH_ITEM* schitem = LastDrawList();
    while( schitem && GetSheetsCount() < NB_MAX_SHEET )
    {
        if( schitem->Type() == DRAW_SHEET_STRUCT_TYPE )
        {
            DrawSheetStruct* sheet = (DrawSheetStruct*) schitem;
            Push( sheet );
            if  ( aPath == Path() )
                return true;
            if ( BuildSheetPathInfoFromSheetPathValue( aPath ) )
                return true;
            Pop();
        }
        schitem = schitem->Next();
    }
    return false;
}

/*******************************************************************/
int DrawSheetPath::Cmp( const DrawSheetPath& aSheetPathToTest ) const
/********************************************************************/

/** Function Cmp
 * Compare if this is the same sheet path as aSheetPathToTest
 * @param aSheetPathToTest = sheet path to compare
 * @return -1 if differents, 0 if same
 */
{
    if( m_numSheets > aSheetPathToTest.m_numSheets )
        return 1;
    if( m_numSheets < aSheetPathToTest.m_numSheets )
        return -1;

    //otherwise, same number of sheets.
    for( unsigned i = 0; i<m_numSheets; i++ )
    {
        if( m_sheets[i]->m_TimeStamp > aSheetPathToTest.m_sheets[i]->m_TimeStamp )
            return 1;
        if( m_sheets[i]->m_TimeStamp < aSheetPathToTest.m_sheets[i]->m_TimeStamp )
            return -1;
    }

    return 0;
}


/** Function Last
  * returns a pointer to the last sheet of the list
  * One can see the others sheet as the "path" to reach this last sheet
 */
DrawSheetStruct* DrawSheetPath::Last()
{
    if( m_numSheets )
        return m_sheets[m_numSheets - 1];
    return NULL;
}


/** Function LastScreen
 * @return the SCH_SCREEN relative to the last sheet in list
 */
SCH_SCREEN* DrawSheetPath::LastScreen()
{
    if( m_numSheets )
        return m_sheets[m_numSheets - 1]->m_AssociatedScreen;
    return NULL;
}


/** Function LastScreen
 * @return a pointer to the first schematic item handled by the
 * SCH_SCREEN relative to the last sheet in list
 */
SCH_ITEM* DrawSheetPath::LastDrawList()
{
    if( m_numSheets && m_sheets[m_numSheets - 1]->m_AssociatedScreen )
        return m_sheets[m_numSheets - 1]->m_AssociatedScreen->EEDrawList;
    return NULL;
}


/**************************************************/
void DrawSheetPath::Push( DrawSheetStruct* aSheet )
/**************************************************/

/** Function Push
 * store (push) aSheet in list
 * @param aSheet = pointer to the DrawSheetStruct to store in list
 */
{
    if( m_numSheets > DSLSZ )
        wxMessageBox( wxT( "DrawSheetPath::Push() error: no room in buffer to store sheet" ) );
    if( m_numSheets < DSLSZ )
    {
        m_sheets[m_numSheets] = aSheet;
        m_numSheets++;
    }
}


DrawSheetStruct* DrawSheetPath::Pop()

/** Function Pop
 * retrieves (pop) the last entered sheet and remove it from list
 * @return a DrawSheetStruct* pointer to the removed sheet in list
 */
{
    if( m_numSheets > 0 )
    {
        m_numSheets--;
        return m_sheets[m_numSheets];
    }
    return NULL;
}


wxString DrawSheetPath::Path()

/** Function Path
 * the path uses the time stamps which do not changes even when editing sheet parameters
 * a path is something like / (root) or /34005677 or /34005677/00AE4523
 */
{
    wxString s, t;

    s = wxT( "/" );     // This is the root path

    //start at 1 to avoid the root sheet,
    //which does not need to be added to the path
    //it's timestamp changes anyway.
    for( unsigned i = 1; i< m_numSheets; i++ )
    {
        t.Printf( _( "%8.8lX/" ), m_sheets[i]->m_TimeStamp );
        s = s + t;
    }

    return s;
}


/******************************************/
wxString DrawSheetPath::PathHumanReadable()
/******************************************/

/** Function PathHumanReadable
 * Return the sheet path in a readable form, i.e.
 * as a path made from sheet names.
 * (the "normal" path uses the time stamps which do not changes even when editing sheet parameters)
 */
{
    wxString s, t;

    s = wxT( "/" );

    //start at 1 to avoid the root sheet, as above.
    for( unsigned i = 1; i< m_numSheets; i++ )
    {
        s = s + m_sheets[i]->m_SheetName + wxT( "/" );
    }

    return s;
}


/***********************************************/
void DrawSheetPath::UpdateAllScreenReferences()
/***********************************************/
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


bool DrawSheetPath::operator=( const DrawSheetPath& d1 )
{
    m_numSheets = d1.m_numSheets;
    unsigned i;
    for( i = 0; i<m_numSheets; i++ )
    {
        m_sheets[i] = d1.m_sheets[i];
    }

    for( ; i<DSLSZ; i++ )
    {
        m_sheets[i] = 0;
    }

    return true;
}


bool DrawSheetPath::operator==( const DrawSheetPath& d1 )
{
    if( m_numSheets != d1.m_numSheets )
        return false;
    for( unsigned i = 0; i<m_numSheets; i++ )
    {
        if( m_sheets[i] != d1.m_sheets[i] )
            return false;
    }

    return true;
}


bool DrawSheetPath::operator!=( const DrawSheetPath& d1 )
{
    if( m_numSheets != d1.m_numSheets )
        return true;
    for( unsigned i = 0; i<m_numSheets; i++ )
    {
        if( m_sheets[i] != d1.m_sheets[i] )
            return true;
    }

    return false;
}


/*********************************************************************/
/* Class EDA_SheetList to handle the list of Sheets in a hierarchy */
/*********************************************************************/


/*******************************************************/
EDA_SheetList::EDA_SheetList( DrawSheetStruct* aSheet )
/*******************************************************/

/* The constructor: build the list of sheets from aSheet.
 * If aSheet == NULL (default) build the whole list of sheets in hierarchy
 * So usually call it with no param.
 */
{
    m_index = 0;
    m_count = 0;
    m_List  = NULL;
    if( aSheet == NULL )
        aSheet = g_RootSheet;
    BuildSheetList( aSheet );
}


/*****************************************/
DrawSheetPath* EDA_SheetList::GetFirst()
/*****************************************/

/** Function GetFirst
 *  @return the first item (sheet) in m_List and prepare calls to GetNext()
 */
{
    m_index = 0;
    if( GetCount() > 0 )
        return &( m_List[0] );
    return NULL;
}


/*****************************************/
DrawSheetPath* EDA_SheetList::GetNext()
/*****************************************/

/** Function GetNext
 *  @return the next item (sheet) in m_List or NULL if no more item in sheet list
 */
{
    if( m_index < GetCount() )
        m_index++;
    return GetSheet( m_index );
}


/************************************************/
DrawSheetPath* EDA_SheetList::GetSheet( int aIndex )
/************************************************/

/** Function GetSheet
 *  @return the item (sheet) in aIndex position in m_List or NULL if less than index items
 * @param aIndex = index in sheet list to get the sheet
 */
{
    if( aIndex < GetCount() )
        return &(m_List[aIndex]);
    return NULL;
}


/************************************************************************/
void EDA_SheetList::BuildSheetList( DrawSheetStruct* aSheet )
/************************************************************************/

/** Function BuildSheetList
 * Build the list of sheets and their sheet path from the aSheet sheet
 * if aSheet = g_RootSheet, the full sheet path list (and full sheet list) is built
 * @param aSheet = the starting sheet to  build list
 */
{
    if( m_List == NULL )
    {
        int count = aSheet->CountSheets();
        m_count = count;
        m_index = 0;
        count  *= sizeof(DrawSheetPath);
        m_List  = (DrawSheetPath*) MyZMalloc( count );
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
                DrawSheetStruct* sheet = (DrawSheetStruct*) strct;
                BuildSheetList( sheet );
            }
            strct = strct->Next();
        }
    }
    m_currList.Pop();
}
