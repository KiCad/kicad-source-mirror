/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>

#include <sch_screen.h>
#include <sch_item.h>
#include <sch_marker.h>
#include <sch_reference_list.h>
#include <class_library.h>
#include <sch_sheet_path.h>
#include <sch_component.h>
#include <sch_sheet.h>
#include <template_fieldnames.h>
#include <trace_helpers.h>

#include <boost/functional/hash.hpp>
#include <wx/filename.h>
#include "erc_item.h"

namespace std
{
    size_t hash<SCH_SHEET_PATH>::operator()( const SCH_SHEET_PATH& path ) const
    {
        return path.GetCurrentHash();
    }
}


SCH_SHEET_PATH::SCH_SHEET_PATH()
{
    m_pageNumber = 0;
    m_current_hash = 0;
}


void SCH_SHEET_PATH::Rehash()
{
    m_current_hash = 0;

    for( auto sheet : m_sheets )
        boost::hash_combine( m_current_hash, sheet->m_Uuid.Hash() );
}


int SCH_SHEET_PATH::Cmp( const SCH_SHEET_PATH& aSheetPathToTest ) const
{
    if( size() > aSheetPathToTest.size() )
        return 1;

    if( size() < aSheetPathToTest.size() )
        return -1;

    //otherwise, same number of sheets.
    for( unsigned i = 0; i < size(); i++ )
    {
        if( at( i )->m_Uuid < aSheetPathToTest.at( i )->m_Uuid )
            return -1;

        if( at( i )->m_Uuid != aSheetPathToTest.at( i )->m_Uuid )
            return 1;
    }

    return 0;
}


SCH_SHEET* SCH_SHEET_PATH::Last() const
{
    if( !empty() )
        return m_sheets.back();

    return nullptr;
}


SCH_SCREEN* SCH_SHEET_PATH::LastScreen()
{
    SCH_SHEET* lastSheet = Last();

    if( lastSheet )
        return lastSheet->GetScreen();

    return nullptr;
}


SCH_SCREEN* SCH_SHEET_PATH::LastScreen() const
{
    SCH_SHEET* lastSheet = Last();

    if( lastSheet )
        return lastSheet->GetScreen();

    return nullptr;
}


wxString SCH_SHEET_PATH::PathAsString() const
{
    wxString s;

    s = wxT( "/" );     // This is the root path

    // Start at 1 to avoid the root sheet, which does not need to be added to the path.
    // It's timestamp changes anyway.
    for( unsigned i = 1; i < size(); i++ )
        s += at( i )->m_Uuid.AsString() + "/";

    return s;
}


KIID_PATH SCH_SHEET_PATH::Path() const
{
    KIID_PATH path;

    for( const SCH_SHEET* sheet : m_sheets )
        path.push_back( sheet->m_Uuid );

    return path;
}


wxString SCH_SHEET_PATH::GetRootPathName( bool aUseShortName )
{
    // return a PathName for the root sheet (like "/" or "<root>"
    // DO NOT use it in netlists, because it can easily break these netlists
    // especially after translation, because many netlists (i.e. spice) do not accept any char
    // Use only the short name ("/") and the full name only in messages
    return aUseShortName ? wxT( "/" ) : _( "<root_sheet>" );
}


wxString SCH_SHEET_PATH::PathHumanReadable() const
{
    wxString s;

    if( size() == 1 )
        return GetRootPathName( true );  // Use only the short name in netlists

    s = wxT( "/" );

    // Start at 1 to avoid the root sheet, as above.
    for( unsigned i = 1; i < size(); i++ )
        s = s + at( i )->GetFields()[ SHEETNAME ].GetShownText() + wxT( "/" );

    return s;
}


void SCH_SHEET_PATH::UpdateAllScreenReferences()
{
    for( auto item : LastScreen()->Items().OfType( SCH_COMPONENT_T ) )
    {
        auto component = static_cast<SCH_COMPONENT*>( item );
        component->GetField( REFERENCE )->SetText( component->GetRef( this ) );
        component->UpdateUnit( component->GetUnitSelection( this ) );
    }
}



void SCH_SHEET_PATH::GetComponents( SCH_REFERENCE_LIST& aReferences, bool aIncludePowerSymbols,
                                    bool aForceIncludeOrphanComponents ) const
{
    for( auto item : LastScreen()->Items().OfType( SCH_COMPONENT_T ) )
    {
        auto component = static_cast<SCH_COMPONENT*>( item );

        // Skip pseudo components, which have a reference starting with #.  This mainly
        // affects power symbols.
        if( aIncludePowerSymbols || component->GetRef( this )[0] != wxT( '#' ) )
        {
            LIB_PART* part = component->GetPartRef().get();

            if( part || aForceIncludeOrphanComponents )
            {
                SCH_REFERENCE schReference( component, part, *this );

                schReference.SetSheetNumber( m_pageNumber );
                aReferences.AddItem( schReference );
            }
        }
    }
}


void SCH_SHEET_PATH::GetMultiUnitComponents( SCH_MULTI_UNIT_REFERENCE_MAP& aRefList,
                                             bool aIncludePowerSymbols )
{
    for( auto item : LastScreen()->Items().OfType( SCH_COMPONENT_T ) )
    {
        auto component = static_cast<SCH_COMPONENT*>( item );

        // Skip pseudo components, which have a reference starting with #.  This mainly
        // affects power symbols.
        if( !aIncludePowerSymbols && component->GetRef( this )[0] == wxT( '#' ) )
            continue;

        LIB_PART* part = component->GetPartRef().get();

        if( part && part->GetUnitCount() > 1 )
        {
            SCH_REFERENCE schReference = SCH_REFERENCE( component, part, *this );
            schReference.SetSheetNumber( m_pageNumber );
            wxString reference_str = schReference.GetRef();

            // Never lock unassigned references
            if( reference_str[reference_str.Len() - 1] == '?' )
                continue;

            aRefList[reference_str].AddItem( schReference );
        }
    }
}


bool SCH_SHEET_PATH::SetComponentFootprint( const wxString& aReference, const wxString& aFootPrint,
                                            bool aSetVisible )
{
    SCH_SCREEN* screen = LastScreen();

    if( screen == NULL )
        return false;

    return screen->SetComponentFootprint( this, aReference, aFootPrint, aSetVisible );
}


bool SCH_SHEET_PATH::operator==( const SCH_SHEET_PATH& d1 ) const
{
    return m_current_hash == d1.GetCurrentHash();
}


bool SCH_SHEET_PATH::TestForRecursion( const wxString& aSrcFileName, const wxString& aDestFileName )
{
    auto pair = std::make_pair( aSrcFileName, aDestFileName );

    if( m_recursion_test_cache.count( pair ) )
        return m_recursion_test_cache.at( pair );

    wxFileName rootFn = g_RootSheet->GetFileName();
    wxFileName srcFn = aSrcFileName;
    wxFileName destFn = aDestFileName;

    if( srcFn.IsRelative() )
        srcFn.MakeAbsolute( rootFn.GetPath() );

    if( destFn.IsRelative() )
        destFn.MakeAbsolute( rootFn.GetPath() );


    // The source and destination sheet file names cannot be the same.
    if( srcFn == destFn )
    {
        m_recursion_test_cache[pair] = true;
        return true;
    }

    /// @todo Store sheet file names with full path, either relative to project path
    ///       or absolute path.  The current design always assumes subsheet files are
    ///       located in the project folder which may or may not be desirable.
    unsigned i = 0;

    while( i < size() )
    {
        wxFileName cmpFn = at( i )->GetFileName();

        if( cmpFn.IsRelative() )
            cmpFn.MakeAbsolute( rootFn.GetPath() );

        // Test if the file name of the destination sheet is in anywhere in this sheet path.
        if( cmpFn == destFn )
            break;

        i++;
    }

    // The destination sheet file name was not found in the sheet path or the destination
    // sheet file name is the root sheet so no recursion is possible.
    if( i >= size() || i == 0 )
    {
        m_recursion_test_cache[pair] = false;
        return false;
    }

    // Walk back up to the root sheet to see if the source file name is already a parent in
    // the sheet path.  If so, recursion will occur.
    do
    {
        i -= 1;

        wxFileName cmpFn = at( i )->GetFileName();

        if( cmpFn.IsRelative() )
            cmpFn.MakeAbsolute( rootFn.GetPath() );

        if( cmpFn == srcFn )
        {
            m_recursion_test_cache[pair] = true;
            return true;
        }

    } while( i != 0 );

    // The source sheet file name is not a parent of the destination sheet file name.
    m_recursion_test_cache[pair] = false;
    return false;
}


SCH_SHEET_LIST::SCH_SHEET_LIST( SCH_SHEET* aSheet )
{
    m_isRootSheet = false;

    if( aSheet != NULL )
        BuildSheetList( aSheet );
}


void SCH_SHEET_LIST::BuildSheetList( SCH_SHEET* aSheet )
{
    wxCHECK_RET( aSheet != NULL, wxT( "Cannot build sheet list from undefined sheet." ) );

    std::vector<SCH_SHEET*> badSheets;

    if( aSheet == g_RootSheet )
        m_isRootSheet = true;

    m_currentSheetPath.push_back( aSheet );

    /**
     * @todo:  Schematic page number is currently a left over relic and is generated as
     *         SCH_SHEET_PATH object is pushed to the list.  This only has meaning when
     *         entire hierarchy is created from the root sheet down.
     */
    m_currentSheetPath.SetPageNumber( size() + 1 );
    push_back( m_currentSheetPath );

    if( m_currentSheetPath.LastScreen() )
    {
        std::vector<SCH_ITEM*> childSheets;
        m_currentSheetPath.LastScreen()->GetSheets( &childSheets );

        for( SCH_ITEM* item : childSheets )
        {
            auto sheet = static_cast<SCH_SHEET*>( item );

            if( !m_currentSheetPath.TestForRecursion(
                        sheet->GetFileName(), aSheet->GetFileName() ) )
                BuildSheetList( sheet );
            else
                badSheets.push_back( sheet );
        }
    }

    for( auto sheet : badSheets )
    {
        m_currentSheetPath.LastScreen()->Remove( sheet );
        m_currentSheetPath.LastScreen()->SetModify();
    }

    m_currentSheetPath.pop_back();
}


bool SCH_SHEET_LIST::NameExists( const wxString& aSheetName )
{
    for( const SCH_SHEET_PATH& sheet : *this )
    {
        if( sheet.Last()->GetName() == aSheetName )
            return true;
    }

    return false;
}


bool SCH_SHEET_LIST::IsModified()
{
    for( const SCH_SHEET_PATH& sheet : *this )
    {
        if( sheet.LastScreen() && sheet.LastScreen()->IsModify() )
            return true;
    }

    return false;
}


void SCH_SHEET_LIST::ClearModifyStatus()
{
    for( const SCH_SHEET_PATH& sheet : *this )
    {
        if( sheet.LastScreen() )
            sheet.LastScreen()->ClrModify();
    }
}


SCH_ITEM* SCH_SHEET_LIST::GetItem( const KIID& aID, SCH_SHEET_PATH* aPathOut )
{
    for( const SCH_SHEET_PATH& sheet : *this )
    {
        SCH_SCREEN* screen = sheet.LastScreen();

        for( SCH_ITEM* aItem : screen->Items() )
        {
            if( aItem->m_Uuid == aID )
            {
                *aPathOut = sheet;
                return aItem;
            }
            else if( aItem->Type() == SCH_COMPONENT_T )
            {
                SCH_COMPONENT* comp = static_cast<SCH_COMPONENT*>( aItem );

                for( SCH_FIELD& field : comp->GetFields() )
                {
                    if( field.m_Uuid == aID )
                    {
                        *aPathOut = sheet;
                        return &field;
                    }
                }

                for( SCH_PIN* pin : comp->GetSchPins() )
                {
                    if( pin->m_Uuid == aID )
                    {
                        *aPathOut = sheet;
                        return pin;
                    }
                }
            }
            else if( aItem->Type() == SCH_SHEET_T )
            {
                SCH_SHEET* sch_sheet = static_cast<SCH_SHEET*>( aItem );

                for( SCH_FIELD& field : sch_sheet->GetFields() )
                {
                    if( field.m_Uuid == aID )
                    {
                        *aPathOut = sheet;
                        return &field;
                    }
                }

                for( SCH_SHEET_PIN* pin : sch_sheet->GetPins() )
                {
                    if( pin->m_Uuid == aID )
                    {
                        *aPathOut = sheet;
                        return pin;
                    }
                }
            }
        }
    }

    return nullptr;
}


void SCH_SHEET_LIST::FillItemMap( std::map<KIID, EDA_ITEM*>& aMap )
{
    for( const SCH_SHEET_PATH& sheet : *this )
    {
        SCH_SCREEN* screen = sheet.LastScreen();

        for( SCH_ITEM* aItem : screen->Items() )
        {
            aMap[ aItem->m_Uuid ] = aItem;

            if( aItem->Type() == SCH_COMPONENT_T )
            {
                SCH_COMPONENT* comp = static_cast<SCH_COMPONENT*>( aItem );

                for( SCH_FIELD& field : comp->GetFields() )
                    aMap[ field.m_Uuid ] = &field;

                for( SCH_PIN* pin : comp->GetSchPins() )
                    aMap[ pin->m_Uuid ] = pin;
            }
            else if( aItem->Type() == SCH_SHEET_T )
            {
                SCH_SHEET* sch_sheet = static_cast<SCH_SHEET*>( aItem );

                for( SCH_FIELD& field : sch_sheet->GetFields() )
                    aMap[ field.m_Uuid ] = &field;

                for( SCH_SHEET_PIN* pin : sch_sheet->GetPins() )
                    aMap[ pin->m_Uuid ] = pin;
            }
        }
    }
}


void SCH_SHEET_LIST::AnnotatePowerSymbols()
{
    // List of reference for power symbols
    SCH_REFERENCE_LIST references;

    // Map of locked components (not used, but needed by Annotate()
    SCH_MULTI_UNIT_REFERENCE_MAP lockedComponents;

    // Build the list of power components:
    for( SCH_SHEET_PATH& sheet : *this )
    {
        for( auto item : sheet.LastScreen()->Items().OfType( SCH_COMPONENT_T ) )
        {
            auto      component = static_cast<SCH_COMPONENT*>( item );
            LIB_PART* part = component->GetPartRef().get();

            if( !part || !part->IsPower() )
                continue;

            if( part )
            {
                SCH_REFERENCE schReference( component, part, sheet );
                references.AddItem( schReference );
            }
        }
    }

    // Find duplicate, and silently clear annotation of duplicate
    std::map<wxString, int> ref_list;   // stores the existing references

    for( unsigned ii = 0; ii< references.GetCount(); ++ii )
    {
        wxString curr_ref = references[ii].GetRef();

        if( ref_list.find( curr_ref ) == ref_list.end() )
        {
            ref_list[curr_ref] = ii;
            continue;
        }

        // Possible duplicate, if the ref ends by a number:
        if( curr_ref.Last() < '0' && curr_ref.Last() > '9' )
            continue;   // not annotated

        // Duplicate: clear annotation by removing the number ending the ref
        while( curr_ref.Last() >= '0' && curr_ref.Last() <= '9' )
            curr_ref.RemoveLast();

        references[ii].SetRef( curr_ref );
    }


    // Break full components reference in name (prefix) and number:
    // example: IC1 become IC, and 1
    references.SplitReferences();

    // Ensure all power symbols have the reference starting by '#'
    // (No sure this is really useful)
    for( unsigned ii = 0; ii< references.GetCount(); ++ii )
    {
        if( references[ii].GetRef()[0] != '#' )
        {
            wxString new_ref = "#" + references[ii].GetRef();
            references[ii].SetRef( new_ref );
        }
    }

    // Recalculate and update reference numbers in schematic
    references.Annotate( false, 0, 100, lockedComponents );
    references.UpdateAnnotation();
}


void SCH_SHEET_LIST::GetComponents( SCH_REFERENCE_LIST& aReferences, bool aIncludePowerSymbols,
                                    bool aForceIncludeOrphanComponents )
{
    for( SCH_SHEET_PATH& sheet : *this )
        sheet.GetComponents( aReferences, aIncludePowerSymbols, aForceIncludeOrphanComponents );
}

void SCH_SHEET_LIST::GetMultiUnitComponents( SCH_MULTI_UNIT_REFERENCE_MAP &aRefList,
                                             bool aIncludePowerSymbols )
{
    for( SCH_SHEET_PATHS_ITER it = begin(); it != end(); ++it )
    {
        SCH_MULTI_UNIT_REFERENCE_MAP tempMap;
        (*it).GetMultiUnitComponents( tempMap );

        for( SCH_MULTI_UNIT_REFERENCE_MAP::value_type& pair : tempMap )
        {
            // Merge this list into the main one
            unsigned n_refs = pair.second.GetCount();

            for( unsigned thisRef = 0; thisRef < n_refs; ++thisRef )
            {
                aRefList[pair.first].AddItem( pair.second[thisRef] );
            }
        }
    }
}


bool SCH_SHEET_LIST::SetComponentFootprint( const wxString& aReference,
                                            const wxString& aFootPrint, bool aSetVisible )
{
    bool found = false;

    for( SCH_SHEET_PATH& sheet : *this )
        found = sheet.SetComponentFootprint( aReference, aFootPrint, aSetVisible );

    return found;
}


bool SCH_SHEET_LIST::TestForRecursion( const SCH_SHEET_LIST& aSrcSheetHierarchy,
                                       const wxString& aDestFileName )
{
    wxFileName rootFn = g_RootSheet->GetFileName();
    wxFileName destFn = aDestFileName;

    if( destFn.IsRelative() )
        destFn.MakeAbsolute( rootFn.GetPath() );

    // Test each SCH_SHEET_PATH in this SCH_SHEET_LIST for potential recursion.
    for( unsigned i = 0; i < size(); i++ )
    {
        // Test each SCH_SHEET_PATH in the source sheet.
        for( unsigned j = 0; j < aSrcSheetHierarchy.size(); j++ )
        {
            const SCH_SHEET_PATH* sheetPath = &aSrcSheetHierarchy[j];

            for( unsigned k = 0; k < sheetPath->size(); k++ )
            {
                if( at( i ).TestForRecursion( sheetPath->GetSheet( k )->GetFileName(),
                                              aDestFileName ) )
                    return true;
            }
        }
    }

    // The source sheet file can safely be added to the destination sheet file.
    return false;
}


SCH_SHEET_PATH* SCH_SHEET_LIST::FindSheetForScreen( SCH_SCREEN* aScreen )
{
    for( SCH_SHEET_PATH& sheetpath : *this )
    {
        if( sheetpath.LastScreen() == aScreen )
            return &sheetpath;
    }

    return nullptr;
}


void SCH_SHEET_LIST::UpdateSymbolInstances(
        std::vector<COMPONENT_INSTANCE_REFERENCE>& aSymbolInstances )
{
    wxCHECK( m_isRootSheet, /* void */ );   // Only performed for the entire schematic.

    SCH_REFERENCE_LIST symbolInstances;

    GetComponents( symbolInstances, true, true );

    for( size_t i = 0; i < symbolInstances.GetCount(); i++ )
    {
        // The instance paths are stored in the file sans root path so the comparison
        // should not include the root path.
        wxString path = symbolInstances[i].GetPath();

        auto it = std::find_if( aSymbolInstances.begin(), aSymbolInstances.end(),
                    [ path ]( COMPONENT_INSTANCE_REFERENCE& r )->bool
                    {
                        return path == r.m_Path.AsString();
                    }
                );

        if( it == aSymbolInstances.end() )
        {
            wxLogTrace( traceSchSheetPaths, "No symbol instance found for path \"%s\"", path );
            continue;
        }

        SCH_COMPONENT* symbol = symbolInstances[i].GetComp();

        wxCHECK2( symbol, continue );

        // Symbol instance paths are stored and looked up in memory with the root path so use
        // the full path here.
        symbol->AddHierarchicalReference( symbolInstances[i].GetSheetPath().Path(),
                it->m_Reference, it->m_Unit );
        symbol->GetField( REFERENCE )->SetText( it->m_Reference );
    }
}


std::vector<KIID_PATH> SCH_SHEET_LIST::GetPaths() const
{
    std::vector<KIID_PATH> paths;

    for( auto sheetPath : *this )
        paths.emplace_back( sheetPath.Path() );

    return paths;
}


void SCH_SHEET_LIST::ReplaceLegacySheetPaths( const std::vector<KIID_PATH>& aOldSheetPaths )
{
    wxCHECK( size() == aOldSheetPaths.size(), /* void */ );

    for( size_t i = 0;  i < size(); i++ )
    {
        const KIID_PATH oldSheetPath = aOldSheetPaths.at( i );
        const KIID_PATH newSheetPath = at( i ).Path();
        SCH_SCREEN* screen = at(i).LastScreen();

        wxCHECK( screen, /* void */ );

        for( auto symbol : screen->Items().OfType( SCH_COMPONENT_T ) )
        {
            static_cast<SCH_COMPONENT*>( symbol )->ReplaceInstanceSheetPath( oldSheetPath,
                    newSheetPath );
        }
    }
}


void SHEETLIST_ERC_ITEMS_PROVIDER::SetSeverities( int aSeverities )
{
    m_severities = aSeverities;

    m_filteredMarkers.clear();

    SCH_SHEET_LIST sheetList( g_RootSheet);

    for( unsigned i = 0; i < sheetList.size(); i++ )
    {
        for( SCH_ITEM* aItem : sheetList[i].LastScreen()->Items().OfType( SCH_MARKER_T ) )
        {
            SCH_MARKER* marker = static_cast<SCH_MARKER*>( aItem );
            int markerSeverity;

            if( marker->GetMarkerType() != MARKER_BASE::MARKER_ERC )
                continue;

            if( marker->IsExcluded() )
                markerSeverity = RPT_SEVERITY_EXCLUSION;
            else
                markerSeverity = GetSeverity( marker->GetRCItem()->GetErrorCode() );

            if( markerSeverity & m_severities )
                m_filteredMarkers.push_back( marker );
        }
    }
}


int SHEETLIST_ERC_ITEMS_PROVIDER::GetCount( int aSeverity )
{
    if( aSeverity < 0 )
        return m_filteredMarkers.size();

    int count = 0;

    SCH_SHEET_LIST sheetList( g_RootSheet);

    for( unsigned i = 0; i < sheetList.size(); i++ )
    {
        for( SCH_ITEM* aItem : sheetList[i].LastScreen()->Items().OfType( SCH_MARKER_T ) )
        {
            SCH_MARKER* marker = static_cast<SCH_MARKER*>( aItem );
            int markerSeverity;

            if( marker->GetMarkerType() != MARKER_BASE::MARKER_ERC )
                continue;

            if( marker->IsExcluded() )
                markerSeverity = RPT_SEVERITY_EXCLUSION;
            else
                markerSeverity = GetSeverity( marker->GetRCItem()->GetErrorCode() );

            if( markerSeverity == aSeverity )
                count++;
        }
    }

    return count;
}


ERC_ITEM* SHEETLIST_ERC_ITEMS_PROVIDER::GetItem( int aIndex )
{
    SCH_MARKER* marker = m_filteredMarkers[ aIndex ];

    return marker ? static_cast<ERC_ITEM*>( marker->GetRCItem() ) : nullptr;
}


void SHEETLIST_ERC_ITEMS_PROVIDER::DeleteItem( int aIndex, bool aDeep )
{
    SCH_MARKER* marker = m_filteredMarkers[ aIndex ];
    m_filteredMarkers.erase( m_filteredMarkers.begin() + aIndex );

    if( aDeep )
    {
        SCH_SCREENS ScreenList;
        ScreenList.DeleteMarker( marker );
    }
}


void SHEETLIST_ERC_ITEMS_PROVIDER::DeleteAllItems()
{
    SCH_SCREENS ScreenList;
    ScreenList.DeleteAllMarkers( MARKER_BASE::MARKER_ERC );
    m_filteredMarkers.clear();
}
