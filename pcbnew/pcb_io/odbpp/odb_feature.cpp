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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "odb_feature.h"

#include <sstream>
#include <map>

#include <wx/log.h>

#include "pcb_shape.h"
#include "odb_defines.h"
#include "pcb_track.h"
#include "pcb_textbox.h"
#include "zone.h"
#include "board.h"
#include "board_design_settings.h"
#include "geometry/eda_angle.h"
#include "odb_eda_data.h"
#include "pcb_io_odbpp.h"
#include <callback_gal.h>
#include <string_utils.h>


void FEATURES_MANAGER::AddFeatureLine( const VECTOR2I& aStart, const VECTOR2I& aEnd,
                                       uint64_t aWidth )
{
    AddFeature<ODB_LINE>( ODB::AddXY( aStart ), ODB::AddXY( aEnd ),
                          AddCircleSymbol( ODB::SymDouble2String( aWidth ) ) );
}


void FEATURES_MANAGER::AddFeatureArc( const VECTOR2I& aStart, const VECTOR2I& aEnd,
                                      const VECTOR2I& aCenter, uint64_t aWidth,
                                      ODB_DIRECTION aDirection )
{
    AddFeature<ODB_ARC>( ODB::AddXY( aStart ), ODB::AddXY( aEnd ), ODB::AddXY( aCenter ),
                         AddCircleSymbol( ODB::SymDouble2String( aWidth ) ), aDirection );
}


void FEATURES_MANAGER::AddPadCircle( const VECTOR2I& aCenter, uint64_t aDiameter,
                                     const EDA_ANGLE& aAngle, bool aMirror,
                                     double aResize /*= 1.0 */ )
{
    AddFeature<ODB_PAD>( ODB::AddXY( aCenter ),
                         AddCircleSymbol( ODB::SymDouble2String( aDiameter ) ), aAngle, aMirror,
                         aResize );
}


bool FEATURES_MANAGER::AddContour( const SHAPE_POLY_SET& aPolySet, int aOutline /*= 0*/,
                                   FILL_T aFillType /*= FILL_T::FILLED_SHAPE*/ )
{
    // todo: args modify aPolySet.Polygon( aOutline ) instead of aPolySet

    if( aPolySet.OutlineCount() < ( aOutline + 1 ) )
        return false;

    AddFeatureSurface( aPolySet.Polygon( aOutline ), aFillType );

    return true;
}


void FEATURES_MANAGER::AddShape( const PCB_SHAPE& aShape, PCB_LAYER_ID aLayer )
{
    int stroke_width = aShape.GetWidth();

    switch( aShape.GetShape() )
    {
    case SHAPE_T::CIRCLE:
    {
        int      diameter = aShape.GetRadius() * 2;
        VECTOR2I center = ODB::GetShapePosition( aShape );
        wxString innerDim = ODB::SymDouble2String( ( diameter - stroke_width / 2 ) );
        wxString outerDim = ODB::SymDouble2String( ( stroke_width + diameter ) );

        if( aShape.IsSolidFill() )
            AddFeature<ODB_PAD>( ODB::AddXY( center ), AddCircleSymbol( outerDim ) );
        else
            AddFeature<ODB_PAD>( ODB::AddXY( center ), AddRoundDonutSymbol( outerDim, innerDim ) );

        break;
    }

    case SHAPE_T::RECTANGLE:
    {
        int      width = std::abs( aShape.GetRectangleWidth() ) + stroke_width;
        int      height = std::abs( aShape.GetRectangleHeight() ) + stroke_width;
        wxString rad = ODB::SymDouble2String( ( stroke_width / 2.0 ) );
        VECTOR2I center = ODB::GetShapePosition( aShape );

        if( aShape.IsSolidFill() )
        {
            AddFeature<ODB_PAD>( ODB::AddXY( center ),
                                 AddRoundRectSymbol( ODB::SymDouble2String( width ),
                                                     ODB::SymDouble2String( height ), rad ) );
        }
        else
        {
            AddFeature<ODB_PAD>( ODB::AddXY( center ),
                                 AddRoundRectDonutSymbol( ODB::SymDouble2String( width ),
                                                          ODB::SymDouble2String( height ),
                                                          ODB::SymDouble2String( stroke_width ),
                                                          rad ) );
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

    default:
        wxLogError( wxT( "Unknown shape when adding ODB++ layer feature" ) );
        break;
    }

    if( aShape.IsHatchedFill() )
    {
        for( int ii = 0; ii < aShape.GetHatching().OutlineCount(); ++ii )
            AddContour( aShape.GetHatching(), ii, FILL_T::FILLED_SHAPE );
    }
}


void FEATURES_MANAGER::AddFeatureSurface( const SHAPE_POLY_SET::POLYGON& aPolygon,
                                          FILL_T aFillType /*= FILL_T::FILLED_SHAPE */ )
{
    AddFeature<ODB_SURFACE>( aPolygon, aFillType );
}


void FEATURES_MANAGER::AddPadShape( const PAD& aPad, PCB_LAYER_ID aLayer )
{
    FOOTPRINT* fp = aPad.GetParentFootprint();
    bool       mirror = false;

    if( aPad.GetOrientation() != ANGLE_0 )
    {
        if( fp && fp->IsFlipped() )
            mirror = true;
    }

    int maxError = m_board->GetDesignSettings().m_MaxError;

    VECTOR2I expansion{ 0, 0 };

    if( aLayer != UNDEFINED_LAYER && LSET( { F_Mask, B_Mask } ).Contains( aLayer ) )
        expansion.x = expansion.y = aPad.GetSolderMaskExpansion( aLayer );

    if( aLayer != UNDEFINED_LAYER && LSET( { F_Paste, B_Paste } ).Contains( aLayer ) )
        expansion = aPad.GetSolderPasteMargin( aLayer );

    int mask_clearance = expansion.x;

    VECTOR2I plotSize = aPad.GetSize( aLayer ) + 2 * expansion;

    VECTOR2I center = aPad.ShapePos( aLayer );

    wxString width = ODB::SymDouble2String( std::abs( plotSize.x ) );
    wxString height = ODB::SymDouble2String( std::abs( plotSize.y ) );

    switch( aPad.GetShape( aLayer ) )
    {
    case PAD_SHAPE::CIRCLE:
    {
        wxString diam = ODB::SymDouble2String( plotSize.x );

        AddFeature<ODB_PAD>( ODB::AddXY( center ), AddCircleSymbol( diam ), aPad.GetOrientation(),
                             mirror );

        break;
    }
    case PAD_SHAPE::RECTANGLE:
    {
        if( mask_clearance > 0 )
        {
            wxString rad = ODB::SymDouble2String( mask_clearance );

            AddFeature<ODB_PAD>( ODB::AddXY( center ), AddRoundRectSymbol( width, height, rad ),
                                 aPad.GetOrientation(), mirror );
        }
        else
        {
            AddFeature<ODB_PAD>( ODB::AddXY( center ), AddRectSymbol( width, height ),
                                 aPad.GetOrientation(), mirror );
        }

        break;
    }
    case PAD_SHAPE::OVAL:
    {
        AddFeature<ODB_PAD>( ODB::AddXY( center ), AddOvalSymbol( width, height ),
                             aPad.GetOrientation(), mirror );
        break;
    }
    case PAD_SHAPE::ROUNDRECT:
    {
        wxString rad = ODB::SymDouble2String( aPad.GetRoundRectCornerRadius( aLayer ) );

        AddFeature<ODB_PAD>( ODB::AddXY( center ), AddRoundRectSymbol( width, height, rad ),
                             aPad.GetOrientation(), mirror );

        break;
    }
    case PAD_SHAPE::CHAMFERED_RECT:
    {
        int shorterSide = std::min( plotSize.x, plotSize.y );
        int chamfer = std::max(
                0, KiROUND( aPad.GetChamferRectRatio( aLayer ) * shorterSide ) );
        wxString rad = ODB::SymDouble2String( chamfer );
        int      positions = aPad.GetChamferPositions( aLayer );

        AddFeature<ODB_PAD>( ODB::AddXY( center ),
                             AddChamferRectSymbol( width, height, rad, positions ),
                             aPad.GetOrientation(), mirror );

        break;
    }
    case PAD_SHAPE::TRAPEZOID:
    {
        SHAPE_POLY_SET outline;

        aPad.TransformShapeToPolygon( outline, aLayer, 0, maxError, ERROR_INSIDE );

        // Shape polygon can have holes so use InflateWithLinkedHoles(), not Inflate()
        // which can create bad shapes if margin.x is < 0

        if( mask_clearance )
        {
            outline.InflateWithLinkedHoles( expansion.x, CORNER_STRATEGY::ROUND_ALL_CORNERS,
                                            maxError );
        }

        for( int ii = 0; ii < outline.OutlineCount(); ++ii )
            AddContour( outline, ii );

        break;
    }
    case PAD_SHAPE::CUSTOM:
    {
        SHAPE_POLY_SET shape;
        aPad.MergePrimitivesAsPolygon( aLayer, &shape );

        // as for custome shape, odb++ don't rotate the polygon,
        // so we rotate the polygon in kicad anticlockwise

        shape.Rotate( aPad.GetOrientation() );
        shape.Move( center );

        if( expansion != VECTOR2I( 0, 0 ) )
        {
            shape.InflateWithLinkedHoles( std::max( expansion.x, expansion.y ),
                                          CORNER_STRATEGY::ROUND_ALL_CORNERS, maxError );
        }

        for( int ii = 0; ii < shape.OutlineCount(); ++ii )
            AddContour( shape, ii );

        break;
    }
    default: wxLogError( wxT( "Unknown pad type" ) ); break;
    }
}


void FEATURES_MANAGER::InitFeatureList( PCB_LAYER_ID aLayer, std::vector<BOARD_ITEM*>& aItems )
{
    auto add_track = [&]( PCB_TRACK* track )
    {
        auto iter = GetODBPlugin()->GetViaTraceSubnetMap().find( track );

        if( iter == GetODBPlugin()->GetViaTraceSubnetMap().end() )
        {
            wxLogError( wxT( "Failed to get subnet track data" ) );
            return;
        }

        auto subnet = iter->second;

        if( track->Type() == PCB_TRACE_T )
        {
            PCB_SHAPE shape( nullptr, SHAPE_T::SEGMENT );
            shape.SetStart( track->GetStart() );
            shape.SetEnd( track->GetEnd() );
            shape.SetWidth( track->GetWidth() );

            AddShape( shape );
            subnet->AddFeatureID( EDA_DATA::FEATURE_ID::TYPE::COPPER, m_layerName,
                                  m_featuresList.size() - 1 );
        }
        else if( track->Type() == PCB_ARC_T )
        {
            PCB_ARC*  arc = static_cast<PCB_ARC*>( track );
            PCB_SHAPE shape( nullptr, SHAPE_T::ARC );
            shape.SetArcGeometry( arc->GetStart(), arc->GetMid(), arc->GetEnd() );
            shape.SetWidth( arc->GetWidth() );

            AddShape( shape );

            subnet->AddFeatureID( EDA_DATA::FEATURE_ID::TYPE::COPPER, m_layerName,
                                  m_featuresList.size() - 1 );
        }
        else
        {
            // add via
            PCB_VIA* via = static_cast<PCB_VIA*>( track );

            bool hole = false;

            if( aLayer != PCB_LAYER_ID::UNDEFINED_LAYER )
            {
                hole = m_layerName.Contains( "plugging" );
            }
            else
            {
                hole = m_layerName.Contains( "drill" ) || m_layerName.Contains( "filling" )
                       || m_layerName.Contains( "capping" );
            }

            if( hole )
            {
                AddViaDrillHole( via, aLayer );
                subnet->AddFeatureID( EDA_DATA::FEATURE_ID::TYPE::HOLE, m_layerName,
                                      m_featuresList.size() - 1 );

                // TODO: confirm TOOLING_HOLE
                // AddSystemAttribute( *m_featuresList.back(), ODB_ATTR::PAD_USAGE::TOOLING_HOLE );

                if( !m_featuresList.empty() )
                {
                    AddSystemAttribute( *m_featuresList.back(), ODB_ATTR::DRILL::VIA );
                    AddSystemAttribute(
                            *m_featuresList.back(),
                            ODB_ATTR::GEOMETRY{ "VIA_RoundD" + std::to_string( via->GetWidth( aLayer ) ) } );
                }
            }
            else
            {
                // to draw via copper shape on copper layer
                AddVia( via, aLayer );
                subnet->AddFeatureID( EDA_DATA::FEATURE_ID::TYPE::COPPER, m_layerName,
                                      m_featuresList.size() - 1 );

                if( !m_featuresList.empty() )
                {
                    AddSystemAttribute( *m_featuresList.back(), ODB_ATTR::PAD_USAGE::VIA );
                    AddSystemAttribute(
                            *m_featuresList.back(),
                            ODB_ATTR::GEOMETRY{ "VIA_RoundD" + std::to_string( via->GetWidth( aLayer ) ) } );
                }
            }
        }
    };

    auto add_zone = [&]( ZONE* zone )
    {
        SHAPE_POLY_SET zone_shape = zone->GetFilledPolysList( aLayer )->CloneDropTriangulation();

        for( int ii = 0; ii < zone_shape.OutlineCount(); ++ii )
        {
            AddContour( zone_shape, ii );

            auto iter = GetODBPlugin()->GetPlaneSubnetMap().find( std::make_pair( aLayer, zone ) );

            if( iter == GetODBPlugin()->GetPlaneSubnetMap().end() )
            {
                wxLogError( wxT( "Failed to get subnet plane data" ) );
                return;
            }

            iter->second->AddFeatureID( EDA_DATA::FEATURE_ID::TYPE::COPPER, m_layerName,
                                        m_featuresList.size() - 1 );

            if( zone->IsTeardropArea() && !m_featuresList.empty() )
                AddSystemAttribute( *m_featuresList.back(), ODB_ATTR::TEAR_DROP{ true } );
        }
    };

    auto add_text = [&]( BOARD_ITEM* item )
    {
        EDA_TEXT* text_item = nullptr;

        if( PCB_TEXT* tmp_text = dynamic_cast<PCB_TEXT*>( item ) )
            text_item = static_cast<EDA_TEXT*>( tmp_text );
        else if( PCB_TEXTBOX* tmp_text = dynamic_cast<PCB_TEXTBOX*>( item ) )
            text_item = static_cast<EDA_TEXT*>( tmp_text );

        if( !text_item || !text_item->IsVisible() || text_item->GetShownText( false ).empty() )
            return;

        auto plot_text = [&]( const VECTOR2I& aPos, const wxString& aTextString,
                              const TEXT_ATTRIBUTES& aAttributes, KIFONT::FONT* aFont,
                              const KIFONT::METRICS& aFontMetrics )
        {
            KIGFX::GAL_DISPLAY_OPTIONS empty_opts;

            TEXT_ATTRIBUTES attributes = aAttributes;
            int             penWidth = attributes.m_StrokeWidth;

            if( penWidth == 0 && attributes.m_Bold ) // Use default values if aPenWidth == 0
                penWidth =
                        GetPenSizeForBold( std::min( attributes.m_Size.x, attributes.m_Size.y ) );

            if( penWidth < 0 )
                penWidth = -penWidth;

            attributes.m_StrokeWidth = penWidth;

            std::list<VECTOR2I> pts;

            auto push_pts = [&]()
            {
                if( pts.size() < 2 )
                    return;

                // Polylines are only allowed for more than 3 points.
                // Otherwise, we have to use a line

                if( pts.size() < 3 )
                {
                    PCB_SHAPE shape( nullptr, SHAPE_T::SEGMENT );
                    shape.SetStart( pts.front() );
                    shape.SetEnd( pts.back() );
                    shape.SetWidth( attributes.m_StrokeWidth );

                    AddShape( shape );
                    AddSystemAttribute( *m_featuresList.back(),
                                         ODB_ATTR::STRING{ aTextString.ToStdString() } );
                }
                else
                {
                    for( auto it = pts.begin(); std::next( it ) != pts.end(); ++it )
                    {
                        auto      it2 = std::next( it );
                        PCB_SHAPE shape( nullptr, SHAPE_T::SEGMENT );
                        shape.SetStart( *it );
                        shape.SetEnd( *it2 );
                        shape.SetWidth( attributes.m_StrokeWidth );
                        AddShape( shape );

                        if( !m_featuresList.empty() )
                        {
                            AddSystemAttribute( *m_featuresList.back(),
                                                 ODB_ATTR::STRING{ aTextString.ToStdString() } );
                        }
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
                            AddContour( poly_set, ii, FILL_T::FILLED_SHAPE );

                            if( !m_featuresList.empty() )
                            {
                                AddSystemAttribute( *m_featuresList.back(),
                                                    ODB_ATTR::STRING{ aTextString.ToStdString() } );
                            }
                        }
                    } );

            aFont->Draw( &callback_gal, aTextString, aPos, aAttributes, aFontMetrics );

            if( !pts.empty() )
                push_pts();
        };

        bool isKnockout = false;

        if( item->Type() == PCB_TEXT_T || item->Type() == PCB_FIELD_T )
            isKnockout = static_cast<PCB_TEXT*>( item )->IsKnockout();
        else if( item->Type() == PCB_TEXTBOX_T )
            isKnockout = static_cast<PCB_TEXTBOX*>( item )->IsKnockout();

        const KIFONT::METRICS& fontMetrics = item->GetFontMetrics();
        KIFONT::FONT*          font = text_item->GetDrawFont( nullptr );
        wxString               shownText( text_item->GetShownText( true ) );

        if( shownText.IsEmpty() )
            return;

        VECTOR2I pos = text_item->GetTextPos();

        TEXT_ATTRIBUTES attrs = text_item->GetAttributes();
        attrs.m_StrokeWidth = text_item->GetEffectiveTextPenWidth();
        attrs.m_Angle = text_item->GetDrawRotation();
        attrs.m_Multiline = false;

        if( isKnockout )
        {
            PCB_TEXT*      text = static_cast<PCB_TEXT*>( item );
            SHAPE_POLY_SET finalpolyset;

            text->TransformTextToPolySet( finalpolyset, 0, m_board->GetDesignSettings().m_MaxError,
                                          ERROR_INSIDE );
            finalpolyset.Fracture();

            for( int ii = 0; ii < finalpolyset.OutlineCount(); ++ii )
            {
                AddContour( finalpolyset, ii, FILL_T::FILLED_SHAPE );

                if( !m_featuresList.empty() )
                {
                    AddSystemAttribute( *m_featuresList.back(),
                                         ODB_ATTR::STRING{ shownText.ToStdString() } );
                }
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


    auto add_shape = [&]( PCB_SHAPE* shape )
    {
        // FOOTPRINT* fp = shape->GetParentFootprint();
        AddShape( *shape, aLayer );
    };

    auto add_pad = [&]( PAD* pad )
    {
        auto iter = GetODBPlugin()->GetPadSubnetMap().find( pad );

        if( iter == GetODBPlugin()->GetPadSubnetMap().end() )
        {
            wxLogError( wxT( "Failed to get subnet top data" ) );
            return;
        }

        if( aLayer != PCB_LAYER_ID::UNDEFINED_LAYER )
        {
            // FOOTPRINT* fp = pad->GetParentFootprint();

            AddPadShape( *pad, aLayer );

            iter->second->AddFeatureID( EDA_DATA::FEATURE_ID::TYPE::COPPER, m_layerName,
                                        m_featuresList.size() - 1 );
            if( !m_featuresList.empty() )
                AddSystemAttribute( *m_featuresList.back(), ODB_ATTR::PAD_USAGE::TOEPRINT );

            if( !pad->HasHole() && !m_featuresList.empty() )
                AddSystemAttribute( *m_featuresList.back(), ODB_ATTR::SMD{ true } );
        }
        else
        {
            // drill layer round hole or slot hole
            if( m_layerName.Contains( "drill" ) )
            {
                // here we exchange round hole or slot hole into pad to draw in drill layer
                PAD dummy( *pad );
                dummy.Padstack().SetMode( PADSTACK::MODE::NORMAL );

                if( pad->GetDrillSizeX() == pad->GetDrillSizeY() )
                    dummy.SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE ); // round hole shape
                else
                    dummy.SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::OVAL ); // slot hole shape

                dummy.SetOffset( PADSTACK::ALL_LAYERS,
                                 VECTOR2I( 0, 0 ) ); // use hole position not pad position
                dummy.SetSize( PADSTACK::ALL_LAYERS, pad->GetDrillSize() );

                AddPadShape( dummy, aLayer );

                if( pad->GetAttribute() == PAD_ATTRIB::PTH )
                {
                    // only plated holes link to subnet
                    iter->second->AddFeatureID( EDA_DATA::FEATURE_ID::TYPE::HOLE, m_layerName,
                                                m_featuresList.size() - 1 );

                    if( !m_featuresList.empty() )
                        AddSystemAttribute( *m_featuresList.back(), ODB_ATTR::DRILL::PLATED );
                }
                else
                {
                    if( !m_featuresList.empty() )
                        AddSystemAttribute( *m_featuresList.back(), ODB_ATTR::DRILL::NON_PLATED );
                }
            }
        }
        // AddSystemAttribute( *m_featuresList.back(),
        //         ODB_ATTR::GEOMETRY{ "PAD_xxxx" } );
    };

    for( BOARD_ITEM* item : aItems )
    {
        switch( item->Type() )
        {
        case PCB_TRACE_T:
        case PCB_ARC_T:
        case PCB_VIA_T:
            add_track( static_cast<PCB_TRACK*>( item ) );
            break;

        case PCB_ZONE_T:
            add_zone( static_cast<ZONE*>( item ) );
            break;

        case PCB_PAD_T:
            add_pad( static_cast<PAD*>( item ) );
            break;

        case PCB_SHAPE_T:
            add_shape( static_cast<PCB_SHAPE*>( item ) );
            break;

        case PCB_TEXT_T:
        case PCB_FIELD_T:
            add_text( item );
            break;

        case PCB_TEXTBOX_T:
            add_text( item );

            if( static_cast<PCB_TEXTBOX*>( item )->IsBorderEnabled() )
                add_shape( static_cast<PCB_TEXTBOX*>( item ) );

            break;

        case PCB_DIMENSION_T:
        case PCB_TARGET_T:
        case PCB_DIM_ALIGNED_T:
        case PCB_DIM_LEADER_T:
        case PCB_DIM_CENTER_T:
        case PCB_DIM_RADIAL_T:
        case PCB_DIM_ORTHOGONAL_T:
            //TODO: Add support for dimensions
            break;

        case PCB_BARCODE_T:
            //TODO: Add support for barcodes
            break;

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


void FEATURES_MANAGER::AddViaDrillHole( const PCB_VIA* aVia, PCB_LAYER_ID aLayer )
{
    PAD dummy( nullptr ); // default pad shape is circle
    int hole = aVia->GetDrillValue();
    dummy.SetPosition( aVia->GetStart() );
    dummy.SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( hole, hole ) );

    AddPadShape( dummy, aLayer );
}


void FEATURES_MANAGER::GenerateProfileFeatures( std::ostream& ost ) const
{
    ost << "UNITS=" << PCB_IO_ODBPP::m_unitsStr << std::endl;
    ost << "#\n#Num Features\n#" << std::endl;
    ost << "F " << m_featuresList.size() << std::endl;

    if( m_featuresList.empty() )
        return;

    ost << "#\n#Layer features\n#" << std::endl;

    for( const auto& feat : m_featuresList )
    {
        feat->WriteFeatures( ost );
    }
}


void FEATURES_MANAGER::GenerateFeatureFile( std::ostream& ost ) const
{
    ost << "UNITS=" << PCB_IO_ODBPP::m_unitsStr << std::endl;
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
        feat->WriteFeatures( ost );
    }
}


void ODB_FEATURE::WriteFeatures( std::ostream& ost )
{
    switch( GetFeatureType() )
    {
    case FEATURE_TYPE::LINE: ost << "L "; break;

    case FEATURE_TYPE::ARC: ost << "A "; break;

    case FEATURE_TYPE::PAD: ost << "P "; break;

    case FEATURE_TYPE::SURFACE: ost << "S "; break;
    default: return;
    }

    WriteRecordContent( ost );
    ost << std::endl;
}


void ODB_LINE::WriteRecordContent( std::ostream& ost )
{
    ost << m_start.first << " " << m_start.second << " " << m_end.first << " " << m_end.second
        << " " << m_symIndex << " P 0";

    WriteAttributes( ost );
}


void ODB_ARC::WriteRecordContent( std::ostream& ost )
{
    ost << m_start.first << " " << m_start.second << " " << m_end.first << " " << m_end.second
        << " " << m_center.first << " " << m_center.second << " " << m_symIndex << " P 0 "
        << ( m_direction == ODB_DIRECTION::CW ? "Y" : "N" );

    WriteAttributes( ost );
}


void ODB_PAD::WriteRecordContent( std::ostream& ost )
{
    ost << m_center.first << " " << m_center.second << " ";

    // TODO: support resize symbol
    // ost << "-1" << " " << m_symIndex << " "
    //     << m_resize << " P 0 ";

    ost << m_symIndex << " P 0 ";

    if( m_mirror )
        ost << "9 " << ODB::Double2String( m_angle.Normalize().AsDegrees() );
    else
        ost << "8 " << ODB::Double2String( ( ANGLE_360 - m_angle ).Normalize().AsDegrees() );

    WriteAttributes( ost );
}


ODB_SURFACE::ODB_SURFACE( uint32_t aIndex, const SHAPE_POLY_SET::POLYGON& aPolygon,
                          FILL_T aFillType /*= FILL_T::FILLED_SHAPE*/ ) : ODB_FEATURE( aIndex )
{
    if( !aPolygon.empty() && aPolygon[0].PointCount() >= 3 )
    {
        m_surfaces = std::make_unique<ODB_SURFACE_DATA>( aPolygon );
        if( aFillType != FILL_T::NO_FILL )
        {
            m_surfaces->AddPolygonHoles( aPolygon );
        }
    }
    else
    {
        delete this;
    }
}


void ODB_SURFACE::WriteRecordContent( std::ostream& ost )
{
    ost << "P 0";
    WriteAttributes( ost );
    ost << std::endl;
    m_surfaces->WriteData( ost );
    ost << "SE";
}


ODB_SURFACE_DATA::ODB_SURFACE_DATA( const SHAPE_POLY_SET::POLYGON& aPolygon )
{
    const std::vector<VECTOR2I>& pts = aPolygon[0].CPoints();
    if( !pts.empty() )
    {
        if( m_polygons.empty() )
        {
            m_polygons.resize( 1 );
        }

        m_polygons.at( 0 ).reserve( pts.size() );
        m_polygons.at( 0 ).emplace_back( pts.back() );

        for( size_t jj = 0; jj < pts.size(); ++jj )
        {
            m_polygons.at( 0 ).emplace_back( pts.at( jj ) );
        }
    }
}


void ODB_SURFACE_DATA::AddPolygonHoles( const SHAPE_POLY_SET::POLYGON& aPolygon )
{
    for( size_t ii = 1; ii < aPolygon.size(); ++ii )
    {
        wxCHECK2( aPolygon[ii].PointCount() >= 3, continue );

        const std::vector<VECTOR2I>& hole = aPolygon[ii].CPoints();

        if( hole.empty() )
            continue;

        if( m_polygons.size() <= ii )
        {
            m_polygons.resize( ii + 1 );

            m_polygons[ii].reserve( hole.size() );
        }

        m_polygons.at( ii ).emplace_back( hole.back() );

        for( size_t jj = 0; jj < hole.size(); ++jj )
        {
            m_polygons.at( ii ).emplace_back( hole[jj] );
        }
    }
}


void ODB_SURFACE_DATA::WriteData( std::ostream& ost ) const
{
    ODB::CHECK_ONCE is_island;

    for( const auto& contour : m_polygons )
    {
        if( contour.empty() )
            continue;

        ost << "OB " << ODB::AddXY( contour.back().m_end ).first << " "
            << ODB::AddXY( contour.back().m_end ).second << " ";

        if( is_island() )
            ost << "I";
        else
            ost << "H";
        ost << std::endl;

        for( const auto& line : contour )
        {
            if( SURFACE_LINE::LINE_TYPE::SEGMENT == line.m_type )
                ost << "OS " << ODB::AddXY( line.m_end ).first << " "
                    << ODB::AddXY( line.m_end ).second << std::endl;
            else
                ost << "OC " << ODB::AddXY( line.m_end ).first << " "
                    << ODB::AddXY( line.m_end ).second << " " << ODB::AddXY( line.m_center ).first
                    << " " << ODB::AddXY( line.m_center ).second << " "
                    << ( line.m_direction == ODB_DIRECTION::CW ? "Y" : "N" ) << std::endl;
        }
        ost << "OE" << std::endl;
    }
}
