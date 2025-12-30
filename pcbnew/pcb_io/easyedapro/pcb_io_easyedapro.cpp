/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Alex Shvartzkop <dudesuchamazing@gmail.com>
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

#include <io/easyedapro/easyedapro_import_utils.h>
#include <io/easyedapro/easyedapro_parser.h>
#include <pcb_io/easyedapro/pcb_io_easyedapro.h>
#include <pcb_io/easyedapro/pcb_io_easyedapro_parser.h>
#include <pcb_io/pcb_io.h>

#include <board.h>
#include <font/fontconfig.h>
#include <footprint.h>
#include <progress_reporter.h>
#include <common.h>
#include <macros.h>
#include <reporter.h>

#include <fstream>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>
#include <wx/mstream.h>
#include <wx/zipstrm.h>
#include <wx/log.h>

#include <json_common.h>
#include <core/json_serializers.h>
#include <core/map_helpers.h>


struct PCB_IO_EASYEDAPRO::PRJ_DATA
{
    std::map<wxString, std::unique_ptr<FOOTPRINT>>                  m_Footprints;
    std::map<wxString, EASYEDAPRO::BLOB>                            m_Blobs;
    std::map<wxString, std::multimap<wxString, EASYEDAPRO::POURED>> m_Poured;
};


PCB_IO_EASYEDAPRO::PCB_IO_EASYEDAPRO() : PCB_IO( wxS( "EasyEDA (JLCEDA) Professional" ) )
{
}


PCB_IO_EASYEDAPRO::~PCB_IO_EASYEDAPRO()
{
    if( m_projectData )
        delete m_projectData;
}


bool PCB_IO_EASYEDAPRO::CanReadBoard( const wxString& aFileName ) const
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


BOARD* PCB_IO_EASYEDAPRO::LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                                     const std::map<std::string, UTF8>* aProperties, PROJECT* aProject )
{
    m_props = aProperties;

    m_board = aAppendToMe ? aAppendToMe : new BOARD();

    // Give the filename to the board if it's new
    if( !aAppendToMe )
        m_board->SetFileName( aFileName );

    // Collect the font substitution warnings (RAII - automatically reset on scope exit)
    FONTCONFIG_REPORTER_SCOPE fontconfigScope( &LOAD_INFO_REPORTER::GetInstance() );

    if( m_progressReporter )
    {
        m_progressReporter->Report( wxString::Format( _( "Loading %s..." ), aFileName ) );

        if( !m_progressReporter->KeepRefreshing() )
            THROW_IO_ERROR( _( "File import canceled by user." ) );
    }

    PCB_IO_EASYEDAPRO_PARSER parser( nullptr, nullptr );

    wxFileName fname( aFileName );
    wxString   fpLibName = EASYEDAPRO::ShortenLibName( fname.GetName() );

    if( fname.GetExt() == wxS( "epro" ) || fname.GetExt() == wxS( "zip" ) )
    {
        nlohmann::json project = EASYEDAPRO::ReadProjectOrDeviceFile( aFileName );

        wxString pcbToLoad;

        if( m_props && m_props->contains( "pcb_id" ) )
        {
            pcbToLoad = wxString::FromUTF8( m_props->at( "pcb_id" ) );
        }
        else
        {
            std::map<wxString, wxString> prjPcbNames = project.at( "pcbs" );

            if( prjPcbNames.size() == 1 )
            {
                pcbToLoad = prjPcbNames.begin()->first;
            }
            else
            {
                std::vector<IMPORT_PROJECT_DESC> chosen = m_choose_project_handler(
                        EASYEDAPRO::ProjectToSelectorDialog( project, true, false ) );

                if( chosen.size() > 0 )
                    pcbToLoad = chosen[0].PCBId;
            }
        }

        if( pcbToLoad.empty() )
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

            std::vector<nlohmann::json>* pcbLines = nullptr;

            std::vector<std::vector<nlohmann::json>> lineBlocks =
                    EASYEDAPRO::ParseJsonLinesWithSeparation( zip, name );

            if( lineBlocks.empty() )
                EASY_IT_CONTINUE;

            if( lineBlocks.size() > 1 )
            {
                for( std::vector<nlohmann::json>& block : lineBlocks )
                {
                    wxString       docType;
                    nlohmann::json headData;

                    for( const nlohmann::json& line : block )
                    {
                        if( line.size() < 2 )
                            continue;

                        if( !line.at( 0 ).is_string() )
                            continue;

                        wxString lineType = line.at( 0 ).get<wxString>();

                        if( lineType == wxS( "DOCTYPE" ) )
                        {
                            if( !line.at( 1 ).is_string() )
                                continue;

                            docType = line.at( 1 ).get<wxString>();
                        }
                        else if( lineType == wxS( "HEAD" ) )
                        {
                            if( !line.at( 1 ).is_object() )
                                continue;

                            headData = line.at( 1 );
                        }
                    }

                    if( docType == wxS( "FOOTPRINT" ) )
                    {
                        wxString fpUuid = headData.at( "uuid" );
                        wxString fpTitle = headData.at( "title" );

                        FOOTPRINT* footprint = parser.ParseFootprint( project, fpUuid, block );

                        if( !footprint )
                            EASY_IT_CONTINUE;

                        LIB_ID fpID = EASYEDAPRO::ToKiCadLibID( fpLibName, fpTitle );
                        footprint->SetFPID( fpID );

                        m_projectData->m_Footprints.emplace( fpUuid, footprint );
                    }
                    else if( docType == wxS( "PCB" ) )
                    {
                        pcbLines = &block;
                    }
                }
            }

            if( pcbLines == nullptr )
                pcbLines = &lineBlocks[0];

            wxString           boardKey = pcbUuid + wxS( "_0" );
            wxScopedCharBuffer cb = boardKey.ToUTF8();
            wxString           boardPouredKey = wxBase64Encode( cb.data(), cb.length() );

            const std::multimap<wxString, EASYEDAPRO::POURED>& boardPoured =
                    get_def( m_projectData->m_Poured, boardPouredKey );

            parser.ParseBoard( m_board, project, m_projectData->m_Footprints,
                               m_projectData->m_Blobs, boardPoured, *pcbLines,
                               EASYEDAPRO::ShortenLibName( fname.GetName() ) );

            EASY_IT_BREAK;
        };
        EASYEDAPRO::IterateZipFiles( aFileName, cb );
    }

    return m_board;
}


long long PCB_IO_EASYEDAPRO::GetLibraryTimestamp( const wxString& aLibraryPath ) const
{
    return 0;
}


void PCB_IO_EASYEDAPRO::FootprintEnumerate( wxArrayString&  aFootprintNames,
                                            const wxString& aLibraryPath, bool aBestEfforts,
                                            const std::map<std::string, UTF8>* aProperties )
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
    else if( fname.GetExt() == wxS( "elibz" ) || fname.GetExt() == wxS( "epro" )
             || fname.GetExt() == wxS( "zip" ) )
    {
        nlohmann::json project = EASYEDAPRO::ReadProjectOrDeviceFile( aLibraryPath );
        std::map<wxString, nlohmann::json> footprintMap = project.at( "footprints" );

        for( auto& [key, value] : footprintMap )
        {
            wxString title;

            if( value.contains( "display_title" ) )
                title = value.at( "display_title" ).get<wxString>();
            else
                title = value.at( "title" ).get<wxString>();

            aFootprintNames.Add( title );
        }
    }
}


void PCB_IO_EASYEDAPRO::LoadAllDataFromProject( const wxString&       aProjectPath,
                                                const nlohmann::json& aProject )
{
    if( m_projectData )
        delete m_projectData;

    m_projectData = new PRJ_DATA();

    PCB_IO_EASYEDAPRO_PARSER parser( nullptr, nullptr );
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
        else if( name.EndsWith( wxS( ".ecop" ) ) && EASYEDAPRO::IMPORT_POURED_ECOP )
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


FOOTPRINT* PCB_IO_EASYEDAPRO::FootprintLoad( const wxString& aLibraryPath,
                                             const wxString& aFootprintName, bool aKeepUUID,
                                             const std::map<std::string, UTF8>* aProperties )
{
    // Suppress font substitution warnings (RAII - automatically restored on scope exit)
    FONTCONFIG_REPORTER_SCOPE fontconfigScope( nullptr );

    PCB_IO_EASYEDAPRO_PARSER parser( nullptr, nullptr );
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
    else if( libFname.GetExt() == wxS( "elibz" ) || libFname.GetExt() == wxS( "epro" )
             || libFname.GetExt() == wxS( "zip" ) )
    {
        nlohmann::json project = EASYEDAPRO::ReadProjectOrDeviceFile( aLibraryPath );

        wxString fpUuid;

        std::map<wxString, nlohmann::json> footprintMap = project.at( "footprints" );
        for( auto& [uuid, data] : footprintMap )
        {
            wxString title;

            if( data.contains( "display_title" ) )
                title = data.at( "display_title" ).get<wxString>();
            else
                title = data.at( "title" ).get<wxString>();

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


std::vector<FOOTPRINT*> PCB_IO_EASYEDAPRO::GetImportedCachedLibraryFootprints()
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
