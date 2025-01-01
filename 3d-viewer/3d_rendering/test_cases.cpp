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
 * @file  test_cases.cpp
 * @brief Implements a test cases to test individual implementations classes
 * @brief it run only once and only in debug build
 */

#include "raytracing/frustum.h"
#include "raytracing/shapes2D/bbox_2d.h"
#include "raytracing/shapes2D/filled_circle_2d.h"
#include "raytracing/shapes2D/polygon_2d.h"
#include "raytracing/shapes2D/round_segment_2d.h"
#include "raytracing/shapes3D/bbox_3d.h"
#include <wx/debug.h>

//#ifdef DEBUG
#if 0
static bool s_Run_Test_Cases = true;

void Run_3d_viewer_test_cases()
{
    if( s_Run_Test_Cases == false )
        return;

    s_Run_Test_Cases = true;

    // Test BBOX_2D
    BBOX_2D bbox2d_A;
    BBOX_2D bbox2d_B;

    // Test a not initialized box conditions
    bbox2d_A.Set( SFVEC2F( 1.0f, -1.0f ), SFVEC2F( -1.0f, 1.0f ) );

    wxASSERT( bbox2d_A.IsInitialized() == true );
    wxASSERT( bbox2d_A.Area() == 4.0f );
    wxASSERT( bbox2d_A.GetCenter() == SFVEC2F( 0.0f, 0.0f ) );
    wxASSERT( bbox2d_A.GetExtent() == SFVEC2F( 2.0f, 2.0f ) );
    wxASSERT( bbox2d_A.Inside( SFVEC2F( 0.0f, 0.0f ) ) == true );
    wxASSERT( bbox2d_A.Max() == SFVEC2F( 1.0f, 1.0f ) );
    wxASSERT( bbox2d_A.Min() == SFVEC2F(-1.0f,-1.0f ) );
    wxASSERT( bbox2d_A.Intersects( bbox2d_A ) == true );
    wxASSERT( bbox2d_A.MaxDimension() == 0 );
    wxASSERT( bbox2d_A.Perimeter() == 8.0f );

    bbox2d_A.Scale( 2.0f );

    wxASSERT( bbox2d_A.IsInitialized() == true );
    wxASSERT( bbox2d_A.Area() == 16.0f );
    wxASSERT( bbox2d_A.GetCenter() == SFVEC2F( 0.0f, 0.0f ) );
    wxASSERT( bbox2d_A.GetExtent() == SFVEC2F( 4.0f, 4.0f ) );
    wxASSERT( bbox2d_A.Inside( SFVEC2F( 0.0f, 0.0f ) ) == true );
    wxASSERT( bbox2d_A.Max() == SFVEC2F( 2.0f, 2.0f ) );
    wxASSERT( bbox2d_A.Min() == SFVEC2F(-2.0f,-2.0f ) );
    wxASSERT( bbox2d_A.Intersects( bbox2d_A ) == true );
    wxASSERT( bbox2d_A.MaxDimension() == 0 );
    wxASSERT( bbox2d_A.Perimeter() == 16.0f );

    bbox2d_B.Set( SFVEC2F(2.1f, 2.0f), SFVEC2F( 3.0f, 3.0f) );
    wxASSERT( bbox2d_A.Intersects( bbox2d_B ) == false );
    bbox2d_B.Set( SFVEC2F(2.0f, 2.1f), SFVEC2F( 3.0f, 3.0f) );
    wxASSERT( bbox2d_A.Intersects( bbox2d_B ) == false );
    bbox2d_B.Set( SFVEC2F(2.1f, 2.1f), SFVEC2F( 3.0f, 3.0f) );
    wxASSERT( bbox2d_A.Intersects( bbox2d_B ) == false );

    bbox2d_B.Set( SFVEC2F(2.0f, 2.0f), SFVEC2F( 3.0f, 3.0f) );
    wxASSERT( bbox2d_A.Intersects( bbox2d_B ) == true );

    bbox2d_A.Union( bbox2d_B );

    wxASSERT( bbox2d_A.IsInitialized() == true );
    wxASSERT( bbox2d_A.Area() == 25.0f );
    wxASSERT( bbox2d_A.GetCenter() == SFVEC2F( 0.5f, 0.5f ) );
    wxASSERT( bbox2d_A.GetExtent() == SFVEC2F( 5.0f, 5.0f ) );
    wxASSERT( bbox2d_A.Inside( SFVEC2F( 0.0f, 0.0f ) ) == true );
    wxASSERT( bbox2d_A.Inside( SFVEC2F( 3.0f, 3.0f ) ) == true );
    wxASSERT( bbox2d_A.Inside( SFVEC2F(-2.0f,-2.0f ) ) == true );
    wxASSERT( bbox2d_A.Max() == SFVEC2F( 3.0f, 3.0f ) );
    wxASSERT( bbox2d_A.Min() == SFVEC2F(-2.0f,-2.0f ) );
    wxASSERT( bbox2d_A.Intersects( bbox2d_B ) == true );
    wxASSERT( bbox2d_A.Intersects( bbox2d_A ) == true );
    wxASSERT( bbox2d_A.MaxDimension() == 0 );
    wxASSERT( bbox2d_A.Perimeter() == 20.0f );

    bbox2d_A.Set( SFVEC2F( -1.0f, -1.0f ), SFVEC2F( 1.0f, 1.0f ) );
    bbox2d_B.Set( SFVEC2F( -2.0f, -2.0f ), SFVEC2F( 2.0f, 2.0f ) );
    wxASSERT( bbox2d_A.Intersects( bbox2d_B ) == true );
    wxASSERT( bbox2d_B.Intersects( bbox2d_A ) == true );

    bbox2d_B.Set( SFVEC2F( 1.0f, 1.0f ), SFVEC2F( 1.0f, 1.0f ) );
    wxASSERT( bbox2d_A.Intersects( bbox2d_B ) == true );

    bbox2d_B.Set( SFVEC2F( 1.1f, 1.1f ), SFVEC2F( 2.0f, 2.0f ) );
    wxASSERT(  bbox2d_A.Intersects( bbox2d_B ) == false );

    bbox2d_B.Set( SFVEC2F(-0.5f, -0.5f ), SFVEC2F( 0.5f, 0.5f ) );
    wxASSERT( bbox2d_A.Intersects( bbox2d_B ) == true );

    // Test FILLED_CIRCLE_2D
    FILLED_CIRCLE_2D filledCircle2d( SFVEC2F( 2.0f, 2.0f ), 1.0f );

    wxASSERT( filledCircle2d.IsPointInside( SFVEC2F( 2.0f, 2.0f ) ) == true );

    wxASSERT( filledCircle2d.IsPointInside( SFVEC2F( 2.0f, 3.0f ) ) == true );
    wxASSERT( filledCircle2d.IsPointInside( SFVEC2F( 3.0f, 2.0f ) ) == true );
    wxASSERT( filledCircle2d.IsPointInside( SFVEC2F( 1.0f, 2.0f ) ) == true );
    wxASSERT( filledCircle2d.IsPointInside( SFVEC2F( 2.0f, 1.0f ) ) == true );

    wxASSERT( filledCircle2d.IsPointInside( SFVEC2F( 2.8f, 2.8f ) ) == false );
    wxASSERT( filledCircle2d.IsPointInside( SFVEC2F( 2.6f, 2.6f ) ) == true );
    wxASSERT( filledCircle2d.IsPointInside( SFVEC2F( 1.2f, 1.2f ) ) == false );

    bbox2d_B.Set( SFVEC2F( 0.0f, 0.0f ), SFVEC2F( 4.0f, 4.0f ) );
    wxASSERT( filledCircle2d.Intersects( bbox2d_B ) == true );

    bbox2d_B.Set( SFVEC2F( 1.5f, 1.5f ), SFVEC2F( 2.5f, 2.5f ) );
    wxASSERT( filledCircle2d.Intersects( bbox2d_B ) == true );

    // A box that does not intersect the sphere but still intersect the bbox of the sphere
    bbox2d_B.Set( SFVEC2F( 0.0f, 0.0f ), SFVEC2F( 1.2f, 1.2f ) );
    wxASSERT( filledCircle2d.Intersects( bbox2d_B ) == false );

    bbox2d_B.Set( SFVEC2F(-1.0f, -1.0f ), SFVEC2F( 0.5f, 0.5f ) );
    wxASSERT( filledCircle2d.Intersects( bbox2d_B ) == false );

    bbox2d_B.Set( SFVEC2F( 0.0f, 0.0f ), SFVEC2F( 2.0f, 2.0f ) );
    wxASSERT( filledCircle2d.Intersects( bbox2d_B ) == true );

    bbox2d_B.Set( SFVEC2F( 0.0f, 0.0f ), SFVEC2F( 2.0f, 4.0f ) );
    wxASSERT( filledCircle2d.Intersects( bbox2d_B ) == true );

    bbox2d_B.Set( SFVEC2F( 2.0f, 0.0f ), SFVEC2F( 4.0f, 4.0f ) );
    wxASSERT( filledCircle2d.Intersects( bbox2d_B ) == true );

    bbox2d_B.Set( SFVEC2F( 0.0f, 2.0f ), SFVEC2F( 4.0f, 4.0f ) );
    wxASSERT( filledCircle2d.Intersects( bbox2d_B ) == true );

    bbox2d_B.Set( SFVEC2F( 0.0f, 0.0f ), SFVEC2F( 4.0f, 2.0f ) );
    wxASSERT( filledCircle2d.Intersects( bbox2d_B ) == true );


    // Test ROUND_SEGMENT_2D
    ROUND_SEGMENT_2D roundSegment2d( SFVEC2F( -1.0f, 0.0f ), SFVEC2F( 1.0f, 0.0f ), 2.0f );

    wxASSERT( roundSegment2d.IsPointInside( SFVEC2F( 0.0f, 0.0f ) ) == true );
    wxASSERT( roundSegment2d.IsPointInside( SFVEC2F( 1.0f, 0.0f ) ) == true );
    wxASSERT( roundSegment2d.IsPointInside( SFVEC2F(-1.0f, 0.0f ) ) == true );
    wxASSERT( roundSegment2d.IsPointInside( SFVEC2F(-2.0f, 0.0f ) ) == true );
    wxASSERT( roundSegment2d.IsPointInside( SFVEC2F( 2.0f, 0.0f ) ) == true );
    wxASSERT( roundSegment2d.IsPointInside( SFVEC2F( 0.0f, 1.0f ) ) == true );
    wxASSERT( roundSegment2d.IsPointInside( SFVEC2F( 0.0f,-1.0f ) ) == true );
    wxASSERT( roundSegment2d.IsPointInside( SFVEC2F( 0.0f, 1.1f ) ) == false );
    wxASSERT( roundSegment2d.IsPointInside( SFVEC2F( 0.0f,-1.1f ) ) == false );
    wxASSERT( roundSegment2d.IsPointInside( SFVEC2F( 2.1f, 0.0f ) ) == false );
    wxASSERT( roundSegment2d.IsPointInside( SFVEC2F( 2.1f, 0.0f ) ) == false );

    wxASSERT( roundSegment2d.IsPointInside( SFVEC2F( 1.8f, 0.8f ) ) == false );
    wxASSERT( roundSegment2d.IsPointInside( SFVEC2F( 1.8f,-0.8f ) ) == false );

    wxASSERT( roundSegment2d.IsPointInside( SFVEC2F( -1.8f, 0.8f ) ) == false );
    wxASSERT( roundSegment2d.IsPointInside( SFVEC2F( -1.8f,-0.8f ) ) == false );

    wxASSERT( roundSegment2d.IsPointInside( SFVEC2F( 1.6f, 0.6f ) ) == true );
    wxASSERT( roundSegment2d.IsPointInside( SFVEC2F( 1.6f,-0.6f ) ) == true );

    wxASSERT( roundSegment2d.IsPointInside( SFVEC2F( -1.6f, 0.6f ) ) == true );
    wxASSERT( roundSegment2d.IsPointInside( SFVEC2F( -1.6f,-0.6f ) ) == true );

    bbox2d_A.Set( SFVEC2F(-2.0f,-1.0f), SFVEC2F( 2.0f, 1.0f) );
    wxASSERT( roundSegment2d.Intersects( bbox2d_A ) == true );

    bbox2d_A.Set( SFVEC2F(-2.1f,-1.1f ), SFVEC2F( 2.1f, 1.1f ) );
    wxASSERT( roundSegment2d.Intersects( bbox2d_A ) == true );

    bbox2d_A.Set( SFVEC2F( -1.9f,-0.9f ), SFVEC2F( 1.9f, 0.9f ) );
    wxASSERT( roundSegment2d.Intersects( bbox2d_A ) == true );

    bbox2d_A.Set( SFVEC2F( -1.0f,-1.0f ), SFVEC2F( 1.0f, 1.0f ) );
    wxASSERT( roundSegment2d.Intersects( bbox2d_A ) == true );

    bbox2d_A.Set( SFVEC2F( -1.0f,-0.5f ), SFVEC2F( 1.0f, 0.5f ) );
    wxASSERT( roundSegment2d.Intersects( bbox2d_A ) == true );

    bbox2d_A.Set( SFVEC2F( -4.0f,-0.5f ), SFVEC2F( -3.0f, 0.5f ) );
    wxASSERT( roundSegment2d.Intersects( bbox2d_A ) == false );

    bbox2d_A.Set( SFVEC2F( 1.8f, 0.8f ), SFVEC2F( 2.0f, 1.0f ) );
    wxASSERT( roundSegment2d.Intersects( bbox2d_A ) == false );

    bbox2d_A.Set( SFVEC2F( -2.0f, 0.8f ), SFVEC2F( -1.8f, 1.0f ) );
    wxASSERT( roundSegment2d.Intersects( bbox2d_A ) == false );

    bbox2d_A.Set( SFVEC2F( -2.0f, -1.0f ), SFVEC2F( -1.8f, -0.8f ) );
    wxASSERT( roundSegment2d.Intersects( bbox2d_A ) == false );

    // Test CPOLYGON2D
    Polygon2d_TestModule();
#if 0
    // Test Frustum
    {
    FRUSTUM frustum;

    SFVEC3F ori = SFVEC3F( 0.0, 0.0, 0.0 );

    float z = 10.0;

    const RAY topLeft(     ori, glm::normalize( ori - SFVEC3F(+1.0,+1.0, z) ) );
    const RAY topRight(    ori, glm::normalize( ori - SFVEC3F(-1.0,+1.0, z) ) );
    const RAY bottomLeft(  ori, glm::normalize( ori - SFVEC3F(+1.0,-1.0, z) ) );
    const RAY bottomRight( ori, glm::normalize( ori - SFVEC3F(-1.0,-1.0, z) ) );

    frustum.GenerateFrustum( topLeft, topRight, bottomLeft, bottomRight );

    BBOX_3D bbox3d;

    bbox3d.Set( SFVEC3F( -1.0f, -1.0f, z ), SFVEC3F( +1.0f, +1.0f, z + 1.0f ) );
    wxASSERT( frustum.Intersect( bbox3d ) == true );

    bbox3d.Set( SFVEC3F( -1.0f, -1.0f,  z + z ), SFVEC3F( +1.0f, +1.0f, z + z + 1.0f ) );
    wxASSERT( frustum.Intersect( bbox3d ) == true );

    bbox3d.Set( SFVEC3F( -1.0f, -1.0f, 1.0f ), SFVEC3F( 0.0f, 0.0f, 2.0f ) );
    wxASSERT( frustum.Intersect( bbox3d ) == true );

    bbox3d.Set( SFVEC3F( -1.0f, -1.0f, -z - 1.0f ), SFVEC3F( +1.0f, +1.0f, -z ) );
    wxASSERT( frustum.Intersect( bbox3d ) == false );

    bbox3d.Set( SFVEC3F( z - 1.0f, z - 1.0f, 0.0 ), SFVEC3F( z + 1.0f, z + 1.0f, 1.0 ) );
    wxASSERT( frustum.Intersect( bbox3d ) == false );

    bbox3d.Set( SFVEC3F( -z, -z, 0.0), SFVEC3F( -1.0f, -1.0f, 1.0f ) );
    wxASSERT( frustum.Intersect( bbox3d ) == false );

    bbox3d.Set( SFVEC3F( -z, -z, -1.0f ), SFVEC3F( +z, +z, 1.0f ) );
    wxASSERT( frustum.Intersect( bbox3d ) == true );

    bbox3d.Set( SFVEC3F( -z, -z, z-1 ), SFVEC3F( +z, +z, z + 1.0f ) );
    wxASSERT( frustum.Intersect( bbox3d ) == true );

    bbox3d.Set( SFVEC3F( 0.5, 0.5, 0.0 ), SFVEC3F( 2.0, 2.0, z ) );
    wxASSERT( frustum.Intersect( bbox3d ) == true );

    bbox3d.Set( SFVEC3F( 1.1, 1.0, 0.0 ), SFVEC3F( 2.0, 2.0, z ) );
    wxASSERT( frustum.Intersect( bbox3d ) == false );
    }
    {
    FRUSTUM frustum;

    float z = 10.0;

    SFVEC3F ori = SFVEC3F( 0.0, 0.0, z );

    const RAY topLeft( ori, glm::normalize( SFVEC3F( -1.0, 1.0, 0.0 ) - ori ) );
    const RAY topRight( ori, glm::normalize( SFVEC3F( +1.0, 1.0, 0.0 ) - ori ) );
    const RAY bottomLeft( ori, glm::normalize( SFVEC3F( -1.0, -1.0, 0.0 ) - ori ) );
    const RAY bottomRight( ori, glm::normalize( SFVEC3F( +1.0, -1.0, 0.0 ) - ori ) );

    frustum.GenerateFrustum( topLeft, topRight, bottomLeft, bottomRight );

    BBOX_3D bbox3d;

    bbox3d.Set( SFVEC3F( -1.0f, -1.0f, -z), SFVEC3F( +1.0f, +1.0f, -z + 1.0f ) );
    wxASSERT( frustum.Intersect( bbox3d ) == true );

    bbox3d.Set( SFVEC3F( -1.0f, -1.0f, -( z + z ) ), SFVEC3F( +1.0f, +1.0f, -( z + z + 1.0f ) ) );
    wxASSERT( frustum.Intersect( bbox3d ) == true );

    bbox3d.Set( SFVEC3F(- 1.0f, -1.0f, 1.0f ), SFVEC3F( 0.0f, 0.0f, 2.0f ) );
    wxASSERT( frustum.Intersect( bbox3d ) == true );

    // !TODO: The frustum alg does not exclude all the situations
    //bbox3d.Set( SFVEC3F(-1.0f, -1.0f, z+1.0f), SFVEC3F(+1.0f,+1.0f, +z+2.0f) );
    //wxASSERT( frustum.Intersect( bbox3d ) == false );

    bbox3d.Set( SFVEC3F( z - 1.0f, z - 1.0f, 0.0), SFVEC3F( z + 1.0f, z + 1.0f, 1.0 ) );
    wxASSERT( frustum.Intersect( bbox3d ) == false );

    bbox3d.Set( SFVEC3F( -z, -z, 0.0 ), SFVEC3F( -1.0f,-1.0f, 1.0f ) );
    wxASSERT( frustum.Intersect( bbox3d ) == false );

    bbox3d.Set( SFVEC3F( -z, -z, -1.0f ), SFVEC3F( +z, +z, 1.0f ) );
    wxASSERT( frustum.Intersect( bbox3d ) == true );

    bbox3d.Set( SFVEC3F( -z, -z, -( z - 1 ) ), SFVEC3F( +z, +z, -( z + 1.0f ) ) );
    wxASSERT( frustum.Intersect( bbox3d ) == true );

    bbox3d.Set( SFVEC3F( 0.5, 0.5, 0.0 ), SFVEC3F( 2.0, 2.0, z ) );
    wxASSERT( frustum.Intersect( bbox3d ) == true );

    bbox3d.Set( SFVEC3F( 1.1, 1.0, 0.0 ), SFVEC3F( 2.0, 2.0, z ) );
    wxASSERT( frustum.Intersect( bbox3d ) == false );
    }
#endif
}
#endif
