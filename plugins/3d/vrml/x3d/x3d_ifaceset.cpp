/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
#include <wx/xml/xml.h>
#include <wx/tokenzr.h>
#include "x3d_ops.h"
#include "x3d_ifaceset.h"


X3DIFACESET::X3DIFACESET() : X3DNODE()
{
    m_Type = X3D_INDEXED_FACE_SET;
    init();

    return;
}


X3DIFACESET::X3DIFACESET( X3DNODE* aParent ) : X3DNODE()
{
    m_Type = X3D_INDEXED_FACE_SET;
    init();

    if( NULL != aParent )
    {
        X3DNODES ptype = aParent->GetNodeType();

        if( X3D_SHAPE == ptype )
            m_Parent = aParent;
    }

    if( NULL != m_Parent )
        m_Parent->AddChildNode( this );

    return;
}


X3DIFACESET::~X3DIFACESET()
{
    #if defined( DEBUG_X3D ) && ( DEBUG_X3D > 2 )
    std::cerr << " * [INFO] Destroying IndexedFaceSet\n";
    #endif

    return;
}


void X3DIFACESET::init()
{
    coord = NULL;

    ccw = true;
    creaseAngle = 0.5;
    creaseLimit = 0.878;    // approx cos( 0.5 )
    return;
}


void X3DIFACESET::readFields( wxXmlNode* aNode )
{
    // DEF
    // ccw
    // creaseAngle
    // coordIndex

    wxXmlAttribute* prop;

    for( prop = aNode->GetAttributes();
         prop != NULL;
         prop = prop->GetNext() )
    {
        wxString pname = prop->GetName();

        if( pname == "DEF" )
        {
            m_Name = prop->GetValue();
            m_Dict->AddName( m_Name, this );
        }
        else if( pname == "ccw" )
            X3D::ParseSFBool( prop->GetValue(), ccw );
        else if( pname == "creaseAngle" )
        {
            X3D::ParseSFFloat( prop->GetValue(), creaseAngle );

            if( creaseAngle < 0.0f )
                creaseAngle = 0.0f;
            else if( creaseAngle > M_PI * 0.34 )
                creaseAngle = M_PI / 3.0;

            creaseLimit = cosf( creaseAngle );
        }
        else if( pname == "coordIndex" )
        {
            wxStringTokenizer indices( prop->GetValue() );

            while( indices.HasMoreTokens() )
            {
                long index = 0;
                indices.GetNextToken().ToLong( &index );
                coordIndex.push_back( (int) index );
            }
        }
    }

    return;
}


bool X3DIFACESET::Read( wxXmlNode* aNode, X3DNODE* aTopNode, X3D_DICT& aDict )
{
    if( NULL == aTopNode || NULL == aNode )
        return false;

    m_Dict = &aDict;
    readFields( aNode );
    bool ok = false;

    for( wxXmlNode* child = aNode->GetChildren();
         child != NULL;
         child = child->GetNext() )
    {
        if( child->GetName() == "Coordinate" )
            ok = X3D::ReadCoordinates( child, this, aDict );

    }

    if( false == ok )
        return false;


    if( !SetParent( aTopNode ) )
        return false;

    std::cerr << "XXX: Got " << coordIndex.size() << " indices\n";

    return true;
}


bool X3DIFACESET::SetParent( X3DNODE* aParent, bool doUnlink )
{
    if( aParent == m_Parent )
        return true;

    if( NULL != aParent )
    {
        X3DNODES nt = aParent->GetNodeType();

        if( nt != X3D_SHAPE )
            return false;
    }

    if( NULL != m_Parent && doUnlink )
        m_Parent->unlinkChildNode( this );

    m_Parent = aParent;

    if( NULL != m_Parent )
        m_Parent->AddChildNode( this );

    return true;
}


bool X3DIFACESET::AddChildNode( X3DNODE* aNode )
{
    return false;
}


bool X3DIFACESET::AddRefNode( X3DNODE* aNode )
{
    return false;
}


SGNODE* X3DIFACESET::TranslateToSG( SGNODE* aParent )
{
    // XXX -
    return NULL;
}
