/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 */

#ifndef KICAD_DIFF_SCENE_H
#define KICAD_DIFF_SCENE_H

#include <kicommon.h>

#include <diff_merge/kicad_diff_types.h>
#include <diff_merge/diff_doc_kind.h>

#include <base_units.h>
#include <gal/color4d.h>
#include <lset.h>
#include <math/box2.h>

#include <wx/string.h>

#include <array>
#include <map>
#include <optional>

class SHAPE_POLY_SET;


namespace KICAD_DIFF
{

/**
 * Visual category each ITEM_CHANGE belongs to in the scene. There are four
 * categories because the renderer buckets shapes into four lists; both
 * COLLISION and DUPLICATE_UUID map to CONFLICT.
 */
enum class CATEGORY
{
    ADDED,
    REMOVED,
    MODIFIED,
    CONFLICT,
};


/**
 * Paint order. Modified shapes are drawn first (background), conflicts last
 * (foreground) so the most important changes win when bboxes overlap.
 * Renderers (plotter, GAL, thumbnail) must walk this array forward when
 * painting and backward when hit-testing. Also the canonical "iterate all
 * categories" sequence — its size is the number of categories.
 */
inline constexpr std::array<CATEGORY, 4> PAINT_ORDER{
    CATEGORY::MODIFIED,
    CATEGORY::ADDED,
    CATEGORY::REMOVED,
    CATEGORY::CONFLICT,
};


inline constexpr std::size_t CATEGORY_COUNT = PAINT_ORDER.size();

/**
 * Shared rendering model consumed by both the GAL renderer (interactive
 * widget) and the plotter renderer (headless PNG/SVG output).
 *
 * The scene is a flat list of "shapes to draw" derived from a DOCUMENT_DIFF.
 * Currently each shape is just a colored rectangle around the changed item's
 * bbox; a future revision can pull the actual item geometry from the source
 * board/schematic for a true overlay render.
 *
 * Color palette is set by the consumer (UI colors come from the user's
 * theme; CLI uses a fixed printable-on-light-bg default).
 */
struct KICOMMON_API SCENE_SHAPE
{
    BOX2I          bbox;
    KIGFX::COLOR4D color;
    wxString       label; // shown by interactive renderer as a hover tooltip

    /// Stable identifier of the ITEM_CHANGE that produced this shape. Used by
    /// the interactive widgets to cross-probe between change tree selection
    /// and the rendered scene without relying on bbox equality (two distinct
    /// changes can share an enclosing rectangle).
    KIID_PATH changeId;

    using PolygonList = std::vector<std::vector<std::vector<VECTOR2I>>>;
    PolygonList polygons;
};


/**
 * Stroked line segment from one of the source documents. Width and color are
 * carried so the renderer doesn't have to consult a theme per shape; the
 * caller picks the color based on the side and any layer-specific tint it
 * wants. Width 0 yields a one-device-pixel hairline in the GAL renderer; the
 * plotter promotes it to a visible minimum.
 *
 * Side membership is implied by whether the shape lives in DIFF_SCENE's
 * referenceGeometry or comparisonGeometry — no separate field needed.
 */
struct KICOMMON_API DOCUMENT_SEGMENT
{
    VECTOR2I       start;
    VECTOR2I       end;
    int            width = 0;
    KIGFX::COLOR4D color;
    LSET           layers;
};


/**
 * Closed polygon outline from a source document. When `filled` is true the
 * interior is shaded with `color`; otherwise only the outline is stroked at
 * `lineWidth`. Outline points are expected to be in order — first/last
 * connect implicitly.
 */
struct KICOMMON_API DOCUMENT_POLYGON
{
    std::vector<VECTOR2I> outline;
    bool                  filled = false;
    int                   lineWidth = 0;
    KIGFX::COLOR4D        color;
    LSET                  layers;
};


/**
 * Filled or stroked circle. Mainly used for pads, vias and junction dots.
 */
struct KICOMMON_API DOCUMENT_CIRCLE
{
    VECTOR2I       center;
    int            radius = 0;
    bool           filled = false;
    int            lineWidth = 0;
    KIGFX::COLOR4D color;
    LSET           layers;
};


/**
 * Aggregate of background geometry extracted from one source document.
 * Populated by per-format extractors (PCB, schematic, library) and rendered
 * underneath the diff bbox rectangles so the user sees what's being diffed,
 * not just where the differences cluster.
 */
struct KICOMMON_API DOCUMENT_GEOMETRY
{
    std::vector<DOCUMENT_SEGMENT> segments;
    std::vector<DOCUMENT_POLYGON> polygons;
    std::vector<DOCUMENT_CIRCLE>  circles;

    bool Empty() const { return segments.empty() && polygons.empty() && circles.empty(); }
};


/**
 * Sink for the shared DOCUMENT_GEOMETRY walk. The walk (IterateDocumentGeometry)
 * owns the iteration order common to every renderer; each sink owns the
 * API-specific draw calls and per-primitive guards.
 *
 * Polygons are visited in two passes (every fill, then every stroke) so a
 * filled-and-stroked outline gets both and a later filled polygon does not
 * cover an earlier outline. A renderer that draws each polygon in a single
 * call (e.g. the plotter, whose PlotPoly either fills or strokes) does its
 * work in FillPolygon and leaves StrokePolygon empty.
 */
struct GEOMETRY_SINK
{
    virtual ~GEOMETRY_SINK() = default;

    virtual void FillPolygon( const DOCUMENT_POLYGON& aPoly ) = 0;
    virtual void StrokePolygon( const DOCUMENT_POLYGON& aPoly ) = 0;
    virtual void DrawSegment( const DOCUMENT_SEGMENT& aSegment ) = 0;
    virtual void DrawCircle( const DOCUMENT_CIRCLE& aCircle ) = 0;
};


/**
 * Walk a DOCUMENT_GEOMETRY in the canonical render order shared by the GAL and
 * plotter renderers: every polygon fill pass, every polygon stroke pass, then
 * segments, then circles. Empty geometry draws nothing. The sink decides how
 * each primitive is drawn and which ones to skip.
 */
inline void IterateDocumentGeometry( const DOCUMENT_GEOMETRY& aGeometry, GEOMETRY_SINK& aSink )
{
    if( aGeometry.Empty() )
        return;

    for( const DOCUMENT_POLYGON& poly : aGeometry.polygons )
        aSink.FillPolygon( poly );

    for( const DOCUMENT_POLYGON& poly : aGeometry.polygons )
        aSink.StrokePolygon( poly );

    for( const DOCUMENT_SEGMENT& segment : aGeometry.segments )
        aSink.DrawSegment( segment );

    for( const DOCUMENT_CIRCLE& circle : aGeometry.circles )
        aSink.DrawCircle( circle );
}


struct KICOMMON_API DIFF_SCENE
{
    /// Source document type. Selects the internal-unit scale the headless
    /// plotter renderer uses to size its viewport; PCB and schematic carry
    /// different IU-per-distance ratios, so a scene built from schematic
    /// coordinates must say so or the PNG/SVG output mis-scales. The GAL
    /// overlay renderer works in raw IU and ignores this. Defaults to PCB so
    /// existing board scenes keep their current output.
    DOC_KIND                 docKind = DOC_KIND::PCB;

    BOX2I                    documentBBox; // union of all change bboxes
    std::vector<SCENE_SHAPE> addedShapes;
    std::vector<SCENE_SHAPE> removedShapes;
    std::vector<SCENE_SHAPE> modifiedShapes;
    std::vector<SCENE_SHAPE> conflictShapes;

    /// Background geometry from the two source documents. Optional — the
    /// scene renders fine without it (just the change rectangles). When
    /// populated, renderers draw these first, in a dimmed style, so the
    /// diff rectangles read as annotations on the actual content.
    DOCUMENT_GEOMETRY referenceGeometry;
    DOCUMENT_GEOMETRY comparisonGeometry;
};


/**
 * Internal-unit scale a document kind's coordinates are expressed in. PCB and
 * footprint geometry are 1 nm/IU (pcbIUScale); schematic and symbol geometry
 * are 100 nm/IU (schIUScale). The headless plotter renderer uses this to size
 * its viewport so a schematic diff is not silently scaled by the PCB ratio.
 *
 * Inline so both the renderer (in `common`) and kicommon callers share one
 * mapping without a cross-library link dependency on base_units.
 */
inline const EDA_IU_SCALE& IuScaleForDocKind( DOC_KIND aKind )
{
    switch( aKind )
    {
    case DOC_KIND::SCH:
    case DOC_KIND::SYM_LIB:
        return schIUScale;

    case DOC_KIND::PCB:
    case DOC_KIND::FP_LIB:
    case DOC_KIND::FOOTPRINT:
    case DOC_KIND::UNKNOWN:
    default:
        return pcbIUScale;
    }
}


/**
 * Color theme. Defaults are tuned for light backgrounds; the interactive
 * widget overrides per the user's theme.
 */
struct KICOMMON_API DIFF_COLOR_THEME
{
    KIGFX::COLOR4D added = KIGFX::COLOR4D( 0.20, 0.65, 0.20, 1.0 );      // green
    KIGFX::COLOR4D removed = KIGFX::COLOR4D( 0.80, 0.20, 0.20, 1.0 );    // red
    KIGFX::COLOR4D modified = KIGFX::COLOR4D( 0.85, 0.65, 0.10, 1.0 );   // amber
    KIGFX::COLOR4D conflict = KIGFX::COLOR4D( 0.65, 0.20, 0.70, 1.0 );   // magenta
    KIGFX::COLOR4D background = KIGFX::COLOR4D( 0.97, 0.97, 0.97, 1.0 ); // near-white
    KIGFX::COLOR4D foreground = KIGFX::COLOR4D( 0.10, 0.10, 0.10, 1.0 ); // near-black

    /// Default color for source-document context geometry. Both sides share
    /// this neutral grey so common board elements recede behind colored
    /// added/removed/modified overlays.
    KIGFX::COLOR4D reference = KIGFX::COLOR4D( 0.38, 0.38, 0.38, 0.55 );
    KIGFX::COLOR4D comparison = KIGFX::COLOR4D( 0.38, 0.38, 0.38, 0.55 );
};


/**
 * Build a DIFF_SCENE from a DOCUMENT_DIFF, populating the shape lists and
 * computing the union bbox. Items whose bbox is empty are skipped (no
 * meaningful rectangle to draw).
 *
 * For nested ITEM_CHANGEs (footprint children), each child contributes its
 * own bbox; the parent's bbox is also added so the overall enclosing area
 * still shows up. PCB routing changes (tracks/arcs/vias) that carry the
 * same net-name refdes are collapsed into one net-level shape per kind for
 * display only; the underlying DOCUMENT_DIFF remains item-keyed for merge.
 */
KICOMMON_API DIFF_SCENE BuildScene( const DOCUMENT_DIFF& aDiff, const DIFF_COLOR_THEME& aTheme );


/**
 * Map a CHANGE_KIND to the visual category it belongs to. Both COLLISION
 * and DUPLICATE_UUID land on CONFLICT.
 */
KICOMMON_API CATEGORY CategoryFor( CHANGE_KIND aKind );


/**
 * Grow the scene's documentBBox to also include the extent of any background
 * geometry. Called by callers that populated referenceGeometry / comparison-
 * Geometry after BuildScene — without this the initial fit-to-view would
 * clip the geometry to the change cluster's bbox.
 */
KICOMMON_API void ExpandBBoxToGeometry( DIFF_SCENE& aScene );


/**
 * Compute the tight bounding box of a DOCUMENT_GEOMETRY, inflating each
 * primitive by half its stroke so the bbox actually contains the rendered
 * pixels.
 *
 * Returns std::nullopt for an empty geometry so callers can distinguish
 * "no shapes" from "shapes that collapsed to a degenerate box at origin".
 *
 * Pure function — exposed so headless renderers and tests can reason about
 * the bbox without instantiating a full DIFF_SCENE.
 */
KICOMMON_API std::optional<BOX2I> BBoxFromGeometry( const DOCUMENT_GEOMETRY& aGeometry );


/**
 * Return the union of every non-empty layer set carried by the geometry.
 * Layerless primitives are omitted; they are treated as always-visible
 * context by FilterGeometryByVisibleLayers.
 */
KICOMMON_API LSET GeometryLayerSet( const DOCUMENT_GEOMETRY& aGeometry );


/**
 * Copy geometry primitives whose layer set intersects aVisibleLayers.
 * Layerless primitives are preserved so schematic/context geometry without
 * PCB layer metadata does not disappear when board-layer filtering is active.
 */
KICOMMON_API DOCUMENT_GEOMETRY FilterGeometryByVisibleLayers( const DOCUMENT_GEOMETRY& aGeometry,
                                                              const LSET&              aVisibleLayers );


/**
 * Move all primitives from aSrc into aDst.
 *
 * Presentation code often builds document geometry one library item at a
 * time; keep the move/append details in one helper so job handlers and
 * dialogs do not duplicate three vector insertions.
 */
KICOMMON_API void AppendGeometry( DOCUMENT_GEOMETRY& aDst, DOCUMENT_GEOMETRY&& aSrc );


/**
 * Presentation predicate for PCB routing changes that should be displayed
 * as one net-level entry/shape. The underlying DOCUMENT_DIFF remains
 * item-keyed; this only affects UI/rendering grouping.
 */
KICOMMON_API bool IsRoutingNetChange( const ITEM_CHANGE& aChange );


/**
 * User-facing item label used consistently by scene tooltips and change
 * tree entries. Format: `typeName` or `typeName [refdes]`.
 */
KICOMMON_API wxString ChangeDisplayLabel( const ITEM_CHANGE& aChange );


/**
 * Map a CATEGORY to its color in a DIFF_COLOR_THEME. Pure dispatch — exposed
 * so renderers and tests can pin the theme-to-shape-color mapping without
 * driving BuildScene. A future theme that adds (or accidentally drops) a
 * category color must trip an explicit test rather than silently render
 * grey on screen.
 *
 * Out-of-range CATEGORY values trip a `wxFAIL_MSG` and return a default-
 * constructed COLOR4D (transparent black). Under QA's `wxAssertThrower`
 * install the failure throws `KI_TEST::WX_ASSERT_ERROR` instead of
 * silently returning grey.
 */
KICOMMON_API KIGFX::COLOR4D ThemeColorFor( const DIFF_COLOR_THEME& aTheme, CATEGORY aCategory );


/**
 * Build a DOCUMENT_POLYGON outlining a bounding box. Convenience for
 * extractors that want to emit an item's bbox as background context.
 * Returns an empty polygon (no points) when the box is degenerate so
 * callers can guard with !poly.outline.empty().
 */
KICOMMON_API DOCUMENT_POLYGON MakeBBoxOutline( const BOX2I& aBBox, const KIGFX::COLOR4D& aColor, int aLineWidth = 0 );


/**
 * Build a SHAPE_POLY_SET from a SCENE_SHAPE::PolygonList. Outer vector is
 * polygons, middle is contours (index 0 outline, 1+ holes), inner is points.
 * Polygons whose outline has fewer than three points are dropped. Shared by
 * the GAL and plotter renderers so the delta-region fill builds the same way
 * in interactive and headless output.
 */
KICOMMON_API SHAPE_POLY_SET PolySetFromPolygonList( const SCENE_SHAPE::PolygonList& aPolygons );


/**
 * Walk a DOCUMENT_DIFF and populate a (KIID_PATH → BOX2I) map with each
 * changed item's bbox, recursing into nested children. Empty bboxes are
 * skipped. emplace is used so existing entries in aOut are preserved when
 * the same key appears more than once across siblings. Used by the 3-way
 * merge dialog to look up where conflicts live for per-side highlight.
 */
KICOMMON_API void CollectChangeBBoxes( const DOCUMENT_DIFF& aDiff, std::map<KIID_PATH, BOX2I>& aOut );


/**
 * Read-only access to a DIFF_SCENE's shape list for a given category.
 */
KICOMMON_API const std::vector<SCENE_SHAPE>& ShapesFor( const DIFF_SCENE& aScene, CATEGORY aCategory );


/**
 * Union bbox of every visible SCENE_SHAPE whose changeId matches aChangeId.
 * Returns std::nullopt when no visible shape matches. Used by interactive
 * viewers to zoom to the currently-highlighted change.
 */
KICOMMON_API std::optional<BOX2I> HighlightedBBox( const DIFF_SCENE& aScene, const KIID_PATH& aChangeId,
                                                   const std::array<bool, CATEGORY_COUNT>& aCategoryVisible );

} // namespace KICAD_DIFF

#endif // KICAD_DIFF_SCENE_H
