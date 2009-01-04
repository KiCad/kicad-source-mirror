/////////////////////////////////////////////////////////////////////////////

// Name:        class_drawsheet.cpp
// Purpose:		member functions for DrawSheetStruct
//				header = class_drawsheet.h
// Author:      jean-pierre Charras
// Modified by:
// Created:     08/02/2006 18:37:02
// RCS-ID:
// Copyright:
// Licence:     License GNU
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "fctsys.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"


/**********************************************/
/* class to handle a series of sheets *********/
/* a 'path' so to speak.. *********************/
/**********************************************/
DrawSheetPath::DrawSheetPath()
{
    for( int i = 0; i<DSLSZ; i++ )
        m_sheets[i] = NULL;

    m_numSheets = 0;
}


int DrawSheetPath::Cmp( const DrawSheetPath& d ) const
{
    if( m_numSheets > d.m_numSheets )
        return 1;
    if( m_numSheets < d.m_numSheets )
        return -1;

    //otherwise, same number of sheets.
    for( int i = 0; i<m_numSheets; i++ )
    {
        if( m_sheets[i]->m_TimeStamp > d.m_sheets[i]->m_TimeStamp )
            return 1;
        if( m_sheets[i]->m_TimeStamp < d.m_sheets[i]->m_TimeStamp )
            return -1;
    }

    return 0;
}


DrawSheetStruct* DrawSheetPath::Last()
{
    if( m_numSheets )
        return m_sheets[m_numSheets - 1];
    return NULL;
}


SCH_SCREEN* DrawSheetPath::LastScreen()
{
    if( m_numSheets )
        return m_sheets[m_numSheets - 1]->m_AssociatedScreen;
    return NULL;
}


EDA_BaseStruct* DrawSheetPath::LastDrawList()
{
    if( m_numSheets && m_sheets[m_numSheets - 1]->m_AssociatedScreen )
        return m_sheets[m_numSheets - 1]->m_AssociatedScreen->EEDrawList;
    return NULL;
}


void DrawSheetPath::Push( DrawSheetStruct* sheet )
{
    wxASSERT( m_numSheets <= DSLSZ );
    if( m_numSheets < DSLSZ )
    {
        m_sheets[m_numSheets] = sheet;
        m_numSheets++;
    }
}


DrawSheetStruct* DrawSheetPath::Pop()
{
    if( m_numSheets > 0 )
    {
        m_numSheets--;
        return m_sheets[m_numSheets];
    }
    return NULL;
}


wxString DrawSheetPath::Path()
{
    wxString s, t;

    s = wxT( "/" );

    //start at 1 to avoid the root sheet,
    //which does not need to be added to the path
    //it's timestamp changes anyway.
    for( int i = 1; i< m_numSheets; i++ )
    {
        t.Printf( _( "%8.8lX/" ), m_sheets[i]->m_TimeStamp );
        s = s + t;
    }

    return s;
}


wxString DrawSheetPath::PathHumanReadable()
{
    wxString s, t;

    s = wxT( "/" );

    //start at 1 to avoid the root sheet, as above.
    for( int i = 1; i< m_numSheets; i++ )
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
            component->GetField(REFERENCE)->m_Text = component->GetRef( this );
            component->m_Multi = component->GetUnitSelection( this );
        }
        t = t->Next();
    }
}


bool DrawSheetPath::operator=( const DrawSheetPath& d1 )
{
    m_numSheets = d1.m_numSheets;
    int i;
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
    for( int i = 0; i<m_numSheets; i++ )
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
    for( int i = 0; i<m_numSheets; i++ )
    {
        if( m_sheets[i] != d1.m_sheets[i] )
            return true;
    }

    return false;
}



/*********************************************************************/
/* Class EDA_SheetList to handle the list of Sheets in a hierarchy */
/*********************************************************************/

/*****************************************/
DrawSheetPath* EDA_SheetList::GetFirst()
/*****************************************/
{
    m_index = 0;
    if( m_count > 0 )
        return &( m_List[0] );
    return NULL;
}


/*****************************************/
DrawSheetPath* EDA_SheetList::GetNext()
/*****************************************/
{
    if( m_index < m_count )
        m_index++;
    return GetSheet( m_index );
}


/************************************************/
DrawSheetPath* EDA_SheetList::GetSheet( int index )
/************************************************/

/* return the m_List[index] item
 */
{
    if( index < m_count )
        return &(m_List[index]);
    return NULL;
}


/************************************************************************/
void EDA_SheetList::BuildSheetList( DrawSheetStruct* sheet )
/************************************************************************/
{
    if( m_List == NULL )
    {
        int count = sheet->CountSheets();
        m_count = count;
        m_index = 0;
        count *= sizeof(DrawSheetPath);
        m_List = (DrawSheetPath*) MyZMalloc( count );
        m_currList.Clear();
    }
    m_currList.Push( sheet );
    m_List[m_index] = m_currList;
    m_index++;
    if( sheet->m_AssociatedScreen != NULL )
    {
        EDA_BaseStruct* strct = m_currList.LastDrawList();
        while( strct )
        {
            if( strct->Type() == DRAW_SHEET_STRUCT_TYPE )
            {
                DrawSheetStruct* sht = (DrawSheetStruct*) strct;
                BuildSheetList( sht );
            }
            strct = strct->Next();
        }
    }
    m_currList.Pop();
}
