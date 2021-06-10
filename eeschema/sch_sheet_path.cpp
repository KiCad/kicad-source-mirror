/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <sch_screen.h>
#include <sch_item.h>
#include <sch_marker.h>
#include <sch_reference_list.h>
#include <class_library.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <sch_sheet.h>
#include <schematic.h>
#include <template_fieldnames.h>
#include <trace_helpers.h>

#include <boost/functional/hash.hpp>
#include <wx/filename.h>
#include <wx/log.h>
#include "erc_item.h"


/**
 * A singleton item of this class is returned for a weak reference that no longer exists.
 * Its sole purpose is to flag the item as having been deleted.
 */
class DELETED_SHEET_ITEM : public SCH_ITEM
{
public:
    DELETED_SHEET_ITEM() :
        SCH_ITEM( nullptr, NOT_USED )
    {}

    wxString GetSelectMenuText( EDA_UNITS aUnits ) const override
    {
        return _( "(Deleted Item)" );
    }

    wxString GetClass() const override
    {
        return wxT( "DELETED_SHEET_ITEM" );
    }

    static DELETED_SHEET_ITEM* GetInstance()
    {
        static DELETED_SHEET_ITEM* item = nullptr;

        if( !item )
            item = new DELETED_SHEET_ITEM();

        return item;
    }

    // pure virtuals:
    void SetPosition( const wxPoint& ) override {}
    void Print( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset ) override {}
    void Move( const wxPoint& aMoveVector ) override {}
    void MirrorHorizontally( int aCenter ) override {}
    void MirrorVertically( int aCenter ) override {}
    void Rotate( const wxPoint& aCenter ) override {}

#if defined(DEBUG)
    void Show( int , std::ostream&  ) const override {}
#endif
};


namespace std
{
    size_t hash<SCH_SHEET_PATH>::operator()( const SCH_SHEET_PATH& path ) const
    {
        return path.GetCurrentHash();
    }
}


SCH_SHEET_PATH::SCH_SHEET_PATH()
{
    m_virtualPageNumber = 0;
    m_current_hash = 0;
}


SCH_SHEET_PATH::SCH_SHEET_PATH( const SCH_SHEET_PATH& aOther )
{
    initFromOther( aOther );
}


SCH_SHEET_PATH& SCH_SHEET_PATH::operator=( const SCH_SHEET_PATH& aOther )
{
    initFromOther( aOther );
    return *this;
}


void SCH_SHEET_PATH::initFromOther( const SCH_SHEET_PATH& aOther )
{
    m_sheets            = aOther.m_sheets;
    m_virtualPageNumber = aOther.m_virtualPageNumber;
    m_current_hash      = aOther.m_current_hash;

    // Note: don't copy m_recursion_test_cache as it is slow and we want SCH_SHEET_PATHS to be
    // very fast to construct for use in the connectivity algorithm.
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


int SCH_SHEET_PATH::ComparePageNumAndName( const SCH_SHEET_PATH& aSheetPathToTest ) const
{
    wxString pageA = GetPageNumber();
    wxString pageB = aSheetPathToTest.GetPageNumber();

    int pageNumComp = SCH_SHEET::ComparePageNum( pageA, pageB );

    if( pageNumComp == 0 )
    {
        wxString nameA = Last()->GetName();
        wxString nameB = aSheetPathToTest.Last()->GetName();

        return nameA.Cmp( nameB );
    }
    else
    {
        return pageNumComp;
    }
}


bool SCH_SHEET_PATH::IsContainedWithin( const SCH_SHEET_PATH& aSheetPathToTest ) const
{
    if( aSheetPathToTest.size() > size() )
        return false;

    for( size_t i = 0; i < aSheetPathToTest.size(); ++i )
    {
        if( at( i )->m_Uuid != aSheetPathToTest.at( i )->m_Uuid )
            return false;
    }

    return true;
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


KIID_PATH SCH_SHEET_PATH::PathWithoutRootUuid() const
{
    KIID_PATH path;

    for( size_t i = 1; i < size(); i++ )
        path.push_back( at( i )->m_Uuid );

    return path;
}


wxString SCH_SHEET_PATH::PathHumanReadable( bool aUseShortRootName ) const
{
    wxString s;
    wxString fileName;

    if( !empty() && at( 0 )->GetScreen() )
        fileName = at( 0 )->GetScreen()->GetFileName();

    wxFileName  fn = fileName;

    if( aUseShortRootName )
        s = wxT( "/" );  // Use only the short name in netlists
    else
        s = fn.GetName() + wxT( "/" );

    // Start at 1 since we've already processed the root sheet.
    for( unsigned i = 1; i < size(); i++ )
        s = s + at( i )->GetFields()[ SHEETNAME ].GetShownText() + wxT( "/" );

    return s;
}


void SCH_SHEET_PATH::UpdateAllScreenReferences()
{
    for( SCH_ITEM* item : LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
        symbol->GetField( REFERENCE_FIELD )->SetText( symbol->GetRef( this ) );
        symbol->GetField( VALUE_FIELD )->SetText( symbol->GetValue( this, false ) );
        symbol->GetField( FOOTPRINT_FIELD )->SetText( symbol->GetFootprint( this, false ) );
        symbol->UpdateUnit( symbol->GetUnitSelection( this ) );
    }
}



void SCH_SHEET_PATH::GetSymbols( SCH_REFERENCE_LIST& aReferences, bool aIncludePowerSymbols,
                                 bool aForceIncludeOrphanSymbols ) const
{
    for( SCH_ITEM* item : LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
        AppendSymbol( aReferences, symbol, aIncludePowerSymbols, aForceIncludeOrphanSymbols );
    }
}


void SCH_SHEET_PATH::AppendSymbol( SCH_REFERENCE_LIST& aReferences, SCH_SYMBOL* aSymbol,
                                   bool aIncludePowerSymbols,
                                   bool aForceIncludeOrphanSymbols ) const
{
    // Skip pseudo-symbols, which have a reference starting with #.  This mainly
    // affects power symbols.
    if( aIncludePowerSymbols || aSymbol->GetRef( this )[0] != wxT( '#' ) )
    {
        LIB_PART* part = aSymbol->GetPartRef().get();

        if( part || aForceIncludeOrphanSymbols )
        {
            SCH_REFERENCE schReference( aSymbol, part, *this );

            schReference.SetSheetNumber( m_virtualPageNumber );
            aReferences.AddItem( schReference );
        }
    }
}


void SCH_SHEET_PATH::GetMultiUnitSymbols( SCH_MULTI_UNIT_REFERENCE_MAP& aRefList,
                                          bool aIncludePowerSymbols ) const
{
    for( SCH_ITEM* item : LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
        AppendMultiUnitSymbol( aRefList, symbol, aIncludePowerSymbols );
    }
}


void SCH_SHEET_PATH::AppendMultiUnitSymbol( SCH_MULTI_UNIT_REFERENCE_MAP& aRefList,
                                            SCH_SYMBOL* aSymbol,
                                            bool aIncludePowerSymbols ) const
{
    // Skip pseudo-symbols, which have a reference starting with #.  This mainly
    // affects power symbols.
    if( !aIncludePowerSymbols && aSymbol->GetRef( this )[0] == wxT( '#' ) )
        return;

    LIB_PART* part = aSymbol->GetPartRef().get();

    if( part && part->GetUnitCount() > 1 )
    {
        SCH_REFERENCE schReference = SCH_REFERENCE( aSymbol, part, *this );
        schReference.SetSheetNumber( m_virtualPageNumber );
        wxString reference_str = schReference.GetRef();

        // Never lock unassigned references
        if( reference_str[reference_str.Len() - 1] == '?' )
            return;

        aRefList[reference_str].AddItem( schReference );
    }
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

    SCHEMATIC* sch = LastScreen()->Schematic();

    wxCHECK_MSG( sch, false, "No SCHEMATIC found in SCH_SHEET_PATH::TestForRecursion!" );

    wxFileName rootFn = sch->GetFileName();
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


wxString SCH_SHEET_PATH::GetPageNumber() const
{
    SCH_SHEET* sheet = Last();

    wxCHECK( sheet, wxEmptyString );

    return sheet->GetPageNumber( *this );
}


void SCH_SHEET_PATH::SetPageNumber( const wxString& aPageNumber )
{
    SCH_SHEET* sheet = Last();

    wxCHECK( sheet, /* void */ );

    sheet->SetPageNumber( *this, aPageNumber );
}


void SCH_SHEET_PATH::MakeFilePathRelativeToParentSheet()
{
    wxCHECK( m_sheets.size() > 1, /* void */  );

    wxFileName sheetFileName = Last()->GetFileName();

    // If the sheet file name is absolute, then the user requested is so don't make it relative.
    if( sheetFileName.IsAbsolute() )
        return;

    SCH_SCREEN* screen = LastScreen();
    SCH_SCREEN* parentScreen = m_sheets[ m_sheets.size() - 2 ]->GetScreen();

    wxCHECK( screen && parentScreen, /* void */ );

    wxFileName fileName = screen->GetFileName();
    wxFileName parentFileName = parentScreen->GetFileName();

    // SCH_SCREEN file names must be absolute.  If they are not, someone set them incorrectly
    // on load or on creation.
    wxCHECK( fileName.IsAbsolute() && parentFileName.IsAbsolute(), /* void */ );

    if( fileName.GetPath() == parentFileName.GetPath() )
    {
        Last()->SetFileName( fileName.GetFullName() );
    }
    else if( fileName.MakeRelativeTo( parentFileName.GetPath() ) )
    {
        Last()->SetFileName( fileName.GetFullPath() );
    }
    else
    {
        Last()->SetFileName( screen->GetFileName() );
    }

    wxLogTrace( tracePathsAndFiles,
                wxT( "\n    File name: '%s'"
                     "\n    parent file name '%s',"
                     "\n    sheet '%s' file name '%s'." ),
                screen->GetFileName(), parentScreen->GetFileName(), PathHumanReadable(),
                Last()->GetFileName() );
}


SCH_SHEET_LIST::SCH_SHEET_LIST( SCH_SHEET* aSheet, bool aCheckIntegrity )
{
    if( aSheet != nullptr )
    {
        BuildSheetList( aSheet, aCheckIntegrity );
        SortByPageNumbers();
    }
}


void SCH_SHEET_LIST::BuildSheetList( SCH_SHEET* aSheet, bool aCheckIntegrity )
{
    wxCHECK_RET( aSheet != nullptr, wxT( "Cannot build sheet list from undefined sheet." ) );

    std::vector<SCH_SHEET*> badSheets;

    m_currentSheetPath.push_back( aSheet );
    m_currentSheetPath.SetVirtualPageNumber( static_cast<int>( size() ) + 1 );
    push_back( m_currentSheetPath );

    if( m_currentSheetPath.LastScreen() )
    {
        wxString               parentFileName = aSheet->GetFileName();
        std::vector<SCH_ITEM*> childSheets;
        m_currentSheetPath.LastScreen()->GetSheets( &childSheets );

        for( SCH_ITEM* item : childSheets )
        {
            SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );

            if( aCheckIntegrity )
            {
                if( !m_currentSheetPath.TestForRecursion( sheet->GetFileName(), parentFileName ) )
                    BuildSheetList( sheet, true );
                else
                    badSheets.push_back( sheet );
            }
            else
            {
                BuildSheetList( sheet, false );
            }
        }
    }

    if( aCheckIntegrity )
    {
        for( SCH_SHEET* sheet : badSheets )
        {
            m_currentSheetPath.LastScreen()->Remove( sheet );
            m_currentSheetPath.LastScreen()->SetContentModified();
        }
    }

    m_currentSheetPath.pop_back();
}


void SCH_SHEET_LIST::SortByPageNumbers( bool aUpdateVirtualPageNums )
{
    std::sort( begin(), end(),
        []( SCH_SHEET_PATH a, SCH_SHEET_PATH b ) -> bool
        {
             return a.ComparePageNumAndName(b) < 0;
        } );

    if( aUpdateVirtualPageNums )
    {
        int virtualPageNum = 1;

        for( SCH_SHEET_PATH& sheet : *this )
        {
            sheet.SetVirtualPageNumber( virtualPageNum++ );
        }
    }
}


bool SCH_SHEET_LIST::NameExists( const wxString& aSheetName ) const
{
    for( const SCH_SHEET_PATH& sheet : *this )
    {
        if( sheet.Last()->GetName() == aSheetName )
            return true;
    }

    return false;
}


bool SCH_SHEET_LIST::PageNumberExists( const wxString& aPageNumber ) const
{
    for( const SCH_SHEET_PATH& sheet : *this )
    {
        if( sheet.GetPageNumber() == aPageNumber )
            return true;
    }

    return false;
}


bool SCH_SHEET_LIST::IsModified() const
{
    for( const SCH_SHEET_PATH& sheet : *this )
    {
        if( sheet.LastScreen() && sheet.LastScreen()->IsContentModified() )
            return true;
    }

    return false;
}


void SCH_SHEET_LIST::ClearModifyStatus()
{
    for( const SCH_SHEET_PATH& sheet : *this )
    {
        if( sheet.LastScreen() )
            sheet.LastScreen()->SetContentModified( false );
    }
}


SCH_ITEM* SCH_SHEET_LIST::GetItem( const KIID& aID, SCH_SHEET_PATH* aPathOut ) const
{
    for( const SCH_SHEET_PATH& sheet : *this )
    {
        SCH_SCREEN* screen = sheet.LastScreen();

        for( SCH_ITEM* aItem : screen->Items() )
        {
            if( aItem->m_Uuid == aID )
            {
                if( aPathOut )
                    *aPathOut = sheet;

                return aItem;
            }

            SCH_ITEM* childMatch = nullptr;

            aItem->RunOnChildren(
                    [&]( SCH_ITEM* aChild )
                    {
                        if( aChild->m_Uuid == aID )
                            childMatch = aChild;
                    } );

            if( childMatch )
            {
                if( aPathOut )
                    *aPathOut = sheet;

                return childMatch;
            }
        }
    }

    // Not found; weak reference has been deleted.
    return DELETED_SHEET_ITEM::GetInstance();
}


void SCH_SHEET_LIST::FillItemMap( std::map<KIID, EDA_ITEM*>& aMap )
{
    for( const SCH_SHEET_PATH& sheet : *this )
    {
        SCH_SCREEN* screen = sheet.LastScreen();

        for( SCH_ITEM* aItem : screen->Items() )
        {
            aMap[ aItem->m_Uuid ] = aItem;

            aItem->RunOnChildren(
                    [&]( SCH_ITEM* aChild )
                    {
                        aMap[ aChild->m_Uuid ] = aChild;
                    } );
        }
    }
}


void SCH_SHEET_LIST::AnnotatePowerSymbols()
{
    // List of reference for power symbols
    SCH_REFERENCE_LIST references;
    SCH_REFERENCE_LIST additionalreferences; // Todo: add as a parameter to this function

    // Map of locked symbols (not used, but needed by Annotate()
    SCH_MULTI_UNIT_REFERENCE_MAP lockedSymbols;

    // Build the list of power symbols:
    for( SCH_SHEET_PATH& sheet : *this )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            LIB_PART*   part = symbol->GetPartRef().get();

            if( part && part->IsPower() )
            {
                SCH_REFERENCE schReference( symbol, part, sheet );
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


    // Break full symbol reference into name (prefix) and number:
    // example: IC1 become IC, and 1
    references.SplitReferences();

    // Ensure all power symbols have the reference starting by '#'
    // (Not sure this is really useful)
    for( unsigned ii = 0; ii< references.GetCount(); ++ii )
    {
        if( references[ii].GetRef()[0] != '#' )
        {
            wxString new_ref = "#" + references[ii].GetRef();
            references[ii].SetRef( new_ref );
        }
    }

    // Recalculate and update reference numbers in schematic
    references.Annotate( false, 0, 100, lockedSymbols, additionalreferences );
    references.UpdateAnnotation();
}


void SCH_SHEET_LIST::GetSymbols( SCH_REFERENCE_LIST& aReferences, bool aIncludePowerSymbols,
                                 bool aForceIncludeOrphanSymbols ) const
{
    for( const SCH_SHEET_PATH& sheet : *this )
        sheet.GetSymbols( aReferences, aIncludePowerSymbols, aForceIncludeOrphanSymbols );
}


void SCH_SHEET_LIST::GetSymbolsWithinPath( SCH_REFERENCE_LIST&   aReferences,
                                           const SCH_SHEET_PATH& aSheetPath,
                                           bool                  aIncludePowerSymbols,
                                           bool                  aForceIncludeOrphanSymbols ) const
{
    for( const SCH_SHEET_PATH& sheet : *this )
    {
        if( sheet.IsContainedWithin( aSheetPath ) )
            sheet.GetSymbols( aReferences, aIncludePowerSymbols, aForceIncludeOrphanSymbols );
    }
}


void SCH_SHEET_LIST::GetSheetsWithinPath( SCH_SHEET_PATHS&      aSheets,
                                          const SCH_SHEET_PATH& aSheetPath ) const
{
    for( const SCH_SHEET_PATH& sheet : *this )
    {
        if( sheet.IsContainedWithin( aSheetPath ) )
            aSheets.push_back( sheet );
    }
}


void SCH_SHEET_LIST::GetMultiUnitSymbols( SCH_MULTI_UNIT_REFERENCE_MAP &aRefList,
                                          bool aIncludePowerSymbols ) const
{
    for( SCH_SHEET_PATHS::const_iterator it = begin(); it != end(); ++it )
    {
        SCH_MULTI_UNIT_REFERENCE_MAP tempMap;
        ( *it ).GetMultiUnitSymbols( tempMap );

        for( SCH_MULTI_UNIT_REFERENCE_MAP::value_type& pair : tempMap )
        {
            // Merge this list into the main one
            unsigned n_refs = pair.second.GetCount();

            for( unsigned thisRef = 0; thisRef < n_refs; ++thisRef )
                aRefList[pair.first].AddItem( pair.second[thisRef] );
        }
    }
}


bool SCH_SHEET_LIST::TestForRecursion( const SCH_SHEET_LIST& aSrcSheetHierarchy,
                                       const wxString& aDestFileName )
{
    if( empty() )
        return false;

    SCHEMATIC* sch = at( 0 ).LastScreen()->Schematic();

    wxCHECK_MSG( sch, false, "No SCHEMATIC found in SCH_SHEET_LIST::TestForRecursion!" );

    wxFileName rootFn = sch->GetFileName();
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
                {
                    return true;
                }
            }
        }
    }

    // The source sheet file can safely be added to the destination sheet file.
    return false;
}


SCH_SHEET_PATH* SCH_SHEET_LIST::FindSheetForScreen( const SCH_SCREEN* aScreen )
{
    for( SCH_SHEET_PATH& sheetpath : *this )
    {
        if( sheetpath.LastScreen() == aScreen )
            return &sheetpath;
    }

    return nullptr;
}


SCH_SHEET_LIST SCH_SHEET_LIST::FindAllSheetsForScreen( const SCH_SCREEN* aScreen ) const
{
    SCH_SHEET_LIST retval;

    for( const SCH_SHEET_PATH& sheetpath : *this )
    {
        if( sheetpath.LastScreen() == aScreen )
            retval.push_back( sheetpath );
    }

    return retval;
}


void SCH_SHEET_LIST::UpdateSymbolInstances(
                                const std::vector<SYMBOL_INSTANCE_REFERENCE>& aSymbolInstances )
{
    SCH_REFERENCE_LIST symbolInstances;

    GetSymbols( symbolInstances, true, true );

    std::map<KIID_PATH, wxString> pathNameCache;

    // Calculating the name of a path is somewhat expensive; on large designs with many symbols
    // this can blow up to a serious amount of time when loading the schematic
    auto getName = [&pathNameCache]( const KIID_PATH& aPath ) -> const wxString&
                   {
                       if( pathNameCache.count( aPath ) )
                           return pathNameCache.at( aPath );

                       pathNameCache[aPath] = aPath.AsString();
                       return pathNameCache[aPath];
                   };

    for( size_t i = 0; i < symbolInstances.GetCount(); i++ )
    {
        // The instance paths are stored in the file sans root path so the comparison
        // should not include the root path.
        wxString path = symbolInstances[i].GetPath();

        auto it = std::find_if( aSymbolInstances.begin(), aSymbolInstances.end(),
                                [ path, &getName ]( const SYMBOL_INSTANCE_REFERENCE& r ) -> bool
                                {
                                    return path == getName( r.m_Path );
                                } );

        if( it == aSymbolInstances.end() )
        {
            wxLogTrace( traceSchSheetPaths, "No symbol instance found for path \"%s\"", path );
            continue;
        }

        SCH_SYMBOL* symbol = symbolInstances[ i ].GetSymbol();

        wxCHECK2( symbol, continue );

        // Symbol instance paths are stored and looked up in memory with the root path so use
        // the full path here.
        symbol->AddHierarchicalReference( symbolInstances[i].GetSheetPath().Path(),
                                          it->m_Reference, it->m_Unit, it->m_Value,
                                          it->m_Footprint );
        symbol->GetField( REFERENCE_FIELD )->SetText( it->m_Reference );
    }
}


void SCH_SHEET_LIST::UpdateSheetInstances( const std::vector<SCH_SHEET_INSTANCE>& aSheetInstances )
{

    for( const SCH_SHEET_PATH& instance : *this )
    {
        auto it = std::find_if( aSheetInstances.begin(), aSheetInstances.end(),
                                [ instance ]( const SCH_SHEET_INSTANCE& r ) -> bool
                                {
                                    return instance.PathWithoutRootUuid() == r.m_Path;
                                } );

        if( it == aSheetInstances.end() )
        {
            wxLogTrace( traceSchSheetPaths, "No sheet instance found for path \"%s\"",
                        instance.PathWithoutRootUuid().AsString() );
            continue;
        }

        SCH_SHEET* sheet = instance.Last();

        wxCHECK2( sheet, continue );

        sheet->AddInstance( instance.Path() );
        sheet->SetPageNumber( instance, it->m_PageNumber );
    }
}


std::vector<KIID_PATH> SCH_SHEET_LIST::GetPaths() const
{
    std::vector<KIID_PATH> paths;

    for( const SCH_SHEET_PATH& sheetPath : *this )
        paths.emplace_back( sheetPath.Path() );

    return paths;
}


std::vector<SCH_SHEET_INSTANCE> SCH_SHEET_LIST::GetSheetInstances() const
{
    std::vector<SCH_SHEET_INSTANCE> retval;

    for( const SCH_SHEET_PATH& path : *this )
    {
        SCH_SHEET_INSTANCE instance;
        const SCH_SHEET* sheet = path.Last();

        wxCHECK2( sheet, continue );

        instance.m_Path = path.PathWithoutRootUuid();
        instance.m_PageNumber = sheet->GetPageNumber( path );

        retval.push_back( instance );
    }

    return retval;
}


bool SCH_SHEET_LIST::AllSheetPageNumbersEmpty() const
{
    for( const SCH_SHEET_PATH& instance : *this )
    {
        const SCH_SHEET* sheet = instance.Last();

        wxCHECK2( sheet, continue );

        if( !sheet->GetPageNumber( instance ).IsEmpty() )
            return false;
    }

    return true;
}


void SCH_SHEET_LIST::SetInitialPageNumbers()
{
    // Don't accidentally renumber existing sheets.
    wxCHECK( AllSheetPageNumbersEmpty(), /* void */ );

    wxString tmp;
    int pageNumber = 1;

    for( const SCH_SHEET_PATH& instance : *this )
    {
        SCH_SHEET* sheet = instance.Last();

        wxCHECK2( sheet, continue );

        sheet->AddInstance( instance.Path() );
        tmp.Printf( "%d", pageNumber );
        sheet->SetPageNumber( instance, tmp );
        pageNumber += 1;
    }
}
