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

#include <wx/tokenzr.h>
#include <wx/xml/xml.h>

#include "x3d_appearance.h"
#include "x3d_coords.h"
#include "x3d_ops.h"
#include "x3d_transform.h"


bool X3D::ReadTransform( wxXmlNode* aNode, X3DNODE* aParent, X3D_DICT& aDict )
{
    // note: we must have a parent or else we will have a memory leak
    if( NULL == aParent || NULL == aNode )
        return false;

    wxXmlAttribute* prop;

    for( prop = aNode->GetAttributes();
         prop != NULL;
         prop = prop->GetNext() )
    {
        wxString pname = prop->GetName();

        if( pname == "USE" )
        {
            X3DNODE* np = aDict.FindName( prop->GetValue() );

            if( NULL == np )
                return false;

            if( !aParent->AddRefNode( np ) )
                return false;

            return true;
        }
    }

    X3DNODE* node = new X3DTRANSFORM;

    if( !node->Read( aNode, aParent, aDict ) )
    {
        delete node;
        return false;
    }

    return true;
}


bool X3D::ReadSwitch( wxXmlNode* aNode, X3DNODE* aParent, X3D_DICT& aDict )
{
    // XXX - TO BE IMPLEMENTED
    return false;
}


bool X3D::ReadShape( wxXmlNode* aNode, X3DNODE* aParent, X3D_DICT& aDict )
{
    // note: we must have a parent or else we will have a memory leak
    if( NULL == aParent || NULL == aNode )
        return false;

    wxXmlAttribute* prop;

    for( prop = aNode->GetAttributes();
         prop != NULL;
         prop = prop->GetNext() )
    {
        wxString pname = prop->GetName();

        if( pname == "USE" )
        {
            X3DNODE* np = aDict.FindName( prop->GetValue() );

            if( NULL == np )
                return false;

            if( !aParent->AddRefNode( np ) )
                return false;

            return true;
        }
    }

    X3DNODE* node = new X3DSHAPE;

    if( !node->Read( aNode, aParent, aDict ) )
    {
        delete node;
        return false;
    }

    return true;
}


bool X3D::ReadAppearance( wxXmlNode* aNode, X3DNODE* aParent, X3D_DICT& aDict )
{
    // note: we must have a parent or else we will have a memory leak
    if( NULL == aParent || NULL == aNode )
        return false;

    wxXmlAttribute* prop;

    for( prop = aNode->GetAttributes();
         prop != NULL;
         prop = prop->GetNext() )
    {
        wxString pname = prop->GetName();

        if( pname == "USE" )
        {
            X3DNODE* np = aDict.FindName( prop->GetValue() );

            if( NULL == np )
                return false;

            if( !aParent->AddRefNode( np ) )
                return false;

            return true;
        }
    }

    X3DNODE* node = new X3DAPP;

    if( !node->Read( aNode, aParent, aDict ) )
    {
        delete node;
        return false;
    }

    return true;
}


bool X3D::ReadIndexedFaceSet( wxXmlNode* aNode, X3DNODE* aParent, X3D_DICT& aDict )
{
    // note: we must have a parent or else we will have a memory leak
    if( NULL == aParent || NULL == aNode )
        return false;

    wxXmlAttribute* prop;

    for( prop = aNode->GetAttributes();
         prop != NULL;
         prop = prop->GetNext() )
    {
        wxString pname = prop->GetName();

        if( pname == "USE" )
        {
            X3DNODE* np = aDict.FindName( prop->GetValue() );

            if( NULL == np )
                return false;

            if( !aParent->AddRefNode( np ) )
                return false;

            return true;
        }
    }

    X3DNODE* node = new X3DIFACESET;

    if( !node->Read( aNode, aParent, aDict ) )
    {
        delete node;
        return false;
    }

    return true;
}


bool X3D::ReadCoordinates( wxXmlNode* aNode, X3DNODE* aParent, X3D_DICT& aDict )
{
    // note: we must have a parent or else we will have a memory leak
    if( NULL == aParent || NULL == aNode )
        return false;

    wxXmlAttribute* prop;

    for( prop = aNode->GetAttributes();
         prop != NULL;
         prop = prop->GetNext() )
    {
        wxString pname = prop->GetName();

        if( pname == "USE" )
        {
            X3DNODE* np = aDict.FindName( prop->GetValue() );

            if( NULL == np )
                return false;

            if( !aParent->AddRefNode( np ) )
                return false;

            return true;
        }
    }

    X3DNODE* node = new X3DCOORDS;

    if( !node->Read( aNode, aParent, aDict ) )
    {
        delete node;
        return false;
    }

    return true;
}


bool X3D::ParseSFBool( const wxString& aSource, bool& aResult )
{
    wxStringTokenizer tokens( aSource );
    wxString val = tokens.GetNextToken();

    if( val == "TRUE" || val == "1" )
    {
        aResult = true;
        return true;
    }

    if( val == "FALSE" || val == "0" )
    {
        aResult = false;
        return true;
    }

    return false;
}


bool X3D::ParseSFFloat( const wxString& aSource, float& aResult )
{
    wxStringTokenizer tokens( aSource );

    double x = 0;
    bool ret = tokens.GetNextToken().ToDouble( &x );

    aResult = x;
    return ret;
}


bool X3D::ParseSFVec3( const wxString& aSource, WRLVEC3F& aResult )
{
    wxStringTokenizer tokens( aSource );

    double x = 0;
    double y = 0;
    double z = 0;

    bool ret = tokens.GetNextToken().ToDouble( &x )
               && tokens.GetNextToken().ToDouble( &y )
               && tokens.GetNextToken().ToDouble( &z );

    aResult.x = x;
    aResult.y = y;
    aResult.z = z;

    return ret;
}


bool X3D::ParseSFRotation( const wxString& aSource, WRLROTATION& aResult )
{
    wxStringTokenizer tokens( aSource );

    double x = 0;
    double y = 0;
    double z = 0;
    double w = 0;

    bool ret = tokens.GetNextToken().ToDouble( &x )
               && tokens.GetNextToken().ToDouble( &y )
               && tokens.GetNextToken().ToDouble( &z )
               && tokens.GetNextToken().ToDouble( &w );

    aResult.x = x;
    aResult.y = y;
    aResult.z = z;
    aResult.w = w;

    return ret;
}
