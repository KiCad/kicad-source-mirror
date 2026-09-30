/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Author: SYSUEric <jzzhuang666@gmail.com>.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "odb_feature.h"

#include <sstream>
#include <map>

#include <wx/log.h>

#include "footprint.h"
#include "pad.h"
#include "pcb_shape.h"
#include "odb_defines.h"
#include "pcb_track.h"
#include "pcb_textbox.h"
#include "pcb_table.h"
#include "pcb_barcode.h"
#include "pcb_dimension.h"
#include "zone.h"
#include "board.h"
#include "board_design_settings.h"
#include <drill/drill_enumerator.h>
#include <drill/drill_operation.h>
#include "geometry/eda_angle.h"
#include "geometry/shape_circle.h"
#include "geometry/shape_ellipse.h"
#include "geometry/shape_line_chain.h"
#include "geometry/shape_segment.h"
#include "geometry/shape_poly_set.h"
#include "odb_eda_data.h"
#include <exporters/fab_model/fab_pin.h>
#include <exporters/fab_model/fab_drill_model.h>
#include <exporters/fab_model/fab_layer_index.h>
#include <exporters/fab_model/fab_pad_geometry.h>
#include "pcb_io_odbpp.h"
#include <callback_gal.h>
#include <string_utils.h>
#include <trace_helpers.h>


void FEATURES_MANAGER::AddFeatureLine( const VECTOR2I& aStart, const VECTOR2I& aEnd,
                                       uint64_t aWidth )
{
    AddFeature<ODB_LINE>( ODB::AddXY( m_plugin->GetFormat(), aStart ), ODB::AddXY( m_plugin->GetFormat(), aEnd ),
                          AddCircleSymbol( ODB::SymDouble2String( m_plugin->GetFormat(), aWidth ) ) );
}


void FEATURES_MANAGER::AddFeatureArc( const VECTOR2I& aStart, const VECTOR2I& aEnd,
                                      const VECTOR2I& aCenter, uint64_t aWidth,
                                      ODB_DIRECTION aDirection )
{
    const ODB_FORMAT& fmt = m_plugin->GetFormat();

    AddFeature<ODB_ARC>( ODB::AddXY( fmt, aStart ),
                         ODB::AddXY( fmt, aEnd ),
                         ODB::AddXY( fmt, aCenter ),
                         AddCircleSymbol( ODB::SymDouble2String( fmt, aWidth ) ),
                         aDirection );
}


void FEATURES_MANAGER::AddRoutContour( const SHAPE_LINE_CHAIN& aContour, int aChain, bool aPlated )
{
    size_t first = FeatureCount();
    int at = 0;

    if( aContour.PointCount() < 2 )
        return;

    do
    {
        int next = aContour.NextShape( at );

        if( next < 0 )
            next = 0;

        if( aContour.IsArcStart( at ) )
        {
            const SHAPE_ARC& arc = aContour.Arc( aContour.ArcIndex( at ) );
            AddFeatureArc( arc.GetP0(), arc.GetP1(), arc.GetCenter(), 0,
                           arc.IsClockwise() ? ODB_DIRECTION::CCW : ODB_DIRECTION::CW );
        }
        else if( aContour.CPoint( at ) != aContour.CPoint( next ) )
        {
            AddFeatureLine( aContour.CPoint( at ), aContour.CPoint( next ), 0 );
        }

        at = next;
    } while( at != 0 );

    tagRoutFeatures( first, aChain, aPlated );
}


void FEATURES_MANAGER::AddRoutSlot( const PAD& aPad, int aChain )
{
    std::shared_ptr<SHAPE_SEGMENT> hole = aPad.GetEffectiveHoleShape();

    if( !hole || hole->GetSeg().A == hole->GetSeg().B )
        return;

    size_t first = FeatureCount();
    AddFeatureLine( hole->GetSeg().A, hole->GetSeg().B, hole->GetWidth() );

    tagRoutFeatures( first, aChain, aPad.GetAttribute() == PAD_ATTRIB::PTH );
}


void FEATURES_MANAGER::tagRoutFeatures( size_t aFirst, int aChain, bool aPlated )
{
    forEachNewFeature( aFirst,
                       [&]( ODB_FEATURE& aFeature, size_t )
                       {
                           AddSystemAttribute( aFeature, ODB_ATTR::ROUT_CHAIN{ aChain } );

                           if( aPlated )
                               AddSystemAttribute( aFeature, ODB_ATTR::ROUT_PLATED{ true } );
                       } );
}


void FEATURES_MANAGER::AddPadCircle( const VECTOR2I& aCenter, uint64_t aDiameter )
{
    AddFeature<ODB_PAD>( ODB::AddXY( m_plugin->GetFormat(), aCenter ),
                         AddCircleSymbol( ODB::SymDouble2String( m_plugin->GetFormat(), aDiameter ) ) );
}


bool FEATURES_MANAGER::AddContour( const SHAPE_POLY_SET& aPolySet, int aOutline /*= 0*/,
                                   FILL_T aFillType /*= FILL_T::FILLED_SHAPE*/ )
{
    // todo: args modify aPolySet.Polygon( aOutline ) instead of aPolySet

    if( aPolySet.OutlineCount() < ( aOutline + 1 ) )
        return false;

    return AddFeatureSurface( aPolySet.Polygon( aOutline ), aFillType );
}


void FEATURES_MANAGER::AddShape( const PCB_SHAPE& aShape, PCB_LAYER_ID aLayer )
{
    const ODB_FORMAT& fmt = m_plugin->GetFormat();

    int stroke_width = aShape.GetWidth();

    switch( aShape.GetShape() )
    {
    case SHAPE_T::CIRCLE:
    {
        // GetRadius() can reach INT_MAX / 2 rounded up, which overflows a signed int when doubled
        int64_t  diameter = static_cast<int64_t>( aShape.GetRadius() ) * 2;
        VECTOR2I center = ODB::GetShapePosition( aShape );

        // The stroke straddles the radius, so in diameter terms the whole width comes off the
        // inner edge and goes onto the outer
        int64_t  innerDiameter = diameter - stroke_width;
        wxString outerDim = ODB::SymDouble2String( fmt, diameter + stroke_width );

        // donut_r has no spelling for a hole closed by its own stroke
        if( aShape.IsSolidFill() || innerDiameter <= 0 )
        {
            AddFeature<ODB_PAD>( ODB::AddXY( fmt, center ), AddCircleSymbol( outerDim ) );
        }
        else
        {
            AddFeature<ODB_PAD>( ODB::AddXY( fmt, center ),
                                 AddRoundDonutSymbol( outerDim,
                                                      ODB::SymDouble2String( fmt, innerDiameter ) ) );
        }

        break;
    }

    case SHAPE_T::RECTANGLE:
    {
        // ODB++ donut_rc symbols degenerate when the corner radius is smaller than half the
        // line width, and some viewers drop the feature entirely.  Emit the rectangle as a
        // filled pad for the fill (if any) plus four line segments for the stroke, matching
        // how a rectangle drawn with the line tool is exported.
        if( aShape.IsSolidFill() )
        {
            int      width = std::abs( aShape.GetRectangleWidth() );
            int      height = std::abs( aShape.GetRectangleHeight() );
            VECTOR2I center = ODB::GetShapePosition( aShape );

            AddFeature<ODB_PAD>( ODB::AddXY( fmt, center ),
                                 AddRectSymbol( ODB::SymDouble2String( fmt, width ),
                                                ODB::SymDouble2String( fmt, height ) ) );
        }

        if( stroke_width > 0 )
        {
            std::vector<VECTOR2I> corners = aShape.GetRectCorners();

            for( size_t ii = 0; ii < corners.size(); ++ii )
                AddFeatureLine( corners[ii], corners[( ii + 1 ) % corners.size()], stroke_width );
        }

        break;
    }

    case SHAPE_T::POLY:
    {
        int soldermask_min_thickness = 0;

        // TODO: check if soldermask_min_thickness should be Stroke width

        if( aLayer != UNDEFINED_LAYER && LSET( { F_Mask, B_Mask } ).Contains( aLayer ) )
            soldermask_min_thickness = stroke_width;

        int            maxError = m_board->GetDesignSettings().m_MaxError;
        SHAPE_POLY_SET poly_set;

        if( soldermask_min_thickness == 0 )
        {
            poly_set = aShape.GetPolyShape().CloneDropTriangulation();
            poly_set.Fracture();
        }
        else
        {
            SHAPE_POLY_SET initialPolys;

            // add shapes inflated by aMinThickness/2 in areas
            aShape.TransformShapeToPolygon( initialPolys, aLayer, 0, maxError, ERROR_OUTSIDE );
            aShape.TransformShapeToPolygon( poly_set, aLayer, soldermask_min_thickness / 2 - 1,
                                            maxError, ERROR_OUTSIDE );

            poly_set.Simplify();
            poly_set.Deflate( soldermask_min_thickness / 2 - 1,
                              CORNER_STRATEGY::CHAMFER_ALL_CORNERS, maxError );
            poly_set.BooleanAdd( initialPolys );
            poly_set.Fracture();
        }

        // ODB++ surface features can only represent closed polygons.  We add a surface for
        // the fill of the shape, if present, and add line segments for the outline, if present.
        if( aShape.IsSolidFill() )
        {
            for( int ii = 0; ii < poly_set.OutlineCount(); ++ii )
            {
                AddContour( poly_set, ii, FILL_T::FILLED_SHAPE );

                if( stroke_width != 0 )
                {
                    for( int jj = 0; jj < poly_set.COutline( ii ).SegmentCount(); ++jj )
                    {
                        const SEG& seg = poly_set.COutline( ii ).CSegment( jj );
                        AddFeatureLine( seg.A, seg.B, stroke_width );
                    }
                }
            }
        }
        else
        {
            for( int ii = 0; ii < poly_set.OutlineCount(); ++ii )
            {
                for( int jj = 0; jj < poly_set.COutline( ii ).SegmentCount(); ++jj )
                {
                    const SEG& seg = poly_set.COutline( ii ).CSegment( jj );
                    AddFeatureLine( seg.A, seg.B, stroke_width );
                }
            }
        }

        break;
    }

    case SHAPE_T::ARC:
    {
        ODB_DIRECTION dir = !aShape.IsClockwiseArc() ? ODB_DIRECTION::CW : ODB_DIRECTION::CCW;

        AddFeatureArc( aShape.GetStart(), aShape.GetEnd(), aShape.GetCenter(), stroke_width, dir );
        break;
    }

    case SHAPE_T::BEZIER:
    {
        const std::vector<VECTOR2I>& points = aShape.GetBezierPoints();

        for( size_t i = 0; i < points.size() - 1; i++ )
            AddFeatureLine( points[i], points[i + 1], stroke_width );

        break;
    }

    case SHAPE_T::SEGMENT:
        AddFeatureLine( aShape.GetStart(), aShape.GetEnd(), stroke_width );
        break;

    case SHAPE_T::ELLIPSE:
    {
        int maxError = m_board->GetDesignSettings().m_MaxError;

        SHAPE_ELLIPSE e( aShape.GetEllipseCenter(), aShape.GetEllipseMajorRadius(), aShape.GetEllipseMinorRadius(),
                         aShape.GetEllipseRotation() );

        SHAPE_LINE_CHAIN chain = e.ConvertToPolyline( maxError );
        chain.SetClosed( true );

        if( aShape.IsSolidFill() )
        {
            SHAPE_POLY_SET poly_set;
            poly_set.AddOutline( chain );
            poly_set.Fracture();

            for( int ii = 0; ii < poly_set.OutlineCount(); ++ii )
                AddContour( poly_set, ii, FILL_T::FILLED_SHAPE );
        }

        if( stroke_width > 0 )
        {
            for( int ii = 0; ii < chain.SegmentCount(); ++ii )
            {
                const SEG& seg = chain.CSegment( ii );
                AddFeatureLine( seg.A, seg.B, stroke_width );
            }
        }

        break;
    }

    case SHAPE_T::ELLIPSE_ARC:
    {
        int maxError = m_board->GetDesignSettings().m_MaxError;

        SHAPE_ELLIPSE e( aShape.GetEllipseCenter(), aShape.GetEllipseMajorRadius(), aShape.GetEllipseMinorRadius(),
                         aShape.GetEllipseRotation(), aShape.GetEllipseStartAngle(), aShape.GetEllipseEndAngle() );

        SHAPE_LINE_CHAIN chain = e.ConvertToPolyline( maxError );

        for( int ii = 0; ii < chain.SegmentCount(); ++ii )
        {
            const SEG& seg = chain.CSegment( ii );
            AddFeatureLine( seg.A, seg.B, stroke_width );
        }

        break;
    }

    default:
        wxLogTrace( traceOdbppIo, wxT( "Unknown shape when adding ODB++ layer feature" ) );
        break;
    }

    if( aShape.IsHatchedFill() )
    {
        for( int ii = 0; ii < aShape.GetHatching().OutlineCount(); ++ii )
            AddContour( aShape.GetHatching(), ii, FILL_T::FILLED_SHAPE );
    }
}


bool FEATURES_MANAGER::AddFeatureSurface( const SHAPE_POLY_SET::POLYGON& aPolygon,
                                          FILL_T                         aFillType /*= FILL_T::FILLED_SHAPE */ )
{
    if( aPolygon.empty() || aPolygon[0].PointCount() < 3 || aPolygon[0].Area( false ) == 0.0 )
        return false;

    AddFeature<ODB_SURFACE>( aPolygon, aFillType );
    return true;
}


void FEATURES_MANAGER::AddPadShape( const PAD& aPad, PCB_LAYER_ID aLayer )
{
    const ODB_FORMAT& fmt = m_plugin->GetFormat();
    PAD_LAYER_GEOMETRY geometry = ResolvePadLayer( aPad, aLayer, m_board->GetDesignSettings().m_MaxError );

    if( geometry.IsEmpty() )
        return;

    const PAD& resolved = geometry.Pad();
    VECTOR2I   plotSize = resolved.GetSize( aLayer );
    VECTOR2I   center = resolved.ShapePos( aLayer );

    wxString width = ODB::SymDouble2String( fmt, std::abs( plotSize.x ) );
    wxString height = ODB::SymDouble2String( fmt, std::abs( plotSize.y ) );

    switch( aPad.GetShape( aLayer ) )
    {
    case PAD_SHAPE::CIRCLE:
    {
        wxString diam = ODB::SymDouble2String( fmt, plotSize.x );

        AddFeature<ODB_PAD>( ODB::AddXY( fmt, center ), AddCircleSymbol( diam ),
                             aPad.GetOrientation() );

        break;
    }
    case PAD_SHAPE::RECTANGLE:
    {
        if( geometry.Margin().x > 0 )
        {
            wxString rad = ODB::SymDouble2String( fmt, geometry.CornerRadius() );

            AddFeature<ODB_PAD>( ODB::AddXY( fmt, center ), AddRoundRectSymbol( width, height, rad ),
                                 aPad.GetOrientation() );
        }
        else
        {
            AddFeature<ODB_PAD>( ODB::AddXY( fmt, center ),
                                 AddRectSymbol( width, height ), aPad.GetOrientation() );
        }

        break;
    }
    case PAD_SHAPE::OVAL:
    {
        AddFeature<ODB_PAD>( ODB::AddXY( fmt, center ), AddOvalSymbol( width, height ),
                             aPad.GetOrientation() );
        break;
    }
    case PAD_SHAPE::ROUNDRECT:
    {
        wxString rad = ODB::SymDouble2String( fmt, geometry.CornerRadius() );

        AddFeature<ODB_PAD>( ODB::AddXY( fmt, center ),
                             AddRoundRectSymbol( width, height, rad ), aPad.GetOrientation() );

        break;
    }
    case PAD_SHAPE::CHAMFERED_RECT:
    {
        if( geometry.Margin().x > 0 || resolved.GetRoundRectCornerRadius( aLayer ) > 0 )
        {
            SHAPE_POLY_SET outline = geometry.Polygon();

            for( int ii = 0; ii < outline.OutlineCount(); ++ii )
                AddContour( outline, ii );

            break;
        }

        int shorterSide = std::min( plotSize.x, plotSize.y );
        int chamfer = std::max(
                0, KiROUND( aPad.GetChamferRectRatio( aLayer ) * shorterSide ) );
        wxString rad = ODB::SymDouble2String( fmt, chamfer );
        int      positions = aPad.GetChamferPositions( aLayer );

        AddFeature<ODB_PAD>( ODB::AddXY( fmt, center ),
                             AddChamferRectSymbol( width, height, rad, positions ), aPad.GetOrientation() );

        break;
    }
    case PAD_SHAPE::TRAPEZOID:
    {
        SHAPE_POLY_SET outline = geometry.Polygon();

        for( int ii = 0; ii < outline.OutlineCount(); ++ii )
            AddContour( outline, ii );

        break;
    }
    case PAD_SHAPE::CUSTOM:
    {
        SHAPE_POLY_SET shape = geometry.Polygon();

        for( int ii = 0; ii < shape.OutlineCount(); ++ii )
            AddContour( shape, ii );

        break;
    }
    default: wxLogTrace( traceOdbppIo, wxT( "Unknown pad type" ) ); break;
    }
}


void FEATURES_MANAGER::addPostMachiningAttributes( ODB_FEATURE& aFeature,
                                                   const DRILL_OPERATION& aOperation )
{
    const ODB_FORMAT& format = m_plugin->GetFormat();
    auto addSide = [&]( const DRILL_POST_MACHINING& aPm, auto aCountersink, auto aCounterbore,
                        auto aDiameter, auto aDepth, auto aAngle )
    {
        if( aPm.m_Size <= 0 || ( aPm.m_Mode != PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK
                               && aPm.m_Mode != PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE ) )
        {
            return;
        }

        bool sink = aPm.m_Mode == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK;
        AddUserDefAttribute( aFeature, sink ? aCountersink : aCounterbore );
        AddUserDefAttribute( aFeature, decltype( aDiameter ){ format.m_symbolScale * aPm.m_Size } );

        if( aPm.m_Depth > 0 )
            AddUserDefAttribute( aFeature, decltype( aDepth ){ format.m_symbolScale * aPm.m_Depth } );

        if( sink && aPm.m_Angle > 0 )
            AddUserDefAttribute( aFeature, decltype( aAngle ){ aPm.m_Angle / 10.0 } );
    };

    addSide( aOperation.m_FrontPostMachining, ODB_ATTR::post_machining_top::COUNTERSINK,
             ODB_ATTR::post_machining_top::COUNTERBORE, ODB_ATTR::post_machining_top_diameter{ 0.0 },
             ODB_ATTR::post_machining_top_depth{ 0.0 }, ODB_ATTR::post_machining_top_angle{ 0.0 } );
    addSide( aOperation.m_BackPostMachining, ODB_ATTR::post_machining_bottom::COUNTERSINK,
             ODB_ATTR::post_machining_bottom::COUNTERBORE, ODB_ATTR::post_machining_bottom_diameter{ 0.0 },
             ODB_ATTR::post_machining_bottom_depth{ 0.0 }, ODB_ATTR::post_machining_bottom_angle{ 0.0 } );
}


void FEATURES_MANAGER::addViaDrillAttributes( ODB_FEATURE& aFeature, const PCB_VIA* aVia )
{
    if( m_role != ODB_LAYER_ROLE::DRILL || !m_drillSpan )
        return;

    const DRILL_OPERATION* op = m_plugin->GetFabDrillModel().Find( *m_drillSpan, aVia );

    if( !op )
        return;

    int topType = ODB::IpcViaType( *op, true );
    int bottomType = ODB::IpcViaType( *op, false );

    if( topType )
        AddSystemAttribute( aFeature, ODB_ATTR::IPC_VIA_TYPE_TOP{ std::to_string( topType ) } );

    if( bottomType )
        AddSystemAttribute( aFeature, ODB_ATTR::IPC_VIA_TYPE_BOTTOM{ std::to_string( bottomType ) } );

    if( ODB::ViaInPad( *aVia ) )
        AddSystemAttribute( aFeature, ODB_ATTR::VIA_IN_PAD{ true } );

    addPostMachiningAttributes( aFeature, *op );
}


void FEATURES_MANAGER::AddTrack( PCB_LAYER_ID aLayer, PCB_TRACK* track )
{
    auto iter = GetODBPlugin()->GetViaTraceSubnetMap().find( track );

    if( iter == GetODBPlugin()->GetViaTraceSubnetMap().end() )
    {
        wxLogTrace( traceOdbppIo, wxT( "Failed to get subnet track data" ) );
        return;
    }

    auto   subnet = iter->second;
    size_t first = FeatureCount();

    auto linkCopper = [&]( ODB_FEATURE&, size_t index )
    {
        subnet->AddFeatureID( EDA_DATA::FEATURE_ID::TYPE::COPPER, m_layerName, index );
    };

    if( track->Type() == PCB_TRACE_T )
    {
        PCB_SHAPE shape( nullptr, SHAPE_T::SEGMENT );
        shape.SetStart( track->GetStart() );
        shape.SetEnd( track->GetEnd() );
        shape.SetWidth( FabTrackWidth( *track, aLayer ) );

        AddShape( shape );

        if( IsCopperLayer( aLayer ) )
            forEachNewFeature( first, linkCopper );
    }
    else if( track->Type() == PCB_ARC_T )
    {
        const PCB_ARC* arc = static_cast<const PCB_ARC*>( track );

        // Too small arcs cannot be really handled: arc center (and arc radius)
        // cannot be safely computed
        if( !arc->IsDegenerated( 10 /* in IU */ ) )
        {
            PCB_SHAPE shape( nullptr, SHAPE_T::ARC );
            shape.SetArcGeometry( arc->GetStart(), arc->GetMid(), arc->GetEnd() );
            shape.SetWidth( FabTrackWidth( *arc, aLayer ) );

            AddShape( shape );
        }
        else
        {
            // Approximate this very small arc by a segment.
            AddFeatureLine( track->GetStart(), track->GetEnd(), FabTrackWidth( *track, aLayer ) );
        }


        if( IsCopperLayer( aLayer ) )
            forEachNewFeature( first, linkCopper );
    }
    else
    {
        // add via
        PCB_VIA* via = static_cast<PCB_VIA*>( track );

        bool hole = false;

        if( m_role == ODB_LAYER_ROLE::VIA_PROTECTION )
        {
            hole = m_auxType == ODB_AUX_LAYER_TYPE::PLUGGING || m_auxType == ODB_AUX_LAYER_TYPE::FILLING
                   || m_auxType == ODB_AUX_LAYER_TYPE::CAPPING;
        }
        else
        {
            hole = m_role == ODB_LAYER_ROLE::DRILL || m_role == ODB_LAYER_ROLE::BACKDRILL;
        }

        if( hole )
        {
            if( !AddViaDrillHole( via, aLayer ) )
                return;

            // TODO: confirm TOOLING_HOLE
            // AddSystemAttribute( *m_featuresList.back(), ODB_ATTR::PAD_USAGE::TOOLING_HOLE );

            forEachNewFeature(
                    first,
                    [&]( ODB_FEATURE& feature, size_t index )
                    {
                        subnet->AddFeatureID( EDA_DATA::FEATURE_ID::TYPE::HOLE, m_layerName, index );
                        AddSystemAttribute( feature, m_role == ODB_LAYER_ROLE::BACKDRILL ? ODB_ATTR::DRILL::NON_PLATED
                                                                                         : ODB_ATTR::DRILL::VIA );

                        addViaDrillAttributes( feature, via );

                        if( via->GetViaType() == VIATYPE::MICROVIA && m_role == ODB_LAYER_ROLE::DRILL )
                            AddSystemAttribute( feature, ODB_ATTR::VIA_TYPE::LASER );

                        AddSystemAttribute(
                                feature,
                                ODB_ATTR::GEOMETRY{ "VIA_RoundD" + std::to_string( via->GetWidth( aLayer ) ) } );
                    } );
        }
        else
        {
            // to draw via copper shape on copper layer
            AddVia( via, aLayer );

            forEachNewFeature( first,
                               [&]( ODB_FEATURE& feature, size_t index )
                               {
                                   if( m_role == ODB_LAYER_ROLE::BOARD_LAYER && IsCopperLayer( aLayer ) )
                                       subnet->AddFeatureID( EDA_DATA::FEATURE_ID::TYPE::COPPER, m_layerName, index );

                                   AddSystemAttribute( feature, ODB_ATTR::PAD_USAGE::VIA );
                                   AddSystemAttribute(
                                           feature, ODB_ATTR::GEOMETRY{ "VIA_RoundD"
                                                                        + std::to_string( via->GetWidth( aLayer ) ) } );
                               } );
        }
    }
}


void FEATURES_MANAGER::AddZone( PCB_LAYER_ID aLayer, ZONE* zone )
{
    SHAPE_POLY_SET zone_shape = zone->GetFilledPolysList( aLayer )->CloneDropTriangulation();
    EDA_DATA::SUB_NET_PLANE* plane = nullptr;

    if( IsCopperLayer( aLayer ) )
    {
        auto iter = GetODBPlugin()->GetPlaneSubnetMap().find( std::make_pair( aLayer, zone ) );

        if( iter == GetODBPlugin()->GetPlaneSubnetMap().end() )
        {
            wxLogTrace( traceOdbppIo, wxT( "Failed to get subnet plane data" ) );
            return;
        }

        plane = iter->second;
    }

    for( int ii = 0; ii < zone_shape.OutlineCount(); ++ii )
    {
        size_t first = FeatureCount();
        AddContour( zone_shape, ii );

        forEachNewFeature( first,
                           [&]( ODB_FEATURE& feature, size_t index )
                           {
                               if( plane )
                                   plane->AddFeatureID( EDA_DATA::FEATURE_ID::TYPE::COPPER, m_layerName, index );

                               if( zone->IsTeardropArea() )
                                   AddSystemAttribute( feature, ODB_ATTR::TEAR_DROP{ true } );
                           } );
    }
};


void FEATURES_MANAGER::AddText( PCB_LAYER_ID aLayer, BOARD_ITEM* item, RESOLUTION_CONTEXT aContext )
{
    EDA_TEXT* text_item = nullptr;

    if( PCB_TEXT* tmp_text = dynamic_cast<PCB_TEXT*>( item ) )
        text_item = static_cast<EDA_TEXT*>( tmp_text );
    else if( PCB_TEXTBOX* tmp_textbox = dynamic_cast<PCB_TEXTBOX*>( item ) )
        text_item = static_cast<EDA_TEXT*>( tmp_textbox );

    if( !text_item || !text_item->IsVisible() )
        return;

    wxString shownText = text_item->GetShownText( aContext );

    if( shownText.empty() )
        return;

    auto plot_text =
            [&]( const VECTOR2I& aPos, const wxString& aTextString, const TEXT_ATTRIBUTES& aAttributes,
                 KIFONT::FONT* aFont, const KIFONT::METRICS& aFontMetrics )
            {
                KIGFX::GAL_DISPLAY_OPTIONS empty_opts;

                TEXT_ATTRIBUTES attributes = aAttributes;
                int             penWidth = attributes.m_StrokeWidth;

                if( penWidth == 0 && attributes.m_Bold ) // Use default values if aPenWidth == 0
                    penWidth = GetPenSizeForBold( std::min( attributes.m_Size.x, attributes.m_Size.y ) );

                if( penWidth < 0 )
                    penWidth = -penWidth;

                attributes.m_StrokeWidth = penWidth;

                std::list<VECTOR2I> pts;

                auto tagString = [&]( ODB_FEATURE& aFeature, size_t )
                {
                    AddSystemAttribute( aFeature, ODB_ATTR::STRING{ aTextString.utf8_string() } );
                };

                auto push_pts =
                        [&]()
                        {
                            if( pts.size() < 2 )
                                return;

                            // Polylines are only allowed for more than 3 points.
                            // Otherwise, we have to use a line

                            if( pts.size() < 3 )
                            {
                                size_t    first = FeatureCount();
                                PCB_SHAPE shape( nullptr, SHAPE_T::SEGMENT );
                                shape.SetStart( pts.front() );
                                shape.SetEnd( pts.back() );
                                shape.SetWidth( attributes.m_StrokeWidth );

                                AddShape( shape );
                                forEachNewFeature( first, tagString );
                            }
                            else
                            {
                                for( auto it = pts.begin(); std::next( it ) != pts.end(); ++it )
                                {
                                    size_t    first = FeatureCount();
                                    auto      it2 = std::next( it );
                                    PCB_SHAPE shape( nullptr, SHAPE_T::SEGMENT );
                                    shape.SetStart( *it );
                                    shape.SetEnd( *it2 );
                                    shape.SetWidth( attributes.m_StrokeWidth );
                                    AddShape( shape );

                                    forEachNewFeature( first, tagString );
                                }
                            }

                            pts.clear();
                        };

                CALLBACK_GAL callback_gal(
                        empty_opts,
                        // Stroke callback
                        [&]( const VECTOR2I& aPt1, const VECTOR2I& aPt2 )
                        {
                            if( !pts.empty() )
                            {
                                if( aPt1 == pts.back() )
                                    pts.push_back( aPt2 );
                                else if( aPt2 == pts.front() )
                                    pts.push_front( aPt1 );
                                else if( aPt1 == pts.front() )
                                    pts.push_front( aPt2 );
                                else if( aPt2 == pts.back() )
                                    pts.push_back( aPt1 );
                                else
                                {
                                    push_pts();
                                    pts.push_back( aPt1 );
                                    pts.push_back( aPt2 );
                                }
                            }
                            else
                            {
                                pts.push_back( aPt1 );
                                pts.push_back( aPt2 );
                            }
                        },
                        // Polygon callback
                        [&]( const SHAPE_LINE_CHAIN& aPoly )
                        {
                            if( aPoly.PointCount() < 3 )
                                return;

                            SHAPE_POLY_SET poly_set;
                            poly_set.AddOutline( aPoly );

                            for( int ii = 0; ii < poly_set.OutlineCount(); ++ii )
                            {
                                size_t first = FeatureCount();
                                AddContour( poly_set, ii, FILL_T::FILLED_SHAPE );

                                forEachNewFeature( first, tagString );
                            }
                        } );

                aFont->Draw( &callback_gal, aTextString, aPos, aAttributes, aFontMetrics );

                if( !pts.empty() )
                    push_pts();
            };

    PCB_TEXT*    text = nullptr;
    PCB_TEXTBOX* textbox = nullptr;
    bool         isKnockout = false;

    if( item->Type() == PCB_TEXT_T || item->Type() == PCB_FIELD_T )
    {
        text = static_cast<PCB_TEXT*>( item );
        isKnockout = text->IsKnockout();
    }
    else if( item->Type() == PCB_TEXTBOX_T )
    {
        textbox = static_cast<PCB_TEXTBOX*>( item );
        isKnockout = textbox->IsKnockout();
    }

    const KIFONT::METRICS& fontMetrics = item->GetFontMetrics();
    KIFONT::FONT*          font = text_item->GetDrawFont( nullptr );

    VECTOR2I pos = text_item->GetTextPos();

    TEXT_ATTRIBUTES attrs = text_item->GetAttributes();
    attrs.m_StrokeWidth = text_item->GetEffectiveTextPenWidth();
    attrs.m_Angle = text_item->GetDrawRotation();
    attrs.m_Multiline = false;

    if( isKnockout )
    {
        SHAPE_POLY_SET finalpolyset;
        int            maxError = m_board->GetDesignSettings().m_MaxError;

        if( text )
            text->TransformTextToPolySet( finalpolyset, 0, maxError, ERROR_INSIDE );
        else if( textbox )
            textbox->TransformTextToPolySet( finalpolyset, 0, maxError, ERROR_INSIDE );

        finalpolyset.Fracture();

        for( int ii = 0; ii < finalpolyset.OutlineCount(); ++ii )
        {
            size_t first = FeatureCount();
            AddContour( finalpolyset, ii, FILL_T::FILLED_SHAPE );

            forEachNewFeature( first,
                               [&]( ODB_FEATURE& feature, size_t )
                               {
                                   AddSystemAttribute( feature, ODB_ATTR::STRING{ shownText.utf8_string() } );
                               } );
        }
    }
    else if( text_item->IsMultilineAllowed() )
    {
        std::vector<VECTOR2I> positions;
        wxArrayString         strings_list;
        wxStringSplit( shownText, strings_list, '\n' );
        positions.reserve( strings_list.Count() );

        text_item->GetLinePositions( nullptr, positions, strings_list.Count() );

        for( unsigned ii = 0; ii < strings_list.Count(); ii++ )
        {
            wxString& txt = strings_list.Item( ii );
            plot_text( positions[ii], txt, attrs, font, fontMetrics );
        }
    }
    else
    {
        plot_text( pos, shownText, attrs, font, fontMetrics );
    }
};


void FEATURES_MANAGER::AddShape( PCB_LAYER_ID aLayer, PCB_SHAPE* shape )
{
    // FOOTPRINT* fp = shape->GetParentFootprint();
    AddShape( *shape, aLayer );
};


void FEATURES_MANAGER::AddDimension( PCB_LAYER_ID aLayer, PCB_DIMENSION_BASE* dimension )
{
    // A dimension is a PCB_TEXT subclass, so the value text is plotted via add_text.

    AddText( aLayer, dimension, FOR_CANVAS );

    PCB_SHAPE temp_shape;
    temp_shape.SetStroke( STROKE_PARAMS( dimension->GetLineThickness(), LINE_STYLE::SOLID ) );
    temp_shape.SetLayer( dimension->GetLayer() );

    for( const std::shared_ptr<SHAPE>& shape : dimension->GetShapes() )
    {
        switch( shape->Type() )
        {
        case SH_SEGMENT:
        {
            const SEG& seg = static_cast<const SHAPE_SEGMENT*>( shape.get() )->GetSeg();

            temp_shape.SetShape( SHAPE_T::SEGMENT );
            temp_shape.SetStart( seg.A );
            temp_shape.SetEnd( seg.B );

            AddShape( aLayer, &temp_shape );
            break;
        }

        case SH_CIRCLE:
        {
            VECTOR2I center( shape->Centre() );
            int      radius = static_cast<const SHAPE_CIRCLE*>( shape.get() )->GetRadius();

            temp_shape.SetShape( SHAPE_T::CIRCLE );
            temp_shape.SetFilled( false );
            temp_shape.SetStart( center );
            temp_shape.SetEnd( VECTOR2I( center.x + radius, center.y ) );

            AddShape( aLayer, &temp_shape );
            break;
        }

        default:
            break;
        }
    }
};


void FEATURES_MANAGER::AddPad( PCB_LAYER_ID aLayer, PAD* pad )
{
    auto iter = GetODBPlugin()->GetPadSubnetMap().find( pad );

    if( iter == GetODBPlugin()->GetPadSubnetMap().end() )
    {
        wxLogTrace( traceOdbppIo, wxT( "Failed to get subnet top data" ) );
        return;
    }

    if( aLayer != PCB_LAYER_ID::UNDEFINED_LAYER )
    {
        // FOOTPRINT* fp = pad->GetParentFootprint();

        size_t first = FeatureCount();
        AddPadShape( *pad, aLayer );
        FAB_PAD_ROLE role = GetFabPadRole( *pad );

        forEachNewFeature( first,
                           [&]( ODB_FEATURE& feature, size_t index )
                           {
                               if( IsCopperLayer( aLayer ) )
                                   iter->second->AddFeatureID( EDA_DATA::FEATURE_ID::TYPE::COPPER, m_layerName,
                                                               index );

                               switch( role )
                               {
                               case FAB_PAD_ROLE::FIDUCIAL_GLOBAL:
                                   AddSystemAttribute( feature, ODB_ATTR::PAD_USAGE::G_FIDUCIAL );
                                   break;
                               case FAB_PAD_ROLE::FIDUCIAL_LOCAL:
                                   AddSystemAttribute( feature, ODB_ATTR::PAD_USAGE::L_FIDUCIAL );
                                   break;
                               default:
                                   AddSystemAttribute( feature, ODB_ATTR::PAD_USAGE::TOEPRINT );
                                   break;
                               }

                               if( role == FAB_PAD_ROLE::TESTPOINT )
                                   AddSystemAttribute( feature, ODB_ATTR::TEST_POINT{ true } );

                               if( !pad->HasHole() )
                                   AddSystemAttribute( feature, ODB_ATTR::SMD{ true } );
                           } );
    }
    else
    {
        // drill layer round hole or slot hole
        if( m_role == ODB_LAYER_ROLE::DRILL || m_role == ODB_LAYER_ROLE::BACKDRILL )
        {
            wxCHECK_RET( m_drillSpan.has_value(), "Drill layer has no span" );

            const DRILL_OPERATION* operation = m_plugin->GetFabDrillModel().Find( *m_drillSpan, pad );

            if( !operation )
                return;

            size_t first = FeatureCount();

            // Drill layers hold only circular pads and lines, so a slot is the line its tool cuts
            if( operation->m_IsSlot )
            {
                const VECTOR2I& size = operation->m_SizeXY;
                int             delta = std::abs( size.x - size.y ) / 2;
                VECTOR2I        half = size.x > size.y ? VECTOR2I( delta, 0 ) : VECTOR2I( 0, delta );

                RotatePoint( half, operation->m_Orientation );
                AddFeatureLine( operation->m_Position - half, operation->m_Position + half, operation->m_Diameter );
            }
            else
            {
                AddPadCircle( operation->m_Position, operation->m_Diameter );
            }

            if( pad->GetAttribute() == PAD_ATTRIB::PTH && !operation->IsBackdrill() )
            {
                // only plated holes link to subnet
                forEachNewFeature( first,
                                   [&]( ODB_FEATURE& feature, size_t index )
                                   {
                                       iter->second->AddFeatureID( EDA_DATA::FEATURE_ID::TYPE::HOLE, m_layerName,
                                                                   index );
                                       AddSystemAttribute( feature, ODB_ATTR::DRILL::PLATED );

                                       if( GetFabPadRole( *pad ) == FAB_PAD_ROLE::PRESSFIT )
                                           AddSystemAttribute( feature, ODB_ATTR::PLATED_TYPE::PRESS_FIT );
                                   } );
            }
            else
            {
                forEachNewFeature( first,
                                   [&]( ODB_FEATURE& feature, size_t )
                                   {
                                       AddSystemAttribute( feature, ODB_ATTR::DRILL::NON_PLATED );

                                       if( GetFabPadRole( *pad ) == FAB_PAD_ROLE::TOOLING_HOLE )
                                           AddSystemAttribute( feature, ODB_ATTR::PAD_USAGE::TOOLING_HOLE );
                                   } );
            }

            if( !operation->IsBackdrill() )
            {
                forEachNewFeature( first,
                                   [&]( ODB_FEATURE& feature, size_t )
                                   {
                                       addPostMachiningAttributes( feature, *operation );
                                   } );
            }
        }
    }
    // AddSystemAttribute( *m_featuresList.back(),
    //         ODB_ATTR::GEOMETRY{ "PAD_xxxx" } );
};

void FEATURES_MANAGER::InitFeatureList( PCB_LAYER_ID aLayer, const std::vector<BOARD_ITEM*>& aItems )
{
    for( BOARD_ITEM* item : aItems )
    {
        switch( item->Type() )
        {
        case PCB_TRACE_T:
        case PCB_ARC_T:
        case PCB_VIA_T:
            AddTrack( aLayer, static_cast<PCB_TRACK*>( item ) );
            break;

        case PCB_ZONE_T:
            AddZone( aLayer, static_cast<ZONE*>( item ) );
            break;

        case PCB_PAD_T:
            AddPad( aLayer, static_cast<PAD*>( item ) );
            break;

        case PCB_SHAPE_T:
        {
            size_t first = FeatureCount();
            PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( item );
            PCB_SHAPE adjusted( nullptr );

            if( FabMaskShape( *shape, aLayer, adjusted ) )
                AddShape( adjusted, aLayer );
            else
                AddShape( aLayer, shape );

            const FOOTPRINT* footprint = item->GetParentFootprint();

            if( footprint && footprint->IsNetTie() && IsCopperLayer( aLayer ) )
            {
                if( FeatureCount() == first + 1 )
                    GetODBPlugin()->RecordNetTieFeature( item, aLayer, m_layerName, first );
                else
                    wxLogTrace( traceOdbppIo, wxT( "Net tie shape did not produce one copper feature" ) );
            }

            break;
        }

        case PCB_TEXT_T:
        case PCB_FIELD_T:
            AddText( aLayer, item, FOR_CANVAS );
            break;

        case PCB_TEXTBOX_T:
            AddText( aLayer, item, FOR_CANVAS );

            if( static_cast<PCB_TEXTBOX*>( item )->IsBorderEnabled() )
                AddShape( aLayer, static_cast<PCB_TEXTBOX*>( item ) );

            break;

        case PCB_TABLE_T:
        case PCB_DRILL_CHART_T:
        {
            PCB_TABLE* table = static_cast<PCB_TABLE*>( item );

            for( PCB_TABLECELL* cell : table->GetCells() )
                AddText( aLayer, cell, FOR_CANVAS );

            table->DrawBorders(
                    [&]( const VECTOR2I& aPt1, const VECTOR2I& aPt2, const STROKE_PARAMS& aStroke )
                    {
                        int lineWidth = aStroke.GetWidth();

                        if( lineWidth > 0 )
                            AddFeatureLine( aPt1, aPt2, lineWidth );
                    } );

            break;
        }

        case PCB_DIM_ALIGNED_T:
        case PCB_DIM_LEADER_T:
        case PCB_DIM_CENTER_T:
        case PCB_DIM_RADIAL_T:
        case PCB_DIM_ORTHOGONAL_T:
            AddDimension( aLayer, static_cast<PCB_DIMENSION_BASE*>( item ) );
            break;

        case PCB_TARGET_T:
            //TODO: Add support for targets
            break;

        case PCB_BARCODE_T:
        {
            const PCB_BARCODE* barcode = static_cast<const PCB_BARCODE*>( item );
            SHAPE_POLY_SET     poly_set;

            barcode->TransformShapeToPolygon( poly_set, aLayer, 0, m_board->GetDesignSettings().m_MaxError,
                                              ERROR_INSIDE );
            poly_set.Fracture();

            for( int ii = 0; ii < poly_set.OutlineCount(); ++ii )
                AddContour( poly_set, ii, FILL_T::FILLED_SHAPE );

            break;
        }

        default:
            break;
        }
    }
}


void FEATURES_MANAGER::AddVia( const PCB_VIA* aVia, PCB_LAYER_ID aLayer )
{
    if( !aVia->FlashLayer( aLayer ) )
        return;

    PAD dummy( nullptr ); // default pad shape is circle
    dummy.SetPadstack( aVia->Padstack() );
    dummy.SetPosition( aVia->GetStart() );

    AddPadShape( dummy, aLayer );
}


bool FEATURES_MANAGER::AddViaDrillHole( const PCB_VIA* aVia, PCB_LAYER_ID aLayer )
{
    if( m_role == ODB_LAYER_ROLE::BACKDRILL )
    {
        wxCHECK_MSG( m_drillSpan.has_value(), false, "Backdrill layer has no span" );

        const DRILL_OPERATION* operation = m_plugin->GetFabDrillModel().Find( *m_drillSpan, aVia );

        if( !operation )
            return false;

        AddPadCircle( operation->m_Position, operation->m_Diameter );
        return true;
    }

    DRILL_SPAN span = m_drillSpan.value_or( DRILL_SPAN( aVia->TopLayer(), aVia->BottomLayer(), false, false ) );
    const DRILL_OPERATION* operation = m_plugin->GetFabDrillModel().Find( span, aVia );

    if( !operation )
        return false;

    PAD dummy( nullptr );
    dummy.SetPadstackMode( PADSTACK::MODE::NORMAL );
    dummy.SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
    dummy.SetPosition( aVia->GetStart() );
    dummy.SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( operation->m_Diameter, operation->m_Diameter ) );

    AddPadShape( dummy, aLayer );
    return true;
}


void FEATURES_MANAGER::GenerateProfileFeatures( std::ostream& ost ) const
{
    const ODB_FORMAT& format = m_plugin->GetFormat();
    ost << "UNITS=" << format.m_unitsStr << std::endl;
    ost << "#\n#Num Features\n#" << std::endl;
    ost << "F " << m_featuresList.size() << std::endl;

    if( m_featuresList.empty() )
        return;

    ost << "#\n#Layer features\n#" << std::endl;

    for( const auto& feat : m_featuresList )
    {
        feat->WriteFeatures( ost, format );
    }
}


void FEATURES_MANAGER::GenerateFeatureFile( std::ostream& ost ) const
{
    const ODB_FORMAT& format = m_plugin->GetFormat();
    ost << "UNITS=" << format.m_unitsStr << std::endl;
    ost << "#\n#Num Features\n#" << std::endl;
    ost << "F " << m_featuresList.size() << std::endl << std::endl;

    if( m_featuresList.empty() )
        return;

    ost << "#\n#Feature symbol names\n#" << std::endl;

    for( const auto& [n, name] : m_allSymMap )
    {
        ost << "$" << n << " " << name << std::endl;
    }

    WriteAttributes( ost );

    ost << "#\n#Layer features\n#" << std::endl;

    for( const auto& feat : m_featuresList )
    {
        feat->WriteFeatures( ost, format );
    }
}


void ODB_FEATURE::WriteFeatures( std::ostream& ost, const ODB_FORMAT& aFormat )
{
    switch( GetFeatureType() )
    {
    case FEATURE_TYPE::LINE: ost << "L "; break;

    case FEATURE_TYPE::ARC: ost << "A "; break;

    case FEATURE_TYPE::PAD: ost << "P "; break;

    case FEATURE_TYPE::SURFACE: ost << "S "; break;
    default: return;
    }

    WriteRecordContent( ost, aFormat );
    ost << std::endl;
}


void ODB_LINE::WriteRecordContent( std::ostream& ost, const ODB_FORMAT& )
{
    ost << m_start.first << " " << m_start.second << " " << m_end.first << " " << m_end.second
        << " " << m_symIndex << " P 0";

    WriteAttributes( ost );
}


void ODB_ARC::WriteRecordContent( std::ostream& ost, const ODB_FORMAT& )
{
    ost << m_start.first << " " << m_start.second << " " << m_end.first << " " << m_end.second
        << " " << m_center.first << " " << m_center.second << " " << m_symIndex << " P 0 "
        << ( m_direction == ODB_DIRECTION::CW ? "Y" : "N" );

    WriteAttributes( ost );
}


void ODB_PAD::WriteRecordContent( std::ostream& ost, const ODB_FORMAT& aFormat )
{
    ost << m_center.first << " " << m_center.second << " ";

    // TODO: support resize symbol
    // ost << "-1" << " " << m_symIndex << " "
    //     << m_resize << " P 0 ";

    ost << m_symIndex << " P 0 ";

    ost << "8 " << ODB::Double2String( aFormat, ( ANGLE_360 - m_angle ).Normalize().AsDegrees() );

    WriteAttributes( ost );
}


ODB_SURFACE::ODB_SURFACE( uint32_t aIndex, const SHAPE_POLY_SET::POLYGON& aPolygon,
                          FILL_T aFillType /*= FILL_T::FILLED_SHAPE*/ ) : ODB_FEATURE( aIndex )
{
    m_surfaces = std::make_unique<ODB_SURFACE_DATA>( aPolygon );

    if( aFillType != FILL_T::NO_FILL )
        m_surfaces->AddPolygonHoles( aPolygon );
}


void ODB_SURFACE::WriteRecordContent( std::ostream& ost, const ODB_FORMAT& aFormat )
{
    ost << "P 0";
    WriteAttributes( ost );
    ost << std::endl;
    m_surfaces->WriteData( ost, aFormat );
    ost << "SE";
}


ODB_SURFACE_DATA::ODB_SURFACE_DATA( const SHAPE_POLY_SET::POLYGON& aPolygon )
{
    AddContour( aPolygon[0], false );
}


void ODB_SURFACE_DATA::AddPolygonHoles( const SHAPE_POLY_SET::POLYGON& aPolygon )
{
    for( size_t ii = 1; ii < aPolygon.size(); ++ii )
        AddContour( aPolygon[ii], true );
}


void ODB_SURFACE_DATA::AddContour( const SHAPE_LINE_CHAIN& aChain, bool aHole )
{
    if( aChain.PointCount() < 3 )
        return;

    double area = aChain.Area( false );

    if( area == 0.0 )
        return;

    const std::vector<VECTOR2I>& pts = aChain.CPoints();
    bool                         reverse = aHole ? area > 0 : area < 0;
    auto&                        contour = m_polygons.emplace_back();
    contour.reserve( pts.size() + 1 );

    if( reverse )
    {
        contour.emplace_back( pts.front() );

        for( auto it = pts.rbegin(); it != pts.rend(); ++it )
            contour.emplace_back( *it );
    }
    else
    {
        contour.emplace_back( pts.back() );

        for( const VECTOR2I& pt : pts )
            contour.emplace_back( pt );
    }
}


void ODB_SURFACE_DATA::WriteData( std::ostream& ost, const ODB_FORMAT& aFormat ) const
{
    ODB::CHECK_ONCE is_island;

    for( const auto& contour : m_polygons )
    {
        if( contour.empty() )
            continue;

        ost << "OB " << ODB::AddXY( aFormat, contour.back().m_end ).first << " "
            << ODB::AddXY( aFormat, contour.back().m_end ).second << " ";

        if( is_island() )
            ost << "I";
        else
            ost << "H";
        ost << std::endl;

        for( const auto& line : contour )
        {
            if( SURFACE_LINE::LINE_TYPE::SEGMENT == line.m_type )
                ost << "OS " << ODB::AddXY( aFormat, line.m_end ).first << " "
                    << ODB::AddXY( aFormat, line.m_end ).second << std::endl;
            else
                ost << "OC " << ODB::AddXY( aFormat, line.m_end ).first << " "
                    << ODB::AddXY( aFormat, line.m_end ).second << " " << ODB::AddXY( aFormat, line.m_center ).first
                    << " " << ODB::AddXY( aFormat, line.m_center ).second << " "
                    << ( line.m_direction == ODB_DIRECTION::CW ? "Y" : "N" ) << std::endl;
        }
        ost << "OE" << std::endl;
    }
}
