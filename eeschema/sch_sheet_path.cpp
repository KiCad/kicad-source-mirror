/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <symbol_library.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <sch_sheet.h>
#include <sch_textbox.h>
#include <schematic.h>
#include <template_fieldnames.h>
#include <trace_helpers.h>

#include <boost/functional/hash.hpp>
#include <wx/filename.h>
#include <wx/log.h>
#include "erc_item.h"

#include <sim/sim_model.h> // For V6 to V7 simulation model migration.
#include <sim/sim_value.h> //
#include <locale_io.h>


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

    wxString GetSelectMenuText( UNITS_PROVIDER* aUnitsProvider ) const override
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
    void SetPosition( const VECTOR2I& ) override {}
    void Print( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset ) override {}
    void Move( const VECTOR2I& aMoveVector ) override {}
    void MirrorHorizontally( int aCenter ) override {}
    void MirrorVertically( int aCenter ) override {}
    void Rotate( const VECTOR2I& aCenter ) override {}

#if defined(DEBUG)
    void Show( int , std::ostream&  ) const override {}
#endif
};


bool SortSymbolInstancesByProjectUuid( const SYMBOL_INSTANCE_REFERENCE& aLhs,
                                       const SYMBOL_INSTANCE_REFERENCE& aRhs )
{
    wxCHECK( !aLhs.m_Path.empty() && !aRhs.m_Path.empty(), false );

    return aLhs.m_Path[0] < aRhs.m_Path[0];
}


namespace std
{
    size_t hash<SCH_SHEET_PATH>::operator()( const SCH_SHEET_PATH& path ) const
    {
        return path.GetCurrentHash();
    }
}


SCH_SHEET_PATH::SCH_SHEET_PATH()
{
    m_virtualPageNumber = 1;
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


SCH_SHEET_PATH SCH_SHEET_PATH::operator+( const SCH_SHEET_PATH& aOther )
{
    SCH_SHEET_PATH retv = *this;

    size_t size = aOther.size();

    for( size_t i = 0; i < size; i++ )
        retv.push_back( aOther.at( i ) );

    return retv;
}


void SCH_SHEET_PATH::initFromOther( const SCH_SHEET_PATH& aOther )
{
    m_sheets            = aOther.m_sheets;
    m_virtualPageNumber = aOther.m_virtualPageNumber;
    m_current_hash      = aOther.m_current_hash;

    // Note: don't copy m_recursion_test_cache as it is slow and we want SCH_SHEET_PATHS to be
    // very fast to construct for use in the connectivity algorithm.
}


bool SCH_SHEET_PATH::IsFullPath() const
{
    // The root sheet path is empty.  All other sheet paths must start with the root sheet path.
    return ( m_sheets.size() == 0 ) || ( GetSheet( 0 )->IsRootSheet() );
}


void SCH_SHEET_PATH::Rehash()
{
    m_current_hash = 0;

    for( SCH_SHEET* sheet : m_sheets )
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


int SCH_SHEET_PATH::ComparePageNum( const SCH_SHEET_PATH& aSheetPathToTest ) const
{
    wxString pageA = this->GetPageNumber();
    wxString pageB = aSheetPathToTest.GetPageNumber();

    int pageNumComp = SCH_SHEET::ComparePageNum( pageA, pageB );

    if( pageNumComp == 0 )
    {
        int virtualPageA = GetVirtualPageNumber();
        int virtualPageB = aSheetPathToTest.GetVirtualPageNumber();

        if( virtualPageA > virtualPageB )
            pageNumComp = 1;
        else if( virtualPageA < virtualPageB )
            pageNumComp = -1;
    }

    return pageNumComp;
}


bool SCH_SHEET_PATH::IsContainedWithin( const SCH_SHEET_PATH& aSheetPathToTest ) const
{
    if( aSheetPathToTest.size() > size() )
        return false;

    for( size_t i = 0; i < aSheetPathToTest.size(); ++i )
    {
        if( at( i )->m_Uuid != aSheetPathToTest.at( i )->m_Uuid )
        {
            wxLogTrace( traceSchSheetPaths, "Sheet path '%s' is not within path '%s'.",
                        aSheetPathToTest.Path().AsString(), Path().AsString() );

            return false;
        }
    }

    wxLogTrace( traceSchSheetPaths, "Sheet path '%s' is within path '%s'.",
                aSheetPathToTest.Path().AsString(), Path().AsString() );

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
    // Its timestamp changes anyway.
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


wxString SCH_SHEET_PATH::PathHumanReadable( bool aUseShortRootName ) const
{
    wxString s;

    if( aUseShortRootName )
    {
        s = wxS( "/" ); // Use only the short name in netlists
    }
    else
    {
        wxString fileName;

        if( !empty() && at( 0 )->GetScreen() )
            fileName = at( 0 )->GetScreen()->GetFileName();

        wxFileName fn = fileName;

        s = fn.GetName() + wxS( "/" );
    }

    // Start at 1 since we've already processed the root sheet.
    for( unsigned i = 1; i < size(); i++ )
        s << at( i )->GetFields()[SHEETNAME].GetShownText() << wxS( "/" );

    return s;
}


void SCH_SHEET_PATH::UpdateAllScreenReferences() const
{
    std::vector<SCH_ITEM*> symbols;

    std::copy_if( LastScreen()->Items().begin(),
                  LastScreen()->Items().end(),
                  std::back_inserter( symbols ),
            []( SCH_ITEM* aItem )
            {
                return ( aItem->Type() == SCH_SYMBOL_T );
            } );

    for( SCH_ITEM* item : symbols )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
        symbol->GetField( REFERENCE_FIELD )->SetText( symbol->GetRef( this ) );
        symbol->GetField( VALUE_FIELD )->SetText( symbol->GetValue( this, false ) );
        symbol->GetField( FOOTPRINT_FIELD )->SetText( symbol->GetFootprint( this, false ) );
        symbol->UpdateUnit( symbol->GetUnitSelection( this ) );
        LastScreen()->Update( item );
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
        LIB_SYMBOL* symbol = aSymbol->GetLibSymbolRef().get();

        if( symbol || aForceIncludeOrphanSymbols )
        {
            SCH_REFERENCE schReference( aSymbol, symbol, *this );

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

    LIB_SYMBOL* symbol = aSymbol->GetLibSymbolRef().get();

    if( symbol && symbol->GetUnitCount() > 1 )
    {
        SCH_REFERENCE schReference = SCH_REFERENCE( aSymbol, symbol, *this );
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

    SCH_SHEET_PATH tmpPath = *this;

    tmpPath.pop_back();

    return sheet->getPageNumber( tmpPath );
}


void SCH_SHEET_PATH::SetPageNumber( const wxString& aPageNumber )
{
    SCH_SHEET* sheet = Last();

    wxCHECK( sheet, /* void */ );

    SCH_SHEET_PATH tmpPath = *this;

    tmpPath.pop_back();

    sheet->AddInstance( tmpPath );
    sheet->setPageNumber( tmpPath, aPageNumber );
}


void SCH_SHEET_PATH::AddNewSymbolInstances( const SCH_SHEET_PATH& aPrefixSheetPath )
{
    for( SCH_ITEM* item : LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        wxCHECK2( symbol, continue );

        SYMBOL_INSTANCE_REFERENCE newSymbolInstance;
        SCH_SHEET_PATH newSheetPath( aPrefixSheetPath );
        SCH_SHEET_PATH currentSheetPath( *this );

        // Remove the root sheet from this path.
        currentSheetPath.m_sheets.erase( currentSheetPath.m_sheets.begin() );

        // Prefix the new hierarchical path.
        newSheetPath = newSheetPath + currentSheetPath;
        newSymbolInstance.m_Path = newSheetPath.Path();

        if( symbol->GetInstance( newSymbolInstance, Path() ) )
        {
            // Use an existing symbol instance for this path if it exists.
            newSymbolInstance.m_Path = newSheetPath.Path();
            symbol->AddHierarchicalReference( newSymbolInstance );
        }
        else if( !symbol->GetInstanceReferences().empty() )
        {
            // Use the first symbol instance if any symbol exists.
            newSymbolInstance = symbol->GetInstanceReferences()[0];
            newSymbolInstance.m_Path = newSheetPath.Path();
            symbol->AddHierarchicalReference( newSymbolInstance );
        }
        else if( symbol->GetLibSymbolRef() )
        {
            // Fall back to library symbol reference prefix, value, and footprint fields and
            // set the unit to 1 if the library symbol is valid.
            newSymbolInstance.m_Reference = symbol->GetLibSymbolRef()->GetReferenceField().GetText();
            newSymbolInstance.m_Reference += wxT( "?" );
            newSymbolInstance.m_Unit = 1;
            newSymbolInstance.m_Value = symbol->GetLibSymbolRef()->GetValueField().GetText();
            newSymbolInstance.m_Footprint = symbol->GetLibSymbolRef()->GetFootprintField().GetText();
            symbol->AddHierarchicalReference( newSymbolInstance );
        }
        else
        {
            // All hope is lost so set the reference to 'U' and the unit to 1.
            newSymbolInstance.m_Reference += wxT( "U?" );
            newSymbolInstance.m_Unit = 1;
            symbol->AddHierarchicalReference( newSymbolInstance );
        }
    }
}


void SCH_SHEET_PATH::RemoveSymbolInstances( const SCH_SHEET_PATH& aPrefixSheetPath )
{
    for( SCH_ITEM* item : LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        wxCHECK2( symbol, continue );

        SCH_SHEET_PATH fullSheetPath( aPrefixSheetPath );
        SCH_SHEET_PATH currentSheetPath( *this );

        // Prefix the hierarchical path of the symbol instance to be removed.
        fullSheetPath = fullSheetPath + currentSheetPath;
        symbol->RemoveInstance( fullSheetPath );
    }
}


int SCH_SHEET_PATH::AddNewSheetInstances( const SCH_SHEET_PATH& aPrefixSheetPath,
                                          int aNextVirtualPageNumber )
{
    int nextVirtualPageNumber = aNextVirtualPageNumber;

    for( SCH_ITEM* item : LastScreen()->Items().OfType( SCH_SHEET_T ) )
    {
        SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );

        wxCHECK2( sheet, continue );

        SCH_SHEET_PATH newSheetPath( aPrefixSheetPath );
        SCH_SHEET_PATH currentSheetPath( *this );

        // Remove the root sheet from current sheet path.
        currentSheetPath.m_sheets.erase( currentSheetPath.m_sheets.begin() );

        // Prefix the new hierarchical path.
        newSheetPath = newSheetPath + currentSheetPath;

        wxString pageNumber;

        pageNumber.Printf( wxT( "%d" ), nextVirtualPageNumber );
        sheet->AddInstance( newSheetPath );
        newSheetPath.SetVirtualPageNumber( nextVirtualPageNumber );
        newSheetPath.SetPageNumber( pageNumber );
        nextVirtualPageNumber += 1;
    }

    return nextVirtualPageNumber;
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

        if( aSheet->IsRootSheet() )
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
            int retval = a.ComparePageNum( b );

            if( retval < 0 )
                return true;
            else if( retval > 0 )
                return false;
            else /// Enforce strict ordering.  If the page numbers are the same, use UUIDs
                return a.GetCurrentHash() < b.GetCurrentHash();
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
            LIB_SYMBOL* libSymbol = symbol->GetLibSymbolRef().get();

            if( libSymbol && libSymbol->IsPower() )
            {
                SCH_REFERENCE schReference( symbol, libSymbol, sheet );
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


std::optional<SCH_SHEET_PATH> SCH_SHEET_LIST::GetSheetPathByKIIDPath( const KIID_PATH& aPath ) const
{
    for( const SCH_SHEET_PATH& sheet : *this )
    {
        if( sheet.Path() == aPath )
            return SCH_SHEET_PATH( sheet );
    }

    return std::nullopt;
}


void SCH_SHEET_LIST::GetMultiUnitSymbols( SCH_MULTI_UNIT_REFERENCE_MAP &aRefList,
                                          bool aIncludePowerSymbols ) const
{
    for( SCH_SHEET_PATHS::const_iterator it = begin(); it != end(); ++it )
    {
        SCH_MULTI_UNIT_REFERENCE_MAP tempMap;
        ( *it ).GetMultiUnitSymbols( tempMap, aIncludePowerSymbols );

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


SCH_SHEET_PATH* SCH_SHEET_LIST::FindSheetForPath( const SCH_SHEET_PATH* aPath )
{
    for( SCH_SHEET_PATH& path : *this )
    {
        if( path.Path() == aPath->Path() )
            return &path;
    }

    return nullptr;
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
    for( SCH_SHEET_PATH& sheetPath : *this )
    {
        for( SCH_ITEM* item : sheetPath.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            wxCHECK2( symbol, continue );

            KIID_PATH sheetPathWithSymbolUuid = sheetPath.Path();
            sheetPathWithSymbolUuid.push_back( symbol->m_Uuid );

            auto it = std::find_if( aSymbolInstances.begin(), aSymbolInstances.end(),
                                    [ sheetPathWithSymbolUuid ]( const SYMBOL_INSTANCE_REFERENCE& r ) -> bool
                                    {
                                        return sheetPathWithSymbolUuid == r.m_Path;
                                    } );

            if( it == aSymbolInstances.end() )
            {
                wxLogTrace( traceSchSheetPaths, "No symbol instance found for symbol '%s'",
                            sheetPathWithSymbolUuid.AsString() );
                continue;
            }

            // Symbol instance paths are stored and looked up in memory with the root path so use
            // the full path here.
            symbol->AddHierarchicalReference( sheetPath.Path(),
                                              it->m_Reference, it->m_Unit, it->m_Value,
                                              it->m_Footprint );
            symbol->GetField( REFERENCE_FIELD )->SetText( it->m_Reference );
        }
    }

    if( size() >= 1 && at( 0 ).LastScreen()->GetFileFormatVersionAtLoad() < 20220622 )
        MigrateSimModelNameFields();
}


void SCH_SHEET_LIST::UpdateSheetInstances( const std::vector<SCH_SHEET_INSTANCE>& aSheetInstances )
{

    for( SCH_SHEET_PATH& path : *this )
    {
        SCH_SHEET* sheet = path.Last();

        wxCHECK2( sheet && path.Last(), continue );

        auto it = std::find_if( aSheetInstances.begin(), aSheetInstances.end(),
                                [ path ]( const SCH_SHEET_INSTANCE& r ) -> bool
                                {
                                    return path.Path() == r.m_Path;
                                } );

        if( it == aSheetInstances.end() )
        {
            wxLogTrace( traceSchSheetPaths, "No sheet instance found for path '%s'",
                        path.Path().AsString() );
            continue;
        }

        wxLogTrace( traceSchSheetPaths, "Setting sheet '%s' instance '%s' page number '%s'",
                    ( sheet->GetName().IsEmpty() ) ? wxT( "root" ) : sheet->GetName(),
                    path.Path().AsString(), it->m_PageNumber );
        path.SetPageNumber( it->m_PageNumber );
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
        const SCH_SHEET* sheet = path.Last();

        wxCHECK2( sheet, continue );

        SCH_SHEET_INSTANCE instance;
        SCH_SHEET_PATH tmpPath = path;

        tmpPath.pop_back();
        instance.m_Path = tmpPath.Path();
        instance.m_PageNumber = path.GetPageNumber();

        retval.push_back( instance );
    }

    return retval;
}


bool SCH_SHEET_LIST::AllSheetPageNumbersEmpty() const
{
    for( const SCH_SHEET_PATH& instance : *this )
    {
        if( !instance.GetPageNumber().IsEmpty() )
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

    for( SCH_SHEET_PATH& instance : *this )
    {
        tmp.Printf( "%d", pageNumber );
        instance.SetPageNumber( tmp );
        pageNumber += 1;
    }
}


void SCH_SHEET_LIST::AddNewSymbolInstances( const SCH_SHEET_PATH& aPrefixSheetPath )
{
    for( SCH_SHEET_PATH& sheetPath : *this )
        sheetPath.AddNewSymbolInstances( aPrefixSheetPath );
}


void SCH_SHEET_LIST::RemoveSymbolInstances( const SCH_SHEET_PATH& aPrefixSheetPath )
{
    for( SCH_SHEET_PATH& sheetPath : *this )
        sheetPath.RemoveSymbolInstances( aPrefixSheetPath );
}


void SCH_SHEET_LIST::AddNewSheetInstances( const SCH_SHEET_PATH& aPrefixSheetPath,
                                           int aLastVirtualPageNumber )
{
    int nextVirtualPageNumber = aLastVirtualPageNumber + 1;

    for( SCH_SHEET_PATH& sheetPath : *this )
        nextVirtualPageNumber = sheetPath.AddNewSheetInstances( aPrefixSheetPath,
                                                                nextVirtualPageNumber );
}


int SCH_SHEET_LIST::GetLastVirtualPageNumber() const
{
    int lastVirtualPageNumber = 1;

    for( const SCH_SHEET_PATH& sheetPath : *this )
    {
        if( sheetPath.GetVirtualPageNumber() > lastVirtualPageNumber )
            lastVirtualPageNumber = sheetPath.GetVirtualPageNumber();
    }

    return lastVirtualPageNumber;
}


void SCH_SHEET_LIST::MigrateSimModelNameFields()
{
    LOCALE_IO toggle;

    for( unsigned sheetIndex = 0; sheetIndex < size(); ++sheetIndex )
    {
        SCH_SCREEN* screen = at( sheetIndex ).LastScreen();

        // V6 schematics may specify model names in Value fields, which we don't do in V7.
        // Migrate by adding an equivalent model for these symbols.

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( !symbol )
            {
                // Shouldn't happen.
                wxFAIL;
                continue;
            }

            if( symbol->FindField( "Spice_Primitive" )
                || symbol->FindField( "Spice_Node_Sequence" )
                || symbol->FindField( "Spice_Model" )
                || symbol->FindField( "Spice_Netlist_Enabled" )
                || symbol->FindField( "Spice_Lib_File" ) )
            {
                // Has a legacy raw (plaintext) model -- this is handled in the SIM_MODEL class.
                continue;
            }

            if( symbol->FindField( SIM_MODEL::DEVICE_TYPE_FIELD )
                || symbol->FindField( SIM_MODEL::TYPE_FIELD )
                || symbol->FindField( SIM_MODEL::PINS_FIELD )
                || symbol->FindField( SIM_MODEL::PARAMS_FIELD ) )
            {
                // Has a V7 model field - skip.
                continue;
            }

            // Insert a plaintext model as a substitute.

            wxString refdes = symbol->GetRef( &at( sheetIndex ), true );

            if( refdes.Length() == 0 )
                continue; // No refdes? We need the first character to determine type. Skip.

            wxString value = symbol->GetValue( &at( sheetIndex ), true );

            if( refdes.StartsWith( "R" )
                || refdes.StartsWith( "C" )
                || refdes.StartsWith( "L" ) )
            {
                // This is taken from the former Spice exporter.
                wxRegEx passiveVal(
                    wxT( "^([0-9\\. ]+)([fFpPnNuUmMkKgGtTμµ𝛍𝜇𝝁 ]|M(e|E)(g|G))?([fFhHΩΩ𝛀𝛺𝝮]|ohm)?([-1-9 ]*)$" ) );

                if( passiveVal.Matches( value ) )
                {
                    wxString prefix( passiveVal.GetMatch( value, 1 ) );
                    wxString unit( passiveVal.GetMatch( value, 2 ) );
                    wxString suffix( passiveVal.GetMatch( value, 6 ) );

                    prefix.Trim(); prefix.Trim( false );
                    unit.Trim(); unit.Trim( false );
                    suffix.Trim(); suffix.Trim( false );

                    // Make 'mega' units comply with the Spice expectations
                    if( unit == "M" )
                        unit = "Meg";

                    std::unique_ptr<SIM_VALUE> simValue =
                        SIM_VALUE::Create( SIM_VALUE::TYPE_FLOAT );
                    simValue->FromString( std::string( ( prefix + unit + suffix ).ToUTF8() ),
                                          SIM_VALUE::NOTATION::SPICE );
                }
            }

            SCH_FIELD deviceTypeField( VECTOR2I( 0, 0 ), symbol->GetFieldCount(), symbol,
                                       SIM_MODEL::DEVICE_TYPE_FIELD );
            deviceTypeField.SetText(
                SIM_MODEL::DeviceInfo( SIM_MODEL::DEVICE_TYPE_::SPICE ).fieldValue );
            symbol->AddField( deviceTypeField );

            SCH_FIELD modelParamsField( VECTOR2I( 0, 0 ), symbol->GetFieldCount(), symbol,
                                        SIM_MODEL::PARAMS_FIELD );
            modelParamsField.SetText( wxString::Format( "type=%s model=\"%s\"",
                                                        refdes.Left( 1 ),
                                                        value ) );
            symbol->AddField( modelParamsField );
        }
    }
}
