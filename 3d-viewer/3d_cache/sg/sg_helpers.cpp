/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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


#include <iostream>
#include <sstream>
#include <streambuf>
#include <iomanip>
#include <string>
#include <utility>
#include <map>

#include "3d_cache/sg/sg_helpers.h"
#include "3d_cache/sg/sg_node.h"


// formats a floating point number for text output to a VRML file
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

    size_t p = result.find( "." );

    // trim trailing 0 if appropriate

    if( std::string::npos == p )
        return;

    p = result.find_first_of( "eE" );

    if( std::string::npos == p )
    {
        while( '0' == *(result.rbegin()) )
            result.erase( result.size() - 1 );

        return;
    }

    if( '0' != result.at( p -1 ) )
        return;

    // trim all 0 to the left of 'p'
    std::string tmp = result.substr( p );
    result = result.substr( 0, p );

    while( '0' == *(result.rbegin()) )
        result.erase( result.size() - 1 );

    result.append( tmp );

    return;
}

// format orientation data for VRML output
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

    return;
}

// format point data for VRML output
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

    return;
}


// format vector data for VRML output
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

    return;
}


// format Color data for VRML output
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

    return;
}


bool S3D::WritePoint( std::ofstream& aFile, const SGPOINT& aPoint )
{
    aFile.write( (char*)&aPoint.x, sizeof(aPoint.x) );
    aFile.write( (char*)&aPoint.y, sizeof(aPoint.y) );
    aFile.write( (char*)&aPoint.z, sizeof(aPoint.z) );
}


bool S3D::WriteVector( std::ofstream& aFile, const SGVECTOR& aVector )
{
    double x, y, z;
    aVector.GetVector( x, y, z );
    aFile.write( (char*)&x, sizeof(double) );
    aFile.write( (char*)&y, sizeof(double) );
    aFile.write( (char*)&z, sizeof(double) );

    if( aFile.fail() )
        return false;

    return true;
}


bool S3D::WriteColor( std::ofstream& aFile, const SGCOLOR& aColor )
{
    float r, g, b;
    aColor.GetColor( r, g, b );
    aFile.write( (char*)&r, sizeof(float) );
    aFile.write( (char*)&g, sizeof(float) );
    aFile.write( (char*)&b, sizeof(float) );

    if( aFile.fail() )
        return false;

    return true;
}


S3D::SGTYPES S3D::ReadTag( std::ifstream& aFile, std::string& aName )
{
    char schar;
    aFile.get( schar );

    if( '[' != schar )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] corrupt data; missing left bracket at position ";
        std::cerr << aFile.tellg() << "\n";
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
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] corrupt data; could not find right bracket\n";
        return S3D::SGTYPE_END;
    }

    aName = name;
    size_t upos = name.find( '_' );

    if( std::string::npos == upos )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] corrupt data; no underscore in name '";
        std::cerr << name << "'\n";
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

    std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    std::cerr << " * [INFO] corrupt data; no node type matching '";
    std::cerr << name << "'\n";
    return S3D::SGTYPE_END;
}


bool S3D::ReadPoint( std::ifstream& aFile, SGPOINT& aPoint )
{
    aFile.read( (char*)&aPoint.x, sizeof( aPoint.x ) );
    aFile.read( (char*)&aPoint.y, sizeof( aPoint.y ) );
    aFile.read( (char*)&aPoint.z, sizeof( aPoint.z ) );

    if( aFile.fail() )
        return false;

    return true;
}


bool S3D::ReadVector( std::ifstream& aFile, SGVECTOR& aVector )
{
    double x, y, z;
    aFile.read( (char*)&x, sizeof(double) );
    aFile.read( (char*)&y, sizeof(double) );
    aFile.read( (char*)&z, sizeof(double) );
    aVector.SetVector( x, y, z );

    if( aFile.fail() )
        return false;

    return true;
}


bool S3D::ReadColor( std::ifstream& aFile, SGCOLOR& aColor )
{
    float r, g, b;
    aFile.read( (char*)&r, sizeof(float) );
    aFile.read( (char*)&g, sizeof(float) );
    aFile.read( (char*)&b, sizeof(float) );
    aColor.SetColor( r, g, b );

    if( aFile.fail() )
        return false;

    return true;
}


struct TRIAD
{
    int p1;
    int p2;
    int p3;
};


bool S3D::CalcTriangleNormals( std::vector< SGPOINT > coords,
    std::vector< int >& index, std::vector< SGVECTOR >& norms )
{
    size_t vsize = coords.size();

    if( vsize < 3 )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] invalid vertex set (fewer than 3 vertices)\n";
        #endif

        return false;
    }

    size_t isize = index.size();

    if( 0 != isize % 3 || index.empty() )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] invalid index set (not multiple of 3)\n";
        #endif

        return false;
    }

    if( !norms.empty() )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] normals set is not empty\n";
        #endif

        return false;
    }

    std::map< int, std::list< TRIAD > >vmap;

    int p1, p2, p3;

    // create the map of indices to facet sets
    for( size_t i = 0; i < isize; )
    {
        p1 = index[i++];
        p2 = index[i++];
        p3 = index[i++];

        if( p1 < 0 || p1 >= vsize || p2 < 0 || p2 >= vsize ||
            p3 < 0 || p3 >= vsize )
        {
            #ifdef DEBUG
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] invalid index set; index out of bounds\n";
            #endif

            return false;
        }

        // ignore degenerate triangle indices; note that it is still possible to
        // have degenerate vertices and these may cause problems
        if( p1 == p2 || p2 == p3 || p3 == p1 )
            continue;

        TRIAD tri;
        tri.p1 = p1;
        tri.p2 = p2;
        tri.p3 = p3;

        std::map< int, std::list< TRIAD > >::iterator ip = vmap.find( p1 );

        if( ip != vmap.end() )
        {
            ip->second.push_back( tri );
        }
        else
        {
            vmap.insert( std::pair < int, std::list < TRIAD > > ( p1, std::list < TRIAD >( 1, tri ) ) );
        }

        ip = vmap.find( p2 );

        if( ip != vmap.end() )
        {
            ip->second.push_back( tri );
        }
        else
        {
            vmap.insert( std::pair < int, std::list < TRIAD > > ( p2, std::list < TRIAD >( 1, tri ) ) );
        }

        ip = vmap.find( p2 );

        if( ip != vmap.end() )
        {
            ip->second.push_back( tri );
        }
        else
        {
            vmap.insert( std::pair < int, std::list < TRIAD > > ( p3, std::list < TRIAD >( 1, tri ) ) );
        }
    }

    std::map< int, std::list< TRIAD > >::iterator sM = vmap.begin();
    std::map< int, std::list< TRIAD > >::iterator eM = vmap.end();
    size_t idx = 0;

    while( sM != eM )
    {
        size_t item = sM->first;

        // assign any skipped coordinates a normal of (0,0,1)
        while( item > idx )
        {
            norms.push_back( SGVECTOR( 0, 0, 1 ) );
            ++idx;
        }

        std::list< TRIAD >::iterator sT = sM->second.begin();
        std::list< TRIAD >::iterator eT = sM->second.end();

        double nx = 0.0;
        double ny = 0.0;
        double nz = 0.0;

        // XXX - TODO:
        // eliminate equal face normals in order to prevent distortion of the true vertex normal

        while( sT != eT )
        {
            double x0, x1, x2;
            double y0, y1, y2;
            double z0, z1, z2;

            x1 = coords[sT->p2].x - coords[sT->p1].x;
            y1 = coords[sT->p2].y - coords[sT->p1].y;
            z1 = coords[sT->p2].z - coords[sT->p1].z;

            x2 = coords[sT->p3].x - coords[sT->p1].x;
            y2 = coords[sT->p3].y - coords[sT->p1].y;
            z2 = coords[sT->p3].z - coords[sT->p1].z;

            x0 = (y1 * z2) - (z1 * y2);
            y0 = (z1 * x2) - (x1 * z2);
            z0 = (x1 * y2) - (y1 * x2);

            double m = sqrt( x0*x0 + y0*y0 + z0*z0 );

            // add the normal to the normal accumulated for this facet

            if( m < 1e-12 )
            {
                nz += 1.0;
            }
            else
            {
                nx += ( x0 / m );
                ny += ( y0 / m );
                nz += ( z0 / m );
            }

            ++sT;
        }

        norms.push_back( SGVECTOR( nx, ny, nz ) );

        ++idx;
        ++sM;
    }

    if( norms.size() != coords.size() )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] number of normals does not equal number of vertices\n";
        return false;
    }

    return true;
}
