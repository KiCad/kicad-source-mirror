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

#include <common.h>
#include <env_vars.h>
#include <list>
#include <magic_enum.hpp>
#include <unordered_set>

#include <paths.h>
#include <pgm_base.h>
#include <richio.h>
#include <trace_helpers.h>
#include <wildcards_and_files_ext.h>

#include <libraries/library_manager.h>
#include <settings/kicad_settings.h>
#include <settings/settings_manager.h>
#include <wx/dir.h>
#include <wx/log.h>

struct LIBRARY_MANAGER_INTERNALS
{
    std::vector<LIBRARY_TABLE> tables;
};


LIBRARY_MANAGER::LIBRARY_MANAGER()
{
}


LIBRARY_MANAGER::~LIBRARY_MANAGER() = default;

const std::map<LIBRARY_TABLE_TYPE, const std::string&> LIBRARY_MANAGER::m_typeToFilenameMap =
        { { LIBRARY_TABLE_TYPE::SYMBOL,       FILEEXT::SymbolLibraryTableFileName },
          { LIBRARY_TABLE_TYPE::FOOTPRINT,    FILEEXT::FootprintLibraryTableFileName },
          { LIBRARY_TABLE_TYPE::DESIGN_BLOCK, FILEEXT::DesignBlockLibraryTableFileName } };


void LIBRARY_MANAGER::loadTables( const wxString& aTablePath, LIBRARY_TABLE_SCOPE aScope )
{
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

    aTarget.clear();

    for( const std::string& name : { FILEEXT::SymbolLibraryTableFileName,
                                     FILEEXT::FootprintLibraryTableFileName,
                                     FILEEXT::DesignBlockLibraryTableFileName } )
    {
        wxFileName fn( aTablePath, name );

        if( fn.IsFileReadable() )
        {
            auto table = std::make_unique<LIBRARY_TABLE>( fn, aScope );
            aTarget[table->Type()] = std::move( table );
        }
        else
        {
            wxLogTrace( traceLibraries, "No library table found at %s", fn.GetFullPath() );
        }
    }

    for( const std::unique_ptr<LIBRARY_TABLE>& t : aTarget | std::views::values )
        loadNestedTables( *t );
}


void LIBRARY_MANAGER::loadNestedTables( LIBRARY_TABLE& aRootTable )
{
    std::unordered_set<wxString> seenTables;

    std::function<void(LIBRARY_TABLE&)> processOneTable =
        [&]( LIBRARY_TABLE& aTable )
        {
            seenTables.insert( aTable.Path() );

            if( !aTable.IsOk() )
                return;

            for( LIBRARY_TABLE_ROW& row : aTable.Rows() )
            {
                if( row.Type() == LIBRARY_TABLE_ROW::TABLE_TYPE_NAME )
                {
                    wxFileName file( row.URI() );

                    // URI may be relative to parent
                    file.MakeAbsolute( wxFileName( aTable.Path() ).GetPath() );

                    WX_FILENAME::ResolvePossibleSymlinks( file );
                    wxString src = file.GetFullPath();

                    if( seenTables.contains( src ) )
                    {
                        wxLogTrace( traceLibraries, "Library table %s has already been loaded!",
                                    src );
                        row.SetOk( false );
                        row.SetErrorDescription(
                            _( "A reference to this library table already exists" ) );
                        continue;
                    }

                    auto child = std::make_unique<LIBRARY_TABLE>( file, aRootTable.Scope() );

                    processOneTable( *child );

                    if( !child->IsOk() )
                    {
                        row.SetOk( false );
                        row.SetErrorDescription( child->ErrorDescription() );
                    }

                    m_childTables.insert( { row.URI(), std::move( child ) } );
                }
            }
        };

    processOneTable( aRootTable );
}


wxString LIBRARY_MANAGER::tableFileName( LIBRARY_TABLE_TYPE aType )
{
    switch( aType )
    {
    case LIBRARY_TABLE_TYPE::SYMBOL:        return FILEEXT::SymbolLibraryTableFileName;
    case LIBRARY_TABLE_TYPE::FOOTPRINT:     return FILEEXT::FootprintLibraryTableFileName;
    case LIBRARY_TABLE_TYPE::DESIGN_BLOCK:  return FILEEXT::DesignBlockLibraryTableFileName;
    }

    wxCHECK( false, wxEmptyString );
}


void LIBRARY_MANAGER::createEmptyTable( LIBRARY_TABLE_TYPE aType, LIBRARY_TABLE_SCOPE aScope )
{


    if( aScope == LIBRARY_TABLE_SCOPE::GLOBAL )
    {
        wxCHECK( !m_tables.contains( aType ), /* void */ );
        wxFileName fn( PATHS::GetUserSettingsPath(), tableFileName( aType ) );

        m_tables[aType] = std::make_unique<LIBRARY_TABLE>( fn.GetFullPath(),
                                                           LIBRARY_TABLE_SCOPE::GLOBAL );
    }
    else if( aScope == LIBRARY_TABLE_SCOPE::PROJECT )
    {
        wxCHECK( !m_projectTables.contains( aType ), /* void */ );
        wxFileName fn( Pgm().GetSettingsManager().Prj().GetProjectDirectory(),
                       tableFileName( aType ) );

        m_projectTables[aType] = std::make_unique<LIBRARY_TABLE>( fn.GetFullPath(),
                                                                  LIBRARY_TABLE_SCOPE::PROJECT );
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

        m_symbolTable = m_manager.Table( LIBRARY_TABLE_TYPE::SYMBOL, LIBRARY_TABLE_SCOPE::GLOBAL )
                                .value_or( nullptr );
        m_fpTable = m_manager.Table( LIBRARY_TABLE_TYPE::FOOTPRINT, LIBRARY_TABLE_SCOPE::GLOBAL )
                            .value_or( nullptr );
        m_designBlockTable =
            m_manager.Table( LIBRARY_TABLE_TYPE::DESIGN_BLOCK, LIBRARY_TABLE_SCOPE::GLOBAL )
                    .value_or( nullptr );
    }

    /// Handles symbol library files, minimum nest level 2
    wxDirTraverseResult OnFile( const wxString& aFilePath ) override
    {
        wxFileName file = wxFileName::FileName( aFilePath );

        // consider a file to be a lib if it's name ends with .kicad_sym and
        // it is under $KICADn_3RD_PARTY/symbols/<pkgid>/ i.e. has nested level of at least +2
        if( file.GetExt() == wxT( "kicad_sym" ) && file.GetDirCount() >= m_prefix_dir_count + 2
            && file.GetDirs()[m_prefix_dir_count] == wxT( "symbols" ) )
        {
            addRowIfNecessary( m_symbolTable, file, ADD_MODE::FILE, 10 );
        }

        return wxDIR_CONTINUE;
    }

    /// Handles footprint library and design block library directories, minimum nest level 3
    wxDirTraverseResult OnDir( const wxString& dirPath ) override
    {
        static wxString designBlockExt = wxString::Format( wxS( ".%s" ),
                                                           FILEEXT::KiCadDesignBlockLibPathExtension );
        wxFileName dir = wxFileName::DirName( dirPath );

        // consider a directory to be a lib if it's name ends with .pretty and
        // it is under $KICADn_3RD_PARTY/footprints/<pkgid>/ i.e. has nested level of at least +3
        if( dirPath.EndsWith( wxS( ".pretty" ) ) && dir.GetDirCount() >= m_prefix_dir_count + 3
            && dir.GetDirs()[m_prefix_dir_count] == wxT( "footprints" ) )
        {
            addRowIfNecessary( m_fpTable, dir, ADD_MODE::DIRECTORY, 7 );
        }
        else if( dirPath.EndsWith( designBlockExt )
                 && dir.GetDirCount() >= m_prefix_dir_count + 3
                 && dir.GetDirs()[m_prefix_dir_count] == wxT( "design_blocks" ) )
        {
            addRowIfNecessary( m_designBlockTable, dir, ADD_MODE::DIRECTORY, designBlockExt.Len() );
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
        FILE,
        DIRECTORY
    };

    void addRowIfNecessary( LIBRARY_TABLE* aTable, const wxFileName& aSource, ADD_MODE aMode,
                            int aExtensionLength )
    {
        wxString versionedPath = wxString::Format( wxS( "${%s}" ),
        ENV_VAR::GetVersionedEnvVarName( wxS( "3RD_PARTY" ) ) );

        wxArrayString parts = aSource.GetDirs();
        parts.RemoveAt( 0, m_prefix_dir_count );
        parts.Insert( versionedPath, 0 );

        if( aMode == ADD_MODE::FILE )
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
    wxCHECK( m_typeToFilenameMap.contains( aType ), wxEmptyString );
    wxString basePath = PATHS::GetUserSettingsPath();

    wxFileName fn( basePath, m_typeToFilenameMap.at( aType ) );
    fn.Normalize( FN_NORMALIZE_FLAGS | wxPATH_NORM_ENV_VARS );

    return fn.GetFullPath();
}


bool LIBRARY_MANAGER::IsTableValid( const wxString& aPath )
{
    if( wxFileName fn( aPath ); fn.IsFileReadable() )
    {
        if( LIBRARY_TABLE temp( fn, LIBRARY_TABLE_SCOPE::GLOBAL ); temp.IsOk() )
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

    for( auto [type, name] : m_typeToFilenameMap )
    {
        if( wxFileName fn( basePath, name ); !IsTableValid( fn.GetFullPath() ) )
            invalidTables.emplace_back( type );
    }

    return invalidTables;
}


bool LIBRARY_MANAGER::CreateGlobalTable( LIBRARY_TABLE_TYPE aType, bool aPopulateDefaultLibraries )
{
    wxCHECK( m_typeToFilenameMap.contains( aType ), false );
    wxFileName fn( DefaultGlobalTablePath( aType ) );

    LIBRARY_TABLE table( fn, LIBRARY_TABLE_SCOPE::GLOBAL );
    table.SetType( aType );

    wxFileName defaultLib( PATHS::GetStockTemplatesPath(), m_typeToFilenameMap.at( aType ) );

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
        chained.SetURI( defaultLib.GetFullPath() );
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


void LIBRARY_MANAGER::LoadGlobalTables()
{
    // Cancel any in-progress load
    for( const std::unique_ptr<LIBRARY_MANAGER_ADAPTER>& adapter : m_adapters | std::views::values )
        adapter->ProjectChanged();

    loadTables( PATHS::GetUserSettingsPath(), LIBRARY_TABLE_SCOPE::GLOBAL );

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
                Pgm().GetLibraryManager().Save( table ).map_error(
                    []( const LIBRARY_ERROR& aError )
                    {
                        wxLogTrace( traceLibraries, "Warning: save failed after PCM auto-add: %s",
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
                    wxString path = GetFullURI( &aRow, true );
                    return path.StartsWith( *packagesPath ) && !wxFile::Exists( path );
                } );

            table->Rows().erase( toErase.begin(), toErase.end() );

            if( !toErase.empty() )
            {
                Pgm().GetLibraryManager().Save( table ).map_error(
                        []( const LIBRARY_ERROR& aError )
                        {
                            wxLogTrace( traceLibraries,
                                        "Warning: save failed after PCM auto-remove: %s",
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


void LIBRARY_MANAGER::ProjectChanged()
{
    LoadProjectTables( Pgm().GetSettingsManager().Prj().GetProjectDirectory() );

    for( const std::unique_ptr<LIBRARY_MANAGER_ADAPTER>& adapter : m_adapters | std::views::values )
        adapter->ProjectChanged();
}


void LIBRARY_MANAGER::RegisterAdapter( LIBRARY_TABLE_TYPE aType,
                                       std::unique_ptr<LIBRARY_MANAGER_ADAPTER>&& aAdapter )
{
    wxCHECK_MSG( !m_adapters.contains( aType ), /**/, "You should only register an adapter once!" );

    m_adapters[aType] = std::move( aAdapter );
}


std::optional<LIBRARY_MANAGER_ADAPTER*> LIBRARY_MANAGER::Adapter( LIBRARY_TABLE_TYPE aType ) const
{
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


std::vector<LIBRARY_TABLE_ROW*> LIBRARY_MANAGER::Rows( LIBRARY_TABLE_TYPE aType,
                                                       LIBRARY_TABLE_SCOPE aScope,
                                                       bool aIncludeInvalid ) const
{
    std::map<wxString, LIBRARY_TABLE_ROW*> rows;
    std::vector<wxString> rowOrder;

    std::list<std::ranges::ref_view<
            const std::map<LIBRARY_TABLE_TYPE, std::unique_ptr<LIBRARY_TABLE>>
        >> tables;

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

    std::function<void(const std::unique_ptr<LIBRARY_TABLE>&)> processTable =
            [&]( const std::unique_ptr<LIBRARY_TABLE>& aTable )
            {
                if( aTable->Type() != aType )
                    return;

                if( aTable->IsOk() || aIncludeInvalid )
                {
                    for( LIBRARY_TABLE_ROW& row : aTable->Rows() )
                    {
                        if( row.IsOk() || aIncludeInvalid )
                        {
                            if( row.Type() == LIBRARY_TABLE_ROW::TABLE_TYPE_NAME )
                            {
                                if( !m_childTables.contains( row.URI() ) )
                                    continue;

                                processTable( m_childTables.at( row.URI() ) );
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
        processTable( table );
    }

    std::vector<LIBRARY_TABLE_ROW*> ret;

    for( const wxString& row : rowOrder )
        ret.emplace_back( rows[row] );

    return ret;
}


std::optional<LIBRARY_TABLE_ROW*> LIBRARY_MANAGER::GetRow( LIBRARY_TABLE_TYPE  aType,
                                                           const wxString&     aNickname,
                                                           LIBRARY_TABLE_SCOPE aScope ) const
{
    for( LIBRARY_TABLE_ROW* row : Rows( aType, aScope, true ) )
    {
        if( row->Nickname() == aNickname )
            return row;
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


void LIBRARY_MANAGER::LoadProjectTables( const wxString& aProjectPath )
{
    if( wxFileName::IsDirReadable( aProjectPath ) )
    {
        loadTables( aProjectPath, LIBRARY_TABLE_SCOPE::PROJECT );
    }
    else
    {
        m_projectTables.clear();
        wxLogTrace( traceLibraries,
                    "New project path %s is not readable, not loading project tables",
                    aProjectPath );
    }
}


LIBRARY_RESULT<void> LIBRARY_MANAGER::Save( LIBRARY_TABLE* aTable ) const
{
    wxCHECK( aTable, tl::unexpected( LIBRARY_ERROR( "Internal error" ) ) );

    // TODO(JE) clean this up; shouldn't need to iterate
    for( const std::unique_ptr<LIBRARY_TABLE>& t : m_tables | std::views::values )
    {
        if( t.get() == aTable )
        {
            wxLogTrace( traceLibraries, "Saving %s", aTable->Path() );
            wxFileName fn( aTable->Path() );
            // This should already be normalized, but just in case...
            fn.Normalize( FN_NORMALIZE_FLAGS | wxPATH_NORM_ENV_VARS );

            try
            {
                PRETTIFIED_FILE_OUTPUTFORMATTER formatter( fn.GetFullPath(), KICAD_FORMAT::FORMAT_MODE::LIBRARY_TABLE );
                aTable->Format( &formatter );
            }
            catch( IO_ERROR& e )
            {
                wxLogTrace( traceLibraries, "Exception while saving: %s", e.What() );
                return tl::unexpected( LIBRARY_ERROR( e.What() ) );
            }

            return LIBRARY_RESULT<void>();
        }
    }

    // Unmanaged table?  TODO(JE) should this happen?
    wxLogTrace( traceLibraries, "Can't save %s; unmanaged library", aTable->Path() );
    return tl::unexpected( LIBRARY_ERROR( "Internal error" ) );
}


std::optional<wxString> LIBRARY_MANAGER::GetFullURI( LIBRARY_TABLE_TYPE aType,
                                                     const wxString& aNickname,
                                                     bool aSubstituted ) const
{
    if( std::optional<const LIBRARY_TABLE_ROW*> result = GetRow( aType, aNickname ) )
        return GetFullURI( *result, aSubstituted );

    return std::nullopt;
}


wxString LIBRARY_MANAGER::GetFullURI( const LIBRARY_TABLE_ROW* aRow,
                                      bool aSubstituted )
{
    if( aSubstituted )
        return ExpandEnvVarSubstitutions( aRow->URI(), nullptr );

    return aRow->URI();
}


wxString LIBRARY_MANAGER::ExpandURI( const wxString& aShortURI, const PROJECT& aProject )
{
    wxFileName path( ExpandEnvVarSubstitutions( aShortURI, &aProject ) );
    path.MakeAbsolute();
    return path.GetFullPath();
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



LIBRARY_MANAGER_ADAPTER::~LIBRARY_MANAGER_ADAPTER()
{
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


std::vector<LIBRARY_TABLE_ROW *> LIBRARY_MANAGER_ADAPTER::Rows(
        LIBRARY_TABLE_SCOPE aScope, bool aIncludeInvalid ) const
{
    return m_manager.Rows( Type(), aScope, aIncludeInvalid );
}


std::optional<LIBRARY_TABLE_ROW *> LIBRARY_MANAGER_ADAPTER::GetRow(
    const wxString &aNickname, LIBRARY_TABLE_SCOPE aScope ) const
{
    return m_manager.GetRow( Type(), aNickname, aScope );
}


std::optional<LIBRARY_TABLE_ROW*> LIBRARY_MANAGER_ADAPTER::FindRowByURI(
        const wxString& aUri,
        LIBRARY_TABLE_SCOPE aScope ) const
{
    return m_manager.FindRowByURI( Type(), aUri, aScope );
}
