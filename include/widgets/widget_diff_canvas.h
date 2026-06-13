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

#ifndef KICAD_WIDGET_DIFF_CANVAS_H
#define KICAD_WIDGET_DIFF_CANVAS_H

#include <class_draw_panel_gal.h>
#include <diff_merge/diff_scene.h>

#include <array>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <vector>


namespace KIGFX
{
class GAL_DISPLAY_OPTIONS;
class PAINTER;
class VIEW_ITEM;
class VIEW_GROUP;
}


namespace KIGFX
{
class VIEW_OVERLAY;
}


/**
 * GAL-backed canvas for visualizing a KICAD_DIFF::DIFF_SCENE. This is the
 * hardware-accelerated counterpart to DIFF_THUMBNAIL.
 *
 * The canvas owns a KIGFX::VIEW, a VIEW_OVERLAY, and an optional non-owning
 * VIEW_GROUP for source document items. Module callers can install a native
 * painter/context group so unchanged document context is drawn as real items,
 * while the overlay still holds selectable diff change shapes.
 *
 * Pan and zoom come from WX_VIEW_CONTROLS (mouse middle-drag, wheel) for
 * free. Pick handling and home/F-zoom are wired explicitly so the public
 * API matches DIFF_THUMBNAIL's contract; dialogs can swap one for the
 * other without further changes.
 */
class WIDGET_DIFF_CANVAS : public EDA_DRAW_PANEL_GAL
{
public:
    WIDGET_DIFF_CANVAS( wxWindow* aParent, wxWindowID aId = wxID_ANY );

    ~WIDGET_DIFF_CANVAS() override;

private:
    /// Delegating ctor target — keeps the GAL options unique_ptr alive across
    /// the base-class initialization that needs to deref it.
    WIDGET_DIFF_CANVAS( wxWindow* aParent, wxWindowID aId, std::unique_ptr<KIGFX::GAL_DISPLAY_OPTIONS> aGalOptions );

public:
    /// Replace the displayed scene. Pass an empty scene to clear the canvas.
    void SetScene( KICAD_DIFF::DIFF_SCENE aScene );

    /// Install the native painter used for drawing source document context.
    /// pcbnew/eeschema provide module-specific painters configured with a
    /// forced neutral color. The diff overlay remains owned by this widget.
    void SetContextPainter( std::unique_ptr<KIGFX::PAINTER> aPainter );

    /// Replace source document context items. Items are not owned by the
    /// widget; callers must keep the source documents alive while the dialog
    /// is open.
    void SetContextItems( const std::vector<KIGFX::VIEW_ITEM*>& aItems );

    /// Tag context items by change category so SetCategoryVisible can hide /
    /// show them in lockstep with the diff-category checkboxes.
    void SetItemCategories( std::map<KIGFX::VIEW_ITEM*, KICAD_DIFF::CATEGORY> aMap );

    /// Kiface-supplied callback that dims/undims a context item.
    using DIMMER = std::function<void( KIGFX::VIEW_ITEM*, bool aDim )>;
    void SetItemDimmer( DIMMER aDimmer ) { m_itemDimmer = std::move( aDimmer ); }

    /// Outline shape(s) whose SCENE_SHAPE::changeId matches the given path.
    /// Multiple shapes can share a changeId — DUPLICATE_UUID changes for
    /// example — and all matching shapes get the highlight treatment. Pass
    /// std::nullopt to clear.
    void HighlightChange( std::optional<KIID_PATH> aChangeId );

    /// Toggle whether shapes of a given change category render. Defaults
    /// to all visible.
    void SetCategoryVisible( KICAD_DIFF::CATEGORY aCategory, bool aVisible );

    bool IsCategoryVisible( KICAD_DIFF::CATEGORY aCategory ) const;

    /// Replace the set of muted changes. Hidden changes render muted grey and
    /// stop grabbing clicks.
    void SetHiddenChanges( std::set<KIID_PATH> aHidden );

    bool IsChangeHidden( const KIID_PATH& aChangeId ) const;

    /// Toggle whether board-context geometry on a given PCB layer renders.
    /// Layerless geometry remains visible regardless of this filter.
    void SetLayerVisible( PCB_LAYER_ID aLayer, bool aVisible );

    bool IsLayerVisible( PCB_LAYER_ID aLayer ) const;

    /// Hold overlay rebuilds/zoom until EndUpdate, so a batch of changes (e.g.
    /// a reload that re-sets context and scene) renders once instead of flashing
    /// an intermediate state.
    void BeginUpdate();
    void EndUpdate();

    /// Center the view on the scene's document bbox. Safe to call before the
    /// canvas has been sized — the request will be deferred to the next
    /// wxEVT_SIZE.
    void ZoomToFit();

    /// Zoom and center on a specific document-space bbox with a small margin.
    void ZoomToBBox( const BOX2I& aBBox );

    /// Pan so the currently highlighted change is at
    /// the center of the canvas. No-op when nothing is highlighted.
    void CenterOnHighlight();

    /// Register a callback to fire when the user left-clicks the canvas.
    /// The optional holds the picked change id when a shape was hit, or
    /// std::nullopt when the click landed on empty space (deselect).
    using PICK_HANDLER = std::function<void( const std::optional<KIID_PATH>& )>;

    void SetPickHandler( PICK_HANDLER aHandler ) { m_pickHandler = std::move( aHandler ); }

    using ITEM_HANDLER = std::function<void( KIGFX::VIEW_ITEM* )>;
    void SetDoubleClickHandler( ITEM_HANDLER aHandler ) { m_dclickHandler = std::move( aHandler ); }

private:
    /// Rebuild the overlay from the render scene + current highlight state.
    /// aOverlayOnly marks only the overlay target dirty (for hover/highlight
    /// changes that leave the cached board untouched).
    void rebuildOverlay( bool aOverlayOnly = false );

    /// Recompute the cached render scene (layer-filtered geometry, cleared in
    /// native-context mode). Only depends on the scene, layer visibility, and
    /// context mode, so it runs when those change, not on every interaction.
    void buildRenderScene();

    /// Dim context items that are hidden, or unfocused while a highlight is active.
    void refreshItemDimming();

    /// Hit test in screen coordinates → topmost SCENE_SHAPE under the cursor.
    const KICAD_DIFF::SCENE_SHAPE* shapeAt( const wxPoint& aScreenPoint ) const;

    /// Union bbox of all shapes whose changeId matches the current highlight,
    /// honoring per-category visibility. std::nullopt when nothing matches.
    std::optional<BOX2I> highlightedBBox() const;

    void onLeftDown( wxMouseEvent& aEvent );
    void onDoubleClick( wxMouseEvent& aEvent );
    void onMotion( wxMouseEvent& aEvent );
    void onLeave( wxMouseEvent& aEvent );
    void onChar( wxKeyEvent& aEvent );
    void onSize( wxSizeEvent& aEvent );

    struct HIGHLIGHT_BOX_ITEM;

    KICAD_DIFF::DIFF_SCENE   m_scene;
    KICAD_DIFF::DIFF_SCENE              m_renderScene;
    std::optional<KIID_PATH> m_highlight;
    std::optional<KIID_PATH>            m_hover;
    PICK_HANDLER             m_pickHandler;
    ITEM_HANDLER                        m_dclickHandler;
    DIMMER                              m_itemDimmer;
    std::unique_ptr<HIGHLIGHT_BOX_ITEM> m_highlightBox;
    std::unique_ptr<HIGHLIGHT_BOX_ITEM> m_hoverBox;

    std::array<bool, KICAD_DIFF::CATEGORY_COUNT> m_categoryVisible{ { true, true, true, true } };

    std::set<KIID_PATH> m_hiddenChanges;

    /// While true, rebuildOverlay()/ZoomToFit() are held so a batch renders once.
    bool m_holdRebuild = false;

    LSET m_layerVisible;

    bool m_hasNativeContext = false;
    std::unique_ptr<KIGFX::VIEW_GROUP> m_contextGroup;
    std::vector<KIGFX::VIEW_ITEM*>                    m_contextGroupItems;
    std::map<KIGFX::VIEW_ITEM*, KICAD_DIFF::CATEGORY> m_itemCategories;
    std::shared_ptr<KIGFX::VIEW_OVERLAY> m_overlay;

    /// EDA_DRAW_PANEL_GAL keeps a reference to the options struct, so it
    /// must outlive the panel. Per-instance so siblings can vary if needed.
    std::unique_ptr<KIGFX::GAL_DISPLAY_OPTIONS> m_galOptions;

    /// True when a ZoomToFit was requested before the canvas had a real
    /// size — the next onSize will retry the fit and clear this.
    bool m_zoomToFitPending = false;
};

#endif // KICAD_WIDGET_DIFF_CANVAS_H
