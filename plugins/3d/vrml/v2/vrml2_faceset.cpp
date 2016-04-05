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

#include "vrml2_base.h"
#include "vrml2_faceset.h"
#include "vrml2_coords.h"
#include "vrml2_color.h"
#include "wrlfacet.h"
#include "plugins/3dapi/ifsg_all.h"


WRL2FACESET::WRL2FACESET() : WRL2NODE()
{
    setDefaults();
    m_Type = WRL2_INDEXEDFACESET;

    return;
}


WRL2FACESET::WRL2FACESET( WRL2NODE* aParent ) : WRL2NODE()
{
    setDefaults();
    m_Type = WRL2_INDEXEDFACESET;
    m_Parent = aParent;

    if( NULL != m_Parent )
        m_Parent->AddChildNode( this );

    return;
}


WRL2FACESET::~WRL2FACESET()
{
    #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 2 )
    do {
        std::ostringstream ostr;
        ostr << " * [INFO] Destroying IndexedFaceSet with " << m_Children.size();
        ostr << " children, " << m_Refs.size() << " references and ";
        ostr << m_BackPointers.size() << " backpointers";
        wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
    } while( 0 );
    #endif

    return;
}


void WRL2FACESET::setDefaults( void )
{
    color = NULL;
    coord = NULL;
    normal = NULL;
    texCoord = NULL;

    ccw = true;
    colorPerVertex = true;
    convex = true;
    normalPerVertex = true;
    solid = true;

    creaseAngle = 0.733;    // approx 42 degrees; this is larger than VRML spec.
    creaseLimit = 0.74317;  // cos( 0.733 )
}


bool WRL2FACESET::checkNodeType( WRL2NODES aType )
{
    // nodes must be one of:
    // Color
    // Coordinate
    // Normal
    // TextureCoordinate

    switch( aType )
    {
    case WRL2_COLOR:
    case WRL2_COORDINATE:
    case WRL2_NORMAL:
    case WRL2_TEXTURECOORDINATE:
        break;

    default:
        return false;
        break;
    }

    return true;
}


bool WRL2FACESET::isDangling( void )
{
    // this node is dangling unless it has a parent of type WRL2_SHAPE

    if( NULL == m_Parent || m_Parent->GetNodeType() != WRL2_SHAPE )
        return true;

    return false;
}


bool WRL2FACESET::AddRefNode( WRL2NODE* aNode )
{
    if( NULL == aNode )
    {
        #ifdef DEBUG_VRML2
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [BUG] NULL passed for aNode";
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return false;
    }

    WRL2NODES type = aNode->GetNodeType();

    if( !checkNodeType( type ) )
    {
        #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [INFO] bad file format; unexpected child node '";
            ostr << aNode->GetNodeTypeName( type ) << "'";
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return false;
    }

    if( WRL2_COLOR == type )
    {
        if( NULL != color )
        {
            #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
            do {
                std::ostringstream ostr;
                ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                ostr << " * [INFO] bad file format; multiple color nodes";
                wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
            } while( 0 );
            #endif

            return false;
        }

        color = aNode;
        return WRL2NODE::AddRefNode( aNode );
    }

    if( WRL2_COORDINATE == type )
    {
        if( NULL != coord )
        {
            #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
            do {
                std::ostringstream ostr;
                ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                ostr << " * [INFO] bad file format; multiple coordinate nodes";
                wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
            } while( 0 );
            #endif

            return false;
        }

        coord = aNode;
        return WRL2NODE::AddRefNode( aNode );
    }

    if( WRL2_NORMAL == type )
    {
        if( NULL != normal )
        {
            #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
            do {
                std::ostringstream ostr;
                ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                ostr << " * [INFO] bad file format; multiple normal nodes";
                wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
            } while( 0 );
            #endif

            return false;
        }

        normal = aNode;
        return WRL2NODE::AddRefNode( aNode );
    }

    if( WRL2_TEXTURECOORDINATE != type )
    {
        #ifdef DEBUG_VRML2
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [BUG] unexpected code branch";
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return false;
    }

    if( NULL != texCoord )
    {
        #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [INFO] bad file format; multiple texCoord nodes";
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return false;
    }

    texCoord = aNode;
    return WRL2NODE::AddRefNode( aNode );
}


bool WRL2FACESET::AddChildNode( WRL2NODE* aNode )
{
    if( NULL == aNode )
    {
        #ifdef DEBUG_VRML2
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [BUG] NULL passed for aNode";
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return false;
    }

    WRL2NODES type = aNode->GetNodeType();

    if( !checkNodeType( type ) )
    {
        #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [INFO] bad file format; unexpected child node '";
            ostr << aNode->GetNodeTypeName( type ) << "'";
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return false;
    }

    if( WRL2_COLOR == type )
    {
        if( NULL != color )
        {
            #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
            do {
                std::ostringstream ostr;
                ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                ostr << " * [INFO] bad file format; multiple color nodes";
                wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
            } while( 0 );
            #endif

            return false;
        }

        color = aNode;
        return WRL2NODE::AddChildNode( aNode );
    }

    if( WRL2_COORDINATE == type )
    {
        if( NULL != coord )
        {
            #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
            do {
                std::ostringstream ostr;
                ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                ostr << " * [INFO] bad file format; multiple coordinate nodes";
                wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
            } while( 0 );
            #endif

            return false;
        }

        coord = aNode;
        return WRL2NODE::AddChildNode( aNode );
    }

    if( WRL2_NORMAL == type )
    {
        if( NULL != normal )
        {
            #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
            do {
                std::ostringstream ostr;
                ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                ostr << " * [INFO] bad file format; multiple normal nodes";
                wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
            } while( 0 );
            #endif

            return false;
        }

        normal = aNode;
        return WRL2NODE::AddChildNode( aNode );
    }

    if( WRL2_TEXTURECOORDINATE != type )
    {
        #ifdef DEBUG_VRML2
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [BUG] unexpected code branch";
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return false;
    }

    if( NULL != texCoord )
    {
        #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [INFO] bad file format; multiple texCoord nodes";
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return false;
    }

    texCoord = aNode;
    return WRL2NODE::AddChildNode( aNode );
}



bool WRL2FACESET::Read( WRLPROC& proc, WRL2BASE* aTopNode )
{
    size_t line, column;
    proc.GetFilePosData( line, column );

    char tok = proc.Peek();

    if( proc.eof() )
    {
        #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
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
        #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
        do {
            std::ostringstream ostr;
            ostr << proc.GetError() << "\n";
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [INFO] bad file format; expecting '{' but got '" << tok;
            ostr  << "' at line " << line << ", column " << column;
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
            #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
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
        // [node]
        // color
        // coord
        // normal
        // texCoord
        // [bool]
        // ccw
        // colorPerVertex
        // convex
        // normalPerVertex
        // solid
        // [ vector<int> ]
        // colorIndex
        // coordIndex
        // normalIndex;
        // [float]
        // creaseAngle

        proc.GetFilePosData( line, column );

        if( !glob.compare( "ccw" ) )
        {
            if( !proc.ReadSFBool( ccw ) )
            {
                #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
                do {
                    std::ostringstream ostr;
                    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                    ostr << " * [INFO] invalid ccw at line " << line << ", column ";
                    ostr << column << "\n";
                    ostr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                    ostr << " * [INFO] message: '" << proc.GetError() << "'";
                    wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
                } while( 0 );
                #endif

                return false;
            }
        }
        else if( !glob.compare( "colorPerVertex" ) )
        {
            if( !proc.ReadSFBool( colorPerVertex ) )
            {
                #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
                do {
                    std::ostringstream ostr;
                    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                    ostr << " * [INFO] invalid colorPerVertex at line " << line << ", column ";
                    ostr << column << "\n";
                    ostr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                    ostr << " * [INFO] message: '" << proc.GetError() << "'";
                    wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
                } while( 0 );
                #endif

                return false;
            }
        }
        else if( !glob.compare( "convex" ) )
        {
            if( !proc.ReadSFBool( convex ) )
            {
                #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
                do {
                    std::ostringstream ostr;
                    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                    ostr << " * [INFO] invalid convex at line " << line << ", column ";
                    ostr << column << "\n";
                    ostr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                    ostr << " * [INFO] message: '" << proc.GetError() << "'";
                    wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
                } while( 0 );
                #endif

                return false;
            }
        }
        else if( !glob.compare( "normalPerVertex" ) )
        {
            if( !proc.ReadSFBool( normalPerVertex ) )
            {
                #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
                do {
                    std::ostringstream ostr;
                    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                    ostr << " * [INFO] invalid normalPerVertex at line " << line << ", column ";
                    ostr << column << "\n";
                    ostr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                    ostr << " * [INFO] message: '" << proc.GetError() << "'";
                    wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
                } while( 0 );
                #endif

                return false;
            }
        }
        else if( !glob.compare( "solid" ) )
        {
            if( !proc.ReadSFBool( solid ) )
            {
                #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
                do {
                    std::ostringstream ostr;
                    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                    ostr << " * [INFO] invalid solid at line " << line << ", column ";
                    ostr << column << "\n";
                    ostr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                    ostr << " * [INFO] message: '" << proc.GetError() << "'";
                    wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
                } while( 0 );
                #endif

                return false;
            }
        }
        else if( !glob.compare( "creaseAngle" ) )
        {
            if( !proc.ReadSFFloat( creaseAngle ) )
            {
                #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
                do {
                    std::ostringstream ostr;
                    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                    ostr << " * [INFO] invalid creaseAngle at line " << line << ", column ";
                    ostr << column << "\n";
                    ostr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                    ostr << " * [INFO] message: '" << proc.GetError() << "'\n";
                    wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
                } while( 0 );
                #endif

                return false;
            }

            if( creaseAngle < 0.0 )
                creaseAngle = 0.0;
            else if( creaseAngle > M_PI_2 )
                creaseAngle = M_PI_2;

            creaseLimit = cosf( creaseAngle );
        }
        else if( !glob.compare( "colorIndex" ) )
        {
            if( !proc.ReadMFInt( colorIndex ) )
            {
                #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
                do {
                    std::ostringstream ostr;
                    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                    ostr << " * [INFO] invalid colorIndex at line " << line << ", column ";
                    ostr << column << "\n";
                    ostr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                    ostr << " * [INFO] message: '" << proc.GetError() << "'";
                    wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
                } while( 0 );
                #endif

                return false;
            }
        }
        else if( !glob.compare( "coordIndex" ) )
        {
            if( !proc.ReadMFInt( coordIndex ) )
            {
                #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
                do {
                    std::ostringstream ostr;
                    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                    ostr << " * [INFO] invalid coordIndex at line " << line << ", column ";
                    ostr << column << "\n";
                    ostr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                    ostr << " * [INFO] message: '" << proc.GetError() << "'";
                    wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
                } while( 0 );
                #endif

                return false;
            }
        }
        else if( !glob.compare( "normalIndex" ) )
        {
            if( !proc.ReadMFInt( normalIndex ) )
            {
                #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
                do {
                    std::ostringstream ostr;
                    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                    ostr << " * [INFO] invalid normalIndex at line " << line << ", column ";
                    ostr << column << "\n";
                    ostr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                    ostr << " * [INFO] message: '" << proc.GetError() << "'";
                    wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
                } while( 0 );
                #endif

                return false;
            }
        }
        else if( !glob.compare( "color" ) )
        {
            if( !aTopNode->ReadNode( proc, this, NULL ) )
            {
                #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
                do {
                    std::ostringstream ostr;
                    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                    ostr << " * [INFO] could not read color node information";
                    wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
                } while( 0 );
                #endif

                return false;
            }
        }
        else if( !glob.compare( "coord" ) )
        {
            if( !aTopNode->ReadNode( proc, this, NULL ) )
            {
                #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
                do {
                    std::ostringstream ostr;
                    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                    ostr << " * [INFO] could not read coord node information";
                    wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
                } while( 0 );
                #endif

                return false;
            }
        }
        else if( !glob.compare( "normal" ) )
        {
            if( !aTopNode->ReadNode( proc, this, NULL ) )
            {
                #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
                do {
                    std::ostringstream ostr;
                    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                    ostr << " * [INFO] could not read normal node information";
                    wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
                } while( 0 );
                #endif

                return false;
            }
        }
        else if( !glob.compare( "texCoord" ) )
        {
            if( !aTopNode->ReadNode( proc, this, NULL ) )
            {
                #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
                do {
                    std::ostringstream ostr;
                    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                    ostr << " * [INFO] could not read texCoord node information";
                    wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
                } while( 0 );
                #endif

                return false;
            }
        }
        else
        {
            #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
            do {
                std::ostringstream ostr;
                ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                ostr << " * [INFO] bad IndexedFaceSet at line " << line << ", column ";
                ostr << column << "\n";
                ostr << " * [INFO] file: '" << proc.GetFileName() << "'";
                wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
            } while( 0 );
            #endif

            return false;
        }
    }   // while( true ) -- reading contents of IndexedFaceSet{}

    return true;
}


SGNODE* WRL2FACESET::TranslateToSG( SGNODE* aParent )
{
    S3D::SGTYPES ptype = S3D::GetSGNodeType( aParent );

    if( NULL != aParent && ptype != S3D::SGTYPE_SHAPE )
    {
        #ifdef DEBUG_VRML2
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [BUG] IndexedFaceSet does not have a Shape parent (parent ID: ";
            ostr << ptype << ")";
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return NULL;
    }

    #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 2 )
    do {
        std::ostringstream ostr;
        ostr << " * [INFO] Translating IndexedFaceSet with " << m_Children.size();
        ostr << " children, " << m_Refs.size() << " references, ";
        ostr << m_BackPointers.size() << " backpointers and ";
        ostr << coordIndex.size() << " coord indices";
        wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
    } while( 0 );
    #endif

    if( m_sgNode )
    {
        if( NULL != aParent )
        {
            if( NULL == S3D::GetSGNodeParent( m_sgNode )
                && !S3D::AddSGNodeChild( aParent, m_sgNode ) )
            {
                return NULL;
            }
            else if( aParent != S3D::GetSGNodeParent( m_sgNode )
                     && !S3D::AddSGNodeRef( aParent, m_sgNode ) )
            {
                return NULL;
            }
        }

        return m_sgNode;
    }

    size_t vsize = coordIndex.size();

    if( NULL == coord || vsize < 3 )
        return NULL;

    WRLVEC3F* pcoords;
    size_t coordsize;
    ((WRL2COORDS*) coord)->GetCoords( pcoords, coordsize );

    if( coordsize < 3 )
        return NULL;

    // check that all indices are valid
    for( size_t idx = 0; idx < vsize; ++idx )
    {
        if( coordIndex[idx] < 0 )
            continue;

        if( coordIndex[idx] >= (int)coordsize )
            return NULL;
    }

    SHAPE   lShape;
    FACET*  fp = NULL;
    size_t  iCoord;
    int     idx;        // coordinate index
    size_t  cidx = 0;   // color index
    SGCOLOR pc1;

    if( NULL == color )
    {
        // no per-vertex colors; we can save a few CPU cycles
        for( iCoord = 0; iCoord < vsize; ++iCoord )
        {
            idx = coordIndex[iCoord];

            if( idx < 0 )
            {
                if( NULL != fp )
                {
                    if( fp->HasMinPoints() )
                        fp = NULL;
                    else
                        fp->Init();
                }

                continue;
            }

            // if the coordinate is bad then skip it
            if( idx >= (int)coordsize )
                continue;

            if( NULL == fp )
                fp = lShape.NewFacet();

            // push the vertex value and index
            fp->AddVertex( pcoords[idx], idx );
        }
    }
    else
    {
        WRL2COLOR* cn = (WRL2COLOR*) color;
        WRLVEC3F tc;

        for( iCoord = 0; iCoord < vsize; ++iCoord )
        {
            idx = coordIndex[iCoord];

            if( idx < 0 )
            {
                if( NULL != fp )
                {
                    if( fp->HasMinPoints() )
                        fp = NULL;
                    else
                        fp->Init();
                }

                if( !colorPerVertex )
                    ++cidx;

                continue;
            }

            // if the coordinate is bad then skip it
            if( idx >= (int)coordsize )
                continue;

            if( NULL == fp )
                fp = lShape.NewFacet();

            // push the vertex value and index
            fp->AddVertex( pcoords[idx], idx );

            // push the color if appropriate
            if( !colorPerVertex )
            {
                if( colorIndex.empty() )
                {
                    cn->GetColor( cidx, tc.x, tc.y, tc.z );
                    pc1.SetColor( tc.x, tc.y, tc.z );
                    fp->AddColor( pc1 );
                }
                else
                {
                    if( cidx < colorIndex.size() )
                        cn->GetColor( colorIndex[cidx], tc.x, tc.y, tc.z );
                    else
                        cn->GetColor( colorIndex.back(), tc.x, tc.y, tc.z );

                    pc1.SetColor( tc.x, tc.y, tc.z );
                    fp->AddColor( pc1 );
                }
            }
            else
            {
                if( colorIndex.empty() )
                {
                    cn->GetColor( idx, tc.x, tc.y, tc.z );
                    pc1.SetColor( tc.x, tc.y, tc.z );
                    fp->AddColor( pc1 );
                }
                else
                {
                    if( iCoord < colorIndex.size() )
                        cn->GetColor( colorIndex[iCoord], tc.x, tc.y, tc.z );
                    else
                        cn->GetColor( colorIndex.back(), tc.x, tc.y, tc.z );

                    pc1.SetColor( tc.x, tc.y, tc.z );
                    fp->AddColor( pc1 );
                }
            }
        }
    }

    SGNODE* np = NULL;

    if( ccw )
        np = lShape.CalcShape( aParent, NULL, ORD_CCW, creaseLimit, true );
    else
        np = lShape.CalcShape( aParent, NULL, ORD_CLOCKWISE, creaseLimit, true );

    return np;
}


void WRL2FACESET::unlinkChildNode( const WRL2NODE* aNode )
{
    if( NULL == aNode )
        return;

    if( aNode->GetParent() == this )
    {
        if( aNode == color )
            color = NULL;
        else if( aNode == coord )
            coord = NULL;
        else if( aNode == normal )
            normal = NULL;
        else if( aNode == texCoord )
            texCoord = NULL;

    }

    WRL2NODE::unlinkChildNode( aNode );
    return;
}


void WRL2FACESET::unlinkRefNode( const WRL2NODE* aNode )
{
    if( NULL == aNode )
        return;

    if( aNode->GetParent() != this )
    {
        if( aNode == color )
            color = NULL;
        else if( aNode == coord )
            coord = NULL;
        else if( aNode == normal )
            normal = NULL;
        else if( aNode == texCoord )
            texCoord = NULL;

    }

    WRL2NODE::unlinkRefNode( aNode );
    return;
}


bool WRL2FACESET::HasColors( void )
{
    if( NULL == color )
        return false;

    return ((WRL2COLOR*) color)->HasColors();
}
