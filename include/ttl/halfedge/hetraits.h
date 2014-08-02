/*
 * Copyright (C) 1998, 2000-2007, 2010, 2011, 2012, 2013 SINTEF ICT,
 * Applied Mathematics, Norway.
 *
 * Contact information: E-mail: tor.dokken@sintef.no                      
 * SINTEF ICT, Department of Applied Mathematics,                         
 * P.O. Box 124 Blindern,                                                 
 * 0314 Oslo, Norway.                                                     
 *
 * This file is part of TTL.
 *
 * TTL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version. 
 *
 * TTL is distributed in the hope that it will be useful,        
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          
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

#ifndef _HALF_EDGE_TRAITS_
#define _HALF_EDGE_TRAITS_

#include <ttl/halfedge/hetriang.h>
#include <ttl/halfedge/hedart.h>

namespace hed
{
/**
 * \struct TTLtraits
 * \brief \b Traits class (static struct) for the half-edge data structure.
 *
 * The member functions are those required by different function templates
 * in the TTL. Documentation is given here to explain what actions
 * should be carried out on the actual data structure as required by the functions
 * in the \ref ttl namespace.
 *
 * The source code of \c %HeTraits.h shows how the traits class is implemented for the
 * half-edge data structure.
 *
 * \see \ref api
 */
struct TTLtraits
{
    /**
     * The floating point type used in calculations involving scalar products and cross products.
     */
    typedef double REAL_TYPE;
    
    /** @name Geometric Predicates */
    //@{
    /**
     * Scalar product between two 2D vectors represented as darts.\n
     *
     * ttl_util::scalarProduct2d can be used.
     */
    static REAL_TYPE ScalarProduct2D( const DART& aV1, const DART& aV2 )
    {
        DART v10 = aV1;
        v10.Alpha0();

        DART v20 = aV2;
        v20.Alpha0();

        return ttl_util::ScalarProduct2D( v10.X() - aV1.X(),  v10.Y() - aV1.Y(),
                                          v20.X() - aV2.X(),  v20.Y() - aV2.Y() );
    }

    /**
     * Scalar product between two 2D vectors.
     * The first vector is represented by a dart \e v, and the second
     * vector has direction from the source node of \e v to the point \e p.\n
     *
     * ttl_util::ScalarProduct2D can be used.
     */
    static REAL_TYPE ScalarProduct2D( const DART& aV, const NODE_PTR& aP )
    {
        DART d0 = aV;
        d0.Alpha0();

        return ttl_util::ScalarProduct2D( d0.X() - aV.X(),     d0.Y() - aV.Y(),
                                          aP->GetX() - aV.X(), aP->GetY() - aV.Y() );
    }

    /**
     * Cross product between two vectors in the plane represented as darts.
     * The z-component of the cross product is returned.\n
     *
     * ttl_util::CrossProduct2D can be used.
     */
    static REAL_TYPE CrossProduct2D( const DART& aV1, const DART& aV2 )
    {
        DART v10 = aV1;
        v10.Alpha0();

        DART v20 = aV2;
        v20.Alpha0();

        return ttl_util::CrossProduct2D( v10.X() - aV1.X(), v10.Y() - aV1.Y(),
                                         v20.X() - aV2.X(), v20.Y() - aV2.Y() );
    }

    /**
     * Cross product between two vectors in the plane.
     * The first vector is represented by a dart \e v, and the second
     * vector has direction from the source node of \e v to the point \e p.
     * The z-component of the cross product is returned.\n
     *
     * ttl_util::CrossProduct2d can be used.
     */
    static REAL_TYPE CrossProduct2D( const DART& aV, const NODE_PTR& aP )
    {
        DART d0 = aV;
        d0.Alpha0();

        return ttl_util::CrossProduct2D( d0.X() - aV.X(),     d0.Y() - aV.Y(),
                                         aP->GetX() - aV.X(), aP->GetY() - aV.Y() );
    }

    /**
     * Let \e n1 and \e n2 be the nodes associated with two darts, and let \e p
     * be a point in the plane. Return a positive value if \e n1, \e n2,
     * and \e p occur in counterclockwise order; a negative value if they occur
     * in clockwise order; and zero if they are collinear.
     */
    static REAL_TYPE Orient2D( const DART& aN1, const DART& aN2, const NODE_PTR& aP )
    {
        REAL_TYPE pa[2];
        REAL_TYPE pb[2];
        REAL_TYPE pc[2];

        pa[0] = aN1.X();
        pa[1] = aN1.Y();
        pb[0] = aN2.X();
        pb[1] = aN2.Y();
        pc[0] = aP->GetX();
        pc[1] = aP->GetY();

        return ttl_util::Orient2DFast( pa, pb, pc );
    }

    /**
     * This is the same predicate as represented with the function above,
     * but with a slighty different interface:
     * The last parameter is given as a dart where the source node of the dart
     * represents a point in the plane.
     * This function is required for constrained triangulation.
     */
    static REAL_TYPE Orient2D( const DART& aN1, const DART& aN2, const DART& aP )
    {
        REAL_TYPE pa[2];
        REAL_TYPE pb[2];
        REAL_TYPE pc[2];

        pa[0] = aN1.X();
        pa[1] = aN1.Y();
        pb[0] = aN2.X();
        pb[1] = aN2.Y();
        pc[0] = aP.X();
        pc[1] = aP.Y();

        return ttl_util::Orient2DFast( pa, pb, pc );
    }

    //@} // End of Geometric Predicates Group
};

}; // End of hed namespace

#endif
