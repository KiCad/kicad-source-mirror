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
#include "s3d_plugin_idf.h"
#include "plugins/3dapi/ifsg_all.h"
#include "idf_parser.h"
#include "vrml_layer.h"


static S3D_PLUGIN_IDF idf_plugin;

#ifndef _WIN32
extern "C" __attribute__((__visibility__("default"))) S3D_PLUGIN* Get3DPlugin( void )
{
    return &idf_plugin;
}
#else
extern "C" __declspec( dllexport ) S3D_PLUGIN* Get3DPlugin( void )
    {
        return &idf_plugin;
    }
#endif


static bool PopulateVRML( VRML_LAYER& model, const std::list< IDF_OUTLINE* >* items );
static bool AddSegment( VRML_LAYER& model, IDF_SEGMENT* seg, int icont, int iseg );


S3D_PLUGIN_IDF::S3D_PLUGIN_IDF()
{
    m_extensions.push_back( wxString::FromUTF8Unchecked( "idf" ) );

#ifdef _WIN32
    // assume a case-insensitive file system
    m_filters.push_back( wxT( "IDF 2.0/3.0 (*.idf)|*.idf" ) );
#else
    // assume the filesystem is case sensitive
    m_extensions.push_back( wxString::FromUTF8Unchecked( "IDF" ) );

    m_filters.push_back( wxT( "IDF 2.0/3.0 (*.idf;*.IDF)|*.idf;*.IDF" ) );
#endif


    return;
}


S3D_PLUGIN_IDF::~S3D_PLUGIN_IDF()
{
    return;
}


int S3D_PLUGIN_IDF::GetNExtensions( void ) const
{
    return (int) m_extensions.size();
}


const wxString S3D_PLUGIN_IDF::GetModelExtension( int aIndex ) const
{
    if( aIndex < 0 || aIndex >= (int) m_extensions.size() )
        return wxString( "" );

    return m_extensions[aIndex];
}


int S3D_PLUGIN_IDF::GetNFilters( void ) const
{
    return (int)m_filters.size();
}


const wxString S3D_PLUGIN_IDF::GetFileFilter( int aIndex ) const
{
    if( aIndex < 0 || aIndex >= (int)m_filters.size() )
        return wxEmptyString;

    return m_filters[aIndex];
}


bool S3D_PLUGIN_IDF::CanRender( void ) const
{
    // this plugin supports rendering of IDF component outlines
    return true;
}


SCENEGRAPH* S3D_PLUGIN_IDF::Load( const wxString& aFileName )
{
    // load and render the file
    IDF3_BOARD brd( IDF3::CAD_ELEC );
    IDF3_COMP_OUTLINE* outline = brd.GetComponentOutline( aFileName );

    if( NULL == outline )
    {
        #ifdef DEBUG
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] no outline for file '";
            std::cerr << aFileName.ToUTF8() << "'\n";
        #endif
        return NULL;
    }

    VRML_LAYER vpcb;

    if( !PopulateVRML( vpcb, outline->GetOutlines() ) )
    {
        #ifdef DEBUG
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] no valid outline data in '";
            std::cerr << aFileName.ToUTF8() << "'\n";
        #endif
        return NULL;
    }

    vpcb.Tesselate( NULL );
    std::vector< double > vertices;
    std::vector< int > indices;
    double thick = outline->GetThickness();

    if( !vpcb.Get3DTriangles( vertices, indices, thick, 0.0 ) )
    {
        #ifdef DEBUG
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] no vertex data in '";
            std::cerr << aFileName.ToUTF8() << "'\n";
        #endif
        return NULL;
    }

    std::cerr << "XXX - Got " << vertices.size() / 3 << " vertices and " << indices.size() << " indices\n";
    std::vector< SGPOINT > vlist;
    size_t nvert = vertices.size() / 3;
    size_t j = 0;

    for( size_t i = 0; i < nvert; ++i, j+= 3 )
        vlist.push_back( SGPOINT( vertices[j], vertices[j+1], vertices[j+2] ) );

    // create the intermediate scenegraph
    IFSG_TRANSFORM* tx0 = new IFSG_TRANSFORM( true );
    IFSG_SHAPE* shape = new IFSG_SHAPE( *tx0 );
    IFSG_FACESET* face = new IFSG_FACESET( *shape );
    IFSG_COORDS* cp = new IFSG_COORDS( *face );
    cp->SetCoordsList( nvert, &vlist[0] );
    IFSG_COORDINDEX* coordIdx = new IFSG_COORDINDEX( *face );
    coordIdx->SetIndices( indices.size(), &indices[0] );

    // XXX - TO BE IMPLEMENTED : add correct normals and colors
    std::vector< SGVECTOR > norms;

    for( size_t i = 0; i < nvert; ++i )
        norms.push_back( SGVECTOR( 0.0, 0.0, 1.0 ) );

    IFSG_NORMALS* np = new IFSG_NORMALS( *face );
    np->SetNormalList( nvert, &norms[0] );

    // magenta
    IFSG_APPEARANCE* material = new IFSG_APPEARANCE( *shape);
    material->SetSpecular( 1.0, 0.0, 1.0 );
    material->SetDiffuse( 0.9, 0.0, 0.9 );
    material->SetAmbient( 0.9 );
    material->SetShininess( 0.3 );

    SCENEGRAPH* data = (SCENEGRAPH*)tx0->GetRawPtr();

    // delete the API wrappers
    delete shape;
    delete face;
    delete coordIdx;
    delete material;
    delete cp;
    delete np;
    delete tx0;

    return data;
}


static bool PopulateVRML( VRML_LAYER& model, const std::list< IDF_OUTLINE* >* items )
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

            if( !AddSegment( model, &lseg, nvcont, iseg ) )
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


static bool AddSegment( VRML_LAYER& model, IDF_SEGMENT* seg, int icont, int iseg )
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
