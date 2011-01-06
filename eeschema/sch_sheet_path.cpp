/////////////////////////////////////////////////////////////////////////////
// Name:        sch_sheet_path.cpp
// Purpose:     member functions for SCH_SHEET_PATH
//              header = sch_sheet_path.h
// Author:      jean-pierre Charras
// Modified by:
// License:     License GNU
/////////////////////////////////////////////////////////////////////////////

#include "fctsys.h"

#include "common.h"
#include "general.h"
#include "dlist.h"
#include "class_sch_screen.h"
#include "sch_item_struct.h"

#include "netlist.h"
#include "class_library.h"
#include "sch_sheet.h"
#include "sch_sheet_path.h"
#include "sch_component.h"
#include "template_fieldnames.h"

#include "dialogs/dialog_schematic_find.h"


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


/**
 * Function BuildSheetPathInfoFromSheetPathValue
 * Fill this with data to access to the hierarchical sheet known by its path
 * aPath
 * @param aPath = path of the sheet to reach (in non human readable format)
 * @return true if success else false
 */
bool SCH_SHEET_PATH::BuildSheetPathInfoFromSheetPathValue( const wxString& aPath, bool aFound )
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
        if( schitem->Type() == SCH_SHEET_T )
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


/**
 * Function Cmp
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
        if( m_sheets[i]->m_TimeStamp > aSheetPathToTest.m_sheets[i]->m_TimeStamp )
            return 1;

        if( m_sheets[i]->m_TimeStamp < aSheetPathToTest.m_sheets[i]->m_TimeStamp )
            return -1;
    }

    return 0;
}


/**
 * Function Last
 * returns a pointer to the last sheet of the list
 * One can see the others sheet as the "path" to reach this last sheet
 */
SCH_SHEET* SCH_SHEET_PATH::Last()
{
    if( m_numSheets )
        return m_sheets[m_numSheets - 1];

    return NULL;
}


/**
 * Function LastScreen
 * @return the SCH_SCREEN relative to the last sheet in list
 */
SCH_SCREEN* SCH_SHEET_PATH::LastScreen()
{
    SCH_SHEET* lastSheet = Last();

    if( lastSheet )
        return lastSheet->m_AssociatedScreen;

    return NULL;
}


/**
 * Function LastScreen
 * @return a pointer to the first schematic item handled by the
 * SCH_SCREEN relative to the last sheet in list
 */
SCH_ITEM* SCH_SHEET_PATH::LastDrawList()
{
    SCH_SHEET* lastSheet = Last();

    if( lastSheet && lastSheet->m_AssociatedScreen )
        return lastSheet->m_AssociatedScreen->GetDrawItems();

    return NULL;
}


SCH_ITEM* SCH_SHEET_PATH::FirstDrawList()
{
    SCH_ITEM* item = NULL;

    if( m_numSheets && m_sheets[0]->m_AssociatedScreen )
        item = m_sheets[0]->m_AssociatedScreen->GetDrawItems();

    /* @fixme - These lists really should be one of the boost pointer containers.  This
     *          is a brain dead hack to allow reverse iteration of EDA_ITEM linked
     *          list.
     */
    SCH_ITEM* lastItem = NULL;

    while( item != NULL )
    {
        lastItem = item;
        item = item->Next();
    }

    return lastItem;
}


void SCH_SHEET_PATH::Push( SCH_SHEET* aSheet )
{
    if( m_numSheets > DSLSZ )
    {
        wxString msg;
        msg.Printf( _( "Schematic sheets can only be nested %d levels deep." ), DSLSZ );
        wxMessageBox( msg );
    }

    if( m_numSheets < DSLSZ )
    {
        m_sheets[m_numSheets] = aSheet;
        m_numSheets++;
    }
}


/**
 * Function Pop
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


/**
 * Function Path
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


/**
 * Function PathHumanReadable
 * Return the sheet path in a readable form, i.e.
 * as a path made from sheet names.
 * (the "normal" path uses the time stamps which do not changes even when
 * editing sheet parameters)
 */
wxString SCH_SHEET_PATH::PathHumanReadable() const
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
    EDA_ITEM* t = LastDrawList();

    while( t )
    {
        if( t->Type() == SCH_COMPONENT_T )
        {
            SCH_COMPONENT* component = (SCH_COMPONENT*) t;
            component->GetField( REFERENCE )->m_Text = component->GetRef( this );
            component->SetUnit( component->GetUnitSelection( this ) );
        }

        t = t->Next();
    }
}


void SCH_SHEET_PATH::AnnotatePowerSymbols( int* aReference )
{
    int ref = 1;

    if( aReference != NULL )
        ref = *aReference;

    for( EDA_ITEM* item = LastDrawList(); item != NULL; item = item->Next() )
    {
        if( item->Type() != SCH_COMPONENT_T )
                continue;

        SCH_COMPONENT* component = (SCH_COMPONENT*) item;
        LIB_COMPONENT* entry = CMP_LIBRARY::FindLibraryComponent( component->GetLibName() );

        if( ( entry == NULL ) || !entry->IsPower() )
            continue;

        wxString refstr = component->GetPrefix();

        //str will be "C?" or so after the ClearAnnotation call.
        while( refstr.Last() == '?' )
            refstr.RemoveLast();

        if( !refstr.StartsWith( wxT( "#" ) ) )
            refstr = wxT( "#" ) + refstr;

        refstr << wxT( "0" ) << ref;
        component->SetRef( this, refstr );
        ref++;
    }

    if( aReference != NULL )
        *aReference = ref;
}


void SCH_SHEET_PATH::GetComponents( SCH_REFERENCE_LIST& aReferences,
                                    bool                aIncludePowerSymbols )
{
    // Search to sheet path number:
    int sheetnumber = 1;    // 1 = root
    SCH_SHEET_LIST sheetList;
    for( SCH_SHEET_PATH* path = sheetList.GetFirst(); path != NULL;
         path = sheetList.GetNext(), sheetnumber++ )
        if( Cmp(*path) == 0 )
            break;

    for( SCH_ITEM* item = LastDrawList(); item != NULL; item = item->Next() )
    {
        if( item->Type() == SCH_COMPONENT_T )
        {
            SCH_COMPONENT* component = (SCH_COMPONENT*) item;

            // Skip pseudo components, which have a reference starting with #.  This mainly
            // effects power symbols.
            if( !aIncludePowerSymbols && component->GetRef( this )[0] == wxT( '#' ) )
                continue;

            LIB_COMPONENT* entry = CMP_LIBRARY::FindLibraryComponent( component->GetLibName() );

            if( entry == NULL )
                continue;

            SCH_REFERENCE reference = SCH_REFERENCE( component, entry, *this );
            reference.SetSheetNumber( sheetnumber );
            aReferences.AddItem( reference );
        }
    }
}


SCH_ITEM* SCH_SHEET_PATH::FindNextItem( KICAD_T aType, SCH_ITEM* aLastItem, bool aWrap )
{
    bool hasWrapped = false;
    bool firstItemFound = false;
    SCH_ITEM* drawItem = LastDrawList();

    while( drawItem != NULL )
    {
        if( drawItem->Type() == aType )
        {
            if( aLastItem == NULL || firstItemFound )
            {
                return drawItem;
            }
            else if( !firstItemFound && drawItem == aLastItem )
            {
                firstItemFound = true;
            }
        }

        drawItem = drawItem->Next();

        if( drawItem == NULL && aLastItem && aWrap && !hasWrapped )
        {
            hasWrapped = true;
            drawItem = LastDrawList();
        }
    }

    return NULL;
}


SCH_ITEM* SCH_SHEET_PATH::FindPreviousItem( KICAD_T aType, SCH_ITEM* aLastItem, bool aWrap )
{
    bool hasWrapped = false;
    bool firstItemFound = false;
    SCH_ITEM* drawItem = FirstDrawList();

    while( drawItem != NULL )
    {
        if( drawItem->Type() == aType )
        {
            if( aLastItem == NULL || firstItemFound )
            {
                return drawItem;
            }
            else if( !firstItemFound && drawItem == aLastItem )
            {
                firstItemFound = true;
            }
        }

        drawItem = drawItem->Back();

        if( drawItem == NULL && aLastItem && aWrap && !hasWrapped )
        {
            hasWrapped = true;
            drawItem = FirstDrawList();
        }
    }

    return NULL;
}


SCH_ITEM* SCH_SHEET_PATH::MatchNextItem( wxFindReplaceData& aSearchData,
                                         SCH_ITEM*          aLastItem,
                                         wxPoint * aFindLocation )
{
    bool hasWrapped = false;
    bool firstItemFound = false;
    bool wrap = ( aSearchData.GetFlags() & FR_SEARCH_WRAP ) != 0;
    SCH_ITEM* drawItem = LastDrawList();

    while( drawItem != NULL )
    {
        if( aLastItem && !firstItemFound )
        {
            firstItemFound = ( drawItem == aLastItem );
        }
        else
        {
            if( drawItem->Matches( aSearchData, this, aFindLocation ) )
                return drawItem;
        }

        drawItem = drawItem->Next();

        if( drawItem == NULL && aLastItem && firstItemFound && wrap && !hasWrapped )
        {
            hasWrapped = true;
            drawItem = LastDrawList();
        }
    }

    return NULL;
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


bool SCH_SHEET_PATH::operator==( const SCH_SHEET_PATH& d1 ) const
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


bool SCH_SHEET_PATH::operator!=( const SCH_SHEET_PATH& d1 ) const
{
    if( m_numSheets != d1.m_numSheets )
        return true;

    for( unsigned i = 0; i < m_numSheets; i++ )
    {
        if( m_sheets[i] != d1.m_sheets[i] )
        {
            /*
            printf( "micompare this:'%s' d1:'%s'\n",
                CONV_TO_UTF8( PathHumanReadable() ),
                CONV_TO_UTF8( d1.PathHumanReadable() ) );
            */

            return true;
        }
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


/**
 * Function GetFirst
 *  @return the first item (sheet) in m_List and prepare calls to GetNext()
 */
SCH_SHEET_PATH* SCH_SHEET_LIST::GetFirst()
{
    m_index = 0;

    if( GetCount() > 0 )
        return &( m_List[0] );

    return NULL;
}


/**
 * Function GetNext
 *  @return the next item (sheet) in m_List or NULL if no more item in sheet
 * list
 */
SCH_SHEET_PATH* SCH_SHEET_LIST::GetNext()
{
    if( m_index < GetCount() )
        m_index++;

    return GetSheet( m_index );
}


SCH_SHEET_PATH* SCH_SHEET_LIST::GetLast()
{
    if( GetCount() == 0 )
        return NULL;

    m_index = GetCount() - 1;

    return GetSheet( m_index );
}


SCH_SHEET_PATH* SCH_SHEET_LIST::GetPrevious()
{
    if( m_index == 0 )
        return NULL;

    m_index -= 1;

    return GetSheet( m_index );
}


/**
 * Function GetSheet
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
        EDA_ITEM* strct = m_currList.LastDrawList();

        while( strct )
        {
            if( strct->Type() == SCH_SHEET_T )
            {
                SCH_SHEET* sheet = (SCH_SHEET*) strct;
                BuildSheetList( sheet );
            }

            strct = strct->Next();
        }
    }

    m_currList.Pop();
}


void SCH_SHEET_LIST::AnnotatePowerSymbols()
{
    int ref = 1;

    for( SCH_SHEET_PATH* path = GetFirst();  path != NULL;  path = GetNext() )
        path->AnnotatePowerSymbols( &ref );
}


void SCH_SHEET_LIST::GetComponents( SCH_REFERENCE_LIST& aReferences,
                                    bool                aIncludePowerSymbols )
{
    for( SCH_SHEET_PATH* path = GetFirst();  path != NULL;  path = GetNext() )
        path->GetComponents( aReferences, aIncludePowerSymbols );
}


SCH_ITEM* SCH_SHEET_LIST::FindNextItem( KICAD_T aType, SCH_SHEET_PATH** aSheetFoundIn,
                                        SCH_ITEM* aLastItem, bool aWrap )
{
    bool hasWrapped = false;
    bool firstItemFound = false;
    SCH_ITEM* drawItem = NULL;
    SCH_SHEET_PATH* sheet = GetFirst();

    while( sheet != NULL )
    {
        drawItem = sheet->LastDrawList();

        while( drawItem != NULL )
        {
            if( drawItem->Type() == aType )
            {
                if( aLastItem == NULL || firstItemFound )
                {
                    if( aSheetFoundIn )
                        *aSheetFoundIn = sheet;

                    return drawItem;
                }
                else if( !firstItemFound && drawItem == aLastItem )
                {
                    firstItemFound = true;
                }
            }

            drawItem = drawItem->Next();
        }

        sheet = GetNext();

        if( sheet == NULL && aLastItem && aWrap && !hasWrapped )
        {
            hasWrapped = true;
            sheet = GetFirst();
        }
    }

    return NULL;
}


SCH_ITEM* SCH_SHEET_LIST::FindPreviousItem( KICAD_T aType, SCH_SHEET_PATH** aSheetFoundIn,
                                            SCH_ITEM* aLastItem, bool aWrap )
{
    bool hasWrapped = false;
    bool firstItemFound = false;
    SCH_ITEM* drawItem = NULL;
    SCH_SHEET_PATH* sheet = GetLast();

    while( sheet != NULL )
    {
        drawItem = sheet->FirstDrawList();

        while( drawItem != NULL )
        {
            if( drawItem->Type() == aType )
            {
                if( aLastItem == NULL || firstItemFound )
                {
                    if( aSheetFoundIn )
                        *aSheetFoundIn = sheet;

                    return drawItem;
                }
                else if( !firstItemFound && drawItem == aLastItem )
                {
                    firstItemFound = true;
                }
            }

            drawItem = drawItem->Back();
        }

        sheet = GetPrevious();

        if( sheet == NULL && aLastItem && aWrap && !hasWrapped )
        {
            hasWrapped = true;
            sheet = GetLast();
        }
    }

    return NULL;
}


SCH_ITEM* SCH_SHEET_LIST::MatchNextItem( wxFindReplaceData& aSearchData,
                                         SCH_SHEET_PATH**   aSheetFoundIn,
                                         SCH_ITEM*          aLastItem,
                                         wxPoint * aFindLocation )
{
    bool hasWrapped = false;
    bool firstItemFound = false;
    bool wrap = ( aSearchData.GetFlags() & FR_SEARCH_WRAP ) != 0;
    SCH_ITEM* drawItem = NULL;
    SCH_SHEET_PATH* sheet = GetFirst();

    while( sheet != NULL )
    {
        drawItem = sheet->LastDrawList();

        while( drawItem != NULL )
        {
            if( aLastItem && !firstItemFound )
            {
                firstItemFound = ( drawItem == aLastItem );
            }
            else
            {
                if( drawItem->Matches( aSearchData, sheet, aFindLocation ) )
                {
                    if( aSheetFoundIn )
                        *aSheetFoundIn = sheet;

                    return drawItem;
                }
            }

            drawItem = drawItem->Next();
        }

        sheet = GetNext();

        if( sheet == NULL && aLastItem && firstItemFound && wrap && !hasWrapped )
        {
            hasWrapped = true;
            sheet = GetFirst();
        }
    }

    return NULL;
}
