/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <refdes_utils.h>
#include <hash.h>
#include <sch_screen.h>
#include <sch_marker.h>
#include <sch_label.h>
#include <sch_shape.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <sch_sheet.h>
#include <schematic.h>
#include <template_fieldnames.h>
#include <trace_helpers.h>

#include <wx/filename.h>
#include <wx/log.h>


/**
 * A singleton item of this class is returned for a weak reference that no longer exists.
 *
 * Its sole purpose is to flag the item as having been deleted.
 */
class DELETED_SHEET_ITEM : public SCH_ITEM
{
public:
    DELETED_SHEET_ITEM() :
        SCH_ITEM( nullptr, NOT_USED )
    {}

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override
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
    void Move( const VECTOR2I& aMoveVector ) override {}
    void MirrorHorizontally( int aCenter ) override {}
    void MirrorVertically( int aCenter ) override {}
    void Rotate( const VECTOR2I& aCenter, bool aRotateCCW ) override {}

    double Similarity( const SCH_ITEM& aOther ) const override
    {
        return 0.0;
    }

    bool operator==( const SCH_ITEM& aOther ) const override
    {
        return false;
    }

#if defined(DEBUG)
    void Show( int , std::ostream&  ) const override {}
#endif
};


void SCH_SYMBOL_VARIANT::InitializeAttributes( const SCH_SYMBOL& aSymbol )
{
    m_DNP = aSymbol.GetDNP();
    m_ExcludedFromBOM = aSymbol.GetExcludedFromBOM();
    m_ExcludedFromSim = aSymbol.GetExcludedFromSim();
    m_ExcludedFromBoard = aSymbol.GetExcludedFromBoard();
    m_ExcludedFromPosFiles = aSymbol.GetExcludedFromPosFiles();
}


void SCH_SHEET_VARIANT::InitializeAttributes( const SCH_SHEET& aSheet )
{
    m_DNP = aSheet.GetDNP();
    m_ExcludedFromBOM = aSheet.GetExcludedFromBOM();
    m_ExcludedFromSim = aSheet.GetExcludedFromSim();
    m_ExcludedFromBoard = aSheet.GetExcludedFromBoard();
    m_ExcludedFromPosFiles = false;  // Sheets don't have position files exclusion
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


// Move assignment operator
SCH_SHEET_PATH& SCH_SHEET_PATH::operator=( SCH_SHEET_PATH&& aOther )
{
    m_sheets = std::move( aOther.m_sheets );

    m_virtualPageNumber  = aOther.m_virtualPageNumber;
    m_current_hash       = aOther.m_current_hash;
    m_cached_page_number = aOther.m_cached_page_number;

    m_recursion_test_cache = std::move( aOther.m_recursion_test_cache );

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
    m_sheets             = aOther.m_sheets;
    m_virtualPageNumber  = aOther.m_virtualPageNumber;
    m_current_hash       = aOther.m_current_hash;
    m_cached_page_number = aOther.m_cached_page_number;

    // Note: don't copy m_recursion_test_cache as it is slow and we want std::vector<SCH_SHEET_PATH>
    // to be very fast to construct for use in the connectivity algorithm.
    m_recursion_test_cache.clear();
}

void SCH_SHEET_PATH::Rehash()
{
    m_current_hash = 0;

    for( SCH_SHEET* sheet : m_sheets )
        hash_combine( m_current_hash, sheet->m_Uuid.Hash() );
}


int SCH_SHEET_PATH::Cmp( const SCH_SHEET_PATH& aSheetPathToTest ) const
{
    if( size() > aSheetPathToTest.size() )
        return 1;

    if( size() < aSheetPathToTest.size() )
        return -1;

    // otherwise, same number of sheets.
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


bool SCH_SHEET_PATH::GetExcludedFromSim() const
{
    for( SCH_SHEET* sheet : m_sheets )
    {
        if( sheet->GetExcludedFromSim() )
            return true;
    }

    return false;
}


bool SCH_SHEET_PATH::GetExcludedFromSim( const wxString& aVariantName ) const
{
    if( aVariantName.IsEmpty() )
        return GetExcludedFromSim();

    for( SCH_SHEET* sheet : m_sheets )
    {
        if( sheet->GetExcludedFromSim( this, aVariantName ) )
            return true;
    }

    return false;
}


bool SCH_SHEET_PATH::GetExcludedFromBOM() const
{
    for( SCH_SHEET* sheet : m_sheets )
    {
        if( sheet->GetExcludedFromBOM() )
            return true;
    }

    return false;
}


bool SCH_SHEET_PATH::GetExcludedFromBOM( const wxString& aVariantName ) const
{
    if( aVariantName.IsEmpty() )
        return GetExcludedFromBOM();

    for( SCH_SHEET* sheet : m_sheets )
    {
        if( sheet->GetExcludedFromBOM( this, aVariantName ) )
            return true;
    }

    return false;
}


bool SCH_SHEET_PATH::GetExcludedFromBoard() const
{
    for( SCH_SHEET* sheet : m_sheets )
    {
        if( sheet->GetExcludedFromBoard() )
            return true;
    }

    return false;
}


bool SCH_SHEET_PATH::GetDNP() const
{
    for( SCH_SHEET* sheet : m_sheets )
    {
        if( sheet->GetDNP() )
            return true;
    }

    return false;
}


bool SCH_SHEET_PATH::GetDNP( const wxString& aVariantName ) const
{
    if( aVariantName.IsEmpty() )
        return GetDNP();

    for( SCH_SHEET* sheet : m_sheets )
    {
        if( sheet->GetDNP( this, aVariantName ) )
            return true;
    }

    return false;
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
    size_t size = m_sheets.size();

    if( m_sheets.empty() )
        return path;

    if( m_sheets[0]->m_Uuid != niluuid )
    {
        path.reserve( size );
        path.push_back( m_sheets[0]->m_Uuid );
    }
    else
    {
        // Skip the virtual root
        path.reserve( size - 1 );
    }

    for( size_t i = 1; i < size; i++ )
        path.push_back( m_sheets[i]->m_Uuid );

    return path;
}


wxString SCH_SHEET_PATH::PathHumanReadable( bool aUseShortRootName,
                                            bool aStripTrailingSeparator ) const
{
    wxString s;

    // Determine the starting index - skip virtual root if present
    size_t startIdx = 0;

    if( !empty() && at( 0 )->IsVirtualRootSheet() )
        startIdx = 1;

    if( aUseShortRootName )
    {
        s = wxS( "/" ); // Use only the short name in netlists
    }
    else
    {
        wxString fileName;

        if( size() > startIdx && at( startIdx )->GetScreen() )
            fileName = at( startIdx )->GetScreen()->GetFileName();

        wxFileName fn = fileName;

        s = fn.GetName() + wxS( "/" );
    }

    // Start at startIdx + 1 since we've already processed the root sheet.
    for( unsigned i = startIdx + 1; i < size(); i++ )
        s << at( i )->GetField( FIELD_T::SHEET_NAME )->GetShownText( false ) << wxS( "/" );

    if( aStripTrailingSeparator && s.EndsWith( "/" ) )
        s = s.Left( s.length() - 1 );

    return s;
}


void SCH_SHEET_PATH::UpdateAllScreenReferences() const
{
    std::vector<SCH_ITEM*> items;

    std::copy_if( LastScreen()->Items().begin(), LastScreen()->Items().end(),
                  std::back_inserter( items ),
            []( SCH_ITEM* aItem )
            {
                return ( aItem->Type() == SCH_SYMBOL_T
                        || aItem->Type() == SCH_GLOBAL_LABEL_T
                        || aItem->Type() == SCH_SHAPE_T );
            } );

    std::optional<wxString> variantName;
    const SCHEMATIC* schematic = LastScreen()->Schematic();

    if( schematic )
        variantName = schematic->GetCurrentVariant();

    for( SCH_ITEM* item : items )
    {
        if( item->Type() == SCH_SYMBOL_T )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            symbol->GetField( FIELD_T::REFERENCE )->SetText( symbol->GetRef( this ) );
            symbol->SetUnit( symbol->GetUnitSelection( this ) );
            LastScreen()->Update( item, false );
        }
        else if( item->Type() == SCH_GLOBAL_LABEL_T )
        {
            SCH_GLOBALLABEL* label = static_cast<SCH_GLOBALLABEL*>( item );

            if( label->GetFields().size() > 0 ) // Possible when reading a legacy .sch schematic
            {
                SCH_FIELD* intersheetRefs = label->GetField( FIELD_T::INTERSHEET_REFS );

                // Fixup for legacy files which didn't store a position for the intersheet refs
                // unless they were shown.
                if( intersheetRefs->GetPosition() == VECTOR2I() && !intersheetRefs->IsVisible() )
                    label->AutoplaceFields( LastScreen(), AUTOPLACE_AUTO );

                intersheetRefs->SetVisible( label->Schematic()->Settings().m_IntersheetRefsShow );
                LastScreen()->Update( intersheetRefs );
            }
        }
        else if( item->Type() == SCH_SHAPE_T )
        {
            SCH_SHAPE* shape = static_cast<SCH_SHAPE*>( item );
            shape->UpdateHatching();
        }
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
        if( aSymbol->GetLibSymbolRef() || aForceIncludeOrphanSymbols )
        {
            SCH_REFERENCE schReference( aSymbol, *this );

            schReference.SetSheetNumber( GetPageNumberAsInt() );
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
        SCH_REFERENCE schReference = SCH_REFERENCE( aSymbol, *this );
        schReference.SetSheetNumber( GetPageNumberAsInt() );
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

    KIID_PATH tmpPath = Path();

    if( !tmpPath.empty() )
        tmpPath.pop_back();
    else
        wxFAIL_MSG( wxS( "Sheet paths must have a least one valid sheet." ) );

    return sheet->getPageNumber( tmpPath );
}

int SCH_SHEET_PATH::GetPageNumberAsInt() const
{
    long page;
    wxString pageStr = GetPageNumber();

    if( pageStr.ToLong( &page ) )
        return (int) page;

    return GetVirtualPageNumber();
}


void SCH_SHEET_PATH::SetPageNumber( const wxString& aPageNumber )
{
    SCH_SHEET* sheet = Last();

    wxCHECK( sheet, /* void */ );

    KIID_PATH tmpPath = Path();

    if( !tmpPath.empty() )
    {
        tmpPath.pop_back();
    }
    else
    {
        wxCHECK_MSG( false, /* void */, wxS( "Sheet paths must have a least one valid sheet." ) );
    }

    sheet->addInstance( tmpPath );
    sheet->setPageNumber( tmpPath, aPageNumber );
}


void SCH_SHEET_PATH::AddNewSymbolInstances( const SCH_SHEET_PATH& aPrefixSheetPath,
                                            const wxString& aProjectName )
{
    wxCHECK( !aProjectName.IsEmpty(), /* void */ );

    SCH_SHEET_PATH newSheetPath( aPrefixSheetPath );
    SCH_SHEET_PATH currentSheetPath( *this );

    // Prefix the new hierarchical path.
    newSheetPath = newSheetPath + currentSheetPath;

    for( SCH_ITEM* item : LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        wxCHECK2( symbol, continue );

        SCH_SYMBOL_INSTANCE newSymbolInstance;

        if( symbol->GetInstance( newSymbolInstance, Path(), true ) )
        {
            newSymbolInstance.m_ProjectName = aProjectName;

            // Use an existing symbol instance for this path if it exists.
            newSymbolInstance.m_Path = newSheetPath.Path();
            symbol->AddHierarchicalReference( newSymbolInstance );
        }
        else if( !symbol->GetInstances().empty() )
        {
            newSymbolInstance.m_ProjectName = aProjectName;

            // Use the first symbol instance if any symbol instance data exists.
            newSymbolInstance = symbol->GetInstances()[0];
            newSymbolInstance.m_Path = newSheetPath.Path();
            symbol->AddHierarchicalReference( newSymbolInstance );
        }
        else
        {
            newSymbolInstance.m_ProjectName = aProjectName;

            // Fall back to the last saved symbol field and unit settings if there is no
            // instance data.
            newSymbolInstance.m_Path = newSheetPath.Path();
            newSymbolInstance.m_Reference = symbol->GetField( FIELD_T::REFERENCE )->GetText();
            newSymbolInstance.m_Unit = symbol->GetUnit();
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


void SCH_SHEET_PATH::CheckForMissingSymbolInstances( const wxString& aProjectName )
{
    // Skip sheet paths without screens (e.g., sheets that haven't been loaded yet or virtual root)
    if( aProjectName.IsEmpty() || !LastScreen() )
        return;

    wxLogTrace( traceSchSheetPaths, "CheckForMissingSymbolInstances for path: %s (project: %s)",
                PathHumanReadable( false ), aProjectName );
    wxLogTrace( traceSchSheetPaths, "  Sheet path size=%zu, Path().AsString()='%s'",
                size(), Path().AsString() );

    for( SCH_ITEM* item : LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        wxCHECK2( symbol, continue );

        SCH_SYMBOL_INSTANCE symbolInstance;

        if( !symbol->GetInstance( symbolInstance, Path() ) )
        {
            wxLogTrace( traceSchSheetPaths, "Adding missing symbol \"%s\" instance data for "
                        "sheet path '%s'.",
                        symbol->m_Uuid.AsString(), PathHumanReadable( false ) );

            // Legacy schematics that are not shared do not contain separate instance data.
            // The symbol reference and unit are saved in the reference field and unit entries.
            if( ( LastScreen()->GetRefCount() <= 1 ) &&
                ( LastScreen()->GetFileFormatVersionAtLoad() <= 20200310 ) )
            {
                SCH_FIELD* refField = symbol->GetField( FIELD_T::REFERENCE );
                symbolInstance.m_Reference = refField->GetShownText( this, true );
                symbolInstance.m_Unit = symbol->GetUnit();

                wxLogTrace( traceSchSheetPaths,
                           "  Legacy format: Using reference '%s' from field, unit %d",
                           symbolInstance.m_Reference, symbolInstance.m_Unit );
            }
            else if( !symbol->GetInstances().empty() )
            {
                // When a schematic is opened as a different project (e.g., a subsheet opened
                // directly from File Browser), use the first available instance data.
                // This provides better UX than showing unannotated references.
                const SCH_SYMBOL_INSTANCE& firstInstance = symbol->GetInstances()[0];
                symbolInstance.m_Reference = firstInstance.m_Reference;
                symbolInstance.m_Unit = firstInstance.m_Unit;

                wxLogTrace( traceSchSheetPaths,
                           "  Using first available instance: ref=%s, unit=%d",
                           symbolInstance.m_Reference, symbolInstance.m_Unit );
            }
            else
            {
                // Fall back to the symbol's reference field and unit if no instance data exists.
                SCH_FIELD* refField = symbol->GetField( FIELD_T::REFERENCE );
                symbolInstance.m_Reference = refField->GetText();
                symbolInstance.m_Unit = symbol->GetUnit();

                wxLogTrace( traceSchSheetPaths,
                           "  No instance data: Using reference '%s' from field, unit %d",
                           symbolInstance.m_Reference, symbolInstance.m_Unit );
            }

            symbolInstance.m_ProjectName = aProjectName;
            symbolInstance.m_Path = Path();
            symbol->AddHierarchicalReference( symbolInstance );

            wxLogTrace( traceSchSheetPaths,
                       "  Created instance: ref=%s, path=%s",
                       symbolInstance.m_Reference, symbolInstance.m_Path.AsString() );
        }
        else
        {
            wxLogTrace( traceSchSheetPaths,
                       "  Symbol %s already has instance: ref=%s, path=%s",
                       symbol->m_Uuid.AsString(),
                       symbolInstance.m_Reference,
                       symbolInstance.m_Path.AsString() );
        }
    }
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


SCH_SHEET_LIST::SCH_SHEET_LIST( SCH_SHEET* aSheet )
{
    if( aSheet != nullptr )
        BuildSheetList( aSheet, false );
}


void SCH_SHEET_LIST::BuildSheetList( SCH_SHEET* aSheet, bool aCheckIntegrity )
{
    if( !aSheet )
        return;

    wxLogTrace( traceSchSheetPaths,
               "BuildSheetList called with sheet '%s' (UUID=%s, isVirtualRoot=%d)",
               aSheet->GetName(),
               aSheet->m_Uuid.AsString(),
               aSheet->m_Uuid == niluuid ? 1 : 0 );

    // Special handling for virtual root: process its children without adding the root itself
    if( aSheet->IsVirtualRootSheet() )
    {
        wxLogTrace( traceSchSheetPaths, "  Skipping virtual root, processing children only" );

        if( aSheet->GetScreen() )
        {
            std::vector<SCH_ITEM*> childSheets;
            aSheet->GetScreen()->GetSheets( &childSheets );

            for( SCH_ITEM* item : childSheets )
            {
                SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );
                BuildSheetList( sheet, aCheckIntegrity );
            }
        }

        return;
    }

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
                // If we are not performing a full recursion test, at least check if we are in
                // a simple recursion scenario to prevent stack overflow crashes
                wxCHECK2_MSG( sheet->GetFileName() != aSheet->GetFileName(), continue,
                              wxT( "Recursion prevented in SCH_SHEET_LIST::BuildSheetList" ) );

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


void SCH_SHEET_LIST::SortByHierarchicalPageNumbers( bool aUpdateVirtualPageNums )
{
    for( const SCH_SHEET_PATH& path : *this )
        path.CachePageNumber();

    std::sort( begin(), end(),
        []( const SCH_SHEET_PATH& a, const SCH_SHEET_PATH& b ) -> bool
        {
            // Find the divergence point in the paths
            size_t common_len = 0;
            size_t min_len = std::min( a.size(), b.size() );

            while( common_len < min_len && a.at( common_len )->m_Uuid == b.at( common_len )->m_Uuid )
                common_len++;

            // If one path is a prefix of the other, the shorter one comes first
            // This ensures parents come before children
            if( common_len == a.size() )
                return true;  // a is a prefix of b - a is the parent
            if( common_len == b.size() )
                return false; // b is a prefix of a - b is the parent

            // Paths diverge at common_len
            // If they share the same parent, sort by page number
            // This ensures siblings are sorted by page number
            SCH_SHEET* sheet_a = a.at( common_len );
            SCH_SHEET* sheet_b = b.at( common_len );

            // Create partial paths to get to these sheets for page number comparison
            KIID_PATH ancestor;
            for( size_t i = 0; i < common_len; i++ )
                ancestor.push_back( a.at( i )->m_Uuid );

            // Compare page numbers - use the last sheet's page number
            wxString page_a = sheet_a->getPageNumber( ancestor );
            wxString page_b = sheet_b->getPageNumber( ancestor );

            int retval = SCH_SHEET::ComparePageNum( page_a, page_b );

            if( retval != 0 )
                return retval < 0;

            // If page numbers are the same, use virtual page numbers as a tie-breaker
            if( a.GetVirtualPageNumber() < b.GetVirtualPageNumber() )
                return true;
            else if( a.GetVirtualPageNumber() > b.GetVirtualPageNumber() )
                return false;

            // Finally, use UUIDs for stable ordering when everything else is equal
            return a.GetCurrentHash() < b.GetCurrentHash();
        } );

    if( aUpdateVirtualPageNums )
    {
        int virtualPageNum = 1;

        for( SCH_SHEET_PATH& sheet : *this )
            sheet.SetVirtualPageNumber( virtualPageNum++ );
    }
}


void SCH_SHEET_LIST::SortByPageNumbers( bool aUpdateVirtualPageNums )
{
    for( const SCH_SHEET_PATH& path : *this )
        path.CachePageNumber();

    std::sort( begin(), end(),
        []( const SCH_SHEET_PATH& a, const SCH_SHEET_PATH& b ) -> bool
        {
            int retval = SCH_SHEET::ComparePageNum( a.GetCachedPageNumber(),
                                                    b.GetCachedPageNumber() );

            if( retval < 0 )
                return true;
            else if( retval > 0 )
                return false;

            if( a.GetVirtualPageNumber() < b.GetVirtualPageNumber() )
                return true;
            else if( a.GetVirtualPageNumber() > b.GetVirtualPageNumber() )
                return false;

            // Enforce strict ordering.  If the page numbers are the same, use UUIDs
            return a.GetCurrentHash() < b.GetCurrentHash();
        } );

    if( aUpdateVirtualPageNums )
    {
        int virtualPageNum = 1;

        for( SCH_SHEET_PATH& sheet : *this )
            sheet.SetVirtualPageNumber( virtualPageNum++ );
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


void SCH_SHEET_LIST::TrimToPageNumbers( const std::vector<wxString>& aPageInclusions )
{
    auto it = std::remove_if( begin(), end(),
                              [&]( const SCH_SHEET_PATH& sheet )
                              {
                                  return std::find( aPageInclusions.begin(),
                                                    aPageInclusions.end(),
                                                    sheet.GetPageNumber() ) == aPageInclusions.end();
                              } );

    erase( it, end() );
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


SCH_ITEM* SCH_SHEET_LIST::ResolveItem( const KIID& aID, SCH_SHEET_PATH* aPathOut, bool aAllowNullptrReturn ) const
{
    for( const SCH_SHEET_PATH& sheet : *this )
    {
        SCH_ITEM* item = sheet.ResolveItem( aID );

        if( item )
        {
            if( aPathOut )
                *aPathOut = sheet;

            return item;
        }
    }

    // Not found; weak reference has been deleted.
    if( aAllowNullptrReturn )
        return nullptr;
    else
        return DELETED_SHEET_ITEM::GetInstance();
}


SCH_ITEM* SCH_SHEET_PATH::ResolveItem( const KIID& aID ) const
{
    for( SCH_ITEM* aItem : LastScreen()->Items() )
    {
        if( aItem->m_Uuid == aID )
            return aItem;

        SCH_ITEM* childMatch = nullptr;

        aItem->RunOnChildren(
                [&]( SCH_ITEM* aChild )
                {
                    if( aChild->m_Uuid == aID )
                        childMatch = aChild;
                },
                RECURSE_MODE::NO_RECURSE );

        if( childMatch )
            return childMatch;
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

            aItem->RunOnChildren(
                    [&]( SCH_ITEM* aChild )
                    {
                        aMap[ aChild->m_Uuid ] = aChild;
                    },
                    RECURSE_MODE::NO_RECURSE );
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
                SCH_REFERENCE schReference( symbol, sheet );
                references.AddItem( schReference );
            }
        }
    }

    // Find duplicate, and silently clear annotation of duplicate
    std::map<wxString, int> ref_list;   // stores the existing references

    for( unsigned ii = 0; ii< references.GetCount(); ++ii )
    {
        wxString curr_ref = references[ii].GetRef();

        if( curr_ref.IsEmpty() )
            continue;

        if( ref_list.find( curr_ref ) == ref_list.end() )
        {
            ref_list[curr_ref] = ii;
            continue;
        }

        // Possible duplicate, if the ref ends by a number:
        if( curr_ref.Last() < '0' && curr_ref.Last() > '9' )
            continue;   // not annotated

        // Duplicate: clear annotation by removing the number ending the ref
        while( !curr_ref.IsEmpty() && curr_ref.Last() >= '0' && curr_ref.Last() <= '9' )
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
        SCH_REFERENCE& ref_unit = references[ii];

        if( ref_unit.GetRef()[0] != '#' )
        {
            wxString new_ref = "#" + ref_unit.GetRef();
            ref_unit.SetRef( new_ref );
            ref_unit.SetRefNum( ii );
        }
    }
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


void SCH_SHEET_LIST::GetSheetsWithinPath( std::vector<SCH_SHEET_PATH>& aSheets,
                                          const SCH_SHEET_PATH& aSheetPath ) const
{
    for( const SCH_SHEET_PATH& sheet : *this )
    {
        if( sheet.IsContainedWithin( aSheetPath ) )
            aSheets.push_back( sheet );
    }
}


std::optional<SCH_SHEET_PATH> SCH_SHEET_LIST::GetSheetPathByKIIDPath( const KIID_PATH& aPath,
                                                                      bool aIncludeLastSheet ) const
{
    for( const SCH_SHEET_PATH& sheet : *this )
    {
        KIID_PATH testPath = sheet.Path();

        if( !aIncludeLastSheet )
            testPath.pop_back();

        if( testPath == aPath )
            return SCH_SHEET_PATH( sheet );
    }

    return std::nullopt;
}


void SCH_SHEET_LIST::GetMultiUnitSymbols( SCH_MULTI_UNIT_REFERENCE_MAP &aRefList,
                                          bool aIncludePowerSymbols ) const
{
    for( auto it = begin(); it != end(); ++it )
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


SCH_SHEET_PATH SCH_SHEET_LIST::FindSheetForScreen( const SCH_SCREEN* aScreen )
{
    for( SCH_SHEET_PATH& sheetpath : *this )
    {
        if( sheetpath.LastScreen() == aScreen )
            return sheetpath;
    }

    return SCH_SHEET_PATH();
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


void SCH_SHEET_LIST::UpdateSymbolInstanceData(
                                const std::vector<SCH_SYMBOL_INSTANCE>& aSymbolInstances )
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
                    [ sheetPathWithSymbolUuid ]( const SCH_SYMBOL_INSTANCE& r ) -> bool
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
            symbol->AddHierarchicalReference( sheetPath.Path(), it->m_Reference, it->m_Unit );
            symbol->GetField( FIELD_T::REFERENCE )->SetText( it->m_Reference );

            if( !it->m_Value.IsEmpty() )
                symbol->SetValueFieldText( it->m_Value );

            if( !it->m_Footprint.IsEmpty() )
                symbol->SetFootprintFieldText( it->m_Footprint );

            symbol->UpdatePrefix();
        }
    }
}


void SCH_SHEET_LIST::UpdateSheetInstanceData( const std::vector<SCH_SHEET_INSTANCE>& aSheetInstances )
{

    for( SCH_SHEET_PATH& path : *this )
    {
        SCH_SHEET* sheet = path.Last();

        wxCHECK2( sheet && path.Last(), continue );

        auto it = std::find_if( aSheetInstances.begin(), aSheetInstances.end(),
                                [&path]( const SCH_SHEET_INSTANCE& r ) -> bool
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
                    ( sheet->GetName().IsEmpty() ) ? wxString( wxT( "root" ) ) : sheet->GetName(),
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

        retval.push_back( std::move( instance ) );
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
        if( instance.Last()->IsVirtualRootSheet() )
            continue;

        tmp.Printf( "%d", pageNumber );
        instance.SetPageNumber( tmp );
        pageNumber += 1;
    }
}


void SCH_SHEET_LIST::AddNewSymbolInstances( const SCH_SHEET_PATH& aPrefixSheetPath,
                                            const wxString& aProjectName )
{
    for( SCH_SHEET_PATH& sheetPath : *this )
        sheetPath.AddNewSymbolInstances( aPrefixSheetPath, aProjectName );
}


void SCH_SHEET_LIST::RemoveSymbolInstances( const SCH_SHEET_PATH& aPrefixSheetPath )
{
    for( SCH_SHEET_PATH& sheetPath : *this )
        sheetPath.RemoveSymbolInstances( aPrefixSheetPath );
}


void SCH_SHEET_LIST::AddNewSheetInstances( const SCH_SHEET_PATH& aPrefixSheetPath,
                                           int aLastVirtualPageNumber )
{
    wxString pageNumber;
    int lastUsedPageNumber = 1;
    int nextVirtualPageNumber = aLastVirtualPageNumber;

    // Fetch the list of page numbers already in use.
    std::vector< wxString > usedPageNumbers;

    if( aPrefixSheetPath.size() )
    {
        SCH_SHEET_LIST prefixHierarchy( aPrefixSheetPath.at( 0 ) );

        for( const SCH_SHEET_PATH& path : prefixHierarchy )
        {
            pageNumber = path.GetPageNumber();

            if( !pageNumber.IsEmpty() )
                usedPageNumbers.emplace_back( pageNumber );
        }
    }

    for( SCH_SHEET_PATH& sheetPath : *this )
    {
        KIID_PATH tmp = sheetPath.Path();
        SCH_SHEET_PATH newSheetPath( aPrefixSheetPath );

        // Prefix the new hierarchical path.
        newSheetPath = newSheetPath + sheetPath;

        // Sheets cannot have themselves in the path.
        tmp.pop_back();

        SCH_SHEET* sheet = sheetPath.Last();

        wxCHECK2( sheet, continue );

        nextVirtualPageNumber += 1;

        SCH_SHEET_INSTANCE instance;

        // Add the instance if it doesn't already exist
        if( !sheet->getInstance( instance, tmp, true ) )
        {
            sheet->addInstance( tmp );
            sheet->getInstance( instance, tmp, true );
        }

        // Get a new page number if we don't have one
        if( instance.m_PageNumber.IsEmpty() )
        {
            // Generate the next available page number.
            do
            {
                pageNumber.Printf( wxT( "%d" ), lastUsedPageNumber );
                lastUsedPageNumber += 1;
            } while( std::find( usedPageNumbers.begin(), usedPageNumbers.end(), pageNumber ) !=
                     usedPageNumbers.end() );

            instance.m_PageNumber = pageNumber;
            newSheetPath.SetVirtualPageNumber( nextVirtualPageNumber );
        }

        newSheetPath.SetPageNumber( instance.m_PageNumber );
        usedPageNumbers.push_back( instance.m_PageNumber );
    }
}


void SCH_SHEET_LIST::CheckForMissingSymbolInstances( const wxString& aProjectName )
{
    wxLogTrace( traceSchSheetPaths,
               "SCH_SHEET_LIST::CheckForMissingSymbolInstances: Processing %zu sheet paths",
               size() );

    for( SCH_SHEET_PATH& sheetPath : *this )
    {
        wxLogTrace( traceSchSheetPaths,
                   "  Processing sheet path: '%s' (size=%zu, KIID_PATH='%s')",
                   sheetPath.PathHumanReadable( false ),
                   sheetPath.size(),
                   sheetPath.Path().AsString() );
        sheetPath.CheckForMissingSymbolInstances( aProjectName );
    }
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


bool SCH_SHEET_LIST::HasPath( const KIID_PATH& aPath ) const
{
    for( const SCH_SHEET_PATH& path : *this )
    {
        if( path.Path() == aPath )
            return true;
    }

    return false;
}


bool SCH_SHEET_LIST::ContainsSheet( const SCH_SHEET* aSheet ) const
{
    for( const SCH_SHEET_PATH& path : *this )
    {
        for( size_t i = 0; i < path.size(); i++ )
        {
            if( path.at( i ) == aSheet )
                return true;
        }
    }

    return false;
}


std::optional<SCH_SHEET_PATH> SCH_SHEET_LIST::GetOrdinalPath( const SCH_SCREEN* aScreen ) const
{
    // Sheet paths with sheets that do not have a screen object are not valid.
    if( !aScreen )
        return std::nullopt;

    for( const SCH_SHEET_PATH& path: *this )
    {
        if( path.LastScreen() == aScreen )
            return std::optional<SCH_SHEET_PATH>( path );
    }

    return std::nullopt;
}
