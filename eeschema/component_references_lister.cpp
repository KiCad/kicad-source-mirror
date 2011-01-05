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



bool SCH_REFERENCE_LIST::sortByXPosition( const SCH_REFERENCE& item1,
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


bool SCH_REFERENCE_LIST::sortByYPosition( const SCH_REFERENCE& item1,
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


bool SCH_REFERENCE_LIST::sortByRefAndValue( const SCH_REFERENCE& item1,
                                            const SCH_REFERENCE& item2 )
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


bool SCH_REFERENCE_LIST::sortByValueOnly( const SCH_REFERENCE& item1,
                                          const SCH_REFERENCE& item2 )
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


bool SCH_REFERENCE_LIST::sortByReferenceOnly( const SCH_REFERENCE& item1,
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


bool SCH_REFERENCE_LIST::sortByTimeStamp( const SCH_REFERENCE& item1,
                                          const SCH_REFERENCE& item2 )
{
    int ii = item1.m_SheetPath.Cmp( item2.m_SheetPath );

    if( ii == 0 )
        ii = item1.m_TimeStamp - item2.m_TimeStamp;

    return ii < 0;
}


int SCH_REFERENCE_LIST::FindUnit( size_t aIndex, int aUnit )
{
    int NumRef;

    NumRef = componentFlatList[aIndex].m_NumRef;

    for( size_t ii = 0; ii < componentFlatList.size(); ii++ )
    {
        if(  ( aIndex == ii )
          || ( componentFlatList[ii].m_IsNew )
          || ( componentFlatList[ii].m_NumRef != NumRef )
          || ( componentFlatList[aIndex].CompareRef( componentFlatList[ii] ) != 0 ) )
            continue;

        if( componentFlatList[ii].m_Unit == aUnit )
            return (int) ii;
    }

    return -1;
}


/* Remove sub components from the list, when multiples parts per package are
 * found in this list
 */
void SCH_REFERENCE_LIST::RemoveSubComponentsFromList()
{
    SCH_COMPONENT* libItem;
    wxString       oldName;
    wxString       currName;

    // The component list **MUST** be sorted by reference and by unit number
    // in order to find all parts of a component
    SortByReferenceOnly();

    for( unsigned ii = 0; ii < componentFlatList.size(); ii++ )
    {
        libItem = componentFlatList[ii].m_RootCmp;
        if( libItem == NULL )
            continue;

        currName = componentFlatList[ii].GetRef();

        if( !oldName.IsEmpty() )
        {
            if( oldName == currName )   // currName is a subpart of oldName: remove it
            {
                componentFlatList.erase( componentFlatList.begin() + ii );
                ii--;
            }
        }

        oldName = currName;
    }
}


void SCH_REFERENCE_LIST::ResetHiddenReferences()
{
    for( unsigned ii = 0; ii < componentFlatList.size(); ii++ )
    {
        if( componentFlatList[ii].GetRefStr()[0] == '#' )
        {
            componentFlatList[ii].m_IsNew  = true;
            componentFlatList[ii].m_NumRef = 0;
        }
    }
}


void SCH_REFERENCE_LIST::GetRefsInUse( int aIndex, std::vector< int >& aIdList, int aMinRefId )
{
    aIdList.clear();

    for( unsigned ii = 0; ii < componentFlatList.size(); ii++ )
    {
        if(    ( componentFlatList[aIndex].CompareRef( componentFlatList[ii] ) == 0 )
            && ( componentFlatList[ii].m_NumRef >= aMinRefId ) )
            aIdList.push_back( componentFlatList[ii].m_NumRef );
    }

    sort( aIdList.begin(), aIdList.end() );

    // Ensure each reference number appears only once.  If there are components with
    // multiple parts per package the same number will be stored for each part.
    std::vector< int >::iterator it = unique( aIdList.begin(), aIdList.end() );

    // Using the C++ unique algorithm only moves the duplicate entries to the end of
    // of the array.  This removes the duplicate entries from the array.
    aIdList.resize( it - aIdList.begin() );
}


int SCH_REFERENCE_LIST::GetLastReference( int aIndex, int aMinValue )
{
    int lastNumber = aMinValue;

    for( unsigned ii = 0; ii < componentFlatList.size(); ii++ )
    {
        // search only for the current reference prefix:
        if( componentFlatList[aIndex].CompareRef( componentFlatList[ii] ) != 0 )
            continue;

        // update max value for the current reference prefix
        if( lastNumber < componentFlatList[ii].m_NumRef )
            lastNumber = componentFlatList[ii].m_NumRef;
    }

    return lastNumber;
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

