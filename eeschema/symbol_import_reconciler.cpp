/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include "symbol_import_reconciler.h"

#include <map>
#include <set>

#include <wx/filefn.h>
#include <wx/filename.h>

#include <import_proj_properties.h>
#include <lib_id.h>
#include <lib_symbol.h>
#include <project.h>
#include <project_sch.h>
#include <reporter.h>
#include <schematic.h>
#include <sch_pin.h>
#include <sch_screen.h>
#include <sch_symbol.h>
#include <wildcards_and_files_ext.h>
#include <io/io_mgr.h>
#include <sch_io/sch_io.h>
#include <sch_io/sch_io_mgr.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <libraries/symbol_library_adapter.h>
#include <libraries/library_table.h>


SYMBOL_IMPORT_RECONCILER::SYMBOL_IMPORT_RECONCILER( SYMBOL_LIBRARY_ADAPTER& aAdapter,
                                                    const wxString&         aProjectPath,
                                                    REPORTER&               aReporter ) :
        m_adapter( aAdapter ),
        m_projectPath( aProjectPath ),
        m_reporter( aReporter )
{
}


namespace
{
// every screen once, so a sheet instantiated many times does not re-walk its symbols
std::vector<SCH_SYMBOL*> placedSymbols( SCHEMATIC* aSchematic )
{
    std::vector<SCH_SYMBOL*> symbols;
    SCH_SCREENS              screens( aSchematic->Root() );

    for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
    {
        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
            symbols.push_back( static_cast<SCH_SYMBOL*>( item ) );
    }

    return symbols;
}


// structural signature, flags same-name placed instances that differ
wxString placedSignature( const SCH_SYMBOL* aSymbol )
{
    const std::unique_ptr<LIB_SYMBOL>& part = aSymbol->GetLibSymbolRef();

    if( !part )
        return wxEmptyString;

    return wxString::Format( wxS( "%d:%d" ), part->GetPinCount(), part->GetUnitCount() );
}


// lookup that tolerates an unreadable or absent library
LIB_SYMBOL* loadSymbol( SYMBOL_LIBRARY_ADAPTER& aAdapter, const wxString& aNickname,
                        const wxString& aName )
{
    try
    {
        return aAdapter.LoadSymbol( aNickname, aName );
    }
    catch( const IO_ERROR& )
    {
        return nullptr;
    }
}


std::multiset<wxString> pinNumbers( const LIB_SYMBOL& aSymbol )
{
    std::multiset<wxString> numbers;

    for( const SCH_PIN* pin : aSymbol.GetPins() )
        numbers.insert( pin->GetNumber() );

    return numbers;
}


// two tools never draw a part identically, so equivalence is the electrical interface
bool sameInterface( const LIB_SYMBOL& aLhs, const LIB_SYMBOL& aRhs )
{
    return aLhs.GetUnitCount() == aRhs.GetUnitCount() && pinNumbers( aLhs ) == pinNumbers( aRhs );
}


// reuse existing row/file only if prior import-managed cache
bool isManagedCache( const LIBRARY_TABLE_ROW* aRow )
{
    return aRow && aRow->GetOptionsMap().count( IMPORT_PROJ_PROPS::MANAGED_CACHE_KEY ) > 0;
}
}


SYMBOL_IMPORT_RECONCILE_RESULT
SYMBOL_IMPORT_RECONCILER::Reconcile( SCHEMATIC*                               aSchematic,
                                     std::vector<std::unique_ptr<LIB_SYMBOL>> aDefinitions,
                                     const wxString&                          aCacheNickname,
                                     const std::vector<wxString>& aSourceLibNicknames )
{
    SYMBOL_IMPORT_RECONCILE_RESULT result;

    if( !aSchematic )
        return result;

    std::map<wxString, LIB_SYMBOL*> defByName;

    for( const std::unique_ptr<LIB_SYMBOL>& def : aDefinitions )
    {
        wxString name = def->GetLibId().GetUniStringLibItemName();

        if( name.IsEmpty() )
            name = def->GetName();

        if( !name.IsEmpty() )
            defByName.emplace( name, def.get() );
    }

    // preload source libs before membership queries
    for( const wxString& nick : aSourceLibNicknames )
    {
        if( m_adapter.GetRow( nick ) )
            m_adapter.LoadOne( nick );
    }

    std::set<wxString> provenance( aSourceLibNicknames.begin(), aSourceLibNicknames.end() );

    // resolve one source lib, empty if none or ambiguous
    auto resolveSource = [&]( const SCH_SYMBOL* aSymbol, const wxString& aName ) -> wxString
    {
        std::vector<wxString> candidates;
        wxString              ownNick = aSymbol->GetLibId().GetUniStringLibNickname();

        if( !ownNick.IsEmpty() )
            candidates.push_back( ownNick );

        for( const wxString& nick : aSourceLibNicknames )
        {
            if( nick != ownNick )
                candidates.push_back( nick );
        }

        std::vector<wxString> matches;

        for( const wxString& nick : candidates )
        {
            if( !m_adapter.GetRow( nick ) )
                continue;

            LIB_SYMBOL* candidate = loadSymbol( m_adapter, nick, aName );

            if( !candidate )
                continue;

            // A nickname the importer emitted is not provenance.  An unrelated library that
            // happens to carry the name must not swallow the imported definition, so it takes the
            // link only when it holds the same part.
            if( !provenance.count( nick ) )
            {
                auto def = defByName.find( aName );

                if( def == defByName.end() || !sameInterface( *candidate, *def->second ) )
                    continue;
            }

            matches.push_back( nick );
        }

        return matches.size() == 1 ? matches.front() : wxString( wxEmptyString );
    };

    std::vector<SCH_SYMBOL*> symbols = placedSymbols( aSchematic );

    // per-instance target keyed by nick+name, so same-name parts from different libs stay split
    // empty target = cache-bound
    std::map<wxString, wxString>                 targetByKey;
    std::set<wxString>                           cacheNames;
    std::map<wxString, std::vector<SCH_SYMBOL*>> instancesByName;

    auto keyOf = []( const wxString& aNick, const wxString& aName )
    {
        return aNick + wxS( "\x1f" ) + aName;
    };

    for( SCH_SYMBOL* symbol : symbols )
    {
        wxString name = symbol->GetLibId().GetUniStringLibItemName();

        if( name.IsEmpty() )
            continue;

        instancesByName[name].push_back( symbol );

        wxString key = keyOf( symbol->GetLibId().GetUniStringLibNickname(), name );

        if( targetByKey.count( key ) )
            continue;

        wxString sourceNick = resolveSource( symbol, name );
        targetByKey[key] = sourceNick;

        if( sourceNick.IsEmpty() )
            cacheNames.insert( name );
    }

    // canonical def per cache name, fall back to the placed instance cache if importer gave none
    std::map<wxString, LIB_SYMBOL*>          cacheDefs;
    std::vector<std::unique_ptr<LIB_SYMBOL>> placedDefs;

    for( const wxString& name : cacheNames )
    {
        if( auto it = defByName.find( name ); it != defByName.end() )
        {
            cacheDefs[name] = it->second;
            continue;
        }

        const std::vector<SCH_SYMBOL*>& instances = instancesByName[name];

        if( instances.empty() || !instances.front()->GetLibSymbolRef() )
            continue;

        wxString firstSig = placedSignature( instances.front() );

        for( auto it = instances.begin() + 1; it != instances.end(); ++it )
        {
            if( placedSignature( *it ) != firstSig )
            {
                m_reporter.Report( wxString::Format( _( "Imported symbol '%s' has conflicting "
                                                        "placed definitions; keeping the first." ),
                                                     name ),
                                   RPT_SEVERITY_WARNING );
                break;
            }
        }

        placedDefs.push_back(
                std::make_unique<LIB_SYMBOL>( *instances.front()->GetLibSymbolRef() ) );
        cacheDefs[name] = placedDefs.back().get();
    }

    if( !cacheDefs.empty() )
        writeAndRegisterCache( aCacheNickname, cacheDefs, result );

    // re-point nicks to the resolved lib, keep the item name
    for( SCH_SYMBOL* symbol : symbols )
    {
        LIB_ID   libId = symbol->GetLibId();
        wxString name = libId.GetUniStringLibItemName();

        if( name.IsEmpty() )
            continue;

        auto it = targetByKey.find( keyOf( libId.GetUniStringLibNickname(), name ) );

        if( it == targetByKey.end() )
        {
            result.m_unresolved++;
            continue;
        }

        // empty resolution = cache-bound, resolves only once the cache is published
        if( it->second.IsEmpty() )
        {
            if( result.m_cacheNickname.IsEmpty() )
            {
                result.m_unresolved++;
                continue;
            }

            libId.SetLibNickname( aCacheNickname );
            symbol->SetLibId( libId );
            result.m_linkedToCache++;
        }
        else
        {
            libId.SetLibNickname( it->second );
            symbol->SetLibId( libId );
            result.m_linkedToSource++;
        }
    }

    return result;
}


void SYMBOL_IMPORT_RECONCILER::writeAndRegisterCache(
        const wxString& aCacheNickname, const std::map<wxString, LIB_SYMBOL*>& aCacheDefs,
        SYMBOL_IMPORT_RECONCILE_RESULT& aResult )
{
    wxFileName finalFn( m_projectPath, aCacheNickname, FILEEXT::KiCadSymbolLibFileExtension );
    wxString   finalPath = finalFn.GetFullPath();
    wxString   tempPath = finalPath + wxS( ".tmp" );

    // a nickname the user already owns is never repurposed, whatever its row points at
    LIBRARY_TABLE_ROW* existingRow = m_adapter.GetRow( aCacheNickname ).value_or( nullptr );

    if( existingRow && !isManagedCache( existingRow ) )
    {
        m_reporter.Report( wxString::Format( _( "A symbol library named '%s' is already "
                                                "registered; leaving imported symbols "
                                                "unresolved." ), aCacheNickname ),
                           RPT_SEVERITY_ERROR );
        return;
    }

    IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );

    if( !pi )
    {
        m_reporter.Report( _( "Cannot reconcile imported symbols: no KiCad symbol writer." ),
                           RPT_SEVERITY_ERROR );
        return;
    }

    // best-effort cleanup, must not throw
    auto safeDelete = [&pi]( const wxString& aPath )
    {
        try
        {
            pi->DeleteLibrary( aPath );
        }
        catch( const IO_ERROR& )
        {
        }
    };

    bool wrote = false;

    try
    {
        if( wxFileExists( tempPath ) )
            pi->DeleteLibrary( tempPath );

        pi->CreateLibrary( tempPath );

        // buffer the writes, the library is flushed once by SaveLibrary
        std::map<std::string, UTF8> properties;
        properties.emplace( SCH_IO_KICAD_SEXPR::PropBuffering, wxEmptyString );

        for( const auto& [name, def] : aCacheDefs )
        {
            std::unique_ptr<LIB_SYMBOL> copy = std::make_unique<LIB_SYMBOL>( *def );
            LIB_ID                      id = copy->GetLibId();

            id.SetLibNickname( aCacheNickname );
            copy->SetLibId( id );
            pi->SaveSymbol( tempPath, copy.release(), &properties );
        }

        pi->SaveLibrary( tempPath );
        wrote = true;
    }
    catch( const IO_ERROR& ioe )
    {
        m_reporter.Report( wxString::Format( _( "Error writing imported symbol cache '%s': %s" ),
                                            aCacheNickname, ioe.What() ),
                           RPT_SEVERITY_ERROR );
    }

    if( !wrote )
    {
        if( wxFileExists( tempPath ) )
            safeDelete( tempPath );

        return;
    }

    // publish temp->final, replace only a managed cache, never a user lib
    if( wxFileExists( finalPath ) )
    {
        if( isManagedCache( existingRow ) )
        {
            safeDelete( finalPath );
        }
        else
        {
            m_reporter.Report( wxString::Format( _( "A library already exists at '%s'; leaving "
                                                    "imported symbols unresolved." ), finalPath ),
                               RPT_SEVERITY_ERROR );
            safeDelete( tempPath );
            return;
        }
    }

    if( !wxRenameFile( tempPath, finalPath, false ) )
    {
        m_reporter.Report( wxString::Format( _( "Could not publish imported symbol cache to "
                                                "'%s'." ), finalPath ),
                           RPT_SEVERITY_ERROR );
        safeDelete( tempPath );
        return;
    }

    // only claim the cache when its table row is registered, else LIB_IDs re-point to a dead nickname
    if( !registerCacheRow( aCacheNickname ) )
        return;

    aResult.m_cacheNickname = aCacheNickname;
    aResult.m_savedToCache = static_cast<int>( aCacheDefs.size() );
}


bool SYMBOL_IMPORT_RECONCILER::registerCacheRow( const wxString& aCacheNickname )
{
    std::optional<LIBRARY_TABLE*> tableOpt = m_adapter.ProjectTable();

    if( !tableOpt || !*tableOpt )
    {
        m_reporter.Report( _( "Cannot register imported symbol cache: no project library "
                              "table." ), RPT_SEVERITY_ERROR );
        return false;
    }

    LIBRARY_TABLE*     table = *tableOpt;
    wxString           cacheFile = aCacheNickname + wxS( "." )
                                   + wxString( FILEEXT::KiCadSymbolLibFileExtension );
    wxString           uri = wxS( "${KIPRJMOD}/" ) + cacheFile;
    LIBRARY_TABLE_ROW* row = table->HasRow( aCacheNickname )
                                     ? table->Row( aCacheNickname ).value_or( nullptr )
                                     : &table->InsertRow();

    if( !row )
        return false;

    row->SetNickname( aCacheNickname );
    row->SetURI( uri );
    row->SetType( wxS( "KiCad" ) );
    row->SetOptions( IMPORT_PROJ_PROPS::ManagedCacheOption() );
    row->SetScope( LIBRARY_TABLE_SCOPE::PROJECT );

    // an unsaved row is gone on restart, so the cache cannot be claimed
    if( !table->Save() )
    {
        m_reporter.Report( _( "Error saving project symbol library table; imported symbols left "
                              "unresolved." ), RPT_SEVERITY_ERROR );
        return false;
    }

    // load the cache so membership and later lookups resolve it
    m_adapter.LoadOne( aCacheNickname );
    return true;
}


SYMBOL_IMPORT_RECONCILE_RESULT
ReconcileImportedSymbols( SCH_IO& aPlugin, SCHEMATIC& aSchematic, PROJECT& aProject,
                          const wxString& aSchematicPath,
                          const std::map<std::string, UTF8>* aProperties, REPORTER& aReporter )
{
    SYMBOL_IMPORT_RECONCILE_RESULT           result;
    std::vector<std::unique_ptr<LIB_SYMBOL>> definitions;

    try
    {
        for( LIB_SYMBOL* symbol : aPlugin.GetImportedCachedLibrarySymbols() )
            definitions.emplace_back( symbol );
    }
    catch( const IO_ERROR& )
    {
        return result;
    }

    // Importers still writing their own project library during load (Eagle) must be left alone:
    // with no definitions to interface-match, resolveSource would reject the library they just
    // wrote and duplicate every symbol into the cache.  Drop this once they all publish here.
    if( definitions.empty() )
        return result;

    SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( &aProject );

    if( !adapter )
        return result;

    // manager pre-commits the cache nickname + source libs; standalone import derives from filename
    wxString              cacheNick;
    std::vector<wxString> sourceLibs;
    IMPORT_PROJ_PROPS::ReadSymbolProps( aProperties, cacheNick, sourceLibs );

    if( cacheNick.IsEmpty() )
    {
        cacheNick = IMPORT_PROJ_PROPS::MakeSymbolCacheNickname(
                wxFileName( aSchematicPath ).GetName() );
    }

    SYMBOL_IMPORT_RECONCILER reconciler( *adapter, aProject.GetProjectPath(), aReporter );

    // reconciliation failure must not abort the import
    try
    {
        result = reconciler.Reconcile( &aSchematic, std::move( definitions ), cacheNick,
                                       sourceLibs );
    }
    catch( const IO_ERROR& ioe )
    {
        aReporter.Report( wxString::Format( _( "Could not reconcile imported symbol libraries: "
                                               "%s" ), ioe.What() ), RPT_SEVERITY_ERROR );
    }

    return result;
}
