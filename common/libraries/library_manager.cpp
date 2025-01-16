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

#include <list>

#include <paths.h>
#include <pgm_base.h>
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
        [&]() -> std::vector<std::unique_ptr<LIBRARY_TABLE>>&
        {
            switch( aScope )
            {
            case LIBRARY_TABLE_SCOPE::GLOBAL:
                return m_tables;

            case LIBRARY_TABLE_SCOPE::PROJECT:
                return m_project_tables;

            default:
                wxCHECK_MSG( false, m_tables, "Invalid scope passed to loadTables" );
            }
        };

    std::vector<std::unique_ptr<LIBRARY_TABLE>>& aTarget = getTarget();

    aTarget.clear();

    for( const std::string& name : { FILEEXT::SymbolLibraryTableFileName,
                                     FILEEXT::FootprintLibraryTableFileName,
                                     FILEEXT::DesignBlockLibraryTableFileName } )
    {
        wxFileName fn( aTablePath, name );

        if( fn.IsFileReadable() )
            aTarget.emplace_back( std::make_unique<LIBRARY_TABLE>( fn, aScope ) );
        else
            wxLogTrace( traceLibraries, "No library table found at %s", fn.GetFullPath() );
    }

    for( const std::unique_ptr<LIBRARY_TABLE>& t : aTarget )
        t->LoadNestedTables();
}


void LIBRARY_MANAGER::LoadGlobalTables()
{
    loadTables( PATHS::GetUserSettingsPath(), LIBRARY_TABLE_SCOPE::GLOBAL );
}


std::vector<const LIBRARY_TABLE_ROW*> LIBRARY_MANAGER::Rows( LIBRARY_TABLE_TYPE aType,
                                                             LIBRARY_TABLE_SCOPE aScope,
                                                             bool aIncludeInvalid ) const
{
    std::map<wxString, const LIBRARY_TABLE_ROW*> rows;
    std::vector<wxString> rowOrder;

    std::list<std::ranges::ref_view<const std::vector<std::unique_ptr<LIBRARY_TABLE>>>> tables;

    switch( aScope )
    {
    case LIBRARY_TABLE_SCOPE::GLOBAL:
        tables = { std::views::all( m_tables ) };
        break;

    case LIBRARY_TABLE_SCOPE::PROJECT:
        tables = { std::views::all( m_project_tables ) };
        break;

    case LIBRARY_TABLE_SCOPE::BOTH:
        tables = { std::views::all( m_tables ), std::views::all( m_project_tables ) };
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
                                wxCHECK2( aTable->Children().contains( row.Nickname() ), continue );
                                processTable( aTable->Children().at( row.Nickname() ) );
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

    for( const std::unique_ptr<LIBRARY_TABLE>& table : std::views::join( tables ) )
        processTable( table );

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
        m_project_tables.clear();
        wxLogTrace( traceLibraries,
                    "New project path %s is not readable, not loading project tables",
                    aProjectPath );
    }
}
