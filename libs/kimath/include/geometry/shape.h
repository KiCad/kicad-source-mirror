/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef __SHAPE_H
#define __SHAPE_H

#include <sstream>
#include <vector>
#include <geometry/seg.h>
#include <math/vector2d.h>
#include <math/box2.h>

class SHAPE_LINE_CHAIN;

/**
 * Enum SHAPE_TYPE
 * Lists all supported shapes
 */

enum SHAPE_TYPE
{
    SH_RECT = 0,         ///> axis-aligned rectangle
    SH_SEGMENT,          ///> line segment
    SH_LINE_CHAIN,       ///> line chain (polyline)
    SH_CIRCLE,           ///> circle
    SH_SIMPLE,          ///> simple polygon
    SH_POLY_SET,         ///> set of polygons (with holes, etc.)
    SH_COMPOUND,         ///> compound shape, consisting of multiple simple shapes
    SH_ARC,              ///> circular arc
    SH_NULL,             ///> empty shape (no shape...),
    SH_POLY_SET_TRIANGLE ///> a single triangle belonging to a POLY_SET triangulation
};

static inline wxString SHAPE_TYPE_asString( SHAPE_TYPE a )
{
    switch( a )
    {
    case SH_RECT:       return "SH_RECT";
    case SH_SEGMENT:    return "SH_SEGMENT";
    case SH_LINE_CHAIN: return "SH_LINE_CHAIN";
    case SH_CIRCLE:     return "SH_CIRCLE";
    case SH_SIMPLE:     return "SH_SIMPLE";
    case SH_POLY_SET:   return "SH_POLY_SET";
    case SH_COMPOUND:   return "SH_COMPOUND";
    case SH_ARC:        return "SH_ARC";
    case SH_NULL:       return "SH_NULL";
    case SH_POLY_SET_TRIANGLE:       return "SH_POLY_SET_TRIANGLE";
    }

    return wxEmptyString;  // Just to quiet GCC.
}

class SHAPE;

class SHAPE_BASE
{
public:
    /**
     * Constructor
     *
     * Creates an empty shape of type aType
     */

    SHAPE_BASE( SHAPE_TYPE aType ) :
        m_type( aType )
    {}

    // Destructor
    virtual ~SHAPE_BASE()
    {}

    /**
     * Function Type()
     *
     * Returns the type of the shape.
     * @retval the type
     */
    SHAPE_TYPE Type() const
    {
        return m_type;
    }

    virtual bool HasIndexableSubshapes() const
    {
        return false;
    }

    virtual size_t GetIndexableSubshapeCount() const { return 0; }

    virtual void GetIndexableSubshapes( std::vector<SHAPE*>& aSubshapes ) { }

protected:
    ///> type of our shape
    SHAPE_TYPE m_type;
};

/**
 * SHAPE
 *
 * Represents an abstract shape on 2D plane.
 */
class SHAPE : public SHAPE_BASE
{
public:
    /**
     * @brief This is the minimum precision for all the points in a shape
     */
    static const int MIN_PRECISION_IU = 4;

    /**
     * Constructor
     *
     * Creates an empty shape of type aType
     */
    SHAPE( SHAPE_TYPE aType ) :
        SHAPE_BASE( aType )
    {}

    // Destructor
    virtual ~SHAPE()
    {}

    /**
     * Function Clone()
     *
     * Returns a dynamically allocated copy of the shape
     * @retval copy of the shape
     */
    virtual SHAPE* Clone() const
    {
        assert( false );
        return NULL;
    };

    /**
     * Function IsNull()
     *
     * Returns true if the shape is a null shape.
     * @retval true if null :-)
     */
    bool IsNull() const
    {
        return m_type == SH_NULL;
    }

    /**
     * Function Collide()
     *
     * Checks if the boundary of shape (this) lies closer to the point aP than aClearance,
     * indicating a collision.
     * @param aActual [out] an optional pointer to an int to store the actual distance in the
     *                event of a collision.
     * @param aLocation [out] an option pointer to a point to store a nearby location in the
     *                  event of a collision.
     * @return true, if there is a collision.
     */
    virtual bool Collide( const VECTOR2I& aP, int aClearance = 0, int* aActual = nullptr,
                          VECTOR2I* aLocation = nullptr ) const
    {
        return Collide( SEG( aP, aP ), aClearance, aActual, aLocation );
    }

    /**
     * Function Collide()
     *
     * Checks if the boundary of shape (this) lies closer to the shape aShape than aClearance,
     * indicating a collision.
     * @param aShape shape to check collision against
     * @param aClearance minimum clearance
     * @param aMTV minimum translation vector
     * @param aActual [out] an optional pointer to an int to store the actual distance in the
     *                event of a collision.
     * @param aLocation [out] an option pointer to a point to store a nearby location in the
     *                  event of a collision.
     * @return true, if there is a collision.
     */
    virtual bool Collide( const SHAPE* aShape, int aClearance, VECTOR2I* aMTV ) const;

    virtual bool Collide( const SHAPE* aShape, int aClearance = 0, int* aActual = nullptr,
                          VECTOR2I* aLocation = nullptr ) const;

    /**
     * Function Collide()
     *
     * Checks if the boundary of shape (this) lies closer to the segment aSeg than aClearance,
     * indicating a collision.
     * @param aActual [out] an optional pointer to an int to be updated with the actual distance
     *                int the event of a collision.
     * @param aLocation [out] an option pointer to a point to store a nearby location in the
     *                  event of a collision.
     * @return true, if there is a collision.
     */
    virtual bool Collide( const SEG& aSeg, int aClearance = 0, int* aActual = nullptr,
                          VECTOR2I* aLocation = nullptr ) const = 0;

    /**
     * Function BBox()
     *
     * Computes a bounding box of the shape, with a margin of aClearance
     * a collision.
     * @param aClearance how much the bounding box is expanded wrs to the minimum enclosing rectangle
     * for the shape.
     * @return the bounding box.
     */
    virtual const BOX2I BBox( int aClearance = 0 ) const = 0;

    /**
     * Function Centre()
     *
     * Computes a center-of-mass of the shape
     * @return the center-of-mass point
     */
    virtual VECTOR2I Centre() const
    {
        return BBox( 0 ).Centre(); // if nothing better is available....
    }

    /**
     * Function Rotate
     * @param aCenter is the rotation center
     * @param aAngle rotation angle in radians
     */
    virtual void Rotate( double aAngle, const VECTOR2I& aCenter = { 0, 0 } ) = 0;

    virtual void Move( const VECTOR2I& aVector ) = 0;

    virtual bool IsSolid() const = 0;

    virtual bool Parse( std::stringstream& aStream );

    virtual const std::string Format( ) const;

protected:
    typedef VECTOR2I::extended_type ecoord;
};


class SHAPE_LINE_CHAIN_BASE : public SHAPE
{
public:
    SHAPE_LINE_CHAIN_BASE( SHAPE_TYPE aType ) :
        SHAPE( aType )
    {
    }

    // Destructor
    virtual ~SHAPE_LINE_CHAIN_BASE()
    {
    }

    /**
     * Function Collide()
     *
     * Checks if point aP lies closer to us than aClearance.
     * @param aP the point to check for collisions with
     * @param aClearance minimum distance that does not qualify as a collision.
     * @param aActual an optional pointer to an int to store the actual distance in the event
     *                of a collision.
     * @return true, when a collision has been found
     */
    virtual bool Collide( const VECTOR2I& aP, int aClearance = 0, int* aActual = nullptr,
                          VECTOR2I* aLocation = nullptr ) const override;

    /**
     * Function Collide()
     *
     * Checks if segment aSeg lies closer to us than aClearance.
     * @param aSeg the segment to check for collisions with
     * @param aClearance minimum distance that does not qualify as a collision.
     * @param aActual an optional pointer to an int to store the actual distance in the event
     *                of a collision.
     * @return true, when a collision has been found
     */

    virtual bool Collide( const SEG& aSeg, int aClearance = 0, int* aActual = nullptr,
                          VECTOR2I* aLocation = nullptr ) const override;

    SEG::ecoord SquaredDistance( const VECTOR2I& aP, bool aOutlineOnly = false ) const;

    /**
     * Function PointInside()
     *
     * Checks if point aP lies inside a polygon (any type) defined by the line chain.
     * For closed shapes only.
     * @param aPt point to check
     * @param aUseBBoxCache gives better peformance if the bounding box caches have been
     *                      generated.
     * @return true if the point is inside the shape (edge is not treated as being inside).
     */
    bool PointInside( const VECTOR2I& aPt, int aAccuracy = 0, bool aUseBBoxCache = false ) const;

    /**
     * Function PointOnEdge()
     *
     * Checks if point aP lies on an edge or vertex of the line chain.
     * @param aP point to check
     * @return true if the point lies on the edge.
     */
    bool PointOnEdge( const VECTOR2I& aP, int aAccuracy = 0 ) const;

    /**
     * Function EdgeContainingPoint()
     *
     * Checks if point aP lies on an edge or vertex of the line chain.
     * @param aP point to check
     * @return index of the first edge containing the point, otherwise negative
     */
    int EdgeContainingPoint( const VECTOR2I& aP, int aAccuracy = 0 ) const;

    virtual const VECTOR2I GetPoint( int aIndex ) const   = 0;
    virtual const SEG      GetSegment( int aIndex ) const = 0;
    virtual size_t         GetPointCount() const          = 0;
    virtual size_t         GetSegmentCount() const        = 0;
    virtual bool IsClosed() const = 0;
};

#endif // __SHAPE_H
