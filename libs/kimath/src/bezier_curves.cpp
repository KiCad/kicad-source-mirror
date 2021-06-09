/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <math/vector2d.h>  // for VECTOR2D, operator*, VECTOR2
#include <wx/debug.h>       // for wxASSERT
#include <wx/gdicmn.h>      // for wxPoint


BEZIER_POLY::BEZIER_POLY( const std::vector<wxPoint>& aControlPoints )
{
    for( unsigned ii = 0; ii < aControlPoints.size(); ++ii )
        m_ctrlPts.emplace_back( VECTOR2D( aControlPoints[ii] ) );

    m_minSegLen = 0.0;
}


void BEZIER_POLY::GetPoly( std::vector<wxPoint>& aOutput, int aMinSegLen )
{
    aOutput.clear();
    std::vector<VECTOR2D> buffer;
    GetPoly( buffer, double( aMinSegLen ) );

    for( unsigned ii = 0; ii < buffer.size(); ++ii )
        aOutput.emplace_back( wxPoint( int( buffer[ii].x ), int( buffer[ii].y ) ) );
}


void BEZIER_POLY::GetPoly( std::vector<VECTOR2D>& aOutput, double aMinSegLen )
{
    wxASSERT( m_ctrlPts.size() == 4 );
    // FIXME Brute force method, use a better (recursive?) algorithm
    // with a max error value.
    // to optimize the number of segments
    #define CURVE_POINTS 32
    double dt = 1.0 / CURVE_POINTS;

    aOutput.clear();
    aOutput.push_back( m_ctrlPts[0] );

    // If the Bezier curve is degenerated (straight line), skip intermediate points:
    bool degenerated = m_ctrlPts[0] == m_ctrlPts[1] && m_ctrlPts[2] == m_ctrlPts[3];

    if( !degenerated )
    {
        for( int ii = 1; ii < CURVE_POINTS; ii++ )
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
            VECTOR2D delta = vertex - aOutput.back();
            double dist = delta.EuclideanNorm();

            if( dist > aMinSegLen )
                aOutput.push_back( vertex );
        }
    }

    if( aOutput.back() != m_ctrlPts[3] )
        aOutput.push_back( m_ctrlPts[3] );
}
