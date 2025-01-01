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

#include <iostream>
#include <sstream>
#include <fstream>
#include <memory>
#include <wx/filename.h>
#include <wx/log.h>
#include "plugins/3dapi/ifsg_api.h"
#include "plugins/3dapi/sg_version.h"
#include "streamwrapper.h"
#include "3d_cache/sg/sg_node.h"
#include "3d_cache/sg/scenegraph.h"
#include "3d_cache/sg/sg_appearance.h"
#include "3d_cache/sg/sg_shape.h"
#include "3d_cache/sg/sg_helpers.h"


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
}


bool S3D::WriteVRML( const char* filename, bool overwrite, SGNODE* aTopNode,
                     bool reuse, bool renameNodes )
{
    if( nullptr == filename || filename[0] == 0 )
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

    wxCHECK( aTopNode && aTopNode->GetNodeType() == S3D::SGTYPE_TRANSFORM, false );

    OPEN_OSTREAM( op, filename );

    if( op.fail() )
    {
        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d  * [INFO] failed to open file '%s'" ),
                    __FILE__, __FUNCTION__, __LINE__, filename );

        return false;
    }

    op.imbue( std::locale::classic() );
    op << "#VRML V2.0 utf8\n";

    if( renameNodes )
    {
        aTopNode->ResetNodeIndex();
        aTopNode->ReNameNodes();
    }

    aTopNode->WriteVRML( op, reuse );

    if( !op.fail() )
    {
        CLOSE_STREAM( op );
        return true;
    }

    CLOSE_STREAM( op );

    wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d  * [INFO] problems encountered writing file '%s'" ),
                __FILE__, __FUNCTION__, __LINE__, filename );

    return false;
}


void S3D::ResetNodeIndex( SGNODE* aNode )
{
    wxCHECK( aNode, /* void */ );

    aNode->ResetNodeIndex();
}


void S3D::RenameNodes( SGNODE* aNode )
{
    wxCHECK( aNode, /* void */ );

    aNode->ReNameNodes();
}


void S3D::DestroyNode( SGNODE* aNode ) noexcept
{
    wxCHECK( aNode, /* void */ );

    delete aNode;
}


bool S3D::WriteCache( const char* aFileName, bool overwrite, SGNODE* aNode,
                      const char* aPluginInfo )
{
    if( nullptr == aFileName || aFileName[0] == 0 )
        return false;

    wxString ofile = wxString::FromUTF8Unchecked( aFileName );

    wxCHECK( aNode, false );

    if( wxFileName::Exists( ofile ) )
    {
        if( !overwrite )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] file exists not overwriting '%s'" ),
                        __FILE__, __FUNCTION__, __LINE__, aFileName );

            return false;
        }

        // make sure we make no attempt to write a directory
        if( !wxFileName::FileExists( aFileName ) )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] specified path is a directory '%s'" ),
                        __FILE__, __FUNCTION__, __LINE__, aFileName );

            return false;
        }
    }

    OPEN_OSTREAM( output, aFileName );

    if( output.fail() )
    {
        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] failed to open file '%s'" ),
                    __FILE__, __FUNCTION__, __LINE__, aFileName );

        return false;
    }

    output << "(" << SG_VERSION_TAG << ")";

    if( nullptr != aPluginInfo && aPluginInfo[0] != 0 )
        output << "(" << aPluginInfo << ")";
    else
        output << "(INTERNAL:0.0.0.0)";

    bool rval = aNode->WriteCache( output, nullptr );
    CLOSE_STREAM( output );

    if( !rval )
    {
        wxLogTrace( MASK_3D_SG,
                    wxT( "%s:%s:%d * [INFO] problems encountered writing cache file '%s'" ),
                    __FILE__, __FUNCTION__, __LINE__, aFileName );

        // delete the defective file
        wxRemoveFile( ofile );
    }

    return rval;
}


SGNODE* S3D::ReadCache( const char* aFileName, void* aPluginMgr,
                        bool (*aTagCheck)( const char*, void* ) )
{
    if( nullptr == aFileName || aFileName[0] == 0 )
        return nullptr;

    wxString ofile = wxString::FromUTF8Unchecked( aFileName );

    if( !wxFileName::FileExists( aFileName ) )
    {
        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] no such file '%s'" ),
                    __FILE__, __FUNCTION__, __LINE__, aFileName );

        return nullptr;
    }

    std::unique_ptr<SGNODE> np = std::make_unique<SCENEGRAPH>( nullptr );

    OPEN_ISTREAM( file, aFileName );

    if( file.fail() )
    {
        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] failed to open file '%s'" ),
                    __FILE__, __FUNCTION__, __LINE__, aFileName );

        return nullptr;
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
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data; missing left parenthesis"
                                         " at position '%d'" ),
                        __FILE__, __FUNCTION__, __LINE__,
                        static_cast<int>( file.tellg() ) );

            CLOSE_STREAM( file );
            return nullptr;
        }

        file.get( schar );

        while( ')' != schar && file.good() )
        {
            name.push_back( schar );
            file.get( schar );
        }

        if( name.compare( SG_VERSION_TAG ) )
        {
            CLOSE_STREAM( file );
            return nullptr;
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
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data; missing left parenthesis"
                                         " at position '%d'" ),
                        __FILE__, __FUNCTION__, __LINE__,
                        static_cast<int>( file.tellg() ) );

            CLOSE_STREAM( file );
            return nullptr;
        }

        file.get( schar );

        while( ')' != schar && file.good() )
        {
            name.push_back( schar );
            file.get( schar );
        }

        // check the plugin tag
        if( nullptr != aTagCheck && nullptr != aPluginMgr
          && !aTagCheck( name.c_str(), aPluginMgr ) )
        {
            CLOSE_STREAM( file );
            return nullptr;
        }

    } while( 0 );

    bool rval = np->ReadCache( file, nullptr );
    CLOSE_STREAM( file );

    if( !rval )
    {
        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] problems encountered reading cache file "
                                     "'%s'" ),
                    __FILE__, __FUNCTION__, __LINE__,
                    aFileName );

        return nullptr;
    }

    return np.release();
}


S3DMODEL* S3D::GetModel( SCENEGRAPH* aNode )
{
    if( nullptr == aNode )
        return nullptr;

    if( aNode->GetNodeType() != S3D::SGTYPE_TRANSFORM )
        return nullptr;

    S3D::MATLIST materials;
    std::vector< SMESH > meshes;

    // the materials list shall have a default color; although the VRML
    // default is an opaque black, the default used here shall be a median
    // gray in hopes that it may help highlight faulty models; this color is
    // also typical of MCAD applications. When a model has no associated
    // material color it shall be assigned the index 0.
    SGAPPEARANCE app( nullptr );
    app.ambient = SGCOLOR( 0.6f, 0.6f, 0.6f );
    app.diffuse = SGCOLOR( 0.6f, 0.6f, 0.6f );
    app.specular = app.diffuse;
    app.shininess = 0.05f;
    app.transparency = 0.0f;

    materials.matorder.push_back( &app );
    materials.matmap.emplace( &app, 0 );

    if( aNode->Prepare( nullptr, materials, meshes ) )
    {
        if( meshes.empty() )
            return nullptr;

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

    return nullptr;
}


void S3D::Destroy3DModel( S3DMODEL** aModel )
{
    if( nullptr == aModel || nullptr == *aModel )
        return;

    S3DMODEL* m = *aModel;
    S3D::FREE_S3DMODEL( *m );
    delete m;
    *aModel = nullptr;
}


void Free3DModel( S3DMODEL& aModel )
{
    S3D::FREE_S3DMODEL( aModel );
}


void S3D::Free3DMesh( SMESH& aMesh )
{
    S3D::FREE_SMESH( aMesh );
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
}


void S3D::Init3DMesh( SMESH& aMesh )
{
    S3D::INIT_SMESH( aMesh );
}


void S3D::GetLibVersion( unsigned char* Major, unsigned char* Minor, unsigned char* Patch,
                         unsigned char* Revision ) noexcept
{
    if( Major )
        *Major = KICADSG_VERSION_MAJOR;

    if( Minor )
        *Minor = KICADSG_VERSION_MINOR;

    if( Revision )
        *Revision = KICADSG_VERSION_REVISION;

    if( Patch )
        *Patch = KICADSG_VERSION_PATCH;
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
    (void)glm::normalize( tri );

    return SGVECTOR( tri.x, tri.y, tri.z );
}


S3D::SGTYPES S3D::GetSGNodeType( SGNODE* aNode )
{
    if( nullptr == aNode )
        return SGTYPE_END;

    return aNode->GetNodeType();
}


SGNODE* S3D::GetSGNodeParent( SGNODE* aNode )
{
    if( nullptr == aNode )
        return nullptr;

    return aNode->GetParent();
}


bool S3D::AddSGNodeRef( SGNODE* aParent, SGNODE* aChild )
{
    if( nullptr == aParent || nullptr == aChild )
        return false;

    return aParent->AddRefNode( aChild );
}


bool S3D::AddSGNodeChild( SGNODE* aParent, SGNODE* aChild )
{
    if( nullptr == aParent || nullptr == aChild )
        return false;

    return aParent->AddChildNode( aChild );
}


void S3D::AssociateSGNodeWrapper( SGNODE* aObject, SGNODE** aRefPtr )
{
    if( nullptr == aObject || nullptr == aRefPtr || aObject != *aRefPtr )
        return;

    aObject->AssociateWrapper( aRefPtr );
}
