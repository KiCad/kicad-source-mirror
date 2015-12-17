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
#include <cmath>
#include <string>
#include <map>
#include <wx/string.h>
#include "plugins/3d/3d_plugin.h"
#include "plugins/3dapi/ifsg_all.h"
#include "idf_parser.h"
#include "vrml_layer.h"

#define PLUGIN_3D_IDF_MAJOR 1
#define PLUGIN_3D_IDF_MINOR 0
#define PLUGIN_3D_IDF_PATCH 0
#define PLUGIN_3D_IDF_REVNO 0


static SCENEGRAPH* loadIDFOutline( const wxString& aFileName );
static SCENEGRAPH* loadIDFBoard( const wxString& aFileName );


static SGNODE* getColor( IFSG_SHAPE& shape )
{
    IFSG_APPEARANCE material( shape );

    static int idx = 0;

    switch( idx )
    {
    case 0:
        // magenta
        material.SetSpecular( 0.8, 0.0, 0.8 );
        material.SetDiffuse( 0.6, 0.0, 0.6 );
        material.SetAmbient( 0.9 );
        material.SetShininess( 0.3 );

        ++idx;
        break;

    case 1:
        // red
        material.SetSpecular( 0.69, 0.14, 0.14 );
        material.SetDiffuse( 0.69, 0.14, 0.14 );
        material.SetAmbient( 0.9 );
        material.SetShininess( 0.3 );

        ++idx;
        break;

    case 2:
        // orange
        material.SetSpecular( 1.0, 0.44, 0.0 );
        material.SetDiffuse( 1.0, 0.44, 0.0 );
        material.SetAmbient( 0.9 );
        material.SetShininess( 0.3 );

        ++idx;
        break;

    case 3:
        // yellow
        material.SetSpecular( 0.93, 0.94, 0.16 );
        material.SetDiffuse( 0.93, 0.94, 0.16 );
        material.SetAmbient( 0.9 );
        material.SetShininess( 0.3 );

        ++idx;
        break;

    case 4:
        // green
        material.SetSpecular( 0.13, 0.81, 0.22 );
        material.SetDiffuse( 0.13, 0.81, 0.22 );
        material.SetAmbient( 0.9 );
        material.SetShininess( 0.3 );

        ++idx;
        break;

    case 5:
        // blue
        material.SetSpecular( 0.1, 0.11, 0.88 );
        material.SetDiffuse( 0.1, 0.11, 0.88 );
        material.SetAmbient( 0.9 );
        material.SetShininess( 0.3 );

        ++idx;
        break;

    default:
        // violet
        material.SetSpecular( 0.32, 0.07, 0.64 );
        material.SetDiffuse( 0.32, 0.07, 0.64 );
        material.SetAmbient( 0.9 );
        material.SetShininess( 0.3 );

        idx = 0;
        break;
    }

    return material.GetRawPtr();
};


const char* GetKicadPluginName( void )
{
    return "PLUGIN_3D_IDF";
}


void GetPluginVersion( unsigned char* Major,
    unsigned char* Minor, unsigned char* Patch, unsigned char* Revision )
{
    if( Major )
        *Major = PLUGIN_3D_IDF_MAJOR;

    if( Minor )
        *Minor = PLUGIN_3D_IDF_MINOR;

    if( Patch )
        *Patch = PLUGIN_3D_IDF_PATCH;

    if( Revision )
        *Revision = PLUGIN_3D_IDF_REVNO;

    return;
}

// number of extensions supported
#ifdef _WIN32
    #define NEXTS 2
#else
    #define NEXTS 4
#endif

// number of filter sets supported
#define NFILS 2

static char ext0[] = "idf";
static char ext1[] = "emn";

#ifdef _WIN32
    static char fil0[] = "IDF (*.idf)|*.idf";
    static char fil0[] = "IDF BRD v2/v3 (*.emn)|*.emn";
#else
    static char ext2[] = "IDF";
    static char ext3[] = "EMN";
    static char fil0[] = "IDF (*.idf;*.IDF)|*.idf;*.IDF";
    static char fil1[] = "IDF BRD (*.emn;*.EMN)|*.emn;*.EMN";
#endif

static struct FILE_DATA
{
    char const* extensions[NEXTS];
    char const* filters[NFILS];

    FILE_DATA()
    {
        extensions[0] = ext0;
        extensions[1] = ext1;
        filters[0] = fil0;
        filters[1] = fil1;

#ifndef _WIN32
        extensions[2] = ext2;
        extensions[3] = ext3;
#endif

        return;
    }

} file_data;


static bool getOutlineModel( VRML_LAYER& model, const std::list< IDF_OUTLINE* >* items );
static bool addSegment( VRML_LAYER& model, IDF_SEGMENT* seg, int icont, int iseg );


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
    // this plugin supports rendering of IDF component outlines
    return true;
}


SCENEGRAPH* Load( char const* aFileName )
{
    if( NULL == aFileName )
        return NULL;

    wxFileName fname;
    fname.Assign( wxString::FromUTF8Unchecked( aFileName ) );

    wxString ext = fname.GetExt();

    if( !ext.Cmp( wxT( "idf" ) ) || !ext.Cmp( wxT( "IDF" ) ) )
    {
        return loadIDFOutline( fname.GetFullPath() );
    }

    if( !ext.Cmp( wxT( "emn" ) ) || !ext.Cmp( wxT( "EMN" ) ) )
    {
        return loadIDFBoard( fname.GetFullPath() );
    }

    return NULL;
}


static bool getOutlineModel( VRML_LAYER& model, const std::list< IDF_OUTLINE* >* items )
{
    // empty outlines are not unusual so we fail quietly
    if( items->size() < 1 )
        return false;

    int nvcont = 0;
    int iseg   = 0;

    std::list< IDF_OUTLINE* >::const_iterator scont = items->begin();
    std::list< IDF_OUTLINE* >::const_iterator econt = items->end();
    std::list<IDF_SEGMENT*>::iterator sseg;
    std::list<IDF_SEGMENT*>::iterator eseg;

    IDF_SEGMENT lseg;

    while( scont != econt )
    {
        nvcont = model.NewContour();

        if( nvcont < 0 )
        {
            #ifdef DEBUG
                std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                std::cerr << " * [INFO] cannot create an outline\n";
            #endif

            return false;
        }

        if( (*scont)->size() < 1 )
        {
            #ifdef DEBUG
                std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                std::cerr << " * [INFO] invalid contour: no vertices\n";
            #endif

            return false;
        }

        sseg = (*scont)->begin();
        eseg = (*scont)->end();

        iseg = 0;
        while( sseg != eseg )
        {
            lseg = **sseg;

            if( !addSegment( model, &lseg, nvcont, iseg ) )
            {
                #ifdef DEBUG
                    std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                    std::cerr << " * [BUG] cannot add segment\n";
                #endif

                return false;
            }

            ++iseg;
            ++sseg;
        }

        ++scont;
    }

    return true;
}


static bool addSegment( VRML_LAYER& model, IDF_SEGMENT* seg, int icont, int iseg )
{
    // note: in all cases we must add all but the last point in the segment
    // to avoid redundant points

    if( seg->angle != 0.0 )
    {
        if( seg->IsCircle() )
        {
            if( iseg != 0 )
            {
                #ifdef DEBUG
                    std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                    std::cerr << " * [INFO] adding a circle to an existing vertex list\n";
                #endif

                return false;
            }

            return model.AppendCircle( seg->center.x, seg->center.y, seg->radius, icont );
        }
        else
        {
            return model.AppendArc( seg->center.x, seg->center.y, seg->radius,
                                    seg->offsetAngle, seg->angle, icont );
        }
    }

    if( !model.AddVertex( icont, seg->startPoint.x, seg->startPoint.y ) )
        return false;

    return true;
}


static SCENEGRAPH* loadIDFOutline( const wxString& aFileName )
{
    // load and render the file
    IDF3_BOARD brd( IDF3::CAD_ELEC );
    SCENEGRAPH* data = NULL;

    try
    {
        IDF3_COMP_OUTLINE* outline =
            brd.GetComponentOutline( aFileName );

        if( NULL == outline )
        {
            #ifdef DEBUG
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] no outline for file '";
            std::cerr << aFileName << "'\n";
            #endif
            return NULL;
        }

        VRML_LAYER vpcb;

        if( !getOutlineModel( vpcb, outline->GetOutlines() ) )
        {
            #ifdef DEBUG
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] no valid outline data in '";
            std::cerr << aFileName << "'\n";
            #endif
            return NULL;
        }

        vpcb.Tesselate( NULL );
        std::vector< double > vertices;
        std::vector< int > idxPlane;
        std::vector< int > idxSide;
        double thick = outline->GetThickness();

        if( !vpcb.Get3DTriangles( vertices, idxPlane, idxSide, thick, 0.0 ) )
        {
            #ifdef DEBUG
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] no vertex data in '";
            std::cerr << aFileName << "'\n";
            #endif
            return NULL;
        }

        if( ( idxPlane.size() % 3 ) || ( idxSide.size() % 3 ) )
        {
            #ifdef DEBUG
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] index lists are not a multiple of 3 (not a triangle list)\n";
            #endif
            return NULL;
        }

        std::vector< SGPOINT > vlist;
        size_t nvert = vertices.size() / 3;
        size_t j = 0;

        for( size_t i = 0; i < nvert; ++i, j+= 3 )
            vlist.push_back( SGPOINT( vertices[j], vertices[j+1], vertices[j+2] ) );

        // create the intermediate scenegraph
        IFSG_TRANSFORM* tx0 = new IFSG_TRANSFORM( true );               // tx0 = top level Transform
        IFSG_SHAPE* shape = new IFSG_SHAPE( *tx0 );                     // shape will hold (a) all vertices and (b) a local list of normals
        IFSG_FACESET* face = new IFSG_FACESET( *shape );                // this face shall represent the top and bottom planes
        IFSG_COORDS* cp = new IFSG_COORDS( *face );                     // coordinates for all faces
        cp->SetCoordsList( nvert, &vlist[0] );
        IFSG_COORDINDEX* coordIdx = new IFSG_COORDINDEX( *face );       // coordinate indices for top and bottom planes only
        coordIdx->SetIndices( idxPlane.size(), &idxPlane[0] );
        IFSG_NORMALS* norms = new IFSG_NORMALS( *face );                // normals for the top and bottom planes

        // number of TOP (and bottom) vertices
        j = nvert / 2;

        // set the TOP normals
        for( size_t i = 0; i < j; ++i )
            norms->AddNormal( 0.0, 0.0, 1.0 );

        // set the BOTTOM normals
        for( size_t i = 0; i < j; ++i )
            norms->AddNormal( 0.0, 0.0, -1.0 );

        // assign a color from the rotating palette
        SGNODE* modelColor = getColor( *shape );

        // create a second shape describing the vertical walls of the IDF extrusion
        // using per-vertex-per-face-normals
        shape->NewNode( *tx0 );
        shape->AddRefNode( modelColor );    // set the color to be the same as the top/bottom
        face->NewNode( *shape );
        cp->NewNode( *face );               // new vertex list
        norms->NewNode( *face );            // new normals list
        coordIdx->NewNode( *face );         // new index list

        // populate the new per-face vertex list and its indices and normals
        std::vector< int >::iterator sI = idxSide.begin();
        std::vector< int >::iterator eI = idxSide.end();

        size_t sidx = 0;    // index to the new coord set
        SGPOINT p1, p2, p3;
        SGVECTOR vnorm;

        while( sI != eI )
        {
            p1 = vlist[*sI];
            cp->AddCoord( p1 );
            ++sI;

            p2 = vlist[*sI];
            cp->AddCoord( p2 );
            ++sI;

            p3 = vlist[*sI];
            cp->AddCoord( p3 );
            ++sI;

            vnorm.SetVector( S3D::CalcTriNorm( p1, p2, p3 ) );
            norms->AddNormal( vnorm );
            norms->AddNormal( vnorm );
            norms->AddNormal( vnorm );

            coordIdx->AddIndex( (int)sidx );
            ++sidx;
            coordIdx->AddIndex( (int)sidx );
            ++sidx;
            coordIdx->AddIndex( (int)sidx );
            ++sidx;
        }

        data = (SCENEGRAPH*)tx0->GetRawPtr();

        // delete the API wrappers
        delete shape;
        delete face;
        delete coordIdx;
        delete cp;
        delete tx0;
    }
    catch( const IDF_ERROR& e )
    {
        std::cerr << e.what() << "\n";
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] cannot load the outline '" << aFileName.ToUTF8() << "'\n";
        return NULL;
    }
    catch( ... )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] cannot load the outline '" << aFileName.ToUTF8() << "'\n";
        return NULL;
    }

    // DEBUG: WRITE OUT IDF FILE TO CONFIRM NORMALS
    #ifdef DEBUG
    wxFileName fn( aFileName );
    wxString output = fn.GetName();
    output.append( wxT(".wrl") );
    S3D::WriteVRML( output, true, (SGNODE*)(data), true, true );
    #endif

    return data;
}


static SCENEGRAPH* loadIDFBoard( const wxString& aFileName )
{
    // XXX - TO BE IMPLEMENTED
    return NULL;
}
