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

 /**
 * @file post_shader.h
 * @brief A base class to create post shaders.
 */

#ifndef POST_SHADER_H
#define POST_SHADER_H

#include <gal/3d/camera.h>

class POST_SHADER
{
public:
    explicit POST_SHADER( const CAMERA& aCamera );
    virtual ~POST_SHADER();

    virtual SFVEC3F Shade( const SFVEC2I& aShaderPos ) const = 0;

    /**
     * Apply the final color process using a previous stage color.
     *
     * @param aShadeColor The result of the shader.
     * @return the result of the shade process
     */
    virtual SFVEC4F ApplyShadeColor( const SFVEC2I& aShaderPos, const SFVEC4F& aInputColor,
                                     const SFVEC3F& aShadeColor ) const = 0;

    void UpdateSize( const SFVEC2UI& aSize );

    void UpdateSize( unsigned int xSize, unsigned int ySize );

    void InitFrame() { m_tmin = FLT_MAX; m_tmax = 0.0f; }

    void SetPixelData( unsigned int x, unsigned int y, const SFVEC3F& aNormal,
                       const SFVEC4F& aColor, const SFVEC3F& aHitPosition,
                       float aDepth, float aShadowAttFactor );

    const SFVEC4F& GetColorAtNotProtected( const SFVEC2I& aPos ) const;

    void DebugBuffersOutputAsImages() const;

    inline unsigned int GetIndex( const SFVEC2F& aPos ) const
    {
        SFVEC2F clampPos;

        clampPos.x = glm::clamp( aPos.x, 0.0f, 1.0f );
        clampPos.y = glm::clamp( aPos.y, 0.0f, 1.0f );

        const unsigned int idx = (unsigned int)( (float)m_size.x * clampPos.x +
                                                 (float)m_size.x * (float)m_size.y *
                                                 clampPos.y );

        return glm::min( idx, m_size.x * m_size.y );
    }

    inline unsigned int GetIndex( const SFVEC2I& aPos ) const
    {
        SFVEC2I clampPos;
        clampPos.x = glm::clamp( aPos.x, 0, (int)m_size.x - 1 );
        clampPos.y = glm::clamp( aPos.y, 0, (int)m_size.y - 1 );

        return (unsigned int)( clampPos.x + m_size.x * clampPos.y );
    }

protected:
    const SFVEC3F& GetNormalAt( const SFVEC2F& aPos ) const;
    const SFVEC4F& GetColorAt( const SFVEC2F& aPos ) const;
    const SFVEC3F& GetPositionAt( const SFVEC2F& aPos ) const;
    float GetDepthAt( const SFVEC2F& aPos ) const;

    const SFVEC3F& GetNormalAt( const SFVEC2I& aPos ) const;
    const SFVEC4F& GetColorAt( const SFVEC2I& aPos ) const;
    const SFVEC3F& GetPositionAt( const SFVEC2I& aPos ) const;
    const float& GetShadowFactorAt( const SFVEC2I& aPos ) const;

    float GetDepthAt( const SFVEC2I& aPos ) const;
    float GetDepthNormalizedAt( const SFVEC2I& aPos ) const;
    float GetMaxDepth() const { return m_tmax; }

private:
    void destroy_buffers();

protected:
    const CAMERA& m_camera;

    SFVEC2UI m_size;
    SFVEC3F* m_normals;
    SFVEC4F* m_color;
    SFVEC3F* m_wc_hitposition;
    float*   m_depth;
    float*   m_shadow_att_factor;
    float    m_tmin;
    float    m_tmax;
};


#endif   // POST_SHADER_H
