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
#include <view/view_rtree.h>
#include <gal/definitions.h>
#include <gal/graphics_abstraction_layer.h>
#include <painter.h>

#ifdef __WXDEBUG__
#include <profile.h>
#endif /* __WXDEBUG__ */

using namespace KiGfx;

// Static constants
const unsigned int VIEW::VIEW_MAX_LAYERS = 64;
const int          VIEW::TOP_LAYER       = -1;

void VIEW::AddLayer( int aLayer, bool aDisplayOnly )
{
    if( m_layers.find( aLayer ) == m_layers.end() )
    {
        m_layers[aLayer] = VIEW_LAYER();
        m_layers[aLayer].id     = aLayer;
        m_layers[aLayer].items  = new VIEW_RTREE();
        m_layers[aLayer].renderingOrder = aLayer;
        m_layers[aLayer].enabled        = true;
        m_layers[aLayer].isDirty        = false;
        m_layers[aLayer].displayOnly    = aDisplayOnly;
    }

    sortLayers();
}


void VIEW::Add( VIEW_ITEM* aItem )
{
    int layers[VIEW_MAX_LAYERS], layers_count;

    aItem->ViewGetLayers( layers, layers_count );

    for( int i = 0; i < layers_count; i++ )
    {
        VIEW_LAYER* l = &m_layers[layers[i]];
        l->items->Insert( aItem );
        l->dirtyExtents.Merge( aItem->ViewBBox() );
    }

    if( m_dynamic )
        aItem->viewAssign( this );
}


void VIEW::Remove( VIEW_ITEM* aItem )
{
    if( m_dynamic )
        aItem->m_view = NULL;

// fixme: this is so sloooow!
    for( LayerMapIter i = m_layers.begin(); i != m_layers.end(); ++i )
    {
        VIEW_LAYER* l = & ( ( *i ).second );
        l->items->Remove( aItem );
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


VIEW::VIEW( bool aIsDynamic, bool aUseGroups ) :
    m_enableTopLayer( false ),
    m_scale ( 1.0 ),
    m_painter( NULL ),
    m_gal( NULL ),
    m_dynamic( aIsDynamic ),
    m_useGroups( aUseGroups )
{
    // By default there is no layer on the top
    m_topLayer.enabled = false;
}


VIEW::~VIEW()
{
    BOOST_FOREACH( LayerMap::value_type& l, m_layers )
    {
        delete l.second.items;
    }
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
    if( m_useGroups )
        clearGroupCache();

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
    wxASSERT_MSG( false, wxT( "This is not implemented" ) );
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
}


void VIEW::SetCenter( const VECTOR2D& aCenter )
{
    m_center = aCenter;
    m_gal->SetLookAtPoint( m_center );
    m_gal->ComputeWorldScreenMatrix();
}


void VIEW::SetLayerVisible( int aLayer, bool aVisible )
{
    m_layers[aLayer].enabled = aVisible;
}


void VIEW::sortLayers()
{
    int n = 0;

    m_orderedLayers.resize( m_layers.size() );

    for( LayerMapIter i = m_layers.begin(); i != m_layers.end(); ++i )
        m_orderedLayers[n++] = &i->second;

    sort( m_orderedLayers.begin(), m_orderedLayers.end(), compareRenderingOrder );
}


void VIEW::SetLayerOrder( int aLayer, int aRenderingOrder )
{
    m_layers[aLayer].renderingOrder = aRenderingOrder;
    sortLayers();
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

        gal->ChangeGroupColor( group, color );
    }

    int layer;
    PAINTER* painter;
    GAL* gal;
};


void VIEW::UpdateLayerColor( int aLayer )
{
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
        updateItemsColor visitor( l->id, m_painter, m_gal );

        l->items->Query( r, visitor );
    }
}


void VIEW::SetTopLayer( int aLayer )
{
    // Restore previous order
    if( m_topLayer.enabled )
        m_layers[m_topLayer.id].renderingOrder = m_topLayer.renderingOrder;

    if( aLayer >= 0 && aLayer < VIEW_MAX_LAYERS )
    {
        // Save settings, so it can be restored later
        m_topLayer.renderingOrder = m_layers[aLayer].renderingOrder;
        m_topLayer.id = m_layers[aLayer].id;

        // Apply new settings only if the option is enabled
        if( m_enableTopLayer )
            m_layers[aLayer].renderingOrder = TOP_LAYER;

        // Set the flag saying that settings stored in m_topLayer are valid
        m_topLayer.enabled = true;
    }
    else
    {
        // There are no valid settings in m_topLayer
        m_topLayer.enabled = false;
    }

    sortLayers();
}


void VIEW::EnableTopLayer( bool aEnable )
{
    if( aEnable == m_enableTopLayer ) return;

    // Use stored settings only if applicable
    // (topLayer.enabled == false means there are no valid settings stored)
    if( m_topLayer.enabled )
    {
        if( aEnable )
        {
            m_layers[m_topLayer.id].renderingOrder = TOP_LAYER;
        }
        else
        {
            m_layers[m_topLayer.id].renderingOrder = m_topLayer.renderingOrder;
        }
    }

    m_enableTopLayer = aEnable;
}


struct VIEW::drawItem
{
    drawItem( VIEW* aView, int aCurrentLayer ) :
        currentLayer( aCurrentLayer ), view( aView )
    {
    }

    void operator()( VIEW_ITEM* aItem )
    {
        GAL*     gal = view->GetGAL();

        if( view->m_useGroups )
        {
            int group = aItem->getGroup( currentLayer );

            if( group >= 0 && aItem->ViewIsVisible() )
            {
                gal->DrawGroup( group );
            }
            else
            {
                group = gal->BeginGroup();
                aItem->setGroup( currentLayer, group );
                view->m_painter->Draw( aItem, currentLayer );
                gal->EndGroup();
            }
        }
        else if( aItem->ViewIsVisible() )
        {
            view->m_painter->Draw( aItem, currentLayer );
        }
    }

    int      currentLayer;
    VIEW*    view;
};


void VIEW::redrawRect( const BOX2I& aRect )
{
    BOOST_FOREACH( VIEW_LAYER* l, m_orderedLayers )
    {
        if( l->enabled )
        {
            drawItem drawFunc( this, l->id );

            if( !m_useGroups )
                m_gal->SetLayerDepth( static_cast<double>( l->renderingOrder ) );
            l->items->Query( aRect, drawFunc );
            l->isDirty = false;
        }
    }
}


struct VIEW::unlinkItem
{
    void operator()( VIEW_ITEM* aItem )
    {
        aItem->m_view = NULL;
    }
};


struct VIEW::recacheItem
{
    recacheItem( VIEW* aView, GAL* aGal, int aLayer, bool aImmediately ) :
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

    if( m_useGroups )
    {
        m_gal->ClearCache();
    }
}


void VIEW::Redraw()
{
    VECTOR2D screenSize = m_gal->GetScreenPixelSize();
    BOX2I    rect( ToWorld( VECTOR2D( 0, 0 ) ),
                   ToWorld( screenSize ) - ToWorld( VECTOR2D( 0, 0 ) ) );

    rect.Normalize();
    redrawRect( rect );
}


VECTOR2D VIEW::GetScreenPixelSize() const
{
    return m_gal->GetScreenPixelSize();
}


void VIEW::invalidateItem( VIEW_ITEM* aItem, int aUpdateFlags )
{
    int layer_indices[VIEW_MAX_LAYERS], layer_count;

    aItem->ViewGetLayers( layer_indices, layer_count );

    for( int i = 0; i < layer_count; i++ )
    {
        VIEW_LAYER* l = &m_layers[layer_indices[i]];

        l->dirtyExtents =
            l->isDirty ? aItem->ViewBBox() : l->dirtyExtents.Merge( aItem->ViewBBox() );

        if( aUpdateFlags & VIEW_ITEM::GEOMETRY )
        {
            l->items->Remove( aItem );
            l->items->Insert( aItem );    /* reinsert */

            if( m_useGroups )
                aItem->deleteGroups();
        }
    }

    if( m_useGroups && aItem->storesGroups() )
    {
        std::vector<int> groups = aItem->getAllGroups();
        for(std::vector<int>::iterator i = groups.begin(); i != groups.end(); i++ )
        {
            m_gal->DeleteGroup( *i );
        }

        aItem->deleteGroups();
    }
}


struct VIEW::clearItemCache
{
    clearItemCache( VIEW* aView ) :
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
    if( !m_useGroups )
            return;

    BOX2I r;

    r.SetMaximum();
    clearItemCache visitor( this );

    for( LayerMapIter i = m_layers.begin(); i != m_layers.end(); ++i )
    {
        VIEW_LAYER* l = & ( ( *i ).second );
        l->items->Query( r, visitor );
    }
}


void VIEW::RecacheAllItems( bool aImmediately )
{
    if( !m_useGroups )
        return;

    BOX2I r;

    r.SetMaximum();

#ifdef __WXDEBUG__
    wxLogDebug( wxT( "RecacheAllItems::immediately: %u" ), aImmediately );

    prof_counter totalRealTime;
    prof_start( &totalRealTime, false );
#endif /* __WXDEBUG__ */

    for( LayerMapIter i = m_layers.begin(); i != m_layers.end(); ++i )
    {
        VIEW_LAYER* l = & ( ( *i ).second );
        m_gal->SetLayerDepth( (double) l->renderingOrder );
        recacheItem visitor( this, m_gal, l->id, aImmediately );
        l->items->Query( r, visitor );
    }

#ifdef __WXDEBUG__
    prof_end( &totalRealTime );

    wxLogDebug( wxT( "RecacheAllItems::%.1f ms" ), (double) totalRealTime.value / 1000.0 );
#endif /* __WXDEBUG__ */
}
