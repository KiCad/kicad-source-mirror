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

// There are differente sources for this shader on the web
//https://github.com/scanberg/hbao/blob/master/resources/shaders/ssao_frag.glsl

//http://www.gamedev.net/topic/556187-the-best-ssao-ive-seen/
//http://www.gamedev.net/topic/556187-the-best-ssao-ive-seen/?view=findpost&p=4632208

float CPOSTSHADER_SSAO::aoFF( const SFVEC2I &aShaderPos,
                              const SFVEC3F &ddiff,
                              const SFVEC3F &cnorm,
                              int c1,
                              int c2 ) const
{
    const float shadowGain = 0.5f;
    const float aoGain = 1.0f;
    const float outGain = 0.80f;

    float return_value = 0.0f;

    const float rd = glm::length( ddiff );

    // This limits the zero of the function (see below)
    if( rd < 1.0f )
    {
        const SFVEC2I vr = aShaderPos + SFVEC2I( c1, c2 );

        const float shadow_factor_at_sample = ( 1.0f - GetShadowFactorAt( vr ) ) * shadowGain;

        if( rd > FLT_EPSILON )
        {
            const SFVEC3F vv = glm::normalize( ddiff );

            // Calculate an attenuation distance factor, this was get the best
            // results by experimentation
            // Changing this factor will change how much shadow in relation to the
            // distance of the hit it will be in shadow

            // http://www.fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIwLjYteCowLjQ1IiwiY29sb3IiOiIjMDAwMDAwIn0seyJ0eXBlIjowLCJlcSI6IiIsImNvbG9yIjoiIzAwMDAwMCJ9LHsidHlwZSI6MTAwMCwid2luZG93IjpbIi0wLjIxNTcyODA1NTg4MzI1ODYiLCIyLjEyNjE0Mzc1MDM0OTM4ODciLCItMC4wOTM1NDA0NzY0MjczNjAzIiwiMS4zNDc2MTE0MDQzMzExOTIyIl0sInNpemUiOls2NDksMzk5XX1d
            // zero: 1.0
            const float attDistFactor = 0.6f - rd * 0.6f;

            // Original:
            // http://www.fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIoMS0xL3NxcnQoMS8oeCp4KSsxKSkiLCJjb2xvciI6IiMwMDAwMDAifSx7InR5cGUiOjEwMDAsIndpbmRvdyI6WyItMC42ODY3NDc3NDcxMDg0MTQyIiwiMy44ODcyMjA2MjQ0Mzk3MzM0IiwiLTAuOTA5NTYyNzcyOTMyNDk2IiwiMS45MDUxODY5OTQxNzQwNTczIl19XQ--
            // zero: inf
            //const float attDistFactor = (1.0f - 1.0f / sqrt( 1.0f / ( rd * rd) + 1.0f) );

            //const float attDistFactor = 1.0f;


            // Tool for visualize dot product:
            // http://www.falstad.com/dotproduct/

            // This is a dot product threshold factor.
            // it defines after wich angle we consider that the point starts to occlude.
            // if the value is high, it will distart low angles point
            const float aDotThreshold = 0.15f;

            // This is the normal factor using the normal at the sampled point (of the shader)
            // agaisnt the vector from the center to the position at sampled point
            const float sampledNormalFactor = glm::dot( GetNormalAt( vr ), -vv );

            // http://www.fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIobWF4KHgsMC4zKS0wLjMpLygxLTAuMykiLCJjb2xvciI6IiMwMDAwMDAifSx7InR5cGUiOjEwMDAsIndpbmRvdyI6WyItMC42ODY3NDc3NDcxMDg0MTQyIiwiMy44ODcyMjA2MjQ0Mzk3MzM0IiwiLTAuOTA5NTYyNzcyOTMyNDk2IiwiMS45MDUxODY5OTQxNzQwNTczIl19XQ--

            const float sampledNormalFactorWithThreshold = (glm::max( sampledNormalFactor, aDotThreshold ) - aDotThreshold) /
                                                           (1.0f - aDotThreshold);


            // This is the dot product between the center pixel (the one that is being shaded)
            // and the vector from the center to the sampled point
            const float localNormalFactor = glm::dot( cnorm, vv );

            const float localNormalFactorWithThreshold = (glm::max( localNormalFactor, aDotThreshold ) - aDotThreshold) /
                                                         (1.0f - aDotThreshold);


            const float aoFactor = (1.0f - sampledNormalFactorWithThreshold) *
                                   localNormalFactorWithThreshold *
                                   aoGain;

            return_value = ( ( aoFactor + shadow_factor_at_sample ) * attDistFactor );

            // Test / Debug code
            //return_value = glm::max( aaFactor, shadow_factor );
            //return_value = aaFactor;
            //return_value = shadow_factor;
            //return_value = glm::clamp( aaFactor, 0.0f, 1.0f );
        }
        else
        {
            return_value = shadow_factor_at_sample;
        }
    }

    return return_value * outGain;
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

        //const float cNormalizedDepth = GetDepthNormalizedAt( aShaderPos );
        //wxASSERT( cNormalizedDepth <= 1.0f );
        //wxASSERT( cNormalizedDepth >= 0.0f );

        cdepth = (10.0f / (cdepth + 1.0f) );

        // read current normal,position and color.
        const SFVEC3F n = GetNormalAt( aShaderPos );
        const SFVEC3F p = GetPositionAt( aShaderPos );
        //const SFVEC3F col = GetColorAt( aShaderPos );

        // initialize variables:
        float ao = 0.0f;
        SFVEC3F gi = SFVEC3F(0.0f);

        // This calculated the "window range" of the shader. So it will get
        // more or less sparsed samples
        const int incx = 2;
        const int incy = 2;

        //3 rounds of 8 samples each.
        for( unsigned int i = 0; i < 3; ++i )
        {
            static const int mask[3] = { 0x01, 0x03, 0x03 };
            const int pw = 0 + (Fast_rand() & mask[i]);
            const int ph = 0 + (Fast_rand() & mask[i]);

            const int npw = (int)((pw + incx * i) * cdepth ) + (i + 1);
            const int nph = (int)((ph + incy * i) * cdepth ) + (i + 1);

            const SFVEC3F ddiff  = GetPositionAt( aShaderPos + SFVEC2I( npw, nph ) ) - p;
            const SFVEC3F ddiff2 = GetPositionAt( aShaderPos + SFVEC2I( npw,-nph ) ) - p;
            const SFVEC3F ddiff3 = GetPositionAt( aShaderPos + SFVEC2I(-npw, nph ) ) - p;
            const SFVEC3F ddiff4 = GetPositionAt( aShaderPos + SFVEC2I(-npw,-nph ) ) - p;
            const SFVEC3F ddiff5 = GetPositionAt( aShaderPos + SFVEC2I(  pw, nph ) ) - p;
            const SFVEC3F ddiff6 = GetPositionAt( aShaderPos + SFVEC2I(  pw,-nph ) ) - p;
            const SFVEC3F ddiff7 = GetPositionAt( aShaderPos + SFVEC2I( npw,  ph ) ) - p;
            const SFVEC3F ddiff8 = GetPositionAt( aShaderPos + SFVEC2I(-npw,  ph ) ) - p;

            ao+=  aoFF( aShaderPos, ddiff , n,  npw, nph );
            ao+=  aoFF( aShaderPos, ddiff2, n,  npw,-nph );
            ao+=  aoFF( aShaderPos, ddiff3, n, -npw, nph );
            ao+=  aoFF( aShaderPos, ddiff4, n, -npw,-nph );
            ao+=  aoFF( aShaderPos, ddiff5, n,   pw, nph );
            ao+=  aoFF( aShaderPos, ddiff6, n,   pw,-nph );
            ao+=  aoFF( aShaderPos, ddiff7, n,  npw,  ph );
            ao+=  aoFF( aShaderPos, ddiff8, n, -npw,  ph );

            gi+=  giFF( aShaderPos, ddiff , n, npw,  nph) *
                    giColorCurve( GetColorAt( aShaderPos + SFVEC2I(  npw, nph ) ) );
            gi+=  giFF( aShaderPos, ddiff2, n, npw, -nph) *
                    giColorCurve( GetColorAt( aShaderPos + SFVEC2I(  npw,-nph ) ) );
            gi+=  giFF( aShaderPos, ddiff3, n,-npw,  nph) *
                    giColorCurve( GetColorAt( aShaderPos + SFVEC2I( -npw, nph ) ) );
            gi+=  giFF( aShaderPos, ddiff4, n,-npw, -nph) *
                    giColorCurve( GetColorAt( aShaderPos + SFVEC2I( -npw,-nph ) ) );
            gi+=  giFF( aShaderPos, ddiff5, n, pw, nph) *
                    giColorCurve( GetColorAt( aShaderPos + SFVEC2I(   pw, nph ) ) );
            gi+=  giFF( aShaderPos, ddiff6, n, pw,-nph) *
                    giColorCurve( GetColorAt( aShaderPos + SFVEC2I(   pw,-nph ) ) );
            gi+=  giFF( aShaderPos, ddiff7, n, npw, ph) *
                    giColorCurve( GetColorAt( aShaderPos + SFVEC2I(  npw,  ph ) ) );
            gi+=  giFF( aShaderPos, ddiff8, n,-npw, ph) *
                    giColorCurve( GetColorAt( aShaderPos + SFVEC2I( -npw,  ph ) ) );
        }
        ao = (ao / 24.0f) + 0.0f; // Apply a bias for the ambient oclusion
        gi = (gi * 5.0f / 24.0f); // Apply a bias for the global illumination

        //return SFVEC3F(ao);
        return SFVEC3F(ao) - gi;

        // Test source code
        //return SFVEC3F( col );
        //return SFVEC3F( col - SFVEC3F(ao) + gi * 5.0f );
        //return SFVEC3F( SFVEC3F(1.0f) - SFVEC3F(ao) + gi * 5.0f );
        //return SFVEC3F(cdepth);
        //return SFVEC3F(cNormalizedDepth);
        //return 1.0f - SFVEC3F(ao);
        //return SFVEC3F(ao);
    }
    else
        return SFVEC3F(0.0f);
#endif
}


SFVEC3F CPOSTSHADER_SSAO::ApplyShadeColor( const SFVEC2I &aShaderPos, const SFVEC3F &aInputColor, const SFVEC3F &aShadeColor ) const
{
    // This is the final stage of the shader and make the last calculation how to apply the shader
    const SFVEC3F shadedColor = aInputColor - ( -aShadeColor * (aShadeColor * SFVEC3F(0.1f) - SFVEC3F(1.0f) ) );

    return shadedColor;
}


SFVEC3F CPOSTSHADER_SSAO::giColorCurve( const SFVEC3F &aColor ) const
{
    const SFVEC3F vec1 = SFVEC3F(1.0f);

    // http://fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIxLjAtKDEvKHgqMS4wKzEuMCkpK3gqMC4xIiwiY29sb3IiOiIjMDAwMDAwIn0seyJ0eXBlIjoxMDAwLCJ3aW5kb3ciOlsiLTAuMDYyMTg0NjE1Mzg0NjE1NTA1IiwiMS4xNDI5ODQ2MTUzODQ2MTQ2IiwiLTAuMTI3MDk5OTk5OTk5OTk5NzciLCIxLjEzMjYiXX1d
    //return vec1 - ( vec1 / (aColor + vec1) ) + aColor * SFVEC3F(0.10f);

    // http://fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIxLjAtKDEuMC8oeCoyLjArMS4wKSkreCowLjEiLCJjb2xvciI6IiMwMDAwMDAifSx7InR5cGUiOjEwMDAsIndpbmRvdyI6WyItMC4wNjIxODQ2MTUzODQ2MTU1MDUiLCIxLjE0Mjk4NDYxNTM4NDYxNDYiLCItMC4xMjcwOTk5OTk5OTk5OTk3NyIsIjEuMTMyNiJdfV0-
    //return vec1 - ( vec1 / (aColor * SFVEC3F(2.0f) + vec1) ) + aColor * SFVEC3F(0.10f);

    // This option actually apply a gama since we are using linear color space
    // and the result shader will be applied after convert back to sRGB

    // http://fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIxLjAtKDEuMC8oeCo5LjArMS4wKSkreCowLjEiLCJjb2xvciI6IiMwMDAwMDAifSx7InR5cGUiOjEwMDAsIndpbmRvdyI6WyItMC4wNjIxODQ2MTUzODQ2MTU1MDUiLCIxLjE0Mjk4NDYxNTM4NDYxNDYiLCItMC4xMjcwOTk5OTk5OTk5OTk3NyIsIjEuMTMyNiJdfV0-
    return vec1 - ( vec1 / (aColor * SFVEC3F(9.0f) + vec1) ) + aColor * SFVEC3F(0.10f);

    // return aColor;
}
