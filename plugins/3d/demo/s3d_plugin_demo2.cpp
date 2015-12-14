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
    // For this demonstration we create a tetrahedron and
    // paint its faces Magenta Red Green Blue. Steps:
    // * Create a top level transform tx0 which represent the VRML file
    // * Create a child transform tx1, parent tx0, to define the tetrahedron
    //    + Create 'shape' to define one facet
    //    ++ Create a 'face' to hold vertices and indices
    //    +++ Create 'cp' which is the coordinate list
    //    +++ Create 'np' which is the per-vertex normals list
    //    +++ Create 'coordIndex' which is the (triangular) vertex index list
    //        for facet1 of the tetrahedron
    //    ++ Create a 'material' to define the appearance of 'shape'
    //    **
    //    + shape->NewNode() to define next facet
    //    ++ face->NewNode() for a new facet
    //    +++ Add Ref to 'cp' for coordinate list
    //    +++ Add Ref to 'np' for normals list
    //    +++ coordIndex->NewNode() for vertex index list of new facet
    //    ++ material->NewNode() for material of new facet
    //    + repeat twice from ** to produce last 2 facets
    // * Create a child transform tx2, parent tx0, for a referenced tetrahedron
    //    + Set a translation and rotation so that this is distinct from tx1
    //    + Add Reference to tx1
    // ALL DONE: we now have:
    // tx0
    //  - contains tx1 which contains all elements of a tetrahedron
    //  - contains tx0 which contains a reference to tx1 and offsets it so
    //    that it renders in a different position

    // create the top level transform; this will hold all other
    // scenegraph objects; a transform may hold other transforms and
    // shapes
    IFSG_TRANSFORM* tx0 = new IFSG_TRANSFORM( true );

    // create the transform which will house the shapes
    IFSG_TRANSFORM* tx1 = new IFSG_TRANSFORM( tx0->GetRawPtr() );

    // add a shape which we will use to define a tetrahedron; shapes
    // hold facesets and appearances
    IFSG_SHAPE* shape = new IFSG_SHAPE( *tx1 );

    // add a faceset; these contain coordinate lists, coordinate indices,
    // vertex lists, vertex indices, and may also contain color lists and
    // their indices.

    IFSG_FACESET* face = new IFSG_FACESET( *shape );

    // define the vertices of the tetrahedron
    double SQ2 = sqrt( 0.5 );
    SGPOINT vert[4];
    vert[0] = SGPOINT( 1.0, 0.0, -SQ2 );
    vert[1] = SGPOINT( -1.0, 0.0, -SQ2 );
    vert[2] = SGPOINT( 0.0, 1.0, SQ2 );
    vert[3] = SGPOINT( 0.0, -1.0, SQ2 );
    IFSG_COORDS* cp = new IFSG_COORDS( *face );
    cp->SetCoordsList( 4, vert );
    // coordinate indices - note: enforce triangles;
    // in real plugins where it is not necessarily possible
    // to determine which side a triangle is visible from,
    // 2 point orders must be specified for each triangle
    IFSG_COORDINDEX* coordIdx = new IFSG_COORDINDEX( *face );
    int cidx[12] = { 0, 3, 1, 0, 2, 3, 1, 3, 2, 0, 1, 2 };
    coordIdx->SetIndices( 3, cidx );

    // the vertex normals in this case are the normalized
    // vertex points
    SGVECTOR norm[4];
    norm[0] = SGVECTOR( -1.0, 0.0, -SQ2 );
    norm[1] = SGVECTOR( 1.0, 0.0, -SQ2 );
    norm[2] = SGVECTOR( 0.0, -1.0, SQ2 );
    norm[3] = SGVECTOR( 0.0, 1.0, SQ2 );
    IFSG_NORMALS* np = new IFSG_NORMALS( *face );
    np->SetNormalList( 4, norm );

    // create an appearance; appearances are owned by shapes
    // magenta
    IFSG_APPEARANCE* material = new IFSG_APPEARANCE( *shape);
    material->SetSpecular( 1.0, 0.0, 1.0 );
    material->SetDiffuse( 0.9, 0.0, 0.9 );
    material->SetAmbient( 0.9 );
    material->SetShininess( 0.3 );

    // Shape2
    shape->NewNode( *tx1 );
    face->NewNode( *shape );
    face->AddRefNode( *cp );
    face->AddRefNode( *np );
    coordIdx->NewNode( *face );
    coordIdx->SetIndices( 3, &cidx[3] );
    // red
    material->NewNode( *shape );
    material->SetSpecular( 1.0, 0.0, 0.0 );
    material->SetDiffuse( 0.9, 0.0, 0.0 );
    material->SetAmbient( 0.9 );
    material->SetShininess( 0.3 );

    // Shape3
    shape->NewNode( *tx1 );
    face->NewNode( *shape );
    face->AddRefNode( *cp );
    face->AddRefNode( *np );
    coordIdx->NewNode( *face );
    coordIdx->SetIndices( 3, &cidx[6] );
    // green
    material->NewNode( *shape );
    material->SetSpecular( 0.0, 1.0, 0.0 );
    material->SetDiffuse( 0.0, 0.9, 0.0 );
    material->SetAmbient( 0.9 );
    material->SetShininess( 0.3 );

    // Shape4
    shape->NewNode( *tx1 );
    face->NewNode( *shape );
    face->AddRefNode( *cp );
    face->AddRefNode( *np );
    coordIdx->NewNode( *face );
    coordIdx->SetIndices( 3, &cidx[9] );
    // blue
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
