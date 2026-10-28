/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2018 jean-pierre Charras <jp.charras at wanadoo.fr>
 * Copyright (C) 1992-2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file sch_reference_list.cpp
 * @brief functions to create a symbol flat list and to annotate schematic.
 */

#include <sch_reference_list.h>
#include <core/kicad_algo.h>

#include <wx/regex.h>
#include <algorithm>
#include <map>
#include <optional>
#include <set>
#include <vector>
#include <unordered_set>

#include <string_utils.h>
#include <erc/erc_settings.h>
#include <refdes_tracker.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <sch_edit_frame.h>


void SCH_REFERENCE_LIST::RemoveItem( unsigned int aIndex )
{
    if( aIndex < m_flatList.size() )
        m_flatList.erase( m_flatList.begin() + aIndex );
}


bool SCH_REFERENCE_LIST::Contains( const SCH_REFERENCE& aItem ) const
{
    for( unsigned ii = 0; ii < GetCount(); ii++ )
    {
        if( m_flatList[ii].IsSameInstance( aItem ) )
            return true;
    }

    return false;
}


SCH_REFERENCE* SCH_REFERENCE_LIST::FindItem( const SCH_REFERENCE& aItem )
{
    for( unsigned ii = 0; ii < GetCount(); ii++ )
    {
        if( m_flatList[ii].IsSameInstance( aItem ) )
            return &m_flatList[ii];
    }

    return nullptr;
}


bool SCH_REFERENCE_LIST::sortByXPosition( const SCH_REFERENCE& item1, const SCH_REFERENCE& item2 )
{
    int ii = item1.CompareRef( item2 );

    if( ii == 0 )
        ii = item1.m_sheetNum - item2.m_sheetNum;

    if( ii == 0 )
        ii = item1.m_symbolPos.x - item2.m_symbolPos.x;

    if( ii == 0 )
        ii = item1.m_symbolPos.y - item2.m_symbolPos.y;

    if( ii == 0 )
        return item1.m_symbolUuid < item2.m_symbolUuid;     // ensure a deterministic sort
    else
        return ii < 0;
}


bool SCH_REFERENCE_LIST::sortByYPosition( const SCH_REFERENCE& item1, const SCH_REFERENCE& item2 )
{
    int ii = item1.CompareRef( item2 );

    if( ii == 0 )
        ii = item1.m_sheetNum - item2.m_sheetNum;

    if( ii == 0 )
        ii = item1.m_symbolPos.y - item2.m_symbolPos.y;

    if( ii == 0 )
        ii = item1.m_symbolPos.x - item2.m_symbolPos.x;

    if( ii == 0 )
        return item1.m_symbolUuid < item2.m_symbolUuid;     // ensure a deterministic sort
    else
        return ii < 0;
}


bool SCH_REFERENCE_LIST::sortByRefAndValue( const SCH_REFERENCE& item1,
                                            const SCH_REFERENCE& item2 )
{
    int ii = item1.CompareRef( item2 );

    // Keeps each channel's units adjacent for the duplicate check
    if( ii == 0 )
        ii = item1.GetRefSuffix().Cmp( item2.GetRefSuffix() );

    if( ii == 0 )
        ii = item1.CompareValue( item2 );

    if( ii == 0 )
        ii = item1.m_unit - item2.m_unit;

    if( ii == 0 )
        ii = item1.m_sheetNum - item2.m_sheetNum;

    if( ii == 0 )
        ii = item1.m_symbolPos.x - item2.m_symbolPos.x;

    if( ii == 0 )
        ii = item1.m_symbolPos.y - item2.m_symbolPos.y;

    if( ii == 0 )
        return item1.m_symbolUuid < item2.m_symbolUuid;     // ensure a deterministic sort
    else
        return ii < 0;
}


bool SCH_REFERENCE_LIST::sortByReferenceOnly( const SCH_REFERENCE& item1,
                                              const SCH_REFERENCE& item2 )
{
    int ii = StrNumCmp( item1.GetRef(), item2.GetRef(), false );

    if( ii == 0 )
        ii = item1.m_unit - item2.m_unit;

    if( ii == 0 )
        return item1.m_symbolUuid < item2.m_symbolUuid;     // ensure a deterministic sort
    else
        return ii < 0;
}


bool SCH_REFERENCE_LIST::sortByTimeStamp( const SCH_REFERENCE& item1,
                                          const SCH_REFERENCE& item2 )
{
    int ii = item1.m_sheetPath.Cmp( item2.m_sheetPath );

    if( ii == 0 )
        return item1.m_symbolUuid < item2.m_symbolUuid;     // ensure a deterministic sort
    else
        return ii < 0;
}


bool SCH_REFERENCE_LIST::sortBySymbolPtr( const SCH_REFERENCE& item1, const SCH_REFERENCE& item2 )
{
    return item1.m_rootSymbol < item2.m_rootSymbol;
}


int SCH_REFERENCE_LIST::FindRefByFullPath( const wxString& aFullPath ) const
{
    for( size_t i = 0; i < m_flatList.size(); ++i )
    {
        if( m_flatList[i].GetFullPath() == aFullPath )
            return i;
    }

    return -1;
}


int SCH_REFERENCE_LIST::FindRef( const wxString& aRef ) const
{
    for( size_t i = 0; i < m_flatList.size(); ++i )
    {
        if( m_flatList[i].GetComposedRef() == aRef )
            return i;
    }

    return -1;
}


void SCH_REFERENCE_LIST::GetRefsInUse( int aIndex, std::vector< int >& aIdList,
                                       int aMinRefId ) const
{
    aIdList.clear();

    for( const SCH_REFERENCE& ref : m_flatList )
    {
        // Don't add new references to the list as we will reannotate those
        if( m_flatList[aIndex].CompareRef( ref ) == 0 && ref.m_numRef >= aMinRefId && !ref.m_isNew )
            aIdList.push_back( ref.m_numRef );
    }

    std::sort( aIdList.begin(), aIdList.end() );

    // Ensure each reference number appears only once.  If there are symbols with
    // multiple parts per package the same number will be stored for each part.
    alg::remove_duplicates( aIdList );
}


std::vector<int> SCH_REFERENCE_LIST::GetPackageUnits( const SCH_REFERENCE& aRef ) const
{
    std::vector<int> unitsList;

    // Always add this reference to the list
    unitsList.push_back( aRef.m_unit );

    for( const SCH_REFERENCE& ref : m_flatList )
    {
        if( ref.CompareValue( aRef ) != 0 )
            continue;

        if( ref.CompareLibId( aRef ) != 0 )
            continue;

        unitsList.push_back( ref.m_unit );
    }

    std::sort( unitsList.begin(), unitsList.end() );

    // Ensure each reference number appears only once.  If there are symbols with
    // multiple parts per package the same number will be stored for each part.
    alg::remove_duplicates( unitsList );

    return unitsList;
}


static bool sharesReusedBase( const SCH_REFERENCE& aRef )
{
    const SCHEMATIC* schematic = aRef.GetSymbol()->Schematic();
    return !schematic || !schematic->CopiesSelectDifferentUnits( aRef.GetSymbol() );
}


// A copy that keeps its own base can sit in another package than its siblings, so it keys apart
static std::pair<const SCH_SYMBOL*, KIID_PATH> packageMemberKey( const SCH_REFERENCE& aRef )
{
    return { aRef.GetSymbol(), sharesReusedBase( aRef ) ? KIID_PATH() : aRef.GetSheetPath().Path() };
}


void SCH_REFERENCE_LIST::UseReusedSheetRepresentatives( const SCH_SHEET_LIST& aSheets )
{
    std::map<const SCH_SCREEN*, const SCH_SHEET_PATH*> firstPath;

    for( const SCH_SHEET_PATH& path : aSheets )
        firstPath.emplace( path.LastScreen(), &path );

    SCH_REFERENCE_LIST              representatives;
    std::unordered_set<SCH_SYMBOL*> seen;

    for( const SCH_REFERENCE& ref : m_flatList )
    {
        // Copies that select different units each annotate on their own path
        if( !sharesReusedBase( ref ) )
        {
            representatives.AddItem( ref );
            continue;
        }

        if( !seen.insert( ref.GetSymbol() ).second )
            continue;

        auto it = firstPath.find( ref.GetSheetPath().LastScreen() );

        if( it == firstPath.end() || *it->second == ref.GetSheetPath() )
            representatives.AddItem( ref );
        else
            it->second->AppendSymbol( representatives, ref.GetSymbol(), SYMBOL_FILTER_ALL, true );
    }

    m_flatList = std::move( representatives.m_flatList );
}


SCH_MULTI_UNIT_REFERENCE_MAP
SCH_REFERENCE_LIST::LockedForRepresentatives( const SCH_MULTI_UNIT_REFERENCE_MAP& aLocked ) const
{
    std::map<std::pair<const SCH_SYMBOL*, KIID_PATH>, size_t> indexOf;
    std::vector<size_t>                                       parent( m_flatList.size() );
    std::set<size_t>                                          locked;

    for( size_t i = 0; i < m_flatList.size(); i++ )
    {
        indexOf[packageMemberKey( m_flatList[i] )] = i;
        parent[i] = i;
    }

    auto root =
            [&]( size_t aIndex )
            {
                while( parent[aIndex] != aIndex )
                    aIndex = parent[aIndex] = parent[parent[aIndex]];

                return aIndex;
            };

    for( const auto& [key, group] : aLocked )
    {
        std::optional<size_t> first;

        for( const SCH_REFERENCE& ref : group )
        {
            auto it = indexOf.find( packageMemberKey( ref ) );

            if( it == indexOf.end() )
                continue;

            locked.insert( it->second );

            if( first )
                parent[root( it->second )] = root( *first );
            else
                first = it->second;
        }
    }

    SCH_MULTI_UNIT_REFERENCE_MAP result;

    // The root index keeps two packages apart even when a duplicate gave them one composed ref
    for( size_t index : locked )
    {
        const size_t group = root( index );
        result[wxString::Format( wxS( "%s:%zu" ), m_flatList[group].GetComposedRef(), group )].AddItem(
                m_flatList[index] );
    }

    return result;
}


void SCH_REFERENCE_LIST::RenumberBasesUsedBy( const SCH_REFERENCE_LIST&           aOthers,
                                              const SCH_MULTI_UNIT_REFERENCE_MAP& aLocked,
                                              const SCH_SHEET_LIST&               aSheets )
{
    std::map<std::pair<const SCH_SYMBOL*, KIID_PATH>, wxString> packageOf;
    std::map<wxString, std::set<int>>                           packageUnits;
    std::set<wxString>                                          renumberedPackages;

    for( const auto& [key, group] : aLocked )
    {
        for( const SCH_REFERENCE& ref : group )
        {
            packageOf[packageMemberKey( ref )] = key;
            packageUnits[key].insert( ref.GetUnit() );

            // Sharing would move all but one base onto destinations nobody checked
            if( ref.GetRef() != group[0].GetRef() )
                renumberedPackages.insert( key );
        }
    }

    auto packageFor =
            [&]( const SCH_REFERENCE& aRef ) -> const wxString*
            {
                auto it = packageOf.find( packageMemberKey( aRef ) );
                return it == packageOf.end() ? nullptr : &it->second;
            };

    // Another part's unit can only join a package where the package leaves that unit free
    auto joins =
            [&]( const SCH_REFERENCE& aRef, const SCH_REFERENCE& aOther )
            {
                if( aOther.CompareLibId( aRef ) != 0 || aOther.CompareValue( aRef ) != 0 )
                    return false;

                if( const wxString* package = packageFor( aRef ) )
                    return !packageUnits[*package].contains( aOther.GetUnit() );

                return aOther.GetUnit() != aRef.GetUnit();
            };

    auto samePackage =
            [&]( const SCH_REFERENCE& aRef, const SCH_REFERENCE& aOther )
            {
                const wxString* ref = packageFor( aRef );
                const wxString* other = packageFor( aOther );

                if( ref && other )
                    return *ref == *other;

                return aOther.GetUnit() != aRef.GetUnit() && aOther.CompareLibId( aRef ) == 0
                       && aOther.CompareValue( aRef ) == 0;
            };

    std::multimap<wxString, const SCH_REFERENCE*>                   othersByBase;
    std::multimap<wxString, const SCH_REFERENCE*>                   othersByComposed;
    std::map<const SCH_SCREEN*, std::vector<const SCH_SHEET_PATH*>> pathsOf;

    for( const SCH_REFERENCE& other : aOthers.m_flatList )
    {
        othersByBase.emplace( other.GetRef(), &other );
        othersByComposed.emplace( other.GetComposedRef(), &other );
    }

    for( const SCH_SHEET_PATH& path : aSheets )
        pathsOf[path.LastScreen()].push_back( &path );

    // A shared base shows on every copy, while a copy keeping its own shows only there
    // A multi-unit symbol shows its package's suffix, which can be another sheet's
    auto suffixesFor =
            [&]( const SCH_REFERENCE& aRef ) -> std::vector<wxString>
            {
                const SCH_SYMBOL*     symbol = aRef.GetSymbol();
                const SCHEMATIC*      schematic = symbol->Schematic();
                std::vector<wxString> result;

                auto add =
                        [&]( const SCH_SHEET_PATH& aPath )
                        {
                            if( !schematic )
                                result.emplace_back();
                            else
                                result.push_back( schematic->GetPackageRefSuffix( symbol, aPath ) );
                        };

                if( !sharesReusedBase( aRef ) )
                {
                    add( aRef.GetSheetPath() );
                    return result;
                }

                for( const SCH_SHEET_PATH* path : pathsOf[aRef.GetSheetPath().LastScreen()] )
                    add( *path );

                return result;
            };

    auto collides =
            [&]( const SCH_REFERENCE& aRef, const std::vector<wxString>& aSuffixes )
            {
                auto blocks =
                        [&]( const std::multimap<wxString, const SCH_REFERENCE*>& aIndex, const wxString& aKey )
                        {
                            auto [begin, end] = aIndex.equal_range( aKey );

                            return std::any_of( begin, end,
                                                [&]( const auto& aEntry )
                                                {
                                                    return !joins( aRef, *aEntry.second );
                                                } );
                        };

                if( blocks( othersByBase, aRef.GetRef() ) )
                    return true;

                // The suffix need not separate cleanly from the base, so compare what each instance will show
                for( const wxString& suffix : aSuffixes )
                {
                    if( blocks( othersByComposed, aRef.GetRef() + suffix ) )
                        return true;
                }

                return false;
            };

    std::set<const SCH_REFERENCE*>                        renumbered;
    std::map<wxString, std::vector<const SCH_REFERENCE*>> holders;
    std::map<wxString, std::vector<const SCH_REFERENCE*>> composedHolders;

    for( const SCH_REFERENCE& ref : m_flatList )
    {
        const std::vector<wxString> suffixes = suffixesFor( ref );

        auto clashes =
                [&]( const std::vector<const SCH_REFERENCE*>& aHolders )
                {
                    return !std::all_of( aHolders.begin(), aHolders.end(),
                                         [&]( const SCH_REFERENCE* aHolder )
                                         {
                                             return samePackage( ref, *aHolder );
                                         } );
                };

        bool conflict = collides( ref, suffixes ) || clashes( holders[ref.GetRef()] )
                        || std::any_of( suffixes.begin(), suffixes.end(),
                                        [&]( const wxString& aSuffix )
                                        {
                                            return clashes( composedHolders[ref.GetRef() + aSuffix] );
                                        } );

        if( !conflict )
        {
            holders[ref.GetRef()].push_back( &ref );

            for( const wxString& suffix : suffixes )
                composedHolders[ref.GetRef() + suffix].push_back( &ref );
        }
        else if( const wxString* package = packageFor( ref ) )
            renumberedPackages.insert( *package );
        else
            renumbered.insert( &ref );
    }

    for( SCH_REFERENCE& ref : m_flatList )
    {
        const wxString* package = packageFor( ref );

        if( renumbered.contains( &ref ) || ( package && renumberedPackages.contains( *package ) ) )
            ref.m_isNew = true;
    }
}


int SCH_REFERENCE_LIST::FindFirstUnusedReference(
        const SCH_REFERENCE& aRef, int aMinValue, const std::vector<int>& aRequiredUnits,
        const std::map<const SCH_REFERENCE*, std::vector<SCH_REFERENCE>>& aDestinations,
        const SCH_REFERENCE_LIST*                                         aLocked ) const
{
    // Create a map of references indexed by reference number, only including those with the same
    // reference prefix as aRef
    std::map<int, std::vector<SCH_REFERENCE>> refNumberMap;

    // Without required units the tracker only asks whether a number is taken, so skip the copies
    const bool needUnits = std::ranges::any_of( aRequiredUnits, []( int aUnit ) { return aUnit >= 0; } );

    for( const SCH_REFERENCE& ref : m_flatList )
    {
        // search only for the current reference prefix:
        if( ref.CompareRef( aRef ) != 0 )
            continue;

        if( ref.m_isNew )
            continue; // It will be reannotated

        std::vector<SCH_REFERENCE>& numbered = refNumberMap[ref.m_numRef];

        if( needUnits )
            numbered.push_back( ref );
    }

    std::function<bool( int )> acceptNumber;

    if( !aDestinations.empty() )
    {
        std::multimap<wxString, SCH_REFERENCE> occupied;
        std::vector<SCH_REFERENCE>             candidates;

        for( const auto& [ref, destinations] : aDestinations )
        {
            if( ref == &aRef
                || ( aLocked && ref->CompareRef( aRef ) == 0 && ref->CompareLibId( aRef ) == 0
                     && ref->CompareValue( aRef ) == 0 && aLocked->Contains( *ref ) ) )
            {
                candidates.insert( candidates.end(), destinations.begin(), destinations.end() );
            }

            if( ref->m_isNew || ref->m_numRef < 0 )
                continue;

            for( SCH_REFERENCE destination : destinations )
            {
                destination.SetUnit( ref->GetUnit() );

                // Symbols still hold the old references until allocation finishes
                wxString composed = ref->GetRef() + ref->GetRefNumber() + destination.m_refSuffix;
                occupied.emplace( composed, std::move( destination ) );
            }
        }

        acceptNumber = [&aRef, occupied = std::move( occupied ), candidates = std::move( candidates )]( int aNumber )
        {
            wxString base = aRef.GetRef() + aRef.formatRefStr( aNumber );

            for( const SCH_REFERENCE& candidate : candidates )
            {
                auto [begin, end] = occupied.equal_range( base + candidate.m_refSuffix );

                for( auto it = begin; it != end; ++it )
                {
                    const SCH_REFERENCE& other = it->second;

                    if( other.GetUnit() == candidate.GetUnit() || other.CompareLibId( candidate ) != 0
                        || other.CompareValue( candidate ) != 0 )
                    {
                        return false;
                    }
                }
            }

            return true;
        };
    }

    return m_refDesTracker->GetNextRefDesForUnits( aRef, refNumberMap, aRequiredUnits, aMinValue, acceptNumber );
}


std::vector<SCH_SYMBOL_INSTANCE> SCH_REFERENCE_LIST::GetSymbolInstances() const
{
    std::vector<SCH_SYMBOL_INSTANCE> retval;

    for( const SCH_REFERENCE& ref : m_flatList )
    {
        SCH_SYMBOL_INSTANCE instance;
        instance.m_Path = ref.GetSheetPath().Path();
        instance.m_Reference = ref.GetRef();
        instance.m_Unit = ref.GetUnit();

        retval.push_back( instance );
    }

    return retval;
}


// A helper function to build a full reference string of a SCH_REFERENCE item
wxString buildFullReference( const SCH_REFERENCE& aItem, int aUnitNumber = -1 )
{
    wxString fullref;
    fullref = aItem.GetRef() + aItem.GetRefNumber();

    if( aUnitNumber < 0 )
        fullref << ".." << aItem.GetUnit();
    else
        fullref << ".." << aUnitNumber;

    return fullref;
}


void SCH_REFERENCE_LIST::ReannotateByOptions( ANNOTATE_ORDER_T             aSortOption,
                                              ANNOTATE_ALGO_T              aAlgoOption,
                                              int                          aStartNumber,
                                              const SCH_REFERENCE_LIST&    aAdditionalRefs,
                                              bool                         aStartAtCurrent,
                                              SCH_SHEET_LIST*              aHierarchy )
{
    SplitReferences();

    // All multi-unit symbols always locked to ensure consistent re-annotation
    SCH_MULTI_UNIT_REFERENCE_MAP lockedSymbols;

    for( size_t i = 0; i < GetCount(); i++ )
    {
        SCH_REFERENCE& ref = m_flatList[i];
        wxString       refstr = ref.GetSymbol()->GetRef( &ref.GetSheetPath() );

        // Update sheet numbers based on the reference's sheet's position within the full
        // hierarchy; we do this now before we annotate so annotation by sheet number * X
        // works correctly.
        if( aHierarchy )
        {
            SCH_SHEET_PATH* path = aHierarchy->FindSheetForPath( &ref.GetSheetPath() );
            wxCHECK2_MSG( path, continue,
                          wxS( "Attempting to annotate item on sheet not part of the "
                               "hierarchy?" ) );

            ref.SetSheetNumber( path->GetPageNumberAsInt() );
        }

        // Never lock unassigned references
        if( refstr[refstr.Len() - 1] == '?' )
            continue;

        ref.m_isNew = true; // We want to reannotate all references

        lockedSymbols[refstr].AddItem( ref );
    }

    AnnotateByOptions( aSortOption, aAlgoOption, aStartNumber, lockedSymbols, aAdditionalRefs,
                       aStartAtCurrent );
}


void SCH_REFERENCE_LIST::ReannotateDuplicates( const SCH_REFERENCE_LIST& aAdditionalReferences, ANNOTATE_ALGO_T aAlgoOption )
{
    ReannotateByOptions( UNSORTED, aAlgoOption, 0, aAdditionalReferences, true, nullptr );
}


void SCH_REFERENCE_LIST::AnnotateByOptions( ANNOTATE_ORDER_T aSortOption, ANNOTATE_ALGO_T aAlgoOption, int aStartNumber,
                                            const SCH_MULTI_UNIT_REFERENCE_MAP& aLockedUnitMap,
                                            const SCH_REFERENCE_LIST& aAdditionalRefs, bool aStartAtCurrent,
                                            const SCH_SHEET_LIST* aSharedSheets )
{
    switch( aSortOption )
    {
    default:
    case SORT_BY_X_POSITION: SortByXCoordinate(); break;
    case SORT_BY_Y_POSITION: SortByYCoordinate(); break;
    }

    bool useSheetNum;
    int  idStep;

    switch( aAlgoOption )
    {
    default:
    case INCREMENTAL_BY_REF:
        useSheetNum = false;
        idStep = 1;
        break;

    case SHEET_NUMBER_X_100:
        useSheetNum = true;
        idStep = 100;
        aStartAtCurrent = false; // Not implemented for sheet # * 100
        break;

    case SHEET_NUMBER_X_1000:
        useSheetNum = true;
        idStep = 1000;
        aStartAtCurrent = false; // Not implemented for sheet # * 1000
        break;
    }

    Annotate( useSheetNum, idStep, aStartNumber, aLockedUnitMap, aAdditionalRefs, aStartAtCurrent, aSharedSheets );
}


void SCH_REFERENCE_LIST::Annotate( bool aUseSheetNum, int aSheetIntervalId, int aStartNumber,
                                   const SCH_MULTI_UNIT_REFERENCE_MAP& aLockedUnitMap,
                                   const SCH_REFERENCE_LIST& aAdditionalRefs, bool aStartAtCurrent,
                                   const SCH_SHEET_LIST* aSharedSheets )
{
    if( !m_refDesTracker )
    {
        wxLogError( wxS( "No reference tracker set for SCH_REFERENCE_LIST::Annotate()" ) );
        return;
    }

    if ( m_flatList.size() == 0 )
        return;

    size_t originalSize = GetCount();

    // For multi units symbols, store the list of already used full references.
    // The algorithm tries to allocate the new reference to symbols having the same
    // old reference.
    // This algo works fine as long as the previous annotation has no duplicates.
    // But when a hierarchy is reannotated with this option, the previous annotation can
    // have duplicate references, and obviously we must fix these duplicate.
    // therefore do not try to allocate a full reference more than once when trying
    // to keep this order of multi units.
    // inUseRefs keep trace of previously allocated references
    std::unordered_set<wxString> inUseRefs;

    for( size_t i = 0; i < aAdditionalRefs.GetCount(); i++ )
    {
        SCH_REFERENCE additionalRef = aAdditionalRefs[i];
        additionalRef.Split();

        // Add the additional reference to the multi-unit set if annotated
        if( !additionalRef.m_isNew )
            inUseRefs.insert( buildFullReference( additionalRef ) );

        // We don't want to reannotate the additional references even if not annotated
        // so we change the m_isNew flag to be false after splitting
        additionalRef.m_isNew = false;
        AddItem( additionalRef ); //add to this container
    }

    std::map<const SCH_REFERENCE*, std::vector<SCH_REFERENCE>> destinations;

    if( aSharedSheets )
    {
        for( size_t i = 0; i < m_flatList.size(); ++i )
        {
            const SCH_REFERENCE&        ref = m_flatList[i];
            std::vector<SCH_REFERENCE>& instances = destinations[&ref];

            if( i < originalSize && sharesReusedBase( ref ) )
            {
                for( const SCH_SHEET_PATH& path : *aSharedSheets )
                {
                    if( path.LastScreen() == ref.GetSheetPath().LastScreen() )
                        instances.emplace_back( ref.GetSymbol(), path );
                }
            }
            else
            {
                instances.push_back( ref );
            }
        }
    }

    int LastReferenceNumber = 0;

    /* calculate index of the first symbol with the same reference prefix
     * than the current symbol.  All symbols having the same reference
     * prefix will receive a reference number with consecutive values:
     * IC .. will be set to IC4, IC4, IC5 ...
     */
    unsigned first = 0;

    // calculate the last used number for this reference prefix:
    int minRefId;

    // when using sheet number, ensure ref number >= sheet number* aSheetIntervalId
    if( aUseSheetNum )
        minRefId = m_flatList[first].m_sheetNum * aSheetIntervalId + 1;
    else
        minRefId = aStartNumber + 1;


    for( unsigned ii = 0; ii < m_flatList.size(); ii++ )
    {
        auto& ref_unit = m_flatList[ii];

        if( ref_unit.m_flag )
            continue;

        // Check whether this symbol is in aLockedUnitMap.
        const SCH_REFERENCE_LIST* lockedList = nullptr;

        for( const SCH_MULTI_UNIT_REFERENCE_MAP::value_type& pair : aLockedUnitMap )
        {
            unsigned n_refs = pair.second.GetCount();

            for( unsigned thisRefI = 0; thisRefI < n_refs; ++thisRefI )
            {
                const SCH_REFERENCE &thisRef = pair.second[thisRefI];

                if( thisRef.IsSameInstance( ref_unit ) )
                {
                    lockedList = &pair.second;
                    break;
                }
            }

            if( lockedList != nullptr )
                break;
        }

        if(  ( m_flatList[first].CompareRef( ref_unit ) != 0 )
          || ( aUseSheetNum && ( m_flatList[first].m_sheetNum != ref_unit.m_sheetNum ) )  )
        {
            // New reference found: we need a new ref number for this reference
            first = ii;

            // when using sheet number, ensure ref number >= sheet number* aSheetIntervalId
            if( aUseSheetNum )
                minRefId = ref_unit.m_sheetNum * aSheetIntervalId + 1;
            else
                minRefId = aStartNumber + 1;
        }

        // Find references greater than current reference (unless not annotated)
        if( aStartAtCurrent && ref_unit.m_numRef > 0 )
            minRefId = ref_unit.m_numRef;

        wxCHECK( ref_unit.GetLibPart(), /* void */ );

        // Annotation of one part per package symbols (trivial case).
        if( ref_unit.GetLibPart()->GetUnitCount() <= 1 )
        {
            if( ref_unit.m_isNew )
            {
                LastReferenceNumber = FindFirstUnusedReference( ref_unit, minRefId, {}, destinations );
                ref_unit.m_numRef = LastReferenceNumber;
                ref_unit.m_numRefStr = ref_unit.formatRefStr( LastReferenceNumber );
            }

            ref_unit.m_flag  = 1;
            ref_unit.m_isNew = false;
            continue;
        }

        // If this symbol is in aLockedUnitMap, copy the annotation to all
        // symbols that are not it
        if( lockedList != nullptr )
        {
            unsigned n_refs = lockedList->GetCount();
            std::vector<int> units = lockedList->GetPackageUnits( ref_unit );

            if( ref_unit.m_isNew )
            {
                LastReferenceNumber = FindFirstUnusedReference( ref_unit, minRefId, units, destinations, lockedList );
                ref_unit.m_numRef = LastReferenceNumber;
                ref_unit.m_numRefStr = ref_unit.formatRefStr( LastReferenceNumber );
                ref_unit.m_isNew = false;
                ref_unit.m_flag = 1;
            }

            for( unsigned lockedRefI = 0; lockedRefI < n_refs; ++lockedRefI )
            {
                const SCH_REFERENCE& lockedRef = ( *lockedList )[lockedRefI];

                if( lockedRef.IsSameInstance( ref_unit ) )
                {
                    // This is the symbol we're currently annotating. Hold the unit!
                    ref_unit.m_unit = lockedRef.m_unit;

                    // lock this new full reference
                    inUseRefs.insert( buildFullReference( ref_unit ) );
                }

                if( lockedRef.CompareValue( ref_unit ) != 0 )
                    continue;

                if( lockedRef.CompareLibId( ref_unit ) != 0 )
                    continue;

                // Find the matching symbol
                for( unsigned jj = ii + 1; jj < m_flatList.size(); jj++ )
                {
                    if( !lockedRef.IsSameInstance( m_flatList[jj] ) )
                        continue;

                    // A merged reused-sheet group can mix prefixes, and the number is only free for this one
                    if( m_flatList[jj].CompareRef( ref_unit ) != 0 )
                        continue;

                    wxString ref_candidate = buildFullReference( ref_unit, lockedRef.m_unit );

                    // propagate the new reference and unit selection to the "old" symbol,
                    // if this new full reference is not already used (can happens when initial
                    // multiunits symbols have duplicate references)
                    if( inUseRefs.find( ref_candidate ) == inUseRefs.end() )
                    {
                        m_flatList[jj].m_numRef = ref_unit.m_numRef;
                        m_flatList[jj].m_numRefStr = ref_unit.m_numRefStr;
                        m_flatList[jj].m_isNew = false;
                        m_flatList[jj].m_flag = 1;

                        // lock this new full reference
                        inUseRefs.insert( ref_candidate );
                        break;
                    }
                }
            }
        }
        else if( ref_unit.m_isNew )
        {
            // Reference belonging to multi-unit symbol that has not yet been annotated. We don't
            // know what group this might belong to, so just find the first unused reference for
            // this specific unit. The other units will be annotated in the following passes.
            std::vector<int> units = { ref_unit.GetUnit() };
            LastReferenceNumber = FindFirstUnusedReference( ref_unit, minRefId, units, destinations );
            ref_unit.m_numRef = LastReferenceNumber;
            ref_unit.m_numRefStr = ref_unit.formatRefStr( LastReferenceNumber );
            ref_unit.m_isNew = false;
            ref_unit.m_flag = 1;
        }
    }

    // Remove aAdditionalRefs references
    m_flatList.erase( m_flatList.begin() + originalSize, m_flatList.end() );

    wxASSERT( originalSize == GetCount() ); // Make sure we didn't make a mistake
}


int SCH_REFERENCE_LIST::CheckAnnotation( ANNOTATION_ERROR_HANDLER aHandler )
{
    int            error = 0;
    wxString       tmp;
    wxString       tmp2;
    wxString       msg;

    SortByRefAndValue();

    // Split reference designators into name (prefix) and number: IC1 becomes IC, and 1.
    SplitReferences();

    // count not yet annotated items or annotation error.
    for( unsigned ii = 0; ii < m_flatList.size(); ii++ )
    {
        msg.Empty();
        tmp.Empty();

        LIB_SYMBOL* libSymbol = m_flatList[ii].GetLibPart();

        if( m_flatList[ii].m_isNew )    // Not yet annotated
        {
            if( m_flatList[ii].m_numRef >= 0 )
                tmp << m_flatList[ii].m_numRefStr;
            else
                tmp = wxT( "?" );

            if( ( m_flatList[ii].m_unit > 0 ) && ( m_flatList[ii].m_unit < 0x7FFFFFFF )
                && libSymbol && libSymbol->GetUnitCount() > 1 )
            {
                msg.Printf( _( "Item not annotated: %s%s (unit %d)" ),
                            m_flatList[ii].GetRef(),
                            tmp,
                            m_flatList[ii].m_unit );
            }
            else
            {
                msg.Printf( _( "Item not annotated: %s%s" ), m_flatList[ii].GetRef(), tmp );
            }

            aHandler( ERCE_UNANNOTATED, msg, &m_flatList[ii], nullptr );
            error++;
            break;
        }

        // Error if unit number selected does not exist (greater than the  number of units in
        // the symbol).  This can happen if a symbol has changed in a library after a
        // previous annotation.
        if( libSymbol && std::max( libSymbol->GetUnitCount(), 1 ) < m_flatList[ii].m_unit )
        {
            if( m_flatList[ii].m_numRef >= 0 )
                tmp << m_flatList[ii].m_numRefStr;
            else
                tmp = wxT( "?" );

            msg.Printf( _( "Error: symbol %s%s%s (unit %d) exceeds units defined (%d)" ),
                        m_flatList[ii].GetRef(),
                        tmp,
                        m_flatList[ii].GetSymbol()->SubReference( m_flatList[ii].GetUnit() ),
                        m_flatList[ii].m_unit,
                        libSymbol->GetUnitCount() );

            aHandler( ERCE_EXTRA_UNITS, msg, &m_flatList[ii], nullptr );
            error++;
            break;
        }
    }

    auto sameBaseAndSuffix = []( const SCH_REFERENCE& aFirst, const SCH_REFERENCE& aSecond )
    {
        return aFirst.CompareRef( aSecond ) == 0 && aFirst.m_numRef == aSecond.m_numRef
               && aFirst.GetRefSuffix() == aSecond.GetRefSuffix();
    };

    auto checkPackage = [&]( SCH_REFERENCE& first, SCH_REFERENCE& second )
    {
        msg.Empty();
        tmp.Empty();
        tmp2.Empty();

        // Same reference found. If same unit, error!
        if( first.m_unit == second.m_unit )
        {
            if( first.m_numRef >= 0 )
                tmp << first.m_numRefStr;
            else
                tmp = wxT( "?" );

            msg.Printf( _( "Duplicate items %s%s%s%s\n" ),
                        first.GetRef(),
                        tmp,
                        first.GetLibPart() && first.GetLibPart()->GetUnitCount() > 1
                                ? first.GetSymbol()->SubReference( first.GetUnit() )
                                : wxString( wxT( "" ) ),
                        first.GetRefSuffix() );

            aHandler( ERCE_DUPLICATE_REFERENCE, msg, &first, &second );
            error++;
            return;
        }

        /* Test error if units are different but number of parts per package
         * too high (ex U3 ( 1 part) and we find U3B this is an error) */
        if( first.GetLibPart() && second.GetLibPart()
            && first.GetLibPart()->GetUnitCount() != second.GetLibPart()->GetUnitCount() )
        {
            if( first.m_numRef >= 0 )
                tmp << first.m_numRefStr;
            else
                tmp = wxT( "?" );

            if( second.m_numRef >= 0 )
                tmp2 << second.m_numRefStr;
            else
                tmp2 = wxT( "?" );

            msg.Printf( _( "Differing unit counts for item %s%s%s%s and %s%s%s%s\n" ),
                        first.GetRef(),
                        tmp,
                        first.GetSymbol()->SubReference( first.GetUnit() ),
                        first.GetRefSuffix(),
                        second.GetRef(),
                        tmp2,
                        first.GetSymbol()->SubReference( second.GetUnit() ),
                        second.GetRefSuffix() );

            aHandler( ERCE_DUPLICATE_REFERENCE, msg, &first, &second );
            error++;
            return;
        }

        // Error if values are different between units, for the same reference
        if( first.CompareValue( second ) != 0 )
        {
            msg.Printf( _( "Different values for %s%d%s (%s) and %s%d%s (%s)" ),
                        first.GetRef(),
                        first.m_numRef,
                        first.GetSymbol()->SubReference( first.GetUnit() ) + first.GetRefSuffix(),
                        first.m_value,
                        second.GetRef(),
                        second.m_numRef,
                        first.GetSymbol()->SubReference( second.GetUnit() ) + second.GetRefSuffix(),
                        second.m_value );

            aHandler( ERCE_DIFFERENT_UNIT_VALUE, msg, &first, &second );
            error++;
        }
    };

    // count the duplicated elements (if all are annotated)
    for( size_t ii = 1; ii < m_flatList.size(); ++ii )
    {
        if( sameBaseAndSuffix( m_flatList[ii - 1], m_flatList[ii] ) )
            checkPackage( m_flatList[ii - 1], m_flatList[ii] );
    }

    // Different base and suffix splits can identify the same complete package reference
    std::map<wxString, std::map<int, SCH_REFERENCE*>> composed;

    for( SCH_REFERENCE& ref : m_flatList )
    {
        if( ref.m_isNew )
            continue;

        auto& units = composed[ref.GetRef() + ref.m_numRefStr + ref.GetRefSuffix()];
        auto  first = units.find( ref.m_unit );

        if( first == units.end() )
            first = units.begin();

        // The loop above already checked units with the same base and suffix
        if( first != units.end() && !sameBaseAndSuffix( *first->second, ref ) )
            checkPackage( *first->second, ref );

        units.emplace( ref.m_unit, &ref );
    }

    return error;
}


SCH_REFERENCE::SCH_REFERENCE( SCH_SYMBOL* aSymbol, const SCH_SHEET_PATH& aSheetPath )
{
    wxASSERT( aSymbol != nullptr );

    m_rootSymbol = aSymbol;

    // Ensure the symbol has instance data for the current sheet path so that the unit selection
    // remains consistent even when loading a sheet without symbol instance records (for example
    // when editing a subsheet directly).
    SCH_SYMBOL_INSTANCE instance;

    if( !aSymbol->GetInstance( instance, aSheetPath.Path(), false ) )
    {
        instance.m_Path = aSheetPath.Path();
        instance.m_Reference = aSymbol->GetBaseRef( &aSheetPath );

        if( instance.m_Reference.IsEmpty() )
            instance.m_Reference = aSymbol->GetField( FIELD_T::REFERENCE )->GetText();

        instance.m_Unit = aSymbol->GetUnit();
        aSymbol->AddHierarchicalReference( instance );
    }

    m_unit       = aSymbol->GetUnitSelection( &aSheetPath );
    m_footprint  = aSymbol->GetFootprintFieldText( &aSheetPath, RESOLVED );
    m_sheetPath  = aSheetPath;
    m_isNew      = false;
    m_flag       = 0;
    m_symbolUuid = aSymbol->m_Uuid;
    m_symbolPos  = aSymbol->GetPosition();
    m_sheetNum   = 0;

    if( aSymbol->GetBaseRef( &aSheetPath ).IsEmpty() )
        aSymbol->SetRef( &aSheetPath, wxT( "DefRef?" ) );

    wxString ref = aSymbol->GetBaseRef( &aSheetPath );
    SetRef( ref );

    // A unit of a package spanning sheets composes with the package's suffix, which keeps its units together
    if( const SCHEMATIC* schematic = aSymbol->Schematic() )
        m_refSuffix = schematic->GetPackageRefSuffix( aSymbol, aSheetPath );

    m_numRef = -1;

    // Unresolved so building a reference list never re-enters text resolution
    wxString value = aSymbol->GetValue( &aSheetPath, RAW_VALUE );

    if( value.IsEmpty() )
        value = wxT( "~" );

    m_value = std::move( value );
}


wxString SCH_REFERENCE::GetFullRef() const
{
    wxString ref = GetRef();

    if( IsSplit() )
    {
        if( !m_numRefStr.IsEmpty() )
            ref += m_numRefStr;
        else
            ref << m_numRef;
    }

    if( GetSymbol()->GetUnitCount() > 1 )
        ref += GetSymbol()->SubReference( GetUnit() );

    // A unit shows its own sheet's suffix even when its package composes with another
    if( const SCHEMATIC* schematic = m_rootSymbol->Schematic(); schematic && hasRef() )
        schematic->AppendRefSuffix( ref, m_sheetPath );

    return ref;
}


void SCH_REFERENCE::Annotate()
{
    if( m_numRef < 0 )
        m_ref += '?';
    else
        m_ref = TO_UTF8( GetRef() << GetRefNumber() );

    m_rootSymbol->SetRef( &m_sheetPath, From_UTF8( m_ref.c_str() ) );
    m_rootSymbol->SetUnit( m_unit );
    m_rootSymbol->SetUnitSelection( &m_sheetPath, m_unit );
}


bool SCH_REFERENCE::AlwaysAnnotate() const
{
    wxCHECK( m_rootSymbol && m_rootSymbol->GetLibSymbolRef()
          && !m_rootSymbol->GetRef( &m_sheetPath ).IsEmpty(), false );

    return m_rootSymbol->GetLibSymbolRef()->IsPower()
        || m_rootSymbol->GetRef( &m_sheetPath )[0] == wxUniChar( '#' );
}


void SCH_REFERENCE::Split()
{
    std::string refText = GetRefStr();

    m_numRef = -1;
    m_numRefStr.Clear();

    int ll = refText.length() - 1;

    if( refText[ll] == '?' )
    {
        m_isNew = true;

        refText.erase( ll );  // delete last char

        SetRefStr( refText );
    }
    else if( isdigit( refText[ll] ) == 0 )
    {
        m_isNew = true;
    }
    else
    {
        while( ll >= 0 )
        {
            if( (refText[ll] <= ' ' ) || isdigit( refText[ll] ) )
            {
                ll--;
            }
            else
            {
                if( isdigit( refText[ll + 1] ) )
                {
                    // null terminated C string into cp
                    const char* cp = refText.c_str() + ll + 1;

                    m_numRef = atoi( cp );
                }

                m_numRefStr = std::string( refText, ll + 1 );
                refText.erase( ll + 1 );
                break;
            }
        }

        SetRefStr( refText );
    }
}


bool SCH_REFERENCE::IsSplitNeeded() const
{
    if( m_ref.IsEmpty() )
        return false;

    const wxUniChar last = m_ref.Last();

    return last == '?' || ( last >= '0' && last <= '9' );
}


bool SCH_REFERENCE::GetSymbolDNP( const wxString& aVariant ) const
{
    wxCHECK( m_rootSymbol, false );

    return m_rootSymbol->GetDNP( &m_sheetPath, aVariant );
}


bool SCH_REFERENCE::GetSymbolExcludedFromBOM( const wxString& aVariant ) const
{
    wxCHECK( m_rootSymbol, false );

    return m_rootSymbol->GetExcludedFromBOM( &m_sheetPath, aVariant );
}


bool SCH_REFERENCE::GetSymbolExcludedFromSim( const wxString& aVariant ) const
{
    wxCHECK( m_rootSymbol, false );

    return m_rootSymbol->GetExcludedFromSim( &m_sheetPath, aVariant );
}


bool SCH_REFERENCE::GetSymbolExcludedFromBoard() const
{
    wxCHECK( m_rootSymbol, false );

    return m_rootSymbol->GetExcludedFromBoard();
}


bool SCH_REFERENCE::GetSymbolExcludedFromPosFiles( const wxString& aVariant ) const
{
    wxCHECK( m_rootSymbol, false );

    return m_rootSymbol->GetExcludedFromPosFiles( &m_sheetPath, aVariant );
}


wxString SCH_REFERENCE::formatRefStr( int aNumber ) const
{
    // To avoid a risk of duplicate, for power symbols the ref number is 0nnn instead of nnn.
    // Just because sometimes only power symbols are annotated
    if( GetSymbol() && GetLibPart() && GetLibPart()->IsPower() )
        return wxString::Format( "0%d", aNumber );

    return wxString::Format( "%d", aNumber );
}


void SCH_REFERENCE::SetSymbolDNP( bool aEnable, const wxString& aVariant )
{
    wxCHECK( m_rootSymbol, /* void */ );

    m_rootSymbol->SetDNP( aEnable, &m_sheetPath, aVariant );
}


void SCH_REFERENCE::SetSymbolExcludedFromBOM( bool aEnable, const wxString& aVariant )
{
    wxCHECK( m_rootSymbol, /* void */ );

    m_rootSymbol->SetExcludedFromBOM( aEnable, &m_sheetPath, aVariant );
}


void SCH_REFERENCE::SetSymbolExcludedFromSim( bool aEnable, const wxString& aVariant )
{
    wxCHECK( m_rootSymbol, /* void */ );

    m_rootSymbol->SetExcludedFromSim( aEnable, &m_sheetPath, aVariant );
}


void SCH_REFERENCE::SetSymbolExcludedFromBoard( bool aEnable )
{
    wxCHECK( m_rootSymbol, /* void */ );

    m_rootSymbol->SetExcludedFromBoard( aEnable );
}


void SCH_REFERENCE::SetSymbolExcludedFromPosFiles( bool aEnable, const wxString& aVariant )
{
    wxCHECK( m_rootSymbol, /* void */ );

    m_rootSymbol->SetExcludedFromPosFiles( aEnable, &m_sheetPath, aVariant );
}


#if defined( DEBUG )
void SCH_REFERENCE_LIST::Show( const char* aPrefix )
{
    printf( "%s\n", aPrefix );

    for( unsigned i = 0; i < m_flatList.size(); ++i )
    {
        SCH_REFERENCE& schref = m_flatList[i];

        printf( " [%-2d] ref:%-8s num:%-3d lib_part:%s\n", i, schref.m_ref.ToStdString().c_str(),
                schref.m_numRef, TO_UTF8( schref.GetLibPart()->GetName() ) );
    }
}
#endif


void AnnotateNewSymbol( SCH_SYMBOL* aSymbol, const std::vector<SCH_SHEET_PATH>& aPaths,
                        SCH_REFERENCE_LIST& aExistingRefs, const SCHEMATIC_SETTINGS& aSettings,
                        SCH_SHEET_LIST* aHierarchy )
{
    std::optional<wxString>         sharedBase;
    std::vector<wxString>           pathSuffixes;
    std::multimap<wxString, size_t> composedInUse;
    bool                            share = aSettings.m_AnnotateReusedSheetSuffixes;

    for( const SCH_SHEET_PATH& path : aPaths )
    {
        wxString suffix = path.ComposeRefSuffix();

        // Paths that compose alike cannot share a base, which full annotation refuses outright
        if( std::ranges::find( pathSuffixes, suffix ) != pathSuffixes.end() )
            share = false;

        pathSuffixes.push_back( std::move( suffix ) );
    }

    if( share )
    {
        for( size_t ii = 0; ii < aExistingRefs.GetCount(); ii++ )
        {
            if( aExistingRefs[ii].GetSymbol() != aSymbol )
                composedInUse.emplace( aExistingRefs[ii].GetComposedRef(), ii );
        }
    }

    // A free base can compose to a reference held elsewhere, as R1 on _M2 meets a root R1_M2
    // A free unit of the same part is no collision, since the allocator shares those on purpose
    auto composesTaken =
            [&]( const wxString& aBase, int aUnit )
            {
                for( size_t ii = 0; ii < aPaths.size(); ii++ )
                {
                    // Values can resolve per path, so each destination compares its own
                    SCH_REFERENCE newRef( aSymbol, aPaths[ii] );
                    newRef.SetUnit( aUnit );

                    auto [begin, end] = composedInUse.equal_range( aBase + pathSuffixes[ii] );

                    for( auto it = begin; it != end; ++it )
                    {
                        const SCH_REFERENCE& other = aExistingRefs[it->second];

                        if( other.GetUnit() == aUnit || other.CompareLibId( newRef ) != 0
                                || other.CompareValue( newRef ) != 0 )
                        {
                            return true;
                        }
                    }
                }

                return false;
            };

    for( const SCH_SHEET_PATH& path : aPaths )
    {
        if( sharedBase )
        {
            aSymbol->SetRef( &path, *sharedBase );
            aExistingRefs.AddItem( SCH_REFERENCE( aSymbol, path ) );
            continue;
        }

        std::optional<SCH_REFERENCE_LIST> reserved;
        SCH_REFERENCE_LIST                refs;
        std::set<wxString>                tried;

        for( ;; )
        {
            refs = SCH_REFERENCE_LIST();
            refs.AddItem( SCH_REFERENCE( aSymbol, path ) );
            refs.SetRefDesTracker( aSettings.m_refDesTracker );
            refs.ReannotateByOptions( (ANNOTATE_ORDER_T) aSettings.m_AnnotateSortOrder,
                                      (ANNOTATE_ALGO_T) aSettings.m_AnnotateMethod, aSettings.m_AnnotateStartNum,
                                      reserved ? *reserved : aExistingRefs, false, aHierarchy );
            refs.UpdateAnnotation();

            // Power symbols keep a number per sheet, as annotation never shares them
            if( !share || refs[0].AlwaysAnnotate() )
                break;

            sharedBase = aSymbol->GetBaseRef( &path );

            // A base offered twice means the reservation did not hold, so stop rather than spin
            if( !tried.insert( *sharedBase ).second || !composesTaken( *sharedBase, refs[0].GetUnit() ) )
                break;

            // Copy only when a retry must reserve the base it rejected
            if( !reserved )
                reserved = aExistingRefs;

            reserved->AddItem( refs[0] );
        }

        for( size_t ii = 0; ii < refs.GetCount(); ii++ )
            aExistingRefs.AddItem( refs[ii] );
    }
}


std::optional<std::pair<SCH_SHEET_PATH, SCH_SHEET_PATH>> FindRefSuffixCollision( const SCH_REFERENCE_LIST& aRefs,
                                                                                 const SCHEMATIC&          aSchematic )
{
    if( !aSchematic.Settings().m_AnnotateReusedSheetSuffixes )
        return std::nullopt;

    std::set<const SCH_SCREEN*> screensInScope;

    for( const SCH_REFERENCE& ref : aRefs )
        screensInScope.insert( ref.GetSheetPath().LastScreen() );

    for( const std::pair<SCH_SHEET_PATH, SCH_SHEET_PATH>& collision : aSchematic.RefSuffixCollisions( true ) )
    {
        if( screensInScope.contains( collision.first.LastScreen() ) )
            return collision;
    }

    return std::nullopt;
}


static void removeUnitConflicts( SCH_MULTI_UNIT_REFERENCE_MAP& aLocked )
{
    std::erase_if( aLocked,
                   []( const auto& aEntry )
                   {
                       std::set<int> seenUnits;

                       for( const SCH_REFERENCE& ref : aEntry.second )
                       {
                           if( !seenUnits.insert( ref.GetUnit() ).second )
                               return true;
                       }

                       return false;
                   } );
}


std::set<const SCH_SCREEN*> AnnotateReferences( SCH_REFERENCE_LIST& aRefs, SCH_MULTI_UNIT_REFERENCE_MAP aLocked,
                                                const SCHEMATIC& aSchematic, ANNOTATE_SCOPE_T aScope,
                                                ANNOTATE_ORDER_T aSortOption, ANNOTATE_ALGO_T aAlgoOption,
                                                int aStartNumber, bool aResetAnnotation,
                                                const std::function<void( SCH_REFERENCE& )>& aBeforeAnnotate )
{
    const SCH_SHEET_LIST sheets = aSchematic.Hierarchy();
    const bool           suffixMode = aSchematic.Settings().m_AnnotateReusedSheetSuffixes;

    // A group holding one unit twice gets fresh assignments when resetting
    if( aResetAnnotation )
        removeUnitConflicts( aLocked );

    if( suffixMode )
    {
        aRefs.UseReusedSheetRepresentatives( sheets );

        // Merging can join two symbols on one unit even when annotation is kept
        aLocked = aRefs.LockedForRepresentatives( aLocked );
        removeUnitConflicts( aLocked );
    }

    // Remove annotation only updates the "new" flag to indicate to the algorithm
    // that these references must be reannotated, but keeps the original reference
    // so that we can reannotate multi-unit symbols together.
    if( aResetAnnotation )
        aRefs.RemoveAnnotation();

    // Build additional list of references to be used during reannotation
    // to avoid duplicate designators (no additional references when annotating
    // the full schematic)
    SCH_REFERENCE_LIST additionalRefs;

    if( aScope != ANNOTATE_ALL )
    {
        SCH_REFERENCE_LIST              allRefs;
        std::unordered_set<SCH_SYMBOL*> annotatedSymbols;
        sheets.GetSymbols( allRefs, SYMBOL_FILTER_NON_POWER );

        // The other instances of a renumbered symbol take its new base, so their old one is free
        if( suffixMode )
        {
            for( const SCH_REFERENCE& ref : aRefs )
            {
                if( !aSchematic.CopiesSelectDifferentUnits( ref.GetSymbol() ) )
                    annotatedSymbols.insert( ref.GetSymbol() );
            }
        }

        for( const SCH_REFERENCE& ref : allRefs )
        {
            if( !annotatedSymbols.contains( ref.GetSymbol() ) && !aRefs.Contains( ref ) )
                additionalRefs.AddItem( ref );
        }
    }

    if( suffixMode )
        aRefs.RenumberBasesUsedBy( additionalRefs, aLocked, sheets );

    aRefs.SetRefDesTracker( aSchematic.Settings().m_refDesTracker );

    // Break full symbol reference into name (prefix) and number:
    // example: IC1 become IC, and 1
    aRefs.SplitReferences();
    aRefs.AnnotateByOptions( aSortOption, aAlgoOption, aStartNumber, aLocked, additionalRefs, false,
                             suffixMode ? &sheets : nullptr );

    std::set<const SCH_SCREEN*> sharedScreens;

    // Annotating changes references, so decide which copies share before any of them do
    const std::set<KIID> splitCopies = suffixMode ? aSchematic.SplitUnitSymbols() : std::set<KIID>();

    for( SCH_REFERENCE& ref : aRefs )
    {
        if( aBeforeAnnotate )
            aBeforeAnnotate( ref );

        ref.Annotate();

        if( suffixMode && !splitCopies.contains( ref.GetSymbol()->m_Uuid )
                && sheets.ShareReusedSheetRef( ref.GetSymbol(), ref.GetSheetPath() ) > 0 )
        {
            sharedScreens.insert( ref.GetSheetPath().LastScreen() );
        }
    }

    return sharedScreens;
}
