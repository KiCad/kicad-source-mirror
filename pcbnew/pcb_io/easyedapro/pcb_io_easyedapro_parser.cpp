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

#include "pcb_io_easyedapro_parser.h"
#include <io/easyedapro/easyedapro_import_utils.h>

#include <memory>

#include <json_common.h>
#include <core/json_serializers.h>
#include <core/map_helpers.h>
#include <string_utils.h>

#include <wx/wfstream.h>
#include <wx/stdstream.h>
#include <wx/log.h>
#include <glm/glm.hpp>

#include <progress_reporter.h>
#include <footprint.h>
#include <board.h>
#include <board_design_settings.h>
#include <bezier_curves.h>
#include <pcb_group.h>
#include <pcb_track.h>
#include <pcb_shape.h>
#include <pcb_text.h>
#include <font/font.h>
#include <geometry/shape_arc.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_simple.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_compound.h>
#include <zone.h>
#include <pad.h>
#include <convert_basic_shapes_to_polygon.h>
#include <project.h>
#include <fix_board_shape.h>
#include <pcb_reference_image.h>
#include <core/mirror.h>


static const wxString QUERY_MODEL_UUID_KEY = wxS( "JLC_3DModel_Q" );
static const wxString MODEL_SIZE_KEY = wxS( "JLC_3D_Size" );

static const int SHAPE_JOIN_DISTANCE = pcbIUScale.mmToIU( 1.5 );


double PCB_IO_EASYEDAPRO_PARSER::Convert( wxString aValue )
{
    double value = 0;

    if( !aValue.ToCDouble( &value ) )
        THROW_IO_ERROR( wxString::Format( _( "Failed to parse value: '%s'" ), aValue ) );

    return value;
}


PCB_IO_EASYEDAPRO_PARSER::PCB_IO_EASYEDAPRO_PARSER( BOARD* aBoard, PROGRESS_REPORTER* aProgressReporter )
{
    m_board = aBoard;
}


PCB_IO_EASYEDAPRO_PARSER::~PCB_IO_EASYEDAPRO_PARSER()
{
}


PCB_LAYER_ID PCB_IO_EASYEDAPRO_PARSER::LayerToKi( int aLayer )
{
    switch( aLayer )
    {
    case 1: return F_Cu;
    case 2: return B_Cu;
    case 3: return F_SilkS;
    case 4: return B_SilkS;
    case 5: return F_Mask;
    case 6: return B_Mask;
    case 7: return F_Paste;
    case 8: return B_Paste;
    case 9: return F_Fab;
    case 10: return B_Fab;
    case 11: return Edge_Cuts;
    case 12: return Edge_Cuts; // Multi
    case 13: return Dwgs_User;
    case 14: return Eco2_User;

    case 15: return In1_Cu;
    case 16: return In2_Cu;
    case 17: return In3_Cu;
    case 18: return In4_Cu;
    case 19: return In5_Cu;
    case 20: return In6_Cu;
    case 21: return In7_Cu;
    case 22: return In8_Cu;
    case 23: return In9_Cu;
    case 24: return In10_Cu;
    case 25: return In11_Cu;
    case 26: return In12_Cu;
    case 27: return In13_Cu;
    case 28: return In14_Cu;
    case 29: return In15_Cu;
    case 30: return In16_Cu;
    case 31: return In17_Cu;
    case 32: return In18_Cu;
    case 33: return In19_Cu;
    case 34: return In20_Cu;
    case 35: return In21_Cu;
    case 36: return In22_Cu;
    case 37: return In23_Cu;
    case 38: return In24_Cu;
    case 39: return In25_Cu;
    case 40: return In26_Cu;
    case 41: return In27_Cu;
    case 42: return In28_Cu;
    case 43: return In29_Cu;
    case 44: return In30_Cu;

    case 48: return F_Fab; // Component shape layer
    case 49: return F_Fab; // Component marking

    case 53: return User_4; // 3D shell outline
    case 54: return User_5; // 3D shell top
    case 55: return User_6; // 3D shell bot
    case 56: return User_7; // Drill drawing

    default: break;
    }

    return User_1;
}


static void AlignText( EDA_TEXT* text, int align )
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


void PCB_IO_EASYEDAPRO_PARSER::fillFootprintModelInfo( FOOTPRINT* footprint,
                                                       const wxString& modelUuid,
                                                       const wxString& modelTitle,
                                                       const wxString& modelTransform ) const
{
    // TODO: make this path configurable?
    const wxString easyedaModelDir = wxS( "EASYEDA_MODELS" );
    const wxString kicadModelPrefix = wxS( "${KIPRJMOD}/" ) + easyedaModelDir + wxS( "/" );

    VECTOR3D kmodelOffset;
    VECTOR3D kmodelRotation;

    if( !modelUuid.IsEmpty() && !footprint->GetField( QUERY_MODEL_UUID_KEY ) )
    {
        PCB_FIELD* field = new PCB_FIELD( footprint, FIELD_T::USER, QUERY_MODEL_UUID_KEY );
        field->SetLayer( Cmts_User );
        field->SetVisible( false );
        field->SetText( modelUuid );
        footprint->Add( field );
    }

    if( !modelTransform.IsEmpty() && !footprint->GetField( MODEL_SIZE_KEY ) )
    {
        wxArrayString arr = wxSplit( modelTransform, ',', '\0' );

        double fitXmm = pcbIUScale.IUTomm( ScaleSize( Convert( arr[0] ) ) );
        double fitYmm = pcbIUScale.IUTomm( ScaleSize( Convert( arr[1] ) ) );

        if( fitXmm > 0.0 && fitYmm > 0.0 )
        {
            PCB_FIELD* field = new PCB_FIELD( footprint, FIELD_T::USER, MODEL_SIZE_KEY );
            field->SetLayer( Cmts_User );
            field->SetVisible( false );
            field->SetText( wxString::FromCDouble( fitXmm ) + wxS( " " )
                           + wxString::FromCDouble( fitYmm ) );
            footprint->Add( field );
        }

        kmodelRotation.z = -Convert( arr[3] );
        kmodelRotation.x = -Convert( arr[4] );
        kmodelRotation.y = -Convert( arr[5] );

        kmodelOffset.x = pcbIUScale.IUTomm( ScaleSize( Convert( arr[6] ) ) );
        kmodelOffset.y = pcbIUScale.IUTomm( ScaleSize( Convert( arr[7] ) ) );
        kmodelOffset.z = pcbIUScale.IUTomm( ScaleSize( Convert( arr[8] ) ) );
    }

    if( !modelTitle.IsEmpty() && footprint->Models().empty() )
    {
        FP_3DMODEL model;
        model.m_Filename = kicadModelPrefix
                           + EscapeString( modelTitle, ESCAPE_CONTEXT::CTX_FILENAME )
                           + wxS( ".step" );
        model.m_Offset = kmodelOffset;
        model.m_Rotation = kmodelRotation;
        footprint->Models().push_back( model );
    }
}


std::vector<std::unique_ptr<PCB_SHAPE>>
PCB_IO_EASYEDAPRO_PARSER::ParsePoly( BOARD_ITEM_CONTAINER* aContainer, nlohmann::json polyData,
                                  bool aClosed, bool aInFill ) const
{
    std::vector<std::unique_ptr<PCB_SHAPE>> results;

    VECTOR2D prevPt;
    for( int i = 0; i < polyData.size(); i++ )
    {
        nlohmann::json val = polyData.at( i );

        if( val.is_string() )
        {
            wxString str = val;
            if( str == wxS( "CIRCLE" ) )
            {
                VECTOR2D center;
                center.x = ( polyData.at( ++i ) );
                center.y = ( polyData.at( ++i ) );
                double r = ( polyData.at( ++i ) );

                std::unique_ptr<PCB_SHAPE> shape =
                        std::make_unique<PCB_SHAPE>( aContainer, SHAPE_T::CIRCLE );

                shape->SetCenter( ScalePos( center ) );
                shape->SetEnd( ScalePos( center + VECTOR2D( r, 0 ) ) );
                shape->SetFilled( aClosed );

                results.emplace_back( std::move( shape ) );
            }
            else if( str == wxS( "R" ) )
            {
                VECTOR2D start, size;
                start.x = ( polyData.at( ++i ) );
                start.y = ( polyData.at( ++i ) );
                size.x = ( polyData.at( ++i ) );
                size.y = -( polyData.at( ++i ).get<double>() );
                double angle = polyData.at( ++i );
                double cr = ( i + 1 ) < polyData.size() ? polyData.at( ++i ).get<double>() : 0;

                if( cr == 0 )
                {
                    std::unique_ptr<PCB_SHAPE> shape =
                            std::make_unique<PCB_SHAPE>( aContainer, SHAPE_T::RECTANGLE );

                    shape->SetStart( ScalePos( start ) );
                    shape->SetEnd( ScalePos( start + size ) );
                    shape->SetFilled( aClosed );
                    shape->Rotate( ScalePos( start ), EDA_ANGLE( angle, DEGREES_T ) );

                    results.emplace_back( std::move( shape ) );
                }
                else
                {
                    VECTOR2D end = start + size;

                    auto addSegment = [&]( VECTOR2D aStart, VECTOR2D aEnd )
                    {
                        std::unique_ptr<PCB_SHAPE> shape =
                                std::make_unique<PCB_SHAPE>( aContainer, SHAPE_T::SEGMENT );

                        shape->SetStart( ScalePos( aStart ) );
                        shape->SetEnd( ScalePos( aEnd ) );
                        shape->SetFilled( aClosed );
                        shape->Rotate( ScalePos( start ), EDA_ANGLE( angle, DEGREES_T ) );

                        results.emplace_back( std::move( shape ) );
                    };

                    auto addArc = [&]( VECTOR2D aStart, VECTOR2D aEnd, VECTOR2D center )
                    {
                        std::unique_ptr<PCB_SHAPE> shape =
                                std::make_unique<PCB_SHAPE>( aContainer, SHAPE_T::ARC );

                        shape->SetStart( ScalePos( aStart ) );
                        shape->SetEnd( ScalePos( aEnd ) );
                        shape->SetCenter( ScalePos( center ) );
                        shape->SetFilled( aClosed );
                        shape->Rotate( ScalePos( start ), EDA_ANGLE( angle, DEGREES_T ) );

                        results.emplace_back( std::move( shape ) );
                    };

                    addSegment( { start.x + cr, start.y }, { end.x - cr, start.y } );
                    addSegment( { end.x, start.y - cr }, { end.x, end.y + cr } );
                    addSegment( { start.x + cr, end.y }, { end.x - cr, end.y } );
                    addSegment( { start.x, start.y - cr }, { start.x, end.y + cr } );

                    addArc( { end.x - cr, start.y }, { end.x, start.y - cr },
                            { end.x - cr, start.y - cr } );

                    addArc( { end.x, end.y + cr }, { end.x - cr, end.y },
                            { end.x - cr, end.y + cr } );

                    addArc( { start.x + cr, end.y }, { start.x, end.y + cr },
                            { start.x + cr, end.y + cr } );

                    addArc( { start.x, start.y - cr }, { start.x + cr, start.y },
                            { start.x + cr, start.y - cr } );
                }
            }
            else if( str == wxS( "ARC" ) || str == wxS( "CARC" ) )
            {
                VECTOR2D end;
                double   angle = polyData.at( ++i ).get<double>() / ( aInFill ? 10 : 1 );
                end.x = ( polyData.at( ++i ) );
                end.y = ( polyData.at( ++i ) );

                std::unique_ptr<PCB_SHAPE> shape =
                        std::make_unique<PCB_SHAPE>( aContainer, SHAPE_T::ARC );

                if( angle < 0 )
                {
                    shape->SetStart( ScalePos( prevPt ) );
                    shape->SetEnd( ScalePos( end ) );
                }
                else
                {
                    shape->SetStart( ScalePos( end ) );
                    shape->SetEnd( ScalePos( prevPt ) );
                }

                VECTOR2D delta = end - prevPt;
                VECTOR2D mid = ( prevPt + delta / 2 );

                double   ha = angle / 2;
                double   hd = delta.EuclideanNorm() / 2;
                double   cdist = hd / tan( DEG2RAD( ha ) );
                VECTOR2D center = mid + delta.Perpendicular().Resize( cdist );
                shape->SetCenter( ScalePos( center ) );

                shape->SetFilled( aClosed );

                results.emplace_back( std::move( shape ) );

                prevPt = end;
            }
            else if( str == wxS( "L" ) )
            {
                SHAPE_LINE_CHAIN chain;
                chain.Append( ScalePos( prevPt ) );

                while( i < polyData.size() - 2 && polyData.at( i + 1 ).is_number() )
                {
                    VECTOR2D pt;
                    pt.x = ( polyData.at( ++i ) );
                    pt.y = ( polyData.at( ++i ) );

                    chain.Append( ScalePos( pt ) );

                    prevPt = pt;
                }

                if( aClosed )
                {
                    std::unique_ptr<PCB_SHAPE> shape =
                            std::make_unique<PCB_SHAPE>( aContainer, SHAPE_T::POLY );

                    wxASSERT( chain.PointCount() > 2 );

                    if( chain.PointCount() > 2 )
                    {
                        chain.SetClosed( true );
                        shape->SetFilled( true );
                        shape->SetPolyShape( chain );

                        results.emplace_back( std::move( shape ) );
                    }
                }
                else
                {
                    for( int s = 0; s < chain.SegmentCount(); s++ )
                    {
                        SEG seg = chain.Segment( s );

                        std::unique_ptr<PCB_SHAPE> shape =
                                std::make_unique<PCB_SHAPE>( aContainer, SHAPE_T::SEGMENT );

                        shape->SetStart( seg.A );
                        shape->SetEnd( seg.B );

                        results.emplace_back( std::move( shape ) );
                    }
                }
            }
        }
        else if( val.is_number() )
        {
            prevPt.x = ( polyData.at( i ) );
            prevPt.y = ( polyData.at( ++i ) );
        }
    }

    return results;
}


SHAPE_LINE_CHAIN
PCB_IO_EASYEDAPRO_PARSER::ParseContour( nlohmann::json polyData, bool aInFill,
                                        int aMaxError ) const
{
    SHAPE_LINE_CHAIN result;
    VECTOR2D         prevPt;

    for( int i = 0; i < polyData.size(); i++ )
    {
        nlohmann::json val = polyData.at( i );

        if( val.is_string() )
        {
            wxString str = val;
            if( str == wxS( "CIRCLE" ) )
            {
                VECTOR2D center;
                center.x = ( polyData.at( ++i ) );
                center.y = ( polyData.at( ++i ) );
                double r = ( polyData.at( ++i ) );

                TransformCircleToPolygon( result, ScalePos( center ), ScaleSize( r ), aMaxError,
                                          ERROR_INSIDE );
            }
            else if( str == wxS( "R" ) )
            {
                VECTOR2D start, size;
                start.x = ( polyData.at( ++i ) );
                start.y = ( polyData.at( ++i ) );
                size.x = ( polyData.at( ++i ) );
                size.y = ( polyData.at( ++i ).get<double>() );
                double angle = polyData.at( ++i );
                double cr = ( i + 1 ) < polyData.size() ? polyData.at( ++i ).get<double>() : 0;

                SHAPE_POLY_SET poly;

                VECTOR2D kstart = ScalePos( start );
                VECTOR2D ksize = ScaleSize( size );
                VECTOR2D kcenter = kstart + ksize / 2;
                RotatePoint( kcenter, kstart, EDA_ANGLE( angle, DEGREES_T ) );

                TransformRoundChamferedRectToPolygon( poly, kcenter, ksize, EDA_ANGLE( angle, DEGREES_T ),
                                                      ScaleSize( cr ), 0, 0, 0, aMaxError, ERROR_INSIDE );

                result.Append( poly.Outline( 0 ) );
            }
            else if( str == wxS( "ARC" ) || str == wxS( "CARC" ) )
            {
                VECTOR2D end;
                double   angle = polyData.at( ++i ).get<double>();

                if( aInFill ) // In .epcb fills, the angle is 10x for some reason
                    angle /= 10;

                end.x = ( polyData.at( ++i ) );
                end.y = ( polyData.at( ++i ) );

                VECTOR2D arcStart, arcEnd;
                arcStart = prevPt;
                arcEnd = end;

                VECTOR2D delta = end - prevPt;
                VECTOR2D mid = ( prevPt + delta / 2 );

                double   ha = angle / 2;
                double   hd = delta.EuclideanNorm() / 2;
                double   cdist = hd / tan( DEG2RAD( ha ) );
                VECTOR2D center = mid + delta.Perpendicular().Resize( cdist );

                SHAPE_ARC sarc;
                sarc.ConstructFromStartEndCenter( ScalePos( arcStart ), ScalePos( arcEnd ),
                                                  ScalePos( center ), angle >= 0, 0 );

                result.Append( sarc, aMaxError );

                prevPt = end;
            }
            else if( str == wxS( "C" ) )
            {
                VECTOR2D pt1;
                pt1.x = ( polyData.at( ++i ) );
                pt1.y = ( polyData.at( ++i ) );

                VECTOR2D pt2;
                pt2.x = ( polyData.at( ++i ) );
                pt2.y = ( polyData.at( ++i ) );

                VECTOR2D pt3;
                pt3.x = ( polyData.at( ++i ) );
                pt3.y = ( polyData.at( ++i ) );

                std::vector<VECTOR2I> ctrlPoints = { ScalePos( prevPt ), ScalePos( pt1 ),
                                                     ScalePos( pt2 ), ScalePos( pt3 ) };
                BEZIER_POLY           converter( ctrlPoints );

                std::vector<VECTOR2I> bezierPoints;
                converter.GetPoly( bezierPoints, aMaxError );

                result.Append( bezierPoints );

                prevPt = pt3;
            }
            else if( str == wxS( "L" ) )
            {
                result.Append( ScalePos( prevPt ) );

                while( i < polyData.size() - 2 && polyData.at( i + 1 ).is_number() )
                {
                    VECTOR2D pt;
                    pt.x = ( polyData.at( ++i ) );
                    pt.y = ( polyData.at( ++i ) );

                    result.Append( ScalePos( pt ) );

                    prevPt = pt;
                }
            }
        }
        else if( val.is_number() )
        {
            prevPt.x = ( polyData.at( i ) );
            prevPt.y = ( polyData.at( ++i ) );
        }
    }

    return result;
}


std::unique_ptr<PAD> PCB_IO_EASYEDAPRO_PARSER::createPAD( FOOTPRINT*            aFootprint,
                                                          const nlohmann::json& line )
{
    wxString uuid = line.at( 1 );

    // if( line.at( 2 ).is_number() )
    //     int unk = line.at( 2 ).get<int>();
    // else if( line.at( 2 ).is_string() )
    //     int unk = wxAtoi( line.at( 2 ).get<wxString>() );

    wxString     netname = line.at( 3 );
    int          layer = line.at( 4 ).get<int>();
    PCB_LAYER_ID klayer = LayerToKi( layer );

    wxString padNumber = line.at( 5 );

    VECTOR2D center;
    center.x = line.at( 6 );
    center.y = line.at( 7 );

    double orientation = line.at( 8 );

    nlohmann::json padHole = line.at( 9 );
    nlohmann::json padShape = line.at( 10 );

    std::unique_ptr<PAD> pad = std::make_unique<PAD>( aFootprint );

    pad->SetNumber( padNumber );
    pad->SetPosition( ScalePos( center ) );
    pad->SetOrientationDegrees( orientation );

    // Check if this pad has a real drill hole
    // JLCEDA may use ["ROUND",0,0] to indicate SMD pads
    bool hasHole = false;
    
    if( !padHole.is_null() && !padHole.empty() )
    {
        wxString holeShape = padHole.at( 0 );

        if( holeShape == wxS( "ROUND" ) || holeShape == wxS( "SLOT" ) )
        {
            VECTOR2D drill;
            drill.x = padHole.at( 1 );
            drill.y = padHole.at( 2 );

            // Only treat as PTH if hole size is non-zero
            if( drill.x > 0 || drill.y > 0 )
            {
                hasHole = true;

                double drill_dir = 0;

                if( line.at( 14 ).is_number() )
                    drill_dir = line.at( 14 );

                double deg = EDA_ANGLE( drill_dir, DEGREES_T ).Normalize90().AsDegrees();

                if( std::abs( deg ) >= 45 )
                    std::swap( drill.x, drill.y ); // KiCad doesn't support arbitrary hole direction

                if( holeShape == wxS( "SLOT" ) )
                {
                    pad->SetDrillShape( PAD_DRILL_SHAPE::OBLONG );
                }

                pad->SetDrillSize( ScaleSize( drill ) );
                pad->SetLayerSet( PAD::PTHMask() );
                pad->SetAttribute( PAD_ATTRIB::PTH );
            }
        }
    }
    
    // If no valid hole, this is an SMD pad
    if( !hasHole )
    {
        if( klayer == F_Cu )
        {
            pad->SetLayerSet( PAD::SMDMask() );
        }
        else if( klayer == B_Cu )
        {
            pad->SetLayerSet( PAD::SMDMask().FlipStandardLayers() );
        }

        pad->SetAttribute( PAD_ATTRIB::SMD );
    }

    wxString padSh = padShape.at( 0 );
    if( padSh == wxS( "RECT" ) )
    {
        VECTOR2D size;
        size.x = padShape.at( 1 );
        size.y = padShape.at( 2 );
        double cr_p = padShape.size() > 3 ? padShape.at( 3 ).get<double>() : 0;

        pad->SetSize( PADSTACK::ALL_LAYERS, ScaleSize( size ) );

        if( cr_p == 0 )
        {
            pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );
        }
        else
        {
            pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::ROUNDRECT );
            pad->SetRoundRectRadiusRatio( PADSTACK::ALL_LAYERS, cr_p / 100 );
        }
    }
    else if( padSh == wxS( "ELLIPSE" ) )
    {
        VECTOR2D size;
        size.x = padShape.at( 1 );
        size.y = padShape.at( 2 );

        pad->SetSize( PADSTACK::ALL_LAYERS, ScaleSize( size ) );
        pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
    }
    else if( padSh == wxS( "OVAL" ) )
    {
        VECTOR2D size;
        size.x = padShape.at( 1 );
        size.y = padShape.at( 2 );

        pad->SetSize( PADSTACK::ALL_LAYERS, ScaleSize( size ) );
        pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::OVAL );
    }
    else if( padSh == wxS( "POLY" ) )
    {
        pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CUSTOM );
        pad->SetAnchorPadShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
        pad->SetSize( PADSTACK::ALL_LAYERS, { 1, 1 } );

        nlohmann::json polyData = padShape.at( 1 );

        std::vector<std::unique_ptr<PCB_SHAPE>> results =
                ParsePoly( aFootprint, polyData, true, false );

        for( auto& shape : results )
        {
            shape->SetLayer( klayer );
            shape->SetWidth( 0 );

            shape->Move( -pad->GetPosition() );

            pad->AddPrimitive( PADSTACK::ALL_LAYERS, shape.release() );
        }
    }

    pad->SetThermalSpokeAngle( ANGLE_90 );

    return std::move( pad );
}


FOOTPRINT* PCB_IO_EASYEDAPRO_PARSER::ParseFootprint( const nlohmann::json&              aProject,
                                                     const wxString&                    aFpUuid,
                                                     const std::vector<nlohmann::json>& aLines )
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

    for( const nlohmann::json& line : aLines )
    {
        if( line.size() == 0 )
            continue;

        wxString type = line.at( 0 );

        if( type == wxS( "POLY" ) || type == wxS( "PAD" ) || type == wxS( "FILL" )
            || type == wxS( "ATTR" ) )
        {
            wxString uuid = line.at( 1 );

            // if( line.at( 2 ).is_number() )
            //     int unk = line.at( 2 ).get<int>();
            // else if( line.at( 2 ).is_string() )
            //     int unk = wxAtoi( line.at( 2 ).get<wxString>() );

            wxString     netname = line.at( 3 );
            int          layer = line.at( 4 ).get<int>();
            PCB_LAYER_ID klayer = LayerToKi( layer );

            if( type == wxS( "POLY" ) )
            {
                double         thickness = ( line.at( 5 ) );
                nlohmann::json polyData = line.at( 6 );

                std::vector<std::unique_ptr<PCB_SHAPE>> results =
                        ParsePoly( footprint, polyData, false, false );

                for( auto& shape : results )
                {
                    shape->SetLayer( klayer );
                    shape->SetWidth( ScaleSize( thickness ) );

                    footprint->Add( shape.release(), ADD_MODE::APPEND );
                }
            }
            else if( type == wxS( "PAD" ) )
            {
                std::unique_ptr<PAD> pad = createPAD( footprint, line );

                footprint->Add( pad.release(), ADD_MODE::APPEND );
            }
            else if( type == wxS( "FILL" ) )
            {
                int          layer = line.at( 4 ).get<int>();
                PCB_LAYER_ID klayer = LayerToKi( layer );

                double width = line.at( 5 );

                nlohmann::json polyDataList = line.at( 7 );

                if( !polyDataList.is_null() && !polyDataList.empty() )
                {
                    if( !polyDataList.at( 0 ).is_array() )
                        polyDataList = nlohmann::json::array( { polyDataList } );

                    std::vector<SHAPE_LINE_CHAIN> contours;
                    for( nlohmann::json& polyData : polyDataList )
                    {
                        SHAPE_LINE_CHAIN contour = ParseContour( polyData, false );
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

                    BOX2I polyBBox = polySet.BBox();

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
            else if( type == wxS( "ATTR" ) )
            {
                EASYEDAPRO::PCB_ATTR attr = line;

                if( attr.key == wxS( "Designator" ) )
                    footprint->GetField( FIELD_T::REFERENCE )->SetText( attr.value );
            }
        }
        else if( type == wxS( "REGION" ) )
        {
            wxString uuid = line.at( 1 );

            // if( line.at( 2 ).is_number() )
            //     int unk = line.at( 2 ).get<int>();
            // else if( line.at( 2 ).is_string() )
            //     int unk = wxAtoi( line.at( 2 ).get<wxString>() );

            int          layer = line.at( 3 ).get<int>();
            PCB_LAYER_ID klayer = LayerToKi( layer );

            double         width = line.at( 4 );
            std::set<int>  flags = line.at( 5 );
            nlohmann::json polyDataList = line.at( 6 );

            for( nlohmann::json& polyData : polyDataList )
            {
                SHAPE_POLY_SET polySet;

                std::vector<std::unique_ptr<PCB_SHAPE>> results =
                        ParsePoly( nullptr, polyData, true, false );

                for( auto& shape : results )
                {
                    shape->SetFilled( true );
                    shape->TransformShapeToPolygon( polySet, klayer, 0, ARC_HIGH_DEF, ERROR_INSIDE,
                                                    true );
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

    if( aProject.is_object() && aProject.contains( "devices" ) )
    {
        std::map<wxString, EASYEDAPRO::PRJ_DEVICE> devicesMap = aProject.at( "devices" );
        std::map<wxString, wxString>               compAttrs;

        for( auto& [devUuid, devData] : devicesMap )
        {
            if( auto fp = get_opt( devData.attributes, "Footprint" ) )
            {
                if( *fp == aFpUuid )
                {
                    compAttrs = devData.attributes;
                    break;
                }
            }
        }

        wxString modelUuid, modelTitle, modelTransform;

        modelUuid = get_def( compAttrs, "3D Model", "" );
        modelTitle = get_def( compAttrs, "3D Model Title", modelUuid );
        modelTransform = get_def( compAttrs, "3D Model Transform", "" );

        fillFootprintModelInfo( footprint, modelUuid, modelTitle, modelTransform );
    }

    // Heal board outlines
    std::vector<PCB_SHAPE*>                 edgeShapes;

    for( BOARD_ITEM* item : footprint->GraphicalItems() )
    {
        if( item->IsOnLayer( Edge_Cuts ) && item->Type() == PCB_SHAPE_T )
            edgeShapes.push_back( static_cast<PCB_SHAPE*>( item ) );
    }

    ConnectBoardShapes( edgeShapes, SHAPE_JOIN_DISTANCE );

    // EasyEDA footprints don't have courtyard, so build a box ourselves
    if( !footprint->IsOnLayer( F_CrtYd ) )
    {
        BOX2I bbox = footprint->GetLayerBoundingBox( { F_Cu, F_Fab, F_Paste, F_Mask, Edge_Cuts } );
        bbox.Inflate( pcbIUScale.mmToIU( 0.25 ) ); // Default courtyard clearance

        std::unique_ptr<PCB_SHAPE> shape =
                std::make_unique<PCB_SHAPE>( footprint, SHAPE_T::RECTANGLE );

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
        std::unique_ptr<PCB_TEXT> refText = std::make_unique<PCB_TEXT>( footprint );

        refText->SetLayer( F_Fab );
        refText->SetTextSize( VECTOR2I( c_refTextSize, c_refTextSize ) );
        refText->SetTextThickness( c_refTextThickness );
        refText->SetText( wxT( "${REFERENCE}" ) );

        footprint->Add( refText.release(), ADD_MODE::APPEND );
    }

    return footprintPtr.release();
}


void PCB_IO_EASYEDAPRO_PARSER::ParseBoard(
        BOARD* aBoard, const nlohmann::json& aProject,
        std::map<wxString, std::unique_ptr<FOOTPRINT>>&    aFootprintMap,
        const std::map<wxString, EASYEDAPRO::BLOB>&        aBlobMap,
        const std::multimap<wxString, EASYEDAPRO::POURED>& aPouredMap,
        const std::vector<nlohmann::json>& aLines, const wxString& aFpLibName )
{
    std::map<wxString, std::vector<nlohmann::json>> componentLines;
    std::map<wxString, std::vector<nlohmann::json>> ruleLines;

    std::multimap<wxString, EASYEDAPRO::POURED> boardPouredMap = aPouredMap;
    std::map<wxString, ZONE*>                   poursToFill;

    BOARD_DESIGN_SETTINGS& bds = aBoard->GetDesignSettings();

    for( const nlohmann::json& line : aLines )
    {
        if( line.size() == 0 )
            continue;

        wxString type = line.at( 0 );

        if( type == wxS( "LAYER" ) )
        {
            int          layer = line.at( 1 );
            PCB_LAYER_ID klayer = LayerToKi( layer );

            wxString layerType = line.at( 2 );
            wxString layerName = line.at( 3 );
            int      layerFlag = line.at( 4 );

            if( layerFlag != 0 )
            {
                LSET blayers = aBoard->GetEnabledLayers();
                blayers.set( klayer );
                aBoard->SetEnabledLayers( blayers );
                aBoard->SetLayerName( klayer, layerName );
            }
        }
        else if( type == wxS( "NET" ) )
        {
            wxString netname = line.at( 1 );

            aBoard->Add( new NETINFO_ITEM( aBoard, netname, aBoard->GetNetCount() + 1 ),
                         ADD_MODE::APPEND );
        }
        else if( type == wxS( "RULE" ) )
        {
            wxString       ruleType = line.at( 1 );
            wxString       ruleName = line.at( 2 );
            int            isDefault = line.at( 3 );
            nlohmann::json ruleData = line.at( 4 );

            if( ruleType == wxS( "3" ) && isDefault ) // Track width
            {
                wxString units = ruleData.at( 0 );
                double   minVal = ruleData.at( 1 );
                double   optVal = ruleData.at( 2 );
                double   maxVal = ruleData.at( 3 );

                bds.m_TrackMinWidth = ScaleSize( minVal );
            }
            else if( ruleType == wxS( "1" ) && isDefault )
            {
                wxString       units = ruleData.at( 0 );
                nlohmann::json table = ruleData.at( 1 );

                int minVal = INT_MAX;
                for( const std::vector<int>& arr : table )
                {
                    for( int val : arr )
                    {
                        if( val < minVal )
                            minVal = val;
                    }
                }

                bds.m_MinClearance = ScaleSize( minVal );
            }

            ruleLines[ruleType].push_back( line );
        }
        else if( type == wxS( "POURED" ) )
        {
            if( !line.at( 2 ).is_string() )
                continue; // Unknown type of POURED

            EASYEDAPRO::POURED poured = line;
            boardPouredMap.emplace( poured.parentId, poured );
        }
        else if( type == wxS( "VIA" ) || type == wxS( "LINE" ) || type == wxS( "ARC" )
                 || type == wxS( "POLY" ) || type == wxS( "FILL" ) || type == wxS( "POUR" ) )
        {
            wxString uuid = line.at( 1 );

            // if( line.at( 2 ).is_number() )
            //     int unk = line.at( 2 ).get<int>();
            // else if( line.at( 2 ).is_string() )
            //     int unk = wxAtoi( line.at( 2 ).get<wxString>() );

            wxString netname = line.at( 3 );

            if( type == wxS( "VIA" ) )
            {
                VECTOR2D center;
                center.x = line.at( 5 );
                center.y = line.at( 6 );

                double drill = line.at( 7 );
                double dia = line.at( 8 );

                std::unique_ptr<PCB_VIA> via = std::make_unique<PCB_VIA>( aBoard );

                via->SetPosition( ScalePos( center ) );
                via->SetDrill( ScaleSize( drill ) );
                via->SetWidth( PADSTACK::ALL_LAYERS, ScaleSize( dia ) );

                via->SetNet( aBoard->FindNet( netname ) );

                aBoard->Add( via.release(), ADD_MODE::APPEND );
            }
            else if( type == wxS( "LINE" ) )
            {
                int          layer = line.at( 4 ).get<int>();
                PCB_LAYER_ID klayer = LayerToKi( layer );

                VECTOR2D start;
                start.x = line.at( 5 );
                start.y = line.at( 6 );

                VECTOR2D end;
                end.x = line.at( 7 );
                end.y = line.at( 8 );

                double width = line.at( 9 );

                std::unique_ptr<PCB_TRACK> track = std::make_unique<PCB_TRACK>( aBoard );

                track->SetLayer( klayer );
                track->SetStart( ScalePos( start ) );
                track->SetEnd( ScalePos( end ) );
                track->SetWidth( ScaleSize( width ) );

                track->SetNet( aBoard->FindNet( netname ) );

                aBoard->Add( track.release(), ADD_MODE::APPEND );
            }
            else if( type == wxS( "ARC" ) )
            {
                int          layer = line.at( 4 ).get<int>();
                PCB_LAYER_ID klayer = LayerToKi( layer );

                VECTOR2D start;
                start.x = line.at( 5 );
                start.y = line.at( 6 );

                VECTOR2D end;
                end.x = line.at( 7 );
                end.y = line.at( 8 );

                double angle = line.at( 9 );
                double width = line.at( 10 );

                VECTOR2D delta = end - start;
                VECTOR2D mid = ( start + delta / 2 );

                double   ha = angle / 2;
                double   hd = delta.EuclideanNorm() / 2;
                double   cdist = hd / tan( DEG2RAD( ha ) );
                VECTOR2D center = mid + delta.Perpendicular().Resize( cdist );

                SHAPE_ARC sarc;
                sarc.ConstructFromStartEndCenter( ScalePos( start ), ScalePos( end ),
                                                  ScalePos( center ), angle >= 0, width );

                std::unique_ptr<PCB_ARC> arc = std::make_unique<PCB_ARC>( aBoard, &sarc );
                arc->SetWidth( ScaleSize( width ) );

                arc->SetLayer( klayer );
                arc->SetNet( aBoard->FindNet( netname ) );

                aBoard->Add( arc.release(), ADD_MODE::APPEND );
            }
            else if( type == wxS( "FILL" ) )
            {
                int          layer = line.at( 4 ).get<int>();
                PCB_LAYER_ID klayer = LayerToKi( layer );

                double width = line.at( 5 );

                nlohmann::json polyDataList = line.at( 7 );

                if( !polyDataList.at( 0 ).is_array() )
                    polyDataList = nlohmann::json::array( { polyDataList } );

                std::vector<SHAPE_LINE_CHAIN> contours;
                for( nlohmann::json& polyData : polyDataList )
                {
                    SHAPE_LINE_CHAIN contour = ParseContour( polyData, true );
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
            else if( type == wxS( "POLY" ) )
            {
                int          layer = line.at( 4 );
                PCB_LAYER_ID klayer = LayerToKi( layer );

                double         thickness = line.at( 5 );
                nlohmann::json polyData = line.at( 6 );

                std::vector<std::unique_ptr<PCB_SHAPE>> results =
                        ParsePoly( aBoard, polyData, false, false );

                for( auto& shape : results )
                {
                    shape->SetLayer( klayer );
                    shape->SetWidth( ScaleSize( thickness ) );

                    aBoard->Add( shape.release(), ADD_MODE::APPEND );
                }
            }
            else if( type == wxS( "POUR" ) )
            {
                int          layer = line.at( 4 ).get<int>();
                PCB_LAYER_ID klayer = LayerToKi( layer );

                double         lineWidth = line.at( 5 ); // Doesn't matter
                wxString       pourname = line.at( 6 );
                int            fillOrder = line.at( 7 ).get<int>();
                nlohmann::json polyDataList = line.at( 8 );

                std::unique_ptr<ZONE> zone = std::make_unique<ZONE>( aBoard );

                zone->SetNet( aBoard->FindNet( netname ) );
                zone->SetLayer( klayer );
                zone->SetAssignedPriority( 500 - fillOrder );
                zone->SetLocalClearance( bds.m_MinClearance );
                zone->SetMinThickness( bds.m_TrackMinWidth );

                for( nlohmann::json& polyData : polyDataList )
                {
                    SHAPE_LINE_CHAIN contour = ParseContour( polyData, false );
                    contour.SetClosed( true );

                    zone->Outline()->Append( contour );
                }

                wxASSERT( zone->Outline()->OutlineCount() == 1 );

                poursToFill.emplace( uuid, zone.get() );

                aBoard->Add( zone.release(), ADD_MODE::APPEND );
            }
        }
        else if( type == wxS( "TEARDROP" ) )
        {
            wxString     uuid = line.at( 1 );
            wxString     netname = line.at( 2 );
            int          layer = line.at( 3 ).get<int>();
            PCB_LAYER_ID klayer = LayerToKi( layer );

            nlohmann::json polyData = line.at( 4 );

            SHAPE_LINE_CHAIN contour = ParseContour( polyData, false );
            contour.SetClosed( true );

            std::unique_ptr<ZONE> zone = std::make_unique<ZONE>( aBoard );

            zone->SetNet( aBoard->FindNet( netname ) );
            zone->SetLayer( klayer );
            zone->Outline()->Append( contour );
            zone->SetFilledPolysList( klayer, contour );
            zone->SetNeedRefill( false );
            zone->SetIsFilled( true );

            zone->SetAssignedPriority( 600 );
            zone->SetLocalClearance( 0 );
            zone->SetMinThickness( 0 );
            zone->SetTeardropAreaType( TEARDROP_TYPE::TD_UNSPECIFIED );

            aBoard->Add( zone.release(), ADD_MODE::APPEND );
        }
        else if( type == wxS( "REGION" ) )
        {
            wxString uuid = line.at( 1 );

            // if( line.at( 2 ).is_number() )
            //     int unk = line.at( 2 ).get<int>();
            // else if( line.at( 2 ).is_string() )
            //     int unk = wxAtoi( line.at( 2 ).get<wxString>() );

            int          layer = line.at( 3 ).get<int>();
            PCB_LAYER_ID klayer = LayerToKi( layer );

            double         width = line.at( 4 );
            std::set<int>  flags = line.at( 5 );
            nlohmann::json polyDataList = line.at( 6 );

            for( nlohmann::json& polyData : polyDataList )
            {
                SHAPE_LINE_CHAIN contour = ParseContour( polyData, false );
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
        else if( type == wxS( "PAD" ) )
        {
            wxString netname = line.at( 3 );

            std::unique_ptr<FOOTPRINT> footprint = std::make_unique<FOOTPRINT>( aBoard );
            std::unique_ptr<PAD>       pad = createPAD( footprint.get(), line );

            pad->SetNet( aBoard->FindNet( netname ) );

            VECTOR2I  pos = pad->GetPosition();
            EDA_ANGLE orient = pad->GetOrientation();

            pad->SetPosition( VECTOR2I() );
            pad->SetOrientation( ANGLE_0 );

            footprint->Add( pad.release(), ADD_MODE::APPEND );
            footprint->SetPosition( pos );
            footprint->SetOrientation( orient );

            wxString fpName = wxS( "Pad_" ) + line.at( 1 ).get<wxString>();
            LIB_ID   fpID = EASYEDAPRO::ToKiCadLibID( wxEmptyString, fpName );

            footprint->SetFPID( fpID );
            footprint->Reference().SetVisible( true );
            footprint->Value().SetVisible( true );
            footprint->AutoPositionFields();

            aBoard->Add( footprint.release(), ADD_MODE::APPEND );
        }
        else if( type == wxS( "IMAGE" ) )
        {
            wxString uuid = line.at( 1 );

            // if( line.at( 2 ).is_number() )
            //     int unk = line.at( 2 ).get<int>();
            // else if( line.at( 2 ).is_string() )
            //     int unk = wxAtoi( line.at( 2 ).get<wxString>() );

            int          layer = line.at( 3 ).get<int>();
            PCB_LAYER_ID klayer = LayerToKi( layer );

            VECTOR2D start( line.at( 4 ), line.at( 5 ) );
            VECTOR2D size( line.at( 6 ), line.at( 7 ) );

            double         angle = line.at( 8 ); // from top left corner
            int            mirror = line.at( 9 );
            nlohmann::json polyDataList = line.at( 10 );

            BOX2I                         bbox;
            std::vector<SHAPE_LINE_CHAIN> contours;
            for( nlohmann::json& polyData : polyDataList )
            {
                SHAPE_LINE_CHAIN contour = ParseContour( polyData, false );
                contour.SetClosed( true );

                contours.push_back( contour );

                bbox.Merge( contour.BBox() );
            }

            VECTOR2D scale( ScaleSize( size.x ) / bbox.GetSize().x,
                            ScaleSize( size.y ) / bbox.GetSize().y );

            SHAPE_POLY_SET polySet;

            for( SHAPE_LINE_CHAIN& contour : contours )
            {
                for( int i = 0; i < contour.PointCount(); i++ )
                {
                    VECTOR2I pt = contour.CPoint( i );
                    contour.SetPoint( i, VECTOR2I( pt.x * scale.x, pt.y * scale.y ) );
                }

                polySet.AddOutline( contour );
            }

            polySet.RebuildHolesFromContours();

            std::unique_ptr<PCB_GROUP> group;

            if( polySet.OutlineCount() > 1 )
                group = std::make_unique<PCB_GROUP>( aBoard );

            BOX2I polyBBox = polySet.BBox();

            for( const SHAPE_POLY_SET::POLYGON& poly : polySet.CPolygons() )
            {
                std::unique_ptr<PCB_SHAPE> shape =
                        std::make_unique<PCB_SHAPE>( aBoard, SHAPE_T::POLY );

                shape->SetFilled( true );
                shape->SetPolyShape( poly );
                shape->SetLayer( klayer );
                shape->SetWidth( 0 );

                shape->Move( ScalePos( start ) - polyBBox.GetOrigin() );
                shape->Rotate( ScalePos( start ), EDA_ANGLE( angle, DEGREES_T ) );

                if( IsBackLayer( klayer ) ^ !!mirror )
                {
                    FLIP_DIRECTION flipDirection = IsBackLayer( klayer )
                                                           ? FLIP_DIRECTION::TOP_BOTTOM
                                                           : FLIP_DIRECTION::LEFT_RIGHT;
                    shape->Mirror( ScalePos( start ), flipDirection );
                }

                if( group )
                    group->AddItem( shape.get() );

                aBoard->Add( shape.release(), ADD_MODE::APPEND );
            }

            if( group )
                aBoard->Add( group.release(), ADD_MODE::APPEND );
        }
        else if( type == wxS( "OBJ" ) )
        {
            VECTOR2D start, size;
            wxString mimeType, base64Data;
            double   angle = 0;
            int      flipped = 0;

            if( !line.at( 3 ).is_number() )
                continue;

            int          layer = line.at( 3 ).get<int>();
            PCB_LAYER_ID klayer = LayerToKi( layer );

            start = VECTOR2D( line.at( 5 ), line.at( 6 ) );
            size = VECTOR2D( line.at( 7 ), line.at( 8 ) );
            angle = line.at( 9 );
            flipped = line.at( 10 );

            wxString imageUrl = line.at( 11 );

            if( imageUrl.BeforeFirst( ':' ) == wxS( "blob" ) )
            {
                wxString objectId = imageUrl.AfterLast( ':' );

                if( auto blob = get_opt( aBlobMap, objectId ) )
                {
                    wxString blobUrl = blob->url;

                    if( blobUrl.BeforeFirst( ':' ) == wxS( "data" ) )
                    {
                        wxArrayString paramsArr =
                                wxSplit( blobUrl.AfterFirst( ':' ).BeforeFirst( ',' ), ';', '\0' );

                        base64Data = blobUrl.AfterFirst( ',' );

                        if( paramsArr.size() > 0 )
                            mimeType = paramsArr[0];
                    }
                }
            }

            VECTOR2D kstart = ScalePos( start );
            VECTOR2D ksize = ScaleSize( size );

            if( mimeType.empty() || base64Data.empty() )
                continue;

            wxMemoryBuffer buf = wxBase64Decode( base64Data );

            if( mimeType == wxS( "image/svg+xml" ) )
            {
                // Not yet supported by EasyEDA
            }
            else
            {
                VECTOR2D kcenter = kstart + ksize / 2;

                std::unique_ptr<PCB_REFERENCE_IMAGE> bitmap =
                        std::make_unique<PCB_REFERENCE_IMAGE>( aBoard, kcenter, klayer );
                REFERENCE_IMAGE& refImage = bitmap->GetReferenceImage();

                wxImage::SetDefaultLoadFlags( wxImage::GetDefaultLoadFlags()
                                              & ~wxImage::Load_Verbose );

                if( refImage.ReadImageFile( buf ) )
                {
                    double scaleFactor = ScaleSize( size.x ) / refImage.GetSize().x;
                    refImage.SetImageScale( scaleFactor );

                    // TODO: support non-90-deg angles
                    bitmap->Rotate( kstart, EDA_ANGLE( angle, DEGREES_T ) );

                    if( flipped )
                    {
                        int x = bitmap->GetPosition().x;
                        MIRROR( x, KiROUND( kstart.x ) );
                        bitmap->SetX( x );

                        refImage.MutableImage().Mirror( FLIP_DIRECTION::LEFT_RIGHT );
                    }

                    aBoard->Add( bitmap.release(), ADD_MODE::APPEND );
                }
            }
        }
        else if( type == wxS( "STRING" ) )
        {
            wxString uuid = line.at( 1 );

            // if( line.at( 2 ).is_number() )
            //     int unk = line.at( 2 ).get<int>();
            // else if( line.at( 2 ).is_string() )
            //     int unk = wxAtoi( line.at( 2 ).get<wxString>() );

            int          layer = line.at( 3 ).get<int>();
            PCB_LAYER_ID klayer = LayerToKi( layer );

            VECTOR2D location( line.at( 4 ), line.at( 5 ) );
            wxString string = line.at( 6 );
            wxString font = line.at( 7 );

            double height = line.at( 8 );
            double strokew = line.at( 9 );

            int    align = line.at( 12 );
            double angle = line.at( 13 );
            int    inverted = line.at( 14 );
            int    mirror = line.at( 16 );

            PCB_TEXT* text = new PCB_TEXT( aBoard );

            text->SetText( string );
            text->SetLayer( klayer );
            text->SetPosition( ScalePos( location ) );
            text->SetIsKnockout( inverted );
            text->SetTextThickness( ScaleSize( strokew ) );
            text->SetTextSize( VECTOR2D( ScaleSize( height * 0.6 ), ScaleSize( height * 0.7 ) ) );

            if( font != wxS( "default" ) )
            {
                text->SetFont( KIFONT::FONT::GetFont( font ) );
                //text->SetupRenderCache( text->GetShownText(), text->GetFont(), EDA_ANGLE( angle, DEGREES_T ) );

                //text->AddRenderCacheGlyph();
                // TODO: import geometry cache
            }

            AlignText( text, align );

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
        else if( type == wxS( "COMPONENT" ) )
        {
            wxString compId = line.at( 1 );
            componentLines[compId].push_back( line );
        }
        else if( type == wxS( "ATTR" ) )
        {
            wxString compId = line.at( 3 );
            componentLines[compId].push_back( line );
        }
        else if( type == wxS( "PAD_NET" ) )
        {
            wxString compId = line.at( 1 );
            componentLines[compId].push_back( line );
        }
    }

    for( auto const& [compId, lines] : componentLines )
    {
        wxString                     deviceId;
        wxString                     fpIdOverride;
        wxString                     fpDesignator;
        std::map<wxString, wxString> localCompAttribs;

        for( auto& line : lines )
        {
            if( line.size() == 0 )
                continue;

            wxString type = line.at( 0 );

            if( type == wxS( "COMPONENT" ) )
            {
                localCompAttribs = line.at( 7 );
            }
            else if( type == wxS( "ATTR" ) )
            {
                EASYEDAPRO::PCB_ATTR attr = line;

                if( attr.key == wxS( "Device" ) )
                    deviceId = attr.value;

                else if( attr.key == wxS( "Footprint" ) )
                    fpIdOverride = attr.value;

                else if( attr.key == wxS( "Designator" ) )
                    fpDesignator = attr.value;
            }
        }

        if( deviceId.empty() )
            continue;

        nlohmann::json compAttrs = aProject.at( "devices" ).at( deviceId ).at( "attributes" );

        wxString fpId;

        if( !fpIdOverride.IsEmpty() )
            fpId = fpIdOverride;
        else
            fpId = compAttrs.at( "Footprint" ).get<wxString>();

        auto it = aFootprintMap.find( fpId );
        if( it == aFootprintMap.end() )
        {
            wxLogError( "Footprint of '%s' with uuid '%s' not found.", fpDesignator, fpId );
            continue;
        }

        std::unique_ptr<FOOTPRINT>& footprintOrig = it->second;
        std::unique_ptr<FOOTPRINT>  footprint( static_cast<FOOTPRINT*>( footprintOrig->Clone() ) );

        wxString modelUuid, modelTitle, modelTransform;

        if( auto val = get_opt( localCompAttribs, "3D Model" ) )
            modelUuid = *val;
        else
            modelUuid = compAttrs.value<wxString>( "3D Model", "" );

        if( auto val = get_opt( localCompAttribs, "3D Model Title" ) )
            modelTitle = val->Trim();
        else
            modelTitle = compAttrs.value<wxString>( "3D Model Title", modelUuid ).Trim();

        if( auto val = get_opt( localCompAttribs, "3D Model Transform" ) )
            modelTransform = *val;
        else
            modelTransform = compAttrs.value<wxString>( "3D Model Transform", "" );

        fillFootprintModelInfo( footprint.get(), modelUuid, modelTitle, modelTransform );

        footprint->SetParent( aBoard );

        for( auto& line : lines )
        {
            if( line.size() == 0 )
                continue;

            wxString type = line.at( 0 );

            if( type == wxS( "COMPONENT" ) )
            {
                int          layer = line.at( 3 );
                PCB_LAYER_ID klayer = LayerToKi( layer );

                VECTOR2D center( line.at( 4 ), line.at( 5 ) );

                double                       orient = line.at( 6 );
                //std::map<wxString, wxString> props = line.at( 7 );

                if( klayer == B_Cu )
                    footprint->Flip( footprint->GetPosition(), FLIP_DIRECTION::TOP_BOTTOM );

                footprint->SetOrientationDegrees( orient );
                footprint->SetPosition( ScalePos( center ) );
            }
            else if( type == wxS( "ATTR" ) )
            {
                EASYEDAPRO::PCB_ATTR attr = line;

                PCB_LAYER_ID klayer = LayerToKi( attr.layer );

                PCB_FIELD* field = nullptr;

                if( attr.key == wxS( "Designator" ) )
                {
                    if( attr.key == wxS( "Designator" ) )
                    {
                        field = footprint->GetField( FIELD_T::REFERENCE );
                    }
                    else
                    {
                        field = new PCB_FIELD( footprint.get(), FIELD_T::USER, attr.key );
                        footprint->Add( field, ADD_MODE::APPEND );
                    }

                    if( attr.fontName != wxS( "default" ) )
                        field->SetFont( KIFONT::FONT::GetFont( attr.fontName ) );

                    if( attr.valVisible && attr.keyVisible )
                    {
                        field->SetText( attr.key + ':' + attr.value );
                    }
                    else if( attr.keyVisible )
                    {
                        field->SetText( attr.key );
                    }
                    else
                    {
                        field->SetVisible( false );
                        field->SetText( attr.value );
                    }

                    field->SetLayer( klayer );
                    field->SetPosition( ScalePos( attr.position ) );
                    field->SetTextAngleDegrees( footprint->IsFlipped() ? -attr.rotation
                                                                       : attr.rotation );
                    field->SetIsKnockout( attr.inverted );
                    field->SetTextThickness( ScaleSize( attr.strokeWidth ) );
                    field->SetTextSize( VECTOR2D( ScaleSize( attr.height * 0.55 ),
                                                  ScaleSize( attr.height * 0.6 ) ) );

                    AlignText( field, attr.textOrigin );
                }
            }
            else if( type == wxS( "PAD_NET" ) )
            {
                wxString padNumber = line.at( 2 );
                wxString padNet = line.at( 3 );

                PAD* pad = footprint->FindPadByNumber( padNumber );
                if( pad )
                {
                    pad->SetNet( aBoard->FindNet( padNet ) );
                }
                else
                {
                    // Not a pad
                }
            }
        }

        aBoard->Add( footprint.release(), ADD_MODE::APPEND );
    }

    // Set zone fills
    if( EASYEDAPRO::IMPORT_POURED )
    {
        for( auto& [uuid, zone] : poursToFill )
        {
            SHAPE_POLY_SET fillPolySet;
            SHAPE_POLY_SET thermalSpokes;

            auto range = boardPouredMap.equal_range( uuid );
            for( auto& it = range.first; it != range.second; ++it )
            {
                const EASYEDAPRO::POURED& poured = it->second;
                int                       unki = poured.unki;

                SHAPE_POLY_SET thisPoly;

                for( int dataId = 0; dataId < poured.polyData.size(); dataId++ )
                {
                    const nlohmann::json& fillData = poured.polyData[dataId];
                    const double          ptScale = 10;

                    SHAPE_LINE_CHAIN contour = ParseContour( fillData, false, ARC_HIGH_DEF / ptScale );

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
                        const int thermalWidth = pcbIUScale.mmToIU( 0.2 ); // Generic

                        for( int segId = 0; segId < contour.SegmentCount(); segId++ )
                        {
                            const SEG& seg = contour.CSegment( segId );

                            TransformOvalToPolygon( thermalSpokes, seg.A, seg.B, thermalWidth,
                                                    ARC_HIGH_DEF, ERROR_INSIDE );
                        }
                    }
                }

                fillPolySet.Append( thisPoly );
            }

            if( !fillPolySet.IsEmpty() )
            {
                fillPolySet.Simplify();

                const int strokeWidth = pcbIUScale.MilsToIU( 8 ); // Seems to be 8 mils

                fillPolySet.Inflate( strokeWidth / 2, CORNER_STRATEGY::ROUND_ALL_CORNERS,
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
