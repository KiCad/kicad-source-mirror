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

#include <geometry/shape_ellipse.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_poly_set.h>
#include <geometry/shape_arc.h>
#include <geometry/circle.h>

#include <algorithm>
#include <cmath>
#include <sstream>

#include <trigo.h>


namespace
{

/**
 * Adaptive Simpson's method
 * See https://en.wikipedia.org/wiki/Adaptive_Simpson%27s_method
 *
 * @param f        integrand
 * @param a,b      integration bounds
 * @param tol      absolute tolerance
 * @param maxDepth recursion depth cap (20 is plenty for smooth ellipse arcs)
 */
template <typename F>
double adaptiveSimpson( F f, double a, double b, double tol, int maxDepth );

template <typename F>
double adaptiveSimpsonRec( F f, double a, double b, double tol, double whole, double fa, double fb, double fm,
                           int depth )
{
    const double m = 0.5 * ( a + b );
    const double lm = 0.5 * ( a + m );
    const double rm = 0.5 * ( m + b );
    const double flm = f( lm );
    const double frm = f( rm );

    const double left = ( m - a ) * ( fa + 4.0 * flm + fm ) / 6.0;
    const double right = ( b - m ) * ( fm + 4.0 * frm + fb ) / 6.0;
    const double diff = left + right - whole;

    if( depth <= 0 || std::abs( diff ) < 15.0 * tol )
        return left + right + diff / 15.0;

    return adaptiveSimpsonRec( f, a, m, 0.5 * tol, left, fa, fm, flm, depth - 1 )
           + adaptiveSimpsonRec( f, m, b, 0.5 * tol, right, fm, fb, frm, depth - 1 );
}

template <typename F>
double adaptiveSimpson( F f, double a, double b, double tol, int maxDepth )
{
    const double fa = f( a );
    const double fb = f( b );
    const double fm = f( 0.5 * ( a + b ) );
    const double whole = ( b - a ) * ( fa + 4.0 * fm + fb ) / 6.0;
    return adaptiveSimpsonRec( f, a, b, tol, whole, fa, fb, fm, maxDepth );
}


/**
 * Split the curve into line segments.
 * If the straight line between two points is too far from the actual ellipse curve
 * at the midpoint, split in half and try again.
 * Stop when the straight line is close enough.
 */
template <typename Eval>
void subdivideEllipseArc( double t0, const VECTOR2I& p0, double t1, const VECTOR2I& p1, double aMaxErrSq, int aDepth,
                          Eval aEval, SHAPE_LINE_CHAIN& aOut )
{
    if( aDepth <= 0 )
    {
        aOut.Append( p1 );
        return;
    }

    const double   tm = 0.5 * ( t0 + t1 );
    const VECTOR2I pm = aEval( tm );

    const double mx = 0.5 * ( static_cast<double>( p0.x ) + p1.x );
    const double my = 0.5 * ( static_cast<double>( p0.y ) + p1.y );
    const double ex = pm.x - mx;
    const double ey = pm.y - my;

    if( ex * ex + ey * ey <= aMaxErrSq )
    {
        aOut.Append( p1 );
        return;
    }

    subdivideEllipseArc( t0, p0, tm, pm, aMaxErrSq, aDepth - 1, aEval, aOut );
    subdivideEllipseArc( tm, pm, t1, p1, aMaxErrSq, aDepth - 1, aEval, aOut );
}


constexpr double ROOT_EPSILON = 1e-12;


/**
 * Real roots of aA x^2 + aB x + aC, degenerating to the linear case when aA vanishes.
 * The second root is formed by division rather than by subtraction, which keeps its
 * precision when the discriminant nearly cancels the linear term.
 */
std::vector<double> quadraticRoots( double aA, double aB, double aC )
{
    std::vector<double> roots;

    if( std::abs( aA ) < ROOT_EPSILON )
    {
        if( std::abs( aB ) >= ROOT_EPSILON )
            roots.push_back( -aC / aB );

        return roots;
    }

    const double disc = aB * aB - 4.0 * aA * aC;

    if( disc < 0.0 )
        return roots;

    const double sq = std::sqrt( disc );
    const double q = -0.5 * ( aB + ( aB >= 0.0 ? sq : -sq ) );

    roots.push_back( q / aA );

    if( std::abs( q ) >= ROOT_EPSILON )
        roots.push_back( aC / q );

    return roots;
}


/**
 * Real roots of aA x^3 + aB x^2 + aC x + aD.  The three root case uses the
 * trigonometric form, which avoids taking cube roots of complex numbers.
 */
std::vector<double> cubicRoots( double aA, double aB, double aC, double aD )
{
    if( std::abs( aA ) < ROOT_EPSILON )
        return quadraticRoots( aB, aC, aD );

    const double b = aB / aA;
    const double c = aC / aA;
    const double d = aD / aA;

    // Depress to y^3 + p y + q, where x is y minus the shift
    const double shift = b / 3.0;
    const double p = c - b * b / 3.0;
    const double q = 2.0 * b * b * b / 27.0 - b * c / 3.0 + d;

    std::vector<double> roots;
    const double        disc = q * q / 4.0 + p * p * p / 27.0;

    if( std::abs( p ) < ROOT_EPSILON && std::abs( q ) < ROOT_EPSILON )
    {
        roots.push_back( -shift );
    }
    else if( disc > 0.0 )
    {
        const double sq = std::sqrt( disc );
        roots.push_back( std::cbrt( -q / 2.0 + sq ) + std::cbrt( -q / 2.0 - sq ) - shift );
    }
    else
    {
        const double r = 2.0 * std::sqrt( -p / 3.0 );
        const double arg = std::clamp( 3.0 * q / ( p * r ), -1.0, 1.0 );
        const double phi = std::acos( arg ) / 3.0;

        for( int k = 0; k < 3; ++k )
            roots.push_back( r * std::cos( phi - 2.0 * M_PI * k / 3.0 ) - shift );
    }

    return roots;
}


/**
 * Real roots of aA x^4 + aB x^3 + aC x^2 + aD x + aE by Ferrari's method, which splits
 * the depressed quartic into two quadratics using a root of its resolvent cubic.
 */
std::vector<double> quarticRoots( double aA, double aB, double aC, double aD, double aE )
{
    if( std::abs( aA ) < ROOT_EPSILON )
        return cubicRoots( aB, aC, aD, aE );

    const double b = aB / aA;
    const double c = aC / aA;
    const double d = aD / aA;
    const double e = aE / aA;

    // Depress to y^4 + p y^2 + q y + r, where x is y minus the shift
    const double shift = b / 4.0;
    const double p = c - 3.0 * b * b / 8.0;
    const double q = d - b * c / 2.0 + b * b * b / 8.0;
    const double r = e - b * d / 4.0 + b * b * c / 16.0 - 3.0 * b * b * b * b / 256.0;

    std::vector<double> roots;

    if( std::abs( q ) < ROOT_EPSILON )
    {
        for( double ySq : quadraticRoots( 1.0, p, r ) )
        {
            if( ySq >= 0.0 )
            {
                const double y = std::sqrt( ySq );
                roots.push_back( y - shift );
                roots.push_back( -y - shift );
            }
        }

        return roots;
    }

    double alphaSq = 0.0;

    for( double z : cubicRoots( 1.0, 2.0 * p, p * p - 4.0 * r, -q * q ) )
    {
        if( z > alphaSq )
            alphaSq = z;
    }

    if( alphaSq <= 0.0 )
        return roots;

    const double alpha = std::sqrt( alphaSq );
    const double beta = ( p + alphaSq - q / alpha ) / 2.0;
    const double gamma = ( p + alphaSq + q / alpha ) / 2.0;

    for( double y : quadraticRoots( 1.0, alpha, beta ) )
        roots.push_back( y - shift );

    for( double y : quadraticRoots( 1.0, -alpha, gamma ) )
        roots.push_back( y - shift );

    return roots;
}


void dedupePoints( std::vector<VECTOR2I>& aPoints )
{
    std::sort( aPoints.begin(), aPoints.end(),
               []( const VECTOR2I& aLeft, const VECTOR2I& aRight )
               {
                   return aLeft.x != aRight.x ? aLeft.x < aRight.x : aLeft.y < aRight.y;
               } );

    aPoints.erase( std::unique( aPoints.begin(), aPoints.end() ), aPoints.end() );
}

} // namespace


SHAPE_ELLIPSE::SHAPE_ELLIPSE() :
        SHAPE( SH_ELLIPSE ),
        m_ellipse(),
        m_isArc( false ),
        m_sinRot( 0.0 ),
        m_cosRot( 1.0 ),
        m_invMajorRSq( 0.0 ),
        m_invMinorRSq( 0.0 )
{
}


SHAPE_ELLIPSE::SHAPE_ELLIPSE( const VECTOR2I& aCenter, int aMajorRadius, int aMinorRadius,
                              const EDA_ANGLE& aRotation ) :
        SHAPE( SH_ELLIPSE ),
        m_ellipse( aCenter, aMajorRadius, aMinorRadius, aRotation ),
        m_isArc( false )
{
    normalize();
}


SHAPE_ELLIPSE::SHAPE_ELLIPSE( const VECTOR2I& aCenter, int aMajorRadius, int aMinorRadius, const EDA_ANGLE& aRotation,
                              const EDA_ANGLE& aStartAngle, const EDA_ANGLE& aEndAngle ) :
        SHAPE( SH_ELLIPSE ),
        m_ellipse( aCenter, aMajorRadius, aMinorRadius, aRotation, aStartAngle, aEndAngle ),
        m_isArc( true )
{
    normalize();
}


SHAPE_ELLIPSE::SHAPE_ELLIPSE( const VECTOR2I& aCenter, const VECTOR2I& aMajorEndpoint, double aRatio ) :
        SHAPE( SH_ELLIPSE ),
        m_ellipse( aCenter, aMajorEndpoint, aRatio ),
        m_isArc( false )
{
    normalize();
}


SHAPE_ELLIPSE::SHAPE_ELLIPSE( const VECTOR2I& aCenter, const VECTOR2I& aMajorEndpoint, double aRatio,
                              const EDA_ANGLE& aStartAngle, const EDA_ANGLE& aEndAngle ) :
        SHAPE( SH_ELLIPSE ),
        m_ellipse( aCenter, aMajorEndpoint, aRatio, aStartAngle, aEndAngle ),
        m_isArc( true )
{
    normalize();
}


void SHAPE_ELLIPSE::normalize()
{
    m_ellipse.MajorRadius = std::max( 1, m_ellipse.MajorRadius );
    m_ellipse.MinorRadius = std::max( 1, m_ellipse.MinorRadius );

    if( m_ellipse.MajorRadius < m_ellipse.MinorRadius )
    {
        std::swap( m_ellipse.MajorRadius, m_ellipse.MinorRadius );
        m_ellipse.Rotation += ANGLE_90;

        if( m_isArc )
        {
            m_ellipse.StartAngle -= ANGLE_90;
            m_ellipse.EndAngle -= ANGLE_90;
        }
    }

    updateCache();
}


void SHAPE_ELLIPSE::SetCenter( const VECTOR2I& aCenter )
{
    m_ellipse.Center = aCenter;
}


void SHAPE_ELLIPSE::SetMajorRadius( int aRadius )
{
    m_ellipse.MajorRadius = aRadius;
    normalize();
}


void SHAPE_ELLIPSE::SetMinorRadius( int aRadius )
{
    m_ellipse.MinorRadius = aRadius;
    normalize();
}


void SHAPE_ELLIPSE::SetRotation( const EDA_ANGLE& aAngle )
{
    m_ellipse.Rotation = aAngle;
    updateCache();
}


void SHAPE_ELLIPSE::SetStartAngle( const EDA_ANGLE& aAngle )
{
    m_ellipse.StartAngle = aAngle;
}


void SHAPE_ELLIPSE::SetEndAngle( const EDA_ANGLE& aAngle )
{
    m_ellipse.EndAngle = aAngle;
}


const BOX2I SHAPE_ELLIPSE::BBox( int aClearance ) const
{
    const double a = static_cast<double>( m_ellipse.MajorRadius );
    const double b = static_cast<double>( m_ellipse.MinorRadius );

    if( !m_isArc )
    {
        const double cos2 = m_cosRot * m_cosRot;
        const double sin2 = m_sinRot * m_sinRot;

        const double dx = std::sqrt( a * a * cos2 + b * b * sin2 );
        const double dy = std::sqrt( a * a * sin2 + b * b * cos2 );

        const int idx = static_cast<int>( std::ceil( dx ) ) + aClearance;
        const int idy = static_cast<int>( std::ceil( dy ) ) + aClearance;

        return BOX2I( VECTOR2I( m_ellipse.Center.x - idx, m_ellipse.Center.y - idy ), VECTOR2I( 2 * idx, 2 * idy ) );
    }

    auto eval = [&]( double theta ) -> VECTOR2D
    {
        const double ct = std::cos( theta );
        const double st = std::sin( theta );
        return VECTOR2D( a * ct * m_cosRot - b * st * m_sinRot, a * ct * m_sinRot + b * st * m_cosRot );
    };

    const VECTOR2D p0 = eval( m_ellipse.StartAngle.AsRadians() );
    const VECTOR2D p1 = eval( m_ellipse.EndAngle.AsRadians() );

    double minX = std::min( p0.x, p1.x );
    double maxX = std::max( p0.x, p1.x );
    double minY = std::min( p0.y, p1.y );
    double maxY = std::max( p0.y, p1.y );

    const double thetaX = std::atan2( -b * m_sinRot, a * m_cosRot );
    const double thetaY = std::atan2( b * m_cosRot, a * m_sinRot );

    const double candidates[4] = { thetaX, thetaX + M_PI, thetaY, thetaY + M_PI };

    for( double c : candidates )
    {
        if( !isAngleInSweep( c ) )
            continue;

        const VECTOR2D p = eval( c );
        minX = std::min( minX, p.x );
        maxX = std::max( maxX, p.x );
        minY = std::min( minY, p.y );
        maxY = std::max( maxY, p.y );
    }

    const int iMinX = static_cast<int>( std::floor( minX ) ) - aClearance;
    const int iMaxX = static_cast<int>( std::ceil( maxX ) ) + aClearance;
    const int iMinY = static_cast<int>( std::floor( minY ) ) - aClearance;
    const int iMaxY = static_cast<int>( std::ceil( maxY ) ) + aClearance;

    return BOX2I( VECTOR2I( m_ellipse.Center.x + iMinX, m_ellipse.Center.y + iMinY ),
                  VECTOR2I( iMaxX - iMinX, iMaxY - iMinY ) );
}


double SHAPE_ELLIPSE::GetLength() const
{
    const double a = static_cast<double>( m_ellipse.MajorRadius );
    const double b = static_cast<double>( m_ellipse.MinorRadius );

    if( !m_isArc )
    {
        // Ramanujan's second approximation
        // See https://en.wikipedia.org/wiki/Perimeter_of_an_ellipse
        const double h = ( a - b ) / ( a + b );
        const double h2 = h * h;
        return M_PI * ( a + b ) * ( 1.0 + 3.0 * h2 / ( 10.0 + std::sqrt( 4.0 - 3.0 * h2 ) ) );
    }

    auto integrand = [a, b]( double theta ) -> double
    {
        const double s = std::sin( theta );
        const double c = std::cos( theta );
        return std::sqrt( a * a * s * s + b * b * c * c );
    };

    double t0, t1;
    sweepRange( t0, t1 );

    return adaptiveSimpson( integrand, t0, t1, 1e-9, 20 );
}


bool SHAPE_ELLIPSE::Collide( const SEG& aSeg, int aClearance, int* aActual, VECTOR2I* aLocation ) const
{
    if( aSeg.A == aSeg.B )
    {
        const SEG::ecoord dSq = SquaredDistance( aSeg.A, false );
        const SEG::ecoord clearSq = static_cast<SEG::ecoord>( aClearance ) * static_cast<SEG::ecoord>( aClearance );

        if( dSq == 0 || dSq < clearSq )
        {
            if( aActual )
                *aActual = static_cast<int>( std::round( std::sqrt( static_cast<double>( dSq ) ) ) );
            if( aLocation )
                *aLocation = aSeg.A;
            return true;
        }

        return false;
    }

    const VECTOR2D Aloc = toLocal( aSeg.A );
    const VECTOR2D Bloc = toLocal( aSeg.B );
    const VECTOR2D D( Bloc.x - Aloc.x, Bloc.y - Aloc.y );

    const double a = static_cast<double>( m_ellipse.MajorRadius );
    const double b = static_cast<double>( m_ellipse.MinorRadius );
    const double aSq = a * a;
    const double bSq = b * b;

    const double alpha = D.x * D.x / aSq + D.y * D.y / bSq;
    const double beta = 2.0 * ( Aloc.x * D.x / aSq + Aloc.y * D.y / bSq );
    const double gamma = Aloc.x * Aloc.x / aSq + Aloc.y * Aloc.y / bSq - 1.0;

    // A is inside the closed ellipse if gamma < 0, B is inside if alpha + beta + gamma < 0
    const double valB = alpha + beta + gamma;

    if( !m_isArc )
    {
        if( gamma <= 0.0 )
        {
            if( aActual )
                *aActual = 0;
            if( aLocation )
                *aLocation = aSeg.A;
            return true;
        }
        if( valB <= 0.0 )
        {
            if( aActual )
                *aActual = 0;
            if( aLocation )
                *aLocation = aSeg.B;
            return true;
        }
    }

    // disc < 0 means the line misses the ellipse
    const double disc = beta * beta - 4.0 * alpha * gamma;

    if( disc >= 0.0 && alpha > 0.0 )
    {
        const double sqrtDisc = std::sqrt( disc );
        const double twoAlpha = 2.0 * alpha;
        const double t0 = ( -beta - sqrtDisc ) / twoAlpha;
        const double t1 = ( -beta + sqrtDisc ) / twoAlpha;
        const double roots[2] = { t0, t1 };

        for( double t : roots )
        {
            if( t < 0.0 || t > 1.0 )
                continue;

            const VECTOR2D hit( Aloc.x + t * D.x, Aloc.y + t * D.y );

            // For arcs, the intersection must be within the angular sweep.
            if( m_isArc )
            {
                const double angle = std::atan2( hit.y / b, hit.x / a );
                if( !isAngleInSweep( angle ) )
                    continue;
            }

            if( aActual )
                *aActual = 0;
            if( aLocation )
                *aLocation = toWorld( hit );
            return true;
        }
    }

    double   minDistSq = std::numeric_limits<double>::max();
    VECTOR2D bestOnSegment( 0.0, 0.0 );

    {
        const double dA = static_cast<double>( SquaredDistance( aSeg.A, true ) );
        const double dB = static_cast<double>( SquaredDistance( aSeg.B, true ) );

        if( dA < minDistSq )
        {
            minDistSq = dA;
            bestOnSegment = Aloc;
        }
        if( dB < minDistSq )
        {
            minDistSq = dB;
            bestOnSegment = Bloc;
        }
    }

    const double dDotD = D.x * D.x + D.y * D.y;

    // Check where ellipse outline runs parallel to the segment
    if( dDotD > 0.0 )
    {
        const double theta0 = std::atan2( -b * D.x, a * D.y );
        const double thetas[2] = { theta0, theta0 + M_PI };

        for( double theta : thetas )
        {
            if( m_isArc && !isAngleInSweep( theta ) )
                continue;

            const double ex = a * std::cos( theta );
            const double ey = b * std::sin( theta );

            // Orthogonal projection of (ex, ey) onto the segment
            const double pDotD = ( ex - Aloc.x ) * D.x + ( ey - Aloc.y ) * D.y;
            const double t = std::clamp( pDotD / dDotD, 0.0, 1.0 );
            const double qx = Aloc.x + t * D.x;
            const double qy = Aloc.y + t * D.y;

            const double distSq = ( ex - qx ) * ( ex - qx ) + ( ey - qy ) * ( ey - qy );

            if( distSq < minDistSq )
            {
                minDistSq = distSq;
                bestOnSegment = VECTOR2D( qx, qy );
            }
        }
    }

    // Arc endpoints projected onto segment.
    if( m_isArc && dDotD > 0.0 )
    {
        const EDA_ANGLE endAngles[2] = { m_ellipse.StartAngle, m_ellipse.EndAngle };

        for( const EDA_ANGLE& endAngle : endAngles )
        {
            const double angleRad = endAngle.AsRadians();
            const double ex = a * std::cos( angleRad );
            const double ey = b * std::sin( angleRad );

            const double pDotD = ( ex - Aloc.x ) * D.x + ( ey - Aloc.y ) * D.y;
            const double t = std::clamp( pDotD / dDotD, 0.0, 1.0 );
            const double qx = Aloc.x + t * D.x;
            const double qy = Aloc.y + t * D.y;

            const double distSq = ( ex - qx ) * ( ex - qx ) + ( ey - qy ) * ( ey - qy );

            if( distSq < minDistSq )
            {
                minDistSq = distSq;
                bestOnSegment = VECTOR2D( qx, qy );
            }
        }
    }

    // Clearance comparison
    const double thresholdSq = static_cast<double>( aClearance ) * static_cast<double>( aClearance );

    if( minDistSq > 0.0 && minDistSq >= thresholdSq )
        return false;

    if( aActual )
        *aActual = static_cast<int>( std::round( std::sqrt( minDistSq ) ) );
    if( aLocation )
        *aLocation = toWorld( bestOnSegment );
    return true;
}


// ERROR_LOC is unused, tessellation points sit on the true ellipse curve, not offset inward or outward.
void SHAPE_ELLIPSE::TransformToPolygon( SHAPE_POLY_SET& aBuffer, int aError, ERROR_LOC /*aErrorLoc*/ ) const
{
    if( m_isArc )
        return;

    SHAPE_LINE_CHAIN chain = ConvertToPolyline( aError );
    chain.SetClosed( true );
    aBuffer.AddOutline( chain );
}


void SHAPE_ELLIPSE::Rotate( const EDA_ANGLE& aAngle, const VECTOR2I& aCenter )
{
    RotatePoint( m_ellipse.Center, aCenter, aAngle );
    m_ellipse.Rotation -= aAngle;
    updateCache();
}


void SHAPE_ELLIPSE::Mirror( const VECTOR2I& aRef, FLIP_DIRECTION aFlipDirection )
{
    m_ellipse.Mirror( aRef, aFlipDirection );
    updateCache();
}


const std::string SHAPE_ELLIPSE::Format( bool aCplusPlus ) const
{
    std::stringstream ss;

    if( aCplusPlus )
    {
        ss << "SHAPE_ELLIPSE( VECTOR2I( " << m_ellipse.Center.x << ", " << m_ellipse.Center.y << " ), "
           << m_ellipse.MajorRadius << ", " << m_ellipse.MinorRadius << ", EDA_ANGLE( "
           << m_ellipse.Rotation.AsDegrees() << ", DEGREES_T )";

        if( m_isArc )
        {
            ss << ", EDA_ANGLE( " << m_ellipse.StartAngle.AsDegrees() << ", DEGREES_T )"
               << ", EDA_ANGLE( " << m_ellipse.EndAngle.AsDegrees() << ", DEGREES_T )";
        }

        ss << " );";
    }
    else
    {
        ss << SHAPE::Format( aCplusPlus ) << " " << m_ellipse.Center.x << " " << m_ellipse.Center.y << " "
           << m_ellipse.MajorRadius << " " << m_ellipse.MinorRadius << " " << m_ellipse.Rotation.AsDegrees() << " "
           << ( m_isArc ? 1 : 0 );

        if( m_isArc )
        {
            ss << " " << m_ellipse.StartAngle.AsDegrees() << " " << m_ellipse.EndAngle.AsDegrees();
        }
    }

    return ss.str();
}


void SHAPE_ELLIPSE::Move( const VECTOR2I& aVector )
{
    m_ellipse.Center += aVector;
}


void SHAPE_ELLIPSE::updateCache()
{
    const double rotRad = m_ellipse.Rotation.AsRadians();
    m_sinRot = std::sin( rotRad );
    m_cosRot = std::cos( rotRad );

    const double a = static_cast<double>( m_ellipse.MajorRadius );
    const double b = static_cast<double>( m_ellipse.MinorRadius );
    m_invMajorRSq = 1.0 / ( a * a );
    m_invMinorRSq = 1.0 / ( b * b );
}


bool SHAPE_ELLIPSE::PointInside( const VECTOR2I& aPt, int aAccuracy, bool /*aUseBBoxCache*/ ) const
{
    // No interior for elliptical arcs. Open curve
    if( m_isArc )
        return false;

    const double dx = aPt.x - m_ellipse.Center.x;
    const double dy = aPt.y - m_ellipse.Center.y;
    const double lx = dx * m_cosRot + dy * m_sinRot;
    const double ly = -dx * m_sinRot + dy * m_cosRot;

    if( aAccuracy > 0 )
    {
        // Increase both radii by aAccuracy.
        const double a = static_cast<double>( m_ellipse.MajorRadius ) + aAccuracy;
        const double b = static_cast<double>( m_ellipse.MinorRadius ) + aAccuracy;
        return ( lx * lx ) / ( a * a ) + ( ly * ly ) / ( b * b ) < 1.0;
    }

    return lx * lx * m_invMajorRSq + ly * ly * m_invMinorRSq < 1.0;
}


VECTOR2D SHAPE_ELLIPSE::toLocal( const VECTOR2I& aP ) const
{
    const double dx = aP.x - m_ellipse.Center.x;
    const double dy = aP.y - m_ellipse.Center.y;

    return VECTOR2D( dx * m_cosRot + dy * m_sinRot, -dx * m_sinRot + dy * m_cosRot );
}


VECTOR2I SHAPE_ELLIPSE::toWorld( const VECTOR2D& aP ) const
{
    const double wx = aP.x * m_cosRot - aP.y * m_sinRot;
    const double wy = aP.x * m_sinRot + aP.y * m_cosRot;

    return VECTOR2I( static_cast<int>( std::round( m_ellipse.Center.x + wx ) ),
                     static_cast<int>( std::round( m_ellipse.Center.y + wy ) ) );
}


VECTOR2D SHAPE_ELLIPSE::pointAtParam( double aTheta ) const
{
    return VECTOR2D( m_ellipse.MajorRadius * std::cos( aTheta ), m_ellipse.MinorRadius * std::sin( aTheta ) );
}


VECTOR2D SHAPE_ELLIPSE::closestLocalPoint( const VECTOR2D& aLocal ) const
{
    const double lx = aLocal.x;
    const double ly = aLocal.y;

    const double a = static_cast<double>( m_ellipse.MajorRadius );
    const double b = static_cast<double>( m_ellipse.MinorRadius );

    // Closest point on ellipse via Eberly's bisection.
    // Reference: "Distance from a Point to an Ellipse, an Ellipsoid, or a
    // Hyperellipsoid", David Eberly, Geometric Tools.
    // https://www.geometrictools.com/Documentation/DistancePointEllipseEllipsoid.pdf

    const double y0 = std::abs( lx );
    const double y1 = std::abs( ly );

    double x0Local = 0.0;
    double x1Local = 0.0;

    if( y1 > 0.0 )
    {
        if( y0 > 0.0 )
        {
            const double z0 = y0 / a;
            const double z1 = y1 / b;
            const double g = z0 * z0 + z1 * z1 - 1.0;

            if( g != 0.0 )
            {
                const double r0 = ( a / b ) * ( a / b );
                const double n0 = r0 * z0;

                double s0 = z1 - 1.0;
                double s1 = ( g < 0.0 ) ? 0.0 : std::sqrt( n0 * n0 + z1 * z1 ) - 1.0;
                double s = 0.0;

                for( int iter = 0; iter < 64; ++iter )
                {
                    s = 0.5 * ( s0 + s1 );

                    if( s == s0 || s == s1 )
                        break;

                    const double ratio0 = n0 / ( s + r0 );
                    const double ratio1 = z1 / ( s + 1.0 );
                    const double gs = ratio0 * ratio0 + ratio1 * ratio1 - 1.0;

                    if( gs > 0.0 )
                        s0 = s;
                    else if( gs < 0.0 )
                        s1 = s;
                    else
                        break;
                }

                x0Local = r0 * y0 / ( s + r0 );
                x1Local = y1 / ( s + 1.0 );
            }
            else
            {
                // Point is on the ellipse.
                x0Local = y0;
                x1Local = y1;
            }
        }
        else
        {
            // y0 == 0 point lies on the minor axis.
            x0Local = 0.0;
            x1Local = b;
        }
    }
    else
    {
        // y1 == 0 point lies on the major axis.
        const double numer0 = a * y0;
        const double denom0 = a * a - b * b;

        if( numer0 < denom0 )
        {
            const double xde0 = numer0 / denom0;
            x0Local = a * xde0;
            x1Local = b * std::sqrt( std::max( 0.0, 1.0 - xde0 * xde0 ) );
        }
        else
        {
            x0Local = a;
            x1Local = 0.0;
        }
    }

    const VECTOR2D closest( ( lx < 0.0 ) ? -x0Local : x0Local, ( ly < 0.0 ) ? -x1Local : x1Local );

    // An arc that does not reach round to the closest point is nearest at one of its ends
    if( m_isArc )
    {
        const double closestTheta = std::atan2( closest.y / b, closest.x / a );

        if( !isAngleInSweep( closestTheta ) )
        {
            const VECTOR2D start = pointAtParam( m_ellipse.StartAngle.AsRadians() );
            const VECTOR2D end = pointAtParam( m_ellipse.EndAngle.AsRadians() );

            return ( aLocal - start ).SquaredEuclideanNorm() <= ( aLocal - end ).SquaredEuclideanNorm() ? start : end;
        }
    }

    return closest;
}


SEG::ecoord SHAPE_ELLIPSE::SquaredDistance( const VECTOR2I& aP, bool aOutlineOnly ) const
{
    const VECTOR2D local = toLocal( aP );

    // Interior of a closed ellipse if val < 1
    if( !m_isArc && !aOutlineOnly )
    {
        const double val = local.x * local.x * m_invMajorRSq + local.y * local.y * m_invMinorRSq;

        if( val <= 1.0 )
            return 0;
    }

    const VECTOR2D closest = closestLocalPoint( local );
    const double   dxE = closest.x - local.x;
    const double   dyE = closest.y - local.y;

    return static_cast<SEG::ecoord>( dxE * dxE + dyE * dyE );
}


VECTOR2I SHAPE_ELLIPSE::NearestPoint( const VECTOR2I& aP ) const
{
    return toWorld( closestLocalPoint( toLocal( aP ) ) );
}


SHAPE_ELLIPSE::CONIC SHAPE_ELLIPSE::conicOf( const VECTOR2I& aCenter, double aMajorR, double aMinorR,
                                             const EDA_ANGLE& aRotation ) const
{
    // Take a point of this local frame across to the other curve's local frame, then
    // square out the other curve's own equation to get the coefficients.
    const double delta = m_ellipse.Rotation.AsRadians() - aRotation.AsRadians();
    const double cd = std::cos( delta );
    const double sd = std::sin( delta );

    const double cr = std::cos( aRotation.AsRadians() );
    const double sr = std::sin( aRotation.AsRadians() );

    const double wx = static_cast<double>( m_ellipse.Center.x - aCenter.x );
    const double wy = static_cast<double>( m_ellipse.Center.y - aCenter.y );

    const double dx = wx * cr + wy * sr;
    const double dy = -wx * sr + wy * cr;

    const double p = 1.0 / ( aMajorR * aMajorR );
    const double q = 1.0 / ( aMinorR * aMinorR );

    CONIC conic;
    conic.Axx = p * cd * cd + q * sd * sd;
    conic.Axy = 2.0 * sd * cd * ( q - p );
    conic.Ayy = p * sd * sd + q * cd * cd;
    conic.Bx = 2.0 * ( p * dx * cd + q * dy * sd );
    conic.By = 2.0 * ( q * dy * cd - p * dx * sd );
    conic.C = p * dx * dx + q * dy * dy - 1.0;

    return conic;
}


std::vector<double> SHAPE_ELLIPSE::conicRoots( const CONIC& aConic ) const
{
    const double a = static_cast<double>( m_ellipse.MajorRadius );
    const double b = static_cast<double>( m_ellipse.MinorRadius );

    // The conic seen along this ellipse, as a function of the parameter angle
    const double cA = aConic.Axx * a * a;
    const double cB = aConic.Axy * a * b;
    const double cC = aConic.Ayy * b * b;
    const double cD = aConic.Bx * a;
    const double cE = aConic.By * b;
    const double cF = aConic.C;

    const auto value = [&]( double aTheta )
    {
        const double ct = std::cos( aTheta );
        const double st = std::sin( aTheta );
        return cA * ct * ct + cB * ct * st + cC * st * st + cD * ct + cE * st + cF;
    };

    const auto slope = [&]( double aTheta )
    {
        const double ct = std::cos( aTheta );
        const double st = std::sin( aTheta );
        return 2.0 * ( cC - cA ) * ct * st + cB * ( ct * ct - st * st ) - cD * st + cE * ct;
    };

    // The tangent of the half angle turns that into a quartic, at the cost of never
    // reaching pi.  That one angle is offered below as a candidate like any other.
    const double k4 = cA - cD + cF;
    const double k3 = 2.0 * ( cE - cB );
    const double k2 = 2.0 * ( cF - cA ) + 4.0 * cC;
    const double k1 = 2.0 * ( cB + cE );
    const double k0 = cA + cD + cF;

    const double scale = std::max( { std::abs( k4 ), std::abs( k3 ), std::abs( k2 ), std::abs( k1 ), std::abs( k0 ) } );

    // The curves lie on top of each other, so there is no crossing to report
    if( scale == 0.0 )
        return {};

    std::vector<double> params;

    for( double u : quarticRoots( k4 / scale, k3 / scale, k2 / scale, k1 / scale, k0 / scale ) )
        params.push_back( 2.0 * std::atan( u ) );

    params.push_back( M_PI );

    std::vector<double> result;

    for( double theta : params )
    {
        // The quartic loses digits when its roots run large, so finish on the exact equation
        for( int iter = 0; iter < 8; ++iter )
        {
            const double f = value( theta );
            const double df = slope( theta );

            if( df == 0.0 )
                break;

            const double step = f / df;
            theta -= step;

            if( std::abs( step ) < 1e-15 )
                break;
        }

        if( std::abs( value( theta ) ) > scale * 1e-9 )
            continue;

        if( m_isArc && !isAngleInSweep( theta ) )
            continue;

        result.push_back( theta );
    }

    return result;
}


std::vector<VECTOR2I> SHAPE_ELLIPSE::Intersect( const SHAPE_ELLIPSE& aOther ) const
{
    const CONIC conic = conicOf( aOther.m_ellipse.Center, aOther.m_ellipse.MajorRadius, aOther.m_ellipse.MinorRadius,
                                 aOther.m_ellipse.Rotation );

    std::vector<VECTOR2I> points;

    for( double theta : conicRoots( conic ) )
    {
        const VECTOR2I world = m_ellipse.GetPointAtAngle( EDA_ANGLE( theta, RADIANS_T ) );

        if( aOther.m_isArc && !aOther.isAngleInSweep( aOther.m_ellipse.GetAngleAtPoint( world ).AsRadians() ) )
        {
            continue;
        }

        points.push_back( world );
    }

    dedupePoints( points );
    return points;
}


std::vector<VECTOR2I> SHAPE_ELLIPSE::intersectCircle( const VECTOR2I& aCenter, double aRadius ) const
{
    std::vector<VECTOR2I> points;

    if( aRadius <= 0.0 )
        return points;

    for( double theta : conicRoots( conicOf( aCenter, aRadius, aRadius, ANGLE_0 ) ) )
        points.push_back( m_ellipse.GetPointAtAngle( EDA_ANGLE( theta, RADIANS_T ) ) );

    dedupePoints( points );
    return points;
}


std::vector<VECTOR2I> SHAPE_ELLIPSE::Intersect( const CIRCLE& aCircle ) const
{
    return intersectCircle( aCircle.Center, aCircle.Radius );
}


std::vector<VECTOR2I> SHAPE_ELLIPSE::Intersect( const SHAPE_ARC& aArc ) const
{
    // The radius is taken as a double, so the arc is not rounded to a CIRCLE first
    std::vector<VECTOR2I> points = intersectCircle( aArc.GetCenter(), aArc.GetRadius() );

    // Crossings of the full circle that miss the drawn part are pulled back to an end
    // of the arc, so they fail this test
    std::erase_if( points,
                   [&]( const VECTOR2I& aPoint )
                   {
                       return aArc.NearestPoint( aPoint ).Distance( aPoint ) > SHAPE::MIN_PRECISION_IU;
                   } );

    return points;
}


std::vector<VECTOR2I> SHAPE_ELLIPSE::Intersect( const SEG& aSeg, bool aTreatAsLine ) const
{
    const VECTOR2D start = toLocal( aSeg.A );
    const VECTOR2D dir = toLocal( aSeg.B ) - start;

    if( dir.x == 0.0 && dir.y == 0.0 )
        return {};

    // A straight line is a conic with no squared terms, so the quartic falls away by itself
    CONIC conic;
    conic.Axx = 0.0;
    conic.Axy = 0.0;
    conic.Ayy = 0.0;
    conic.Bx = dir.y;
    conic.By = -dir.x;
    conic.C = start.y * dir.x - start.x * dir.y;

    std::vector<VECTOR2I> points;

    for( double theta : conicRoots( conic ) )
    {
        const VECTOR2I world = m_ellipse.GetPointAtAngle( EDA_ANGLE( theta, RADIANS_T ) );

        if( aTreatAsLine || aSeg.Contains( world ) )
            points.push_back( world );
    }

    dedupePoints( points );
    return points;
}


SHAPE_LINE_CHAIN SHAPE_ELLIPSE::ConvertToPolyline( int aMaxError ) const
{
    if( aMaxError < 1 )
        aMaxError = 1;

    const double a = static_cast<double>( m_ellipse.MajorRadius );
    const double b = static_cast<double>( m_ellipse.MinorRadius );
    const double cx = m_ellipse.Center.x;
    const double cy = m_ellipse.Center.y;
    const double sinRot = m_sinRot;
    const double cosRot = m_cosRot;

    auto eval = [=]( double theta ) -> VECTOR2I
    {
        const double ct = std::cos( theta );
        const double st = std::sin( theta );
        const double lx = a * ct;
        const double ly = b * st;
        const double wx = lx * cosRot - ly * sinRot;
        const double wy = lx * sinRot + ly * cosRot;
        return VECTOR2I( static_cast<int>( std::round( cx + wx ) ), static_cast<int>( std::round( cy + wy ) ) );
    };

    double tStart, tEnd;
    sweepRange( tStart, tEnd );

    const double maxErrSq = static_cast<double>( aMaxError ) * aMaxError;

    SHAPE_LINE_CHAIN out;
    const VECTOR2I   pStart = eval( tStart );
    const VECTOR2I   pEnd = eval( tEnd );

    out.Append( pStart );
    subdivideEllipseArc( tStart, pStart, tEnd, pEnd, maxErrSq, 20, eval, out );

    if( !m_isArc )
    {
        if( out.PointCount() > 1 && out.CPoint( 0 ) == out.CPoint( -1 ) )
            out.Remove( out.PointCount() - 1 );

        out.SetClosed( true );
    }

    return out;
}


void SHAPE_ELLIPSE::sweepRange( double& aStart, double& aEnd ) const
{
    const double twoPi = 2.0 * M_PI;

    if( !m_isArc )
    {
        aStart = 0.0;
        aEnd = twoPi;
        return;
    }

    aStart = m_ellipse.StartAngle.AsRadians();
    aEnd = m_ellipse.EndAngle.AsRadians();

    const double sweep = aEnd - aStart;

    if( sweep >= twoPi || sweep <= -twoPi )
        aEnd = aStart + twoPi;
    else if( aEnd < aStart )
        aEnd += twoPi;
}


bool SHAPE_ELLIPSE::isAngleInSweep( double aAngleRad ) const
{
    const double twoPi = 2.0 * M_PI;
    double       tStart, tEnd;
    sweepRange( tStart, tEnd );

    // Reduce aAngleRad into [tStart, tStart + 2*pi).
    double t = aAngleRad;
    while( t < tStart )
        t += twoPi;
    while( t >= tStart + twoPi )
        t -= twoPi;

    return t <= tEnd;
}
