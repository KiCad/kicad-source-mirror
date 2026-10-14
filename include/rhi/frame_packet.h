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
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef RHI_FRAME_PACKET_H
#define RHI_FRAME_PACKET_H

#include <rhi/rhi_handle.h>
#include <gal/definitions.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>


namespace KIGFX
{

enum class RHI_LINE_PIPELINE_KIND : uint8_t
{
    PCB_TRACKS,
    SCH_WIRES
};

/**
 * Shapes a frame draws, each with its own array and pipelines. Instanced shapes come first so arrays of
 * RHI_INSTANCED_SHAPES leave polygons out.
 */
enum RHI_SHAPE : uint8_t
{
    RHI_SHAPE_TRACK,
    RHI_SHAPE_ARC,
    RHI_SHAPE_PAD,
    RHI_SHAPE_POLYGON
};

constexpr int RHI_INSTANCED_SHAPES = RHI_SHAPE_POLYGON;
constexpr int RHI_SHAPES = RHI_SHAPE_POLYGON + 1;

/**
 * Elements of each shape, indexed by RHI_SHAPE.
 */
using RHI_SHAPE_COUNTS = std::array<uint32_t, RHI_SHAPES>;

/**
 * Grid uniform block matching the GridUBO layout in grid.frag (std140).
 */
struct alignas( 16 ) GRID_UBO_DATA
{
    float invProjection[16] = {};
    float viewport[2]       = {};
    float gridOrigin[2]     = {};
    float gridSpacing[2]    = {};   /**< visible spacing from GetVisibleGridSize */
    float gridAngle         = 0.0f;
    int   gridStyle         = 0;    /**< 0 = lines, 1 = dots, 2 = small cross */
    float gridColor[4]      = {};
    float axisColorX[4]     = {};
    float axisColorY[4]     = {};
    float minorLineWidth    = 1.0f; /**< pixels, at least 1 and drawn rounded to whole pixels */
    float majorLineWorld    = 2.0f; /**< world units, see GAL::gridMinorLineWidth() */
    int   gridTick          = 10;   /**< coarse grid multiplier */
    float renderScale       = 1.0f; /**< as FRAME_UBO_DATA::renderScale */
    float worldPixelSize    = 1.0f; /**< world units that majorLineWorld is divided by */
    float _pad0[3]          = {};

    bool operator==( const GRID_UBO_DATA& aOther ) const = default;
};


/**
 * Cursor uniform block matching the CursorUBO layout in cursor.frag (std140).
 */
struct alignas( 16 ) CURSOR_UBO_DATA
{
    float cursorPos[2]      = {};   /**< NDC [-1, 1] */
    float viewport[2]       = {};
    float crosshairColor[4] = {};
    float snapColor[4]      = {};
    float crosshairWidth    = 1.0f;
    float snapRadius        = 5.0f;
    int   showSnap          = 0;
    int   cursorType        = 1;    /**< 0 = small cross, 1 = fullscreen, 2 = diagonal 45° */
    float cursorSize        = 80.0f; /**< small cross extent in pixels */
    float _pad0[3]          = {};
};


/**
 * Per-frame uniform block matching FrameUBO in frame_ubo.glsl (std140).
 */
struct alignas( 16 ) FRAME_UBO_DATA
{
    float projection[16] = {};   /**< column-major for GLSL */
    float viewport[2]    = {};
    float viewScale      = 0.0f; /**< the view's scale, which levels of detail are compared with */
    float renderScale    = 1.0f; /**< target texels per viewport pixel, above 1 in a supersampled target */

    bool operator==( const FRAME_UBO_DATA& aOther ) const = default;
};


/**
 * One textured bitmap quad with its own vertex buffer and texture.
 */
struct BITMAP_DRAW_DESC
{
    RHI_BufferHandle  vertexBuffer;
    RHI_TextureHandle texture;
    uint32_t          vertexCount = 6;
};


/**
 * A stretch of one layer range drawn in one polarity.
 *
 * Where order matters, as on a Gerber layer, a clear run erases what earlier runs drew and a later dark run
 * draws over the cleared area.
 */
struct POLARITY_RUN
{
    bool             negative = false;
    RHI_SHAPE_COUNTS start{};
    RHI_SHAPE_COUNTS count{};

    bool Empty() const
    {
        return std::ranges::all_of( count, []( uint32_t aCount ) { return aCount == 0; } );
    }
};


/**
 * Fade of a whole layer towards a colour, laid out as the push constants of layer_dim.glsl. High contrast
 * uses it to dim an inactive layer, and elements flagged RHI_INSTANCE_UNDIMMED keep their colour.
 */
struct RHI_LAYER_DIM
{
    float toward[3] = { 0.0f, 0.0f, 0.0f };
    float keep      = 1.0f;     /**< 1 leaves the layer as it is */

    bool operator==( const RHI_LAYER_DIM& aOther ) const = default;
};

static_assert( sizeof( RHI_LAYER_DIM ) == 16, "RHI_LAYER_DIM is one vec4 of push constants" );


struct RHI_RUN_TABLE;


/**
 * One layer's instance ranges within the flat GPU buffers, normal instances before negatives.
 */
struct LAYER_RANGE
{
    float    depth = 0.0f;

    /**
     * Compositing alpha. Below 1 the layer goes through a temp FBO, and a layer of mixed alpha keeps 1
     * with per-instance alpha.
     */
    float    alpha = 1.0f;

    /**
     * Subtractive instances go through a temp FBO so their erasure stays within the layer.
     */
    bool     hasNegatives = false;

    /**
     * Picks the static (cached) or dynamic scene pool for this range.
     */
    RENDER_TARGET target = TARGET_CACHED;

    /**
     * Each shape's normal elements, then for an instanced shape its negatives.
     */
    RHI_SHAPE_COUNTS start{};
    RHI_SHAPE_COUNTS count{};
    RHI_SHAPE_COUNTS negStart{};
    RHI_SHAPE_COUNTS negCount{};

    uint32_t bitmapStart = 0;
    uint32_t bitmapCount = 0;

    /**
     * Buffers a retained scene layer owns per shape. A null handle indexes the draw descriptor's buffer.
     */
    std::array<RHI_BufferHandle, RHI_SHAPES> buffer;
    std::array<RHI_BufferHandle, RHI_SHAPES> negBuffer;

    /**
     * Depth slice the range is drawn into. Retained layers store relative depth so reordering layers
     * rewrites no geometry, and the default keeps the elements' own depth.
     */
    float depthNear = 0.0f;
    float depthFar  = 1.0f;

    /**
     * Draw straight into the target using the range's own depth slice, either taking each pixel once
     * at translucent alpha or marking knockouts in depth before the rest.
     */
    bool singleCoverage = false;
    bool depthKnockout  = false;

    /**
     * Elements flagged RHI_INSTANCE_OVERLAY_ALPHA, drawn with RHI_GAL_PIPELINES::GetPremultipliedVariant.
     */
    bool overlayAlpha = false;

    /**
     * A diff layer (GAL::StartDiffLayer) that replaces what is under it with the colour difference, as
     * gerbview's XOR mode needs.
     */
    bool difference = false;

    RHI_LAYER_DIM dim;

    /**
     * The range's runs in FRAME_DRAW_DESC::polarityRuns, none when its order does not matter.
     */
    uint32_t runStart = 0;
    uint32_t runCount = 0;

    /**
     * Bounds of the range's buffers so off-screen runs are skipped. Null draws everything.
     */
    std::shared_ptr<const RHI_RUN_TABLE> cullRuns;

    bool NeedsTempFBO() const { return alpha < 1.0f || ( hasNegatives && !depthKnockout ) || difference; }

    bool HasGeometry() const
    {
        for( int shape = 0; shape < RHI_SHAPES; ++shape )
        {
            if( count[shape] > 0 || negCount[shape] > 0 )
                return true;
        }

        return bitmapCount > 0;
    }
};


/**
 * GPU geometry of one frame, built by GAL_MODERN after EndDrawing() and passed through the FRAME_MAILBOX.
 * Its buffers are already uploaded.
 */
struct FRAME_DRAW_DESC
{
    RHI_LINE_PIPELINE_KIND linePipelineKind = RHI_LINE_PIPELINE_KIND::PCB_TRACKS;

    /**
     * Each shape's buffer, normal elements before negatives.
     */
    std::array<RHI_BufferHandle, RHI_SHAPES> buffer;
    RHI_SHAPE_COUNTS                         count{};
    RHI_SHAPE_COUNTS                         negCount{};

    std::vector<BITMAP_DRAW_DESC> bitmapDraws;
    std::vector<LAYER_RANGE>      layerRanges;
    std::vector<POLARITY_RUN>     polarityRuns;

    bool drawGrid        = false;
    bool drawCursor      = false;
    bool reuseStaticFBO  = false;
    bool reuseDynamicFBO = false;

    /**
     * Draw the grid before the layers instead of over them.
     */
    bool gridFirst = false;

    /**
     * Element depth is relative to its layer's slice in LAYER_RANGE::depthNear, and the projection scales
     * depth to RELATIVE_DEPTH_RANGE.
     */
    bool relativeDepth = false;

    /**
     * Slices share the static target's layer order mapping, so this target can be depth tested against a
     * copy of the static depth.
     */
    bool testAgainstStatic = false;

    /**
     * Largest depth offset an element can have from its layer when relativeDepth is set.
     */
    static constexpr float RELATIVE_DEPTH_RANGE = 16.0f;

    /**
     * Layer FBO scale that the present blit downsamples. Values other than 1 are ignored unless the
     * window's FBOs are sized to match.
     */
    uint32_t supersampleFactor = 1;

    /**
     * Generation of the producer's buffers this descriptor was built from.
     */
    uint64_t bufferGeneration = 0;

    float backgroundColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    FRAME_UBO_DATA  frameUBO;
    GRID_UBO_DATA   gridUBO;
    CURSOR_UBO_DATA cursorUBO;

    bool HasGeometry() const
    {
        return std::ranges::any_of( count, []( uint32_t aCount ) { return aCount > 0; } ) || !bitmapDraws.empty();
    }
};


/**
 * Everything the render thread needs to draw one frame of a window.
 */
struct FRAME_PACKET
{
    FRAME_DRAW_DESC drawDesc;

    /**
     * TARGET_CACHED ranges go to staticDrawDesc and the rest to dynamicDrawDesc.
     */
    FRAME_DRAW_DESC staticDrawDesc;
    FRAME_DRAW_DESC dynamicDrawDesc;
    bool            staticChanged  = true;
    bool            dynamicChanged = true;

    /**
     * Content generation of each pool. A displaced packet can lose a change flag but not a generation,
     * and zero leaves the decision to the flags.
     */
    uint64_t staticEpoch  = 0;
    uint64_t dynamicEpoch = 0;

    /**
     * Oldest buffer generation either descriptor uses, so resources retired before it are unreferenced.
     */
    uint64_t bufferGeneration = 0;

    /**
     * Inputs that had reached the canvas when this packet was built, so a frame answering one is never
     * held back.
     */
    uint64_t inputSerial = 0;
};


} // namespace KIGFX


#endif // RHI_FRAME_PACKET_H
