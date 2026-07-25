/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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

#ifndef SMITH_MATH_H
#define SMITH_MATH_H

#include <algorithm>
#include <cmath>
#include <limits>
#include <math/util.h>
#include <wx/gdicmn.h>
#include <wx/string.h>

///< Smith chart placement plus pan/zoom, maps a gamma point to a screen pixel and back.
struct SMITH_VIEW
{
    wxPoint     center;
    double      radius = 0.0; // unit-circle radius in pixels, already scaled by zoom
    double      zoom = 1.0;   // 1.0 = fit to window
    wxRealPoint pan;          // gamma point shown at the window center
    wxRect      plotRect;

    wxPoint ToScreen( double aRe, double aIm ) const
    {
        // clamp so huge or non-finite gammas cannot overflow the int math
        constexpr double LIMIT = 1e7;

        double x = ( aRe - pan.x ) * radius;
        double y = ( aIm - pan.y ) * radius;

        x = std::isfinite( x ) ? std::clamp( x, -LIMIT, LIMIT ) : 0.0;
        y = std::isfinite( y ) ? std::clamp( y, -LIMIT, LIMIT ) : 0.0;

        return wxPoint( center.x + KiROUND( x ), center.y - KiROUND( y ) );
    }

    wxRealPoint ToGamma( const wxPoint& aPt ) const
    {
        return wxRealPoint( pan.x + ( aPt.x - center.x ) / radius, pan.y - ( aPt.y - center.y ) / radius );
    }
};


namespace SMITH_MATH
{

///< Impedance of a reflection coefficient, z = z0 ( 1 + gamma ) / ( 1 - gamma ),
///< false at the gamma = 1 singularity.
inline bool GammaToImpedance( double aRe, double aIm, double aZ0, double& aResistance, double& aReactance )
{
    double denom = ( 1.0 - aRe ) * ( 1.0 - aRe ) + aIm * aIm;

    if( !std::isfinite( denom ) || denom < 1e-12 )
        return false;

    aResistance = ( 1.0 - aRe * aRe - aIm * aIm ) / denom * aZ0;
    aReactance = 2.0 * aIm / denom * aZ0;

    return true;
}

inline double VSWR( double aGammaMag )
{
    if( aGammaMag >= 1.0 )
        return std::numeric_limits<double>::infinity();

    return ( 1.0 + aGammaMag ) / ( 1.0 - aGammaMag );
}

inline double ReturnLoss( double aGammaMag )
{
    if( aGammaMag <= 0.0 )
        return std::numeric_limits<double>::infinity();

    return -20.0 * std::log10( aGammaMag );
}

///< Series equivalent element of a reactance at one frequency, henries for aReactance > 0,
///< farads for aReactance < 0.
inline double SeriesInductance( double aReactance, double aFreq )
{
    return aReactance / ( 2.0 * M_PI * aFreq );
}

inline double SeriesCapacitance( double aReactance, double aFreq )
{
    return 1.0 / ( 2.0 * M_PI * aFreq * -aReactance );
}

///< Pan that keeps the gamma point under aPos fixed when the view zooms to aNewZoom.
inline wxRealPoint ZoomAboutPoint( const SMITH_VIEW& aView, const wxPoint& aPos, double aNewZoom )
{
    wxRealPoint gamma = aView.ToGamma( aPos );
    double      newRadius = aView.radius / aView.zoom * aNewZoom;

    return wxRealPoint( gamma.x - ( aPos.x - aView.center.x ) / newRadius,
                        gamma.y + ( aPos.y - aView.center.y ) / newRadius );
}

///< S-parameter vectors are named S_<responsePort>_<drivePort>.
inline bool ParseSParamPorts( const wxString& aVectorName, long* aResponsePort, long* aDrivePort )
{
    if( !aVectorName.StartsWith( wxS( "S_" ) ) )
        return false;

    wxString rest = aVectorName.Mid( 2 );

    return rest.BeforeFirst( '_' ).ToLong( aResponsePort ) && rest.AfterFirst( '_' ).ToLong( aDrivePort );
}

} // namespace SMITH_MATH

#endif // SMITH_MATH_H
