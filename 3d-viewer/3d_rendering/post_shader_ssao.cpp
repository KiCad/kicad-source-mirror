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
 * @file post_shader_ssao.cpp
 * @brief Implement a post shader screen space ambient occlusion in software.
 */

#include "post_shader_ssao.h"
#include "../3d_fastmath.h"


POST_SHADER_SSAO::POST_SHADER_SSAO( const CAMERA& aCamera ) :
        POST_SHADER( aCamera ),
        m_shadedBuffer( nullptr ),
        m_isUsingShadows( false )
{
}

// There are different sources for this shader on the web
//https://github.com/scanberg/hbao/blob/master/resources/shaders/ssao_frag.glsl

//http://www.gamedev.net/topic/556187-the-best-ssao-ive-seen/
//http://www.gamedev.net/topic/556187-the-best-ssao-ive-seen/?view=findpost&p=4632208

float POST_SHADER_SSAO::aoFF( const SFVEC2I& aShaderPos, const SFVEC3F& ddiff,
                              const SFVEC3F& cnorm, const float aShadowAtSamplePos,
                              const float aShadowAtCenterPos, int c1, int c2 ) const
{
    const float shadowGain = 0.60f;
    const float aoGain = 1.0f;

    const float shadow_factor_at_sample = ( 1.0f - aShadowAtSamplePos ) * shadowGain;
    const float shadow_factor_at_center = ( 1.0f - aShadowAtCenterPos ) * shadowGain;

    float return_value = shadow_factor_at_center;

    const float rd = glm::length( ddiff );

    // This limits the zero of the function (see below)
    if( rd < 2.0f )
    {
        if( rd > FLT_EPSILON )
        {
            const SFVEC3F vv = glm::normalize( ddiff );

            // Calculate an attenuation distance factor, this was get the best
            // results by experimentation
            // Changing this factor will change how much shadow in relation to the
            // distance of the hit it will be in shadow

            // http://www.fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIwLjgteCowLjYiLCJjb2xvciI6IiMwMDAwMDAifSx7InR5cGUiOjAsImVxIjoiMS8oeCp4KjAuNSsxKSIsImNvbG9yIjoiIzAwMDAwMCJ9LHsidHlwZSI6MTAwMCwid2luZG93IjpbIi0yLjU5Mjk0NTkyNTA5ODA0MSIsIjQuNTUzODc5NjU1NDQ1OTIzIiwiLTEuNzY3MDMwOTAzMjgxNjgxOCIsIjIuNjMxMDE1NjA3ODIyMjk3Il0sInNpemUiOls2NDksMzk5XX1d
            const float attDistFactor = 1.0f / ( rd * rd * 8.0f + 1.0f );

            const SFVEC2I vr = aShaderPos + SFVEC2I( c1, c2 );

            float sampledNormalFactor = glm::max( glm::dot( GetNormalAt( vr ), cnorm ), 0.0f );

            sampledNormalFactor = glm::max( 1.0f - sampledNormalFactor *
                                            sampledNormalFactor, 0.0f );

            const float shadowAttDistFactor = glm::max( glm::min( rd * 5.0f - 0.25f, 1.0f ), 0.0f );

            float shadowAttFactor = glm::min( sampledNormalFactor + shadowAttDistFactor, 1.0f );

            const float shadowFactor = glm::mix( shadow_factor_at_sample, shadow_factor_at_center,
                                                 shadowAttFactor );

            // This is a dot product threshold factor.
            // it defines after which angle we consider that the point starts to occlude.
            // if the value is high, it will discard low angles point
            const float aDotThreshold = 0.15f;

            // This is the dot product between the center pixel normal (the one that is being
            // shaded) and the vector from the center to the sampled point
            const float localNormalFactor = glm::dot( cnorm, vv );

            const float localNormalFactorWithThreshold =
                    ( glm::max( localNormalFactor, aDotThreshold ) - aDotThreshold ) /
                    ( 1.0f - aDotThreshold );

            const float aoFactor = localNormalFactorWithThreshold * aoGain * attDistFactor;

            return_value = glm::min( aoFactor + shadowFactor, 1.0f );
        }
    }

    return return_value;
}


float POST_SHADER_SSAO::giFF( const SFVEC2I& aShaderPos, const SFVEC3F& ddiff,
                              const SFVEC3F& cnorm, const float aShadow, int c1, int c2 ) const
{
    if( ( ddiff.x > FLT_EPSILON ) || ( ddiff.y > FLT_EPSILON ) || ( ddiff.z > FLT_EPSILON ) )
    {
        const SFVEC3F vv = glm::normalize( ddiff );
        const float rd = glm::length( ddiff );
        const SFVEC2I vr = aShaderPos + SFVEC2I( c1, c2 );

        const float attDistFactor = 1.0f / ( rd * rd + 1.0f );

        return ( glm::clamp( glm::dot( GetNormalAt( vr ), -vv), 0.0f, 1.0f ) *
                 glm::clamp( glm::dot( cnorm, vv ), 0.0f, 1.0f ) * attDistFactor ) *
                 ( 0.03f + aShadow ) * 3.0f;
    }

    return 0.0f;
}


SFVEC3F POST_SHADER_SSAO::Shade( const SFVEC2I& aShaderPos ) const
{
    float cdepth = GetDepthAt( aShaderPos );

    if( cdepth > FLT_EPSILON )
    {
        cdepth = ( 30.0f / ( cdepth * 2.0f + 1.0f ) );

        // read current normal, position and color.
        const SFVEC3F n = GetNormalAt( aShaderPos );
        const SFVEC3F p = GetPositionAt( aShaderPos );

        const float shadowAt0 = GetShadowFactorAt( aShaderPos );

        // initialize variables:
        float ao = 0.0f;
        SFVEC3F gi = SFVEC3F( 0.0f );

#define ROUNDS 3
        for( unsigned int i = 0; i < ROUNDS; ++i )
        {
            static const int limit[ROUNDS] = { 0x01, 0x03, 0x03 };

            const int pw = Fast_rand() & limit[i];
            const int ph = Fast_rand() & limit[i];

            const int npw = (int) ( ( pw + i ) * cdepth ) + ( i + 1 );
            const int nph = (int) ( ( ph + i ) * cdepth ) + ( i + 1 );

            const SFVEC3F ddiff  = GetPositionAt( aShaderPos + SFVEC2I(  npw,  nph ) ) - p;
            const SFVEC3F ddiff2 = GetPositionAt( aShaderPos + SFVEC2I(  npw, -nph ) ) - p;
            const SFVEC3F ddiff3 = GetPositionAt( aShaderPos + SFVEC2I( -npw,  nph ) ) - p;
            const SFVEC3F ddiff4 = GetPositionAt( aShaderPos + SFVEC2I( -npw, -nph ) ) - p;
            const SFVEC3F ddiff5 = GetPositionAt( aShaderPos + SFVEC2I(  pw,   nph ) ) - p;
            const SFVEC3F ddiff6 = GetPositionAt( aShaderPos + SFVEC2I(  pw,  -nph ) ) - p;
            const SFVEC3F ddiff7 = GetPositionAt( aShaderPos + SFVEC2I( npw,    ph ) ) - p;
            const SFVEC3F ddiff8 = GetPositionAt( aShaderPos + SFVEC2I(-npw,    ph ) ) - p;

            const float shadowAt1 = GetShadowFactorAt( aShaderPos + SFVEC2I( +npw,  nph ) );
            const float shadowAt2 = GetShadowFactorAt( aShaderPos + SFVEC2I( +npw, -nph ) );
            const float shadowAt3 = GetShadowFactorAt( aShaderPos + SFVEC2I( -npw,  nph ) );
            const float shadowAt4 = GetShadowFactorAt( aShaderPos + SFVEC2I( -npw, -nph ) );
            const float shadowAt5 = GetShadowFactorAt( aShaderPos + SFVEC2I(  +pw,  nph ) );
            const float shadowAt6 = GetShadowFactorAt( aShaderPos + SFVEC2I(   pw, -nph ) );
            const float shadowAt7 = GetShadowFactorAt( aShaderPos + SFVEC2I(  npw,   ph ) );
            const float shadowAt8 = GetShadowFactorAt( aShaderPos + SFVEC2I( -npw,   ph ) );

            ao += aoFF( aShaderPos, ddiff , n, shadowAt1, shadowAt0,  npw,  nph );
            ao += aoFF( aShaderPos, ddiff2, n, shadowAt2, shadowAt0,  npw, -nph );
            ao += aoFF( aShaderPos, ddiff3, n, shadowAt3, shadowAt0, -npw,  nph );
            ao += aoFF( aShaderPos, ddiff4, n, shadowAt4, shadowAt0, -npw, -nph );
            ao += aoFF( aShaderPos, ddiff5, n, shadowAt5, shadowAt0,   pw,  nph );
            ao += aoFF( aShaderPos, ddiff6, n, shadowAt6, shadowAt0,   pw, -nph );
            ao += aoFF( aShaderPos, ddiff7, n, shadowAt7, shadowAt0,  npw,   ph );
            ao += aoFF( aShaderPos, ddiff8, n, shadowAt8, shadowAt0, -npw,   ph );

            gi += giFF( aShaderPos, ddiff , n, shadowAt1, npw,  nph) *
                    giColorCurveShade( GetColorAt( aShaderPos + SFVEC2I(  npw, nph ) ) );
            gi += giFF( aShaderPos, ddiff2, n, shadowAt2, npw, -nph) *
                    giColorCurveShade( GetColorAt( aShaderPos + SFVEC2I(  npw,-nph ) ) );
            gi += giFF( aShaderPos, ddiff3, n, shadowAt3, -npw,  nph) *
                    giColorCurveShade( GetColorAt( aShaderPos + SFVEC2I( -npw, nph ) ) );
            gi += giFF( aShaderPos, ddiff4, n, shadowAt4, -npw, -nph) *
                    giColorCurveShade( GetColorAt( aShaderPos + SFVEC2I( -npw,-nph ) ) );
            gi += giFF( aShaderPos, ddiff5, n, shadowAt5 , pw, nph) *
                    giColorCurveShade( GetColorAt( aShaderPos + SFVEC2I(   pw, nph ) ) );
            gi += giFF( aShaderPos, ddiff6, n, shadowAt6,  pw,-nph) *
                    giColorCurveShade( GetColorAt( aShaderPos + SFVEC2I(   pw,-nph ) ) );
            gi += giFF( aShaderPos, ddiff7, n, shadowAt7,  npw, ph) *
                    giColorCurveShade( GetColorAt( aShaderPos + SFVEC2I(  npw,  ph ) ) );
            gi += giFF( aShaderPos, ddiff8, n, shadowAt8, -npw, ph) *
                    giColorCurveShade( GetColorAt( aShaderPos + SFVEC2I( -npw,  ph ) ) );
        }

        // If it received direct light, it shouldn't consider much AO
        // shadowAt0 1.0 when no shadow
        const float reduceAOwhenNoShadow = m_isUsingShadows ? ( 1.0f - shadowAt0 * 0.3f ) : 1.0f;

        ao = reduceAOwhenNoShadow * ( ao / ( ROUNDS * 8.0f ) );

        ao = ( 1.0f - 1.0f / ( ao * ao * 5.0f + 1.0f ) ) * 1.2f;

        gi = ( gi / ( ROUNDS * 8.0f ) );

        float giL = glm::min( glm::length( gi ) * 4.0f, 1.0f );

        giL = ( 1.0f - 1.0f / ( giL * 4.0f + 1.0f ) ) * 1.5f;

        return glm::mix( SFVEC3F( ao ), -gi, giL );
    }
    else
    {
        return SFVEC3F( 0.0f );
    }
}


SFVEC4F POST_SHADER_SSAO::ApplyShadeColor( const SFVEC2I& aShaderPos, const SFVEC4F& aInputColor,
                                           const SFVEC3F& aShadeColor ) const
{
    SFVEC4F outColor;
    SFVEC3F inColor( aInputColor );

    const SFVEC3F subtracted = inColor - aShadeColor;
    const SFVEC3F mixed = glm::mix( inColor, inColor * 0.50f - aShadeColor * 0.05f,
                                    glm::min( aShadeColor, 1.0f ) );

    outColor.r = ( aShadeColor.r < 0.0f ) ? subtracted.r : mixed.r;
    outColor.g = ( aShadeColor.g < 0.0f ) ? subtracted.g : mixed.g;
    outColor.b = ( aShadeColor.b < 0.0f ) ? subtracted.b : mixed.b;
    outColor.a = std::max( aInputColor.a, ( aShadeColor.r + aShadeColor.g + aShadeColor.b ) / 3 );

    return outColor;
}


SFVEC3F POST_SHADER_SSAO::giColorCurve( const SFVEC3F& aColor ) const
{
    const SFVEC3F vec1 = SFVEC3F( 1.0f );

    // This option actually apply a gamma since we are using linear color space
    // and the result shader will be applied after convert back to sRGB

    // http://fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIxLjAtKDEuMC8oeCo5LjArMS4wKSkreCowLjEiLCJjb2xvciI6IiMwMDAwMDAifSx7InR5cGUiOjEwMDAsIndpbmRvdyI6WyItMC4wNjIxODQ2MTUzODQ2MTU1MDUiLCIxLjE0Mjk4NDYxNTM4NDYxNDYiLCItMC4xMjcwOTk5OTk5OTk5OTk3NyIsIjEuMTMyNiJdfV0-
    return vec1 - ( vec1 / (aColor * SFVEC3F(9.0f) + vec1) ) + aColor * SFVEC3F(0.10f);
}


SFVEC4F POST_SHADER_SSAO::giColorCurve( const SFVEC4F& aColor ) const
{
    return SFVEC4F( giColorCurve( SFVEC3F( aColor ) ), aColor.a );
}


SFVEC3F POST_SHADER_SSAO::giColorCurveShade( const SFVEC4F& aColor ) const
{
    return giColorCurve( SFVEC3F( aColor ) );
}


SFVEC3F POST_SHADER_SSAO::Blur( const SFVEC2I& aShaderPos ) const
{
    const float dCenter = GetDepthAt( aShaderPos );

    SFVEC3F shadedOut = SFVEC3F( 0.0f );

    float totalWeight = 1.0f;

    for( int y = -3; y < 3; y++ )
    {
        for( int x = -3; x < 3; x++ )
        {

            const unsigned int idx = GetIndex( SFVEC2I( aShaderPos.x + x, aShaderPos.y + y ) );

            const SFVEC3F s = m_shadedBuffer[idx];

            if( !( ( x == 0 ) && ( y == 0 ) ) )
            {

                const float d = GetDepthAt( SFVEC2I( aShaderPos.x + x, aShaderPos.y + y ) );

                // Increasing the value will get more sharpness effect.
                const float depthAtt = ( dCenter - d ) * dCenter * 25.0f;

                const float depthAttSqr = depthAtt * depthAtt;

                float weight = ( 1.0f / ( depthAttSqr + 1.0f ) ) - 0.02f * depthAttSqr;

                weight = glm::max( weight, 0.0f );

                shadedOut += s * weight;
                totalWeight += weight;
            }
            else
            {
                shadedOut += s;
            }
        }
    }

    return shadedOut / totalWeight;
}
