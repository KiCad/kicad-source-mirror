/////////////////////////////////////////////////////////////////////////////
// Name:        sch_sheet_path.cpp
// Purpose:     member functions for SCH_SHEET_PATH
//              header = sch_sheet_path.h
// Author:      jean-pierre Charras
// Modified by: Wayne Stambaugh
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


SCH_SHEET_PATH::SCH_SHEET_PATH()
{
}


SCH_SHEET_PATH::~SCH_SHEET_PATH()
{
    // The sheets are not owned by the sheet path object so don't allow them to be destroyed.
    Clear();
}


void SCH_SHEET_PATH::Clear()
{
    while( !m_sheets.empty() )
        m_sheets.pop_back().release();
}


bool SCH_SHEET_PATH::BuildSheetPathInfoFromSheetPathValue( const wxString& aPath, bool aFound )
{
    if( aFound )
        return true;

    if( GetSheetCount() == 0 )
        Push( g_RootSheet );

    if( aPath == Path() )
        return true;

    SCH_ITEM* item = LastDrawList();

    while( item && GetSheetCount() < NB_MAX_SHEET )
    {
        if( item->Type() == SCH_SHEET_T )
        {
            SCH_SHEET* sheet = (SCH_SHEET*) item;
            Push( sheet );

            if( aPath == Path() )
                return true;

            if( BuildSheetPathInfoFromSheetPathValue( aPath ) )
                return true;

            Pop();
        }

        item = item->Next();
    }

    return false;
}


int SCH_SHEET_PATH::Cmp( const SCH_SHEET_PATH& aSheetPathToTest ) const
{
    if( m_sheets.size() > aSheetPathToTest.GetSheetCount() )
        return 1;

    if( m_sheets.size() < aSheetPathToTest.GetSheetCount() )
        return -1;

    // Same number of sheets, use time stamps.
    for( unsigned i = 0; i < GetSheetCount(); i++ )
    {
        if( m_sheets[i].m_TimeStamp > aSheetPathToTest.m_sheets[i].m_TimeStamp )
            return 1;

        if( m_sheets[i].m_TimeStamp < aSheetPathToTest.m_sheets[i].m_TimeStamp )
            return -1;
    }

    return 0;
}


SCH_SHEET* SCH_SHEET_PATH::Last()
{
    if( !m_sheets.empty() )
        return &m_sheets[ m_sheets.size() - 1 ];

    return NULL;
}


SCH_SCREEN* SCH_SHEET_PATH::LastScreen()
{
    SCH_SHEET* lastSheet = Last();

    if( lastSheet )
        return lastSheet->GetScreen();

    return NULL;
}


SCH_ITEM* SCH_SHEET_PATH::LastDrawList()
{
    SCH_SCREEN* screen = LastScreen();

    if( screen )
        return screen->GetDrawItems();

    return NULL;
}


SCH_ITEM* SCH_SHEET_PATH::FirstDrawList()
{
    SCH_ITEM* item = NULL;

    if( !m_sheets.empty() && m_sheets[0].GetScreen() )
        item = m_sheets[0].GetScreen()->GetDrawItems();

    return item;
}


void SCH_SHEET_PATH::Push( SCH_SHEET* aSheet )
{
    if( m_sheets.size() >= MAX_SHEET_PATH_DEPTH )
    {
        wxLogWarning( _( "Schematic sheets can only be nested %d levels deep.  Not adding sheet %s" ),
                      MAX_SHEET_PATH_DEPTH, GetChars( aSheet->m_SheetName ) );
        return;
    }

    m_sheets.push_back( aSheet );
}


SCH_SHEET* SCH_SHEET_PATH::Pop()
{
    if( m_sheets.empty() )
        return NULL;

    // The sheet must be released from the end of the container otherwise it will be destroyed.
    return m_sheets.pop_back().release();
}


wxString SCH_SHEET_PATH::Path() const
{
    wxString s, t;

    s = wxT( "/" );     // This is the root path

    // start at 1 to avoid the root sheet,
    // which does not need to be added to the path
    // it's timestamp changes anyway.
    for( unsigned i = 1; i < m_sheets.size(); i++ )
    {
        t.Printf( _( "%8.8lX/" ), m_sheets[i].m_TimeStamp );
        s = s + t;
    }

    return s;
}


wxString SCH_SHEET_PATH::PathHumanReadable() const
{
    wxString s, t;

    s = wxT( "/" );

    // start at 1 to avoid the root sheet, as above.
    for( unsigned i = 1; i< m_sheets.size(); i++ )
    {
        s = s + m_sheets[i].m_SheetName + wxT( "/" );
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


void SCH_SHEET_PATH::GetComponents( SCH_REFERENCE_LIST& aReferences, bool aIncludePowerSymbols )
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
                                         wxPoint*           aFindLocation )
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


bool SCH_SHEET_PATH::operator==( const SCH_SHEET_PATH& d1 ) const
{
    if( m_sheets.size() != d1.m_sheets.size() )
        return false;

    for( unsigned i = 0; i < m_sheets.size(); i++ )
    {
        if( &m_sheets[i] != &d1.m_sheets[i] )
            return false;
    }

    return true;
}


/*********************************************************************/
/* Class SCH_SHEET_LIST to handle the list of Sheets in a hierarchy */
/*********************************************************************/


SCH_SHEET_LIST::SCH_SHEET_LIST( SCH_SHEET* aSheet )
{
    m_index = 0;
    m_count = 0;
    m_List  = NULL;

    if( aSheet == NULL )
        aSheet = g_RootSheet;

    BuildSheetList( aSheet );
}


SCH_SHEET_PATH* SCH_SHEET_LIST::GetFirst()
{
    m_index = 0;

    if( GetCount() > 0 )
        return &( m_List[0] );

    return NULL;
}


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


SCH_SHEET_PATH* SCH_SHEET_LIST::GetSheet( int aIndex )
{
    if( aIndex < GetCount() )
        return &( m_List[aIndex] );

    return NULL;
}


bool SCH_SHEET_LIST::IsModified()
{
    for( SCH_SHEET_PATH* sheet = GetFirst(); sheet != NULL; sheet = GetNext() )
    {
        if( sheet->LastScreen() && sheet->LastScreen()->IsModify() )
            return true;
    }

    return false;
}


void SCH_SHEET_LIST::ClearModifyStatus()
{
    for( SCH_SHEET_PATH* sheet = GetFirst(); sheet != NULL; sheet = GetNext() )
    {
        if( sheet->LastScreen() )
            sheet->LastScreen()->ClrModify();
    }
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

    if( aSheet->GetScreen() != NULL )
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
                                         wxPoint*           aFindLocation )
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
