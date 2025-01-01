/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
#include <wx/log.h>

#include "vrml1_base.h"
#include "vrml1_material.h"
#include "plugins/3dapi/ifsg_all.h"


WRL1MATERIAL::WRL1MATERIAL( NAMEREGISTER* aDictionary ) : WRL1NODE( aDictionary )
{
    colors[0] = nullptr;
    colors[1] = nullptr;
    m_Type = WRL1NODES::WRL1_MATERIAL;
}


WRL1MATERIAL::WRL1MATERIAL( NAMEREGISTER* aDictionary, WRL1NODE* aParent ) :
    WRL1NODE( aDictionary )
{
    colors[0] = nullptr;
    colors[1] = nullptr;
    m_Type = WRL1NODES::WRL1_MATERIAL;
    m_Parent = aParent;

    if( nullptr != m_Parent )
        m_Parent->AddChildNode( this );
}


WRL1MATERIAL::~WRL1MATERIAL()
{
    wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] Destroying Material node." ) );

    // destroy any orphaned color nodes
    for( int i = 0; i < 2; ++i )
    {
        if( nullptr != colors[i] )
        {
            if( nullptr == S3D::GetSGNodeParent( colors[i] ) )
                S3D::DestroyNode( colors[i] );

            wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] destroyed SGCOLOR #%d" ), i );
        }
    }
}


bool WRL1MATERIAL::AddRefNode( WRL1NODE* aNode )
{
    // this node may not own or reference any other node
    wxCHECK_MSG( false, false, wxT( "AddRefNode is not applicable." ) );
}


bool WRL1MATERIAL::AddChildNode( WRL1NODE* aNode )
{
    // this node may not own or reference any other node
    wxCHECK_MSG( false, false, wxT( "AddChildNode is not applicable." ) );
}


bool WRL1MATERIAL::Read( WRLPROC& proc, WRL1BASE* aTopNode )
{
    wxCHECK_MSG( aTopNode, false, wxT( "aTopNode is nullptr." ) );

    char tok = proc.Peek();

    if( proc.eof() )
    {
        wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                          " * [INFO] bad file format; unexpected eof %s." ),
                    __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition() );

        return false;
    }

    if( '{' != tok )
    {
        wxLogTrace( traceVrmlPlugin,
                    wxT( "%s:%s:%d\n"
                         " * [INFO] bad file format; expecting '{' but got '%s' %s" ),
                    __FILE__, __FUNCTION__, __LINE__, tok, proc.GetFilePosition() );

        return false;
    }

    proc.Pop();
    std::string glob;

    while( true )
    {
        if( proc.Peek() == '}' )
        {
            proc.Pop();
            break;
        }

        if( !proc.ReadName( glob ) )
        {
            wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                              "%s" ),
                        __FILE__, __FUNCTION__, __LINE__, proc.GetError() );

            return false;
        }

        // expecting one of:
        // ambientColor
        // diffuseColor
        // emissiveColor
        // shininess
        // specularColor
        // transparency

        if( !glob.compare( "specularColor" ) )
        {
            if( !proc.ReadMFVec3f( specularColor ) )
            {
                wxLogTrace( traceVrmlPlugin,
                            wxT( "%s:%s:%d\n"
                                 " * [INFO] invalid specularColor %s\n"
                                 " * [INFO] file: '%s'\n"
                                 "%s" ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

                return false;
            }
        }
        else if( !glob.compare( "diffuseColor" ) )
        {
            if( !proc.ReadMFVec3f( diffuseColor ) )
            {
                wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                                  " * [INFO] invalid diffuseColor %s\n"
                                                  " * [INFO] file: '%s'\n"
                                                  " * [INFO] error: '%s'." ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

                return false;
            }
        }
        else if( !glob.compare( "emissiveColor" ) )
        {
            if( !proc.ReadMFVec3f( emissiveColor ) )
            {
                wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                                  " * [INFO] invalid emissiveColor %s\n"
                                                  " * [INFO] file: '%s'\n"
                                                  "%s" ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

                return false;
            }
        }
        else if( !glob.compare( "shininess" ) )
        {
            if( !proc.ReadMFFloat( shininess ) )
            {
                wxLogTrace( traceVrmlPlugin,
                            wxT( "%s:%s:%d\n"
                                 " * [INFO] invalid shininess %s\n"
                                 " * [INFO] file: '%s'\n"
                                 " * [INFO] error: '%s'." ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFileName(),
                            proc.GetFileName(), proc.GetError() );

                return false;
            }
        }
        else if( !glob.compare( "transparency" ) )
        {
            if( !proc.ReadMFFloat( transparency ) )
            {
                wxLogTrace( traceVrmlPlugin,
                            wxT( "%s:%s:%d\n"
                                 " * [INFO] invalid transparency %s\n"
                                 " * [INFO] file: '%s'\n"
                                 " * [INFO] error: '%s'." ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

                return false;
            }
        }
        else if( !glob.compare( "ambientColor" ) )
        {
            if( !proc.ReadMFVec3f( ambientColor ) )
            {
                wxLogTrace( traceVrmlPlugin,
                            wxT( "%s:%s:%d\n"
                                 " * [INFO] invalid ambientColor %s\n"
                                 " * [INFO] file: '%s'\n"
                                 " * [INFO] error: '%s'." ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

                return false;
            }
        }
        else
        {
            wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                              "* [INFO] bad Material %s.\n"
                                              "* [INFO] file: '%s'." ),
                        __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                        proc.GetFileName() );

            return false;
        }
    }   // while( true ) -- reading contents of Material{}

    return true;
}


SGNODE* WRL1MATERIAL::TranslateToSG( SGNODE* aParent, WRL1STATUS* sp )
{
    wxCHECK_MSG( sp, nullptr, wxT( "Bad model: no base data given." ) );

    sp->mat = this;

    return nullptr;
}


SGNODE* WRL1MATERIAL::GetAppearance( int aIndex )
{
    ++aIndex;

    // invalid indices result in the default colors
    if( aIndex != 0 && aIndex != 1 )
        aIndex = 0;

    if( nullptr != colors[ aIndex ] )
        return colors[ aIndex ];

    IFSG_APPEARANCE app( true );

    float red, green, blue, val;

    if( aIndex == 0 || transparency.empty() )
        val = 0.0;
    else
        val = transparency[0];

    checkRange( val );
    app.SetTransparency( val );

    if( aIndex == 0 || shininess.empty() )
        val = 0.2f;
    else
        val = shininess[0];

    checkRange( val );
    app.SetShininess( val );

    if( aIndex ==0 || ambientColor.empty() )
    {
        red = 0.2f;
        green = 0.2f;
        blue = 0.2f;
    }
    else
    {
        red = ambientColor[0].x;
        green = ambientColor[0].y;
        blue = ambientColor[0].z;
    }

    checkRange( red );
    checkRange( green );
    checkRange( blue );
    app.SetAmbient( red, green, blue );

    if( aIndex == 0 || diffuseColor.empty() )
    {
        red = 0.8f;
        green = 0.8f;
        blue = 0.8f;
    }
    else
    {
        red = diffuseColor[0].x;
        green = diffuseColor[0].y;
        blue = diffuseColor[0].z;
    }

    checkRange( red );
    checkRange( green );
    checkRange( blue );
    app.SetDiffuse( red, green, blue );

    if( aIndex > (int)emissiveColor.size() )
    {
        red = 0.0;
        green = 0.0;
        blue = 0.0;
    }
    else
    {
        red = emissiveColor[0].x;
        green = emissiveColor[0].y;
        blue = emissiveColor[0].z;
    }

    checkRange( red );
    checkRange( green );
    checkRange( blue );
    app.SetEmissive( red, green, blue );

    if( aIndex > (int)specularColor.size() )
    {
        red = 0.0;
        green = 0.0;
        blue = 0.0;
    }
    else
    {
        red = specularColor[0].x;
        green = specularColor[0].y;
        blue = specularColor[0].z;
    }

    checkRange( red );
    checkRange( green );
    checkRange( blue );
    app.SetSpecular( red, green, blue );

    colors[aIndex] = app.GetRawPtr();

    return colors[aIndex];
}


void WRL1MATERIAL::GetColor( SGCOLOR* aColor, int aIndex )
{
    if( nullptr == aColor )
        return;

    // Calculate the color based on the given index using the formula:
    // color = ( emission + ambient + diffuse + shininess * specular ) / N
    // where N = number of non-zero components or 1 (if all zero)
    // If the index exceeds the number of items in a list, use the LAST
    // item rather than the default; this behavior caters to some bad
    // models.

    WRLVEC3F rgb;
    float dRed, dBlue, dGreen;
    float eRed, eBlue, eGreen;
    float aRed, aBlue, aGreen;
    float sRed, sBlue, sGreen;
    float shiny;

    if( aIndex < 0 || ( aIndex >= (int)diffuseColor.size() ) )
    {
        if( !diffuseColor.empty() )
        {
            rgb = diffuseColor.back();
            dRed = rgb.x;
            dGreen = rgb.y;
            dBlue = rgb.z;
        }
        else
        {
            dRed = 0.8f;
            dGreen = 0.8f;
            dBlue = 0.8f;
        }
    }
    else
    {
        rgb = diffuseColor[aIndex];
        dRed = rgb.x;
        dGreen = rgb.y;
        dBlue = rgb.z;
    }

    if( aIndex < 0 || ( aIndex >= (int)emissiveColor.size() ) )
    {
        if( !emissiveColor.empty() )
        {
            rgb = emissiveColor.back();
            eRed = rgb.x;
            eGreen = rgb.y;
            eBlue = rgb.z;
        }
        else
        {
            eRed = 0.0f;
            eGreen = 0.0f;
            eBlue = 0.0f;
        }
    }
    else
    {
        rgb = emissiveColor[aIndex];
        eRed = rgb.x;
        eGreen = rgb.y;
        eBlue = rgb.z;
    }

    if( aIndex < 0 || ( aIndex >= (int)ambientColor.size() ) )
    {
        if( !ambientColor.empty() )
        {
            rgb = ambientColor.back();
            aRed = rgb.x;
            aGreen = rgb.y;
            aBlue = rgb.z;
        }
        else
        {
            aRed = 0.2f;
            aGreen = 0.2f;
            aBlue = 0.2f;
        }
    }
    else
    {
        rgb = ambientColor[aIndex];
        aRed = rgb.x;
        aGreen = rgb.y;
        aBlue = rgb.z;
    }

    if( aIndex < 0 || ( aIndex >= (int)specularColor.size() ) )
    {
        if( !specularColor.empty() )
        {
            rgb = specularColor.back();
            sRed = rgb.x;
            sGreen = rgb.y;
            sBlue = rgb.z;
        }
        else
        {
            sRed = 0.2f;
            sGreen = 0.2f;
            sBlue = 0.2f;
        }
    }
    else
    {
        rgb = specularColor[aIndex];
        sRed = rgb.x;
        sGreen = rgb.y;
        sBlue = rgb.z;
    }

    if( aIndex < 0 || ( aIndex >= (int)shininess.size() ) )
    {
        if( !shininess.empty() )
            shiny = shininess.back();
        else
            shiny = 0.2f;
    }
    else
    {
        shiny = shininess[aIndex];
    }

    checkRange( aRed );
    checkRange( aGreen );
    checkRange( aBlue );
    checkRange( eRed );
    checkRange( eGreen );
    checkRange( eBlue );
    checkRange( dRed );
    checkRange( dGreen );
    checkRange( dBlue );
    checkRange( sRed );
    checkRange( sGreen );
    checkRange( sBlue );

    int n = 0;

    if( aRed + aGreen + aBlue > 0.01f )
        ++n;

    if( eRed + eGreen + eBlue > 0.01f )
        ++n;

    if( dRed + dGreen + dBlue > 0.01f )
        ++n;

    if( ( sRed + sGreen + sBlue ) * shiny > 0.01f )
        ++n;

    if( 0 == n )
        ++n;

    float red, green, blue;

    red = (eRed + aRed + dRed + sRed * shiny) / n;
    green = (eGreen + aGreen + dGreen + sGreen * shiny) / n;
    blue = (eBlue + aBlue + dBlue + sBlue * shiny) / n;
    checkRange( red );
    checkRange( green );
    checkRange( blue );
    aColor->SetColor( red, green, blue );
}


void WRL1MATERIAL::checkRange( float& aValue )
{
    if( aValue < 0.0 )
        aValue = 0.0;
    else if( aValue > 1.0 )
        aValue = 1.0;
}


void WRL1MATERIAL::Reclaim( SGNODE* aColor )
{
    if( nullptr == aColor )
        return;

    if( aColor == colors[0] )
    {
        if( nullptr == S3D::GetSGNodeParent( aColor ) )
        {
            colors[0] = nullptr;
            S3D::DestroyNode( aColor );
        }

        return;
    }

    if( aColor == colors[1] && nullptr == S3D::GetSGNodeParent( aColor ) )
    {
        colors[1] = nullptr;
        S3D::DestroyNode( aColor );
    }
}
