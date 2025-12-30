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

#include <io/easyeda/easyeda_parser_structs.h>
#include <pcb_io/easyeda/pcb_io_easyeda_plugin.h>
#include <pcb_io/easyeda/pcb_io_easyeda_parser.h>
#include <pcb_io/pcb_io.h>

#include <font/fontconfig.h>
#include <progress_reporter.h>
#include <common.h>
#include <macros.h>
#include <board.h>
#include <footprint.h>
#include <board_design_settings.h>
#include <reporter.h>

#include <wx/log.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>
#include <wx/stdstream.h>

#include <json_common.h>
#include <core/map_helpers.h>


PCB_IO_EASYEDA::PCB_IO_EASYEDA() : PCB_IO( wxS( "EasyEDA (JLCEDA) Standard" ) )
{
}


PCB_IO_EASYEDA::~PCB_IO_EASYEDA()
{
}


static bool FindBoardInStream( const wxString& aName, wxInputStream& aStream, nlohmann::json& aOut,
                               EASYEDA::DOCUMENT& aDoc )
{
    if( aName.Lower().EndsWith( wxS( ".json" ) ) )
    {
        wxStdInputStream sin( aStream );
        nlohmann::json   js = nlohmann::json::parse( sin, nullptr, false );

        if( js.is_discarded() )
            return false;

        EASYEDA::DOCUMENT doc = js.get<EASYEDA::DOCUMENT>();

        if( doc.head.docType == EASYEDA::DOC_TYPE::PCB
            || doc.head.docType == EASYEDA::DOC_TYPE::PCB_MODULE
            || doc.head.docType == EASYEDA::DOC_TYPE::PCB_COMPONENT )
        {
            aOut = js;
            aDoc = doc;
            return true;
        }
    }
    else if( aName.Lower().EndsWith( wxS( ".zip" ) ) )
    {
        std::shared_ptr<wxZipEntry> entry;
        wxZipInputStream            zip( aStream );

        if( !zip.IsOk() )
            return false;

        while( entry.reset( zip.GetNextEntry() ), entry.get() != NULL )
        {
            wxString name = entry->GetName();

            if( FindBoardInStream( name, zip, aOut, aDoc ) )
                return true;
        }
    }

    return false;
}


bool PCB_IO_EASYEDA::CanReadBoard( const wxString& aFileName ) const
{
    if( !PCB_IO::CanReadBoard( aFileName ) )
        return false;

    try
    {
        wxFFileInputStream in( aFileName );
        nlohmann::json     js;
        EASYEDA::DOCUMENT  doc;

        return FindBoardInStream( aFileName, in, js, doc );
    }
    catch( nlohmann::json::exception& )
    {
    }
    catch( std::exception& )
    {
    }

    return false;
}


bool PCB_IO_EASYEDA::CanReadFootprint( const wxString& aFileName ) const
{
    return CanReadBoard( aFileName );
}


bool PCB_IO_EASYEDA::CanReadLibrary( const wxString& aFileName ) const
{
    return CanReadBoard( aFileName );
}


BOARD* PCB_IO_EASYEDA::LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                                  const std::map<std::string, UTF8>* aProperties, PROJECT* aProject )
{
    m_loadedFootprints.clear();

    m_props = aProperties;
    m_board = aAppendToMe ? aAppendToMe : new BOARD();

    // Collect the font substitution warnings (RAII - automatically reset on scope exit)
    FONTCONFIG_REPORTER_SCOPE fontconfigScope( &LOAD_INFO_REPORTER::GetInstance() );

    // Give the filename to the board if it's new
    if( !aAppendToMe )
        m_board->SetFileName( aFileName );

    if( m_progressReporter )
    {
        m_progressReporter->Report( wxString::Format( _( "Loading %s..." ), aFileName ) );

        if( !m_progressReporter->KeepRefreshing() )
            THROW_IO_ERROR( _( "File import canceled by user." ) );
    }

    PCB_IO_EASYEDA_PARSER parser( nullptr );

    try
    {
        wxFFileInputStream in( aFileName );
        nlohmann::json     js;
        EASYEDA::DOCUMENT  doc;

        if( !FindBoardInStream( aFileName, in, js, doc ) )
        {
            THROW_IO_ERROR(
                    wxString::Format( _( "Unable to find a valid board in '%s'" ), aFileName ) );
        }

        EASYEDA::DOCUMENT_PCB pcbDoc = js.get<EASYEDA::DOCUMENT_PCB>();

        const int innerStart = 21;
        const int innerEnd = 52;

        int                              maxLayer = innerStart;
        std::map<PCB_LAYER_ID, wxString> layerNames;

        for( const wxString& layerLine : pcbDoc.layers )
        {
            wxArrayString parts = wxSplit( layerLine, '~', '\0' );
            int           layerId = wxAtoi( parts[0] );
            wxString      layerName = parts[1];
            wxString      layerColor = parts[2];
            wxString      visible = parts[3];
            wxString      active = parts[4];
            bool          enabled = parts[5] != wxS( "false" );

            if( layerId >= innerStart && layerId <= innerEnd && enabled )
                maxLayer = layerId + 1;

            layerNames[parser.LayerToKi( parts[0] )] = layerName;
        }

        m_board->SetCopperLayerCount( 2 + maxLayer - innerStart );

        for( auto& [klayer, name] : layerNames )
            m_board->SetLayerName( klayer, name );

        BOARD_DESIGN_SETTINGS&    bds = m_board->GetDesignSettings();
        std::shared_ptr<NETCLASS> defNetclass = bds.m_NetSettings->GetDefaultNetclass();

        if( pcbDoc.DRCRULE )
        {
            std::map<wxString, nlohmann::json>& rules = *pcbDoc.DRCRULE;

            if( auto defRules = get_opt( rules, "Default" ) )
            {
                wxString key;

                key = wxS( "trackWidth" );
                if( defRules->find( key ) != defRules->end() && defRules->at( key ).is_number() )
                {
                    double val = parser.ScaleSize( defRules->at( key ) );
                    defNetclass->SetTrackWidth( val );
                }

                key = wxS( "clearance" );
                if( defRules->find( key ) != defRules->end() && defRules->at( key ).is_number() )
                {
                    double val = parser.ScaleSize( defRules->at( key ) );
                    defNetclass->SetClearance( val );
                }

                key = wxS( "viaHoleD" );
                if( defRules->find( key ) != defRules->end() && defRules->at( key ).is_number() )
                {
                    double val = parser.ScaleSize( defRules->at( key ) );

                    defNetclass->SetViaDrill( val );
                }

                key = wxS( "viaHoleDiameter" ); // Yes, this is via diameter, not drill diameter
                if( defRules->find( key ) != defRules->end() && defRules->at( key ).is_number() )
                {
                    double val = parser.ScaleSize( defRules->at( key ) );
                    defNetclass->SetViaDiameter( val );
                }
            }
        }

        VECTOR2D origin( doc.head.x, doc.head.y );
        parser.ParseBoard( m_board, origin, m_loadedFootprints, doc.shape );

        // Center the board
        BOX2I     outlineBbox = m_board->ComputeBoundingBox( true );
        PAGE_INFO pageInfo = m_board->GetPageSettings();

        VECTOR2D pageCenter( pcbIUScale.MilsToIU( pageInfo.GetWidthMils() / 2 ),
                             pcbIUScale.MilsToIU( pageInfo.GetHeightMils() / 2 ) );

        VECTOR2D offset = pageCenter - outlineBbox.GetCenter();

        int alignGrid = pcbIUScale.mmToIU( 10 );
        offset.x = KiROUND( offset.x / alignGrid ) * alignGrid;
        offset.y = KiROUND( offset.y / alignGrid ) * alignGrid;

        m_board->Move( offset );
        bds.SetAuxOrigin( offset );

        return m_board;
    }
    catch( nlohmann::json::exception& e )
    {
        THROW_IO_ERROR( wxString::Format( _( "Error loading board '%s': %s" ), aFileName, e.what() ) );
    }
    catch( std::exception& e )
    {
        THROW_IO_ERROR( wxString::Format( _( "Error loading board '%s': %s" ), aFileName, e.what() ) );
    }
}


long long PCB_IO_EASYEDA::GetLibraryTimestamp( const wxString& aLibraryPath ) const
{
    return 0;
}


void PCB_IO_EASYEDA::FootprintEnumerate( wxArrayString&  aFootprintNames,
                                         const wxString& aLibraryPath, bool aBestEfforts,
                                         const std::map<std::string, UTF8>* aProperties )
{
    try
    {
        wxFFileInputStream in( aLibraryPath );
        nlohmann::json     js;
        EASYEDA::DOCUMENT  doc;

        if( !FindBoardInStream( aLibraryPath, in, js, doc ) )
        {
            THROW_IO_ERROR( wxString::Format( _( "Unable to find valid footprints in '%s'" ),
                                              aLibraryPath ) );
        }

        if( doc.head.docType == EASYEDA::DOC_TYPE::PCB
            || doc.head.docType == EASYEDA::DOC_TYPE::PCB_MODULE )
        {
            for( wxString shap : doc.shape )
            {
                shap.Replace( wxS( "#@$" ), "\n" );
                wxArrayString parts = wxSplit( shap, '\n', '\0' );

                if( parts.size() < 1 )
                    continue;

                wxArrayString paramsRoot = wxSplit( parts[0], '~', '\0' );

                if( paramsRoot.size() < 1 )
                    continue;

                wxString rootType = paramsRoot[0];

                if( rootType == wxS( "LIB" ) )
                {
                    if( paramsRoot.size() < 4 )
                        continue;

                    wxString packageName = wxString::Format( wxS( "Unknown_%s_%s" ), paramsRoot[1],
                                                             paramsRoot[2] );

                    wxArrayString paramParts = wxSplit( paramsRoot[3], '`', '\0' );

                    std::map<wxString, wxString> paramMap;

                    for( int i = 1; i < paramParts.size(); i += 2 )
                    {
                        wxString key = paramParts[i - 1];
                        wxString value = paramParts[i];

                        if( key == wxS( "package" ) )
                            packageName = value;

                        paramMap[key] = value;
                    }

                    aFootprintNames.Add( packageName );
                }
            }
        }
        else if( doc.head.docType == EASYEDA::DOC_TYPE::PCB_COMPONENT )
        {
            EASYEDA::DOCUMENT_PCB pcbDoc = js.get<EASYEDA::DOCUMENT_PCB>();

            wxString packageName = wxString::Format( wxS( "Unknown_%s" ),
                                                     pcbDoc.uuid.value_or( wxS( "Unknown" ) ) );

            std::optional<std::map<wxString, wxString>> c_para;

            if( pcbDoc.c_para )
                c_para = pcbDoc.c_para;
            else if( doc.head.c_para )
                c_para = doc.head.c_para;

            if( c_para )
                packageName = get_def( *c_para, wxS( "package" ), packageName );

            aFootprintNames.Add( packageName );
        }
    }
    catch( nlohmann::json::exception& e )
    {
        THROW_IO_ERROR( wxString::Format( _( "Error enumerating footprints in library '%s': %s" ),
                                          aLibraryPath, e.what() ) );
    }
    catch( std::exception& e )
    {
        THROW_IO_ERROR( wxString::Format( _( "Error enumerating footprints in library '%s': %s" ),
                                          aLibraryPath, e.what() ) );
    }
}


FOOTPRINT* PCB_IO_EASYEDA::FootprintLoad( const wxString& aLibraryPath,
                                          const wxString& aFootprintName, bool aKeepUUID,
                                          const std::map<std::string, UTF8>* aProperties )
{
    // Suppress font substitution warnings (RAII - automatically restored on scope exit)
    FONTCONFIG_REPORTER_SCOPE fontconfigScope( nullptr );

    PCB_IO_EASYEDA_PARSER parser( nullptr );

    m_loadedFootprints.clear();

    try
    {
        wxFFileInputStream in( aLibraryPath );
        nlohmann::json     js;
        EASYEDA::DOCUMENT  doc;

        if( !FindBoardInStream( aLibraryPath, in, js, doc ) )
        {
            THROW_IO_ERROR( wxString::Format( _( "Unable to find valid footprints in '%s'" ),
                                              aLibraryPath ) );
        }

        if( doc.head.docType == EASYEDA::DOC_TYPE::PCB
            || doc.head.docType == EASYEDA::DOC_TYPE::PCB_MODULE )
        {
            for( wxString shap : doc.shape )
            {
                if( !shap.Contains( wxS( "LIB" ) ) )
                    continue;

                shap.Replace( wxS( "#@$" ), "\n" );
                wxArrayString parts = wxSplit( shap, '\n', '\0' );

                if( parts.size() < 1 )
                    continue;

                wxArrayString paramsRoot = wxSplit( parts[0], '~', '\0' );

                if( paramsRoot.size() < 1 )
                    continue;

                wxString rootType = paramsRoot[0];

                if( rootType == wxS( "LIB" ) )
                {
                    if( paramsRoot.size() < 4 )
                        continue;

                    VECTOR2D origin( parser.Convert( paramsRoot[1] ),
                                     parser.Convert( paramsRoot[2] ) );

                    wxString packageName = wxString::Format( wxS( "Unknown_%s_%s" ), paramsRoot[1],
                                                             paramsRoot[2] );

                    wxArrayString paramParts = wxSplit( paramsRoot[3], '`', '\0' );

                    std::map<wxString, wxString> paramMap;

                    for( int i = 1; i < paramParts.size(); i += 2 )
                    {
                        wxString key = paramParts[i - 1];
                        wxString value = paramParts[i];

                        if( key == wxS( "package" ) )
                            packageName = value;

                        paramMap[key] = value;
                    }

                    EDA_ANGLE orientation;
                    if( !paramsRoot[4].IsEmpty() )
                        orientation = EDA_ANGLE( parser.Convert( paramsRoot[4] ), DEGREES_T );

                    int layer = 1;

                    if( !paramsRoot[7].IsEmpty() )
                        layer = parser.Convert( paramsRoot[7] );

                    if( packageName == aFootprintName )
                    {
                        parts.RemoveAt( 0 );

                        FOOTPRINT* footprint = parser.ParseFootprint( origin, orientation, layer, nullptr,
                                                                      paramMap, m_loadedFootprints, parts );

                        if( !footprint )
                            return nullptr;

                        footprint->Reference().SetPosition( VECTOR2I() );
                        footprint->Reference().SetTextAngle( ANGLE_0 );
                        footprint->Reference().SetVisible( true );

                        footprint->Value().SetPosition( VECTOR2I() );
                        footprint->Value().SetTextAngle( ANGLE_0 );
                        footprint->Value().SetVisible( true );

                        footprint->AutoPositionFields();

                        return footprint;
                    }
                }
            }
        }
        else if( doc.head.docType == EASYEDA::DOC_TYPE::PCB_COMPONENT )
        {
            EASYEDA::DOCUMENT_PCB pcbDoc = js.get<EASYEDA::DOCUMENT_PCB>();

            wxString packageName = wxString::Format( wxS( "Unknown_%s" ),
                                                     pcbDoc.uuid.value_or( wxS( "Unknown" ) ) );

            std::optional<std::map<wxString, wxString>> c_para;

            if( pcbDoc.c_para )
                c_para = pcbDoc.c_para;
            else if( doc.head.c_para )
                c_para = doc.head.c_para;

            if( c_para )
            {
                packageName = get_def( *c_para, wxS( "package" ), packageName );

                if( packageName != aFootprintName )
                    return nullptr;

                VECTOR2D origin( doc.head.x, doc.head.y );

                FOOTPRINT* footprint = parser.ParseFootprint( origin, ANGLE_0, F_Cu, nullptr, *c_para,
                                                              m_loadedFootprints, doc.shape );

                if( !footprint )
                    return nullptr;

                footprint->Reference().SetPosition( VECTOR2I() );
                footprint->Reference().SetTextAngle( ANGLE_0 );
                footprint->Reference().SetVisible( true );

                footprint->Value().SetPosition( VECTOR2I() );
                footprint->Value().SetTextAngle( ANGLE_0 );
                footprint->Value().SetVisible( true );

                footprint->AutoPositionFields();

                return footprint;
            }
        }
    }
    catch( nlohmann::json::exception& e )
    {
        THROW_IO_ERROR( wxString::Format( _( "Error reading footprint '%s' from library '%s': %s" ),
                                          aFootprintName, aLibraryPath, e.what() ) );
    }
    catch( std::exception& e )
    {
        THROW_IO_ERROR( wxString::Format( _( "Error reading footprint '%s' from library '%s': %s" ),
                                          aFootprintName, aLibraryPath, e.what() ) );
    }

    return nullptr;
}


std::vector<FOOTPRINT*> PCB_IO_EASYEDA::GetImportedCachedLibraryFootprints()
{
    std::vector<FOOTPRINT*> result;

    for( auto& [fpUuid, footprint] : m_loadedFootprints )
    {
        result.push_back( static_cast<FOOTPRINT*>( footprint->Clone() ) );
    }

    return result;
}
