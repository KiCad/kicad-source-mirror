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
#include <3d_math.h>
#include <wx/debug.h>

// This may be a good value if based on nr of lights
// that contribute to the illumination of that point
#define AMBIENT_FACTOR  (1.0f / 6.0f)
#define SPECULAR_FACTOR 1.0f

CMATERIAL::CMATERIAL()
{
    m_ambientColor  = SFVEC3F( 0.2f, 0.2f, 0.2f );
    m_emissiveColor = SFVEC3F( 0.0f, 0.0f, 0.0f );
    m_specularColor = SFVEC3F( 1.0f, 1.0f, 1.0f );
    m_shinness      = 50.2f;
    m_transparency  = 0.0f; // completely opaque
    m_cast_shadows  = true;
    m_reflection    = 0.0f;
    m_absorbance    = 1.0f;

    m_normal_perturbator = NULL;
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

    m_ambientColor  = aAmbient * SFVEC3F(AMBIENT_FACTOR);

    m_emissiveColor = aEmissive;
    m_specularColor = aSpecular;
    m_shinness      = aShinness;
    m_transparency  = aTransparency;
    m_absorbance    = 1.0f;
    m_reflection    = aReflection;
    m_cast_shadows  = true;

    m_normal_perturbator = NULL;
}


void CMATERIAL::PerturbeNormal( SFVEC3F &aNormal,
                                const RAY &aRay,
                                const HITINFO &aHitInfo ) const
{
    if( m_normal_perturbator )
    {
        aNormal = aNormal + m_normal_perturbator->Generate( aRay, aHitInfo );
        aNormal = glm::normalize( aNormal );
    }
}


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

    // This is a hack to get some kind of fake ambient illumination
    // There is no logic behind this, just pure artistic experimentation
    //const float ambientFactor = glm::max( ( (1.0f - NdotL) /** (1.0f - NdotL)*/ ) *
    //                                      ( AMBIENT_FACTOR + AMBIENT_FACTOR ),
    //                                      AMBIENT_FACTOR );

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

        return  m_ambientColor +
                aShadowAttenuationFactor * ( diffuse * aDiffuseObjColor +
                                             SPECULAR_FACTOR *
                                             aLightColor *
                                             intensitySpecular *
                                             m_specularColor );
    }

    return m_ambientColor;
}


CPROCEDURALGENERATOR::CPROCEDURALGENERATOR()
{
}


CBOARDNORMAL::CBOARDNORMAL( float aScale ) : CPROCEDURALGENERATOR()
{
    m_scale = (2.0f * glm::pi<float>()) / aScale;
}


SFVEC3F CBOARDNORMAL::Generate( const RAY &aRay, const HITINFO &aHitInfo ) const
{
    const SFVEC3F &hitPos = aHitInfo.m_HitPoint;

    // http://www.fooplot.com/#W3sidHlwZSI6MCwiZXEiOiJzaW4oc2luKHNpbih4KSoxLjkpKjEuNSkiLCJjb2xvciI6IiMwMDAwMDAifSx7InR5cGUiOjEwMDAsIndpbmRvdyI6WyItMC45NjIxMDU3MDgwNzg1MjYyIiwiNy45NzE0MjYyNjc2MDE0MyIsIi0yLjUxNzYyMDM1MTQ4MjQ0OSIsIjIuOTc5OTM3Nzg3Mzk3NTMwMyJdLCJzaXplIjpbNjQ2LDM5Nl19XQ--

    // Implement a texture as the "measling crazing blistering" method of FR4

    const float x = (glm::sin(glm::sin( glm::sin( hitPos.x * m_scale ) * 1.9f ) * 1.5f ) + 0.0f) * 0.10f;
    const float y = (glm::sin(glm::sin( glm::sin( hitPos.y * m_scale ) * 1.9f ) * 1.5f ) + 0.0f) * 0.10f;
    const float z = glm::sin( 2.0f * hitPos.z * m_scale + Fast_RandFloat() * 1.0f ) * 0.2f;

    return SFVEC3F( x, y, z );
}


CCOPPERNORMAL::CCOPPERNORMAL( float aScale, const CPROCEDURALGENERATOR *aBoardNormalGenerator )
{
    m_board_normal_generator = aBoardNormalGenerator;
    m_copper_perlin = PerlinNoise( 0 );
    m_scale = 1.0f / aScale;
}


SFVEC3F CCOPPERNORMAL::Generate( const RAY &aRay, const HITINFO &aHitInfo ) const
{
    if( m_board_normal_generator )
    {
        const SFVEC3F boardNormal = m_board_normal_generator->Generate( aRay, aHitInfo );

        SFVEC3F hitPos = aHitInfo.m_HitPoint * m_scale;

        const float noise = (m_copper_perlin.noise( hitPos.x + Fast_RandFloat() * 0.1f,
                                                    hitPos.y ) - 0.5f) * 2.0f;

        float scratchPattern = (m_copper_perlin.noise( hitPos.x / 100.0f, hitPos.y  * 20.0f ) - 0.5f);

        scratchPattern = glm::clamp( scratchPattern * 5.0f, -1.0f, 1.0f );

        const float x = glm::clamp( (noise + scratchPattern)           * 0.04f, -0.10f, 0.10f );
        const float y = glm::clamp( (noise + (noise * scratchPattern)) * 0.04f, -0.10f, 0.10f );

        return SFVEC3F( x, y, 0.0f ) + boardNormal * 0.85f;
    }
    else
        return SFVEC3F(0.0f);
}


CSOLDERMASKNORMAL::CSOLDERMASKNORMAL( const CPROCEDURALGENERATOR *aCopperNormalGenerator )
{
    m_copper_normal_generator = aCopperNormalGenerator;
}


SFVEC3F CSOLDERMASKNORMAL::Generate( const RAY &aRay, const HITINFO &aHitInfo ) const
{
    if( m_copper_normal_generator )
    {
        const SFVEC3F copperNormal = m_copper_normal_generator->Generate( aRay, aHitInfo );

        return copperNormal * SFVEC3F(0.10f);
    }
    else
        return SFVEC3F(0.0f);
}


CPLASTICNORMAL::CPLASTICNORMAL( float aScale )
{
    m_scale = 1.0f / aScale;
}


SFVEC3F CPLASTICNORMAL::Generate( const RAY &aRay, const HITINFO &aHitInfo ) const
{
        const SFVEC3F hitPos = aHitInfo.m_HitPoint * m_scale;

        const float noise1 = (m_perlin.noise( hitPos.x * 0.1f,
                                              hitPos.y * 0.1f,
                                              hitPos.z * 0.1f) - 0.5f);

        const float noise2 = (m_perlin.noise( hitPos.x * 4.0f,
                                              hitPos.y * 4.0f,
                                              hitPos.z * 4.0f ) - 0.5f);

        const float noise3 = (m_perlin.noise( hitPos.x * 8.0f + Fast_RandFloat() * 0.10f,
                                              hitPos.y * 8.0f + Fast_RandFloat() * 0.10f,
                                              hitPos.z * 8.0f + Fast_RandFloat() * 0.10f ) - 0.5f);

        return SFVEC3F( noise1 * 0.10f + noise2 * 0.20f + noise3 * 0.50f );
}


CPLASTICSHINENORMAL::CPLASTICSHINENORMAL( float aScale )
{
    m_scale = 1.0f / aScale;
}


SFVEC3F CPLASTICSHINENORMAL::Generate( const RAY &aRay, const HITINFO &aHitInfo ) const
{
    const SFVEC3F hitPos = aHitInfo.m_HitPoint * m_scale;

    const float noise1 = (m_perlin.noise( hitPos.x * 0.05f,
                                          hitPos.y * 0.05f,
                                          hitPos.z * 0.05f ) - 0.5f);

    const float noise2 = (m_perlin.noise( hitPos.x * 0.2f,
                                          hitPos.y * 0.2f,
                                          hitPos.z * 0.2f ) - 0.5f);

    const float noise3 = (m_perlin.noise( hitPos.x * 0.5f,
                                          hitPos.y * 0.5f,
                                          hitPos.z * 0.5f ) - 0.5f);

    return SFVEC3F( noise1 * 0.5f, noise2 * 0.5f, noise3 * 0.5f );
}


CMETALBRUSHEDNORMAL::CMETALBRUSHEDNORMAL( float aScale )
{
    m_scale = 1.0f / aScale;
}


SFVEC3F CMETALBRUSHEDNORMAL::Generate( const RAY &aRay, const HITINFO &aHitInfo ) const
{
    SFVEC3F hitPos = aHitInfo.m_HitPoint * m_scale;

    SFVEC3F hitPosRelative = hitPos - glm::floor( hitPos );

    const float noiseX = (m_perlin.noise( hitPos.x * (60.0f),
                                          hitPos.y * 1.0f,
                                          hitPos.z * 1.0f ) - 0.5f);

    const float noiseY = (m_perlin.noise( hitPos.x * 1.0f,
                                          hitPos.y * (60.0f),
                                          hitPos.z * 1.0f ) - 0.5f);

    const float noise2 = (m_perlin.noise( hitPos.x * 1.0f,
                                          hitPos.y * 1.0f,
                                          hitPos.z * 1.0f ) - 0.5f);

    const float noise3X = (m_perlin.noise( hitPos.x * (80.0f + noise2 * 0.5f),
                                           hitPos.y * 0.5f + Fast_RandFloat() * 0.05f,
                                           hitPos.z * 0.5f ) - 0.5f );

    const float noise3Y = (m_perlin.noise( hitPos.x * 0.5f + Fast_RandFloat() * 0.05f,
                                           hitPos.y * (80.0f + noise2 * 0.5f),
                                           hitPos.z * 0.5f ) - 0.5f );

    // http://www.fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIoKHgtZmxvb3IoeCkpK3Npbih4KSleMyIsImNvbG9yIjoiIzAwMDAwMCJ9LHsidHlwZSI6MTAwMCwid2luZG93IjpbIi02LjcxNDAwMDAxOTAzMDA3NyIsIjcuMjQ0NjQzNjkyOTY5NzM5IiwiLTMuMTU1NTUyNjAxNDUyNTg4IiwiNS40MzQzODE5OTA1NDczMDY1Il0sInNpemUiOls2NDQsMzk0XX1d
    // ((x - floor(x))+sin(x))^3

    float sawX = (hitPosRelative.x + glm::sin(10.0f * hitPos.x + 5.0f * noise2 + Fast_RandFloat() ) );
          sawX = sawX * sawX * sawX;

    float sawY = (hitPosRelative.y + glm::sin(10.0f * hitPos.y + 5.0f * noise2 + Fast_RandFloat() ) );
          sawY = sawY * sawY * sawY;

    float xOut = sawX * noise3X * 0.17f + noiseX * 0.25f + noise3X * 0.57f;
    float yOut = sawY * noise3Y * 0.17f + noiseY * 0.25f + noise3Y * 0.57f;

    const float outLowFreqNoise = noise2 * 0.05f;

    return SFVEC3F( xOut + outLowFreqNoise,
                    yOut + outLowFreqNoise,
                    0.0f + outLowFreqNoise );
}
