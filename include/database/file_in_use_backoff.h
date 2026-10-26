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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KICAD_FILE_IN_USE_BACKOFF_H
#define KICAD_FILE_IN_USE_BACKOFF_H

#include <algorithm>
#include <chrono>
#include <random>

/**
 * @return the wait before retrying an open that found the file in use.  Instances opening one file together hold it
 *         for milliseconds, so the window starts at 100 ms and doubles up to 1.6 s, and the wait is drawn from its
 *         upper half so that readers that failed together do not all come back together.
 */
inline std::chrono::milliseconds FileInUseBackoff( int aAttempt, std::minstd_rand& aRng )
{
    constexpr long long FIRST_WAIT_MS = 100;
    constexpr int       MAX_DOUBLINGS = 4;

    const long long ceiling = FIRST_WAIT_MS << std::clamp( aAttempt, 0, MAX_DOUBLINGS );

    std::uniform_int_distribution<long long> spread( ceiling / 2, ceiling );

    return std::chrono::milliseconds( spread( aRng ) );
}

#endif // KICAD_FILE_IN_USE_BACKOFF_H
