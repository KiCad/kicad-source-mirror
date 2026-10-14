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

#ifndef RHI_SUPERSAMPLE_H
#define RHI_SUPERSAMPLE_H

#include <rhi/frame_packet.h>

#include <algorithm>
#include <cstdint>

/**
 * High quality antialiasing draws several texels per pixel and the present averages them, so the
 * shaders draw hard edges in such a target, as the OpenGL canvas does.
 *
 * Like the OpenGL canvas, strokes snap to pixel corners and the scene is drawn half a pixel right and
 * down so odd-width strokes land on whole pixels. The uniforms built here carry that offset.
 */

namespace KIGFX
{

/** Offset of a supersampled target from where its frame puts the scene, in pixels. */
constexpr float RHI_SUPERSAMPLE_OFFSET = 0.5f;


/** @return frame uniforms that draw @p aFrame into a target of @p aFactor texels per pixel. */
inline FRAME_UBO_DATA RhiSupersampleFrame( const FRAME_UBO_DATA& aFrame, uint32_t aFactor )
{
    FRAME_UBO_DATA frame = aFrame;

    frame.renderScale = static_cast<float>( std::max( aFactor, 1u ) );

    if( aFactor <= 1 || aFrame.viewport[0] <= 0.0f || aFrame.viewport[1] <= 0.0f )
        return frame;

    const float dx = 2.0f * RHI_SUPERSAMPLE_OFFSET / aFrame.viewport[0];
    const float dy = 2.0f * RHI_SUPERSAMPLE_OFFSET / aFrame.viewport[1];

    /** A move in normalized device X and Y adds the last row to the first two. */
    for( int col = 0; col < 4; ++col )
    {
        frame.projection[col * 4] += dx * aFrame.projection[col * 4 + 3];
        frame.projection[col * 4 + 1] += dy * aFrame.projection[col * 4 + 3];
    }

    return frame;
}


/** @return grid uniforms for the target of RhiSupersampleFrame(). */
inline GRID_UBO_DATA RhiSupersampleGrid( const GRID_UBO_DATA& aGrid, uint32_t aFactor )
{
    GRID_UBO_DATA grid = aGrid;

    grid.renderScale = static_cast<float>( std::max( aFactor, 1u ) );

    if( aFactor <= 1 || aGrid.viewport[0] <= 0.0f || aGrid.viewport[1] <= 0.0f )
        return grid;

    const float dx = 2.0f * RHI_SUPERSAMPLE_OFFSET / aGrid.viewport[0];
    const float dy = 2.0f * RHI_SUPERSAMPLE_OFFSET / aGrid.viewport[1];

    /** The inverse takes the move back out before anything else. */
    for( int row = 0; row < 4; ++row )
        grid.invProjection[12 + row] -= dx * aGrid.invProjection[row] + dy * aGrid.invProjection[4 + row];

    return grid;
}

} // namespace KIGFX

#endif // RHI_SUPERSAMPLE_H
