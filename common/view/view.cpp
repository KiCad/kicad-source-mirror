/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <boost/foreach.hpp>

#include <base_struct.h>
#include <layers_id_colors_and_visibility.h>

#include <view/view.h>
#include <view/view_group.h>
#include <view/view_rtree.h>
#include <gal/definitions.h>
#include <gal/graphics_abstraction_layer.h>
#include <painter.h>

#ifdef __WXDEBUG__
#include <profile.h>
#endif /* __WXDEBUG__ */

using namespace KiGfx;

VIEW::VIEW( bool aIsDynamic ) :
    m_enableOrderModifier( false ),
    m_scale( 1.0 ),
    m_painter( NULL ),
    m_gal( NULL ),
    m_dynamic( aIsDynamic )
{
    // Redraw everything at the beginning
    for( int i = 0; i < TARGETS_NUMBER; ++i )
        MarkTargetDirty( i );

    // View uses layers to display EDA_ITEMs (item may be displayed on several layers, for example
    // pad may be shown on pad, pad hole and solder paste layers). There are usual copper layers
    // (eg. F.Cu, B.Cu, internal and so on) and layers for displaying objects such as texts,
    // silkscreen, pads, vias, etc.
    for( int i = 0; i < VIEW_MAX_LAYERS; i++ )
    {
        AddLayer( i );
    }
}


VIEW::~VIEW()
{
    BOOST_FOREACH( LayerMap::value_type& l, m_layers )
    {
        delete l.second.items;
    }
}


void VIEW::AddLayer( int aLayer, bool aDisplayOnly )
{
    if( m_layers.find( aLayer ) == m_layers.end() )
    {
        m_layers[aLayer]                = VIEW_LAYER();
        m_layers[aLayer].id             = aLayer;
        m_layers[aLayer].items          = new VIEW_RTREE();
        m_layers[aLayer].renderingOrder = aLayer;
        m_layers[aLayer].enabled        = true;
        m_layers[aLayer].isDirty        = false;
        m_layers[aLayer].displayOnly    = aDisplayOnly;
        m_layers[aLayer].target         = TARGET_CACHED;
    }

    sortLayers();
}


void VIEW::Add( VIEW_ITEM* aItem )
{
    int layers[VIEW_MAX_LAYERS], layers_count;

    aItem->ViewGetLayers( layers, layers_count );
    aItem->saveLayers( layers, layers_count );

    for( int i = 0; i < layers_count; i++ )
    {
        VIEW_LAYER& l = m_layers[layers[i]];
        l.items->Insert( aItem );
        l.isDirty = true;
    }

    if( m_dynamic )
        aItem->viewAssign( this );
}


void VIEW::Remove( VIEW_ITEM* aItem )
{
    if( m_dynamic )
        aItem->m_view = NULL;

    int layers[VIEW::VIEW_MAX_LAYERS], layers_count;
    aItem->getLayers( layers, layers_count );

    for( int i = 0; i < layers_count; ++i )
    {
        VIEW_LAYER& l = m_layers[layers[i]];
        l.items->Remove( aItem );
        l.isDirty = true;
    }
}


void VIEW::SetRequired( int aLayerId, int aRequiredId, bool aRequired )
{
    wxASSERT( (unsigned) aLayerId < m_layers.size() );
    wxASSERT( (unsigned) aRequiredId < m_layers.size() );

    if( aRequired )
    {
        m_layers[aLayerId].requiredLayers.insert( aRequiredId );
    }
    else
    {
        m_layers[aLayerId].requiredLayers.erase( aRequired );
    }
}


// stupid C++... python lamda would do this in one line
template <class Container>
struct queryVisitor
{
    typedef typename Container::value_type item_type;

    queryVisitor( Container& aCont, int aLayer ) :
        m_cont( aCont ), m_layer( aLayer )
    {
    }

    void operator()( VIEW_ITEM* aItem )
    {
        if( aItem->ViewIsVisible() )
            m_cont.push_back( VIEW::LayerItemPair( aItem, m_layer ) );
    }

    Container&  m_cont;
    int         m_layer;
};


int VIEW::Query( const BOX2I& aRect, std::vector<LayerItemPair>& aResult )
{
    if( m_orderedLayers.empty() )
        return 0;

    std::vector<VIEW_LAYER*>::reverse_iterator i;

    // execute queries in reverse direction, so that items that are on the top of
    // the rendering stack are returned first.
    for( i = m_orderedLayers.rbegin(); i != m_orderedLayers.rend(); ++i )
    {
        // ignore layers that do not contain actual items (i.e. the selection box, menus, floats)
        if( ( *i )->displayOnly )
            continue;

        queryVisitor<std::vector<LayerItemPair> > visitor( aResult, ( *i )->id );
        ( *i )->items->Query( aRect, visitor );
    }

    return aResult.size();
}


VECTOR2D VIEW::ToWorld( const VECTOR2D& aCoord, bool aAbsolute ) const
{
    MATRIX3x3D matrix = m_gal->GetWorldScreenMatrix().Inverse();

    if( aAbsolute )
    {
        return VECTOR2D( matrix * aCoord );
    }
    else
    {
        return VECTOR2D( matrix.GetScale().x * aCoord.x, matrix.GetScale().y * aCoord.y );
    }
}


VECTOR2D VIEW::ToScreen( const VECTOR2D& aCoord, bool aAbsolute ) const
{
    MATRIX3x3D matrix = m_gal->GetWorldScreenMatrix();

    if( aAbsolute )
    {
        return VECTOR2D( matrix * aCoord );
    }
    else
    {
        return VECTOR2D( matrix.GetScale().x * aCoord.x, matrix.GetScale().y * aCoord.y );
    }
}


double VIEW::ToScreen( double aCoord, bool aAbsolute ) const
{
    VECTOR2D t( aCoord, 0 );

    return ToScreen( t, aAbsolute ).x;
}


void VIEW::CopySettings( const VIEW* aOtherView )
{
    wxASSERT_MSG( false, wxT( "This is not implemented" ) );
}


void VIEW::SetGAL( GAL* aGal )
{
    m_gal = aGal;

    // clear group numbers, so everything is going to be recached
    clearGroupCache();

    // every target has to be refreshed
    MarkTargetDirty( TARGET_CACHED );
    MarkTargetDirty( TARGET_NONCACHED );
    MarkTargetDirty( TARGET_OVERLAY );

    // force the new GAL to display the current viewport.
    SetCenter( m_center );
    SetScale( m_scale );
}


void VIEW::SetPainter( PAINTER* aPainter )
{
    m_painter = aPainter;
}


BOX2D VIEW::GetViewport() const
{
    BOX2D    rect;
    VECTOR2D screenSize = m_gal->GetScreenPixelSize();

    rect.SetOrigin( ToWorld( VECTOR2D( 0, 0 ) ) );
    rect.SetEnd( ToWorld( screenSize ) );

    return rect.Normalize();
}


void VIEW::SetViewport( const BOX2D& aViewport, bool aKeepAspect )
{
    VECTOR2D ssize  = ToWorld( m_gal->GetScreenPixelSize(), false );
    VECTOR2D centre = aViewport.Centre();
    VECTOR2D vsize  = aViewport.GetSize();
    double   zoom   = 1.0 / std::min( fabs( vsize.x / ssize.x ), fabs( vsize.y / ssize.y ) );

    SetCenter( centre );
    SetScale( GetScale() * zoom );
}


void VIEW::SetMirror( bool aMirrorX, bool aMirrorY )
{
    m_gal->SetFlip( aMirrorX, aMirrorY );
}


void VIEW::SetScale( double aScale )
{
    SetScale( aScale, m_center );
}


void VIEW::SetScale( double aScale, const VECTOR2D& aAnchor )
{
    VECTOR2D a = ToScreen( aAnchor );

    m_gal->SetZoomFactor( aScale );
    m_gal->ComputeWorldScreenMatrix();

    VECTOR2D delta = ToWorld( a ) - aAnchor;

    SetCenter( m_center - delta );
    m_scale = aScale;

    // Redraw everything after the viewport has changed
    MarkTargetDirty( TARGET_CACHED );
}


void VIEW::SetCenter( const VECTOR2D& aCenter )
{
    m_center = aCenter;
    m_gal->SetLookAtPoint( m_center );
    m_gal->ComputeWorldScreenMatrix();

    // Redraw everything after the viewport has changed
    MarkTargetDirty( TARGET_CACHED );
}


void VIEW::SetLayerOrder( int aLayer, int aRenderingOrder )
{
    m_layers[aLayer].renderingOrder = aRenderingOrder;

    sortLayers();
}


int VIEW::GetLayerOrder( int aLayer ) const
{
    return m_layers.at(aLayer).renderingOrder;
}


void VIEW::SortLayers( int aLayers[], int& aCount ) const
{
    int maxLay, maxOrd, maxIdx;

    for( int i = 0; i < aCount; ++i )
    {
        maxLay = aLayers[i];
        maxOrd = GetLayerOrder( maxLay );
        maxIdx = i;

        // Look for the max element in the range (j..aCount)
        for( int j = i; j < aCount; ++j )
        {
            if( maxOrd < GetLayerOrder( aLayers[j] ) )
            {
                maxLay = aLayers[j];
                maxOrd = GetLayerOrder( maxLay );
                maxIdx = j;
            }
        }

        // Swap elements
        aLayers[maxIdx] = aLayers[i];
        aLayers[i] = maxLay;
    }
}


struct VIEW::updateItemsColor
{
    updateItemsColor( int aLayer, PAINTER* aPainter, GAL* aGal ) :
        layer( aLayer ), painter( aPainter), gal( aGal )
    {
    }

    void operator()( VIEW_ITEM* aItem )
    {
        // Obtain the color that should be used for coloring the item
        const COLOR4D color = painter->GetColor( aItem, layer );
        int group = aItem->getGroup( layer );

        if( group >= 0 )
            gal->ChangeGroupColor( group, color );
    }

    int layer;
    PAINTER* painter;
    GAL* gal;
};


void VIEW::UpdateLayerColor( int aLayer )
{
    // There is no point in updating non-cached layers
    if( !IsCached( aLayer ) )
        return;

    BOX2I r;

    r.SetMaximum();

    updateItemsColor visitor( aLayer, m_painter, m_gal );
    m_layers[aLayer].items->Query( r, visitor );
}


void VIEW::UpdateAllLayersColor()
{
    BOX2I r;

    r.SetMaximum();

    for( LayerMapIter i = m_layers.begin(); i != m_layers.end(); ++i )
    {
        VIEW_LAYER* l = &( ( *i ).second );

        // There is no point in updating non-cached layers
        if( !IsCached( l->id ) )
            continue;

        updateItemsColor visitor( l->id, m_painter, m_gal );
        l->items->Query( r, visitor );
    }
}


struct VIEW::changeItemsDepth
{
    changeItemsDepth( int aLayer, int aDepth, GAL* aGal ) :
        layer( aLayer ), depth( aDepth ), gal( aGal )
    {
    }

    void operator()( VIEW_ITEM* aItem )
    {
        int group = aItem->getGroup( layer );

        if( group >= 0 )
            gal->ChangeGroupDepth( group, depth );
    }

    int layer, depth;
    GAL* gal;
};


void VIEW::ChangeLayerDepth( int aLayer, int aDepth )
{
    // There is no point in updating non-cached layers
    if( !IsCached( aLayer ) )
        return;

    BOX2I r;

    r.SetMaximum();

    changeItemsDepth visitor( aLayer, aDepth, m_gal );
    m_layers[aLayer].items->Query( r, visitor );
}


void VIEW::SetTopLayer( int aLayer, bool aEnabled )
{
    if( aEnabled )
    {
        if( m_topLayers.count( aLayer ) == 1 )
            return;

        m_topLayers.insert( aLayer );

        // Move the layer closer to front
        if( m_enableOrderModifier )
            m_layers[aLayer].renderingOrder += TOP_LAYER_MODIFIER;
    }
    else
    {
        if( m_topLayers.count( aLayer ) == 0 )
            return;

        m_topLayers.erase( aLayer );

        // Restore the previous rendering order
        if( m_enableOrderModifier )
            m_layers[aLayer].renderingOrder -= TOP_LAYER_MODIFIER;
    }
}


void VIEW::EnableTopLayer( bool aEnable )
{
    if( aEnable == m_enableOrderModifier ) return;
    m_enableOrderModifier = aEnable;

    std::set<unsigned int>::iterator it;
    if( aEnable )
    {
        for( it = m_topLayers.begin(); it != m_topLayers.end(); ++it )
            m_layers[*it].renderingOrder += TOP_LAYER_MODIFIER;
    }
    else
    {
        for( it = m_topLayers.begin(); it != m_topLayers.end(); ++it )
            m_layers[*it].renderingOrder -= TOP_LAYER_MODIFIER;
    }

    UpdateAllLayersOrder();
    UpdateAllLayersColor();
}


void VIEW::ClearTopLayers()
{
    std::set<unsigned int>::iterator it;

    if( m_enableOrderModifier )
    {
        // Restore the previous rendering order for layers that were marked as top
        for( it = m_topLayers.begin(); it != m_topLayers.end(); ++it )
            m_layers[*it].renderingOrder -= TOP_LAYER_MODIFIER;
    }

    m_topLayers.clear();
}


void VIEW::UpdateAllLayersOrder()
{
    sortLayers();

    BOOST_FOREACH( LayerMap::value_type& l, m_layers )
    {
        ChangeLayerDepth( l.first, l.second.renderingOrder );
    }
}


struct VIEW::drawItem
{
    drawItem( VIEW* aView, const VIEW_LAYER* aCurrentLayer ) :
        currentLayer( aCurrentLayer ), view( aView )
    {
    }

    void operator()( VIEW_ITEM* aItem )
    {
        // Conditions that have te be fulfilled for an item to be drawn
        bool drawCondition = aItem->ViewIsVisible() &&
                             aItem->ViewGetLOD( currentLayer->id ) < view->m_scale;
        if( !drawCondition )
            return;

        view->draw( aItem, currentLayer->id );
    }

    const VIEW_LAYER* currentLayer;
    VIEW* view;
    int layersCount, layers[VIEW_MAX_LAYERS];
};


void VIEW::redrawRect( const BOX2I& aRect )
{
    BOOST_FOREACH( VIEW_LAYER* l, m_orderedLayers )
    {
        if( l->enabled && IsTargetDirty( l->target ) && areRequiredLayersEnabled( l->id ) )
        {
            drawItem drawFunc( this, l );

            m_gal->SetTarget( l->target );
            m_gal->SetLayerDepth( l->renderingOrder );
            l->items->Query( aRect, drawFunc );
        }

        l->isDirty = false;
    }
}


void VIEW::draw( VIEW_ITEM* aItem, int aLayer, bool aImmediate ) const
{
    if( IsCached( aLayer ) && !aImmediate )
    {
        // Draw using cached information or create one
        int group = aItem->getGroup( aLayer );

        if( group >= 0 )
        {
            m_gal->DrawGroup( group );
        }
        else
        {
            group = m_gal->BeginGroup();
            aItem->setGroup( aLayer, group );
            if( !m_painter->Draw( aItem, aLayer ) )
                aItem->ViewDraw( aLayer, m_gal, BOX2I() ); // Alternative drawing method
            m_gal->EndGroup();
        }
    }
    else
    {
        // Immediate mode
        if( !m_painter->Draw( aItem, aLayer ) )
            aItem->ViewDraw( aLayer, m_gal, BOX2I() );  // Alternative drawing method
    }

    // Draws a bright contour around the item
    if( static_cast<const EDA_ITEM*>( aItem )->IsBrightened() )
    {
        m_painter->DrawBrightened( aItem );
    }
}


void VIEW::draw( VIEW_ITEM* aItem, bool aImmediate ) const
{
    int layers[VIEW_MAX_LAYERS], layers_count;

    aItem->getLayers( layers, layers_count );
    // Sorting is needed for drawing order dependent GALs (like Cairo)
    SortLayers( layers, layers_count );

    for( int i = 0; i < layers_count; ++i )
    {
        m_gal->SetLayerDepth( m_layers.at( i ).renderingOrder );
        draw( aItem, layers[i], aImmediate );
    }
}


void VIEW::draw( VIEW_GROUP* aGroup, bool aImmediate ) const
{
    std::set<VIEW_ITEM*>::const_iterator it;
    for( it = aGroup->Begin(); it != aGroup->End(); ++it )
    {
        draw( *it, aImmediate );
    }
}


bool VIEW::IsDirty() const
{
    for( int i = 0; i < TARGETS_NUMBER; ++i )
    {
        if( IsTargetDirty( i ) )
            return true;
    }

    return false;
}


struct VIEW::unlinkItem
{
    void operator()( VIEW_ITEM* aItem )
    {
        aItem->m_view = NULL;
    }
};


struct VIEW::recacheLayer
{
    recacheLayer( VIEW* aView, GAL* aGal, int aLayer, bool aImmediately ) :
        view( aView ), gal( aGal ), layer( aLayer ), immediately( aImmediately )
    {
    }

    void operator()( VIEW_ITEM* aItem )
    {
        // Remove previously cached group
        int prevGroup = aItem->getGroup( layer );
        if( prevGroup >= 0 )
        {
            gal->DeleteGroup( prevGroup );
        }

        if( immediately )
        {
            int group = gal->BeginGroup();
            aItem->setGroup( layer, group );
            view->m_painter->Draw( static_cast<EDA_ITEM*>( aItem ), layer );
            gal->EndGroup();
        }
        else
        {
            aItem->setGroup( layer, -1 );
        }
    }

    VIEW* view;
    GAL* gal;
    int layer;
    bool immediately;
};


void VIEW::Clear()
{
    BOX2I r;

    r.SetMaximum();

    for( LayerMapIter i = m_layers.begin(); i != m_layers.end(); ++i )
    {
        VIEW_LAYER* l = &( ( *i ).second );
        unlinkItem v;

        if( m_dynamic )
            l->items->Query( r, v );

        l->items->RemoveAll();
    }

    m_gal->ClearCache();
}


void VIEW::PrepareTargets()
{
    if( IsTargetDirty( TARGET_CACHED ) || IsTargetDirty( TARGET_NONCACHED ) )
    {
        // TARGET_CACHED and TARGET_NONCACHED have to be redrawn together, as they contain
        // layers that rely on each other (eg. netnames are noncached, but tracks - are cached)
        m_gal->ClearTarget( TARGET_NONCACHED );
        m_gal->ClearTarget( TARGET_CACHED );

        MarkTargetDirty( TARGET_NONCACHED );
        MarkTargetDirty( TARGET_CACHED );
        MarkTargetDirty( TARGET_OVERLAY );
    }

    if( IsTargetDirty( TARGET_OVERLAY ) )
    {
        m_gal->ClearTarget( TARGET_OVERLAY );
    }
}


void VIEW::Redraw()
{
    VECTOR2D screenSize = m_gal->GetScreenPixelSize();
    BOX2I    rect( ToWorld( VECTOR2D( 0, 0 ) ),
                   ToWorld( screenSize ) - ToWorld( VECTOR2D( 0, 0 ) ) );
    rect.Normalize();

    redrawRect( rect );

    // All targets were redrawn, so nothing is dirty
    clearTargetDirty( TARGET_CACHED );
    clearTargetDirty( TARGET_NONCACHED );
    clearTargetDirty( TARGET_OVERLAY );
}


VECTOR2D VIEW::GetScreenPixelSize() const
{
    return m_gal->GetScreenPixelSize();
}


struct VIEW::clearLayerCache
{
    clearLayerCache( VIEW* aView ) :
        view( aView )
    {
    }

    void operator()( VIEW_ITEM* aItem )
    {
        if( aItem->storesGroups() )
        {
            aItem->deleteGroups();
        }
    }

    VIEW* view;
};


void VIEW::clearGroupCache()
{
    BOX2I r;

    r.SetMaximum();
    clearLayerCache visitor( this );

    for( LayerMapIter i = m_layers.begin(); i != m_layers.end(); ++i )
    {
        VIEW_LAYER* l = & ( ( *i ).second );
        l->items->Query( r, visitor );
    }
}


void VIEW::invalidateItem( VIEW_ITEM* aItem, int aUpdateFlags )
{
    int layers[VIEW_MAX_LAYERS], layers_count;
    aItem->getLayers( layers, layers_count );

    if( aUpdateFlags & VIEW_ITEM::GEOMETRY )
        updateBbox( aItem );

    // Iterate through layers used by the item and recache it immediately
    for( int i = 0; i < layers_count; i++ )
    {
        int layerId = layers[i];

        if( aUpdateFlags & VIEW_ITEM::GEOMETRY )
        {
            if( IsCached( layerId ) )
                updateItemGeometry( aItem, layerId );
        }
        else if( aUpdateFlags & VIEW_ITEM::COLOR )
        {
            updateItemColor( aItem, layerId );
        }

        // Mark those layers as dirty, so the VIEW will be refreshed
        m_layers[layerId].isDirty = true;
        MarkTargetDirty( m_layers[layerId].target );   // TODO remove?
    }
}


void VIEW::sortLayers()
{
    int n = 0;

    m_orderedLayers.resize( m_layers.size() );

    for( LayerMapIter i = m_layers.begin(); i != m_layers.end(); ++i )
        m_orderedLayers[n++] = &i->second;

    sort( m_orderedLayers.begin(), m_orderedLayers.end(), compareRenderingOrder );

    MarkTargetDirty( TARGET_CACHED );
}


void VIEW::updateItemColor( VIEW_ITEM* aItem, int aLayer )
{
    wxASSERT( (unsigned) aLayer < m_layers.size() );

    // Obtain the color that should be used for coloring the item on the specific layerId
    const COLOR4D color = m_painter->GetColor( aItem, aLayer );
    int group = aItem->getGroup( aLayer );

    // Change the color, only if it has group assigned
    if( group >= 0 )
        m_gal->ChangeGroupColor( group, color );
}


void VIEW::updateItemGeometry( VIEW_ITEM* aItem, int aLayer )
{
    wxASSERT( (unsigned) aLayer < m_layers.size() );
    VIEW_LAYER& l = m_layers.at( aLayer );

    m_gal->SetTarget( l.target );
    m_gal->SetLayerDepth( l.renderingOrder );

    // Redraw the item from scratch
    int group = m_gal->BeginGroup();
    aItem->setGroup( aLayer, group );
    m_painter->Draw( static_cast<EDA_ITEM*>( aItem ), aLayer );
    m_gal->EndGroup();
}


void VIEW::updateBbox( VIEW_ITEM* aItem )
{
    int layers[VIEW_MAX_LAYERS], layers_count;
    aItem->ViewGetLayers( layers, layers_count );

    for( int i = 0; i < layers_count; i++ )
    {
        VIEW_LAYER& l = m_layers[layers[i]];
        l.items->Remove( aItem );
        l.items->Insert( aItem );
        l.isDirty = true;
    }
}


bool VIEW::areRequiredLayersEnabled( int aLayerId ) const
{
    wxASSERT( (unsigned) aLayerId < m_layers.size() );

    std::set<int>::iterator it, it_end;

    for( it = m_layers.at( aLayerId ).requiredLayers.begin(),
            it_end = m_layers.at( aLayerId ).requiredLayers.end(); it != it_end; ++it )
    {
        // That is enough if just one layer is not enabled
        if( !m_layers.at( *it ).enabled )
            return false;
    }

    return true;
}


void VIEW::RecacheAllItems( bool aImmediately )
{
    BOX2I r;

    r.SetMaximum();

#ifdef __WXDEBUG__
    prof_counter totalRealTime;
    prof_start( &totalRealTime, false );
#endif /* __WXDEBUG__ */

    for( LayerMapIter i = m_layers.begin(); i != m_layers.end(); ++i )
    {
        VIEW_LAYER* l = &( ( *i ).second );

        if( IsCached( l->id ) )
        {
            m_gal->SetTarget( l->target );
            m_gal->SetLayerDepth( l->renderingOrder );
            recacheLayer visitor( this, m_gal, l->id, aImmediately );
            l->items->Query( r, visitor );
            l->isDirty = true;
        }
    }

#ifdef __WXDEBUG__
    prof_end( &totalRealTime );

    wxLogDebug( wxT( "RecacheAllItems::immediately: %u %.1f ms" ),
                aImmediately, (double) totalRealTime.value / 1000.0 );
#endif /* __WXDEBUG__ */
}


bool VIEW::IsTargetDirty( int aTarget ) const
{
    wxASSERT( aTarget < TARGETS_NUMBER );

    // Check the target status
    if( m_dirtyTargets[aTarget] )
        return true;

    // Check if any of layers belonging to the target is dirty
    BOOST_FOREACH( VIEW_LAYER* l, m_orderedLayers )
    {
        if( l->target == aTarget && l->isDirty )
            return true;
    }

    return false;
}
