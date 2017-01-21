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
 * @file  cmaterial.h
 * @brief
 */

#ifndef _CMATERIAL_H_
#define _CMATERIAL_H_

#include "ray.h"
#include "hitinfo.h"
#include "PerlinNoise.h"

/// A base class that can be used to derive a procedural generator implementation
class  CPROCEDURALGENERATOR
{
public:
    CPROCEDURALGENERATOR();

    /**
     * @brief Generate - Generates a 3D vector based on the ray and
     * hit information depending on the implementation
     * @param aRay: the camera ray that hits the object
     * @param aHitInfo: the hit information
     * @return the result of the procedural
     */
    virtual SFVEC3F Generate( const RAY &aRay,
                              const HITINFO &aHitInfo ) const = 0;

protected:

};


// Procedural generation of the board normals
class  CBOARDNORMAL : public CPROCEDURALGENERATOR
{
public:
    CBOARDNORMAL() : CPROCEDURALGENERATOR() { m_scale = 1.0f; }
    CBOARDNORMAL( float aScale );

    // Imported from CPROCEDURALGENERATOR
    SFVEC3F Generate( const RAY &aRay,
                      const HITINFO &aHitInfo ) const override;
private:
    float m_scale;
};

// Procedural generation of the copper normals
class  CCOPPERNORMAL : public CPROCEDURALGENERATOR
{
public:
    CCOPPERNORMAL() : CPROCEDURALGENERATOR()
    {
        m_board_normal_generator = NULL;
        m_scale = 1.0f;
    }

    CCOPPERNORMAL( float aScale, const CPROCEDURALGENERATOR *aBoardNormalGenerator );

    // Imported from CPROCEDURALGENERATOR
    SFVEC3F Generate( const RAY &aRay,
                      const HITINFO &aHitInfo ) const override;
private:
    const CPROCEDURALGENERATOR *m_board_normal_generator;
    PerlinNoise                 m_copper_perlin;
    float                       m_scale;
};

// Procedural generation of the solder mask
class  CSOLDERMASKNORMAL : public CPROCEDURALGENERATOR
{
public:
    CSOLDERMASKNORMAL() : CPROCEDURALGENERATOR() { m_copper_normal_generator = NULL; }
    CSOLDERMASKNORMAL( const CPROCEDURALGENERATOR *aCopperNormalGenerator );

    // Imported from CPROCEDURALGENERATOR
    SFVEC3F Generate( const RAY &aRay,
                      const HITINFO &aHitInfo ) const override;
private:
    const CPROCEDURALGENERATOR *m_copper_normal_generator;
};


// Procedural generation of the plastic normals
class  CPLASTICNORMAL : public CPROCEDURALGENERATOR
{
public:
    CPLASTICNORMAL() : CPROCEDURALGENERATOR()
    {
        m_scale = 1.0f;
    }

    CPLASTICNORMAL( float aScale );

    // Imported from CPROCEDURALGENERATOR
    SFVEC3F Generate( const RAY &aRay,
                      const HITINFO &aHitInfo ) const override;
private:
    PerlinNoise                 m_perlin;
    float                       m_scale;
};


// Procedural generation of the shiny plastic normals
class  CPLASTICSHINENORMAL : public CPROCEDURALGENERATOR
{
public:
    CPLASTICSHINENORMAL() : CPROCEDURALGENERATOR()
    {
        m_scale = 1.0f;
    }

    CPLASTICSHINENORMAL( float aScale );

    // Imported from CPROCEDURALGENERATOR
    SFVEC3F Generate( const RAY &aRay,
                      const HITINFO &aHitInfo ) const override;
private:
    PerlinNoise                 m_perlin;
    float                       m_scale;
};

// Procedural generation of the shiny brushed metal
class  CMETALBRUSHEDNORMAL : public CPROCEDURALGENERATOR
{
public:
    CMETALBRUSHEDNORMAL() : CPROCEDURALGENERATOR()
    {
        m_scale = 1.0f;
    }

    CMETALBRUSHEDNORMAL( float aScale );

    // Imported from CPROCEDURALGENERATOR
    SFVEC3F Generate( const RAY &aRay,
                      const HITINFO &aHitInfo ) const override;
private:
    PerlinNoise                 m_perlin;
    float                       m_scale;
};

/// A base material class that can be used to derive a material implementation
class  CMATERIAL
{
public:
    CMATERIAL();
    CMATERIAL( const SFVEC3F &aAmbient,
               const SFVEC3F &aEmissive,
               const SFVEC3F &aSpecular,
               float aShinness,
               float aTransparency,
               float aReflection );

    const SFVEC3F &GetAmbientColor()  const { return m_ambientColor; }
    const SFVEC3F &GetEmissiveColor() const { return m_emissiveColor; }
    const SFVEC3F &GetSpecularColor() const { return m_specularColor; }

    float GetShinness()     const { return m_shinness; }
    float GetTransparency() const { return m_transparency; }
    float GetReflection()   const { return m_reflection; }
    float GetAbsorvance()   const { return m_absorbance; }
    unsigned int GetNrRefractionsSamples() const { return m_refraction_nr_samples; }
    unsigned int GetNrReflectionsSamples() const { return m_reflections_nr_samples; }

    void SetAbsorvance( float aAbsorvanceFactor ) { m_absorbance = aAbsorvanceFactor; }
    void SetNrRefractionsSamples( unsigned int aNrRefractions ) { m_refraction_nr_samples = aNrRefractions; }
    void SetNrReflectionsSamples( unsigned int aNrReflections ) { m_reflections_nr_samples = aNrReflections; }

    /**
     * @brief SetCastShadows - Set if the material can receive shadows
     * @param aCastShadows - true yes it can, false not it cannot
     */
    void SetCastShadows( bool aCastShadows ) { m_cast_shadows = aCastShadows; }

    bool GetCastShadows() const { return m_cast_shadows; }

    /**
     * @brief Shade - Shades an intersection point
     * @param aRay: the camera ray that hits the object
     * @param aHitInfo: the hit information
     * @param NdotL: the dot product between Normal and Light
     * @param aDiffuseObjColor: diffuse object color
     * @param aDirToLight: a vector of the incident light direction
     * @param aLightColor: the light color
     * @param aShadowAttenuationFactor 0.0f total in shadow, 1.0f completely not in shadow
     * @return the resultant color
     */
    virtual SFVEC3F Shade( const RAY &aRay,
                           const HITINFO &aHitInfo,
                           float NdotL,
                           const SFVEC3F &aDiffuseObjColor,
                           const SFVEC3F &aDirToLight,
                           const SFVEC3F &aLightColor,
                           float aShadowAttenuationFactor ) const = 0;

    void SetNormalPerturbator( const CPROCEDURALGENERATOR *aPerturbator ) { m_normal_perturbator = aPerturbator; }
    const CPROCEDURALGENERATOR *GetNormalPerturbator() const { return m_normal_perturbator; }

    void PerturbeNormal( SFVEC3F &aNormal, const RAY &aRay, const HITINFO &aHitInfo ) const;

protected:
    SFVEC3F m_ambientColor;

    // NOTE: we will not use diffuse color material here,
    // because it will be stored in object, since there are objects (i.e: triangles)
    // that can have per vertex color

    SFVEC3F m_emissiveColor;
    SFVEC3F m_specularColor;
    float   m_shinness;
    float   m_transparency;                     ///< 1.0 is completely transparent, 0.0 completely opaque
    float   m_absorbance;                       ///< absorvance factor for the transparent material
    float   m_reflection;                       ///< 1.0 completely reflective, 0.0 no reflective
    bool    m_cast_shadows;                     ///< true if this object will block the light
    unsigned int     m_refraction_nr_samples;   ///< nr of rays that will be interpolated for this material if it is a transparent
    unsigned int     m_reflections_nr_samples;  ///< nr of rays that will be interpolated for this material if it is reflective

    const CPROCEDURALGENERATOR *m_normal_perturbator;
};


/// Blinn Phong based material
/// https://en.wikipedia.org/wiki/Blinn%E2%80%93Phong_shading_model
class  CBLINN_PHONG_MATERIAL : public CMATERIAL
{
public:
    CBLINN_PHONG_MATERIAL() : CMATERIAL() {}

    CBLINN_PHONG_MATERIAL( const SFVEC3F &aAmbient,
                           const SFVEC3F &aEmissive,
                           const SFVEC3F &aSpecular,
                           float aShinness,
                           float aTransparency,
                           float aReflection ) : CMATERIAL( aAmbient,
                                                            aEmissive,
                                                            aSpecular,
                                                            aShinness,
                                                            aTransparency,
                                                            aReflection ) {}

    // Imported from CMATERIAL
    SFVEC3F Shade( const RAY &aRay,
                   const HITINFO &aHitInfo,
                   float NdotL,
                   const SFVEC3F &aDiffuseObjColor,
                   const SFVEC3F &aDirToLight,
                   const SFVEC3F &aLightColor,
                   float aShadowAttenuationFactor ) const override;
};

#endif // _CMATERIAL_H_
