/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <chrono>
#include <common.h>
#include <env_vars.h>
#include <list>
#include <magic_enum.hpp>
#include <thread_pool.h>
#include <ranges>
#include <unordered_set>

#include <paths.h>
#include <pgm_base.h>
#include <richio.h>
#include <string_utils.h>
#include <trace_helpers.h>
#include <wildcards_and_files_ext.h>

#include <libraries/library_manager.h>

using namespace std::chrono_literals;
#include <settings/common_settings.h>
#include <settings/kicad_settings.h>
#include <settings/settings_manager.h>
#include <wx/dir.h>
#include <wx/log.h>

struct LIBRARY_MANAGER_INTERNALS
{
    std::vector<LIBRARY_TABLE> tables;
};


std::mutex& LIBRARY_MANAGER_ADAPTER::pluginMutex( const wxString& aNickname )
{
    // Leaked and never erased: must outlive every caller including static teardown, and stable node
    // addresses keep the returned reference valid.
    static std::mutex&                                      registryLock = *new std::mutex;
    static std::map<wxString, std::unique_ptr<std::mutex>>& registry =
            *new std::map<wxString, std::unique_ptr<std::mutex>>;

    std::lock_guard              guard( registryLock );
    std::unique_ptr<std::mutex>& slot = registry[aNickname];

    if( !slot )
        slot = std::make_unique<std::mutex>();

    return *slot;
}


LIBRARY_MANAGER::LIBRARY_MANAGER()
{
}


LIBRARY_MANAGER::~LIBRARY_MANAGER() = default;


void LIBRARY_MANAGER::loadTables( const wxString& aTablePath, LIBRARY_TABLE_SCOPE aScope,
                                  std::vector<LIBRARY_TABLE_TYPE> aTablesToLoad )
{
    {
        std::lock_guard lock( m_rowCacheMutex );
        m_rowCache.clear();
    }

    auto getTarget =
            [&]() -> std::map<LIBRARY_TABLE_TYPE, std::unique_ptr<LIBRARY_TABLE>>&
            {
                switch( aScope )
                {
                case LIBRARY_TABLE_SCOPE::GLOBAL:
                    return m_tables;

                case LIBRARY_TABLE_SCOPE::PROJECT:
                    return m_projectTables;

                default:
                    wxCHECK_MSG( false, m_tables, "Invalid scope passed to loadTables" );
                }
            };

    std::map<LIBRARY_TABLE_TYPE, std::unique_ptr<LIBRARY_TABLE>>& aTarget = getTarget();

    if( aTablesToLoad.size() == 0 )
        aTablesToLoad = { LIBRARY_TABLE_TYPE::SYMBOL, LIBRARY_TABLE_TYPE::FOOTPRINT, LIBRARY_TABLE_TYPE::DESIGN_BLOCK };

    for( LIBRARY_TABLE_TYPE type : aTablesToLoad )
    {
        aTarget.erase( type );

        wxFileName fn( aTablePath, tableFileName( type ) );

        if( fn.IsFileReadable() )
        {
            std::unique_ptr<LIBRARY_TABLE> table = std::make_unique<LIBRARY_TABLE>( fn, aScope );

            if( table->Type() != type )
            {
                auto actualName = magic_enum::enum_name( table->Type() );
                auto expectedName = magic_enum::enum_name( type );
                wxLogWarning( wxS( "Library table '%s' has type %s but expected %s; skipping" ),
                              fn.GetFullPath(),
                              wxString( actualName.data(), actualName.size() ),
                              wxString( expectedName.data(), expectedName.size() ) );
                continue;
            }

            aTarget[type] = std::move( table );
            loadNestedTables( *aTarget[type] );
        }
        else
        {
            wxLogTrace( traceLibraries, "No library table found at %s", fn.GetFullPath() );
        }
    }
}


void LIBRARY_MANAGER::loadNestedTables( LIBRARY_TABLE& aRootTable )
{
    std::unordered_set<wxString> seenTables;

    std::function<void( LIBRARY_TABLE& )> processOneTable =
            [&]( LIBRARY_TABLE& aTable )
            {
                seenTables.insert( aTable.Path() );

                if( !aTable.IsOk() )
                    return;

                for( LIBRARY_TABLE_ROW& row : aTable.Rows() )
                {
                    if( row.Type() == LIBRARY_TABLE_ROW::TABLE_TYPE_NAME )
                    {
                        wxFileName file( ExpandURI( row.URI(), Pgm().GetSettingsManager().Prj() ) );

                        // URI may be relative to parent
                        file.MakeAbsolute( wxFileName( aTable.Path() ).GetPath() );

                        WX_FILENAME::ResolvePossibleSymlinks( file );
                        wxString src = file.GetFullPath();

                        if( seenTables.contains( src ) )
                        {
                            wxLogTrace( traceLibraries, "Library table %s has already been loaded!", src );
                            row.SetOk( false );
                            row.SetErrorDescription( _( "A reference to this library table already exists" ) );
                            continue;
                        }

                        auto child = std::make_unique<LIBRARY_TABLE>( file, aRootTable.Scope() );

                        processOneTable( *child );

                        if( !child->IsOk() )
                        {
                            row.SetOk( false );
                            row.SetErrorDescription( child->ErrorDescription() );
                        }

                        applyLibOverrides( *child );

                        m_childTables.insert_or_assign( row.URI(), std::move( child ) );
                    }
                }
            };

    processOneTable( aRootTable );
}


void LIBRARY_MANAGER::ApplyLibOverrides( LIBRARY_TABLE& aTable )
{
    if( !aTable.IsReadOnly() )
        return;

    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();
    KICAD_SETTINGS*   settings = mgr.GetAppSettings<KICAD_SETTINGS>( "kicad" );

    if( !settings )
        return;

    wxString tablePath = aTable.Path();

    auto it = settings->m_LibOverrides.find( tablePath );

    if( it == settings->m_LibOverrides.end() )
        return;

    const std::map<wxString, LIB_OVERRIDE>& overrides = it->second;

    for( LIBRARY_TABLE_ROW& row : aTable.Rows() )
    {
        auto overIt = overrides.find( row.Nickname() );

        if( overIt != overrides.end() )
        {
            row.SetDisabled( overIt->second.disabled );
            row.SetHidden( overIt->second.hidden );
        }
    }
}


void LIBRARY_MANAGER::applyLibOverrides( LIBRARY_TABLE& aTable )
{
    ApplyLibOverrides( aTable );
}


void LIBRARY_MANAGER::SetLibOverride( const wxString& aTablePath, const wxString& aNickname,
                                      bool aDisabled, bool aHidden )
{
    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();
    KICAD_SETTINGS*   settings = mgr.GetAppSettings<KICAD_SETTINGS>( "kicad" );

    wxCHECK( settings, /* void */ );

    if( !aDisabled && !aHidden )
    {
        ClearLibOverride( aTablePath, aNickname );
        return;
    }

    settings->m_LibOverrides[aTablePath][aNickname] = { aDisabled, aHidden };
}


void LIBRARY_MANAGER::ClearLibOverride( const wxString& aTablePath, const wxString& aNickname )
{
    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();
    KICAD_SETTINGS*   settings = mgr.GetAppSettings<KICAD_SETTINGS>( "kicad" );

    wxCHECK( settings, /* void */ );

    auto tableIt = settings->m_LibOverrides.find( aTablePath );

    if( tableIt == settings->m_LibOverrides.end() )
        return;

    tableIt->second.erase( aNickname );

    if( tableIt->second.empty() )
        settings->m_LibOverrides.erase( tableIt );
}


wxString LIBRARY_MANAGER::tableFileName( LIBRARY_TABLE_TYPE aType )
{
    switch( aType )
    {
    case LIBRARY_TABLE_TYPE::SYMBOL:        return FILEEXT::SymbolLibraryTableFileName;
    case LIBRARY_TABLE_TYPE::FOOTPRINT:     return FILEEXT::FootprintLibraryTableFileName;
    case LIBRARY_TABLE_TYPE::DESIGN_BLOCK:  return FILEEXT::DesignBlockLibraryTableFileName;
    default:                                wxCHECK( false, wxEmptyString );
    }
}


void LIBRARY_MANAGER::createEmptyTable( LIBRARY_TABLE_TYPE aType, LIBRARY_TABLE_SCOPE aScope )
{
    if( aScope == LIBRARY_TABLE_SCOPE::GLOBAL )
    {
        wxCHECK( !m_tables.contains( aType ), /* void */ );
        wxFileName fn( PATHS::GetUserSettingsPath(), tableFileName( aType ) );

        m_tables[aType] = std::make_unique<LIBRARY_TABLE>( fn, LIBRARY_TABLE_SCOPE::GLOBAL );
        m_tables[aType]->SetType( aType );
    }
    else if( aScope == LIBRARY_TABLE_SCOPE::PROJECT )
    {
        wxCHECK( !m_projectTables.contains( aType ), /* void */ );
        wxFileName fn( Pgm().GetSettingsManager().Prj().GetProjectDirectory(), tableFileName( aType ) );

        m_projectTables[aType] = std::make_unique<LIBRARY_TABLE>( fn, LIBRARY_TABLE_SCOPE::PROJECT );
        m_projectTables[aType]->SetType( aType );
    }
}


class PCM_LIB_TRAVERSER final : public wxDirTraverser
{
public:
    explicit PCM_LIB_TRAVERSER( const wxString& aBasePath, LIBRARY_MANAGER& aManager,
                                const wxString& aPrefix ) :
            m_manager( aManager ),
            m_project( Pgm().GetSettingsManager().Prj() ),
            m_path_prefix( aBasePath ),
            m_lib_prefix( aPrefix )
    {
        wxFileName f( aBasePath, "" );
        m_prefix_dir_count = f.GetDirCount();

        m_symbolTable = m_manager.Table( LIBRARY_TABLE_TYPE::SYMBOL, LIBRARY_TABLE_SCOPE::GLOBAL ).value_or( nullptr );
        m_fpTable = m_manager.Table( LIBRARY_TABLE_TYPE::FOOTPRINT, LIBRARY_TABLE_SCOPE::GLOBAL ).value_or( nullptr );
        m_designBlockTable = m_manager.Table( LIBRARY_TABLE_TYPE::DESIGN_BLOCK, LIBRARY_TABLE_SCOPE::GLOBAL )
                                      .value_or( nullptr );
    }

    /// Handles symbol library files, minimum nest level 2
    wxDirTraverseResult OnFile( const wxString& aFilePath ) override
    {
        wxFileName file = wxFileName::FileName( aFilePath );

        // consider a file to be a lib if it's name ends with .kicad_sym and
        // it is under $KICADn_3RD_PARTY/symbols/<pkgid>/ i.e. has nested level of at least +2
        if( file.GetExt() == wxT( "kicad_sym" )
                && file.GetDirCount() >= m_prefix_dir_count + 2
                && file.GetDirs()[m_prefix_dir_count] == wxT( "symbols" ) )
        {
            addRowIfNecessary( m_symbolTable, file, ADD_MODE::AM_FILE, 10 );
        }

        return wxDIR_CONTINUE;
    }

    /// Handles footprint library and design block library directories, minimum nest level 3
    wxDirTraverseResult OnDir( const wxString& dirPath ) override
    {
        static wxString designBlockExt = wxString::Format( wxS( ".%s" ), FILEEXT::KiCadDesignBlockLibPathExtension );
        wxFileName dir = wxFileName::DirName( dirPath );

        // consider a directory to be a lib if it's name ends with .pretty and
        // it is under $KICADn_3RD_PARTY/footprints/<pkgid>/ i.e. has nested level of at least +3
        if( dirPath.EndsWith( wxS( ".pretty" ) )
                && dir.GetDirCount() >= m_prefix_dir_count + 3
                && dir.GetDirs()[m_prefix_dir_count] == wxT( "footprints" ) )
        {
            addRowIfNecessary( m_fpTable, dir, ADD_MODE::AM_DIRECTORY, 7 );
        }
        else if( dirPath.EndsWith( designBlockExt )
                 && dir.GetDirCount() >= m_prefix_dir_count + 3
                 && dir.GetDirs()[m_prefix_dir_count] == wxT( "design_blocks" ) )
        {
            addRowIfNecessary( m_designBlockTable, dir, ADD_MODE::AM_DIRECTORY, designBlockExt.Len() );
        }

        return wxDIR_CONTINUE;
    }

    std::set<LIBRARY_TABLE*> Modified() const { return m_modified; }

private:
    void ensureUnique( LIBRARY_TABLE* aTable, const wxString& aBaseName, wxString& aNickname ) const
    {
        if( aTable->HasRow( aNickname ) )
        {
            int increment = 1;

            do
            {
                aNickname = wxString::Format( "%s%s_%d", m_lib_prefix, aBaseName, increment );
                increment++;
            } while( aTable->HasRow( aNickname ) );
        }
    }

    enum class ADD_MODE
    {
        AM_FILE,
        AM_DIRECTORY
    };

    void addRowIfNecessary( LIBRARY_TABLE* aTable, const wxFileName& aSource, ADD_MODE aMode,
                            int aExtensionLength )
    {
        wxString versionedPath = wxString::Format( wxS( "${%s}" ),
        ENV_VAR::GetVersionedEnvVarName( wxS( "3RD_PARTY" ) ) );

        wxArrayString parts = aSource.GetDirs();
        parts.RemoveAt( 0, m_prefix_dir_count );
        parts.Insert( versionedPath, 0 );

        if( aMode == ADD_MODE::AM_FILE )
            parts.Add( aSource.GetFullName() );

        wxString libPath = wxJoin( parts, '/' );

        if( !aTable->HasRowWithURI( libPath, m_project ) )
        {
            wxString name = parts.Last().substr( 0, parts.Last().length() - aExtensionLength );
            wxString nickname = wxString::Format( "%s%s", m_lib_prefix, name );

            ensureUnique( aTable, name, nickname );

            wxLogTrace( traceLibraries, "Manager: Adding PCM lib '%s' as '%s'", libPath, nickname );

            LIBRARY_TABLE_ROW& row = aTable->InsertRow();

            row.SetNickname( nickname );
            row.SetURI( libPath );
            row.SetType( wxT( "KiCad" ) );
            row.SetDescription( _( "Added by Plugin and Content Manager" ) );
            m_modified.insert( aTable );
        }
        else
        {
            wxLogTrace( traceLibraries, "Manager: Not adding existing PCM lib '%s'", libPath );
        }
    }

private:
    LIBRARY_MANAGER& m_manager;
    const PROJECT&   m_project;
    wxString         m_path_prefix;
    wxString         m_lib_prefix;
    size_t           m_prefix_dir_count;
    std::set<LIBRARY_TABLE*> m_modified;

    LIBRARY_TABLE*   m_symbolTable;
    LIBRARY_TABLE*   m_fpTable;
    LIBRARY_TABLE*   m_designBlockTable;
};


wxString LIBRARY_MANAGER::DefaultGlobalTablePath( LIBRARY_TABLE_TYPE aType )
{
    wxString basePath = PATHS::GetUserSettingsPath();

    wxFileName fn( basePath, tableFileName( aType ) );
    fn.Normalize( FN_NORMALIZE_FLAGS | wxPATH_NORM_ENV_VARS );

    return fn.GetFullPath();
}


wxString LIBRARY_MANAGER::StockTablePath( LIBRARY_TABLE_TYPE aType )
{
    wxString basePath = PATHS::GetStockTemplatesPath();

    wxFileName fn( basePath, tableFileName( aType ) );
    fn.Normalize( FN_NORMALIZE_FLAGS | wxPATH_NORM_ENV_VARS );

    return fn.GetFullPath();
}


wxString LIBRARY_MANAGER::StockTableTokenizedURI( LIBRARY_TABLE_TYPE aType )
{
    wxString templateDirVar = ENV_VAR::GetVersionedEnvVarName( wxS( "TEMPLATE_DIR" ) );

    return wxString::Format( wxS( "${%s}/%s" ), templateDirVar, tableFileName( aType ) );
}


wxString LIBRARY_MANAGER::StockTableReferenceURI( LIBRARY_TABLE_TYPE aType )
{
    const wxString templateDirVar = ENV_VAR::GetVersionedEnvVarName( wxS( "TEMPLATE_DIR" ) );

    // Only relocatable installs (AppImage, Nix) export the template-dir variable so the stock
    // path follows the remounted prefix. There we store the unresolved token, which re-resolves
    // each launch instead of dangling. A standard install leaves the variable at its built-in
    // default, so we keep the historical resolved absolute path.
    if( COMMON_SETTINGS* common = Pgm().GetCommonSettings() )
    {
        auto it = common->m_Env.vars.find( templateDirVar );

        if( it != common->m_Env.vars.end() && it->second.GetDefinedExternally() )
            return StockTableTokenizedURI( aType );
    }

    return StockTablePath( aType );
}


bool LIBRARY_MANAGER::IsTableValid( const wxString& aPath )
{
    if( wxFileName fn( aPath ); fn.IsFileReadable() )
    {
        LIBRARY_TABLE temp( fn, LIBRARY_TABLE_SCOPE::GLOBAL );

        if( temp.IsOk() )
            return true;
    }

    return false;
}


bool LIBRARY_MANAGER::GlobalTablesValid()
{
    return InvalidGlobalTables().empty();
}


std::vector<LIBRARY_TABLE_TYPE> LIBRARY_MANAGER::InvalidGlobalTables()
{
    std::vector<LIBRARY_TABLE_TYPE> invalidTables;
    wxString basePath = PATHS::GetUserSettingsPath();

    for( LIBRARY_TABLE_TYPE tableType : { LIBRARY_TABLE_TYPE::SYMBOL,
                                          LIBRARY_TABLE_TYPE::FOOTPRINT,
                                          LIBRARY_TABLE_TYPE::DESIGN_BLOCK } )
    {
        wxFileName fn( basePath, tableFileName( tableType ) );

        if( !IsTableValid( fn.GetFullPath() ) )
            invalidTables.emplace_back( tableType );
    }

    return invalidTables;
}


bool LIBRARY_MANAGER::CreateGlobalTable( LIBRARY_TABLE_TYPE aType, bool aPopulateDefaultLibraries )
{
    wxFileName fn( DefaultGlobalTablePath( aType ) );

    LIBRARY_TABLE table( fn, LIBRARY_TABLE_SCOPE::GLOBAL );
    table.SetType( aType );
    table.Rows().clear();

    wxFileName defaultLib( StockTablePath( aType ) );

    if( !defaultLib.IsFileReadable() )
    {
        wxLogTrace( traceLibraries, "Warning: couldn't read default library table for %s at '%s'",
                    magic_enum::enum_name( aType ), defaultLib.GetFullPath() );
    }

    if( aPopulateDefaultLibraries )
    {
        LIBRARY_TABLE_ROW& chained = table.InsertRow();
        chained.SetType( LIBRARY_TABLE_ROW::TABLE_TYPE_NAME );
        chained.SetNickname( wxT( "KiCad" ) );
        chained.SetDescription( _( "KiCad Default Libraries" ) );
        chained.SetURI( StockTableReferenceURI( aType ) );
    }

    try
    {
        PRETTIFIED_FILE_OUTPUTFORMATTER formatter( fn.GetFullPath(), KICAD_FORMAT::FORMAT_MODE::LIBRARY_TABLE );
        table.Format( &formatter );
    }
    catch( IO_ERROR& e )
    {
        wxLogTrace( traceLibraries, "Exception while saving: %s", e.What() );
        return false;
    }

    return true;
}


void LIBRARY_MANAGER::LoadGlobalTables( std::initializer_list<LIBRARY_TABLE_TYPE> aTablesToLoad )
{
    // Cancel any in-progress load
    {
        std::scoped_lock lock( m_adaptersMutex );

        for( const std::unique_ptr<LIBRARY_MANAGER_ADAPTER>& adapter : m_adapters | std::views::values )
            adapter->GlobalTablesChanged( aTablesToLoad );
    }

    loadTables( PATHS::GetUserSettingsPath(), LIBRARY_TABLE_SCOPE::GLOBAL, aTablesToLoad );

    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();
    KICAD_SETTINGS*   settings = mgr.GetAppSettings<KICAD_SETTINGS>( "kicad" );

    wxCHECK( settings, /* void */ );

    const ENV_VAR_MAP& vars = Pgm().GetLocalEnvVariables();
    std::optional<wxString> packagesPath = ENV_VAR::GetVersionedEnvVarValue( vars, wxT( "3RD_PARTY" ) );

    if( packagesPath && settings->m_PcmLibAutoAdd )
    {
        // Scan for libraries in PCM packages directory
        wxFileName d( *packagesPath, "" );

        if( d.DirExists() )
        {
            PCM_LIB_TRAVERSER traverser( *packagesPath, *this, settings->m_PcmLibPrefix );
            wxDir             dir( d.GetPath() );

            dir.Traverse( traverser );

            for( LIBRARY_TABLE* table : traverser.Modified() )
            {
                table->Save().map_error(
                        []( const LIBRARY_ERROR& aError )
                        {
                            wxLogTrace( traceLibraries, wxT( "Warning: save failed after PCM auto-add: %s" ),
                                        aError.message );
                        } );
            }
        }
    }

    auto cleanupRemovedPCMLibraries =
            [&]( LIBRARY_TABLE_TYPE aType )
            {
                LIBRARY_TABLE* table = Table( aType, LIBRARY_TABLE_SCOPE::GLOBAL ).value_or( nullptr );
                wxCHECK( table, /* void */ );

                auto toErase = std::ranges::remove_if( table->Rows(),
                        [&]( const LIBRARY_TABLE_ROW& aRow )
                        {
                            if( !IsPcmManagedRow( aRow ) )
                                return false;

                            wxString path = GetFullURI( &aRow, true );
                            return !wxFileName::Exists( path );
                        } );

                bool hadRemovals = !toErase.empty();
                table->Rows().erase( toErase.begin(), toErase.end() );

                if( hadRemovals )
                {
                    table->Save().map_error(
                            []( const LIBRARY_ERROR& aError )
                            {
                                wxLogTrace( traceLibraries, wxT( "Warning: save failed after PCM auto-remove: %s" ),
                                            aError.message );
                            } );
                }
            };

    if( packagesPath && settings->m_PcmLibAutoRemove )
    {
        cleanupRemovedPCMLibraries( LIBRARY_TABLE_TYPE::SYMBOL );
        cleanupRemovedPCMLibraries( LIBRARY_TABLE_TYPE::FOOTPRINT );
        cleanupRemovedPCMLibraries( LIBRARY_TABLE_TYPE::DESIGN_BLOCK );
    }
}


void LIBRARY_MANAGER::LoadProjectTables( std::initializer_list<LIBRARY_TABLE_TYPE> aTablesToLoad )
{
    LoadProjectTables( Pgm().GetSettingsManager().Prj().GetProjectDirectory(), aTablesToLoad );
}


void LIBRARY_MANAGER::ProjectChanged()
{
    // Abort any running async library loads before reloading project tables.
    // Background workers hold raw LIBRARY_TABLE_ROW pointers that become dangling
    // when loadTables() destroys and replaces the table objects.
    AbortAsyncLoads();

    LoadProjectTables( Pgm().GetSettingsManager().Prj().GetProjectDirectory() );

    std::scoped_lock lock( m_adaptersMutex );

    for( const std::unique_ptr<LIBRARY_MANAGER_ADAPTER>& adapter : m_adapters | std::views::values )
        adapter->ProjectChanged();
}


void LIBRARY_MANAGER::AbortAsyncLoads()
{
    std::scoped_lock lock( m_adaptersMutex );

    for( const std::unique_ptr<LIBRARY_MANAGER_ADAPTER>& adapter : m_adapters | std::views::values )
        adapter->AbortAsyncLoad();
}


void LIBRARY_MANAGER::RegisterAdapter( LIBRARY_TABLE_TYPE aType,
                                       std::unique_ptr<LIBRARY_MANAGER_ADAPTER>&& aAdapter )
{
    std::scoped_lock lock( m_adaptersMutex );

    wxCHECK_MSG( !m_adapters.contains( aType ), /**/, "You should only register an adapter once!" );

    m_adapters[aType] = std::move( aAdapter );
}


bool LIBRARY_MANAGER::RemoveAdapter( LIBRARY_TABLE_TYPE aType, LIBRARY_MANAGER_ADAPTER* aAdapter )
{
    std::scoped_lock lock( m_adaptersMutex );
    if( !m_adapters.contains( aType ) )
        return false;

    if( m_adapters[aType].get() != aAdapter )
        return false;

    m_adapters.erase( aType );
    return true;
}


std::optional<LIBRARY_MANAGER_ADAPTER*> LIBRARY_MANAGER::Adapter( LIBRARY_TABLE_TYPE aType ) const
{
    std::scoped_lock lock( m_adaptersMutex );

    if( m_adapters.contains( aType ) )
        return m_adapters.at( aType ).get();

    return std::nullopt;
}


std::optional<LIBRARY_TABLE*> LIBRARY_MANAGER::Table( LIBRARY_TABLE_TYPE aType,
                                                      LIBRARY_TABLE_SCOPE aScope )
{
    switch( aScope )
    {
    case LIBRARY_TABLE_SCOPE::BOTH:
    case LIBRARY_TABLE_SCOPE::UNINITIALIZED:
        wxCHECK_MSG( false, std::nullopt, "Table() requires a single scope" );

    case LIBRARY_TABLE_SCOPE::GLOBAL:
    {
        if( !m_tables.contains( aType ) )
        {
            wxLogTrace( traceLibraries, "WARNING: missing global table (%s)",
                        magic_enum::enum_name( aType ) );
            return std::nullopt;
        }

        return m_tables.at( aType ).get();
    }

    case LIBRARY_TABLE_SCOPE::PROJECT:
    {
        // TODO: handle multiple projects
        if( !m_projectTables.contains( aType ) )
        {
            if( !Pgm().GetSettingsManager().Prj().IsNullProject() )
                createEmptyTable( aType, LIBRARY_TABLE_SCOPE::PROJECT );
            else
                return std::nullopt;
        }

        return m_projectTables.at( aType ).get();
    }
    }

    return std::nullopt;
}


std::vector<LIBRARY_TABLE_ROW*> LIBRARY_MANAGER::Rows( LIBRARY_TABLE_TYPE aType, LIBRARY_TABLE_SCOPE aScope,
                                                       bool aIncludeInvalid ) const
{
    std::map<wxString, LIBRARY_TABLE_ROW*> rows;
    std::vector<wxString> rowOrder;

    std::list<std::ranges::ref_view<const std::map<LIBRARY_TABLE_TYPE, std::unique_ptr<LIBRARY_TABLE>>>> tables;

    switch( aScope )
    {
    case LIBRARY_TABLE_SCOPE::GLOBAL:
        tables = { std::views::all( m_tables ) };
        break;

    case LIBRARY_TABLE_SCOPE::PROJECT:
        tables = { std::views::all( m_projectTables ) };
        break;

    case LIBRARY_TABLE_SCOPE::BOTH:
        tables = { std::views::all( m_tables ), std::views::all( m_projectTables ) };
        break;

    case LIBRARY_TABLE_SCOPE::UNINITIALIZED:
        wxFAIL;
    }

    std::function<void(const std::unique_ptr<LIBRARY_TABLE>&, bool parentHidden)> processTable =
            [&]( const std::unique_ptr<LIBRARY_TABLE>& aTable, const bool parentHidden )
            {
                if( aTable->Type() != aType )
                    return;

                if( aTable->IsOk() || aIncludeInvalid )
                {
                    for( LIBRARY_TABLE_ROW& row : aTable->Rows() )
                    {
                        if( row.IsOk() || aIncludeInvalid )
                        {
                            // Hide child row if parent is hidden
                            if( parentHidden )
                                row.SetHidden( true );

                            if( row.Type() == LIBRARY_TABLE_ROW::TABLE_TYPE_NAME )
                            {
                                if( !m_childTables.contains( row.URI() ) )
                                    continue;

                                // Don't include libraries from disabled nested tables
                                if( row.Disabled() )
                                    continue;

                                processTable( m_childTables.at( row.URI() ), row.Hidden() );
                            }
                            else
                            {
                                if( !rows.contains( row.Nickname() ) )
                                    rowOrder.emplace_back( row.Nickname() );

                                rows[ row.Nickname() ] = &row;
                            }
                        }
                    }
                }
            };

    for( const std::unique_ptr<LIBRARY_TABLE>& table :
         std::views::join( tables ) | std::views::values )
    {
        processTable( table, false );
    }

    std::vector<LIBRARY_TABLE_ROW*> ret;

    for( const wxString& row : rowOrder )
        ret.emplace_back( rows[row] );

    return ret;
}


std::optional<LIBRARY_TABLE_ROW*> LIBRARY_MANAGER::GetRow( LIBRARY_TABLE_TYPE  aType, const wxString& aNickname,
                                                           LIBRARY_TABLE_SCOPE aScope )
{
    {
        std::lock_guard lock( m_rowCacheMutex );
        auto            key = std::make_tuple( aType, aScope, aNickname );

        if( auto it = m_rowCache.find( key ); it != m_rowCache.end() )
            return it->second;
    }

    for( LIBRARY_TABLE_ROW* row : Rows( aType, aScope, true ) )
    {
        if( row->Nickname() == aNickname )
        {
            std::lock_guard lock( m_rowCacheMutex );
            m_rowCache[std::make_tuple( aType, aScope, aNickname )] = row;
            return row;
        }
    }

    return std::nullopt;
}


std::optional<LIBRARY_TABLE_ROW*> LIBRARY_MANAGER::FindRowByURI( LIBRARY_TABLE_TYPE aType,
        const wxString& aUri,
        LIBRARY_TABLE_SCOPE aScope ) const
{
    for( LIBRARY_TABLE_ROW* row : Rows( aType, aScope, true ) )
    {
        if( UrisAreEquivalent( GetFullURI( row, true ), aUri ) )
            return row;
    }

    return std::nullopt;
}


void LIBRARY_MANAGER::ReloadLibraryEntry( LIBRARY_TABLE_TYPE aType, const wxString& aNickname,
                                          LIBRARY_TABLE_SCOPE aScope )
{
    if( std::optional<LIBRARY_MANAGER_ADAPTER*> adapter = Adapter( aType ); adapter )
        ( *adapter )->ReloadLibraryEntry( aNickname, aScope );
}


std::optional<LIB_STATUS> LIBRARY_MANAGER::LoadLibraryEntry( LIBRARY_TABLE_TYPE aType,
                                                              const wxString& aNickname )
{
    if( std::optional<LIBRARY_MANAGER_ADAPTER*> adapter = Adapter( aType ); adapter )
        return ( *adapter )->LoadLibraryEntry( aNickname );

    return std::nullopt;
}


void LIBRARY_MANAGER::LoadProjectTables( const wxString& aProjectPath,
                                         std::initializer_list<LIBRARY_TABLE_TYPE> aTablesToLoad )
{
    // Cancel any in-progress loads and clear adapter caches before destroying project
    // tables. Cached LIB_DATA entries hold raw LIBRARY_TABLE_ROW pointers into the old
    // tables, which would dangle once loadTables() replaces them. Mirrors the safety
    // ordering in LoadGlobalTables().
    {
        std::scoped_lock lock( m_adaptersMutex );

        for( const std::unique_ptr<LIBRARY_MANAGER_ADAPTER>& adapter : m_adapters | std::views::values )
            adapter->ProjectTablesChanged( aTablesToLoad );
    }

    if( wxFileName::IsDirReadable( aProjectPath ) )
    {
        loadTables( aProjectPath, LIBRARY_TABLE_SCOPE::PROJECT, aTablesToLoad );
    }
    else
    {
        // loadTables() would have cleared m_rowCache before rebuilding the new
        // table; do the same here so cached entries don't point into the
        // project tables we are about to destroy.
        {
            std::lock_guard lock( m_rowCacheMutex );
            m_rowCache.clear();
        }

        m_projectTables.clear();
        wxLogTrace( traceLibraries, "New project path %s is not readable, not loading project tables", aProjectPath );
    }

    // Phase 2: let adapters reconcile their cache against the rebuilt project
    // table. This erases sentinels installed by ProjectTablesChanged() for any
    // nickname that no longer has a project row, so a library removed from the
    // project table stops masking a same-named global library.
    {
        std::scoped_lock lock( m_adaptersMutex );

        for( const std::unique_ptr<LIBRARY_MANAGER_ADAPTER>& adapter : m_adapters | std::views::values )
            adapter->ProjectTablesReloaded( aTablesToLoad );
    }
}


void LIBRARY_MANAGER::ReloadTables( LIBRARY_TABLE_SCOPE aScope,
                                    std::initializer_list<LIBRARY_TABLE_TYPE> aTablesToLoad )
{
    if( aScope == LIBRARY_TABLE_SCOPE::PROJECT )
    {
        AbortAsyncLoads();
        LoadProjectTables( aTablesToLoad );
    }
    else
    {
        LoadGlobalTables( aTablesToLoad );
    }
}


std::optional<wxString> LIBRARY_MANAGER::GetFullURI( LIBRARY_TABLE_TYPE aType, const wxString& aNickname,
                                                     bool aSubstituted )
{
    if( std::optional<const LIBRARY_TABLE_ROW*> result = GetRow( aType, aNickname ) )
        return GetFullURI( *result, aSubstituted );

    return std::nullopt;
}


wxString LIBRARY_MANAGER::GetFullURI( const LIBRARY_TABLE_ROW* aRow, bool aSubstituted )
{
    if( aSubstituted )
        return ExpandEnvVarSubstitutions( aRow->URI(), &Pgm().GetSettingsManager().Prj() );

    return aRow->URI();
}


wxString LIBRARY_MANAGER::ExpandURI( const wxString& aShortURI, const PROJECT& aProject )
{
    wxLogNull doNotLog; // We do our own error reporting; we don't want to hear about missing envvars

    wxFileName path( ExpandEnvVarSubstitutions( aShortURI, &aProject ) );
    path.MakeAbsolute();
    return path.GetFullPath();
}


bool LIBRARY_MANAGER::IsPcmManagedRow( const LIBRARY_TABLE_ROW& aRow )
{
    // PCM_LIB_TRAVERSER stores URIs of the form
    //     ${KICADn_3RD_PARTY}/<category>/<pkgid>/<name>.<ext>
    // where <category> is one of the fixed PCM content folders. Matching this full
    // template is what uniquely identifies a PCM-added row. Matching only the leading
    // ${KICADn_3RD_PARTY} token is not sufficient because users routinely repurpose
    // that env var to point at their own library collection (as they did with
    // KICAD8_3RD_PARTY in earlier versions). A row the user added by hand under their
    // repurposed 3RD_PARTY directory must never be treated as PCM-managed, or the
    // auto-remove pass would silently delete it.
    const wxString& uri = aRow.URI();

    if( !uri.StartsWith( wxS( "${" ) ) )
        return false;

    size_t end = uri.find( wxS( '}' ) );

    if( end == wxString::npos || end <= 2 )
        return false;

    wxString varName = uri.SubString( 2, end - 1 );

    if( !ENV_VAR::IsVersionedEnvVar( varName, wxS( "3RD_PARTY" ) ) )
        return false;

    // PCM_LIB_TRAVERSER always joins the URI with '/', so the token must be followed by a
    // forward slash; a backslash or missing separator (e.g. "${KICAD10_3RD_PARTY}symbols/...")
    // was never emitted by PCM.
    if( end + 1 >= uri.length() || uri[end + 1] != wxS( '/' ) )
        return false;

    // PCM_LIB_TRAVERSER nests libraries at least as <category>/<pkgid>/<library>, so there
    // must be a category folder, at least one package-id folder, and a library leaf, with no
    // empty components. A user library placed directly under the repurposed 3RD_PARTY root
    // (or in a same-named folder with no package level) does not match.
    wxArrayString parts = wxSplit( uri.Mid( end + 2 ), '/', '\0' );

    if( parts.size() < 3 )
        return false;

    for( const wxString& part : parts )
    {
        if( part.IsEmpty() )
            return false;
    }

    wxString category = parts[0];
    wxString leaf = parts.Last();

    // The leaf must carry the category-appropriate library extension over a non-empty stem;
    // a bare extension (e.g. a hidden ".kicad_sym") is never a PCM library and matching it
    // would let the auto-remove pass delete an unrelated file in a same-named folder.
    auto hasLibExtension = [&leaf]( const wxString& aExt )
    {
        return leaf.length() > aExt.length() && leaf.EndsWith( aExt );
    };

    if( category == wxS( "symbols" ) )
        return hasLibExtension( wxS( ".kicad_sym" ) );

    if( category == wxS( "footprints" ) )
        return hasLibExtension( wxS( ".pretty" ) );

    if( category == wxS( "design_blocks" ) )
    {
        static const wxString designBlockExt =
                wxString::Format( wxS( ".%s" ), FILEEXT::KiCadDesignBlockLibPathExtension );

        return hasLibExtension( designBlockExt );
    }

    return false;
}


bool LIBRARY_MANAGER::UrisAreEquivalent( const wxString& aURI1, const wxString& aURI2 )
{
    // Avoid comparing filenames as wxURIs
    if( aURI1.Find( "://" ) != wxNOT_FOUND )
    {
        // found as full path
        return aURI1 == aURI2;
    }
    else
    {
        const wxFileName fn1( aURI1 );
        const wxFileName fn2( aURI2 );

        // This will also test if the file is a symlink so if we are comparing
        // a symlink to the same real file, the comparison will be true.  See
        // wxFileName::SameAs() in the wxWidgets source.

        // found as full path and file name
        return fn1 == fn2;
    }
}


//////  LIBRARY_MANAGER_ADAPTER
///
///


LIBRARY_MANAGER_ADAPTER::LIBRARY_MANAGER_ADAPTER( LIBRARY_MANAGER& aManager ) :
        m_manager( aManager )
{
}


LIBRARY_MANAGER_ADAPTER::~LIBRARY_MANAGER_ADAPTER()
{
}


LIBRARY_MANAGER& LIBRARY_MANAGER_ADAPTER::Manager() const
{
    return m_manager;
}


void LIBRARY_MANAGER_ADAPTER::ProjectChanged()
{
    resetProjectCache();
}


void LIBRARY_MANAGER_ADAPTER::resetProjectCache()
{
    abortLoad();

    std::unique_lock lock( m_librariesMutex );

    // Reset entries in place rather than erasing them. Erasing would let
    // fetchIfLoaded() fall through to globalLibs() for any nickname that is
    // shadowed by a project library, defeating the project-over-global
    // precedence enforced by LIBRARY_MANAGER::Rows(). ProjectTablesReloaded()
    // later prunes any sentinels whose nicknames are no longer in the rebuilt
    // project table, so stale shadowing cannot persist.
    for( auto& entry : m_libraries )
        entry.second = LIB_DATA{};
}


void LIBRARY_MANAGER_ADAPTER::AbortAsyncLoad()
{
    abortLoad();
}


void LIBRARY_MANAGER_ADAPTER::GlobalTablesChanged( std::initializer_list<LIBRARY_TABLE_TYPE> aChangedTables )
{
    bool me = aChangedTables.size() == 0;

    for( LIBRARY_TABLE_TYPE type : aChangedTables )
    {
        if( type == Type() )
        {
            me = true;
            break;
        }
    }

    if( !me )
        return;

    abortLoad();

    {
        std::unique_lock lock( globalLibsMutex() );
        globalLibs().clear();
    }
}


void LIBRARY_MANAGER_ADAPTER::ProjectTablesChanged( std::initializer_list<LIBRARY_TABLE_TYPE> aChangedTables )
{
    bool me = aChangedTables.size() == 0;

    for( LIBRARY_TABLE_TYPE type : aChangedTables )
    {
        if( type == Type() )
        {
            me = true;
            break;
        }
    }

    if( !me )
        return;

    resetProjectCache();
}


void LIBRARY_MANAGER_ADAPTER::ProjectTablesReloaded(
        std::initializer_list<LIBRARY_TABLE_TYPE> aChangedTables )
{
    bool me = aChangedTables.size() == 0;

    for( LIBRARY_TABLE_TYPE type : aChangedTables )
    {
        if( type == Type() )
        {
            me = true;
            break;
        }
    }

    if( !me )
        return;

    // Erase sentinels installed by resetProjectCache() for nicknames that no
    // longer appear in the rebuilt project table. Without this, a library
    // removed from the project would remain masked in m_libraries and hide a
    // same-named global library from HasLibrary() / fetchIfLoaded() / etc.
    // GetRow() is safe to call here: loadTables() reset m_rowCache before
    // building the new table, and async loads were aborted in phase 1.
    std::unique_lock lock( m_librariesMutex );

    std::erase_if( m_libraries,
                   [this]( const auto& aEntry )
                   {
                       return !m_manager.GetRow( Type(), aEntry.first,
                                                 LIBRARY_TABLE_SCOPE::PROJECT ).has_value();
                   } );
}


void LIBRARY_MANAGER_ADAPTER::CheckTableRow( LIBRARY_TABLE_ROW& aRow )
{
    // Testing is expensive; skip it if we already have a library with the same
    // nickname and URI as the row under test
    if( std::optional<LIB_DATA*> libData = fetchIfLoaded( aRow.Nickname() ) )
    {
        const LIBRARY_TABLE_ROW* loadedRow = ( *libData )->row;

        if( loadedRow->URI() == aRow.URI() && loadedRow->Type() == aRow.Type() )
        {
            aRow.SetOk( loadedRow->IsOk() );
            return;
        }
    }

    abortLoad();

    LIBRARY_RESULT<IO_BASE*> plugin = createPlugin( &aRow );

    if( plugin.has_value() )
    {
        LIB_DATA lib;
        lib.row = &aRow;
        lib.plugin.reset( *plugin );

        std::optional<LIB_STATUS> status = LoadOne( &lib );

        if( status.has_value() )
        {
            aRow.SetOk( status.value().load_status == LOAD_STATUS::LOADED );

            if( status.value().error.has_value() )
                aRow.SetErrorDescription( status.value().error.value().message );
        }
    }
    else if( plugin.error().message == LIBRARY_TABLE_OK().message )
    {
        aRow.SetOk( true );
        aRow.SetErrorDescription( wxEmptyString );
    }
    else
    {
        aRow.SetOk( false );
        aRow.SetErrorDescription( plugin.error().message );
    }
}



LIBRARY_TABLE* LIBRARY_MANAGER_ADAPTER::GlobalTable() const
{
    wxCHECK( m_manager.Table( Type(), LIBRARY_TABLE_SCOPE::GLOBAL ), nullptr );
    return *m_manager.Table( Type(), LIBRARY_TABLE_SCOPE::GLOBAL );
}


std::optional<LIBRARY_TABLE*> LIBRARY_MANAGER_ADAPTER::ProjectTable() const
{
    return m_manager.Table( Type(), LIBRARY_TABLE_SCOPE::PROJECT );
}


std::optional<wxString> LIBRARY_MANAGER_ADAPTER::FindLibraryByURI( const wxString& aURI ) const
{
    for( const LIBRARY_TABLE_ROW* row : m_manager.Rows( Type() ) )
    {
        if( LIBRARY_MANAGER::UrisAreEquivalent( row->URI(), aURI ) )
            return row->Nickname();
    }

    return std::nullopt;
}


std::vector<wxString> LIBRARY_MANAGER_ADAPTER::GetLibraryNames() const
{
    std::vector<wxString> ret;
    std::vector<LIBRARY_TABLE_ROW*> rows = m_manager.Rows( Type() );

    wxLogTrace( traceLibraries, "GetLibraryNames: checking %zu rows from table", rows.size() );

    for( const LIBRARY_TABLE_ROW* row : rows )
    {
        wxString nickname = row->Nickname();
        std::optional<const LIB_DATA*> loaded = fetchIfLoaded( nickname );

        if( loaded )
            ret.emplace_back( nickname );
    }

    StrNumSort( ret, CASE_SENSITIVITY::INSENSITIVE );

    wxLogTrace( traceLibraries, "GetLibraryNames: returning %zu of %zu libraries", ret.size(), rows.size() );
    return ret;
}


bool LIBRARY_MANAGER_ADAPTER::HasLibrary( const wxString& aNickname, bool aCheckEnabled ) const
{
    std::optional<const LIB_DATA*> r = fetchIfLoaded( aNickname );

    if( r.has_value() )
        return !aCheckEnabled || !r.value()->row->Disabled();

    return false;
}


bool LIBRARY_MANAGER_ADAPTER::DeleteLibrary( const wxString& aNickname )
{
    if( LIBRARY_RESULT<LIB_DATA*> result = loadIfNeeded( aNickname ); result.has_value() )
    {
        LIB_DATA* data = *result;
        std::map<std::string, UTF8> options = data->row->GetOptionsMap();

        // Serialize cache teardown against readers. loadIfNeeded() already dropped the manager lock
        // (manager > pluginMutex).
        std::lock_guard pluginGuard( pluginMutex( aNickname ) );

        try
        {
            return data->plugin->DeleteLibrary( getUri( data->row ), &options );
        }
        catch( ... )
        {
            return false;
        }
    }

    return false;
}


std::optional<wxString> LIBRARY_MANAGER_ADAPTER::GetLibraryDescription( const wxString& aNickname ) const
{
    if( std::optional<const LIB_DATA*> optRow = fetchIfLoaded( aNickname ); optRow )
        return ( *optRow )->row->Description();

    return std::nullopt;
}


std::vector<LIBRARY_TABLE_ROW*> LIBRARY_MANAGER_ADAPTER::Rows( LIBRARY_TABLE_SCOPE aScope,
                                                               bool aIncludeInvalid ) const
{
    return m_manager.Rows( Type(), aScope, aIncludeInvalid );
}


std::optional<LIBRARY_TABLE_ROW*> LIBRARY_MANAGER_ADAPTER::GetRow( const wxString &aNickname,
                                                                   LIBRARY_TABLE_SCOPE aScope ) const
{
    return m_manager.GetRow( Type(), aNickname, aScope );
}


std::optional<LIBRARY_TABLE_ROW*> LIBRARY_MANAGER_ADAPTER::FindRowByURI(
        const wxString& aUri,
        LIBRARY_TABLE_SCOPE aScope ) const
{
    return m_manager.FindRowByURI( Type(), aUri, aScope );
}


void LIBRARY_MANAGER_ADAPTER::abortLoad()
{
    {
        std::lock_guard lock( m_loadMutex );

        if( m_futures.empty() )
            return;

        wxLogTrace( traceLibraries, "Aborting library load..." );
        m_abort.store( true );
    }

    BlockUntilLoaded();
    wxLogTrace( traceLibraries, "Aborted" );

    {
        std::lock_guard lock( m_loadMutex );
        m_abort.store( false );
        m_futures.clear();
        m_loadTotal.store( 0 );
        m_loadCount.store( 0 );
    }
}


std::optional<float> LIBRARY_MANAGER_ADAPTER::AsyncLoadProgress() const
{
    size_t total = m_loadTotal.load();

    if( total == 0 )
        return std::nullopt;

    size_t loaded = m_loadCount.load();
    return loaded / static_cast<float>( total );
}


void LIBRARY_MANAGER_ADAPTER::BlockUntilLoaded()
{
    wxLogTrace( traceLibraries, "BlockUntilLoaded: entry, acquiring m_loadMutex" );
    std::unique_lock<std::mutex> asyncLock( m_loadMutex );

    wxLogTrace( traceLibraries, "BlockUntilLoaded: waiting on %zu futures", m_futures.size() );

    for( const std::future<void>& future : m_futures )
        future.wait();

    wxLogTrace( traceLibraries, "BlockUntilLoaded: all futures complete, loadCount=%zu, loadTotal=%zu",
                m_loadCount.load(), m_loadTotal.load() );
}


bool LIBRARY_MANAGER_ADAPTER::IsLibraryLoaded( const wxString& aNickname )
{
    {
        std::shared_lock lock( m_librariesMutex );

        if( auto it = m_libraries.find( aNickname ); it != m_libraries.end() )
            return it->second.status.load_status == LOAD_STATUS::LOADED;
    }

    {
        std::shared_lock lock( globalLibsMutex() );

        if( auto it = globalLibs().find( aNickname ); it != globalLibs().end() )
            return it->second.status.load_status == LOAD_STATUS::LOADED;
    }

    return false;
}


std::optional<LIBRARY_ERROR> LIBRARY_MANAGER_ADAPTER::LibraryError( const wxString& aNickname ) const
{
    {
        std::shared_lock lock( m_librariesMutex );

        if( auto it = m_libraries.find( aNickname ); it != m_libraries.end() )
            return it->second.status.error;
    }

    {
        std::shared_lock lock( globalLibsMutex() );

        if( auto it = globalLibs().find( aNickname ); it != globalLibs().end() )
            return it->second.status.error;
    }

    return std::nullopt;
}


std::vector<std::pair<wxString, LIB_STATUS>> LIBRARY_MANAGER_ADAPTER::GetLibraryStatuses() const
{
    std::vector<std::pair<wxString, LIB_STATUS>> ret;

    for( const LIBRARY_TABLE_ROW* row : m_manager.Rows( Type() ) )
    {
        if( row->Disabled() )
            continue;

        if( std::optional<LIB_STATUS> result = GetLibraryStatus( row->Nickname() ) )
        {
            ret.emplace_back( std::make_pair( row->Nickname(), *result ) );
        }
        else
        {
            // This should probably never happen, but until that can be proved...
            ret.emplace_back( std::make_pair( row->Nickname(),
                                              LIB_STATUS( {
                                                  .load_status = LOAD_STATUS::LOAD_ERROR,
                                                  .error = LIBRARY_ERROR( _( "Library not found in library table" ) )
                                              } ) ) );
        }
    }

    return ret;
}


wxString LIBRARY_MANAGER_ADAPTER::GetLibraryLoadErrors() const
{
    wxString errors;

    for( const auto& [nickname, status] : GetLibraryStatuses() )
    {
        if( status.load_status == LOAD_STATUS::LOAD_ERROR && status.error )
        {
            if( !errors.IsEmpty() )
                errors += wxS( "\n" );

            errors += wxString::Format( _( "Library '%s': %s" ),
                                         nickname, status.error->message );
        }
    }

    return errors;
}


std::optional<LIB_STATUS> LIBRARY_MANAGER_ADAPTER::LoadLibraryEntry( const wxString& aNickname )
{
    LIBRARY_RESULT<LIB_DATA*> result = loadIfNeeded( aNickname );

    if( result.has_value() )
        return LoadOne( *result );

    return std::nullopt;
}


void LIBRARY_MANAGER_ADAPTER::ReloadLibraryEntry( const wxString& aNickname, LIBRARY_TABLE_SCOPE aScope )
{
    // Drain the async load before erasing, else a worker mid-enumerate writes status through the
    // freed LIB_DATA (every other invalidator already does this).
    abortLoad();

    auto reloadScope =
            [&]( LIBRARY_TABLE_SCOPE aScopeToReload, std::map<wxString, LIB_DATA>& aTarget,
                 std::shared_mutex& aMutex )
            {
                bool wasLoaded = false;

                {
                    std::unique_lock lock( aMutex );
                    auto it = aTarget.find( aNickname );

                    if( it != aTarget.end() && it->second.plugin )
                    {
                        wasLoaded = true;
                        aTarget.erase( it );
                    }
                }

                if( wasLoaded )
                {
                    LIBRARY_RESULT<LIB_DATA*> result = loadFromScope( aNickname, aScopeToReload, aTarget, aMutex );

                    if( !result.has_value() )
                    {
                        wxLogTrace( traceLibraries, "ReloadLibraryEntry: failed to reload %s (%s): %s",
                                    aNickname, magic_enum::enum_name( aScopeToReload ),
                                    result.error().message );
                    }
                }
            };

    switch( aScope )
    {
    case LIBRARY_TABLE_SCOPE::GLOBAL:
        reloadScope( LIBRARY_TABLE_SCOPE::GLOBAL, globalLibs(), globalLibsMutex() );
        break;

    case LIBRARY_TABLE_SCOPE::PROJECT:
        reloadScope( LIBRARY_TABLE_SCOPE::PROJECT, m_libraries, m_librariesMutex );
        break;

    case LIBRARY_TABLE_SCOPE::BOTH:
    case LIBRARY_TABLE_SCOPE::UNINITIALIZED:
        reloadScope( LIBRARY_TABLE_SCOPE::PROJECT, m_libraries, m_librariesMutex );
        reloadScope( LIBRARY_TABLE_SCOPE::GLOBAL, globalLibs(), globalLibsMutex() );
        break;
    }
}


bool LIBRARY_MANAGER_ADAPTER::IsWritable( const wxString& aNickname ) const
{
    if( std::optional<const LIB_DATA*> result = fetchIfLoaded( aNickname ) )
    {
        const LIB_DATA* rowData = *result;

        // IsLibraryWritable() may rebuild the cache via validateCache(); serialize against readers.
        // fetchIfLoaded() already dropped the manager lock.
        std::lock_guard pluginGuard( pluginMutex( aNickname ) );

        return rowData->plugin->IsLibraryWritable( getUri( rowData->row ) );
    }

    return false;
}


bool LIBRARY_MANAGER_ADAPTER::CreateLibrary( const wxString& aNickname )
{
    if( LIBRARY_RESULT<LIB_DATA*> result = loadIfNeeded( aNickname ); result.has_value() )
    {
        LIB_DATA* data = *result;
        std::map<std::string, UTF8> options = data->row->GetOptionsMap();

        // Serialize cache replacement against readers; loadIfNeeded() already dropped the manager lock.
        std::lock_guard pluginGuard( pluginMutex( aNickname ) );

        try
        {
            data->plugin->CreateLibrary( getUri( data->row ), &options );
            return true;
        }
        catch( const IO_ERROR& ioe )
        {
            wxLogTrace( traceLibraries, "CreateLibrary: IO_ERROR for %s: %s",
                        aNickname, ioe.What() );
            return false;
        }
        catch( const std::exception& e )
        {
            wxLogTrace( traceLibraries, "CreateLibrary: std::exception for %s: %s",
                        aNickname, e.what() );
            return false;
        }
    }

    wxLogTrace( traceLibraries, "CreateLibrary: library row '%s' not found", aNickname );
    return false;
}


wxString LIBRARY_MANAGER_ADAPTER::getUri( const LIBRARY_TABLE_ROW* aRow )
{
    return LIBRARY_MANAGER::ExpandURI( aRow->URI(), Pgm().GetSettingsManager().Prj() );
}


std::optional<const LIB_DATA*> LIBRARY_MANAGER_ADAPTER::fetchIfLoaded( const wxString& aNickname ) const
{
    {
        std::shared_lock lock( m_librariesMutex );

        if( auto it = m_libraries.find( aNickname ); it != m_libraries.end() )
        {
            if( it->second.status.load_status == LOAD_STATUS::LOADED )
                return &it->second;

            return std::nullopt;
        }
    }

    {
        std::shared_lock lock( globalLibsMutex() );

        if( auto it = globalLibs().find( aNickname ); it != globalLibs().end() )
        {
            if( it->second.status.load_status == LOAD_STATUS::LOADED )
                return &it->second;

            return std::nullopt;
        }
    }

    return std::nullopt;
}


std::optional<LIB_DATA*> LIBRARY_MANAGER_ADAPTER::fetchIfLoaded( const wxString& aNickname )
{
    {
        std::shared_lock lock( m_librariesMutex );

        if( auto it = m_libraries.find( aNickname ); it != m_libraries.end() )
        {
            if( it->second.status.load_status == LOAD_STATUS::LOADED )
                return &it->second;

            return std::nullopt;
        }
    }

    {
        std::shared_lock lock( globalLibsMutex() );

        if( auto it = globalLibs().find( aNickname ); it != globalLibs().end() )
        {
            if( it->second.status.load_status == LOAD_STATUS::LOADED )
                return &it->second;

            return std::nullopt;
        }
    }

    return std::nullopt;
}


LIBRARY_RESULT<LIB_DATA*> LIBRARY_MANAGER_ADAPTER::loadFromScope( const wxString& aNickname,
                                                                  LIBRARY_TABLE_SCOPE aScope,
                                                                  std::map<wxString, LIB_DATA>& aTarget,
                                                                  std::shared_mutex& aMutex )
{
    bool present = false;

    {
        std::shared_lock lock( aMutex );
        present = aTarget.contains( aNickname ) && aTarget.at( aNickname ).plugin;
    }

    if( !present )
    {
        if( auto result = m_manager.GetRow( Type(), aNickname, aScope ) )
        {
            const LIBRARY_TABLE_ROW* row = *result;
            wxLogTrace( traceLibraries, "Library %s (%s) not yet loaded, will attempt...",
                        aNickname, magic_enum::enum_name( aScope ) );

            if( LIBRARY_RESULT<IO_BASE*> plugin = createPlugin( row ); plugin.has_value() )
            {
                std::unique_lock lock( aMutex );

                aTarget[ row->Nickname() ].status.load_status = LOAD_STATUS::LOADING;
                aTarget[ row->Nickname() ].row = row;
                aTarget[ row->Nickname() ].plugin.reset( *plugin );

                return &aTarget.at( aNickname );
            }
            else
            {
                return tl::unexpected( plugin.error() );
            }
        }

        return nullptr;
    }

    std::shared_lock lock( aMutex );
    return &aTarget.at( aNickname );
}


LIBRARY_RESULT<LIB_DATA*> LIBRARY_MANAGER_ADAPTER::loadIfNeeded( const wxString& aNickname )
{
    LIBRARY_RESULT<LIB_DATA*> result = loadFromScope( aNickname, LIBRARY_TABLE_SCOPE::PROJECT,
                                                      m_libraries, m_librariesMutex );

    if( !result.has_value() || *result )
        return result;

    result = loadFromScope( aNickname, LIBRARY_TABLE_SCOPE::GLOBAL, globalLibs(), globalLibsMutex() );

    if( !result.has_value() || *result )
        return result;

    wxString msg = wxString::Format( _( "Library %s not found" ), aNickname );
    return tl::unexpected( LIBRARY_ERROR( msg ) );
}


std::optional<LIB_STATUS> LIBRARY_MANAGER_ADAPTER::GetLibraryStatus( const wxString& aNickname ) const
{
    {
        std::shared_lock lock( m_librariesMutex );

        if( auto it = m_libraries.find( aNickname ); it != m_libraries.end() )
            return it->second.status;
    }

    {
        std::shared_lock lock( globalLibsMutex() );

        if( auto it = globalLibs().find( aNickname ); it != globalLibs().end() )
            return it->second.status;
    }

    return std::nullopt;
}


void LIBRARY_MANAGER_ADAPTER::AsyncLoad()
{
    std::unique_lock<std::mutex> asyncLock( m_loadMutex, std::try_to_lock );

    if( !asyncLock )
        return;

    std::erase_if( m_futures,
                   []( std::future<void>& aFuture )
                   {
                       return aFuture.valid()
                              && aFuture.wait_for( 0s ) == std::future_status::ready;
                   } );

    if( !m_futures.empty() )
    {
        wxLogTrace( traceLibraries, "Cannot AsyncLoad, futures from a previous call remain!" );
        return;
    }

    std::vector<LIBRARY_TABLE_ROW*> rows = m_manager.Rows( Type() );

    m_loadTotal.store( rows.size() );
    m_loadCount.store( 0 );

    if( rows.empty() )
    {
        wxLogTrace( traceLibraries, "AsyncLoad: no libraries left to load; exiting" );
        return;
    }

    thread_pool& tp = GetKiCadThreadPool();

    auto check =
            [&]( const wxString& aLib, std::map<wxString, LIB_DATA>& aMap, std::shared_mutex& aMutex )
            {
                std::shared_lock lock( aMutex );

                if( auto it = aMap.find( aLib ); it != aMap.end() )
                {
                    LOAD_STATUS status = it->second.status.load_status;

                    if( status == LOAD_STATUS::LOADED || status == LOAD_STATUS::LOADING )
                        return true;
                }

                return false;
            };

    // Collect work items with pre-resolved URIs. URI expansion accesses PROJECT data
    // (text variables, env vars) that is not thread-safe, so resolve on the calling thread.
    struct LOAD_WORK
    {
        wxString              nickname;
        LIBRARY_TABLE_SCOPE   scope;
        wxString              uri;
    };

    auto workQueue = std::make_shared<std::vector<LOAD_WORK>>();
    workQueue->reserve( rows.size() );

    for( const LIBRARY_TABLE_ROW* row : rows )
    {
        wxString nickname = row->Nickname();
        LIBRARY_TABLE_SCOPE scope = row->Scope();

        if( check( nickname, m_libraries, m_librariesMutex ) )
        {
            m_loadTotal.fetch_sub( 1 );
            continue;
        }

        if( check( nickname, globalLibs(), globalLibsMutex() ) )
        {
            m_loadTotal.fetch_sub( 1 );
            continue;
        }

        workQueue->emplace_back( LOAD_WORK{ nickname, scope, getUri( row ) } );
    }

    if( workQueue->empty() )
    {
        wxLogTrace( traceLibraries, "AsyncLoad: all libraries already loaded; exiting" );
        return;
    }

    // Cap loading threads to leave headroom for the GUI and other thread pool work.
    // Each worker pulls libraries from a shared queue, so we submit fewer tasks than
    // libraries and avoid flooding the pool.
    size_t poolSize = tp.get_thread_count();
    size_t maxLoadThreads = std::max<size_t>( 1, poolSize > 2 ? poolSize - 2 : 1 );
    size_t numWorkers = std::min( maxLoadThreads, workQueue->size() );

    auto workIndex = std::make_shared<std::atomic<size_t>>( 0 );

    wxLogTrace( traceLibraries, "AsyncLoad: %zu libraries to load, using %zu worker threads (pool has %zu)",
                workQueue->size(), numWorkers, poolSize );

    for( size_t w = 0; w < numWorkers; ++w )
    {
        m_futures.emplace_back( tp.submit_task(
                [this, workQueue, workIndex]()
                {
                    while( true )
                    {
                        if( m_abort.load() )
                            return;

                        size_t idx = workIndex->fetch_add( 1 );

                        if( idx >= workQueue->size() )
                            return;

                        const LOAD_WORK& work = ( *workQueue )[idx];
                        LIBRARY_RESULT<LIB_DATA*> result = loadIfNeeded( work.nickname );

                        if( result.has_value() )
                        {
                            LIB_DATA* lib = *result;

                            try
                            {
                                {
                                    std::unique_lock lock(
                                            work.scope == LIBRARY_TABLE_SCOPE::GLOBAL
                                                    ? globalLibsMutex()
                                                    : m_librariesMutex );
                                    lib->status.load_status = LOAD_STATUS::LOADING;
                                }

                                enumerateLibrary( lib, work.uri );

                                {
                                    std::unique_lock lock(
                                            work.scope == LIBRARY_TABLE_SCOPE::GLOBAL
                                                    ? globalLibsMutex()
                                                    : m_librariesMutex );
                                    lib->status.load_status = LOAD_STATUS::LOADED;
                                }
                            }
                            catch( IO_ERROR& e )
                            {
                                std::unique_lock lock(
                                        work.scope == LIBRARY_TABLE_SCOPE::GLOBAL
                                                ? globalLibsMutex()
                                                : m_librariesMutex );
                                lib->status.load_status = LOAD_STATUS::LOAD_ERROR;
                                lib->status.error = LIBRARY_ERROR( { e.What() } );
                                wxLogTrace( traceLibraries, "%s: plugin threw exception: %s",
                                            work.nickname, e.What() );
                            }
                        }
                        else
                        {
                            std::unique_lock lock(
                                    work.scope == LIBRARY_TABLE_SCOPE::GLOBAL
                                            ? globalLibsMutex()
                                            : m_librariesMutex );

                            std::map<wxString, LIB_DATA>& target =
                                    ( work.scope == LIBRARY_TABLE_SCOPE::GLOBAL ) ? globalLibs()
                                                                                  : m_libraries;

                            target[work.nickname].status = LIB_STATUS( {
                                    .load_status = LOAD_STATUS::LOAD_ERROR,
                                    .error = result.error()
                            } );
                        }

                        ++m_loadCount;
                    }
                }, BS::pr::lowest ) );
    }

    wxLogTrace( traceLibraries, "Started async load of %zu libraries", workQueue->size() );
}
