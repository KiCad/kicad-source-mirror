/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
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

#ifndef MATERIAL_H
#define MATERIAL_H

#include "ray.h"
#include "hitinfo.h"
#include "PerlinNoise.h"

/**
 * A base class that can be used to derive procedurally generated materials.
 */
class MATERIAL_GENERATOR
{
public:
    MATERIAL_GENERATOR();

    virtual ~MATERIAL_GENERATOR()
    {
    }

    /**
     * Generate a 3D vector based on the ray and hit information depending on the implementation.
     *
     * @param aRay the camera ray that hits the object
     * @param aHitInfo the hit information
     * @return the result of the procedural
     */
    virtual SFVEC3F Generate( const RAY& aRay, const HITINFO& aHitInfo ) const = 0;
};


class BOARD_NORMAL : public MATERIAL_GENERATOR
{
public:
    BOARD_NORMAL() : MATERIAL_GENERATOR() { m_scale = 1.0f; }
    BOARD_NORMAL( float aScale );

    virtual ~BOARD_NORMAL()
    {
    }

    SFVEC3F Generate( const RAY& aRay, const HITINFO& aHitInfo ) const override;

private:
    float m_scale;
};


/**
 * Procedural generation of the copper normals.
 */
class COPPER_NORMAL : public MATERIAL_GENERATOR
{
public:
    COPPER_NORMAL() : MATERIAL_GENERATOR()
    {
        m_board_normal_generator = nullptr;
        m_scale = 1.0f;
    }

    COPPER_NORMAL( float aScale, const MATERIAL_GENERATOR* aBoardNormalGenerator );

    virtual ~COPPER_NORMAL()
    {
    }

    SFVEC3F Generate( const RAY& aRay, const HITINFO& aHitInfo ) const override;

private:
    const MATERIAL_GENERATOR* m_board_normal_generator;
    float                       m_scale;
};


class PLATED_COPPER_NORMAL : public MATERIAL_GENERATOR
{
public:
    PLATED_COPPER_NORMAL() : MATERIAL_GENERATOR()
    {
        m_scale = 1.0f;
    }

    PLATED_COPPER_NORMAL( float aScale )
    {
        m_scale = 1.0f / aScale;
    }

    virtual ~PLATED_COPPER_NORMAL()
    {
    }

    SFVEC3F Generate( const RAY& aRay, const HITINFO& aHitInfo ) const override;

private:
    float m_scale;
};


/**
 * Procedural generation of the solder mask.
 */
class SOLDER_MASK_NORMAL : public MATERIAL_GENERATOR
{
public:
    SOLDER_MASK_NORMAL() : MATERIAL_GENERATOR() { m_copper_normal_generator = nullptr; }
    SOLDER_MASK_NORMAL( const MATERIAL_GENERATOR* aCopperNormalGenerator );

    virtual ~SOLDER_MASK_NORMAL()
    {
    }

    SFVEC3F Generate( const RAY& aRay, const HITINFO& aHitInfo ) const override;

private:
    const MATERIAL_GENERATOR* m_copper_normal_generator;
};


/**
 * Procedural generation of the plastic normals.
 */
class PLASTIC_NORMAL : public MATERIAL_GENERATOR
{
public:
    PLASTIC_NORMAL() : MATERIAL_GENERATOR()
    {
        m_scale = 1.0f;
    }

    PLASTIC_NORMAL( float aScale );

    virtual ~PLASTIC_NORMAL()
    {
    }

    SFVEC3F Generate( const RAY& aRay, const HITINFO& aHitInfo ) const override;

private:
    float m_scale;
};


/**
 * Procedural generation of the shiny plastic normals.
 */
class PLASTIC_SHINE_NORMAL : public MATERIAL_GENERATOR
{
public:
    PLASTIC_SHINE_NORMAL() : MATERIAL_GENERATOR()
    {
        m_scale = 1.0f;
    }

    PLASTIC_SHINE_NORMAL( float aScale );

    virtual ~PLASTIC_SHINE_NORMAL()
    {
    }

    // Imported from MATERIAL_GENERATOR
    SFVEC3F Generate( const RAY& aRay, const HITINFO& aHitInfo ) const override;

private:
    float m_scale;
};


/**
 * Procedural generation of the shiny brushed metal.
 */
class BRUSHED_METAL_NORMAL : public MATERIAL_GENERATOR
{
public:
    BRUSHED_METAL_NORMAL() : MATERIAL_GENERATOR()
    {
        m_scale = 1.0f;
    }

    BRUSHED_METAL_NORMAL( float aScale );

    virtual ~BRUSHED_METAL_NORMAL()
    {
    }

    SFVEC3F Generate( const RAY& aRay, const HITINFO& aHitInfo ) const override;

private:
    float m_scale;
};


class SILK_SCREEN_NORMAL : public MATERIAL_GENERATOR
{
public:
    SILK_SCREEN_NORMAL() : MATERIAL_GENERATOR()
    {
        m_scale = 1.0f;
    }

    SILK_SCREEN_NORMAL( float aScale );

    virtual ~SILK_SCREEN_NORMAL()
    {
    }

    SFVEC3F Generate( const RAY& aRay, const HITINFO& aHitInfo ) const override;

private:
    float m_scale;
};


/**
 * Base material class that can be used to derive other material implementations.
 */
class MATERIAL
{
public:
    static void SetDefaultRefractionRayCount( unsigned int aCount )
    {
        m_defaultRefractionRayCount = aCount;
    }

    static void SetDefaultReflectionRayCount( unsigned int aCount )
    {
        m_defaultReflectionRayCount = aCount;
    }

    static void SetDefaultRefractionRecursionCount( unsigned int aCount )
    {
        m_defaultRefractionRecursionCount = aCount;
    }

    static void SetDefaultReflectionRecursionCount( unsigned int aCount )
    {
        m_defaultFeflectionRecursionCount = aCount;
    }

    MATERIAL();
    MATERIAL( const SFVEC3F& aAmbient, const SFVEC3F& aEmissive, const SFVEC3F& aSpecular,
              float aShinness, float aTransparency, float aReflection );

    virtual ~MATERIAL() {}

    const SFVEC3F& GetAmbientColor()  const { return m_ambientColor; }
    const SFVEC3F& GetEmissiveColor() const { return m_emissiveColor; }
    const SFVEC3F& GetSpecularColor() const { return m_specularColor; }

    float GetReflectivity() const { return m_reflectivity; }
    float GetTransparency() const { return m_transparency; }
    float GetReflection()   const { return m_reflection; }
    float GetAbsorvance()   const { return m_absorbance; }
    unsigned int GetRefractionRayCount() const { return m_refractionRayCount; }
    unsigned int GetReflectionRayCount() const { return m_reflectionRayCount; }
    unsigned int GetReflectionRecursionCount() const { return m_reflectionRecursionCount; }
    unsigned int GetRefractionRecursionCount() const { return m_refractionRecursionCount; }

    void SetAbsorvance( float aAbsorvanceFactor ) { m_absorbance = aAbsorvanceFactor; }
    void SetRefractionRayCount( unsigned int aCount )
    {
        m_refractionRayCount = aCount;
    }

    void SetReflectionRayCount( unsigned int aCount )
    {
        m_reflectionRayCount = aCount;
    }

    void SetReflectionRecursionCount( unsigned int aCount )
    {
        m_reflectionRecursionCount = aCount;
    }

    void SetRefractionRecursionCount( unsigned int aCount )
    {
        m_refractionRecursionCount = aCount;
    }

    /**
     * Set if the material can receive shadows.
     *
     * @param aCastShadows true yes it can, false not it cannot
     */
    void SetCastShadows( bool aCastShadows ) { m_castShadows = aCastShadows; }

    bool GetCastShadows() const { return m_castShadows; }

    /**
     * Shade an intersection point.
     *
     * @param aRay the camera ray that hits the object
     * @param aHitInfo the hit information
     * @param NdotL the dot product between Normal and Light
     * @param aDiffuseObjColor diffuse object color
     * @param aDirToLight a vector of the incident light direction
     * @param aLightColor the light color
     * @param aShadowAttenuationFactor 0.0f total in shadow, 1.0f completely not in shadow
     * @return the resultant color
     */
    virtual SFVEC3F Shade( const RAY& aRay, const HITINFO& aHitInfo, float NdotL,
                           const SFVEC3F& aDiffuseObjColor, const SFVEC3F& aDirToLight,
                           const SFVEC3F& aLightColor,
                           float aShadowAttenuationFactor ) const = 0;

    void SetGenerator( const MATERIAL_GENERATOR* aGenerator )
    {
        m_generator = aGenerator;
    }

    const MATERIAL_GENERATOR* GetGenerator() const { return m_generator; }

    void Generate( SFVEC3F& aNormal, const RAY& aRay, const HITINFO& aHitInfo ) const;

protected:
    SFVEC3F m_ambientColor;

    // NOTE: we will not use diffuse color material here,
    // because it will be stored in object, since there are objects (i.e: triangles)
    // that can have per vertex color

    SFVEC3F m_emissiveColor;
    SFVEC3F m_specularColor;
    float   m_reflectivity;

    /// 1.0 is completely transparent, 0.0 completely opaque.
    float   m_transparency;
    float   m_absorbance;                     ///< absorbance factor for the transparent material.
    float   m_reflection;                     ///< 1.0 completely reflective, 0.0 no reflective.
    bool    m_castShadows;                    ///< true if this object will block the light.

    /// Number of rays that will be interpolated for this material if it is transparent.
    unsigned int     m_refractionRayCount;

    /// Number of rays that will be interpolated for this material if it is reflective.
    unsigned int     m_reflectionRayCount;

    /// Number of levels it allows for refraction recursiveness.
    unsigned int     m_refractionRecursionCount;

    /// Number of levels it allows for reflection recursiveness.
    unsigned int     m_reflectionRecursionCount;

    const MATERIAL_GENERATOR* m_generator;

private:
    static int m_defaultRefractionRayCount;
    static int m_defaultReflectionRayCount;
    static int m_defaultRefractionRecursionCount;
    static int m_defaultFeflectionRecursionCount;
};


/// Blinn Phong based material
/// https://en.wikipedia.org/wiki/Blinn%E2%80%93Phong_shading_model
class BLINN_PHONG_MATERIAL : public MATERIAL
{
public:
    BLINN_PHONG_MATERIAL() : MATERIAL() {}

    BLINN_PHONG_MATERIAL( const SFVEC3F& aAmbient, const SFVEC3F& aEmissive,
                          const SFVEC3F& aSpecular, float aShinness, float aTransparency,
                          float aReflection ) :
        MATERIAL( aAmbient, aEmissive, aSpecular, aShinness, aTransparency, aReflection ) {}

    // Imported from MATERIAL
    SFVEC3F Shade( const RAY& aRay, const HITINFO& aHitInfo, float NdotL,
                   const SFVEC3F& aDiffuseObjColor, const SFVEC3F& aDirToLight,
                   const SFVEC3F& aLightColor, float aShadowAttenuationFactor ) const override;
};

#endif // MATERIAL_H
