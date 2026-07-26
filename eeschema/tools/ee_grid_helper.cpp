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

#include <functional>
#include <cmath>
#include <advanced_config.h>
#include <macros.h>
#include <gal/graphics_abstraction_layer.h>
#include <eeschema_settings.h>
#include <sch_base_frame.h>
#include <sch_pin.h>
#include <sch_symbol.h>
#include <symbol_editor/symbol_editor_settings.h>
#include <sch_group.h>
#include <sch_item.h>
#include <sch_line.h>
#include <sch_shape.h>
#include <sch_table.h>
#include <sch_tablecell.h>
#include <sch_painter.h>
#include <settings/snap_settings.h>
#include <snap/snap_inference.h>
#include <tool/snap_frame.h>
#include <tool/tool_manager.h>
#include <sch_tool_base.h>
#include <settings/app_settings.h>
#include <trigo.h>
#include <view/view.h>
#include "ee_grid_helper.h"


namespace
{
std::optional<INTERSECTABLE_GEOM> getSchematicIntersectable( const SCH_ITEM& aItem )
{
    if( aItem.Type() == SCH_LINE_T )
    {
        const SCH_LINE& line = static_cast<const SCH_LINE&>( aItem );
        return SEG( line.GetStartPoint(), line.GetEndPoint() );
    }

    if( aItem.Type() != SCH_SHAPE_T )
        return std::nullopt;

    const SCH_SHAPE& shape = static_cast<const SCH_SHAPE&>( aItem );

    switch( shape.GetShape() )
    {
    case SHAPE_T::SEGMENT: return SEG( shape.GetStart(), shape.GetEnd() );
    case SHAPE_T::CIRCLE: return CIRCLE( shape.GetCenter(), shape.GetRadius() );
    case SHAPE_T::ARC: return SHAPE_ARC( shape.GetStart(), shape.GetArcMid(), shape.GetEnd(), 0 );
    case SHAPE_T::RECTANGLE: return BOX2I::ByCorners( shape.GetStart(), shape.GetEnd() );
    default: return std::nullopt;
    }
}
} // namespace


EE_GRID_HELPER::EE_GRID_HELPER() :
        GRID_HELPER()
{
}


EE_GRID_HELPER::EE_GRID_HELPER( TOOL_MANAGER* aToolMgr ) :
        GRID_HELPER( aToolMgr, LAYER_SCHEMATIC_ANCHOR )
{
    if( !m_toolMgr )
        return;

    KIGFX::VIEW* view = m_toolMgr->GetView();

    m_viewAxis.SetSize( 20000 );
    m_viewAxis.SetStyle( KIGFX::ORIGIN_VIEWITEM::CROSS );
    m_viewAxis.SetColor( COLOR4D( 0.0, 0.1, 0.4, 0.8 ) );
    m_viewAxis.SetDrawAtZero( true );
    view->Add( &m_viewAxis );
    view->SetVisible( &m_viewAxis, false );

    m_viewSnapPoint.SetStyle( KIGFX::ORIGIN_VIEWITEM::CIRCLE_CROSS );
    m_viewSnapPoint.SetColor( COLOR4D( 0.0, 0.1, 0.4, 1.0 ) );
    m_viewSnapPoint.SetDrawAtZero( true );
    view->Add( &m_viewSnapPoint );
    view->SetVisible( &m_viewSnapPoint, false );
}


EE_GRID_HELPER::~EE_GRID_HELPER()
{
    if( !m_toolMgr )
        return;

    KIGFX::VIEW* view = m_toolMgr->GetView();

    view->Remove( &m_viewAxis );
    view->Remove( &m_viewSnapPoint );
}


void EE_GRID_HELPER::AddConstructionItems( std::vector<SCH_ITEM*> aItems, bool aExtensionOnly, bool aIsPersistent )
{
    if( !ADVANCED_CFG::GetCfg().m_EnableExtensionSnaps )
        return;

    if( !snapInferenceSettings().constructionExtensions )
        return;

    auto batch = std::make_unique<CONSTRUCTION_MANAGER::CONSTRUCTION_ITEM_BATCH>();

    for( SCH_ITEM* item : aItems )
    {
        std::vector<KIGFX::CONSTRUCTION_GEOM::DRAWABLE> drawables;
        std::optional<SEG>                              segment;

        if( item->Type() == SCH_LINE_T )
        {
            const SCH_LINE& line = static_cast<const SCH_LINE&>( *item );
            segment = SEG( line.GetStartPoint(), line.GetEndPoint() );
        }
        else if( item->Type() == SCH_SHAPE_T )
        {
            const SCH_SHAPE& shape = static_cast<const SCH_SHAPE&>( *item );

            if( shape.GetShape() == SHAPE_T::SEGMENT )
            {
                segment = SEG( shape.GetStart(), shape.GetEnd() );
            }
            else if( shape.GetShape() == SHAPE_T::ARC )
            {
                drawables.emplace_back( CIRCLE( shape.GetCenter(), shape.GetRadius() ) );
                drawables.emplace_back( shape.GetCenter() );
            }
            else if( shape.GetShape() == SHAPE_T::CIRCLE )
            {
                drawables.emplace_back( shape.GetCenter() );
            }
        }

        if( segment )
        {
            if( aExtensionOnly )
            {
                VECTOR2I direction = segment->B - segment->A;
                drawables.emplace_back( HALF_LINE( segment->A, segment->A - direction ) );
                drawables.emplace_back( HALF_LINE( segment->B, segment->B + direction ) );
            }
            else
            {
                drawables.emplace_back( LINE( *segment ) );
            }

            if( aIsPersistent )
            {
                drawables.emplace_back( segment->A );
                drawables.emplace_back( segment->B );
            }
        }

        std::vector<CONSTRUCTION_MANAGER::CONSTRUCTION_ITEM::DRAWABLE_ENTRY> entries;

        for( KIGFX::CONSTRUCTION_GEOM::DRAWABLE& drawable : drawables )
            entries.push_back( { std::move( drawable ), 1 } );

        batch->push_back( { CONSTRUCTION_MANAGER::SOURCE::FROM_ITEMS, item, std::move( entries ) } );
    }

    getSnapManager().GetConstructionManager().ProposeConstructionItems( std::move( batch ), aIsPersistent );
}


SNAP_INFERENCE_SETTINGS EE_GRID_HELPER::snapInferenceSettings() const
{
    SNAP_INFERENCE_SETTINGS settings;

    if( !m_toolMgr )
        return settings;

    if( SCH_BASE_FRAME* frame = dynamic_cast<SCH_BASE_FRAME*>( m_toolMgr->GetToolHolder() ) )
    {
        // eeconfig() and libeditconfig() are unrelated types, so the editor has to be resolved
        // first; a plain dynamic_cast silently yields defaults in the symbol editor.
        if( frame->IsType( FRAME_SCH_SYMBOL_EDITOR ) )
        {
            if( SYMBOL_EDITOR_SETTINGS* cfg = frame->libeditconfig() )
                settings = cfg->m_SnapInference;
        }
        else if( EESCHEMA_SETTINGS* cfg = frame->eeconfig() )
        {
            settings = cfg->m_SnapInference;
        }
    }

    return settings;
}


BOX2I EE_GRID_HELPER::layoutBounds( const SCH_ITEM& aItem )
{
    if( aItem.Type() == SCH_SYMBOL_T )
        return static_cast<const SCH_SYMBOL&>( aItem ).GetBodyAndPinsBoundingBox();

    return aItem.GetBoundingBox();
}


std::vector<std::pair<const SCH_PIN*, VECTOR2I>> EE_GRID_HELPER::layoutPins( SCH_ITEM& aItem )
{
    std::vector<std::pair<const SCH_PIN*, VECTOR2I>> pins;

    if( aItem.Type() == SCH_PIN_T )
    {
        SCH_PIN& pin = static_cast<SCH_PIN&>( aItem );
        pins.emplace_back( &pin, pin.GetPosition() );
    }
    else if( aItem.Type() == SCH_SYMBOL_T )
    {
        for( SCH_PIN* pin : static_cast<SCH_SYMBOL&>( aItem ).GetPins() )
            pins.emplace_back( pin, pin->GetPosition() );
    }

    return pins;
}


VECTOR2I EE_GRID_HELPER::BestDragOrigin( const VECTOR2I& aMousePos, GRID_HELPER_GRIDS aGrid,
                                         const SCH_SELECTION& aItems )
{
    clearAnchors();

    // If we're working with any connectable objects, skip non-connectable objects
    // since they are often off-grid, e.g. text anchors
    bool hasConnectables = false;

    for( EDA_ITEM* item : aItems )
    {
        GRID_HELPER_GRIDS grid = GetItemGrid( static_cast<SCH_ITEM*>( item ) );
        if( grid == GRID_CONNECTABLE || grid == GRID_WIRES )
        {
            hasConnectables = true;
            break;
        }
    }

    for( EDA_ITEM* item : aItems )
        computeAnchors( static_cast<SCH_ITEM*>( item ), aMousePos, true, !hasConnectables );

    double worldScale = m_toolMgr->GetView()->GetGAL()->GetWorldScale();
    double lineSnapMinCornerDistance = 50.0 / worldScale;

    ANCHOR* nearestOutline = nearestAnchor( aMousePos, OUTLINE, aGrid );
    ANCHOR* nearestCorner = nearestAnchor( aMousePos, CORNER, aGrid );
    ANCHOR* nearestOrigin = nearestAnchor( aMousePos, ORIGIN, aGrid );
    ANCHOR* best = nullptr;
    double minDist = std::numeric_limits<double>::max();

    if( nearestOrigin )
    {
        minDist = nearestOrigin->Distance( aMousePos );
        best = nearestOrigin;
    }

    if( nearestCorner )
    {
        double dist = nearestCorner->Distance( aMousePos );

        if( dist < minDist )
        {
            minDist = dist;
            best = nearestCorner;
        }
    }

    if( nearestOutline )
    {
        double dist = nearestOutline->Distance( aMousePos );

        if( minDist > lineSnapMinCornerDistance && dist < minDist )
            best = nearestOutline;
    }

    VECTOR2I ret = best ? best->pos : aMousePos;

    if( best )
    {
        std::optional<BOX2I> movingBounds;
        bool                 pinEnd = false;

        // A symbol's pin anchors are tagged with the symbol, not the pin, so the grab is
        // classified against pin geometry rather than against the anchor's item.
        for( EDA_ITEM* item : aItems )
        {
            SCH_ITEM& schematicItem = *static_cast<SCH_ITEM*>( item );
            BOX2I     bounds = layoutBounds( schematicItem );

            if( movingBounds )
                movingBounds->Merge( bounds );
            else
                movingBounds = bounds;

            if( !pinEnd )
            {
                std::vector<std::pair<const SCH_PIN*, VECTOR2I>> pins = layoutPins( schematicItem );

                pinEnd = std::any_of( pins.begin(), pins.end(),
                                      [&]( const std::pair<const SCH_PIN*, VECTOR2I>& aPin )
                                      {
                                          return aPin.second == ret;
                                      } );
            }
        }

        setLayoutReference( ret, movingBounds, pinEnd );
    }
    else
    {
        setLayoutReference( ret, std::nullopt, false );
    }

    return ret;
}


SNAP_RESULT EE_GRID_HELPER::ResolveSnap( const VECTOR2I& aOrigin, GRID_HELPER_GRIDS aGrid, SCH_ITEM* aSkip )
{
    SCH_SELECTION skipItems;

    // No skip item means nothing is moving, which selects the picker snap profile
    if( aSkip )
        skipItems.Add( aSkip );

    return ResolveSnap( aOrigin, aGrid, skipItems );
}


SNAP_RESULT EE_GRID_HELPER::ResolveSnap( const VECTOR2I& aOrigin, GRID_HELPER_GRIDS aGrid, const SCH_SELECTION& aSkip,
                                         std::optional<VECTOR2I> aMovingReferencePoint )
{
    const SNAP_RANGES ranges = computeSnapRanges( canUseGrid() );
    const double      snapScale = ranges.scale;
    const int         snapRange = ranges.range;
    const int         snapIn = ranges.in;
    const int         snapOut = ranges.out;

    const SNAP_INFERENCE_SETTINGS inferenceSettings = snapInferenceSettings();

    const bool constructionEnabled =
            m_enableSnap && inferenceSettings.constructionExtensions && ADVANCED_CFG::GetCfg().m_EnableExtensionSnaps;

    if( !constructionEnabled )
        getSnapManager().GetConstructionManager().Clear();

    BOX2I bb( VECTOR2I( aOrigin.x - snapRange / 2, aOrigin.y - snapRange / 2 ), VECTOR2I( snapRange, snapRange ) );

    std::optional<ANCHOR> retained = m_snapItem;
    clearAnchors();

    const std::set<SCH_ITEM*> visibleItems = queryVisible( bb, aSkip );

    for( SCH_ITEM* item : visibleItems )
        computeAnchors( item, aOrigin );

    ANCHOR*  nearest = nearestAnchor( aOrigin, SNAPPABLE, aGrid );
    VECTOR2I nearestGrid = Align( aOrigin, aGrid );

    if( KIGFX::ANCHOR_DEBUG* ad = enableAndGetAnchorDebug(); ad )
    {
        ad->ClearAnchors();
        for( const ANCHOR& a : m_anchors )
            ad->AddAnchor( a.pos );

        ad->SetNearest( nearest ? OPT_VECTOR2I( nearest->pos ) : std::nullopt );
        m_toolMgr->GetView()->Update( ad, KIGFX::GEOMETRY );
    }

    showConstructionGeometry( m_enableSnap );

    SNAP_LINE_MANAGER& snapLineManager = getSnapManager().GetSnapLineManager();
    const VECTOR2D     gridSize = GetGridSize( aGrid );

    std::optional<VECTOR2I> guideSnap;

    if( m_enableSnapLine )
        guideSnap = SnapToConstructionLines( aOrigin, nearestGrid, gridSize, snapRange );

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

        if( targets.empty() )
            return MakePointSnapId( kind, aAnchor.pos, aAnchor.pointTypes );

        if( targets.size() == 1 )
            return SNAP_STABLE_ID{ kind, targets.front(), aAnchor.pointTypes, 0 };

        return MakeCompositeSnapId( kind, targets, aAnchor.pointTypes );
    };

    SNAP_SOURCE_CONTEXT context;
    context.profile = aSkip.Empty() ? SNAP_EDITOR_PROFILE::PICKER : SNAP_EDITOR_PROFILE::RIGID_PLACEMENT;
    context.sourcePoint = aOrigin;
    context.movingReferencePoint = aMovingReferencePoint;
    context.referencePreference = m_layoutReferencePreference;

    for( EDA_ITEM* item : aSkip )
    {
        BOX2I bounds = layoutBounds( *static_cast<SCH_ITEM*>( item ) );

        if( context.movingBounds )
            context.movingBounds->Merge( bounds );
        else
            context.movingBounds = bounds;
    }

    if( aSkip.GetSize() == 1 )
    {
        SCH_ITEM* sourceItem = static_cast<SCH_ITEM*>( *aSkip.begin() );
        context.movingItem = SNAP_STABLE_ID{ SNAP_ID_KIND::ITEM_GEOMETRY, SnapTargetId( sourceItem->m_Uuid ) };

        if( m_pointEditProfile )
            context.profile = SNAP_EDITOR_PROFILE::POINT_EDIT;

        if( sourceItem->Type() == SCH_LINE_T && m_pointEditProfile )
        {
            SCH_LINE* line = static_cast<SCH_LINE*>( sourceItem );
            context.stationarySourceLeg =
                    line->GetStartPoint().SquaredDistance( aOrigin ) > line->GetEndPoint().SquaredDistance( aOrigin )
                            ? line->GetStartPoint()
                            : line->GetEndPoint();
        }
    }

    enum class PRESENTATION_KIND
    {
        ANCHOR_MARKER,
        GUIDE
    };

    struct PRESENTATION
    {
        PRESENTATION_KIND     kind;
        std::optional<ANCHOR> anchor;
        bool                  proposeConstruction;
    };

    SNAP_FRAME_INPUT<PRESENTATION> frame;
    frame.context = context;
    frame.stickyIds = m_stickySnapIds;
    frame.rankingHysteresis = ranges.rankingHysteresis;
    frame.feasibility = m_feasibilityCallback;
    frame.trace = snapTraceCallback( context );

    SNAP_INFERENCE_PROVIDER inferenceProvider;
    SNAP_INFERENCE_PROVIDER tangentProvider;
    const bool              inferenceEnabled = m_enableSnap
                                  && ( inferenceSettings.objectGeometry || inferenceSettings.tangentNormal
                                       || inferenceSettings.alignmentDistribution );

    // CollectTangentNormal only reads arcs and circles, and only when dragging away from a fixed leg
    const bool tangentEnabled = inferenceSettings.tangentNormal && context.stationarySourceLeg.has_value();

    if( inferenceEnabled && context.movingItem )
    {
        for( size_t i = 0; i < m_stationarySelfSegments.size(); ++i )
        {
            SNAP_STABLE_ID id =
                    MakeDerivedSnapId( SNAP_ID_KIND::SELF_SEGMENT, *context.movingItem, static_cast<int>( i ) );
            context.stationarySelfFeatures.push_back( id );

            if( inferenceSettings.objectGeometry )
                inferenceProvider.AddPath( { id, m_stationarySelfSegments[i], false } );
        }
    }

    emitAngleBranchCandidates( frame.candidates, aOrigin, snapScale );

    if( inferenceEnabled )
    {
        for( SCH_ITEM* item : visibleItems )
        {
            if( inferenceSettings.objectGeometry || tangentEnabled )
            {
                std::optional<INTERSECTABLE_GEOM> geometry = getSchematicIntersectable( *item );

                if( geometry )
                {
                    SNAP_STABLE_ID id{ SNAP_ID_KIND::ITEM_GEOMETRY, SnapTargetId( item->m_Uuid ),
                                       static_cast<int>( item->Type() ), 0 };
                    const bool     curved = std::holds_alternative<CIRCLE>( *geometry )
                                        || std::holds_alternative<SHAPE_ARC>( *geometry );

                    if( inferenceSettings.objectGeometry )
                        inferenceProvider.AddPath( { id, *geometry, false } );

                    if( tangentEnabled && curved )
                        tangentProvider.AddPath( { std::move( id ), std::move( *geometry ), false } );
                }
            }

            if( inferenceSettings.alignmentDistribution )
            {
                inferenceProvider.AddBounds( { { SNAP_ID_KIND::ITEM_GEOMETRY, SnapTargetId( item->m_Uuid ),
                                                 static_cast<int>( item->Type() ), 0 },
                                               layoutBounds( *item ),
                                               std::nullopt } );

                for( const auto& [pin, position] : layoutPins( *item ) )
                {
                    inferenceProvider.AddAlignmentPoint(
                            { { SNAP_ID_KIND::INTRINSIC_ANCHOR, SnapTargetId( pin->m_Uuid ) },
                              position,
                              SnapTargetId( item->m_Uuid ) } );
                }
            }
        }
    }

    int constructionIndex = 0;

    if( constructionEnabled )
    {
        for( const CONSTRUCTION_MANAGER::CONSTRUCTION_ITEM_BATCH& batch : getSnapManager().GetConstructionItems() )
        {
            for( const CONSTRUCTION_MANAGER::CONSTRUCTION_ITEM& item : batch )
            {
                SNAP_TARGET_ID target = item.Item ? SnapTargetId( item.Item->m_Uuid ) : SNAP_TARGET_ID{};

                for( const CONSTRUCTION_MANAGER::CONSTRUCTION_ITEM::DRAWABLE_ENTRY& entry : item.Constructions )
                {
                    SNAP_STABLE_ID id{ SNAP_ID_KIND::CONSTRUCTION, target, constructionIndex++, 0 };

                    std::visit(
                            [&]( const auto& drawable )
                            {
                                using DRAWABLE_TYPE = std::decay_t<decltype( drawable )>;

                                if constexpr( std::is_same_v<DRAWABLE_TYPE, VECTOR2I> )
                                {
                                    frame.candidates.push_back( SNAP_CANDIDATE::Point(
                                            id, SNAP_PRIORITY_TIER::OBJECT, SNAP_CANDIDATE_SUBTYPE::CONSTRUCTED_POINT,
                                            drawable, drawable.Distance( aOrigin ) / snapScale ) );
                                }
                                else
                                {
                                    inferenceProvider.AddPath( { id, drawable, false, true } );
                                }
                            },
                            entry.Drawable );
                }
            }
        }
    }

    if( m_enableSnap && ( inferenceSettings.objectGeometry || constructionEnabled ) )
    {
        for( SNAP_CANDIDATE& candidate : inferenceProvider.CollectObjectGeometry( context, snapRange ) )
            frame.candidates.push_back( std::move( candidate ) );
    }

    if( m_enableSnap && inferenceSettings.tangentNormal && context.stationarySourceLeg )
    {
        for( SNAP_CANDIDATE& candidate : tangentProvider.CollectTangentNormal( context, snapRange, true, true ) )
        {
            frame.candidates.push_back( std::move( candidate ) );
        }
    }

    if( m_enableSnap && inferenceSettings.alignmentDistribution && context.movingBounds )
    {
        for( SNAP_CANDIDATE& candidate : inferenceProvider.CollectAlignment( context, snapRange ) )
            frame.candidates.push_back( std::move( candidate ) );

        for( SNAP_CANDIDATE& candidate : inferenceProvider.CollectEqualSpacing( context, snapRange ) )
            frame.candidates.push_back( std::move( candidate ) );
    }

    if( m_enableSnap && retained && retained->Distance( aOrigin ) <= snapOut )
    {
        SNAP_STABLE_ID retainedId = anchorId( *retained );
        frame.retainedId = retainedId;
        frame.candidates.push_back( SNAP_CANDIDATE::Point( retainedId, SNAP_PRIORITY_TIER::OBJECT,
                                                           retained->flags & CONSTRUCTED
                                                                   ? SNAP_CANDIDATE_SUBTYPE::CONSTRUCTED_POINT
                                                                   : SNAP_CANDIDATE_SUBTYPE::INTRINSIC_ANCHOR,
                                                           retained->pos, retained->Distance( aOrigin ) / snapScale ) );
        frame.presentation.emplace( retainedId, PRESENTATION{ PRESENTATION_KIND::ANCHOR_MARKER, *retained, false } );
    }

    if( m_enableSnap && nearest && nearest->Distance( aOrigin ) <= snapIn && m_skipPoint != nearest->pos )
    {
        SNAP_STABLE_ID nearestId = anchorId( *nearest );
        frame.candidates.push_back( SNAP_CANDIDATE::Point( nearestId, SNAP_PRIORITY_TIER::OBJECT,
                                                           nearest->flags & CONSTRUCTED
                                                                   ? SNAP_CANDIDATE_SUBTYPE::CONSTRUCTED_POINT
                                                                   : SNAP_CANDIDATE_SUBTYPE::INTRINSIC_ANCHOR,
                                                           nearest->pos, nearest->Distance( aOrigin ) / snapScale ) );
        frame.presentation.insert_or_assign( nearestId,
                                             PRESENTATION{ PRESENTATION_KIND::ANCHOR_MARKER, *nearest, true } );
    }

    if( guideSnap && m_skipPoint != *guideSnap )
    {
        SNAP_STABLE_ID guideId = MakePointSnapId( SNAP_ID_KIND::CONSTRUCTION, *guideSnap );
        SNAP_CANDIDATE candidate =
                SNAP_CANDIDATE::Point( guideId, SNAP_PRIORITY_TIER::OBJECT, SNAP_CANDIDATE_SUBTYPE::CONSTRUCTED_POINT,
                                       *guideSnap, guideSnap->Distance( aOrigin ) / snapScale );
        frame.candidates.push_back( std::move( candidate ) );
        frame.presentation.emplace( guideId, PRESENTATION{ PRESENTATION_KIND::GUIDE, std::nullopt, false } );
    }

    emitSelfAndGridCandidates( frame.candidates, context, aOrigin, nearestGrid, snapScale, snapRange, canUseGrid() );

    // Object retention wins because its tier already outranks angle restriction.
    if( !frame.retainedId && m_retainedAngleBranch )
        frame.retainedId = m_retainedAngleBranch;

    SNAP_FRAME_OUTPUT<PRESENTATION> output = ResolveSnapFrame( std::move( frame ) );
    SNAP_RESULT&                    result = output.result;
    retainAcceptedSnaps( result );

    bool keepConstructionProposal = false;

    if( output.presentation && output.presentation->payload.kind == PRESENTATION_KIND::GUIDE )
    {
        snapLineManager.SetSnapLineEnd( result.position );
        m_toolMgr->GetView()->SetVisible( &m_viewSnapPoint, false );
        m_snapItem = std::nullopt;
    }
    else if( output.presentation && output.presentation->payload.anchor )
    {
        const PRESENTATION&    presentation = output.presentation->payload;
        std::vector<SCH_ITEM*> items;

        for( EDA_ITEM* item : presentation.anchor->items )
        {
            if( item && item->IsSCH_ITEM() )
                items.push_back( static_cast<SCH_ITEM*>( item ) );
        }

        if( presentation.proposeConstruction && constructionEnabled && !items.empty() )
        {
            AddConstructionItems( std::move( items ), true, false );
            keepConstructionProposal = true;
        }

        m_snapItem = *presentation.anchor;
        m_viewSnapPoint.SetPosition( result.position );
        m_viewSnapPoint.SetSnapTypes( m_snapItem->pointTypes );
        snapLineManager.SetSnapLineOrigin( result.position );
        snapLineManager.SetSnapLineEnd( std::nullopt );

        if( m_toolMgr->GetView()->IsVisible( &m_viewSnapPoint ) )
            m_toolMgr->GetView()->Update( &m_viewSnapPoint, KIGFX::GEOMETRY );
        else
            m_toolMgr->GetView()->SetVisible( &m_viewSnapPoint, true );
    }
    else
    {
        m_snapItem = std::nullopt;
        snapLineManager.SetSnapLineEnd( std::nullopt );
        m_toolMgr->GetView()->SetVisible( &m_viewSnapPoint, false );
    }

    applySnapResultGuides( result );

    if( constructionEnabled && !keepConstructionProposal && ADVANCED_CFG::GetCfg().m_ExtensionSnapActivateOnHover )
    {
        for( SCH_ITEM* item : visibleItems )
        {
            if( item->HitTest( aOrigin, 0 ) )
            {
                AddConstructionItems( { item }, true, false );
                keepConstructionProposal = true;
                break;
            }
        }
    }

    if( !keepConstructionProposal )
        getSnapManager().GetConstructionManager().CancelProposal();

    return result;
}


VECTOR2D EE_GRID_HELPER::GetGridSize( GRID_HELPER_GRIDS aGrid ) const
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
        g = grid.grids[idx].ToDouble( schIUScale );

    return g;
}


SCH_ITEM* EE_GRID_HELPER::GetSnapped() const
{
    if( !m_snapItem )
        return nullptr;

    if( m_snapItem->items.empty() )
        return nullptr;

    return static_cast<SCH_ITEM*>( m_snapItem->items[0] );
}


std::set<SCH_ITEM*> EE_GRID_HELPER::queryVisible( const BOX2I& aArea,
                                                  const SCH_SELECTION& aSkipList ) const
{
    std::set<SCH_ITEM*>                       items;
    std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> selectedItems;

    EDA_DRAW_FRAME* frame = dynamic_cast<EDA_DRAW_FRAME*>( m_toolMgr->GetToolHolder() );
    KIGFX::VIEW*    view = m_toolMgr->GetView();

    view->Query( aArea, selectedItems );

    for( const KIGFX::VIEW::LAYER_ITEM_PAIR& it : selectedItems )
    {
        if( !it.first->IsSCH_ITEM() )
            continue;

        SCH_ITEM* item = static_cast<SCH_ITEM*>( it.first );

        if( frame && frame->IsType( FRAME_SCH_SYMBOL_EDITOR ) )
        {
            // If we are in the symbol editor, don't use the symbol itself
            if( item->Type() == LIB_SYMBOL_T )
                continue;
        }
        else
        {
            // If we are not in the symbol editor, don't use symbol-editor-private items
            if( item->IsPrivate() )
                continue;
        }

        // The item must be visible and on an active layer
        if( view->IsVisible( item ) && item->ViewGetLOD( it.second, view ) < view->GetScale() )
            items.insert( item );
    }

    for( EDA_ITEM* skipItem : aSkipList )
        items.erase( static_cast<SCH_ITEM*>( skipItem ) );

    return items;
}


GRID_HELPER_GRIDS EE_GRID_HELPER::GetItemGrid( const EDA_ITEM* aItem ) const
{
    if( !aItem )
        return GRID_CURRENT;

    switch( aItem->Type() )
    {
    case LIB_SYMBOL_T:
    case SCH_SYMBOL_T:
    case SCH_PIN_T:
    case SCH_SHEET_PIN_T:
    case SCH_SHEET_T:
    case SCH_NO_CONNECT_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIER_LABEL_T:
    case SCH_LABEL_T:
    case SCH_DIRECTIVE_LABEL_T:
    case SCH_RULE_AREA_T:
        return GRID_CONNECTABLE;

    case SCH_FIELD_T:
    case SCH_TEXT_T:
        return GRID_TEXT;

    case SCH_SHAPE_T:
    // The text box's border lines are what need to be on the graphic grid
    case SCH_TEXTBOX_T:
    case SCH_BITMAP_T:
        return GRID_GRAPHICS;

    case SCH_JUNCTION_T:
        return GRID_WIRES;

    case SCH_LINE_T:
        if( static_cast<const SCH_LINE*>( aItem )->IsConnectable() )
            return GRID_WIRES;
        else
            return GRID_GRAPHICS;

    case SCH_BUS_BUS_ENTRY_T:
    case SCH_BUS_WIRE_ENTRY_T:
        return GRID_WIRES;

    // Groups need to get the grid of their children
    case SCH_GROUP_T:
    {
        const SCH_GROUP* group = static_cast<const SCH_GROUP*>( aItem );

        // Shouldn't happen
        if( group->GetItems().empty() )
            return GRID_CURRENT;

        GRID_HELPER_GRIDS grid = GetItemGrid( *group->GetItems().begin() );

        for( EDA_ITEM* item : static_cast<const SCH_GROUP*>( aItem )->GetItems() )
        {
            GRID_HELPER_GRIDS itemGrid = GetItemGrid( item );

            if( GetGridSize( itemGrid ) > GetGridSize( grid ) )
                grid = itemGrid;
        }

        return grid;
    }

    default: return GRID_CURRENT;
    }
}


void EE_GRID_HELPER::computeAnchors( SCH_ITEM* aItem, const VECTOR2I& aRefPos, bool aFrom, bool aIncludeText )
{
    bool isGraphicLine = aItem->Type() == SCH_LINE_T && static_cast<SCH_LINE*>( aItem )->IsGraphicLine();

    switch( aItem->Type() )
    {
    case SCH_TEXT_T:
    case SCH_FIELD_T:
    {
        if( aIncludeText )
            addAnchor( aItem->GetPosition(), ORIGIN, aItem );

        break;
    }

    case SCH_TABLE_T:
    {
        if( aIncludeText )
        {
            addAnchor( aItem->GetPosition(), SNAPPABLE | CORNER, aItem );
            addAnchor( static_cast<SCH_TABLE*>( aItem )->GetEnd(), SNAPPABLE | CORNER, aItem );
        }

        break;
    }

    case SCH_TEXTBOX_T:
    case SCH_TABLECELL_T:
    {
        if( aIncludeText )
        {
            addAnchor( aItem->GetPosition(), SNAPPABLE | CORNER, aItem );
            addAnchor( static_cast<SCH_SHAPE*>( aItem )->GetEnd(), SNAPPABLE | CORNER, aItem );
        }

        break;
    }

    case SCH_SYMBOL_T:
    case SCH_SHEET_T:
        addAnchor( aItem->GetPosition(), ORIGIN, aItem );
        KI_FALLTHROUGH;

    case SCH_JUNCTION_T:
    case SCH_NO_CONNECT_T:
    case SCH_LINE_T:
        // Don't add anchors for graphic lines unless we're including text,
        // they may be on a non-connectable grid
        if( isGraphicLine && !aIncludeText )
            break;

        KI_FALLTHROUGH;
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIER_LABEL_T:
    case SCH_LABEL_T:
    case SCH_DIRECTIVE_LABEL_T:
    case SCH_BUS_WIRE_ENTRY_T:
    case SCH_SHEET_PIN_T:
    {
        std::vector<VECTOR2I> pts = aItem->GetConnectionPoints();

        for( const VECTOR2I& pt : pts )
            addAnchor( VECTOR2I( pt ), SNAPPABLE | CORNER, aItem );

        break;
    }
    case SCH_PIN_T:
    {
        SCH_PIN* pin = static_cast<SCH_PIN*>( aItem );
        addAnchor( pin->GetPosition(), SNAPPABLE | ORIGIN, aItem );
        break;
    }

    case SCH_GROUP_T:
        for( EDA_ITEM* item : static_cast<SCH_GROUP*>( aItem )->GetItems() )
        {
            computeAnchors( static_cast<SCH_ITEM*>( item ), aRefPos, aFrom, aIncludeText );
        }

        break;

    default:
        break;
    }

    // Don't add anchors for graphic lines unless we're including text,
    // they may be on a non-connectable grid
    if( aItem->Type() == SCH_LINE_T && ( aIncludeText || !isGraphicLine ) )
    {
        SCH_LINE* line = static_cast<SCH_LINE*>( aItem );
        VECTOR2I  pt = Align( aRefPos );

        if( line->GetStartPoint().x == line->GetEndPoint().x )
        {
            VECTOR2I possible( line->GetStartPoint().x, pt.y );

            if( TestSegmentHit( possible, line->GetStartPoint(), line->GetEndPoint(), 0 ) )
                addAnchor( possible, SNAPPABLE | VERTICAL, aItem );
        }
        else if( line->GetStartPoint().y == line->GetEndPoint().y )
        {
            VECTOR2I possible( pt.x, line->GetStartPoint().y );

            if( TestSegmentHit( possible, line->GetStartPoint(), line->GetEndPoint(), 0 ) )
                addAnchor( possible, SNAPPABLE | HORIZONTAL, aItem );
        }
    }
}


EE_GRID_HELPER::ANCHOR* EE_GRID_HELPER::nearestAnchor( const VECTOR2I& aPos, int aFlags,
                                                       GRID_HELPER_GRIDS aGrid )
{
    double  minDist = std::numeric_limits<double>::max();
    ANCHOR* best = nullptr;

    for( ANCHOR& a : m_anchors )
    {
        if( ( aFlags & a.flags ) != aFlags )
            continue;

        // A "virtual" anchor with no real items associated shouldn't be filtered out
        if( !a.items.empty() )
        {
            // Filter using the first item
            SCH_ITEM* item = static_cast<SCH_ITEM*>( a.items[0] );

            if( aGrid == GRID_CONNECTABLE && !item->IsConnectable() )
                continue;
            else if( aGrid == GRID_GRAPHICS && item->IsConnectable() )
                continue;
        }

        double dist = a.Distance( aPos );

        if( dist < minDist )
        {
            minDist = dist;
            best = &a;
        }
    }

    return best;
}
