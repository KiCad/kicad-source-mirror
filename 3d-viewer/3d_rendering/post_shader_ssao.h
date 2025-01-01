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
 * @file post_shader_ssao.h
 * @brief Implements a post shader screen space ambient occlusion on software
 */

#ifndef POST_SHADER_SSAO_H
#define POST_SHADER_SSAO_H


#include "post_shader.h"


class POST_SHADER_SSAO : public POST_SHADER
{
public:
    explicit POST_SHADER_SSAO( const CAMERA& aCamera );

    SFVEC3F Shade(const SFVEC2I& aShaderPos ) const override;
    SFVEC4F ApplyShadeColor( const SFVEC2I& aShaderPos, const SFVEC4F& aInputColor,
                             const SFVEC3F& aShadeColor ) const override;

    SFVEC3F Blur( const SFVEC2I& aShaderPos ) const;

    void SetShadedBuffer( SFVEC3F* aShadedBuffer )
    {
        m_shadedBuffer = aShadedBuffer;
    }

    void SetShadowsEnabled( bool aIsShadowsEnabled )
    {
        m_isUsingShadows = aIsShadowsEnabled;
    }

private:
    SFVEC3F posFromDepth( const SFVEC2F& coord ) const;

    float ec_depth( const SFVEC2F& tc ) const;

    float aoFF( const SFVEC2I& aShaderPos, const SFVEC3F& ddiff, const SFVEC3F& cnorm,
                const float aShadowAtSamplePos, const float aShadowAtCenterPos,
                int c1, int c2 ) const;

    float giFF( const SFVEC2I& aShaderPos, const SFVEC3F& ddiff, const SFVEC3F& cnorm,
                const float aShadow, int c1, int c2 ) const;

    /**
     * Apply a curve transformation to the original color.
     *
     * It will attenuate the bright colors (works as a gamma function):
     * http://fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIxLjAtKDEvKHgqMS4wKzEuMCkpK3gqMC4zMCIsImNvbG9yIjoiIzAwMDAwMCJ9LHsidHlwZSI6MTAwMCwid2luZG93IjpbIi0wLjA2MjE4NDYxNTM4NDYxNTUwNSIsIjEuMTQyOTg0NjE1Mzg0NjE0NiIsIi0wLjEyNzA5OTk5OTk5OTk5OTc3IiwiMS4xMzI2Il19XQ--
     *
     * @param aColor input color.
     * @return transformed color.
     */
    SFVEC3F giColorCurve( const SFVEC3F& aColor ) const;
    SFVEC4F giColorCurve( const SFVEC4F& aColor ) const;
    SFVEC3F giColorCurveShade( const SFVEC4F& aColor ) const;

    SFVEC3F* m_shadedBuffer;

    bool m_isUsingShadows;
};


#endif   // POST_SHADER_SSAO_H
