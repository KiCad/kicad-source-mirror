/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "mdw_design_parts.h"

#include <gestfich.h>
#include <kiface_services.h>
#include <kiway.h>
#include <lib_id.h>
#include <libraries/library_manager.h>
#include <mdw/client/mdw_library_sources.h>
#include <pgm_base.h>
#include <project.h>
#include <project/project_file.h>
#include <string_utils.h>
#include <widgets/wx_progress_reporters.h>
#include <wildcards_and_files_ext.h>

#include <json_common.h>

#include <wx/filename.h>
#include <wx/intl.h>

#include <algorithm>
#include <array>
#include <tuple>


namespace
{

/// Add the parts placed in the saved document aFile, read by aFace's kiface; false when it does not load
bool collect( KIWAY& aKiway, KIWAY::FACE_T aFace, const wxString& aFile, LIBRARY_TABLE_TYPE aType, PROJECT& aProject,
              PROGRESS_REPORTER& aReporter, std::vector<MDW_PLACED_INPUT>& aInputs )
{
    const MDW_LIBRARY_SOURCES&                   sources = MDW_LIBRARY_SOURCES::Get();
    const std::vector<MDW_LIBRARY_SOURCE_CONFIG> configured = sources.Sources();
    const PROJECT_FILE&                          projectFile = aProject.GetProjectFile();

    KIFACE_COLLECT_PLACED_PARTS_SERVICE::Type collector =
            GetKifaceService<KIFACE_COLLECT_PLACED_PARTS_SERVICE>( aKiway.KiFACE( aFace ) );
    std::optional<std::string> payload = collector ? collector( aFile, aProject, &aReporter ) : std::nullopt;

    if( !payload )
        return false;

    nlohmann::json parts = nlohmann::json::parse( *payload, nullptr, false );

    if( !parts.is_array() )
        return false;

    for( const nlohmann::json& entry : parts )
    {
        LIB_ID                      libId;
        wxString                    libIdText = wxString::FromUTF8( entry.value( "lib_id", "" ) );
        std::optional<MDW_ITEM_REF> member;
        wxString                    pin;
        MDW_TEAM_SOURCE             teamSource;

        if( libId.Parse( libIdText ) < 0 )
        {
            std::optional<LIBRARY_TABLE_ROW*> row = Pgm().GetLibraryManager().GetRow( aType, libId.GetLibNickname() );

            // A symbol and a footprint library may share a nickname, so the type picks the row
            const MDW_LIBRARY_TYPE memberType =
                    aType == LIBRARY_TABLE_TYPE::SYMBOL ? MDW_LIBRARY_TYPE::SYMBOL : MDW_LIBRARY_TYPE::FOOTPRINT;

            // A project row shadows the global one, which is how a pinned copy wins
            if( row )
            {
                const wxString path = LIBRARY_MANAGER::GetFullURI( *row, true );

                member = sources.MemberForAt( memberType, ( *row )->Options(), path, libId.GetUniStringLibItemName() );

                if( std::optional<MDW_LIBRARY_ROW> library = sources.ForRowAt( memberType, ( *row )->Options(), path );
                    library && library->pinned )
                {
                    pin = library->snapshot.label;
                }
            }

            // A shared project table can name a team source that is not set up here
            if( !member )
            {
                std::optional<wxString> marker;

                if( row && *row )
                    marker = MDW_LIBRARY_SOURCES::MarkerOf( ( *row )->Options() );

                teamSource = MDW_LIBRARY_SOURCES::TeamSourceOf( libId.GetUniStringLibNickname(), marker, configured,
                                                                projectFile.m_WorkspaceLibrarySources );
            }
        }

        // A power symbol is a net label, not a part, so it needs no release unless a library source owns it
        if( !member && teamSource.name.IsEmpty() && entry.value( "power", false ) )
            continue;

        MDW_PLACED_INPUT& input = aInputs.emplace_back();
        input.kind = aType == LIBRARY_TABLE_TYPE::SYMBOL ? MDW_PLACED_KIND::SYMBOL : MDW_PLACED_KIND::FOOTPRINT;
        input.reference = wxString::FromUTF8( entry.value( "reference", "" ) );
        input.lib_id = libIdText;
        input.content_digest = wxString::FromUTF8( entry.value( "digest", "" ) );
        input.member = member;
        input.pin_label = pin;
        input.team_source = teamSource.name;
        input.team_locator = teamSource.locator;

        if( const auto part = entry.find( "part" ); part != entry.end() && part->is_object() )
        {
            input.part.source = wxString::FromUTF8( part->value( "source", "" ) );
            input.part.item_id = wxString::FromUTF8( part->value( "part_id", "" ) );
            input.part_revision_id = wxString::FromUTF8( part->value( "revision_id", "" ) );
        }
    }

    return true;
}


/// The project's schematic and board, whether or not they exist
std::array<wxFileName, 2> designFiles( PROJECT& aProject )
{
    const wxString path = aProject.GetProjectPath();

    return { wxFileName( path, aProject.GetProjectName(), FILEEXT::KiCadSchematicFileExtension ),
             wxFileName( path, aProject.GetProjectName(), FILEEXT::KiCadPcbFileExtension ) };
}

} // namespace


std::vector<std::filesystem::file_time_type> MdwDesignWriteTimes( PROJECT& aProject )
{
    std::vector<std::filesystem::file_time_type> times;

    for( const wxFileName& file : designFiles( aProject ) )
    {
        std::error_code ec;
        times.push_back( std::filesystem::last_write_time( ToFsPath( file.GetFullPath() ), ec ) );

        if( ec )
            times.back() = std::filesystem::file_time_type::min();
    }

    return times;
}


wxString MdwCollectPlacedParts( wxWindow* aParent, KIWAY& aKiway, PROJECT& aProject,
                                std::vector<MDW_PLACED_INPUT>& aInputs )
{
    const auto [schematicFile, boardFile] = designFiles( aProject );

    if( !schematicFile.FileExists() && !boardFile.FileExists() )
        return _( "The design has no schematic or board to release." );

    WX_PROGRESS_REPORTER reporter( aParent, _( "Read Design" ), 2, PR_NO_ABORT );
    int                  phase = 0;

    // Every part the design places must be checked, so a document that does not load stops the release
    for( const auto& [face, file, type] : { std::tuple( KIWAY::FACE_SCH, schematicFile, LIBRARY_TABLE_TYPE::SYMBOL ),
                                            std::tuple( KIWAY::FACE_PCB, boardFile, LIBRARY_TABLE_TYPE::FOOTPRINT ) } )
    {
        reporter.BeginPhase( phase++ );

        if( file.FileExists() && !collect( aKiway, face, file.GetFullPath(), type, aProject, reporter, aInputs ) )
            return wxString::Format( _( "Could not read %s." ), file.GetFullName() );
    }

    return wxEmptyString;
}


std::vector<wxString> MdwWhereUsed( const std::vector<MDW_PLACED_INPUT>& aInputs, const MDW_ITEM_REF& aMember )
{
    std::vector<wxString> references;

    if( aMember.item_id.IsEmpty() )
        return references;

    for( const MDW_PLACED_INPUT& input : aInputs )
    {
        // The rows and the member index may spell one source's locator differently
        if( input.member && input.member->item_id == aMember.item_id
            && MDW_LIBRARY_SOURCES::SameLocator( input.member->source, aMember.source ) )
        {
            references.push_back( input.reference );
        }
    }

    // Ties broken by spelling, so only identical references end up adjacent for unique
    std::ranges::sort( references,
                       []( const wxString& aA, const wxString& aB )
                       {
                           int cmp = StrNumCmp( aA, aB, CMP_T::IGNORE_CASE );
                           return cmp != 0 ? cmp < 0 : aA < aB;
                       } );

    // A symbol's units and its footprint each place the same reference
    auto [first, last] = std::ranges::unique( references );
    references.erase( first, last );

    return references;
}
