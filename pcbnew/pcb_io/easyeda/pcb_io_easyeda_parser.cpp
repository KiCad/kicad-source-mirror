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

#include "pcb_io_easyeda_parser.h"

#include <memory>

#include <json_common.h>
#include <core/map_helpers.h>
#include <core/json_serializers.h>
#include <string_utils.h>

#include <wx/log.h>

#include <font/font.h>
#include <footprint.h>
#include <progress_reporter.h>
#include <board.h>
#include <board_design_settings.h>
#include <bezier_curves.h>
#include <pcb_group.h>
#include <pcb_track.h>
#include <pcb_shape.h>
#include <pcb_text.h>
#include <zone.h>
#include <pad.h>
#include <project.h>
#include <fix_board_shape.h>


static const wxString DIRECT_MODEL_UUID_KEY = wxS( "JLC_3DModel" );
static const wxString MODEL_SIZE_KEY = wxS( "JLC_3D_Size" );

static const int      SHAPE_JOIN_DISTANCE = pcbIUScale.mmToIU( 1.5 );
static const VECTOR2I HIDDEN_TEXT_SIZE( pcbIUScale.mmToIU( 0.5 ), pcbIUScale.mmToIU( 0.5 ) );


PCB_IO_EASYEDA_PARSER::PCB_IO_EASYEDA_PARSER( PROGRESS_REPORTER* aProgressReporter )
{
}

PCB_IO_EASYEDA_PARSER::~PCB_IO_EASYEDA_PARSER()
{
}

PCB_LAYER_ID PCB_IO_EASYEDA_PARSER::LayerToKi( const wxString& aLayer )
{
    int elayer = wxAtoi( aLayer );

    switch( elayer )
    {
    case 1: return F_Cu;
    case 2: return B_Cu;
    case 3: return F_SilkS;
    case 4: return B_SilkS;
    case 5: return F_Paste;
    case 6: return B_Paste;
    case 7: return F_Mask;
    case 8: return B_Mask;
    /*case 9: return UNDEFINED_LAYER;*/ // Ratsnest
    case 10: return Edge_Cuts;
    case 11: return Eco1_User;
    case 12: return Dwgs_User;
    case 13: return F_Fab;
    case 14: return B_Fab;
    case 15: return Eco2_User;

    case 19: return F_Fab; // 3D model

    case 21: return In1_Cu;
    case 22: return In2_Cu;
    case 23: return In3_Cu;
    case 24: return In4_Cu;
    case 25: return In5_Cu;
    case 26: return In6_Cu;
    case 27: return In7_Cu;
    case 28: return In8_Cu;
    case 29: return In9_Cu;
    case 30: return In10_Cu;
    case 31: return In11_Cu;
    case 32: return In12_Cu;
    case 33: return In13_Cu;
    case 34: return In14_Cu;
    case 35: return In15_Cu;
    case 36: return In16_Cu;
    case 37: return In17_Cu;
    case 38: return In18_Cu;
    case 39: return In19_Cu;
    case 40: return In20_Cu;
    case 41: return In21_Cu;
    case 42: return In22_Cu;
    case 43: return In23_Cu;
    case 44: return In24_Cu;
    case 45: return In25_Cu;
    case 46: return In26_Cu;
    case 47: return In27_Cu;
    case 48: return In28_Cu;
    case 49: return In29_Cu;
    case 50: return In30_Cu;

    case 99: return User_3;
    case 100: return User_4;
    case 101: return User_5;

    default: break;
    }

    return User_1;
}


static LIB_ID EasyEdaToKiCadLibID( const wxString& aLibName, const wxString& aLibReference )
{
    wxString libReference = EscapeString( aLibReference, CTX_LIBID );

    wxString key = !aLibName.empty() ? ( aLibName + ':' + libReference ) : libReference;

    LIB_ID libId;
    libId.Parse( key, true );

    return libId;
}


void PCB_IO_EASYEDA_PARSER::ParseToBoardItemContainer(
        BOARD_ITEM_CONTAINER* aContainer, BOARD* aParent, std::map<wxString, wxString> paramMap,
        std::map<wxString, std::unique_ptr<FOOTPRINT>>& aFootprintMap, wxArrayString aShapes )
{
    // TODO: make this path configurable?
    const wxString easyedaModelDir = wxS( "EASYEDA_MODELS" );
    wxString       kicadModelPrefix = wxS( "${KIPRJMOD}/" ) + easyedaModelDir + wxS( "/" );

    BOARD*     board = aParent ? aParent : dynamic_cast<BOARD*>( aContainer );
    FOOTPRINT* footprint = dynamic_cast<FOOTPRINT*>( aContainer );

    auto getOrAddNetItem = [&]( const wxString& aNetName ) -> NETINFO_ITEM*
    {
        if( !board )
            return nullptr;

        if( aNetName.empty() )
            return nullptr;

        if( NETINFO_ITEM* item = board->FindNet( aNetName ) )
        {
            return item;
        }
        else
        {
            item = new NETINFO_ITEM( board, aNetName, board->GetNetCount() + 1 );
            board->Add( item, ADD_MODE::APPEND );
            return item;
        }
    };

    if( footprint )
    {
        // TODO: library name
        LIB_ID fpID = EasyEdaToKiCadLibID( wxEmptyString, paramMap[wxS( "package" )] );
        footprint->SetFPID( fpID );
    }

    for( wxString shape : aShapes )
    {
        wxArrayString arr = wxSplit( shape, '~', '\0' );

        wxString elType = arr[0];
        if( elType == wxS( "LIB" ) )
        {
            shape.Replace( wxS( "#@$" ), "\n" );
            wxArrayString parts = wxSplit( shape, '\n', '\0' );

            if( parts.size() < 1 )
                continue;

            wxArrayString paramsRoot = wxSplit( parts[0], '~', '\0' );

            if( paramsRoot.size() < 4 )
                continue;

            VECTOR2D fpOrigin( Convert( paramsRoot[1] ), Convert( paramsRoot[2] ) );

            wxString packageName =
                    wxString::Format( wxS( "Unknown_%s_%s" ), paramsRoot[1], paramsRoot[2] );

            wxArrayString paramParts = wxSplit( paramsRoot[3], '`', '\0' );

            EDA_ANGLE orientation;
            if( !paramsRoot[4].IsEmpty() )
                orientation = EDA_ANGLE( Convert( paramsRoot[4] ), DEGREES_T ); // Already applied

            int layer = 1;

            if( !paramsRoot[7].IsEmpty() )
                layer = Convert( paramsRoot[7] );

            std::map<wxString, wxString> paramMap;

            for( int i = 1; i < paramParts.size(); i += 2 )
            {
                wxString key = paramParts[i - 1];
                wxString value = paramParts[i];

                if( key == wxS( "package" ) )
                    packageName = value;

                paramMap[key] = value;
            }

            parts.RemoveAt( 0 );

            VECTOR2D   pcbOrigin = m_relOrigin;
            FOOTPRINT* fp = ParseFootprint( fpOrigin, orientation, layer, board, paramMap,
                                            aFootprintMap, parts );

            if( !fp )
                continue;

            m_relOrigin = pcbOrigin;

            fp->Move( RelPos( fpOrigin ) );

            aContainer->Add( fp, ADD_MODE::APPEND );
        }
        else if( elType == wxS( "TRACK" ) )
        {
            double        width = ConvertSize( arr[1] );
            PCB_LAYER_ID  layer = LayerToKi( arr[2] );
            wxString      netname = arr[3];
            wxArrayString data = wxSplit( arr[4], ' ', '\0' );

            for( int i = 3; i < data.size(); i += 2 )
            {
                VECTOR2D start, end;
                start.x = RelPosX( data[i - 3] );
                start.y = RelPosY( data[i - 2] );
                end.x = RelPosX( data[i - 1] );
                end.y = RelPosY( data[i] );

                if( !footprint && IsCopperLayer( layer ) )
                {
                    std::unique_ptr<PCB_TRACK> track = std::make_unique<PCB_TRACK>( aContainer );

                    track->SetLayer( layer );
                    track->SetWidth( width );
                    track->SetStart( start );
                    track->SetEnd( end );
                    track->SetNet( getOrAddNetItem( netname ) );

                    aContainer->Add( track.release(), ADD_MODE::APPEND );
                }
                else
                {
                    std::unique_ptr<PCB_SHAPE> seg =
                            std::make_unique<PCB_SHAPE>( aContainer, SHAPE_T::SEGMENT );

                    seg->SetLayer( layer );
                    seg->SetWidth( width );
                    seg->SetStart( start );
                    seg->SetEnd( end );

                    aContainer->Add( seg.release(), ADD_MODE::APPEND );
                }
            }
        }
        else if( elType == wxS( "CIRCLE" ) )
        {
            auto   shape = std::make_unique<PCB_SHAPE>( aContainer, SHAPE_T::CIRCLE );
            double width = ConvertSize( arr[4] );
            shape->SetWidth( width );

            PCB_LAYER_ID layer = LayerToKi( arr[5] );
            shape->SetLayer( layer );

            VECTOR2D center;
            center.x = RelPosX( arr[1] );
            center.y = RelPosY( arr[2] );

            double radius = ConvertSize( arr[3] );

            shape->SetCenter( center );
            shape->SetEnd( center + VECTOR2I( radius, 0 ) );

            if( IsCopperLayer( layer ) )
                shape->SetNet( getOrAddNetItem( arr[8] ) );

            aContainer->Add( shape.release(), ADD_MODE::APPEND );
        }
        else if( elType == wxS( "RECT" ) )
        {
            auto   shape = std::make_unique<PCB_SHAPE>( aContainer, SHAPE_T::RECTANGLE );
            double width = ConvertSize( arr[8] );
            shape->SetWidth( width );

            PCB_LAYER_ID layer = LayerToKi( arr[5] );
            shape->SetLayer( layer );

            bool filled = arr[9] != wxS( "none" );
            shape->SetFilled( filled );

            VECTOR2D start;
            start.x = RelPosX( arr[1] );
            start.y = RelPosY( arr[2] );

            VECTOR2D size;
            size.x = ConvertSize( arr[3] );
            size.y = ConvertSize( arr[4] );

            shape->SetStart( start );
            shape->SetEnd( start + size );

            if( IsCopperLayer( layer ) )
                shape->SetNet( getOrAddNetItem( arr[11] ) );

            aContainer->Add( shape.release(), ADD_MODE::APPEND );
        }
        else if( elType == wxS( "ARC" ) )
        {
            std::unique_ptr<PCB_SHAPE> shape =
                    std::make_unique<PCB_SHAPE>( aContainer, SHAPE_T::ARC );

            double width = ConvertSize( arr[1] );
            shape->SetWidth( width );

            PCB_LAYER_ID layer = LayerToKi( arr[2] );
            shape->SetLayer( layer );

            if( IsCopperLayer( layer ) )
                shape->SetNet( getOrAddNetItem( arr[3] ) );

            VECTOR2D start, end;
            VECTOR2D rad( 10, 10 );
            bool     isFar = false;
            bool     cw = false;

            int      pos = 0;
            wxString data = arr[4];
            auto     readNumber = [&]( wxString& aOut )
            {
                wxUniChar ch = data[pos];

                while( ch == ' ' || ch == ',' )
                    ch = data[++pos];

                while( isdigit( ch ) || ch == '.' || ch == '-' )
                {
                    aOut += ch;
                    pos++;

                    if( pos == data.size() )
                        break;

                    ch = data[pos];
                }
            };

            do
            {
                wxUniChar sym = data[pos++];

                if( sym == 'M' )
                {
                    wxString xStr, yStr;
                    readNumber( xStr );
                    readNumber( yStr );

                    start = VECTOR2D( Convert( xStr ), Convert( yStr ) );
                }
                else if( sym == 'A' )
                {
                    wxString radX, radY, unknown, farFlag, cwFlag, endX, endY;
                    readNumber( radX );
                    readNumber( radY );
                    readNumber( unknown );
                    readNumber( farFlag );
                    readNumber( cwFlag );
                    readNumber( endX );
                    readNumber( endY );

                    isFar = farFlag == wxS( "1" );
                    cw = cwFlag == wxS( "1" );
                    rad = VECTOR2D( Convert( radX ), Convert( radY ) );
                    end = VECTOR2D( Convert( endX ), Convert( endY ) );
                }
            } while( pos < data.size() );

            VECTOR2D delta = end - start;

            double d = delta.EuclideanNorm();
            double h = sqrt( std::max( 0.0, rad.x * rad.x - d * d / 4 ) );

            //( !far && cw ) => h
            //( far && cw ) => -h
            //( !far && !cw ) => -h
            //( far && !cw ) => h
            VECTOR2D arcCenter =
                    start + delta / 2 + delta.Perpendicular().Resize( ( isFar ^ cw ) ? h : -h );

            if( !cw )
                std::swap( start, end );

            shape->SetStart( RelPos( start ) );
            shape->SetEnd( RelPos( end ) );
            shape->SetCenter( RelPos( arcCenter ) );

            aContainer->Add( shape.release(), ADD_MODE::APPEND );
        }
        else if( elType == wxS( "DIMENSION" ) )
        {
            PCB_LAYER_ID layer = LayerToKi( arr[1] );
            double       lineWidth = ConvertSize( arr[7] );
            wxString     shapeData = arr[2].Trim();
            //double       textHeight = !arr[4].IsEmpty() ? ConvertSize( arr[4] ) : 0;

            std::vector<SHAPE_LINE_CHAIN> lineChains =
                    ParseLineChains( shapeData, SHAPE_ARC::DefaultAccuracyForPCB(), false );

            std::unique_ptr<PCB_GROUP> group = std::make_unique<PCB_GROUP>( aContainer );
            group->SetName( wxS( "Dimension" ) );

            for( const SHAPE_LINE_CHAIN& chain : lineChains )
            {
                for( int segId = 0; segId < chain.SegmentCount(); segId++ )
                {
                    SEG  seg = chain.CSegment( segId );
                    auto shape = std::make_unique<PCB_SHAPE>( aContainer, SHAPE_T::SEGMENT );

                    shape->SetLayer( layer );
                    shape->SetWidth( lineWidth );
                    shape->SetStart( seg.A );
                    shape->SetEnd( seg.B );

                    group->AddItem( shape.get() );

                    aContainer->Add( shape.release(), ADD_MODE::APPEND );
                }
            }

            aContainer->Add( group.release(), ADD_MODE::APPEND );
        }
        else if( elType == wxS( "SOLIDREGION" ) )
        {
            wxString layer = arr[1];

            SHAPE_POLY_SET polySet =
                    ParseLineChains( arr[3].Trim(), SHAPE_ARC::DefaultAccuracyForPCB(), true );

            if( layer == wxS( "11" ) ) // Multi-layer (board cutout)
            {
                for( const SHAPE_POLY_SET::POLYGON& poly : polySet.CPolygons() )
                {
                    auto shape = std::make_unique<PCB_SHAPE>( aContainer, SHAPE_T::POLY );

                    shape->SetLayer( Edge_Cuts );
                    shape->SetFilled( false );
                    shape->SetWidth( pcbIUScale.mmToIU( 0.1 ) );
                    shape->SetPolyShape( poly );

                    aContainer->Add( shape.release(), ADD_MODE::APPEND );
                }
            }
            else
            {
                std::unique_ptr<ZONE> zone = std::make_unique<ZONE>( aContainer );

                PCB_LAYER_ID klayer = LayerToKi( layer );
                zone->SetLayer( klayer );

                if( IsCopperLayer( klayer ) )
                    zone->SetNet( getOrAddNetItem( arr[2] ) );

                for( const SHAPE_POLY_SET::POLYGON& poly : polySet.CPolygons() )
                    zone->Outline()->AddPolygon( poly );

                if( arr[4].Lower() == wxS( "cutout" ) )
                {
                    zone->SetIsRuleArea( true );
                    zone->SetDoNotAllowZoneFills( true );
                    zone->SetDoNotAllowTracks( false );
                    zone->SetDoNotAllowVias( false );
                    zone->SetDoNotAllowPads( false );
                    zone->SetDoNotAllowFootprints( false );
                }
                else
                { // solid
                    zone->SetFilledPolysList( klayer, polySet );
                    zone->SetPadConnection( ZONE_CONNECTION::FULL );
                    zone->SetIsFilled( true );
                    zone->SetNeedRefill( false );
                }

                zone->SetMinThickness( 0 );
                zone->SetLocalClearance( 0 );
                zone->SetAssignedPriority( 100 );

                aContainer->Add( zone.release(), ADD_MODE::APPEND );
            }
        }
        else if( elType == wxS( "COPPERAREA" ) )
        {
            std::unique_ptr<ZONE> zone = std::make_unique<ZONE>( aContainer );

            PCB_LAYER_ID layer = LayerToKi( arr[2] );
            zone->SetLayer( layer );

            wxString netname = arr[3];

            if( IsCopperLayer( layer ) )
                zone->SetNet( getOrAddNetItem( netname ) );

            zone->SetLocalClearance( ConvertSize( arr[5] ) );
            zone->SetThermalReliefGap( zone->GetLocalClearance().value() );

            wxString fillStyle = arr[5];
            if( fillStyle == wxS( "none" ) )
            {
                // Do not fill?
            }

            SHAPE_POLY_SET polySet =
                    ParseLineChains( arr[4].Trim(), SHAPE_ARC::DefaultAccuracyForPCB(), true );

            for( const SHAPE_POLY_SET::POLYGON& poly : polySet.CPolygons() )
                zone->Outline()->AddPolygon( poly );

            wxString thermal = arr[8];
            if( thermal == wxS( "direct" ) )
                zone->SetPadConnection( ZONE_CONNECTION::FULL );

            wxString keepIsland = arr[9];
            if( keepIsland == wxS( "yes" ) )
                zone->SetIslandRemovalMode( ISLAND_REMOVAL_MODE::NEVER );

            wxString       fillData = arr[10];
            SHAPE_POLY_SET fillPolySet;
            try
            {
                for( const nlohmann::json& polyData : nlohmann::json::parse( fillData ) )
                {
                    for( const nlohmann::json& contourData : polyData )
                    {
                        SHAPE_POLY_SET contourPolySet = ParseLineChains(
                                contourData.get<wxString>(), SHAPE_ARC::DefaultAccuracyForPCB(), true );

                        SHAPE_POLY_SET currentOutline( contourPolySet.COutline( 0 ) );

                        for( int i = 1; i < contourPolySet.OutlineCount(); i++ )
                            currentOutline.AddHole( contourPolySet.COutline( i ) );

                        fillPolySet.Append( currentOutline );
                    }
                }

                fillPolySet.Fracture();

                zone->SetFilledPolysList( layer, fillPolySet );
                zone->SetIsFilled( true );
                zone->SetNeedRefill( false );
            }
            catch( nlohmann::json::exception& e )
            {
            }

            int fillOrder = wxAtoi( arr[13] );
            zone->SetAssignedPriority( 100 - fillOrder );

            wxString improveFabrication = arr[17];
            if( improveFabrication == wxS( "none" ) )
            {
                zone->SetMinThickness( 0 );
            }
            else
            {
                // arr[1] is "stroke Width" per docs
                int minThickness = std::max( pcbIUScale.mmToIU( 0.03 ),
                                             int( ConvertSize( arr[1] ) ) );
                zone->SetMinThickness( minThickness );
            }

            if( arr.size() > 18 )
            {
                zone->SetThermalReliefSpokeWidth( std::max( int( ConvertSize( arr[18] ) ),
                                                            zone->GetMinThickness() ) );
            }
            else
            {
                wxFAIL_MSG( wxString::Format( "COPPERAREA unexpected size %d: %s ",
                                              arr.size(),
                                              shape ) );

                zone->SetThermalReliefSpokeWidth( zone->GetMinThickness() );
            }

            aContainer->Add( zone.release(), ADD_MODE::APPEND );
        }
        else if( elType == wxS( "SVGNODE" ) )
        {
            nlohmann::json nodeData = nlohmann::json::parse( arr[1] );

            int          nodeType = nodeData.at( "nodeType" );
            wxString     layer = nodeData.at( "layerid" );
            PCB_LAYER_ID klayer = LayerToKi( layer );

            if( nodeType == 1 )
            {
                std::map<wxString, wxString> attributes = nodeData.at( "attrs" );

                if( layer == wxS( "19" ) ) // 3DModel
                {
                    if( !footprint )
                        continue;

                    auto ec_eType = get_opt( attributes, "c_etype" );
                    auto ec_rotation = get_opt( attributes, "c_rotation" );
                    auto ec_origin = get_opt( attributes, "c_origin" );
                    auto ec_width = get_opt( attributes, "c_width" );
                    auto ec_height = get_opt( attributes, "c_height" );
                    auto ez = get_opt( attributes, "z" );
                    auto etitle = get_opt( attributes, "title" );
                    auto euuid = get_opt( attributes, "uuid" );
                    auto etransform = get_opt( attributes, "transform" );

                    if( !ec_eType || *ec_eType != wxS( "outline3D" ) || !etitle )
                        continue;

                    wxString modelTitle = *etitle;
                    VECTOR3D kmodelOffset;
                    VECTOR3D kmodelRotation;

                    if( euuid )
                    {
                        PCB_FIELD* field = new PCB_FIELD( footprint, FIELD_T::USER,
                                                          DIRECT_MODEL_UUID_KEY );
                        field->SetLayer( Cmts_User );
                        field->SetVisible( false );
                        field->SetText( *euuid );
                        footprint->Add( field );
                    }

                    /*if( etransform )
                    {
                        PCB_FIELD* field = new PCB_FIELD( footprint, FIELD_T::USER,
                                                          "3D Transform" );
                        field->SetLayer( Cmts_User );
                        field->SetVisible( false );
                        field->SetText( *etransform );
                        footprint->Add( field );
                    }*/

                    if( ec_width && ec_height )
                    {
                        double fitXmm = pcbIUScale.IUTomm( ScaleSize( Convert( *ec_width ) ) );
                        double fitYmm = pcbIUScale.IUTomm( ScaleSize( Convert( *ec_height ) ) );

                        double rounding = 0.001;
                        fitXmm = KiROUND( fitXmm / rounding ) * rounding;
                        fitYmm = KiROUND( fitYmm / rounding ) * rounding;

                        PCB_FIELD* field = new PCB_FIELD( footprint, FIELD_T::USER,
                                                          MODEL_SIZE_KEY );
                        field->SetLayer( Cmts_User );
                        field->SetVisible( false );
                        field->SetText( wxString::FromCDouble( fitXmm ) + wxS( " " )
                                       + wxString::FromCDouble( fitYmm ) );
                        footprint->Add( field );
                    }

                    if( ec_origin )
                    {
                        wxArrayString orParts = wxSplit( *ec_origin, ',', '\0' );

                        if( orParts.size() == 2 )
                        {
                            VECTOR2D pos;
                            pos.x = Convert( orParts[0].Trim() );
                            pos.y = Convert( orParts[1].Trim() );

                            VECTOR2D rel = RelPos( pos );
                            kmodelOffset.x = -pcbIUScale.IUTomm( rel.x );
                            kmodelOffset.y = -pcbIUScale.IUTomm( rel.y );

                            RotatePoint( &kmodelOffset.x, &kmodelOffset.y,
                                         -footprint->GetOrientation() );
                        }
                    }

                    if( ez )
                    {
                        kmodelOffset.z = pcbIUScale.IUTomm( ScaleSize( Convert( ez->Trim() ) ) );
                    }

                    if( ec_rotation )
                    {
                        wxArrayString rotParts = wxSplit( *ec_rotation, ',', '\0' );

                        if( rotParts.size() == 3 )
                        {
                            kmodelRotation.x = -Convert( rotParts[0].Trim() );
                            kmodelRotation.y = -Convert( rotParts[1].Trim() );
                            kmodelRotation.z = -Convert( rotParts[2].Trim() )
                                               + footprint->GetOrientationDegrees();
                        }
                    }

                    if( footprint->GetLayer() == B_Cu )
                    {
                        kmodelRotation.z = 180 - kmodelRotation.z;
                        RotatePoint( &kmodelOffset.x, &kmodelOffset.y, ANGLE_180 );
                    }

                    FP_3DMODEL model;
                    model.m_Filename = kicadModelPrefix
                                       + EscapeString( modelTitle, ESCAPE_CONTEXT::CTX_FILENAME )
                                       + wxS( ".step" );
                    model.m_Offset = kmodelOffset;
                    model.m_Rotation = kmodelRotation;
                    footprint->Models().push_back( model );
                }
                else
                {
                    if( auto dataStr = get_opt( attributes, "d" ) )
                    {
                        int maxError = SHAPE_ARC::DefaultAccuracyForPCB();

                        if( dataStr->size() >= 8000 )
                            maxError *= 10;

                        SHAPE_POLY_SET polySet = ParseLineChains( dataStr->Trim(), maxError, true );

                        polySet.RebuildHolesFromContours();

                        std::unique_ptr<PCB_GROUP> group;

                        if( polySet.OutlineCount() > 1 )
                            group = std::make_unique<PCB_GROUP>( aContainer );

                        for( const SHAPE_POLY_SET::POLYGON& poly : polySet.CPolygons() )
                        {
                            auto shape = std::make_unique<PCB_SHAPE>( aContainer, SHAPE_T::POLY );

                            shape->SetFilled( true );
                            shape->SetPolyShape( poly );
                            shape->SetLayer( klayer );
                            shape->SetWidth( 0 );

                            if( group )
                                group->AddItem( shape.get() );

                            aContainer->Add( shape.release(), ADD_MODE::APPEND );
                        }

                        if( group )
                            aContainer->Add( group.release(), ADD_MODE::APPEND );
                    }
                }
            }
            else
            {
                THROW_IO_ERROR( wxString::Format( _( "Unknown SVGNODE nodeType %d" ), nodeType ) );
            }
        }
        else if( elType == wxS( "TEXT" ) )
        {
            PCB_TEXT* text;
            wxString  textType = arr[1];

            if( footprint && textType == wxS( "P" ) )
            {
                text = footprint->GetField( FIELD_T::REFERENCE );
            }
            else if( footprint && textType == wxS( "N" ) )
            {
                text = footprint->GetField( FIELD_T::VALUE );
            }
            else if( arr[12] == wxS( "none" ) )
            {
                text = new PCB_FIELD( aContainer, FIELD_T::USER );
                static_cast<PCB_FIELD*>( text )->SetVisible( false );
            }
            else
            {
                text = new PCB_TEXT( aContainer );
                aContainer->Add( text, ADD_MODE::APPEND );
            }

            VECTOR2D start;
            start.x = RelPosX( arr[2] );
            start.y = RelPosY( arr[3] );
            text->SetPosition( start );

            double thickness = ConvertSize( arr[4] );
            text->SetTextThickness( thickness );

            double rot = Convert( arr[5] );
            text->SetTextAngleDegrees( rot );

            text->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
            text->SetVertJustify( GR_TEXT_V_ALIGN_TOP );

            PCB_LAYER_ID layer = LayerToKi( arr[7] );
            text->SetLayer( layer );

            if( IsBackLayer( layer ) )
                text->SetMirrored( true );

            double height = ConvertSize( arr[9] ) * 0.8;
            text->SetTextSize( VECTOR2I( height, height ) );

            wxString textStr = arr[10];
            textStr.Replace( wxS( "\\n" ), wxS( "\n" ) );
            text->SetText( UnescapeHTML( textStr ) );

            //arr[11] // Geometry data

            wxString font = arr[14];
            if( !font.IsEmpty() )
                text->SetFont( KIFONT::FONT::GetFont( font ) );

            TransformTextToBaseline( text, wxEmptyString );
        }
        else if( elType == wxS( "VIA" ) )
        {
            VECTOR2D center( RelPosX( arr[1] ), RelPosY( arr[2] ) );
            int      kdia = ConvertSize( arr[3] );
            int      kdrill = ConvertSize( arr[5] ) * 2;

            if( footprint )
            {
                std::unique_ptr<PAD> pad = std::make_unique<PAD>( footprint );

                pad->SetPosition( center );
                pad->SetLayerSet( PAD::PTHMask() );
                pad->SetAttribute( PAD_ATTRIB::PTH );
                pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
                pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( kdia, kdia ) );
                pad->SetDrillShape( PAD_DRILL_SHAPE::CIRCLE );
                pad->SetDrillSize( VECTOR2I( kdrill, kdrill ) );

                footprint->Add( pad.release(), ADD_MODE::APPEND );
            }
            else
            {
                std::unique_ptr<PCB_VIA> via = std::make_unique<PCB_VIA>( aContainer );

                via->SetPosition( center );

                via->SetWidth( PADSTACK::ALL_LAYERS, kdia );
                via->SetNet( getOrAddNetItem( arr[4] ) );
                via->SetDrill( kdrill );

                aContainer->Add( via.release(), ADD_MODE::APPEND );
            }
        }
        else if( elType == wxS( "HOLE" ) )
        {
            FOOTPRINT* padContainer;

            VECTOR2D center( RelPosX( arr[1] ), RelPosY( arr[2] ) );
            int      kdia = ConvertSize( arr[3] ) * 2;
            wxString holeUuid = arr[4];

            if( footprint )
            {
                padContainer = footprint;
            }
            else
            {
                std::unique_ptr<FOOTPRINT> newFootprint =
                        std::make_unique<FOOTPRINT>( dynamic_cast<BOARD*>( aContainer ) );

                wxString name = wxS( "Hole_" ) + holeUuid;

                newFootprint->SetFPID( LIB_ID( wxEmptyString, name ) );
                newFootprint->SetPosition( center );
                newFootprint->SetLocked( true );

                newFootprint->Reference().SetText( name );
                newFootprint->Reference().SetVisible( false );
                newFootprint->Reference().SetTextSize( HIDDEN_TEXT_SIZE );
                newFootprint->Value().SetText( name );
                newFootprint->Value().SetVisible( false );
                newFootprint->Value().SetTextSize( HIDDEN_TEXT_SIZE );

                padContainer = newFootprint.get();
                aContainer->Add( newFootprint.release(), ADD_MODE::APPEND );
            }

            std::unique_ptr<PAD> pad = std::make_unique<PAD>( padContainer );

            pad->SetPosition( center );
            pad->SetLayerSet( PAD::UnplatedHoleMask() );
            pad->SetAttribute( PAD_ATTRIB::NPTH );
            pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
            pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( kdia, kdia ) );
            pad->SetDrillShape( PAD_DRILL_SHAPE::CIRCLE );
            pad->SetDrillSize( VECTOR2I( kdia, kdia ) );

            padContainer->Add( pad.release(), ADD_MODE::APPEND );
        }
        else if( elType == wxS( "PAD" ) )
        {
            FOOTPRINT* padContainer;

            VECTOR2D center( RelPosX( arr[2] ), RelPosY( arr[3] ) );
            VECTOR2D size( ConvertSize( arr[4] ), ConvertSize( arr[5] ) );
            wxString padUuid = arr[12];

            if( footprint )
            {
                padContainer = footprint;
            }
            else
            {
                std::unique_ptr<FOOTPRINT> newFootprint =
                        std::make_unique<FOOTPRINT>( dynamic_cast<BOARD*>( aContainer ) );

                wxString name = wxS( "Pad_" ) + padUuid;

                newFootprint->SetFPID( LIB_ID( wxEmptyString, name ) );
                newFootprint->SetPosition( center );
                newFootprint->SetLocked( true );

                newFootprint->Reference().SetText( name );
                newFootprint->Reference().SetVisible( false );
                newFootprint->Reference().SetTextSize( HIDDEN_TEXT_SIZE );
                newFootprint->Value().SetText( name );
                newFootprint->Value().SetVisible( false );
                newFootprint->Value().SetTextSize( HIDDEN_TEXT_SIZE );

                padContainer = newFootprint.get();
                aContainer->Add( newFootprint.release(), ADD_MODE::APPEND );
            }

            std::unique_ptr<PAD> pad = std::make_unique<PAD>( padContainer );

            pad->SetNet( getOrAddNetItem( arr[7] ) );
            pad->SetNumber( arr[8] );
            pad->SetPosition( center );
            pad->SetSize( PADSTACK::ALL_LAYERS, size );
            pad->SetOrientationDegrees( Convert( arr[11] ) );
            pad->SetThermalSpokeAngle( ANGLE_0 );

            wxString     elayer = arr[6];
            PCB_LAYER_ID klayer = LayerToKi( elayer );

            bool plated = arr[15] == wxS( "Y" );

            if( klayer == F_Cu )
            {
                pad->SetLayer( F_Cu );
                pad->SetLayerSet( PAD::SMDMask() );
                pad->SetAttribute( PAD_ATTRIB::SMD );
            }
            else if( klayer == B_Cu )
            {
                pad->SetLayer( B_Cu );
                pad->SetLayerSet( PAD::SMDMask().FlipStandardLayers() );
                pad->SetAttribute( PAD_ATTRIB::SMD );
            }
            else if( elayer == wxS( "11" ) )
            {
                pad->SetLayerSet( plated ? PAD::PTHMask() : PAD::UnplatedHoleMask() );
                pad->SetAttribute( plated ? PAD_ATTRIB::PTH : PAD_ATTRIB::NPTH );
            }
            else
            {
                pad->SetLayer( klayer );
                pad->SetLayerSet( LSET( { klayer } ) );
                pad->SetAttribute( PAD_ATTRIB::SMD );
            }

            wxString padType = arr[1];
            if( padType == wxS( "ELLIPSE" ) )
            {
                pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::OVAL );
            }
            else if( padType == wxS( "RECT" ) )
            {
                pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );
            }
            else if( padType == wxS( "OVAL" ) )
            {
                if( pad->GetSizeX() == pad->GetSizeY() )
                    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
                else
                    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::OVAL );
            }
            else if( padType == wxS( "POLYGON" ) )
            {
                pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CUSTOM );
                pad->SetAnchorPadShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
                pad->SetSize( PADSTACK::ALL_LAYERS, { 1, 1 } );

                wxArrayString data = wxSplit( arr[10], ' ', '\0' );

                SHAPE_LINE_CHAIN chain;
                for( int i = 1; i < data.size(); i += 2 )
                {
                    VECTOR2D pt;
                    pt.x = RelPosX( data[i - 1] );
                    pt.y = RelPosY( data[i] );
                    chain.Append( pt );
                }
                chain.SetClosed( true );

                chain.Move( -center );
                chain.Rotate( -pad->GetOrientation() );
                pad->AddPrimitivePoly( PADSTACK::ALL_LAYERS, chain, 0, true );
            }

            wxString holeDia = arr[9];
            if( !holeDia.IsEmpty() )
            {
                double holeD = ConvertSize( holeDia ) * 2;
                double holeL = 0;

                wxString holeLength = arr[13];
                if( !holeLength.IsEmpty() )
                    holeL = ConvertSize( holeLength );

                if( holeL > 0 )
                {
                    pad->SetDrillShape( PAD_DRILL_SHAPE::OBLONG );

                    if( size.x < size.y )
                        pad->SetDrillSize( VECTOR2I( holeD, holeL ) );
                    else
                        pad->SetDrillSize( VECTOR2I( holeL, holeD ) );
                }
                else
                {
                    pad->SetDrillShape( PAD_DRILL_SHAPE::CIRCLE );
                    pad->SetDrillSize( VECTOR2I( holeD, holeD ) );
                }
            }

            wxString pasteExp = arr[17];
            if( !pasteExp.IsEmpty() )
            {
                double pasteExpansion = ConvertSize( pasteExp );
                pad->SetLocalSolderPasteMargin( pasteExpansion );
            }

            wxString maskExp = arr[18];
            if( !maskExp.IsEmpty() )
            {
                double maskExpansion = ConvertSize( maskExp );
                pad->SetLocalSolderMaskMargin( maskExpansion );
            }

            padContainer->Add( pad.release(), ADD_MODE::APPEND );
        }
    }
}


FOOTPRINT* PCB_IO_EASYEDA_PARSER::ParseFootprint(
        const VECTOR2D& aOrigin, const EDA_ANGLE& aOrientation, int aLayer, BOARD* aParent,
        std::map<wxString, wxString>                    aParams,
        std::map<wxString, std::unique_ptr<FOOTPRINT>>& aFootprintMap, wxArrayString aShapes )
{
    std::unique_ptr<FOOTPRINT> footprint = std::make_unique<FOOTPRINT>( aParent );

    if( aLayer == 2 ) // Bottom layer
    {
        footprint->SetLayer( B_Cu );
        footprint->SetOrientation( aOrientation - ANGLE_180 );
    }
    else
    {
        footprint->SetLayer( F_Cu );
        footprint->SetOrientation( aOrientation );
    }

    footprint->Value().SetText( aParams[wxS( "package" )] );

    m_relOrigin = aOrigin;

    ParseToBoardItemContainer( footprint.get(), aParent, aParams, aFootprintMap, aShapes );

    // Heal board outlines
    std::vector<PCB_SHAPE*>                 shapes;

    for( BOARD_ITEM* item : footprint->GraphicalItems() )
    {
        if( !item->IsOnLayer( Edge_Cuts ) )
            continue;

        if( item->Type() == PCB_SHAPE_T )
            shapes.push_back( static_cast<PCB_SHAPE*>( item ) );
    }

    ConnectBoardShapes( shapes, SHAPE_JOIN_DISTANCE );

    // EasyEDA footprints don't have courtyard, so build a box ourselves
    if( !footprint->IsOnLayer( F_CrtYd ) )
    {
        BOX2I bbox = footprint->GetLayerBoundingBox( { F_Cu, F_Fab, F_Paste, F_Mask, Edge_Cuts } );
        bbox.Inflate( pcbIUScale.mmToIU( 0.25 ) ); // Default courtyard clearance

        std::unique_ptr<PCB_SHAPE> shape =
                std::make_unique<PCB_SHAPE>( footprint.get(), SHAPE_T::RECTANGLE );

        shape->SetWidth( pcbIUScale.mmToIU( DEFAULT_COURTYARD_WIDTH ) );
        shape->SetLayer( F_CrtYd );
        shape->SetStart( bbox.GetOrigin() );
        shape->SetEnd( bbox.GetEnd() );

        footprint->Add( shape.release(), ADD_MODE::APPEND );
    }

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
        // Add reference text field on F_Fab
        int c_refTextSize = pcbIUScale.mmToIU( 0.5 );      // KLC min Fab text size
        int c_refTextThickness = pcbIUScale.mmToIU( 0.1 ); // Decent text thickness
        std::unique_ptr<PCB_TEXT> refText = std::make_unique<PCB_TEXT>( footprint.get() );

        refText->SetLayer( F_Fab );
        refText->SetTextSize( VECTOR2I( c_refTextSize, c_refTextSize ) );
        refText->SetTextThickness( c_refTextThickness );
        refText->SetText( wxT( "${REFERENCE}" ) );

        footprint->Add( refText.release(), ADD_MODE::APPEND );
    }

    return footprint.release();
}


void PCB_IO_EASYEDA_PARSER::ParseBoard( BOARD* aBoard, const VECTOR2D& aOrigin,
                                     std::map<wxString, std::unique_ptr<FOOTPRINT>>& aFootprintMap,
                                     wxArrayString                                   aShapes )
{
    m_relOrigin = aOrigin;

    ParseToBoardItemContainer( aBoard, nullptr, {}, aFootprintMap, aShapes );

    // Heal board outlines
    std::vector<PCB_SHAPE*>                 shapes;

    for( BOARD_ITEM* item : aBoard->Drawings() )
    {
        if( !item->IsOnLayer( Edge_Cuts ) )
            continue;

        if( item->Type() == PCB_SHAPE_T )
            shapes.push_back( static_cast<PCB_SHAPE*>( item ) );
    }

    ConnectBoardShapes( shapes, SHAPE_JOIN_DISTANCE );
}
