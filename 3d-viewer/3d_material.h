/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Mario Luzeiro <mrluzeiro@gmail.com>
 * Copyright (C) 1992-2014 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file 3d_material.h
 */

#ifndef STRUCT_3D_MATERIAL_H
#define STRUCT_3D_MATERIAL_H

#include <common.h>
#include <base_struct.h>
#include <gal/opengl/glm/glm.hpp>

class S3D_MASTER;

class S3D_MATERIAL : public EDA_ITEM       // openGL "material" data
{
public:
    wxString   m_Name;

    // Material list
    std::vector< glm::vec3 > m_AmbientColor;
    std::vector< glm::vec3 > m_DiffuseColor;
    std::vector< glm::vec3 > m_EmissiveColor;
    std::vector< glm::vec3 > m_SpecularColor;
    std::vector< float > m_Shininess;
    std::vector< float > m_Transparency;

public:
    S3D_MATERIAL( S3D_MASTER* father, const wxString& name );

    S3D_MATERIAL* Next() const { return (S3D_MATERIAL*) Pnext; }
    S3D_MATERIAL* Back() const { return (S3D_MATERIAL*) Pback; }

    /**
     * Initialize the material prms.
     * @param aMaterialIndex = the index in list of available materials
     * @param aUseMaterial = true to use the values found in the available material
     *                     = false to use only the color, and other prms are fixed
     * @return true if the material is transparency
     */
    bool SetOpenGLMaterial(unsigned int aMaterialIndex, bool aUseMaterial);

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const { ShowDummy( os ); } // override
#endif

    /** Get class name
     * @return  string "S3D_MATERIAL"
     */
    virtual wxString GetClass() const
    {
        return wxT( "S3D_MATERIAL" );
    }
};

void SetOpenGlDefaultMaterial();

#endif
