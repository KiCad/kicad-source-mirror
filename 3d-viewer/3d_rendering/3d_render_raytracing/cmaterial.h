/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Mario Luzeiro <mrluzeiro@ua.pt>
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

/**
 * @file  cmaterial.h
 * @brief
 */

#ifndef _CMATERIAL_H_
#define _CMATERIAL_H_

#include "ray.h"
#include "hitinfo.h"

///
class GLM_ALIGN(CLASS_ALIGNMENT) CMATERIAL
{
public:
    CMATERIAL();
    CMATERIAL( const SFVEC3F &aAmbient, const SFVEC3F &aEmissive, const SFVEC3F &aSpecular, float aShinness, float aTransparency );

    const SFVEC3F &GetAmbientColor()  const { return m_ambientColor;  }
    const SFVEC3F &GetEmissiveColor() const { return m_emissiveColor; }
    const SFVEC3F &GetSpecularColor() const { return m_specularColor; }

    float GetShinness()     const { return m_shinness;     }
    float GetTransparency() const { return m_transparency; }

    void SetCastShadows( bool aCastShadows ) { m_cast_shadows  = aCastShadows; }
    bool GetCastShadows() const { return m_cast_shadows; }

    virtual SFVEC3F Shade( const RAY &aRay, const HITINFO &aHitInfo, float NdotL, const SFVEC3F &aDiffuseObjColor, const SFVEC3F &aDirToLight, const SFVEC3F &aLightColor, bool aIsInShadow ) const = 0;

/*


    virtual SFVEC3F GetReflect(Vec3f point){ return reflectiveColor;}
    virtual Vec3f GetTrans(Vec3f point){ return transparentColor;}
    virtual float getIndexOfRefrac(Vec3f point){ return indexOfRefraction;}

    virtual bool isReflect(Vec3f point){return ((reflectiveColor.x()>0)||(reflectiveColor.y()>0)||(reflectiveColor.z()>0));}
    virtual bool isTransparent(Vec3f point){return ((transparentColor.x()>0)||(transparentColor.y()>0)||(transparentColor.z()>0));}
*/
protected:
    SFVEC3F m_ambientColor;
    // NOTE: we will not use diffuse color material here,
    // because it will be stored in object, since there are objects (i.e: triangles)
    // that can have per vertex color

    SFVEC3F m_emissiveColor;
    SFVEC3F m_specularColor;
    float   m_shinness;
    float   m_transparency;     ///< 1.0 is completely transparent, 0.0 completely opaque
    bool    m_cast_shadows;     ///< true if this object will block the light
};


/// https://en.wikipedia.org/wiki/Blinn%E2%80%93Phong_shading_model
class GLM_ALIGN(CLASS_ALIGNMENT) CBLINN_PHONG_MATERIAL : public CMATERIAL
{
public:
    CBLINN_PHONG_MATERIAL() : CMATERIAL() {}

    CBLINN_PHONG_MATERIAL( const SFVEC3F &aAmbient,
                           const SFVEC3F &aEmissive,
                           const SFVEC3F &aSpecular,
                           float aShinness,
                           float aTransparency ) :
    CMATERIAL( aAmbient, aEmissive, aSpecular, aShinness, aTransparency ) {}

    SFVEC3F Shade( const RAY &aRay, const HITINFO &aHitInfo, float NdotL, const SFVEC3F &aDiffuseObjColor, const SFVEC3F &aDirToLight, const SFVEC3F &aLightColor, bool aIsInShadow ) const;
};

#endif // _CMATERIAL_H_
