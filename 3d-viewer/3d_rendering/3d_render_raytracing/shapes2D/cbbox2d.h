/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
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
 * @file cbbox2d.h
 * @brief 2D Bounding Box class definition
 */

#ifndef _CBBOX2D_H_
#define _CBBOX2D_H_

#include "../ray.h"


/**
 * Class CBBOX
 * manages a bounding box defined by two SFVEC2F min max points.
 */
struct CBBOX2D
{

public:

    /**
     * Constructor CBBOX2D
     * Create with default values a bounding box (not inizialized)
     */
    CBBOX2D();

    /**
     * Constructor CBBOX2D
     * Initialize a bounding box with a given point
     * @param aPbInit a point for the bounding box initialization
     */
    explicit CBBOX2D( const SFVEC2F &aPbInit );

    /**
     * Constructor CBBOX2D
     * Initialize a bounding box with a minimon and a maximun point
     * @param aPbMin the minimun point to initialize the bounding box
     * @param aPbMax the maximun point to initialize the bounding box
     */
    CBBOX2D( const SFVEC2F &aPbMin, const SFVEC2F &aPbMax );

    ~CBBOX2D();


    /**
     * Function Set
     * Set bounding box with new parameters
     * @param aPbMin the minimun point to initialize the bounding box
     * @param aPbMax the maximun point to initialize the bounding box
     */
    void Set( const SFVEC2F &aPbMin, const SFVEC2F &aPbMax );

    /**
     * Function Set
     * Set bounding box based on another bounding box
     * @param CBBOX2D a bounding box to initialize this one
     */
    void Set( const CBBOX2D &aBBox );

    /**
     * Function Union
     * recalculate the bounding box adding a point
     * @param aPoint the point to be bounded
     */
    void Union( const SFVEC2F &aPoint );

    /**
     * Function Union
     * recalculate the bounding box adding other bounding box
     * @param aBBox the bounding box to be bounded
     */
    void Union( const CBBOX2D &aBBox );

    /**
     * Function Scale
     * scales a bounding box by its center
     * @param aScale scale factor to apply
     */
    void Scale( float aScale );

    /**
     * Function ScaleNextUp
     * scales a bounding box to the next float representation making it larger
     */
    void ScaleNextUp();

    /**
     * Function ScaleNextDown
     * scales a bounding box to the next float representation making it smaller
     */
    void ScaleNextDown();

    /**
     * Function Intersects
     * test if a bounding box intersects this box
     * @param aBBox the bounding box to check if it inversects
     */
    bool Intersects( const CBBOX2D &aBBox ) const;

    /**
     * Function Intersects
     * test if a circle intersects this box
     * @param aBBox the bounding box to check if it intersects
     */
    bool Intersects( const SFVEC2F &aCenter, float aRadiusSquared ) const;

    /**
     * Function Inside
     * check is a point is inside this bounding box
     * @param aPoint point to test
     */
    bool Inside( const SFVEC2F &aPoint ) const;

    /**
     * Function Area
     * calculate the area of a bounding box
     * @return float - area of this bounding box
     */
    float Area() const;

    /**
     * Function IsInitialized
     * check if this bounding box is already initialized
     * @return bool - return true if it was initialized, false if otherwise
     */
    bool IsInitialized() const;

    /**
     * Function Reset
     * reset the bounding box to zero and de-initialized it
     */
    void Reset();

    /**
     * Function GetCenter
     * return the center point of the bounding box
     * @return SFVEC2F - the position of the center of this bounding box
     */
    SFVEC2F GetCenter() const;

    /**
     * Function GetExtent
     * @return SFVEC2F - max-min
     */
    SFVEC2F GetExtent() const;

    /**
     * Function Min
     * return the minimun vertex pointer
     * @return SFVEC2F - the minimun vertice position
     */
    const SFVEC2F &Min() const { return m_min; }

    /**
     * Function Max
     * return the maximum vertex pointer
     * @return SFVEC2F - the maximun vertice position
     */
    const SFVEC2F &Max() const { return m_max; }


    /**
     * Function MaxDimension
     * @return the index of the max dimention (0=x, 1=y)
     */
    unsigned int MaxDimension() const;

    /**
     * Function Perimeter
     * @return the surface are of the box
     */
    float Perimeter() const;

    /**
     * Function Intersect
     * @param aRay = ray to intersect the box
     * @param t = distance point of the ray of the intersection (if true)
     * @return true if the ray hits the box
     */
    bool Intersect( const RAY2D &aRay, float *t ) const;

    bool Intersect( const RAY2D &aRay, float *aOutHitT0, float *aOutHitT1 ) const;

    bool Intersect( const RAYSEG2D &aRaySeg ) const;

private:
    SFVEC2F m_min; ///< point of the lower position of the bounding box
    SFVEC2F m_max; ///< point of the higher position of the bounding box
};

#endif // CBBox2d_h
