/**
 * @file component_references_lister.cpp
 * @brief Code for creating a flat list of components needed for annotation and BOM.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2011 jean-pierre Charras <jean-pierre.charras@gipsa-lab.inpg.fr>
 * Copyright (C) 1992-2015 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2015 KiCad Developers, see change_log.txt for contributors.
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


#include <wx/regex.h>
#include <algorithm>
#include <vector>

#include <fctsys.h>
#include <kicad_string.h>
#include <schframe.h>
#include <sch_reference_list.h>
#include <sch_component.h>

#include <boost/foreach.hpp>


//#define USE_OLD_ALGO


void SCH_REFERENCE_LIST::RemoveItem( unsigned int aIndex )
{
    if( aIndex < componentFlatList.size() )
        componentFlatList.erase( componentFlatList.begin() + aIndex );
}


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


bool SCH_REFERENCE_LIST::sortByReferenceOnly( const SCH_REFERENCE& item1,
                                              const SCH_REFERENCE& item2 )
{
    int             ii;

    ii = RefDesStringCompare( item1.GetRef(), item2.GetRef() );

    if( ii == 0 )
    {
        ii = item1.m_RootCmp->GetField( VALUE )->GetText().CmpNoCase( item2.m_RootCmp->GetField( VALUE )->GetText() );
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
        if(  ( componentFlatList[aIndex].CompareRef( componentFlatList[ii] ) == 0 )
          && ( componentFlatList[ii].m_NumRef >= aMinRefId )  )
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


int SCH_REFERENCE_LIST::CreateFirstFreeRefId( std::vector<int>& aIdList, int aFirstValue )
{
    int expectedId = aFirstValue;

    // We search for expected Id a value >= aFirstValue.
    // Skip existing Id < aFirstValue
    unsigned ii = 0;

    for( ; ii < aIdList.size(); ii++ )
    {
        if( expectedId <= aIdList[ii] )
            break;
    }

    // Ids are sorted by increasing value, from aFirstValue
    // So we search from aFirstValue the first not used value, i.e. the first hole in list.
    for( ; ii < aIdList.size(); ii++ )
    {
        if( expectedId != aIdList[ii] )    // This id is not yet used.
        {
            // Insert this free Id, in order to keep list sorted
            aIdList.insert( aIdList.begin() + ii, expectedId );
            return expectedId;
        }

        expectedId++;
    }

    // All existing Id are tested, and all values are found in use.
    // So Create a new one.
    aIdList.push_back( expectedId );
    return expectedId;
}


void SCH_REFERENCE_LIST::Annotate( bool aUseSheetNum, int aSheetIntervalId,
      SCH_MULTI_UNIT_REFERENCE_MAP aLockedUnitMap )
{
    if ( componentFlatList.size() == 0 )
        return;

    int LastReferenceNumber = 0;
    int NumberOfUnits, Unit;

    // Components with an invisible reference (power...) always are re-annotated.
    ResetHiddenReferences();

    /* calculate index of the first component with the same reference prefix
     * than the current component.  All components having the same reference
     * prefix will receive a reference number with consecutive values:
     * IC .. will be set to IC4, IC4, IC5 ...
     */
    unsigned first = 0;

    // calculate the last used number for this reference prefix:
#ifdef USE_OLD_ALGO
    int minRefId = 0;

    // when using sheet number, ensure ref number >= sheet number* aSheetIntervalId
    if( aUseSheetNum )
        minRefId = componentFlatList[first].m_SheetNum * aSheetIntervalId;

    LastReferenceNumber = GetLastReference( first, minRefId );
#else
    int minRefId = 1;

    // when using sheet number, ensure ref number >= sheet number* aSheetIntervalId
    if( aUseSheetNum )
        minRefId = componentFlatList[first].m_SheetNum * aSheetIntervalId + 1;

    // This is the list of all Id already in use for a given reference prefix.
    // Will be refilled for each new reference prefix.
    std::vector<int>idList;
    GetRefsInUse( first, idList, minRefId );
#endif
    for( unsigned ii = 0; ii < componentFlatList.size(); ii++ )
    {
        if( componentFlatList[ii].m_Flag )
            continue;

        // Check whether this component is in aLockedUnitMap.
        SCH_REFERENCE_LIST* lockedList = NULL;
        BOOST_FOREACH( SCH_MULTI_UNIT_REFERENCE_MAP::value_type& pair, aLockedUnitMap )
        {
            unsigned n_refs = pair.second.GetCount();
            for( unsigned thisRefI = 0; thisRefI < n_refs; ++thisRefI )
            {
                SCH_REFERENCE &thisRef = pair.second[thisRefI];

                if( thisRef.IsSameInstance( componentFlatList[ii] ) )
                {
                    lockedList = &pair.second;
                    break;
                }
            }
            if( lockedList != NULL ) break;
        }

        if(  ( componentFlatList[first].CompareRef( componentFlatList[ii] ) != 0 )
          || ( aUseSheetNum && ( componentFlatList[first].m_SheetNum != componentFlatList[ii].m_SheetNum ) )  )
        {
            // New reference found: we need a new ref number for this reference
            first = ii;
#ifdef USE_OLD_ALGO
            minRefId = 0;

            // when using sheet number, ensure ref number >= sheet number* aSheetIntervalId
            if( aUseSheetNum )
                minRefId = componentFlatList[ii].m_SheetNum * aSheetIntervalId;

            LastReferenceNumber = componentFlatList.GetLastReference( ii, minRefId );
#else
            minRefId = 1;

            // when using sheet number, ensure ref number >= sheet number* aSheetIntervalId
            if( aUseSheetNum )
                minRefId = componentFlatList[ii].m_SheetNum * aSheetIntervalId + 1;

            GetRefsInUse( first, idList, minRefId );
#endif
        }

        // Annotation of one part per package components (trivial case).
        if( componentFlatList[ii].GetLibComponent()->GetUnitCount() <= 1 )
        {
            if( componentFlatList[ii].m_IsNew )
            {
#ifdef USE_OLD_ALGO
                LastReferenceNumber++;
#else
                LastReferenceNumber = CreateFirstFreeRefId( idList, minRefId );
#endif
                componentFlatList[ii].m_NumRef = LastReferenceNumber;
            }

            componentFlatList[ii].m_Unit  = 1;
            componentFlatList[ii].m_Flag  = 1;
            componentFlatList[ii].m_IsNew = false;
            continue;
        }

        // Annotation of multi-unit parts ( n units per part ) (complex case)
        NumberOfUnits = componentFlatList[ii].GetLibComponent()->GetUnitCount();

        if( componentFlatList[ii].m_IsNew )
        {
#ifdef USE_OLD_ALGO
            LastReferenceNumber++;
#else
            LastReferenceNumber = CreateFirstFreeRefId( idList, minRefId );
#endif
            componentFlatList[ii].m_NumRef = LastReferenceNumber;

            if( !componentFlatList[ii].IsUnitsLocked() )
                componentFlatList[ii].m_Unit = 1;

            componentFlatList[ii].m_Flag = 1;
        }

        // If this component is in aLockedUnitMap, copy the annotation to all
        // components that are not it
        if( lockedList != NULL )
        {
            unsigned n_refs = lockedList->GetCount();
            for( unsigned thisRefI = 0; thisRefI < n_refs; ++thisRefI )
            {
                SCH_REFERENCE &thisRef = (*lockedList)[thisRefI];
                if( thisRef.IsSameInstance( componentFlatList[ii] ) )
                {
                    // This is the component we're currently annotating. Hold the unit!
                    componentFlatList[ii].m_Unit = thisRef.m_Unit;
                }

                if( thisRef.CompareValue( componentFlatList[ii] ) != 0 ) continue;
                if( thisRef.CompareLibName( componentFlatList[ii] ) != 0 ) continue;

                // Find the matching component
                for( unsigned jj = ii + 1; jj < componentFlatList.size(); jj++ )
                {
                    if( ! thisRef.IsSameInstance( componentFlatList[jj] ) ) continue;
                    componentFlatList[jj].m_NumRef = componentFlatList[ii].m_NumRef;
                    componentFlatList[jj].m_Unit = thisRef.m_Unit;
                    componentFlatList[jj].m_IsNew = false;
                    componentFlatList[jj].m_Flag = 1;
                    break;
                }
            }
        }

        else
        {
            /* search for others units of this component.
            * we search for others parts that have the same value and the same
            * reference prefix (ref without ref number)
            */
            for( Unit = 1; Unit <= NumberOfUnits; Unit++ )
            {
                if( componentFlatList[ii].m_Unit == Unit )
                    continue;

                int found = FindUnit( ii, Unit );

                if( found >= 0 )
                    continue; // this unit exists for this reference (unit already annotated)

                // Search a component to annotate ( same prefix, same value, not annotated)
                for( unsigned jj = ii + 1; jj < componentFlatList.size(); jj++ )
                {
                    if( componentFlatList[jj].m_Flag )    // already tested
                        continue;

                    if( componentFlatList[ii].CompareRef( componentFlatList[jj] ) != 0 )
                        continue;

                    if( componentFlatList[jj].CompareValue( componentFlatList[ii] ) != 0 )
                        continue;

                    if( componentFlatList[jj].CompareLibName( componentFlatList[ii] ) != 0 )
                        continue;

                    if( !componentFlatList[jj].m_IsNew )
                        continue;

                    // Component without reference number found, annotate it if possible
                    if( !componentFlatList[jj].IsUnitsLocked()
                        || ( componentFlatList[jj].m_Unit == Unit ) )
                    {
                        componentFlatList[jj].m_NumRef = componentFlatList[ii].m_NumRef;
                        componentFlatList[jj].m_Unit   = Unit;
                        componentFlatList[jj].m_Flag   = 1;
                        componentFlatList[jj].m_IsNew  = false;
                        break;
                    }
                }
            }
        }
    }
}


int SCH_REFERENCE_LIST::CheckAnnotation( wxArrayString* aMessageList )
{
    int            error = 0;
    wxString       tmp;
    wxString       msg;

    SortByRefAndValue();

    // Spiit reference designators into name (prefix) and number: IC1 becomes IC, and 1.
    SplitReferences();

    // count not yet annotated items or annotation error.
    for( unsigned ii = 0; ii < componentFlatList.size(); ii++ )
    {
        msg.Empty();
        tmp.Empty();

        if( componentFlatList[ii].m_IsNew )    // Not yet annotated
        {
            if( componentFlatList[ii].m_NumRef >= 0 )
                tmp << componentFlatList[ii].m_NumRef;
            else
                tmp = wxT( "?" );


            if(  ( componentFlatList[ii].m_Unit > 0 )
              && ( componentFlatList[ii].m_Unit < 0x7FFFFFFF )  )
            {
                msg.Printf( _( "Item not annotated: %s%s (unit %d)\n" ),
                            GetChars( componentFlatList[ii].GetRef() ),
                            GetChars( tmp ),
                            componentFlatList[ii].m_Unit );
            }
            else
            {
                msg.Printf( _( "Item not annotated: %s%s\n" ),
                            GetChars( componentFlatList[ii].GetRef() ),
                            GetChars( tmp ) );
            }

            if( aMessageList )
                aMessageList->Add( msg + wxT( "\n" ) );

            error++;
            break;
        }

        // Error if unit number selected does not exist ( greater than the  number of
        // parts in the component ).  This can happen if a component has changed in a
        // library after a previous annotation.
        if( std::max( componentFlatList[ii].GetLibComponent()->GetUnitCount(), 1 )
          < componentFlatList[ii].m_Unit )
        {
            if( componentFlatList[ii].m_NumRef >= 0 )
                tmp << componentFlatList[ii].m_NumRef;
            else
                tmp = wxT( "?" );

            msg.Printf( _( "Error item %s%s unit %d and no more than %d parts\n" ),
                        GetChars( componentFlatList[ii].GetRef() ),
                        GetChars( tmp ),
                        componentFlatList[ii].m_Unit,
                        componentFlatList[ii].GetLibComponent()->GetUnitCount() );

            if( aMessageList )
                aMessageList->Add( msg );

            error++;
            break;
        }
    }

    if( error )
        return error;

    // count the duplicated elements (if all are annotated)
    int imax = componentFlatList.size() - 1;

    for( int ii = 0; (ii < imax) && (error < 4); ii++ )
    {
        msg.Empty();
        tmp.Empty();

        if(  ( componentFlatList[ii].CompareRef( componentFlatList[ii + 1] ) != 0 )
          || ( componentFlatList[ii].m_NumRef != componentFlatList[ii + 1].m_NumRef )  )
            continue;

        // Same reference found. If same unit, error!
        if( componentFlatList[ii].m_Unit == componentFlatList[ii + 1].m_Unit )
        {
            if( componentFlatList[ii].m_NumRef >= 0 )
                tmp << componentFlatList[ii].m_NumRef;
            else
                tmp = wxT( "?" );

            if( ( componentFlatList[ii].m_Unit > 0 )
             && ( componentFlatList[ii].m_Unit < 0x7FFFFFFF ) )
            {
                msg.Printf( _( "Multiple item %s%s (unit %d)\n" ),
                            GetChars( componentFlatList[ii].GetRef() ),
                            GetChars( tmp ),
                            componentFlatList[ii].m_Unit );
            }
            else
            {
                msg.Printf( _( "Multiple item %s%s\n" ),
                            GetChars( componentFlatList[ii].GetRef() ),
                            GetChars( tmp ) );
            }

            if( aMessageList )
                aMessageList->Add( msg );

            error++;
            continue;
        }

        /* Test error if units are different but number of parts per package
         * too high (ex U3 ( 1 part) and we find U3B this is an error) */
        if(  componentFlatList[ii].GetLibComponent()->GetUnitCount()
          != componentFlatList[ii + 1].GetLibComponent()->GetUnitCount()  )
        {
            if( componentFlatList[ii].m_NumRef >= 0 )
                tmp << componentFlatList[ii].m_NumRef;
            else
                tmp = wxT( "?" );

            if( ( componentFlatList[ii].m_Unit > 0 )
             && ( componentFlatList[ii].m_Unit < 0x7FFFFFFF ) )
            {
                msg.Printf( _( "Multiple item %s%s (unit %d)\n" ),
                            GetChars( componentFlatList[ii].GetRef() ),
                            GetChars( tmp ),
                            componentFlatList[ii].m_Unit );
            }
            else
            {
                msg.Printf( _( "Multiple item %s%s\n" ),
                            GetChars( componentFlatList[ii].GetRef() ),
                            GetChars( tmp ) );
            }

            if( aMessageList )
                aMessageList->Add( msg );

            error++;
        }

        // Error if values are different between units, for the same reference
        int next = ii + 1;

        if( componentFlatList[ii].CompareValue( componentFlatList[next] ) != 0 )
        {
            msg.Printf( _( "Different values for %s%d%s (%s) and %s%d%s (%s)" ),
                        GetChars( componentFlatList[ii].GetRef() ),
                        componentFlatList[ii].m_NumRef,
                        GetChars( LIB_PART::SubReference(
                                  componentFlatList[ii].m_Unit ) ),
                        GetChars( componentFlatList[ii].m_Value->GetText() ),
                        GetChars( componentFlatList[next].GetRef() ),
                        componentFlatList[next].m_NumRef,
                        GetChars( LIB_PART::SubReference(
                                  componentFlatList[next].m_Unit ) ),
                        GetChars( componentFlatList[next].m_Value->GetText() ) );

            if( aMessageList )
                aMessageList->Add( msg + wxT( "\n" ));

            error++;
        }
    }

    // count the duplicated time stamps
    SortByTimeStamp();

    for( int ii = 0; ( ii < imax ) && ( error < 4 ); ii++ )
    {
        if(  ( componentFlatList[ii].m_TimeStamp != componentFlatList[ii + 1].m_TimeStamp )
          || ( componentFlatList[ii].GetSheetPath() != componentFlatList[ii + 1].GetSheetPath() )  )
            continue;

        // Same time stamp found.
        wxString full_path;

        full_path.Printf( wxT( "%s%8.8X" ),
                          GetChars( componentFlatList[ii].GetSheetPath().Path() ),
                          componentFlatList[ii].m_TimeStamp );

        msg.Printf( _( "Duplicate time stamp (%s) for %s%d and %s%d" ),
                    GetChars( full_path ),
                    GetChars( componentFlatList[ii].GetRef() ), componentFlatList[ii].m_NumRef,
                    GetChars( componentFlatList[ii + 1].GetRef() ),
                    componentFlatList[ii + 1].m_NumRef );

        if( aMessageList )
            aMessageList->Add( msg + wxT( "\n" ));

        error++;
    }

    return error;
}


SCH_REFERENCE::SCH_REFERENCE( SCH_COMPONENT* aComponent, LIB_PART* aLibComponent,
                              SCH_SHEET_PATH& aSheetPath )
{
    wxASSERT( aComponent != NULL && aLibComponent != NULL );

    m_RootCmp   = aComponent;
    m_Entry     = aLibComponent;
    m_Unit      = aComponent->GetUnitSelection( aSheetPath.Last() );
    m_SheetPath = aSheetPath;
    m_IsNew     = false;
    m_Flag      = 0;
    m_TimeStamp = aComponent->GetTimeStamp();
    m_CmpPos    = aComponent->GetPosition();
    m_SheetNum  = 0;

    if( aComponent->GetRef( aSheetPath.Last() ).IsEmpty() )
        aComponent->SetRef( aSheetPath.Last(), wxT( "DefRef?" ) );

    SetRef( aComponent->GetRef( aSheetPath.Last() ) );

    m_NumRef = -1;

    if( aComponent->GetField( VALUE )->GetText().IsEmpty() )
        aComponent->GetField( VALUE )->SetText( wxT( "~" ) );

    m_Value = aComponent->GetField( VALUE );
}


void SCH_REFERENCE::Annotate()
{
    if( m_NumRef < 0 )
        m_Ref += wxChar( '?' );
    else
        m_Ref = TO_UTF8( GetRef() << m_NumRef );

    m_RootCmp->SetRef( m_SheetPath.Last(), FROM_UTF8( m_Ref.c_str() ) );
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

        if( !IsUnitsLocked() )
            m_Unit = 0x7FFFFFFF;

        refText.erase( ll );  // delete last char

        SetRefStr( refText );
    }
    else if( isdigit( refText[ll] ) == 0 )
    {
        m_IsNew = true;

        if( !IsUnitsLocked() )
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
