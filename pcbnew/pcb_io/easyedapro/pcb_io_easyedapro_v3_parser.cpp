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

#include "pcb_io_easyedapro_v3_parser.h"
#include <io/easyedapro/easyedapro_import_utils.h>

#include <memory>

#include <json_common.h>
#include <core/json_serializers.h>
#include <core/map_helpers.h>
#include <string_utils.h>

#include <wx/log.h>
#include <wx/base64.h>
#include <wx/mstream.h>

#include <footprint.h>
#include <board.h>
#include <pcb_group.h>
#include <pcb_shape.h>
#include <pcb_text.h>
#include <pcb_track.h>
#include <pcb_reference_image.h>
#include <geometry/shape_arc.h>
#include <geometry/shape_rect.h>
#include <zone.h>
#include <pad.h>
#include <fix_board_shape.h>
#include <board_design_settings.h>
#include <font/font.h>
#include <core/mirror.h>
#include <convert_basic_shapes_to_polygon.h>

using namespace EASYEDAPRO;

static const int SHAPE_JOIN_DISTANCE = pcbIUScale.mmToIU( 1.5 );


PCB_IO_EASYEDAPRO_V3_PARSER::PCB_IO_EASYEDAPRO_V3_PARSER( BOARD*             aBoard,
                                                            PROGRESS_REPORTER* aProgressReporter ) :
        m_board( aBoard ),
        m_v2Parser( aBoard, aProgressReporter )
{
}


std::unique_ptr<PAD> PCB_IO_EASYEDAPRO_V3_PARSER::createV3PAD( FOOTPRINT*       aFootprint,
                                                                 const V3_ROW&   aRow )
{
    int          layer = V3GetInt( aRow.inner, "layerId", 1 );
    PCB_LAYER_ID klayer = m_v2Parser.LayerToKi( layer );

    wxString padNumber = V3GetString( aRow.inner, "num" );

    VECTOR2D center;
    center.x = V3GetDouble( aRow.inner, "centerX" );
    center.y = V3GetDouble( aRow.inner, "centerY" );

    double orientation = V3GetDouble( aRow.inner, "padAngle" );

    nlohmann::json holeObj = aRow.inner.value( "hole", nlohmann::json() );
    nlohmann::json padDef  = aRow.inner.value( "defaultPad", nlohmann::json() );

    std::unique_ptr<PAD> pad = std::make_unique<PAD>( aFootprint );

    pad->SetNumber( padNumber );
    pad->SetPosition( PCB_IO_EASYEDAPRO_PARSER::ScalePos( center ) );
    pad->SetOrientationDegrees( orientation );

    // Process hole
    bool hasHole = false;

    if( holeObj.is_object() )
    {
        wxString holeType = V3GetString( holeObj, "holeType", wxS( "ROUND" ) );

        if( holeType == wxS( "RECT" ) )
            holeType = wxS( "SLOT" );

        if( holeType != wxS( "ROUND" ) && holeType != wxS( "SLOT" ) )
            holeType = wxS( "ROUND" );

        VECTOR2D drill;
        drill.x = V3GetDouble( holeObj, "width" );
        drill.y = V3GetDouble( holeObj, "height" );

        if( drill.x > 0 || drill.y > 0 )
        {
            hasHole = true;

            double drill_dir = V3GetDouble( aRow.inner, "relativeAngle" );

            double deg = EDA_ANGLE( drill_dir, DEGREES_T ).Normalize90().AsDegrees();

            if( std::abs( deg ) >= 45 )
                std::swap( drill.x, drill.y );

            if( holeType == wxS( "SLOT" ) )
                pad->SetDrillShape( PAD_DRILL_SHAPE::OBLONG );

            pad->SetDrillSize( PCB_IO_EASYEDAPRO_PARSER::ScaleSize( drill ) );
            pad->SetLayerSet( PAD::PTHMask() );
            pad->SetAttribute( PAD_ATTRIB::PTH );
        }
    }

    if( !hasHole )
    {
        if( klayer == F_Cu )
            pad->SetLayerSet( PAD::SMDMask() );
        else if( klayer == B_Cu )
            pad->SetLayerSet( PAD::SMDMask().FlipStandardLayers() );

        pad->SetAttribute( PAD_ATTRIB::SMD );
    }

    // Process pad shape
    if( padDef.is_object() )
    {
        wxString padType = V3GetString( padDef, "padType", wxS( "RECT" ) );

        if( padType == wxS( "RECT" ) )
        {
            VECTOR2D size;
            size.x = V3GetDouble( padDef, "width", 1.0 );
            size.y = V3GetDouble( padDef, "height", 1.0 );
            double radius = V3GetDouble( padDef, "radius" );
            double radiusRatio = std::clamp( radius, 0.0, 100.0 ) / 100 / 2;

            pad->SetSize( PADSTACK::ALL_LAYERS,
                          PCB_IO_EASYEDAPRO_PARSER::ScaleSize( size ) );

            if( radiusRatio == 0 )
                pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );
            else
            {
                pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::ROUNDRECT );
                pad->SetRoundRectRadiusRatio( PADSTACK::ALL_LAYERS, radiusRatio );
            }
        }
        else if( padType == wxS( "ELLIPSE" ) )
        {
            VECTOR2D size;
            size.x = V3GetDouble( padDef, "width", 1.0 );
            size.y = V3GetDouble( padDef, "height", 1.0 );

            pad->SetSize( PADSTACK::ALL_LAYERS,
                          PCB_IO_EASYEDAPRO_PARSER::ScaleSize( size ) );
            pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
        }
        else if( padType == wxS( "OVAL" ) )
        {
            VECTOR2D size;
            size.x = V3GetDouble( padDef, "width", 1.0 );
            size.y = V3GetDouble( padDef, "height", 1.0 );

            pad->SetSize( PADSTACK::ALL_LAYERS,
                          PCB_IO_EASYEDAPRO_PARSER::ScaleSize( size ) );
            pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::OVAL );
        }
        else if( padType == wxS( "POLY" ) || padType == wxS( "POLYGON" ) )
        {
            pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CUSTOM );
            pad->SetAnchorPadShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
            pad->SetSize( PADSTACK::ALL_LAYERS, { 1, 1 } );

            nlohmann::json polyData = padDef.value( "path", nlohmann::json() );

            std::vector<std::unique_ptr<PCB_SHAPE>> results =
                    m_v2Parser.ParsePoly( aFootprint, polyData, true, false );

            for( auto& shape : results )
            {
                shape->SetLayer( klayer );
                shape->SetWidth( 0 );
                shape->Move( -pad->GetPosition() );
                pad->AddPrimitive( PADSTACK::ALL_LAYERS, shape.release() );
            }
        }
    }
    else
    {
        pad->SetSize( PADSTACK::ALL_LAYERS,
                      PCB_IO_EASYEDAPRO_PARSER::ScaleSize( VECTOR2D( 1.0, 1.0 ) ) );
        pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );
    }

    pad->SetThermalSpokeAngle( ANGLE_90 );

    return pad;
}


FOOTPRINT* PCB_IO_EASYEDAPRO_V3_PARSER::ParseFootprint(
        const nlohmann::json& aProject, const wxString& aFpUuid,
        const V3_DOC_RAW& aDoc )
{
    std::unique_ptr<FOOTPRINT> footprintPtr = std::make_unique<FOOTPRINT>( m_board );
    FOOTPRINT*                 footprint = footprintPtr.get();

    const VECTOR2I defaultTextSize( pcbIUScale.mmToIU( 1.0 ), pcbIUScale.mmToIU( 1.0 ) );
    const int      defaultTextThickness( pcbIUScale.mmToIU( 0.15 ) );

    for( PCB_FIELD* field : footprint->GetFields() )
    {
        field->SetTextSize( defaultTextSize );
        field->SetTextThickness( defaultTextThickness );
    }

    for( const V3_ROW& row : aDoc.rows )
    {
        if( row.type == wxS( "POLY" ) )
        {
            int          layer = V3GetInt( row.inner, "layerId", 1 );
            PCB_LAYER_ID klayer = m_v2Parser.LayerToKi( layer );
            double       thickness = V3GetDouble( row.inner, "width" );

            nlohmann::json polyData = row.inner.value( "path", nlohmann::json::array() );

            std::vector<std::unique_ptr<PCB_SHAPE>> results =
                    m_v2Parser.ParsePoly( footprint, polyData, false, false );

            for( auto& shape : results )
            {
                shape->SetLayer( klayer );
                shape->SetWidth( PCB_IO_EASYEDAPRO_PARSER::ScaleSize( thickness ) );
                footprint->Add( shape.release(), ADD_MODE::APPEND );
            }
        }
        else if( row.type == wxS( "PAD" ) )
        {
            std::unique_ptr<PAD> pad = createV3PAD( footprint, row );
            footprint->Add( pad.release(), ADD_MODE::APPEND );
        }
        else if( row.type == wxS( "FILL" ) )
        {
            int          layer = V3GetInt( row.inner, "layerId", 1 );
            PCB_LAYER_ID klayer = m_v2Parser.LayerToKi( layer );

            nlohmann::json polyDataList = row.inner.value( "path", nlohmann::json::array() );

            if( !polyDataList.is_null() && !polyDataList.empty() )
            {
                if( !polyDataList.at( 0 ).is_array() )
                    polyDataList = nlohmann::json::array( { polyDataList } );

                std::vector<SHAPE_LINE_CHAIN> contours;

                for( nlohmann::json& polyData : polyDataList )
                {
                    SHAPE_LINE_CHAIN contour = m_v2Parser.ParseContour( polyData, false );
                    contour.SetClosed( true );
                    contours.push_back( contour );
                }

                SHAPE_POLY_SET polySet;

                for( SHAPE_LINE_CHAIN& contour : contours )
                    polySet.AddOutline( contour );

                polySet.RebuildHolesFromContours();

                std::unique_ptr<PCB_GROUP> group;

                if( polySet.OutlineCount() > 1 )
                    group = std::make_unique<PCB_GROUP>( footprint );

                for( const SHAPE_POLY_SET::POLYGON& poly : polySet.CPolygons() )
                {
                    std::unique_ptr<PCB_SHAPE> shape =
                            std::make_unique<PCB_SHAPE>( footprint, SHAPE_T::POLY );

                    shape->SetFilled( true );
                    shape->SetPolyShape( poly );
                    shape->SetLayer( klayer );
                    shape->SetWidth( 0 );

                    if( group )
                        group->AddItem( shape.get() );

                    footprint->Add( shape.release(), ADD_MODE::APPEND );
                }

                if( group )
                    footprint->Add( group.release(), ADD_MODE::APPEND );
            }
        }
        else if( row.type == wxS( "ATTR" ) )
        {
            wxString key = V3GetString( row.inner, "key" );
            wxString value = V3JsonToString( row.inner.value( "value", nlohmann::json() ) );

            if( key == wxS( "Designator" ) )
                footprint->GetField( FIELD_T::REFERENCE )->SetText( value );
        }
        else if( row.type == wxS( "REGION" ) )
        {
            int          layer = V3GetInt( row.inner, "layerId", 1 );
            PCB_LAYER_ID klayer = m_v2Parser.LayerToKi( layer );

            nlohmann::json polyDataList = row.inner.value( "path", nlohmann::json::array() );
            nlohmann::json prohibitTypes = row.inner.value( "prohibitType", nlohmann::json::array() );

            std::set<int> flags;

            if( prohibitTypes.is_array() )
            {
                for( const auto& pt : prohibitTypes )
                {
                    wxString ptStr;

                    if( pt.is_string() )
                        ptStr = pt.get<std::string>();

                    if( ptStr == wxS( "COMPONENT" ) )
                        flags.insert( 2 );
                    else if( ptStr == wxS( "TRACK" ) )
                        flags.insert( 5 );
                    else if( ptStr == wxS( "COPPER" ) )
                        flags.insert( 6 );
                    else if( ptStr == wxS( "VIA" ) )
                        flags.insert( 7 );
                    else if( ptStr == wxS( "FILL" ) || ptStr == wxS( "PLANE" ) )
                        flags.insert( 8 );
                }
            }

            for( nlohmann::json& polyData : polyDataList )
            {
                SHAPE_POLY_SET polySet;

                std::vector<std::unique_ptr<PCB_SHAPE>> results =
                        m_v2Parser.ParsePoly( nullptr, polyData, true, false );

                for( auto& shape : results )
                {
                    shape->SetFilled( true );
                    shape->TransformShapeToPolygon( polySet, klayer, 0, ARC_HIGH_DEF,
                                                    ERROR_INSIDE, true );
                }

                polySet.Simplify();

                std::unique_ptr<ZONE> zone = std::make_unique<ZONE>( footprint );

                zone->SetIsRuleArea( true );
                zone->SetDoNotAllowFootprints( !!flags.count( 2 ) );
                zone->SetDoNotAllowZoneFills( !!flags.count( 7 ) || !!flags.count( 6 )
                                              || !!flags.count( 8 ) );
                zone->SetDoNotAllowPads( !!flags.count( 7 ) );
                zone->SetDoNotAllowTracks( !!flags.count( 7 ) || !!flags.count( 5 ) );
                zone->SetDoNotAllowVias( !!flags.count( 7 ) );

                zone->SetLayer( klayer );
                zone->Outline()->Append( polySet );

                footprint->Add( zone.release(), ADD_MODE::APPEND );
            }
        }
    }

    // Extract 3D model info from project
    if( aProject.is_object() && aProject.contains( "devices" ) )
    {
        std::map<wxString, EASYEDAPRO::PRJ_DEVICE> devicesMap = aProject.at( "devices" );

        for( auto& [devUuid, devData] : devicesMap )
        {
            if( auto fp = get_opt( devData.attributes, "Footprint" ) )
            {
                if( *fp == aFpUuid )
                    break;
            }
        }
    }

    // Heal board outlines
    std::vector<PCB_SHAPE*> edgeShapes;

    for( BOARD_ITEM* item : footprint->GraphicalItems() )
    {
        if( item->IsOnLayer( Edge_Cuts ) && item->Type() == PCB_SHAPE_T )
            edgeShapes.push_back( static_cast<PCB_SHAPE*>( item ) );
    }

    ConnectBoardShapes( edgeShapes, SHAPE_JOIN_DISTANCE );

    // Build courtyard if missing
    if( !footprint->IsOnLayer( F_CrtYd ) )
    {
        BOX2I bbox = footprint->GetLayerBoundingBox( { F_Cu, F_Fab, F_Paste, F_Mask, Edge_Cuts } );
        bbox.Inflate( pcbIUScale.mmToIU( 0.25 ) );

        std::unique_ptr<PCB_SHAPE> shape =
                std::make_unique<PCB_SHAPE>( footprint, SHAPE_T::RECTANGLE );

        shape->SetWidth( pcbIUScale.mmToIU( DEFAULT_COURTYARD_WIDTH ) );
        shape->SetLayer( F_CrtYd );
        shape->SetStart( bbox.GetOrigin() );
        shape->SetEnd( bbox.GetEnd() );

        footprint->Add( shape.release(), ADD_MODE::APPEND );
    }

    // Add F_Fab reference text if missing
    bool hasFabRef = false;

    for( BOARD_ITEM* item : footprint->GraphicalItems() )
    {
        if( item->Type() == PCB_TEXT_T && item->IsOnLayer( F_Fab ) )
        {
            if( static_cast<PCB_TEXT*>( item )->GetText() == wxT( "${REFERENCE}" ) )
            {
                hasFabRef = true;
                break;
            }
        }
    }

    if( !hasFabRef )
    {
        int c_refTextSize = pcbIUScale.mmToIU( 0.5 );
        int c_refTextThickness = pcbIUScale.mmToIU( 0.1 );
        std::unique_ptr<PCB_TEXT> refText = std::make_unique<PCB_TEXT>( footprint );

        refText->SetLayer( F_Fab );
        refText->SetTextSize( VECTOR2I( c_refTextSize, c_refTextSize ) );
        refText->SetTextThickness( c_refTextThickness );
        refText->SetText( wxT( "${REFERENCE}" ) );

        footprint->Add( refText.release(), ADD_MODE::APPEND );
    }

    return footprintPtr.release();
}


static void V3AlignText( EDA_TEXT* text, int align )
{
    switch( align )
    {
    case 1:
        text->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
        text->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        break;
    case 2:
        text->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
        text->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        break;
    case 3:
        text->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
        text->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        break;
    case 4:
        text->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
        text->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        break;
    case 5:
        text->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
        text->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        break;
    case 6:
        text->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
        text->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        break;
    case 7:
        text->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
        text->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        break;
    case 8:
        text->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
        text->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        break;
    case 9:
        text->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
        text->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        break;
    }
}


static int V3AlignToOriginCode( const wxString& aAlign )
{
    if( aAlign == wxS( "LEFT_TOP" ) )
        return 1;
    if( aAlign == wxS( "LEFT_CENTER" ) || aAlign == wxS( "LEFT_MIDDLE" ) )
        return 2;
    if( aAlign == wxS( "LEFT_BOTTOM" ) )
        return 3;
    if( aAlign == wxS( "CENTER_TOP" ) )
        return 4;
    if( aAlign == wxS( "CENTER_MIDDLE" ) || aAlign == wxS( "CENTER_CENTER" ) )
        return 5;
    if( aAlign == wxS( "CENTER_BOTTOM" ) )
        return 6;
    if( aAlign == wxS( "RIGHT_TOP" ) )
        return 7;
    if( aAlign == wxS( "RIGHT_CENTER" ) || aAlign == wxS( "RIGHT_MIDDLE" ) )
        return 8;
    if( aAlign == wxS( "RIGHT_BOTTOM" ) )
        return 9;

    return 3; // default: LEFT_BOTTOM
}


void PCB_IO_EASYEDAPRO_V3_PARSER::ParseBoard(
        BOARD* aBoard, const nlohmann::json& aProject,
        std::map<wxString, std::unique_ptr<FOOTPRINT>>&    aFootprintMap,
        const std::map<wxString, EASYEDAPRO::BLOB>&        aBlobMap,
        const std::multimap<wxString, EASYEDAPRO::POURED>& aPouredMap,
        const V3_DOC_RAW& aDoc, const wxString& aFpLibName )
{
    // Structures to collect component-related rows for second-pass placement
    struct COMP_DATA
    {
        int      layer = 1;
        VECTOR2D pos;
        double   angle = 0;
        std::map<wxString, wxString> attrs;
    };

    struct ATTR_DATA
    {
        wxString parentId;
        int      layer = 3;
        double   x = 0;
        double   y = 0;
        bool     hasPos = false;
        wxString key;
        wxString value;
        bool     keyVisible = false;
        bool     valVisible = false;
        wxString fontName;
        double   fontSize = 45.0;
        double   strokeWidth = 6.0;
        wxString origin;
        double   angle = 0;
        int      inverted = 0;
        int      mirror = 0;
    };

    struct PAD_NET_DATA
    {
        wxString compId;
        wxString padNumber;
        wxString padNet;
    };

    std::map<wxString, COMP_DATA>              componentMap;
    std::multimap<wxString, ATTR_DATA>         attrMap;
    std::multimap<wxString, PAD_NET_DATA>      padNetMap;

    std::multimap<wxString, EASYEDAPRO::POURED> boardPouredMap = aPouredMap;
    std::map<wxString, ZONE*>                   poursToFill;

    BOARD_DESIGN_SETTINGS& bds = aBoard->GetDesignSettings();

    for( const V3_ROW& row : aDoc.rows )
    {
        if( row.type == wxS( "LAYER" ) )
        {
            int          layer = V3GetInt( row.inner, "layerId", 1 );
            PCB_LAYER_ID klayer = m_v2Parser.LayerToKi( layer );

            bool use = V3GetBool( row.inner, "use", true );
            wxString layerName = V3GetString( row.inner, "layerName" );

            if( use )
            {
                LSET blayers = aBoard->GetEnabledLayers();
                blayers.set( klayer );
                aBoard->SetEnabledLayers( blayers );

                if( !layerName.IsEmpty() )
                    aBoard->SetLayerName( klayer, layerName );
            }
        }
        else if( row.type == wxS( "NET" ) )
        {
            nlohmann::json idArr = V3ParseIdArray( row.id );
            wxString netname;

            if( idArr.is_array() && idArr.size() > 1 )
                netname = V3JsonToString( idArr[1] );

            if( !netname.IsEmpty() )
            {
                aBoard->Add( new NETINFO_ITEM( aBoard, netname,
                                               aBoard->GetNetCount() + 1 ),
                             ADD_MODE::APPEND );
            }
        }
        else if( row.type == wxS( "RULE" ) )
        {
            nlohmann::json idArr = V3ParseIdArray( row.id );

            if( !idArr.is_array() || idArr.size() < 3 )
                continue;

            wxString ruleType = V3JsonToString( idArr[1] );
            bool isDefault = V3GetString( row.inner, "ruleState" ) == wxS( "DEFAULT" );

            nlohmann::json context = row.inner.value( "ruleContext", nlohmann::json() );

            if( ruleType == wxS( "TRACK" ) && isDefault )
            {
                nlohmann::json track = context.value( "track", nlohmann::json() );
                nlohmann::json content = track.value( "content", nlohmann::json::array() );

                if( content.is_array() && !content.empty() )
                {
                    double minVal = V3GetDouble( content[0], "stroMin" );
                    bds.m_TrackMinWidth = PCB_IO_EASYEDAPRO_PARSER::ScaleSize( minVal );
                }
            }
            else if( ruleType == wxS( "SAFE" ) && isDefault )
            {
                nlohmann::json safeSpacing = context.value( "safeSpacing",
                                                            nlohmann::json::array() );

                if( safeSpacing.is_array() && !safeSpacing.empty() )
                {
                    nlohmann::json content = safeSpacing[0].value( "content",
                                                                    nlohmann::json::array() );

                    int minVal = INT_MAX;

                    for( const auto& arr : content )
                    {
                        if( arr.is_array() )
                        {
                            for( const auto& val : arr )
                            {
                                int v = val.is_number() ? val.get<int>() : 0;
                                if( v < minVal )
                                    minVal = v;
                            }
                        }
                    }

                    if( minVal != INT_MAX )
                        bds.m_MinClearance = PCB_IO_EASYEDAPRO_PARSER::ScaleSize( minVal );
                }
            }
        }
        else if( row.type == wxS( "VIA" ) )
        {
            VECTOR2D center;
            center.x = V3GetDouble( row.inner, "centerX" );
            center.y = V3GetDouble( row.inner, "centerY" );

            double drill = V3GetDouble( row.inner, "holeDiameter" );
            double dia = V3GetDouble( row.inner, "viaDiameter" );
            wxString netname = V3GetString( row.inner, "netName" );

            std::unique_ptr<PCB_VIA> via = std::make_unique<PCB_VIA>( aBoard );

            via->SetPosition( PCB_IO_EASYEDAPRO_PARSER::ScalePos( center ) );
            via->SetDrill( PCB_IO_EASYEDAPRO_PARSER::ScaleSize( drill ) );
            via->SetWidth( PADSTACK::ALL_LAYERS,
                           PCB_IO_EASYEDAPRO_PARSER::ScaleSize( dia ) );
            via->SetNet( aBoard->FindNet( netname ) );

            aBoard->Add( via.release(), ADD_MODE::APPEND );
        }
        else if( row.type == wxS( "LINE" ) )
        {
            int          layer = V3GetInt( row.inner, "layerId", 1 );
            PCB_LAYER_ID klayer = m_v2Parser.LayerToKi( layer );

            VECTOR2D start;
            start.x = V3GetDouble( row.inner, "startX" );
            start.y = V3GetDouble( row.inner, "startY" );

            VECTOR2D end;
            end.x = V3GetDouble( row.inner, "endX" );
            end.y = V3GetDouble( row.inner, "endY" );

            double   width = V3GetDouble( row.inner, "width" );
            wxString netname = V3GetString( row.inner, "netName" );

            std::unique_ptr<PCB_TRACK> track = std::make_unique<PCB_TRACK>( aBoard );

            track->SetLayer( klayer );
            track->SetStart( PCB_IO_EASYEDAPRO_PARSER::ScalePos( start ) );
            track->SetEnd( PCB_IO_EASYEDAPRO_PARSER::ScalePos( end ) );
            track->SetWidth( PCB_IO_EASYEDAPRO_PARSER::ScaleSize( width ) );
            track->SetNet( aBoard->FindNet( netname ) );

            aBoard->Add( track.release(), ADD_MODE::APPEND );
        }
        else if( row.type == wxS( "ARC" ) )
        {
            int          layer = V3GetInt( row.inner, "layerId", 1 );
            PCB_LAYER_ID klayer = m_v2Parser.LayerToKi( layer );

            VECTOR2D start;
            start.x = V3GetDouble( row.inner, "startX" );
            start.y = V3GetDouble( row.inner, "startY" );

            VECTOR2D end;
            end.x = V3GetDouble( row.inner, "endX" );
            end.y = V3GetDouble( row.inner, "endY" );

            double   angle = V3GetDouble( row.inner, "angle" );
            double   width = V3GetDouble( row.inner, "width" );
            wxString netname = V3GetString( row.inner, "netName" );

            VECTOR2D delta = end - start;
            VECTOR2D mid = ( start + delta / 2 );

            double   ha = angle / 2;
            double   hd = delta.EuclideanNorm() / 2;
            double   cdist = hd / tan( DEG2RAD( ha ) );
            VECTOR2D center = mid + delta.Perpendicular().Resize( cdist );

            SHAPE_ARC sarc;
            sarc.ConstructFromStartEndCenter(
                    PCB_IO_EASYEDAPRO_PARSER::ScalePos( start ),
                    PCB_IO_EASYEDAPRO_PARSER::ScalePos( end ),
                    PCB_IO_EASYEDAPRO_PARSER::ScalePos( center ), angle >= 0, width );

            std::unique_ptr<PCB_ARC> arc = std::make_unique<PCB_ARC>( aBoard, &sarc );
            arc->SetWidth( PCB_IO_EASYEDAPRO_PARSER::ScaleSize( width ) );
            arc->SetLayer( klayer );
            arc->SetNet( aBoard->FindNet( netname ) );

            aBoard->Add( arc.release(), ADD_MODE::APPEND );
        }
        else if( row.type == wxS( "POLY" ) )
        {
            int          layer = V3GetInt( row.inner, "layerId", 1 );
            PCB_LAYER_ID klayer = m_v2Parser.LayerToKi( layer );

            double         thickness = V3GetDouble( row.inner, "width" );
            nlohmann::json polyData = row.inner.value( "path", nlohmann::json::array() );

            std::vector<std::unique_ptr<PCB_SHAPE>> results =
                    m_v2Parser.ParsePoly( aBoard, polyData, false, false );

            for( auto& shape : results )
            {
                shape->SetLayer( klayer );
                shape->SetWidth( PCB_IO_EASYEDAPRO_PARSER::ScaleSize( thickness ) );

                aBoard->Add( shape.release(), ADD_MODE::APPEND );
            }
        }
        else if( row.type == wxS( "FILL" ) )
        {
            int          layer = V3GetInt( row.inner, "layerId", 1 );
            PCB_LAYER_ID klayer = m_v2Parser.LayerToKi( layer );

            wxString netname = V3GetString( row.inner, "netName" );

            nlohmann::json polyDataList = row.inner.value( "path", nlohmann::json::array() );

            if( polyDataList.is_null() || polyDataList.empty() )
                continue;

            if( !polyDataList.at( 0 ).is_array() )
                polyDataList = nlohmann::json::array( { polyDataList } );

            std::vector<SHAPE_LINE_CHAIN> contours;

            for( nlohmann::json& polyData : polyDataList )
            {
                SHAPE_LINE_CHAIN contour = m_v2Parser.ParseContour( polyData, true );
                contour.SetClosed( true );
                contours.push_back( contour );
            }

            SHAPE_POLY_SET zoneFillPoly;

            for( SHAPE_LINE_CHAIN& contour : contours )
                zoneFillPoly.AddOutline( contour );

            zoneFillPoly.RebuildHolesFromContours();
            zoneFillPoly.Fracture();

            std::unique_ptr<ZONE> zone = std::make_unique<ZONE>( aBoard );

            zone->SetNet( aBoard->FindNet( netname ) );
            zone->SetLayer( klayer );
            zone->Outline()->Append( SHAPE_RECT( zoneFillPoly.BBox() ).Outline() );
            zone->SetFilledPolysList( klayer, zoneFillPoly );
            zone->SetAssignedPriority( 500 );
            zone->SetIsFilled( true );
            zone->SetNeedRefill( false );

            zone->SetLocalClearance( bds.m_MinClearance );
            zone->SetMinThickness( bds.m_TrackMinWidth );

            aBoard->Add( zone.release(), ADD_MODE::APPEND );
        }
        else if( row.type == wxS( "POUR" ) )
        {
            int          layer = V3GetInt( row.inner, "layerId", 1 );
            PCB_LAYER_ID klayer = m_v2Parser.LayerToKi( layer );

            wxString netname = V3GetString( row.inner, "netName" );
            int      fillOrder = V3GetInt( row.inner, "order" );

            nlohmann::json polyDataList = row.inner.value( "path", nlohmann::json::array() );

            std::unique_ptr<ZONE> zone = std::make_unique<ZONE>( aBoard );

            zone->SetNet( aBoard->FindNet( netname ) );
            zone->SetLayer( klayer );
            zone->SetAssignedPriority( 500 - fillOrder );
            zone->SetLocalClearance( bds.m_MinClearance );
            zone->SetMinThickness( bds.m_TrackMinWidth );

            for( nlohmann::json& polyData : polyDataList )
            {
                SHAPE_LINE_CHAIN contour = m_v2Parser.ParseContour( polyData, false );
                contour.SetClosed( true );
                zone->Outline()->Append( contour );
            }

            wxASSERT( zone->Outline()->OutlineCount() == 1 );

            poursToFill.emplace( row.id, zone.get() );

            aBoard->Add( zone.release(), ADD_MODE::APPEND );
        }
        else if( row.type == wxS( "POURED" ) )
        {
            nlohmann::json idArr = V3ParseIdArray( row.id );
            wxString parentId;

            if( idArr.is_array() && idArr.size() > 1 )
                parentId = V3JsonToString( idArr[1] );

            nlohmann::json fills = row.inner.value( "pourFill", nlohmann::json::array() );

            if( !fills.is_array() )
                continue;

            for( const nlohmann::json& fill : fills )
            {
                EASYEDAPRO::POURED poured;
                poured.pouredId = V3JsonToString( fill.value( "id",
                                                              nlohmann::json( row.id ) ) );
                poured.parentId = parentId;
                poured.unki = V3GetInt( fill, "strokeWidth" );
                poured.isPoly = V3GetBool( fill, "fill" );
                poured.polyData = fill.value( "path", nlohmann::json::array() );

                boardPouredMap.emplace( poured.parentId, poured );
            }
        }
        else if( row.type == wxS( "REGION" ) )
        {
            int          layer = V3GetInt( row.inner, "layerId", 1 );
            PCB_LAYER_ID klayer = m_v2Parser.LayerToKi( layer );

            nlohmann::json polyDataList = row.inner.value( "path", nlohmann::json::array() );
            nlohmann::json prohibitTypes = row.inner.value( "prohibitType",
                                                            nlohmann::json::array() );

            std::set<int> flags;

            if( prohibitTypes.is_array() )
            {
                for( const auto& pt : prohibitTypes )
                {
                    wxString ptStr;

                    if( pt.is_string() )
                        ptStr = pt.get<std::string>();

                    if( ptStr == wxS( "COMPONENT" ) )
                        flags.insert( 2 );
                    else if( ptStr == wxS( "TRACK" ) )
                        flags.insert( 5 );
                    else if( ptStr == wxS( "COPPER" ) )
                        flags.insert( 6 );
                    else if( ptStr == wxS( "VIA" ) )
                        flags.insert( 7 );
                    else if( ptStr == wxS( "FILL" ) || ptStr == wxS( "PLANE" ) )
                        flags.insert( 8 );
                }
            }

            for( nlohmann::json& polyData : polyDataList )
            {
                SHAPE_LINE_CHAIN contour = m_v2Parser.ParseContour( polyData, false );
                contour.SetClosed( true );

                std::unique_ptr<ZONE> zone = std::make_unique<ZONE>( aBoard );

                zone->SetIsRuleArea( true );
                zone->SetDoNotAllowFootprints( !!flags.count( 2 ) );
                zone->SetDoNotAllowZoneFills( !!flags.count( 7 ) || !!flags.count( 6 )
                                              || !!flags.count( 8 ) );
                zone->SetDoNotAllowPads( !!flags.count( 7 ) );
                zone->SetDoNotAllowTracks( !!flags.count( 7 ) || !!flags.count( 5 ) );
                zone->SetDoNotAllowVias( !!flags.count( 7 ) );

                zone->SetLayer( klayer );
                zone->Outline()->Append( contour );

                aBoard->Add( zone.release(), ADD_MODE::APPEND );
            }
        }
        else if( row.type == wxS( "STRING" ) )
        {
            int          layer = V3GetInt( row.inner, "layerId", 3 );
            PCB_LAYER_ID klayer = m_v2Parser.LayerToKi( layer );

            VECTOR2D location;
            location.x = V3GetDouble( row.inner, "x" );
            location.y = V3GetDouble( row.inner, "y" );

            wxString string = V3GetString( row.inner, "text" );
            wxString font = V3GetString( row.inner, "fontFamily", wxS( "default" ) );

            double height = V3GetDouble( row.inner, "fontSize", 45.0 );
            double strokew = V3GetDouble( row.inner, "strokeWidth", 6.0 );

            wxString originStr = V3GetString( row.inner, "origin", wxS( "LEFT_BOTTOM" ) );
            int    align = V3AlignToOriginCode( originStr );
            double angle = V3GetDouble( row.inner, "angle" );
            int    inverted = V3GetBool( row.inner, "reverse" ) ? 1 : 0;
            int    mirror = V3GetBool( row.inner, "mirror" ) ? 1 : 0;

            PCB_TEXT* text = new PCB_TEXT( aBoard );

            text->SetText( string );
            text->SetLayer( klayer );
            text->SetPosition( PCB_IO_EASYEDAPRO_PARSER::ScalePos( location ) );
            text->SetIsKnockout( inverted );
            text->SetTextThickness( PCB_IO_EASYEDAPRO_PARSER::ScaleSize( strokew ) );
            text->SetTextSize( VECTOR2D(
                    PCB_IO_EASYEDAPRO_PARSER::ScaleSize( height * 0.6 ),
                    PCB_IO_EASYEDAPRO_PARSER::ScaleSize( height * 0.7 ) ) );

            if( font != wxS( "default" ) )
                text->SetFont( KIFONT::FONT::GetFont( font ) );

            V3AlignText( text, align );

            if( IsBackLayer( klayer ) ^ !!mirror )
            {
                text->SetMirrored( true );
                text->SetTextAngleDegrees( -angle );
            }
            else
            {
                text->SetTextAngleDegrees( angle );
            }

            aBoard->Add( text, ADD_MODE::APPEND );
        }
        else if( row.type == wxS( "COMPONENT" ) )
        {
            COMP_DATA comp;
            comp.layer = V3GetInt( row.inner, "layerId", 1 );
            comp.pos.x = V3GetDouble( row.inner, "x" );
            comp.pos.y = V3GetDouble( row.inner, "y" );
            comp.angle = V3GetDouble( row.inner, "angle" );

            nlohmann::json attrsObj = row.inner.value( "attrs", nlohmann::json::object() );

            if( attrsObj.is_object() )
            {
                for( auto& [k, v] : attrsObj.items() )
                    comp.attrs[wxString( k )] = V3JsonToString( v );
            }

            componentMap[row.id] = comp;
        }
        else if( row.type == wxS( "ATTR" ) )
        {
            ATTR_DATA attr;
            attr.parentId = V3GetString( row.inner, "parentId" );
            attr.layer = V3GetInt( row.inner, "layerId", 3 );
            attr.hasPos = !V3IsNullOrMissing( row.inner, "x" );

            if( attr.hasPos )
            {
                attr.x = V3GetDouble( row.inner, "x" );
                attr.y = V3GetDouble( row.inner, "y" );
            }

            attr.key = V3GetString( row.inner, "key" );
            attr.value = V3JsonToString( row.inner.value( "value", nlohmann::json() ) );
            attr.keyVisible = V3GetBool( row.inner, "keyVisible" );
            attr.valVisible = V3GetBool( row.inner, "valueVisible" );
            attr.fontName = V3GetString( row.inner, "fontFamily", wxS( "default" ) );
            attr.fontSize = V3GetDouble( row.inner, "fontSize", 45.0 );
            attr.strokeWidth = V3GetDouble( row.inner, "strokeWidth", 6.0 );
            attr.origin = V3GetString( row.inner, "origin", wxS( "LEFT_BOTTOM" ) );
            attr.angle = V3GetDouble( row.inner, "angle" );
            attr.inverted = V3GetBool( row.inner, "reverse" ) ? 1 : 0;
            attr.mirror = V3GetBool( row.inner, "mirror" ) ? 1 : 0;

            attrMap.emplace( attr.parentId, attr );
        }
        else if( row.type == wxS( "PAD_NET" ) )
        {
            nlohmann::json idArr = V3ParseIdArray( row.id );

            PAD_NET_DATA pn;
            pn.compId = idArr.is_array() && idArr.size() > 1
                                ? V3JsonToString( idArr[1] )
                                : wxString();
            pn.padNumber = idArr.is_array() && idArr.size() > 2
                                   ? V3JsonToString( idArr[2] )
                                   : wxString();
            pn.padNet = V3GetString( row.inner, "padNet" );

            if( !pn.compId.IsEmpty() )
                padNetMap.emplace( pn.compId, pn );
        }
        else if( row.type == wxS( "PAD" ) )
        {
            wxString netname = V3GetString( row.inner, "netName" );

            std::unique_ptr<FOOTPRINT> footprint =
                    std::make_unique<FOOTPRINT>( aBoard );
            std::unique_ptr<PAD> pad = createV3PAD( footprint.get(), row );

            pad->SetNet( aBoard->FindNet( netname ) );

            VECTOR2I  pos = pad->GetPosition();
            EDA_ANGLE orient = pad->GetOrientation();

            pad->SetPosition( VECTOR2I() );
            pad->SetOrientation( ANGLE_0 );

            footprint->Add( pad.release(), ADD_MODE::APPEND );
            footprint->SetPosition( pos );
            footprint->SetOrientation( orient );

            wxString fpName = wxS( "Pad_" ) + row.id;
            LIB_ID   fpID = EASYEDAPRO::ToKiCadLibID( wxEmptyString, fpName );

            footprint->SetFPID( fpID );
            footprint->Reference().SetVisible( true );
            footprint->Value().SetVisible( true );
            footprint->AutoPositionFields();

            aBoard->Add( footprint.release(), ADD_MODE::APPEND );
        }
    } // end first pass

    // Second pass: place components
    for( auto const& [compId, comp] : componentMap )
    {
        wxString deviceId;
        wxString fpIdOverride;
        wxString fpDesignator;

        // Collect attrs for this component
        auto attrRange = attrMap.equal_range( compId );

        for( auto it = attrRange.first; it != attrRange.second; ++it )
        {
            const ATTR_DATA& attr = it->second;

            if( attr.key == wxS( "Device" ) )
                deviceId = attr.value;
            else if( attr.key == wxS( "Footprint" ) )
                fpIdOverride = attr.value;
            else if( attr.key == wxS( "Designator" ) )
                fpDesignator = attr.value;
        }

        wxString fpId;

        if( !fpIdOverride.IsEmpty() )
        {
            fpId = fpIdOverride;
        }
        else
        {
            std::string deviceIdUtf8 = std::string( deviceId.ToUTF8() );

            if( !deviceId.empty()
                && aProject.contains( "devices" )
                && aProject.at( "devices" ).is_object()
                && aProject.at( "devices" ).contains( deviceIdUtf8 ) )
            {
                const nlohmann::json& dev = aProject.at( "devices" ).at( deviceIdUtf8 );

                if( dev.contains( "attributes" ) && dev.at( "attributes" ).is_object()
                    && dev.at( "attributes" ).contains( "Footprint" ) )
                {
                    fpId = V3JsonToString( dev.at( "attributes" ).at( "Footprint" ) );
                }
            }
        }

        if( fpId.empty() )
        {
            wxLogWarning( wxString::Format(
                    _( "EasyEDA Pro v3 component '%s' (%s): no footprint mapping, skipping." ),
                    compId, fpDesignator ) );
            continue;
        }

        auto it = aFootprintMap.find( fpId );

        if( it == aFootprintMap.end() )
        {
            wxLogError( "Footprint of '%s' with uuid '%s' not found.", fpDesignator, fpId );
            continue;
        }

        std::unique_ptr<FOOTPRINT>& footprintOrig = it->second;
        std::unique_ptr<FOOTPRINT>  footprint(
                static_cast<FOOTPRINT*>( footprintOrig->Clone() ) );

        footprint->SetParent( aBoard );

        // Apply position, rotation, flip
        PCB_LAYER_ID klayer = m_v2Parser.LayerToKi( comp.layer );

        if( klayer == B_Cu )
            footprint->Flip( footprint->GetPosition(), FLIP_DIRECTION::TOP_BOTTOM );

        footprint->SetOrientationDegrees( comp.angle );
        footprint->SetPosition( PCB_IO_EASYEDAPRO_PARSER::ScalePos( comp.pos ) );

        // Apply attributes (designator, etc.)
        for( auto attrIt = attrRange.first; attrIt != attrRange.second; ++attrIt )
        {
            const ATTR_DATA& attr = attrIt->second;

            if( attr.key == wxS( "Designator" ) )
            {
                PCB_FIELD* field = footprint->GetField( FIELD_T::REFERENCE );

                if( attr.fontName != wxS( "default" ) )
                    field->SetFont( KIFONT::FONT::GetFont( attr.fontName ) );

                if( attr.valVisible && attr.keyVisible )
                    field->SetText( attr.key + ':' + attr.value );
                else if( attr.keyVisible )
                    field->SetText( attr.key );
                else
                {
                    field->SetVisible( false );
                    field->SetText( attr.value );
                }

                PCB_LAYER_ID attrLayer = m_v2Parser.LayerToKi( attr.layer );
                field->SetLayer( attrLayer );

                if( attr.hasPos )
                {
                    field->SetPosition( PCB_IO_EASYEDAPRO_PARSER::ScalePos(
                            VECTOR2D( attr.x, attr.y ) ) );
                }

                field->SetTextAngleDegrees( footprint->IsFlipped() ? -attr.angle
                                                                    : attr.angle );
                field->SetIsKnockout( attr.inverted );
                field->SetTextThickness(
                        PCB_IO_EASYEDAPRO_PARSER::ScaleSize( attr.strokeWidth ) );
                field->SetTextSize( VECTOR2D(
                        PCB_IO_EASYEDAPRO_PARSER::ScaleSize( attr.fontSize * 0.55 ),
                        PCB_IO_EASYEDAPRO_PARSER::ScaleSize( attr.fontSize * 0.6 ) ) );

                int alignCode = V3AlignToOriginCode( attr.origin );
                V3AlignText( field, alignCode );
            }
        }

        // Apply pad nets
        auto padNetRange = padNetMap.equal_range( compId );

        for( auto pnIt = padNetRange.first; pnIt != padNetRange.second; ++pnIt )
        {
            const PAD_NET_DATA& pn = pnIt->second;

            PAD* pad = footprint->FindPadByNumber( pn.padNumber );

            if( pad )
                pad->SetNet( aBoard->FindNet( pn.padNet ) );
        }

        aBoard->Add( footprint.release(), ADD_MODE::APPEND );
    }

    // Set zone fills from POURED data
    if( EASYEDAPRO::IMPORT_POURED )
    {
        for( auto& [uuid, zone] : poursToFill )
        {
            SHAPE_POLY_SET fillPolySet;
            SHAPE_POLY_SET thermalSpokes;

            auto range = boardPouredMap.equal_range( uuid );

            for( auto& pIt = range.first; pIt != range.second; ++pIt )
            {
                const EASYEDAPRO::POURED& poured = pIt->second;

                SHAPE_POLY_SET thisPoly;

                for( int dataId = 0;
                     dataId < static_cast<int>( poured.polyData.size() ); dataId++ )
                {
                    const nlohmann::json& fillData = poured.polyData[dataId];
                    const double          ptScale = 10;

                    SHAPE_LINE_CHAIN contour = m_v2Parser.ParseContour(
                            fillData, false, ARC_HIGH_DEF / ptScale );

                    // Scale the fill
                    for( int i = 0; i < contour.PointCount(); i++ )
                        contour.SetPoint( i, contour.GetPoint( i ) * ptScale );

                    if( poured.isPoly )
                    {
                        contour.SetClosed( true );

                        // The contour can be self-intersecting
                        SHAPE_POLY_SET simple( contour );
                        simple.Simplify();

                        if( dataId == 0 )
                        {
                            thisPoly.Append( simple );
                        }
                        else
                        {
                            thisPoly.BooleanSubtract( simple );
                        }
                    }
                    else
                    {
                        const int thermalWidth = pcbIUScale.mmToIU( 0.2 );

                        for( int segId = 0; segId < contour.SegmentCount(); segId++ )
                        {
                            const SEG& seg = contour.CSegment( segId );

                            TransformOvalToPolygon( thermalSpokes, seg.A, seg.B,
                                                    thermalWidth, ARC_HIGH_DEF,
                                                    ERROR_INSIDE );
                        }
                    }
                }

                fillPolySet.Append( thisPoly );
            }

            if( !fillPolySet.IsEmpty() )
            {
                fillPolySet.Simplify();

                const int strokeWidth = pcbIUScale.MilsToIU( 8 );

                fillPolySet.Inflate( strokeWidth / 2,
                                     CORNER_STRATEGY::ROUND_ALL_CORNERS,
                                     ARC_HIGH_DEF, false );

                fillPolySet.BooleanAdd( thermalSpokes );
                fillPolySet.Fracture();

                zone->SetFilledPolysList( zone->GetFirstLayer(), fillPolySet );
                zone->SetNeedRefill( false );
                zone->SetIsFilled( true );
            }
        }
    }

    // Heal board outlines
    std::vector<PCB_SHAPE*> shapes;

    for( BOARD_ITEM* item : aBoard->Drawings() )
    {
        if( !item->IsOnLayer( Edge_Cuts ) )
            continue;

        if( item->Type() == PCB_SHAPE_T )
            shapes.push_back( static_cast<PCB_SHAPE*>( item ) );
    }

    ConnectBoardShapes( shapes, SHAPE_JOIN_DISTANCE );

    // Center the board
    BOX2I     outlineBbox = aBoard->ComputeBoundingBox( true );
    PAGE_INFO pageInfo = aBoard->GetPageSettings();

    VECTOR2D pageCenter( pcbIUScale.MilsToIU( pageInfo.GetWidthMils() / 2 ),
                         pcbIUScale.MilsToIU( pageInfo.GetHeightMils() / 2 ) );

    VECTOR2D offset = pageCenter - outlineBbox.GetCenter();

    int alignGrid = pcbIUScale.mmToIU( 10 );
    offset.x = KiROUND( offset.x / alignGrid ) * alignGrid;
    offset.y = KiROUND( offset.y / alignGrid ) * alignGrid;

    aBoard->Move( offset );
    bds.SetAuxOrigin( offset );
}
