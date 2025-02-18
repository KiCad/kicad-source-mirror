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
#include <list>
#include <magic_enum.hpp>
#include <unordered_set>

#include <paths.h>
#include <pgm_base.h>
#include <richio.h>
#include <trace_helpers.h>
#include <wildcards_and_files_ext.h>

#include <libraries/library_manager.h>
#include <settings/settings_manager.h>
#include <wx/log.h>

struct LIBRARY_MANAGER_INTERNALS
{
    std::vector<LIBRARY_TABLE> tables;
};


LIBRARY_MANAGER::LIBRARY_MANAGER()
{
}


LIBRARY_MANAGER::~LIBRARY_MANAGER() = default;


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
                if( row.Type() == wxT( "Table" ) )
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


void LIBRARY_MANAGER::LoadGlobalTables()
{
    loadTables( PATHS::GetUserSettingsPath(), LIBRARY_TABLE_SCOPE::GLOBAL );
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
                                                      LIBRARY_TABLE_SCOPE aScope ) const
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
            return std::nullopt;

        return m_projectTables.at( aType ).get();
    }
    }

    return std::nullopt;
}


std::vector<const LIBRARY_TABLE_ROW*> LIBRARY_MANAGER::Rows( LIBRARY_TABLE_TYPE aType,
                                                             LIBRARY_TABLE_SCOPE aScope,
                                                             bool aIncludeInvalid ) const
{
    std::map<wxString, const LIBRARY_TABLE_ROW*> rows;
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
                    for( const LIBRARY_TABLE_ROW& row : aTable->Rows() )
                    {
                        if( row.IsOk() || aIncludeInvalid )
                        {
                            if( row.Type() == "Table" )
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

    std::vector<const LIBRARY_TABLE_ROW*> ret;

    for( const wxString& row : rowOrder )
        ret.emplace_back( rows[row] );

    return ret;
}


LIBRARY_RESULT<const LIBRARY_TABLE_ROW*> LIBRARY_MANAGER::GetRow( LIBRARY_TABLE_TYPE aType,
                                                                  const wxString& aNickname,
                                                                  LIBRARY_TABLE_SCOPE aScope ) const
{
    for( const LIBRARY_TABLE_ROW* row : Rows( aType, aScope, true ) )
    {
        if( row->Nickname() == aNickname )
            return row;
    }

    return tl::unexpected( LIBRARY_ERROR { .message = _( "Library not found" ) } );
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
    if( LIBRARY_RESULT<const LIBRARY_TABLE_ROW*> result = GetRow( aType, aNickname ) )
    {
        const LIBRARY_TABLE_ROW* row = *result;

        if( aSubstituted )
            return ExpandEnvVarSubstitutions( row->URI(), nullptr );

        return row->URI();
    }

    return std::nullopt;
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
