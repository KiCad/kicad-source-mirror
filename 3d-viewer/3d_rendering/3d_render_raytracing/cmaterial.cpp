/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file  cmaterial.cpp
 * @brief
 */

#include "cmaterial.h"
#include <wx/debug.h>


CMATERIAL::CMATERIAL()
{
    m_ambientColor  = SFVEC3F( 0.2f, 0.2f, 0.2f );
    m_emissiveColor = SFVEC3F( 0.0f, 0.0f, 0.0f );
    m_specularColor = SFVEC3F( 1.0f, 1.0f, 1.0f );
    m_shinness      = 50.2f;
    m_transparency  = 0.0f; // completely opaque
    m_cast_shadows  = true;
    m_reflection    = 0.0f;
}


CMATERIAL::CMATERIAL( const SFVEC3F &aAmbient,
                      const SFVEC3F &aEmissive,
                      const SFVEC3F &aSpecular,
                      float aShinness,
                      float aTransparency,
                      float aReflection )
{
    wxASSERT( aReflection >= 0.0f );
    wxASSERT( aReflection <= 1.0f );

    wxASSERT( aTransparency >= 0.0f );
    wxASSERT( aTransparency <= 1.0f );

    wxASSERT( aShinness >= 0.0f );
    wxASSERT( aShinness <= 180.0f );

    m_ambientColor  = aAmbient;
    m_emissiveColor = aEmissive;
    m_specularColor = aSpecular;
    m_shinness      = aShinness;
    m_transparency  = aTransparency;
    m_reflection    = aReflection;
    m_cast_shadows  = true;
}


// This may be a good value if based on nr of lights
// that contribute to the illumination of that point
#define AMBIENT_FACTOR  0.160f
#define SPECULAR_FACTOR 1.000f

// https://en.wikipedia.org/wiki/Blinn%E2%80%93Phong_shading_model
SFVEC3F CBLINN_PHONG_MATERIAL::Shade( const RAY &aRay,
                                      const HITINFO &aHitInfo,
                                      float NdotL,
                                      const SFVEC3F &aDiffuseObjColor,
                                      const SFVEC3F &aDirToLight,
                                      const SFVEC3F &aLightColor,
                                      float aShadowAttenuationFactor ) const
{
    wxASSERT( NdotL >= FLT_EPSILON );

    //const float ambientFactor = AMBIENT_FACTOR;

    // This is a hack to get some kind of fake ambient illumination
    // There is no logic behind this, just pure artistic experimentation
    const float ambientFactor = glm::max( ( (1.0f - NdotL) /** (1.0f - NdotL)*/ ) *
                                          ( AMBIENT_FACTOR + AMBIENT_FACTOR ),
                                          AMBIENT_FACTOR );

    if( aShadowAttenuationFactor > FLT_EPSILON )
    {
        // Calculate the diffuse light factoring in light color,
        // power and the attenuation
        const SFVEC3F diffuse = NdotL * aLightColor;

        // Calculate the half vector between the light vector and the view vector.
        const SFVEC3F H = glm::normalize( aDirToLight - aRay.m_Dir );

        //Intensity of the specular light
        const float NdotH = glm::dot( H, aHitInfo.m_HitNormal );
        const float intensitySpecular = glm::pow( glm::max( NdotH, 0.0f ),
                                                  m_shinness );

        return  m_ambientColor * ambientFactor +
                aShadowAttenuationFactor * ( diffuse * aDiffuseObjColor +
                                             SPECULAR_FACTOR *
                                             aLightColor *
                                             intensitySpecular *
                                             m_specularColor );
    }

    return m_ambientColor * ambientFactor;
}
