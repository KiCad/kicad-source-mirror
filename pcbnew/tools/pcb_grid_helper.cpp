/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include "pcb_grid_helper.h"

#include <functional>
#include <algorithm>
#include <cmath>
#include <unordered_set>

#include <advanced_config.h>
#include <board_item.h>
#include <pcb_dimension.h>
#include <pcb_shape.h>
#include <footprint.h>
#include <pcb_table.h>
#include <pad.h>
#include <pcb_group.h>
#include <pcb_point.h>
#include <pcb_barcode.h>
#include <pcb_reference_image.h>
#include <pcb_track.h>
#include <zone.h>
#include <gal/graphics_abstraction_layer.h>
#include <geometry/intersection.h>
#include <geometry/nearest.h>
#include <geometry/oval.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_simple.h>
#include <geometry/shape_utils.h>
#include <macros.h>
#include <math/util.h> // for KiROUND
#include <gal/painter.h>
#include <footprint_editor_settings.h>
#include <pcb_base_frame.h>
#include <pcbnew_settings.h>
#include <settings/snap_settings.h>
#include <snap/snap_inference.h>
#include <tool/snap_frame.h>
#include <tool/tool_manager.h>
#include <view/view.h>
#include <trace_helpers.h>

namespace
{
/**
 * Get the INTERSECTABLE_GEOM for a BOARD_ITEM if it's supported.
 *
 * This is the idealised geometry, e.g. a zero-width line or circle.
 */
std::optional<INTERSECTABLE_GEOM> GetBoardIntersectable( const BOARD_ITEM& aItem )
{
    switch( aItem.Type() )
    {
    case PCB_SHAPE_T:
    {
        const PCB_SHAPE& shape = static_cast<const PCB_SHAPE&>( aItem );

        switch( shape.GetShape() )
        {
        case SHAPE_T::SEGMENT:   return SEG{ shape.GetStart(), shape.GetEnd() };
        case SHAPE_T::CIRCLE:    return CIRCLE{ shape.GetCenter(), shape.GetRadius() };
        case SHAPE_T::ARC:       return SHAPE_ARC{ shape.GetStart(), shape.GetArcMid(), shape.GetEnd(), 0 };
        case SHAPE_T::RECTANGLE: return BOX2I::ByCorners( shape.GetStart(), shape.GetEnd() );
        default:                 break;
        }

        break;
    }

    case PCB_TRACE_T:
    {
        const PCB_TRACK& track = static_cast<const PCB_TRACK&>( aItem );
        return SEG{ track.GetStart(), track.GetEnd() };
    }

    case PCB_ARC_T:
    {
        const PCB_ARC& arc = static_cast<const PCB_ARC&>( aItem );
        return SHAPE_ARC{ arc.GetStart(), arc.GetMid(), arc.GetEnd(), 0 };
    }

    case PCB_REFERENCE_IMAGE_T:
    {
        const PCB_REFERENCE_IMAGE& refImage = static_cast<const PCB_REFERENCE_IMAGE&>( aItem );
        return refImage.GetBoundingBox();
    }

    default:
        break;
    }

    return std::nullopt;
}

/**
 * Find the closest point on a BOARD_ITEM to a given point.
 *
 * Only works for items that have a NEARABLE_GEOM defined, it's
 * not a general purpose function.
 *
 * @return The closest point on the item to aPos, or std::nullopt if the item
 *        doesn't have a NEARABLE_GEOM defined.
 */
std::optional<int64_t> FindSquareDistanceToItem( const BOARD_ITEM& item, const VECTOR2I& aPos )
{
    std::optional<INTERSECTABLE_GEOM> intersectable = GetBoardIntersectable( item );
    std::optional<NEARABLE_GEOM>      nearable;

    if( intersectable )
    {
        // Exploit the intersectable as a nearable
        std::visit(
                [&]( const auto& geom )
                {
                    nearable = NEARABLE_GEOM( geom );
                },
                *intersectable );
    }

    // Whatever the item is, we don't have a nearable for it
    if( !nearable )
        return std::nullopt;

    const VECTOR2I nearestPt = GetNearestPoint( *nearable, aPos );
    return nearestPt.SquaredDistance( aPos );
}

} // namespace

PCB_GRID_HELPER::PCB_GRID_HELPER() :
        GRID_HELPER(),
        m_magneticSettings( nullptr )
{
}


PCB_GRID_HELPER::PCB_GRID_HELPER( TOOL_MANAGER* aToolMgr, MAGNETIC_SETTINGS* aMagneticSettings ) :
        GRID_HELPER( aToolMgr, LAYER_ANCHOR ),
        m_magneticSettings( aMagneticSettings )
{
    if( !m_toolMgr )
        return;

    KIGFX::VIEW*            view = m_toolMgr->GetView();
    KIGFX::RENDER_SETTINGS* settings = view->GetPainter()->GetSettings();
    KIGFX::COLOR4D          auxItemsColor = settings->GetLayerColor( LAYER_AUX_ITEMS );
    KIGFX::COLOR4D          anchorColor = settings->GetLayerColor( LAYER_ANCHOR );

    m_viewAxis.SetSize( 20000 );
    m_viewAxis.SetStyle( KIGFX::ORIGIN_VIEWITEM::CROSS );
    m_viewAxis.SetColor( auxItemsColor.WithAlpha( 0.4 ) );
    m_viewAxis.SetDrawAtZero( true );
    view->Add( &m_viewAxis );
    view->SetVisible( &m_viewAxis, false );

    m_viewSnapPoint.SetSize( 10 );
    m_viewSnapPoint.SetStyle( KIGFX::ORIGIN_VIEWITEM::CIRCLE_CROSS );
    m_viewSnapPoint.SetColor( auxItemsColor );
    m_viewSnapPoint.SetDrawAtZero( true );
    view->Add( &m_viewSnapPoint );
    getSnapManager().SetSnapGuideColors( anchorColor, anchorColor.Brightened( 0.2 ) );
    view->SetVisible( &m_viewSnapPoint, false );

    if( m_toolMgr->GetModel() )
        static_cast<BOARD*>( aToolMgr->GetModel() )->AddListener( this );
}


PCB_GRID_HELPER::~PCB_GRID_HELPER()
{
    if( !m_toolMgr )
        return;

    KIGFX::VIEW* view = m_toolMgr->GetView();

    view->Remove( &m_viewAxis );
    view->Remove( &m_viewSnapPoint );

    if( m_toolMgr->GetModel() )
        static_cast<BOARD*>( m_toolMgr->GetModel() )->RemoveListener( this );
}


void PCB_GRID_HELPER::AddConstructionItems( std::vector<BOARD_ITEM*> aItems, bool aExtensionOnly, bool aIsPersistent )
{
    if( !ADVANCED_CFG::GetCfg().m_EnableExtensionSnaps )
        return;

    if( !snapInferenceSettings().constructionExtensions )
        return;

    // For all the elements that get drawn construction geometry,
    // add something suitable to the construction helper.
    // This can be nothing.
    auto constructionItemsBatch = std::make_unique<CONSTRUCTION_MANAGER::CONSTRUCTION_ITEM_BATCH>();

    std::vector<VECTOR2I> referenceOnlyPoints;

    for( BOARD_ITEM* item : aItems )
    {
        std::vector<KIGFX::CONSTRUCTION_GEOM::DRAWABLE> constructionDrawables;

        switch( item->Type() )
        {
        case PCB_SHAPE_T:
        {
            PCB_SHAPE& shape = static_cast<PCB_SHAPE&>( *item );

            switch( shape.GetShape() )
            {
            case SHAPE_T::SEGMENT:
            {
                if( !aExtensionOnly )
                {
                    constructionDrawables.emplace_back( LINE{ shape.GetStart(), shape.GetEnd() } );
                }
                else
                {
                    // Two rays, extending from the segment ends
                    const VECTOR2I segVec = shape.GetEnd() - shape.GetStart();
                    constructionDrawables.emplace_back( HALF_LINE{ shape.GetStart(), shape.GetStart() - segVec } );
                    constructionDrawables.emplace_back( HALF_LINE{ shape.GetEnd(), shape.GetEnd() + segVec } );
                }

                if( aIsPersistent )
                {
                    // include the original endpoints as construction items
                    // (this allows H/V snapping)
                    constructionDrawables.emplace_back( shape.GetStart() );
                    constructionDrawables.emplace_back( shape.GetEnd() );

                    // But mark them as references, so they don't get snapped to themsevles
                    referenceOnlyPoints.emplace_back( shape.GetStart() );
                    referenceOnlyPoints.emplace_back( shape.GetEnd() );
                }
                break;
            }
            case SHAPE_T::ARC:
            {
                if( !aExtensionOnly )
                {
                    constructionDrawables.push_back( CIRCLE{ shape.GetCenter(), shape.GetRadius() } );
                }
                else
                {
                    // The rest of the circle is the arc through the opposite point to the midpoint
                    const VECTOR2I oppositeMid = shape.GetCenter() + ( shape.GetCenter() - shape.GetArcMid() );
                    constructionDrawables.push_back( SHAPE_ARC{ shape.GetStart(), oppositeMid, shape.GetEnd(), 0 } );
                }

                constructionDrawables.push_back( shape.GetCenter() );

                if( aIsPersistent )
                {
                    // include the original endpoints as construction items
                    // (this allows H/V snapping)
                    constructionDrawables.emplace_back( shape.GetStart() );
                    constructionDrawables.emplace_back( shape.GetEnd() );

                    // But mark them as references, so they don't get snapped to themselves
                    referenceOnlyPoints.emplace_back( shape.GetStart() );
                    referenceOnlyPoints.emplace_back( shape.GetEnd() );
                }

                break;
            }
            case SHAPE_T::CIRCLE:
            case SHAPE_T::RECTANGLE:
            {
                constructionDrawables.push_back( shape.GetCenter() );
                break;
            }
            case SHAPE_T::ELLIPSE:
            case SHAPE_T::ELLIPSE_ARC:
            {
                constructionDrawables.push_back( shape.GetEllipseCenter() );
                break;
            }
            default:
                // This shape doesn't have any construction geometry to draw
                break;
            }
            break;
        }
        case PCB_REFERENCE_IMAGE_T:
        {
            const PCB_REFERENCE_IMAGE& pcbRefImg = static_cast<PCB_REFERENCE_IMAGE&>( *item );
            const REFERENCE_IMAGE&     refImg = pcbRefImg.GetReferenceImage();

            constructionDrawables.push_back( refImg.GetPosition() );

            if( refImg.GetTransformOriginOffset() != VECTOR2I( 0, 0 ) )
                constructionDrawables.push_back( refImg.GetPosition() + refImg.GetTransformOriginOffset() );

            for( const SEG& seg : KIGEOM::BoxToSegs( refImg.GetBoundingBox() ) )
                constructionDrawables.push_back( seg );

            break;
        }
        default:
            // This item doesn't have any construction geometry to draw
            break;
        }

        // At this point, constructionDrawables can be empty, which is fine
        // (it means there's no additional construction geometry to draw, but
        // the item is still going to be proposed for activation)

        // Convert the drawables to DRAWABLE_ENTRY format
        std::vector<CONSTRUCTION_MANAGER::CONSTRUCTION_ITEM::DRAWABLE_ENTRY> drawableEntries;
        drawableEntries.reserve( constructionDrawables.size() );
        for( auto& drawable : constructionDrawables )
        {
            drawableEntries.emplace_back(
                    CONSTRUCTION_MANAGER::CONSTRUCTION_ITEM::DRAWABLE_ENTRY{ std::move( drawable ), 1 } );
        }

        constructionItemsBatch->emplace_back( CONSTRUCTION_MANAGER::CONSTRUCTION_ITEM{
                CONSTRUCTION_MANAGER::SOURCE::FROM_ITEMS,
                item,
                std::move( drawableEntries ),
        } );
    }

    if( referenceOnlyPoints.size() )
        getSnapManager().SetReferenceOnlyPoints( std::move( referenceOnlyPoints ) );

    //  Let the manager handle it
    getSnapManager().GetConstructionManager().ProposeConstructionItems( std::move( constructionItemsBatch ),
                                                                        aIsPersistent );
}


VECTOR2I PCB_GRID_HELPER::AlignToSegment( const VECTOR2I& aPoint, const SEG& aSeg )
{
    const int c_gridSnapEpsilon_sq = 4;

    VECTOR2I aligned = Align( aPoint );

    if( !m_enableSnap )
        return aligned;

    std::vector<VECTOR2I> points;

    const SEG testSegments[] = { SEG( aligned, aligned + VECTOR2( 1, 0 ) ),
                                 SEG( aligned, aligned + VECTOR2( 0, 1 ) ),
                                 SEG( aligned, aligned + VECTOR2( 1, 1 ) ),
                                 SEG( aligned, aligned + VECTOR2( 1, -1 ) ) };

    for( const SEG& seg : testSegments )
    {
        OPT_VECTOR2I vec = aSeg.IntersectLines( seg );

        if( vec && aSeg.SquaredDistance( *vec ) <= c_gridSnapEpsilon_sq )
            points.push_back( *vec );
    }

    VECTOR2I    nearest = aligned;
    SEG::ecoord min_d_sq = VECTOR2I::ECOORD_MAX;

    // Snap by distance between pointer and endpoints
    for( const VECTOR2I& pt : { aSeg.A, aSeg.B } )
    {
        SEG::ecoord d_sq = ( pt - aPoint ).SquaredEuclideanNorm();

        if( d_sq < min_d_sq )
        {
            min_d_sq = d_sq;
            nearest = pt;
        }
    }

    // Snap by distance between aligned cursor and intersections
    for( const VECTOR2I& pt : points )
    {
        SEG::ecoord d_sq = ( pt - aligned ).SquaredEuclideanNorm();

        if( d_sq < min_d_sq )
        {
            min_d_sq = d_sq;
            nearest = pt;
        }
    }

    return nearest;
}


VECTOR2I PCB_GRID_HELPER::AlignToArc( const VECTOR2I& aPoint, const SHAPE_ARC& aArc )
{
    VECTOR2I aligned = Align( aPoint );

    if( !m_enableSnap )
        return aligned;

    std::vector<VECTOR2I> points;

    aArc.IntersectLine( SEG( aligned, aligned + VECTOR2( 1, 0 ) ), &points );
    aArc.IntersectLine( SEG( aligned, aligned + VECTOR2( 0, 1 ) ), &points );
    aArc.IntersectLine( SEG( aligned, aligned + VECTOR2( 1, 1 ) ), &points );
    aArc.IntersectLine( SEG( aligned, aligned + VECTOR2( 1, -1 ) ), &points );

    VECTOR2I    nearest = aligned;
    SEG::ecoord min_d_sq = VECTOR2I::ECOORD_MAX;

    // Snap by distance between pointer and endpoints
    for( const VECTOR2I& pt : { aArc.GetP0(), aArc.GetP1() } )
    {
        SEG::ecoord d_sq = ( pt - aPoint ).SquaredEuclideanNorm();

        if( d_sq < min_d_sq )
        {
            min_d_sq = d_sq;
            nearest = pt;
        }
    }

    // Snap by distance between aligned cursor and intersections
    for( const VECTOR2I& pt : points )
    {
        SEG::ecoord d_sq = ( pt - aligned ).SquaredEuclideanNorm();

        if( d_sq < min_d_sq )
        {
            min_d_sq = d_sq;
            nearest = pt;
        }
    }

    return nearest;
}


VECTOR2I PCB_GRID_HELPER::SnapToPad( const VECTOR2I& aMousePos, std::deque<PAD*>& aPads )
{
    wxLogTrace( traceSnap, "SnapToPad: mouse pos (%d, %d), pads count: %zu", aMousePos.x, aMousePos.y, aPads.size() );
    clearAnchors();

    for( BOARD_ITEM* item : aPads )
    {
        if( item->HitTest( aMousePos ) )
            computeAnchors( item, aMousePos, true, nullptr );
    }

    double  minDist = std::numeric_limits<double>::max();
    ANCHOR* nearestOrigin = nullptr;

    for( ANCHOR& a : m_anchors )
    {
        if( ( ORIGIN & a.flags ) != ORIGIN )
            continue;

        double dist = a.Distance( aMousePos );

        if( dist < minDist )
        {
            minDist = dist;
            nearestOrigin = &a;
        }
    }

    return nearestOrigin ? nearestOrigin->pos : aMousePos;
}


void PCB_GRID_HELPER::OnBoardItemRemoved( BOARD& aBoard, BOARD_ITEM* aRemovedItem )
{
    // If the item being removed is involved in the snap, clear the snap item
    if( m_snapItem )
    {
        for( EDA_ITEM* eda_item : m_snapItem->items )
        {
            if( eda_item->IsBOARD_ITEM() )
            {
                BOARD_ITEM* item = static_cast<BOARD_ITEM*>( eda_item );

                if( item == aRemovedItem || item->GetParentFootprint() == aRemovedItem )
                {
                    m_snapItem = std::nullopt;
                    break;
                }
            }
        }
    }
}


void PCB_GRID_HELPER::OnBoardItemsRemoved( BOARD& aBoard, std::vector<BOARD_ITEM*>& aBoardItems )
{
    // This is a bulk-remove.  Simply clearing the snap item will be the most performant.
    m_snapItem = std::nullopt;
}


BOX2I PCB_GRID_HELPER::layoutBounds( const BOARD_ITEM& aItem )
{
    if( aItem.Type() == PCB_FOOTPRINT_T )
        return static_cast<const FOOTPRINT&>( aItem ).GetBoundingBox( false );

    return aItem.GetBoundingBox();
}


bool PCB_GRID_HELPER::editingInsideFootprint() const
{
    if( !m_toolMgr )
        return false;

    // Keyed off the board rather than the current tool: PCB_TOOL_BASE lives in the pcbnew
    // kiface, so casting to it from pcbcommon leaves cvpcb with an undefined typeinfo
    const BOARD* board = static_cast<const BOARD*>( m_toolMgr->GetModel() );

    return board && board->IsFootprintHolder();
}


SNAP_INFERENCE_SETTINGS PCB_GRID_HELPER::snapInferenceSettings() const
{
    SNAP_INFERENCE_SETTINGS settings;

    if( !m_toolMgr )
        return settings;

    if( PCB_BASE_FRAME* frame = dynamic_cast<PCB_BASE_FRAME*>( m_toolMgr->GetToolHolder() ) )
    {
        if( editingInsideFootprint() )
        {
            if( FOOTPRINT_EDITOR_SETTINGS* cfg = frame->GetFootprintEditorSettings() )
                settings = cfg->m_SnapInference;
        }
        else if( PCBNEW_SETTINGS* cfg = frame->GetPcbNewSettings() )
        {
            settings = cfg->m_SnapInference;
        }
    }
    else if( PCBNEW_SETTINGS* cfg = dynamic_cast<PCBNEW_SETTINGS*>( m_toolMgr->GetSettings() ) )
    {
        // Headless callers have settings but no PCB frame.
        settings = cfg->m_SnapInference;
    }

    return settings;
}


VECTOR2I PCB_GRID_HELPER::BestDragOrigin( const VECTOR2I& aMousePos, std::vector<BOARD_ITEM*>& aItems,
                                          GRID_HELPER_GRIDS                   aGrid,
                                          const PCB_SELECTION_FILTER_OPTIONS* aSelectionFilter )
{
    wxLogTrace( traceSnap, "BestDragOrigin: mouse pos (%d, %d), items count: %zu", aMousePos.x, aMousePos.y,
                aItems.size() );
    clearAnchors();

    computeAnchors( aItems, aMousePos, true, aSelectionFilter, nullptr, true );

    double lineSnapMinCornerDistance = m_toolMgr->GetView()->ToWorld( 50 );

    ANCHOR* nearestOutline = nearestAnchor( aMousePos, OUTLINE );
    ANCHOR* nearestCorner = nearestAnchor( aMousePos, CORNER );
    ANCHOR* nearestOrigin = nearestAnchor( aMousePos, ORIGIN );
    ANCHOR* best = nullptr;
    double minDist = std::numeric_limits<double>::max();

    if( nearestOrigin )
    {
        minDist = nearestOrigin->Distance( aMousePos );
        best = nearestOrigin;

        wxLogTrace( traceSnap, "  nearest origin winning at (%d, %d), distance=%f", nearestOrigin->pos.x,
                    nearestOrigin->pos.y, minDist );
    }

    if( nearestCorner )
    {
        double dist = nearestCorner->Distance( aMousePos );

        if( dist < minDist )
        {
            minDist = dist;
            best = nearestCorner;

            wxLogTrace( traceSnap, "  nearest corner winning at (%d, %d), distance=%f", nearestCorner->pos.x,
                        nearestCorner->pos.y, dist );
        }
    }

    if( nearestOutline )
    {
        double dist = nearestOutline->Distance( aMousePos );

        if( minDist > lineSnapMinCornerDistance && dist < minDist )
        {
            best = nearestOutline;

            wxLogTrace( traceSnap, "  nearest outline winning at (%d, %d), distance=%f", nearestOutline->pos.x,
                        nearestOutline->pos.y, dist );
        }
    }

    VECTOR2I ret = best ? best->pos : aMousePos;

    if( best )
    {
        std::optional<BOX2I> movingBounds;

        for( BOARD_ITEM* item : aItems )
        {
            if( !item )
                continue;

            if( movingBounds )
                movingBounds->Merge( layoutBounds( *item ) );
            else
                movingBounds = layoutBounds( *item );
        }

        bool padCenter = ( best->pointTypes & POINT_TYPE::PT_CENTER )
                         && std::any_of( best->items.begin(), best->items.end(),
                                         []( const EDA_ITEM* aItem )
                                         {
                                             return aItem && aItem->Type() == PCB_PAD_T;
                                         } );

        setLayoutReference( ret, movingBounds, padCenter );
    }
    else
    {
        setLayoutReference( ret, std::nullopt, false );
    }

    wxLogTrace( traceSnap, "  have best: %s, returning (%d, %d)", best ? "yes" : "no", ret.x, ret.y );
    return ret;
}


SNAP_RESULT PCB_GRID_HELPER::ResolveSnap( const VECTOR2I& aOrigin, BOARD_ITEM* aReferenceItem, GRID_HELPER_GRIDS aGrid )
{
    LSET                     layers;
    std::vector<BOARD_ITEM*> item;

    if( aReferenceItem )
    {
        layers = aReferenceItem->GetLayerSet();
        item.push_back( aReferenceItem );
    }
    else if( PCB_BASE_FRAME* frame = dynamic_cast<PCB_BASE_FRAME*>( m_toolMgr->GetToolHolder() );
             frame && frame->GetScreen() )
    {
        layers = LSET( { frame->GetActiveLayer() } );
    }
    else
    {
        layers = LSET::AllLayersMask();
    }

    return ResolveSnap( aOrigin, layers, aGrid, item );
}


SNAP_RESULT PCB_GRID_HELPER::ResolveSnap( const VECTOR2I& aOrigin, const LSET& aLayers, GRID_HELPER_GRIDS aGrid,
                                          const std::vector<BOARD_ITEM*>& aSkip,
                                          std::optional<VECTOR2I>         aMovingReferencePoint )
{
    wxLogTrace( traceSnap, "ResolveSnap: origin (%d, %d), enableSnap=%d, enableGrid=%d, enableSnapLine=%d", aOrigin.x,
                aOrigin.y, m_enableSnap, m_enableGrid, m_enableSnapLine );

    const SNAP_RANGES ranges = computeSnapRanges( m_enableGrid );
    const double      snapScale = ranges.scale;
    const int         snapRange = ranges.range;

    const SNAP_INFERENCE_SETTINGS inferenceSettings = snapInferenceSettings();

    const bool constructionEnabled =
            m_enableSnap && inferenceSettings.constructionExtensions && ADVANCED_CFG::GetCfg().m_EnableExtensionSnaps;

    if( !constructionEnabled )
        getSnapManager().GetConstructionManager().Clear();

    //Respect limits of coordinates representation
    const BOX2I visibilityHorizon =
            BOX2ISafe( VECTOR2D( aOrigin ) - snapRange / 2.0, VECTOR2D( snapRange, snapRange ) );

    clearAnchors();

    const std::vector<BOARD_ITEM*> visibleItems = queryVisible( { visibilityHorizon }, aSkip );
    computeAnchors( visibleItems, aOrigin, false, nullptr, &aLayers, false );

    ANCHOR*        nearest = nearestAnchor( aOrigin, SNAPPABLE );
    VECTOR2I       nearestGrid = Align( aOrigin, aGrid );
    const VECTOR2D gridSize = GetGridSize( aGrid );

    SNAP_SOURCE_CONTEXT context;
    context.profile = aSkip.empty() ? SNAP_EDITOR_PROFILE::PICKER : SNAP_EDITOR_PROFILE::RIGID_PLACEMENT;
    context.sourcePoint = aOrigin;
    context.movingReferencePoint = aMovingReferencePoint;
    context.referencePreference = m_layoutReferencePreference;

    for( BOARD_ITEM* item : aSkip )
    {
        if( !item )
            continue;

        if( context.movingBounds )
            context.movingBounds->Merge( layoutBounds( *item ) );
        else
            context.movingBounds = layoutBounds( *item );
    }

    if( aSkip.size() == 1 && aSkip.front() )
    {
        BOARD_ITEM* sourceItem = aSkip.front();
        context.movingItem = SNAP_STABLE_ID{ SNAP_ID_KIND::ITEM_GEOMETRY, SnapTargetId( sourceItem->m_Uuid ) };
        std::optional<std::pair<VECTOR2I, VECTOR2I>> endpoints;

        if( m_pointEditProfile )
            context.profile = SNAP_EDITOR_PROFILE::POINT_EDIT;

        if( sourceItem->Type() == PCB_TRACE_T )
        {
            PCB_TRACK* track = static_cast<PCB_TRACK*>( sourceItem );
            endpoints = std::pair( track->GetStart(), track->GetEnd() );
        }
        else if( sourceItem->Type() == PCB_SHAPE_T )
        {
            PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( sourceItem );

            if( shape->GetShape() == SHAPE_T::SEGMENT )
                endpoints = std::pair( shape->GetStart(), shape->GetEnd() );
        }

        if( endpoints && m_pointEditProfile )
        {
            context.stationarySourceLeg =
                    endpoints->first.SquaredDistance( aOrigin ) > endpoints->second.SquaredDistance( aOrigin )
                            ? endpoints->first
                            : endpoints->second;
        }
    }

    SNAP_INFERENCE_PROVIDER inferenceProvider;
    const bool              inferenceEnabled = m_enableSnap
                                  && ( inferenceSettings.objectGeometry || inferenceSettings.tangentNormal
                                       || inferenceSettings.alignmentDistribution );
    const bool geometryEnabled =
            m_enableSnap && ( inferenceSettings.objectGeometry || inferenceSettings.tangentNormal );

    if( geometryEnabled && context.movingItem )
    {
        for( size_t i = 0; i < m_stationarySelfSegments.size(); ++i )
        {
            SNAP_STABLE_ID id =
                    MakeDerivedSnapId( SNAP_ID_KIND::SELF_SEGMENT, *context.movingItem, static_cast<int>( i ) );
            context.stationarySelfFeatures.push_back( id );
            inferenceProvider.AddPath( { id, m_stationarySelfSegments[i], false } );
        }
    }

    if( inferenceEnabled )
    {
        const auto eligibleInferenceTarget = [&]( BOARD_ITEM* aItem )
        {
            if( !m_magneticSettings->allLayers && !( aLayers & aItem->GetLayerSet() ).any() )
            {
                return false;
            }

            switch( aItem->Type() )
            {
            case PCB_TRACE_T:
            case PCB_ARC_T: return m_magneticSettings->tracks == MAGNETIC_OPTIONS::CAPTURE_ALWAYS;

            case PCB_PAD_T: return m_magneticSettings->pads == MAGNETIC_OPTIONS::CAPTURE_ALWAYS;

            default: return m_magneticSettings->graphics;
            }
        };

        for( BOARD_ITEM* item : visibleItems )
        {
            if( !eligibleInferenceTarget( item ) )
                continue;

            if( !geometryEnabled )
                continue;

            std::optional<INTERSECTABLE_GEOM> geometry = GetBoardIntersectable( *item );

            if( !geometry )
                continue;

            inferenceProvider.AddPath( { { SNAP_ID_KIND::ITEM_GEOMETRY, SnapTargetId( item->m_Uuid ),
                                           static_cast<int>( item->Type() ), 0 },
                                         std::move( *geometry ),
                                         false } );
        }

        if( inferenceSettings.alignmentDistribution && context.movingBounds )
        {
            std::vector<BOARD_ITEM*> layoutItems;
            const BOX2I              viewport = BOX2ISafe( m_toolMgr->GetView()->GetViewport() );
            BOX2I                    movingBounds = *context.movingBounds;

            if( context.movingReferencePoint )
                movingBounds.Offset( context.sourcePoint - *context.movingReferencePoint );

            // Alignment ignores separation along its guide; equal spacing only requires overlap
            // perpendicular to its axis.  Their exact query closure is therefore a cross.
            BOX2I verticalStrip =
                    BOX2ISafe( VECTOR2D( static_cast<double>( movingBounds.GetLeft() ) - snapRange, viewport.GetTop() ),
                               VECTOR2D( movingBounds.GetWidth() + 2.0 * snapRange, viewport.GetHeight() ) );
            BOX2I horizontalStrip =
                    BOX2ISafe( VECTOR2D( viewport.GetLeft(), static_cast<double>( movingBounds.GetTop() ) - snapRange ),
                               VECTOR2D( viewport.GetWidth(), movingBounds.GetHeight() + 2.0 * snapRange ) );
            verticalStrip = verticalStrip.Intersect( viewport );
            horizontalStrip = horizontalStrip.Intersect( viewport );
            std::vector<BOARD_ITEM*> visibleLayoutItems = queryVisible( { verticalStrip, horizontalStrip }, aSkip );

            const bool insideFootprint = editingInsideFootprint();

            for( BOARD_ITEM* item : visibleLayoutItems )
            {
                FOOTPRINT* footprint = item->GetParentFootprint();

                if( footprint && !insideFootprint )
                    layoutItems.push_back( footprint );
                else
                    layoutItems.push_back( item );
            }

            std::sort( layoutItems.begin(), layoutItems.end(), std::less<>() );
            layoutItems.erase( std::unique( layoutItems.begin(), layoutItems.end() ), layoutItems.end() );

            // Moving items and their containers.  A container's bounds enclose what is being
            // moved, and pads reached through their footprint bypass the queryVisible skip list.
            std::unordered_set<BOARD_ITEM*> moving( aSkip.begin(), aSkip.end() );

            for( BOARD_ITEM* item : aSkip )
            {
                for( FOOTPRINT* parent = item ? item->GetParentFootprint() : nullptr; parent;
                     parent = parent->GetParentFootprint() )
                {
                    moving.insert( parent );
                }
            }

            for( BOARD_ITEM* item : layoutItems )
            {
                // Aligning to a container of the move would align the move to itself.  The
                // container's other children remain valid targets.
                if( !eligibleInferenceTarget( item ) || moving.count( item ) )
                    continue;

                std::optional<SNAP_TARGET_ID> parent;

                if( item->GetParent() )
                    parent = SnapTargetId( item->GetParent()->m_Uuid );

                inferenceProvider.AddBounds( { { SNAP_ID_KIND::ITEM_GEOMETRY, SnapTargetId( item->m_Uuid ),
                                                 static_cast<int>( item->Type() ), 0 },
                                               layoutBounds( *item ),
                                               std::move( parent ) } );
            }

            size_t padCenters = 0;

            const auto addPadCenter = [&]( PAD* aPad )
            {
                if( !eligibleInferenceTarget( aPad ) || moving.count( aPad ) )
                    return;

                std::optional<SNAP_TARGET_ID> parent;

                if( FOOTPRINT* footprint = aPad->GetParentFootprint() )
                    parent = SnapTargetId( footprint->m_Uuid );

                inferenceProvider.AddAlignmentPoint( { { SNAP_ID_KIND::INTRINSIC_ANCHOR, SnapTargetId( aPad->m_Uuid ) },
                                                       aPad->GetPosition(),
                                                       std::move( parent ) } );
                ++padCenters;
            };

            for( BOARD_ITEM* item : layoutItems )
            {
                if( item->Type() == PCB_PAD_T )
                {
                    addPadCenter( static_cast<PAD*>( item ) );
                }
                else if( item->Type() == PCB_FOOTPRINT_T )
                {
                    for( PAD* pad : static_cast<FOOTPRINT*>( item )->Pads() )
                        addPadCenter( pad );
                }
            }

            wxLogTrace( wxT( "KICAD_SNAP_RESOLVER" ), "layout targets=%zu pad-centers=%zu", layoutItems.size(),
                        padCenters );
        }
    }

    enum class PRESENTATION_KIND
    {
        ANCHOR_MARKER,
        GUIDE,
        POINT_ON_ELEMENT
    };

    struct PRESENTATION
    {
        PRESENTATION_KIND     kind;
        std::optional<ANCHOR> anchor;
        bool                  proposeConstruction = false;
    };

    SNAP_FRAME_INPUT<PRESENTATION> frame;
    frame.context = context;
    frame.stickyIds = m_stickySnapIds;
    frame.rankingHysteresis = ranges.rankingHysteresis;
    frame.feasibility = m_feasibilityCallback;
    frame.trace = snapTraceCallback( context );
    emitAngleBranchCandidates( frame.candidates, aOrigin, snapScale );

    if( m_enableSnap && inferenceSettings.objectGeometry )
    {
        for( SNAP_CANDIDATE& candidate : inferenceProvider.CollectObjectGeometry( context, snapRange ) )
            frame.candidates.push_back( std::move( candidate ) );
    }

    if( m_enableSnap && inferenceSettings.tangentNormal && context.stationarySourceLeg )
    {
        for( SNAP_CANDIDATE& candidate : inferenceProvider.CollectTangentNormal( context, snapRange, true, true ) )
            frame.candidates.push_back( std::move( candidate ) );
    }

    if( m_enableSnap && inferenceSettings.alignmentDistribution && context.movingBounds )
    {
        std::vector<SNAP_CANDIDATE> alignment = inferenceProvider.CollectAlignment( context, snapRange );
        std::vector<SNAP_CANDIDATE> spacing = inferenceProvider.CollectEqualSpacing( context, snapRange );

        wxLogTrace( wxT( "KICAD_SNAP_RESOLVER" ), "layout candidates alignment=%zu spacing=%zu", alignment.size(),
                    spacing.size() );

        for( SNAP_CANDIDATE& candidate : alignment )
            frame.candidates.push_back( std::move( candidate ) );

        for( SNAP_CANDIDATE& candidate : spacing )
            frame.candidates.push_back( std::move( candidate ) );
    }

    emitSelfAndGridCandidates( frame.candidates, context, aOrigin, nearestGrid, snapScale, snapRange, m_enableGrid );

    const int snapIn = ranges.in;
    const int snapOut = ranges.out;

    wxLogTrace( traceSnap, "  snapRange=%d, snapIn=%d, snapOut=%d", snapRange, snapIn, snapOut );
    wxLogTrace( traceSnap, "  visibleItems count=%zu, anchors count=%zu", visibleItems.size(), m_anchors.size() );
    wxLogTrace( traceSnap, "  nearest anchor: %s at (%d, %d), distance=%f", nearest ? "found" : "none",
                nearest ? nearest->pos.x : 0, nearest ? nearest->pos.y : 0,
                nearest ? nearest->Distance( aOrigin ) : -1.0 );
    wxLogTrace( traceSnap, "  nearestGrid: (%d, %d)", nearestGrid.x, nearestGrid.y );

    if( KIGFX::ANCHOR_DEBUG* ad = enableAndGetAnchorDebug(); ad )
    {
        ad->ClearAnchors();

        for( const ANCHOR& anchor : m_anchors )
            ad->AddAnchor( anchor.pos );

        ad->SetNearest( nearest ? OPT_VECTOR2I{ nearest->pos } : std::nullopt );
        m_toolMgr->GetView()->Update( ad, KIGFX::GEOMETRY );
    }

    // The distance to the nearest snap point, if any
    std::optional<int> snapDist;

    if( nearest )
        snapDist = nearest->Distance( aOrigin );

    if( m_snapItem )
    {
        int existingDist = m_snapItem->Distance( aOrigin );
        if( !snapDist || existingDist < *snapDist )
            snapDist = existingDist;
    }

    wxLogTrace( traceSnap, "  snapDist: %s (value=%d)", snapDist ? "set" : "none", snapDist ? *snapDist : -1 );
    wxLogTrace( traceSnap, "  m_snapItem: %s", m_snapItem ? "exists" : "none" );

    showConstructionGeometry( constructionEnabled );

    SNAP_MANAGER&      snapManager = getSnapManager();
    SNAP_LINE_MANAGER& snapLineManager = snapManager.GetSnapLineManager();

    const auto ptIsReferenceOnly = [&]( const VECTOR2I& aPt )
    {
        const std::vector<VECTOR2I>& referenceOnlyPoints = snapManager.GetReferenceOnlyPoints();
        return std::find( referenceOnlyPoints.begin(), referenceOnlyPoints.end(), aPt ) != referenceOnlyPoints.end();
    };

    const auto proposeConstructionForItems = [&]( const std::vector<EDA_ITEM*>& aItems )
    {
        // Add any involved item as a temporary construction item
        // (de-duplication with existing construction items is handled later)
        std::vector<BOARD_ITEM*> items;

        for( EDA_ITEM* item : aItems )
        {
            if( !item->IsBOARD_ITEM() )
                continue;

            BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( item );

            // Null items are allowed to arrive here as they represent geometry that isn't
            // specifically tied to a board item. For example snap lines from some
            // other anchor.
            // But they don't produce new construction items.
            if( boardItem )
            {
                if( m_magneticSettings->allLayers || ( ( aLayers & boardItem->GetLayerSet() ).any() ) )
                    items.push_back( boardItem );
            }
        }

        // Temporary construction items are not persistent and don't
        // overlay the items themselves (as the items will not be moved)
        if( constructionEnabled )
            AddConstructionItems( items, true, false );
    };

    const auto anchorId = [&]( const ANCHOR& aAnchor )
    {
        std::vector<SNAP_TARGET_ID> targets;

        for( const EDA_ITEM* item : aAnchor.items )
        {
            if( item )
                targets.push_back( SnapTargetId( item->m_Uuid ) );
        }

        SNAP_ID_KIND kind =
                aAnchor.flags & CONSTRUCTED ? SNAP_ID_KIND::CONSTRUCTED_ANCHOR : SNAP_ID_KIND::INTRINSIC_ANCHOR;
        SNAP_STABLE_ID pointId = MakePointSnapId( kind, aAnchor.pos, aAnchor.pointTypes );

        if( targets.empty() )
            return pointId;

        targets.push_back( pointId.target );
        return MakeCompositeSnapId( kind, targets, aAnchor.pointTypes );
    };

    const auto addAnchorCandidate = [&]( const ANCHOR& aAnchor, bool aRetained )
    {
        SNAP_STABLE_ID id = anchorId( aAnchor );

        if( frame.presentation.contains( id ) )
        {
            if( aRetained )
                frame.retainedId = id;

            return;
        }

        const bool constructed = aAnchor.flags & CONSTRUCTED;
        frame.candidates.push_back( SNAP_CANDIDATE::Point( id, SNAP_PRIORITY_TIER::OBJECT,
                                                           constructed ? SNAP_CANDIDATE_SUBTYPE::CONSTRUCTED_POINT
                                                                       : SNAP_CANDIDATE_SUBTYPE::INTRINSIC_ANCHOR,
                                                           aAnchor.pos, aAnchor.Distance( aOrigin ) / snapScale ) );
        frame.presentation.emplace(
                id, PRESENTATION{ PRESENTATION_KIND::ANCHOR_MARKER, aAnchor, !aRetained && !constructed } );

        if( aRetained )
            frame.retainedId = id;
    };

    // Hover activation, snap-line suppression and anchor acceptance are all the same question.
    const bool nearestCaptured = nearest && nearest->Distance( aOrigin ) <= snapIn;
    bool       keepConstructionProposal = false;
    bool       allowHoverActivation = false;

    if( m_enableSnap )
    {
        wxLogTrace( traceSnap, "  Snap enabled, checking snap options..." );
        allowHoverActivation = !nearestCaptured;

        if( m_enableSnapLine )
        {
            wxLogTrace( traceSnap, "    Checking snap lines..." );

            OPT_VECTOR2I snapLineSnap = snapLineManager.GetNearestSnapLinePoint( aOrigin, nearestGrid, snapDist,
                                                                                 snapRange, gridSize, GetOrigin() );

            if( !snapLineSnap && constructionEnabled )
            {
                std::optional<VECTOR2I> constructionSnap =
                        SnapToConstructionLines( aOrigin, nearestGrid, gridSize, snapRange );

                if( constructionSnap )
                    snapLineSnap = *constructionSnap;
            }

            if( snapLineSnap && m_skipPoint != *snapLineSnap )
            {
                wxLogTrace( traceSnap, "    Snap line found at (%d, %d)", snapLineSnap->x, snapLineSnap->y );

                if( !nearestCaptured )
                {
                    if( !ptIsReferenceOnly( *snapLineSnap ) )
                    {
                        SNAP_STABLE_ID id = MakePointSnapId( SNAP_ID_KIND::CONSTRUCTION, *snapLineSnap );
                        frame.candidates.push_back( SNAP_CANDIDATE::Point(
                                id, SNAP_PRIORITY_TIER::OBJECT, SNAP_CANDIDATE_SUBTYPE::CONSTRUCTED_POINT,
                                *snapLineSnap, snapLineSnap->Distance( aOrigin ) / snapScale ) );
                        frame.presentation.emplace( id, PRESENTATION{ PRESENTATION_KIND::GUIDE, std::nullopt, false } );
                    }
                    else
                    {
                        wxLogTrace( traceSnap, "    Snap line point is reference-only, continuing..." );
                        keepConstructionProposal = true;
                    }
                }
            }
        }

        if( m_snapItem )
        {
            int dist = m_snapItem->Distance( aOrigin );

            wxLogTrace( traceSnap, "    Checking existing m_snapItem, dist=%d (snapOut=%d)", dist, snapOut );

            if( dist <= snapOut && !ptIsReferenceOnly( m_snapItem->pos ) )
            {
                if( nearest && ptIsReferenceOnly( nearest->pos ) && nearest->Distance( aOrigin ) <= snapRange )
                    snapLineManager.SetSnapLineOrigin( nearest->pos );

                addAnchorCandidate( *m_snapItem, true );
            }
        }

        if( nearestCaptured )
        {
            wxLogTrace( traceSnap, "    Nearest anchor within snapIn range" );

            if( ptIsReferenceOnly( nearest->pos ) )
            {
                wxLogTrace( traceSnap, "    Nearest anchor is reference-only, setting snap line origin" );
                snapLineManager.SetSnapLineOrigin( nearest->pos );
                keepConstructionProposal = true;
            }
            else
            {
                addAnchorCandidate( *nearest, false );
            }
        }

        if( !m_enableGrid )
        {
            wxLogTrace( traceSnap, "    Grid disabled, checking point-on-element snap..." );

            OPT_VECTOR2I nearestPointOnAnElement = GetNearestPoint( m_pointOnLineCandidates, aOrigin );

            if( nearestPointOnAnElement && nearestPointOnAnElement->Distance( aOrigin ) <= snapRange )
            {
                SNAP_STABLE_ID id = MakePointSnapId( SNAP_ID_KIND::ITEM_GEOMETRY, *nearestPointOnAnElement );
                frame.candidates.push_back( SNAP_CANDIDATE::Point(
                        id, SNAP_PRIORITY_TIER::OBJECT, SNAP_CANDIDATE_SUBTYPE::FINITE_MANIFOLD,
                        *nearestPointOnAnElement, nearestPointOnAnElement->Distance( aOrigin ) / snapScale ) );
                frame.presentation.emplace( id,
                                            PRESENTATION{ PRESENTATION_KIND::POINT_ON_ELEMENT, std::nullopt, false } );
            }
        }
    }

    // Object retention wins because its tier already outranks angle restriction.
    if( !frame.retainedId && m_retainedAngleBranch )
        frame.retainedId = m_retainedAngleBranch;

    SNAP_FRAME_OUTPUT<PRESENTATION> output = ResolveSnapFrame( std::move( frame ) );
    SNAP_RESULT&                    result = output.result;
    retainAcceptedSnaps( result );

    m_snapItem = std::nullopt;
    snapLineManager.SetSnapLineEnd( std::nullopt );
    bool suppressHoverActivation = false;

    if( output.presentation )
    {
        const PRESENTATION& presentation = output.presentation->payload;
        keepConstructionProposal = true;

        if( presentation.kind == PRESENTATION_KIND::ANCHOR_MARKER && presentation.anchor )
        {
            suppressHoverActivation = true;
            m_snapItem = *presentation.anchor;
            snapLineManager.SetSnappedAnchor( m_snapItem->pos );
            updateSnapPoint( { m_snapItem->pos, m_snapItem->pointTypes } );

            if( presentation.proposeConstruction )
                proposeConstructionForItems( m_snapItem->items );
        }
        else if( presentation.kind == PRESENTATION_KIND::GUIDE )
        {
            suppressHoverActivation = true;
            snapLineManager.SetSnapLineEnd( result.position );
            m_viewSnapPoint.SetSnapTypes( POINT_TYPE::PT_NONE );
            m_toolMgr->GetView()->SetVisible( &m_viewSnapPoint, false );
        }
        else if( presentation.kind == PRESENTATION_KIND::POINT_ON_ELEMENT )
        {
            updateSnapPoint( { result.position, POINT_TYPE::PT_ON_ELEMENT } );
        }
    }
    else
    {
        m_toolMgr->GetView()->SetVisible( &m_viewSnapPoint, false );
    }

    applySnapResultGuides( result );

    static const bool canActivateByHitTest = ADVANCED_CFG::GetCfg().m_ExtensionSnapActivateOnHover;

    if( constructionEnabled && canActivateByHitTest && allowHoverActivation && !suppressHoverActivation )
    {
        for( BOARD_ITEM* item : visibleItems )
        {
            if( item->HitTest( aOrigin, 0 ) )
            {
                proposeConstructionForItems( { item } );
                keepConstructionProposal = true;
                break;
            }
        }
    }

    if( !keepConstructionProposal )
        snapManager.GetConstructionManager().CancelProposal();

    return result;
}


BOARD_ITEM* PCB_GRID_HELPER::GetSnapped() const
{
    if( !m_snapItem )
        return nullptr;

    // The snap anchor doesn't have an item associated with it
    // (odd, could it be entirely made of construction geometry?)
    if( m_snapItem->items.empty() )
        return nullptr;

    return static_cast<BOARD_ITEM*>( m_snapItem->items[0] );
}


void PCB_GRID_HELPER::ClearSnapFeedback()
{
    m_snapItem = std::nullopt;
    getSnapManager().SetDimensionBrackets( {} );
    SNAP_LINE_MANAGER& manager = getSnapManager().GetSnapLineManager();
    manager.ClearSnapLine();
    m_toolMgr->GetView()->SetVisible( &m_viewSnapPoint, false );
}


GRID_HELPER_GRIDS PCB_GRID_HELPER::GetItemGrid( const EDA_ITEM* aItem ) const
{
    if( !aItem )
        return GRID_CURRENT;

    switch( aItem->Type() )
    {
    case PCB_FOOTPRINT_T:
    case PCB_PAD_T:
        return GRID_CONNECTABLE;

    case PCB_TEXT_T:
    case PCB_FIELD_T:
        return GRID_TEXT;

    case PCB_SHAPE_T:
    case PCB_DIMENSION_T:
    case PCB_REFERENCE_IMAGE_T:
    case PCB_TEXTBOX_T:
    case PCB_BARCODE_T:
        return GRID_GRAPHICS;

    case PCB_TRACE_T:
    case PCB_ARC_T:
        return GRID_WIRES;

    case PCB_VIA_T:
        return GRID_VIAS;

    default:
        return GRID_CURRENT;
    }
}


VECTOR2D PCB_GRID_HELPER::GetGridSize( GRID_HELPER_GRIDS aGrid ) const
{
    const GRID_SETTINGS& grid = m_toolMgr->GetSettings()->m_Window.grid;
    int                  idx = -1;

    VECTOR2D g = m_toolMgr->GetView()->GetGAL()->GetGridSize();

    if( !grid.overrides_enabled )
        return g;

    switch( aGrid )
    {
    case GRID_CONNECTABLE:
        if( grid.override_connected )
            idx = grid.override_connected_idx;

        break;

    case GRID_WIRES:
        if( grid.override_wires )
            idx = grid.override_wires_idx;

        break;

    case GRID_VIAS:
        if( grid.override_vias )
            idx = grid.override_vias_idx;

        break;

    case GRID_TEXT:
        if( grid.override_text )
            idx = grid.override_text_idx;

        break;

    case GRID_GRAPHICS:
        if( grid.override_graphics )
            idx = grid.override_graphics_idx;

        break;

    default:
        break;
    }

    if( idx >= 0 && idx < (int) grid.grids.size() )
        g = grid.grids[idx].ToDouble( pcbIUScale );

    return g;
}


std::vector<BOARD_ITEM*> PCB_GRID_HELPER::queryVisible( std::initializer_list<BOX2I>    aAreas,
                                                        const std::vector<BOARD_ITEM*>& aSkip ) const
{
    std::vector<BOARD_ITEM*>                  items;
    std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> visibleItems;

    const bool           inFootprintEditor = editingInsideFootprint();
    KIGFX::VIEW*         view = m_toolMgr->GetView();
    RENDER_SETTINGS*     settings = view->GetPainter()->GetSettings();
    const std::set<int>& activeLayers = settings->GetHighContrastLayers();
    bool                 isHighContrast = settings->GetHighContrast();

    for( const BOX2I& area : aAreas )
    {
        if( area.GetWidth() > 0 && area.GetHeight() > 0 )
            view->Query( area, visibleItems );
    }

    for( const auto& [viewItem, layer] : visibleItems )
    {
        if( !viewItem->IsBOARD_ITEM() )
            continue;

        BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( viewItem );

        if( inFootprintEditor )
        {
            // If we are in the footprint editor, don't use the footprint itself
            if( boardItem->Type() == PCB_FOOTPRINT_T )
                continue;
        }
        else
        {
            // If we are not in the footprint editor, don't use footprint-editor-private items
            if( FOOTPRINT* parentFP = boardItem->GetParentFootprint() )
            {
                if( IsPcbLayer( layer ) && parentFP->GetPrivateLayers().test( layer ) )
                    continue;
            }
        }

        // The boardItem must be visible and on an active layer
        if( view->IsVisible( boardItem ) && ( !isHighContrast || activeLayers.count( layer ) )
            && boardItem->ViewGetLOD( layer, view ) < view->GetScale() )
        {
            items.push_back( boardItem );
        }
    }

    std::sort( items.begin(), items.end(), std::less<>() );
    items.erase( std::unique( items.begin(), items.end() ), items.end() );

    std::unordered_set<BOARD_ITEM*> skippedItems;

    for( BOARD_ITEM* item : aSkip )
    {
        if( !item )
            continue;

        skippedItems.insert( item );
        item->RunOnChildren(
                [&]( BOARD_ITEM* aChild )
                {
                    skippedItems.insert( aChild );
                },
                RECURSE_MODE::RECURSE );
    }

    items.erase( std::remove_if( items.begin(), items.end(),
                                 [&]( BOARD_ITEM* aItem )
                                 {
                                     return skippedItems.contains( aItem );
                                 } ),
                 items.end() );

    return items;
}


struct PCB_INTERSECTABLE
{
    BOARD_ITEM*        Item;
    INTERSECTABLE_GEOM Geometry;

    // Clang wants this constructor
    PCB_INTERSECTABLE( BOARD_ITEM* aItem, INTERSECTABLE_GEOM aSeg ) :
            Item( aItem ),
            Geometry( std::move( aSeg ) )
    {
    }
};


void PCB_GRID_HELPER::computeAnchors( const std::vector<BOARD_ITEM*>& aItems, const VECTOR2I& aRefPos, bool aFrom,
                                      const PCB_SELECTION_FILTER_OPTIONS* aSelectionFilter, const LSET* aMatchLayers,
                                      bool aForDrag )
{
    std::vector<PCB_INTERSECTABLE> intersectables;
    intersectables.reserve( aItems.size() );

    // These could come from a more granular snap mode filter
    // But when looking for drag points, we don't want construction geometry
    const bool computeIntersections = !aForDrag;
    const bool computePointsOnElements = !aForDrag;
    const bool excludeGraphics = aSelectionFilter && !aSelectionFilter->graphics;
    const bool excludeTracks = aSelectionFilter && !aSelectionFilter->tracks;

    const auto itemIsSnappable =
            [&]( const BOARD_ITEM& aItem )
            {
                // If we are filtering by layers, check if the item matches
                if( aMatchLayers )
                    return m_magneticSettings->allLayers || ( ( *aMatchLayers & aItem.GetLayerSet() ).any() );

                return true;
            };

    const auto processItem =
            [&]( BOARD_ITEM& item )
            {
                // Don't even process the item if it doesn't match the layers
                if( !itemIsSnappable( item ) )
                    return;

                // First, add all the key points of the item itself
                computeAnchors( &item, aRefPos, aFrom, aSelectionFilter );

                // If we are computing intersections, construct the relevant intersectables
                // Points on elements also use the intersectables.
                if( computeIntersections || computePointsOnElements )
                {
                    std::optional<INTERSECTABLE_GEOM> intersectableGeom;

                    if( !excludeGraphics
                        && ( item.Type() == PCB_SHAPE_T || item.Type() == PCB_REFERENCE_IMAGE_T ) )
                    {
                        intersectableGeom = GetBoardIntersectable( item );
                    }
                    else if( !excludeTracks && ( item.Type() == PCB_TRACE_T || item.Type() == PCB_ARC_T ) )
                    {
                        intersectableGeom = GetBoardIntersectable( item );
                    }

                    if( intersectableGeom )
                        intersectables.emplace_back( &item, *intersectableGeom );
                }
            };

    for( BOARD_ITEM* item : aItems )
    {
        processItem( *item );
    }

    for( const CONSTRUCTION_MANAGER::CONSTRUCTION_ITEM_BATCH& batch : getSnapManager().GetConstructionItems() )
    {
        for( const CONSTRUCTION_MANAGER::CONSTRUCTION_ITEM& constructionItem : batch )
        {
            BOARD_ITEM* involvedItem = static_cast<BOARD_ITEM*>( constructionItem.Item );

            for( const CONSTRUCTION_MANAGER::CONSTRUCTION_ITEM::DRAWABLE_ENTRY& drawable : constructionItem.Constructions )
            {
                std::visit(
                        [&]( const auto& visited )
                        {
                            using ItemType = std::decay_t<decltype( visited )>;

                            if constexpr( std::is_same_v<ItemType, LINE>
                                          || std::is_same_v<ItemType, CIRCLE>
                                          || std::is_same_v<ItemType, HALF_LINE>
                                          || std::is_same_v<ItemType, SHAPE_ARC> )
                            {
                                intersectables.emplace_back( involvedItem, visited );
                            }
                            else if constexpr( std::is_same_v<ItemType, VECTOR2I> )
                            {
                                // Add any free-floating points as snap points.
                                addAnchor( visited, SNAPPABLE | CONSTRUCTED, involvedItem, POINT_TYPE::PT_NONE );
                            }
                        },
                        drawable.Drawable );
            }
        }
    }

    // Now, add all the intersections between the items
    // This is obviously quadratic, so performance may be a concern for large selections
    // But, so far up to ~20k comparisons seems not to be an issue with run times in the ms range
    // and it's usually only a handful of items.

    if( computeIntersections )
    {
        for( std::size_t ii = 0; ii < intersectables.size(); ++ii )
        {
            const PCB_INTERSECTABLE& intersectableA = intersectables[ii];

            for( std::size_t jj = ii + 1; jj < intersectables.size(); ++jj )
            {
                const PCB_INTERSECTABLE& intersectableB = intersectables[jj];

                // An item and its own extension will often have intersections (as they are on top of each other),
                // but they not useful points to snap to
                if( intersectableA.Item == intersectableB.Item )
                    continue;

                std::vector<VECTOR2I>      intersections;
                const INTERSECTION_VISITOR visitor{ intersectableA.Geometry, intersections };

                std::visit( visitor, intersectableB.Geometry );

                // For each intersection, add an intersection snap anchor
                for( const VECTOR2I& intersection : intersections )
                {
                    std::vector<EDA_ITEM*> items = {
                        intersectableA.Item,
                        intersectableB.Item,
                    };
                    addAnchor( intersection, SNAPPABLE | CONSTRUCTED, std::move( items ),
                               POINT_TYPE::PT_INTERSECTION );
                }
            }
        }
    }

    // The intersectables can also be used for fall-back snapping to "point on line"
    // snaps if no other snap is found
    m_pointOnLineCandidates.clear();

    if( computePointsOnElements )
    {
        // For the moment, it's trivial to make a NEARABLE from an INTERSECTABLE,
        // because all INTERSECTABLEs are also NEARABLEs.
        for( const PCB_INTERSECTABLE& intersectable : intersectables )
        {
            std::visit(
                    [&]( const auto& geom )
                    {
                        NEARABLE_GEOM nearable( geom );
                        m_pointOnLineCandidates.emplace_back( nearable );
                    },
                    intersectable.Geometry );
        }
    }
}


// Padstacks report a set of "unique" layers, which may each represent one or more
// "real" layers. This function takes a unique layer and checks if it applies to the
// given "real" layer.
static bool PadstackUniqueLayerAppliesToLayer( const PADSTACK& aPadStack, PCB_LAYER_ID aPadstackUniqueLayer,
                                               const PCB_LAYER_ID aRealLayer )
{
    switch( aPadStack.Mode() )
    {
    case PADSTACK::MODE::NORMAL:
    {
        // Normal mode padstacks are the same on every layer, so they'll apply to any
        // "real" copper layer.
        return IsCopperLayer( aRealLayer );
    }
    case PADSTACK::MODE::FRONT_INNER_BACK:
    {
        switch( aPadstackUniqueLayer )
        {
        case F_Cu:
        case B_Cu:
            // The outer-layer uhique layers only apply to those exact "real" layers
            return aPadstackUniqueLayer == aRealLayer;
        case PADSTACK::INNER_LAYERS:
            // But the inner layers apply to any inner layer
            return IsInnerCopperLayer( aRealLayer );
        default:
            wxFAIL_MSG( wxString::Format( "Unexpected padstack unique layer %d in FRONT_INNER_BACK mode",
                                          aPadstackUniqueLayer ) );
            break;
        }
        break;
    }
    case PADSTACK::MODE::CUSTOM:
    {
        // Custom modes are unique per layer, so it's 1:1
        return aRealLayer == aPadstackUniqueLayer;
    }
    }

    return false;
};


std::vector<PCB_GRID_HELPER::ANCHOR_SPEC> PCB_GRID_HELPER::GetArcAnchors( const PCB_ARC& aArc,
                                                                         bool aFrom )
{
    std::vector<ANCHOR_SPEC> anchors;

    // The stored midpoint is grid-aligned when the arc is; expose it alongside the endpoints so
    // BestDragOrigin picks a grid-aligned corner as the drag/paste reference.
    anchors.push_back( { aArc.GetMid(), CORNER | SNAPPABLE, POINT_TYPE::PT_MID } );

    // The derived geometric center is rarely grid-aligned. It stays available as a drag origin for
    // other items (aFrom=false) but is never offered as this arc's own origin, which was the cause
    // of pasted arcs landing off grid.
    if( !aFrom )
        anchors.push_back( { aArc.GetCenter(), ORIGIN, POINT_TYPE::PT_CENTER } );

    return anchors;
}


void PCB_GRID_HELPER::computeAnchors( BOARD_ITEM* aItem, const VECTOR2I& aRefPos, bool aFrom,
                                      const PCB_SELECTION_FILTER_OPTIONS* aSelectionFilter )
{
    KIGFX::VIEW*         view = m_toolMgr->GetView();
    RENDER_SETTINGS*     settings = view->GetPainter()->GetSettings();
    const std::set<int>& activeLayers = settings->GetHighContrastLayers();
    const PCB_LAYER_ID   activeHighContrastPrimaryLayer = settings->GetPrimaryHighContrastLayer();
    bool                 isHighContrast = settings->GetHighContrast();

    const auto checkVisibility =
            [&]( const BOARD_ITEM* item )
            {
                // New moved items don't yet have view flags so VIEW will call them invisible
                if( !view->IsVisible( item ) && !item->IsMoving() )
                    return false;

                bool onActiveLayer = !isHighContrast;
                bool isLODVisible = false;

                for( PCB_LAYER_ID layer : item->GetLayerSet() )
                {
                    if( !onActiveLayer && activeLayers.count( layer ) )
                        onActiveLayer = true;

                    if( !isLODVisible && item->ViewGetLOD( layer, view ) < view->GetScale() )
                        isLODVisible = true;

                    if( onActiveLayer && isLODVisible )
                        return true;
                }

                return false;
            };

    // As defaults, these are probably reasonable to avoid spamming key points
    const KIGEOM::OVAL_KEY_POINT_FLAGS ovalKeyPointFlags = KIGEOM::OVAL_CENTER
                                                           | KIGEOM::OVAL_CAP_TIPS
                                                           | KIGEOM::OVAL_SIDE_MIDPOINTS
                                                           | KIGEOM::OVAL_CARDINAL_EXTREMES;

    auto handlePadShape =
            [&]( PAD* aPad, PCB_LAYER_ID aLayer )
            {
                addAnchor( aPad->GetPosition(), ORIGIN | SNAPPABLE, aPad, POINT_TYPE::PT_CENTER );

                /// If we are getting a drag point, we don't want to center the edge of pads
                if( aFrom )
                    return;

                switch( aPad->GetShape( aLayer ) )
                {
                case PAD_SHAPE::CIRCLE:
                {
                    const CIRCLE circle( aPad->ShapePos( aLayer ), aPad->GetSizeX() / 2 );

                    for( const TYPED_POINT2I& pt : KIGEOM::GetCircleKeyPoints( circle, false ) )
                        addAnchor( pt.m_point, OUTLINE | SNAPPABLE, aPad, pt.m_types );

                    break;
                }
                case PAD_SHAPE::OVAL:
                {
                    const SHAPE_SEGMENT oval = SHAPE_SEGMENT::BySizeAndCenter(
                            aPad->GetSize( aLayer ), aPad->GetPosition(), aPad->GetOrientation() );

                    for( const TYPED_POINT2I& pt : KIGEOM::GetOvalKeyPoints( oval, ovalKeyPointFlags ) )
                        addAnchor( pt.m_point, OUTLINE | SNAPPABLE, aPad, pt.m_types );

                    break;
                }
                case PAD_SHAPE::RECTANGLE:
                case PAD_SHAPE::TRAPEZOID:
                case PAD_SHAPE::ROUNDRECT:
                case PAD_SHAPE::CHAMFERED_RECT:
                {
                    VECTOR2I half_size( aPad->GetSize( aLayer ) / 2 );
                    VECTOR2I trap_delta( 0, 0 );

                    if( aPad->GetShape( aLayer ) == PAD_SHAPE::TRAPEZOID )
                        trap_delta = aPad->GetDelta( aLayer ) / 2;

                    SHAPE_LINE_CHAIN corners;

                    corners.Append( -half_size.x - trap_delta.y, half_size.y + trap_delta.x );
                    corners.Append( half_size.x + trap_delta.y, half_size.y - trap_delta.x );
                    corners.Append( half_size.x - trap_delta.y, -half_size.y + trap_delta.x );
                    corners.Append( -half_size.x + trap_delta.y, -half_size.y - trap_delta.x );
                    corners.SetClosed( true );

                    corners.Rotate( aPad->GetOrientation() );
                    corners.Move( aPad->ShapePos( aLayer ) );

                    for( std::size_t ii = 0; ii < corners.GetSegmentCount(); ++ii )
                    {
                        const SEG& seg = corners.GetSegment( ii );
                        addAnchor( seg.A, OUTLINE | SNAPPABLE, aPad, POINT_TYPE::PT_CORNER );
                        addAnchor( seg.Center(), OUTLINE | SNAPPABLE, aPad, POINT_TYPE::PT_MID );

                        if( ii == corners.GetSegmentCount() - 1 )
                            addAnchor( seg.B, OUTLINE | SNAPPABLE, aPad, POINT_TYPE::PT_CORNER );
                    }

                    break;
                }

                default:
                {
                    const auto& outline = aPad->GetEffectivePolygon( aLayer, ERROR_INSIDE );

                    if( !outline->IsEmpty() )
                    {
                        for( const VECTOR2I& pt : outline->Outline( 0 ).CPoints() )
                            addAnchor( pt, OUTLINE | SNAPPABLE, aPad );
                    }

                    break;
                }
                }

                if( aPad->HasHole() )
                {
                    // Holes are at the pad centre (it's the shape that may be offset)
                    const VECTOR2I hole_pos = aPad->GetPosition();
                    const VECTOR2I hole_size = aPad->GetDrillSize();

                    std::vector<TYPED_POINT2I> snap_pts;

                    if( hole_size.x == hole_size.y )
                    {
                        // Circle
                        const CIRCLE circle( hole_pos, hole_size.x / 2 );
                        snap_pts = KIGEOM::GetCircleKeyPoints( circle, true );
                    }
                    else
                    {
                        // Oval

                        // For now there's no way to have an off-angle hole, so this is the
                        // same as the pad. In future, this may not be true:
                        // https://gitlab.com/kicad/code/kicad/-/issues/4124
                        const SHAPE_SEGMENT oval =
                                SHAPE_SEGMENT::BySizeAndCenter( hole_size, hole_pos, aPad->GetOrientation() );
                        snap_pts = KIGEOM::GetOvalKeyPoints( oval, ovalKeyPointFlags );
                    }

                    for( const TYPED_POINT2I& snap_pt : snap_pts )
                        addAnchor( snap_pt.m_point, OUTLINE | SNAPPABLE, aPad, snap_pt.m_types );
                }
            };

    const auto addRectPoints =
            [&]( const BOX2I& aBox, EDA_ITEM& aRelatedItem )
            {
                const VECTOR2I topRight( aBox.GetRight(), aBox.GetTop() );
                const VECTOR2I bottomLeft( aBox.GetLeft(), aBox.GetBottom() );

                const SEG first( aBox.GetOrigin(), topRight );
                const SEG second( topRight, aBox.GetEnd() );
                const SEG third( aBox.GetEnd(), bottomLeft );
                const SEG fourth( bottomLeft, aBox.GetOrigin() );

                const int snapFlags = CORNER | SNAPPABLE;

                addAnchor( aBox.GetCenter(), snapFlags, &aRelatedItem, POINT_TYPE::PT_CENTER );

                addAnchor( first.A,         snapFlags, &aRelatedItem, POINT_TYPE::PT_CORNER );
                addAnchor( first.Center(),  snapFlags, &aRelatedItem, POINT_TYPE::PT_MID );
                addAnchor( second.A,        snapFlags, &aRelatedItem, POINT_TYPE::PT_CORNER );
                addAnchor( second.Center(), snapFlags, &aRelatedItem, POINT_TYPE::PT_MID );
                addAnchor( third.A,         snapFlags, &aRelatedItem, POINT_TYPE::PT_CORNER );
                addAnchor( third.Center(),  snapFlags, &aRelatedItem, POINT_TYPE::PT_MID );
                addAnchor( fourth.A,        snapFlags, &aRelatedItem, POINT_TYPE::PT_CORNER );
                addAnchor( fourth.Center(), snapFlags, &aRelatedItem, POINT_TYPE::PT_MID );
            };

    const auto handleShape =
            [&]( PCB_SHAPE* shape )
            {
                VECTOR2I   start = shape->GetStart();
                VECTOR2I   end = shape->GetEnd();

                switch( shape->GetShape() )
                {
                case SHAPE_T::CIRCLE:
                {
                    const int r = ( start - end ).EuclideanNorm();

                    addAnchor( start, ORIGIN | SNAPPABLE, shape, POINT_TYPE::PT_CENTER );

                    addAnchor( start + VECTOR2I( -r, 0 ), OUTLINE | SNAPPABLE, shape, POINT_TYPE::PT_QUADRANT );
                    addAnchor( start + VECTOR2I( r, 0 ), OUTLINE | SNAPPABLE, shape, POINT_TYPE::PT_QUADRANT );
                    addAnchor( start + VECTOR2I( 0, -r ), OUTLINE | SNAPPABLE, shape, POINT_TYPE::PT_QUADRANT );
                    addAnchor( start + VECTOR2I( 0, r ), OUTLINE | SNAPPABLE, shape, POINT_TYPE::PT_QUADRANT );
                    break;
                }

                case SHAPE_T::ARC:
                    addAnchor( shape->GetStart(), CORNER | SNAPPABLE, shape, POINT_TYPE::PT_END );
                    addAnchor( shape->GetEnd(), CORNER | SNAPPABLE, shape, POINT_TYPE::PT_END );
                    addAnchor( shape->GetArcMid(), CORNER | SNAPPABLE, shape, POINT_TYPE::PT_MID );
                    addAnchor( shape->GetCenter(), ORIGIN | SNAPPABLE, shape, POINT_TYPE::PT_CENTER );
                    break;

                case SHAPE_T::RECTANGLE:
                {
                    addRectPoints( BOX2I::ByCorners( start, end ), *shape );
                    break;
                }

                case SHAPE_T::SEGMENT:
                    addAnchor( start, CORNER | SNAPPABLE, shape, POINT_TYPE::PT_END );
                    addAnchor( end, CORNER | SNAPPABLE, shape, POINT_TYPE::PT_END );
                    addAnchor( shape->GetCenter(), CORNER | SNAPPABLE, shape, POINT_TYPE::PT_MID );
                    break;

                case SHAPE_T::POLY:
                {
                    SHAPE_LINE_CHAIN lc;
                    lc.SetClosed( true );
                    for( const VECTOR2I& p : shape->GetPolyPoints() )
                    {
                        addAnchor( p, CORNER | SNAPPABLE, shape, POINT_TYPE::PT_CORNER );
                        lc.Append( p );
                    }

                    addAnchor( lc.NearestPoint( aRefPos ), OUTLINE, aItem );
                    break;
                }

                case SHAPE_T::ELLIPSE:
                {
                    VECTOR2I  center = shape->GetEllipseCenter();
                    int       majorR = shape->GetEllipseMajorRadius();
                    int       minorR = shape->GetEllipseMinorRadius();
                    EDA_ANGLE rot = shape->GetEllipseRotation();
                    VECTOR2I  majorEnd( KiROUND( majorR * rot.Cos() ), KiROUND( majorR * rot.Sin() ) );
                    VECTOR2I  minorEnd( KiROUND( -minorR * rot.Sin() ), KiROUND( minorR * rot.Cos() ) );

                    addAnchor( center, ORIGIN | SNAPPABLE, shape, POINT_TYPE::PT_CENTER );
                    addAnchor( center + majorEnd, OUTLINE | SNAPPABLE, shape, POINT_TYPE::PT_QUADRANT );
                    addAnchor( center - majorEnd, OUTLINE | SNAPPABLE, shape, POINT_TYPE::PT_QUADRANT );
                    addAnchor( center + minorEnd, OUTLINE | SNAPPABLE, shape, POINT_TYPE::PT_QUADRANT );
                    addAnchor( center - minorEnd, OUTLINE | SNAPPABLE, shape, POINT_TYPE::PT_QUADRANT );
                    break;
                }

                case SHAPE_T::ELLIPSE_ARC:
                {
                    addAnchor( shape->GetStart(), CORNER | SNAPPABLE, shape, POINT_TYPE::PT_END );
                    addAnchor( shape->GetEnd(), CORNER | SNAPPABLE, shape, POINT_TYPE::PT_END );
                    addAnchor( shape->GetEllipseCenter(), ORIGIN | SNAPPABLE, shape, POINT_TYPE::PT_CENTER );
                    break;
                }

                case SHAPE_T::BEZIER:
                    addAnchor( start, CORNER | SNAPPABLE, shape, POINT_TYPE::PT_END );
                    addAnchor( end, CORNER | SNAPPABLE, shape, POINT_TYPE::PT_END );
                    KI_FALLTHROUGH;

                default:
                    addAnchor( shape->GetPosition(), ORIGIN | SNAPPABLE, shape );
                    break;
                }
            };

    switch( aItem->Type() )
    {
    case PCB_FOOTPRINT_T:
    {
        FOOTPRINT* footprint = static_cast<FOOTPRINT*>( aItem );
        bool       footprintVisible = checkVisibility( footprint );

        for( PAD* pad : footprint->Pads() )
        {
            if( aFrom )
            {
                if( aSelectionFilter && !aSelectionFilter->pads )
                    continue;
            }
            else
            {
                if( m_magneticSettings->pads != MAGNETIC_OPTIONS::CAPTURE_ALWAYS )
                    continue;
            }

            if( !checkVisibility( pad ) )
                continue;

            if( !pad->GetBoundingBox().Contains( aRefPos ) )
                continue;

            pad->Padstack().ForEachUniqueLayer(
                    [&]( PCB_LAYER_ID aLayer )
                    {
                        if( !isHighContrast
                            || PadstackUniqueLayerAppliesToLayer( pad->Padstack(), aLayer,
                                                                  activeHighContrastPrimaryLayer ) )
                        {
                            handlePadShape( pad, aLayer );
                        }
                    } );
        }

        // Points are also pick-up points
        for( const PCB_POINT* pt : footprint->Points() )
        {
            if( aSelectionFilter && !aSelectionFilter->points )
                continue;

            if( !checkVisibility( pt ) )
                continue;

            addAnchor( pt->GetPosition(), ORIGIN | SNAPPABLE, footprint, POINT_TYPE::PT_CENTER );
        }

        // When computing drag origins (aFrom=true), always proceed to add the footprint
        // position anchor regardless of the visibility state. The footprint is already
        // selected, so its anchor must be reachable as a drag point even if the active layer
        // or zoom level causes checkVisibility to return false. Snapping TO an external
        // footprint (aFrom=false) should still respect visibility.
        if( !footprintVisible && !aFrom )
            break;

        if( aFrom && aSelectionFilter && !aSelectionFilter->footprints )
            break;

        // Snap to the footprint origin so that move operations keep the part aligned to
        // the grid regardless of anchor layer visibility, but not when the footprint's
        // side is hidden.
        int fpRenderLayer = ( footprint->GetLayer() == F_Cu ) ? LAYER_FOOTPRINTS_FR
                            : ( footprint->GetLayer() == B_Cu ) ? LAYER_FOOTPRINTS_BK
                                                                 : LAYER_ANCHOR;

        if( !view->IsLayerVisible( fpRenderLayer ) )
            break;

        VECTOR2I position = footprint->GetPosition();
        VECTOR2I center = footprint->GetBoundingBox( false ).Centre();
        VECTOR2I grid( GetGrid() );

        addAnchor( position, ORIGIN | SNAPPABLE, footprint, POINT_TYPE::PT_CENTER );

        if( ( center - position ).SquaredEuclideanNorm() > grid.SquaredEuclideanNorm() )
            addAnchor( center, ORIGIN | SNAPPABLE, footprint, POINT_TYPE::PT_CENTER );

        break;
    }

    case PCB_PAD_T:
        if( aFrom )
        {
            if( aSelectionFilter && !aSelectionFilter->pads )
                break;
        }
        else
        {
            if( m_magneticSettings->pads != MAGNETIC_OPTIONS::CAPTURE_ALWAYS )
                break;
        }

        if( checkVisibility( aItem ) )
        {
            PAD* pad = static_cast<PAD*>( aItem );

            pad->Padstack().ForEachUniqueLayer(
                    [&]( PCB_LAYER_ID aLayer )
                    {
                        if( !isHighContrast
                            || PadstackUniqueLayerAppliesToLayer( pad->Padstack(), aLayer,
                                                                  activeHighContrastPrimaryLayer ) )
                        {
                            handlePadShape( pad, aLayer );
                        }
                    } );
        }

        break;

    case PCB_TEXTBOX_T:
        if( aFrom )
        {
            if( aSelectionFilter && !aSelectionFilter->text )
                break;
        }
        else
        {
            if( !m_magneticSettings->graphics )
                break;
        }

        if( checkVisibility( aItem ) )
            handleShape( static_cast<PCB_SHAPE*>( aItem ) );

        break;

    case PCB_TABLE_T:
        if( aFrom )
        {
            if( aSelectionFilter && !aSelectionFilter->text )
                break;
        }
        else
        {
            if( !m_magneticSettings->graphics )
                break;
        }

        if( checkVisibility( aItem ) )
        {
            PCB_TABLE* table = static_cast<PCB_TABLE*>( aItem );

            EDA_ANGLE drawAngle = table->GetCell( 0, 0 )->GetDrawRotation();
            VECTOR2I  topLeft = table->GetCell( 0, 0 )->GetCornersInSequence( drawAngle )[0];
            VECTOR2I  bottomLeft =
                    table->GetCell( table->GetRowCount() - 1, 0 )->GetCornersInSequence( drawAngle )[3];
            VECTOR2I topRight = table->GetCell( 0, table->GetColCount() - 1 )->GetCornersInSequence( drawAngle )[1];
            VECTOR2I bottomRight = table->GetCell( table->GetRowCount() - 1, table->GetColCount() - 1 )
                                           ->GetCornersInSequence( drawAngle )[2];

            addAnchor( topLeft, CORNER | SNAPPABLE, table, POINT_TYPE::PT_END );
            addAnchor( bottomLeft, CORNER | SNAPPABLE, table, POINT_TYPE::PT_END );
            addAnchor( topRight, CORNER | SNAPPABLE, table, POINT_TYPE::PT_END );
            addAnchor( bottomRight, CORNER | SNAPPABLE, table, POINT_TYPE::PT_END );

            addAnchor( table->GetCenter(), ORIGIN, table, POINT_TYPE::PT_MID );
        }

        break;

    case PCB_SHAPE_T:
        if( aFrom )
        {
            if( aSelectionFilter && !aSelectionFilter->graphics )
                break;
        }
        else
        {
            if( !m_magneticSettings->graphics )
                break;
        }

        if( checkVisibility( aItem ) )
            handleShape( static_cast<PCB_SHAPE*>( aItem ) );

        break;

    case PCB_TRACE_T:
    case PCB_ARC_T:
        if( aFrom )
        {
            if( aSelectionFilter && !aSelectionFilter->tracks )
                break;
        }
        else
        {
            if( m_magneticSettings->tracks != MAGNETIC_OPTIONS::CAPTURE_ALWAYS )
                break;
        }

        if( checkVisibility( aItem ) )
        {
            PCB_TRACK* track = static_cast<PCB_TRACK*>( aItem );

            addAnchor( track->GetStart(), CORNER | SNAPPABLE, track, POINT_TYPE::PT_END );
            addAnchor( track->GetEnd(), CORNER | SNAPPABLE, track, POINT_TYPE::PT_END );

            if( aItem->Type() == PCB_ARC_T )
            {
                PCB_ARC* arc = static_cast<PCB_ARC*>( aItem );

                for( const ANCHOR_SPEC& spec : GetArcAnchors( *arc, aFrom ) )
                    addAnchor( spec.pos, spec.flags, arc, spec.pointType );
            }
            else
            {
                addAnchor( track->GetCenter(), ORIGIN, track, POINT_TYPE::PT_MID );
            }
        }

        break;

    case PCB_MARKER_T:
    case PCB_TARGET_T:
        addAnchor( aItem->GetPosition(), ORIGIN | CORNER | SNAPPABLE, aItem, POINT_TYPE::PT_CENTER );
        break;

    case PCB_POINT_T:
        if( aSelectionFilter && !aSelectionFilter->points )
            break;

        if( checkVisibility( aItem ) )
            addAnchor( aItem->GetPosition(), ORIGIN | SNAPPABLE, aItem, POINT_TYPE::PT_CENTER );

        break;

    case PCB_VIA_T:
        if( aFrom )
        {
            if( aSelectionFilter && !aSelectionFilter->vias )
                break;
        }
        else
        {
            if( m_magneticSettings->tracks != MAGNETIC_OPTIONS::CAPTURE_ALWAYS )
                break;
        }

        if( checkVisibility( aItem ) )
            addAnchor( aItem->GetPosition(), ORIGIN | CORNER | SNAPPABLE, aItem, POINT_TYPE::PT_CENTER );

        break;

    case PCB_ZONE_T:
        if( aFrom && aSelectionFilter && !aSelectionFilter->zones )
            break;

        if( checkVisibility( aItem ) )
        {
            const SHAPE_POLY_SET* outline = static_cast<const ZONE*>( aItem )->Outline();

            SHAPE_LINE_CHAIN lc;
            lc.SetClosed( true );

            for( auto iter = outline->CIterateWithHoles(); iter; iter++ )
            {
                addAnchor( *iter, CORNER | SNAPPABLE, aItem, POINT_TYPE::PT_CORNER );
                lc.Append( *iter );
            }

            addAnchor( lc.NearestPoint( aRefPos ), OUTLINE, aItem );
        }

        break;

    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_ORTHOGONAL_T:
        if( aFrom && aSelectionFilter && !aSelectionFilter->dimensions )
            break;

        if( checkVisibility( aItem ) )
        {
            PCB_DIM_ALIGNED* dim = static_cast<PCB_DIM_ALIGNED*>( aItem );
            addAnchor( dim->GetCrossbarStart(), CORNER | SNAPPABLE, dim );
            addAnchor( dim->GetCrossbarEnd(), CORNER | SNAPPABLE, dim );
            addAnchor( dim->GetStart(), CORNER | SNAPPABLE, dim );
            addAnchor( dim->GetEnd(), CORNER | SNAPPABLE, dim );
        }

        break;

    case PCB_DIM_CENTER_T:
        if( aFrom && aSelectionFilter && !aSelectionFilter->dimensions )
            break;

        if( checkVisibility( aItem ) )
        {
            PCB_DIM_CENTER* dim = static_cast<PCB_DIM_CENTER*>( aItem );
            addAnchor( dim->GetStart(), CORNER | SNAPPABLE, dim );
            addAnchor( dim->GetEnd(), CORNER | SNAPPABLE, dim );

            VECTOR2I start( dim->GetStart() );
            VECTOR2I radial( dim->GetEnd() - dim->GetStart() );

            for( int i = 0; i < 2; i++ )
            {
                RotatePoint( radial, -ANGLE_90 );
                addAnchor( start + radial, CORNER | SNAPPABLE, dim );
            }
        }

        break;

    case PCB_DIM_RADIAL_T:
        if( aFrom && aSelectionFilter && !aSelectionFilter->dimensions )
            break;

        if( checkVisibility( aItem ) )
        {
            PCB_DIM_RADIAL* radialDim = static_cast<PCB_DIM_RADIAL*>( aItem );
            addAnchor( radialDim->GetStart(), CORNER | SNAPPABLE, radialDim );
            addAnchor( radialDim->GetEnd(), CORNER | SNAPPABLE, radialDim );
            addAnchor( radialDim->GetKnee(), CORNER | SNAPPABLE, radialDim );
            addAnchor( radialDim->GetTextPos(), CORNER | SNAPPABLE, radialDim );
        }

        break;

    case PCB_DIM_LEADER_T:
        if( aFrom && aSelectionFilter && !aSelectionFilter->dimensions )
            break;

        if( checkVisibility( aItem ) )
        {
            PCB_DIM_LEADER* leader = static_cast<PCB_DIM_LEADER*>( aItem );
            addAnchor( leader->GetStart(), CORNER | SNAPPABLE, leader );
            addAnchor( leader->GetEnd(), CORNER | SNAPPABLE, leader );
            addAnchor( leader->GetTextPos(), CORNER | SNAPPABLE, leader );
        }

        break;

    case PCB_FIELD_T:
    case PCB_TEXT_T:
        if( aFrom && aSelectionFilter && !aSelectionFilter->text )
            break;

        if( checkVisibility( aItem ) )
            addAnchor( aItem->GetPosition(), ORIGIN, aItem );

        break;

    case PCB_BARCODE_T:
        if( aFrom && aSelectionFilter && !aSelectionFilter->otherItems )
            break;

        if( checkVisibility( aItem ) )
        {
            PCB_BARCODE* barcode = static_cast<PCB_BARCODE*>( aItem );
            const BOX2I  bbox = barcode->GetSymbolPoly().BBox();

            addAnchor( aItem->GetPosition(), ORIGIN, barcode, POINT_TYPE::PT_CENTER );
            addRectPoints( bbox, *barcode );
        }

        break;

    case PCB_GROUP_T:
        for( BOARD_ITEM* item : static_cast<PCB_GROUP*>( aItem )->GetBoardItems() )
        {
            if( checkVisibility( item ) )
                computeAnchors( item, aRefPos, aFrom, nullptr );
        }

        break;

    case PCB_REFERENCE_IMAGE_T:
        if( aFrom && aSelectionFilter && !aSelectionFilter->graphics )
            break;

        if( checkVisibility( aItem ) )
        {
            PCB_REFERENCE_IMAGE*   image = static_cast<PCB_REFERENCE_IMAGE*>( aItem );
            const REFERENCE_IMAGE& refImg = image->GetReferenceImage();
            const BOX2I            bbox = refImg.GetBoundingBox();

            addRectPoints( bbox, *image );

            if( refImg.GetTransformOriginOffset() != VECTOR2I( 0, 0 ) )
            {
                addAnchor( image->GetPosition() + refImg.GetTransformOriginOffset(), ORIGIN,
                           image, POINT_TYPE::PT_CENTER );
            }
        }

        break;

    default:
        break;
   }
}


PCB_GRID_HELPER::ANCHOR* PCB_GRID_HELPER::nearestAnchor( const VECTOR2I& aPos, int aFlags )
{
    // Do this all in squared distances as we only care about relative distances
    using ecoord = VECTOR2I::extended_type;

    ecoord               minDist = std::numeric_limits<ecoord>::max();
    std::vector<ANCHOR*> anchorsAtMinDistance;

    for( ANCHOR& anchor : m_anchors )
    {
        // There is no need to filter by layers here, as the items are already filtered
        // by layer (if needed) when the anchors are computed.
        if( ( aFlags & anchor.flags ) != aFlags )
            continue;

        if( !anchorsAtMinDistance.empty() && anchor.pos == anchorsAtMinDistance.front()->pos )
        {
            // Same distance as the previous best anchor
            anchorsAtMinDistance.push_back( &anchor );
        }
        else
        {
            const double dist = anchor.pos.SquaredDistance( aPos );

            if( dist < minDist )
            {
                // New minimum distance
                minDist = dist;
                anchorsAtMinDistance.clear();
                anchorsAtMinDistance.push_back( &anchor );
            }
        }
    }

    // Check that any involved real items are 'active'
    // (i.e. the user has moused over a key point previously)
    // If any are not real (e.g. snap lines), they are allowed to be involved
    //
    // This is an area most likely to be controversial/need tuning,
    // as some users will think it's fiddly; without 'activation', others will
    // think the snaps are intrusive.
    SNAP_MANAGER& snapManager = getSnapManager();

    auto noRealItemsInAnchorAreInvolved =
            [&]( ANCHOR* aAnchor ) -> bool
            {
                // If no extension snaps are enabled, don't inhibit
                static const bool haveExtensions = ADVANCED_CFG::GetCfg().m_EnableExtensionSnaps;

                if( !haveExtensions )
                    return false;

                // If the anchor is not constructed, it may be involved (because it is one
                // of the nearest anchors). The items will only be activated later, but don't
                // discard the anchor yet.
                const bool anchorIsConstructed = aAnchor->flags & ANCHOR_FLAGS::CONSTRUCTED;

                if( !anchorIsConstructed )
                    return false;

                bool allRealAreInvolved = snapManager.GetConstructionManager().InvolvesAllGivenRealItems( aAnchor->items );
                return !allRealAreInvolved;
            };

    // Trim out items that aren't involved
    std::erase_if( anchorsAtMinDistance, noRealItemsInAnchorAreInvolved );

    // More than one anchor can be at the same distance, for example
    // two lines end-to-end each have the same endpoint anchor.
    // So, check which one has an involved item that's closest to the origin,
    // and use that one (which allows the user to choose which items
    // gets extended - it's the one nearest the cursor)
    ecoord  minDistToItem = std::numeric_limits<ecoord>::max();
    ANCHOR* best = nullptr;

    // One of the anchors at the minimum distance
    for( ANCHOR* const anchor : anchorsAtMinDistance )
    {
        ecoord distToNearestItem = std::numeric_limits<ecoord>::max();

        for( EDA_ITEM* const item : anchor->items )
        {
            if( !item || !item->IsBOARD_ITEM() )
                continue;

            std::optional<ecoord> distToThisItem =
                    FindSquareDistanceToItem( static_cast<const BOARD_ITEM&>( *item ), aPos );

            if( distToThisItem )
                distToNearestItem = std::min( distToNearestItem, *distToThisItem );
        }

        // If the item doesn't have any special min-dist handler,
        // just use the distance to the anchor
        distToNearestItem = std::min( distToNearestItem, minDist );

        if( distToNearestItem < minDistToItem )
        {
            minDistToItem = distToNearestItem;
            best = anchor;
        }
    }

    return best;
}
