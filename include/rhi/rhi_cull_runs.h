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

#ifndef RHI_CULL_RUNS_H
#define RHI_CULL_RUNS_H

#include <rhi/frame_packet.h>
#include <rhi/rhi_instance.h>
#include <rhi/rhi_vertex.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

#include <glm/glm.hpp>

/**
 * Off-screen culling of instanced geometry in runs of RHI_CULL_RUN_LENGTH elements in draw order.
 *
 * Only whole runs that cannot reach the target are dropped, so culling never changes a covered pixel.
 * Bounds carry a reach in pixels because pixel padding and antialiasing do not scale with zoom.
 */

namespace KIGFX
{

constexpr uint32_t RHI_CULL_RUN_LENGTH = 256;

/**
 * Reach of every element beyond its own extent for antialiasing padding, pixel snapping and the edge.
 */
constexpr float RHI_CULL_PIXEL_SLACK = 4.0f;

struct RHI_RUN_BOUNDS
{
    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();
    float pixels = RHI_CULL_PIXEL_SLACK;    /**< reach beyond the world bounds, in pixels */

    /**
     * Lowest level of detail among the run's elements, 0 when one is always shown. The run shows nothing
     * while the view scale is not above it.
     */
    float lod = std::numeric_limits<float>::max();

    bool Empty() const { return minX > maxX; }

    void Add( float aX, float aY, float aWorldReach, float aPixelReach, float aLod = 0.0f )
    {
        lod = std::min( lod, aLod );
        minX = std::min( minX, aX - aWorldReach );
        minY = std::min( minY, aY - aWorldReach );
        maxX = std::max( maxX, aX + aWorldReach );
        maxY = std::max( maxY, aY + aWorldReach );
        pixels = std::max( pixels, RHI_CULL_PIXEL_SLACK + aPixelReach );
    }
};


/**
 * Runs of each array a layer range draws from, indexed by element / RHI_CULL_RUN_LENGTH. Negatives have
 * their own entries for a range that keeps them in separate buffers.
 */
struct RHI_RUN_TABLE
{
    /**
     * By RHI_SHAPE. Polygon runs are of RHI_CULL_VERTEX_RUN_LENGTH vertices.
     */
    std::array<std::vector<RHI_RUN_BOUNDS>, RHI_SHAPES> runs;
    std::array<std::vector<RHI_RUN_BOUNDS>, RHI_SHAPES> negRuns;

    const std::vector<RHI_RUN_BOUNDS>& Of( RHI_SHAPE aShape, bool aNegatives ) const
    {
        return aNegatives ? negRuns[aShape] : runs[aShape];
    }
};


/**
 * How far an instance reaches from its position, in world units and in pixels.
 */
inline void RhiInstanceReach( const RHI_INSTANCE& aInstance, RHI_SHAPE aShape, float& aWorld, float& aPixels )
{
    const float scale = std::max( std::abs( glm::unpackHalf1x16( aInstance.scaleX ) ),
                                  std::abs( glm::unpackHalf1x16( aInstance.scaleY ) ) );

    float extent = 0.0f;

    if( aShape == RHI_SHAPE_PAD )
        extent = std::sqrt( 2.0f ) * std::max( std::abs( aInstance.params[0] ), std::abs( aInstance.params[1] ) );
    else
        extent = std::abs( aInstance.params[0] ) + std::abs( aInstance.params[1] );

    aPixels = static_cast<float>( aInstance.pixelPad ) / 32.0f;

    if( aShape == RHI_SHAPE_TRACK && aInstance.HasFlag( RHI_INSTANCE_LENGTH_PAD ) )
        aPixels += aInstance.params[2];

    /**
     * A screen-space track gives its size in pixels. The other shaders ignore the flag.
     */
    if( aShape == RHI_SHAPE_TRACK && aInstance.HasFlag( RHI_INSTANCE_SCREEN_SPACE ) )
    {
        aWorld = 0.0f;
        aPixels += extent * scale;
    }
    else
    {
        aWorld = extent * scale;
    }
}


inline RHI_RUN_BOUNDS RhiInstanceRun( const RHI_INSTANCE* aInstances, size_t aCount, RHI_SHAPE aShape )
{
    RHI_RUN_BOUNDS run;

    for( size_t ii = 0; ii < aCount; ++ii )
    {
        /**
         * A removed element is collapsed to a point and covers nothing.
         */
        if( aInstances[ii].scaleX == 0 && aInstances[ii].scaleY == 0 )
            continue;

        float world = 0.0f;
        float pixels = 0.0f;

        RhiInstanceReach( aInstances[ii], aShape, world, pixels );

        const bool  levelled = aShape == RHI_SHAPE_TRACK && aInstances[ii].HasFlag( RHI_INSTANCE_SCALE_LOD );
        const float lod = levelled ? aInstances[ii].params[3] : 0.0f;

        run.Add( aInstances[ii].x, aInstances[ii].y, world, pixels, lod );
    }

    return run;
}


inline RHI_RUN_BOUNDS RhiVertexRun( const RHI_VERTEX* aVertices, size_t aCount )
{
    RHI_RUN_BOUNDS run;

    for( size_t ii = 0; ii < aCount; ++ii )
        run.Add( aVertices[ii].x, aVertices[ii].y, 0.0f, 0.0f );

    return run;
}


/**
 * Polygon vertices per run, three per triangle.
 */
constexpr uint32_t RHI_CULL_VERTEX_RUN_LENGTH = 3 * RHI_CULL_RUN_LENGTH;


constexpr uint32_t RhiCullRunLength( int aShape )
{
    return aShape == RHI_SHAPE_POLYGON ? RHI_CULL_VERTEX_RUN_LENGTH : RHI_CULL_RUN_LENGTH;
}


/**
 * The world rectangle a frame shows and the world size of one of its pixels.
 */
struct RHI_CULL_VIEW
{
    float minX = 0.0f;
    float minY = 0.0f;
    float maxX = 0.0f;
    float maxY = 0.0f;
    float worldPerPixel = 0.0f;
    float viewScale = 0.0f;     /**< the view's scale, which the elements' levels of detail are compared with */
    bool  valid = false;

    bool Reaches( const RHI_RUN_BOUNDS& aRun ) const
    {
        if( !valid )
            return true;

        /**
         * Every element of the run is past its level of detail, decided as the track shader does.
         */
        if( aRun.Empty() || ( aRun.lod > 0.0f && !( aRun.lod < viewScale ) ) )
            return false;

        const float grow = aRun.pixels * worldPerPixel;

        return aRun.maxX + grow >= minX && aRun.minX - grow <= maxX && aRun.maxY + grow >= minY
               && aRun.minY - grow <= maxY;
    }
};


/**
 * Invert the frame's 2D projection over the normalized device square.
 */
inline RHI_CULL_VIEW RhiCullView( const FRAME_UBO_DATA& aFrame )
{
    RHI_CULL_VIEW view;

    /**
     * Column-major, so NDC = M * world + t.
     */
    const glm::dmat2 m( aFrame.projection[0], aFrame.projection[1], aFrame.projection[4], aFrame.projection[5] );
    const glm::dvec2 t( aFrame.projection[12], aFrame.projection[13] );

    if( glm::determinant( m ) == 0.0 || aFrame.viewport[0] <= 0.0f || aFrame.viewport[1] <= 0.0f )
        return view;

    const glm::dmat2 inverse = glm::inverse( m );

    double minX = std::numeric_limits<double>::max();
    double minY = std::numeric_limits<double>::max();
    double maxX = std::numeric_limits<double>::lowest();
    double maxY = std::numeric_limits<double>::lowest();

    for( double nx : { -1.0, 1.0 } )
    {
        for( double ny : { -1.0, 1.0 } )
        {
            const glm::dvec2 world = inverse * ( glm::dvec2( nx, ny ) - t );

            minX = std::min( minX, world.x );
            minY = std::min( minY, world.y );
            maxX = std::max( maxX, world.x );
            maxY = std::max( maxY, world.y );
        }
    }

    /**
     * Measured as the shaders do, from how far a world unit moves along X on screen.
     */
    const double pixelsPerUnit = 0.5 * std::hypot( m[0][0] * aFrame.viewport[0], m[0][1] * aFrame.viewport[1] );

    if( pixelsPerUnit <= 0.0 )
        return view;

    view.minX = static_cast<float>( minX );
    view.minY = static_cast<float>( minY );
    view.maxX = static_cast<float>( maxX );
    view.maxY = static_cast<float>( maxY );
    view.worldPerPixel = static_cast<float>( 1.0 / pixelsPerUnit );
    view.viewScale = aFrame.viewScale;
    view.valid = true;
    return view;
}


/**
 * Runs a pass looked at and how many it drew, to tell how well culling works.
 */
struct RHI_CULL_COUNT
{
    uint64_t runs = 0;
    uint64_t drawn = 0;
    uint64_t draws = 0;     /**< stretches drawn, each one draw call */
};


/**
 * Call aDraw( first, count ) for each stretch of [aStart, aStart + aCount) whose runs can reach aView.
 * Without runs the whole span is one stretch.
 */
template <typename DRAW>
void RhiForVisibleStretches( const std::vector<RHI_RUN_BOUNDS>* aRuns, uint32_t aRunLength, uint32_t aStart,
                             uint32_t aCount, const RHI_CULL_VIEW& aView, DRAW&& aDraw,
                             RHI_CULL_COUNT* aTally = nullptr )
{
    if( aCount == 0 )
        return;

    const uint32_t end = aStart + aCount;
    const uint32_t spanned = ( end - 1 ) / aRunLength - aStart / aRunLength + 1;

    if( aTally )
        aTally->runs += spanned;

    if( !aRuns || !aView.valid || static_cast<size_t>( ( end - 1 ) / aRunLength ) >= aRuns->size() )
    {
        if( aTally )
        {
            aTally->drawn += spanned;
            aTally->draws++;
        }

        aDraw( aStart, aCount );
        return;
    }

    uint32_t stretchStart = aStart;
    bool     open = false;

    for( uint32_t run = aStart / aRunLength; run * aRunLength < end; ++run )
    {
        const uint32_t runStart = std::max( aStart, run * aRunLength );
        const uint32_t runEnd = std::min( end, ( run + 1 ) * aRunLength );

        if( aView.Reaches( ( *aRuns )[run] ) )
        {
            if( aTally )
                aTally->drawn++;

            if( !open )
                stretchStart = runStart;

            open = true;
        }
        else if( open )
        {
            aDraw( stretchStart, runStart - stretchStart );
            open = false;

            if( aTally )
                aTally->draws++;
        }

        if( runEnd == end && open )
        {
            aDraw( stretchStart, end - stretchStart );

            if( aTally )
                aTally->draws++;
        }
    }
}

/**
 * A range without runs or with bitmaps always reaches.
 */
inline bool RhiRangeReaches( const LAYER_RANGE& aRange, const RHI_CULL_VIEW& aView )
{
    if( !aRange.cullRuns || !aView.valid || aRange.bitmapCount > 0 )
        return true;

    const RHI_RUN_TABLE& runs = *aRange.cullRuns;
    bool                 reaches = false;

    auto found = [&]( uint32_t, uint32_t )
    {
        reaches = true;
    };

    for( int shape = 0; shape < RHI_SHAPES && !reaches; ++shape )
    {
        RhiForVisibleStretches( &runs.runs[shape], RhiCullRunLength( shape ), aRange.start[shape],
                                aRange.count[shape], aView, found );
        RhiForVisibleStretches( &runs.negRuns[shape], RhiCullRunLength( shape ), aRange.negStart[shape],
                                aRange.negCount[shape], aView, found );
    }

    return reaches;
}

} // namespace KIGFX

#endif // RHI_CULL_RUNS_H
