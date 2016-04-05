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
#include <sstream>
#include <wx/log.h>

#include "vrml1_base.h"
#include "vrml1_material.h"
#include "plugins/3dapi/ifsg_all.h"


WRL1MATERIAL::WRL1MATERIAL( NAMEREGISTER* aDictionary ) : WRL1NODE( aDictionary )
{
    colors[0] = NULL;
    colors[1] = NULL;
    m_Type = WRL1_MATERIAL;
    return;
}


WRL1MATERIAL::WRL1MATERIAL( NAMEREGISTER* aDictionary, WRL1NODE* aParent ) :
    WRL1NODE( aDictionary )
{
    colors[0] = NULL;
    colors[1] = NULL;
    m_Type = WRL1_MATERIAL;
    m_Parent = aParent;

    if( NULL != m_Parent )
        m_Parent->AddChildNode( this );

    return;
}


WRL1MATERIAL::~WRL1MATERIAL()
{
    #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 2 )
    wxLogTrace( MASK_VRML, " * [INFO] Destroying Material node\n" );
    #endif

    // destroy any orphaned color nodes
    for( int i = 0; i < 2; ++i )
    {
        if( NULL != colors[i] )
        {
            #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 2 )
            do {
                std::ostringstream ostr;
                ostr << " * [INFO] Destroying SGCOLOR #" << i;
                wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
            } while( 0 );
            #endif

            if( NULL == S3D::GetSGNodeParent( colors[i] ) )
                S3D::DestroyNode( colors[i] );

            #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 2 )
            do {
                std::ostringstream ostr;
                ostr << " * [INFO] destroyed SGCOLOR #" << i;
                wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
            } while( 0 );
            #endif
        }
    }

    return;
}


bool WRL1MATERIAL::AddRefNode( WRL1NODE* aNode )
{
    // this node may not own or reference any other node

    #ifdef DEBUG_VRML1
    do {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [BUG] AddRefNode is not applicable";
        wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
    } while( 0 );
    #endif

    return false;
}


bool WRL1MATERIAL::AddChildNode( WRL1NODE* aNode )
{
    // this node may not own or reference any other node

    #ifdef DEBUG_VRML1
    do {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [BUG] AddChildNode is not applicable";
        wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
    } while( 0 );
    #endif

    return false;
}


bool WRL1MATERIAL::Read( WRLPROC& proc, WRL1BASE* aTopNode )
{
    if( NULL == aTopNode )
    {
        #ifdef DEBUG_VRML1
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [BUG] aTopNode is NULL";
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return false;
    }

    size_t line, column;
    proc.GetFilePosData( line, column );

    char tok = proc.Peek();

    if( proc.eof() )
    {
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [INFO] bad file format; unexpected eof at line ";
            ostr << line << ", column " << column;
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return false;
    }

    if( '{' != tok )
    {
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
        do {
            std::ostringstream ostr;
            ostr << proc.GetError() << "\n";
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [INFO] bad file format; expecting '{' but got '" << tok;
            ostr << "' at line " << line << ", column " << column;
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

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
            #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
            do {
                std::ostringstream ostr;
                ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                ostr << proc.GetError();
                wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
            } while( 0 );
            #endif

            return false;
        }

        // expecting one of:
        // ambientColor
        // diffuseColor
        // emissiveColor
        // shininess
        // specularColor
        // transparency

        proc.GetFilePosData( line, column );

        if( !glob.compare( "specularColor" ) )
        {
            if( !proc.ReadMFVec3f( specularColor ) )
            {
                #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
                do {
                    std::ostringstream ostr;
                    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                    ostr << " * [INFO] invalid specularColor at line " << line << ", column ";
                    ostr << column << "\n";
                    ostr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                    ostr << " * [INFO] message: '" << proc.GetError() << "'";
                    wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
                } while( 0 );
                #endif

                return false;
            }
        }
        else if( !glob.compare( "diffuseColor" ) )
        {
            if( !proc.ReadMFVec3f( diffuseColor ) )
            {
                #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
                do {
                    std::ostringstream ostr;
                    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                    ostr << " * [INFO] invalid diffuseColor at line " << line << ", column ";
                    ostr << column << "\n";
                    ostr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                    ostr << " * [INFO] message: '" << proc.GetError() << "'";
                    wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
                } while( 0 );
                #endif

                return false;
            }
        }
        else if( !glob.compare( "emissiveColor" ) )
        {
            if( !proc.ReadMFVec3f( emissiveColor ) )
            {
                #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
                do {
                    std::ostringstream ostr;
                    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                    ostr << " * [INFO] invalid emissiveColor at line " << line << ", column ";
                    ostr << column << "\n";
                    ostr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                    ostr << " * [INFO] message: '" << proc.GetError() << "'";
                    wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
                } while( 0 );
                #endif

                return false;
            }
        }
        else if( !glob.compare( "shininess" ) )
        {
            if( !proc.ReadMFFloat( shininess ) )
            {
                #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
                do {
                    std::ostringstream ostr;
                    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                    ostr << " * [INFO] invalid shininess at line " << line << ", column ";
                    ostr << column << "\n";
                    ostr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                    ostr << " * [INFO] message: '" << proc.GetError() << "'";
                    wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
                } while( 0 );
                #endif

                return false;
            }
        }
        else if( !glob.compare( "transparency" ) )
        {
            if( !proc.ReadMFFloat( transparency ) )
            {
                #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
                do {
                    std::ostringstream ostr;
                    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                    ostr << " * [INFO] invalid transparency at line " << line << ", column ";
                    ostr << column << "\n";
                    ostr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                    ostr << " * [INFO] message: '" << proc.GetError() << "'";
                    wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
                } while( 0 );
                #endif

                return false;
            }
        }
        else if( !glob.compare( "ambientColor" ) )
        {
            if( !proc.ReadMFVec3f( ambientColor ) )
            {
                #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
                do {
                    std::ostringstream ostr;
                    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                    ostr << " * [INFO] invalid ambientColor at line " << line << ", column ";
                    ostr << column << "\n";
                    ostr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                    ostr << " * [INFO] message: '" << proc.GetError() << "'";
                    wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
                } while( 0 );
                #endif

                return false;
            }
        }
        else
        {
            #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
            do {
                std::ostringstream ostr;
                ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                ostr << " * [INFO] bad Material at line " << line << ", column ";
                ostr << column << "\n";
                ostr << " * [INFO] file: '" << proc.GetFileName() << "'";
                wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
            } while( 0 );
            #endif

            return false;
        }
    }   // while( true ) -- reading contents of Material{}

    return true;
}


SGNODE* WRL1MATERIAL::TranslateToSG( SGNODE* aParent, WRL1STATUS* sp )
{
    if( NULL == sp )
    {
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
        wxLogTrace( MASK_VRML, " * [INFO] bad model: no base data given\n" );
        #endif

        return NULL;
    }

    sp->mat = this;

    return NULL;
}


SGNODE* WRL1MATERIAL::GetAppearance( int aIndex )
{
    ++aIndex;

    // invalid indices result in the default colors
    if( aIndex != 0 && aIndex != 1 )
        aIndex = 0;

    if( NULL != colors[ aIndex ] )
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
        val = 0.2;
    else
        val = shininess[0];

    checkRange( val );
    app.SetShininess( val );

    if( aIndex ==0 || ambientColor.empty() )
    {
        red = 0.2;
        green = 0.2;
        blue = 0.2;
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
        red = 0.8;
        green = 0.8;
        blue = 0.8;
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
    if( NULL == aColor )
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
            dRed = 0.8;
            dGreen = 0.8;
            dBlue = 0.8;
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
            eRed = 0.0;
            eGreen = 0.0;
            eBlue = 0.0;
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
            aRed = 0.2;
            aGreen = 0.2;
            aBlue = 0.2;
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
            sRed = 0.2;
            sGreen = 0.2;
            sBlue = 0.2;
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
            shiny = 0.2;
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

    if( aRed + aGreen + aBlue > 0.01 )
        ++n;

    if( eRed + eGreen + eBlue > 0.01 )
        ++n;

    if( dRed + dGreen + dBlue > 0.01 )
        ++n;

    if( (sRed + sGreen + sBlue) * shiny > 0.01 )
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

    return;
}


void WRL1MATERIAL::checkRange( float& aValue )
{
    if( aValue < 0.0 )
        aValue = 0.0;
    else if( aValue > 1.0 )
        aValue = 1.0;

    return;
}


void WRL1MATERIAL::Reclaim( SGNODE* aColor )
{
    if( NULL == aColor )
        return;

    if( aColor == colors[0] )
    {
        if( NULL == S3D::GetSGNodeParent( aColor ) )
        {
            colors[0] = NULL;
            S3D::DestroyNode( aColor );
        }

        return;
    }

    if( aColor == colors[1] && NULL == S3D::GetSGNodeParent( aColor ) )
    {
        colors[1] = NULL;
        S3D::DestroyNode( aColor );
    }

    return;
}
