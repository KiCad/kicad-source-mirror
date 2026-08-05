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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <core/mirror.h>
#include <geometry/shape.h>
#include <geometry/ellipse.h>
#include <geometry/eda_angle.h>
#include <math/box2.h>
#include <math/vector2d.h>

class SHAPE_LINE_CHAIN;
class SHAPE_ARC;
class CIRCLE;

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

    /// The cached trigonometry is derived from these fields, so it is not compared.
    bool operator==( const SHAPE_ELLIPSE& aOther ) const
    {
        return m_isArc == aOther.m_isArc && m_ellipse.Center == aOther.m_ellipse.Center
               && m_ellipse.MajorRadius == aOther.m_ellipse.MajorRadius
               && m_ellipse.MinorRadius == aOther.m_ellipse.MinorRadius
               && m_ellipse.Rotation == aOther.m_ellipse.Rotation && m_ellipse.StartAngle == aOther.m_ellipse.StartAngle
               && m_ellipse.EndAngle == aOther.m_ellipse.EndAngle;
    }

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

    /**
     * Find the point on the curve closest to aP.  For an arc the result is limited to
     * the drawn sweep.
     */
    VECTOR2I NearestPoint( const VECTOR2I& aP ) const;

    /**
     * Find the points where this curve crosses another one.
     *
     * Crossings outside either drawn sweep are dropped, so an elliptical arc never
     * reports a point beyond its own start and end angles.
     */
    std::vector<VECTOR2I> Intersect( const SHAPE_ELLIPSE& aOther ) const;
    std::vector<VECTOR2I> Intersect( const CIRCLE& aCircle ) const;
    std::vector<VECTOR2I> Intersect( const SHAPE_ARC& aArc ) const;

    /**
     * Find the points where this curve crosses aSeg.
     *
     * @param aTreatAsLine extends the segment to an infinite line.
     */
    std::vector<VECTOR2I> Intersect( const SEG& aSeg, bool aTreatAsLine = false ) const;

private:
    /// A conic curve Axx x^2 + Axy xy + Ayy y^2 + Bx x + By y + C = 0, written in this
    /// ellipse's local frame.  Every curve this one can meet takes this form.
    struct CONIC
    {
        double Axx;
        double Axy;
        double Ayy;
        double Bx;
        double By;
        double C;
    };

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

    VECTOR2D toLocal( const VECTOR2I& aP ) const;
    VECTOR2I toWorld( const VECTOR2D& aP ) const;

    /// Point on the full ellipse at parameter angle aTheta, in the local frame.
    VECTOR2D pointAtParam( double aTheta ) const;

    /// Point of the curve closest to aLocal, both in the local frame.
    VECTOR2D closestLocalPoint( const VECTOR2D& aLocal ) const;

    /// Write an ellipse with the given world placement as a conic in this local frame.
    CONIC conicOf( const VECTOR2I& aCenter, double aMajorR, double aMinorR, const EDA_ANGLE& aRotation ) const;

    /// Parameter angles of this curve where aConic is zero, already limited to its sweep.
    std::vector<double> conicRoots( const CONIC& aConic ) const;

    /// Points where this curve crosses a full circle, before any sweep of that circle applies.
    std::vector<VECTOR2I> intersectCircle( const VECTOR2I& aCenter, double aRadius ) const;

    ELLIPSE<int> m_ellipse; ///< Wrapped geometric data (from geometry/ellipse.h)
    bool         m_isArc;   ///< true if open elliptical arc, false if closed ellipse

    double m_sinRot;      ///< sin(Rotation)
    double m_cosRot;      ///< cos(Rotation)
    double m_invMajorRSq; ///< 1 / MajorRadius ^ 2
    double m_invMinorRSq; ///< 1 / MinorRadius ^ 2
};
