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

#ifndef GAL_MODERN_H
#define GAL_MODERN_H

#include <gal/graphics_abstraction_layer.h>
#include <gal/definitions.h>
#include <rhi/rhi_handle.h>
#include <rhi/rhi_instance.h>
#include <rhi/frame_packet.h>

#include <array>
#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

class SHAPE_ELLIPSE;

namespace KIGFX
{

class RENDER_THREAD;
class RHI_DEVICE_PROXY;

/**
 * GAL that turns drawing commands into instanced primitives for the RHI device.
 *
 * It has no wxWidgets base so it can run headless. EDA_DRAW_PANEL_GAL_MODERN does the wx integration.
 */
class GAL_API GAL_MODERN : public GAL
{
public:
    GAL_MODERN( GAL_DISPLAY_OPTIONS& aOptions, RHI_DEVICE_PROXY& aDeviceProxy );
    ~GAL_MODERN() override;

    bool IsInitialized() const override { return m_rhiInitialized; }
    bool IsDrawingMode() const override { return m_isDrawing; }
    bool HasStaticRebuildPending() const override { return m_staticDirty; }
    bool HasRetainedScene() const override { return true; }
    bool RefillsNonCachedTargets() const override { return !m_persistentTargets; }
    bool SupportsTypedPads() const override { return true; }
    bool UsesPolygonTriangulationCache() const override { return true; }

    void BeginDrawing() override;
    void EndDrawing() override;

    void ResizeScreen( int aWidth, int aHeight ) override;
    void Flush() override;
    void ClearScreen() override;

    void           SetTarget( RENDER_TARGET aTarget ) override;
    RENDER_TARGET  GetTarget() const override;
    void           ClearTarget( RENDER_TARGET aTarget ) override;

    void StartDiffLayer() override;
    void EndDiffLayer() override;
    void StartNegativesLayer() override;
    void EndNegativesLayer() override;
    void SetNegativeDrawMode( bool aSetting ) override;
    void SetScreenSpaceMode( bool aEnabled ) override { m_screenSpaceMode = aEnabled; }

    bool SupportsStrokePixelPad() const override { return true; }

    bool SupportsStrokeLengthPad() const override { return true; }

    void SetStrokeLengthPad( double aPixels ) override { m_strokeLengthPad = static_cast<float>( aPixels ); }

    void SetStrokePixelPad( double aPixels ) override
    {
        m_strokePixelPad = static_cast<uint16_t>( std::clamp( std::lround( aPixels * 16.0 ), 0L, 65535L ) );
    }

    bool SupportsStrokeLevelOfDetail() const override { return true; }

    void SetStrokeLevelOfDetail( double aLod ) override { m_strokeLod = aLod; }

    void BitmapText( const wxString& aText, const VECTOR2I& aPosition, const EDA_ANGLE& aAngle ) override;

    /** Draw bitmap text as one font atlas glyph per quad, for editors whose track shader supports it */
    void SetBitmapGlyphs( bool aEnable ) { m_bitmapGlyphs = aEnable; }

    void DrawGrid() override;
    void DrawCursor( const VECTOR2D& aCursorPosition ) override;

    int  BeginGroup() override;
    void EndGroup() override;
    std::array<uint64_t, 2> GroupContentFingerprint( int aGroupNumber ) const override;
    void DrawGroup( int aGroupNumber ) override;
    void ChangeGroupColor( int aGroupNumber, const COLOR4D& aNewColor ) override;
    void ChangeGroupDepth( int aGroupNumber, int aDepth ) override;
    void ChangeGroupUndimmed( int aGroupNumber, bool aUndimmed ) override;
    void DeleteGroup( int aGroupNumber ) override;
    void ClearCache() override;

    void SetSceneLayerState( int aLayer, int aOrder, bool aVisible, bool aHasNegatives ) override;
    void SetSceneLayerDim( int aLayer, const COLOR4D& aToward, double aKeep ) override;
    void SetSceneLayerOverlayAlpha( int aLayer ) override;
    void SetSceneDrawOrdered( bool aOrdered ) override;
    void BeginSceneLayer( int aLayer ) override;
    void SetGroupPlacement( int aGroup, int aLayer, double aLodScale, bool aShown,
                            const RETAINED_GROUP_ORDER& aOrder ) override;
    void SetSceneScale( double aScale ) override;

    /**
     * Upload scene changes made outside a drawing context, such as groups crossing their level of detail
     * on a zoom that redraws nothing.
     *
     * @return true if the static scene changed and its descriptor must be fetched again.
     */
    bool SyncScene();

    void Save() override;
    void Restore() override;
    void Transform( const MATRIX3x3D& aTransformation ) override;
    void Rotate( double aAngle ) override;
    void Translate( const VECTOR2D& aTranslation ) override;
    void Scale( const VECTOR2D& aScale ) override;

    void SetLayerDepth( double aLayerDepth ) override;

    int  GetSwapInterval() const override;

    /** Render thread used for fence-based synchronization */
    void SetRenderThread( RENDER_THREAD* aThread ) { m_renderThread = aThread; }

    /**
     * Backing-store scale factor, 2.0 on Retina. The GAL stays in logical coordinates for VIEW and hit
     * testing, so the viewport UBO needs this to size antialiasing edges in real pixels.
     */
    void SetDpiScale( float aScale ) { m_dpiScale = aScale; }

    /**
     * The other canvases lay the view out this many pixels larger than the client area. Matching them
     * keeps the picture in place across a backend switch.
     */
    void SetCentringPad( int aPad ) { m_centringPad = aPad; }

    VECTOR2I GetLayoutPixelSize() const override
    {
        return m_screenSize + VECTOR2I( m_centringPad, m_centringPad );
    }

    /** Drop all GPU handles without destroying them, before the render thread and its device go away */
    void ReleaseBuffers()
    {
        auto release = []( GEOMETRY_BUFFER_SET& bs )
        {
            bs.buffer.fill( {} );
            bs.capacity.fill( 0 );

            for( auto& bq : bs.bitmapQuads )
            {
                bq.texture      = {};
                bq.vertexBuffer = {};
            }
        };

        for( auto& bs : m_bufferSets )
            release( bs );

        for( auto& bs : m_overlaySets )
            release( bs );

        release( m_staticPool );
        release( m_gridPool );

        for( auto& [id, layer] : m_sceneLayers )
        {
            for( SCENE_ARENA<RHI_INSTANCE>& arena : layer.instances )
            {
                arena.buffer   = {};
                arena.capacity = 0;
            }

            layer.polygons.buffer   = {};
            layer.polygons.capacity = 0;
        }

        m_retiredBuffers.clear();
        m_retiredTextures.clear();

        /**
         * The dying device frees these textures itself. Forgetting them keeps ~GAL_MODERN from
         * destroying them through a dangling device.
         */
        m_bitmapTextureCache.clear();
    }

    void DrawLine( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint ) override;
    void DrawSegment( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint,
                      double aWidth ) override;
    void DrawTrack( const VECTOR2D& aStart, const VECTOR2D& aEnd,
                    double aWidth, const COLOR4D& aColor ) override;
    void DrawPad( const VECTOR2D& aCenter, double aHalfWidth, double aHalfHeight,
                  double aCornerRadius, float aShapeType, double aRotation,
                  const COLOR4D& aColor ) override;
    void DrawSegmentChain( const std::vector<VECTOR2D>& aPointList, double aWidth ) override;
    void DrawSegmentChain( const SHAPE_LINE_CHAIN& aLineChain, double aWidth ) override;

    void DrawArc( const VECTOR2D& aCenterPoint, double aRadius,
                  const EDA_ANGLE& aStartAngle, const EDA_ANGLE& aAngle ) override;
    void DrawArcSegment( const VECTOR2D& aCenterPoint, double aRadius,
                         const EDA_ANGLE& aStartAngle, const EDA_ANGLE& aAngle,
                         double aWidth, double aMaxError ) override;

    void DrawCircle( const VECTOR2D& aCenterPoint, double aRadius ) override;
    void DrawEllipse( const VECTOR2D& aCenterPoint, double aMajorRadius, double aMinorRadius,
                      const EDA_ANGLE& aRotation ) override;
    void DrawEllipseArc( const VECTOR2D& aCenterPoint, double aMajorRadius, double aMinorRadius,
                         const EDA_ANGLE& aRotation, const EDA_ANGLE& aStartAngle,
                         const EDA_ANGLE& aEndAngle ) override;
    void DrawHoleWall( const VECTOR2D& aCenterPoint, double aHoleRadius,
                       double aWallWidth ) override;
    void DrawRectangle( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint ) override;
    void DrawPolygon( const std::deque<VECTOR2D>& aPointList ) override;
    void DrawPolygon( const VECTOR2D aPointList[], int aListSize ) override;
    void DrawPolygon( const SHAPE_POLY_SET& aPolySet, bool aStrokeTriangulation = false ) override;
    void DrawPolygon( const SHAPE_LINE_CHAIN& aPolySet ) override;

    void DrawCurve( const VECTOR2D& aStartPoint, const VECTOR2D& aControlPointA,
                    const VECTOR2D& aControlPointB, const VECTOR2D& aEndPoint,
                    double aFilterValue = 0.0 ) override;

    void DrawBitmap( const BITMAP_BASE& aBitmap, double alphaBlend = 1.0 ) override;

    void DrawPolyline( const std::deque<VECTOR2D>& aPointList ) override;
    void DrawPolyline( const std::vector<VECTOR2D>& aPointList ) override;
    void DrawPolyline( const VECTOR2D aPointList[], int aListSize ) override;
    void DrawPolyline( const SHAPE_LINE_CHAIN& aLineChain ) override;
    void DrawPolylines( const std::vector<std::vector<VECTOR2D>>& aPointLists ) override;

    void DrawGlyph( const KIFONT::GLYPH& aGlyph, int aNth = 0, int aTotal = 1 ) override;

    struct BITMAP_QUAD
    {
        RHI_VERTEX        vertices[6];
        float             alpha = 1.0f;
        RHI_TextureHandle texture;
        RHI_BufferHandle  vertexBuffer;
        bool              ownsResources = true; /**< false for group copies, which own nothing */
        bool              ownsTexture   = true; /**< false when the shared cache owns the texture */
        std::array<uint64_t, 2> contentHash{};
    };

    /** Instances of each instanced shape, indexed by RHI_SHAPE */
    using SHAPE_INSTANCES = std::array<std::vector<RHI_INSTANCE>, RHI_INSTANCED_SHAPES>;

    /**
     * Arenas of one retained scene layer. Each instanced shape has a normal and a negative arena so a
     * layer draws every normal before any negative.
     */
    static constexpr int ARENA_INSTANCE_COUNT = 2 * RHI_INSTANCED_SHAPES;
    static constexpr int ARENA_POLYGON = ARENA_INSTANCE_COUNT;
    static constexpr int ARENA_COUNT = ARENA_POLYGON + 1;

    static constexpr int arenaOf( int aShape, bool aNegative ) { return 2 * aShape + ( aNegative ? 1 : 0 ); }

    struct GAL_GROUP
    {
        SHAPE_INSTANCES          instances;
        std::vector<RHI_VERTEX>  polygonVertices;
        std::vector<BITMAP_QUAD> bitmapQuads;
        mutable std::array<uint64_t, 2> contentFingerprint{};
        mutable bool                    contentFingerprintValid = false;

        /** Recorded as a knockout, so it erases instead of drawing */
        bool     negative   = false;

        /** Keeps its colour when its layer is dimmed, see ChangeGroupUndimmed */
        bool     undimmed   = false;

        /** When the group was placed in the scene, which orders a layer's polarity runs */
        uint64_t placeSeq   = 0;

        /** Layer depth the group was cached at. The scene stores each element's offset from it. */
        float    baseDepth  = 0.0f;

        /** Placement in the retained scene, set by SetGroupPlacement */
        int      sceneLayer = -1;
        double   lodScale   = 0.0;
        bool     shown      = false;
        RETAINED_GROUP_ORDER order;
        /** Entry in m_lodGroups while a zoom can show or hide the group */
        std::optional<std::multimap<double, int>::iterator> lodEntry;

        /** Slot held in each arena of sceneLayer while resident */
        bool     resident      = false;
        uint32_t residentIndex = 0;
        uint32_t slotStart[ARENA_COUNT] = {};
        uint32_t slotCount[ARENA_COUNT] = {};
    };

    /**
     * Free retired buffers and textures once the render thread has consumed a frame built after them.
     * Counting frames is not enough because an idle canvas keeps drawing its newest descriptor.
     *
     * @param aConsumedGeneration is FRAME_MAILBOX::ConsumedBufferGeneration().
     */
    void ReleaseRetiredBuffers( uint64_t aConsumedGeneration );

    /** Snapshot buffer handles, draw counts and grid and cursor uniforms after EndDrawing() */
    KIGFX::FRAME_DRAW_DESC GetDrawDesc() const;

    /** Update only the cursor UBO fields, for overlay-only frames that reuse the scene geometry */
    void PopulateCursorUBO( KIGFX::FRAME_DRAW_DESC& aDesc ) const;

    /**
     * Keep the non-cached and overlay pools across frames until ClearTarget() empties one, as the other
     * backends do, so moving an overlay item re-emits only the overlay. When off, BeginDrawing() empties both.
     */
    void SetPersistentTargets( bool aPersistent ) { m_persistentTargets = aPersistent; }

    /** Force the next Flush() to rebuild the static pool from scratch */
    void MarkStaticDirty() override { m_staticDirty = true; }

    /** Draw descriptor of the static pool. Safe to call at any time. */
    KIGFX::FRAME_DRAW_DESC GetStaticDrawDesc() const;

    /** Draw descriptor of the current dynamic ring slot, which is rebuilt every frame */
    KIGFX::FRAME_DRAW_DESC GetDynamicDrawDesc() const;

    /** Whether the last Flush() rebuilt the static pool. Reading clears it so each rebuild is seen once. */
    bool TakeStaticChanged()
    {
        bool v = m_staticRebuiltThisFlush;
        m_staticRebuiltThisFlush = false;
        return v;
    }

    /**
     * Update the camera-dependent UBOs of an existing descriptor. The camera fast path uses this rather
     * than read ring-buffered geometry state that reuse_scene frames may have overwritten.
     */
    void PopulateCameraUBOs( KIGFX::FRAME_DRAW_DESC& aDesc ) const;

protected:
    void beginUpdate() override;
    void endUpdate() override;

private:
    RHI_DEVICE_PROXY& m_deviceProxy;
    RENDER_THREAD*    m_renderThread = nullptr;

    bool          m_rhiInitialized = false;
    bool          m_isDrawing      = false;
    bool          m_isUpdating     = false;
    RENDER_TARGET m_currentTarget  = TARGET_CACHED;

    int   m_screenWidth  = 0;
    int   m_screenHeight = 0;
    float m_dpiScale     = 1.0f;   /**< see SetDpiScale() */
    int   m_centringPad  = 0;      /**< see SetCentringPad() */

    bool m_negativeMode      = false;
    bool m_screenSpaceMode   = false;
    uint16_t m_strokePixelPad = 0;      /**< see SetStrokePixelPad(), in sixteenths of a pixel */
    float    m_strokeLengthPad = 0.0f;  /**< see SetStrokeLengthPad(), 0 when off */
    double   m_strokeLod = 0.0;         /**< see SetStrokeLevelOfDetail(), 0 when off */
    double m_bitmapTextHeight = 0.0;    /**< glyph height while bitmap text is stroked, else 0 */
    bool   m_bitmapGlyphs = false;      /**< see SetBitmapGlyphs() */
    bool m_diffLayerActive = false;
    bool m_drawGrid        = false;
    bool m_drawCursor      = false;

    /** The GPU applies world to screen, so most geometry skips the matrix math on the hot path */
    bool m_transformIsIdentity = true;

    /** Exact comparison is enough since a false negative only takes the slow path */
    void recomputeTransformIdentity()
    {
        const auto& m = m_currentTransform.m_data;
        m_transformIsIdentity = m[0][0] == 1.0 && m[0][1] == 0.0 && m[0][2] == 0.0
                                && m[1][0] == 0.0 && m[1][1] == 1.0 && m[1][2] == 0.0
                                && m[2][0] == 0.0 && m[2][1] == 0.0 && m[2][2] == 1.0;
    }

    VECTOR2D xform( const VECTOR2D& aPt ) const
    {
        return m_transformIsIdentity ? aPt : VECTOR2D( m_currentTransform * aPt );
    }

    double xformScale() const
    {
        return m_transformIsIdentity ? 1.0
                                     : std::sqrt( std::abs( m_currentTransform.Determinant() ) );
    }

    double xformAngle() const
    {
        return m_transformIsIdentity ? 0.0 : std::atan2( m_currentTransform.m_data[1][0],
                                                         m_currentTransform.m_data[0][0] );
    }

    /** An instance in \p aColor at the layer depth, with the current polarity and padding */
    RHI_INSTANCE makeInstance( const COLOR4D& aColor );

    /** Append to the current target's pool */
    void emitInstance( RHI_SHAPE aShape, const RHI_INSTANCE& aInstance )
    {
        poolForCurrentTarget().instances[aShape].push_back( aInstance );
    }

    /** Retire the textures and vertex buffers the quads own, then empty the list */
    void releaseBitmapQuads( std::vector<BITMAP_QUAD>& aQuads, bool aOwnedOnly );

    /** Emit bitmap text glyph by glyph. False for text the font quads do not cover. */
    bool drawBitmapGlyphs( const wxString& aText, const VECTOR2I& aPosition, const EDA_ANGLE& aAngle );

    void addCapsuleInstance( const VECTOR2D& aStart, const VECTOR2D& aEnd,
                             double aWidth, const COLOR4D& aColor );

    /** @return the appended instance. */
    RHI_INSTANCE& addArcInstance( const VECTOR2D& aCenter, double aRadius, double aStartAngle,
                                  double aSweepAngle, double aWidth, const COLOR4D& aColor );

    RHI_INSTANCE& addPadInstance( const VECTOR2D& aCenter, double aHalfWidth, double aHalfHeight,
                                  double aCornerRadius, float aShapeType, double aRotation,
                                  const COLOR4D& aColor );

    /** Fill and stroke an ellipse per the current settings, filling an arc as a sector */
    void drawEllipseShape( const SHAPE_ELLIPSE& aEllipse );

    /** Fill an outline, closed or not, without stroking it */
    void fillOutline( SHAPE_LINE_CHAIN aOutline, const COLOR4D& aColor );

    void emitTriangulatedPolygon( const SHAPE_POLY_SET& aPolySet, const COLOR4D& aColor );

    /** A vertex filled in \p aColor at the layer depth, for emitTriangle() to place */
    RHI_VERTEX fillVertex( const COLOR4D& aColor );

    /** Append a triangle given in world coordinates under the current transform */
    void emitTriangle( const VECTOR2D& aA, const VECTOR2D& aB, const VECTOR2D& aC, const RHI_VERTEX& aFill );

    /** Stroke a chain as segments, adding a last-to-first segment when it is closed */
    void strokeLineChain( const SHAPE_LINE_CHAIN& aChain );

    /**
     * Grow \p aBuffer to hold \p aNeeded elements. On allocation failure return false and leave the buffer
     * intact, so the caller can take the GPU out-of-memory fallback instead of dropping geometry.
     */
    bool ensureBuffer( RHI_BufferHandle& aBuffer, size_t& aCapacity,
                       size_t aNeeded, size_t aStride );

    static constexpr int BUFFER_RING_SIZE = 2;

    /** A point in a pool's drawing order where the polarity of what is drawn changes */
    struct POLARITY_MARK
    {
        bool             negative;
        int              range;     /**< index of the layer range that was open */
        RHI_SHAPE_COUNTS at;        /**< elements of each shape drawn before it */
    };

    struct GEOMETRY_BUFFER_SET
    {
        SHAPE_INSTANCES          instances;
        std::vector<RHI_VERTEX>  polygonVertices;
        std::vector<BITMAP_QUAD> bitmapQuads;

        std::vector<POLARITY_MARK> polarityMarks;
        std::vector<POLARITY_RUN>  polarityRuns;            /**< built from the marks by Flush() */
        bool                       drawsNegative = false;   /**< polarity of the last element drawn */

        RHI_SHAPE_COUNTS Sizes() const
        {
            RHI_SHAPE_COUNTS sizes;

            for( int shape = 0; shape < RHI_INSTANCED_SHAPES; ++shape )
                sizes[shape] = static_cast<uint32_t>( instances[shape].size() );

            sizes[RHI_SHAPE_POLYGON] = static_cast<uint32_t>( polygonVertices.size() );
            return sizes;
        }

        bool Empty() const
        {
            return std::ranges::all_of( Sizes(), []( uint32_t aSize ) { return aSize == 0; } )
                   && bitmapQuads.empty();
        }

        void ClearGeometry()
        {
            for( std::vector<RHI_INSTANCE>& shape : instances )
                shape.clear();

            polygonVertices.clear();
            polarityMarks.clear();
            polarityRuns.clear();
            drawsNegative = false;
            lastCount = {};
            lastNegCount = {};
        }

        std::array<RHI_BufferHandle, RHI_SHAPES> buffer;
        std::array<size_t, RHI_SHAPES>           capacity{};

        /** Elements of each shape as last processed, normal and negative */
        RHI_SHAPE_COUNTS lastCount{};
        RHI_SHAPE_COUNTS lastNegCount{};

        /** Bounds of the arrays above as last processed, for culling off-screen runs */
        std::shared_ptr<const RHI_RUN_TABLE> cullRuns;
    };

    /**
     * The non-cached pool. Each dynamic target has its own ring so one can be refilled while the other
     * keeps the buffers its last frame uploaded.
     */
    GEOMETRY_BUFFER_SET m_bufferSets[BUFFER_RING_SIZE];
    int                 m_currentBufferSet = 0;

    GEOMETRY_BUFFER_SET m_overlaySets[BUFFER_RING_SIZE];
    int                 m_currentOverlaySet = 0;

    bool m_persistentTargets = false;
    bool m_nonCachedRefilled = false;   /**< non-cached pool was emptied for the frame being drawn */
    bool m_overlayRefilled   = false;

    /** Rotate a dynamic target's ring, empty the new slot and open a range at the current depth */
    void refillDynamicTarget( RENDER_TARGET aTarget );

    /** Whether drawing to the current target lands in a pool that this frame has emptied */
    bool currentTargetOpen() const
    {
        if( !m_persistentTargets || m_currentTarget == TARGET_CACHED || m_drawingGridSources )
            return true;

        return m_currentTarget == TARGET_OVERLAY ? m_overlayRefilled : m_nonCachedRefilled;
    }

    /**
     * Pool for TARGET_CACHED. It survives across frames and is cleared only when m_staticDirty is set,
     * so frames that change only dynamic content do not re-emit the whole board.
     */
    GEOMETRY_BUFFER_SET m_staticPool;
    bool                m_staticDirty = true;

    /**
     * The grids of the grid items, which the static target draws over the grid and under all else.
     * Rebuilt by each DrawGrid, and uploaded and redrawn only when that changed them.
     */
    GEOMETRY_BUFFER_SET      m_gridPool;
    std::vector<LAYER_RANGE> m_gridLayerRanges;
    bool                     m_gridPoolRefilled = false;
    bool                     m_drawingGridSources = false;   /**< emissions go to m_gridPool */
    std::array<uint64_t, 2>  m_gridPoolHash{};                /**< hash of what the GPU buffers hold */

    /** Draw the grid sources into m_gridPool, each over its own area as the Cairo canvas paints them */
    void drawGridSources();
    void drawGridSource( const GRID_SOURCE& aSrc );

    /** The world size of a screen pixel, which grid lines are measured in */
    double screenPixelSize() const;

    /** see TakeStaticChanged() */
    bool m_staticRebuiltThisFlush = false;

    /**
     * Bumped whenever a resource is retired. A descriptor built afterwards carries a generation at least
     * as new and no longer references it.
     */
    uint64_t m_bufferGeneration = 1;

    std::vector<std::pair<uint64_t, RHI_BufferHandle>>  m_retiredBuffers;
    std::vector<std::pair<uint64_t, RHI_TextureHandle>> m_retiredTextures;

    void retireBuffer( RHI_BufferHandle aBuffer );
    void retireTexture( RHI_TextureHandle aTexture );

    /**
     * Where a cached group landed in the static pool at the last full build, so a colour or depth change
     * is patched in place instead of rebuilding and uploading the whole board.
     */
    struct STATIC_GROUP_SLOT
    {
        RHI_SHAPE_COUNTS start{};
        RHI_SHAPE_COUNTS count{};
    };

    /** A group may be drawn into the static pool more than once per build, so a patch touches every slot */
    std::unordered_map<int, std::vector<STATIC_GROUP_SLOT>> m_staticGroupSlots;

    /** Half-open index span touched by patches since the last upload, empty when end <= begin */
    struct PATCH_SPAN
    {
        uint32_t begin = UINT32_MAX;
        uint32_t end   = 0;

        void add( uint32_t aStart, uint32_t aCount )
        {
            if( aCount == 0 )
                return;

            begin = std::min( begin, aStart );
            end   = std::max( end, aStart + aCount );
        }

        bool empty() const { return end <= begin; }
        void clear() { begin = UINT32_MAX; end = 0; }
    };

    /** Queue an upload of a span of @p aElements to the same place in @p aBuffer */
    template <typename T>
    void submitSpan( const std::vector<T>& aElements, RHI_BufferHandle aBuffer, const PATCH_SPAN& aSpan );

    std::array<PATCH_SPAN, RHI_SHAPES> m_staticPatches;

    /** Forget where groups sit in the static pool and the patches pending for it */
    void clearStaticSlots()
    {
        m_staticGroupSlots.clear();

        for( PATCH_SPAN& span : m_staticPatches )
            span.clear();
    }

    /**
     * Patch a cached group in place in the static pool, applying \p aInstance to each instance and
     * \p aVertex to each polygon vertex, and record the touched spans for a partial upload.
     *
     * @return true if the group was resident and patched, so the full rebuild can be skipped.
     */
    template <typename INSTANCE_FN, typename VERTEX_FN>
    bool patchStaticGroup( int aGroup, INSTANCE_FN aInstance, VERTEX_FN aVertex );

    /** Write a resident group back into its scene layer after its elements changed */
    void rewriteResidentGroup( int aGroupNumber, GAL_GROUP& aGroup );

    /**
     * Retained scene arena holding one slot per placed group, patched in place so changing one item
     * uploads only that item. Freed slots hold elements that rasterize nothing until a group of the same
     * size reuses them.
     */
    template <typename T>
    struct SCENE_ARENA
    {
        std::vector<T>   elements;      /**< CPU mirror of the GPU buffer, dead slots included */
        RHI_BufferHandle buffer;
        size_t           capacity = 0;  /**< elements the GPU buffer can hold */
        uint32_t         dead     = 0;  /**< elements sitting in freed slots */

        /**
         * Freed slot starts by slot length, reused last-in first-out so a group re-cached with the same
         * size gets its own slot back.
         */
        std::unordered_map<uint32_t, std::vector<uint32_t>> freeSlots;

        std::vector<PATCH_SPAN> dirty;  /**< spans to upload at the next Flush */
        bool                    uploadAll = false;
    };

    struct SCENE_LAYER
    {
        float depth        = 0.0f;
        bool  hasNegatives = false;
        bool  inOrder      = false;     /**< declared, so it has a place in the draw order */
        bool  visible      = true;      /**< hidden layers keep their groups but are not uploaded */

        RHI_LAYER_DIM dim;
        bool          overlayAlpha = false;   /**< see SetSceneLayerOverlayAlpha */

        SCENE_ARENA<RHI_INSTANCE> instances[ARENA_INSTANCE_COUNT];
        SCENE_ARENA<RHI_VERTEX>   polygons;

        std::vector<int> residents;         /**< groups holding a slot here */
        std::vector<int> bitmapResidents;   /**< the residents that carry bitmap quads */
        std::optional<std::unordered_multimap<size_t, int>> retainedPrefixIndex;

        /**
         * Elements per alpha value. A layer whose elements all share one translucent alpha is composited
         * with that alpha and stores opaque elements, as the full rebuild does.
         */
        uint32_t alphaCount[256] = {};
        int      bakedAlpha      = -1;      /**< hoisted alpha, or -1 when elements keep their own */
        bool     alphaStale      = false;
        bool     translucent     = false;   /**< some element is drawn with less than full alpha */

        /**
         * The layer's content in the order and polarity it was placed in. Kept only where that order
         * matters, and such a layer is drawn through the temp target.
         */
        std::vector<POLARITY_RUN> runs;
        bool                      runsStale = false;
        uint32_t                  knockoutGroups = 0;   /**< residents recorded as knockouts */

        /** Bounds of the arenas as last uploaded, for culling off-screen runs */
        std::shared_ptr<const RHI_RUN_TABLE> cullRuns;

        /** Elements each arena held when cullRuns was counted */
        size_t cullCounted[ARENA_COUNT] = {};
    };

    /**
     * Bring a layer's culling bounds up to date, recounting only runs whose elements changed.
     *
     * @return the new bounds, or null when nothing changed.
     */
    static std::shared_ptr<const RHI_RUN_TABLE> updateSceneCullRuns( SCENE_LAYER& aLayer );

    /** Work out a layer's polarity runs again after its residents changed or moved */
    void buildSceneRuns( SCENE_LAYER& aLayer );

    bool gridShown() const { return m_gridVisibility && m_gridSize.x != 0 && m_gridSize.y != 0; }

    /**
     * Whether a scene layer goes through the temp target and is composited with its alpha. Only a layer
     * whose order matters does, and the rest are drawn in their own depth slice in one pass.
     */
    static bool sceneLayerComposited( const SCENE_LAYER& aLayer ) { return !aLayer.runs.empty(); }

    uint64_t m_placeSeq = 0;
    bool     m_sceneDrawOrdered = false;

    std::unordered_map<int, SCENE_LAYER> m_sceneLayers;      /**< by VIEW layer id */
    std::vector<int>                     m_sceneOrder;       /**< layer ids in draw order */
    std::multimap<double, int>           m_lodGroups;        /**< groups with a scale threshold, by it */
    SCENE_LAYER*                         m_openSceneLayer = nullptr;
    int                                  m_openSceneLayerId = -1;
    double                               m_sceneScale     = 1.0;
    bool                                 m_sceneChanged   = false;
    bool                                 m_sceneOrderStale = false;

    /** Put the declared layers back in decreasing depth order after one was added or moved */
    void sortSceneOrder();
    bool                                 m_negativesLayer = false;

    template <typename T>
    uint32_t allocSceneSlot( SCENE_ARENA<T>& aArena, uint32_t aCount );

    template <typename T>
    void freeSceneSlot( SCENE_ARENA<T>& aArena, uint32_t aStart, uint32_t aCount );

    template <typename T>
    void compactSceneArena( SCENE_LAYER& aLayer, SCENE_ARENA<T>& aArena, int aArenaId );

    /** Upload the arena's pending spans, growing its buffer first. False on allocation failure. */
    template <typename T>
    bool uploadSceneArena( SCENE_ARENA<T>& aArena );

    void placeGroupInScene( int aGroupId, GAL_GROUP& aGroup, SCENE_LAYER& aLayer );
    void removeGroupFromScene( int aGroupId, GAL_GROUP& aGroup );

    /** Keep m_lodGroups to the shown groups whose threshold a zoom can actually cross */
    void trackGroupLod( int aGroupId, GAL_GROUP& aGroup, bool aTrack );

    /** Copy a resident group's elements into its slots, applying the layer's hoisted alpha */
    void writeGroupToScene( const GAL_GROUP& aGroup, SCENE_LAYER& aLayer );
    bool retainedTieOrderChanged( int aGroupId, GAL_GROUP& aGroup, SCENE_LAYER& aLayer );
    static std::array<uint64_t, 2> computeGroupFingerprint( const GAL_GROUP& aGroup );

    void countGroupAlpha( const GAL_GROUP& aGroup, SCENE_LAYER& aLayer, int aDelta );

    /** Fill @p aCounts with how many of a group's instances go in each instance arena */
    static void countGroupArenas( const GAL_GROUP& aGroup, uint32_t* aCounts );

    /** Empty every arena and forget residency ahead of a full rebuild. GPU buffers are kept. */
    void resetScene();

    /** Settle hoisted alpha, compact and upload. False on allocation failure. */
    bool flushScene();

    void appendSceneRanges( FRAME_DRAW_DESC& aDesc ) const;

    GEOMETRY_BUFFER_SET&       buf()       { return m_bufferSets[m_currentBufferSet]; }
    const GEOMETRY_BUFFER_SET& buf() const { return m_bufferSets[m_currentBufferSet]; }

    GEOMETRY_BUFFER_SET&       obuf()       { return m_overlaySets[m_currentOverlaySet]; }
    const GEOMETRY_BUFFER_SET& obuf() const { return m_overlaySets[m_currentOverlaySet]; }

    /** TARGET_CACHED draws to the static pool and every other target to its dynamic ring slot */
    GEOMETRY_BUFFER_SET& poolForCurrentTarget()
    {
        /**
         * A target not cleared this frame is started afresh, since what it holds was already handed to
         * the render thread.
         */
        if( m_isDrawing && m_groupDepth == 0 && !currentTargetOpen() )
            refillDynamicTarget( m_currentTarget );

        if( m_drawingGridSources )
            return m_gridPool;

        if( m_currentTarget == TARGET_CACHED )
            return m_staticPool;

        return m_currentTarget == TARGET_OVERLAY ? obuf() : buf();
    }

    const GEOMETRY_BUFFER_SET& poolForCurrentTarget() const
    {
        if( m_drawingGridSources )
            return m_gridPool;

        if( m_currentTarget == TARGET_CACHED )
            return m_staticPool;

        return m_currentTarget == TARGET_OVERLAY ? obuf() : buf();
    }

    /**
     * GPU texture per bitmap, keyed on the image KIID so redrawing reuses the upload. The texture is
     * the original image, which keeps its KIID only while its pixels are unchanged.
     */
    struct BITMAP_CACHE_ENTRY
    {
        RHI_TextureHandle texture;
        size_t            bytes   = 0; /**< texture memory, which can be less than the image takes */
        uint64_t          lastUse = 0; /**< use counter for LRU eviction */
        uint32_t          groupRefs = 0; /**< quads of cached groups drawing this texture */
        std::array<uint64_t, 2> contentHash{};
    };

    /**
     * Count a group's quads against the cached textures they draw, so a texture a cached group still
     * shows is not evicted for not being drawn again each frame.
     */
    void countBitmapRefs( const GAL_GROUP& aGroup, int aDelta );

    /**
     * Give ranges their slices of the depth range from one mapping of layer order, the same for every
     * target. False if the scene has no declared layers to take the mapping from.
     */
    bool assignSlicesByOrder( std::vector<LAYER_RANGE>& aRanges, size_t aFirst ) const;

    /** The pool is a kept target whose ranges are drawn in depth slices, see Flush() */
    bool keepsRelativeDepth( const GEOMETRY_BUFFER_SET& aPool ) const
    {
        return m_persistentTargets && &aPool != &m_staticPool && &aPool != &m_gridPool;
    }

    /** Note the polarity of what is about to be drawn, so that a pool remembers where it changed */
    void notePolarity( bool aNegative );

    bool m_groupNegative = false;   /**< the group being recorded has drawn a knockout */

    static constexpr size_t BITMAP_CACHE_MAX_ENTRIES = 64;

    std::unordered_map<KIID, BITMAP_CACHE_ENTRY> m_bitmapTextureCache;
    uint64_t m_bitmapUseCounter    = 0; /**< bumped on every cache touch */
    uint64_t m_bitmapFrameStartSeq = 0; /**< use counter at frame start, entries used since are pinned */

    /**
     * Evict least recently used entries down to the entry and byte caps, sparing those used this frame
     * since a submitted quad may still reference them.
     */
    void evictBitmapCacheEntries();

    std::unordered_map<int, GAL_GROUP> m_groups;
    int   m_groupCounter   = 0;
    int   m_currentGroupId = -1;
    int   m_groupDepth     = 0;

    /**
     * BeginGroup() swaps these with the current pool's vectors and EndGroup() swaps them back. Swapping
     * keeps valid the storage the render thread may still upload from, so groups can be rebuilt outside a
     * drawing context.
     */
    SHAPE_INSTANCES          m_groupScratchInstances;
    std::vector<RHI_VERTEX>  m_groupScratchPolygonVertices;
    std::vector<BITMAP_QUAD> m_groupScratchBitmapQuads;

    MATRIX3x3D              m_currentTransform;
    std::vector<MATRIX3x3D> m_transformStack;

    /** Kept per pool so static ranges survive BeginDrawing when m_staticDirty is false */
    std::vector<LAYER_RANGE> m_staticLayerRanges;
    std::vector<LAYER_RANGE> m_dynamicLayerRanges;
    std::vector<LAYER_RANGE> m_overlayLayerRanges;

    std::vector<LAYER_RANGE>& layerRangesForCurrentTarget()
    {
        if( m_drawingGridSources )
            return m_gridLayerRanges;

        if( m_currentTarget == TARGET_CACHED )
            return m_staticLayerRanges;

        return m_currentTarget == TARGET_OVERLAY ? m_overlayLayerRanges : m_dynamicLayerRanges;
    }

    const std::vector<LAYER_RANGE>& layerRangesForCurrentTarget() const
    {
        if( m_drawingGridSources )
            return m_gridLayerRanges;

        if( m_currentTarget == TARGET_CACHED )
            return m_staticLayerRanges;

        return m_currentTarget == TARGET_OVERLAY ? m_overlayLayerRanges : m_dynamicLayerRanges;
    }

    void beginLayerRange( float aDepth );
};


} // namespace KIGFX

#endif // GAL_MODERN_H
