/**
 * @file 3d_class.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <fctsys.h>

#include <3d_viewer.h>
#include <3d_struct.h>


bool S3D_MASTER::IsOpenGlAllowed()
{
    if( m_loadNonTransparentObjects )    // return true for non transparent objects only
    {
        if( m_lastTransparency == 0.0 )
            return true;
    }

    if( m_loadTransparentObjects )      // return true for transparent objects only
    {
        if( m_lastTransparency != 0.0 )
            return true;
    }

    return false;
}


void S3D_MASTER::Insert( S3D_MATERIAL* aMaterial )
{
    aMaterial->SetNext( m_Materials );
    m_Materials = aMaterial;
}


void S3D_MASTER::Copy( S3D_MASTER* pattern )
{
    SetShape3DName( pattern->GetShape3DName() );
    m_MatScale    = pattern->m_MatScale;
    m_MatRotation = pattern->m_MatRotation;
    m_MatPosition = pattern->m_MatPosition;
    m_3D_Drawings = NULL;
    m_Materials   = NULL;
}


S3D_MASTER::S3D_MASTER( EDA_ITEM* aParent ) :
    EDA_ITEM( aParent, NOT_USED )
{
    m_MatScale.x  = m_MatScale.y = m_MatScale.z = 1.0;
    m_lastTransparency = 0.0;
    m_3D_Drawings = NULL;
    m_Materials   = NULL;
    m_ShapeType   = FILE3D_NONE;

    m_use_modelfile_diffuseColor = true;
    m_use_modelfile_emissiveColor = true;
    m_use_modelfile_specularColor = true;
    m_use_modelfile_ambientIntensity = true;
    m_use_modelfile_transparency = true;
    m_use_modelfile_shininess = true;
    m_loadTransparentObjects = true;
    m_loadNonTransparentObjects = true;
}


S3D_MASTER:: ~S3D_MASTER()
{
    STRUCT_3D_SHAPE* next;
    S3D_MATERIAL*   nextmat;

    for( ; m_3D_Drawings != NULL; m_3D_Drawings = next )
    {
        next = m_3D_Drawings->Next();
        delete m_3D_Drawings;
    }

    for( ; m_Materials != NULL; m_Materials = nextmat )
    {
        nextmat = m_Materials->Next();
        delete m_Materials;
    }
}


bool S3D_MASTER::Is3DType( enum FILE3D_TYPE aShapeType )
{
    // type 'none' is not valid and will always return false
    if( aShapeType == FILE3D_NONE )
        return false;

    // no one is interested if we have no file
    if( m_Shape3DName.empty() )
        return false;

    if( aShapeType == m_ShapeType )
        return true;

    return false;
}


void S3D_MASTER::SetShape3DName( const wxString& aShapeName )
{
    m_ShapeType = FILE3D_NONE;
    m_Shape3DName = aShapeName;

    if( m_Shape3DName.empty() )
        return;

    wxFileName fn = m_Shape3DName;
    wxString ext  = fn.GetExt();

    if( ext == wxT( "wrl" ) || ext == wxT( "x3d" ) )
        m_ShapeType = FILE3D_VRML;
    else if( ext == wxT( "idf" ) )
        m_ShapeType = FILE3D_IDF;
    else
        m_ShapeType = FILE3D_UNKNOWN;

    return;
}


STRUCT_3D_SHAPE::STRUCT_3D_SHAPE( EDA_ITEM* aParent ) :
    EDA_ITEM( aParent, NOT_USED )
{
    m_3D_Coord = NULL;
    m_3D_CoordIndex = NULL;
    m_3D_Points     = 0;
}


STRUCT_3D_SHAPE:: ~STRUCT_3D_SHAPE()
{
    delete m_3D_Coord;
    delete m_3D_CoordIndex;
}
