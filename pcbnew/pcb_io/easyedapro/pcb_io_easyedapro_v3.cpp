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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "pcb_io_easyedapro_v3.h"

#include <io/easyedapro/easyedapro_import_utils.h>
#include <io/easyedapro/easyedapro_parser.h>
#include <io/easyedapro/easyedapro_v3_parser.h>

#include <pcb_io/easyedapro/pcb_io_easyedapro_v3_parser.h>

#include <board.h>
#include <core/ignore.h>
#include <font/fontconfig.h>
#include <footprint.h>
#include <ki_exception.h>
#include <progress_reporter.h>
#include <reporter.h>

#include <wx/filename.h>
#include <wx/log.h>


PCB_IO_EASYEDAPRO_V3::PCB_IO_EASYEDAPRO_V3() :
        PCB_IO( wxS( "EasyEDA (JLCEDA) Pro v3" ) )
{
}


PCB_IO_EASYEDAPRO_V3::~PCB_IO_EASYEDAPRO_V3() = default;


const EASYEDAPRO::V3_DOC_PARSER& PCB_IO_EASYEDAPRO_V3::getCachedLibraryParser( const wxString& aLibraryPath ) const
{
    long long timestamp = GetLibraryTimestamp( aLibraryPath );

    if( !m_cachedLibraryParser || m_cachedLibraryPath != aLibraryPath || m_cachedLibraryTimestamp != timestamp )
    {
        m_cachedLibraryParser = std::make_unique<EASYEDAPRO::V3_DOC_PARSER>( aLibraryPath );
        m_cachedLibraryParser->LoadLibrary();
        m_cachedLibraryPath = aLibraryPath;
        m_cachedLibraryTimestamp = timestamp;
    }

    return *m_cachedLibraryParser;
}


bool PCB_IO_EASYEDAPRO_V3::CanReadBoard( const wxString& aFileName ) const
{
    return EASYEDAPRO::V3_DOC_PARSER::IsV3Archive( aFileName );
}


bool PCB_IO_EASYEDAPRO_V3::CanReadLibrary( const wxString& aFileName ) const
{
    if( !PCB_IO::CanReadLibrary( aFileName ) )
        return false;

    return EASYEDAPRO::V3_DOC_PARSER::IsV3Library( aFileName, wxS( "FOOTPRINT" ) );
}


long long PCB_IO_EASYEDAPRO_V3::GetLibraryTimestamp( const wxString& aLibraryPath ) const
{
    wxFileName fileName( aLibraryPath );

    if( !fileName.FileExists() )
        return 0;

    wxDateTime modified = fileName.GetModificationTime();

    if( !modified.IsValid() )
        return 0;

    return modified.GetTicks();
}


void PCB_IO_EASYEDAPRO_V3::FootprintEnumerate( wxArrayString& aFootprintNames, const wxString& aLibraryPath,
                                               bool aBestEfforts, const std::map<std::string, UTF8>* aProperties )
{
    const EASYEDAPRO::V3_DOC_PARSER& v3 = getCachedLibraryParser( aLibraryPath );

    std::map<wxString, wxString> footprintMap =
            EASYEDAPRO::BuildV3LibraryItemMap( v3, "footprints", wxS( "FOOTPRINT" ) );

    for( const auto& [name, uuid] : footprintMap )
        aFootprintNames.Add( name );
}


FOOTPRINT* PCB_IO_EASYEDAPRO_V3::FootprintLoad( const wxString& aLibraryPath, const wxString& aFootprintName,
                                                bool aKeepUUID, const std::map<std::string, UTF8>* aProperties )
{
    FONTCONFIG_REPORTER_SCOPE fontconfigScope( nullptr );

    const EASYEDAPRO::V3_DOC_PARSER& v3 = getCachedLibraryParser( aLibraryPath );

    std::map<wxString, wxString> footprintMap =
            EASYEDAPRO::BuildV3LibraryItemMap( v3, "footprints", wxS( "FOOTPRINT" ) );

    auto fpIt = footprintMap.find( aFootprintName );

    if( fpIt == footprintMap.end() )
        return nullptr;

    const EASYEDAPRO::V3_DOC_RAW* rawDoc = v3.FindRawDoc( wxS( "FOOTPRINT" ), fpIt->second );

    if( !rawDoc )
        return nullptr;

    PCB_IO_EASYEDAPRO_V3_PARSER parser( nullptr, nullptr );
    FOOTPRINT*                  footprint = nullptr;

    try
    {
        footprint = parser.ParseFootprint( nlohmann::json::object(), fpIt->second, EASYEDAPRO::BuildV3BlobMap( v3 ),
                                           *rawDoc );
    }
    catch( nlohmann::json::exception& e )
    {
        THROW_IO_ERROR( wxString::Format( _( "Cannot load footprint '%s' from '%s': %s" ), aFootprintName, aLibraryPath,
                                          e.what() ) );
    }

    if( !footprint )
    {
        THROW_IO_ERROR( wxString::Format( _( "Cannot load footprint '%s' from '%s'" ), aFootprintName, aLibraryPath ) );
    }

    footprint->SetFPID( EASYEDAPRO::ToKiCadLibID( wxEmptyString, aFootprintName ) );
    footprint->Reference().SetVisible( true );
    footprint->Value().SetText( aFootprintName );
    footprint->Value().SetVisible( true );
    footprint->AutoPositionFields();

    return footprint;
}


BOARD* PCB_IO_EASYEDAPRO_V3::LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                                        const std::map<std::string, UTF8>* aProperties, PROJECT* aProject )
{
    ignore_unused( aProject );

    m_props = aProperties;

    m_board = aAppendToMe ? aAppendToMe : new BOARD();

    if( !aAppendToMe )
        m_board->SetFileName( aFileName );

    FONTCONFIG_REPORTER_SCOPE fontconfigScope( &LOAD_INFO_REPORTER::GetInstance() );

    if( m_progressReporter )
    {
        m_progressReporter->Report( wxString::Format( _( "Loading %s..." ), aFileName ) );

        if( !m_progressReporter->KeepRefreshing() )
            THROW_IO_ERROR( _( "File import canceled by user." ) );
    }

    EASYEDAPRO::V3_DOC_PARSER adapter( aFileName );
    adapter.Load();

    nlohmann::json project = EASYEDAPRO::BuildV3ProjectIndexFromRawDocs( adapter );

    wxString pcbToLoad;

    if( m_props && m_props->contains( "pcb_id" ) )
    {
        pcbToLoad = wxString::FromUTF8( m_props->at( "pcb_id" ) );
    }
    else
    {
        std::map<wxString, nlohmann::json> prjPcbs = project.at( "pcbs" );

        if( prjPcbs.size() == 1 )
        {
            pcbToLoad = prjPcbs.begin()->first;
        }
        else if( m_choose_project_handler )
        {
            std::vector<IMPORT_PROJECT_DESC> chosen =
                    m_choose_project_handler( EASYEDAPRO::ProjectToSelectorDialog( project, true, false ) );

            if( !chosen.empty() )
                pcbToLoad = chosen[0].PCBId;
        }
        else if( !prjPcbs.empty() )
        {
            pcbToLoad = prjPcbs.begin()->first;
        }
    }

    if( pcbToLoad.empty() )
        return nullptr;

    PCB_IO_EASYEDAPRO_V3_PARSER parser( nullptr, nullptr );

    std::map<wxString, EASYEDAPRO::BLOB> blobs = EASYEDAPRO::BuildV3BlobMap( adapter );

    wxFileName fp( aFileName );
    wxString   fpLibName = EASYEDAPRO::ShortenLibName( fp.GetName() );

    std::map<wxString, std::unique_ptr<FOOTPRINT>> footprints;

    for( const auto& [uuid, rawDoc] : adapter.GetRawDocs( wxS( "FOOTPRINT" ) ) )
    {
        std::unique_ptr<FOOTPRINT> footprint( parser.ParseFootprint( project, uuid, blobs, rawDoc ) );

        if( !footprint )
            continue;

        std::string           uuidKey = std::string( uuid.ToUTF8() );
        const nlohmann::json& fpMetas = project.at( "footprints" );
        wxString              fpTitle = uuid;

        if( fpMetas.contains( uuidKey ) )
            fpTitle = EASYEDAPRO::GetV3LibraryItemTitle( fpMetas.at( uuidKey ), uuid );

        footprint->SetFPID( EASYEDAPRO::ToKiCadLibID( fpLibName, fpTitle ) );
        footprints.emplace( uuid, std::move( footprint ) );
    }

    const EASYEDAPRO::V3_DOC_RAW* pcbRawDoc = adapter.FindRawDoc( wxS( "PCB" ), pcbToLoad );

    if( !pcbRawDoc )
    {
        THROW_IO_ERROR( wxString::Format( _( "PCB document '%s' not found in '%s'" ), pcbToLoad, aFileName ) );
    }

    std::multimap<wxString, EASYEDAPRO::POURED> poured; // Empty - v3 parser extracts from raw doc

    parser.ParseBoard( m_board, project, footprints, blobs, poured, *pcbRawDoc, fpLibName );

    if( adapter.GetSkippedCount() > 0 )
    {
        wxLogWarning( wxString::Format( _( "EasyEDA (JLCEDA) Pro v3 import skipped %d unsupported object(s)." ),
                                        adapter.GetSkippedCount() ) );
    }

    return m_board;
}
