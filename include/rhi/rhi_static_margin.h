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

#ifndef RHI_STATIC_MARGIN_H
#define RHI_STATIC_MARGIN_H

#include <rhi/frame_packet.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>

/**
 * The static scene is drawn into a target larger than the view by a margin on each side, so a pan
 * within the margin shifts the target instead of redrawing the scene.
 *
 * Margins and shifts are in FRAME_UBO_DATA::viewport pixels, the units the shaders snap in. Target
 * rows run with normalized device Y on every backend, so one shift serves both a copy and a composite.
 */

namespace KIGFX
{

/** How far the view has moved since the static target was drawn, in whole frame pixels. */
struct RHI_VIEW_SHIFT
{
    int x = 0;
    int y = 0;
};


/**
 * @return frame uniforms that draw the same view, at the same pixel size, centred in a target
 *         @p aMargin pixels larger on each side.
 */
inline FRAME_UBO_DATA RhiWidenFrame( const FRAME_UBO_DATA& aFrame, uint32_t aMargin )
{
    FRAME_UBO_DATA wide = aFrame;
    const float    w = aFrame.viewport[0];
    const float    h = aFrame.viewport[1];

    if( aMargin == 0 || w <= 0.0f || h <= 0.0f )
        return wide;

    const float sx = w / ( w + 2.0f * aMargin );
    const float sy = h / ( h + 2.0f * aMargin );

    /** Scaling normalized device X and Y about the centre scales the first two rows. */
    for( int col = 0; col < 4; ++col )
    {
        wide.projection[col * 4] *= sx;
        wide.projection[col * 4 + 1] *= sy;
    }

    wide.viewport[0] = w + 2.0f * aMargin;
    wide.viewport[1] = h + 2.0f * aMargin;
    return wide;
}


/** @return grid uniforms for the target of RhiWidenFrame(). */
inline GRID_UBO_DATA RhiWidenGrid( const GRID_UBO_DATA& aGrid, uint32_t aMargin )
{
    GRID_UBO_DATA wide = aGrid;
    const float   w = aGrid.viewport[0];
    const float   h = aGrid.viewport[1];

    if( aMargin == 0 || w <= 0.0f || h <= 0.0f )
        return wide;

    const float sx = w / ( w + 2.0f * aMargin );
    const float sy = h / ( h + 2.0f * aMargin );

    /** The inverse takes the scale back out of the first two columns. */
    for( int row = 0; row < 4; ++row )
    {
        wide.invProjection[row] /= sx;
        wide.invProjection[4 + row] /= sy;
    }

    wide.viewport[0] = w + 2.0f * aMargin;
    wide.viewport[1] = h + 2.0f * aMargin;
    return wide;
}


/** @return true when two frames differ only by translation. */
inline bool RhiViewMovedOnly( const FRAME_UBO_DATA& aDrawn, const FRAME_UBO_DATA& aNow )
{
    if( aDrawn.viewport[0] != aNow.viewport[0] || aDrawn.viewport[1] != aNow.viewport[1] )
        return false;

    for( int ii = 0; ii < 16; ++ii )
    {
        if( ii == 12 || ii == 13 )
            continue;

        const float a = aDrawn.projection[ii];
        const float b = aNow.projection[ii];

        if( std::abs( a - b ) > 1e-6f * std::max( std::abs( a ), std::abs( b ) ) )
            return false;
    }

    return true;
}


/**
 * @return the shift from @p aDrawn to @p aNow when they differ only by whole pixels, or nothing.
 *
 * A fractional shift would land elements on different pixels than a fresh draw snaps them to. The
 * tolerance absorbs float error in a large board's offset.
 */
inline std::optional<RHI_VIEW_SHIFT> RhiViewShift( const FRAME_UBO_DATA& aDrawn, const FRAME_UBO_DATA& aNow )
{
    if( !RhiViewMovedOnly( aDrawn, aNow ) )
        return std::nullopt;

    const double x = ( double( aNow.projection[12] ) - aDrawn.projection[12] ) * aNow.viewport[0] * 0.5;
    const double y = ( double( aNow.projection[13] ) - aDrawn.projection[13] ) * aNow.viewport[1] * 0.5;

    constexpr double tolerance = 0.02;

    if( std::abs( x - std::round( x ) ) > tolerance || std::abs( y - std::round( y ) ) > tolerance )
        return std::nullopt;

    return RHI_VIEW_SHIFT{ static_cast<int>( std::lround( x ) ), static_cast<int>( std::lround( y ) ) };
}


/** @return true when the grid of @p aNow is that of @p aDrawn moved, as RhiViewShift() found the frame. */
inline bool RhiGridMovedOnly( const GRID_UBO_DATA& aDrawn, const GRID_UBO_DATA& aNow )
{
    GRID_UBO_DATA moved = aDrawn;

    /** The inverse is recomputed each frame, so entries other than the offset may round differently. */
    for( int ii = 0; ii < 16; ++ii )
    {
        const float a = aDrawn.invProjection[ii];
        const float b = aNow.invProjection[ii];

        if( ii != 12 && ii != 13 && std::abs( a - b ) > 1e-6f * std::max( std::abs( a ), std::abs( b ) ) )
            return false;

        moved.invProjection[ii] = b;
    }

    return moved == aNow;
}

} // namespace KIGFX

#endif // RHI_STATIC_MARGIN_H
