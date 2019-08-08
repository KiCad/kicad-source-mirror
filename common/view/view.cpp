/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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


#include <base_struct.h>
#include <layers_id_colors_and_visibility.h>

#include <view/view.h>
#include <view/view_group.h>
#include <view/view_item.h>
#include <view/view_rtree.h>
#include <view/view_overlay.h>

#include <gal/definitions.h>
#include <gal/graphics_abstraction_layer.h>
#include <painter.h>

#ifdef __WXDEBUG__
#include <profile.h>
#endif /* __WXDEBUG__  */

namespace KIGFX {

class VIEW;

class VIEW_ITEM_DATA
{
public:
    VIEW_ITEM_DATA() :
        m_view( nullptr ),
        m_flags( KIGFX::VISIBLE ),
        m_requiredUpdate( KIGFX::NONE ),
        m_drawPriority( 0 ),
        m_groups( nullptr ),
        m_groupsSize( 0 ) {}

    ~VIEW_ITEM_DATA()
    {
        deleteGroups();
    }

    int getFlags() const
    {
        return m_flags;
    }

private:
    friend class VIEW;

    /**
     * Function getLayers()
     * Returns layer numbers used by the item.
     *
     * @param aLayers[]: output layer index array
     * @param aCount: number of layer indices in aLayers[]
     */
    void getLayers( int* aLayers, int& aCount ) const
    {
        int* layersPtr = aLayers;

        for( auto layer : m_layers )
            *layersPtr++ = layer;

        aCount = m_layers.size();
    }

    VIEW*   m_view;             ///< Current dynamic view the item is assigned to.
    int     m_flags;            ///< Visibility flags
    int     m_requiredUpdate;   ///< Flag required for updating
    int     m_drawPriority;     ///< Order to draw this item in a layer, lowest first

    ///> Helper for storing cached items group ids
    typedef std::pair<int, int> GroupPair;

    ///> Indexes of cached GAL display lists corresponding to the item (for every layer it occupies).
    ///> (in the std::pair "first" stores layer number, "second" stores group id).
    GroupPair* m_groups;
    int        m_groupsSize;

    /**
     * Function getGroup()
     * Returns number of the group id for the given layer, or -1 in case it was not cached before.
     *
     * @param aLayer is the layer number for which group id is queried.
     * @return group id or -1 in case there is no group id (ie. item is not cached).
     */
    int getGroup( int aLayer ) const
    {
        for( int i = 0; i < m_groupsSize; ++i )
        {
            if( m_groups[i].first == aLayer )
                return m_groups[i].second;
        }

        return -1;
    }

    /**
     * Function getAllGroups()
     * Returns all group ids for the item (collected from all layers the item occupies).
     *
     * @return vector of group ids.
     */
    std::vector<int> getAllGroups() const
    {
        std::vector<int> groups( m_groupsSize );

        for( int i = 0; i < m_groupsSize; ++i )
        {
            groups[i] = m_groups[i].second;
        }

        return groups;
    }

    /**
     * Function setGroup()
     * Sets a group id for the item and the layer combination.
     *
     * @param aLayer is the layer numbe.
     * @param aGroup is the group id.
     */
    void setGroup( int aLayer, int aGroup )
    {
        // Look if there is already an entry for the layer
        for( int i = 0; i < m_groupsSize; ++i )
        {
            if( m_groups[i].first == aLayer )
            {
                m_groups[i].second = aGroup;
                return;
            }
        }

        // If there was no entry for the given layer - create one
        GroupPair* newGroups = new GroupPair[m_groupsSize + 1];

        if( m_groupsSize > 0 )
        {
            std::copy( m_groups, m_groups + m_groupsSize, newGroups );
            delete[] m_groups;
        }

        m_groups = newGroups;
        newGroups[m_groupsSize++] = GroupPair( aLayer, aGroup );
    }


    /**
     * Function deleteGroups()
     * Removes all of the stored group ids. Forces recaching of the item.
     */
    void deleteGroups()
    {
        delete[] m_groups;
        m_groups = nullptr;
        m_groupsSize = 0;
    }


    /**
     * Function storesGroups()
     * Returns information if the item uses at least one group id (ie. if it is cached at all).
     *
     * @returns true in case it is cached at least for one layer.
     */
    inline bool storesGroups() const
    {
        return m_groupsSize > 0;
    }


    /**
     * Reorders the stored groups (to facilitate reordering of layers)
     * @see VIEW::ReorderLayerData
     *
     * @param aReorderMap is the mapping of old to new layer ids
     */
    void reorderGroups( std::unordered_map<int, int> aReorderMap )
    {
        for( int i = 0; i < m_groupsSize; ++i )
        {
            int orig_layer = m_groups[i].first;
            int new_layer = orig_layer;

            try
            {
                new_layer = aReorderMap.at( orig_layer );
            }
            catch( const std::out_of_range& ) {}

            m_groups[i].first = new_layer;
        }
    }


    /// Stores layer numbers used by the item.
    std::vector<int> m_layers;

    /**
     * Function saveLayers()
     * Saves layers used by the item.
     *
     * @param aLayers is an array containing layer numbers to be saved.
     * @param aCount is the size of the array.
     */
    void saveLayers( int* aLayers, int aCount )
    {
        m_layers.clear();

        for( int i = 0; i < aCount; ++i )
        {
            // this fires on some eagle board after EAGLE_PLUGIN::Load()
            wxASSERT( unsigned( aLayers[i] ) <= unsigned( VIEW::VIEW_MAX_LAYERS ) );

            m_layers.push_back( aLayers[i] );
        }
    }

    /**
     * Function viewRequiredUpdate()
     * Returns current update flag for an item.
     */
    int requiredUpdate() const
    {
        return m_requiredUpdate;
    }

    /**
     * Function clearUpdateFlags()
     * Marks an item as already updated, so it is not going to be redrawn.
     */
    void clearUpdateFlags()
    {
        m_requiredUpdate = NONE;
    }

    /**
     * Function isRenderable()
     * Returns if the item should be drawn or not.
     */
    bool isRenderable() const
    {
        return m_flags == VISIBLE;
    }
};


void VIEW::OnDestroy( VIEW_ITEM* aItem )
{
    auto data = aItem->viewPrivData();

    if( !data )
        return;

    if( data->m_view )
        data->m_view->VIEW::Remove( aItem );

    delete data;
    aItem->ClearViewPrivData();
}


VIEW::VIEW( bool aIsDynamic ) :
    m_enableOrderModifier( true ),
    m_scale( 4.0 ),
    m_minScale( 0.2 ), m_maxScale( 25000.0 ),
    m_mirrorX( false ), m_mirrorY( false ),
    m_painter( NULL ),
    m_gal( NULL ),
    m_dynamic( aIsDynamic ),
    m_useDrawPriority( false ),
    m_nextDrawPriority( 0 ),
    m_reverseDrawOrder( false )
{
    // Set m_boundary to define the max area size. The default area size
    // is defined here as the max value of a int.
    // this is a default value acceptable for Pcbnew and Gerbview, but too large for Eeschema.
    // So in eeschema a call to SetBoundary() with a smaller value will be needed.
    typedef std::numeric_limits<int> coord_limits;
    double pos = coord_limits::lowest() / 2 + coord_limits::epsilon();
    double size = coord_limits::max() - coord_limits::epsilon();
    m_boundary.SetOrigin( pos, pos );
    m_boundary.SetSize( size, size );
    SetPrintMode( 0 );

    m_allItems.reset( new std::vector<VIEW_ITEM*> );
    m_allItems->reserve( 32768 );

    // Redraw everything at the beginning
    MarkDirty();

    // View uses layers to display EDA_ITEMs (item may be displayed on several layers, for example
    // pad may be shown on pad, pad hole and solder paste layers). There are usual copper layers
    // (eg. F.Cu, B.Cu, internal and so on) and layers for displaying objects such as texts,
    // silkscreen, pads, vias, etc.
    for( int i = 0; i < VIEW_MAX_LAYERS; i++ )
        AddLayer( i );

    sortLayers();
}


VIEW::~VIEW()
{
}


void VIEW::AddLayer( int aLayer, bool aDisplayOnly )
{
    if( m_layers.find( aLayer ) == m_layers.end() )
    {
        m_layers[aLayer]                = VIEW_LAYER();
        m_layers[aLayer].items.reset( new VIEW_RTREE() );
        m_layers[aLayer].id             = aLayer;
        m_layers[aLayer].renderingOrder = aLayer;
        m_layers[aLayer].visible        = true;
        m_layers[aLayer].displayOnly    = aDisplayOnly;
        m_layers[aLayer].target         = TARGET_CACHED;
    }
}


void VIEW::Add( VIEW_ITEM* aItem, int aDrawPriority )
{
    int layers[VIEW_MAX_LAYERS], layers_count;

    if( aDrawPriority < 0 )
        aDrawPriority = m_nextDrawPriority++;

    if( !aItem->m_viewPrivData )
        aItem->m_viewPrivData = new VIEW_ITEM_DATA;

    aItem->m_viewPrivData->m_view = this;
    aItem->m_viewPrivData->m_drawPriority = aDrawPriority;

    aItem->ViewGetLayers( layers, layers_count );
    aItem->viewPrivData()->saveLayers( layers, layers_count );

    m_allItems->push_back( aItem );

    for( int i = 0; i < layers_count; ++i )
    {
        VIEW_LAYER& l = m_layers[layers[i]];
        l.items->Insert( aItem );
        MarkTargetDirty( l.target );
    }

    SetVisible( aItem, true );
    Update( aItem, KIGFX::INITIAL_ADD );
}


void VIEW::Remove( VIEW_ITEM* aItem )
{
    if( !aItem )
        return;

    auto viewData = aItem->viewPrivData();

    if( !viewData )
        return;

    wxCHECK( viewData->m_view == this, /*void*/ );
    auto item = std::find( m_allItems->begin(), m_allItems->end(), aItem );

    if( item != m_allItems->end() )
    {
        m_allItems->erase( item );
        viewData->clearUpdateFlags();
    }

    int layers[VIEW::VIEW_MAX_LAYERS], layers_count;
    viewData->getLayers( layers, layers_count );

    for( int i = 0; i < layers_count; ++i )
    {
        VIEW_LAYER& l = m_layers[layers[i]];
        l.items->Remove( aItem );
        MarkTargetDirty( l.target );

        // Clear the GAL cache
        int prevGroup = viewData->getGroup( layers[i] );

        if( prevGroup >= 0 )
            m_gal->DeleteGroup( prevGroup );
    }

    viewData->deleteGroups();
    viewData->m_view = nullptr;
}


void VIEW::SetRequired( int aLayerId, int aRequiredId, bool aRequired )
{
    wxCHECK( (unsigned) aLayerId < m_layers.size(), /*void*/ );
    wxCHECK( (unsigned) aRequiredId < m_layers.size(), /*void*/ );

    if( aRequired )
        m_layers[aLayerId].requiredLayers.insert( aRequiredId );
    else
        m_layers[aLayerId].requiredLayers.erase( aRequired );
}


// stupid C++... python lambda would do this in one line
template <class Container>
struct queryVisitor
{
    typedef typename Container::value_type item_type;

    queryVisitor( Container& aCont, int aLayer ) :
        m_cont( aCont ), m_layer( aLayer )
    {
    }

    bool operator()( VIEW_ITEM* aItem )
    {
        if( aItem->viewPrivData()->getFlags() & VISIBLE )
            m_cont.push_back( VIEW::LAYER_ITEM_PAIR( aItem, m_layer ) );

        return true;
    }

    Container&  m_cont;
    int         m_layer;
};


int VIEW::Query( const BOX2I& aRect, std::vector<LAYER_ITEM_PAIR>& aResult ) const
{
    if( m_orderedLayers.empty() )
        return 0;

    std::vector<VIEW_LAYER*>::const_reverse_iterator i;

    // execute queries in reverse direction, so that items that are on the top of
    // the rendering stack are returned first.
    for( i = m_orderedLayers.rbegin(); i != m_orderedLayers.rend(); ++i )
    {
        // ignore layers that do not contain actual items (i.e. the selection box, menus, floats)
        if( ( *i )->displayOnly || !( *i )->visible )
            continue;

        queryVisitor<std::vector<LAYER_ITEM_PAIR> > visitor( aResult, ( *i )->id );
        ( *i )->items->Query( aRect, visitor );
    }

    return aResult.size();
}


VECTOR2D VIEW::ToWorld( const VECTOR2D& aCoord, bool aAbsolute ) const
{
    const MATRIX3x3D& matrix = m_gal->GetScreenWorldMatrix();

    if( aAbsolute )
        return VECTOR2D( matrix * aCoord );
    else
        return VECTOR2D( matrix.GetScale().x * aCoord.x, matrix.GetScale().y * aCoord.y );
}


double VIEW::ToWorld( double aSize ) const
{
    const MATRIX3x3D& matrix = m_gal->GetScreenWorldMatrix();

    return fabs( matrix.GetScale().x * aSize );
}


VECTOR2D VIEW::ToScreen( const VECTOR2D& aCoord, bool aAbsolute ) const
{
    const MATRIX3x3D& matrix = m_gal->GetWorldScreenMatrix();

    if( aAbsolute )
        return VECTOR2D( matrix * aCoord );
    else
        return VECTOR2D( matrix.GetScale().x * aCoord.x, matrix.GetScale().y * aCoord.y );
}


double VIEW::ToScreen( double aSize ) const
{
    const MATRIX3x3D& matrix = m_gal->GetWorldScreenMatrix();

    return matrix.GetScale().x * aSize;
}


void VIEW::CopySettings( const VIEW* aOtherView )
{
    wxASSERT_MSG( false, wxT( "This is not implemented" ) );
}


void VIEW::SetGAL( GAL* aGal )
{
    bool recacheGroups = ( m_gal != nullptr );    // recache groups only if GAL is reassigned
    m_gal = aGal;

    // clear group numbers, so everything is going to be recached
    if( recacheGroups )
        clearGroupCache();

    // every target has to be refreshed
    MarkDirty();

    // force the new GAL to display the current viewport.
    SetCenter( m_center );
    SetScale( m_scale );
    SetMirror( m_mirrorX, m_mirrorY );
}


BOX2D VIEW::GetViewport() const
{
    BOX2D    rect;
    VECTOR2D screenSize = m_gal->GetScreenPixelSize();

    rect.SetOrigin( ToWorld( VECTOR2D( 0, 0 ) ) );
    rect.SetEnd( ToWorld( screenSize ) );

    return rect.Normalize();
}


void VIEW::SetViewport( const BOX2D& aViewport )
{
    VECTOR2D ssize = ToWorld( m_gal->GetScreenPixelSize(), false );

    wxCHECK( ssize.x > 0 && ssize.y > 0, /*void*/ );

    VECTOR2D centre = aViewport.Centre();
    VECTOR2D vsize  = aViewport.GetSize();
    double   zoom   = 1.0 / std::max( fabs( vsize.x / ssize.x ), fabs( vsize.y / ssize.y ) );

    SetCenter( centre );
    SetScale( GetScale() * zoom );
}


void VIEW::SetMirror( bool aMirrorX, bool aMirrorY )
{
    wxASSERT_MSG( !aMirrorY, _( "Mirroring for Y axis is not supported yet" ) );

    m_mirrorX = aMirrorX;
    m_mirrorY = aMirrorY;
    m_gal->SetFlip( aMirrorX, aMirrorY );

    // Redraw everything
    MarkDirty();
}


void VIEW::SetScale( double aScale, VECTOR2D aAnchor )
{
    if( aAnchor == VECTOR2D( 0, 0 ) )
        aAnchor = m_center;

    VECTOR2D a = ToScreen( aAnchor );

    if( aScale < m_minScale )
        m_scale = m_minScale;
    else if( aScale > m_maxScale )
        m_scale = m_maxScale;
    else
        m_scale = aScale;

    m_gal->SetZoomFactor( m_scale );
    m_gal->ComputeWorldScreenMatrix();

    VECTOR2D delta = ToWorld( a ) - aAnchor;

    SetCenter( m_center - delta );

    // Redraw everything after the viewport has changed
    MarkDirty();
}


void VIEW::SetCenter( const VECTOR2D& aCenter )
{
    m_center = aCenter;

    if( !m_boundary.Contains( aCenter ) )
    {
        if( m_center.x < m_boundary.GetLeft() )
            m_center.x = m_boundary.GetLeft();
        else if( aCenter.x > m_boundary.GetRight() )
            m_center.x = m_boundary.GetRight();

        if( m_center.y < m_boundary.GetTop() )
            m_center.y = m_boundary.GetTop();
        else if( m_center.y > m_boundary.GetBottom() )
            m_center.y = m_boundary.GetBottom();
    }

    m_gal->SetLookAtPoint( m_center );
    m_gal->ComputeWorldScreenMatrix();

    // Redraw everything after the viewport has changed
    MarkDirty();
}


void VIEW::SetCenter( VECTOR2D aCenter, const BOX2D& occultingScreenRect )
{
    BOX2D screenRect( VECTOR2D( 0, 0 ), m_gal->GetScreenPixelSize() );

    if( !screenRect.Intersects( occultingScreenRect ) )
    {
        SetCenter( aCenter );
        return;
    }

    BOX2D  occultedRect  = screenRect.Intersect( occultingScreenRect );
    double topExposed    = occultedRect.GetTop() - screenRect.GetTop();
    double bottomExposed = screenRect.GetBottom() - occultedRect.GetBottom();
    double leftExposed   = occultedRect.GetLeft() - screenRect.GetLeft();
    double rightExposed  = screenRect.GetRight() - occultedRect.GetRight();

    if( std::max( topExposed, bottomExposed ) > std::max( leftExposed, rightExposed ) )
    {
        if( topExposed > bottomExposed )
            aCenter.y += ToWorld( screenRect.GetHeight() / 2 - topExposed / 2 );
        else
            aCenter.y -= ToWorld( screenRect.GetHeight() / 2 - bottomExposed / 2 );
    }
    else
    {
        if( leftExposed > rightExposed )
            aCenter.x += ToWorld( screenRect.GetWidth() / 2 - leftExposed / 2 );
        else
            aCenter.x -= ToWorld( screenRect.GetWidth() / 2 - rightExposed / 2 );
    }

    SetCenter( aCenter );
}


void VIEW::SetLayerOrder( int aLayer, int aRenderingOrder )
{
    m_layers[aLayer].renderingOrder = aRenderingOrder;

    sortLayers();
}


int VIEW::GetLayerOrder( int aLayer ) const
{
    return m_layers.at( aLayer ).renderingOrder;
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


void VIEW::ReorderLayerData( std::unordered_map<int, int> aReorderMap )
{
    LAYER_MAP new_map;

    for( const auto& it : m_layers )
    {
        int orig_idx = it.first;
        VIEW_LAYER layer = it.second;
        int new_idx;

        try
        {
            new_idx = aReorderMap.at( orig_idx );
        }
        catch( const std::out_of_range& )
        {
            new_idx = orig_idx;
        }

        layer.id = new_idx;
        new_map[new_idx] = layer;
    }

    m_layers = new_map;

    for( VIEW_ITEM* item : *m_allItems )
    {
        auto viewData = item->viewPrivData();

        if( !viewData )
            continue;

        int layers[VIEW::VIEW_MAX_LAYERS], layers_count;

        item->ViewGetLayers( layers, layers_count );
        viewData->saveLayers( layers, layers_count );

        viewData->reorderGroups( aReorderMap );

        viewData->m_requiredUpdate |= COLOR;
    }

    UpdateItems();
}


struct VIEW::updateItemsColor
{
    updateItemsColor( int aLayer, PAINTER* aPainter, GAL* aGal ) :
        layer( aLayer ), painter( aPainter ), gal( aGal )
    {
    }

    bool operator()( VIEW_ITEM* aItem )
    {
        // Obtain the color that should be used for coloring the item
        const COLOR4D color = painter->GetSettings()->GetColor( aItem, layer );
        int group = aItem->viewPrivData()->getGroup( layer );

        if( group >= 0 )
            gal->ChangeGroupColor( group, color );

        return true;
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

    if( m_gal->IsVisible() )
    {
        GAL_UPDATE_CONTEXT ctx( m_gal );

        updateItemsColor visitor( aLayer, m_painter, m_gal );
        m_layers[aLayer].items->Query( r, visitor );
        MarkTargetDirty( m_layers[aLayer].target );
    }
}


void VIEW::UpdateAllLayersColor()
{
    if( m_gal->IsVisible() )
    {
        GAL_UPDATE_CONTEXT ctx( m_gal );

        for( VIEW_ITEM* item : *m_allItems )
        {
            auto viewData = item->viewPrivData();

            if( !viewData )
                continue;

            int layers[VIEW::VIEW_MAX_LAYERS], layers_count;
            viewData->getLayers( layers, layers_count );

            for( int i = 0; i < layers_count; ++i )
            {
                const COLOR4D color = m_painter->GetSettings()->GetColor( item, layers[i] );
                int group = viewData->getGroup( layers[i] );

                if( group >= 0 )
                    m_gal->ChangeGroupColor( group, color );
            }
        }
    }

    MarkDirty();
}


struct VIEW::changeItemsDepth
{
    changeItemsDepth( int aLayer, int aDepth, GAL* aGal ) :
        layer( aLayer ), depth( aDepth ), gal( aGal )
    {
    }

    bool operator()( VIEW_ITEM* aItem )
    {
        int group = aItem->viewPrivData()->getGroup( layer );

        if( group >= 0 )
            gal->ChangeGroupDepth( group, depth );

        return true;
    }

    int layer, depth;
    GAL* gal;
};


int VIEW::GetTopLayer() const
{
    if( m_topLayers.size() == 0 )
        return 0;

    return *m_topLayers.begin();
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
    if( aEnable == m_enableOrderModifier )
        return;

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

    if( m_gal->IsVisible() )
    {
        GAL_UPDATE_CONTEXT ctx( m_gal );

        for( VIEW_ITEM* item : *m_allItems )
        {
            auto viewData = item->viewPrivData();

            if( !viewData )
                continue;

            int layers[VIEW::VIEW_MAX_LAYERS], layers_count;
            viewData->getLayers( layers, layers_count );

            for( int i = 0; i < layers_count; ++i )
            {
                int group = viewData->getGroup( layers[i] );

                if( group >= 0 )
                    m_gal->ChangeGroupDepth( group, m_layers[layers[i]].renderingOrder );
            }
        }
    }

    MarkDirty();
}


struct VIEW::drawItem
{
    drawItem( VIEW* aView, int aLayer, bool aUseDrawPriority, bool aReverseDrawOrder ) :
        view( aView ), layer( aLayer ),
        useDrawPriority( aUseDrawPriority ),
        reverseDrawOrder( aReverseDrawOrder )
    {
    }

    bool operator()( VIEW_ITEM* aItem )
    {
        wxCHECK( aItem->viewPrivData(), false );

        // Conditions that have to be fulfilled for an item to be drawn
        bool drawCondition = aItem->viewPrivData()->isRenderable() &&
                             aItem->ViewGetLOD( layer, view ) < view->m_scale;
        if( !drawCondition )
            return true;

        if( useDrawPriority )
            drawItems.push_back( aItem );
        else
            view->draw( aItem, layer );

        return true;
    }

    void deferredDraw()
    {
        if( reverseDrawOrder )
            std::sort( drawItems.begin(), drawItems.end(),
                       []( VIEW_ITEM* a, VIEW_ITEM* b ) -> bool {
                           return b->viewPrivData()->m_drawPriority < a->viewPrivData()->m_drawPriority;
                       });
        else
            std::sort( drawItems.begin(), drawItems.end(),
                       []( VIEW_ITEM* a, VIEW_ITEM* b ) -> bool {
                           return a->viewPrivData()->m_drawPriority < b->viewPrivData()->m_drawPriority;
                       });

        for( auto item : drawItems )
            view->draw( item, layer );
    }

    VIEW* view;
    int layer, layers[VIEW_MAX_LAYERS];
    bool useDrawPriority, reverseDrawOrder;
    std::vector<VIEW_ITEM*> drawItems;
};


void VIEW::redrawRect( const BOX2I& aRect )
{
    for( VIEW_LAYER* l : m_orderedLayers )
    {
        if( l->visible && IsTargetDirty( l->target ) && areRequiredLayersEnabled( l->id ) )
        {
            drawItem drawFunc( this, l->id, m_useDrawPriority, m_reverseDrawOrder );

            m_gal->SetTarget( l->target );
            m_gal->SetLayerDepth( l->renderingOrder );
            l->items->Query( aRect, drawFunc );

            if( m_useDrawPriority )
                drawFunc.deferredDraw();
        }
    }
}


void VIEW::draw( VIEW_ITEM* aItem, int aLayer, bool aImmediate )
{
    auto viewData = aItem->viewPrivData();

    if( !viewData )
        return;

    if( IsCached( aLayer ) && !aImmediate )
    {
        // Draw using cached information or create one
        int group = viewData->getGroup( aLayer );

        if( group >= 0 )
            m_gal->DrawGroup( group );
        else
            Update( aItem );
    }
    else
    {
        // Immediate mode
        if( !m_painter->Draw( aItem, aLayer ) )
            aItem->ViewDraw( aLayer, this );  // Alternative drawing method
    }
}


void VIEW::draw( VIEW_ITEM* aItem, bool aImmediate )
{
    int layers[VIEW_MAX_LAYERS], layers_count;

    aItem->ViewGetLayers( layers, layers_count );

    // Sorting is needed for drawing order dependent GALs (like Cairo)
    SortLayers( layers, layers_count );

    for( int i = 0; i < layers_count; ++i )
    {
        m_gal->SetLayerDepth( m_layers.at( layers[i] ).renderingOrder );
        draw( aItem, layers[i], aImmediate );
    }
}


void VIEW::draw( VIEW_GROUP* aGroup, bool aImmediate )
{
    for( unsigned int i = 0; i < aGroup->GetSize(); i++)
        draw( aGroup->GetItem(i), aImmediate );
}


struct VIEW::recacheItem
{
    recacheItem( VIEW* aView, GAL* aGal, int aLayer ) :
        view( aView ), gal( aGal ), layer( aLayer )
    {
    }

    bool operator()( VIEW_ITEM* aItem )
    {
        auto viewData = aItem->viewPrivData();

        if( !viewData )
            return false;

        // Remove previously cached group
        int group = viewData->getGroup( layer );

        if( group >= 0 )
            gal->DeleteGroup( group );

        viewData->setGroup( layer, -1 );
        view->Update( aItem );

        return true;
    }

    VIEW* view;
    GAL* gal;
    int layer;
};


void VIEW::Clear()
{
    BOX2I r;
    r.SetMaximum();
    m_allItems->clear();

    for( LAYER_MAP_ITER i = m_layers.begin(); i != m_layers.end(); ++i )
        i->second.items->RemoveAll();

    m_nextDrawPriority = 0;

    m_gal->ClearCache();
}


void VIEW::ClearTargets()
{
    if( IsTargetDirty( TARGET_CACHED ) || IsTargetDirty( TARGET_NONCACHED ) )
    {
        // TARGET_CACHED and TARGET_NONCACHED have to be redrawn together, as they contain
        // layers that rely on each other (eg. netnames are noncached, but tracks - are cached)
        m_gal->ClearTarget( TARGET_NONCACHED );
        m_gal->ClearTarget( TARGET_CACHED );

        MarkDirty();
    }

    if( IsTargetDirty( TARGET_OVERLAY ) )
    {
        m_gal->ClearTarget( TARGET_OVERLAY );
    }
}


void VIEW::Redraw()
{
#ifdef __WXDEBUG__
    PROF_COUNTER totalRealTime;
#endif /* __WXDEBUG__ */

    VECTOR2D screenSize = m_gal->GetScreenPixelSize();
    BOX2D    rect( ToWorld( VECTOR2D( 0, 0 ) ),
                   ToWorld( screenSize ) - ToWorld( VECTOR2D( 0, 0 ) ) );

    rect.Normalize();
    BOX2I recti( rect.GetPosition(), rect.GetSize() );

    // The view rtree uses integer positions.  Large screens can overflow
    // this size so in this case, simply set the rectangle to the full rtree
    if( rect.GetWidth() > std::numeric_limits<int>::max() ||
            rect.GetHeight() > std::numeric_limits<int>::max() )
        recti.SetMaximum();

    redrawRect( recti );
    // All targets were redrawn, so nothing is dirty
    markTargetClean( TARGET_CACHED );
    markTargetClean( TARGET_NONCACHED );
    markTargetClean( TARGET_OVERLAY );

#ifdef __WXDEBUG__
    totalRealTime.Stop();
    wxLogTrace( "GAL_PROFILE", "VIEW::Redraw(): %.1f ms", totalRealTime.msecs() );
#endif /* __WXDEBUG__ */
}


const VECTOR2I& VIEW::GetScreenPixelSize() const
{
    return m_gal->GetScreenPixelSize();
}


struct VIEW::clearLayerCache
{
    clearLayerCache( VIEW* aView ) :
        view( aView )
    {
    }

    bool operator()( VIEW_ITEM* aItem )
    {
        aItem->viewPrivData()->deleteGroups();

        return true;
    }

    VIEW* view;
};


void VIEW::clearGroupCache()
{
    BOX2I r;

    r.SetMaximum();
    clearLayerCache visitor( this );

    for( LAYER_MAP_ITER i = m_layers.begin(); i != m_layers.end(); ++i )
    {
        VIEW_LAYER* l = &( ( *i ).second );
        l->items->Query( r, visitor );
    }
}


void VIEW::invalidateItem( VIEW_ITEM* aItem, int aUpdateFlags )
{
    if( aUpdateFlags & INITIAL_ADD )
    {
        // Don't update layers or bbox, since it was done in VIEW::Add()
        // Now that we have initialized, set flags to ALL for the code below
        aUpdateFlags = ALL;
    }
    else
    {
        // updateLayers updates geometry too, so we do not have to update both of them at the same time
        if( aUpdateFlags & LAYERS )
        {
            updateLayers( aItem );
        }
        else if( aUpdateFlags & GEOMETRY )
        {
            updateBbox( aItem );
        }
    }

    int layers[VIEW_MAX_LAYERS], layers_count;
    aItem->ViewGetLayers( layers, layers_count );

    // Iterate through layers used by the item and recache it immediately
    for( int i = 0; i < layers_count; ++i )
    {
        int layerId = layers[i];

        if( IsCached( layerId ) )
        {
            if( aUpdateFlags & ( GEOMETRY | LAYERS | REPAINT ) )
                updateItemGeometry( aItem, layerId );
            else if( aUpdateFlags & COLOR )
                updateItemColor( aItem, layerId );
        }

        // Mark those layers as dirty, so the VIEW will be refreshed
        MarkTargetDirty( m_layers[layerId].target );
    }

    aItem->viewPrivData()->clearUpdateFlags();
}


void VIEW::sortLayers()
{
    int n = 0;

    m_orderedLayers.resize( m_layers.size() );

    for( LAYER_MAP_ITER i = m_layers.begin(); i != m_layers.end(); ++i )
        m_orderedLayers[n++] = &i->second;

    sort( m_orderedLayers.begin(), m_orderedLayers.end(), compareRenderingOrder );

    MarkDirty();
}


void VIEW::updateItemColor( VIEW_ITEM* aItem, int aLayer )
{
    auto viewData = aItem->viewPrivData();
    wxCHECK( (unsigned) aLayer < m_layers.size(), /*void*/ );
    wxCHECK( IsCached( aLayer ), /*void*/ );

    if( !viewData )
        return;

    // Obtain the color that should be used for coloring the item on the specific layerId
    const COLOR4D color = m_painter->GetSettings()->GetColor( aItem, aLayer );
    int group = viewData->getGroup( aLayer );

    // Change the color, only if it has group assigned
    if( group >= 0 )
        m_gal->ChangeGroupColor( group, color );
}


void VIEW::updateItemGeometry( VIEW_ITEM* aItem, int aLayer )
{
    auto viewData = aItem->viewPrivData();
    wxCHECK( (unsigned) aLayer < m_layers.size(), /*void*/ );
    wxCHECK( IsCached( aLayer ), /*void*/ );

    if( !viewData )
        return;

    VIEW_LAYER& l = m_layers.at( aLayer );

    m_gal->SetTarget( l.target );
    m_gal->SetLayerDepth( l.renderingOrder );

    // Redraw the item from scratch
    int group = viewData->getGroup( aLayer );

    if( group >= 0 )
        m_gal->DeleteGroup( group );

    group = m_gal->BeginGroup();
    viewData->setGroup( aLayer, group );

    if( !m_painter->Draw( static_cast<EDA_ITEM*>( aItem ), aLayer ) )
        aItem->ViewDraw( aLayer, this ); // Alternative drawing method

    m_gal->EndGroup();
}


void VIEW::updateBbox( VIEW_ITEM* aItem )
{
    int layers[VIEW_MAX_LAYERS], layers_count;

    aItem->ViewGetLayers( layers, layers_count );

    for( int i = 0; i < layers_count; ++i )
    {
        VIEW_LAYER& l = m_layers[layers[i]];
        l.items->Remove( aItem );
        l.items->Insert( aItem );
        MarkTargetDirty( l.target );
    }
}


void VIEW::updateLayers( VIEW_ITEM* aItem )
{
    auto viewData = aItem->viewPrivData();
    int layers[VIEW_MAX_LAYERS], layers_count;

    if( !viewData )
        return;

    // Remove the item from previous layer set
    viewData->getLayers( layers, layers_count );

    for( int i = 0; i < layers_count; ++i )
    {
        VIEW_LAYER& l = m_layers[layers[i]];
        l.items->Remove( aItem );
        MarkTargetDirty( l.target );

        if( IsCached( l.id ) )
        {
            // Redraw the item from scratch
            int prevGroup = viewData->getGroup( layers[i] );

            if( prevGroup >= 0 )
            {
                m_gal->DeleteGroup( prevGroup );
                viewData->setGroup( l.id, -1 );
            }
        }
    }

    // Add the item to new layer set
    aItem->ViewGetLayers( layers, layers_count );
    viewData->saveLayers( layers, layers_count );

    for( int i = 0; i < layers_count; i++ )
    {
        VIEW_LAYER& l = m_layers[layers[i]];
        l.items->Insert( aItem );
        MarkTargetDirty( l.target );
    }
}


bool VIEW::areRequiredLayersEnabled( int aLayerId ) const
{
    wxCHECK( (unsigned) aLayerId < m_layers.size(), false );

    std::set<int>::const_iterator it, it_end;

    for( it = m_layers.at( aLayerId ).requiredLayers.begin(),
         it_end = m_layers.at( aLayerId ).requiredLayers.end(); it != it_end; ++it )
    {
        // That is enough if just one layer is not enabled
        if( !m_layers.at( *it ).visible || !areRequiredLayersEnabled( *it ) )
            return false;
    }

    return true;
}


void VIEW::RecacheAllItems()
{
    BOX2I r;

    r.SetMaximum();

    for( LAYER_MAP_ITER i = m_layers.begin(); i != m_layers.end(); ++i )
    {
        VIEW_LAYER* l = &( ( *i ).second );

        if( IsCached( l->id ) )
        {
            recacheItem visitor( this, m_gal, l->id );
            l->items->Query( r, visitor );
        }
    }
}


void VIEW::UpdateItems()
{
    if( m_gal->IsVisible() )
    {
        GAL_UPDATE_CONTEXT ctx( m_gal );

        for( VIEW_ITEM* item : *m_allItems )
        {
            auto viewData = item->viewPrivData();

            if( !viewData )
                continue;

            if( viewData->m_requiredUpdate != NONE )
            {
                invalidateItem( item, viewData->m_requiredUpdate );
                viewData->m_requiredUpdate = NONE;
            }
        }
    }
}


void VIEW::UpdateAllItems( int aUpdateFlags )
{
    for( VIEW_ITEM* item : *m_allItems )
    {
        auto viewData = item->viewPrivData();

        if( !viewData )
            continue;

        viewData->m_requiredUpdate |= aUpdateFlags;
    }
}


void VIEW::UpdateAllItemsConditionally( int aUpdateFlags,
                                        std::function<bool( VIEW_ITEM* )> aCondition )
{
    for( VIEW_ITEM* item : *m_allItems )
    {
        if( aCondition( item ) )
        {
            auto viewData = item->viewPrivData();

            if( !viewData )
                continue;

            viewData->m_requiredUpdate |= aUpdateFlags;
        }
    }
}


std::unique_ptr<VIEW> VIEW::DataReference() const
{
    auto ret = std::make_unique<VIEW>();
    ret->m_allItems = m_allItems;
    ret->m_layers = m_layers;
    ret->sortLayers();
    return ret;
}


void VIEW::SetVisible( VIEW_ITEM* aItem, bool aIsVisible )
{
    auto viewData = aItem->viewPrivData();

    if( !viewData )
        return;

    bool cur_visible = viewData->m_flags & VISIBLE;

    if( cur_visible != aIsVisible )
    {
        if( aIsVisible )
            viewData->m_flags |= VISIBLE;
        else
            viewData->m_flags &= ~VISIBLE;

        Update( aItem, APPEARANCE | COLOR );
    }
}


void VIEW::Hide( VIEW_ITEM* aItem, bool aHide )
{
    auto viewData = aItem->viewPrivData();

    if( !viewData )
        return;

    if( !( viewData->m_flags & VISIBLE ) )
        return;

    if( aHide )
        viewData->m_flags |= HIDDEN;
    else
        viewData->m_flags &= ~HIDDEN;

    Update( aItem, APPEARANCE );
}


bool VIEW::IsVisible( const VIEW_ITEM* aItem ) const
{
    const auto viewData = aItem->viewPrivData();

    return viewData->m_flags & VISIBLE;
}


void VIEW::Update( VIEW_ITEM* aItem )
{
    Update( aItem, ALL );
}


void VIEW::Update( VIEW_ITEM* aItem, int aUpdateFlags )
{
    auto viewData = aItem->viewPrivData();

    if( !viewData )
        return;

    assert( aUpdateFlags != NONE );

    viewData->m_requiredUpdate |= aUpdateFlags;

}


std::shared_ptr<VIEW_OVERLAY> VIEW::MakeOverlay()
{
    std::shared_ptr<VIEW_OVERLAY> overlay( new VIEW_OVERLAY );

    Add( overlay.get() );
    return overlay;
}


const int VIEW::TOP_LAYER_MODIFIER = -VIEW_MAX_LAYERS;

}
