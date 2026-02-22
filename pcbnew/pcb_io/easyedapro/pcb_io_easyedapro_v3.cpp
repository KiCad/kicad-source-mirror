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


bool PCB_IO_EASYEDAPRO_V3::CanReadBoard( const wxString& aFileName ) const
{
    return EASYEDAPRO::V3_DOC_PARSER::IsV3Archive( aFileName );
}


BOARD* PCB_IO_EASYEDAPRO_V3::LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                                        const std::map<std::string, UTF8>* aProperties,
                                        PROJECT* aProject )
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
            std::vector<IMPORT_PROJECT_DESC> chosen = m_choose_project_handler(
                    EASYEDAPRO::ProjectToSelectorDialog( project, true, false ) );

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

    wxFileName fp( aFileName );
    wxString   fpLibName = EASYEDAPRO::ShortenLibName( fp.GetName() );

    std::map<wxString, std::unique_ptr<FOOTPRINT>> footprints;

    for( const auto& [uuid, rawDoc] : adapter.GetRawDocs( wxS( "FOOTPRINT" ) ) )
    {
        FOOTPRINT* footprint = parser.ParseFootprint( project, uuid, rawDoc );

        if( !footprint )
            continue;

        wxString fpTitle = uuid;

        try
        {
            std::string uuidKey = std::string( uuid.ToUTF8() );

            if( project.contains( "footprints" ) && project.at( "footprints" ).contains( uuidKey ) )
            {
                const nlohmann::json& fpMeta = project.at( "footprints" ).at( uuidKey );

                if( fpMeta.contains( "display_title" ) )
                    fpTitle = fpMeta.at( "display_title" ).get<wxString>();
                else if( fpMeta.contains( "title" ) )
                    fpTitle = fpMeta.at( "title" ).get<wxString>();
            }
        }
        catch( ... )
        {
        }

        LIB_ID fpID = EASYEDAPRO::ToKiCadLibID( fpLibName, fpTitle );
        footprint->SetFPID( fpID );

        footprints.emplace( uuid, footprint );
    }

    std::map<wxString, EASYEDAPRO::BLOB> blobs;

    for( const auto& [uuid, rawDoc] : adapter.GetRawDocs( wxS( "BLOB" ) ) )
    {
        for( const EASYEDAPRO::V3_ROW& row : rawDoc.rows )
        {
            if( row.type != wxS( "BLOB" ) )
                continue;

            EASYEDAPRO::BLOB blob;
            blob.objectId = EASYEDAPRO::V3GetString( row.inner, "objectId" );
            blob.url = EASYEDAPRO::V3GetString( row.inner, "url" );
            blobs[blob.objectId] = blob;
        }
    }

    const EASYEDAPRO::V3_DOC_RAW* pcbRawDoc = adapter.FindRawDoc( wxS( "PCB" ), pcbToLoad );

    if( !pcbRawDoc )
    {
        THROW_IO_ERROR( wxString::Format( _( "PCB document '%s' not found in '%s'" ),
                                          pcbToLoad, aFileName ) );
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
