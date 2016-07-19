/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file  edgeshrink.cpp
 * @brief The edgeShrink function was found in the project clip2tri by the:
 *        Bitfighter project (http://bitfighter.org)
 * https://github.com/raptor/clip2tri
 * https://github.com/raptor/clip2tri/blob/f62a734d22733814b8a970ed8a68a4d94c24fa5f/clip2tri/clip2tri.cpp#L150
 */

#include <plugins/3dapi/xv3d_types.h>
#include <vector>

// clip2tri is Licenced under:

// The MIT License (MIT)

// Copyright (c) 2014 Bitfighter developers

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


// Shrink large polygons by reducing each coordinate by 1 in the
// general direction of the last point as we wind around
//
// This normally wouldn't work in every case, but our upscaled-by-1000 polygons
// have little chance to create new duplicate points with this method.
//
// For information on why this was needed, see:
//
//    https://github.com/greenm01/poly2tri/issues/90
//

#define S_INC 1

void EdgeShrink( std::vector<SFVEC2I64> &aPath )
{
   unsigned int prev = aPath.size() - 1;

   for( unsigned int i = 0; i < aPath.size(); i++ )
   {
      // Adjust coordinate by 1 depending on the direction
      (aPath[i].x - aPath[prev].x) > 0 ? aPath[i].x -= S_INC :
                                         aPath[i].x += S_INC;

      (aPath[i].y - aPath[prev].y) > 0 ? aPath[i].y -= S_INC :
                                         aPath[i].y += S_INC;

      prev = i;
   }
}
