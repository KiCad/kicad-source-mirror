/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Alex Shvartzkop <dudesuchamazing@gmail.com>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "pcb_easyedapro_plugin.h"
#include "pcb_easyedapro_parser.h"
#include <plugins/easyedapro/easyedapro_import_utils.h>
#include <plugins/easyedapro/easyedapro_parser.h>

#include <board.h>
#include <footprint.h>
#include <progress_reporter.h>
#include <common.h>
#include <macros.h>

#include <fstream>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>
#include <wx/mstream.h>
#include <wx/zipstrm.h>
#include <wx/log.h>

#include <nlohmann/json.hpp>
#include <core/json_serializers.h>
#include <core/map_helpers.h>


struct EASYEDAPRO_PLUGIN::PRJ_DATA
{
    std::map<wxString, std::unique_ptr<FOOTPRINT>>                  m_Footprints;
    std::map<wxString, EASYEDAPRO::BLOB>                            m_Blobs;
    std::map<wxString, std::multimap<wxString, EASYEDAPRO::POURED>> m_Poured;
};


EASYEDAPRO_PLUGIN::EASYEDAPRO_PLUGIN()
{
    m_board = nullptr;
    m_props = nullptr;
}


EASYEDAPRO_PLUGIN::~EASYEDAPRO_PLUGIN()
{
    if( m_projectData )
        delete m_projectData;
}


bool EASYEDAPRO_PLUGIN::CanReadBoard( const wxString& aFileName ) const
{
    if( aFileName.Lower().EndsWith( wxS( ".epro" ) ) )
    {
        return true;
    }
    else if( aFileName.Lower().EndsWith( wxS( ".zip" ) ) )
    {
        std::shared_ptr<wxZipEntry> entry;
        wxFFileInputStream          in( aFileName );
        wxZipInputStream            zip( in );

        if( !zip.IsOk() )
            return false;

        while( entry.reset( zip.GetNextEntry() ), entry.get() != NULL )
        {
            wxString name = entry->GetName();

            if( name == wxS( "project.json" ) )
                return true;
        }

        return false;
    }

    return false;
}


BOARD* EASYEDAPRO_PLUGIN::LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                                     const STRING_UTF8_MAP* aProperties, PROJECT* aProject,
                                     PROGRESS_REPORTER* aProgressReporter )
{
    m_props = aProperties;

    m_board = aAppendToMe ? aAppendToMe : new BOARD();

    // Give the filename to the board if it's new
    if( !aAppendToMe )
        m_board->SetFileName( aFileName );

    if( aProgressReporter )
    {
        aProgressReporter->Report( wxString::Format( _( "Loading %s..." ), aFileName ) );

        if( !aProgressReporter->KeepRefreshing() )
            THROW_IO_ERROR( _( "Open cancelled by user." ) );
    }

    PCB_EASYEDAPRO_PARSER parser( nullptr, nullptr );

    wxFileName fname( aFileName );

    if( fname.GetExt() == wxS( "epro" ) || fname.GetExt() == wxS( "zip" ) )
    {
        nlohmann::json project = EASYEDAPRO::ReadProjectFile( aFileName );

        std::map<wxString, EASYEDAPRO::PRJ_SCHEMATIC> prjSchematics = project.at( "schematics" );
        std::map<wxString, EASYEDAPRO::PRJ_BOARD>     prjBoards = project.at( "boards" );
        std::map<wxString, wxString>                  prjPcbNames = project.at( "pcbs" );

        wxString pcbToLoad;

        if( prjBoards.size() > 0 )
        {
            EASYEDAPRO::PRJ_BOARD boardToLoad = prjBoards.begin()->second;
            pcbToLoad = boardToLoad.pcb;
        }
        else if( prjPcbNames.size() > 0 )
        {
            pcbToLoad = prjPcbNames.begin()->first;
        }

        if( prjPcbNames.empty() )
            return nullptr;

        LoadAllDataFromProject( aFileName, project );

        if( !m_projectData )
            return nullptr;

        auto cb = [&]( const wxString& name, const wxString& pcbUuid, wxInputStream& zip ) -> bool
        {
            if( !name.EndsWith( wxS( ".epcb" ) ) )
                EASY_IT_CONTINUE;

            if( pcbUuid != pcbToLoad )
                EASY_IT_CONTINUE;

            std::vector<nlohmann::json> lines = EASYEDAPRO::ParseJsonLines( zip, name );

            wxString           boardKey = pcbUuid + wxS( "_0" );
            wxScopedCharBuffer cb = boardKey.ToUTF8();
            wxString           boardPouredKey = wxBase64Encode( cb.data(), cb.length() );

            const std::multimap<wxString, EASYEDAPRO::POURED>& boardPoured =
                    get_def( m_projectData->m_Poured, boardPouredKey );

            parser.ParseBoard( m_board, project, m_projectData->m_Footprints,
                               m_projectData->m_Blobs, boardPoured, lines,
                               EASYEDAPRO::ShortenLibName( fname.GetName() ) );

            EASY_IT_BREAK;
        };
        EASYEDAPRO::IterateZipFiles( aFileName, cb );
    }

    return m_board;
}


long long EASYEDAPRO_PLUGIN::GetLibraryTimestamp( const wxString& aLibraryPath ) const
{
    return 0;
}


void EASYEDAPRO_PLUGIN::FootprintEnumerate( wxArrayString&  aFootprintNames,
                                            const wxString& aLibraryPath, bool aBestEfforts,
                                            const STRING_UTF8_MAP* aProperties )
{
    wxFileName fname( aLibraryPath );

    if( fname.GetExt() == wxS( "efoo" ) )
    {
        wxFFileInputStream ffis( aLibraryPath );
        wxTextInputStream  txt( ffis, wxS( " " ), wxConvUTF8 );

        while( ffis.CanRead() )
        {
            wxString line = txt.ReadLine();

            if( !line.Contains( wxS( "ATTR" ) ) )
                continue; // Don't bother parsing

            nlohmann::json js = nlohmann::json::parse( line );
            if( js.at( 0 ) == "ATTR" && js.at( 7 ) == "Footprint" )
            {
                aFootprintNames.Add( js.at( 8 ).get<wxString>() );
            }
        }
    }
    else if( fname.GetExt() == wxS( "epro" ) || fname.GetExt() == wxS( "zip" ) )
    {
        nlohmann::json                     project = EASYEDAPRO::ReadProjectFile( aLibraryPath );
        std::map<wxString, nlohmann::json> footprintMap = project.at( "footprints" );

        for( auto& [key, value] : footprintMap )
            aFootprintNames.Add( value.at( "title" ) );
    }
}


void EASYEDAPRO_PLUGIN::LoadAllDataFromProject( const wxString&       aProjectPath,
                                                const nlohmann::json& aProject )
{
    if( m_projectData )
        delete m_projectData;

    m_projectData = new PRJ_DATA();

    PCB_EASYEDAPRO_PARSER parser( nullptr, nullptr );
    wxFileName            fname( aProjectPath );
    wxString              fpLibName = EASYEDAPRO::ShortenLibName( fname.GetName() );

    std::map<wxString, std::unique_ptr<FOOTPRINT>> result;

    auto cb = [&]( const wxString& name, const wxString& baseName, wxInputStream& zip ) -> bool
    {
        if( !name.EndsWith( wxS( ".efoo" ) ) && !name.EndsWith( wxS( ".eblob" ) )
            && !name.EndsWith( wxS( ".ecop" ) ) )
        {
            EASY_IT_CONTINUE;
        }

        std::vector<nlohmann::json> lines = EASYEDAPRO::ParseJsonLines( zip, name );

        if( name.EndsWith( wxS( ".efoo" ) ) )
        {
            nlohmann::json fpData = aProject.at( "footprints" ).at( baseName );
            wxString       fpTitle = fpData.at( "title" );

            FOOTPRINT* footprint = parser.ParseFootprint( aProject, baseName, lines );

            if( !footprint )
                EASY_IT_CONTINUE;

            LIB_ID fpID = EASYEDAPRO::ToKiCadLibID( fpLibName, fpTitle );
            footprint->SetFPID( fpID );

            m_projectData->m_Footprints.emplace( baseName, footprint );
        }
        else if( name.EndsWith( wxS( ".eblob" ) ) )
        {
            for( const nlohmann::json& line : lines )
            {
                if( line.at( 0 ) == "BLOB" )
                {
                    EASYEDAPRO::BLOB blob = line;
                    m_projectData->m_Blobs[blob.objectId] = blob;
                }
            }
        }
        else if( name.EndsWith( wxS( ".ecop" ) ) && EASYEDAPRO::IMPORT_POURED )
        {
            for( const nlohmann::json& line : lines )
            {
                if( line.at( 0 ) == "POURED" )
                {
                    if( !line.at( 2 ).is_string() )
                        continue; // Unknown type of POURED

                    EASYEDAPRO::POURED poured = line;
                    m_projectData->m_Poured[baseName].emplace( poured.parentId, poured );
                }
            }
        }
        EASY_IT_CONTINUE;
    };
    EASYEDAPRO::IterateZipFiles( aProjectPath, cb );
}


FOOTPRINT* EASYEDAPRO_PLUGIN::FootprintLoad( const wxString& aLibraryPath,
                                             const wxString& aFootprintName, bool aKeepUUID,
                                             const STRING_UTF8_MAP* aProperties )
{
    PCB_EASYEDAPRO_PARSER parser( nullptr, nullptr );
    FOOTPRINT*            footprint = nullptr;

    wxFileName libFname( aLibraryPath );

    if( libFname.GetExt() == wxS( "efoo" ) )
    {
        wxFFileInputStream ffis( aLibraryPath );
        wxTextInputStream  txt( ffis, wxS( " " ), wxConvUTF8 );

        std::vector<nlohmann::json> lines = EASYEDAPRO::ParseJsonLines( ffis, aLibraryPath );

        for( const nlohmann::json& js : lines )
        {
            if( js.at( 0 ) == "ATTR" )
            {
                EASYEDAPRO::PCB_ATTR attr = js;

                if( attr.key == wxS( "Footprint" ) && attr.value != aFootprintName )
                    return nullptr;
            }
        }

        footprint = parser.ParseFootprint( nlohmann::json(), wxEmptyString, lines );

        if( !footprint )
        {
            THROW_IO_ERROR( wxString::Format( _( "Cannot load footprint '%s' from '%s'" ),
                                              aFootprintName, aLibraryPath ) );
        }

        LIB_ID fpID = EASYEDAPRO::ToKiCadLibID( wxEmptyString, aFootprintName );
        footprint->SetFPID( fpID );

        footprint->Reference().SetVisible( true );
        footprint->Value().SetText( aFootprintName );
        footprint->Value().SetVisible( true );
        footprint->AutoPositionFields();
    }
    else if( libFname.GetExt() == wxS( "epro" ) || libFname.GetExt() == wxS( "zip" ) )
    {
        nlohmann::json project = EASYEDAPRO::ReadProjectFile( aLibraryPath );

        wxString fpUuid;

        std::map<wxString, nlohmann::json> footprintMap = project.at( "footprints" );
        for( auto& [uuid, data] : footprintMap )
        {
            wxString title = data.at( "title" );

            if( title == aFootprintName )
            {
                fpUuid = uuid;
                break;
            }
        }

        if( !fpUuid )
        {
            THROW_IO_ERROR( wxString::Format( _( "Footprint '%s' not found in project '%s'" ),
                                              aFootprintName, aLibraryPath ) );
        }

        auto cb = [&]( const wxString& name, const wxString& baseName, wxInputStream& zip ) -> bool
        {
            if( !name.EndsWith( wxS( ".efoo" ) ) )
                EASY_IT_CONTINUE;

            if( baseName != fpUuid )
                EASY_IT_CONTINUE;

            std::vector<nlohmann::json> lines = EASYEDAPRO::ParseJsonLines( zip, name );

            footprint = parser.ParseFootprint( project, fpUuid, lines );

            if( !footprint )
            {
                THROW_IO_ERROR( wxString::Format( _( "Cannot load footprint '%s' from '%s'" ),
                                                  aFootprintName, aLibraryPath ) );
            }

            LIB_ID fpID = EASYEDAPRO::ToKiCadLibID( wxEmptyString, aFootprintName );
            footprint->SetFPID( fpID );

            footprint->Reference().SetVisible( true );
            footprint->Value().SetText( aFootprintName );
            footprint->Value().SetVisible( true );
            footprint->AutoPositionFields();

            EASY_IT_BREAK;
        };
        EASYEDAPRO::IterateZipFiles( aLibraryPath, cb );
    }

    return footprint;
}


std::vector<FOOTPRINT*> EASYEDAPRO_PLUGIN::GetImportedCachedLibraryFootprints()
{
    std::vector<FOOTPRINT*> result;

    if( !m_projectData )
        return result;

    for( auto& [fpUuid, footprint] : m_projectData->m_Footprints )
    {
        result.push_back( static_cast<FOOTPRINT*>( footprint->Clone() ) );
    }

    return result;
}
