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
 * @file cpostshader_ssao.h
 * @brief Implements a post shader screen space ambient occlusion on software
 */

#ifndef CPOSTSHADER_SSAO_H
#define CPOSTSHADER_SSAO_H


#include "cpostshader.h"


class  CPOSTSHADER_SSAO : public CPOSTSHADER
{
public:
    explicit CPOSTSHADER_SSAO( const CCAMERA &aCamera );
    //~CPOSTSHADER_SSAO();

    // Imported from CPOSTSHADER
    SFVEC3F Shade(const SFVEC2I &aShaderPos ) const override;

private:
    SFVEC3F posFromDepth( const SFVEC2F &coord ) const;

    float ec_depth( const SFVEC2F &tc ) const;

    float aoFF( const SFVEC2I &aShaderPos,
                const SFVEC3F &ddiff,
                const SFVEC3F &cnorm,
                int c1,
                int c2,
                float aAttShadowFactor ) const;

    float giFF( const SFVEC2I &aShaderPos,
                const SFVEC3F &ddiff,
                const SFVEC3F &cnorm,
                int c1,
                int c2 ) const;

    /**
     * @brief giColorCurve - Apply a curve transformation to the original color
     * it will atenuate the bright colors (works as a gamma function):
     * http://fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIxLjAtKDEvKHgqMS4wKzEuMCkpK3gqMC4zMCIsImNvbG9yIjoiIzAwMDAwMCJ9LHsidHlwZSI6MTAwMCwid2luZG93IjpbIi0wLjA2MjE4NDYxNTM4NDYxNTUwNSIsIjEuMTQyOTg0NjE1Mzg0NjE0NiIsIi0wLjEyNzA5OTk5OTk5OTk5OTc3IiwiMS4xMzI2Il19XQ--
     * @param aColor input color
     * @return transformated color
     */
    SFVEC3F giColorCurve( const SFVEC3F &aColor ) const;
};


#endif   // CPOSTSHADER_SSAO_H
