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
 * @file cbbox.h
 * @brief Bounding Box class definition
 */

#ifndef _CBBOX_H_
#define _CBBOX_H_

#include "../ray.h"
#include <fctsys.h>     // For the DBG(

/**
 * Class CBBOX
 * manages a bounding box defined by two SFVEC3F min max points.
 */
struct CBBOX
{

public:

    /**
     * Constructor CBBOX
     * Create with default values a bounding box (not inizialized)
     */
    CBBOX();

    /**
     * Constructor CBBOX
     * Initialize a bounding box with a given point
     * @param aPbInit a point for the bounding box initialization
     */
    explicit CBBOX( const SFVEC3F &aPbInit );

    /**
     * Constructor CBBOX
     * Initialize a bounding box with a minimon and a maximun point
     * @param aPbMin the minimun point to initialize the bounding box
     * @param aPbMax the maximun point to initialize the bounding box
     */
    CBBOX( const SFVEC3F &aPbMin, const SFVEC3F &aPbMax );

    ~CBBOX();


    /**
     * Function Set
     * Set bounding box with new parameters
     * @param aPbMin the minimun point to initialize the bounding box
     * @param aPbMax the maximun point to initialize the bounding box
     */
    void Set( const SFVEC3F &aPbMin, const SFVEC3F &aPbMax );

    void Set( const CBBOX &aBBox );

    /**
     * @brief Set
     * @param aPbMin
     * @param aPbMax
     */
    void Set( const SFVEC3F &aPoint );

    /**
     * Function Union
     * recalculate the bounding box adding a point
     * @param aPoint the point to be bounded
     */
    void Union( const SFVEC3F &aPoint );

    /**
     * Function Union
     * recalculate the bounding box adding other bounding box
     * @param aBBox the bounding box to be bounded
     */
    void Union( const CBBOX &aBBox );

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
     * @param aBBox the bounding box to check if it intersects
     */
    bool Intersects( const CBBOX &aBBox ) const;

    /**
     * Function Inside
     * check is a point is inside this bounding box
     * @param aPoint point to test
     */
    bool Inside( const SFVEC3F &aPoint ) const;

    /**
     * Function ApplyTransformation
     * apply a transformation matrix to the box points
     * @param aTransformMatrix matrix to apply to the points of the bounding box
     */
    void ApplyTransformation( glm::mat4 aTransformMatrix );

    /**
     * Function ApplyTransformationAA
     * apply a transformation matrix to the box points and recalculate it
     * to fit an axis aligned bounding box
     * @param aTransformMatrix matrix to apply to the points of the bounding box
     */
    void ApplyTransformationAA( glm::mat4 aTransformMatrix );

    /**
     * Function Volume
     * calculate the volume of a bounding box
     * @return float - volume of this bounding box
     */
    float Volume() const;

    /**
     * Function debug
     * output to stdout
     */
    void debug() const;

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
     * @return SFVEC3F - the position of the center of this bounding box
     */
    SFVEC3F GetCenter() const;

    /**
     * Function GetCenter
     * return the center point of the bounding box for one Axis (0, 1 or 2)
     * @return float - the position of the center of this bounding box for the axis
     */
    float GetCenter( unsigned int aAxis ) const;

    /** Function Offset
     *
     * @return SFVEC3F - return the offset relative to max-min
     */
    SFVEC3F Offset( const SFVEC3F &p ) const;

    /**
     * Function GetExtent
     * @return SFVEC3F - max-min
     */
    const SFVEC3F GetExtent() const;

    /**
     * Function Min
     * return the minimun vertex pointer
     * @return SFVEC3F - the minimun vertice position
     */
    const SFVEC3F &Min() const { return m_min; }

    /**
     * Function Max
     * return the maximum vertex pointer
     * @return SFVEC3F - the maximun vertice position
     */
    const SFVEC3F &Max() const { return m_max; }


    /**
     * Function MaxDimension
     * @return the index of the max dimention (0=x, 1=y, 2=z)
     */
    unsigned int MaxDimension() const;

    /**
     * @brief GetMaxDimension
     * @return the max dimension
     */
    float GetMaxDimension() const;

    /**
     * Function SurfaceArea
     * @return the surface are of the box
     */
    float SurfaceArea() const;

    /**
     * Function Intersect
     * @param aRay = ray to intersect the box
     * @param t = distance point of the ray of the intersection (if true)
     * @return true if the ray hits the box
     */
    bool Intersect( const RAY &aRay, float *t ) const;

    bool Intersect( const RAY &aRay ) const;

    /**
     * Function Intersect - Useful for get the enter and exit position
     * If the ray starts inside the bbox, it will return aOutHitt0 = 0.0
     * @param aRay = ray to intersect the box
     * @param aOutHitt0 = distance point of the ray of the intersection (if true)
     * @param aOutHitt1 = distance point of the ray of the exit (if true)
     * @return true if the ray hits the box
     */
    bool Intersect( const RAY &aRay, float *aOutHitt0, float *aOutHitt1 ) const;

private:

    SFVEC3F m_min;  ///< (12) point of the lower position of the bounding box
    SFVEC3F m_max;  ///< (12) point of the higher position of the bounding box
};

#endif // CBBox_h
