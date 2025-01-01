/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2017 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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


#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <utility>
#include <wx/log.h>

#include "3d_cache/sg/sg_helpers.h"
#include "3d_cache/sg/sg_node.h"


void S3D::FormatFloat( std::string& result, double value )
{
    if( value < 1e-8 &&  value > -1e-8 )
    {
        result = "0";
        return;
    }

    // note: many VRML implementations use float so we use the max.
    // precision here of 8 digits.
    std::ostringstream out;
    out << std::setprecision( 8 ) << value;

    result = out.str();

    size_t p = result.find( '.' );

    // trim trailing 0 if appropriate
    if( std::string::npos == p )
        return;

    p = result.find_first_of( "eE" );

    if( std::string::npos == p )
    {
        while( '0' == *( result.rbegin() ) )
            result.erase( result.size() - 1 );

        return;
    }

    if( '0' != result.at( p - 1 ) )
        return;

    // trim all 0 to the left of 'p'
    std::string tmp = result.substr( p );
    result = result.substr( 0, p );

    while( '0' == *( result.rbegin() ) )
        result.erase( result.size() - 1 );

    result.append( tmp );
}


void S3D::FormatOrientation( std::string& result, const SGVECTOR& axis, double rotation )
{
    double aX;
    double aY;
    double aZ;

    axis.GetVector( aX, aY, aZ );
    FormatFloat( result, aX );
    std::string tmp;
    FormatFloat( tmp, aY );
    result.append( " " );
    result.append( tmp );
    FormatFloat( tmp, aZ );
    result.append( " " );
    result.append( tmp );
    FormatFloat( tmp, rotation );
    result.append( " " );
    result.append( tmp );
}


void S3D::FormatPoint( std::string& result, const SGPOINT& point )
{
    FormatFloat( result, point.x );

    std::string tmp;
    FormatFloat( tmp, point.y );
    result.append( " " );
    result.append( tmp );

    FormatFloat( tmp, point.z );
    result.append( " " );
    result.append( tmp );
}


void S3D::FormatVector( std::string& result, const SGVECTOR& aVector )
{
    double X, Y, Z;
    aVector.GetVector( X, Y, Z );
    FormatFloat( result, X );

    std::string tmp;
    FormatFloat( tmp, Y );
    result.append( " " );
    result.append( tmp );

    FormatFloat( tmp, Z );
    result.append( " " );
    result.append( tmp );
}


void S3D::FormatColor( std::string& result, const SGCOLOR& aColor )
{
    float R, G, B;
    aColor.GetColor( R, G, B );
    FormatFloat( result, R );

    std::string tmp;
    FormatFloat( tmp, G );
    result.append( " " );
    result.append( tmp );

    FormatFloat( tmp, B );
    result.append( " " );
    result.append( tmp );
}


bool S3D::WritePoint( std::ostream& aFile, const SGPOINT& aPoint )
{
    aFile.write( (char*) &aPoint.x, sizeof( aPoint.x ) );
    aFile.write( (char*) &aPoint.y, sizeof( aPoint.y ) );
    aFile.write( (char*) &aPoint.z, sizeof( aPoint.z ) );

    if( aFile.fail() )
        return false;

    return true;
}


bool S3D::WriteVector( std::ostream& aFile, const SGVECTOR& aVector )
{
    double x, y, z;
    aVector.GetVector( x, y, z );
    aFile.write( (char*) &x, sizeof( double ) );
    aFile.write( (char*) &y, sizeof( double ) );
    aFile.write( (char*) &z, sizeof( double ) );

    if( aFile.fail() )
        return false;

    return true;
}


bool S3D::WriteColor( std::ostream& aFile, const SGCOLOR& aColor )
{
    float r, g, b;
    aColor.GetColor( r, g, b );
    aFile.write( (char*) &r, sizeof( float ) );
    aFile.write( (char*) &g, sizeof( float ) );
    aFile.write( (char*) &b, sizeof( float ) );

    if( aFile.fail() )
        return false;

    return true;
}


S3D::SGTYPES S3D::ReadTag( std::istream& aFile, std::string& aName )
{
    char schar;
    aFile.get( schar );

    if( '[' != schar )
    {
        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data; missing left bracket at "
                                     "position %d" ),
                    __FILE__, __FUNCTION__, __LINE__,
                    static_cast<int>( aFile.tellg() ) );

        return S3D::SGTYPE_END;
    }

    std::string name;
    aFile.get( schar );

    while( ']' != schar && aFile.good() )
    {
        name.push_back( schar );
        aFile.get( schar );
    }

    if( schar != ']' )
    {
        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data; could not find right "
                                     "bracket" ),
                    __FILE__, __FUNCTION__, __LINE__ );

        return S3D::SGTYPE_END;
    }

    aName = name;
    size_t upos = name.find( '_' );

    if( std::string::npos == upos )
    {
        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data; no underscore in name '%s'" ),
                    __FILE__, __FUNCTION__, __LINE__,
                    name );

        return S3D::SGTYPE_END;
    }

    name = name.substr( 0, upos );
    S3D::SGTYPES types[S3D::SGTYPE_END] = {
        SGTYPE_TRANSFORM,
        SGTYPE_APPEARANCE,
        SGTYPE_COLORS,
        SGTYPE_COLORINDEX,
        SGTYPE_FACESET,
        SGTYPE_COORDS,
        SGTYPE_COORDINDEX,
        SGTYPE_NORMALS,
        SGTYPE_SHAPE
    };

    for( int i = 0; i < S3D::SGTYPE_END; ++i )
    {
        if( !name.compare( S3D::GetNodeTypeName( types[i] ) ) )
            return types[i];
    }

    wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data; no node type matching '%s'" ),
                __FILE__, __FUNCTION__, __LINE__,
                name );

    return S3D::SGTYPE_END;
}


bool S3D::ReadPoint( std::istream& aFile, SGPOINT& aPoint )
{
    aFile.read( (char*) &aPoint.x, sizeof( aPoint.x ) );
    aFile.read( (char*) &aPoint.y, sizeof( aPoint.y ) );
    aFile.read( (char*) &aPoint.z, sizeof( aPoint.z ) );

    if( aFile.fail() )
        return false;

    return true;
}


bool S3D::ReadVector( std::istream& aFile, SGVECTOR& aVector )
{
    double x, y, z;
    aFile.read( (char*) &x, sizeof( double ) );
    aFile.read( (char*) &y, sizeof( double ) );
    aFile.read( (char*) &z, sizeof( double ) );
    aVector.SetVector( x, y, z );

    if( aFile.fail() )
        return false;

    return true;
}


bool S3D::ReadColor( std::istream& aFile, SGCOLOR& aColor )
{
    float r, g, b;
    aFile.read( (char*) &r, sizeof( float ) );
    aFile.read( (char*) &g, sizeof( float ) );
    aFile.read( (char*) &b, sizeof( float ) );
    aColor.SetColor( r, g, b );

    if( aFile.fail() )
        return false;

    return true;
}


bool S3D::degenerate( glm::dvec3* pts ) noexcept
{
    double dx, dy, dz;

    dx = pts[1].x - pts[0].x;
    dy = pts[1].y - pts[0].y;
    dz = pts[1].z - pts[0].z;

    if( ( dx*dx + dy*dy + dz*dz ) < 1e-15 )
        return true;

    dx = pts[2].x - pts[0].x;
    dy = pts[2].y - pts[0].y;
    dz = pts[2].z - pts[0].z;

    if( ( dx*dx + dy*dy + dz*dz ) < 1e-15 )
        return true;

    dx = pts[2].x - pts[1].x;
    dy = pts[2].y - pts[1].y;
    dz = pts[2].z - pts[1].z;

    if( ( dx*dx + dy*dy + dz*dz ) < 1e-15 )
        return true;

    return false;
}


static void calcTriad( glm::dvec3* pts, glm::dvec3& tri )
{
    if( S3D::degenerate( pts ) )
    {
        // degenerate points should contribute nothing to the result
        tri = glm::dvec3( 0.0, 0.0, 0.0 );
        return;
    }

    // normal * 2 * area
    tri = glm::cross( pts[1] - pts[0], pts[2] - pts[0] );
}


bool S3D::CalcTriangleNormals( std::vector< SGPOINT > coords, std::vector< int >& index,
                               std::vector< SGVECTOR >& norms )
{
    size_t vsize = coords.size();

    if( vsize < 3 )
    {
        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] invalid vertex set (fewer than 3 "
                                     "vertices)" ),
                    __FILE__, __FUNCTION__, __LINE__ );

        return false;
    }

    size_t isize = index.size();

    if( 0 != isize % 3 || index.empty() )
    {
        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] invalid index set (not multiple of 3)" ),
                    __FILE__, __FUNCTION__, __LINE__ );

        return false;
    }

    if( !norms.empty() )
    {
        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] normals set is not empty" ),
                    __FILE__, __FUNCTION__, __LINE__ );

        return false;
    }

    std::map< int, std::list< glm::dvec3 > >vmap;

    int p1, p2, p3;

    // create the map of indices to facet sets
    for( size_t i = 0; i < isize; )
    {
        p1 = index[i++];
        p2 = index[i++];
        p3 = index[i++];

        if( p1 < 0 || p1 >= (int)vsize || p2 < 0 || p2 >= (int)vsize || p3 < 0 || p3 >= (int)vsize )
        {
#ifdef DEBUG
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [INFO] invalid index set; index out of bounds";
            wxLogTrace( MASK_3D_SG, wxT( "%s\n" ), ostr.str().c_str() );
#endif

            return false;
        }

        glm::dvec3 tri;
        glm::dvec3 trip[3];
        trip[0] = glm::dvec3( coords[p1].x, coords[p1].y, coords[p1].z );
        trip[1] = glm::dvec3( coords[p2].x, coords[p2].y, coords[p2].z );
        trip[2] = glm::dvec3( coords[p3].x, coords[p3].y, coords[p3].z );
        calcTriad( trip, tri );

        std::map< int, std::list< glm::dvec3 > >::iterator ip = vmap.find( p1 );

        if( ip != vmap.end() )
        {
            ip->second.push_back( tri );
        }
        else
        {
            vmap.emplace( p1, std::list < glm::dvec3 >( 1, tri ) );
        }

        ip = vmap.find( p2 );

        if( ip != vmap.end() )
        {
            ip->second.push_back( tri );
        }
        else
        {
            vmap.emplace( p2, std::list < glm::dvec3 >( 1, tri ) );
        }

        ip = vmap.find( p3 );

        if( ip != vmap.end() )
        {
            ip->second.push_back( tri );
        }
        else
        {
            vmap.emplace( p3, std::list < glm::dvec3 >( 1, tri ) );
        }
    }

    std::map< int, std::list< glm::dvec3 > >::iterator sM = vmap.begin();
    std::map< int, std::list< glm::dvec3 > >::iterator eM = vmap.end();
    size_t idx = 0;

    while( sM != eM )
    {
        size_t item = sM->first;

        // assign any skipped coordinates a normal of (0,0,1)
        while( item > idx )
        {
            norms.emplace_back( 0, 0, 1 );
            ++idx;
        }

        std::list< glm::dvec3 >::iterator sT = sM->second.begin();
        std::list< glm::dvec3 >::iterator eT = sM->second.end();
        glm::dvec3 norm( 0.0, 0.0, 0.0 );

        while( sT != eT )
        {
            norm += *sT;
            ++sT;
        }

        norms.emplace_back( norm.x, norm.y, norm.z );

        ++idx;
        ++sM;
    }

    if( norms.size() != coords.size() )
    {
        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [BUG] number of normals does not equal number "
                                     "of vertices" ),
                    __FILE__, __FUNCTION__, __LINE__ );

        return false;
    }

    return true;
}
