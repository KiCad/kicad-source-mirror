/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <widgets/widget_diff_canvas.h>

#include <diff_merge/diff_renderer_gal.h>

#include <drawing_sheet/ds_painter.h>
#include <eda_item.h>
#include <gal/color4d.h>
#include <gal/gal_display_options.h>
#include <gal/graphics_abstraction_layer.h>
#include <layer_ids.h>
#include <view/view.h>
#include <view/view_group.h>
#include <view/view_item.h>
#include <view/view_overlay.h>
#include <view/wx_view_controls.h>
#include <zoom_defines.h>


struct WIDGET_DIFF_CANVAS::HIGHLIGHT_BOX_ITEM
{
    BOX2I          bbox;
    KIGFX::COLOR4D color;
    bool           shown = false;
};


WIDGET_DIFF_CANVAS::WIDGET_DIFF_CANVAS( wxWindow* aParent, wxWindowID aId ) :
        WIDGET_DIFF_CANVAS( aParent, aId, std::make_unique<KIGFX::GAL_DISPLAY_OPTIONS>() )
{
}


WIDGET_DIFF_CANVAS::WIDGET_DIFF_CANVAS( wxWindow* aParent, wxWindowID aId,
                                        std::unique_ptr<KIGFX::GAL_DISPLAY_OPTIONS> aGalOptions ) :
        EDA_DRAW_PANEL_GAL( aParent, aId, wxDefaultPosition, wxDefaultSize, *aGalOptions, GAL_FALLBACK ),
        m_layerVisible( LSET::AllLayersMask() ),
        m_galOptions( std::move( aGalOptions ) )
{
    m_view = new KIGFX::VIEW();
    m_view->SetGAL( m_gal );

    // Until a module installs a native context painter, the canvas renders only
    // VIEW_OVERLAY commands, which bypass the painter. Reuse the drawing-sheet
    // pair (also used by PL_DRAW_PANEL_GAL) rather than a private stub. Force
    // the historical near-white background the diff canvas expects;
    // DS_RENDER_SETTINGS otherwise defaults to pure white.
    m_painter = std::make_unique<KIGFX::DS_PAINTER>( m_gal );
    m_painter->GetSettings()->SetBackgroundColor( KIGFX::COLOR4D( 0.97, 0.97, 0.97, 1.0 ) );
    m_view->SetPainter( m_painter.get() );

    m_view->SetScaleLimits( ZOOM_MAX_LIMIT_DIFF, ZOOM_MIN_LIMIT_DIFF );
    m_view->SetMirror( false, false );

    m_contextGroup = std::make_unique<KIGFX::VIEW_GROUP>();
    m_contextGroup->SetLayer( LAYER_GP_OVERLAY );
    m_view->Add( m_contextGroup.get(), 0 );

    // The overlay needs to live on a layer the view is willing to draw.
    // LAYER_GP_OVERLAY is a general-purpose overlay slot used elsewhere in
    // KiCad GAL panels for non-cached annotation.
    m_view->SetLayerTarget( LAYER_GP_OVERLAY, KIGFX::TARGET_OVERLAY );
    m_view->SetLayerDisplayOnly( LAYER_GP_OVERLAY );
    m_view->SetLayerVisible( LAYER_GP_OVERLAY, true );

    m_overlay = m_view->MakeOverlay();

    m_highlightBox = std::make_unique<HIGHLIGHT_BOX_ITEM>();
    m_hoverBox = std::make_unique<HIGHLIGHT_BOX_ITEM>();

    m_viewControls = new KIGFX::WX_VIEW_CONTROLS( m_view, this );

    m_gal->SetCursorEnabled( false );

    // Matches DIFF_THUMBNAIL so existing dialog layouts don't collapse the
    // canvas when sizers are tight.
    SetMinSize( wxSize( -1, 100 ) );

    Bind( wxEVT_LEFT_DOWN, &WIDGET_DIFF_CANVAS::onLeftDown, this );
    Bind( wxEVT_LEFT_DCLICK, &WIDGET_DIFF_CANVAS::onDoubleClick, this );
    Bind( wxEVT_MOTION, &WIDGET_DIFF_CANVAS::onMotion, this );
    Bind( wxEVT_LEAVE_WINDOW, &WIDGET_DIFF_CANVAS::onLeave, this );
    Bind( wxEVT_CHAR_HOOK, &WIDGET_DIFF_CANVAS::onChar, this );
    Bind( wxEVT_SIZE, &WIDGET_DIFF_CANVAS::onSize, this );

    SetEvtHandlerEnabled( true );
    Show( true );
    StartDrawing();
}


WIDGET_DIFF_CANVAS::~WIDGET_DIFF_CANVAS()
{
    StopDrawing();

    for( KIGFX::VIEW_ITEM* item : m_contextGroupItems )
    {
        if( item )
            m_view->Remove( item );
    }

    m_view->Remove( m_contextGroup.get() );
    // m_view owns the overlay since MakeOverlay() — no manual Remove needed.
}


void WIDGET_DIFF_CANVAS::SetScene( KICAD_DIFF::DIFF_SCENE aScene )
{
    m_scene = std::move( aScene );
    m_highlight = std::nullopt;

    buildRenderScene();
    rebuildOverlay();
    ZoomToFit();
}


void WIDGET_DIFF_CANVAS::SetWorldUnitLength( double aWorldUnitLength )
{
    m_gal->SetWorldUnitLength( aWorldUnitLength );
    m_gal->ComputeWorldScreenMatrix();
    ZoomToFit();
}


void WIDGET_DIFF_CANVAS::SetContextPainter( std::unique_ptr<KIGFX::PAINTER> aPainter )
{
    if( !aPainter )
        return;

    m_painter = std::move( aPainter );
    m_view->SetPainter( m_painter.get() );
    m_view->MarkDirty();
    Refresh();
}


void WIDGET_DIFF_CANVAS::SetContextItems( const std::vector<KIGFX::VIEW_ITEM*>& aItems )
{
    if( !m_contextGroup )
        return;

    for( KIGFX::VIEW_ITEM* item : m_contextGroupItems )
    {
        if( item )
            m_view->Remove( item );
    }

    m_contextGroupItems.clear();
    m_itemCategories.clear();
    m_hasNativeContext = false;

    for( KIGFX::VIEW_ITEM* item : aItems )
    {
        if( !item )
            continue;

        m_view->Add( item );
        m_contextGroupItems.push_back( item );
        m_hasNativeContext = true;
    }

    buildRenderScene();
    rebuildOverlay();
}


void WIDGET_DIFF_CANVAS::HighlightChange( std::optional<KIID_PATH> aChangeId )
{
    if( m_highlight == aChangeId )
        return;

    m_highlight = std::move( aChangeId );

    const std::optional<KIID> focusKiid =
            m_highlight && !m_highlight->empty() ? std::optional( m_highlight->back() ) : std::nullopt;

    // Dimming depends only on the hidden set, not the highlight, so it does not
    // need re-applying here. Re-running it would REPAINT every board item on
    // each click, which is the source of the highlight lag.

    m_highlightBox->shown = false;

    // The committed selection takes over from any active hover so the rect
    // doesn't double-draw on the next mouse move.
    if( focusKiid && m_hover == m_highlight )
    {
        m_hover.reset();
        m_hoverBox->shown = false;
    }

    if( focusKiid )
    {
        const KICAD_DIFF::DIFF_COLOR_THEME theme;

        for( KIGFX::VIEW_ITEM* item : m_contextGroupItems )
        {
            EDA_ITEM* eda = dynamic_cast<EDA_ITEM*>( item );

            if( !eda || eda->m_Uuid != *focusKiid )
                continue;

            const BOX2I bb = item->ViewBBox();

            if( bb.GetWidth() <= 0 || bb.GetHeight() <= 0 )
                break;

            auto           catIt = m_itemCategories.find( item );
            KIGFX::COLOR4D base = catIt != m_itemCategories.end() ? KICAD_DIFF::ThemeColorFor( theme, catIt->second )
                                                                  : theme.modified;

            base.a = 0.35;

            m_highlightBox->bbox = bb;
            m_highlightBox->color = base;
            m_highlightBox->shown = true;
            break;
        }
    }

    rebuildOverlay( /*aOverlayOnly=*/true );
}


void WIDGET_DIFF_CANVAS::SetCategoryVisible( KICAD_DIFF::CATEGORY aCategory, bool aVisible )
{
    const std::size_t idx = static_cast<std::size_t>( aCategory );

    wxCHECK_RET( idx < m_categoryVisible.size(), wxS( "Invalid CATEGORY" ) );

    if( m_categoryVisible[idx] == aVisible )
        return;

    m_categoryVisible[idx] = aVisible;

    for( const auto& [item, cat] : m_itemCategories )
    {
        if( cat != aCategory )
            continue;

        m_view->SetVisible( item, aVisible );

        // Force a full repaint so per-item color overrides re-apply.
        if( aVisible )
            m_view->Update( item, KIGFX::REPAINT );
    }

    rebuildOverlay();
}


void WIDGET_DIFF_CANVAS::SetItemCategories( std::map<KIGFX::VIEW_ITEM*, KICAD_DIFF::CATEGORY> aMap )
{
    m_itemCategories = std::move( aMap );

    for( const auto& [item, cat] : m_itemCategories )
    {
        const std::size_t idx = static_cast<std::size_t>( cat );

        if( idx < m_categoryVisible.size() )
            m_view->SetVisible( item, m_categoryVisible[idx] );
    }
}


bool WIDGET_DIFF_CANVAS::IsCategoryVisible( KICAD_DIFF::CATEGORY aCategory ) const
{
    const std::size_t idx = static_cast<std::size_t>( aCategory );

    wxCHECK( idx < m_categoryVisible.size(), false );

    return m_categoryVisible[idx];
}


void WIDGET_DIFF_CANVAS::SetHiddenChanges( std::set<KIID_PATH> aHidden )
{
    if( m_hiddenChanges == aHidden )
        return;

    m_hiddenChanges = std::move( aHidden );
    refreshItemDimming();
    rebuildOverlay();
}


bool WIDGET_DIFF_CANVAS::IsChangeHidden( const KIID_PATH& aChangeId ) const
{
    return m_hiddenChanges.count( aChangeId ) > 0;
}


void WIDGET_DIFF_CANVAS::refreshItemDimming()
{
    if( !m_itemDimmer )
        return;

    // Item Uuids of the hidden changes (a change id's last element is its item).
    std::set<KIID> hiddenItems;

    for( const KIID_PATH& path : m_hiddenChanges )
    {
        if( !path.empty() )
            hiddenItems.insert( path.back() );
    }

    for( KIGFX::VIEW_ITEM* item : m_contextGroupItems )
    {
        if( !item )
            continue;

        EDA_ITEM*  eda = dynamic_cast<EDA_ITEM*>( item );
        const bool hidden = eda && hiddenItems.count( eda->m_Uuid ) > 0;

        m_itemDimmer( item, hidden );
        m_view->Update( item, KIGFX::REPAINT );
    }
}


void WIDGET_DIFF_CANVAS::SetLayerVisible( PCB_LAYER_ID aLayer, bool aVisible )
{
    if( aLayer < 0 || aLayer >= PCB_LAYER_ID_COUNT )
        return;

    if( m_layerVisible.Contains( aLayer ) == aVisible )
        return;

    m_layerVisible.set( aLayer, aVisible );
    m_view->SetLayerVisible( aLayer, aVisible );
    buildRenderScene();
    rebuildOverlay();
}


bool WIDGET_DIFF_CANVAS::IsLayerVisible( PCB_LAYER_ID aLayer ) const
{
    return m_layerVisible.Contains( aLayer );
}


void WIDGET_DIFF_CANVAS::BeginUpdate()
{
    m_holdRebuild = true;
}


void WIDGET_DIFF_CANVAS::EndUpdate()
{
    m_holdRebuild = false;
    rebuildOverlay();

    if( m_zoomToFitPending )
        ZoomToFit();
}


void WIDGET_DIFF_CANVAS::ZoomToFit()
{
    if( m_holdRebuild )
    {
        m_zoomToFitPending = true;
        return;
    }

    BOX2I bb = m_scene.documentBBox;
    bool  haveBox = bb.GetWidth() > 0 && bb.GetHeight() > 0;

    // Also frame the context items so the whole board fits, not just the changes.
    for( KIGFX::VIEW_ITEM* item : m_contextGroupItems )
    {
        if( !item )
            continue;

        BOX2I ib = item->ViewBBox();

        if( ib.GetWidth() <= 0 || ib.GetHeight() <= 0 )
            continue;

        if( haveBox )
        {
            bb.Merge( ib );
        }
        else
        {
            bb = ib;
            haveBox = true;
        }
    }

    if( !haveBox )
    {
        m_zoomToFitPending = false;
        return;
    }

    // VIEW::SetViewport silently no-ops when the GAL has no screen size, so
    // defer the fit until we've been sized at least once.
    const wxSize client = GetClientSize();

    if( client.x <= 0 || client.y <= 0 )
    {
        m_zoomToFitPending = true;
        return;
    }

    ZoomToBBox( bb );
    m_zoomToFitPending = false;
}


void WIDGET_DIFF_CANVAS::ZoomToBBox( const BOX2I& aBBox )
{
    if( !aBBox.GetWidth() || !aBBox.GetHeight() )
        return;

    BOX2D viewport( VECTOR2D( aBBox.GetLeft(), aBBox.GetTop() ), VECTOR2D( aBBox.GetWidth(), aBBox.GetHeight() ) );

    // Inflate slightly so the outermost shape isn't flush with the edge.
    viewport.Inflate( aBBox.GetWidth() * 0.05, aBBox.GetHeight() * 0.05 );

    m_view->SetViewport( viewport );
    Refresh();
}


void WIDGET_DIFF_CANVAS::CenterOnHighlight()
{
    std::optional<BOX2I> bb = highlightedBBox();

    if( !bb || bb->GetWidth() <= 0 || bb->GetHeight() <= 0 )
        return;

    const VECTOR2I center = bb->GetCenter();
    m_view->SetCenter( VECTOR2D( center.x, center.y ) );
    m_view->MarkDirty();
    Refresh();
}


void WIDGET_DIFF_CANVAS::buildRenderScene()
{
    m_renderScene = m_scene;

    if( m_hasNativeContext )
    {
        m_renderScene.referenceGeometry = {};
        m_renderScene.comparisonGeometry = {};
    }
    else
    {
        m_renderScene.referenceGeometry =
                KICAD_DIFF::FilterGeometryByVisibleLayers( m_scene.referenceGeometry, m_layerVisible );
        m_renderScene.comparisonGeometry =
                KICAD_DIFF::FilterGeometryByVisibleLayers( m_scene.comparisonGeometry, m_layerVisible );
    }
}


void WIDGET_DIFF_CANVAS::rebuildOverlay( bool aOverlayOnly )
{
    if( m_holdRebuild )
        return;

    KICAD_DIFF::RenderSceneToOverlay( *m_overlay, m_renderScene, m_categoryVisible, m_highlight, m_hiddenChanges );

    auto drawBox = [&]( const HIGHLIGHT_BOX_ITEM* aBox )
    {
        if( !aBox || !aBox->shown || aBox->bbox.GetWidth() <= 0 || aBox->bbox.GetHeight() <= 0 )
            return;

        m_overlay->SetIsFill( true );
        m_overlay->SetIsStroke( false );
        m_overlay->SetFillColor( aBox->color );
        m_overlay->Rectangle( aBox->bbox.GetOrigin(), aBox->bbox.GetEnd() );
    };

    drawBox( m_hoverBox.get() );
    drawBox( m_highlightBox.get() );

    if( aOverlayOnly )
        m_view->MarkTargetDirty( KIGFX::TARGET_OVERLAY );
    else
        m_view->MarkDirty();

    Refresh();
}


const KICAD_DIFF::SCENE_SHAPE* WIDGET_DIFF_CANVAS::shapeAt( const wxPoint& aScreenPoint ) const
{
    if( !m_view )
        return nullptr;

    // Convert from screen to document coordinates.
    const VECTOR2D doc = m_view->ToWorld( VECTOR2D( aScreenPoint.x, aScreenPoint.y ) );

    // Walk PAINT_ORDER in reverse so topmost shape wins, mirroring the
    // thumbnail's hit-test order.
    for( auto it = KICAD_DIFF::PAINT_ORDER.rbegin(); it != KICAD_DIFF::PAINT_ORDER.rend(); ++it )
    {
        if( !IsCategoryVisible( *it ) )
            continue;

        const std::vector<KICAD_DIFF::SCENE_SHAPE>& list = KICAD_DIFF::ShapesFor( m_scene, *it );

        for( auto sIt = list.rbegin(); sIt != list.rend(); ++sIt )
        {
            // Muted changes let clicks fall through to what is underneath.
            if( m_hiddenChanges.count( sIt->changeId ) > 0 )
                continue;

            if( sIt->bbox.Contains( VECTOR2I( KiROUND( doc.x ), KiROUND( doc.y ) ) ) )
                return &*sIt;
        }
    }

    return nullptr;
}


void WIDGET_DIFF_CANVAS::onLeftDown( wxMouseEvent& aEvent )
{
    std::optional<KIID_PATH> picked;

    if( const KICAD_DIFF::SCENE_SHAPE* shape = shapeAt( aEvent.GetPosition() ) )
        picked = shape->changeId;

    SetFocus();
    aEvent.Skip();

    if( m_pickHandler )
        m_pickHandler( picked );
}


void WIDGET_DIFF_CANVAS::onDoubleClick( wxMouseEvent& aEvent )
{
    aEvent.Skip();

    if( !m_dclickHandler || m_contextGroupItems.empty() )
        return;

    const VECTOR2D world = m_view->ToWorld( VECTOR2D( aEvent.GetPosition().x, aEvent.GetPosition().y ) );
    const VECTOR2I worldI( KiROUND( world.x ), KiROUND( world.y ) );

    KIGFX::VIEW_ITEM* hit = nullptr;
    BOX2I             hitBBox;

    // Reverse so the topmost item wins if bboxes overlap.
    for( auto it = m_contextGroupItems.rbegin(); it != m_contextGroupItems.rend(); ++it )
    {
        KIGFX::VIEW_ITEM* item = *it;

        if( !item )
            continue;

        const BOX2I bb = item->ViewBBox();

        if( bb.GetWidth() <= 0 || bb.GetHeight() <= 0 )
            continue;

        if( !bb.Contains( worldI ) )
            continue;

        // Prefer the smallest matching bbox so a nested sheet wins over its parent.
        if( !hit || bb.GetArea() < hitBBox.GetArea() )
        {
            hit = item;
            hitBBox = bb;
        }
    }

    if( hit )
        m_dclickHandler( hit );
}


void WIDGET_DIFF_CANVAS::onMotion( wxMouseEvent& aEvent )
{
    aEvent.Skip();

    std::optional<KIID_PATH>       hovered;
    const KICAD_DIFF::SCENE_SHAPE* shape = shapeAt( aEvent.GetPosition() );

    // Suppress hover when it would land on the already-selected change so the
    // committed highlight stays clean.
    if( shape && ( !m_highlight || shape->changeId != *m_highlight ) )
        hovered = shape->changeId;

    if( hovered == m_hover )
        return;

    m_hover = hovered;
    m_hoverBox->shown = false;

    if( shape && hovered )
    {
        KIGFX::COLOR4D base = shape->color;
        base.a = 0.20;
        m_hoverBox->bbox = shape->bbox;
        m_hoverBox->color = base;
        m_hoverBox->shown = true;
    }

    rebuildOverlay( /*aOverlayOnly=*/true );
}


void WIDGET_DIFF_CANVAS::onLeave( wxMouseEvent& aEvent )
{
    aEvent.Skip();

    if( !m_hover && !m_hoverBox->shown )
        return;

    m_hover.reset();
    m_hoverBox->shown = false;
    rebuildOverlay( /*aOverlayOnly=*/true );
}


void WIDGET_DIFF_CANVAS::onChar( wxKeyEvent& aEvent )
{
    switch( aEvent.GetKeyCode() )
    {
    case WXK_HOME:
        if( m_scene.documentBBox.GetWidth() || m_scene.documentBBox.GetHeight() )
        {
            ZoomToFit();
            return;
        }

        aEvent.Skip();
        return;

    case 'F':
    case 'f':
        if( std::optional<BOX2I> bbox = highlightedBBox() )
        {
            ZoomToBBox( *bbox );
            return;
        }

        aEvent.Skip();
        return;

    case WXK_ESCAPE:
        if( m_highlight && m_pickHandler )
        {
            m_pickHandler( std::nullopt );
            return;
        }

        aEvent.Skip();
        return;

    default: aEvent.Skip(); return;
    }
}


void WIDGET_DIFF_CANVAS::onSize( wxSizeEvent& aEvent )
{
    aEvent.Skip();

    if( m_zoomToFitPending )
        ZoomToFit();
}


std::optional<BOX2I> WIDGET_DIFF_CANVAS::highlightedBBox() const
{
    if( !m_highlight.has_value() )
        return std::nullopt;

    return KICAD_DIFF::HighlightedBBox( m_scene, *m_highlight, m_categoryVisible );
}
