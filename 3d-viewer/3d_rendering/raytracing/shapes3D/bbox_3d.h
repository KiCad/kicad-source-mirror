/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
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
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

 /**
 * @file bbox_3d.h
 * @brief Bounding Box class definition
 */

#ifndef _BBOX_3D_H_
#define _BBOX_3D_H_

#include <plugins/3dapi/xv3d_types.h>   // SFVEC2F
#include <glm/mat4x4.hpp>

struct RAY;


/**
 * Manage a bounding box defined by two SFVEC3F min max points.
 */
struct BBOX_3D
{
public:
    /**
     * Create with default values a bounding box (not initialized)
     */
    BBOX_3D();

    /**
     * Initialize a bounding box with a given point.
     *
     * @param aPbInit a point for the bounding box initialization.
     */
    explicit BBOX_3D( const SFVEC3F& aPbInit );

    /**
     * Initialize a bounding box with a minimum and a maximum point.
     *
     * @param aPbMin the minimum point to initialize the bounding box.
     * @param aPbMax the maximum point to initialize the bounding box.
     */
    BBOX_3D( const SFVEC3F& aPbMin, const SFVEC3F& aPbMax );

    ~BBOX_3D();


    /**
     * Set bounding box with new parameters.
     *
     * @param aPbMin the minimum point to set for the bounding box.
     * @param aPbMax the maximum point to set for the bounding box.
     */
    void Set( const SFVEC3F& aPbMin, const SFVEC3F& aPbMax );

    void Set( const BBOX_3D& aBBox );

    /**
     * @brief Set bounding box to one point.
     * @param aPoint the single point to set the bounding box to.
     */
    void Set( const SFVEC3F& aPoint );

    /**
     * Recalculate the bounding box adding a point.
     *
     * @param aPoint the point to be bounded.
     */
    void Union( const SFVEC3F& aPoint );

    /**
     * Recalculate the bounding box adding other bounding box.
     *
     * @param aBBox the bounding box to be bounded.
     */
    void Union( const BBOX_3D& aBBox );

    /**
     * Scales a bounding box by its center.
     *
     * @param aScale scale factor to apply.
     */
    void Scale( float aScale );

    /**
     * Scale a bounding box to the next float representation making it larger.
     */
    void ScaleNextUp();

    /**
     * Scale a bounding box to the next float representation making it smaller.
     */
    void ScaleNextDown();

    /**
     * Test if a bounding box intersects this box.
     *
     * @param aBBox the bounding box to check if it intersects.
     */
    bool Intersects( const BBOX_3D& aBBox ) const;

    /**
     * Check if a point is inside this bounding box.
     *
     * @param aPoint point to test.
     */
    bool Inside( const SFVEC3F& aPoint ) const;

    /**
     * Check if a bounding box is inside this bounding box.
     *
     * @param aBBox the bounding box to test if it is inside
     * @return true if aBBox is smaller and all points are inside
     */
    bool Inside( const BBOX_3D& aBBox ) const;

    /**
     * Apply a transformation matrix to the box points.
     *
     * @param aTransformMatrix matrix to apply to the points of the bounding box
     */
    void ApplyTransformation( glm::mat4 aTransformMatrix );

    /**
     * Calculate the volume of a bounding box.
     *
     * @return float - volume of this bounding box.
     */
    float Volume() const;

    /**
     * Check if this bounding box is already initialized.
     *
     * @return bool - return true if it was initialized, false if otherwise.
     */
    bool IsInitialized() const;

    /**
     * Reset the bounding box to zero and de-initialize it.
     */
    void Reset();

    /**
     * Return the center point of the bounding box.
     *
     * @return SFVEC3F - the position of the center of this bounding box.
     */
    SFVEC3F GetCenter() const;

    /**
     * Return the center point of the bounding box for one axis (0, 1 or 2).
     *
     * @return float - the position of the center of this bounding box for the axis
     */
    float GetCenter( unsigned int aAxis ) const;

    /**
     * @return SFVEC3F - return the offset relative to max-min.
     */
    SFVEC3F Offset( const SFVEC3F& p ) const;

    /**
     * @return SFVEC3F - max-min.
     */
    const SFVEC3F GetExtent() const;

    /**
     * Return the minimum vertex pointer.
     *
     * @return SFVEC3F - the minimum vertex position.
     */
    const SFVEC3F& Min() const { return m_min; }

    /**
     * Return the maximum vertex pointer.
     *
     * @return SFVEC3F - the maximum vertex position.
     */
    const SFVEC3F& Max() const { return m_max; }


    /**
     * @return the index of the max dimension (0=x, 1=y, 2=z).
     */
    unsigned int MaxDimension() const;

    /**
     * @return the max dimension.
     */
    float GetMaxDimension() const;

    /**
     * @return the surface area of the box.
     */
    float SurfaceArea() const;

    /**
     * @param aRay The ray to intersect the box.
     * @param t The distance point of the ray of the intersection (if true).
     * @return true if the ray hits the box.
     */
    bool Intersect( const RAY& aRay, float* t ) const;

    bool Intersect( const RAY& aRay ) const;

    /**
     * Fetch the enter and exit position when a ray starts inside the bounding box.
     *
     * @param aRay The ray to intersect the box.
     * @param aOutHitt0 The distance point of the ray of the intersection (if true).
     * @param aOutHitt1 The distance point of the ray of the exit (if true).
     * @return true if the ray hits the box
     */
    bool Intersect( const RAY& aRay, float* aOutHitt0, float* aOutHitt1 ) const;

private:
    SFVEC3F m_min;  ///< (12) point of the lower position of the bounding box
    SFVEC3F m_max;  ///< (12) point of the higher position of the bounding box
};

#endif // _BBOX_3D_H_
