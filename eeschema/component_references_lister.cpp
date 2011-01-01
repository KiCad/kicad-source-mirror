/*
 * component_references_lister.cpp: creates a flat list of components.
 * Needed for annotation and BOM.
 */

/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2011 jean-pierre Charras <jean-pierre.charras@gipsa-lab.inpg.fr>
 * Copyright (C) 1992-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 Kicad Developers, see change_log.txt for contributors.
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


#include <algorithm> // to use sort vector
#include <vector>

#include "fctsys.h"
#include "common.h"
#include "kicad_string.h"
#include "wxEeschemaStruct.h"
#include "wxstruct.h"
#include "netlist.h"
#include "class_sch_screen.h"
#include "sch_component.h"



/* sort function to annotate items from their position.
 *  Components are sorted
 *      by reference
 *      if same reference: by sheet
 *          if same sheet, by X pos
 *                if same X pos, by Y pos
 *                  if same Y pos, by time stamp
 */
bool SCH_REFERENCE_LIST::sortBy_X_Position( const SCH_REFERENCE& item1,
                                   const SCH_REFERENCE& item2 )
{
    int ii = item1.CompareRef( item2 );

    if( ii == 0 )
        ii = item1.m_SheetNum - item2.m_SheetNum;
    if( ii == 0 )
        ii = item1.m_CmpPos.x - item2.m_CmpPos.x;
    if( ii == 0 )
        ii = item1.m_CmpPos.y - item2.m_CmpPos.y;
    if( ii == 0 )
        ii = item1.m_TimeStamp - item2.m_TimeStamp;

    return ii < 0;
}


/* sort function to annotate items by their position.
 *  Components are sorted
 *      by reference
 *      if same reference: by sheet
 *          if same sheet, by Y pos
 *                if same Y pos, by X pos
 *                  if same X pos, by time stamp
 */
bool SCH_REFERENCE_LIST::sortBy_Y_Position( const SCH_REFERENCE& item1,
                                   const SCH_REFERENCE& item2 )
{
    int ii = item1.CompareRef( item2 );

    if( ii == 0 )
        ii = item1.m_SheetNum - item2.m_SheetNum;
    if( ii == 0 )
        ii = item1.m_CmpPos.y - item2.m_CmpPos.y;
    if( ii == 0 )
        ii = item1.m_CmpPos.x - item2.m_CmpPos.x;
    if( ii == 0 )
        ii = item1.m_TimeStamp - item2.m_TimeStamp;

    return ii < 0;
}

/*
 * sort function to annotate items by value
 *  Components are sorted
 *      by reference
 *      if same reference: by value
 *          if same value: by unit number
 *              if same unit number, by sheet
 *                  if same sheet, by position X, and Y
 */
bool SCH_REFERENCE_LIST::sortByRefAndValue( const SCH_REFERENCE& item1, const SCH_REFERENCE& item2 )
{
    int ii = item1.CompareRef( item2 );
    if( ii == 0 )
        ii = item1.CompareValue( item2 );
    if( ii == 0 )
        ii = item1.m_Unit - item2.m_Unit;
    if( ii == 0 )
        ii = item1.m_SheetNum - item2.m_SheetNum;
    if( ii == 0 )
        ii = item1.m_CmpPos.x - item2.m_CmpPos.x;
    if( ii == 0 )
        ii = item1.m_CmpPos.y - item2.m_CmpPos.y;
    if( ii == 0 )
        ii = item1.m_TimeStamp - item2.m_TimeStamp;

    return ii < 0;
}

/* sort function for for list by values
 * components are sorted
 *    by value
 *    if same value: by reference
 *         if same reference: by unit number
 */
bool SCH_REFERENCE_LIST::sortComponentsByValueOnly( const SCH_REFERENCE& item1, const SCH_REFERENCE& item2 )
{
    int             ii;
    const wxString* Text1, * Text2;

    Text1 = &( item1.m_RootCmp->GetField( VALUE )->m_Text );
    Text2 = &( item2.m_RootCmp->GetField( VALUE )->m_Text );
    ii    = Text1->CmpNoCase( *Text2 );

    if( ii == 0 )
    {
        ii = RefDesStringCompare( item1.GetRef(), item2.GetRef() );
    }

    if( ii == 0 )
    {
        ii = item1.m_Unit - item2.m_Unit;
    }

    return ii < 0;
}

/**
 * Function sortComponentsByReferenceOnly
 * compare function for sorting in BOM creation.
 * components are sorted
 *     by reference
 *     if same reference: by value
 *         if same value: by unit number
 */
bool SCH_REFERENCE_LIST::sortComponentsByReferenceOnly( const SCH_REFERENCE& item1,
                                                  const SCH_REFERENCE& item2 )
{
    int             ii;
    const wxString* Text1, * Text2;

    ii = RefDesStringCompare( item1.GetRef(), item2.GetRef() );

    if( ii == 0 )
    {
        Text1 = &( item1.m_RootCmp->GetField( VALUE )->m_Text );
        Text2 = &( item2.m_RootCmp->GetField( VALUE )->m_Text );
        ii    = Text1->CmpNoCase( *Text2 );
    }

    if( ii == 0 )
    {
        ii = item1.m_Unit - item2.m_Unit;
    }

    return ii < 0;
}


/*****************************************************************************
 * qsort function to annotate items by value
 *  Components are sorted by time stamp
 *****************************************************************************/
bool SCH_REFERENCE_LIST::sortByTimeStamp( const SCH_REFERENCE& item1,
                             const SCH_REFERENCE& item2 )
{
    int ii = item1.m_SheetPath.Cmp( item2.m_SheetPath );

    if( ii == 0 )
        ii = item1.m_TimeStamp - item2.m_TimeStamp;

    return ii < 0;
}


/* Remove sub components from the list, when multiples parts per package are
 * found in this list
 */
void SCH_REFERENCE_LIST::RemoveSubComponentsFromList( )
{
    SCH_COMPONENT* libItem;
    wxString       oldName;
    wxString       currName;

    // The component list **MUST** be sorted by reference and by unit number
    // in order to find all parts of a component
    SortComponentsByReferenceOnly();
    for( unsigned ii = 0; ii < componentFlatList.size(); ii++ )
    {
        libItem = componentFlatList[ii].m_RootCmp;
        if( libItem == NULL )
            continue;

        currName = componentFlatList[ii].GetRef();

        if( !oldName.IsEmpty() )
        {
            if( oldName == currName )   // currName is a subpart of oldName:
                                        // remove it
            {
                componentFlatList.erase( componentFlatList.begin() + ii );
                ii--;
            }
        }
        oldName = currName;
    }
}



SCH_REFERENCE::SCH_REFERENCE( SCH_COMPONENT* aComponent, LIB_COMPONENT* aLibComponent,
                              SCH_SHEET_PATH& aSheetPath )
{
    wxASSERT( aComponent != NULL && aLibComponent != NULL );

    m_RootCmp   = aComponent;
    m_Entry     = aLibComponent;
    m_Unit      = aComponent->GetUnitSelection( &aSheetPath );
    m_SheetPath = aSheetPath;
    m_IsNew     = false;
    m_Flag      = 0;
    m_TimeStamp = aComponent->m_TimeStamp;
    m_CmpPos    = aComponent->m_Pos;
    m_SheetNum  = 0;

    if( aComponent->GetRef( &aSheetPath ).IsEmpty() )
        aComponent->SetRef( &aSheetPath, wxT( "DefRef?" ) );

    SetRef( aComponent->GetRef( &aSheetPath ) );

    m_NumRef = -1;

    if( aComponent->GetField( VALUE )->GetText().IsEmpty() )
        aComponent->GetField( VALUE )->SetText( wxT( "~" ) );

    m_Value = &aComponent->GetField( VALUE )->m_Text;
}


void SCH_REFERENCE::Annotate()
{
    if( m_NumRef < 0 )
        m_Ref += wxChar( '?' );
    else
        m_Ref = CONV_TO_UTF8( GetRef() << m_NumRef );

    m_RootCmp->SetRef( &m_SheetPath, CONV_FROM_UTF8( m_Ref.c_str() ) );
    m_RootCmp->SetUnit( m_Unit );
    m_RootCmp->SetUnitSelection( &m_SheetPath, m_Unit );
}


void SCH_REFERENCE::Split()
{
    std::string refText = GetRefStr();

    m_NumRef = -1;

    int ll = refText.length() - 1;

    if( refText[ll] == '?' )
    {
        m_IsNew = true;

        if( !IsPartsLocked() )
            m_Unit = 0x7FFFFFFF;

        refText.erase( ll );  // delete last char

        SetRefStr( refText );
    }
    else if( isdigit( refText[ll] ) == 0 )
    {
        m_IsNew = true;

        if( !IsPartsLocked() )
            m_Unit = 0x7FFFFFFF;
    }
    else
    {
        while( ll >= 0 )
        {
            if( (refText[ll] <= ' ' ) || isdigit( refText[ll] ) )
                ll--;
            else
            {
                if( isdigit( refText[ll + 1] ) )
                {
                    // null terminated C string into cp
                    const char* cp = refText.c_str() + ll + 1;

                    m_NumRef = atoi( cp );
                }

                refText.erase( ll+1 );  // delete from ll+1 to end
                break;
            }
        }

        SetRefStr( refText );
    }
}

