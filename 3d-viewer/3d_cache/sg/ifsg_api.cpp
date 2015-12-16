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
#include <fstream>
#include <string>
#include <locale.h>
#include <wx/filename.h>
#include <wx/string.h>
#include "plugins/3dapi/ifsg_api.h"
#include "plugins/3dapi/sg_types.h"
#include "3d_cache/sg/sg_node.h"
#include "3d_cache/sg/scenegraph.h"
#include "3d_cache/sg/sg_appearance.h"
#include "3d_cache/sg/sg_colorindex.h"
#include "3d_cache/sg/sg_colors.h"
#include "3d_cache/sg/sg_coordindex.h"
#include "3d_cache/sg/sg_coords.h"
#include "3d_cache/sg/sg_faceset.h"
#include "3d_cache/sg/sg_normals.h"
#include "3d_cache/sg/sg_shape.h"
#include "3d_cache/sg/sg_version.h"
#include "3d_info.h"
#include "plugins/3dapi/c3dmodel.h"


static char BadNode[] = " * [BUG] NULL pointer passed for aNode\n";

static void formatMaterial( SMATERIAL& mat, SGAPPEARANCE const* app )
{
    float v0, v1, v2;

    v0 = app->ambient;
    mat.m_Ambient.x = v0;
    mat.m_Ambient.y = v0;
    mat.m_Ambient.z = v0;

    app->diffuse.GetColor( v0, v1, v2 );
    mat.m_Diffuse.x = v0;
    mat.m_Diffuse.y = v1;
    mat.m_Diffuse.z = v2;

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
    VRML_LOCALE()
    {
        lname = setlocale( LC_NUMERIC, NULL );
        setlocale( LC_NUMERIC, "C" );   // switch the numerics locale to "C"
    }

    ~VRML_LOCALE()
    {
        setlocale( LC_NUMERIC, lname.c_str() ); // revert to the previous locale
    }
};


bool S3D::WriteVRML( const wxString& filename, bool overwrite, SGNODE* aTopNode,
    bool reuse, bool renameNodes )
{
    if( wxFileName::Exists( filename ) )
    {
        if( !overwrite )
            return false;

        // make sure we make no attempt to write a directory
        if( !wxFileName::FileExists( filename ) )
            return false;
    }

    if( NULL == aTopNode )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] NULL pointer passed for aTopNode\n";
        return false;
    }

    if( S3D::SGTYPE_TRANSFORM != aTopNode->GetNodeType() )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] aTopNode is not a SCENEGRAPH object\n";
        return false;
    }

    VRML_LOCALE vrmlLocale;
    std::ofstream op;
    op.open( filename.ToUTF8(), std::ios_base::out | std::ios_base::trunc
                                 | std::ios_base::binary );

    if( !op.is_open() )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] failed to open file '" << filename.ToUTF8() << "'\n";
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
    std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    std::cerr << " * [INFO] problems encountered writing file '" << filename.ToUTF8() << "'\n";
    return false;
}


void S3D::ResetNodeIndex( SGNODE* aNode )
{
    if( NULL == aNode )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << BadNode;
        return;
    }

    aNode->ResetNodeIndex();

    return;
}


void S3D::RenameNodes( SGNODE* aNode )
{
    if( NULL == aNode )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << BadNode;
        return;
    }

    aNode->ReNameNodes();

    return;
}


void S3D::DestroyNode( SGNODE* aNode )
{
    if( NULL == aNode )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << BadNode;
        return;
    }

    delete aNode;

    return;
}


bool S3D::WriteCache( const wxString& aFileName, bool overwrite, SGNODE* aNode )
{
    if( NULL == aNode )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << BadNode;
        return false;
    }


    if( wxFileName::Exists( aFileName ) )
    {
        if( !overwrite )
        {
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] file exists; not overwriting: '";
            std::cerr << aFileName.ToUTF8() << "'\n";
            return false;
        }

        // make sure we make no attempt to write a directory
        if( !wxFileName::FileExists( aFileName ) )
        {
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] specified path is a directory: '";
            std::cerr << aFileName.ToUTF8() << "'\n";
            return false;
        }
    }

    std::ofstream output;
    output.open( aFileName.ToUTF8(), std::ios_base::out | std::ios_base::trunc
                                | std::ios_base::binary );

    if( !output.is_open() )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] failed to open file '" << aFileName.ToUTF8() << "'\n";
        return false;
    }

    bool rval = aNode->WriteCache( output, NULL );
    output.close();

    if( !rval )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] problems encountered writing cache file '";
        std::cerr << aFileName.ToUTF8() << "'\n";
    }

    return rval;
}


SGNODE* S3D::ReadCache( const wxString& aFileName )
{
    if( !wxFileName::FileExists( aFileName ) )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] no such file: '";
        std::cerr << aFileName.ToUTF8() << "'\n";
    }

    SGNODE* np = new SCENEGRAPH( NULL );

    if( NULL == np )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] failed to instantiate SCENEGRAPH\n";
        return NULL;
    }

    std::ifstream file;
    file.open( aFileName.ToUTF8(), std::ios_base::in | std::ios_base::binary );

    if( !file.is_open() )
    {
        delete np;
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] failed to open file '";
        std::cerr << aFileName.ToUTF8() << "'\n";
        return NULL;
    }

    bool rval = np->ReadCache( file, NULL );
    file.close();

    if( !rval )
    {
        delete np;
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] problems encountered reading cache file '";
        std::cerr << aFileName.ToUTF8() << "'\n";
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
    app.ambient = 0.9;
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

        for( int i = 0; i < j; ++i )
            formatMaterial( lmat[i], materials.matorder[i] );

        model->m_Materials = lmat;
        model->m_MaterialsSize = j;

        // add all the meshes
        j = meshes.size();
        SMESH* lmesh = new SMESH[j];

        for( int i = 0; i < j; ++i )
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
        *Major = SG_VERSION_MAJOR;

    if( Minor )
        *Minor = SG_VERSION_MINOR;

    if( Revision )
        *Revision = SG_VERSION_REVNO;

    if( Patch )
        *Patch = SG_VERSION_PATCH;

    return;
}
