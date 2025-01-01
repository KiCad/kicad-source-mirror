/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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

#define GLM_FORCE_RADIANS

#include <iostream>
#include <sstream>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <wx/log.h>

#include "3d_cache/sg/scenegraph.h"
#include "3d_cache/sg/sg_shape.h"
#include "3d_cache/sg/sg_helpers.h"


SCENEGRAPH::SCENEGRAPH( SGNODE* aParent ) : SGNODE( aParent )
{
    m_SGtype = S3D::SGTYPE_TRANSFORM;
    rotation_angle = 0.0;
    scale_angle = 0.0;

    scale.x = 1.0;
    scale.y = 1.0;
    scale.z = 1.0;

    if( nullptr != aParent && S3D::SGTYPE_TRANSFORM != aParent->GetNodeType() )
    {
        m_Parent = nullptr;

        wxLogTrace( MASK_3D_SG,
                    wxT( "%s:%s:%d * [BUG] inappropriate parent to SCENEGRAPH (type %d)" ),
                    __FILE__, __FUNCTION__, __LINE__,
                    aParent->GetNodeType() );
    }
    else if( nullptr != aParent && S3D::SGTYPE_TRANSFORM == aParent->GetNodeType() )
    {
        m_Parent->AddChildNode( this );
    }
}


SCENEGRAPH::~SCENEGRAPH()
{
    // drop references
    DROP_REFS( SCENEGRAPH, m_RTransforms );
    DROP_REFS( SGSHAPE, m_RShape );

    // delete owned objects
    DEL_OBJS( SCENEGRAPH, m_Transforms );
    DEL_OBJS( SGSHAPE, m_Shape );
}


bool SCENEGRAPH::SetParent( SGNODE* aParent, bool notify )
{
    if( nullptr != m_Parent )
    {
        if( aParent == m_Parent )
            return true;

        // handle the change in parents
        if( notify )
            m_Parent->unlinkChildNode( this );

        m_Parent = nullptr;

        if( nullptr == aParent )
            return true;
    }

    // only a transform may be parent to a transform
    if( nullptr != aParent && S3D::SGTYPE_TRANSFORM != aParent->GetNodeType() )
        return false;

    m_Parent = aParent;

    if( m_Parent )
        m_Parent->AddChildNode( this );

    return true;
}


SGNODE* SCENEGRAPH::FindNode(const char *aNodeName, const SGNODE *aCaller)
{
    if( nullptr == aNodeName || 0 == aNodeName[0] )
        return nullptr;

    if( !m_Name.compare( aNodeName ) )
        return this;

    FIND_NODE( SCENEGRAPH, aNodeName, m_Transforms, aCaller );
    FIND_NODE( SGSHAPE, aNodeName, m_Shape, aCaller );

    // query the parent if appropriate
    if( aCaller == m_Parent || nullptr == m_Parent )
        return nullptr;

    return m_Parent->FindNode( aNodeName, this );
}


void SCENEGRAPH::unlinkNode( const SGNODE* aNode, bool isChild )
{
    if( nullptr == aNode )
        return;

    switch( aNode->GetNodeType() )
    {
    case S3D::SGTYPE_TRANSFORM:
        UNLINK_NODE( S3D::SGTYPE_TRANSFORM, SCENEGRAPH, aNode, m_Transforms, m_RTransforms,
                     isChild );
        break;

    case S3D::SGTYPE_SHAPE:
        UNLINK_NODE( S3D::SGTYPE_SHAPE, SGSHAPE, aNode, m_Shape, m_RShape, isChild );
        break;

    default:
        break;
    }

    wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [BUG] unlinkNode() did not find its target" ),
                __FILE__, __FUNCTION__, __LINE__ );
}


void SCENEGRAPH::unlinkChildNode( const SGNODE* aNode )
{
    unlinkNode( aNode, true );
    return;
}


void SCENEGRAPH::unlinkRefNode( const SGNODE* aNode )
{
    unlinkNode( aNode, false );
    return;
}


bool SCENEGRAPH::addNode( SGNODE* aNode, bool isChild )
{
    wxCHECK( aNode, false );

    ADD_NODE( S3D::SGTYPE_TRANSFORM, SCENEGRAPH, aNode, m_Transforms, m_RTransforms, isChild );
    ADD_NODE( S3D::SGTYPE_SHAPE, SGSHAPE, aNode, m_Shape, m_RShape, isChild );

    wxLogTrace( MASK_3D_SG,
                wxT( "%s:%s:%d * [BUG] object '%s' is not a valid type for this object (%d)" ),
                __FILE__, __FUNCTION__, __LINE__,
                aNode->GetName(),
                aNode->GetNodeType() );

    return false;
}


bool SCENEGRAPH::AddRefNode( SGNODE* aNode )
{
    return addNode( aNode, false );
}


bool SCENEGRAPH::AddChildNode( SGNODE* aNode )
{
    return addNode( aNode, true );
}


void SCENEGRAPH::ReNameNodes( void )
{
    m_written = false;

    // rename this node
    m_Name.clear();
    GetName();

    // rename all shapes
    do
    {
        std::vector< SGSHAPE* >::iterator sL = m_Shape.begin();
        std::vector< SGSHAPE* >::iterator eL = m_Shape.end();

        while( sL != eL )
        {
            (*sL)->ReNameNodes();
            ++sL;
        }

    } while( 0 );

    // rename all transforms
    do
    {
        std::vector< SCENEGRAPH* >::iterator sL = m_Transforms.begin();
        std::vector< SCENEGRAPH* >::iterator eL = m_Transforms.end();

        while( sL != eL )
        {
            (*sL)->ReNameNodes();
            ++sL;
        }

    } while( 0 );
}


bool SCENEGRAPH::WriteVRML( std::ostream& aFile, bool aReuseFlag )
{
    if( m_Transforms.empty() && m_RTransforms.empty() && m_Shape.empty() && m_RShape.empty() )
    {
        return false;
    }

    std::string tmp;

    if( aReuseFlag )
    {
        if( !m_written )
        {
            aFile << "DEF " << GetName() << " Transform {\n";
            m_written = true;
        }
        else
        {
            aFile << "USE " << GetName() << "\n";
            return true;
        }
    }
    else
    {
        aFile << " Transform {\n";
    }

    // convert center to 1VRML unit = 0.1 inch
    SGPOINT pt = center;
    pt.x /= 2.54;
    pt.y /= 2.54;
    pt.z /= 2.54;

    S3D::FormatPoint( tmp, pt );
    aFile << "  center " << tmp << "\n";
    S3D::FormatOrientation( tmp, rotation_axis, rotation_angle );
    aFile << "  rotation " << tmp << "\n";
    S3D::FormatPoint( tmp, scale );
    aFile << "  scale " << tmp << "\n";
    S3D::FormatOrientation( tmp, scale_axis, scale_angle );
    aFile << "  scaleOrientation " << tmp << "\n";

    // convert translation to 1VRML unit = 0.1 inch
    pt = translation;
    pt.x /= 2.54;
    pt.y /= 2.54;
    pt.z /= 2.54;
    S3D::FormatPoint( tmp, pt );
    aFile << "  translation " << tmp << "\n";

    aFile << " children [\n";

    if( !m_Transforms.empty() )
    {
        std::vector< SCENEGRAPH* >::iterator sL = m_Transforms.begin();
        std::vector< SCENEGRAPH* >::iterator eL = m_Transforms.end();

        while( sL != eL )
        {
            (*sL)->WriteVRML( aFile, aReuseFlag );
            ++sL;
        }
    }

    if( !m_RTransforms.empty() )
    {
        std::vector< SCENEGRAPH* >::iterator sL = m_RTransforms.begin();
        std::vector< SCENEGRAPH* >::iterator eL = m_RTransforms.end();

        while( sL != eL )
        {
            (*sL)->WriteVRML( aFile, aReuseFlag );
            ++sL;
        }
    }

    if( !m_Shape.empty() )
    {
        std::vector< SGSHAPE* >::iterator sL = m_Shape.begin();
        std::vector< SGSHAPE* >::iterator eL = m_Shape.end();

        while( sL != eL )
        {
            (*sL)->WriteVRML( aFile, aReuseFlag );
            ++sL;
        }
    }

    if( !m_RShape.empty() )
    {
        std::vector< SGSHAPE* >::iterator sL = m_RShape.begin();
        std::vector< SGSHAPE* >::iterator eL = m_RShape.end();

        while( sL != eL )
        {
            (*sL)->WriteVRML( aFile, aReuseFlag );
            ++sL;
        }
    }

    aFile << "] }\n";

    return true;
}


bool SCENEGRAPH::WriteCache( std::ostream& aFile, SGNODE* parentNode )
{
    if( nullptr == parentNode && nullptr != m_Parent )
    {
        SGNODE* np = m_Parent;

        while( nullptr != np->GetParent() )
            np = np->GetParent();

        if( np->WriteCache( aFile, nullptr ) )
        {
            m_written = true;
            return true;
        }

        return false;
    }

    wxCHECK( parentNode == m_Parent, false );

    if( nullptr == m_Parent )
    {
        // ensure unique node names
        ResetNodeIndex();
        ReNameNodes();
    }

    if( aFile.fail() )
    {
        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] bad stream" ),
                    __FILE__, __FUNCTION__, __LINE__ );

        return false;
    }

    aFile << "[" << GetName() << "]";
    S3D::WritePoint( aFile, center );
    S3D::WritePoint( aFile, translation );
    S3D::WriteVector( aFile, rotation_axis );
    aFile.write( (char*)&rotation_angle, sizeof( rotation_angle ) );
    S3D::WritePoint( aFile, scale );
    S3D::WriteVector( aFile, scale_axis );
    aFile.write( (char*)&scale_angle, sizeof( scale_angle ) );

    // Transfer ownership of any Transform references which hadn't been written
    size_t asize = m_RTransforms.size();
    size_t i;

    for( i = 0; i < asize; ++i )
    {
        if( !m_RTransforms[i]->isWritten() )
        {
            m_RTransforms[i]->SwapParent( this );
            --asize;
            --i;
        }
    }

    // Transfer ownership of any Shape references which hadn't been written
    asize = m_RShape.size();

    for( i = 0; i < asize; ++i )
    {
        if( !m_RShape[i]->isWritten() )
        {
            m_RShape[i]->SwapParent( this );
            --asize;
            --i;
        }
    }

    asize = m_Transforms.size();
    aFile.write( (char*)&asize, sizeof( size_t ) );
    asize = m_RTransforms.size();
    aFile.write( (char*)&asize, sizeof( size_t ) );
    asize = m_Shape.size();
    aFile.write( (char*)&asize, sizeof( size_t ) );
    asize = m_RShape.size();
    aFile.write( (char*)&asize, sizeof( size_t ) );
    asize = m_Transforms.size();

    // write child transforms
    for( i = 0; i < asize; ++i )
    {
        if( !m_Transforms[i]->WriteCache( aFile, this ) )
        {
            wxLogTrace( MASK_3D_SG,
                        wxT( "%s:%s:%d * [INFO] bad stream while writing child transforms" ),
                        __FILE__, __FUNCTION__, __LINE__ );

            return false;
        }
    }

    // write referenced transform names
    asize = m_RTransforms.size();

    for( i = 0; i < asize; ++i )
        aFile << "[" << m_RTransforms[i]->GetName() << "]";

    // write child shapes
    asize = m_Shape.size();

    for( i = 0; i < asize; ++i )
    {
        if( !m_Shape[i]->WriteCache( aFile, this ) )
        {
            wxLogTrace( MASK_3D_SG,
                        wxT( "%s:%s:%d * [INFO] bad stream while writing child shapes" ),
                        __FILE__, __FUNCTION__, __LINE__ );

            return false;
        }
    }

    // write referenced transform names
    asize = m_RShape.size();

    for( i = 0; i < asize; ++i )
        aFile << "[" << m_RShape[i]->GetName() << "]";

    if( aFile.fail() )
        return false;

    m_written = true;
    return true;
}


bool SCENEGRAPH::ReadCache( std::istream& aFile, SGNODE* parentNode )
{
    wxCHECK( m_Transforms.empty() && m_RTransforms.empty() && m_Shape.empty() && m_RShape.empty(),
             false );

    std::string name;   // name of the node

    if( nullptr == parentNode )
    {
        // we need to read the tag and verify its type
        if( S3D::SGTYPE_TRANSFORM != S3D::ReadTag( aFile, name ) )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data; tag mismatch at position "
                                         "%ul" ),
                        __FILE__, __FUNCTION__, __LINE__,
                        static_cast<unsigned long>( aFile.tellg() ) );

            return false;
        }

        m_Name = name;
    }

    // read fixed member data
    S3D::ReadPoint( aFile, center );
    S3D::ReadPoint( aFile, translation );
    S3D::ReadVector( aFile, rotation_axis );
    aFile.read( (char*)&rotation_angle, sizeof( rotation_angle ) );
    S3D::ReadPoint( aFile, scale );
    S3D::ReadVector( aFile, scale_axis );
    aFile.read( (char*)&scale_angle, sizeof( scale_angle ) );

    size_t sizeCT = 0;  // child transforms
    size_t sizeRT = 0;  // referenced transforms
    size_t sizeCS = 0;  // child shapes
    size_t sizeRS = 0;  // referenced shapes

    aFile.read( (char*)&sizeCT, sizeof( size_t ) );
    aFile.read( (char*)&sizeRT, sizeof( size_t ) );
    aFile.read( (char*)&sizeCS, sizeof( size_t ) );
    aFile.read( (char*)&sizeRS, sizeof( size_t ) );

    size_t i;

    // read child transforms
    for( i = 0; i < sizeCT; ++i )
    {
        if( S3D::SGTYPE_TRANSFORM != S3D::ReadTag( aFile, name ) )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data; bad child transform tag "
                                         "at position %ul" ),
                        __FILE__, __FUNCTION__, __LINE__,
                        static_cast<unsigned long>( aFile.tellg() ) );

            return false;
        }

        SCENEGRAPH* sp = new SCENEGRAPH( this );
        sp->SetName( name.c_str() );

        if( !sp->ReadCache( aFile, this ) )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data while reading transform "
                                         "%ul" ),
                        __FILE__, __FUNCTION__, __LINE__,
                        static_cast<unsigned long>( aFile.tellg() ) );

            return false;
        }
    }

    // read referenced transforms
    for( i = 0; i < sizeRT; ++i )
    {
        if( S3D::SGTYPE_TRANSFORM != S3D::ReadTag( aFile, name ) )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data; bad ref transform tag at "
                                         "position %ul" ),
                        __FILE__, __FUNCTION__, __LINE__,
                        static_cast<unsigned long>( aFile.tellg() ) );

            return false;
        }

        SGNODE* sp = FindNode( name.c_str(), this );

        if( !sp )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data: cannot find ref "
                                         "transform at position %ul" ),
                        __FILE__, __FUNCTION__, __LINE__,
                        static_cast<unsigned long>( aFile.tellg() ) );

            return false;
        }

        if( S3D::SGTYPE_TRANSFORM != sp->GetNodeType() )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data: type is not TRANSFORM "
                                         "at position %ul" ),
                        __FILE__, __FUNCTION__, __LINE__,
                        static_cast<unsigned long>( aFile.tellg() ) );

            return false;
        }

        AddRefNode( sp );
    }

    // read child shapes
    for( i = 0; i < sizeCS; ++i )
    {
        if( S3D::SGTYPE_SHAPE != S3D::ReadTag( aFile, name ) )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data; bad child shape tag at "
                                         "position %ul" ),
                        __FILE__, __FUNCTION__, __LINE__,
                        static_cast<unsigned long>( aFile.tellg() ) );

            return false;
        }

        SGSHAPE* sp = new SGSHAPE( this );
        sp->SetName( name.c_str() );

        if( !sp->ReadCache( aFile, this ) )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data; corrupt data while "
                                        "reading shape at position %ul" ),
                        __FILE__, __FUNCTION__, __LINE__,
                        static_cast<unsigned long>( aFile.tellg() ) );

            return false;
        }
    }

    // read referenced shapes
    for( i = 0; i < sizeRS; ++i )
    {
        if( S3D::SGTYPE_SHAPE != S3D::ReadTag( aFile, name ) )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data; bad ref shape tag at "
                                         "position %ul" ),
                        __FILE__, __FUNCTION__, __LINE__,
                        static_cast<unsigned long>( aFile.tellg() ) );

            return false;
        }

        SGNODE* sp = FindNode( name.c_str(), this );

        if( !sp )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data: cannot find ref shape "
                                         "at position %ul" ),
                        __FILE__, __FUNCTION__, __LINE__,
                        static_cast<unsigned long>( aFile.tellg() ) );

            return false;
        }

        if( S3D::SGTYPE_SHAPE != sp->GetNodeType() )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data: type is not SGSHAPE at "
                                         "position %ul" ),
                        __FILE__, __FUNCTION__, __LINE__,
                        static_cast<unsigned long>( aFile.tellg() ) );

            return false;
        }

        AddRefNode( sp );
    }

    if( aFile.fail() )
        return false;

    return true;
}


bool SCENEGRAPH::Prepare( const glm::dmat4* aTransform, S3D::MATLIST& materials,
                          std::vector< SMESH >& meshes )
{
    // calculate the accumulated transform
    double rX, rY, rZ;

    // rotation
    rotation_axis.GetVector( rX, rY, rZ );
    glm::dmat4 rM = glm::rotate( glm::dmat4( 1.0 ), rotation_angle, glm::dvec3( rX, rY, rZ ) );

    // translation
    glm::dmat4 tM = glm::translate( glm::dmat4( 1.0 ), glm::dvec3( translation.x, translation.y,
                                                                   translation.z ) );
    // center
    glm::dmat4 cM = glm::translate( glm::dmat4( 1.0 ), glm::dvec3( center.x, center.y, center.z ) );
    glm::dmat4 ncM = glm::translate( glm::dmat4( 1.0 ), glm::dvec3( -center.x, -center.y,
                                                                  -center.z ) );

    // scale
    glm::dmat4 sM = glm::scale( glm::dmat4( 1.0 ), glm::dvec3( scale.x, scale.y, scale.z ) );

    // scaleOrientation
    scale_axis.GetVector( rX, rY, rZ );
    glm::dmat4 srM = glm::rotate( glm::dmat4( 1.0 ), scale_angle, glm::dvec3( rX, rY, rZ ) );
    glm::dmat4 nsrM = glm::rotate( glm::dmat4( 1.0 ), -scale_angle, glm::dvec3( rX, rY, rZ ) );

    // resultant point:
    // P' = T x C x R x SR x S x -SR x -C x P
    // resultant transform:
    // tx0 = tM * cM * rM * srM * sM * nsrM * ncM
    glm::dmat4 tx0;

    if( nullptr != aTransform )
        tx0  = (*aTransform) * tM * cM * rM * srM * sM * nsrM * ncM;
    else
        tx0  = tM * cM * rM * srM * sM * nsrM * ncM;

    bool ok = true;

    // prepare all shapes
    do
    {
        std::vector< SGSHAPE* >::iterator sL = m_Shape.begin();
        std::vector< SGSHAPE* >::iterator eL = m_Shape.end();

        while( sL != eL && ok )
        {
            ok = (*sL)->Prepare( &tx0, materials, meshes );
            ++sL;
        }

        sL = m_RShape.begin();
        eL = m_RShape.end();

        while( sL != eL && ok )
        {
            ok = (*sL)->Prepare( &tx0, materials, meshes );
            ++sL;
        }

    } while( 0 );

    // prepare all transforms
    do
    {
        std::vector< SCENEGRAPH* >::iterator sL = m_Transforms.begin();
        std::vector< SCENEGRAPH* >::iterator eL = m_Transforms.end();

        while( sL != eL && ok )
        {
            ok = (*sL)->Prepare( &tx0, materials, meshes );
            ++sL;
        }

        sL = m_RTransforms.begin();
        eL = m_RTransforms.end();

        while( sL != eL && ok )
        {
            ok = (*sL)->Prepare( &tx0, materials, meshes );
            ++sL;
        }

    } while( 0 );

    return ok;
}
