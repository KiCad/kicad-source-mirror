/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Mario Luzeiro <mrluzeiro@gmail.com>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file CBBox.h
 * @brief Bounding Box class definition
 */

#ifndef CBBox_h
#define CBBox_h

#define GLM_FORCE_RADIANS
#include <gal/opengl/glm/glm.hpp>
#include <3d_types.h>


/**
 * Class CBBOX
 * manages a bounding box defined by two S3D_VERTEX points.
 */
class CBBOX
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
    CBBOX( const S3D_VERTEX &aPbInit );

    /**
     * Constructor CBBOX
     * Initialize a bounding box with a minimon and a maximun point
     * @param aPbMin the minimun point to initialize the bounding box
     * @param aPbMax the maximun point to initialize the bounding box
     */
    CBBOX( const S3D_VERTEX &aPbMin, const S3D_VERTEX &aPbMax );

    ~CBBOX();


    /**
     * Function Set
     * Set bounding box with new parameters
     * @param aPbMin the minimun point to initialize the bounding box
     * @param aPbMax the maximun point to initialize the bounding box
     */
    void Set( const S3D_VERTEX &aPbMin, const S3D_VERTEX &aPbMax );

    /**
     * Function Union
     * recalculate the bounding box adding a point
     * @param aPoint the point to be bounded
     */
    void Union( const S3D_VERTEX &aPoint );

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
     * Function OverlapsBox
     * test if a bounding box overlaps this box
     * @param aBBox the bounding box to check if it overlaps
     */
    bool OverlapsBox( const CBBOX &aBBox ) const;

    /**
     * Function Inside
     * check is a point is inside this bounding box
     * @param aPoint point to test
     */
    bool Inside( const S3D_VERTEX &aPoint ) const;

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
     * Function GLdebug
     * render a wired bounding box using openGL
     */
    void GLdebug() const;

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
     * @return S3D_VERTEX - the position of the center of this bounding box
     */
    S3D_VERTEX GetCenter() const;

    /**
     * Function Min
     * return the minimun vertex pointer
     * @return S3D_VERTEX - the minimun vertice position
     */
    S3D_VERTEX Min() const;

    /**
     * Function Max
     * return the maximum vertex pointer
     * @return S3D_VERTEX - the maximun vertice position
     */
    S3D_VERTEX Max() const;

private:
    S3D_VERTEX m_min;           ///< point of the lower position of the bounding box
    S3D_VERTEX m_max;           ///< point of the higher position of the bounding box
    bool m_initialized;         ///< initialization status of the bounding box. true - if initialized, false otherwise
};

#endif // CBBox_h
