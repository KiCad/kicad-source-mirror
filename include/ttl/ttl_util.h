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

#ifndef _TTL_UTIL_H_
#define _TTL_UTIL_H_


#include <vector>
#include <algorithm>


#ifdef _MSC_VER
#  if _MSC_VER < 1300
#    include <minmax.h>
#  endif
#endif


//using namespace std;


/** \brief Utilities
*   
*   This name space contains utility functions for TTL.\n
*
*   Point and vector algebra such as scalar product and cross product
*   between vectors are implemented here.
*   These functions are required by functions in the \ref ttl namespace,
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
*   Øyvind Hjelle, oyvindhj@ifi.uio.no
*/


namespace ttl_util {


  //------------------------------------------------------------------------------------------------
  // ------------------------------ Computational Geometry Group ----------------------------------
  //------------------------------------------------------------------------------------------------

  /** @name Computational geometry */
  //@{

  //------------------------------------------------------------------------------------------------
  /** Scalar product between two 2D vectors.
  *
  *   \par Returns:
  *   \code 
  *   dx1*dx2 + dy1*dy2
  *   \endcode
  */
  template <class real_type>
    real_type scalarProduct2d(real_type dx1, real_type dy1, real_type dx2, real_type dy2) {
    return dx1*dx2 + dy1*dy2;
  }


  //------------------------------------------------------------------------------------------------
  /** Cross product between two 2D vectors. (The z-component of the actual cross product.)
  *
  *   \par Returns:
  *   \code 
  *   dx1*dy2 - dy1*dx2
  *   \endcode
  */
  template <class real_type>
    real_type crossProduct2d(real_type dx1, real_type dy1, real_type dx2, real_type dy2) {
    return dx1*dy2 - dy1*dx2;
  }

 
  //------------------------------------------------------------------------------------------------
  /** Returns a positive value if the 2D nodes/points \e pa, \e pb, and
  *   \e pc occur in counterclockwise order; a negative value if they occur
  *   in clockwise order; and zero if they are collinear.
  *
  *   \note
  *   - This is a finite arithmetic fast version. It can be made more robust using
  *     exact arithmetic schemes by Jonathan Richard Shewchuk. See
  *     http://www-2.cs.cmu.edu/~quake/robust.html
  */
  template <class real_type>
    real_type orient2dfast(real_type pa[2], real_type pb[2], real_type pc[2]) {
    real_type acx = pa[0] - pc[0];
    real_type bcx = pb[0] - pc[0];
    real_type acy = pa[1] - pc[1];
    real_type bcy = pb[1] - pc[1];
    return acx * bcy - acy * bcx;
  }


  //------------------------------------------------------------------------------------------------
  /* Scalar product between 2D vectors represented as darts.
  *
  *   \par Requires:
  *   - real_type DartType::x()
  *   - real_type DartType::y()
  */  
  /*
  template <class TTLtraits, class DartType>
    typename TTLtraits::real_type scalarProduct2d(const DartType& d1, const DartType& d2) {
    
    DartType d10 = d1;
    d10.alpha0();
    
    DartType d20 = d2;
    d20.alpha0();
    
    return scalarProduct2d(d10.x() - d1.x(), d10.y() - d1.y(), d20.x() - d2.x(), d20.y() - d2.y());
  }
  */


  //------------------------------------------------------------------------------------------------
  /* Scalar product between 2D vectors.
  *   The first vector is represented by the given dart, and the second vector has
  *   direction from the node of the given dart - and to the given point.
  *
  *   \par Requires:
  *   - real_type DartType::x(), real_type DartType::y()
  *   - real_type PointType2d::x(),  real_type PointType2d::y()
  */
  /*
  template <class TTLtraits>
    typename TTLtraits::real_type scalarProduct2d(const typename TTLtraits::DartType& d,
                                                  const typename TTLtraits::PointType2d& p) {
    typename TTLtraits::DartType d0 = d;
    d0.alpha0();
    
    return scalarProduct2d(d0.x() - d.x(), d0.y() - d.y(), p.x() - d.x(), p.y() - d.y());
  }
  */


  //------------------------------------------------------------------------------------------------
  /* Cross product between 2D vectors represented as darts.
  *
  *   \par Requires:
  *   - real_type DartType::x(), real_type DartType::y()
  */
  /*
  template <class TTLtraits>
    typename TTLtraits::real_type crossProduct2d(const typename TTLtraits::DartType& d1,
                                                 const typename TTLtraits::DartType& d2) {
  
    TTLtraits::DartType d10 = d1;
    d10.alpha0();
    
    TTLtraits::DartType d20 = d2;
    d20.alpha0();
    
    return crossProduct2d(d10.x() - d1.x(), d10.y() - d1.y(), d20.x() - d2.x(), d20.y() - d2.y());
  }
  */


  //------------------------------------------------------------------------------------------------
  /* Cross product between 2D vectors.
  *   The first vector is represented by the given dart, and the second vector has
  *   direction from the node associated with given dart - and to the given point.
  * 
  *   \par Requires:
  *   - real_type DartType::x() 
  *   - real_type DartType::y()
  */
  /*
  template <class TTLtraits>
    typename TTLtraits::real_type crossProduct2d(const typename TTLtraits::DartType& d,
                                                 const typename TTLtraits::PointType2d& p) {
  
    TTLtraits::DartType d0 = d;
    d0.alpha0();
    
    return crossProduct2d(d0.x() - d.x(), d0.y() - d.y(), p.x() - d.x(), p.y() - d.y());
  }
  */
  // Geometric predicates; see more robust schemes by Jonathan Richard Shewchuk at
  // http://www.cs.cmu.edu/~quake/robust.html


  //------------------------------------------------------------------------------------------------
  /* Return a positive value if the 2d nodes/points \e d, \e d.alpha0(), and
  *   \e p occur in counterclockwise order; a negative value if they occur
  *   in clockwise order; and zero if they are collinear. The
  *   result is also a rough approximation of twice the signed
  *   area of the triangle defined by the three points.
  *
  *   \par Requires:
  *   - DartType::x(), DartType::y(),
  *   - PointType2d::x(), PointType2d::y()
  */
  /*
  template <class TTLtraits, class DartType, class PointType2d>
    typename TTLtraits::real_type orient2dfast(const DartType& n1, const DartType& n2,
                                               const PointType2d& p) {
    return ((n2.x()-n1.x())*(p.y()-n1.y()) - (p.x()-n1.x())*(n2.y()-n1.y()));
  }
  */

  //@} // End of Computational geometry


  //------------------------------------------------------------------------------------------------
  // ---------------------------- Utilities Involving Points Group --------------------------------
  //------------------------------------------------------------------------------------------------

  /** @name Utilities involving points */
  //@{

  //------------------------------------------------------------------------------------------------
  /** Creates random data on the unit square.
  *
  *   \param noPoints
  *   Number of random points to be generated
  *
  *   \param seed
  *   Initial value for pseudorandom number generator
  *
  *   \require
  *   - Constructor \c PointType::PointType(double x, double y).\n
  *     For example, one can use \c pair<double, double>.
  *
  *   \note 
  *   - To deduce template argument for PointType the function must be
  *    called with the syntax: \c createRandomData<MyPoint>(...) where \c MyPoint
  *    is the actual point type.
  */
  template <class PointType>
    std::vector<PointType*>* createRandomData(int noPoints, int seed=1) {
    
#ifdef _MSC_VER
    srand(seed);
#else
    srand48((long int)seed);
#endif
    
    double x, y;
    std::vector<PointType*>* points = new std::vector<PointType*>(noPoints);
    typename std::vector<PointType*>::iterator it;
    for (it = points->begin(); it != points->end(); ++it) {

#ifdef _MSC_VER
      int random = rand();
      x = ((double)random/(double)RAND_MAX);
      random = rand();
      y = ((double)random/(double)RAND_MAX);
      *it = new PointType(x,y);
#else
      double random = drand48();
      x = random;
      random = drand48();
      y = random;
      *it = new PointType(x,y);
#endif

    }
    return points;
  }

  //@} // End of Utilities involving points
  
}; // End of ttl_util namespace scope

#endif // _TTL_UTIL_H_
