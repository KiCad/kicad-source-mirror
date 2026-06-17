/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>

#include <gal/opengl/utils.h>

using KIGFX::chooseResizeStrategy;
using KIGFX::VRAM_RESIZE_STRATEGY;

/**
 * Tests for the predictive GPU buffer resize strategy guard that keeps a large-board
 * defragmentResize() from tripping a fatal driver out-of-memory abort.
 */
BOOST_AUTO_TEST_SUITE( GalResizeStrategy )

static const size_t MB = static_cast<size_t>( 1024 ) * 1024;

// Unknown VRAM (no NVX/ATI extension, e.g. Intel/Mesa) must preserve the historical fast path.
BOOST_AUTO_TEST_CASE( UnknownVramKeepsGpuCopy )
{
    BOOST_CHECK( chooseResizeStrategy( 0, 64 * MB, 128 * MB, 0.15 )
                 == VRAM_RESIZE_STRATEGY::GPU_COPY );

    // Even an absurd request stays on the fast path when VRAM is unknown.
    BOOST_CHECK( chooseResizeStrategy( 0, 4096 * MB, 8192 * MB, 0.15 )
                 == VRAM_RESIZE_STRATEGY::GPU_COPY );
}

// Plenty of headroom for both buffers plus margin -> fast GPU-side copy.
BOOST_AUTO_TEST_CASE( AmpleVramUsesGpuCopy )
{
    // old + new = 192 MB, +15% margin = ~221 MB; 2 GB free is plenty.
    BOOST_CHECK( chooseResizeStrategy( 2048 * MB, 64 * MB, 128 * MB, 0.15 )
                 == VRAM_RESIZE_STRATEGY::GPU_COPY );
}

// Room for the new buffer (plus margin) but not both -> stage through RAM.
BOOST_AUTO_TEST_CASE( TightVramStagesThroughRam )
{
    // old=64, new=128. Both+margin = ~221 MB (too big). New+margin = ~147 MB (fits in 160).
    BOOST_CHECK( chooseResizeStrategy( 160 * MB, 64 * MB, 128 * MB, 0.15 )
                 == VRAM_RESIZE_STRATEGY::RAM_STAGE );
}

// Not even the new buffer (plus margin) fits -> refuse and fall back to software.
BOOST_AUTO_TEST_CASE( InsufficientVramRefuses )
{
    BOOST_CHECK( chooseResizeStrategy( 100 * MB, 64 * MB, 128 * MB, 0.15 )
                 == VRAM_RESIZE_STRATEGY::REFUSE );
}

// The margin is honored at the boundary between strategies.
BOOST_AUTO_TEST_CASE( MarginBoundaries )
{
    const size_t oldBytes = 100 * MB;
    const size_t newBytes = 100 * MB;

    // Exactly old+new with a zero margin -> GPU_COPY (>= comparison).
    BOOST_CHECK( chooseResizeStrategy( 200 * MB, oldBytes, newBytes, 0.0 )
                 == VRAM_RESIZE_STRATEGY::GPU_COPY );

    // One byte short of old+new -> drops to RAM_STAGE.
    BOOST_CHECK( chooseResizeStrategy( 200 * MB - 1, oldBytes, newBytes, 0.0 )
                 == VRAM_RESIZE_STRATEGY::RAM_STAGE );

    // Exactly the new buffer with zero margin -> RAM_STAGE.
    BOOST_CHECK( chooseResizeStrategy( 100 * MB, oldBytes, newBytes, 0.0 )
                 == VRAM_RESIZE_STRATEGY::RAM_STAGE );

    // One byte short of the new buffer -> REFUSE.
    BOOST_CHECK( chooseResizeStrategy( 100 * MB - 1, oldBytes, newBytes, 0.0 )
                 == VRAM_RESIZE_STRATEGY::REFUSE );
}

// A negative margin is clamped to zero rather than loosening the budget.
BOOST_AUTO_TEST_CASE( NegativeMarginClampedToZero )
{
    BOOST_CHECK( chooseResizeStrategy( 200 * MB, 100 * MB, 100 * MB, -0.5 )
                 == VRAM_RESIZE_STRATEGY::GPU_COPY );

    BOOST_CHECK( chooseResizeStrategy( 200 * MB - 1, 100 * MB, 100 * MB, -0.5 )
                 == VRAM_RESIZE_STRATEGY::RAM_STAGE );
}

BOOST_AUTO_TEST_SUITE_END()
