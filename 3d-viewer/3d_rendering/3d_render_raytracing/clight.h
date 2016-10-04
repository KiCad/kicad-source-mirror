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
 * @file  clight.h
 * @brief declare and implement light types classes
 */

#ifndef _CLIGHT_H_
#define _CLIGHT_H_

#include "ray.h"
#include "hitinfo.h"

/// A base light class to derive to implement other light classes
class  CLIGHT
{
public:
    CLIGHT() { m_castShadow = true; }

    virtual ~CLIGHT() {}

    /**
     * @brief GetLightParameters - Get parameters from this light
     * @param aHitPoint: input hit position
     * @param aOutVectorToLight: a vector that points from the hit
     * position in direction to the light
     * @param aOutLightColor: the color of this light
     * @param aOutDistance: the distance from the point to the light
     */
    virtual void GetLightParameters( const SFVEC3F &aHitPoint,
                                     SFVEC3F &aOutVectorToLight,
                                     SFVEC3F &aOutLightColor,
                                     float &aOutDistance ) const = 0;

    void SetCastShadows( bool aCastShadow ) { m_castShadow = aCastShadow; }
    bool GetCastShadows() const { return m_castShadow; }

protected:
    bool m_castShadow;
};


/// Point light based on:
/// http://ogldev.atspace.co.uk/www/tutorial20/tutorial20.html
class  CPOINTLIGHT : public CLIGHT
{

public:
    CPOINTLIGHT( const SFVEC3F &aPos, const SFVEC3F &aColor )
    {
        m_position     = aPos;
        m_color        = aColor;
        m_att_constant = 0.9f;
        m_att_linear   = 0.0005f;
        m_att_exp      = 0.001f;
        m_castShadow   = true;
    }

    // Imported functions from CLIGHT

    void GetLightParameters( const SFVEC3F &aHitPoint,
                             SFVEC3F &aOutVectorToLight,
                             SFVEC3F &aOutLightColor,
                             float &aOutDistance ) const override
    {
        const SFVEC3F vectorLight = m_position - aHitPoint;

        aOutDistance = glm::length( vectorLight );
        aOutVectorToLight = vectorLight / aOutDistance; // normalize

        const float att = 1.0f / ( m_att_constant +
                                   m_att_linear   * aOutDistance +
                                   m_att_exp      * aOutDistance * aOutDistance );

        if( att <= 0.0f )
            aOutLightColor = SFVEC3F( 0.0f, 0.0f, 0.0f );
        else
            aOutLightColor = m_color * att;
    }

private:
    SFVEC3F m_position;
    SFVEC3F m_color;

    float   m_att_constant;
    float   m_att_linear;
    float   m_att_exp;
};


/// Directional light - a light based only on a direction vector
class  CDIRECTIONALLIGHT : public CLIGHT
{
public:
    CDIRECTIONALLIGHT( const SFVEC3F &aDir, const SFVEC3F &aColor )
    {
        // Invert light direction and make sure it is normalized
        m_inv_direction = glm::normalize( -aDir );
        m_color         = aColor;
        m_castShadow    = true; // Set as default to cast shadows
    }

    /**
     * @brief SetDirection - Set directional light orientation
     * @param aDir: vector from the light
     */
    void SetDirection( const SFVEC3F &aDir ) { m_inv_direction = -aDir; }

    // Imported functions from CLIGHT

    void GetLightParameters( const SFVEC3F &aHitPoint,
                             SFVEC3F &aOutVectorToLight,
                             SFVEC3F &aOutLightColor,
                             float &aOutDistance ) const override
    {
        (void)aHitPoint; // unused

        aOutVectorToLight = m_inv_direction;
        aOutDistance      = std::numeric_limits<float>::infinity();
        aOutLightColor    = m_color;
    }

private:
    SFVEC3F m_inv_direction;        ///< oposite direction of the light
    SFVEC3F m_color;                ///< light color
};


typedef std::list< CLIGHT * > LIST_LIGHT;


/// A light contariner. It will add lights and remove it in the end
class  CLIGHTCONTAINER
{
public:
    CLIGHTCONTAINER() {}

    ~CLIGHTCONTAINER() { Clear(); }

    /**
     * @brief Clear - Remove all lights from the container
     */
    void Clear()
    {
        if( !m_lights.empty() )
        {
            for( LIST_LIGHT::iterator ii = m_lights.begin();
                 ii != m_lights.end();
                 --ii )
            {
                delete *ii;
                *ii = NULL;
            }

            m_lights.clear();
        }
    }


    /**
     * @brief Add - Add a light to the container
     * @param aLight
     */
    void Add( CLIGHT *aLight )
    {
        if( aLight )
            m_lights.push_back( aLight );
    }

    /**
     * @brief GetList - get light list of this container
     * @return a list of lights
     */
    const LIST_LIGHT &GetList() const { return m_lights; }

private:
    LIST_LIGHT m_lights;    ///< list of lights
};

#endif // _CLIGHT_H_
