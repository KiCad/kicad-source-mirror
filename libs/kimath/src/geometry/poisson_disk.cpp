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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 */

#include <geometry/poisson_disk.h>

#include <algorithm>
#include <cmath>
#include <utility>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/uniform_real_distribution.hpp>


std::vector<VECTOR2D> POISSON_DISK::ToroidalUnitTile( double aMinDist, uint32_t aSeed )
{
    std::vector<VECTOR2D> samples;

    if( aMinDist <= 0.0 || aMinDist > 0.5 )
        return samples;  // unsupported / degenerate

    // The cell count must tile the torus exactly: with a truncated seam cell the +-2 cell
    // neighborhood spans less than aMinDist of actual distance near the wrap seam, letting
    // cross-seam sample pairs slip under the minimum distance.  1/gridN <= aMinDist/sqrt(2)
    // keeps at most one sample per cell, and 2/gridN >= aMinDist holds for any aMinDist
    // <= 0.5, so the +-2 scan still covers the exclusion radius.
    const int    gridN    = std::max( 3, (int) std::ceil( std::sqrt( 2.0 ) / aMinDist ) );
    const double cellSize = 1.0 / gridN;

    std::vector<int> grid( (size_t) gridN * gridN, -1 );
    std::vector<int> active;

    auto cellOf =
            [&]( const VECTOR2D& p ) -> std::pair<int, int>
            {
                int gx = (int) std::floor( p.x / cellSize );
                int gy = (int) std::floor( p.y / cellSize );
                gx = ( ( gx % gridN ) + gridN ) % gridN;
                gy = ( ( gy % gridN ) + gridN ) % gridN;
                return { gx, gy };
            };

    auto torDistSq =
            []( double dx, double dy ) -> double
            {
                if( dx >  0.5 ) dx -= 1.0;
                if( dx < -0.5 ) dx += 1.0;
                if( dy >  0.5 ) dy -= 1.0;
                if( dy < -0.5 ) dy += 1.0;
                return dx * dx + dy * dy;
            };

    boost::random::mt19937                           rng( aSeed );
    boost::random::uniform_real_distribution<double> uniform( 0.0, 1.0 );

    // Initial sample
    VECTOR2D p( uniform( rng ), uniform( rng ) );
    auto [gx, gy] = cellOf( p );
    samples.push_back( p );
    grid[(size_t) gy * gridN + gx] = 0;
    active.push_back( 0 );

    constexpr int K = 30;  // candidate attempts per active sample (Bridson default)

    while( !active.empty() )
    {
        boost::random::uniform_int_distribution<size_t> pickIdx( 0, active.size() - 1 );
        size_t   idx      = pickIdx( rng );
        VECTOR2D base     = samples[active[idx]];
        bool     accepted = false;

        for( int k = 0; k < K; ++k )
        {
            double   theta = uniform( rng ) * 2.0 * M_PI;
            double   rho   = aMinDist + uniform( rng ) * aMinDist;  // annulus [r, 2r)
            VECTOR2D cand( base.x + rho * std::cos( theta ),
                           base.y + rho * std::sin( theta ) );

            // Wrap into the unit square.
            cand.x -= std::floor( cand.x );
            cand.y -= std::floor( cand.y );

            auto [cx, cy]  = cellOf( cand );
            bool tooClose = false;

            // Any sample within aMinDist is within +-2 background cells, modulo
            // toroidal wraparound.
            for( int dy = -2; dy <= 2 && !tooClose; ++dy )
            {
                int ny = ( ( cy + dy ) % gridN + gridN ) % gridN;

                for( int dx = -2; dx <= 2 && !tooClose; ++dx )
                {
                    int nx = ( ( cx + dx ) % gridN + gridN ) % gridN;
                    int idxN = grid[(size_t) ny * gridN + nx];

                    if( idxN < 0 )
                        continue;

                    if( torDistSq( samples[idxN].x - cand.x, samples[idxN].y - cand.y )
                            < aMinDist * aMinDist )
                    {
                        tooClose = true;
                    }
                }
            }

            if( tooClose )
                continue;

            samples.push_back( cand );
            grid[(size_t) cy * gridN + cx] = (int) samples.size() - 1;
            active.push_back( (int) samples.size() - 1 );
            accepted = true;
            break;
        }

        if( !accepted )
        {
            active[idx] = active.back();
            active.pop_back();
        }
    }

    return samples;
}
