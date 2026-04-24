/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#pragma once

#include <core/mirror.h>
#include <geometry/shape.h>
#include <geometry/ellipse.h>
#include <geometry/eda_angle.h>
#include <math/box2.h>
#include <math/vector2d.h>

class SHAPE_LINE_CHAIN;

class SHAPE_ELLIPSE : public SHAPE
{
public:
    SHAPE_ELLIPSE();

    SHAPE_ELLIPSE( const VECTOR2I& aCenter, int aMajorRadius, int aMinorRadius, const EDA_ANGLE& aRotation );

    SHAPE_ELLIPSE( const VECTOR2I& aCenter, int aMajorRadius, int aMinorRadius, const EDA_ANGLE& aRotation,
                   const EDA_ANGLE& aStartAngle, const EDA_ANGLE& aEndAngle );

    SHAPE_ELLIPSE( const VECTOR2I& aCenter, const VECTOR2I& aMajorEndpoint, double aRatio );

    SHAPE_ELLIPSE( const VECTOR2I& aCenter, const VECTOR2I& aMajorEndpoint, double aRatio, const EDA_ANGLE& aStartAngle,
                   const EDA_ANGLE& aEndAngle );

    SHAPE_ELLIPSE( const SHAPE_ELLIPSE& aOther ) = default;
    SHAPE_ELLIPSE& operator=( const SHAPE_ELLIPSE& ) = default;
    ~SHAPE_ELLIPSE() override = default;

    SHAPE* Clone() const override { return new SHAPE_ELLIPSE( *this ); }

    const VECTOR2I&  GetCenter() const { return m_ellipse.Center; }
    int              GetMajorRadius() const { return m_ellipse.MajorRadius; }
    int              GetMinorRadius() const { return m_ellipse.MinorRadius; }
    const EDA_ANGLE& GetRotation() const { return m_ellipse.Rotation; }
    const EDA_ANGLE& GetStartAngle() const { return m_ellipse.StartAngle; }
    const EDA_ANGLE& GetEndAngle() const { return m_ellipse.EndAngle; }

    bool IsArc() const { return m_isArc; }

    void SetCenter( const VECTOR2I& aCenter );
    void SetMajorRadius( int aRadius );
    void SetMinorRadius( int aRadius );
    void SetRotation( const EDA_ANGLE& aAngle );
    void SetStartAngle( const EDA_ANGLE& aAngle );
    void SetEndAngle( const EDA_ANGLE& aAngle );

    const BOX2I BBox( int aClearance = 0 ) const override;

    bool Collide( const SEG& aSeg, int aClearance = 0, int* aActual = nullptr,
                  VECTOR2I* aLocation = nullptr ) const override;

    bool Collide( const SHAPE* aShape, int aClearance = 0, int* aActual = nullptr,
                  VECTOR2I* aLocation = nullptr ) const override
    {
        return SHAPE::Collide( aShape, aClearance, aActual, aLocation );
    }

    void TransformToPolygon( SHAPE_POLY_SET& aBuffer, int aError, ERROR_LOC aErrorLoc ) const override;

    void Rotate( const EDA_ANGLE& aAngle, const VECTOR2I& aCenter = { 0, 0 } ) override;
    void Move( const VECTOR2I& aVector ) override;

    bool IsSolid() const override { return !m_isArc; }

    /**
     * Mirror the ellipse across a horizontal or vertical axis passing through aRef.
     */
    void Mirror( const VECTOR2I& aRef, FLIP_DIRECTION aFlipDirection );

    /**
     * Serialize the ellipse.
     */
    const std::string Format( bool aCplusPlus = true ) const override;

    double GetLength() const;

    /**
     * Build a polyline approximation of the ellipse or arc.
     * Stay within aMaxError IU of the true curve.
     *
     * @param aMaxError maximum error in IU
     */
    SHAPE_LINE_CHAIN ConvertToPolyline( int aMaxError ) const;

    bool PointInside( const VECTOR2I& aPt, int aAccuracy = 0, bool aUseBBoxCache = false ) const override;

    SEG::ecoord SquaredDistance( const VECTOR2I& aP, bool aOutlineOnly = false ) const override;

private:
    /**
     * If major < minor, swap them and add 90 degrees to rotation.
     * Clamps non positive radii to 1.
     */
    void normalize();

    /** Recompute cached sin/cos and inverse-radius-squared values. */
    void updateCache();

    /**
     * Return true if aAngleRad falls between StartAngle and EndAngle
     * (counter-clockwise sweep). Only valid for arcs.
     */
    bool isAngleInSweep( double aAngleRad ) const;

    /// Canonical CCW sweep in radians; aEnd >= aStart.  Used by all sweep-aware paths.
    void sweepRange( double& aStart, double& aEnd ) const;

    ELLIPSE<int> m_ellipse; ///< Wrapped geometric data (from geometry/ellipse.h)
    bool         m_isArc;   ///< true if open elliptical arc, false if closed ellipse

    double m_sinRot;      ///< sin(Rotation)
    double m_cosRot;      ///< cos(Rotation)
    double m_invMajorRSq; ///< 1 / MajorRadius ^ 2
    double m_invMinorRSq; ///< 1 / MinorRadius ^ 2
};
