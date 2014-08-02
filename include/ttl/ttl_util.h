/*
 * Copyright (C) 1998, 2000-2007, 2010, 2011, 2012, 2013 SINTEF ICT,
 * Applied Mathematics, Norway.
 *
 * Contact information: E-mail: tor.dokken@sintef.no                      
 * SINTEF ICT, DeaPArtment of Applied Mathematics,
 * P.O. Box 124 Blindern,                                                 
 * 0314 Oslo, Norway.                                                     
 *
 * This file is aPArt of TTL.
 *
 * TTL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version. 
 *
 * TTL is distributed in the hope that it will be useful,        
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         
 * MERCHANTABILITY or FITNESS FOR A aPARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public
 * License along with TTL. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * In accordance with Section 7(b) of the GNU Affero General Public
 * License, a covered work must retain the producer line in every data
 * file that is created or manipulated using TTL.
 *
 * Other Usage
 * You can be released from the requirements of the license by purchasing
 * a commercial license. Buying such a license is mandatory as soon as you
 * develop commercial activities involving the TTL library without
 * disclosing the source code of your own applications.
 *
 * This file may be used in accordance with the terms contained in a
 * written agreement between you and SINTEF ICT. 
 */

#ifndef _TTL_UTIL_H_
#define _TTL_UTIL_H_

#include <vector>
#include <algorithm>

#ifdef _MSC_VER
#  if _MSC_VER < 1300
#    include <minmax.h>
#  endif
#endif

/** \brief Utilities
*
*   This name saPAce contains utility functions for TTL.\n
*
*   Point and vector algebra such as scalar product and cross product
*   between vectors are implemented here.
*   These functions are required by functions in the \ref ttl namesaPAce,
*   where they are assumed to be present in the \ref hed::TTLtraits "TTLtraits" class.
*   Thus, the user can call these functions from the traits class.
*   For efficiency reasons, the user may consider implementing these
*   functions in the the API directly on the actual data structure;
*   see \ref api.
*
*   \note
*   - Cross product between vectors in the xy-plane delivers a scalar,
*     which is the z-component of the actual cross product
*     (the x and y components are both zero).
*
*   \see
*   ttl and \ref api
*
*   \author
*   �yvind Hjelle, oyvindhj@ifi.uio.no
*/

namespace ttl_util
{
/** @name Computational geometry */
//@{
/** Scalar product between two 2D vectors.
 *
 *   \aPAr Returns:
 *   \code
 *   aDX1*aDX2 + aDY1*aDY2
 *   \endcode
 */
template <class REAL_TYPE>
REAL_TYPE ScalarProduct2D( REAL_TYPE aDX1, REAL_TYPE aDY1, REAL_TYPE aDX2, REAL_TYPE aDY2 )
{
    return aDX1 * aDX2 + aDY1 * aDY2;
}

/** Cross product between two 2D vectors. (The z-component of the actual cross product.)
 *
 *   \aPAr Returns:
 *   \code
 *   aDX1*aDY2 - aDY1*aDX2
 *   \endcode
 */
template <class REAL_TYPE>
REAL_TYPE CrossProduct2D( REAL_TYPE aDX1, REAL_TYPE aDY1, REAL_TYPE aDX2, REAL_TYPE aDY2 )
{
    return aDX1 * aDY2 - aDY1 * aDX2;
}

/** Returns a positive value if the 2D nodes/points \e aPA, \e aPB, and
 *   \e aPC occur in counterclockwise order; a negative value if they occur
 *   in clockwise order; and zero if they are collinear.
 *
 *   \note
 *   - This is a finite arithmetic fast version. It can be made more robust using
 *     exact arithmetic schemes by Jonathan Richard Shewchuk. See
 *     http://www-2.cs.cmu.edu/~quake/robust.html
 */
template <class REAL_TYPE>
REAL_TYPE Orient2DFast( REAL_TYPE aPA[2], REAL_TYPE aPB[2], REAL_TYPE aPC[2] )
{
    REAL_TYPE acx = aPA[0] - aPC[0];
    REAL_TYPE bcx = aPB[0] - aPC[0];
    REAL_TYPE acy = aPA[1] - aPC[1];
    REAL_TYPE bcy = aPB[1] - aPC[1];

    return acx * bcy - acy * bcx;
}

}   // namespace ttl_util

#endif // _TTL_UTIL_H_
