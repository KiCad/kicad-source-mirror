/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

/************************************/
/* routines to handle bezier curves */
/************************************/

#include <bezier_curves.h>
#include <geometry/ellipse.h>
#include <trigo.h>
#include <math/vector2d.h>  // for VECTOR2D, operator*, VECTOR2
#include <wx/debug.h>       // for wxASSERT


BEZIER_POLY::BEZIER_POLY( const VECTOR2I& aStart, const VECTOR2I& aCtrl1,
                          const VECTOR2I& aCtrl2, const VECTOR2I& aEnd )
{
    m_ctrlPts.emplace_back( VECTOR2D( aStart ) );
    m_ctrlPts.emplace_back( VECTOR2D( aCtrl1 ) );
    m_ctrlPts.emplace_back( VECTOR2D( aCtrl2 ) );
    m_ctrlPts.emplace_back( VECTOR2D( aEnd ) );

    m_minSegLen = 0.0;
}


BEZIER_POLY::BEZIER_POLY( const std::vector<VECTOR2I>& aControlPoints )
{
    m_ctrlPts.reserve( aControlPoints.size() );

    for( const VECTOR2I& pt : aControlPoints )
        m_ctrlPts.emplace_back( pt );

    m_minSegLen = 0.0;
}


void BEZIER_POLY::GetPoly( std::vector<VECTOR2I>& aOutput, int aMinSegLen, int aMaxSegCount )
{
    aOutput.clear();
    std::vector<VECTOR2D> buffer;
    GetPoly( buffer, double( aMinSegLen ), aMaxSegCount );

    aOutput.reserve( buffer.size() );

    for( const VECTOR2D& pt : buffer )
        aOutput.emplace_back( VECTOR2I( (int) pt.x, (int) pt.y ) );
}


void BEZIER_POLY::GetPoly( std::vector<VECTOR2D>& aOutput, double aMinSegLen, int aMaxSegCount )
{
    wxASSERT( m_ctrlPts.size() == 4 );
    // FIXME Brute force method, use a better (recursive?) algorithm
    // with a max error value.
    // to optimize the number of segments
    double                  dt = 1.0 / aMaxSegCount;
    VECTOR2D::extended_type minSegLen_sq = aMinSegLen * aMinSegLen;

    aOutput.clear();
    aOutput.push_back( m_ctrlPts[0] );

    // If the Bezier curve is degenerated (straight line), skip intermediate points:
    bool degenerated = m_ctrlPts[0] == m_ctrlPts[1] && m_ctrlPts[2] == m_ctrlPts[3];

    if( !degenerated )
    {
        for( int ii = 1; ii < aMaxSegCount; ii++ )
        {
            double t = dt * ii;
            double omt  = 1.0 - t;
            double omt2 = omt * omt;
            double omt3 = omt * omt2;
            double t2   = t * t;
            double t3   = t * t2;

            VECTOR2D vertex = omt3 * m_ctrlPts[0]
                              + 3.0 * t * omt2 * m_ctrlPts[1]
                              + 3.0 * t2 * omt * m_ctrlPts[2]
                              + t3 * m_ctrlPts[3];

            // a minimal filter on the length of the segment being created:
            // The offset from last point:
            VECTOR2D                delta = vertex - aOutput.back();
            VECTOR2D::extended_type dist_sq = delta.SquaredEuclideanNorm();

            if( dist_sq > minSegLen_sq )
                aOutput.push_back( vertex );
        }
    }

    if( aOutput.back() != m_ctrlPts[3] )
        aOutput.push_back( m_ctrlPts[3] );
}


template<typename T>
void TransformEllipseToBeziers( const ELLIPSE<T>& aEllipse, std::vector<BEZIER<T>>& aBeziers )
{
    EDA_ANGLE arcAngle = -( aEllipse.EndAngle - aEllipse.StartAngle );

    if( arcAngle >= ANGLE_0 )
        arcAngle -= ANGLE_360;

    /*
     * KiCad does not natively support ellipses or elliptical arcs.  So, we convert them to Bezier
     * splines as these are the nearest thing we have that represents them in a way that is both
     * editable and preserves their curvature accurately (enough).
     *
     * Credit to Kliment for developing and documenting this method.
     */
    /// Minimum number of Beziers to use for a full circle to keep error manageable.
    const int minBeziersPerCircle = 4;

    /// The number of Beziers needed for the given arc
    const int numBeziers = static_cast<int>(
            std::ceil( std::abs( arcAngle.AsRadians() / ( 2 * M_PI / minBeziersPerCircle ) ) ) );

    /// Angle occupied by each Bezier
    const double angleIncrement = arcAngle.AsRadians() / numBeziers;

    /*
     * Now, let's assume a circle of radius 1, centered on origin, with angle startangle
     * x-axis-aligned. We'll move, scale, and rotate it later. We're creating Bezier curves that hug
     * this circle as closely as possible, with the angles that will be used on the final ellipse
     * too.
     *
     * Thanks to the beautiful and excellent https://pomax.github.io/bezierinfo we know how to
     * define a curve that hugs a circle as closely as possible.
     *
     * We need the value k, which is the optimal distance from the endpoint to the control point to
     * make the curve match the circle for a given circle arc angle.
     *
     * k = 4/3 * tan(θ/4), where θ is the angle of the arc. In our case, θ=angleIncrement
     */
    double theta = angleIncrement;
    double k     = ( 4. / 3. ) * std::tan( theta / 4 );

    /*
     * Define our Bezier:
     * - Start point is on the circle at the x-axis
     * - First control point just uses k as the y-value
     * - Second control point is offset from the end point
     * - End point is defined by the angle of the arc segment
     * Note that we use double here no matter what the template param is; round at the end only.
     */
    BEZIER<double> first = { { 1, 0 },
                             { 1, k },
                             { std::cos( theta ) + k * std::sin( theta ),
                               std::sin( theta ) - k * std::cos( theta ) },
                             { std::cos( theta ), std::sin( theta ) } };

    /*
     * Now construct the actual segments by transforming/rotating the first one
     */
    auto transformPoint =
            [&]( VECTOR2D aPoint, const double aAngle ) -> VECTOR2D
            {
                // Bring to the actual starting angle
                RotatePoint( aPoint,
                             -EDA_ANGLE( aAngle - aEllipse.StartAngle.AsRadians(), RADIANS_T ) );

                // Then scale to the major and minor radiuses of the ellipse
                aPoint *= VECTOR2D( aEllipse.MajorRadius, aEllipse.MinorRadius );

                // Now rotate to the ellipse coordinate system
                RotatePoint( aPoint, -aEllipse.Rotation );

                // And finally offset to the center location of the ellipse
                aPoint += aEllipse.Center;

                return aPoint;
            };

    for( int i = 0; i < numBeziers; i++ )
    {
        aBeziers.emplace_back( BEZIER<T>( {
                transformPoint( first.Start, i * angleIncrement ),
                transformPoint( first.C1,    i * angleIncrement ),
                transformPoint( first.C2,    i * angleIncrement ),
                transformPoint( first.End,   i * angleIncrement )
        } ) );
    }
}


template void TransformEllipseToBeziers( const ELLIPSE<double>& aEllipse,
                                         std::vector<BEZIER<double>>& aBeziers );
template void TransformEllipseToBeziers( const ELLIPSE<int>& aEllipse,
                                         std::vector<BEZIER<int>>& aBeziers );