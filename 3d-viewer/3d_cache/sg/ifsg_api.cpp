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
#include <fstream>
#include <wx/filename.h>
#include <wx/log.h>
#include "plugins/3dapi/ifsg_api.h"
#include "plugins/3dapi/sg_version.h"
#include "3d_cache/sg/sg_node.h"
#include "3d_cache/sg/scenegraph.h"
#include "3d_cache/sg/sg_appearance.h"
#include "3d_cache/sg/sg_shape.h"
#include "3d_cache/sg/sg_helpers.h"


#ifdef DEBUG
static char BadNode[] = " * [BUG] NULL pointer passed for aNode\n";
#endif

// version format of the cache file
#define SG_VERSION_TAG "VERSION:2"


static void formatMaterial( SMATERIAL& mat, SGAPPEARANCE const* app )
{
    float v0, v1, v2;

    app->ambient.GetColor( v0, v1, v2 );
    mat.m_Ambient.x = v0;
    mat.m_Ambient.y = v1;
    mat.m_Ambient.z = v2;

    app->diffuse.GetColor( v0, v1, v2 );
    mat.m_Diffuse.x = v0;
    mat.m_Diffuse.y = v1;
    mat.m_Diffuse.z = v2;
    mat.m_Ambient.x *= v0;
    mat.m_Ambient.y *= v1;
    mat.m_Ambient.z *= v2;

    app->emissive.GetColor( v0, v1, v2 );
    mat.m_Emissive.x = v0;
    mat.m_Emissive.y = v1;
    mat.m_Emissive.z = v2;

    app->specular.GetColor( v0, v1, v2 );
    mat.m_Specular.x = v0;
    mat.m_Specular.y = v1;
    mat.m_Specular.z = v2;

    mat.m_Shininess = app->shininess;
    mat.m_Transparency = app->transparency;

    return;
}


class VRML_LOCALE
{
private:
    std::string lname;

public:
    VRML_LOCALE() : lname( setlocale( LC_NUMERIC, NULL ) )
    {
        setlocale( LC_NUMERIC, "C" );   // switch the numerics locale to "C"
    }

    ~VRML_LOCALE()
    {
        setlocale( LC_NUMERIC, lname.c_str() ); // revert to the previous locale
    }
};


bool S3D::WriteVRML( const char* filename, bool overwrite, SGNODE* aTopNode,
    bool reuse, bool renameNodes )
{
    if( NULL == filename || filename[0] == 0 )
        return false;

    wxString ofile = wxString::FromUTF8Unchecked( filename );

    if( wxFileName::Exists( ofile ) )
    {
        if( !overwrite )
            return false;

        // make sure we make no attempt to write a directory
        if( !wxFileName::FileExists( ofile ) )
            return false;
    }

    if( NULL == aTopNode )
    {
        #ifdef DEBUG
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [BUG] NULL pointer passed for aTopNode";
            wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return false;
    }

    if( S3D::SGTYPE_TRANSFORM != aTopNode->GetNodeType() )
    {
        #ifdef DEBUG
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [BUG] aTopNode is not a SCENEGRAPH object";
            wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return false;
    }

    VRML_LOCALE vrmlLocale;
    std::ofstream op;
    op.open( filename, std::ios_base::out | std::ios_base::trunc
                                 | std::ios_base::binary );

    if( !op.is_open() )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = _( "failed to open file" );
        ostr << " * [INFO] " << errmsg.ToUTF8() << " '" << filename << "'";
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        return false;
    }

    op << "#VRML V2.0 utf8\n";

    if( renameNodes )
    {
        aTopNode->ResetNodeIndex();
        aTopNode->ReNameNodes();
    }

    aTopNode->WriteVRML( op, reuse );

    if( !op.fail() )
    {
        op.close();
        return true;
    }

    op.close();

    do {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = _( "problems encountered writing file" );
        ostr << " * [INFO] " << errmsg.ToUTF8() << " '" << filename << "'";
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
    } while( 0 );

    return false;
}


void S3D::ResetNodeIndex( SGNODE* aNode )
{
    if( NULL == aNode )
    {
        #ifdef DEBUG
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << BadNode;
            wxLogTrace( MASK_3D_SG, "%s", ostr.str().c_str() );
        } while( 0 );
        #endif

        return;
    }

    aNode->ResetNodeIndex();

    return;
}


void S3D::RenameNodes( SGNODE* aNode )
{
    if( NULL == aNode )
    {
        #ifdef DEBUG
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << BadNode;
            wxLogTrace( MASK_3D_SG, "%s", ostr.str().c_str() );
        } while( 0 );
        #endif

        return;
    }

    aNode->ReNameNodes();

    return;
}


void S3D::DestroyNode( SGNODE* aNode )
{
    if( NULL == aNode )
    {
        #ifdef DEBUG
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << BadNode;
            wxLogTrace( MASK_3D_SG, "%s", ostr.str().c_str() );
        } while( 0 );
        #endif

        return;
    }

    delete aNode;

    return;
}


bool S3D::WriteCache( const char* aFileName, bool overwrite, SGNODE* aNode,
    const char* aPluginInfo )
{
    if( NULL == aFileName || aFileName[0] == 0 )
        return false;

    wxString ofile = wxString::FromUTF8Unchecked( aFileName );

    if( NULL == aNode )
    {
        #ifdef DEBUG
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << BadNode;
            wxLogTrace( MASK_3D_SG, "%s", ostr.str().c_str() );
        } while( 0 );
        #endif

        return false;
    }


    if( wxFileName::Exists( ofile ) )
    {
        if( !overwrite )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            wxString errmsg = _( "file exists; not overwriting" );
            ostr << " * [INFO] " << errmsg.ToUTF8() << " '";
            ostr << aFileName << "'";
            wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );

            return false;
        }

        // make sure we make no attempt to write a directory
        if( !wxFileName::FileExists( aFileName ) )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            wxString errmsg = _( "specified path is a directory" );
            ostr << " * [INFO] " << errmsg.ToUTF8() << " '";
            ostr << aFileName << "'";
            wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
            return false;
        }
    }

    std::ofstream output;
    output.open( aFileName, std::ios_base::out | std::ios_base::trunc
                                | std::ios_base::binary );

    if( !output.is_open() )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = _( "failed to open file" );
        ostr << " * [INFO] " << errmsg.ToUTF8() << " '" << aFileName << "'";
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        return false;
    }

    output << "(" << SG_VERSION_TAG << ")";

    if( NULL != aPluginInfo && aPluginInfo[0] != 0 )
        output << "(" << aPluginInfo << ")";
    else
        output << "(INTERNAL:0.0.0.0)";

    bool rval = aNode->WriteCache( output, NULL );
    output.close();

    if( !rval )
    {
        #ifdef DEBUG
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [INFO] problems encountered writing cache file '";
            ostr << aFileName << "'";
            wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        // delete the defective file
        wxRemoveFile( ofile );
    }

    return rval;
}


SGNODE* S3D::ReadCache( const char* aFileName, void* aPluginMgr,
        bool (*aTagCheck)( const char*, void* ) )
{
    if( NULL == aFileName || aFileName[0] == 0 )
        return NULL;

    wxString ofile = wxString::FromUTF8Unchecked( aFileName );

    if( !wxFileName::FileExists( aFileName ) )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = _( "no such file" );
        ostr << " * [INFO] " << errmsg.ToUTF8() << " '";
        ostr << aFileName << "'";
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );

        return NULL;
    }

    SGNODE* np = new SCENEGRAPH( NULL );

    if( NULL == np )
    {
        #ifdef DEBUG
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [INFO] failed to instantiate SCENEGRAPH";
            wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return NULL;
    }

    std::ifstream file;
    file.open( aFileName, std::ios_base::in | std::ios_base::binary );

    if( !file.is_open() )
    {
        delete np;
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = _( "failed to open file" );
        ostr << " * [INFO] " << errmsg.ToUTF8() << " '";
        ostr << aFileName << "'";
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        return NULL;
    }

    // from SG_VERSION_TAG 1, read the version tag; if it's not the expected tag
    // then we fail to read the cache file
    do
    {
        std::string name;
        char schar;
        file.get( schar );

        if( '(' != schar )
        {
            #ifdef DEBUG
            do {
                std::ostringstream ostr;
                ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                ostr << " * [INFO] corrupt data; missing left parenthesis at position ";
                ostr << file.tellg();
                wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
            } while( 0 );
            #endif

            file.close();
            return NULL;
        }

        file.get( schar );

        while( ')' != schar && file.good() )
        {
            name.push_back( schar );
            file.get( schar );
        }

        if( name.compare( SG_VERSION_TAG ) )
        {
            file.close();
            return NULL;
        }

    } while( 0 );

    // from SG_VERSION_TAG 2, read the PluginInfo string and check that it matches
    // version tag; if it's not the expected tag then we fail to read the file
    do
    {
        std::string name;
        char schar;
        file.get( schar );

        if( '(' != schar )
        {
            #ifdef DEBUG
            do {
                std::ostringstream ostr;
                ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                ostr << " * [INFO] corrupt data; missing left parenthesis at position ";
                ostr << file.tellg();
                wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
            } while( 0 );
            #endif

            file.close();
            return NULL;
        }

        file.get( schar );

        while( ')' != schar && file.good() )
        {
            name.push_back( schar );
            file.get( schar );
        }

        // check the plugin tag
        if( NULL != aTagCheck && NULL != aPluginMgr && !aTagCheck( name.c_str(), aPluginMgr ) )
        {
            file.close();
            return NULL;
        }

    } while( 0 );

    bool rval = np->ReadCache( file, NULL );
    file.close();

    if( !rval )
    {
        delete np;
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        wxString errmsg = "problems encountered reading cache file";
        ostr << " * [INFO] " << errmsg.ToUTF8() << " '";
        ostr << aFileName << "'";
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        return NULL;
    }

    return np;
}


S3DMODEL* S3D::GetModel( SCENEGRAPH* aNode )
{
    if( NULL == aNode )
        return NULL;

    if( aNode->GetNodeType() != S3D::SGTYPE_TRANSFORM )
        return NULL;

    S3D::MATLIST materials;
    std::vector< SMESH > meshes;

    // the materials list shall have a default color; although the VRML
    // default is an opaque black, the default used here shall be a median
    // gray in hopes that it may help highlight faulty models; this color is
    // also typical of MCAD applications. When a model has no associated
    // material color it shall be assigned the index 0.
    SGAPPEARANCE app( NULL );
    app.ambient = SGCOLOR( 0.6, 0.6, 0.6 );
    app.diffuse = SGCOLOR( 0.6, 0.6, 0.6 );
    app.specular = app.diffuse;
    app.shininess = 0.05;
    app.transparency = 0.0;

    materials.matorder.push_back( &app );
    materials.matmap.insert( std::pair< SGAPPEARANCE const*, int >( &app, 0 ) );

    if( aNode->Prepare( NULL, materials, meshes ) )
    {
        if( meshes.empty() )
            return NULL;

        S3DMODEL* model = S3D::New3DModel();

        // add all the materials
        size_t j = materials.matorder.size();
        SMATERIAL* lmat = new SMATERIAL[j];

        for( size_t i = 0; i < j; ++i )
            formatMaterial( lmat[i], materials.matorder[i] );

        model->m_Materials = lmat;
        model->m_MaterialsSize = j;

        // add all the meshes
        j = meshes.size();
        SMESH* lmesh = new SMESH[j];

        for( size_t i = 0; i < j; ++i )
            lmesh[i] = meshes[i];

        model->m_Meshes = lmesh;
        model->m_MeshesSize = j;

        return model;
    }

    size_t j = meshes.size();

    for( size_t i = 0; i < j; ++i )
        S3D::Free3DMesh( meshes[i] );

    return NULL;
}


void S3D::Destroy3DModel( S3DMODEL** aModel )
{
    if( NULL == aModel || NULL == *aModel )
        return;

    S3DMODEL* m = *aModel;
    S3D::FREE_S3DMODEL( *m );
    delete m;
    *aModel = NULL;

    return;
}


void Free3DModel( S3DMODEL& aModel )
{
    S3D::FREE_S3DMODEL( aModel );
    return;
}


void S3D::Free3DMesh( SMESH& aMesh )
{
    S3D::FREE_SMESH( aMesh );
    return;
}


S3DMODEL* S3D::New3DModel( void )
{
    S3DMODEL* mp = new S3DMODEL;
    S3D::INIT_S3DMODEL( *mp );
    return mp;
}


void S3D::Init3DMaterial( SMATERIAL& aMat )
{
    S3D::INIT_SMATERIAL( aMat );
    return;
}


void S3D::Init3DMesh( SMESH& aMesh )
{
    S3D::INIT_SMESH( aMesh );

    return;
}


void S3D::GetLibVersion( unsigned char* Major, unsigned char* Minor,
    unsigned char* Patch, unsigned char* Revision )
{
    if( Major )
        *Major = KICADSG_VERSION_MAJOR;

    if( Minor )
        *Minor = KICADSG_VERSION_MINOR;

    if( Revision )
        *Revision = KICADSG_VERSION_REVISION;

    if( Patch )
        *Patch = KICADSG_VERSION_PATCH;

    return;
}


SGVECTOR S3D::CalcTriNorm( const SGPOINT& p1, const SGPOINT& p2, const SGPOINT& p3 )
{
    glm::dvec3 tri = glm::dvec3( 0.0, 0.0, 0.0 );
    glm::dvec3 pts[3];

    pts[0] = glm::dvec3( p1.x, p1.y, p1.z );
    pts[1] = glm::dvec3( p2.x, p2.y, p2.z );
    pts[2] = glm::dvec3( p3.x, p3.y, p3.z );

    // degenerate points are given a default 0, 0, 1 normal
    if( S3D::degenerate( pts ) )
        return SGVECTOR( 0.0, 0.0, 1.0 );

    // normal
    tri = glm::cross( pts[1] - pts[0], pts[2] - pts[0] );
    glm::normalize( tri );

    return SGVECTOR( tri.x, tri.y, tri.z );
}


S3D::SGTYPES S3D::GetSGNodeType( SGNODE* aNode )
{
    if( NULL == aNode )
        return SGTYPE_END;

    return aNode->GetNodeType();
}


SGNODE* S3D::GetSGNodeParent( SGNODE* aNode )
{
    if( NULL == aNode )
        return NULL;

    return aNode->GetParent();
}


bool S3D::AddSGNodeRef( SGNODE* aParent, SGNODE* aChild )
{
    if( NULL == aParent || NULL == aChild )
        return false;

    return aParent->AddRefNode( aChild );
}


bool S3D::AddSGNodeChild( SGNODE* aParent, SGNODE* aChild )
{
    if( NULL == aParent || NULL == aChild )
        return false;

    return aParent->AddChildNode( aChild );
}


void S3D::AssociateSGNodeWrapper( SGNODE* aObject, SGNODE** aRefPtr )
{
    if( NULL == aObject || NULL == aRefPtr || aObject != *aRefPtr )
        return;

    aObject->AssociateWrapper( aRefPtr );

    return;
}
