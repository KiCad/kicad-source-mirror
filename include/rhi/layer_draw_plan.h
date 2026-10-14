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

#ifndef RHI_LAYER_DRAW_PLAN_H
#define RHI_LAYER_DRAW_PLAN_H

#include <rhi/frame_packet.h>

#include <vector>


namespace KIGFX
{

enum class LAYER_DRAW_MODE : uint8_t
{
    BLEND,      /**< ordinary alpha blending */
    NEGATIVE,   /**< erase what the layer has drawn so far, inside a temp target */
    ONCE,       /**< blend, but let each pixel take the layer only once */
    MARK        /**< write depth only, so the layer's later draws stay out of these pixels */
};

/**
 * One draw of a layer range.
 */
struct LAYER_DRAW_STEP
{
    RHI_SHAPE       shape;
    LAYER_DRAW_MODE mode;
    bool            negatives;  /**< draws the range's negative elements instead of its normal ones */
    bool            nearHalf;   /**< drawn in the nearer half of the layer's depth slice */
    bool            ordered;    /**< part of a polarity run, taken from the range's drawing order */
    uint32_t        start;      /**< first element, or first vertex of a polygon step */
    uint32_t        count;
};

/**
 * Order the draws of one layer range.
 *
 * Depth knockouts are marked in the nearer half of the layer's depth slice before the rest is drawn in the
 * farther half, so the marks always win without reaching the next layer. Through a temp target, normal
 * elements come before the negatives that erase them. Polarity runs are drawn in emission order.
 */
inline std::vector<LAYER_DRAW_STEP> RhiPlanLayerDraws( const LAYER_RANGE&               aRange,
                                                       const std::vector<POLARITY_RUN>& aRuns = {} )
{
    const LAYER_DRAW_MODE fill = aRange.singleCoverage ? LAYER_DRAW_MODE::ONCE : LAYER_DRAW_MODE::BLEND;

    std::vector<LAYER_DRAW_STEP> steps;

    if( aRange.runCount > 0 && aRange.runStart + aRange.runCount <= aRuns.size() )
    {
        for( uint32_t ii = aRange.runStart; ii < aRange.runStart + aRange.runCount; ++ii )
        {
            const POLARITY_RUN&   run = aRuns[ii];
            const LAYER_DRAW_MODE mode = run.negative ? LAYER_DRAW_MODE::NEGATIVE : LAYER_DRAW_MODE::BLEND;

            for( int shape = 0; shape < RHI_SHAPES; ++shape )
            {
                steps.push_back( { RHI_SHAPE( shape ), mode, run.negative, false, true, run.start[shape],
                                   run.count[shape] } );
            }
        }

        return steps;
    }

    auto instances = [&]( LAYER_DRAW_MODE aMode, bool aNegatives, bool aNearHalf )
    {
        for( int shape = 0; shape < RHI_INSTANCED_SHAPES; ++shape )
        {
            steps.push_back( { RHI_SHAPE( shape ), aMode, aNegatives, aNearHalf, false,
                               aNegatives ? aRange.negStart[shape] : aRange.start[shape],
                               aNegatives ? aRange.negCount[shape] : aRange.count[shape] } );
        }
    };

    if( aRange.depthKnockout )
        instances( LAYER_DRAW_MODE::MARK, true, true );

    instances( fill, false, false );

    if( !aRange.depthKnockout )
        instances( LAYER_DRAW_MODE::NEGATIVE, true, false );

    steps.push_back( { RHI_SHAPE_POLYGON, fill, false, false, false, aRange.start[RHI_SHAPE_POLYGON],
                       aRange.count[RHI_SHAPE_POLYGON] } );

    return steps;
}

} // namespace KIGFX

#endif // RHI_LAYER_DRAW_PLAN_H
