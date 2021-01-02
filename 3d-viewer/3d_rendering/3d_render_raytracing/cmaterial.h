/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 2015-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef _MATERIAL_H_
#define _MATERIAL_H_

#include "ray.h"
#include "hitinfo.h"
#include "PerlinNoise.h"

/**
 * A base class that can be used to derive procedurally generated materials.
 */
class PROCEDURAL_GENERATOR
{
public:
    PROCEDURAL_GENERATOR();

    virtual ~PROCEDURAL_GENERATOR()
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


class BOARD_NORMAL : public PROCEDURAL_GENERATOR
{
public:
    BOARD_NORMAL() : PROCEDURAL_GENERATOR() { m_scale = 1.0f; }
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
class COPPER_NORMAL : public PROCEDURAL_GENERATOR
{
public:
    COPPER_NORMAL() : PROCEDURAL_GENERATOR()
    {
        m_board_normal_generator = nullptr;
        m_scale = 1.0f;
    }

    COPPER_NORMAL( float aScale, const PROCEDURAL_GENERATOR* aBoardNormalGenerator );

    virtual ~COPPER_NORMAL()
    {
    }

    SFVEC3F Generate( const RAY& aRay, const HITINFO& aHitInfo ) const override;

private:
    const PROCEDURAL_GENERATOR* m_board_normal_generator;
    float                       m_scale;
};


class PLATED_COPPER_NORMAL : public PROCEDURAL_GENERATOR
{
public:
    PLATED_COPPER_NORMAL() : PROCEDURAL_GENERATOR()
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
class SOLDER_MASK_NORMAL : public PROCEDURAL_GENERATOR
{
public:
    SOLDER_MASK_NORMAL() : PROCEDURAL_GENERATOR() { m_copper_normal_generator = nullptr; }
    SOLDER_MASK_NORMAL( const PROCEDURAL_GENERATOR* aCopperNormalGenerator );

    virtual ~SOLDER_MASK_NORMAL()
    {
    }

    SFVEC3F Generate( const RAY& aRay, const HITINFO& aHitInfo ) const override;

private:
    const PROCEDURAL_GENERATOR* m_copper_normal_generator;
};


/**
 * Procedural generation of the plastic normals.
 */
class PLASTIC_NORMAL : public PROCEDURAL_GENERATOR
{
public:
    PLASTIC_NORMAL() : PROCEDURAL_GENERATOR()
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
class PLASTIC_SHINE_NORMAL : public PROCEDURAL_GENERATOR
{
public:
    PLASTIC_SHINE_NORMAL() : PROCEDURAL_GENERATOR()
    {
        m_scale = 1.0f;
    }

    PLASTIC_SHINE_NORMAL( float aScale );

    virtual ~PLASTIC_SHINE_NORMAL()
    {
    }

    // Imported from PROCEDURAL_GENERATOR
    SFVEC3F Generate( const RAY& aRay, const HITINFO& aHitInfo ) const override;

private:
    float m_scale;
};


/**
 * Procedural generation of the shiny brushed metal.
 */
class BRUSHED_METAL_NORMAL : public PROCEDURAL_GENERATOR
{
public:
    BRUSHED_METAL_NORMAL() : PROCEDURAL_GENERATOR()
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


class SILK_SCREEN_NORMAL : public PROCEDURAL_GENERATOR
{
public:
    SILK_SCREEN_NORMAL() : PROCEDURAL_GENERATOR()
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
    static void SetDefaultNrRefractionsSamples( unsigned int aNrRefractions )
    {
        m_default_nrsamples_refractions = aNrRefractions;
    }

    static void SetDefaultNrReflectionsSamples( unsigned int aNrReflections )
    {
        m_default_nrsamples_reflections = aNrReflections;
    }

    static void SetDefaultRefractionsLevel( unsigned int aRefractionLevel )
    {
        m_default_refractions_recursive_levels = aRefractionLevel;
    }

    static void SetDefaultReflectionsLevel( unsigned int aReflectionLevel )
    {
        m_default_reflections_recursive_levels = aReflectionLevel;
    }

    MATERIAL();
    MATERIAL( const SFVEC3F& aAmbient, const SFVEC3F& aEmissive, const SFVEC3F& aSpecular,
               float aShinness, float aTransparency, float aReflection );

    virtual ~MATERIAL() {}

    const SFVEC3F& GetAmbientColor()  const { return m_ambientColor; }
    const SFVEC3F& GetEmissiveColor() const { return m_emissiveColor; }
    const SFVEC3F& GetSpecularColor() const { return m_specularColor; }

    float GetShinness()     const { return m_shinness; }
    float GetTransparency() const { return m_transparency; }
    float GetReflection()   const { return m_reflection; }
    float GetAbsorvance()   const { return m_absorbance; }
    unsigned int GetNrRefractionsSamples() const { return m_refraction_nr_samples; }
    unsigned int GetNrReflectionsSamples() const { return m_reflections_nr_samples; }
    unsigned int GetReflectionsRecursiveLevel() const { return m_reflections_recursive_levels; }
    unsigned int GetRefractionsRecursiveLevel() const { return m_refractions_recursive_levels; }

    void SetAbsorvance( float aAbsorvanceFactor ) { m_absorbance = aAbsorvanceFactor; }
    void SetNrRefractionsSamples( unsigned int aNrRefractions )
    {
        m_refraction_nr_samples = aNrRefractions;
    }

    void SetNrReflectionsSamples( unsigned int aNrReflections )
    {
        m_reflections_nr_samples = aNrReflections;
    }

    void SetReflectionsRecursiveLevel( unsigned int aReflectionsLevel )
    {
        m_reflections_recursive_levels = aReflectionsLevel;
    }

    void SetRefractionsRecursiveLevel( unsigned int aRefractionsLevel )
    {
        m_refractions_recursive_levels = aRefractionsLevel;
    }

    /**
     * Set if the material can receive shadows.
     *
     * @param aCastShadows true yes it can, false not it cannot
     */
    void SetCastShadows( bool aCastShadows ) { m_cast_shadows = aCastShadows; }

    bool GetCastShadows() const { return m_cast_shadows; }

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

    void SetNormalPerturbator( const PROCEDURAL_GENERATOR* aPerturbator )
    {
        m_normal_perturbator = aPerturbator;
    }

    const PROCEDURAL_GENERATOR* GetNormalPerturbator() const { return m_normal_perturbator; }

    void PerturbeNormal( SFVEC3F& aNormal, const RAY& aRay, const HITINFO& aHitInfo ) const;

protected:
    SFVEC3F m_ambientColor;

    // NOTE: we will not use diffuse color material here,
    // because it will be stored in object, since there are objects (i.e: triangles)
    // that can have per vertex color

    SFVEC3F m_emissiveColor;
    SFVEC3F m_specularColor;
    float   m_shinness;

    ///< 1.0 is completely transparent, 0.0 completely opaque.
    float   m_transparency;
    float   m_absorbance;                     ///< absorbance factor for the transparent material.
    float   m_reflection;                     ///< 1.0 completely reflective, 0.0 no reflective.
    bool    m_cast_shadows;                   ///< true if this object will block the light.

    ///< Number of rays that will be interpolated for this material if it is a transparent.
    unsigned int     m_refraction_nr_samples;

    ///< Number of rays that will be interpolated for this material if it is reflective.
    unsigned int     m_reflections_nr_samples;

    ///< Number of levels it allows for refraction recursiveness.
    unsigned int     m_refractions_recursive_levels;

    ///< Number of levels it allows for reflection recursiveness.
    unsigned int     m_reflections_recursive_levels;

    const PROCEDURAL_GENERATOR* m_normal_perturbator;

private:
    static int m_default_nrsamples_refractions;
    static int m_default_nrsamples_reflections;
    static int m_default_refractions_recursive_levels;
    static int m_default_reflections_recursive_levels;
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

#endif // _MATERIAL_H_
