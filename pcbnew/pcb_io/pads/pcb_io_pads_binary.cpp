/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "pcb_io_pads_binary.h"
#include "pads_binary_parser.h"
#include "pads_layer_mapper.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <set>

#include <board.h>
#include <pcb_track.h>
#include <pcb_text.h>
#include <footprint.h>
#include <zone.h>

#include <io/pads/pads_unit_converter.h>
#include <io/pads/pads_common.h>

#include <netinfo.h>
#include <wx/log.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <pad.h>
#include <pcb_shape.h>
#include <board_design_settings.h>
#include <netclass.h>
#include <geometry/eda_angle.h>
#include <string_utils.h>
#include <progress_reporter.h>
#include <reporter.h>
#include <locale_io.h>
#include <advanced_config.h>
#include <geometry/shape_arc.h>


PCB_IO_PADS_BINARY::PCB_IO_PADS_BINARY() : PCB_IO( "PADS Binary" )
{
    LAYER_MAPPABLE_PLUGIN::RegisterCallback(
            std::bind( &PCB_IO_PADS_BINARY::DefaultLayerMappingCallback, this,
                       std::placeholders::_1 ) );
}


PCB_IO_PADS_BINARY::~PCB_IO_PADS_BINARY() = default;


const IO_BASE::IO_FILE_DESC PCB_IO_PADS_BINARY::GetBoardFileDesc() const
{
    IO_FILE_DESC desc;
    desc.m_FileExtensions.emplace_back( "pcb" );
    desc.m_Description = "PADS Binary";
    return desc;
}


const IO_BASE::IO_FILE_DESC PCB_IO_PADS_BINARY::GetLibraryDesc() const
{
    return IO_FILE_DESC( "PADS Binary Library", { "pcb" } );
}


long long PCB_IO_PADS_BINARY::GetLibraryTimestamp( const wxString& aLibraryPath ) const
{
    return 0;
}


bool PCB_IO_PADS_BINARY::CanReadBoard( const wxString& aFileName ) const
{
    if( !PCB_IO::CanReadBoard( aFileName ) )
        return false;

    return PADS_IO::BINARY_PARSER::IsBinaryPadsFile( aFileName );
}


BOARD* PCB_IO_PADS_BINARY::LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                                       const std::map<std::string, UTF8>* aProperties,
                                       PROJECT* aProject )
{
    LOCALE_IO setlocale;

    std::unique_ptr<BOARD> board( aAppendToMe ? aAppendToMe : new BOARD() );

    if( m_reporter )
        m_reporter->Report( _( "Starting PADS binary PCB import" ), RPT_SEVERITY_INFO );

    if( m_progressReporter )
        m_progressReporter->SetNumPhases( 3 );

    PADS_IO::BINARY_PARSER parser;

    try
    {
        parser.Parse( aFileName );
    }
    catch( const std::exception& e )
    {
        THROW_IO_ERROR( wxString::Format( "Error parsing PADS binary file: %s", e.what() ) );
    }

    m_loadBoard = board.get();
    m_parser = &parser;

    try
    {
        if( m_progressReporter )
            m_progressReporter->BeginPhase( 1 );

        loadBoardSetup();
        loadNets();

        if( m_progressReporter )
            m_progressReporter->BeginPhase( 2 );

        loadFootprints();
        loadBoardOutline();
        loadTracksAndVias();
        loadTexts();
        loadZones();

        reportStatistics();
    }
    catch( ... )
    {
        clearLoadingState();
        throw;
    }

    clearLoadingState();
    return board.release();
}


void PCB_IO_PADS_BINARY::loadBoardSetup()
{
    m_layerMapper.SetCopperLayerCount( m_parser->GetParameters().layer_count );

    std::vector<PADS_IO::LAYER_INFO> padsLayerInfos = m_parser->GetLayerInfos();

    auto convertLayerType = []( PADS_IO::PADS_LAYER_FUNCTION func ) -> PADS_LAYER_TYPE {
        switch( func )
        {
        case PADS_IO::PADS_LAYER_FUNCTION::ROUTING:
        case PADS_IO::PADS_LAYER_FUNCTION::PLANE:
        case PADS_IO::PADS_LAYER_FUNCTION::MIXED:
            return PADS_LAYER_TYPE::COPPER_INNER;
        case PADS_IO::PADS_LAYER_FUNCTION::SOLDER_MASK:
            return PADS_LAYER_TYPE::SOLDERMASK_TOP;
        case PADS_IO::PADS_LAYER_FUNCTION::PASTE_MASK:
            return PADS_LAYER_TYPE::PASTE_TOP;
        case PADS_IO::PADS_LAYER_FUNCTION::SILK_SCREEN:
            return PADS_LAYER_TYPE::SILKSCREEN_TOP;
        case PADS_IO::PADS_LAYER_FUNCTION::ASSEMBLY:
            return PADS_LAYER_TYPE::ASSEMBLY_TOP;
        case PADS_IO::PADS_LAYER_FUNCTION::DOCUMENTATION:
            return PADS_LAYER_TYPE::DOCUMENTATION;
        case PADS_IO::PADS_LAYER_FUNCTION::DRILL:
            return PADS_LAYER_TYPE::DRILL_DRAWING;
        default:
            return PADS_LAYER_TYPE::UNKNOWN;
        }
    };

    for( const auto& padsInfo : padsLayerInfos )
    {
        PADS_LAYER_INFO info;
        info.padsLayerNum = padsInfo.number;
        info.name = padsInfo.name;

        if( padsInfo.layer_type != PADS_IO::PADS_LAYER_FUNCTION::UNKNOWN )
        {
            info.type = convertLayerType( padsInfo.layer_type );

            if( info.type == PADS_LAYER_TYPE::COPPER_INNER )
            {
                if( padsInfo.number == 1 )
                    info.type = PADS_LAYER_TYPE::COPPER_TOP;
                else if( padsInfo.number == m_parser->GetParameters().layer_count )
                    info.type = PADS_LAYER_TYPE::COPPER_BOTTOM;
            }
        }
        else
        {
            info.type = m_layerMapper.GetLayerType( padsInfo.number );
        }

        info.required = padsInfo.required;
        m_layerInfos.push_back( info );
    }

    std::vector<INPUT_LAYER_DESC> inputDescs =
            m_layerMapper.BuildInputLayerDescriptions( m_layerInfos );

    if( m_layer_mapping_handler )
        m_layerMap = m_layer_mapping_handler( inputDescs );

    int copperLayerCount = m_parser->GetParameters().layer_count;

    if( copperLayerCount < 1 )
        copperLayerCount = 2;

    m_loadBoard->SetCopperLayerCount( copperLayerCount );

    // Binary files always use BASIC units
    m_unitConverter.SetBasicUnitsMode( true );
    m_scaleFactor = PADS_UNIT_CONVERTER::BASIC_TO_NM;

    m_originX = m_parser->GetParameters().origin.x;
    m_originY = m_parser->GetParameters().origin.y;

    // Fall back to board outline center only when the parser found no DFT origin.
    // The DFT origin produces exact coordinate matches; overriding it shifts all parts.
    if( m_originX == 0.0 && m_originY == 0.0 )
    {
        const auto& boardOutlines = m_parser->GetBoardOutlines();

        if( !boardOutlines.empty() )
        {
            double minX = std::numeric_limits<double>::max();
            double maxX = std::numeric_limits<double>::lowest();
            double minY = std::numeric_limits<double>::max();
            double maxY = std::numeric_limits<double>::lowest();

            for( const auto& outline : boardOutlines )
            {
                for( const auto& pt : outline.points )
                {
                    minX = std::min( minX, pt.x );
                    maxX = std::max( maxX, pt.x );
                    minY = std::min( minY, pt.y );
                    maxY = std::max( maxY, pt.y );
                }
            }

            if( minX < maxX && minY < maxY )
            {
                m_originX = ( minX + maxX ) / 2.0;
                m_originY = ( minY + maxY ) / 2.0;
            }
        }
    }
}


void PCB_IO_PADS_BINARY::loadNets()
{
    const auto& nets = m_parser->GetNets();

    for( const auto& padsNet : nets )
        ensureNet( padsNet.name );
}


void PCB_IO_PADS_BINARY::loadFootprints()
{
    const auto& decals = m_parser->GetPartDecals();
    const auto& parts = m_parser->GetParts();

    for( const auto& padsPart : parts )
    {
        FOOTPRINT* footprint = new FOOTPRINT( m_loadBoard );
        footprint->SetReference( padsPart.name );

        KIID symbolUuid = PADS_COMMON::GenerateDeterministicUuid( padsPart.name );
        KIID_PATH path;
        path.push_back( symbolUuid );
        footprint->SetPath( path );

        std::string decalName = padsPart.decal;

        LIB_ID fpid;

        if( !decalName.empty() )
            fpid.SetLibItemName( wxString::FromUTF8( decalName ) );
        else
            fpid.SetLibItemName( wxString::FromUTF8( padsPart.name ) );

        footprint->SetFPID( fpid );
        footprint->SetValue( padsPart.decal );

        footprint->SetPosition( VECTOR2I( scaleCoord( padsPart.location.x, true ),
                                           scaleCoord( padsPart.location.y, false ) ) );
        footprint->SetOrientation( EDA_ANGLE( padsPart.rotation, DEGREES_T ) );
        footprint->SetLayer( F_Cu );

        m_loadBoard->Add( footprint );

        if( padsPart.bottom_layer )
            footprint->Flip( footprint->GetPosition(), FLIP_DIRECTION::LEFT_RIGHT );
    }
}


void PCB_IO_PADS_BINARY::loadBoardOutline()
{
    for( const PADS_IO::POLYLINE& polyline : m_parser->GetBoardOutlines() )
    {
        const auto& pts = polyline.points;

        if( pts.size() < 2 )
            continue;

        for( size_t i = 0; i < pts.size() - 1; ++i )
        {
            const PADS_IO::ARC_POINT& p1 = pts[i];
            const PADS_IO::ARC_POINT& p2 = pts[i + 1];

            if( std::abs( p1.x - p2.x ) < 0.001 && std::abs( p1.y - p2.y ) < 0.001 )
                continue;

            PCB_SHAPE* shape = new PCB_SHAPE( m_loadBoard );
            shape->SetShape( SHAPE_T::SEGMENT );
            shape->SetStart( VECTOR2I( scaleCoord( p1.x, true ),
                                       scaleCoord( p1.y, false ) ) );
            shape->SetEnd( VECTOR2I( scaleCoord( p2.x, true ),
                                     scaleCoord( p2.y, false ) ) );
            shape->SetWidth( scaleSize( polyline.width ) );
            shape->SetLayer( Edge_Cuts );
            m_loadBoard->Add( shape );
        }

        if( polyline.closed && pts.size() > 2 )
        {
            const PADS_IO::ARC_POINT& pLast = pts.back();
            const PADS_IO::ARC_POINT& pFirst = pts.front();

            bool needsClosing = ( std::abs( pLast.x - pFirst.x ) > 0.001
                                  || std::abs( pLast.y - pFirst.y ) > 0.001 );

            if( needsClosing )
            {
                PCB_SHAPE* shape = new PCB_SHAPE( m_loadBoard );
                shape->SetShape( SHAPE_T::SEGMENT );
                shape->SetStart( VECTOR2I( scaleCoord( pLast.x, true ),
                                           scaleCoord( pLast.y, false ) ) );
                shape->SetEnd( VECTOR2I( scaleCoord( pFirst.x, true ),
                                         scaleCoord( pFirst.y, false ) ) );
                shape->SetWidth( scaleSize( polyline.width ) );
                shape->SetLayer( Edge_Cuts );
                m_loadBoard->Add( shape );
            }
        }
    }
}


void PCB_IO_PADS_BINARY::loadTracksAndVias()
{
    const auto& routes = m_parser->GetRoutes();
    std::set<std::pair<int, int>> placedThroughVias;

    for( const auto& route : routes )
    {
        NETINFO_ITEM* net = nullptr;

        if( !route.net_name.empty() )
        {
            net = m_loadBoard->FindNet(
                    PADS_COMMON::ConvertInvertedNetName( route.net_name ) );

            if( !net )
                continue;
        }

        for( const auto& track_def : route.tracks )
        {
            if( track_def.points.size() < 2 )
                continue;

            PCB_LAYER_ID track_layer = getMappedLayer( track_def.layer );

            // Binary routes with layer 0 default to F_Cu since the binary
            // format's layer field mapping is not yet fully decoded.
            if( !IsCopperLayer( track_layer ) )
            {
                if( route.net_name.empty() && track_def.layer == 0 )
                {
                    track_layer = F_Cu;
                }
                else
                {
                    if( m_reporter )
                    {
                        m_reporter->Report( wxString::Format(
                                _( "Skipping track on non-copper layer %d" ), track_def.layer ),
                                RPT_SEVERITY_WARNING );
                    }

                    continue;
                }
            }

            int track_width = scaleSize( track_def.width );

            if( track_width <= 0 )
                track_width = scaleSize( 8.0 );

            for( size_t i = 0; i < track_def.points.size() - 1; ++i )
            {
                const PADS_IO::ARC_POINT& p1 = track_def.points[i];
                const PADS_IO::ARC_POINT& p2 = track_def.points[i + 1];

                VECTOR2I start( scaleCoord( p1.x, true ), scaleCoord( p1.y, false ) );
                VECTOR2I end( scaleCoord( p2.x, true ), scaleCoord( p2.y, false ) );

                if( ( start - end ).EuclideanNorm() < 1000 )
                    continue;

                if( p2.is_arc )
                {
                    VECTOR2I center( scaleCoord( p2.arc.cx, true ),
                                     scaleCoord( p2.arc.cy, false ) );

                    bool clockwise = ( p2.arc.delta_angle < 0 );

                    SHAPE_ARC shapeArc;
                    shapeArc.ConstructFromStartEndCenter( start, end, center, clockwise,
                                                         track_width );

                    PCB_ARC* arc = new PCB_ARC( m_loadBoard, &shapeArc );

                    if( net )
                        arc->SetNet( net );

                    arc->SetWidth( track_width );
                    arc->SetLayer( track_layer );
                    m_loadBoard->Add( arc );
                }
                else
                {
                    PCB_TRACK* track = new PCB_TRACK( m_loadBoard );

                    if( net )
                        track->SetNet( net );

                    track->SetWidth( track_width );
                    track->SetLayer( track_layer );
                    track->SetStart( start );
                    track->SetEnd( end );
                    m_loadBoard->Add( track );
                }
            }
        }

        for( const auto& via_def : route.vias )
        {
            VECTOR2I pos( scaleCoord( via_def.location.x, true ),
                          scaleCoord( via_def.location.y, false ) );

            if( placedThroughVias.count( std::make_pair( pos.x, pos.y ) ) )
                continue;

            placedThroughVias.insert( std::make_pair( pos.x, pos.y ) );

            PCB_VIA* via = new PCB_VIA( m_loadBoard );

            if( net )
                via->SetNet( net );

            via->SetPosition( pos );
            via->SetWidth( scaleSize( 20.0 ) );
            via->SetDrill( scaleSize( 10.0 ) );
            via->SetLayerPair( F_Cu, B_Cu );
            via->SetViaType( VIATYPE::THROUGH );
            m_loadBoard->Add( via );
        }
    }
}


void PCB_IO_PADS_BINARY::loadTexts()
{
    const auto& texts = m_parser->GetTexts();

    for( const auto& pads_text : texts )
    {
        PCB_LAYER_ID textLayer = getMappedLayer( pads_text.layer );

        if( textLayer == UNDEFINED_LAYER )
        {
            if( m_reporter )
            {
                m_reporter->Report( wxString::Format(
                        _( "Text on unmapped layer %d assigned to Comments layer" ),
                        pads_text.layer ), RPT_SEVERITY_WARNING );
            }

            textLayer = Cmts_User;
        }

        PCB_TEXT* text = new PCB_TEXT( m_loadBoard );
        text->SetText( pads_text.content );

        int scaledSize = scaleSize( pads_text.height );
        int charHeight =
                static_cast<int>( scaledSize * ADVANCED_CFG::GetCfg().m_PadsPcbTextHeightScale );
        int charWidth =
                static_cast<int>( scaledSize * ADVANCED_CFG::GetCfg().m_PadsPcbTextWidthScale );
        text->SetTextSize( VECTOR2I( charWidth, charHeight ) );

        if( pads_text.width > 0 )
            text->SetTextThickness( scaleSize( pads_text.width ) );

        EDA_ANGLE textAngle( pads_text.rotation, DEGREES_T );
        text->SetTextAngle( textAngle );

        VECTOR2I pos( scaleCoord( pads_text.location.x, true ),
                      scaleCoord( pads_text.location.y, false ) );
        VECTOR2I textShift( -ADVANCED_CFG::GetCfg().m_PadsTextAnchorOffsetNm, 0 );
        RotatePoint( textShift, textAngle );
        text->SetPosition( pos + textShift );

        if( pads_text.hjust == "LEFT" )
            text->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        else if( pads_text.hjust == "RIGHT" )
            text->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        else
            text->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );

        if( pads_text.vjust == "UP" )
            text->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
        else if( pads_text.vjust == "DOWN" )
            text->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
        else
            text->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );

        text->SetKeepUpright( false );
        text->SetLayer( textLayer );
        m_loadBoard->Add( text );
    }
}


void PCB_IO_PADS_BINARY::loadZones()
{
    const auto& pours = m_parser->GetPours();
    const auto& params = m_parser->GetParameters();

    int maxPriority = 0;

    for( const auto& pour_def : pours )
    {
        if( pour_def.priority > maxPriority )
            maxPriority = pour_def.priority;
    }

    for( const auto& pour_def : pours )
    {
        PCB_LAYER_ID pourLayer = getMappedLayer( pour_def.layer );

        if( pourLayer == UNDEFINED_LAYER )
        {
            if( m_reporter )
            {
                m_reporter->Report( wxString::Format(
                        _( "Skipping pour on unmapped layer %d" ), pour_def.layer ),
                        RPT_SEVERITY_WARNING );
            }

            continue;
        }

        ZONE* zone = new ZONE( m_loadBoard );
        zone->SetLayer( pourLayer );

        zone->Outline()->NewOutline();

        for( const auto& pt : pour_def.points )
        {
            zone->Outline()->Append( scaleCoord( pt.x, true ), scaleCoord( pt.y, false ) );
        }

        if( pour_def.is_cutout )
        {
            zone->SetIsRuleArea( true );
            zone->SetDoNotAllowZoneFills( true );
            zone->SetDoNotAllowTracks( false );
            zone->SetDoNotAllowVias( false );
            zone->SetDoNotAllowPads( false );
            zone->SetDoNotAllowFootprints( false );
            zone->SetZoneName( wxString::Format( wxT( "Cutout_%s" ), pour_def.owner_pour ) );
        }
        else
        {
            NETINFO_ITEM* net = m_loadBoard->FindNet(
                    PADS_COMMON::ConvertInvertedNetName( pour_def.net_name ) );

            if( net )
                zone->SetNet( net );

            int kicadPriority = maxPriority - pour_def.priority + 1;
            zone->SetAssignedPriority( kicadPriority );
            zone->SetMinThickness( scaleSize( pour_def.width ) );

            zone->SetThermalReliefGap( scaleSize( params.thermal_min_clearance ) );
            zone->SetThermalReliefSpokeWidth( scaleSize( params.thermal_line_width ) );

            zone->SetPadConnection( ZONE_CONNECTION::THERMAL );
        }

        m_loadBoard->Add( zone );
    }
}


void PCB_IO_PADS_BINARY::reportStatistics()
{
    if( !m_reporter )
        return;

    size_t trackCount = 0;
    size_t viaCount = 0;

    for( PCB_TRACK* track : m_loadBoard->Tracks() )
    {
        if( track->Type() == PCB_VIA_T )
            viaCount++;
        else
            trackCount++;
    }

    m_reporter->Report( wxString::Format( _( "Imported %zu footprints, %d nets, %zu tracks,"
                                              " %zu vias, %zu zones" ),
                                           m_loadBoard->Footprints().size(),
                                           m_loadBoard->GetNetCount(),
                                           trackCount, viaCount,
                                           m_loadBoard->Zones().size() ),
                         RPT_SEVERITY_INFO );
}


std::map<wxString, PCB_LAYER_ID> PCB_IO_PADS_BINARY::DefaultLayerMappingCallback(
        const std::vector<INPUT_LAYER_DESC>& aInputLayerDescriptionVector )
{
    std::map<wxString, PCB_LAYER_ID> layerMap;

    for( const INPUT_LAYER_DESC& layer : aInputLayerDescriptionVector )
        layerMap[layer.Name] = layer.AutoMapLayer;

    return layerMap;
}


int PCB_IO_PADS_BINARY::scaleSize( double aVal ) const
{
    return static_cast<int>( m_unitConverter.ToNanometersSize( aVal ) );
}


int PCB_IO_PADS_BINARY::scaleCoord( double aVal, bool aIsX ) const
{
    double origin = aIsX ? m_originX : m_originY;

    long long originNm = static_cast<long long>( std::round( origin * m_scaleFactor ) );
    long long valNm = static_cast<long long>( std::round( aVal * m_scaleFactor ) );

    if( aIsX )
        return static_cast<int>( valNm - originNm );
    else
        return static_cast<int>( originNm - valNm );
}


PCB_LAYER_ID PCB_IO_PADS_BINARY::getMappedLayer( int aPadsLayer ) const
{
    for( const auto& info : m_layerInfos )
    {
        if( info.padsLayerNum == aPadsLayer )
        {
            auto it = m_layerMap.find( wxString::FromUTF8( info.name ) );

            if( it != m_layerMap.end() && it->second != UNDEFINED_LAYER )
                return it->second;

            return m_layerMapper.GetAutoMapLayer( aPadsLayer, info.type );
        }
    }

    return m_layerMapper.GetAutoMapLayer( aPadsLayer );
}


void PCB_IO_PADS_BINARY::ensureNet( const std::string& aNetName )
{
    if( aNetName.empty() )
        return;

    wxString wxName = PADS_COMMON::ConvertInvertedNetName( aNetName );

    if( m_loadBoard->FindNet( wxName ) == nullptr )
    {
        NETINFO_ITEM* net = new NETINFO_ITEM(
                m_loadBoard, wxName,
                static_cast<int>( m_loadBoard->GetNetCount() ) + 1 );
        m_loadBoard->Add( net );
    }
}


void PCB_IO_PADS_BINARY::clearLoadingState()
{
    m_loadBoard = nullptr;
    m_parser = nullptr;
    m_unitConverter = PADS_UNIT_CONVERTER();
    m_layerMapper = PADS_LAYER_MAPPER();
    m_layerInfos.clear();
    m_scaleFactor = 0.0;
    m_originX = 0.0;
    m_originY = 0.0;
    m_pinToNetMap.clear();
}
