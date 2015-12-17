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

#include <cmath>
#include "plugins/3d/3d_plugin.h"
#include "plugins/3dapi/ifsg_all.h"


#define PLUGIN_3D_DEMO2_MAJOR 1
#define PLUGIN_3D_DEMO2_MINOR 0
#define PLUGIN_3D_DEMO2_PATCH 0
#define PLUGIN_3D_DEMO2_REVNO 0


const char* GetKicadPluginName( void )
{
    return "PLUGIN_3D_DEMO2";
}


void GetPluginVersion( unsigned char* Major, unsigned char* Minor,
    unsigned char* Patch, unsigned char* Revision )
{
    if( Major )
        *Major = PLUGIN_3D_DEMO2_MAJOR;

    if( Minor )
        *Minor = PLUGIN_3D_DEMO2_MINOR;

    if( Patch )
        *Patch = PLUGIN_3D_DEMO2_PATCH;

    if( Revision )
        *Revision = PLUGIN_3D_DEMO2_REVNO;

    return;
}


// number of extensions supported
#ifdef _WIN32
#define NEXTS 1
#else
#define NEXTS 2
#endif

// number of filter sets supported
#define NFILS 1

static char ext0[] = "wrl";

#ifdef _WIN32
static char fil0[] = "VRML 1.0/2.0 (*.wrl)|*.wrl";
#else
static char ext1[] = "WRL";

static char fil0[] = "VRML 1.0/2.0 (*.wrl;*.WRL)|*.wrl;*.WRL";
#endif


static struct FILE_DATA
{
    char const* extensions[NEXTS];
    char const* filters[NFILS];

    FILE_DATA()
    {
        extensions[0] = ext0;
        filters[0] = fil0;

#ifndef _WIN32
        extensions[1] = ext1;
#endif
        return;
    }

} file_data;


int GetNExtensions( void )
{
    return NEXTS;
}


char const* GetModelExtension( int aIndex )
{
    if( aIndex < 0 || aIndex >= NEXTS )
        return NULL;

    return file_data.extensions[aIndex];
}


int GetNFilters( void )
{
    return NFILS;
}


char const* GetFileFilter( int aIndex )
{
    if( aIndex < 0 || aIndex >= NFILS )
        return NULL;

    return file_data.filters[aIndex];
}


bool CanRender( void )
{
    // this dummy pretends to render a VRML model
    return true;
}


SCENEGRAPH* Load( char const* aFileName )
{
    // For this demonstration we create a tetrahedron (tx1) consisting of a SCENEGRAPH
    // (VRML Transform) which in turn contains 4 SGSHAPE (VRML Shape) objects
    // representing each of the sides of the tetrahedron. Each Shape is associated
    // with a color (SGAPPEARANCE) and a SGFACESET (VRML Geometry->indexedFaceSet).
    // Each SGFACESET is associated with a vertex list (SGCOORDS), a per-vertex normals
    // list (SGNORMALS), and a coordinate index (SGCOORDINDEX). One shape is used to
    // represent each face so that we may use per-vertex-per-face normals.
    //
    // The tetrahedron in turn is a child of a top level SCENEGRAPH (tx0) which has
    // a second SCENEGRAPH child (tx2) which is a transformation of the tetrahedron tx1
    // (rotation + translation). This demonstrates the reuse of components within
    // the model heirarchy.

    // define the vertices of the tetrahedron
    // face 1: 0, 3, 1
    // face 2: 0, 2, 3
    // face 3: 1, 3, 2
    // face 4: 0, 1, 2
    double SQ2 = sqrt( 0.5 );
    SGPOINT vert[4];
    vert[0] = SGPOINT( 1.0, 0.0, -SQ2 );
    vert[1] = SGPOINT( -1.0, 0.0, -SQ2 );
    vert[2] = SGPOINT( 0.0, 1.0, SQ2 );
    vert[3] = SGPOINT( 0.0, -1.0, SQ2 );


    // create the top level transform; this will hold all other
    // scenegraph objects; a transform may hold other transforms and
    // shapes
    IFSG_TRANSFORM* tx0 = new IFSG_TRANSFORM( true );

    // create the transform which will house the shapes
    IFSG_TRANSFORM* tx1 = new IFSG_TRANSFORM( tx0->GetRawPtr() );

    // add a shape which we will use to define one face of the tetrahedron; shapes
    // hold facesets and appearances
    IFSG_SHAPE* shape = new IFSG_SHAPE( *tx1 );

    // add a faceset; these contain coordinate lists, coordinate indices,
    // vertex lists, vertex indices, and may also contain color lists and
    // their indices.

    IFSG_FACESET* face = new IFSG_FACESET( *shape );

    IFSG_COORDS* cp = new IFSG_COORDS( *face );
    cp->AddCoord( vert[0] );
    cp->AddCoord( vert[3] );
    cp->AddCoord( vert[1] );

    // coordinate indices - note: enforce triangles;
    // in real plugins where it is not necessarily possible
    // to determine which side a triangle is visible from,
    // 2 point orders must be specified for each triangle
    IFSG_COORDINDEX* coordIdx = new IFSG_COORDINDEX( *face );
    coordIdx->AddIndex( 0 );
    coordIdx->AddIndex( 1 );
    coordIdx->AddIndex( 2 );

    // create an appearance; appearances are owned by shapes
    // magenta
    IFSG_APPEARANCE* material = new IFSG_APPEARANCE( *shape);
    material->SetSpecular( 1.0, 0.0, 1.0 );
    material->SetDiffuse( 0.9, 0.0, 0.9 );
    material->SetAmbient( 0.9 );
    material->SetShininess( 0.3 );

    // normals
    IFSG_NORMALS* np = new IFSG_NORMALS( *face );
    SGVECTOR nval = S3D::CalcTriNorm( vert[0], vert[3], vert[1] );
    np->AddNormal( nval );
    np->AddNormal( nval );
    np->AddNormal( nval );

    //
    // Shape2
    // Note: we reuse the IFSG* wrappers to create and manipulate new
    // data structures.
    //
    shape->NewNode( *tx1 );
    face->NewNode( *shape );
    coordIdx->NewNode( *face );
    cp->NewNode( *face );
    np->NewNode( *face );
    // vertices
    cp->AddCoord( vert[0] );
    cp->AddCoord( vert[2] );
    cp->AddCoord( vert[3] );
    // indices
    coordIdx->AddIndex( 0 );
    coordIdx->AddIndex( 1 );
    coordIdx->AddIndex( 2 );
    // normals
    nval = S3D::CalcTriNorm( vert[0], vert[2], vert[3] );
    np->AddNormal( nval );
    np->AddNormal( nval );
    np->AddNormal( nval );
    // color (red)
    material->NewNode( *shape );
    material->SetSpecular( 1.0, 0.0, 0.0 );
    material->SetDiffuse( 0.9, 0.0, 0.0 );
    material->SetAmbient( 0.9 );
    material->SetShininess( 0.3 );

    //
    // Shape3
    //
    shape->NewNode( *tx1 );
    face->NewNode( *shape );
    coordIdx->NewNode( *face );
    cp->NewNode( *face );
    np->NewNode( *face );
    // vertices
    cp->AddCoord( vert[1] );
    cp->AddCoord( vert[3] );
    cp->AddCoord( vert[2] );
    // indices
    coordIdx->AddIndex( 0 );
    coordIdx->AddIndex( 1 );
    coordIdx->AddIndex( 2 );
    // normals
    nval = S3D::CalcTriNorm( vert[1], vert[3], vert[2] );
    np->AddNormal( nval );
    np->AddNormal( nval );
    np->AddNormal( nval );
    // color (green)
    material->NewNode( *shape );
    material->SetSpecular( 0.0, 1.0, 0.0 );
    material->SetDiffuse( 0.0, 0.9, 0.0 );
    material->SetAmbient( 0.9 );
    material->SetShininess( 0.3 );

    //
    // Shape4
    //
    shape->NewNode( *tx1 );
    face->NewNode( *shape );
    coordIdx->NewNode( *face );
    cp->NewNode( *face );
    np->NewNode( *face );
    // vertices
    cp->AddCoord( vert[0] );
    cp->AddCoord( vert[1] );
    cp->AddCoord( vert[2] );
    // indices
    coordIdx->AddIndex( 0 );
    coordIdx->AddIndex( 1 );
    coordIdx->AddIndex( 2 );
    // normals
    nval = S3D::CalcTriNorm( vert[0], vert[1], vert[2] );
    np->AddNormal( nval );
    np->AddNormal( nval );
    np->AddNormal( nval );
    // color (blue)
    material->NewNode( *shape );
    material->SetSpecular( 0.0, 0.0, 1.0 );
    material->SetDiffuse( 0.0, 0.0, 0.9 );
    material->SetAmbient( 0.9 );
    material->SetShininess( 0.3 );

    // create a copy of the entire tetrahedron shifted Z+2 and rotated 2/3PI
    IFSG_TRANSFORM* tx2 = new IFSG_TRANSFORM( tx0->GetRawPtr() );
    tx2->AddRefNode( *tx1 );
    tx2->SetTranslation( SGPOINT( 0, 0, 2 ) );
    tx2->SetRotation( SGVECTOR( 0, 0, 1 ), M_PI*2.0/3.0 );

    SGNODE* data = tx0->GetRawPtr();

    // delete the wrappers
    delete shape;
    delete face;
    delete coordIdx;
    delete material;
    delete cp;
    delete np;
    delete tx0;
    delete tx1;
    delete tx2;

    return (SCENEGRAPH*)data;
}
