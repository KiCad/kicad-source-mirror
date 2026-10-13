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

#include <gal/gal_modern.h>
#include <gal/opengl/gpu_oom_error.h>
#include <rhi/rhi_device.h>
#include <rhi/rhi_device_proxy.h>
#include <rhi/rhi_buffer.h>
#include <rhi/rhi_upload_commands.h>
#include <rhi/rhi_control_commands.h>
#include <rhi/rhi_projection.h>
#include <rhi/rhi_cull_runs.h>
#include <rhi/render_thread.h>
#include <rhi/rhi_glyph_atlas.h>
#include <bitmap_base.h>
#include <gal/bitmap_texture.h>
#include <bezier_curves.h>
#include <font/glyph_types.h>
#include <geometry/shape_ellipse.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_poly_set.h>
#include <hash.h>
#include <math/util.h>
#include <mmh3_hash.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <exception>
#include <iterator>
#include <tuple>
#include <limits>
#include <memory>
#include <cmath>
#include <utility>

#include <core/utf8.h>
#include <trace_helpers.h>
#include <wx/log.h>

#include "opengl/gl_resources.h"

using namespace KIGFX;


// Selection and highlight arithmetic can push COLOR4D components outside [0,1], and converting
// those to uint8_t unclamped is undefined behavior.
static inline uint8_t packColorChannel( double aValue )
{
    return static_cast<uint8_t>( std::clamp( aValue, 0.0, 1.0 ) * 255.0 );
}


static void hashBytes( MMH3_HASH& aHash, const void* aData, size_t aSize )
{
    aHash.addData( static_cast<const uint8_t*>( aData ), aSize );
}


static std::array<uint64_t, 2> hashDigest( MMH3_HASH& aHash )
{
    const HASH_128 digest = aHash.digest();

    return { digest.Value64[0], digest.Value64[1] };
}


// Recount only the runs holding elements written since the last count or past aCounted. Every write
// the GPU has to see marks the arena dirty, so the other runs are still correct.
template <typename T, typename COUNT>
static void updateRuns( std::vector<RHI_RUN_BOUNDS>& aRuns, const std::vector<T>& aElements,
                        const std::vector<std::pair<uint32_t, uint32_t>>& aDirty, size_t aCounted, bool aAll,
                        size_t aRunLength, COUNT aCount )
{
    const size_t size = aElements.size();
    const size_t runs = ( size + aRunLength - 1 ) / aRunLength;

    aRuns.resize( runs );

    auto recount = [&]( size_t aRun )
    {
        const size_t first = aRun * aRunLength;

        aRuns[aRun] = aCount( aElements.data() + first, std::min( aRunLength, size - first ) );
    };

    const size_t tail = aAll ? 0 : ( size == aCounted ? runs : std::min( aCounted, size ) / aRunLength );

    for( size_t run = tail; run < runs; ++run )
        recount( run );

    for( const auto& [begin, spanEnd] : aDirty )
    {
        const size_t end = std::min<size_t>( spanEnd, size );

        for( size_t run = begin / aRunLength; run < tail && run * aRunLength < end; ++run )
            recount( run );
    }
}


// Offset of a half-float depth from aBase, clamped to the range of a layer's depth slice
static uint16_t relativeDepth( uint16_t aDepth, float aBase )
{
    const float limit = FRAME_DRAW_DESC::RELATIVE_DEPTH_RANGE - 1.0f;

    return glm::packHalf1x16( std::clamp( glm::unpackHalf1x16( aDepth ) - aBase, -limit, limit ) );
}


template <typename POOL>
static void hashPool( MMH3_HASH& aHash, const POOL& aPool, const std::vector<LAYER_RANGE>& aRanges )
{
    auto mixVector = [&]( const auto& aVector )
    {
        aHash.add( static_cast<int32_t>( aVector.size() ) );
        hashBytes( aHash, aVector.data(), aVector.size() * sizeof( aVector[0] ) );
    };

    for( const std::vector<RHI_INSTANCE>& instances : aPool.instances )
        mixVector( instances );

    mixVector( aPool.polygonVertices );

    for( const LAYER_RANGE& range : aRanges )
    {
        hashBytes( aHash, range.start.data(), sizeof( range.start ) );
        hashBytes( aHash, range.count.data(), sizeof( range.count ) );
        hashBytes( aHash, &range.alpha, sizeof( range.alpha ) );
        hashBytes( aHash, &range.depth, sizeof( range.depth ) );
    }
}


template <typename IT>
static void drawPolylineSegments( GAL_MODERN& aGal, IT aBegin, IT aEnd )
{
    if( aBegin == aEnd )
        return;

    for( IT prev = aBegin, it = std::next( aBegin ); it != aEnd; ++prev, ++it )
        aGal.DrawLine( *prev, *it );
}


// SHAPE_POLY_SET holds integer internal units
template <typename IT>
static SHAPE_LINE_CHAIN roundedOutline( IT aBegin, IT aEnd )
{
    SHAPE_LINE_CHAIN outline;

    for( IT it = aBegin; it != aEnd; ++it )
        outline.Append( KiROUND( *it ) );

    return outline;
}


GAL_MODERN::GAL_MODERN( GAL_DISPLAY_OPTIONS& aOptions, RHI_DEVICE_PROXY& aDeviceProxy ) :
    GAL( aOptions ),
    m_deviceProxy( aDeviceProxy ),
    m_rhiInitialized( aDeviceProxy.IsInitialized() )
{
    // MATRIX3x3D constructs as zero, and VIEW caches groups before the first BeginDrawing
    m_currentTransform.SetIdentity();

    // The same default as the other canvases, which gerbview never overrides
    SetAxesColor( COLOR4D( BLUE ) );
}


GAL_MODERN::~GAL_MODERN()
{
    // Destroyed now rather than retired, as nothing will draw them again
    m_persistentTargets = false;

    auto destroyPool = [&]( GEOMETRY_BUFFER_SET& bs )
    {
        for( RHI_BufferHandle buffer : bs.buffer )
        {
            if( buffer )
                m_deviceProxy.DestroyBuffer( buffer );
        }

        releaseBitmapQuads( bs.bitmapQuads, true );
    };

    for( auto& bs : m_bufferSets )
        destroyPool( bs );

    for( auto& bs : m_overlaySets )
        destroyPool( bs );

    destroyPool( m_staticPool );
    destroyPool( m_gridPool );

    ReleaseRetiredBuffers( std::numeric_limits<uint64_t>::max() );

    for( auto& [id, layer] : m_sceneLayers )
    {
        for( SCENE_ARENA<RHI_INSTANCE>& arena : layer.instances )
        {
            if( arena.buffer )
                m_deviceProxy.DestroyBuffer( arena.buffer );
        }

        if( layer.polygons.buffer )
            m_deviceProxy.DestroyBuffer( layer.polygons.buffer );
    }

    for( auto& [id, entry] : m_bitmapTextureCache )
    {
        if( entry.texture )
            m_deviceProxy.DestroyTexture( entry.texture );
    }

    m_bitmapTextureCache.clear();
}


void GAL_MODERN::BeginDrawing()
{
    m_isDrawing = true;
    m_drawCursor = false;

    m_currentTransform.SetIdentity();
    m_transformIsIdentity = true;
    m_transformStack.clear();

    // Bitmap cache entries used from here on may back a submitted quad, so eviction only retires
    // entries from earlier frames.
    m_bitmapFrameStartSeq = m_bitmapUseCounter + 1;

    m_nonCachedRefilled = false;
    m_overlayRefilled   = false;

    // Without persistent targets both dynamic pools are rebuilt every frame
    if( !m_persistentTargets )
    {
        refillDynamicTarget( TARGET_NONCACHED );
        refillDynamicTarget( TARGET_OVERLAY );
    }

    // The static pool survives clean frames so Redraw() need not emit the cached layers again
    if( m_staticDirty )
    {
        m_staticPool.ClearGeometry();
        releaseBitmapQuads( m_staticPool.bitmapQuads, true );

        // The rebuild uploads the whole pool, which covers any pending in-place patches
        clearStaticSlots();
        resetScene();

        m_staticLayerRanges.clear();
    }
}


void GAL_MODERN::refillDynamicTarget( RENDER_TARGET aTarget )
{
    const bool overlay = aTarget == TARGET_OVERLAY;

    // The render thread may still be reading the previous set
    if( overlay )
        m_currentOverlaySet = ( m_currentOverlaySet + 1 ) % BUFFER_RING_SIZE;
    else
        m_currentBufferSet = ( m_currentBufferSet + 1 ) % BUFFER_RING_SIZE;

    GEOMETRY_BUFFER_SET&      pool = overlay ? obuf() : buf();
    std::vector<LAYER_RANGE>& ranges = overlay ? m_overlayLayerRanges : m_dynamicLayerRanges;

    pool.ClearGeometry();
    releaseBitmapQuads( pool.bitmapQuads, true );
    ranges.clear();

    if( overlay )
    {
        m_overlayRefilled = true;
    }
    else
    {
        // The grid is drawn with the non-cached target
        m_nonCachedRefilled = true;
        m_drawGrid = false;
    }

    // A pool opened by the first draw into it has missed the range its layer depth opened
    if( m_persistentTargets && m_isDrawing && m_currentTarget == aTarget )
        beginLayerRange( static_cast<float>( m_layerDepth ) );
}


void GAL_MODERN::EndDrawing()
{
    // Cleared first so a GPU-OOM throw from Flush() leaves no stale drawing state behind
    m_isDrawing = false;
    Flush();
}


void GAL_MODERN::ResizeScreen( int aWidth, int aHeight )
{
    m_screenWidth  = aWidth;
    m_screenHeight = aHeight;

    SetScreenSize( VECTOR2I( aWidth, aHeight ) );
    ComputeWorldScreenMatrix();
}


void GAL_MODERN::retireBuffer( RHI_BufferHandle aBuffer )
{
    // Nobody keeps a descriptor across frames, so nothing can still be drawing from it
    if( !m_persistentTargets )
    {
        m_deviceProxy.DestroyBuffer( aBuffer );
        return;
    }

    m_retiredBuffers.emplace_back( ++m_bufferGeneration, aBuffer );
}


void GAL_MODERN::retireTexture( RHI_TextureHandle aTexture )
{
    if( !m_persistentTargets )
    {
        m_deviceProxy.DestroyTexture( aTexture );
        return;
    }

    m_retiredTextures.emplace_back( ++m_bufferGeneration, aTexture );
}


void GAL_MODERN::releaseBitmapQuads( std::vector<BITMAP_QUAD>& aQuads, bool aOwnedOnly )
{
    for( const BITMAP_QUAD& bq : aQuads )
    {
        if( aOwnedOnly && !bq.ownsResources )
            continue;

        if( bq.ownsTexture && bq.texture )
            retireTexture( bq.texture );

        if( bq.vertexBuffer )
            retireBuffer( bq.vertexBuffer );
    }

    aQuads.clear();
}


void GAL_MODERN::ReleaseRetiredBuffers( uint64_t aConsumedGeneration )
{
    std::erase_if( m_retiredBuffers,
                   [&]( const std::pair<uint64_t, RHI_BufferHandle>& aRetired )
                   {
                       if( aRetired.first > aConsumedGeneration )
                           return false;

                       m_deviceProxy.DestroyBuffer( aRetired.second );
                       return true;
                   } );

    std::erase_if( m_retiredTextures,
                   [&]( const std::pair<uint64_t, RHI_TextureHandle>& aRetired )
                   {
                       if( aRetired.first > aConsumedGeneration )
                           return false;

                       m_deviceProxy.DestroyTexture( aRetired.second );
                       return true;
                   } );
}


bool GAL_MODERN::ensureBuffer( RHI_BufferHandle& aBuffer, size_t& aCapacity,
                               size_t aNeeded, size_t aStride )
{
    if( aNeeded == 0 )
        return true;

    if( aCapacity < aNeeded )
    {
        size_t newCapacity = std::max( aNeeded, aCapacity * 2 );

        // A byte size overflow is an allocation failure, so the caller escalates
        if( aStride != 0 && newCapacity > std::numeric_limits<size_t>::max() / aStride )
            return false;

        RHI_BUFFER_DESC desc;
        desc.type      = RHI_BUFFER_TYPE::VERTEX;
        desc.sizeBytes = newCapacity * aStride;

        RHI_BufferHandle newBuffer = m_deviceProxy.CreateBuffer( desc );

        // Keep the old buffer so a later frame can retry, and let the caller escalate to the GPU-OOM
        // fallback
        if( !newBuffer )
            return false;

        if( aBuffer )
            retireBuffer( aBuffer );

        aBuffer   = newBuffer;
        aCapacity = newCapacity;
    }

    return true;
}


void GAL_MODERN::Flush()
{
    auto partitionRange = []( std::vector<RHI_INSTANCE>& aInstances, uint32_t aStart, uint32_t aEnd,
                              uint32_t& aNormalCount, uint32_t& aNegStart, uint32_t& aNegCount )
    {
        auto rangeBegin = aInstances.begin() + aStart;
        auto rangeEnd   = aInstances.begin() + aEnd;

        auto it = std::stable_partition( rangeBegin, rangeEnd,
                                         []( const RHI_INSTANCE& inst )
                                         {
                                             return !inst.HasFlag( RHI_INSTANCE_NEGATIVE );
                                         } );

        aNormalCount = static_cast<uint32_t>( std::distance( rangeBegin, it ) );
        aNegStart    = aStart + aNormalCount;
        aNegCount    = static_cast<uint32_t>( std::distance( it, rangeEnd ) );
    };

    auto processPool = [&]( GEOMETRY_BUFFER_SET& aPool, std::vector<LAYER_RANGE>& aRanges )
    {
        // A kept target is drawn the way the retained scene is, each range in its own slice of
        // the depth range with its elements' depth relative to the range
        const bool             sliced = keepsRelativeDepth( aPool );
        const RHI_SHAPE_COUNTS sizes = aPool.Sizes();

        // Callers that never set a layer depth leave no ranges, which would drop every instance
        if( aRanges.empty() && !aPool.Empty() )
        {
            LAYER_RANGE synth;
            synth.target = m_currentTarget;
            aRanges.push_back( synth );
        }

        aPool.polarityRuns.clear();

        size_t mark = 0;
        bool   negative = false;

        for( size_t i = 0; i < aRanges.size(); ++i )
        {
            LAYER_RANGE&     lr = aRanges[i];
            const bool       last = i + 1 == aRanges.size();
            RHI_SHAPE_COUNTS end = last ? sizes : aRanges[i + 1].start;
            const uint32_t   bmpEnd = last ? static_cast<uint32_t>( aPool.bitmapQuads.size() )
                                           : aRanges[i + 1].bitmapStart;

            // A range that changed polarity keeps its drawing order and is drawn in runs, each
            // clear run erasing what came before it and each dark run drawing over what was cleared
            const size_t firstMark = mark;

            while( mark < aPool.polarityMarks.size() && aPool.polarityMarks[mark].range <= static_cast<int>( i ) )
                mark++;

            lr.runStart = static_cast<uint32_t>( aPool.polarityRuns.size() );
            lr.runCount = 0;

            if( negative || mark > firstMark )
            {
                POLARITY_RUN run;
                run.negative = negative;
                run.start    = lr.start;

                auto closeRun = [&]( const RHI_SHAPE_COUNTS& aAt )
                {
                    for( int shape = 0; shape < RHI_SHAPES; ++shape )
                        run.count[shape] = std::min( aAt[shape], end[shape] ) - run.start[shape];

                    if( !run.Empty() )
                        aPool.polarityRuns.push_back( run );

                    for( int shape = 0; shape < RHI_SHAPES; ++shape )
                        run.start[shape] += run.count[shape];
                };

                for( size_t m = firstMark; m < mark; ++m )
                {
                    const POLARITY_MARK& change = aPool.polarityMarks[m];

                    closeRun( change.at );
                    negative = change.negative;
                    run.negative = negative;
                }

                closeRun( end );
                lr.runCount = static_cast<uint32_t>( aPool.polarityRuns.size() ) - lr.runStart;

                // Order matters once something dark follows a knockout. Polygons need the runs
                // too, a clear one is told from a dark one only by the run it is in.
                bool ordered = false;
                bool cleared = false;

                for( uint32_t r = lr.runStart; r < lr.runStart + lr.runCount; ++r )
                {
                    const POLARITY_RUN& made = aPool.polarityRuns[r];

                    if( made.negative )
                        ordered = ordered || made.count[RHI_SHAPE_POLYGON] > 0;
                    else
                        ordered = ordered || cleared;

                    cleared = cleared || made.negative;
                }

                if( !ordered )
                {
                    aPool.polarityRuns.resize( lr.runStart );
                    lr.runCount = 0;
                }
            }

            for( int shape = 0; shape < RHI_INSTANCED_SHAPES; ++shape )
            {
                if( lr.runCount > 0 )
                {
                    lr.count[shape] = end[shape] - lr.start[shape];
                    lr.negStart[shape] = end[shape];
                    lr.negCount[shape] = 0;
                }
                else
                {
                    partitionRange( aPool.instances[shape], lr.start[shape], end[shape], lr.count[shape],
                                    lr.negStart[shape], lr.negCount[shape] );
                }
            }

            // A cached group can bring knockouts into a range that never set the negative mode itself
            lr.hasNegatives = lr.hasNegatives || lr.runCount > 0
                              || std::ranges::any_of( lr.negCount, []( uint32_t aCount ) { return aCount > 0; } );
            lr.count[RHI_SHAPE_POLYGON] = end[RHI_SHAPE_POLYGON] - lr.start[RHI_SHAPE_POLYGON];
            lr.bitmapCount = bmpEnd - lr.bitmapStart;

            // Alpha shared by a whole range moves to the range, with its elements made opaque, so the
            // composite applies it once. Mixed alpha stays per element.
            uint8_t minAlpha = 255;
            uint8_t maxAlpha = 0;
            bool    hasAny   = false;

            auto scan = [&]( const auto& aElements, uint32_t aStart, uint32_t aCount )
            {
                for( uint32_t j = aStart; j < aStart + aCount; ++j )
                {
                    minAlpha = std::min( minAlpha, aElements[j].a );
                    maxAlpha = std::max( maxAlpha, aElements[j].a );
                    hasAny = true;
                }
            };

            auto scanSpans = [&]( const RHI_SHAPE_COUNTS& aStart, const RHI_SHAPE_COUNTS& aCount )
            {
                for( int shape = 0; shape < RHI_INSTANCED_SHAPES; ++shape )
                    scan( aPool.instances[shape], aStart[shape], aCount[shape] );

                scan( aPool.polygonVertices, aStart[RHI_SHAPE_POLYGON], aCount[RHI_SHAPE_POLYGON] );
            };

            // A knockout covers fully whatever the layer's alpha, so only what is drawn counts
            if( lr.runCount > 0 )
            {
                for( uint32_t r = lr.runStart; r < lr.runStart + lr.runCount; ++r )
                {
                    if( !aPool.polarityRuns[r].negative )
                        scanSpans( aPool.polarityRuns[r].start, aPool.polarityRuns[r].count );
                }
            }
            else
            {
                scanSpans( lr.start, lr.count );
            }

            if( sliced )
            {
                // A translucent pixel takes a layer once, however many of its elements overlap there
                lr.alpha          = 1.0f;
                lr.singleCoverage = hasAny && minAlpha < 255;
                lr.depthKnockout  = lr.hasNegatives && lr.runCount == 0;

                const bool hard = lr.singleCoverage || lr.depthKnockout;

                for( int shape = 0; shape < RHI_INSTANCED_SHAPES; ++shape )
                {
                    for( uint32_t j = lr.start[shape]; j < end[shape]; ++j )
                    {
                        RHI_INSTANCE& inst = aPool.instances[shape][j];

                        inst.depth = relativeDepth( inst.depth, lr.depth );

                        if( hard )
                            inst.SetFlag( RHI_INSTANCE_HARD_EDGE );
                    }
                }

                for( uint32_t j = lr.start[RHI_SHAPE_POLYGON]; j < end[RHI_SHAPE_POLYGON]; ++j )
                    aPool.polygonVertices[j].u = relativeDepth( aPool.polygonVertices[j].u, lr.depth );
            }
            else if( hasAny && minAlpha == maxAlpha && minAlpha < 255 )
            {
                lr.alpha = static_cast<float>( minAlpha ) / 255.0f;

                for( int shape = 0; shape < RHI_INSTANCED_SHAPES; ++shape )
                {
                    for( uint32_t j = lr.start[shape]; j < end[shape]; ++j )
                        aPool.instances[shape][j].a = 255;
                }

                for( uint32_t j = lr.start[RHI_SHAPE_POLYGON]; j < end[RHI_SHAPE_POLYGON]; ++j )
                    aPool.polygonVertices[j].a = 255;
            }
            else
            {
                lr.alpha = 1.0f;
            }
        }

        aPool.lastCount = {};
        aPool.lastNegCount = {};

        for( const LAYER_RANGE& lr : aRanges )
        {
            for( int shape = 0; shape < RHI_INSTANCED_SHAPES; ++shape )
            {
                aPool.lastCount[shape] += lr.count[shape];
                aPool.lastNegCount[shape] += lr.negCount[shape];
            }
        }

        aPool.lastCount[RHI_SHAPE_POLYGON] = sizes[RHI_SHAPE_POLYGON];
    };

    // Content that rides the camera path through a zoom is culled against each frame's view
    auto makeCullRuns = []( GEOMETRY_BUFFER_SET& aPool )
    {
        auto table = std::make_shared<RHI_RUN_TABLE>();

        for( int shape = 0; shape < RHI_INSTANCED_SHAPES; ++shape )
        {
            const std::vector<RHI_INSTANCE>& instances = aPool.instances[shape];

            updateRuns( table->runs[shape], instances, {}, 0, true, RHI_CULL_RUN_LENGTH,
                        [shape]( const RHI_INSTANCE* aFirst, size_t aCount )
                        {
                            return RhiInstanceRun( aFirst, aCount, RHI_SHAPE( shape ) );
                        } );

            // A pool's negatives sit in the same arrays as the rest
            table->negRuns[shape] = table->runs[shape];
        }

        updateRuns( table->runs[RHI_SHAPE_POLYGON], aPool.polygonVertices, {}, 0, true, RHI_CULL_VERTEX_RUN_LENGTH,
                    &RhiVertexRun );

        aPool.cullRuns = std::move( table );
    };

    // A dynamic pool that was not refilled keeps the ranges, counts and buffers of its last frame
    if( m_nonCachedRefilled )
    {
        processPool( buf(), m_dynamicLayerRanges );
        makeCullRuns( buf() );
    }

    if( m_overlayRefilled )
    {
        processPool( obuf(), m_overlayLayerRanges );
        makeCullRuns( obuf() );
    }

    // Likewise the static pool on clean frames
    if( m_staticDirty )
        processPool( m_staticPool, m_staticLayerRanges );

    // The grid pool is drawn again on every DrawGrid, but takes a static redraw only when it changed
    bool gridChanged = false;

    if( m_gridPoolRefilled )
    {
        processPool( m_gridPool, m_gridLayerRanges );
        makeCullRuns( m_gridPool );

        MMH3_HASH hash;

        hashPool( hash, m_gridPool, m_gridLayerRanges );

        const std::array<uint64_t, 2> digest = hashDigest( hash );

        gridChanged = digest != m_gridPoolHash;
        m_gridPoolHash = digest;
        m_gridPoolRefilled = false;
    }

    bool allocationFailed = !flushScene();

    if( !m_renderThread )
    {
        m_staticRebuiltThisFlush = m_staticDirty || m_sceneChanged || gridChanged;
        m_staticDirty = false;
        m_sceneChanged = false;
        return;
    }

    // Adopting moves the elements in without a copy, which suits only pools refilled before they are
    // read again. The static pool is copied as it is kept for patching.
    auto uploadBatch = [&]( auto& aElements, RHI_BufferHandle& aBuffer, size_t& aCapacity, bool aAdopt )
    {
        if( aElements.empty() )
            return;

        using ElemType = typename std::decay_t<decltype( aElements )>::value_type;
        size_t count = aElements.size();

        if( !ensureBuffer( aBuffer, aCapacity, count, sizeof( ElemType ) ) || !aBuffer )
        {
            allocationFailed = true;
            return;
        }

        RHI_GEOMETRY_UPLOAD upload;

        if( aAdopt )
            RhiUploadAdoptStaging( upload, std::move( aElements ) );
        else
            RhiUploadCopyStaging( upload, aElements.data(), count * sizeof( ElemType ) );

        upload.targetBuffer = aBuffer;
        upload.targetOffset = 0;
        m_renderThread->SubmitUpload( std::move( upload ) );
    };

    auto uploadPool = [&]( GEOMETRY_BUFFER_SET& aPool, bool aAdopt )
    {
        for( int shape = 0; shape < RHI_INSTANCED_SHAPES; ++shape )
            uploadBatch( aPool.instances[shape], aPool.buffer[shape], aPool.capacity[shape], aAdopt );

        uploadBatch( aPool.polygonVertices, aPool.buffer[RHI_SHAPE_POLYGON], aPool.capacity[RHI_SHAPE_POLYGON],
                     aAdopt );
    };

    if( m_nonCachedRefilled )
        uploadPool( buf(), true );

    if( m_overlayRefilled )
        uploadPool( obuf(), true );

    if( gridChanged )
        uploadPool( m_gridPool, true );

    if( m_staticDirty )
        uploadPool( m_staticPool, false );

    // A clean frame uploads only the spans that in-place patches touched
    bool patchedStatic = false;

    if( !m_staticDirty )
    {
        auto uploadPatch = [&]( const auto& aElements, RHI_BufferHandle aBuffer, PATCH_SPAN& aSpan )
        {
            if( aSpan.empty() )
                return;

            // A span that no longer fits the pool cannot be uploaded, so rebuild rather than drop it
            if( !aBuffer || aSpan.end > aElements.size() )
            {
                aSpan.clear();
                m_staticDirty = true;
                return;
            }

            submitSpan( aElements, aBuffer, aSpan );
            patchedStatic = true;
            aSpan.clear();
        };

        for( int shape = 0; shape < RHI_INSTANCED_SHAPES; ++shape )
            uploadPatch( m_staticPool.instances[shape], m_staticPool.buffer[shape], m_staticPatches[shape] );

        uploadPatch( m_staticPool.polygonVertices, m_staticPool.buffer[RHI_SHAPE_POLYGON],
                     m_staticPatches[RHI_SHAPE_POLYGON] );
    }

    // A patch needs the static target drawn again just as a rebuild does
    m_staticRebuiltThisFlush = m_staticDirty || patchedStatic || m_sceneChanged || gridChanged;
    m_staticDirty            = false;
    m_sceneChanged           = false;

    // GPU-OOM makes the draw panel leave this canvas. EndDrawing() runs from a destructor, where a
    // second exception during unwinding would call std::terminate.
    if( allocationFailed && std::uncaught_exceptions() == 0 )
        throw GPU_OOM_ERROR( "RHI geometry buffer allocation failed (GPU out of memory)" );
}


void GAL_MODERN::ClearScreen()
{
    for( GEOMETRY_BUFFER_SET* pool : { &buf(), &obuf(), &m_staticPool, &m_gridPool } )
    {
        pool->ClearGeometry();
        releaseBitmapQuads( pool->bitmapQuads, true );
    }

    m_staticLayerRanges.clear();
    m_dynamicLayerRanges.clear();
    m_overlayLayerRanges.clear();
    m_gridLayerRanges.clear();
    m_gridPoolRefilled = true;

    // The static pool is empty, so its group slots and pending patches are meaningless
    clearStaticSlots();
    resetScene();

    m_staticDirty = true;

    m_drawGrid   = false;
    m_drawCursor = false;
}


void GAL_MODERN::SetTarget( RENDER_TARGET aTarget )
{
    // A range belongs to one target, so a target change opens a range even at the same depth. The
    // depth carries across the switch.
    if( m_currentTarget != aTarget )
    {
        float depth = 0.0f;

        if( !m_staticLayerRanges.empty() )
            depth = m_staticLayerRanges.back().depth;

        if( !m_dynamicLayerRanges.empty() )
            depth = m_dynamicLayerRanges.back().depth;

        if( m_currentTarget == TARGET_OVERLAY && !m_overlayLayerRanges.empty() )
            depth = m_overlayLayerRanges.back().depth;

        m_currentTarget = aTarget;

        if( !m_staticLayerRanges.empty() || !m_dynamicLayerRanges.empty() || !m_overlayLayerRanges.empty() )
            beginLayerRange( depth );
    }
}


void GAL_MODERN::SetLayerDepth( double aLayerDepth )
{
    GAL::SetLayerDepth( aLayerDepth );

    float                           depth      = static_cast<float>( aLayerDepth );
    const std::vector<LAYER_RANGE>& ranges     = layerRangesForCurrentTarget();

    if( ranges.empty() || ranges.back().depth != depth )
        beginLayerRange( depth );
}


void GAL_MODERN::beginLayerRange( float aDepth )
{
    // VIEW sets target and depth before caching a group outside a frame. A range opened then holds no
    // geometry and would pile up, as the pools are not rebuilt for every changed item.
    if( m_isUpdating && !m_isDrawing )
        return;

    // A kept target has no use for a range until something draws into it, which opens one
    if( m_isDrawing && !currentTargetOpen() )
        return;

    const GEOMETRY_BUFFER_SET& pool = std::as_const( *this ).poolForCurrentTarget();

    LAYER_RANGE range;
    range.depth       = aDepth;
    range.target      = m_currentTarget;
    range.start       = pool.Sizes();
    range.bitmapStart = static_cast<uint32_t>( pool.bitmapQuads.size() );
    range.difference  = m_diffLayerActive;

    layerRangesForCurrentTarget().push_back( range );
}


RENDER_TARGET GAL_MODERN::GetTarget() const
{
    return m_currentTarget;
}


void GAL_MODERN::ClearTarget( RENDER_TARGET aTarget )
{
    if( !m_persistentTargets || !m_isDrawing )
        return;

    if( aTarget == TARGET_OVERLAY && !m_overlayRefilled )
        refillDynamicTarget( TARGET_OVERLAY );
    else if( aTarget == TARGET_NONCACHED && !m_nonCachedRefilled )
        refillDynamicTarget( TARGET_NONCACHED );
}


void GAL_MODERN::StartDiffLayer()
{
    // Only the non-cached and overlay ranges carry this. A retained scene layer is not differenced,
    // which is why gerbview moves its diff layers off the cached target.
    m_diffLayerActive = true;

    // A target not yet opened gets the flag with the range its first draw opens
    if( m_isDrawing && !currentTargetOpen() )
        return;

    std::vector<LAYER_RANGE>&  ranges = layerRangesForCurrentTarget();
    const GEOMETRY_BUFFER_SET& pool = std::as_const( *this ).poolForCurrentTarget();

    // The range SetLayerDepth opened for this layer holds nothing yet
    const bool fresh = !ranges.empty() && ranges.back().start == pool.Sizes()
                       && ranges.back().bitmapStart == pool.bitmapQuads.size();

    if( fresh )
        ranges.back().difference = true;
    else
        beginLayerRange( static_cast<float>( m_layerDepth ) );
}


void GAL_MODERN::EndDiffLayer()
{
    m_diffLayerActive = false;

    if( m_isDrawing && !currentTargetOpen() )
        return;

    // Whatever is drawn next at the same depth is not part of the diff layer
    const std::vector<LAYER_RANGE>& ranges = layerRangesForCurrentTarget();

    if( !ranges.empty() && ranges.back().difference )
        beginLayerRange( static_cast<float>( m_layerDepth ) );
}


void GAL_MODERN::StartNegativesLayer()
{
    m_negativeMode   = true;
    m_negativesLayer = true;

    std::vector<LAYER_RANGE>& ranges = layerRangesForCurrentTarget();

    if( !ranges.empty() )
        ranges.back().hasNegatives = true;
}


void GAL_MODERN::EndNegativesLayer()
{
    m_negativeMode   = false;
    m_negativesLayer = false;
}


void GAL_MODERN::notePolarity( bool aNegative )
{
    // A group being recorded keeps its polarity, and the pool learns it when the group is drawn
    if( m_currentGroupId >= 0 )
    {
        m_groupNegative = m_groupNegative || aNegative;
        return;
    }

    GEOMETRY_BUFFER_SET& pool = poolForCurrentTarget();

    if( pool.drawsNegative == aNegative )
        return;

    pool.drawsNegative = aNegative;
    pool.polarityMarks.push_back(
            { aNegative, static_cast<int>( layerRangesForCurrentTarget().size() ) - 1, pool.Sizes() } );
}


void GAL_MODERN::SetNegativeDrawMode( bool aSetting )
{
    m_negativeMode = aSetting;

    std::vector<LAYER_RANGE>& ranges = layerRangesForCurrentTarget();

    if( aSetting && !ranges.empty() )
        ranges.back().hasNegatives = true;
}


void GAL_MODERN::DrawGrid()
{
    drawGridSources();

    // The grid pass also draws the axes, which the other canvases show without the grid
    if( !gridShown() && !m_axesEnabled )
        return;

    m_drawGrid = true;
}


double GAL_MODERN::screenPixelSize() const
{
    const VECTOR2D pixelScale = m_screenWorldMatrix.GetScale();

    return std::min( std::abs( pixelScale.x ), std::abs( pixelScale.y ) );
}


void GAL_MODERN::drawGridSources()
{
    m_gridPool.ClearGeometry();
    m_gridLayerRanges.clear();
    m_gridPoolRefilled = true;

    if( m_gridSources.empty() )
        return;

    const RENDER_TARGET target = m_currentTarget;
    const double        depth = m_layerDepth;

    m_currentTarget = TARGET_CACHED;
    m_drawingGridSources = true;

    {
        GAL_SCOPED_ATTRS attrs( *this, GAL_SCOPED_ATTRS::STROKE_FILL );

        // Each bounded grid clears its area of the grids under it, so painting from last to first in
        // precedence leaves the first on top. A grid being edited goes over the others without hiding them.
        for( auto it = m_gridSources.rbegin(); it != m_gridSources.rend(); ++it )
        {
            if( !it->unbounded && !it->highlighted )
                drawGridSource( *it );
        }

        for( auto it = m_gridSources.rbegin(); it != m_gridSources.rend(); ++it )
        {
            if( !it->unbounded && it->highlighted )
                drawGridSource( *it );
        }
    }

    m_drawingGridSources = false;
    m_currentTarget = target;
    GAL::SetLayerDepth( depth );
}


void GAL_MODERN::drawGridSource( const GRID_SOURCE& aSrc )
{
    const COLOR4D      color = gridSourceColor( aSrc );
    const GRID_LATTICE lattice = gridLattice( aSrc );
    COLOR4D            wash = m_clearColor;

    if( aSrc.highlighted )
        wash.a = GRID_DIM_ALPHA;

    const double pixel = screenPixelSize();
    const double minorWidth = gridMinorLineWidth( pixel, m_dpiScale );
    const double majorWidth = 2.0 * minorWidth;

    // The static target draws a range kind by kind, so each step takes its own range to paint over the
    // last. Their slice puts them all at the far end of the depth range.
    auto nextStep = [&]()
    {
        GAL::SetLayerDepth( 0.0 );
        beginLayerRange( 0.0f );
    };

    nextStep();
    SetIsFill( true );
    SetIsStroke( false );
    SetFillColor( wash );
    drawGridCoverageShape( aSrc );

    // Only what a pan within one cell could show, so a huge grid costs no more than the view and the
    // geometry stays the same while the view moves within a cell.
    const BOX2D  view = gridScreenBBox( aSrc );
    const double cell = std::max( view.GetWidth(), view.GetHeight() ) / 2.0;
    const double left = ( std::floor( view.GetLeft() / cell ) - 1.0 ) * cell;
    const double top = ( std::floor( view.GetTop() / cell ) - 1.0 ) * cell;
    const double right = ( std::ceil( view.GetRight() / cell ) + 1.0 ) * cell;
    const double bottom = ( std::ceil( view.GetBottom() / cell ) + 1.0 ) * cell;
    const BOX2D  reach( VECTOR2D( left, top ), VECTOR2D( right - left, bottom - top ) );

    auto dotAt = [&]( const VECTOR2D& aPos, bool aMajorX, bool aMajorY )
    {
        if( !reach.Contains( aPos ) )
            return;

        const VECTOR2D half( ( aMajorX ? majorWidth : minorWidth ) / 2.0, ( aMajorY ? majorWidth : minorWidth ) / 2.0 );
        DrawRectangle( aPos - half, aPos + half );
    };

    auto crossAt = [&]( const VECTOR2D& aPos, bool aMajor, double aArmAngle )
    {
        if( !reach.Contains( aPos ) )
            return;

        const double   width = aMajor ? majorWidth : minorWidth;
        const VECTOR2D arm1( std::cos( aArmAngle ) * 2.0 * width, std::sin( aArmAngle ) * 2.0 * width );
        const VECTOR2D arm2( -arm1.y, arm1.x );

        SetLineWidth( width );
        DrawLine( aPos - arm1, aPos + arm1 );
        DrawLine( aPos - arm2, aPos + arm2 );
    };

    nextStep();
    SetIsFill( aSrc.style == GRID_STYLE::DOTS );
    SetIsStroke( aSrc.style != GRID_STYLE::DOTS );
    SetFillColor( color );
    SetStrokeColor( color );

    Save();
    Translate( aSrc.origin );
    Rotate( -aSrc.orientation );

    if( aSrc.kind == GRID_SOURCE::KIND::POLAR && aSrc.extent.x > 0.0 && aSrc.extent.y > 0.0 )
    {
        const double rMax = aSrc.extent.x;
        const double phiMax = aSrc.extent.y;
        const double dr = lattice.pitch.x;
        const double dPhi = lattice.pitch.y;

        if( aSrc.style == GRID_STYLE::LINES )
        {
            int rIdx = 1;

            for( double r = dr; r <= rMax + 1e-6; r += dr, ++rIdx )
            {
                SetLineWidth( lattice.IsMajor( rIdx ) ? majorWidth : minorWidth );
                DrawArc( VECTOR2D( 0, 0 ), r, ANGLE_0, EDA_ANGLE( phiMax, RADIANS_T ) );
            }

            int pIdx = 0;

            for( double phi = 0; phi <= phiMax + 1e-6; phi += dPhi, ++pIdx )
            {
                SetLineWidth( lattice.IsMajor( pIdx ) ? majorWidth : minorWidth );
                DrawLine( VECTOR2D( 0, 0 ), VECTOR2D( rMax * std::cos( phi ), rMax * std::sin( phi ) ) );
            }
        }
        else
        {
            int rIdx = 0;

            for( double r = 0; r <= rMax + 1e-6; r += dr, ++rIdx )
            {
                int pIdx = 0;

                for( double phi = 0; phi <= phiMax + 1e-6; phi += dPhi, ++pIdx )
                {
                    const VECTOR2D pos( r * std::cos( phi ), r * std::sin( phi ) );

                    if( aSrc.style == GRID_STYLE::DOTS )
                        dotAt( pos, lattice.IsMajor( rIdx ), lattice.IsMajor( pIdx ) );
                    else
                        crossAt( pos, lattice.IsMajor( rIdx ) && lattice.IsMajor( pIdx ), phi );
                }
            }
        }
    }
    else if( aSrc.kind == GRID_SOURCE::KIND::CARTESIAN )
    {
        const double dx = lattice.pitch.x;
        const double dy = lattice.pitch.y;

        const double xMin = std::max( reach.GetLeft(), -aSrc.extent.x );
        const double xMax = std::min( reach.GetRight(), aSrc.extent.x );
        const double yMin = std::max( reach.GetTop(), -aSrc.extent.y );
        const double yMax = std::min( reach.GetBottom(), aSrc.extent.y );

        const int ixMin = static_cast<int>( std::ceil( xMin / dx ) );
        const int ixMax = static_cast<int>( std::floor( xMax / dx ) );
        const int iyMin = static_cast<int>( std::ceil( yMin / dy ) );
        const int iyMax = static_cast<int>( std::floor( yMax / dy ) );

        if( aSrc.style == GRID_STYLE::LINES )
        {
            for( int ix = ixMin; ix <= ixMax; ++ix )
            {
                SetLineWidth( lattice.IsMajor( ix ) ? majorWidth : minorWidth );
                DrawLine( VECTOR2D( ix * dx, yMin ), VECTOR2D( ix * dx, yMax ) );
            }

            for( int iy = iyMin; iy <= iyMax; ++iy )
            {
                SetLineWidth( lattice.IsMajor( iy ) ? majorWidth : minorWidth );
                DrawLine( VECTOR2D( xMin, iy * dy ), VECTOR2D( xMax, iy * dy ) );
            }
        }
        else
        {
            for( int ix = ixMin; ix <= ixMax; ++ix )
            {
                for( int iy = iyMin; iy <= iyMax; ++iy )
                {
                    if( aSrc.style == GRID_STYLE::DOTS )
                        dotAt( VECTOR2D( ix * dx, iy * dy ), lattice.IsMajor( ix ), lattice.IsMajor( iy ) );
                    else
                        crossAt( VECTOR2D( ix * dx, iy * dy ), lattice.IsMajor( ix ) && lattice.IsMajor( iy ), 0.0 );
                }
            }
        }
    }

    Restore();

    // Outlined, so that a grid too sparse to show still reads as an area
    nextStep();
    SetIsFill( false );
    SetIsStroke( true );
    SetStrokeColor( color.Darkened( GRID_EDGE_DARKEN ) );
    SetLineWidth( pixel / m_dpiScale );
    drawGridCoverageShape( aSrc );
}


void GAL_MODERN::DrawCursor( const VECTOR2D& aCursorPosition )
{
    m_cursorPosition = aCursorPosition;
    m_drawCursor = true;
}


void GAL_MODERN::DrawLine( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint )
{
    addCapsuleInstance( aStartPoint, aEndPoint, m_lineWidth, m_strokeColor );
}


void GAL_MODERN::DrawSegment( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint,
                               double aWidth )
{
    if( m_isFillEnabled || aWidth == 1.0 )
    {
        addCapsuleInstance( aStartPoint, aEndPoint, aWidth, m_fillColor );
        return;
    }

    // The OpenGL canvas compares in float, and draws a segment of no length as a circle
    if( static_cast<float>( aStartPoint.x ) == static_cast<float>( aEndPoint.x )
        && static_cast<float>( aStartPoint.y ) == static_cast<float>( aEndPoint.y ) )
    {
        DrawCircle( aStartPoint, aWidth / 2.0 );
        return;
    }

    // Outlined as the other canvases do it, two hairline sides and a semicircle at each end
    const double   halfWidth = aWidth / 2.0;
    const VECTOR2D delta = aEndPoint - aStartPoint;
    const double   angle = std::atan2( delta.y, delta.x );
    const VECTOR2D side = VECTOR2D( -delta.y, delta.x ).Resize( halfWidth );

    addCapsuleInstance( aStartPoint + side, aEndPoint + side, 1.0, m_strokeColor );
    addCapsuleInstance( aStartPoint - side, aEndPoint - side, 1.0, m_strokeColor );
    addArcInstance( aStartPoint, halfWidth, angle + M_PI / 2.0, M_PI, 1.0, m_strokeColor );
    addArcInstance( aEndPoint, halfWidth, angle - M_PI / 2.0, M_PI, 1.0, m_strokeColor );
}


void GAL_MODERN::DrawTrack( const VECTOR2D& aStart, const VECTOR2D& aEnd,
                             double aWidth, const COLOR4D& aColor )
{
    addCapsuleInstance( aStart, aEnd, aWidth, aColor );
}


void GAL_MODERN::DrawPad( const VECTOR2D& aCenter, double aHalfWidth, double aHalfHeight,
                           double aCornerRadius, float aShapeType, double aRotation,
                           const COLOR4D& aColor )
{
    addPadInstance( aCenter, aHalfWidth, aHalfHeight, aCornerRadius, aShapeType, aRotation,
                   aColor );
}


void GAL_MODERN::DrawSegmentChain( const std::vector<VECTOR2D>& aPointList, double aWidth )
{
    for( size_t i = 1; i < aPointList.size(); ++i )
    {
        DrawSegment( aPointList[i - 1], aPointList[i], aWidth );
    }
}


void GAL_MODERN::DrawSegmentChain( const SHAPE_LINE_CHAIN& aLineChain, double aWidth )
{
    // A closed chain of one point counts one segment, from the point to itself
    if( aLineChain.PointCount() < 2 )
        return;

    for( int i = 0; i < aLineChain.SegmentCount(); ++i )
    {
        const SEG seg = aLineChain.CSegment( i );
        DrawSegment( VECTOR2D( seg.A ), VECTOR2D( seg.B ), aWidth );
    }
}


void GAL_MODERN::addCapsuleInstance( const VECTOR2D& aStart, const VECTOR2D& aEnd,
                                      double aWidth, const COLOR4D& aColor )
{
    VECTOR2D start = xform( aStart );
    VECTOR2D end   = xform( aEnd );
    double   scale = xformScale();

    VECTOR2D delta = end - start;
    double   length = delta.EuclideanNorm();
    double   halfWidth = ( aWidth * scale ) / 2.0;
    double   halfLength = length / 2.0;
    double   rotation = std::atan2( delta.y, delta.x );

    // Screen-space elements keep their pixel size across zoom, as the vertex shader rebuilds world
    // sizes from the live projection
    if( m_screenSpaceMode && m_worldScale > 0 )
    {
        halfWidth  *= m_worldScale;
        halfLength *= m_worldScale;
    }

    VECTOR2D midpoint( ( start.x + end.x ) / 2.0, ( start.y + end.y ) / 2.0 );

    RHI_INSTANCE inst = makeInstance( aColor );
    inst.SetPosition( static_cast<float>( midpoint.x ), static_cast<float>( midpoint.y ) );
    inst.SetRotation( static_cast<float>( rotation ) );
    inst.SetParam( 0, static_cast<float>( halfWidth ) );
    inst.SetParam( 1, static_cast<float>( halfLength ) );

    if( m_screenSpaceMode )
        inst.SetFlag( RHI_INSTANCE_SCREEN_SPACE );

    if( m_bitmapTextHeight > 0 )
    {
        inst.SetFlag( RHI_INSTANCE_FADE_SMALL );
        inst.SetParam( 2, static_cast<float>( m_bitmapTextHeight * scale ) );
    }
    else if( m_strokeLengthPad > 0.0f )
    {
        inst.SetFlag( RHI_INSTANCE_LENGTH_PAD );
        inst.SetParam( 2, m_strokeLengthPad );
    }

    if( m_strokeLod > 0.0 )
    {
        inst.SetFlag( RHI_INSTANCE_SCALE_LOD );
        inst.SetParam( 3, static_cast<float>( m_strokeLod ) );
    }

    emitInstance( RHI_SHAPE_TRACK, inst );
}


RHI_INSTANCE GAL_MODERN::makeInstance( const COLOR4D& aColor )
{
    RHI_INSTANCE inst;
    inst.SetColor( packColorChannel( aColor.r ), packColorChannel( aColor.g ), packColorChannel( aColor.b ),
                   packColorChannel( aColor.a ) );
    inst.SetDepth( static_cast<float>( m_layerDepth ) );

    // A knockout erases by coverage, whatever colour its item is painted in
    if( m_negativeMode )
    {
        inst.a = 255;
        inst.SetFlag( RHI_INSTANCE_NEGATIVE );
    }

    inst.pixelPad = m_strokePixelPad;
    notePolarity( m_negativeMode );

    if( m_groupDepth == 0 && m_currentTarget == TARGET_OVERLAY )
        inst.SetFlag( RHI_INSTANCE_OVERLAY_ALPHA );

    return inst;
}


void GAL_MODERN::DrawArc( const VECTOR2D& aCenterPoint, double aRadius,
                           const EDA_ANGLE& aStartAngle, const EDA_ANGLE& aAngle )
{
    if( aRadius <= 0 )
        return;

    if( m_isFillEnabled )
    {
        // The arc shader's rounded end caps would bulge a sector into a lens, so fill it as a polygon
        double startAngle = aStartAngle.AsRadians();
        double endAngle   = startAngle + aAngle.AsRadians();
        normalize( startAngle, endAngle );

        const double sweep = endAngle - startAngle;

        const int segCount = std::max( 1, static_cast<int>( std::ceil( sweep / calcAngleStep( aRadius ) ) ) );

        std::vector<VECTOR2D> sectorPts;
        sectorPts.reserve( static_cast<size_t>( segCount ) + 2 );
        sectorPts.push_back( aCenterPoint );

        for( int ii = 0; ii <= segCount; ++ii )
        {
            const double a = startAngle + sweep * ii / segCount;
            sectorPts.emplace_back( aCenterPoint.x + std::cos( a ) * aRadius,
                                    aCenterPoint.y + std::sin( a ) * aRadius );
        }

        // Not DrawPolygon(), which would also stroke the radial edges
        fillOutline( roundedOutline( sectorPts.begin(), sectorPts.end() ), m_fillColor );
    }

    if( m_isStrokeEnabled )
    {
        addArcInstance( aCenterPoint, aRadius, aStartAngle.AsRadians(), aAngle.AsRadians(),
                        m_lineWidth, m_strokeColor );
    }
}


void GAL_MODERN::DrawArcSegment( const VECTOR2D& aCenterPoint, double aRadius,
                                  const EDA_ANGLE& aStartAngle, const EDA_ANGLE& aAngle,
                                  double aWidth, double /*aMaxError*/ )
{
    if( aRadius <= 0 )
    {
        if( aWidth > 0 )
            DrawCircle( aCenterPoint, aWidth / 2.0 );

        return;
    }

    if( m_isStrokeEnabled )
    {
        double startAngle = aStartAngle.AsRadians();
        double endAngle = startAngle + aAngle.AsRadians();
        normalize( startAngle, endAngle );

        const double   halfWidth = aWidth / 2.0;
        const double   sweep = endAngle - startAngle;
        const VECTOR2D startPoint( aCenterPoint.x + std::cos( startAngle ) * aRadius,
                                   aCenterPoint.y + std::sin( startAngle ) * aRadius );
        const VECTOR2D endPoint( aCenterPoint.x + std::cos( endAngle ) * aRadius,
                                 aCenterPoint.y + std::sin( endAngle ) * aRadius );

        addArcInstance( aCenterPoint, aRadius + halfWidth, startAngle, sweep, m_lineWidth, m_strokeColor );

        if( aRadius > halfWidth )
            addArcInstance( aCenterPoint, aRadius - halfWidth, startAngle, sweep, m_lineWidth, m_strokeColor );

        addArcInstance( startPoint, halfWidth, startAngle + M_PI, M_PI, m_lineWidth, m_strokeColor );
        addArcInstance( endPoint, halfWidth, endAngle, M_PI, m_lineWidth, m_strokeColor );
    }

    if( m_isFillEnabled )
    {
        addArcInstance( aCenterPoint, aRadius, aStartAngle.AsRadians(), aAngle.AsRadians(), aWidth,
                        m_fillColor );
    }
}


RHI_INSTANCE& GAL_MODERN::addArcInstance( const VECTOR2D& aCenter, double aRadius, double aStartAngle,
                                           double aSweepAngle, double aWidth, const COLOR4D& aColor )
{
    const VECTOR2D center = xform( aCenter );
    const double   scale = xformScale();

    RHI_INSTANCE inst = makeInstance( aColor );
    inst.SetPosition( static_cast<float>( center.x ), static_cast<float>( center.y ) );
    inst.SetParam( 0, static_cast<float>( aRadius * scale ) );
    inst.SetParam( 1, static_cast<float>( aWidth * scale / 2.0 ) );
    inst.SetParam( 2, static_cast<float>( aStartAngle + xformAngle() ) );
    inst.SetParam( 3, static_cast<float>( aSweepAngle ) );

    emitInstance( RHI_SHAPE_ARC, inst );
    return poolForCurrentTarget().instances[RHI_SHAPE_ARC].back();
}


void GAL_MODERN::DrawCircle( const VECTOR2D& aCenterPoint, double aRadius )
{
    if( aRadius <= 0 )
        return;

    if( m_isFillEnabled )
    {
        addPadInstance( aCenterPoint, aRadius, aRadius, 0.0, 0.0f, 0.0, m_fillColor )
                .SetFlag( RHI_INSTANCE_PIXEL_CIRCLE );
    }

    if( m_isStrokeEnabled )
    {
        addArcInstance( aCenterPoint, aRadius, 0.0, 2.0 * M_PI, m_lineWidth, m_strokeColor )
                .SetFlag( RHI_INSTANCE_PIXEL_CIRCLE );
    }
}


void GAL_MODERN::DrawEllipse( const VECTOR2D& aCenterPoint, double aMajorRadius, double aMinorRadius,
                              const EDA_ANGLE& aRotation )
{
    if( aMajorRadius <= 0 || aMinorRadius <= 0 )
        return;

    drawEllipseShape( SHAPE_ELLIPSE( KiROUND( aCenterPoint ), KiROUND( aMajorRadius ), KiROUND( aMinorRadius ),
                                     aRotation ) );
}


void GAL_MODERN::DrawEllipseArc( const VECTOR2D& aCenterPoint, double aMajorRadius, double aMinorRadius,
                                 const EDA_ANGLE& aRotation, const EDA_ANGLE& aStartAngle,
                                 const EDA_ANGLE& aEndAngle )
{
    if( aMajorRadius <= 0 || aMinorRadius <= 0 )
        return;

    drawEllipseShape( SHAPE_ELLIPSE( KiROUND( aCenterPoint ), KiROUND( aMajorRadius ), KiROUND( aMinorRadius ),
                                     aRotation, aStartAngle, aEndAngle ) );
}


void GAL_MODERN::drawEllipseShape( const SHAPE_ELLIPSE& aEllipse )
{
    // As finely as GAL::DrawEllipse divides it for the other canvases
    const int              maxError = std::max( 1, KiROUND( aEllipse.GetMajorRadius() / 500.0 ) );
    const SHAPE_LINE_CHAIN curve = aEllipse.ConvertToPolyline( maxError );

    if( m_isFillEnabled )
    {
        SHAPE_LINE_CHAIN outline = curve;

        if( aEllipse.IsArc() )
            outline.Insert( 0, aEllipse.GetCenter() );

        fillOutline( outline, m_fillColor );
    }

    if( m_isStrokeEnabled )
        strokeLineChain( curve );
}


void GAL_MODERN::DrawHoleWall( const VECTOR2D& aCenterPoint, double aHoleRadius,
                                double aWallWidth )
{
    if( !m_isFillEnabled || aHoleRadius + aWallWidth <= 0 )
        return;

    // The hole covers a wall thinner than a pixel unless the ring is widened outward from it
    RHI_INSTANCE& ring = addArcInstance( aCenterPoint, aHoleRadius + aWallWidth / 2.0, 0.0, 2.0 * M_PI, aWallWidth,
                                         m_fillColor );

    ring.SetFlag( RHI_INSTANCE_GROW_OUTWARD );
    ring.SetFlag( RHI_INSTANCE_PIXEL_CIRCLE );
}


void GAL_MODERN::DrawRectangle( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint )
{
    double hw = std::abs( aEndPoint.x - aStartPoint.x ) / 2.0;
    double hh = std::abs( aEndPoint.y - aStartPoint.y ) / 2.0;
    VECTOR2D center( ( aStartPoint.x + aEndPoint.x ) / 2.0,
                     ( aStartPoint.y + aEndPoint.y ) / 2.0 );

    if( m_isFillEnabled )
    {
        addPadInstance( center, hw, hh, 0.0, 1.0f, 0.0, m_fillColor );
    }

    if( m_isStrokeEnabled )
    {
        VECTOR2D a( aStartPoint.x, aStartPoint.y );
        VECTOR2D b( aEndPoint.x,   aStartPoint.y );
        VECTOR2D c( aEndPoint.x,   aEndPoint.y );
        VECTOR2D d( aStartPoint.x, aEndPoint.y );

        addCapsuleInstance( a, b, m_lineWidth, m_strokeColor );
        addCapsuleInstance( b, c, m_lineWidth, m_strokeColor );
        addCapsuleInstance( c, d, m_lineWidth, m_strokeColor );
        addCapsuleInstance( d, a, m_lineWidth, m_strokeColor );
    }
}


void GAL_MODERN::DrawPolygon( const std::deque<VECTOR2D>& aPointList )
{
    if( aPointList.size() < 2 )
        return;

    if( m_isFillEnabled )
        fillOutline( roundedOutline( aPointList.begin(), aPointList.end() ), m_fillColor );

    // Match Cairo and the OpenGL canvas, which do not close the stroke. Callers wanting a closed
    // outline repeat the first point at the end.
    if( m_isStrokeEnabled )
        drawPolylineSegments( *this, aPointList.begin(), aPointList.end() );
}


void GAL_MODERN::DrawPolygon( const VECTOR2D aPointList[], int aListSize )
{
    if( aListSize < 2 )
        return;

    if( m_isFillEnabled )
        fillOutline( roundedOutline( aPointList, aPointList + aListSize ), m_fillColor );

    if( m_isStrokeEnabled )
        drawPolylineSegments( *this, aPointList, aPointList + aListSize );
}


void GAL_MODERN::DrawPolygon( const SHAPE_POLY_SET& aPolySet, bool /*aStrokeTriangulation*/ )
{
    if( m_isFillEnabled )
    {
        if( aPolySet.IsTriangulationUpToDate() )
        {
            emitTriangulatedPolygon( aPolySet, m_fillColor );
        }
        else
        {
            SHAPE_POLY_SET triangulated( aPolySet );
            triangulated.CacheTriangulation( false );
            emitTriangulatedPolygon( triangulated, m_fillColor );
        }
    }

    if( m_isStrokeEnabled )
    {
        for( int j = 0; j < aPolySet.OutlineCount(); ++j )
            strokeLineChain( aPolySet.COutline( j ) );

        for( int j = 0; j < aPolySet.OutlineCount(); ++j )
        {
            for( int h = 0; h < aPolySet.HoleCount( j ); ++h )
                strokeLineChain( aPolySet.CHole( j, h ) );
        }
    }
}


void GAL_MODERN::DrawPolygon( const SHAPE_LINE_CHAIN& aPolygon )
{
    if( aPolygon.PointCount() < 2 )
        return;

    if( m_isFillEnabled )
        fillOutline( aPolygon, m_fillColor );

    if( m_isStrokeEnabled )
        strokeLineChain( aPolygon );
}


void GAL_MODERN::DrawCurve( const VECTOR2D& aStartPoint, const VECTOR2D& aControlPointA,
                             const VECTOR2D& aControlPointB, const VECTOR2D& aEndPoint,
                             double aFilterValue )
{
    std::vector<VECTOR2D> controlPts = { aStartPoint, aControlPointA, aControlPointB, aEndPoint };
    BEZIER_POLY converter( controlPts );

    std::vector<VECTOR2D> output;
    converter.GetPoly( output, aFilterValue );

    if( output.size() < 2 )
        return;

    for( size_t i = 1; i < output.size(); ++i )
    {
        DrawLine( output[i - 1], output[i] );
    }
}


void GAL_MODERN::DrawBitmap( const BITMAP_BASE& aBitmap, double alphaBlend )
{
    // Mirroring and rotation go to the texture coordinates as on the OpenGL canvas, so the image as
    // loaded is uploaded once per KIID however it is turned
    const wxImage* image = aBitmap.GetOriginalImageData();

    if( !image || !image->IsOk() )
        return;

    float alpha = static_cast<float>( std::clamp( alphaBlend, 0.0, 1.0 ) );

    const VECTOR2I sizePixels = aBitmap.GetSizePixels();

    if( image->GetWidth() <= 0 || image->GetHeight() <= 0 || sizePixels.x <= 0 || sizePixels.y <= 0 )
        return;

    const KIID        imageId = aBitmap.GetImageID();
    RHI_TextureHandle texture;
    std::array<uint64_t, 2> bitmapContentHash{};

    auto cacheIt = m_bitmapTextureCache.find( imageId );

    if( cacheIt != m_bitmapTextureCache.end() )
    {
        texture                  = cacheIt->second.texture;
        bitmapContentHash        = cacheIt->second.contentHash;
        cacheIt->second.lastUse  = ++m_bitmapUseCounter;
    }
    else
    {
        // The quad keeps the image's size, so a texture scaled down to fit only loses detail
        const wxImage fitted = FitImageToTexture( *image, static_cast<int>( m_deviceProxy.MaxTextureSize() ) );

        if( !fitted.IsOk() )
            return;

        RHI_TEXTURE_DESC texDesc;
        texDesc.width  = static_cast<uint32_t>( fitted.GetWidth() );
        texDesc.height = static_cast<uint32_t>( fitted.GetHeight() );
        texDesc.format = RHI_TEXTURE_FORMAT::RGBA8;
        texDesc.filter = RHI_FILTER::BILINEAR;

        texture = m_deviceProxy.CreateTexture( texDesc );

        if( !texture )
            return;

        std::vector<uint8_t> rgba = ImageToRgba( fitted );

        MMH3_HASH contentHash;
        contentHash.add( static_cast<int32_t>( texDesc.width ) );
        contentHash.add( static_cast<int32_t>( texDesc.height ) );
        hashBytes( contentHash, rgba.data(), rgba.size() );
        bitmapContentHash = hashDigest( contentHash );

        m_deviceProxy.UploadTexture( texture, rgba.data(), rgba.size() );

        BITMAP_CACHE_ENTRY& entry = m_bitmapTextureCache[imageId];
        entry.texture = texture;
        entry.bytes = rgba.size();
        entry.lastUse = ++m_bitmapUseCounter;
        entry.contentHash = bitmapContentHash;

        evictBitmapCacheEntries();
    }

    double scale = 1.0 / ( aBitmap.GetPPI() * m_worldUnitLength );
    double w = static_cast<double>( sizePixels.x ) * scale;
    double h = static_cast<double>( sizePixels.y ) * scale;

    float hw = static_cast<float>( w / 2.0 );
    float hh = static_cast<float>( h / 2.0 );

    float texStartX = aBitmap.IsMirroredX() ? 1.0f : 0.0f;
    float texEndX   = aBitmap.IsMirroredX() ? 0.0f : 1.0f;
    float texStartY = aBitmap.IsMirroredY() ? 1.0f : 0.0f;
    float texEndY   = aBitmap.IsMirroredY() ? 0.0f : 1.0f;

    const double texCos = aBitmap.Rotation().Cos();
    const double texSin = aBitmap.Rotation().Sin();

    VECTOR2D corners[4] = {
        { -hw, -hh },
        {  hw, -hh },
        {  hw,  hh },
        { -hw,  hh }
    };

    for( auto& corner : corners )
        corner = xform( corner );

    uint8_t alphaU8 = packColorChannel( alpha );

    // The OpenGL canvas turns the texture matrix about the texture centre
    auto setVert = [&]( RHI_VERTEX& v, const VECTOR2D& pos, float tu, float tv )
    {
        const double du = tu - 0.5;
        const double dv = tv - 0.5;

        v.SetPosition( static_cast<float>( pos.x ), static_cast<float>( pos.y ) );
        v.SetUV( static_cast<float>( 0.5 + du * texCos - dv * texSin ),
                 static_cast<float>( 0.5 + du * texSin + dv * texCos ) );
        v.SetColor( 255, 255, 255, alphaU8 );
    };

    BITMAP_QUAD quad;
    quad.alpha       = alpha;
    quad.contentHash = bitmapContentHash;
    quad.texture     = texture;
    quad.ownsTexture = false; // owned by m_bitmapTextureCache

    setVert( quad.vertices[0], corners[0], texStartX, texStartY );
    setVert( quad.vertices[1], corners[1], texEndX,   texStartY );
    setVert( quad.vertices[2], corners[2], texEndX,   texEndY );
    setVert( quad.vertices[3], corners[0], texStartX, texStartY );
    setVert( quad.vertices[4], corners[2], texEndX,   texEndY );
    setVert( quad.vertices[5], corners[3], texStartX, texEndY );

    RHI_BUFFER_DESC bufDesc;
    bufDesc.type      = RHI_BUFFER_TYPE::VERTEX;
    bufDesc.sizeBytes = sizeof( RHI_VERTEX ) * 6;

    quad.vertexBuffer = m_deviceProxy.CreateBuffer( bufDesc );

    if( quad.vertexBuffer )
        m_deviceProxy.UpdateBuffer( quad.vertexBuffer, quad.vertices, sizeof( RHI_VERTEX ) * 6 );

    // The current target's pool, so a group being recorded captures the quad
    poolForCurrentTarget().bitmapQuads.push_back( quad );
}


void GAL_MODERN::DrawPolyline( const std::deque<VECTOR2D>& aPointList )
{
    drawPolylineSegments( *this, aPointList.begin(), aPointList.end() );
}


void GAL_MODERN::DrawPolyline( const std::vector<VECTOR2D>& aPointList )
{
    drawPolylineSegments( *this, aPointList.begin(), aPointList.end() );
}


void GAL_MODERN::DrawPolyline( const VECTOR2D aPointList[], int aListSize )
{
    if( aListSize < 2 )
        return;

    drawPolylineSegments( *this, aPointList, aPointList + aListSize );
}


void GAL_MODERN::DrawPolyline( const SHAPE_LINE_CHAIN& aLineChain )
{
    strokeLineChain( aLineChain );
}


void GAL_MODERN::DrawPolylines( const std::vector<std::vector<VECTOR2D>>& aPointLists )
{
    for( const auto& points : aPointLists )
    {
        DrawPolyline( points );
    }
}


namespace
{
// Glyphs the shader table holds, which is every glyph of the font unless the font outgrows it
uint32_t tableGlyphCount()
{
    return std::min( BUILTIN_FONT::GlyphCount(), RHI_GLYPH_TABLE_SIZE );
}


// The atlas the OpenGL canvas draws bitmap text from, as the render thread reads it
std::shared_ptr<const RHI_GLYPH_ATLAS_DATA> buildGlyphAtlas()
{
    static const std::shared_ptr<const RHI_GLYPH_ATLAS_DATA> built = []()
    {
        using namespace BUILTIN_FONT;

        auto atlas = std::make_shared<RHI_GLYPH_ATLAS_DATA>();
        atlas->width = font_image.width;
        atlas->height = font_image.height;
        atlas->rgba = RgbToRgba( font_image.pixels, nullptr, size_t( atlas->width ) * atlas->height );

        atlas->table.assign( ( RHI_GLYPH_TABLE_SIZE + 1 ) * 4, 0.0f );

        const FONT_GLYPH_TYPE* first = LookupGlyph( '!' );

        for( uint32_t ii = 0; ii < tableGlyphCount() && first; ++ii )
        {
            const GLYPH_BOX box = ComputeGlyphBox( first[ii] );

            atlas->table[ii * 4 + 0] = box.atlasX / atlas->width;
            atlas->table[ii * 4 + 1] = box.atlasY / atlas->height;
            atlas->table[ii * 4 + 2] = box.w / atlas->width;
            atlas->table[ii * 4 + 3] = box.h / atlas->height;
        }

        atlas->table[RHI_GLYPH_TABLE_SIZE * 4 + 0] = static_cast<float>( atlas->width );
        atlas->table[RHI_GLYPH_TABLE_SIZE * 4 + 1] = static_cast<float>( atlas->height );

        return atlas;
    }();

    return built;
}
} // namespace


bool GAL_MODERN::drawBitmapGlyphs( const wxString& aText, const VECTOR2I& aPosition, const EDA_ANGLE& aAngle )
{
    using namespace BUILTIN_FONT;

    // Cases the glyph quads leave to strokes
    if( !m_bitmapGlyphs || m_negativeMode || m_screenSpaceMode || m_strokePixelPad != 0 || IsTextMirrored()
        || m_globalFlipX || m_globalFlipY || m_currentTransform.Determinant() <= 0.0 || aText.Contains( wxT( "^{" ) )
        || aText.Contains( wxT( "_{" ) ) || aText.Contains( wxT( "~{" ) ) || aText.Contains( wxT( "\n" ) ) )
    {
        return false;
    }

    const UTF8 text( aText );
    VECTOR2D   textSize;
    float      commonOffset;
    std::tie( textSize, commonOffset ) = ComputeBitmapTextSize( text );

    if( textSize.y <= 0.0 )
        return false;

    if( m_renderThread )
        m_renderThread->SetGlyphAtlas( buildGlyphAtlas() );

    const FONT_GLYPH_TYPE* first = LookupGlyph( '!' );

    // The shader table indexes glyphs by their place in the font
    const FONT_GLYPH_TYPE* const limit = first + tableGlyphCount();

    const double scale = 1.4 * GetGlyphSize().y / textSize.y;
    const double angle = aAngle.AsRadians();
    const double xfScale = xformScale();
    const double xfAngle = xformAngle();

    // The origin of the first glyph, in the font's units from the text position
    double cursorX = 0.0;
    double cursorY = -commonOffset;

    switch( GetHorizontalJustify() )
    {
    case GR_TEXT_H_ALIGN_CENTER: cursorX -= textSize.x / 2.0; break;
    case GR_TEXT_H_ALIGN_RIGHT: cursorX -= textSize.x; break;
    default: break;
    }

    switch( GetVerticalJustify() )
    {
    case GR_TEXT_V_ALIGN_CENTER: cursorY -= textSize.y / 2.0; break;
    case GR_TEXT_V_ALIGN_BOTTOM: cursorY -= textSize.y; break;
    default: break;
    }

    const COLOR4D& color = m_strokeColor;
    const double   cosA = std::cos( angle );
    const double   sinA = std::sin( angle );

    std::vector<RHI_INSTANCE> glyphs;
    glyphs.reserve( size_t( aText.length() ) );

    for( UTF8::uni_iter chIt = text.ubegin(), end = text.uend(); chIt < end; ++chIt )
    {
        if( *chIt == ' ' )
        {
            cursorX += SpaceAdvance();
            continue;
        }

        const FONT_GLYPH_TYPE* glyph = LookupGlyph( *chIt );

        if( !glyph )
            glyph = LookupGlyph( '?' );

        if( !glyph )
            continue;

        if( glyph < first || glyph >= limit )
            return false;

        const GLYPH_BOX box = ComputeGlyphBox( *glyph );
        const double    width = box.w;
        const double    height = box.h;
        const double    centerX = cursorX + box.x + width / 2.0;
        const double    centerY = cursorY + box.y + height / 2.0;

        // The text turns about its position the way the OpenGL canvas turns it, against the angle
        VECTOR2D local( scale * centerX, scale * centerY );
        VECTOR2D turned( local.x * cosA + local.y * sinA, -local.x * sinA + local.y * cosA );
        VECTOR2D center = xform( VECTOR2D( aPosition.x + turned.x, aPosition.y + turned.y ) );

        RHI_INSTANCE inst;
        inst.SetPosition( static_cast<float>( center.x ), static_cast<float>( center.y ) );
        inst.SetRotation( static_cast<float>( xfAngle - angle ) );
        inst.SetParam( 0, static_cast<float>( scale * width / 2.0 * xfScale ) );
        inst.SetParam( 1, static_cast<float>( scale * height / 2.0 * xfScale ) );
        inst.SetParam( 2, static_cast<float>( glyph - first ) );
        inst.SetColor( packColorChannel( color.r ), packColorChannel( color.g ), packColorChannel( color.b ),
                       packColorChannel( color.a ) );
        inst.SetDepth( static_cast<float>( m_layerDepth ) );
        inst.SetFlag( RHI_INSTANCE_BITMAP_GLYPH );

        if( m_groupDepth == 0 && m_currentTarget == TARGET_OVERLAY )
            inst.SetFlag( RHI_INSTANCE_OVERLAY_ALPHA );

        if( m_strokeLod > 0.0 )
        {
            inst.SetFlag( RHI_INSTANCE_SCALE_LOD );
            inst.SetParam( 3, static_cast<float>( m_strokeLod ) );
        }

        glyphs.push_back( inst );
        cursorX += glyph->advance;
    }

    notePolarity( false );

    std::vector<RHI_INSTANCE>& tracks = poolForCurrentTarget().instances[RHI_SHAPE_TRACK];
    tracks.insert( tracks.end(), glyphs.begin(), glyphs.end() );

    return true;
}


void GAL_MODERN::BitmapText( const wxString& aText, const VECTOR2I& aPosition, const EDA_ANGLE& aAngle )
{
    if( drawBitmapGlyphs( aText, aPosition, aAngle ) )
        return;

    // The OpenGL canvas draws this from a font texture, which fades away as the text shrinks.
    // Strokes held at one pixel would turn every small net name into a smudge instead.
    m_bitmapTextHeight = GetGlyphSize().y;
    GAL::BitmapText( aText, aPosition, aAngle );
    m_bitmapTextHeight = 0.0;
}


void GAL_MODERN::DrawGlyph( const KIFONT::GLYPH& aGlyph, int /*aNth*/, int /*aTotal*/ )
{
    if( aGlyph.IsStroke() )
    {
        const auto& strokeGlyph = static_cast<const KIFONT::STROKE_GLYPH&>( aGlyph );
        DrawPolylines( strokeGlyph );
    }
    else if( aGlyph.IsOutline() )
    {
        const auto&      outlineGlyph = static_cast<const KIFONT::OUTLINE_GLYPH&>( aGlyph );
        const RHI_VERTEX fill = fillVertex( m_fillColor );

        outlineGlyph.Triangulate(
                [&]( const VECTOR2I& aPt1, const VECTOR2I& aPt2, const VECTOR2I& aPt3 )
                {
                    emitTriangle( aPt1, aPt2, aPt3, fill );
                } );
    }
}


RHI_INSTANCE& GAL_MODERN::addPadInstance( const VECTOR2D& aCenter, double aHalfWidth, double aHalfHeight,
                                           double aCornerRadius, float aShapeType, double aRotation,
                                           const COLOR4D& aColor )
{
    const VECTOR2D center = xform( aCenter );
    const double   scale = xformScale();

    RHI_INSTANCE inst = makeInstance( aColor );
    inst.SetPosition( static_cast<float>( center.x ), static_cast<float>( center.y ) );
    inst.SetRotation( static_cast<float>( aRotation + xformAngle() ) );
    inst.SetParam( 0, static_cast<float>( aHalfWidth * scale ) );
    inst.SetParam( 1, static_cast<float>( aHalfHeight * scale ) );
    inst.SetParam( 2, static_cast<float>( aCornerRadius * scale ) );
    inst.SetParam( 3, aShapeType );

    emitInstance( RHI_SHAPE_PAD, inst );
    return poolForCurrentTarget().instances[RHI_SHAPE_PAD].back();
}


void GAL_MODERN::strokeLineChain( const SHAPE_LINE_CHAIN& aChain )
{
    // A closed chain takes a closing segment, which SegmentCount() includes even for one point
    if( aChain.PointCount() < 2 )
        return;

    for( int i = 0; i < aChain.SegmentCount(); ++i )
    {
        const SEG seg = aChain.CSegment( i );
        DrawLine( VECTOR2D( seg.A ), VECTOR2D( seg.B ) );
    }
}


void GAL_MODERN::fillOutline( SHAPE_LINE_CHAIN aOutline, const COLOR4D& aColor )
{
    if( aOutline.PointCount() < 3 )
        return;

    // Line endings arrive as open chains, which SHAPE_POLY_SET rejects
    aOutline.SetClosed( true );

    SHAPE_POLY_SET polySet( aOutline );
    polySet.CacheTriangulation( false );
    emitTriangulatedPolygon( polySet, aColor );
}


void GAL_MODERN::emitTriangulatedPolygon( const SHAPE_POLY_SET& aPolySet, const COLOR4D& aColor )
{
    const RHI_VERTEX fill = fillVertex( aColor );

    for( int j = 0; j < static_cast<int>( aPolySet.TriangulatedPolyCount() ); ++j )
    {
        const auto* triPoly = aPolySet.TriangulatedPolygon( j );

        for( int i = 0; i < static_cast<int>( triPoly->GetTriangleCount() ); ++i )
        {
            VECTOR2I a, b, c;
            triPoly->GetTriangle( i, a, b, c );
            emitTriangle( a, b, c, fill );
        }
    }
}


RHI_VERTEX GAL_MODERN::fillVertex( const COLOR4D& aColor )
{
    RHI_VERTEX vertex;
    vertex.SetColor( packColorChannel( aColor.r ), packColorChannel( aColor.g ), packColorChannel( aColor.b ),
                     m_negativeMode ? 255 : packColorChannel( aColor.a ) );
    vertex.SetUV( static_cast<float>( m_layerDepth ), 0.0f );

    notePolarity( m_negativeMode );

    return vertex;
}


void GAL_MODERN::emitTriangle( const VECTOR2D& aA, const VECTOR2D& aB, const VECTOR2D& aC, const RHI_VERTEX& aFill )
{
    std::vector<RHI_VERTEX>& vertices = poolForCurrentTarget().polygonVertices;

    for( const VECTOR2D& corner : { aA, aB, aC } )
    {
        const VECTOR2D placed = xform( corner );

        vertices.push_back( aFill );
        vertices.back().SetPosition( static_cast<float>( placed.x ), static_cast<float>( placed.y ) );
    }
}


int GAL_MODERN::BeginGroup()
{
    // Groups do not nest. A nested BeginGroup() draws into the open group and only counts depth, as a
    // second swap would corrupt the scratch state.
    if( m_groupDepth > 0 )
    {
        m_groupDepth++;
        return m_currentGroupId;
    }

    int groupId = m_groupCounter++;

    // swap() keeps the data() pointers, so staging data the render thread holds stays valid
    GEOMETRY_BUFFER_SET& pool = poolForCurrentTarget();

    for( int shape = 0; shape < RHI_INSTANCED_SHAPES; ++shape )
        m_groupScratchInstances[shape].swap( pool.instances[shape] );

    m_groupScratchPolygonVertices.swap( pool.polygonVertices );
    m_groupScratchBitmapQuads.swap( pool.bitmapQuads );

    m_currentGroupId = groupId;
    m_groupNegative = false;
    m_groupDepth     = 1;

    return groupId;
}


void GAL_MODERN::EndGroup()
{
    if( m_groupDepth == 0 )
        return;

    if( m_groupDepth > 1 )
    {
        m_groupDepth--;
        return;
    }

    GEOMETRY_BUFFER_SET& pool = poolForCurrentTarget();

    GAL_GROUP group;
    group.instances       = std::move( pool.instances );
    group.polygonVertices = std::move( pool.polygonVertices );
    group.bitmapQuads     = std::move( pool.bitmapQuads );

    for( int shape = 0; shape < RHI_INSTANCED_SHAPES; ++shape )
        m_groupScratchInstances[shape].swap( pool.instances[shape] );

    m_groupScratchPolygonVertices.swap( pool.polygonVertices );
    m_groupScratchBitmapQuads.swap( pool.bitmapQuads );

    group.baseDepth = static_cast<float>( m_layerDepth );
    group.negative = m_groupNegative;
    countBitmapRefs( group, 1 );

    m_groups[m_currentGroupId] = std::move( group );
    m_currentGroupId = -1;
    m_groupDepth     = 0;
}


std::array<uint64_t, 2> GAL_MODERN::computeGroupFingerprint( const GAL_GROUP& aGroup )
{
    MMH3_HASH hash;

    auto mix = [&]( const void* aData, size_t aSize )
    {
        hashBytes( hash, aData, aSize );
    };

    auto mixVector = [&]( const auto& aVector )
    {
        const uint64_t count = aVector.size();
        mix( &count, sizeof( count ) );
        mix( aVector.data(), aVector.size() * sizeof( aVector[0] ) );
    };

    for( const std::vector<RHI_INSTANCE>& instances : aGroup.instances )
        mixVector( instances );

    mixVector( aGroup.polygonVertices );
    mix( &aGroup.baseDepth, sizeof( aGroup.baseDepth ) );
    mix( &aGroup.negative, sizeof( aGroup.negative ) );
    mix( &aGroup.undimmed, sizeof( aGroup.undimmed ) );

    const uint64_t quadCount = aGroup.bitmapQuads.size();
    mix( &quadCount, sizeof( quadCount ) );

    for( const BITMAP_QUAD& quad : aGroup.bitmapQuads )
    {
        mix( quad.vertices, sizeof( quad.vertices ) );
        mix( &quad.alpha, sizeof( quad.alpha ) );
        mix( quad.contentHash.data(), sizeof( quad.contentHash ) );
    }

    return hashDigest( hash );
}


std::array<uint64_t, 2> GAL_MODERN::GroupContentFingerprint( int aGroupNumber ) const
{
    auto it = m_groups.find( aGroupNumber );

    if( it == m_groups.end() )
        return {};

    const GAL_GROUP& group = it->second;

    if( !group.contentFingerprintValid )
    {
        group.contentFingerprint = computeGroupFingerprint( group );
        group.contentFingerprintValid = true;
    }

    return group.contentFingerprint;
}


void GAL_MODERN::DrawGroup( int aGroupNumber )
{
    auto it = m_groups.find( aGroupNumber );

    if( it == m_groups.end() )
    {
        wxLogTrace( traceGalGroups, wxS( "DrawGroup(%d) NOT FOUND (total groups=%zu)" ),
                    aGroupNumber, m_groups.size() );
        return;
    }

    const GAL_GROUP&     group = it->second;
    GEOMETRY_BUFFER_SET& pool  = poolForCurrentTarget();

    notePolarity( group.negative );

    const RHI_SHAPE_COUNTS before = pool.Sizes();

    // Record where the group lands in the static pool so a later color or depth change can be patched
    // in place. Inside BeginGroup() the static vectors are the scratch.
    if( &pool == &m_staticPool && m_groupDepth == 0 )
    {
        STATIC_GROUP_SLOT slot;
        slot.start = before;

        for( int shape = 0; shape < RHI_INSTANCED_SHAPES; ++shape )
            slot.count[shape] = static_cast<uint32_t>( group.instances[shape].size() );

        slot.count[RHI_SHAPE_POLYGON] = static_cast<uint32_t>( group.polygonVertices.size() );
        m_staticGroupSlots[aGroupNumber].push_back( slot );
    }

    // A placed group is not told when its layer moves, the scene keeps depth per layer. Drawn
    // anywhere else it has to be carried to the depth its layer has now.
    const float shift = group.sceneLayer >= 0 ? static_cast<float>( m_layerDepth ) - group.baseDepth : 0.0f;
    const bool  overlay = m_groupDepth == 0 && m_currentTarget == TARGET_OVERLAY;

    auto rebase = [shift]( uint16_t& aDepth )
    {
        aDepth = glm::packHalf1x16( glm::unpackHalf1x16( aDepth ) + shift );
    };

    for( int shape = 0; shape < RHI_INSTANCED_SHAPES; ++shape )
    {
        std::vector<RHI_INSTANCE>& instances = pool.instances[shape];

        instances.insert( instances.end(), group.instances[shape].begin(), group.instances[shape].end() );

        if( !overlay && shift == 0.0f )
            continue;

        for( size_t ii = before[shape]; ii < instances.size(); ++ii )
        {
            if( overlay )
                instances[ii].SetFlag( RHI_INSTANCE_OVERLAY_ALPHA );

            if( shift != 0.0f )
                rebase( instances[ii].depth );
        }
    }

    pool.polygonVertices.insert( pool.polygonVertices.end(), group.polygonVertices.begin(),
                                 group.polygonVertices.end() );

    if( shift != 0.0f )
    {
        for( size_t ii = before[RHI_SHAPE_POLYGON]; ii < pool.polygonVertices.size(); ++ii )
            rebase( pool.polygonVertices[ii].u );
    }

    if( !group.bitmapQuads.empty() )
    {
        // A group's bitmap quads always go to a dynamic pool
        GEOMETRY_BUFFER_SET& bitmapPool = m_currentTarget == TARGET_OVERLAY ? obuf() : buf();
        const bool           open = !m_persistentTargets
                                    || ( m_currentTarget == TARGET_OVERLAY ? m_overlayRefilled
                                                                           : m_nonCachedRefilled );

        // A kept pool still lists the quads of the frame that filled it
        for( const auto& bq : group.bitmapQuads )
        {
            if( !open )
                break;

            BITMAP_QUAD copy = bq;
            copy.ownsResources = false;
            bitmapPool.bitmapQuads.push_back( copy );
        }
    }
}


static RHI_INSTANCE deadInstance()
{
    // Zero scale collapses the instance quad to a point, so it produces no fragments
    RHI_INSTANCE inst;
    inst.scaleX = 0;
    inst.scaleY = 0;
    inst.SetColor( 0, 0, 0, 0 );
    return inst;
}


static RHI_VERTEX deadVertex()
{
    RHI_VERTEX vert;
    vert.SetColor( 0, 0, 0, 0 );
    return vert;
}


static void fillDead( RHI_INSTANCE* aFirst, uint32_t aCount )
{
    std::fill_n( aFirst, aCount, deadInstance() );
}


static void fillDead( RHI_VERTEX* aFirst, uint32_t aCount )
{
    std::fill_n( aFirst, aCount, deadVertex() );
}


template <typename ARENA>
static void markSceneDirty( ARENA& aArena, uint32_t aStart, uint32_t aCount )
{
    // Every span is its own staging buffer on the render thread, so keep the list short
    static constexpr size_t MAX_SPANS = 8;

    if( aCount == 0 || aArena.uploadAll )
        return;

    for( auto& span : aArena.dirty )
    {
        // A slot is freed, reused and written in one update, which is still one span
        if( aStart >= span.begin && aStart <= span.end )
        {
            span.end = std::max( span.end, aStart + aCount );
            return;
        }
    }

    if( aArena.dirty.size() >= MAX_SPANS )
    {
        for( size_t ii = 1; ii < aArena.dirty.size(); ++ii )
            aArena.dirty[0].add( aArena.dirty[ii].begin, aArena.dirty[ii].end - aArena.dirty[ii].begin );

        aArena.dirty.resize( 1 );
    }

    aArena.dirty.emplace_back();
    aArena.dirty.back().add( aStart, aCount );
}


template <typename T>
uint32_t GAL_MODERN::allocSceneSlot( SCENE_ARENA<T>& aArena, uint32_t aCount )
{
    if( aCount == 0 )
        return 0;

    uint32_t start = 0;
    auto     it = aArena.freeSlots.find( aCount );

    if( it != aArena.freeSlots.end() && !it->second.empty() )
    {
        start = it->second.back();
        it->second.pop_back();
        aArena.dead -= aCount;
    }
    else
    {
        start = static_cast<uint32_t>( aArena.elements.size() );
        aArena.elements.resize( start + aCount );
    }

    markSceneDirty( aArena, start, aCount );
    return start;
}


template <typename T>
void GAL_MODERN::freeSceneSlot( SCENE_ARENA<T>& aArena, uint32_t aStart, uint32_t aCount )
{
    if( aCount == 0 )
        return;

    // The last slot just shortens the draw
    if( static_cast<size_t>( aStart ) + aCount == aArena.elements.size() )
    {
        aArena.elements.resize( aStart );
        return;
    }

    fillDead( &aArena.elements[aStart], aCount );
    markSceneDirty( aArena, aStart, aCount );
    aArena.dead += aCount;
    aArena.freeSlots[aCount].push_back( aStart );
}


template <typename T>
void GAL_MODERN::compactSceneArena( SCENE_LAYER& aLayer, SCENE_ARENA<T>& aArena, int aArenaId )
{
    std::vector<GAL_GROUP*> owners;
    owners.reserve( aLayer.residents.size() );

    for( int groupId : aLayer.residents )
    {
        GAL_GROUP& group = m_groups.at( groupId );

        if( group.slotCount[aArenaId] > 0 )
            owners.push_back( &group );
    }

    // Keeping slot order keeps the draw order inside the layer
    std::sort( owners.begin(), owners.end(),
               [aArenaId]( const GAL_GROUP* aLhs, const GAL_GROUP* aRhs )
               {
                   return aLhs->slotStart[aArenaId] < aRhs->slotStart[aArenaId];
               } );

    std::vector<T> packed;
    packed.reserve( aArena.elements.size() - aArena.dead );

    for( GAL_GROUP* group : owners )
    {
        auto first = aArena.elements.begin() + group->slotStart[aArenaId];

        group->slotStart[aArenaId] = static_cast<uint32_t>( packed.size() );
        packed.insert( packed.end(), first, first + group->slotCount[aArenaId] );
    }

    aArena.elements.swap( packed );
    aArena.freeSlots.clear();
    aArena.dirty.clear();
    aArena.dead      = 0;
    aArena.uploadAll = true;
}


void GAL_MODERN::countGroupAlpha( const GAL_GROUP& aGroup, SCENE_LAYER& aLayer, int aDelta )
{
    // A knockout covers fully whatever alpha the layer is drawn with, so it has no say in it
    auto count = [&]( const std::vector<RHI_INSTANCE>& aInstances )
    {
        for( const RHI_INSTANCE& inst : aInstances )
        {
            if( !inst.HasFlag( RHI_INSTANCE_NEGATIVE ) )
                aLayer.alphaCount[inst.a] += aDelta;
        }
    };

    for( const std::vector<RHI_INSTANCE>& instances : aGroup.instances )
        count( instances );

    if( !aGroup.negative )
    {
        for( const RHI_VERTEX& vert : aGroup.polygonVertices )
            aLayer.alphaCount[vert.a] += aDelta;
    }
}


void GAL_MODERN::countGroupArenas( const GAL_GROUP& aGroup, uint32_t* aCounts )
{
    for( int shape = 0; shape < RHI_INSTANCED_SHAPES; ++shape )
    {
        const std::vector<RHI_INSTANCE>& instances = aGroup.instances[shape];
        const auto                       negatives = std::ranges::count_if( instances,
                                                                             []( const RHI_INSTANCE& aInstance )
                                                                             {
                                                                                 return aInstance.HasFlag(
                                                                                         RHI_INSTANCE_NEGATIVE );
                                                                             } );

        aCounts[arenaOf( shape, false )] = static_cast<uint32_t>( instances.size() - negatives );
        aCounts[arenaOf( shape, true )] = static_cast<uint32_t>( negatives );
    }
}


void GAL_MODERN::writeGroupToScene( const GAL_GROUP& aGroup, SCENE_LAYER& aLayer )
{
    // In its own depth slice a layer keeps its alpha instead of being composited with it. Its
    // edges are hard, a partly covered one would claim the pixel from an element behind it.
    const bool composited = sceneLayerComposited( aLayer );
    const bool opaque = aLayer.bakedAlpha >= 0 && composited;
    const bool hard = !composited && ( aLayer.translucent || aLayer.hasNegatives );

    // The scene draws each layer into its own slice of the depth range, so an element keeps
    // only how far it sits from its layer
    for( int shape = 0; shape < RHI_INSTANCED_SHAPES; ++shape )
    {
        const int                  normalArena = arenaOf( shape, false );
        const int                  negativeArena = arenaOf( shape, true );
        SCENE_ARENA<RHI_INSTANCE>& normals = aLayer.instances[normalArena];
        SCENE_ARENA<RHI_INSTANCE>& negatives = aLayer.instances[negativeArena];
        uint32_t                   normal = aGroup.slotStart[normalArena];
        uint32_t                   negative = aGroup.slotStart[negativeArena];

        for( const RHI_INSTANCE& source : aGroup.instances[shape] )
        {
            RHI_INSTANCE& dest = source.HasFlag( RHI_INSTANCE_NEGATIVE ) ? negatives.elements[negative++]
                                                                           : normals.elements[normal++];
            dest = source;
            dest.depth = relativeDepth( source.depth, aGroup.baseDepth );

            if( opaque )
                dest.a = 255;

            if( hard )
                dest.SetFlag( RHI_INSTANCE_HARD_EDGE );

            if( aLayer.overlayAlpha )
                dest.SetFlag( RHI_INSTANCE_OVERLAY_ALPHA );
        }

        markSceneDirty( normals, aGroup.slotStart[normalArena], aGroup.slotCount[normalArena] );
        markSceneDirty( negatives, aGroup.slotStart[negativeArena], aGroup.slotCount[negativeArena] );
    }

    uint32_t vertex = aGroup.slotStart[ARENA_POLYGON];

    for( const RHI_VERTEX& source : aGroup.polygonVertices )
    {
        RHI_VERTEX& dest = aLayer.polygons.elements[vertex++];
        dest = source;
        dest.u = relativeDepth( source.u, aGroup.baseDepth );

        if( opaque )
            dest.a = 255;
    }

    markSceneDirty( aLayer.polygons, aGroup.slotStart[ARENA_POLYGON], aGroup.slotCount[ARENA_POLYGON] );
}


static size_t retainedPrefixHash( const RETAINED_GROUP_ORDER& aOrder )
{
    return hash_val( aOrder.priority, aOrder.bbox[0], aOrder.bbox[1], aOrder.bbox[2], aOrder.bbox[3],
                     aOrder.lodScale );
}


void GAL_MODERN::placeGroupInScene( int aGroupId, GAL_GROUP& aGroup, SCENE_LAYER& aLayer )
{
    countGroupArenas( aGroup, aGroup.slotCount );

    for( int arena = 0; arena < ARENA_INSTANCE_COUNT; ++arena )
        aGroup.slotStart[arena] = allocSceneSlot( aLayer.instances[arena], aGroup.slotCount[arena] );

    aGroup.slotCount[ARENA_POLYGON] = static_cast<uint32_t>( aGroup.polygonVertices.size() );
    aGroup.slotStart[ARENA_POLYGON] = allocSceneSlot( aLayer.polygons, aGroup.slotCount[ARENA_POLYGON] );

    aGroup.resident = true;
    aGroup.residentIndex = static_cast<uint32_t>( aLayer.residents.size() );
    aGroup.placeSeq = ++m_placeSeq;
    aLayer.residents.push_back( aGroupId );

    if( aLayer.retainedPrefixIndex )
        aLayer.retainedPrefixIndex->emplace( retainedPrefixHash( aGroup.order ), aGroupId );
    aLayer.runsStale = true;

    if( aGroup.negative )
        aLayer.knockoutGroups++;

    if( !aGroup.bitmapQuads.empty() )
        aLayer.bitmapResidents.push_back( aGroupId );

    countGroupAlpha( aGroup, aLayer, 1 );
    aLayer.alphaStale = true;

    writeGroupToScene( aGroup, aLayer );
    m_sceneChanged = true;
}


void GAL_MODERN::removeGroupFromScene( int aGroupId, GAL_GROUP& aGroup )
{
    SCENE_LAYER& layer = m_sceneLayers.at( aGroup.sceneLayer );

    if( layer.retainedPrefixIndex )
    {
        auto [first, last] = layer.retainedPrefixIndex->equal_range( retainedPrefixHash( aGroup.order ) );

        for( auto it = first; it != last; ++it )
        {
            if( it->second == aGroupId )
            {
                layer.retainedPrefixIndex->erase( it );
                break;
            }
        }
    }

    for( int arena = 0; arena < ARENA_INSTANCE_COUNT; ++arena )
        freeSceneSlot( layer.instances[arena], aGroup.slotStart[arena], aGroup.slotCount[arena] );

    freeSceneSlot( layer.polygons, aGroup.slotStart[ARENA_POLYGON], aGroup.slotCount[ARENA_POLYGON] );

    const int last = layer.residents.back();

    layer.residents[aGroup.residentIndex] = last;
    m_groups.at( last ).residentIndex = aGroup.residentIndex;
    layer.residents.pop_back();

    if( !aGroup.bitmapQuads.empty() )
        std::erase( layer.bitmapResidents, aGroupId );

    countGroupAlpha( aGroup, layer, -1 );
    layer.alphaStale = true;
    layer.runsStale = true;

    if( aGroup.negative )
        layer.knockoutGroups--;

    aGroup.resident = false;
    m_sceneChanged = true;
}


bool GAL_MODERN::retainedTieOrderChanged( int aGroupId, GAL_GROUP& aGroup, SCENE_LAYER& aLayer )
{
    if( !aLayer.retainedPrefixIndex )
    {
        aLayer.retainedPrefixIndex.emplace();
        aLayer.retainedPrefixIndex->reserve( aLayer.residents.size() );

        for( int id : aLayer.residents )
            aLayer.retainedPrefixIndex->emplace( retainedPrefixHash( m_groups.at( id ).order ), id );
    }

    uint32_t candidateCounts[ARENA_COUNT] = {};
    bool     candidateCounted = false;

    auto countCandidate = [&]()
    {
        if( candidateCounted )
            return;

        countGroupArenas( aGroup, candidateCounts );
        candidateCounts[ARENA_POLYGON] = static_cast<uint32_t>( aGroup.polygonVertices.size() );
        candidateCounted = true;
    };

    auto candidateStart = [&]( int aArena )
    {
        const uint32_t count = candidateCounts[aArena];

        if( aArena == ARENA_POLYGON )
        {
            auto it = aLayer.polygons.freeSlots.find( count );
            return it != aLayer.polygons.freeSlots.end() && !it->second.empty()
                           ? it->second.back()
                           : static_cast<uint32_t>( aLayer.polygons.elements.size() );
        }

        const SCENE_ARENA<RHI_INSTANCE>& arena = aLayer.instances[aArena];
        auto it = arena.freeSlots.find( count );
        return it != arena.freeSlots.end() && !it->second.empty()
                       ? it->second.back()
                       : static_cast<uint32_t>( arena.elements.size() );
    };

    auto [first, last] = aLayer.retainedPrefixIndex->equal_range( retainedPrefixHash( aGroup.order ) );

    for( auto it = first; it != last; ++it )
    {
        const int peerId = it->second;

        if( peerId == aGroupId )
            continue;

        GAL_GROUP& peer = m_groups.at( peerId );

        if( !aGroup.order.SamePrefix( peer.order ) )
            continue;

        aGroup.order.contentTie = true;
        peer.order.contentTie = true;
        aGroup.order.contentHash = GroupContentFingerprint( aGroupId );
        peer.order.contentHash = GroupContentFingerprint( peerId );

        if( !aGroup.resident )
        {
            if( !( aGroup.order < peer.order ) && !( peer.order < aGroup.order ) )
                continue;

            countCandidate();

            for( int arena = 0; arena <= ARENA_POLYGON; ++arena )
            {
                if( candidateCounts[arena] == 0 || peer.slotCount[arena] == 0 )
                    continue;

                if( ( candidateStart( arena ) < peer.slotStart[arena] ) != ( aGroup.order < peer.order ) )
                    return true;
            }

            continue;
        }

        if( !( aGroup.order < peer.order ) && !( peer.order < aGroup.order ) )
            continue;

        for( int arena = 0; arena <= ARENA_POLYGON; ++arena )
        {
            if( aGroup.slotCount[arena] == 0 || peer.slotCount[arena] == 0 )
                continue;

            if( ( aGroup.slotStart[arena] < peer.slotStart[arena] ) != ( aGroup.order < peer.order ) )
                return true;
        }
    }

    return false;
}


void GAL_MODERN::resetScene()
{
    auto reset = []( auto& aArena )
    {
        aArena.elements.clear();
        aArena.freeSlots.clear();
        aArena.dirty.clear();
        aArena.dead      = 0;
        aArena.uploadAll = true;
    };

    for( auto& [id, layer] : m_sceneLayers )
    {
        for( SCENE_ARENA<RHI_INSTANCE>& arena : layer.instances )
            reset( arena );

        reset( layer.polygons );

        layer.residents.clear();
        layer.bitmapResidents.clear();
        layer.retainedPrefixIndex.reset();
        std::fill( std::begin( layer.alphaCount ), std::end( layer.alphaCount ), 0 );
        layer.bakedAlpha = -1;
        layer.alphaStale = false;
        layer.translucent = false;
        layer.runs.clear();
        layer.runsStale = false;
        layer.knockoutGroups = 0;
    }

    for( auto& [id, group] : m_groups )
        group.resident = false;

    m_openSceneLayer   = nullptr;
    m_openSceneLayerId = -1;
    m_sceneChanged     = true;
}


void GAL_MODERN::SetSceneLayerState( int aLayer, int aOrder, bool aVisible, bool aHasNegatives )
{
    SCENE_LAYER& layer = m_sceneLayers[aLayer];
    const float  depth = static_cast<float>( aOrder );

    if( layer.inOrder && layer.depth == depth && layer.visible == aVisible
        && layer.hasNegatives == aHasNegatives )
    {
        return;
    }

    if( !layer.inOrder )
        m_sceneOrder.push_back( aLayer );

    if( !layer.inOrder || layer.depth != depth )
        m_sceneOrderStale = true;

    // A hidden layer is not uploaded, so one that comes back has all of it to send
    if( aVisible && !layer.visible )
    {
        for( SCENE_ARENA<RHI_INSTANCE>& arena : layer.instances )
            arena.uploadAll = true;

        layer.polygons.uploadAll = true;
    }

    const bool rewrite = layer.inOrder && layer.hasNegatives != aHasNegatives;

    if( layer.hasNegatives != aHasNegatives )
        layer.runsStale = true;

    layer.inOrder      = true;
    layer.depth        = depth;
    layer.visible      = aVisible;
    layer.hasNegatives = aHasNegatives;
    m_sceneChanged     = true;

    // Whether the layer has knockouts decides how its elements' edges are drawn
    if( rewrite && !m_staticDirty )
    {
        for( int groupId : layer.residents )
            writeGroupToScene( m_groups.at( groupId ), layer );
    }
}


void GAL_MODERN::SetSceneLayerDim( int aLayer, const COLOR4D& aToward, double aKeep )
{
    // Without a fade the colour it would fade towards does not matter, so all such dims are equal
    RHI_LAYER_DIM dim;

    if( aKeep < 1.0 )
    {
        dim.toward[0] = static_cast<float>( aToward.r );
        dim.toward[1] = static_cast<float>( aToward.g );
        dim.toward[2] = static_cast<float>( aToward.b );
        dim.keep      = static_cast<float>( std::max( aKeep, 0.0 ) );
    }

    auto it = m_sceneLayers.find( aLayer );

    if( it == m_sceneLayers.end() && dim == RHI_LAYER_DIM() )
        return;

    SCENE_LAYER& layer = it != m_sceneLayers.end() ? it->second : m_sceneLayers[aLayer];

    if( layer.dim == dim )
        return;

    layer.dim      = dim;
    m_sceneChanged = true;
}


void GAL_MODERN::SetSceneLayerOverlayAlpha( int aLayer )
{
    SCENE_LAYER& layer = m_sceneLayers[aLayer];

    if( layer.overlayAlpha )
        return;

    layer.overlayAlpha = true;

    for( int groupId : layer.residents )
        writeGroupToScene( m_groups.at( groupId ), layer );

    m_sceneChanged = true;
}


void GAL_MODERN::sortSceneOrder()
{
    // Far layers first, as VIEW orders them
    std::stable_sort( m_sceneOrder.begin(), m_sceneOrder.end(),
                      [this]( int aLhs, int aRhs )
                      {
                          return m_sceneLayers.at( aLhs ).depth > m_sceneLayers.at( aRhs ).depth;
                      } );

    m_sceneOrderStale = false;
}


void GAL_MODERN::BeginSceneLayer( int aLayer )
{
    if( !m_isDrawing || !m_staticDirty )
        return;

    // A caller that does not describe its layers gets them in the order it opens them
    if( !m_sceneLayers.contains( aLayer ) || !m_sceneLayers.at( aLayer ).inOrder )
        SetSceneLayerState( aLayer, static_cast<int>( m_layerDepth ), true, m_negativesLayer );

    m_openSceneLayer   = &m_sceneLayers.at( aLayer );
    m_openSceneLayerId = aLayer;
}


void GAL_MODERN::trackGroupLod( int aGroupId, GAL_GROUP& aGroup, bool aTrack )
{
    // The list is ordered by threshold, so a group whose threshold changed moves
    if( aGroup.lodEntry && ( !aTrack || ( *aGroup.lodEntry )->first != aGroup.lodScale ) )
    {
        m_lodGroups.erase( *aGroup.lodEntry );
        aGroup.lodEntry.reset();
    }

    if( aTrack && !aGroup.lodEntry )
        aGroup.lodEntry = m_lodGroups.emplace( aGroup.lodScale, aGroupId );
}


void GAL_MODERN::SetSceneScale( double aScale )
{
    if( aScale == m_sceneScale )
        return;

    const double previous = m_sceneScale;
    m_sceneScale = aScale;

    // A pending rebuild places everything against the new scale anyway
    if( m_staticDirty )
        return;

    // Every other group already matches the scale, so only those whose threshold lies between the
    // two scales change. A zoom step crosses few of the many thresholds a board has.
    auto first = m_lodGroups.lower_bound( std::min( previous, aScale ) );
    auto last = m_lodGroups.lower_bound( std::max( previous, aScale ) );
    std::vector<int> crossed;

    for( auto entry = first; entry != last; ++entry )
        crossed.push_back( entry->second );

    std::sort( crossed.begin(), crossed.end(), [&]( int a, int b )
               {
                   const GAL_GROUP& lhs = m_groups.at( a );
                   const GAL_GROUP& rhs = m_groups.at( b );

                   if( lhs.lodScale != rhs.lodScale )
                       return lhs.lodScale < rhs.lodScale;

                   if( lhs.sceneLayer != rhs.sceneLayer )
                       return lhs.sceneLayer < rhs.sceneLayer;

                   return lhs.order < rhs.order;
               } );

    // Free lists reuse same-size slots from the end, so hide in reverse placement order.
    if( aScale < previous )
        std::reverse( crossed.begin(), crossed.end() );

    for( int groupId : crossed )
    {
        GAL_GROUP& group = m_groups.at( groupId );
        const bool wanted = group.lodScale < m_sceneScale;

        if( wanted == group.resident )
            continue;

        if( !wanted )
        {
            removeGroupFromScene( groupId, group );
            continue;
        }

        auto layerIt = m_sceneLayers.find( group.sceneLayer );

        if( layerIt == m_sceneLayers.end() || !layerIt->second.inOrder )
        {
            m_staticDirty = true;
            return;
        }

        placeGroupInScene( groupId, group, layerIt->second );
    }
}


bool GAL_MODERN::SyncScene()
{
    if( m_staticDirty || !m_sceneChanged )
        return false;

    // A failed upload leaves the scene for the next drawn frame to rebuild and report
    if( !flushScene() )
        m_staticDirty = true;

    m_sceneChanged = false;
    return true;
}


void GAL_MODERN::SetGroupPlacement( int aGroup, int aLayer, double aLodScale, bool aShown,
                                    const RETAINED_GROUP_ORDER& aOrder )
{
    auto it = m_groups.find( aGroup );

    if( it == m_groups.end() )
        return;

    GAL_GROUP& group = it->second;
    const bool wanted = aShown && aLodScale < m_sceneScale;
    RETAINED_GROUP_ORDER order = aOrder;

    if( group.order.contentTie && !order.contentTie && group.order.SamePrefix( order ) )
    {
        order.contentTie = true;
        order.contentHash = GroupContentFingerprint( aGroup );
    }

    if( group.resident && ( !wanted || group.sceneLayer != aLayer ) )
        removeGroupFromScene( aGroup, group );

    if( group.resident && !group.order.SamePrefix( order ) )
    {
        m_sceneLayers.at( group.sceneLayer ).retainedPrefixIndex.reset();
        m_staticDirty = true;
    }

    group.sceneLayer = aLayer;
    group.lodScale   = aLodScale;
    group.shown      = aShown;
    group.order      = std::move( order );

    trackGroupLod( aGroup, group, aShown && aLodScale > 0.0 && aLodScale < std::numeric_limits<double>::max() );

    if( !wanted || group.resident )
        return;

    if( m_staticDirty )
    {
        // Outside the rebuild itself the group waits for it
        if( m_openSceneLayer && m_openSceneLayerId == aLayer )
            placeGroupInScene( aGroup, group, *m_openSceneLayer );

        return;
    }

    auto layerIt = m_sceneLayers.find( aLayer );

    // Only a rebuild knows where a layer the scene has not drawn yet sits in the draw order
    if( layerIt == m_sceneLayers.end() || !layerIt->second.inOrder )
    {
        m_staticDirty = true;
        return;
    }

    if( retainedTieOrderChanged( aGroup, group, layerIt->second ) )
    {
        m_staticDirty = true;
        return;
    }

    placeGroupInScene( aGroup, group, layerIt->second );
}


template <typename T>
void GAL_MODERN::submitSpan( const std::vector<T>& aElements, RHI_BufferHandle aBuffer, const PATCH_SPAN& aSpan )
{
    const size_t bytes = static_cast<size_t>( aSpan.end - aSpan.begin ) * sizeof( T );

    RHI_GEOMETRY_UPLOAD upload;
    RhiUploadCopyStaging( upload, &aElements[aSpan.begin], bytes );
    upload.targetBuffer = aBuffer;
    upload.targetOffset = static_cast<size_t>( aSpan.begin ) * sizeof( T );
    m_renderThread->SubmitUpload( std::move( upload ) );
}


template <typename T>
bool GAL_MODERN::uploadSceneArena( SCENE_ARENA<T>& aArena )
{
    const size_t count = aArena.elements.size();

    // A stale upload flag on an empty arena would mark its layer edited on every flush
    if( count == 0 )
    {
        aArena.dirty.clear();
        aArena.uploadAll = false;
        return true;
    }

    if( aArena.capacity < count )
    {
        // Headroom lets a layer take new items without reallocating and re-uploading itself
        const size_t wanted = std::max( count + count / 4 + 64, aArena.capacity * 2 );

        if( m_renderThread && !ensureBuffer( aArena.buffer, aArena.capacity, wanted, sizeof( T ) ) )
            return false;

        if( !m_renderThread )
            aArena.capacity = wanted;

        aArena.uploadAll = true;
    }

    if( aArena.uploadAll )
    {
        aArena.dirty.clear();
        aArena.dirty.emplace_back();
        aArena.dirty.back().add( 0, static_cast<uint32_t>( count ) );
    }

    for( PATCH_SPAN span : aArena.dirty )
    {
        // A slot freed off the end of the arena after it was written leaves a span past the end
        span.end = std::min<uint32_t>( span.end, static_cast<uint32_t>( count ) );

        if( span.empty() )
            continue;

        if( m_renderThread && aArena.buffer )
            submitSpan( aArena.elements, aArena.buffer, span );
    }

    aArena.dirty.clear();
    aArena.uploadAll = false;
    return true;
}


std::shared_ptr<const RHI_RUN_TABLE> GAL_MODERN::updateSceneCullRuns( SCENE_LAYER& aLayer )
{
    auto changed = []( const auto& aArena, size_t aCounted )
    {
        return !aArena.dirty.empty() || aArena.elements.size() != aCounted
               || ( aArena.uploadAll && !aArena.elements.empty() );
    };

    bool any = !aLayer.cullRuns || changed( aLayer.polygons, aLayer.cullCounted[ARENA_POLYGON] );

    for( int arena = 0; arena < ARENA_INSTANCE_COUNT; ++arena )
        any = any || changed( aLayer.instances[arena], aLayer.cullCounted[arena] );

    if( !any )
        return nullptr;

    auto table =
            aLayer.cullRuns ? std::make_shared<RHI_RUN_TABLE>( *aLayer.cullRuns ) : std::make_shared<RHI_RUN_TABLE>();

    auto spans = []( const auto& aArena )
    {
        std::vector<std::pair<uint32_t, uint32_t>> out;

        for( const auto& span : aArena.dirty )
            out.emplace_back( span.begin, span.end );

        return out;
    };

    for( int arena = 0; arena < ARENA_INSTANCE_COUNT; ++arena )
    {
        const SCENE_ARENA<RHI_INSTANCE>& instances = aLayer.instances[arena];
        const RHI_SHAPE                  shape = RHI_SHAPE( arena / 2 );
        std::vector<RHI_RUN_BOUNDS>&     runs = arena % 2 ? table->negRuns[shape] : table->runs[shape];

        updateRuns( runs, instances.elements, spans( instances ), aLayer.cullCounted[arena],
                    !aLayer.cullRuns || instances.uploadAll, RHI_CULL_RUN_LENGTH,
                    [shape]( const RHI_INSTANCE* aFirst, size_t aCount )
                    {
                        return RhiInstanceRun( aFirst, aCount, shape );
                    } );

        aLayer.cullCounted[arena] = instances.elements.size();
    }

    updateRuns( table->runs[RHI_SHAPE_POLYGON], aLayer.polygons.elements, spans( aLayer.polygons ),
                aLayer.cullCounted[ARENA_POLYGON],
                !aLayer.cullRuns || aLayer.polygons.uploadAll, RHI_CULL_VERTEX_RUN_LENGTH, &RhiVertexRun );

    aLayer.cullCounted[ARENA_POLYGON] = aLayer.polygons.elements.size();
    return table;
}


bool GAL_MODERN::flushScene()
{
    // Past this many dead elements an arena that is mostly holes is repacked
    static constexpr uint32_t COMPACT_MIN_DEAD = 256;

    bool ok = true;

    m_openSceneLayer   = nullptr;
    m_openSceneLayerId = -1;

    auto wantsCompaction = []( const auto& aArena )
    {
        return aArena.dead > COMPACT_MIN_DEAD && static_cast<size_t>( aArena.dead ) * 2 > aArena.elements.size();
    };

    if( m_sceneOrderStale )
        sortSceneOrder();

    for( int layerId : m_sceneOrder )
    {
        SCENE_LAYER& layer = m_sceneLayers.at( layerId );

        // Edits to a hidden layer stay in its arenas until it is shown
        if( !layer.visible )
            continue;

        if( layer.alphaStale )
        {
            int minAlpha = 256;
            int maxAlpha = -1;

            for( int alpha = 0; alpha < 256; ++alpha )
            {
                if( layer.alphaCount[alpha] > 0 )
                {
                    minAlpha = std::min( minAlpha, alpha );
                    maxAlpha = std::max( maxAlpha, alpha );
                }
            }

            // Same rule as the full rebuild, so one translucent alpha shared by the whole layer
            // is applied once at composite
            const int hoisted = ( maxAlpha >= 0 && minAlpha == maxAlpha && minAlpha < 255 ) ? minAlpha : -1;

            const bool translucent = minAlpha < 255;

            if( ( hoisted >= 0 ) != ( layer.bakedAlpha >= 0 ) || translucent != layer.translucent )
            {
                layer.bakedAlpha = hoisted;
                layer.translucent = translucent;

                for( int groupId : layer.residents )
                    writeGroupToScene( m_groups.at( groupId ), layer );
            }

            layer.bakedAlpha = hoisted;
            layer.translucent = translucent;
            layer.alphaStale = false;
        }

        for( int arena = 0; arena < ARENA_INSTANCE_COUNT; ++arena )
        {
            if( wantsCompaction( layer.instances[arena] ) )
            {
                compactSceneArena( layer, layer.instances[arena], arena );
                layer.runsStale = true;
            }
        }

        if( wantsCompaction( layer.polygons ) )
        {
            compactSceneArena( layer, layer.polygons, ARENA_POLYGON );
            layer.runsStale = true;
        }

        if( layer.runsStale )
        {
            const bool composited = sceneLayerComposited( layer );

            buildSceneRuns( layer );

            // Going through the temp target or not decides how the elements carry their alpha
            if( sceneLayerComposited( layer ) != composited )
            {
                for( int groupId : layer.residents )
                    writeGroupToScene( m_groups.at( groupId ), layer );
            }
        }

        if( std::shared_ptr<const RHI_RUN_TABLE> runs = updateSceneCullRuns( layer ) )
            layer.cullRuns = std::move( runs );

        for( int arena = 0; arena < ARENA_INSTANCE_COUNT; ++arena )
            ok = uploadSceneArena( layer.instances[arena] ) && ok;

        ok = uploadSceneArena( layer.polygons ) && ok;
    }

    return ok;
}


void GAL_MODERN::SetSceneDrawOrdered( bool aOrdered )
{
    if( m_sceneDrawOrdered == aOrdered )
        return;

    m_sceneDrawOrdered = aOrdered;

    for( auto& [id, layer] : m_sceneLayers )
        layer.runsStale = layer.runsStale || layer.knockoutGroups > 0;

    m_sceneChanged = true;
}


void GAL_MODERN::buildSceneRuns( SCENE_LAYER& aLayer )
{
    aLayer.runs.clear();
    aLayer.runsStale = false;

    // The groups say whether the layer has knockouts. The view's flag for it is not set on
    // every path that loads a Gerber file.
    if( aLayer.knockoutGroups == 0 || !m_sceneDrawOrdered )
        return;

    std::vector<const GAL_GROUP*> placed;
    placed.reserve( aLayer.residents.size() );

    for( int groupId : aLayer.residents )
        placed.push_back( &m_groups.at( groupId ) );

    std::sort( placed.begin(), placed.end(),
               []( const GAL_GROUP* aLhs, const GAL_GROUP* aRhs )
               {
                   return aLhs->placeSeq < aRhs->placeSeq;
               } );

    // Order matters once something dark follows a knockout. Polygons need the runs too, a
    // clear one is told from a dark one only by the run it is in.
    bool ordered = false;
    bool cleared = false;

    auto joins = [&]( const POLARITY_RUN& aLast, const POLARITY_RUN& aNext )
    {
        for( int shape = 0; shape < RHI_SHAPES; ++shape )
        {
            if( aNext.count[shape] != 0 && aLast.count[shape] != 0
                && aLast.start[shape] + aLast.count[shape] != aNext.start[shape] )
            {
                return false;
            }
        }

        return aLast.negative == aNext.negative;
    };

    auto add = [&]( const POLARITY_RUN& aNext )
    {
        if( aNext.Empty() )
            return;

        if( aNext.negative )
            ordered = ordered || aNext.count[RHI_SHAPE_POLYGON] > 0;
        else
            ordered = ordered || cleared;

        cleared = cleared || aNext.negative;

        if( aLayer.runs.empty() || !joins( aLayer.runs.back(), aNext ) )
        {
            aLayer.runs.push_back( aNext );
            return;
        }

        POLARITY_RUN& last = aLayer.runs.back();

        for( int shape = 0; shape < RHI_SHAPES; ++shape )
        {
            if( last.count[shape] == 0 )
                last.start[shape] = aNext.start[shape];

            last.count[shape] += aNext.count[shape];
        }
    };

    for( const GAL_GROUP* group : placed )
    {
        // Normal elements sit in one set of arenas and knockouts in another. A group's polygons
        // carry no flag of their own and take the polarity the group was recorded in.
        POLARITY_RUN dark;
        POLARITY_RUN clear;
        clear.negative = true;

        for( int shape = 0; shape < RHI_INSTANCED_SHAPES; ++shape )
        {
            dark.start[shape] = group->slotStart[arenaOf( shape, false )];
            dark.count[shape] = group->slotCount[arenaOf( shape, false )];
            clear.start[shape] = group->slotStart[arenaOf( shape, true )];
            clear.count[shape] = group->slotCount[arenaOf( shape, true )];
        }

        POLARITY_RUN& polygons = group->negative ? clear : dark;
        polygons.start[RHI_SHAPE_POLYGON] = group->slotStart[ARENA_POLYGON];
        polygons.count[RHI_SHAPE_POLYGON] = group->slotCount[ARENA_POLYGON];

        add( dark );
        add( clear );
    }

    if( !ordered )
        aLayer.runs.clear();
}


bool GAL_MODERN::assignSlicesByOrder( std::vector<LAYER_RANGE>& aRanges, size_t aFirst ) const
{
    float lowest = std::numeric_limits<float>::max();
    float highest = std::numeric_limits<float>::lowest();

    for( const auto& [id, layer] : m_sceneLayers )
    {
        if( layer.inOrder )
        {
            lowest = std::min( lowest, layer.depth );
            highest = std::max( highest, layer.depth );
        }
    }

    if( lowest > highest )
        return false;

    // A layer takes one unit of order around its own, so the tenth steps a group advances by
    // sort between their layer and the next. A unit of margin keeps those steps inside.
    const float base = lowest - 1.0f;
    const float span = highest - lowest + 2.0f;

    for( size_t ii = aFirst; ii < aRanges.size(); ++ii )
    {
        LAYER_RANGE& range = aRanges[ii];

        range.depthNear = std::clamp( ( range.depth - 0.5f - base ) / span, 0.0f, 1.0f );
        range.depthFar  = std::clamp( ( range.depth + 0.5f - base ) / span, 0.0f, 1.0f );
    }

    return true;
}


void GAL_MODERN::appendSceneRanges( FRAME_DRAW_DESC& aDesc ) const
{
    auto count = []( const auto& aArena )
    {
        return static_cast<uint32_t>( aArena.elements.size() );
    };

    const size_t firstRange = aDesc.layerRanges.size();

    for( int layerId : m_sceneOrder )
    {
        const SCENE_LAYER& layer = m_sceneLayers.at( layerId );

        if( !layer.visible )
            continue;

        LAYER_RANGE range;
        range.depth        = layer.depth;
        const bool composited = sceneLayerComposited( layer );

        range.alpha        = ( layer.bakedAlpha >= 0 && composited )
                                     ? static_cast<float>( layer.bakedAlpha ) / 255.0f
                                     : 1.0f;
        range.hasNegatives = layer.hasNegatives || !layer.runs.empty();
        range.target       = TARGET_CACHED;

        range.singleCoverage = !composited && layer.translucent;
        range.depthKnockout  = !composited && layer.hasNegatives;
        range.dim            = layer.dim;
        range.overlayAlpha   = layer.overlayAlpha;

        range.runStart = static_cast<uint32_t>( aDesc.polarityRuns.size() );
        range.runCount = static_cast<uint32_t>( layer.runs.size() );
        aDesc.polarityRuns.insert( aDesc.polarityRuns.end(), layer.runs.begin(), layer.runs.end() );

        for( int shape = 0; shape < RHI_INSTANCED_SHAPES; ++shape )
        {
            const SCENE_ARENA<RHI_INSTANCE>& normals = layer.instances[arenaOf( shape, false )];
            const SCENE_ARENA<RHI_INSTANCE>& negatives = layer.instances[arenaOf( shape, true )];

            range.count[shape] = count( normals );
            range.negCount[shape] = count( negatives );
            range.buffer[shape] = normals.buffer;
            range.negBuffer[shape] = negatives.buffer;
        }

        range.count[RHI_SHAPE_POLYGON] = count( layer.polygons );
        range.buffer[RHI_SHAPE_POLYGON] = layer.polygons.buffer;
        range.cullRuns = layer.cullRuns;

        // A placed group's bitmaps are drawn with its layer. The quads stay owned by the group,
        // and the descriptor is fetched again whenever the scene changes.
        range.bitmapStart = static_cast<uint32_t>( aDesc.bitmapDraws.size() );

        for( int groupId : layer.bitmapResidents )
        {
            for( const BITMAP_QUAD& bq : m_groups.at( groupId ).bitmapQuads )
            {
                if( bq.texture && bq.vertexBuffer )
                    aDesc.bitmapDraws.push_back( { bq.vertexBuffer, bq.texture, 6 } );
            }
        }

        range.bitmapCount = static_cast<uint32_t>( aDesc.bitmapDraws.size() ) - range.bitmapStart;

        if( !range.HasGeometry() )
            continue;

        for( int shape = 0; shape < RHI_SHAPES; ++shape )
        {
            aDesc.count[shape] += range.count[shape];
            aDesc.negCount[shape] += range.negCount[shape];
        }

        aDesc.layerRanges.push_back( range );
    }

    const size_t slices = aDesc.layerRanges.size() - firstRange;

    if( slices == 0 )
        return;

    aDesc.relativeDepth = true;

    if( assignSlicesByOrder( aDesc.layerRanges, firstRange ) )
        return;

    // Later layers are nearer, so each takes the slice in front of the one before it
    for( size_t ii = 0; ii < slices; ++ii )
    {
        LAYER_RANGE& range = aDesc.layerRanges[firstRange + ii];

        range.depthFar  = 1.0f - static_cast<float>( ii ) / static_cast<float>( slices );
        range.depthNear = 1.0f - static_cast<float>( ii + 1 ) / static_cast<float>( slices );
    }
}


template <typename GROUP, typename INSTANCE_FN, typename VERTEX_FN>
static void forEachElement( GROUP& aGroup, INSTANCE_FN aInstance, VERTEX_FN aVertex )
{
    for( std::vector<RHI_INSTANCE>& instances : aGroup.instances )
        std::ranges::for_each( instances, aInstance );

    std::ranges::for_each( aGroup.polygonVertices, aVertex );
}


template <typename INSTANCE_FN, typename VERTEX_FN>
bool GAL_MODERN::patchStaticGroup( int aGroup, INSTANCE_FN aInstance, VERTEX_FN aVertex )
{
    auto slotIt = m_staticGroupSlots.find( aGroup );

    if( slotIt == m_staticGroupSlots.end() )
        return false;

    const RHI_SHAPE_COUNTS sizes = m_staticPool.Sizes();

    for( const STATIC_GROUP_SLOT& slot : slotIt->second )
    {
        // A slot from an earlier build is stale once the pool shrank, so drop the group's slots and
        // let the caller rebuild rather than write out of bounds
        for( int shape = 0; shape < RHI_SHAPES; ++shape )
        {
            if( static_cast<size_t>( slot.start[shape] ) + slot.count[shape] > sizes[shape] )
            {
                m_staticGroupSlots.erase( slotIt );
                return false;
            }
        }

        for( int shape = 0; shape < RHI_INSTANCED_SHAPES; ++shape )
        {
            auto first = m_staticPool.instances[shape].begin() + slot.start[shape];
            std::for_each( first, first + slot.count[shape], aInstance );
        }

        auto first = m_staticPool.polygonVertices.begin() + slot.start[RHI_SHAPE_POLYGON];
        std::for_each( first, first + slot.count[RHI_SHAPE_POLYGON], aVertex );

        for( int shape = 0; shape < RHI_SHAPES; ++shape )
            m_staticPatches[shape].add( slot.start[shape], slot.count[shape] );
    }

    return true;
}


void GAL_MODERN::rewriteResidentGroup( int aGroupNumber, GAL_GROUP& aGroup )
{
    // A pending rebuild re-places every group, so patching the old scene would be wasted
    if( !aGroup.resident || m_staticDirty )
        return;

    SCENE_LAYER& layer = m_sceneLayers.at( aGroup.sceneLayer );

    if( aGroup.order.contentTie && retainedTieOrderChanged( aGroupNumber, aGroup, layer ) )
    {
        m_staticDirty = true;
        return;
    }

    writeGroupToScene( aGroup, layer );
    m_sceneChanged = true;
}


void GAL_MODERN::ChangeGroupColor( int aGroupNumber, const COLOR4D& aNewColor )
{
    auto it = m_groups.find( aGroupNumber );

    if( it == m_groups.end() )
        return;

    GAL_GROUP& group = it->second;

    const uint8_t cr = packColorChannel( aNewColor.r );
    const uint8_t cg = packColorChannel( aNewColor.g );
    const uint8_t cb = packColorChannel( aNewColor.b );
    const uint8_t ca = packColorChannel( aNewColor.a );

    // A recolour of every layer asks for the colour most groups already have. The groups that
    // come here are painted in one colour, so their first element speaks for the rest.
    auto unchanged = [&]( const auto& aElements )
    {
        return !aElements.empty() && aElements.front().r == cr && aElements.front().g == cg
               && aElements.front().b == cb && aElements.front().a == ca;
    };

    if( std::ranges::any_of( group.instances, unchanged ) || unchanged( group.polygonVertices ) )
        return;

    SCENE_LAYER* sceneLayer = ( group.resident && !m_staticDirty ) ? &m_sceneLayers.at( group.sceneLayer )
                                                                   : nullptr;

    if( sceneLayer )
        countGroupAlpha( group, *sceneLayer, -1 );

    forEachElement(
            group,
            [&]( RHI_INSTANCE& aInstance )
            {
                aInstance.SetColor( cr, cg, cb, aInstance.HasFlag( RHI_INSTANCE_NEGATIVE ) ? 255 : ca );
            },
            [&]( RHI_VERTEX& aVertex )
            {
                aVertex.SetColor( cr, cg, cb, group.negative ? 255 : ca );
            } );

    group.contentFingerprintValid = false;

    if( group.order.contentTie )
        group.order.contentHash = GroupContentFingerprint( aGroupNumber );

    if( sceneLayer )
    {
        countGroupAlpha( group, *sceneLayer, 1 );
        sceneLayer->alphaStale = true;
    }

    rewriteResidentGroup( aGroupNumber, group );

    // A group VIEW has placed lives in the retained scene, or is being kept out of it
    if( group.sceneLayer >= 0 )
        return;

    // Patch the static pool in place, or rebuild when the group has no slot there yet
    auto recolor = [&]( auto& aElement )
    {
        aElement.SetColor( cr, cg, cb, ca );
    };

    if( !patchStaticGroup( aGroupNumber, recolor, recolor ) )
        m_staticDirty = true;
}


void GAL_MODERN::ChangeGroupDepth( int aGroupNumber, int aDepth )
{
    auto it = m_groups.find( aGroupNumber );

    if( it == m_groups.end() )
        return;

    GAL_GROUP&  group = it->second;
    const float depth = static_cast<float>( aDepth );

    // The zone shader reads the layer depth from u
    auto setInstanceDepth = [depth]( RHI_INSTANCE& aInstance )
    {
        aInstance.SetDepth( depth );
    };

    auto setVertexDepth = [depth]( RHI_VERTEX& aVertex )
    {
        aVertex.u = glm::packHalf1x16( depth );
    };

    forEachElement( group, setInstanceDepth, setVertexDepth );

    group.baseDepth = depth;
    group.contentFingerprintValid = false;

    if( group.order.contentTie )
        group.order.contentHash = GroupContentFingerprint( aGroupNumber );

    rewriteResidentGroup( aGroupNumber, group );

    if( group.sceneLayer >= 0 )
        return;

    if( !patchStaticGroup( aGroupNumber, setInstanceDepth, setVertexDepth ) )
        m_staticDirty = true;
}


void GAL_MODERN::ChangeGroupUndimmed( int aGroupNumber, bool aUndimmed )
{
    auto it = m_groups.find( aGroupNumber );

    if( it == m_groups.end() )
        return;

    GAL_GROUP& group = it->second;

    if( group.undimmed == aUndimmed )
        return;

    group.undimmed = aUndimmed;

    // The zone shader reads the exemption from v
    forEachElement(
            group,
            [&]( RHI_INSTANCE& aInstance )
            {
                if( aUndimmed )
                    aInstance.SetFlag( RHI_INSTANCE_UNDIMMED );
                else
                    aInstance.ClearFlag( RHI_INSTANCE_UNDIMMED );
            },
            [&]( RHI_VERTEX& aVertex )
            {
                aVertex.v = glm::packHalf1x16( aUndimmed ? 1.0f : 0.0f );
            } );

    group.contentFingerprintValid = false;

    if( group.order.contentTie )
        group.order.contentHash = GroupContentFingerprint( aGroupNumber );

    rewriteResidentGroup( aGroupNumber, group );
}


void GAL_MODERN::DeleteGroup( int aGroupNumber )
{
    auto it = m_groups.find( aGroupNumber );

    if( it == m_groups.end() )
        return;

    if( it->second.resident )
        removeGroupFromScene( aGroupNumber, it->second );

    trackGroupLod( aGroupNumber, it->second, false );
    countBitmapRefs( it->second, -1 );
    releaseBitmapQuads( it->second.bitmapQuads, false );

    m_groups.erase( it );
}


void GAL_MODERN::countBitmapRefs( const GAL_GROUP& aGroup, int aDelta )
{
    for( const BITMAP_QUAD& bq : aGroup.bitmapQuads )
    {
        if( bq.ownsTexture || !bq.texture )
            continue;

        for( auto& [id, entry] : m_bitmapTextureCache )
        {
            if( entry.texture == bq.texture )
            {
                entry.groupRefs += aDelta;
                break;
            }
        }
    }
}


void GAL_MODERN::evictBitmapCacheEntries()
{
    auto cachedBytes = [&]()
    {
        size_t bytes = 0;

        for( const auto& [id, entry] : m_bitmapTextureCache )
            bytes += entry.bytes;

        return bytes;
    };

    while( m_bitmapTextureCache.size() > BITMAP_CACHE_MAX_ENTRIES || cachedBytes() > BITMAP_CACHE_MAX_BYTES )
    {
        // An entry used this frame may back an emitted quad, so only older ones are evicted
        auto lru = m_bitmapTextureCache.end();

        for( auto it = m_bitmapTextureCache.begin(); it != m_bitmapTextureCache.end(); ++it )
        {
            if( it->second.lastUse >= m_bitmapFrameStartSeq )
                continue;

            if( it->second.groupRefs > 0 )
                continue;

            if( lru == m_bitmapTextureCache.end() || it->second.lastUse < lru->second.lastUse )
                lru = it;
        }

        if( lru == m_bitmapTextureCache.end() )
            break;

        if( lru->second.texture )
            retireTexture( lru->second.texture );

        m_bitmapTextureCache.erase( lru );
    }
}


void GAL_MODERN::ClearCache()
{
    for( auto& [id, group] : m_groups )
        releaseBitmapQuads( group.bitmapQuads, false );

    resetScene();

    // Keep m_groupCounter running. VIEW::Clear leaves stale group ids on its items, and a restarted
    // counter hands those ids to new groups that the next item's recache then deletes
    m_lodGroups.clear();
    m_groups.clear();

    // The bitmap textures belong to the cache being cleared. Retiring them keeps in-flight frames valid.
    for( auto& [id, entry] : m_bitmapTextureCache )
    {
        if( entry.texture )
            retireTexture( entry.texture );
    }

    m_bitmapTextureCache.clear();

    // The slots describe groups that no longer exist
    clearStaticSlots();
    m_staticDirty = true;
}


void GAL_MODERN::Save()
{
    m_transformStack.push_back( m_currentTransform );
}


void GAL_MODERN::Restore()
{
    if( !m_transformStack.empty() )
    {
        m_currentTransform = m_transformStack.back();
        m_transformStack.pop_back();
    }
    else
    {
        m_currentTransform.SetIdentity();
    }

    recomputeTransformIdentity();
}


void GAL_MODERN::Transform( const MATRIX3x3D& aTransformation )
{
    m_currentTransform = m_currentTransform * aTransformation;
    recomputeTransformIdentity();
}


void GAL_MODERN::Rotate( double aAngle )
{
    MATRIX3x3D rot;
    rot.SetIdentity();
    rot.SetRotation( aAngle );
    Transform( rot );
}


void GAL_MODERN::Translate( const VECTOR2D& aTranslation )
{
    MATRIX3x3D tr;
    tr.SetIdentity();
    tr.SetTranslation( aTranslation );
    Transform( tr );
}


void GAL_MODERN::Scale( const VECTOR2D& aScale )
{
    MATRIX3x3D sc;
    sc.SetIdentity();
    sc.SetScale( aScale );
    Transform( sc );
}


int GAL_MODERN::GetSwapInterval() const
{
    // The render thread presents on the vertical blank and takes only the newest frame, so the
    // panel need not hold a repaint back to the refresh rate the way an unsynced canvas does
    return 1;
}


static void toFloat4( float* aDest, const COLOR4D& aColor )
{
    aDest[0] = static_cast<float>( aColor.r );
    aDest[1] = static_cast<float>( aColor.g );
    aDest[2] = static_cast<float>( aColor.b );
    aDest[3] = static_cast<float>( aColor.a );
}


template <typename QUADS>
static void appendBitmapDraws( FRAME_DRAW_DESC& aDesc, const QUADS& aQuads )
{
    for( const auto& bq : aQuads )
    {
        if( bq.texture && bq.vertexBuffer )
            aDesc.bitmapDraws.push_back( { bq.vertexBuffer, bq.texture, 6 } );
    }
}


// For a pool other than the one the descriptor's buffers come from, so each range points at its own
// pool's buffers and runs
template <typename POOL>
static void appendPoolRanges( FRAME_DRAW_DESC& aDesc, const POOL& aPool, const std::vector<LAYER_RANGE>& aRanges,
                              std::vector<LAYER_RANGE>& aDest, size_t aAt, uint32_t aBitmapBase )
{
    const uint32_t runBase = static_cast<uint32_t>( aDesc.polarityRuns.size() );

    aDesc.polarityRuns.insert( aDesc.polarityRuns.end(), aPool.polarityRuns.begin(), aPool.polarityRuns.end() );

    std::vector<LAYER_RANGE> ranges;

    for( LAYER_RANGE range : aRanges )
    {
        range.runStart    += runBase;
        range.buffer       = aPool.buffer;
        range.negBuffer    = aPool.buffer;
        range.bitmapStart += aBitmapBase;
        range.cullRuns     = aPool.cullRuns;
        ranges.push_back( std::move( range ) );
    }

    aDest.insert( aDest.begin() + aAt, ranges.begin(), ranges.end() );

    for( int shape = 0; shape < RHI_SHAPES; ++shape )
    {
        aDesc.count[shape] += aPool.lastCount[shape];
        aDesc.negCount[shape] += aPool.lastNegCount[shape];
    }
}


static void buildDescForPool( FRAME_DRAW_DESC& aDesc, const auto& aPool, const std::vector<LAYER_RANGE>& aRanges )
{
    aDesc.buffer   = aPool.buffer;
    aDesc.count    = aPool.lastCount;
    aDesc.negCount = aPool.lastNegCount;

    appendBitmapDraws( aDesc, aPool.bitmapQuads );

    aDesc.layerRanges  = aRanges;
    aDesc.polarityRuns = aPool.polarityRuns;
}


FRAME_DRAW_DESC GAL_MODERN::GetStaticDrawDesc() const
{
    FRAME_DRAW_DESC desc;
    buildDescForPool( desc, m_staticPool, m_staticLayerRanges );

    // Grid item grids go first, over the grid and under everything else, at the far end of the depth
    // range. Their ranges index their own pool's buffers.
    appendPoolRanges( desc, m_gridPool, m_gridLayerRanges, desc.layerRanges, 0, 0 );

    for( size_t ii = 0; ii < m_gridLayerRanges.size(); ++ii )
    {
        desc.layerRanges[ii].bitmapStart = 0;
        desc.layerRanges[ii].bitmapCount = 0;
        desc.layerRanges[ii].depthNear   = 1.0f;
        desc.layerRanges[ii].depthFar    = 1.0f;
    }

    appendSceneRanges( desc );

    // With kept targets the grid goes under the board, as the first thing in the static
    // target. That target is redrawn whenever the camera or the grid settings change.
    desc.drawGrid  = m_drawGrid && m_persistentTargets;
    desc.gridFirst = true;
    toFloat4( desc.backgroundColor, m_clearColor );

    desc.bufferGeneration = m_bufferGeneration;
    PopulateCameraUBOs( desc );
    return desc;
}


FRAME_DRAW_DESC GAL_MODERN::GetDynamicDrawDesc() const
{
    FRAME_DRAW_DESC desc;
    buildDescForPool( desc, buf(), m_dynamicLayerRanges );

    for( LAYER_RANGE& range : desc.layerRanges )
        range.cullRuns = buf().cullRuns;

    desc.bufferGeneration = m_bufferGeneration;

    // The overlay target sits above the non-cached one, as it does when the other backends
    // composite their targets. Its ranges index their own pool's buffers.
    const GEOMETRY_BUFFER_SET& overlay = obuf();
    const uint32_t             bitmapBase = static_cast<uint32_t>( desc.bitmapDraws.size() );

    appendBitmapDraws( desc, overlay.bitmapQuads );
    appendPoolRanges( desc, overlay, m_overlayLayerRanges, desc.layerRanges, desc.layerRanges.size(), bitmapBase );

    // Draw order is far to near, so each range takes the slice in front of the one before it.
    // The overlay's ranges come last and so in front of the non-cached ones.
    if( keepsRelativeDepth( buf() ) && assignSlicesByOrder( desc.layerRanges, 0 ) )
    {
        desc.relativeDepth     = true;
        desc.testAgainstStatic = true;
    }
    else if( keepsRelativeDepth( buf() ) )
    {
        const size_t slices = std::count_if( desc.layerRanges.begin(), desc.layerRanges.end(),
                                             []( const LAYER_RANGE& aRange )
                                             {
                                                 return aRange.HasGeometry();
                                             } );
        size_t       slice = 0;

        for( LAYER_RANGE& range : desc.layerRanges )
        {
            if( !range.HasGeometry() )
                continue;

            range.depthFar  = 1.0f - static_cast<float>( slice ) / static_cast<float>( slices );
            range.depthNear = 1.0f - static_cast<float>( slice + 1 ) / static_cast<float>( slices );
            slice++;
        }

        desc.relativeDepth = true;
    }

    desc.drawGrid = m_drawGrid && !m_persistentTargets;
    toFloat4( desc.backgroundColor, m_clearColor );

    PopulateCameraUBOs( desc );
    PopulateCursorUBO( desc );
    desc.drawCursor = m_drawCursor;
    return desc;
}


FRAME_DRAW_DESC GAL_MODERN::GetDrawDesc() const
{
    // A merged view for tests that inspect combined counts. The buffers are the dynamic pool's, so
    // the static ranges' offsets are not valid against them.
    FRAME_DRAW_DESC desc = GetDynamicDrawDesc();

    for( int shape = 0; shape < RHI_SHAPES; ++shape )
    {
        desc.count[shape] += m_staticPool.lastCount[shape];
        desc.negCount[shape] += m_staticPool.lastNegCount[shape];
    }

    desc.layerRanges.insert( desc.layerRanges.end(), m_staticLayerRanges.begin(), m_staticLayerRanges.end() );

    appendSceneRanges( desc );
    appendBitmapDraws( desc, m_staticPool.bitmapQuads );
    return desc;
}


void GAL_MODERN::PopulateCameraUBOs( FRAME_DRAW_DESC& aDesc ) const
{
    // The grid shader's NDC to world projection. NDC y points up and screen y down, so
    // y_screen = (1 - (ndc_y + 1) * 0.5) * height.
    float w  = static_cast<float>( m_screenWidth );
    float h  = static_cast<float>( m_screenHeight );
    float hw = w * 0.5f;
    float hh = h * 0.5f;

    const auto& S = m_screenWorldMatrix;

    // M = screenWorldMatrix * ndcToScreen, with ndcToScreen = [hw 0 hw; 0 -hh hh; 0 0 1] row-major
    float M00 = static_cast<float>( S.m_data[0][0] ) * hw;
    float M01 = static_cast<float>( S.m_data[0][1] ) * -hh;
    float M02 = static_cast<float>( S.m_data[0][0] * hw + S.m_data[0][1] * hh + S.m_data[0][2] );
    float M10 = static_cast<float>( S.m_data[1][0] ) * hw;
    float M11 = static_cast<float>( S.m_data[1][1] ) * -hh;
    float M12 = static_cast<float>( S.m_data[1][0] * hw + S.m_data[1][1] * hh + S.m_data[1][2] );
    float M20 = static_cast<float>( S.m_data[2][0] ) * hw;
    float M21 = static_cast<float>( S.m_data[2][1] ) * -hh;
    float M22 = static_cast<float>( S.m_data[2][0] * hw + S.m_data[2][1] * hh + S.m_data[2][2] );

    // A std140 mat4 is column-major. Vulkan and Metal flip NDC y relative to OpenGL, so column 1,
    // the ndc.y term, is negated there.
    RHI_BACKEND backend = m_deviceProxy.GetBackend();
    bool        flipY   = RhiBackendNeedsYFlip( backend );
    float       ySign   = flipY ? -1.0f : 1.0f;

    float* ip = aDesc.gridUBO.invProjection;
    ip[0]  = M00;  ip[1]  = M10;  ip[2]  = 0.0f;  ip[3]  = M20;
    ip[4]  = M01 * ySign;  ip[5]  = M11 * ySign;  ip[6]  = 0.0f;  ip[7]  = M21 * ySign;
    ip[8]  = 0.0f; ip[9]  = 0.0f; ip[10] = 1.0f;  ip[11] = 0.0f;
    ip[12] = M02;  ip[13] = M12;  ip[14] = 0.0f;  ip[15] = M22;

    // Inverting the grid shader's transform keeps geometry and grid on the same math
    MATRIX3x3D ndcToWorld( M00, M01, M02,
                           M10, M11, M12,
                           M20, M21, M22 );
    MATRIX3x3D worldToNdc = ndcToWorld.Inverse();

    float* fp = aDesc.frameUBO.projection;
    fp[0]  = static_cast<float>( worldToNdc.m_data[0][0] );
    fp[1]  = static_cast<float>( worldToNdc.m_data[1][0] );
    fp[2]  = 0.0f;
    fp[3]  = 0.0f;
    fp[4]  = static_cast<float>( worldToNdc.m_data[0][1] );
    fp[5]  = static_cast<float>( worldToNdc.m_data[1][1] );
    fp[6]  = 0.0f;
    fp[7]  = 0.0f;
    // Scale z so GAL depths land in about [-1,1] before the backend adjustment, as unscaled ones would
    // be clipped
    fp[8]  = 0.0f;
    fp[9]  = 0.0f;
    fp[10] = aDesc.relativeDepth ? 1.0f / FRAME_DRAW_DESC::RELATIVE_DEPTH_RANGE
                                 : 1.0f / static_cast<float>( GAL::MAX_DEPTH );
    fp[11] = 0.0f;
    fp[12] = static_cast<float>( worldToNdc.m_data[0][2] );
    fp[13] = static_cast<float>( worldToNdc.m_data[1][2] );
    fp[14] = 0.0f;
    fp[15] = 1.0f;
    RhiAdjustProjectionForBackend( fp, m_deviceProxy.GetBackend() );

    // Shaders size antialiased edges in backing pixels, m_dpiScale times the logical size the
    // projection uses
    float bw = w * m_dpiScale;
    float bh = h * m_dpiScale;
    aDesc.frameUBO.viewport[0] = bw;
    aDesc.frameUBO.viewport[1] = bh;
    aDesc.frameUBO.viewScale = static_cast<float>( m_zoomFactor );

    aDesc.gridUBO.viewport[0]   = bw;
    aDesc.gridUBO.viewport[1]   = bh;
    aDesc.gridUBO.gridOrigin[0] = static_cast<float>( m_gridOrigin.x );
    aDesc.gridUBO.gridOrigin[1] = static_cast<float>( m_gridOrigin.y );
    VECTOR2D visibleGrid = GetVisibleGridSize();
    aDesc.gridUBO.gridSpacing[0] = static_cast<float>( visibleGrid.x );
    aDesc.gridUBO.gridSpacing[1] = static_cast<float>( visibleGrid.y );
    aDesc.gridUBO.gridAngle = 0.0f;

    switch( m_gridStyle )
    {
    case GRID_STYLE::LINES:       aDesc.gridUBO.gridStyle = 0; break;
    case GRID_STYLE::DOTS:        aDesc.gridUBO.gridStyle = 1; break;
    case GRID_STYLE::SMALL_CROSS: aDesc.gridUBO.gridStyle = 2; break;
    default:                      aDesc.gridUBO.gridStyle = 0; break;
    }

    toFloat4( aDesc.gridUBO.gridColor, m_gridColor );
    toFloat4( aDesc.gridUBO.axisColorX, m_axesColor );
    toFloat4( aDesc.gridUBO.axisColorY, m_axesColor );

    // The shader draws the axes and the grid whenever their colour is visible
    if( !m_axesEnabled )
    {
        aDesc.gridUBO.axisColorX[3] = 0.0f;
        aDesc.gridUBO.axisColorY[3] = 0.0f;
    }

    if( !gridShown() )
        aDesc.gridUBO.gridColor[3] = 0.0f;

    const double pixel = screenPixelSize();

    aDesc.gridUBO.minorLineWidth = std::fmax( 1.0f, m_gridLineWidth );
    aDesc.gridUBO.majorLineWorld = 2.0f * gridMinorLineWidth( pixel, m_dpiScale );
    aDesc.gridUBO.worldPixelSize = static_cast<float>( pixel / m_dpiScale );
    aDesc.gridUBO.gridTick       = m_gridTick;
}


void GAL_MODERN::PopulateCursorUBO( FRAME_DRAW_DESC& aDesc ) const
{
    float w = std::fmax( static_cast<float>( m_screenWidth ), 1.0f );
    float h = std::fmax( static_cast<float>( m_screenHeight ), 1.0f );
    bool  flipY = RhiBackendNeedsYFlip( m_deviceProxy.GetBackend() );

    VECTOR2D cursorScreen = m_worldScreenMatrix * m_cursorPosition;
    aDesc.cursorUBO.cursorPos[0] = static_cast<float>( cursorScreen.x / ( w * 0.5 ) - 1.0 );
    float ndcY = static_cast<float>( 1.0 - cursorScreen.y / ( h * 0.5 ) );
    aDesc.cursorUBO.cursorPos[1] = flipY ? -ndcY : ndcY;
    aDesc.cursorUBO.viewport[0]  = w * m_dpiScale;
    aDesc.cursorUBO.viewport[1]  = h * m_dpiScale;
    // Dimmed while no tool has asked for it and it shows only because it is always shown
    const COLOR4D cursorColor = getCursorColor();

    toFloat4( aDesc.cursorUBO.crosshairColor, cursorColor );
    aDesc.cursorUBO.snapColor[0] = 1.0f;
    aDesc.cursorUBO.snapColor[1] = 1.0f;
    aDesc.cursorUBO.snapColor[2] = 1.0f;
    aDesc.cursorUBO.snapColor[3] = 1.0f;
    aDesc.cursorUBO.crosshairWidth = 1.0f;
    aDesc.cursorUBO.snapRadius     = 5.0f;
    aDesc.cursorUBO.showSnap       = 0;

    switch( m_crossHairMode )
    {
    case CROSS_HAIR_MODE::SMALL_CROSS:         aDesc.cursorUBO.cursorType = 0; break;
    case CROSS_HAIR_MODE::FULLSCREEN_CROSS:    aDesc.cursorUBO.cursorType = 1; break;
    case CROSS_HAIR_MODE::FULLSCREEN_DIAGONAL: aDesc.cursorUBO.cursorType = 2; break;
    }

    aDesc.cursorUBO.cursorSize = 80.0f;

    // No cursor while no tool has asked for one and it is not always shown
    aDesc.drawCursor = IsCursorEnabled();
}


void GAL_MODERN::beginUpdate()
{
    m_isUpdating = true;
}


void GAL_MODERN::endUpdate()
{
    m_isUpdating = false;
}
