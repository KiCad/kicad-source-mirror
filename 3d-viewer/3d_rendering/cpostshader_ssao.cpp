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
 * @file cpostshader_ssao.cpp
 * @brief Implements a post shader screen space ambient occlusion on software
 */

#include "cpostshader_ssao.h"
#include "../3d_fastmath.h"


CPOSTSHADER_SSAO::CPOSTSHADER_SSAO( const CCAMERA &aCamera ) : CPOSTSHADER( aCamera )
{

}
//https://github.com/OniDaito/CoffeeGL/blob/master/misc/ssao.frag

// By martinsh
//http://www.gamedev.net/topic/556187-the-best-ssao-ive-seen/?view=findpost&p=4632208

float CPOSTSHADER_SSAO::aoFF( const SFVEC2I &aShaderPos,
                              const SFVEC3F &ddiff,
                              const SFVEC3F &cnorm,
                              int c1,
                              int c2,
                              float aAttShadowFactor ) const
{
    float return_value = -1.0f;

    const float rd = glm::length( ddiff );

    const float shadow_factor_at_shade_pos = aAttShadowFactor * 1.00f;

    float shadow_factor = shadow_factor_at_shade_pos;

    const SFVEC2I vr = aShaderPos + SFVEC2I( c1, c2 );

    // Calculate a blured shadow based on hit-light test calculation
    // /////////////////////////////////////////////////////////////////////////

    // This limit to the zero of the function (see below)
    if( (rd > FLT_EPSILON) && (rd < 1.0f) )
    {
        // http://www.fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIxLSh4Lyh4LzIrMC41KSkiLCJjb2xvciI6IiMwMDAwMDAifSx7InR5cGUiOjEwMDAsIndpbmRvdyI6WyItMC41OTk1NTEyNjc1Njk4MjUiLCIxLjI3Mzk0NjE3NzQxNjI5ODgiLCItMC4xMTQzMjE1NjkyMTMwMTAwOCIsIjEuMDM4NTk5OTM1MzkzODM1MyJdfV0-
        // zero: 1.0
        const float attDistFactor = 1.0f - (rd / (rd / 2.0f  + 0.5f));

        const float shadow_factor_at_sample = GetShadowFactorAt( vr );

        shadow_factor =  shadow_factor_at_sample * attDistFactor +
                         (1.0f - attDistFactor) * shadow_factor_at_shade_pos;
    }

    //shadow_factor =  (1.0f - shadow_factor) / 8.0f;
    //shadow_factor = (0.66f - ( 1.0f - 1.0f / ( shadow_factor * 2.00f + 1.0f ) )) / 12.0f;
    //shadow_factor = 0.50f - ( 1.0f - 1.0f / ( shadow_factor * 1.00f + 1.0f ) ); // http://www.fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIwLjUtKDEuMC0xLjAvKHgqMS4wKzEuMCkpIiwiY29sb3IiOiIjMDAwMDAwIn0seyJ0eXBlIjoxMDAwLCJ3aW5kb3ciOlsiLTAuNTk5NTUxMjY3NTY5ODI1IiwiMS4yNzM5NDYxNzc0MTYyOTg4IiwiLTAuMTE0MzIxNTY5MjEzMDEwMDgiLCIxLjAzODU5OTkzNTM5MzgzNTMiXX1d
    //shadow_factor = 0.40f - ( 1.0f - 1.0f / ( shadow_factor * 0.67f + 1.0f ) ); // http://www.fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIwLjQtKDEuMC0xLjAvKHgqMC42NysxLjApKSIsImNvbG9yIjoiIzAwMDAwMCJ9LHsidHlwZSI6MTAwMCwid2luZG93IjpbIi0wLjU5OTU1MTI2NzU2OTgyNSIsIjEuMjczOTQ2MTc3NDE2Mjk4OCIsIi0wLjExNDMyMTU2OTIxMzAxMDA4IiwiMS4wMzg1OTk5MzUzOTM4MzUzIl19XQ--
    //shadow_factor = 0.20f - ( 1.0f - 1.0f / ( shadow_factor * 0.25f + 1.0f ) ); // http://www.fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIwLjItKDEuMC0xLjAvKHgqMC4yNSsxLjApKSIsImNvbG9yIjoiIzAwMDAwMCJ9LHsidHlwZSI6MTAwMCwid2luZG93IjpbIi0wLjU5OTU1MTI2NzU2OTgyNSIsIjEuMjczOTQ2MTc3NDE2Mjk4OCIsIi0wLjExNDMyMTU2OTIxMzAxMDA4IiwiMS4wMzg1OTk5MzUzOTM4MzUzIl19XQ--
    //shadow_factor = 1.0f / ( shadow_factor * 15.0f + 3.0f ) - 0.05f;            // http://www.fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIxLjAvKHgqMTUuMCszLjApLTAuMDUiLCJjb2xvciI6IiMwMDAwMDAifSx7InR5cGUiOjEwMDAsIndpbmRvdyI6WyItMC41OTk1NTEyNjc1Njk4MjUiLCIxLjI3Mzk0NjE3NzQxNjI5ODgiLCItMC4xMTQzMjE1NjkyMTMwMTAwOCIsIjEuMDM4NTk5OTM1MzkzODM1MyJdfV0-
    //shadow_factor = 1.0f / ( shadow_factor * 10.0f + 2.0f ) - 0.08f;            // http://www.fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIxLjAvKHgqMTAuMCsyLjApLTAuMDgiLCJjb2xvciI6IiMwMDAwMDAifSx7InR5cGUiOjEwMDAsIndpbmRvdyI6WyItMC41OTk1NTEyNjc1Njk4MjUiLCIxLjI3Mzk0NjE3NzQxNjI5ODgiLCItMC4xMTQzMjE1NjkyMTMwMTAwOCIsIjEuMDM4NTk5OTM1MzkzODM1MyJdfV0-
    //shadow_factor = (1.0f / ( shadow_factor * 3.0f + 1.5f ))- 0.22f;            // http://www.fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIxLjAvKHgqMy4wKzEuNSktMC4yMiIsImNvbG9yIjoiIzAwMDAwMCJ9LHsidHlwZSI6MTAwMCwid2luZG93IjpbIi0wLjU5OTU1MTI2NzU2OTgyNSIsIjEuMjczOTQ2MTc3NDE2Mjk4OCIsIi0wLjExNDMyMTU2OTIxMzAxMDA4IiwiMS4wMzg1OTk5MzUzOTM4MzUzIl19XQ--
    //shadow_factor = (1.0f / ( shadow_factor * 1.7f + 1.9f ))- 0.28f;            // http://www.fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIoMS4wLyh4KjEuNysxLjkpKS0wLjI4IiwiY29sb3IiOiIjMDAwMDAwIn0seyJ0eXBlIjoxMDAwLCJ3aW5kb3ciOlsiLTAuNTk5NTUxMjY3NTY5ODI1IiwiMS4yNzM5NDYxNzc0MTYyOTg4IiwiLTAuMTE0MzIxNTY5MjEzMDEwMDgiLCIxLjAzODU5OTkzNTM5MzgzNTMiXX1d
    //shadow_factor = (shadow_factor - 0.1);
    //shadow_factor = (1.0f / ( shadow_factor * shadow_factor * 1.7f + 1.1f ))- 0.58f; // http://www.fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIoMS4wLygoeC0wLjEpKih4LTAuMSkqMS43KzEuMSkpLTAuNTgiLCJjb2xvciI6IiMwMDAwMDAifSx7InR5cGUiOjEwMDAsIndpbmRvdyI6WyItMC41OTk1NTEyNjc1Njk4MjUiLCIxLjI3Mzk0NjE3NzQxNjI5ODgiLCItMC4xMTQzMjE1NjkyMTMwMTAwOCIsIjEuMDM4NTk5OTM1MzkzODM1MyJdLCJzaXplIjpbNjQ5LDM5OV19XQ--
    //shadow_factor = -shadow_factor * shadow_factor * 0.1f + 0.10f; // http://www.fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIoLXgqMC4xMCswLjEwKSIsImNvbG9yIjoiIzAwMDAwMCJ9LHsidHlwZSI6MTAwMCwid2luZG93IjpbIi0wLjU5OTU1MTI2NzU2OTgyNSIsIjEuMjczOTQ2MTc3NDE2Mjk4OCIsIi0wLjExNDMyMTU2OTIxMzAxMDA4IiwiMS4wMzg1OTk5MzUzOTM4MzUzIl0sInNpemUiOls2NDksMzk5XX1d
    shadow_factor = -shadow_factor * shadow_factor * 0.2f + 0.15f; // http://www.fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIoLXgqMC4yKzAuMTUpIiwiY29sb3IiOiIjMDAwMDAwIn0seyJ0eXBlIjoxMDAwLCJ3aW5kb3ciOlsiLTAuNTk5NTUxMjY3NTY5ODI1IiwiMS4yNzM5NDYxNzc0MTYyOTg4IiwiLTAuMTE0MzIxNTY5MjEzMDEwMDgiLCIxLjAzODU5OTkzNTM5MzgzNTMiXSwic2l6ZSI6WzY0OSwzOTldfV0-
    //shadow_factor = -shadow_factor * shadow_factor * 0.3f + 0.25f; // http://www.fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIoLXgqMC4zKzAuMjUpIiwiY29sb3IiOiIjMDAwMDAwIn0seyJ0eXBlIjoxMDAwLCJ3aW5kb3ciOlsiLTAuNTk5NTUxMjY3NTY5ODI1IiwiMS4yNzM5NDYxNzc0MTYyOTg4IiwiLTAuMTE0MzIxNTY5MjEzMDEwMDgiLCIxLjAzODU5OTkzNTM5MzgzNTMiXSwic2l6ZSI6WzY0OSwzOTldfV0-
    //shadow_factor = -shadow_factor * shadow_factor * 0.4f + 0.40f; // http://www.fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIoLXgqMC40MCswLjM1KSIsImNvbG9yIjoiIzAwMDAwMCJ9LHsidHlwZSI6MTAwMCwid2luZG93IjpbIi0wLjU5OTU1MTI2NzU2OTgyNSIsIjEuMjczOTQ2MTc3NDE2Mjk4OCIsIi0wLjExNDMyMTU2OTIxMzAxMDA4IiwiMS4wMzg1OTk5MzUzOTM4MzUzIl0sInNpemUiOls2NDksMzk5XX1d
    shadow_factor = glm::max( shadow_factor, 0.00f );

    // Calculate the edges ambient oclusion
    // /////////////////////////////////////////////////////////////////////////

    //if( (rd > FLT_EPSILON) && (rd < 0.2f) ) // This limit to the zero of the function (see below)
    //if( rd > FLT_EPSILON )
    if( (rd > FLT_EPSILON) && (rd < 1.0f) )
    {
        const SFVEC3F vv = glm::normalize( ddiff );

        // Calculate an attenuation distance factor, this was get the best
        // results by experimentation
        // Changing this factor will change how much shadow in relation to the
        // distance of the hit it will be in shadow

        float attDistFactor = 0.0f;

        // http://www.fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIxLSh4Lyh4LzIrMC4wMTUpKSIsImNvbG9yIjoiIzAwMDAwMCJ9LHsidHlwZSI6MTAwMCwid2luZG93IjpbIi0wLjAzMTM1ODM0NTk2MDIzOTAyIiwiMC4wNDUzODAxMDkzODYzOTI2MyIsIi0wLjAyMjM1MDcxNDk0NzUxMjk0IiwiMC4wMjQ4NzI5NDk4ODExODM0ODIiXX1d
        // zero: 0.03
        //attDistFactor = 1.0f - (rd / (rd/2.0f  + 0.015f));

        // http://www.fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIxLSh4Lyh4LzIrMC4xMCkpIiwiY29sb3IiOiIjMDAwMDAwIn0seyJ0eXBlIjoxMDAwLCJ3aW5kb3ciOlsiLTAuOTI4NjMzODAzMTQ3MTcyOSIsIjAuOTQ0ODYzNjQxODM4OTQ5NyIsIi0wLjE1MjA2MTc2MjkxMzQxMjI4IiwiMS4wMDA4NTk3NDE2OTM0MzI4Il19XQ--
        // zero: 0.2


        // http://www.fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIwLjgtKHgvKHgvMiswLjE1KSkiLCJjb2xvciI6IiMwMDAwMDAifSx7InR5cGUiOjAsImVxIjoiLXgqMC4wNSswLjI1IiwiY29sb3IiOiIjMDAwMDAwIn0seyJ0eXBlIjoxMDAwLCJ3aW5kb3ciOlsiLTAuMjE1NzI4MDU1ODgzMjU4NjYiLCIyLjEyNjE0Mzc1MDM0OTM4ODciLCItMC4wOTM1NDA0NzY0MjczNjA0MiIsIjEuMzQ3NjExNDA0MzMxMTkyNCJdfV0-
        // zero: 0.2
        // attDistFactor = 0.8f - (rd / (rd / 2.0f  + 0.15f));
        // attDistFactor = glm::max( attDistFactor, -rd * 0.05f + 0.25f );

        // http://www.fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIwLjgtKHgvKHgvMC45MCswLjEzKSkiLCJjb2xvciI6IiMwMDAwMDAifSx7InR5cGUiOjAsImVxIjoiKC14KjAuMiswLjE1KSIsImNvbG9yIjoiIzAwMDAwMCJ9LHsidHlwZSI6MTAwMCwid2luZG93IjpbIi0wLjIxNTcyODA1NTg4MzI1ODY2IiwiMi4xMjYxNDM3NTAzNDkzODg3IiwiLTAuMDkzNTQwNDc2NDI3MzYwNDIiLCIxLjM0NzYxMTQwNDMzMTE5MjQiXSwic2l6ZSI6WzY0OSwzOTldfV0-
        // zero: 1.0
        attDistFactor = 0.8f - (rd / (rd / 0.90f  + 0.13f));


        // Original:
        // http://www.fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIoMS0xL3NxcnQoMS8oeCp4KSsxKSkiLCJjb2xvciI6IiMwMDAwMDAifSx7InR5cGUiOjEwMDAsIndpbmRvdyI6WyItMC42ODY3NDc3NDcxMDg0MTQyIiwiMy44ODcyMjA2MjQ0Mzk3MzM0IiwiLTAuOTA5NTYyNzcyOTMyNDk2IiwiMS45MDUxODY5OTQxNzQwNTczIl19XQ--
        // zero: inf
        //const float attDistFactor = (1.0f - 1.0f / sqrt( 1.0f / ( rd * rd) + 1.0f) );

        //float attDistFactor = 1.0f;

        const float aaFactor = (1.0f - glm::clamp( glm::dot( GetNormalAt( vr ), -vv ), 0.0f, 1.0f) ) *
                               glm::clamp( glm::dot( cnorm, vv ), 0.0f, 1.0f ) * attDistFactor;

        return_value = (aaFactor * 1.0f + shadow_factor ) * 1.00f;

        // Test / Debug code
        //return_value = glm::max( aaFactor, shadow_factor );
        //return_value = aaFactor;
        //return_value = shadow_factor;
    }
    else
    {
        return_value = ( 0.0f + shadow_factor ) * 1.00f;

        // Test / Debug code
        //return_value = 0.0f;
    }

    return glm::clamp( return_value, 0.0f, 1.0f );
}


float CPOSTSHADER_SSAO::giFF( const SFVEC2I &aShaderPos,
                              const SFVEC3F &ddiff,
                              const SFVEC3F &cnorm,
                              int c1,
                              int c2 ) const
{
    if( (ddiff.x > FLT_EPSILON) ||
        (ddiff.y > FLT_EPSILON) ||
        (ddiff.z > FLT_EPSILON) )
    {
        const SFVEC3F vv = glm::normalize( ddiff );
        const float rd = glm::length( ddiff );
        const SFVEC2I vr = aShaderPos + SFVEC2I( c1, c2 );

        return glm::clamp( glm::dot( GetNormalAt( vr ), -vv), 0.0f, 1.0f ) *
               glm::clamp( glm::dot( cnorm, vv ), 0.0f, 1.0f ) / ( rd * rd + 1.0f );
    }

    return 0.0f;
}


SFVEC3F CPOSTSHADER_SSAO::Shade( const SFVEC2I &aShaderPos ) const
{
    // Test source code
    //return SFVEC3F( GetShadowFactorAt( aShaderPos ) );
    //return GetColorAt( aShaderPos );
    //return SFVEC3F( 1.0f - GetDepthNormalizedAt( aShaderPos ) );
    //return SFVEC3F( (1.0f / GetDepthAt( aShaderPos )) * 0.5f );
    //return SFVEC3F( 1.0f - GetDepthNormalizedAt( aShaderPos ) +
    //                (1.0f / GetDepthAt( aShaderPos )) * 0.5f );

#if 1
    float cdepth = GetDepthAt( aShaderPos );

    if( cdepth > FLT_EPSILON )
    {

        float cNormalizedDepth = GetDepthNormalizedAt( aShaderPos );

        wxASSERT( cNormalizedDepth <= 1.0f );
        wxASSERT( cNormalizedDepth >= 0.0f );

        cdepth = ( (1.50f - cNormalizedDepth) +
                   ( 1.0f - (1.0f / (cdepth + 1.0f) ) ) * 2.5f );

        // Test source code
        //cdepth = ( (1.75f - cNormalizedDepth) + (1.0f / cdepth) * 2.0f );
        //cdepth = 1.5f - cNormalizedDepth;
        //cdepth = (1.0f / cdepth) * 2.0f;

        // read current normal,position and color.
        const SFVEC3F n = GetNormalAt( aShaderPos );
        const SFVEC3F p = GetPositionAt( aShaderPos );
        //const SFVEC3F col = GetColorAt( aShaderPos );

        const float shadowFactor = GetShadowFactorAt( aShaderPos );

        // initialize variables:
        float ao = 0.0f;
        SFVEC3F gi = SFVEC3F(0.0f);

        // This calculated the "window range" of the shader. So it will get
        // more or less sparsed samples
        const int incx = 3;
        const int incy = 3;

        //3 rounds of 8 samples each.
        for( unsigned int i = 0; i < 3; ++i )
        {
            static const int mask[3] = { 0x01, 0x03, 0x03 };
            const int pw = 1 + (Fast_rand() & mask[i]);
            const int ph = 1 + (Fast_rand() & mask[i]);

            const int npw = (int)((pw + incx * i) * cdepth );
            const int nph = (int)((ph + incy * i) * cdepth );

            const SFVEC3F ddiff  = GetPositionAt( aShaderPos + SFVEC2I( npw, nph ) ) - p;
            const SFVEC3F ddiff2 = GetPositionAt( aShaderPos + SFVEC2I( npw,-nph ) ) - p;
            const SFVEC3F ddiff3 = GetPositionAt( aShaderPos + SFVEC2I(-npw, nph ) ) - p;
            const SFVEC3F ddiff4 = GetPositionAt( aShaderPos + SFVEC2I(-npw,-nph ) ) - p;
            const SFVEC3F ddiff5 = GetPositionAt( aShaderPos + SFVEC2I(   0, nph ) ) - p;
            const SFVEC3F ddiff6 = GetPositionAt( aShaderPos + SFVEC2I(   0,-nph ) ) - p;
            const SFVEC3F ddiff7 = GetPositionAt( aShaderPos + SFVEC2I( npw,   0 ) ) - p;
            const SFVEC3F ddiff8 = GetPositionAt( aShaderPos + SFVEC2I(-npw,   0 ) ) - p;

            ao+=  aoFF( aShaderPos, ddiff , n,  npw, nph, shadowFactor );
            ao+=  aoFF( aShaderPos, ddiff2, n,  npw,-nph, shadowFactor );
            ao+=  aoFF( aShaderPos, ddiff3, n, -npw, nph, shadowFactor );
            ao+=  aoFF( aShaderPos, ddiff4, n, -npw,-nph, shadowFactor );
            ao+=  aoFF( aShaderPos, ddiff5, n,    0, nph, shadowFactor );
            ao+=  aoFF( aShaderPos, ddiff6, n,    0,-nph, shadowFactor );
            ao+=  aoFF( aShaderPos, ddiff7, n,  npw,   0, shadowFactor );
            ao+=  aoFF( aShaderPos, ddiff8, n, -npw,   0, shadowFactor );

            gi+=  giFF( aShaderPos, ddiff , n, npw,  nph) *
                    giColorCurve( GetColorAt( aShaderPos + SFVEC2I(  npw, nph ) ) );
            gi+=  giFF( aShaderPos, ddiff2, n, npw, -nph) *
                    giColorCurve( GetColorAt( aShaderPos + SFVEC2I(  npw,-nph ) ) );
            gi+=  giFF( aShaderPos, ddiff3, n,-npw,  nph) *
                    giColorCurve( GetColorAt( aShaderPos + SFVEC2I( -npw, nph ) ) );
            gi+=  giFF( aShaderPos, ddiff4, n,-npw, -nph) *
                    giColorCurve( GetColorAt( aShaderPos + SFVEC2I( -npw,-nph ) ) );
            gi+=  giFF( aShaderPos, ddiff5, n, 0.0f, nph) *
                    giColorCurve( GetColorAt( aShaderPos + SFVEC2I(    0, nph ) ) );
            gi+=  giFF( aShaderPos, ddiff6, n, 0.0f,-nph) *
                    giColorCurve( GetColorAt( aShaderPos + SFVEC2I(    0,-nph ) ) );
            gi+=  giFF( aShaderPos, ddiff7, n, npw, 0.0f) *
                    giColorCurve( GetColorAt( aShaderPos + SFVEC2I(  npw,    0) ) );
            gi+=  giFF( aShaderPos, ddiff8, n,-npw, 0.0f) *
                    giColorCurve( GetColorAt( aShaderPos + SFVEC2I( -npw,    0) ) );
        }
        ao = (ao / 24.0f) + 0.0f; // Apply a bias for the ambient oclusion
        gi = (gi * 5.0f / 24.0f); // Apply a bias for the global illumination

        //return SFVEC3F(ao);
        return SFVEC3F( SFVEC3F(ao) - gi);

        // Test source code
        //return SFVEC3F( col );
        //return SFVEC3F( col - SFVEC3F(ao) + gi * 5.0f );
        //return SFVEC3F( SFVEC3F(1.0f) - SFVEC3F(ao) + gi * 5.0f );
        //return SFVEC3F(cdepth);
        //return 1.0f - SFVEC3F(ao);
        //return SFVEC3F(ao);
    }
    else
        return SFVEC3F(0.0f);
#endif
}


SFVEC3F CPOSTSHADER_SSAO::giColorCurve( const SFVEC3F &aColor ) const
{
    const SFVEC3F vec1 = SFVEC3F(1.0f);

    // http://fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIxLjAtKDEvKHgqMS4wKzEuMCkpK3gqMC4xIiwiY29sb3IiOiIjMDAwMDAwIn0seyJ0eXBlIjoxMDAwLCJ3aW5kb3ciOlsiLTAuMDYyMTg0NjE1Mzg0NjE1NTA1IiwiMS4xNDI5ODQ2MTUzODQ2MTQ2IiwiLTAuMTI3MDk5OTk5OTk5OTk5NzciLCIxLjEzMjYiXX1d
    return vec1 - ( vec1 / (aColor + vec1) ) + aColor * SFVEC3F(0.10f);

    // http://fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIxLjAtKDEuMC8oeCoyLjArMS4wKSkreCowLjEiLCJjb2xvciI6IiMwMDAwMDAifSx7InR5cGUiOjEwMDAsIndpbmRvdyI6WyItMC4wNjIxODQ2MTUzODQ2MTU1MDUiLCIxLjE0Mjk4NDYxNTM4NDYxNDYiLCItMC4xMjcwOTk5OTk5OTk5OTk3NyIsIjEuMTMyNiJdfV0-
    //return vec1 - ( vec1 / (aColor * SFVEC3F(2.0f) + vec1) ) + aColor * SFVEC3F(0.10f);

    //return aColor;
}
