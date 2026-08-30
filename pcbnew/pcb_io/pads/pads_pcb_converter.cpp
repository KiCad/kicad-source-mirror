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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "pads_pcb_converter.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <limits>

#include <wx/file.h>
#include <wx/filefn.h>
#include <wx/filename.h>

#include <advanced_config.h>
#include <board.h>
#include <board_design_settings.h>
#include <board_stackup_manager/board_stackup.h>
#include <geometry/eda_angle.h>
#include <geometry/shape_arc.h>
#include <geometry/shape_line_chain.h>
#include <io/pads/pads_common.h>
#include <math/util.h>
#include <netinfo.h>
#include <pcb_dimension.h>
#include <pcb_text.h>
#include <pcb_track.h>
#include <reporter.h>
#include <string_utils.h>
#include <trigo.h>
#include <zone.h>


PADS_PCB_CONVERTER::PADS_PCB_CONVERTER( BOARD* aBoard, REPORTER* aReporter ) :
        m_board( aBoard ),
        m_reporter( aReporter )
{
}


void PADS_PCB_CONVERTER::SetOrigin( double aX, double aY )
{
    m_originX = aX;
    m_originY = aY;
}


bool PADS_PCB_CONVERTER::SetOriginFromOutlines( const std::vector<PADS_IO::POLYLINE>& aOutlines )
{
    if( aOutlines.empty() )
        return false;

    double minX = std::numeric_limits<double>::max();
    double maxX = std::numeric_limits<double>::lowest();
    double minY = std::numeric_limits<double>::max();
    double maxY = std::numeric_limits<double>::lowest();

    for( const PADS_IO::POLYLINE& outline : aOutlines )
    {
        for( const PADS_IO::ARC_POINT& pt : outline.points )
        {
            minX = std::min( minX, pt.x );
            maxX = std::max( maxX, pt.x );
            minY = std::min( minY, pt.y );
            maxY = std::max( maxY, pt.y );
        }
    }

    if( minX >= maxX || minY >= maxY )
        return false;

    m_originX = ( minX + maxX ) / 2.0;
    m_originY = ( minY + maxY ) / 2.0;

    return true;
}


int PADS_PCB_CONVERTER::ScaleSize( double aVal ) const
{
    int64_t nm = m_unitConverter.ToNanometersSize( aVal );

    return static_cast<int>( std::clamp<int64_t>( nm, INT_MIN, INT_MAX ) );
}


int PADS_PCB_CONVERTER::ScaleCoord( double aVal, bool aIsX ) const
{
    return PADS_COMMON::PadsScaleCoord( aVal, aIsX, m_originX, m_originY, m_scaleFactor );
}


VECTOR2I PADS_PCB_CONVERTER::ScalePoint( double aX, double aY ) const
{
    return VECTOR2I( ScaleCoord( aX, true ), ScaleCoord( aY, false ) );
}


void PADS_PCB_CONVERTER::SetupLayers( const std::vector<PADS_IO::LAYER_INFO>& aPadsLayers, int aPadsLayerCount,
                                      const LAYER_MAPPING_HANDLER& aMappingHandler, bool aResolveUnknownTypeByName )
{
    m_layerMapper.SetCopperLayerCount( aPadsLayerCount );

    auto convertLayerType =
            []( PADS_IO::PADS_LAYER_FUNCTION func ) -> PADS_LAYER_TYPE
            {
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

    for( const PADS_IO::LAYER_INFO& padsInfo : aPadsLayers )
    {
        PADS_LAYER_INFO info;
        info.padsLayerNum = padsInfo.number;
        info.name = padsInfo.name;

        if( padsInfo.layer_type != PADS_IO::PADS_LAYER_FUNCTION::UNKNOWN
            && padsInfo.layer_type != PADS_IO::PADS_LAYER_FUNCTION::UNASSIGNED )
        {
            info.type = convertLayerType( padsInfo.layer_type );

            std::string lowerName = padsInfo.name;
            std::transform( lowerName.begin(), lowerName.end(), lowerName.begin(),
                            []( unsigned char c )
                            {
                                return std::tolower( c );
                            } );

            bool isBottom = lowerName.find( "bottom" ) != std::string::npos
                            || lowerName.find( "bot" ) != std::string::npos;

            if( info.type == PADS_LAYER_TYPE::SOLDERMASK_TOP && isBottom )
                info.type = PADS_LAYER_TYPE::SOLDERMASK_BOTTOM;
            else if( info.type == PADS_LAYER_TYPE::PASTE_TOP && isBottom )
                info.type = PADS_LAYER_TYPE::PASTE_BOTTOM;
            else if( info.type == PADS_LAYER_TYPE::SILKSCREEN_TOP && isBottom )
                info.type = PADS_LAYER_TYPE::SILKSCREEN_BOTTOM;
            else if( info.type == PADS_LAYER_TYPE::ASSEMBLY_TOP && isBottom )
                info.type = PADS_LAYER_TYPE::ASSEMBLY_BOTTOM;
            else if( info.type == PADS_LAYER_TYPE::COPPER_INNER )
            {
                if( padsInfo.number == 1 )
                    info.type = PADS_LAYER_TYPE::COPPER_TOP;
                else if( padsInfo.number == aPadsLayerCount )
                    info.type = PADS_LAYER_TYPE::COPPER_BOTTOM;
            }
        }
        else
        {
            info.type = m_layerMapper.GetLayerType( padsInfo.number );

            if( aResolveUnknownTypeByName && info.type == PADS_LAYER_TYPE::UNKNOWN )
            {
                info.type = m_layerMapper.ParseLayerName( padsInfo.name );

                if( info.type == PADS_LAYER_TYPE::UNKNOWN )
                    info.type = PADS_LAYER_TYPE::DOCUMENTATION;
            }
        }

        info.required = padsInfo.required;
        m_layerInfos.push_back( info );
    }

    std::vector<INPUT_LAYER_DESC> inputDescs = m_layerMapper.BuildInputLayerDescriptions( m_layerInfos );

    if( aMappingHandler )
        m_layerMap = aMappingHandler( inputDescs );

    // SetCopperLayerCount past MAX_CU_LAYERS enables layer ids beyond PCB_LAYER_ID, and BASE_SET
    // resizes rather than rejecting. KiCad also requires an even count, and a one-layer PADS board
    // is real, so round up before capping.
    int requested = std::max( aPadsLayerCount, 2 );

    if( requested % 2 )
        requested++;

    m_copperLayerCount = std::min( requested, MAX_CU_LAYERS );

    if( aPadsLayerCount > MAX_CU_LAYERS && m_reporter )
    {
        // mapInnerCopperLayer collapses everything past the cap onto the last inner layer rather
        // than discarding it, so say that rather than claiming the layers were dropped
        m_reporter->Report( wxString::Format( _( "The PADS file declares %d copper layers; KiCad supports "
                                                 "%d, so the layers past that share the last inner layer." ),
                                              aPadsLayerCount, MAX_CU_LAYERS ),
                            RPT_SEVERITY_WARNING );
    }

    m_board->SetCopperLayerCount( m_copperLayerCount );
}


PCB_LAYER_ID PADS_PCB_CONVERTER::GetMappedLayer( int aPadsLayer ) const
{
    for( const PADS_LAYER_INFO& info : m_layerInfos )
    {
        if( info.padsLayerNum == aPadsLayer )
        {
            auto it = m_layerMap.find( PADS_COMMON::ConvertText( info.name ) );

            if( it != m_layerMap.end() && it->second != UNDEFINED_LAYER )
                return it->second;

            return m_layerMapper.GetAutoMapLayer( aPadsLayer, info.type );
        }
    }

    return m_layerMapper.GetAutoMapLayer( aPadsLayer );
}


void PADS_PCB_CONVERTER::BuildStackup( const std::vector<PADS_IO::LAYER_INFO>& aPadsLayers )
{
    std::vector<const PADS_IO::LAYER_INFO*> copperLayerInfos;

    for( const PADS_IO::LAYER_INFO& li : aPadsLayers )
    {
        if( li.is_copper )
            copperLayerInfos.push_back( &li );
    }

    bool hasStackupData = false;

    for( const PADS_IO::LAYER_INFO* li : copperLayerInfos )
    {
        if( li->layer_thickness > 0.0 || li->dielectric_constant > 0.0 )
        {
            hasStackupData = true;
            break;
        }
    }

    if( !hasStackupData )
        return;

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
    BOARD_STACKUP&         stackup = bds.GetStackupDescriptor();

    stackup.RemoveAll();
    stackup.BuildDefaultStackupList( &bds, m_copperLayerCount );

    std::map<PCB_LAYER_ID, const PADS_IO::LAYER_INFO*> copperInfoMap;

    for( const PADS_IO::LAYER_INFO* li : copperLayerInfos )
    {
        PCB_LAYER_ID kicadLayer = GetMappedLayer( li->number );

        if( kicadLayer != UNDEFINED_LAYER )
            copperInfoMap[kicadLayer] = li;
    }

    // A dielectric item carries the thickness and permittivity of the copper layer above it.
    const PADS_IO::LAYER_INFO* prevCopperInfo = nullptr;

    for( BOARD_STACKUP_ITEM* item : stackup.GetList() )
    {
        if( item->GetType() == BOARD_STACKUP_ITEM_TYPE::BS_ITEM_TYPE_COPPER )
        {
            auto it = copperInfoMap.find( item->GetBrdLayerId() );

            if( it != copperInfoMap.end() )
            {
                prevCopperInfo = it->second;

                if( it->second->copper_thickness > 0.0 )
                    item->SetThickness( ScaleSize( it->second->copper_thickness ) );
            }
        }
        else if( item->GetType() == BOARD_STACKUP_ITEM_TYPE::BS_ITEM_TYPE_DIELECTRIC )
        {
            if( prevCopperInfo )
            {
                if( prevCopperInfo->layer_thickness > 0.0 )
                    item->SetThickness( ScaleSize( prevCopperInfo->layer_thickness ) );

                if( prevCopperInfo->dielectric_constant > 0.0 )
                    item->SetEpsilonR( prevCopperInfo->dielectric_constant );
            }
        }
        else if( item->GetType() == BOARD_STACKUP_ITEM_TYPE::BS_ITEM_TYPE_SILKSCREEN )
        {
            item->SetColor( wxT( "White" ) );
        }
        else if( item->GetType() == BOARD_STACKUP_ITEM_TYPE::BS_ITEM_TYPE_SOLDERMASK )
        {
            item->SetColor( wxT( "Green" ) );
        }
    }

    bds.SetBoardThickness( stackup.BuildBoardThicknessFromStackup() );
    bds.m_HasStackup = true;
}


void PADS_PCB_CONVERTER::EnsureNet( const std::string& aNetName )
{
    if( aNetName.empty() )
        return;

    wxString wxName = PADS_COMMON::ConvertInvertedNetName( aNetName );

    if( m_board->FindNet( wxName ) == nullptr )
    {
        NETINFO_ITEM* net = new NETINFO_ITEM( m_board, wxName, static_cast<int>( m_board->GetNetCount() ) + 1 );
        m_board->Add( net );
    }
}


void PADS_PCB_CONVERTER::AppendArcPoints( SHAPE_LINE_CHAIN& aChain, const std::vector<PADS_IO::ARC_POINT>& aPts ) const
{
    if( aPts.empty() )
        return;

    // Single full-circle entry becomes a 36-segment polygon
    if( aPts.size() == 1 && aPts[0].is_arc && std::abs( aPts[0].arc.delta_angle ) >= 359.0 )
    {
        VECTOR2I center = ScalePoint( aPts[0].arc.cx, aPts[0].arc.cy );
        int      radius = ScaleSize( aPts[0].arc.radius );

        constexpr int NUM_SEGS = 36;

        for( int i = 0; i < NUM_SEGS; i++ )
        {
            double angle = 2.0 * M_PI * i / NUM_SEGS;
            aChain.Append( center.x + KiROUND( radius * cos( angle ) ),
                           center.y + KiROUND( radius * sin( angle ) ) );
        }

        return;
    }

    aChain.Append( ScalePoint( aPts[0].x, aPts[0].y ) );

    for( size_t i = 1; i < aPts.size(); i++ )
    {
        const PADS_IO::ARC_POINT& pt = aPts[i];

        if( pt.is_arc )
        {
            SHAPE_ARC              arc = MakeMidpointArc( aPts[i - 1], pt, 0 );
            const SHAPE_LINE_CHAIN arcPoly = arc.ConvertToPolyline();

            for( int j = 1; j < arcPoly.PointCount(); j++ )
                aChain.Append( arcPoly.CPoint( j ).x, arcPoly.CPoint( j ).y );
        }
        else
        {
            aChain.Append( ScalePoint( pt.x, pt.y ) );
        }
    }
}


SHAPE_ARC PADS_PCB_CONVERTER::MakeMidpointArc( const PADS_IO::ARC_POINT& aPrev, const PADS_IO::ARC_POINT& aCurr,
                                               int aWidth ) const
{
    VECTOR2I start = ScalePoint( aPrev.x, aPrev.y );
    VECTOR2I end = ScalePoint( aCurr.x, aCurr.y );

    double midX, midY;

    if( aCurr.arc.radius == 0.0 )
    {
        // Route arcs specify only CW/CCW direction without explicit geometry.
        // They are semicircles between the two endpoints. Compute the midpoint
        // on the perpendicular bisector of the chord, at distance radius from
        // the chord center (where radius = half the chord length).
        double dx = aCurr.x - aPrev.x;
        double dy = aCurr.y - aPrev.y;

        if( aCurr.arc.delta_angle < 0 )
        {
            // CW: arc bulges to the left of the start-to-end direction
            midX = ( aPrev.x + aCurr.x ) / 2.0 - dy / 2.0;
            midY = ( aPrev.y + aCurr.y ) / 2.0 + dx / 2.0;
        }
        else
        {
            // CCW: arc bulges to the right of the start-to-end direction
            midX = ( aPrev.x + aCurr.x ) / 2.0 + dy / 2.0;
            midY = ( aPrev.y + aCurr.y ) / 2.0 - dx / 2.0;
        }
    }
    else
    {
        // Full arc with explicit center and radius (pours, decals, board outlines).
        // Compute the arc midpoint in PADS coordinate space (before the Y-axis
        // flip in ScaleCoord) so the 3-point constructor gets the correct winding.
        double startAngleRad = atan2( aPrev.y - aCurr.arc.cy, aPrev.x - aCurr.arc.cx );
        double midAngleRad = startAngleRad + ( aCurr.arc.delta_angle * M_PI / 180.0 ) / 2.0;

        midX = aCurr.arc.cx + aCurr.arc.radius * cos( midAngleRad );
        midY = aCurr.arc.cy + aCurr.arc.radius * sin( midAngleRad );
    }

    return SHAPE_ARC( start, ScalePoint( midX, midY ), end, aWidth );
}


void PADS_PCB_CONVERTER::LoadTexts( const std::vector<PADS_IO::TEXT>& aTexts )
{
    for( const PADS_IO::TEXT& padsText : aTexts )
    {
        PCB_LAYER_ID textLayer = GetMappedLayer( padsText.layer );

        if( textLayer == UNDEFINED_LAYER )
        {
            if( m_reporter )
            {
                m_reporter->Report( wxString::Format( _( "Text on unmapped layer %d assigned to Comments layer" ),
                                                      padsText.layer ),
                                    RPT_SEVERITY_WARNING );
            }

            textLayer = Cmts_User;
        }

        PCB_TEXT* text = new PCB_TEXT( m_board );
        text->SetText( PADS_COMMON::ConvertText( padsText.content ) );

        // PADS text cell height includes internal leading and descender space.
        // Scale factors calibrated to match PADS rendered character dimensions.
        int scaledSize = ScaleSize( padsText.height );
        int charHeight = static_cast<int>( scaledSize * ADVANCED_CFG::GetCfg().m_PadsPcbTextHeightScale );
        int charWidth = static_cast<int>( scaledSize * ADVANCED_CFG::GetCfg().m_PadsPcbTextWidthScale );
        text->SetTextSize( VECTOR2I( charWidth, charHeight ) );

        if( padsText.width > 0 )
            text->SetTextThickness( ScaleSize( padsText.width ) );

        EDA_ANGLE textAngle( padsText.rotation, DEGREES_T );
        text->SetTextAngle( textAngle );

        // PADS text anchor differs from KiCad by a small offset along the
        // reading direction. Shift left (toward text start) to compensate.
        VECTOR2I pos = ScalePoint( padsText.location.x, padsText.location.y );
        VECTOR2I textShift( -ADVANCED_CFG::GetCfg().m_PadsTextAnchorOffsetNm, 0 );
        RotatePoint( textShift, textAngle );
        text->SetPosition( pos + textShift );

        if( padsText.hjust == "LEFT" )
            text->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        else if( padsText.hjust == "RIGHT" )
            text->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        else
            text->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );

        if( padsText.vjust == "UP" )
            text->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
        else if( padsText.vjust == "DOWN" )
            text->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
        else
            text->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );

        text->SetKeepUpright( false );
        text->SetLayer( textLayer );

        // Honor the PADS back-side mirror flag.
        text->SetMirrored( padsText.mirrored );

        m_board->Add( text );
    }
}


void PADS_PCB_CONVERTER::LoadKeepouts( const std::vector<PADS_IO::KEEPOUT>& aKeepouts )
{
    int keepoutIndex = 0;

    for( const PADS_IO::KEEPOUT& ko : aKeepouts )
    {
        if( ko.outline.size() < 3 )
            continue;

        ZONE* zone = new ZONE( m_board );
        zone->SetIsRuleArea( true );

        if( ko.layers.empty() )
        {
            zone->SetLayerSet( LSET::AllCuMask() );
        }
        else if( ko.layers.size() == 1 )
        {
            PCB_LAYER_ID koLayer = GetMappedLayer( ko.layers[0] );

            if( koLayer == UNDEFINED_LAYER )
            {
                if( m_reporter )
                {
                    m_reporter->Report( wxString::Format( _( "Skipping keepout on unmapped layer %d" ), ko.layers[0] ),
                                        RPT_SEVERITY_WARNING );
                }

                delete zone;
                continue;
            }

            zone->SetLayer( koLayer );
        }
        else
        {
            LSET layerSet;

            for( int layer : ko.layers )
            {
                PCB_LAYER_ID mappedLayer = GetMappedLayer( layer );

                if( mappedLayer != UNDEFINED_LAYER )
                    layerSet.set( mappedLayer );
            }

            if( layerSet.none() )
            {
                if( m_reporter )
                    m_reporter->Report( _( "Skipping keepout with no valid layers" ), RPT_SEVERITY_WARNING );

                delete zone;
                continue;
            }

            zone->SetLayerSet( layerSet );
        }

        zone->SetDoNotAllowTracks( ko.no_traces );
        zone->SetDoNotAllowVias( ko.no_vias );
        zone->SetDoNotAllowZoneFills( ko.no_copper );
        zone->SetDoNotAllowFootprints( ko.no_components );
        zone->SetDoNotAllowPads( false );

        wxString typeName;

        switch( ko.type )
        {
        case PADS_IO::KEEPOUT_TYPE::ALL:       typeName = wxT( "Keepout" ); break;
        case PADS_IO::KEEPOUT_TYPE::ROUTE:     typeName = wxT( "RouteKeepout" ); break;
        case PADS_IO::KEEPOUT_TYPE::VIA:       typeName = wxT( "ViaKeepout" ); break;
        case PADS_IO::KEEPOUT_TYPE::COPPER:    typeName = wxT( "CopperKeepout" ); break;
        case PADS_IO::KEEPOUT_TYPE::PLACEMENT: typeName = wxT( "PlacementKeepout" ); break;
        }

        zone->SetZoneName( wxString::Format( wxT( "%s_%d" ), typeName, ++keepoutIndex ) );

        SHAPE_LINE_CHAIN koChain;
        AppendArcPoints( koChain, ko.outline );

        // Close the outline if first and last points don't match
        if( ko.outline.size() > 2 )
        {
            const PADS_IO::ARC_POINT& first = ko.outline.front();
            const PADS_IO::ARC_POINT& last = ko.outline.back();

            if( std::abs( first.x - last.x ) > 0.001 || std::abs( first.y - last.y ) > 0.001 )
                koChain.Append( ScalePoint( first.x, first.y ) );
        }

        koChain.SetClosed( true );
        zone->Outline()->AddOutline( koChain );
        zone->SetBorderDisplayStyle( ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE, ZONE::GetDefaultHatchPitch(), true );

        m_board->Add( zone );
    }
}


void PADS_PCB_CONVERTER::LoadDimensions( const std::vector<PADS_IO::DIMENSION>& aDimensions )
{
    for( const PADS_IO::DIMENSION& dim : aDimensions )
    {
        if( dim.points.size() < 2 )
            continue;

        PCB_DIM_ALIGNED* dimension = new PCB_DIM_ALIGNED( m_board, PCB_DIM_ALIGNED_T );

        VECTOR2I start = ScalePoint( dim.points[0].x, dim.points[0].y );
        VECTOR2I end = ScalePoint( dim.points[1].x, dim.points[1].y );

        // PADS horizontal/vertical dimensions measure only the X or Y projection.
        // PCB_DIM_ALIGNED measures along the start-to-end direction, so if the base
        // points differ on the non-measured axis the line becomes skewed.
        // Project the end point onto the measurement axis.
        if( dim.is_horizontal )
            end.y = start.y;
        else
            end.x = start.x;

        dimension->SetStart( start );
        dimension->SetEnd( end );

        // The crossbar_pos is the absolute coordinate of the crossbar, so the height is its
        // offset from the start point.
        if( dim.is_horizontal )
            dimension->SetHeight( -ScaleSize( dim.crossbar_pos - dim.points[0].y ) );
        else
            dimension->SetHeight( ScaleSize( dim.crossbar_pos - dim.points[0].x ) );

        PCB_LAYER_ID dimLayer = GetMappedLayer( dim.layer );

        if( dimLayer == UNDEFINED_LAYER || IsCopperLayer( dimLayer ) )
            dimLayer = Cmts_User;

        dimension->SetLayer( dimLayer );

        // PADS text_width is stroke thickness, not character width.
        // Calculate character dimensions from height.
        if( dim.text_height > 0 )
        {
            int scaledSize = ScaleSize( dim.text_height );
            int charHeight = static_cast<int>( scaledSize * ADVANCED_CFG::GetCfg().m_PadsPcbTextHeightScale );
            int charWidth = static_cast<int>( scaledSize * ADVANCED_CFG::GetCfg().m_PadsPcbTextWidthScale );
            dimension->SetTextSize( VECTOR2I( charWidth, charHeight ) );

            if( dim.text_width > 0 )
                dimension->SetTextThickness( ScaleSize( dim.text_width ) );
        }

        if( !dim.text.empty() )
        {
            dimension->SetOverrideTextEnabled( true );
            dimension->SetOverrideText( PADS_COMMON::ConvertText( dim.text ) );
        }

        // Both dialects share this converter, so a file-unit constant would be a 3 nm hairline in
        // the binary reader's BASIC mode
        dimension->SetLineThickness( pcbIUScale.mmToIU( 0.127 ) );

        if( dim.rotation != 0.0 )
            dimension->SetTextAngle( EDA_ANGLE( dim.rotation, DEGREES_T ) );

        dimension->Update();
        m_board->Add( dimension );
    }
}


void PADS_PCB_CONVERTER::ApplyPourSettings( ZONE* aZone, const PADS_IO::POUR& aPour, int aMaxPriority,
                                            const PADS_IO::PARAMETERS& aParams )
{
    if( aPour.is_cutout )
    {
        aZone->SetIsRuleArea( true );
        aZone->SetDoNotAllowZoneFills( true );
        aZone->SetDoNotAllowTracks( false );
        aZone->SetDoNotAllowVias( false );
        aZone->SetDoNotAllowPads( false );
        aZone->SetDoNotAllowFootprints( false );
        aZone->SetZoneName( wxString::Format( wxT( "Cutout_%s" ),
                                              PADS_COMMON::ConvertText( aPour.owner_pour ) ) );

        return;
    }

    NETINFO_ITEM* net = m_board->FindNet( PADS_COMMON::ConvertInvertedNetName( aPour.net_name ) );

    if( net )
        aZone->SetNet( net );

    // PADS fills the lowest priority number on top, KiCad the highest.
    aZone->SetAssignedPriority( aMaxPriority - aPour.priority + 1 );
    aZone->SetMinThickness( ScaleSize( aPour.width ) );

    aZone->SetThermalReliefGap( ScaleSize( aParams.thermal_min_clearance ) );
    aZone->SetThermalReliefSpokeWidth( ScaleSize( aParams.thermal_line_width ) );

    // A PADS pour connects solid by default; a pad asks for spoke relief through its own RT/ST
    // padstack rows, which both importers apply as a per-pad override.
    aZone->SetPadConnection( ZONE_CONNECTION::FULL );
}


void PADS_PCB_CONVERTER::WriteDiffPairRules( const wxString& aFileName,
                                             const std::vector<PADS_IO::DIFF_PAIR_DEF>& aDiffPairs )
{
    if( aDiffPairs.empty() )
        return;

    // The expression tokenizer reads \ before the closing quote as an escaped quote and has no
    // rule that yields a trailing backslash, so only that one shape has no faithful encoding
    auto isSerializable =
            []( const wxString& aName )
            {
                return !aName.EndsWith( wxS( "\\" ) );
            };

    // The name is read back through two layers, so escape for the inner one first. The expression
    // tokenizer takes \' inside a quoted operand and the s-expression lexer takes \\ \" \r \n
    auto escapeOperand =
            []( const wxString& aName )
            {
                wxString out = aName;

                out.Replace( wxS( "'" ), wxS( "\\'" ) );
                out.Replace( wxS( "\\" ), wxS( "\\\\" ) );
                out.Replace( wxS( "\"" ), wxS( "\\\"" ) );
                out.Replace( wxS( "\r" ), wxS( "\\r" ) );
                out.Replace( wxS( "\n" ), wxS( "\\n" ) );

                return out;
            };

    auto escapeSymbol =
            []( const wxString& aName )
            {
                wxString out = aName;

                out.Replace( wxS( "\\" ), wxS( "\\\\" ) );
                out.Replace( wxS( "\"" ), wxS( "\\\"" ) );
                out.Replace( wxS( "\r" ), wxS( "\\r" ) );
                out.Replace( wxS( "\n" ), wxS( "\\n" ) );

                return out;
            };

    wxString customRules = wxT( "(version 2)\n" );
    bool     hasAnyRule = false;

    for( const PADS_IO::DIFF_PAIR_DEF& dp : aDiffPairs )
    {
        if( dp.name.empty() || dp.gap <= 0 || dp.positive_net.empty() || dp.negative_net.empty() )
            continue;

        wxString ruleName = wxString::Format( wxT( "DiffPair_%s" ), PADS_COMMON::ConvertText( dp.name ) );
        wxString posNet = PADS_COMMON::ConvertInvertedNetName( dp.positive_net );
        wxString negNet = PADS_COMMON::ConvertInvertedNetName( dp.negative_net );

        // The rule name is followed by _gap, so only the net names can end the quoted operand
        if( !isSerializable( posNet ) || !isSerializable( negNet ) )
        {
            if( m_reporter )
            {
                m_reporter->Report( wxString::Format( _( "Skipped design rule for differential pair "
                                                         "'%s'; a net name ends with a backslash." ),
                                                      ruleName ),
                                    RPT_SEVERITY_WARNING );
            }

            continue;
        }

        double   gapMm = dp.gap * m_scaleFactor / PADS_UNIT_CONVERTER::MM_TO_NM;
        wxString gapStr = wxString::FromUTF8( FormatDouble2Str( gapMm ) ) + wxT( "mm" );

        customRules += wxString::Format( wxT( "\n(rule \"%s_gap\"\n" )
                                         wxT( "  (condition \"A.NetName == '%s' && B.NetName == '%s'\")\n" )
                                         wxT( "  (constraint clearance (min %s)))\n" ),
                                         escapeSymbol( ruleName ), escapeOperand( posNet ),
                                         escapeOperand( negNet ), gapStr );
        hasAnyRule = true;
    }

    // A file holding only the version header is clutter, so write nothing unless a rule was emitted
    if( !hasAnyRule )
        return;

    wxFileName fn( aFileName );
    fn.SetExt( wxT( "kicad_dru" ) );

    // An import must not destroy rules the user already has. Creating exclusively both refuses an
    // existing file and closes the race a FileExists test would leave open
    wxFile rulesFile( fn.GetFullPath(), wxFile::write_excl );

    if( !rulesFile.IsOpened() )
    {
        if( m_reporter )
        {
            wxString msg = fn.FileExists() ? _( "Design rules for the imported differential pairs were not "
                                                "written; '%s' already exists." )
                                           : _( "Could not write design rules to '%s'." );

            m_reporter->Report( wxString::Format( msg, fn.GetFullPath() ), RPT_SEVERITY_WARNING );
        }

        return;
    }

    bool written = rulesFile.Write( customRules );

    if( !rulesFile.Close() )
        written = false;

    // A partial sidecar will not parse, and because the file is created exclusively it would also
    // block the next import from writing a good one
    if( !written )
    {
        wxRemoveFile( fn.GetFullPath() );

        if( m_reporter )
        {
            m_reporter->Report( wxString::Format( _( "Could not write design rules to '%s'." ),
                                                  fn.GetFullPath() ),
                                RPT_SEVERITY_WARNING );
        }
    }
}


void PADS_PCB_CONVERTER::ReportStatistics()
{
    if( !m_reporter )
        return;

    size_t trackCount = 0;
    size_t viaCount = 0;

    for( PCB_TRACK* track : m_board->Tracks() )
    {
        if( track->Type() == PCB_VIA_T )
            viaCount++;
        else
            trackCount++;
    }

    m_reporter->Report( wxString::Format( _( "Imported %zu footprints, %d nets, %zu tracks, %zu vias, %zu zones" ),
                                          m_board->Footprints().size(), m_board->GetNetCount(), trackCount, viaCount,
                                          m_board->Zones().size() ),
                        RPT_SEVERITY_INFO );
}
