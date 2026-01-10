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

#include <algorithm>
#include <cstdio>
#include <memory>
#include <mutex>
#include <wx/log.h>
#include <board.h>
#include <board_design_settings.h>
#include <component_classes/component_class.h>
#include <drc/drc_rtree.h>
#include <drc/drc_engine.h>
#include <lset.h>
#include <pcb_track.h>
#include <pcb_group.h>
#include <geometry/shape_segment.h>
#include <pcbexpr_evaluator.h>
#include <connectivity/connectivity_data.h>
#include <connectivity/connectivity_algo.h>
#include <connectivity/from_to_cache.h>


bool fromToFunc( LIBEVAL::CONTEXT* aCtx, void* self )
{
    PCBEXPR_VAR_REF* vref = static_cast<PCBEXPR_VAR_REF*>( self );
    BOARD_ITEM*      item = vref ? vref->GetObject( aCtx ) : nullptr;
    LIBEVAL::VALUE*  result = aCtx->AllocValue();
    LIBEVAL::VALUE*  argTo = aCtx->Pop();
    LIBEVAL::VALUE*  argFrom = aCtx->Pop();

    result->Set(0.0);
    aCtx->Push( result );

    if(!item)
        return false;

    auto ftCache = item->GetBoard()->GetConnectivity()->GetFromToCache();

    if( !ftCache )
    {
        wxLogWarning( wxT( "Attempting to call fromTo() with non-existent from-to cache." ) );
        return true;
    }

    if( ftCache->IsOnFromToPath( static_cast<BOARD_CONNECTED_ITEM*>( item ),
                                 argFrom->AsString(), argTo->AsString() ) )
    {
        result->Set(1.0);
    }

    return true;
}


#define MISSING_LAYER_ARG( f ) wxString::Format( _( "Missing layer name argument to %s." ), f )

static void existsOnLayerFunc( LIBEVAL::CONTEXT* aCtx, void *self )
{
    PCBEXPR_VAR_REF* vref = static_cast<PCBEXPR_VAR_REF*>( self );
    BOARD_ITEM*      item = vref ? vref->GetObject( aCtx ) : nullptr;
    LIBEVAL::VALUE*  arg = aCtx->Pop();
    LIBEVAL::VALUE*  result = aCtx->AllocValue();

    result->Set( 0.0 );
    aCtx->Push( result );

    if( !item )
        return;

    if( !arg || arg->AsString().IsEmpty() )
    {
        if( aCtx->HasErrorCallback() )
            aCtx->ReportError( MISSING_LAYER_ARG( wxT( "existsOnLayer()" ) ) );

        return;
    }

    result->SetDeferredEval(
            [item, arg, aCtx]() -> double
            {
                const wxString& layerName = arg->AsString();
                wxPGChoices& layerMap = ENUM_MAP<PCB_LAYER_ID>::Instance().Choices();

                if( aCtx->HasErrorCallback())
                {
                    /*
                     * Interpreted version
                     */

                    bool anyMatch = false;

                    for( unsigned ii = 0; ii < layerMap.GetCount(); ++ii )
                    {
                        wxPGChoiceEntry& entry = layerMap[ ii ];

                        if( entry.GetText().Matches( layerName ))
                        {
                            anyMatch = true;

                            if( item->IsOnLayer( ToLAYER_ID( entry.GetValue() ) ) )
                                return 1.0;
                        }
                    }

                    if( !anyMatch )
                    {
                        aCtx->ReportError( wxString::Format( _( "Unrecognized layer '%s'" ),
                                                             layerName ) );
                    }

                    return 0.0;
                }
                else
                {
                    /*
                     * Compiled version
                     */

                    BOARD* board = item->GetBoard();

                    {
                        std::shared_lock<std::shared_mutex> readLock( board->m_CachesMutex );

                        auto i = board->m_LayerExpressionCache.find( layerName );

                        if( i != board->m_LayerExpressionCache.end() )
                            return ( item->GetLayerSet() & i->second ).any() ? 1.0 : 0.0;
                    }

                    LSET mask;

                    for( unsigned ii = 0; ii < layerMap.GetCount(); ++ii )
                    {
                        wxPGChoiceEntry& entry = layerMap[ ii ];

                        if( entry.GetText().Matches( layerName ) )
                            mask.set( ToLAYER_ID( entry.GetValue() ) );
                    }

                    {
                        std::unique_lock<std::shared_mutex> writeLock( board->m_CachesMutex );
                        board->m_LayerExpressionCache[ layerName ] = mask;
                    }

                    return ( item->GetLayerSet() & mask ).any() ? 1.0 : 0.0;
                }
            } );
}


static void isPlatedFunc( LIBEVAL::CONTEXT* aCtx, void* self )
{
    LIBEVAL::VALUE* result = aCtx->AllocValue();

    result->Set( 0.0 );
    aCtx->Push( result );

    PCBEXPR_VAR_REF* vref = static_cast<PCBEXPR_VAR_REF*>( self );
    BOARD_ITEM*       item = vref ? vref->GetObject( aCtx ) : nullptr;

    if( !item )
        return;

    if( item->Type() == PCB_PAD_T && static_cast<PAD*>( item )->GetAttribute() == PAD_ATTRIB::PTH )
        result->Set( 1.0 );
    else if( item->Type() == PCB_VIA_T )
        result->Set( 1.0 );
}


bool collidesWithCourtyard( BOARD_ITEM* aItem, std::shared_ptr<SHAPE>& aItemShape,
                            PCBEXPR_CONTEXT* aCtx, FOOTPRINT* aFootprint, PCB_LAYER_ID aSide )
{
    SHAPE_POLY_SET footprintCourtyard;

    footprintCourtyard = aFootprint->GetCourtyard( aSide );

    if( !aItemShape )
    {
        // Since rules are used for zone filling we can't rely on the filled shapes.
        // Use the zone outline instead.
        if( ZONE* zone = dynamic_cast<ZONE*>( aItem ) )
            aItemShape.reset( zone->Outline()->Clone() );
        else
            aItemShape = aItem->GetEffectiveShape( aCtx->GetLayer() );
    }

    return footprintCourtyard.Collide( aItemShape.get() );
};


static bool testFootprintSelector( FOOTPRINT* aFp, const wxString& aSelector )
{
    // NOTE: This code may want to be somewhat more generalized, but for now it's implemented
    // here to support functions like insersectsCourtyard where we want multiple ways to search
    // for the footprints in question.
    // If support for text variable replacement is added, it should happen before any other
    // logic here, so that people can use text variables to contain references or LIBIDs.
    // (see: https://gitlab.com/kicad/code/kicad/-/issues/11231)

    // First check if we have a known directive
    if( aSelector.Upper().StartsWith( wxT( "${CLASS:" ) ) && aSelector.EndsWith( '}' ) )
    {
        wxString name = aSelector.Mid( 8, aSelector.Length() - 9 );

        const COMPONENT_CLASS* compClass = aFp->GetComponentClass();

        if( compClass && compClass->ContainsClassName( name ) )
            return true;
    }
    else if( aFp->GetReference().Matches( aSelector ) )
    {
        return true;
    }
    else if( aSelector.Contains( ':' ) && aFp->GetFPIDAsString().Matches( aSelector ) )
    {
        return true;
    }

    return false;
}


static bool searchFootprints( BOARD* aBoard, const wxString& aArg, PCBEXPR_CONTEXT* aCtx,
                              const std::function<bool( FOOTPRINT* )>& aFunc )
{
    if( aArg == wxT( "A" ) )
    {
        FOOTPRINT* fp = dynamic_cast<FOOTPRINT*>( aCtx->GetItem( 0 ) );

        if( fp && aFunc( fp ) )
            return true;
    }
    else if( aArg == wxT( "B" ) )
    {
        FOOTPRINT* fp = dynamic_cast<FOOTPRINT*>( aCtx->GetItem( 1 ) );

        if( fp && aFunc( fp ) )
            return true;
    }
    else for( FOOTPRINT* fp : aBoard->Footprints() )
    {
        if( testFootprintSelector( fp, aArg ) && aFunc( fp ) )
            return true;
    }

    return false;
}


#define MISSING_FP_ARG( f ) \
    wxString::Format( _( "Missing footprint argument (A, B, or reference designator) to %s." ), f )

static void intersectsCourtyardFunc( LIBEVAL::CONTEXT* aCtx, void* self )
{
    PCBEXPR_CONTEXT* context = static_cast<PCBEXPR_CONTEXT*>( aCtx );
    LIBEVAL::VALUE*  arg = context->Pop();
    LIBEVAL::VALUE*  result = context->AllocValue();

    result->Set( 0.0 );
    context->Push( result );

    if( !arg || arg->AsString().IsEmpty() )
    {
        if( context->HasErrorCallback() )
            context->ReportError( MISSING_FP_ARG( wxT( "intersectsCourtyard()" ) ) );

        return;
    }

    PCBEXPR_VAR_REF* vref = static_cast<PCBEXPR_VAR_REF*>( self );
    BOARD_ITEM*      item = vref ? vref->GetObject( context ) : nullptr;

    if( !item )
        return;

    result->SetDeferredEval(
            [item, arg, context]() -> double
            {
                BOARD*                 board = item->GetBoard();
                std::shared_ptr<SHAPE> itemShape;

                if( searchFootprints( board, arg->AsString(), context,
                        [&]( FOOTPRINT* fp )
                        {
                            PTR_PTR_CACHE_KEY key = { fp, item };

                            if( ( item->GetFlags() & ROUTER_TRANSIENT ) == 0 )
                            {
                                std::shared_lock<std::shared_mutex> readLock( board->m_CachesMutex );

                                auto i = board->m_IntersectsCourtyardCache.find( key );

                                if( i != board->m_IntersectsCourtyardCache.end() )
                                    return i->second;
                            }

                            bool res = collidesWithCourtyard( item, itemShape, context, fp, F_Cu )
                                    || collidesWithCourtyard( item, itemShape, context, fp, B_Cu );

                            if( ( item->GetFlags() & ROUTER_TRANSIENT ) == 0 )
                            {
                                std::unique_lock<std::shared_mutex> cacheLock( board->m_CachesMutex );
                                board->m_IntersectsCourtyardCache[ key ] = res;
                            }

                            return res;
                        } ) )
                {
                    return 1.0;
                }

                return 0.0;
            } );
}


static void intersectsFrontCourtyardFunc( LIBEVAL::CONTEXT* aCtx, void* self )
{
    PCBEXPR_CONTEXT* context = static_cast<PCBEXPR_CONTEXT*>( aCtx );
    LIBEVAL::VALUE*  arg = context->Pop();
    LIBEVAL::VALUE*  result = context->AllocValue();

    result->Set( 0.0 );
    context->Push( result );

    if( !arg || arg->AsString().IsEmpty() )
    {
        if( context->HasErrorCallback() )
            context->ReportError( MISSING_FP_ARG( wxT( "intersectsFrontCourtyard()" ) ) );

        return;
    }

    PCBEXPR_VAR_REF* vref = static_cast<PCBEXPR_VAR_REF*>( self );
    BOARD_ITEM*      item = vref ? vref->GetObject( context ) : nullptr;

    if( !item )
        return;

    result->SetDeferredEval(
            [item, arg, context]() -> double
            {
                BOARD*                 board = item->GetBoard();
                std::shared_ptr<SHAPE> itemShape;

                if( searchFootprints( board, arg->AsString(), context,
                        [&]( FOOTPRINT* fp )
                        {
                            PTR_PTR_CACHE_KEY key = { fp, item };

                            if( ( item->GetFlags() & ROUTER_TRANSIENT ) == 0 )
                            {
                                std::shared_lock<std::shared_mutex> readLock( board->m_CachesMutex );

                                auto i = board->m_IntersectsFCourtyardCache.find( key );

                                if( i != board->m_IntersectsFCourtyardCache.end() )
                                    return i->second;
                            }

                            PCB_LAYER_ID layerId = fp->IsFlipped() ? B_Cu : F_Cu;

                            bool res = collidesWithCourtyard( item, itemShape, context, fp, layerId );

                            if( ( item->GetFlags() & ROUTER_TRANSIENT ) == 0 )
                            {
                                std::unique_lock<std::shared_mutex> writeLock( board->m_CachesMutex );
                                board->m_IntersectsFCourtyardCache[ key ] = res;
                            }

                            return res;
                        } ) )
                {
                    return 1.0;
                }

                return 0.0;
            } );
}


static void intersectsBackCourtyardFunc( LIBEVAL::CONTEXT* aCtx, void* self )
{
    PCBEXPR_CONTEXT* context = static_cast<PCBEXPR_CONTEXT*>( aCtx );
    LIBEVAL::VALUE*  arg = context->Pop();
    LIBEVAL::VALUE*  result = context->AllocValue();

    result->Set( 0.0 );
    context->Push( result );

    if( !arg || arg->AsString().IsEmpty() )
    {
        if( context->HasErrorCallback() )
            context->ReportError( MISSING_FP_ARG( wxT( "intersectsBackCourtyard()" ) ) );

        return;
    }

    PCBEXPR_VAR_REF* vref = static_cast<PCBEXPR_VAR_REF*>( self );
    BOARD_ITEM*      item = vref ? vref->GetObject( context ) : nullptr;

    if( !item )
        return;

    result->SetDeferredEval(
            [item, arg, context]() -> double
            {
                BOARD*                 board = item->GetBoard();
                std::shared_ptr<SHAPE> itemShape;

                if( searchFootprints( board, arg->AsString(), context,
                        [&]( FOOTPRINT* fp )
                        {
                            PTR_PTR_CACHE_KEY key = { fp, item };

                            if( ( item->GetFlags() & ROUTER_TRANSIENT ) == 0 )
                            {
                                std::shared_lock<std::shared_mutex> readLock( board->m_CachesMutex );

                                auto i = board->m_IntersectsBCourtyardCache.find( key );

                                if( i != board->m_IntersectsBCourtyardCache.end() )
                                    return i->second;
                            }

                                PCB_LAYER_ID layerId = fp->IsFlipped() ? F_Cu : B_Cu;

                                bool res = collidesWithCourtyard( item, itemShape, context, fp, layerId );

                            if( ( item->GetFlags() & ROUTER_TRANSIENT ) == 0 )
                            {
                                std::unique_lock<std::shared_mutex> writeLock( board->m_CachesMutex );
                                board->m_IntersectsBCourtyardCache[ key ] = res;
                            }

                            return res;
                        } ) )
                {
                    return 1.0;
                }

                return 0.0;
            } );
}


bool collidesWithArea( BOARD_ITEM* aItem, PCB_LAYER_ID aLayer, PCBEXPR_CONTEXT* aCtx, ZONE* aArea )
{
    BOARD* board = aArea->GetBoard();
    BOX2I  areaBBox = aArea->GetBoundingBox();

    // Collisions include touching, so we need to deflate outline by enough to exclude it.
    // This is particularly important for detecting copper fills as they will be exactly
    // touching along the entire exclusion border.
    SHAPE_POLY_SET areaOutline = aArea->Outline()->CloneDropTriangulation();
    areaOutline.ClearArcs();
    areaOutline.Deflate( board->GetDesignSettings().GetDRCEpsilon(),
                         CORNER_STRATEGY::ALLOW_ACUTE_CORNERS, ARC_LOW_DEF );

    if( aItem->GetFlags() & HOLE_PROXY )
    {
        if( aItem->Type() == PCB_PAD_T )
        {
            return areaOutline.Collide( aItem->GetEffectiveHoleShape().get() );
        }
        else if( aItem->Type() == PCB_VIA_T )
        {
            LSET overlap = aItem->GetLayerSet() & aArea->GetLayerSet();

            /// Avoid buried vias that don't overlap the zone's layers
            if( overlap.any() )
            {
                if( aCtx->GetLayer() == UNDEFINED_LAYER || overlap.Contains( aCtx->GetLayer() ) )
                    return areaOutline.Collide( aItem->GetEffectiveHoleShape().get() );
            }
        }

        return false;
    }

    if( aItem->Type() == PCB_FOOTPRINT_T )
    {
        FOOTPRINT* footprint = static_cast<FOOTPRINT*>( aItem );

        if( ( footprint->GetFlags() & MALFORMED_COURTYARDS ) != 0 )
        {
            if( aCtx->HasErrorCallback() )
                aCtx->ReportError( _( "Footprint's courtyard is not a single, closed shape." ) );

            return false;
        }

        if( ( aArea->GetLayerSet() & LSET::FrontMask() ).any() )
        {
            const SHAPE_POLY_SET& courtyard = footprint->GetCourtyard( F_CrtYd );

            if( courtyard.OutlineCount() == 0 )
            {
                if( aCtx->HasErrorCallback() )
                    aCtx->ReportError( _( "Footprint has no front courtyard." ) );
            }
            else if( areaOutline.Collide( &courtyard.Outline( 0 ) ) )
            {
                return true;
            }
        }

        if( ( aArea->GetLayerSet() & LSET::BackMask() ).any() )
        {
            const SHAPE_POLY_SET& courtyard = footprint->GetCourtyard( B_CrtYd );

            if( courtyard.OutlineCount() == 0 )
            {
                if( aCtx->HasErrorCallback() )
                    aCtx->ReportError( _( "Footprint has no back courtyard." ) );
            }
            else if( areaOutline.Collide( &courtyard.Outline( 0 ) ) )
            {
                return true;
            }
        }

        return false;
    }

    if( aItem->Type() == PCB_ZONE_T )
    {
        ZONE* zone = static_cast<ZONE*>( aItem );

        if( !zone->IsFilled() )
            return false;

        DRC_RTREE* zoneRTree = board->m_CopperZoneRTreeCache[ zone ].get();

        if( zoneRTree )
        {
            if( zoneRTree->QueryColliding( areaBBox, &areaOutline, aLayer ) )
                return true;
        }

        return false;
    }
    else
    {
        if( !aArea->GetLayerSet().Contains( aLayer ) )
            return false;

        return areaOutline.Collide( aItem->GetEffectiveShape( aLayer ).get() );
    }
}


bool searchAreas( BOARD* aBoard, const wxString& aArg, PCBEXPR_CONTEXT* aCtx,
                  const std::function<bool( ZONE* )>& aFunc )
{
    if( aArg == wxT( "A" ) )
    {
        return aFunc( dynamic_cast<ZONE*>( aCtx->GetItem( 0 ) ) );
    }
    else if( aArg == wxT( "B" ) )
    {
        return aFunc( dynamic_cast<ZONE*>( aCtx->GetItem( 1 ) ) );
    }
    else if( KIID::SniffTest( aArg ) )
    {
        KIID target( aArg );

        for( ZONE* area : aBoard->Zones() )
        {
            // Only a single zone can match the UUID; exit once we find a match whether
            // "inside" or not
            if( area->m_Uuid == target )
                return aFunc( area );
        }

        for( FOOTPRINT* footprint : aBoard->Footprints() )
        {
            for( ZONE* area : footprint->Zones() )
            {
                // Only a single zone can match the UUID; exit once we find a match
                // whether "inside" or not
                if( area->m_Uuid == target )
                    return aFunc( area );
            }
        }

        return false;
    }
    else  // Match on zone name
    {
        for( ZONE* area : aBoard->Zones() )
        {
            if( area->GetZoneName().Matches( aArg ) )
            {
                // Many zones can match the name; exit only when we find an "inside"
                if( aFunc( area ) )
                    return true;
            }
        }

        for( FOOTPRINT* footprint : aBoard->Footprints() )
        {
            for( ZONE* area : footprint->Zones() )
            {
                // Many zones can match the name; exit only when we find an "inside"
                if( area->GetZoneName().Matches( aArg ) )
                {
                    if( aFunc( area ) )
                        return true;
                }
            }
        }

        return false;
    }
}


class SCOPED_LAYERSET
{
public:
    SCOPED_LAYERSET( BOARD_ITEM* aItem )
    {
        m_item = aItem;
        m_layers = aItem->GetLayerSet();
    }

    ~SCOPED_LAYERSET()
    {
        m_item->SetLayerSet( m_layers );
    }

    void Add( PCB_LAYER_ID aLayer )
    {
        m_item->SetLayerSet( m_item->GetLayerSet().set( aLayer ) );
    }

private:
    BOARD_ITEM* m_item;
    LSET        m_layers;
};


#define MISSING_AREA_ARG( f ) \
    wxString::Format( _( "Missing rule-area argument (A, B, or rule-area name) to %s." ), f )

static void intersectsAreaFunc( LIBEVAL::CONTEXT* aCtx, void* self )
{
    PCBEXPR_CONTEXT* context = static_cast<PCBEXPR_CONTEXT*>( aCtx );
    LIBEVAL::VALUE*  arg = aCtx->Pop();
    LIBEVAL::VALUE*  result = aCtx->AllocValue();

    result->Set( 0.0 );
    aCtx->Push( result );

    if( !arg || arg->AsString().IsEmpty() )
    {
        if( aCtx->HasErrorCallback() )
            aCtx->ReportError( MISSING_AREA_ARG( wxT( "intersectsArea()" ) ) );

        return;
    }

    PCBEXPR_VAR_REF* vref = static_cast<PCBEXPR_VAR_REF*>( self );
    BOARD_ITEM*      item = vref ? vref->GetObject( context ) : nullptr;

    if( !item )
        return;

    result->SetDeferredEval(
            [item, arg, context]() -> double
            {
                BOARD*       board = item->GetBoard();
                PCB_LAYER_ID aLayer = context->GetLayer();
                BOX2I        itemBBox = item->GetBoundingBox();

                if( searchAreas( board, arg->AsString(), context,
                        [&]( ZONE* aArea )
                        {
                            if( !aArea || aArea == item || aArea->GetParent() == item )
                                return false;

                            SCOPED_LAYERSET scopedLayerSet( aArea );

                            if( context->GetConstraint() == SILK_CLEARANCE_CONSTRAINT )
                            {
                                // Silk clearance tests are run across layer pairs
                                if(    ( aArea->IsOnLayer( F_SilkS ) && IsFrontLayer( aLayer ) )
                                    || ( aArea->IsOnLayer( B_SilkS ) && IsBackLayer( aLayer ) ) )
                                {
                                    scopedLayerSet.Add( aLayer );
                                }
                            }

                            LSET commonLayers = aArea->GetLayerSet() & item->GetLayerSet();

                            if( !commonLayers.any() )
                                return false;

                            if( !aArea->GetBoundingBox().Intersects( itemBBox ) )
                                return false;

                            LSET testLayers;

                            if( aLayer != UNDEFINED_LAYER )
                                testLayers.set( aLayer );
                            else
                                testLayers = commonLayers;

                            bool isTransient = ( item->GetFlags() & ROUTER_TRANSIENT ) != 0;
                            std::vector<PCB_LAYER_ID> layersToCompute;

                            if( !isTransient )
                            {
                                std::shared_lock<std::shared_mutex> readLock( board->m_CachesMutex );

                                for( PCB_LAYER_ID layer : testLayers.UIOrder() )
                                {
                                    PTR_PTR_LAYER_CACHE_KEY key = { aArea, item, layer };
                                    auto i = board->m_IntersectsAreaCache.find( key );

                                    if( i != board->m_IntersectsAreaCache.end() )
                                    {
                                        if( i->second )
                                            return true;
                                    }
                                    else
                                    {
                                        layersToCompute.push_back( layer );
                                    }
                                }
                            }
                            else
                            {
                                for( PCB_LAYER_ID layer : testLayers.UIOrder() )
                                    layersToCompute.push_back( layer );
                            }

                            std::vector<std::pair<PTR_PTR_LAYER_CACHE_KEY, bool>> results;
                            bool anyCollision = false;

                            for( PCB_LAYER_ID layer : layersToCompute )
                            {
                                bool collides = collidesWithArea( item, layer, context, aArea );

                                if( !isTransient )
                                    results.push_back( { { aArea, item, layer }, collides } );

                                if( collides )
                                    anyCollision = true;
                            }

                            if( !isTransient && !results.empty() )
                            {
                                std::unique_lock<std::shared_mutex> writeLock( board->m_CachesMutex );

                                for( const auto& [key, collides] : results )
                                    board->m_IntersectsAreaCache[key] = collides;
                            }

                            return anyCollision;
                        } ) )
                {
                    return 1.0;
                }

                return 0.0;
            } );
}


static void enclosedByAreaFunc( LIBEVAL::CONTEXT* aCtx, void* self )
{
    PCBEXPR_CONTEXT* context = static_cast<PCBEXPR_CONTEXT*>( aCtx );
    LIBEVAL::VALUE*  arg = aCtx->Pop();
    LIBEVAL::VALUE*  result = aCtx->AllocValue();

    result->Set( 0.0 );
    aCtx->Push( result );

    if( !arg || arg->AsString().IsEmpty() )
    {
        if( aCtx->HasErrorCallback() )
            aCtx->ReportError( MISSING_AREA_ARG( wxT( "enclosedByArea()" ) ) );

        return;
    }

    PCBEXPR_VAR_REF* vref = static_cast<PCBEXPR_VAR_REF*>( self );
    BOARD_ITEM*      item = vref ? vref->GetObject( context ) : nullptr;

    if( !item )
        return;

    result->SetDeferredEval(
            [item, arg, context]() -> double
            {
                BOARD*       board = item->GetBoard();
                int          maxError = board->GetDesignSettings().m_MaxError;
                PCB_LAYER_ID layer = context->GetLayer();
                BOX2I        itemBBox = item->GetBoundingBox();

                if( searchAreas( board, arg->AsString(), context,
                        [&]( ZONE* aArea )
                        {
                            if( !aArea || aArea == item || aArea->GetParent() == item )
                                return false;

                            if( item->Type() != PCB_FOOTPRINT_T )
                            {
                                if( !( aArea->GetLayerSet() & item->GetLayerSet() ).any() )
                                    return false;
                            }

                            if( !aArea->GetBoundingBox().Intersects( itemBBox ) )
                                return false;

                            PTR_PTR_LAYER_CACHE_KEY key = { aArea, item, layer };

                            if( ( item->GetFlags() & ROUTER_TRANSIENT ) == 0 )
                            {
                                std::shared_lock<std::shared_mutex> readLock( board->m_CachesMutex );

                                auto i = board->m_EnclosedByAreaCache.find( key );

                                if( i != board->m_EnclosedByAreaCache.end() )
                                    return i->second;
                            }

                            SHAPE_POLY_SET itemShape;
                            bool           enclosedByArea;

                            if( item->Type() == PCB_ZONE_T )
                            {
                                itemShape = *static_cast<ZONE*>( item )->Outline();
                            }
                            else if( item->Type() == PCB_FOOTPRINT_T )
                            {
                                FOOTPRINT* fp = static_cast<FOOTPRINT*>( item );

                                for( PCB_LAYER_ID testLayer : aArea->GetLayerSet() )
                                {
                                    fp->TransformPadsToPolySet( itemShape, testLayer, 0,
                                                                maxError, ERROR_OUTSIDE );
                                    fp->TransformFPShapesToPolySet( itemShape, testLayer, 0,
                                                                    maxError, ERROR_OUTSIDE );
                                }
                            }
                            else
                            {
                                item->TransformShapeToPolygon( itemShape, layer, 0, maxError,
                                                               ERROR_OUTSIDE );
                            }

                            if( itemShape.IsEmpty() )
                            {
                                // If it's already empty then our test will have no meaning.
                                enclosedByArea = false;
                            }
                            else
                            {
                                itemShape.BooleanSubtract( *aArea->Outline() );

                                enclosedByArea = itemShape.IsEmpty();
                            }

                            if( ( item->GetFlags() & ROUTER_TRANSIENT ) == 0 )
                            {
                                std::unique_lock<std::shared_mutex> writeLock( board->m_CachesMutex );
                                board->m_EnclosedByAreaCache[ key ] = enclosedByArea;
                            }

                            return enclosedByArea;
                        } ) )
                {
                    return 1.0;
                }

                return 0.0;
            } );
}


#define MISSING_GROUP_ARG( f ) \
    wxString::Format( _( "Missing group name argument to %s." ), f )

static void memberOfGroupFunc( LIBEVAL::CONTEXT* aCtx, void* self )
{
    LIBEVAL::VALUE* arg = aCtx->Pop();
    LIBEVAL::VALUE* result = aCtx->AllocValue();

    result->Set( 0.0 );
    aCtx->Push( result );

    if( !arg || arg->AsString().IsEmpty() )
    {
        if( aCtx->HasErrorCallback() )
            aCtx->ReportError( MISSING_GROUP_ARG( wxT( "memberOfGroup()" ) ) );

        return;
    }

    PCBEXPR_VAR_REF* vref = static_cast<PCBEXPR_VAR_REF*>( self );
    BOARD_ITEM*      item = vref ? vref->GetObject( aCtx ) : nullptr;

    if( !item )
        return;

    result->SetDeferredEval(
            [item, arg]() -> double
            {
                EDA_GROUP* group = item->GetParentGroup();

                if( !group && item->GetParent() && item->GetParent()->Type() == PCB_FOOTPRINT_T )
                    group = item->GetParent()->GetParentGroup();

                while( group )
                {
                    if( group->GetName().Matches( arg->AsString() ) )
                        return 1.0;

                    group = group->AsEdaItem()->GetParentGroup();
                }

                return 0.0;
            } );
}


#define MISSING_SHEET_ARG( f ) \
    wxString::Format( _( "Missing sheet name argument to %s." ), f )

static void memberOfSheetFunc( LIBEVAL::CONTEXT* aCtx, void* self )
{
    LIBEVAL::VALUE* arg = aCtx->Pop();
    LIBEVAL::VALUE* result = aCtx->AllocValue();

    result->Set( 0.0 );
    aCtx->Push( result );

    if( !arg || arg->AsString().IsEmpty() )
    {
        if( aCtx->HasErrorCallback() )
            aCtx->ReportError( MISSING_SHEET_ARG( wxT( "memberOfSheet()" ) ) );

        return;
    }

    PCBEXPR_VAR_REF* vref = static_cast<PCBEXPR_VAR_REF*>( self );
    BOARD_ITEM*      item = vref ? vref->GetObject( aCtx ) : nullptr;

    if( !item )
        return;

    result->SetDeferredEval(
            [item, arg]() -> double
            {
                FOOTPRINT* fp = item->GetParentFootprint();

                if( !fp && item->Type() == PCB_FOOTPRINT_T )
                    fp = static_cast<FOOTPRINT*>( item );

                if( !fp )
                    return 0.0;

                wxString sheetName = fp->GetSheetname();
                wxString refName = arg->AsString();

                if( sheetName.EndsWith( wxT( "/" ) ) )
                    sheetName.RemoveLast();
                if( refName.EndsWith( wxT( "/" ) ) )
                    refName.RemoveLast();

                if( sheetName.Matches( refName ) )
                    return 1.0;

                if( ( refName.Matches( wxT( "/" ) ) || refName.IsEmpty() )
                    && sheetName.IsEmpty() )
                {
                    return 1.0;
                }

                return 0.0;
            } );
}


static void memberOfSheetOrChildrenFunc( LIBEVAL::CONTEXT* aCtx, void* self )
{
    LIBEVAL::VALUE* arg = aCtx->Pop();
    LIBEVAL::VALUE* result = aCtx->AllocValue();

    result->Set( 0.0 );
    aCtx->Push( result );

    if( !arg || arg->AsString().IsEmpty() )
    {
        if( aCtx->HasErrorCallback() )
            aCtx->ReportError( MISSING_SHEET_ARG( wxT( "memberOfSheetOrChildren()" ) ) );

        return;
    }

    PCBEXPR_VAR_REF* vref = static_cast<PCBEXPR_VAR_REF*>( self );
    BOARD_ITEM*      item = vref ? vref->GetObject( aCtx ) : nullptr;

    if( !item )
        return;

    result->SetDeferredEval(
            [item, arg]() -> double
            {
                FOOTPRINT* fp = item->GetParentFootprint();

                if( !fp && item->Type() == PCB_FOOTPRINT_T )
                    fp = static_cast<FOOTPRINT*>( item );

                if( !fp )
                    return 0.0;

                wxString sheetName = fp->GetSheetname();
                wxString refName = arg->AsString();

                if( sheetName.EndsWith( wxT( "/" ) ) )
                    sheetName.RemoveLast();
                if( refName.EndsWith( wxT( "/" ) ) )
                    refName.RemoveLast();

                wxArrayString sheetPath = wxSplit( sheetName, '/' );
                wxArrayString refPath = wxSplit( refName, '/' );

                if( refPath.size() > sheetPath.size() )
                    return 0.0;

                if( ( refName.Matches( wxT( "/" ) ) || refName.IsEmpty() ) && sheetName.IsEmpty() )
                {
                    return 1.0;
                }

                for( size_t i = 0; i < refPath.size(); i++ )
                {
                    if( !sheetPath[i].Matches( refPath[i] ) )
                        return 0.0;
                }

                return 1.0;
            } );
}


#define MISSING_REF_ARG( f ) \
    wxString::Format( _( "Missing footprint argument (reference designator) to %s." ), f )

static void memberOfFootprintFunc( LIBEVAL::CONTEXT* aCtx, void* self )
{
    LIBEVAL::VALUE* arg = aCtx->Pop();
    LIBEVAL::VALUE* result = aCtx->AllocValue();

    result->Set( 0.0 );
    aCtx->Push( result );

    if( !arg || arg->AsString().IsEmpty() )
    {
        if( aCtx->HasErrorCallback() )
            aCtx->ReportError( MISSING_REF_ARG( wxT( "memberOfFootprint()" ) ) );

        return;
    }

    PCBEXPR_VAR_REF* vref = static_cast<PCBEXPR_VAR_REF*>( self );
    BOARD_ITEM*      item = vref ? vref->GetObject( aCtx ) : nullptr;

    if( !item )
        return;

    result->SetDeferredEval(
            [item, arg]() -> double
            {
                if( FOOTPRINT* parentFP = item->GetParentFootprint() )
                {
                    if( testFootprintSelector( parentFP, arg->AsString() ) )
                        return 1.0;
                }

                return 0.0;
            } );
}


static void isMicroVia( LIBEVAL::CONTEXT* aCtx, void* self )
{
    PCBEXPR_VAR_REF* vref = static_cast<PCBEXPR_VAR_REF*>( self );
    BOARD_ITEM*      item = vref ? vref->GetObject( aCtx ) : nullptr;
    LIBEVAL::VALUE*  result = aCtx->AllocValue();

    result->Set( 0.0 );
    aCtx->Push( result );

    if( item && item->Type() == PCB_VIA_T && static_cast<PCB_VIA*>( item )->IsMicroVia() )
        result->Set( 1.0 );
}

static void isBlindVia( LIBEVAL::CONTEXT* aCtx, void* self )
{
    PCBEXPR_VAR_REF* vref = static_cast<PCBEXPR_VAR_REF*>( self );
    BOARD_ITEM*      item = vref ? vref->GetObject( aCtx ) : nullptr;
    LIBEVAL::VALUE*  result = aCtx->AllocValue();

    result->Set( 0.0 );
    aCtx->Push( result );

    if( item && item->Type() == PCB_VIA_T && static_cast<PCB_VIA*>( item )->IsBlindVia() )
        result->Set( 1.0 );
}

static void isBuriedVia( LIBEVAL::CONTEXT* aCtx, void* self )
{
    PCBEXPR_VAR_REF* vref = static_cast<PCBEXPR_VAR_REF*>( self );
    BOARD_ITEM*      item = vref ? vref->GetObject( aCtx ) : nullptr;
    LIBEVAL::VALUE*  result = aCtx->AllocValue();

    result->Set( 0.0 );
    aCtx->Push( result );

    if( item && item->Type() == PCB_VIA_T && static_cast<PCB_VIA*>( item )->IsBuriedVia() )
        result->Set( 1.0 );
}

static void isBlindBuriedViaFunc( LIBEVAL::CONTEXT* aCtx, void* self )
{
    PCBEXPR_VAR_REF* vref = static_cast<PCBEXPR_VAR_REF*>( self );
    BOARD_ITEM*      item = vref ? vref->GetObject( aCtx ) : nullptr;
    LIBEVAL::VALUE*  result = aCtx->AllocValue();

    result->Set( 0.0 );
    aCtx->Push( result );

    if( item && item->Type() == PCB_VIA_T )
    {
        PCB_VIA* via = static_cast<PCB_VIA*>( item );

        if( via->IsBlindVia() || via->IsBuriedVia() )
            result->Set( 1.0 );
    }
}


static void isCoupledDiffPairFunc( LIBEVAL::CONTEXT* aCtx, void* self )
{
    PCBEXPR_CONTEXT*      context = static_cast<PCBEXPR_CONTEXT*>( aCtx );
    BOARD_CONNECTED_ITEM* a = dynamic_cast<BOARD_CONNECTED_ITEM*>( context->GetItem( 0 ) );
    BOARD_CONNECTED_ITEM* b = dynamic_cast<BOARD_CONNECTED_ITEM*>( context->GetItem( 1 ) );
    LIBEVAL::VALUE*       result = aCtx->AllocValue();

    result->Set( 0.0 );
    aCtx->Push( result );

    result->SetDeferredEval(
            [a, b, context]() -> double
            {
                NETINFO_ITEM* netinfo = a ? a->GetNet() : nullptr;

                if( !netinfo )
                    return 0.0;

                wxString coupledNet;
                wxString dummy;

                if( !DRC_ENGINE::MatchDpSuffix( netinfo->GetNetname(), coupledNet, dummy ) )
                    return 0.0;

                if( context->GetConstraint() == DRC_CONSTRAINT_T::DIFF_PAIR_GAP_CONSTRAINT
                        || context->GetConstraint() == DRC_CONSTRAINT_T::LENGTH_CONSTRAINT
                        || context->GetConstraint() == DRC_CONSTRAINT_T::SKEW_CONSTRAINT )
                {
                    // DRC engine evaluates these only in the context of a diffpair, but doesn't
                    // always supply the second (B) item.
                    if( BOARD* board = a->GetBoard() )
                    {
                        if( board->FindNet( coupledNet ) )
                            return 1.0;
                    }
                }

                if( b && b->GetNetname() == coupledNet )
                    return 1.0;

                return 0.0;
            } );
}


#define MISSING_DP_ARG( f ) \
    wxString::Format( _( "Missing diff-pair name argument to %s." ), f )

static void inDiffPairFunc( LIBEVAL::CONTEXT* aCtx, void* self )
{
    LIBEVAL::VALUE*  argv   = aCtx->Pop();
    PCBEXPR_VAR_REF* vref   = static_cast<PCBEXPR_VAR_REF*>( self );
    BOARD_ITEM*      item   = vref ? vref->GetObject( aCtx ) : nullptr;
    LIBEVAL::VALUE*  result = aCtx->AllocValue();

    result->Set( 0.0 );
    aCtx->Push( result );

    if( !argv || argv->AsString().IsEmpty() )
    {
        if( aCtx->HasErrorCallback() )
            aCtx->ReportError( MISSING_DP_ARG( wxT( "inDiffPair()" ) ) );

        return;
    }

    if( !item || !item->GetBoard() )
        return;

    result->SetDeferredEval(
            [item, argv]() -> double
            {
                if( item && item->IsConnected() )
                {
                    NETINFO_ITEM* netinfo = static_cast<BOARD_CONNECTED_ITEM*>( item )->GetNet();

                    if( !netinfo )
                        return 0.0;

                    wxString refName = netinfo->GetNetname();
                    wxString arg = argv->AsString();
                    wxString baseName, coupledNet;
                    int      polarity = DRC_ENGINE::MatchDpSuffix( refName, coupledNet, baseName );

                    if( polarity != 0 && item->GetBoard()->FindNet( coupledNet ) )
                    {
                        if( baseName.Matches( arg ) )
                            return 1.0;

                        if( baseName.EndsWith( "_" ) && baseName.BeforeLast( '_' ).Matches( arg ) )
                            return 1.0;
                    }
                }

                return 0.0;
            } );
}


static void getFieldFunc( LIBEVAL::CONTEXT* aCtx, void* self )
{
    LIBEVAL::VALUE*  arg    = aCtx->Pop();
    PCBEXPR_VAR_REF* vref   = static_cast<PCBEXPR_VAR_REF*>( self );
    BOARD_ITEM*      item   = vref ? vref->GetObject( aCtx ) : nullptr;
    LIBEVAL::VALUE*  result = aCtx->AllocValue();

    result->Set( "" );
    aCtx->Push( result );

    if( !arg )
    {
        if( aCtx->HasErrorCallback() )
        {
            aCtx->ReportError( wxString::Format( _( "Missing field name argument to %s." ),
                                                 wxT( "getField()" ) ) );
        }

        return;
    }

    if( !item || !item->GetBoard() )
        return;

    result->SetDeferredEval(
            [item, arg]() -> wxString
            {
                if( item && item->Type() == PCB_FOOTPRINT_T )
                {
                    FOOTPRINT* fp = static_cast<FOOTPRINT*>( item );

                    PCB_FIELD* field = fp->GetField( arg->AsString() );

                    if( field )
                        return field->GetText();
                }

                return "";
            } );
}


static void hasNetclassFunc( LIBEVAL::CONTEXT* aCtx, void* self )
{
    LIBEVAL::VALUE* arg = aCtx->Pop();
    LIBEVAL::VALUE* result = aCtx->AllocValue();

    result->Set( 0.0 );
    aCtx->Push( result );

    if( !arg || arg->AsString().IsEmpty() )
    {
        if( aCtx->HasErrorCallback() )
            aCtx->ReportError( _( "Missing netclass name argument to hasNetclass()" ) );

        return;
    }

    PCBEXPR_VAR_REF* vref = static_cast<PCBEXPR_VAR_REF*>( self );
    BOARD_ITEM*      item = vref ? vref->GetObject( aCtx ) : nullptr;

    if( !item )
        return;

    result->SetDeferredEval(
            [item, arg]() -> double
            {
                if( !item->IsConnected() )
                    return 0.0;

                BOARD_CONNECTED_ITEM* bcItem = static_cast<BOARD_CONNECTED_ITEM*>( item );
                NETCLASS*             netclass = bcItem->GetEffectiveNetClass();

                if( netclass && netclass->ContainsNetclassWithName( arg->AsString() ) )
                    return 1.0;

                return 0.0;
            } );
}


static void hasExactNetclassFunc( LIBEVAL::CONTEXT* aCtx, void* self )
{
    LIBEVAL::VALUE* arg = aCtx->Pop();
    LIBEVAL::VALUE* result = aCtx->AllocValue();

    result->Set( 0.0 );
    aCtx->Push( result );

    if( !arg || arg->AsString().IsEmpty() )
    {
        if( aCtx->HasErrorCallback() )
            aCtx->ReportError( _( "Missing netclass name argument to hasExactNetclass()" ) );

        return;
    }

    PCBEXPR_VAR_REF* vref = static_cast<PCBEXPR_VAR_REF*>( self );
    BOARD_ITEM*      item = vref ? vref->GetObject( aCtx ) : nullptr;

    if( !item )
        return;

    result->SetDeferredEval(
            [item, arg]() -> double
            {
                if( !item->IsConnected() )
                    return 0.0;

                BOARD_CONNECTED_ITEM* bcItem = static_cast<BOARD_CONNECTED_ITEM*>( item );
                BOARD*                board = bcItem->GetBoard();
                wxString              netclassName;

                if( board && ( item->GetFlags() & ROUTER_TRANSIENT ) == 0 )
                {
                    std::shared_lock<std::shared_mutex> readLock( board->m_CachesMutex );

                    auto it = board->m_ItemNetclassCache.find( item );

                    if( it != board->m_ItemNetclassCache.end() )
                        netclassName = it->second;
                }

                if( netclassName.empty() )
                {
                    NETCLASS* netclass = bcItem->GetEffectiveNetClass();

                    if( netclass )
                        netclassName = netclass->GetName();

                    if( board && !netclassName.empty() && ( item->GetFlags() & ROUTER_TRANSIENT ) == 0 )
                    {
                        std::unique_lock<std::shared_mutex> writeLock( board->m_CachesMutex );
                        board->m_ItemNetclassCache[item] = netclassName;
                    }
                }

                return ( netclassName == arg->AsString() ) ? 1.0 : 0.0;
            } );
}


static void hasComponentClassFunc( LIBEVAL::CONTEXT* aCtx, void* self )
{
    LIBEVAL::VALUE* arg = aCtx->Pop();
    LIBEVAL::VALUE* result = aCtx->AllocValue();

    result->Set( 0.0 );
    aCtx->Push( result );

    if( !arg || arg->AsString().IsEmpty() )
    {
        if( aCtx->HasErrorCallback() )
            aCtx->ReportError( _( "Missing component class name argument to hasComponentClass()" ) );

        return;
    }

    PCBEXPR_VAR_REF* vref = static_cast<PCBEXPR_VAR_REF*>( self );
    BOARD_ITEM*      item = vref ? vref->GetObject( aCtx ) : nullptr;

    if( !item )
        return;

    result->SetDeferredEval(
            [item, arg]() -> double
            {
                FOOTPRINT* footprint = nullptr;

                if( item->Type() == PCB_FOOTPRINT_T )
                    footprint = static_cast<FOOTPRINT*>( item );
                else
                    footprint = item->GetParentFootprint();

                if( !footprint )
                    return 0.0;

                const COMPONENT_CLASS* compClass = footprint->GetComponentClass();

                if( compClass && compClass->ContainsClassName( arg->AsString() ) )
                    return 1.0;

                return 0.0;
            } );
}


PCBEXPR_BUILTIN_FUNCTIONS::PCBEXPR_BUILTIN_FUNCTIONS()
{
    RegisterAllFunctions();
}


void PCBEXPR_BUILTIN_FUNCTIONS::RegisterAllFunctions()
{
    m_funcs.clear();

    RegisterFunc( wxT( "existsOnLayer('x')" ), existsOnLayerFunc );

    RegisterFunc( wxT( "isPlated()" ), isPlatedFunc );

    RegisterFunc( wxT( "insideCourtyard('x') DEPRECATED" ), intersectsCourtyardFunc );
    RegisterFunc( wxT( "insideFrontCourtyard('x') DEPRECATED" ), intersectsFrontCourtyardFunc );
    RegisterFunc( wxT( "insideBackCourtyard('x') DEPRECATED" ), intersectsBackCourtyardFunc );
    RegisterFunc( wxT( "intersectsCourtyard('x')" ), intersectsCourtyardFunc );
    RegisterFunc( wxT( "intersectsFrontCourtyard('x')" ), intersectsFrontCourtyardFunc );
    RegisterFunc( wxT( "intersectsBackCourtyard('x')" ), intersectsBackCourtyardFunc );

    RegisterFunc( wxT( "insideArea('x') DEPRECATED" ), intersectsAreaFunc );
    RegisterFunc( wxT( "intersectsArea('x')" ), intersectsAreaFunc );
    RegisterFunc( wxT( "enclosedByArea('x')" ), enclosedByAreaFunc );

    RegisterFunc( wxT( "isMicroVia()" ), isMicroVia );
    RegisterFunc( wxT( "isBlindVia()" ), isBlindVia );
    RegisterFunc( wxT( "isBuriedVia()" ), isBuriedVia );
    RegisterFunc( wxT( "isBlindBuriedVia()" ), isBlindBuriedViaFunc );

    RegisterFunc( wxT( "memberOf('x') DEPRECATED" ), memberOfGroupFunc );
    RegisterFunc( wxT( "memberOfGroup('x')" ), memberOfGroupFunc );
    RegisterFunc( wxT( "memberOfFootprint('x')" ), memberOfFootprintFunc );
    RegisterFunc( wxT( "memberOfSheet('x')" ), memberOfSheetFunc );
    RegisterFunc( wxT( "memberOfSheetOrChildren('x')" ), memberOfSheetOrChildrenFunc );

    RegisterFunc( wxT( "fromTo('x','y')" ), fromToFunc );
    RegisterFunc( wxT( "isCoupledDiffPair()" ), isCoupledDiffPairFunc );
    RegisterFunc( wxT( "inDiffPair('x')" ), inDiffPairFunc );

    RegisterFunc( wxT( "getField('x')" ), getFieldFunc );

    RegisterFunc( wxT( "hasNetclass('x')" ), hasNetclassFunc );
    RegisterFunc( wxT( "hasExactNetclass('x')" ), hasExactNetclassFunc );
    RegisterFunc( wxT( "hasComponentClass('x')" ), hasComponentClassFunc );
}
