/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2020 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 2015-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file render_3d_raytrace.cpp
 */

#include <gal/opengl/kiglew.h>    // Must be included first

#include <algorithm>
#include <atomic>
#include <chrono>
#include <climits>
#include <thread>

#include "render_3d_raytrace.h"
#include "mortoncodes.h"
#include "../color_rgb.h"
#include "3d_fastmath.h"
#include "3d_math.h"
#include "../common_ogl/ogl_utils.h"
#include <profile.h>        // To use GetRunningMicroSecs or another profiling utility
#include <wx/log.h>


RENDER_3D_RAYTRACE::RENDER_3D_RAYTRACE( EDA_3D_CANVAS* aCanvas, BOARD_ADAPTER& aAdapter, CAMERA& aCamera ) :
    RENDER_3D_BASE( aCanvas, aAdapter, aCamera ),
    m_postShaderSsao( aCamera )
{
    wxLogTrace( m_logTrace, wxT( "RENDER_3D_RAYTRACE::RENDER_3D_RAYTRACE" ) );

    m_openglSupportsVertexBufferObjects = false;
    m_pboId       = GL_NONE;
    m_pboDataSize = 0;
    m_accelerator = nullptr;
    m_convertedDummyBlockCount = 0;
    m_converted2dRoundSegmentCount = 0;
    m_oldWindowsSize.x = 0;
    m_oldWindowsSize.y = 0;
    m_outlineBoard2dObjects = nullptr;
    m_antioutlineBoard2dObjects = nullptr;
    m_firstHitinfo = nullptr;
    m_shaderBuffer = nullptr;
    m_cameraLight = nullptr;

    m_xoffset = 0;
    m_yoffset = 0;

    m_isPreview = false;
    m_renderState = RT_RENDER_STATE_MAX; // Set to an initial invalid state
    m_renderStartTime = 0;
    m_blockRenderProgressCount = 0;
}


RENDER_3D_RAYTRACE::~RENDER_3D_RAYTRACE()
{
    wxLogTrace( m_logTrace, wxT( "RENDER_3D_RAYTRACE::~RENDER_3D_RAYTRACE" ) );

    delete m_accelerator;
    m_accelerator = nullptr;

    delete m_outlineBoard2dObjects;
    m_outlineBoard2dObjects = nullptr;

    delete m_antioutlineBoard2dObjects;
    m_antioutlineBoard2dObjects = nullptr;

    delete[] m_shaderBuffer;
    m_shaderBuffer = nullptr;

    deletePbo();
}


int RENDER_3D_RAYTRACE::GetWaitForEditingTimeOut()
{
    return 1000; // ms
}


void RENDER_3D_RAYTRACE::deletePbo()
{
    // Delete PBO if it was created
    if( m_openglSupportsVertexBufferObjects )
    {
        if( glIsBufferARB( m_pboId ) )
            glDeleteBuffers( 1, &m_pboId );

        m_pboId = GL_NONE;
    }
}


void RENDER_3D_RAYTRACE::SetCurWindowSize( const wxSize& aSize )
{
    if( m_windowSize != aSize )
    {
        m_windowSize = aSize;
        glViewport( 0, 0, m_windowSize.x, m_windowSize.y );

        initializeNewWindowSize();
    }
}


void RENDER_3D_RAYTRACE::restartRenderState()
{
    m_renderStartTime = GetRunningMicroSecs();

    m_renderState = RT_RENDER_STATE_TRACING;
    m_blockRenderProgressCount = 0;

    m_postShaderSsao.InitFrame();

    m_blockPositionsWasProcessed.resize( m_blockPositions.size() );

    // Mark the blocks not processed yet
    std::fill( m_blockPositionsWasProcessed.begin(), m_blockPositionsWasProcessed.end(), 0 );
}


static inline void SetPixel( GLubyte* p, const COLOR_RGB& v )
{
    p[0] = v.c[0];
    p[1] = v.c[1];
    p[2] = v.c[2];
    p[3] = 255;
}


bool RENDER_3D_RAYTRACE::Redraw( bool aIsMoving, REPORTER* aStatusReporter,
                                 REPORTER* aWarningReporter )
{
    bool requestRedraw = false;

    // Initialize openGL if need
    if( !m_is_opengl_initialized )
    {
        if( !initializeOpenGL() )
            return false;

        //aIsMoving = true;
        requestRedraw = true;

        // It will assign the first time the windows size, so it will now
        // revert to preview mode the first time the Redraw is called
        m_oldWindowsSize = m_windowSize;
        initializeBlockPositions();
    }

    std::unique_ptr<BUSY_INDICATOR> busy = CreateBusyIndicator();

    // Reload board if it was requested
    if( m_reloadRequested )
    {
        if( aStatusReporter )
            aStatusReporter->Report( _( "Loading..." ) );

        //aIsMoving = true;
        requestRedraw = true;
        Reload( aStatusReporter, aWarningReporter, false );
    }


    // Recalculate constants if windows size was changed
    if( m_windowSize != m_oldWindowsSize )
    {
        m_oldWindowsSize = m_windowSize;
        aIsMoving = true;
        requestRedraw = true;

        initializeBlockPositions();
    }


    // Clear buffers
    glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
    glClearDepth( 1.0f );
    glClearStencil( 0x00 );
    glClear( GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

    // 4-byte pixel alignment
    glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );

    glDisable( GL_STENCIL_TEST );
    glDisable( GL_LIGHTING );
    glDisable( GL_COLOR_MATERIAL );
    glDisable( GL_DEPTH_TEST );
    glDisable( GL_TEXTURE_2D );
    glDisable( GL_BLEND );
    glDisable( GL_MULTISAMPLE );

    const bool was_camera_changed = m_camera.ParametersChanged();

    if( requestRedraw || aIsMoving || was_camera_changed )
        m_renderState = RT_RENDER_STATE_MAX; // Set to an invalid state,
                                             // so it will restart again latter

    // This will only render if need, otherwise it will redraw the PBO on the screen again
    if( aIsMoving || was_camera_changed )
    {
        // Set head light (camera view light) with the opposite direction of the camera
        if( m_cameraLight )
            m_cameraLight->SetDirection( -m_camera.GetDir() );

        OglDrawBackground( SFVEC3F( m_boardAdapter.m_BgColorTop),
                           SFVEC3F( m_boardAdapter.m_BgColorBot) );

        // Bind PBO
        glBindBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB, m_pboId );

        // Get the PBO pixel pointer to write the data
        GLubyte* ptrPBO = (GLubyte *)glMapBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB,
                                                     GL_WRITE_ONLY_ARB );

        if( ptrPBO )
        {
            renderPreview( ptrPBO );

            // release pointer to mapping buffer, this initialize the coping to PBO
            glUnmapBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB );
        }

        glWindowPos2i( m_xoffset, m_yoffset );
    }
    else
    {
        // Bind PBO
        glBindBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB, m_pboId );

        if( m_renderState != RT_RENDER_STATE_FINISH )
        {
            // Get the PBO pixel pointer to write the data
            GLubyte* ptrPBO = (GLubyte *)glMapBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB,
                                                         GL_WRITE_ONLY_ARB );

            if( ptrPBO )
            {
                render( ptrPBO, aStatusReporter );

                if( m_renderState != RT_RENDER_STATE_FINISH )
                    requestRedraw = true;

                // release pointer to mapping buffer, this initialize the coping to PBO
                glUnmapBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB );
            }
        }

        if( m_renderState == RT_RENDER_STATE_FINISH )
        {
            glClear( GL_COLOR_BUFFER_BIT );
        }

        glWindowPos2i( m_xoffset, m_yoffset );
    }

    // This way it will blend the progress rendering with the last buffer. eg:
    // if it was called after a openGL.
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable( GL_ALPHA_TEST );
    glDrawPixels( m_realBufferSize.x, m_realBufferSize.y, GL_RGBA, GL_UNSIGNED_BYTE, 0 );
    glBindBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB, 0 );

    return requestRedraw;
}


void RENDER_3D_RAYTRACE::render( GLubyte* ptrPBO, REPORTER* aStatusReporter )
{
    if( ( m_renderState == RT_RENDER_STATE_FINISH ) || ( m_renderState >= RT_RENDER_STATE_MAX ) )
    {
        restartRenderState();

        if( m_cameraLight )
            m_cameraLight->SetDirection( -m_camera.GetDir() );

        if( m_boardAdapter.GetRenderEngine() == RENDER_ENGINE::OPENGL_LEGACY )
        {
            // Set all pixels of PBO transparent (Alpha to 0)
            // This way it will draw the full buffer but only shows the updated (
            // already calculated) squares
            unsigned int nPixels = m_realBufferSize.x * m_realBufferSize.y;
            GLubyte* tmp_ptrPBO = ptrPBO + 3;   // PBO is RGBA

            for( unsigned int i = 0; i < nPixels; ++i )
            {
                *tmp_ptrPBO = 0;
                tmp_ptrPBO += 4;                // PBO is RGBA
            }
        }

        m_backgroundColorTop = ConvertSRGBToLinear( (SFVEC3F)m_boardAdapter.m_BgColorTop );
        m_backgroundColorBottom = ConvertSRGBToLinear( (SFVEC3F)m_boardAdapter.m_BgColorBot );
    }

    switch( m_renderState )
    {
    case RT_RENDER_STATE_TRACING:
        renderTracing( ptrPBO, aStatusReporter );
        break;

    case RT_RENDER_STATE_POST_PROCESS_SHADE:
        postProcessShading( ptrPBO, aStatusReporter );
        break;

    case RT_RENDER_STATE_POST_PROCESS_BLUR_AND_FINISH:
        postProcessBlurFinish( ptrPBO, aStatusReporter );
        break;

    default:
        wxASSERT_MSG( false, "Invalid state on m_renderState");
        restartRenderState();
        break;
    }

    if( aStatusReporter && ( m_renderState == RT_RENDER_STATE_FINISH ) )
    {
        // Calculation time in seconds
        const double elapsed_time = (double)( GetRunningMicroSecs() - m_renderStartTime ) / 1e6;

        aStatusReporter->Report( wxString::Format( _( "Rendering time %.3f s" ), elapsed_time ) );
    }
}


void RENDER_3D_RAYTRACE::renderTracing( GLubyte* ptrPBO, REPORTER* aStatusReporter )
{
    m_isPreview = false;

    auto startTime = std::chrono::steady_clock::now();
    bool breakLoop = false;

    std::atomic<size_t> numBlocksRendered( 0 );
    std::atomic<size_t> currentBlock( 0 );
    std::atomic<size_t> threadsFinished( 0 );

    size_t parallelThreadCount = std::min<size_t>(
            std::max<size_t>( std::thread::hardware_concurrency(), 2 ),
            m_blockPositions.size() );

    for( size_t ii = 0; ii < parallelThreadCount; ++ii )
    {
        std::thread t = std::thread( [&]()
        {
            for( size_t iBlock = currentBlock.fetch_add( 1 );
                 iBlock < m_blockPositions.size() && !breakLoop;
                 iBlock = currentBlock.fetch_add( 1 ) )
            {
                if( !m_blockPositionsWasProcessed[iBlock] )
                {
                    renderBlockTracing( ptrPBO, iBlock );
                    numBlocksRendered++;
                    m_blockPositionsWasProcessed[iBlock] = 1;

                    // Check if it spend already some time render and request to exit
                    // to display the progress
                    if( std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - startTime ).count() > 150 )
                        breakLoop = true;
                }
            }

            threadsFinished++;
        } );

        t.detach();
    }

    while( threadsFinished < parallelThreadCount )
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );

    m_blockRenderProgressCount += numBlocksRendered;

    if( aStatusReporter )
        aStatusReporter->Report( wxString::Format( _( "Rendering: %.0f %%" ),
                                                   (float) ( m_blockRenderProgressCount * 100 )
                                                   / (float) m_blockPositions.size() ) );

    // Check if it finish the rendering and if should continue to a post processing
    // or mark it as finished
    if( m_blockRenderProgressCount >= m_blockPositions.size() )
    {
        if( m_boardAdapter.GetFlag( FL_RENDER_RAYTRACING_POST_PROCESSING ) )
            m_renderState = RT_RENDER_STATE_POST_PROCESS_SHADE;
        else
            m_renderState = RT_RENDER_STATE_FINISH;
    }
}


#ifdef USE_SRGB_SPACE

/// @todo This should be removed in future when KiCad supports a greater version of glm lib.
#define SRGB_GAMA 2.4f


// This function implements the conversion from linear RGB to sRGB
// https://github.com/g-truc/glm/blob/master/glm/gtc/color_space.inl#L12
static SFVEC3F convertLinearToSRGB( const SFVEC3F& aRGBcolor )
{
    const float gammaCorrection = 1.0f / SRGB_GAMA;
    const SFVEC3F clampedColor = glm::clamp( aRGBcolor, SFVEC3F( 0.0f ), SFVEC3F( 1.0f ) );

    return glm::mix( glm::pow( clampedColor, SFVEC3F(gammaCorrection) ) * 1.055f - 0.055f,
                     clampedColor * 12.92f,
                     glm::lessThan( clampedColor, SFVEC3F(0.0031308f) ) );
}


// This function implements the conversion from sRGB to linear RGB
// https://github.com/g-truc/glm/blob/master/glm/gtc/color_space.inl#L35
SFVEC3F ConvertSRGBToLinear( const SFVEC3F& aSRGBcolor )
{
    const float gammaCorrection = SRGB_GAMA;

    return glm::mix( glm::pow( ( aSRGBcolor + SFVEC3F( 0.055f ) )
                               * SFVEC3F( 0.94786729857819905213270142180095f ),
                               SFVEC3F( gammaCorrection ) ),
                     aSRGBcolor * SFVEC3F( 0.07739938080495356037151702786378f ),
                     glm::lessThanEqual( aSRGBcolor, SFVEC3F( 0.04045f ) ) );
}

#endif


void RENDER_3D_RAYTRACE::renderFinalColor( GLubyte* ptrPBO, const SFVEC3F& rgbColor,
                                         bool applyColorSpaceConversion )
{
    SFVEC3F color = rgbColor;

#ifdef USE_SRGB_SPACE
    /// @note This should be used in future when the KiCad support a greater version of glm lib.
    // if( applyColorSpaceConversion )
    //    rgbColor = glm::convertLinearToSRGB( rgbColor );

    if( applyColorSpaceConversion )
        color = convertLinearToSRGB( rgbColor );
#endif

    ptrPBO[0] = (unsigned int) glm::clamp( (int) ( color.r * 255 ), 0, 255 );
    ptrPBO[1] = (unsigned int) glm::clamp( (int) ( color.g * 255 ), 0, 255 );
    ptrPBO[2] = (unsigned int) glm::clamp( (int) ( color.b * 255 ), 0, 255 );
    ptrPBO[3] = 255;
}


static void HITINFO_PACKET_init( HITINFO_PACKET* aHitPacket )
{
    // Initialize hitPacket with a "not hit" information
    for( unsigned int i = 0; i < RAYPACKET_RAYS_PER_PACKET; ++i )
    {
        aHitPacket[i].m_HitInfo.m_tHit = std::numeric_limits<float>::infinity();
        aHitPacket[i].m_HitInfo.m_acc_node_info = 0;
        aHitPacket[i].m_hitresult = false;
        aHitPacket[i].m_HitInfo.m_HitNormal = SFVEC3F( 0.0f );
        aHitPacket[i].m_HitInfo.m_ShadowFactor = 1.0f;
    }
}


void RENDER_3D_RAYTRACE::renderRayPackets( const SFVEC3F* bgColorY, const RAY* aRayPkt,
                                           HITINFO_PACKET* aHitPacket, bool is_testShadow,
                                           SFVEC3F* aOutHitColor )
{
    for( unsigned int y = 0, i = 0; y < RAYPACKET_DIM; ++y )
    {
        for( unsigned int x = 0; x < RAYPACKET_DIM; ++x, ++i )
        {
            if( aHitPacket[i].m_hitresult == true )
            {
                aOutHitColor[i] = shadeHit( bgColorY[y], aRayPkt[i], aHitPacket[i].m_HitInfo,
                                            false, 0, is_testShadow );
            }
            else
            {
                aOutHitColor[i] = bgColorY[y];
            }
        }
    }
}


void RENDER_3D_RAYTRACE::renderAntiAliasPackets( const SFVEC3F* aBgColorY,
                                                 const HITINFO_PACKET* aHitPck_X0Y0,
                                                 const HITINFO_PACKET* aHitPck_AA_X1Y1,
                                                 const RAY* aRayPck, SFVEC3F* aOutHitColor )
{
    const bool is_testShadow =  m_boardAdapter.GetFlag( FL_RENDER_RAYTRACING_SHADOWS );

    for( unsigned int y = 0, i = 0; y < RAYPACKET_DIM; ++y )
    {
        for( unsigned int x = 0; x < RAYPACKET_DIM; ++x, ++i )
        {
            const RAY& rayAA = aRayPck[i];

            HITINFO hitAA;
            hitAA.m_tHit = std::numeric_limits<float>::infinity();
            hitAA.m_acc_node_info = 0;

            bool hitted = false;

            const unsigned int idx0y1 = ( x + 0 ) + RAYPACKET_DIM * ( y + 1 );
            const unsigned int idx1y1 = ( x + 1 ) + RAYPACKET_DIM * ( y + 1 );

            // Gets the node info from the hit.
            const unsigned int nodex0y0 = aHitPck_X0Y0[ i ].m_HitInfo.m_acc_node_info;
            const unsigned int node_AA_x0y0 = aHitPck_AA_X1Y1[ i ].m_HitInfo.m_acc_node_info;

            unsigned int nodex1y0 = 0;

            if( x < (RAYPACKET_DIM - 1) )
                nodex1y0 = aHitPck_X0Y0[ i + 1 ].m_HitInfo.m_acc_node_info;

            unsigned int nodex0y1 = 0;

            if( y < ( RAYPACKET_DIM - 1 ) && idx0y1 < RAYPACKET_RAYS_PER_PACKET )
                nodex0y1 = aHitPck_X0Y0[idx0y1].m_HitInfo.m_acc_node_info;

            unsigned int nodex1y1 = 0;

            if( ( x < ( RAYPACKET_DIM - 1 ) ) && ( y < ( RAYPACKET_DIM - 1 ) )
              && idx1y1 < RAYPACKET_RAYS_PER_PACKET )
                nodex1y1 = aHitPck_X0Y0[idx1y1].m_HitInfo.m_acc_node_info;

            // If all notes are equal we assume there was no change on the object hits.
            if( ( ( nodex0y0 == nodex1y0 ) || ( nodex1y0 == 0 ) )
              && ( ( nodex0y0 == nodex0y1 ) || ( nodex0y1 == 0 ) )
              && ( ( nodex0y0 == nodex1y1 ) || ( nodex1y1 == 0 ) )
              && ( nodex0y0 == node_AA_x0y0 ) )
            {
                /// @todo Either get rid of the if statement above or do something with the
                ///       commented out code below.
                // Option 1
                // This option will give a very good quality on reflections (slow)
                /*
                if( m_accelerator->Intersect( rayAA, hitAA, nodex0y0 ) )
                {
                    aOutHitColor[i] += shadeHit( aBgColorY[y], rayAA, hitAA, false, 0 );
                }
                else
                {
                    if( m_accelerator->Intersect( rayAA, hitAA ) )
                        aOutHitColor[i] += shadeHit( aBgColorY[y], rayAA, hitAA, false, 0 );
                    else
                        aOutHitColor[i] += hitColor[i];
                }
                */

                // Option 2
                // Trace again with the same node,
                // then if miss just give the same color as before
                //if( m_accelerator->Intersect( rayAA, hitAA, nodex0y0 ) )
                //    aOutHitColor[i] += shadeHit( aBgColorY[y], rayAA, hitAA, false, 0 );

                // Option 3
                // Use same color
            }
            else
            {
                // Try to intersect the different nodes
                // It tests the possible combination of hitted or not hitted points
                // This will try to get the best hit for this ray

                if( nodex0y0 != 0 )
                    hitted |= m_accelerator->Intersect( rayAA, hitAA, nodex0y0 );

                if( ( nodex1y0 != 0 ) && ( nodex0y0 != nodex1y0 ) )
                    hitted |= m_accelerator->Intersect( rayAA, hitAA, nodex1y0 );

                if( ( nodex0y1 != 0 ) && ( nodex0y0 != nodex0y1 ) && ( nodex1y0 != nodex0y1 ) )
                    hitted |= m_accelerator->Intersect( rayAA, hitAA, nodex0y1 );

                if( ( nodex1y1 != 0 ) && ( nodex0y0 != nodex1y1 ) && ( nodex0y1 != nodex1y1 ) &&
                    ( nodex1y0 != nodex1y1 ) )
                    hitted |= m_accelerator->Intersect( rayAA, hitAA, nodex1y1 );

                if( (node_AA_x0y0 != 0 ) && ( nodex0y0 != node_AA_x0y0 ) &&
                    ( nodex0y1 != node_AA_x0y0 ) && ( nodex1y0 != node_AA_x0y0 ) &&
                    ( nodex1y1 != node_AA_x0y0 ) )
                    hitted |= m_accelerator->Intersect( rayAA, hitAA, node_AA_x0y0 );

                if( hitted )
                {
                    // If we got any result, shade it
                    aOutHitColor[i] = shadeHit( aBgColorY[y], rayAA, hitAA, false, 0,
                                                is_testShadow );
                }
                else
                {
                    // Note: There are very few cases that will end on this situation
                    // so it is not so expensive to trace a single ray from the beginning

                    // It was missed the 'last nodes' so, trace a ray from the beginning
                    if( m_accelerator->Intersect( rayAA, hitAA ) )
                        aOutHitColor[i] = shadeHit( aBgColorY[y], rayAA, hitAA, false, 0,
                                                    is_testShadow );
                }
            }
        }
    }
}


#define DISP_FACTOR 0.075f


void RENDER_3D_RAYTRACE::renderBlockTracing( GLubyte* ptrPBO, signed int iBlock )
{
    // Initialize ray packets
    const SFVEC2UI& blockPos = m_blockPositions[iBlock];
    const SFVEC2I blockPosI = SFVEC2I( blockPos.x + m_xoffset, blockPos.y + m_yoffset );

    RAYPACKET blockPacket( m_camera, (SFVEC2F) blockPosI + SFVEC2F( DISP_FACTOR, DISP_FACTOR ),
                           SFVEC2F( DISP_FACTOR, DISP_FACTOR ) /* Displacement random factor */ );


    HITINFO_PACKET hitPacket_X0Y0[RAYPACKET_RAYS_PER_PACKET];

    HITINFO_PACKET_init( hitPacket_X0Y0 );

    // Calculate background gradient color
    SFVEC3F bgColor[RAYPACKET_DIM];// Store a vertical gradient color

    for( unsigned int y = 0; y < RAYPACKET_DIM; ++y )
    {
        const float posYfactor = (float) ( blockPosI.y + y ) / (float) m_windowSize.y;

        bgColor[y] = m_backgroundColorTop * SFVEC3F(posYfactor) +
                     m_backgroundColorBottom * ( SFVEC3F(1.0f) - SFVEC3F(posYfactor) );
    }

    // Intersect ray packets (calculate the intersection with rays and objects)
    if( !m_accelerator->Intersect( blockPacket, hitPacket_X0Y0 ) )
    {
        // If block is empty then set shades and continue
        if( m_boardAdapter.GetFlag( FL_RENDER_RAYTRACING_POST_PROCESSING ) )
        {
            for( unsigned int y = 0; y < RAYPACKET_DIM; ++y )
            {
                const SFVEC3F& outColor = bgColor[y];

                const unsigned int yBlockPos = blockPos.y + y;

                for( unsigned int x = 0; x < RAYPACKET_DIM; ++x )
                {
                        m_postShaderSsao.SetPixelData( blockPos.x + x, yBlockPos,
                                                       SFVEC3F( 0.0f ), outColor,
                                                       SFVEC3F( 0.0f ), 0, 1.0f );
                }
            }
        }

        // This will set the output color to be displayed
        // If post processing is enabled, it will not reflect the final result
        // (as the final color will be computed on post processing)
        // but it is used for report progress
        const bool isFinalColor = !m_boardAdapter.GetFlag( FL_RENDER_RAYTRACING_POST_PROCESSING );

        for( unsigned int y = 0; y < RAYPACKET_DIM; ++y )
        {
            const SFVEC3F& outColor = bgColor[y];

            const unsigned int yConst = blockPos.x + ( ( y + blockPos.y ) * m_realBufferSize.x );

            for( unsigned int x = 0; x < RAYPACKET_DIM; ++x )
            {
                GLubyte* ptr = &ptrPBO[( yConst + x ) * 4];

                renderFinalColor( ptr, outColor, isFinalColor );
            }
        }

        // There is nothing more here to do.. there are no hits ..
        // just background so continue
        return;
    }

    SFVEC3F hitColor_X0Y0[RAYPACKET_RAYS_PER_PACKET];

    // Shade original (0, 0) hits ("paint" the intersected objects)
    renderRayPackets( bgColor, blockPacket.m_ray, hitPacket_X0Y0,
                      m_boardAdapter.GetFlag( FL_RENDER_RAYTRACING_SHADOWS ), hitColor_X0Y0 );

    if( m_boardAdapter.GetFlag( FL_RENDER_RAYTRACING_ANTI_ALIASING ) )
    {
        SFVEC3F hitColor_AA_X1Y1[RAYPACKET_RAYS_PER_PACKET];

        // Intersect one blockPosI + (0.5, 0.5) used for anti aliasing calculation
        HITINFO_PACKET hitPacket_AA_X1Y1[RAYPACKET_RAYS_PER_PACKET];
        HITINFO_PACKET_init( hitPacket_AA_X1Y1 );

        RAYPACKET blockPacket_AA_X1Y1( m_camera, (SFVEC2F) blockPosI + SFVEC2F( 0.5f, 0.5f ),
                                       SFVEC2F( DISP_FACTOR, DISP_FACTOR ) );

        if( !m_accelerator->Intersect( blockPacket_AA_X1Y1, hitPacket_AA_X1Y1 ) )
        {
            // Missed all the package
            for( unsigned int y = 0, i = 0; y < RAYPACKET_DIM; ++y )
            {
                const SFVEC3F& outColor = bgColor[y];

                for( unsigned int x = 0; x < RAYPACKET_DIM; ++x, ++i )
                {
                    hitColor_AA_X1Y1[i] = outColor;
                }
            }
        }
        else
        {
            renderRayPackets( bgColor, blockPacket_AA_X1Y1.m_ray, hitPacket_AA_X1Y1,
                              m_boardAdapter.GetFlag( FL_RENDER_RAYTRACING_SHADOWS ),
                              hitColor_AA_X1Y1 );
        }

        SFVEC3F hitColor_AA_X1Y0[RAYPACKET_RAYS_PER_PACKET];
        SFVEC3F hitColor_AA_X0Y1[RAYPACKET_RAYS_PER_PACKET];
        SFVEC3F hitColor_AA_X0Y1_half[RAYPACKET_RAYS_PER_PACKET];

        for( unsigned int i = 0; i < RAYPACKET_RAYS_PER_PACKET; ++i )
        {
            const SFVEC3F color_average = ( hitColor_X0Y0[i] +
                                            hitColor_AA_X1Y1[i] ) * SFVEC3F( 0.5f );

            hitColor_AA_X1Y0[i] = color_average;
            hitColor_AA_X0Y1[i] = color_average;
            hitColor_AA_X0Y1_half[i] = color_average;
        }

        RAY blockRayPck_AA_X1Y0[RAYPACKET_RAYS_PER_PACKET];
        RAY blockRayPck_AA_X0Y1[RAYPACKET_RAYS_PER_PACKET];
        RAY blockRayPck_AA_X1Y1_half[RAYPACKET_RAYS_PER_PACKET];

        RAYPACKET_InitRays_with2DDisplacement(
                m_camera, (SFVEC2F) blockPosI + SFVEC2F( 0.5f - DISP_FACTOR, DISP_FACTOR ),
                SFVEC2F( DISP_FACTOR, DISP_FACTOR ), blockRayPck_AA_X1Y0 );

        RAYPACKET_InitRays_with2DDisplacement(
                m_camera, (SFVEC2F) blockPosI + SFVEC2F( DISP_FACTOR, 0.5f - DISP_FACTOR ),
                SFVEC2F( DISP_FACTOR, DISP_FACTOR ), blockRayPck_AA_X0Y1 );

        RAYPACKET_InitRays_with2DDisplacement(
                m_camera, (SFVEC2F) blockPosI + SFVEC2F( 0.25f - DISP_FACTOR, 0.25f - DISP_FACTOR ),
                SFVEC2F( DISP_FACTOR, DISP_FACTOR ), blockRayPck_AA_X1Y1_half );

        renderAntiAliasPackets( bgColor, hitPacket_X0Y0, hitPacket_AA_X1Y1, blockRayPck_AA_X1Y0,
                                hitColor_AA_X1Y0 );

        renderAntiAliasPackets( bgColor, hitPacket_X0Y0, hitPacket_AA_X1Y1, blockRayPck_AA_X0Y1,
                                hitColor_AA_X0Y1 );

        renderAntiAliasPackets( bgColor, hitPacket_X0Y0, hitPacket_AA_X1Y1,
                                blockRayPck_AA_X1Y1_half, hitColor_AA_X0Y1_half );

        // Average the result
        for( unsigned int i = 0; i < RAYPACKET_RAYS_PER_PACKET; ++i )
        {
            hitColor_X0Y0[i] = ( hitColor_X0Y0[i] + hitColor_AA_X1Y1[i] + hitColor_AA_X1Y0[i] +
                                 hitColor_AA_X0Y1[i] + hitColor_AA_X0Y1_half[i] ) *
                    SFVEC3F( 1.0f / 5.0f );
        }
    }

    // Copy results to the next stage
    GLubyte* ptr = &ptrPBO[( blockPos.x + ( blockPos.y * m_realBufferSize.x ) ) * 4];

    const uint32_t ptrInc = ( m_realBufferSize.x - RAYPACKET_DIM ) * 4;

    if( m_boardAdapter.GetFlag( FL_RENDER_RAYTRACING_POST_PROCESSING ) )
    {
        SFVEC2I bPos;
        bPos.y = blockPos.y;

        for( unsigned int y = 0, i = 0; y < RAYPACKET_DIM; ++y )
        {
            bPos.x = blockPos.x;

            for( unsigned int x = 0; x < RAYPACKET_DIM; ++x, ++i )
            {
                const SFVEC3F& hColor = hitColor_X0Y0[i];

                if( hitPacket_X0Y0[i].m_hitresult == true )
                    m_postShaderSsao.SetPixelData( bPos.x, bPos.y,
                                                   hitPacket_X0Y0[i].m_HitInfo.m_HitNormal,
                                                   hColor,
                                                   blockPacket.m_ray[i].at(
                                                           hitPacket_X0Y0[i].m_HitInfo.m_tHit ),
                                                   hitPacket_X0Y0[i].m_HitInfo.m_tHit,
                                                   hitPacket_X0Y0[i].m_HitInfo.m_ShadowFactor );
                else
                    m_postShaderSsao.SetPixelData( bPos.x, bPos.y, SFVEC3F( 0.0f ), hColor,
                                                   SFVEC3F( 0.0f ), 0, 1.0f );

                renderFinalColor( ptr, hColor, false );

                bPos.x++;
                ptr += 4;
            }

            ptr += ptrInc;
            bPos.y++;
        }
    }
    else
    {
        for( unsigned int y = 0, i = 0; y < RAYPACKET_DIM; ++y )
        {
            for( unsigned int x = 0; x < RAYPACKET_DIM; ++x, ++i )
            {
                renderFinalColor( ptr, hitColor_X0Y0[i], true );
                ptr += 4;
            }

            ptr += ptrInc;
        }
    }
}


void RENDER_3D_RAYTRACE::postProcessShading( GLubyte* ptrPBO, REPORTER* aStatusReporter )
{
    (void)ptrPBO; // unused

    if( m_boardAdapter.GetFlag( FL_RENDER_RAYTRACING_POST_PROCESSING ) )
    {
        if( aStatusReporter )
            aStatusReporter->Report( _( "Rendering: Post processing shader" ) );

        m_postShaderSsao.SetShadowsEnabled(
                m_boardAdapter.GetFlag( FL_RENDER_RAYTRACING_SHADOWS ) );

        std::atomic<size_t> nextBlock( 0 );
        std::atomic<size_t> threadsFinished( 0 );

        size_t parallelThreadCount = std::max<size_t>( std::thread::hardware_concurrency(), 2 );

        for( size_t ii = 0; ii < parallelThreadCount; ++ii )
        {
            std::thread t = std::thread( [&]()
            {
                for( size_t y = nextBlock.fetch_add( 1 ); y < m_realBufferSize.y;
                     y = nextBlock.fetch_add( 1 ) )
                {
                    SFVEC3F* ptr = &m_shaderBuffer[ y * m_realBufferSize.x ];

                    for( signed int x = 0; x < (int)m_realBufferSize.x; ++x )
                    {
                        *ptr = m_postShaderSsao.Shade( SFVEC2I( x, y ) );
                        ptr++;
                    }
                }

                threadsFinished++;
            } );

            t.detach();
        }

        while( threadsFinished < parallelThreadCount )
            std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );

        m_postShaderSsao.SetShadedBuffer( m_shaderBuffer );

        // Set next state
        m_renderState = RT_RENDER_STATE_POST_PROCESS_BLUR_AND_FINISH;
    }
    else
    {
        // As this was an invalid state, set to finish
        m_renderState = RT_RENDER_STATE_FINISH;
    }
}


void RENDER_3D_RAYTRACE::postProcessBlurFinish( GLubyte* ptrPBO, REPORTER* aStatusReporter )
{
    (void) aStatusReporter; //unused

    if( m_boardAdapter.GetFlag( FL_RENDER_RAYTRACING_POST_PROCESSING ) )
    {
        // Now blurs the shader result and compute the final color
        std::atomic<size_t> nextBlock( 0 );
        std::atomic<size_t> threadsFinished( 0 );

        size_t parallelThreadCount = std::max<size_t>( std::thread::hardware_concurrency(), 2 );

        for( size_t ii = 0; ii < parallelThreadCount; ++ii )
        {
            std::thread t = std::thread( [&]()
            {
                for( size_t y = nextBlock.fetch_add( 1 ); y < m_realBufferSize.y;
                     y = nextBlock.fetch_add( 1 ) )
                {
                    GLubyte* ptr = &ptrPBO[ y * m_realBufferSize.x * 4 ];

                    for( signed int x = 0; x < (int)m_realBufferSize.x; ++x )
                    {
                        const SFVEC3F bluredShadeColor = m_postShaderSsao.Blur( SFVEC2I( x, y ) );

#ifdef USE_SRGB_SPACE
                        const SFVEC3F originColor = convertLinearToSRGB(
                                m_postShaderSsao.GetColorAtNotProtected( SFVEC2I( x, y ) ) );
#else
                        const SFVEC3F originColor =
                                m_postShaderSsao.GetColorAtNotProtected( SFVEC2I( x, y ) );
#endif
                        const SFVEC3F shadedColor = m_postShaderSsao.ApplyShadeColor(
                                SFVEC2I( x, y ), originColor, bluredShadeColor );

                        renderFinalColor( ptr, shadedColor, false );

                        ptr += 4;
                    }
                }

                threadsFinished++;
            } );

            t.detach();
        }

        while( threadsFinished < parallelThreadCount )
            std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );

        // Debug code
        //m_postShaderSsao.DebugBuffersOutputAsImages();
    }

    // End rendering
    m_renderState = RT_RENDER_STATE_FINISH;
}


void RENDER_3D_RAYTRACE::renderPreview( GLubyte* ptrPBO )
{
    m_isPreview = true;

    std::atomic<size_t> nextBlock( 0 );
    std::atomic<size_t> threadsFinished( 0 );

    size_t parallelThreadCount = std::min<size_t>(
            std::max<size_t>( std::thread::hardware_concurrency(), 2 ),
            m_blockPositions.size() );

    for( size_t ii = 0; ii < parallelThreadCount; ++ii )
    {
        std::thread t = std::thread( [&]()
        {
            for( size_t iBlock = nextBlock.fetch_add( 1 ); iBlock < m_blockPositionsFast.size();
                 iBlock = nextBlock.fetch_add( 1 ) )
            {
                const SFVEC2UI& windowPosUI = m_blockPositionsFast[ iBlock ];
                const SFVEC2I windowsPos = SFVEC2I( windowPosUI.x + m_xoffset,
                                                    windowPosUI.y + m_yoffset );

                RAYPACKET blockPacket( m_camera, windowsPos, 4 );

                HITINFO_PACKET hitPacket[RAYPACKET_RAYS_PER_PACKET];

                // Initialize hitPacket with a "not hit" information
                for( HITINFO_PACKET& packet : hitPacket )
                {
                    packet.m_HitInfo.m_tHit = std::numeric_limits<float>::infinity();
                    packet.m_HitInfo.m_acc_node_info = 0;
                    packet.m_hitresult = false;
                }

                //  Intersect packet block
                m_accelerator->Intersect( blockPacket, hitPacket );

                // Calculate background gradient color
                SFVEC3F bgColor[RAYPACKET_DIM];

                for( unsigned int y = 0; y < RAYPACKET_DIM; ++y )
                {
                    const float posYfactor =
                            (float) ( windowsPos.y + y * 4.0f ) / (float) m_windowSize.y;

                    bgColor[y] = (SFVEC3F) m_boardAdapter.m_BgColorTop * SFVEC3F( posYfactor )
                                 + (SFVEC3F) m_boardAdapter.m_BgColorBot
                                           * ( SFVEC3F( 1.0f ) - SFVEC3F( posYfactor ) );
                }

                COLOR_RGB hitColorShading[RAYPACKET_RAYS_PER_PACKET];

                for( unsigned int i = 0; i < RAYPACKET_RAYS_PER_PACKET; ++i )
                {
                    const SFVEC3F bhColorY = bgColor[i / RAYPACKET_DIM];

                    if( hitPacket[i].m_hitresult == true )
                    {
                        const SFVEC3F hitColor = shadeHit( bhColorY, blockPacket.m_ray[i],
                                                           hitPacket[i].m_HitInfo, false,
                                                           0, false );

                        hitColorShading[i] = COLOR_RGB( hitColor );
                    }
                    else
                        hitColorShading[i] = bhColorY;
                }

                COLOR_RGB cLRB_old[(RAYPACKET_DIM - 1)];

                for( unsigned int y = 0; y < (RAYPACKET_DIM - 1); ++y )
                {
                    const SFVEC3F     bgColorY = bgColor[y];
                    const COLOR_RGB   bgColorYRGB = COLOR_RGB( bgColorY );

                    // This stores cRTB from the last block to be reused next time in a cLTB pixel
                    COLOR_RGB cRTB_old;

                    //RAY       cRTB_ray;
                    //HITINFO   cRTB_hitInfo;

                    for( unsigned int x = 0; x < ( RAYPACKET_DIM - 1 ); ++x )
                    {
                        //      pxl 0  pxl 1  pxl 2  pxl 3  pxl 4
                        //        x0                          x1  ...
                        //     .---------------------------.
                        // y0  | cLT  | cxxx | cLRT | cxxx | cRT  |
                        //     | cxxx | cLTC | cxxx | cRTC | cxxx |
                        //     | cLTB | cxxx | cC   | cxxx | cRTB |
                        //     | cxxx | cLBC | cxxx | cRBC | cxxx |
                        //     '---------------------------'
                        // y1  | cLB  | cxxx | cLRB | cxxx | cRB  |

                        const unsigned int iLT = ( ( x + 0 ) + RAYPACKET_DIM * ( y + 0 ) );
                        const unsigned int iRT = ( ( x + 1 ) + RAYPACKET_DIM * ( y + 0 ) );
                        const unsigned int iLB = ( ( x + 0 ) + RAYPACKET_DIM * ( y + 1 ) );
                        const unsigned int iRB = ( ( x + 1 ) + RAYPACKET_DIM * ( y + 1 ) );

                        // !TODO: skip when there are no hits
                        const COLOR_RGB& cLT = hitColorShading[ iLT ];
                        const COLOR_RGB& cRT = hitColorShading[ iRT ];
                        const COLOR_RGB& cLB = hitColorShading[ iLB ];
                        const COLOR_RGB& cRB = hitColorShading[ iRB ];

                        // Trace and shade cC
                        COLOR_RGB cC = bgColorYRGB;

                        const SFVEC3F& oriLT = blockPacket.m_ray[ iLT ].m_Origin;
                        const SFVEC3F& oriRB = blockPacket.m_ray[ iRB ].m_Origin;

                        const SFVEC3F& dirLT = blockPacket.m_ray[ iLT ].m_Dir;
                        const SFVEC3F& dirRB = blockPacket.m_ray[ iRB ].m_Dir;

                        SFVEC3F oriC;
                        SFVEC3F dirC;

                        HITINFO centerHitInfo;
                        centerHitInfo.m_tHit = std::numeric_limits<float>::infinity();

                        bool hittedC = false;

                        if( ( hitPacket[iLT].m_hitresult == true )
                          || ( hitPacket[iRT].m_hitresult == true )
                          || ( hitPacket[iLB].m_hitresult == true )
                          || ( hitPacket[iRB].m_hitresult == true ) )
                        {
                            oriC = ( oriLT + oriRB ) * 0.5f;
                            dirC = glm::normalize( ( dirLT + dirRB ) * 0.5f );

                            // Trace the center ray
                            RAY centerRay;
                            centerRay.Init( oriC, dirC );

                            const unsigned int nodeLT = hitPacket[ iLT ].m_HitInfo.m_acc_node_info;
                            const unsigned int nodeRT = hitPacket[ iRT ].m_HitInfo.m_acc_node_info;
                            const unsigned int nodeLB = hitPacket[ iLB ].m_HitInfo.m_acc_node_info;
                            const unsigned int nodeRB = hitPacket[ iRB ].m_HitInfo.m_acc_node_info;

                            if( nodeLT != 0 )
                                hittedC |= m_accelerator->Intersect( centerRay, centerHitInfo,
                                                                     nodeLT );

                            if( ( nodeRT != 0 ) && ( nodeRT != nodeLT ) )
                                hittedC |= m_accelerator->Intersect( centerRay, centerHitInfo,
                                                                     nodeRT );

                            if( ( nodeLB != 0 ) && ( nodeLB != nodeLT ) && ( nodeLB != nodeRT ) )
                                hittedC |= m_accelerator->Intersect( centerRay, centerHitInfo,
                                                                     nodeLB );

                            if( ( nodeRB != 0 ) && ( nodeRB != nodeLB ) && ( nodeRB != nodeLT )
                              && ( nodeRB != nodeRT ) )
                                hittedC |= m_accelerator->Intersect( centerRay, centerHitInfo,
                                                                     nodeRB );

                            if( hittedC )
                            {
                                cC = COLOR_RGB( shadeHit( bgColorY, centerRay, centerHitInfo,
                                                          false, 0, false ) );
                            }
                            else
                            {
                                centerHitInfo.m_tHit = std::numeric_limits<float>::infinity();
                                hittedC = m_accelerator->Intersect( centerRay, centerHitInfo );

                                if( hittedC )
                                    cC = COLOR_RGB( shadeHit( bgColorY, centerRay, centerHitInfo,
                                                              false, 0, false ) );
                            }
                        }

                        // Trace and shade cLRT
                        COLOR_RGB cLRT = bgColorYRGB;

                        const SFVEC3F& oriRT = blockPacket.m_ray[ iRT ].m_Origin;
                        const SFVEC3F& dirRT = blockPacket.m_ray[ iRT ].m_Dir;

                        if( y == 0 )
                        {
                            // Trace the center ray
                            RAY rayLRT;
                            rayLRT.Init( ( oriLT + oriRT ) * 0.5f,
                                         glm::normalize( ( dirLT + dirRT ) * 0.5f ) );

                            HITINFO hitInfoLRT;
                            hitInfoLRT.m_tHit = std::numeric_limits<float>::infinity();

                            if( hitPacket[iLT].m_hitresult && hitPacket[iRT].m_hitresult
                                && ( hitPacket[iLT].m_HitInfo.pHitObject
                                     == hitPacket[iRT].m_HitInfo.pHitObject ) )
                            {
                                hitInfoLRT.pHitObject = hitPacket[ iLT ].m_HitInfo.pHitObject;
                                hitInfoLRT.m_tHit = ( hitPacket[ iLT ].m_HitInfo.m_tHit +
                                                      hitPacket[ iRT ].m_HitInfo.m_tHit ) * 0.5f;
                                hitInfoLRT.m_HitNormal =
                                        glm::normalize( ( hitPacket[ iLT ].m_HitInfo.m_HitNormal +
                                                          hitPacket[ iRT ].m_HitInfo.m_HitNormal ) * 0.5f );

                                cLRT = COLOR_RGB( shadeHit( bgColorY, rayLRT, hitInfoLRT, false,
                                                            0, false ) );
                                cLRT = BlendColor( cLRT, BlendColor( cLT, cRT ) );
                            }
                            else
                            {
                                // If any hits
                                if( hitPacket[ iLT ].m_hitresult || hitPacket[ iRT ].m_hitresult )
                                {
                                    const unsigned int nodeLT =
                                            hitPacket[ iLT ].m_HitInfo.m_acc_node_info;
                                    const unsigned int nodeRT =
                                            hitPacket[ iRT ].m_HitInfo.m_acc_node_info;

                                    bool hittedLRT = false;

                                    if( nodeLT != 0 )
                                        hittedLRT |= m_accelerator->Intersect( rayLRT, hitInfoLRT,
                                                                               nodeLT );

                                    if( ( nodeRT != 0 ) && ( nodeRT != nodeLT ) )
                                        hittedLRT |= m_accelerator->Intersect( rayLRT, hitInfoLRT,
                                                                               nodeRT );

                                    if( hittedLRT )
                                        cLRT = COLOR_RGB( shadeHit( bgColorY, rayLRT, hitInfoLRT,
                                                                    false, 0, false ) );
                                    else
                                    {
                                        hitInfoLRT.m_tHit = std::numeric_limits<float>::infinity();

                                        if( m_accelerator->Intersect( rayLRT,hitInfoLRT ) )
                                            cLRT = COLOR_RGB( shadeHit( bgColorY, rayLRT,
                                                                        hitInfoLRT, false,
                                                                        0, false ) );
                                    }
                                }
                            }
                        }
                        else
                        {
                            cLRT = cLRB_old[x];
                        }

                        // Trace and shade cLTB
                        COLOR_RGB cLTB = bgColorYRGB;

                        if( x == 0 )
                        {
                            const SFVEC3F &oriLB = blockPacket.m_ray[ iLB ].m_Origin;
                            const SFVEC3F& dirLB = blockPacket.m_ray[ iLB ].m_Dir;

                            // Trace the center ray
                            RAY rayLTB;
                            rayLTB.Init( ( oriLT + oriLB ) * 0.5f,
                                            glm::normalize( ( dirLT + dirLB ) * 0.5f ) );

                            HITINFO hitInfoLTB;
                            hitInfoLTB.m_tHit = std::numeric_limits<float>::infinity();

                            if( hitPacket[ iLT ].m_hitresult && hitPacket[ iLB ].m_hitresult
                              && ( hitPacket[ iLT ].m_HitInfo.pHitObject ==
                                   hitPacket[ iLB ].m_HitInfo.pHitObject ) )
                            {
                                hitInfoLTB.pHitObject = hitPacket[ iLT ].m_HitInfo.pHitObject;
                                hitInfoLTB.m_tHit = ( hitPacket[ iLT ].m_HitInfo.m_tHit +
                                                      hitPacket[ iLB ].m_HitInfo.m_tHit ) * 0.5f;
                                hitInfoLTB.m_HitNormal =
                                        glm::normalize( ( hitPacket[ iLT ].m_HitInfo.m_HitNormal +
                                                          hitPacket[ iLB ].m_HitInfo.m_HitNormal ) * 0.5f );
                                cLTB = COLOR_RGB( shadeHit( bgColorY, rayLTB, hitInfoLTB, false,
                                                            0, false ) );
                                cLTB = BlendColor( cLTB, BlendColor( cLT, cLB) );
                            }
                            else
                            {
                                // If any hits
                                if( hitPacket[ iLT ].m_hitresult || hitPacket[ iLB ].m_hitresult )
                                {
                                    const unsigned int nodeLT =
                                            hitPacket[ iLT ].m_HitInfo.m_acc_node_info;
                                    const unsigned int nodeLB =
                                            hitPacket[ iLB ].m_HitInfo.m_acc_node_info;

                                    bool hittedLTB = false;

                                    if( nodeLT != 0 )
                                        hittedLTB |= m_accelerator->Intersect( rayLTB, hitInfoLTB,
                                                                               nodeLT );

                                    if( ( nodeLB != 0 ) && ( nodeLB != nodeLT ) )
                                        hittedLTB |= m_accelerator->Intersect( rayLTB, hitInfoLTB,
                                                                               nodeLB );

                                    if( hittedLTB )
                                        cLTB = COLOR_RGB( shadeHit( bgColorY, rayLTB, hitInfoLTB,
                                                                    false, 0, false ) );
                                    else
                                    {
                                        hitInfoLTB.m_tHit = std::numeric_limits<float>::infinity();

                                        if( m_accelerator->Intersect( rayLTB, hitInfoLTB ) )
                                            cLTB = COLOR_RGB( shadeHit( bgColorY, rayLTB,
                                                                        hitInfoLTB, false,
                                                                        0, false ) );
                                    }
                                }
                            }
                        }
                        else
                        {
                            cLTB = cRTB_old;
                        }

                        // Trace and shade cRTB
                        COLOR_RGB cRTB = bgColorYRGB;

                        // Trace the center ray
                        RAY rayRTB;
                        rayRTB.Init( ( oriRT + oriRB ) * 0.5f,
                                     glm::normalize( ( dirRT + dirRB ) * 0.5f ) );

                        HITINFO hitInfoRTB;
                        hitInfoRTB.m_tHit = std::numeric_limits<float>::infinity();

                        if( hitPacket[ iRT ].m_hitresult && hitPacket[ iRB ].m_hitresult
                          && ( hitPacket[ iRT ].m_HitInfo.pHitObject ==
                               hitPacket[ iRB ].m_HitInfo.pHitObject ) )
                        {
                            hitInfoRTB.pHitObject = hitPacket[ iRT ].m_HitInfo.pHitObject;

                            hitInfoRTB.m_tHit = ( hitPacket[ iRT ].m_HitInfo.m_tHit +
                                                  hitPacket[ iRB ].m_HitInfo.m_tHit ) * 0.5f;

                            hitInfoRTB.m_HitNormal =
                                    glm::normalize( ( hitPacket[ iRT ].m_HitInfo.m_HitNormal +
                                                      hitPacket[ iRB ].m_HitInfo.m_HitNormal ) * 0.5f );

                            cRTB = COLOR_RGB( shadeHit( bgColorY, rayRTB, hitInfoRTB, false, 0,
                                                        false ) );
                            cRTB = BlendColor( cRTB, BlendColor( cRT, cRB ) );
                        }
                        else
                        {
                            // If any hits
                            if( hitPacket[ iRT ].m_hitresult || hitPacket[ iRB ].m_hitresult )
                            {
                                const unsigned int nodeRT =
                                        hitPacket[ iRT ].m_HitInfo.m_acc_node_info;
                                const unsigned int nodeRB =
                                        hitPacket[ iRB ].m_HitInfo.m_acc_node_info;

                                bool hittedRTB = false;

                                if( nodeRT != 0 )
                                    hittedRTB |= m_accelerator->Intersect( rayRTB, hitInfoRTB,
                                                                           nodeRT );

                                if( ( nodeRB != 0 ) && ( nodeRB != nodeRT ) )
                                    hittedRTB |= m_accelerator->Intersect( rayRTB, hitInfoRTB,
                                                                           nodeRB );

                                if( hittedRTB )
                                {
                                    cRTB = COLOR_RGB( shadeHit( bgColorY, rayRTB, hitInfoRTB,
                                                                false, 0, false) );
                                }
                                else
                                {
                                    hitInfoRTB.m_tHit = std::numeric_limits<float>::infinity();

                                    if( m_accelerator->Intersect( rayRTB, hitInfoRTB ) )
                                        cRTB = COLOR_RGB( shadeHit( bgColorY, rayRTB, hitInfoRTB,
                                                                    false, 0, false ) );
                                }
                            }
                        }

                        cRTB_old = cRTB;

                        // Trace and shade cLRB
                        COLOR_RGB cLRB = bgColorYRGB;

                        const SFVEC3F& oriLB = blockPacket.m_ray[ iLB ].m_Origin;
                        const SFVEC3F& dirLB = blockPacket.m_ray[ iLB ].m_Dir;

                        // Trace the center ray
                        RAY rayLRB;
                        rayLRB.Init( ( oriLB + oriRB ) * 0.5f,
                                     glm::normalize( ( dirLB + dirRB ) * 0.5f ) );

                        HITINFO hitInfoLRB;
                        hitInfoLRB.m_tHit = std::numeric_limits<float>::infinity();

                        if( hitPacket[iLB].m_hitresult && hitPacket[iRB].m_hitresult
                            && ( hitPacket[iLB].m_HitInfo.pHitObject ==
                                 hitPacket[iRB].m_HitInfo.pHitObject ) )
                        {
                            hitInfoLRB.pHitObject = hitPacket[ iLB ].m_HitInfo.pHitObject;

                            hitInfoLRB.m_tHit = ( hitPacket[ iLB ].m_HitInfo.m_tHit +
                                                  hitPacket[ iRB ].m_HitInfo.m_tHit ) * 0.5f;

                            hitInfoLRB.m_HitNormal =
                                    glm::normalize( ( hitPacket[ iLB ].m_HitInfo.m_HitNormal +
                                                      hitPacket[ iRB ].m_HitInfo.m_HitNormal ) * 0.5f );

                            cLRB = COLOR_RGB( shadeHit( bgColorY, rayLRB, hitInfoLRB, false, 0,
                                                        false ) );
                            cLRB = BlendColor( cLRB, BlendColor( cLB, cRB ) );
                        }
                        else
                        {
                            // If any hits
                            if( hitPacket[ iLB ].m_hitresult || hitPacket[ iRB ].m_hitresult )
                            {
                                const unsigned int nodeLB =
                                        hitPacket[ iLB ].m_HitInfo.m_acc_node_info;
                                const unsigned int nodeRB =
                                        hitPacket[ iRB ].m_HitInfo.m_acc_node_info;

                                bool hittedLRB = false;

                                if( nodeLB != 0 )
                                    hittedLRB |= m_accelerator->Intersect( rayLRB, hitInfoLRB,
                                                                           nodeLB );

                                if( ( nodeRB != 0 ) && ( nodeRB != nodeLB ) )
                                    hittedLRB |= m_accelerator->Intersect( rayLRB, hitInfoLRB,
                                                                           nodeRB );

                                if( hittedLRB )
                                {
                                    cLRB = COLOR_RGB( shadeHit( bgColorY, rayLRB, hitInfoLRB,
                                                                false, 0, false ) );
                                }
                                else
                                {
                                    hitInfoLRB.m_tHit = std::numeric_limits<float>::infinity();

                                    if( m_accelerator->Intersect( rayLRB, hitInfoLRB ) )
                                        cLRB = COLOR_RGB( shadeHit( bgColorY, rayLRB, hitInfoLRB,
                                                                    false, 0, false ) );
                                }
                            }
                        }

                        cLRB_old[x] = cLRB;

                        // Trace and shade cLTC
                        COLOR_RGB cLTC = BlendColor( cLT , cC );

                        if( hitPacket[ iLT ].m_hitresult || hittedC )
                        {
                            // Trace the center ray
                            RAY rayLTC;
                            rayLTC.Init( ( oriLT + oriC ) * 0.5f,
                                         glm::normalize( ( dirLT + dirC ) * 0.5f ) );

                            HITINFO hitInfoLTC;
                            hitInfoLTC.m_tHit = std::numeric_limits<float>::infinity();

                            bool hitted = false;

                            if( hittedC )
                                hitted = centerHitInfo.pHitObject->Intersect( rayLTC, hitInfoLTC );
                            else if( hitPacket[ iLT ].m_hitresult )
                                hitted = hitPacket[ iLT ].m_HitInfo.pHitObject->Intersect(
                                        rayLTC,
                                        hitInfoLTC );

                            if( hitted )
                                cLTC = COLOR_RGB( shadeHit( bgColorY, rayLTC, hitInfoLTC, false,
                                                            0, false ) );
                        }

                        // Trace and shade cRTC
                        COLOR_RGB cRTC = BlendColor( cRT , cC );

                        if( hitPacket[ iRT ].m_hitresult || hittedC )
                        {
                            // Trace the center ray
                            RAY rayRTC;
                            rayRTC.Init( ( oriRT + oriC ) * 0.5f,
                                         glm::normalize( ( dirRT + dirC ) * 0.5f ) );

                            HITINFO hitInfoRTC;
                            hitInfoRTC.m_tHit = std::numeric_limits<float>::infinity();

                            bool hitted = false;

                            if( hittedC )
                                hitted = centerHitInfo.pHitObject->Intersect( rayRTC, hitInfoRTC );
                            else if( hitPacket[ iRT ].m_hitresult )
                                hitted = hitPacket[ iRT ].m_HitInfo.pHitObject->Intersect( rayRTC,
                                                                                           hitInfoRTC );

                            if( hitted )
                                cRTC = COLOR_RGB( shadeHit( bgColorY, rayRTC, hitInfoRTC, false,
                                                            0, false ) );
                        }

                        // Trace and shade cLBC
                        COLOR_RGB cLBC = BlendColor( cLB , cC );

                        if( hitPacket[ iLB ].m_hitresult || hittedC )
                        {
                            // Trace the center ray
                            RAY rayLBC;
                            rayLBC.Init( ( oriLB + oriC ) * 0.5f,
                                         glm::normalize( ( dirLB + dirC ) * 0.5f ) );

                            HITINFO hitInfoLBC;
                            hitInfoLBC.m_tHit = std::numeric_limits<float>::infinity();

                            bool hitted = false;

                            if( hittedC )
                                hitted = centerHitInfo.pHitObject->Intersect( rayLBC, hitInfoLBC );
                            else if( hitPacket[ iLB ].m_hitresult )
                                hitted = hitPacket[ iLB ].m_HitInfo.pHitObject->Intersect( rayLBC,
                                                                                           hitInfoLBC );

                            if( hitted )
                                cLBC = COLOR_RGB( shadeHit( bgColorY, rayLBC, hitInfoLBC, false,
                                                            0, false ) );
                        }

                        // Trace and shade cRBC
                        COLOR_RGB cRBC = BlendColor( cRB , cC );

                        if( hitPacket[ iRB ].m_hitresult || hittedC )
                        {
                            // Trace the center ray
                            RAY rayRBC;
                            rayRBC.Init( ( oriRB + oriC ) * 0.5f,
                                         glm::normalize( ( dirRB + dirC ) * 0.5f ) );

                            HITINFO hitInfoRBC;
                            hitInfoRBC.m_tHit = std::numeric_limits<float>::infinity();

                            bool hitted = false;

                            if( hittedC )
                                hitted = centerHitInfo.pHitObject->Intersect( rayRBC, hitInfoRBC );
                            else if( hitPacket[ iRB ].m_hitresult )
                                hitted = hitPacket[ iRB ].m_HitInfo.pHitObject->Intersect( rayRBC,
                                                                                           hitInfoRBC );

                            if( hitted )
                                cRBC = COLOR_RGB( shadeHit( bgColorY, rayRBC, hitInfoRBC, false,
                                                            0, false ) );
                        }

                        // Set pixel colors
                        GLubyte* ptr =
                                &ptrPBO[( 4 * x + m_blockPositionsFast[iBlock].x
                                          + m_realBufferSize.x
                                          * ( m_blockPositionsFast[iBlock].y + 4 * y ) ) * 4];
                        SetPixel( ptr + 0, cLT );
                        SetPixel( ptr +  4, BlendColor( cLT, cLRT, cLTC ) );
                        SetPixel( ptr +  8, cLRT );
                        SetPixel( ptr + 12, BlendColor( cLRT, cRT, cRTC ) );

                        ptr += m_realBufferSize.x * 4;
                        SetPixel( ptr +  0, BlendColor( cLT , cLTB, cLTC ) );
                        SetPixel( ptr +  4, BlendColor( cLTC, BlendColor( cLT , cC ) ) );
                        SetPixel( ptr +  8, BlendColor( cC, BlendColor( cLRT, cLTC, cRTC ) ) );
                        SetPixel( ptr + 12, BlendColor( cRTC, BlendColor( cRT , cC ) ) );

                        ptr += m_realBufferSize.x * 4;
                        SetPixel( ptr +  0, cLTB );
                        SetPixel( ptr +  4, BlendColor( cC, BlendColor( cLTB, cLTC, cLBC ) ) );
                        SetPixel( ptr +  8, cC );
                        SetPixel( ptr + 12, BlendColor( cC, BlendColor( cRTB, cRTC, cRBC ) ) );

                        ptr += m_realBufferSize.x * 4;
                        SetPixel( ptr +  0, BlendColor( cLB , cLTB, cLBC ) );
                        SetPixel( ptr +  4, BlendColor( cLBC, BlendColor( cLB , cC ) ) );
                        SetPixel( ptr +  8, BlendColor( cC, BlendColor( cLRB, cLBC, cRBC ) ) );
                        SetPixel( ptr + 12, BlendColor( cRBC, BlendColor( cRB , cC ) ) );
                    }
                }
            }

            threadsFinished++;
        } );

        t.detach();
    }

    while( threadsFinished < parallelThreadCount )
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
}


#define USE_EXPERIMENTAL_SOFT_SHADOWS 1

SFVEC3F RENDER_3D_RAYTRACE::shadeHit( const SFVEC3F& aBgColor, const RAY& aRay, HITINFO& aHitInfo,
                                      bool aIsInsideObject, unsigned int aRecursiveLevel,
                                      bool is_testShadow ) const
{
    const MATERIAL* objMaterial = aHitInfo.pHitObject->GetMaterial();
    wxASSERT( objMaterial != nullptr );

    SFVEC3F outColor = objMaterial->GetEmissiveColor() + objMaterial->GetAmbientColor();

    if( aRecursiveLevel > 7 )
        return outColor;

    SFVEC3F hitPoint = aHitInfo.m_HitPoint;

    hitPoint += aHitInfo.m_HitNormal * m_boardAdapter.GetNonCopperLayerThickness() * 0.6f;

    const SFVEC3F diffuseColorObj = aHitInfo.pHitObject->GetDiffuseColor( aHitInfo );

#if USE_EXPERIMENTAL_SOFT_SHADOWS
    const bool is_aa_enabled = m_boardAdapter.GetFlag( FL_RENDER_RAYTRACING_ANTI_ALIASING ) &&
                               (!m_isPreview);
#endif

    float shadow_att_factor_sum = 0.0f;

    unsigned int nr_lights_that_can_cast_shadows = 0;

    for( const LIGHT* light : m_lights )
    {
        SFVEC3F vectorToLight;
        SFVEC3F colorOfLight;
        float   distToLight;

        light->GetLightParameters( hitPoint, vectorToLight, colorOfLight, distToLight );

        if( m_isPreview )
            colorOfLight = SFVEC3F( 1.0f );

        const float NdotL = glm::dot( aHitInfo.m_HitNormal, vectorToLight );

        // Only calc shade if the normal is facing the direction of light,
        // otherwise it is in the shadow
        if( NdotL >= FLT_EPSILON )
        {
            float shadow_att_factor_light = 1.0f;

            if( is_testShadow && light->GetCastShadows() )
            {
                nr_lights_that_can_cast_shadows++;
#if USE_EXPERIMENTAL_SOFT_SHADOWS
                // For rays that are recursive, just calculate one hit shadow
                if( aRecursiveLevel > 0 )
                {
#endif
                    RAY rayToLight;
                    rayToLight.Init( hitPoint, vectorToLight );

                    // Test if point is not in the shadow.
                    // Test for any hit from the point in the direction of light
                    if( m_accelerator->IntersectP( rayToLight, distToLight ) )
                        shadow_att_factor_light = 0.0f;

#if USE_EXPERIMENTAL_SOFT_SHADOWS
                }
                else  // Experimental softshadow calculation
                {

                    const unsigned int shadow_number_of_samples =
                            m_boardAdapter.m_RtShadowSampleCount;
                    const float shadow_inc_factor = 1.0f / (float) ( shadow_number_of_samples );

                    for( unsigned int i = 0; i < shadow_number_of_samples; ++i )
                    {
                        RAY rayToLight;

                        if( i == 0 )
                        {
                            rayToLight.Init( hitPoint, vectorToLight );
                        }
                        else
                        {
                            const SFVEC3F unifVector = UniformRandomHemisphereDirection();
                            const SFVEC3F disturbed_vector_to_light =
                                    glm::normalize( vectorToLight + unifVector *
                                                    m_boardAdapter.m_RtSpreadShadows );

                            rayToLight.Init( hitPoint, disturbed_vector_to_light );
                        }

                        // !TODO: there are multiple ways that this tests can be
                        // optimized. Eg: by packing rays or to test against the
                        // latest hit object.
                        if( m_accelerator->IntersectP( rayToLight, distToLight ) )
                        {
                            shadow_att_factor_light -= shadow_inc_factor;
                        }
                    }
                }
#endif
                shadow_att_factor_sum += shadow_att_factor_light;
            }

            outColor += objMaterial->Shade( aRay, aHitInfo, NdotL, diffuseColorObj, vectorToLight,
                                            colorOfLight, shadow_att_factor_light );
        }

        // Only use the headlight for preview
        if( m_isPreview )
            break;
    }

    // Improvement: this is not taking in account the lightcolor
    if( nr_lights_that_can_cast_shadows > 0 )
    {
        aHitInfo.m_ShadowFactor = glm::max(
                shadow_att_factor_sum / (float) ( nr_lights_that_can_cast_shadows * 1.0f ), 0.0f );
    }
    else
    {
        aHitInfo.m_ShadowFactor = 1.0f;
    }

    // Clamp color to not be brighter than 1.0f
    outColor = glm::min( outColor, SFVEC3F( 1.0f ) );

    if( !m_isPreview )
    {
        // Reflections
        if( ( objMaterial->GetReflection() > 0.0f )
          && m_boardAdapter.GetFlag( FL_RENDER_RAYTRACING_REFLECTIONS )
          && ( aRecursiveLevel < objMaterial->GetReflectionRecursionCount() ) )
        {
            const unsigned int reflection_number_of_samples =
                    objMaterial->GetReflectionRayCount();

            SFVEC3F sum_color = SFVEC3F( 0.0f );

            const SFVEC3F reflectVector = aRay.m_Dir - 2.0f *
                    glm::dot( aRay.m_Dir, aHitInfo.m_HitNormal ) * aHitInfo.m_HitNormal;

            for( unsigned int i = 0; i < reflection_number_of_samples; ++i )
            {
                RAY reflectedRay;

                if( i == 0 )
                {
                    reflectedRay.Init( hitPoint, reflectVector );
                }
                else
                {
                    // Apply some randomize to the reflected vector
                    const SFVEC3F random_reflectVector =
                            glm::normalize( reflectVector +
                                            UniformRandomHemisphereDirection() *
                                            m_boardAdapter.m_RtSpreadReflections );

                    reflectedRay.Init( hitPoint, random_reflectVector );
                }

                HITINFO reflectedHit;
                reflectedHit.m_tHit = std::numeric_limits<float>::infinity();

                if( m_accelerator->Intersect( reflectedRay, reflectedHit ) )
                {
                    sum_color += ( diffuseColorObj + objMaterial->GetSpecularColor() ) *
                                 shadeHit( aBgColor, reflectedRay, reflectedHit, false,
                                           aRecursiveLevel + 1, is_testShadow ) *
                                 SFVEC3F( objMaterial->GetReflection() *
                                          // Falloff factor
                                          (1.0f / ( 1.0f + 0.75f * reflectedHit.m_tHit *
                                                    reflectedHit.m_tHit) ) );
                }
            }

            outColor += (sum_color / SFVEC3F( (float)reflection_number_of_samples) );
        }

        // Refraction
        const float objTransparency = aHitInfo.pHitObject->GetModelTransparency();

        if( ( objTransparency > 0.0f ) && m_boardAdapter.GetFlag( FL_RENDER_RAYTRACING_REFRACTIONS )
          && ( aRecursiveLevel < objMaterial->GetRefractionRecursionCount() ) )
        {
            const float airIndex = 1.000293f;
            const float glassIndex = 1.49f;
            const float air_over_glass = airIndex / glassIndex;
            const float glass_over_air = glassIndex / airIndex;

            const float refractionRatio = aIsInsideObject?glass_over_air:air_over_glass;

            SFVEC3F refractedVector;

            if( Refract( aRay.m_Dir, aHitInfo.m_HitNormal, refractionRatio, refractedVector ) )
            {
                // This increase the start point by a "fixed" factor so it will work the
                // same for all distances
                const SFVEC3F startPoint =
                        aRay.at( aHitInfo.m_tHit + m_boardAdapter.GetNonCopperLayerThickness() *
                                 0.25f );

                const unsigned int refractions_number_of_samples =
                        objMaterial->GetRefractionRayCount();

                SFVEC3F sum_color = SFVEC3F(0.0f);

                for( unsigned int i = 0; i < refractions_number_of_samples; ++i )
                {
                    RAY refractedRay;

                    if( i == 0 )
                    {
                        refractedRay.Init( startPoint, refractedVector );
                    }
                    else
                    {
                        // apply some randomize to the refracted vector
                        const SFVEC3F randomizeRefractedVector =
                                glm::normalize( refractedVector +
                                                UniformRandomHemisphereDirection() *
                                                m_boardAdapter.m_RtSpreadRefractions );

                        refractedRay.Init( startPoint, randomizeRefractedVector );
                    }

                    HITINFO refractedHit;
                    refractedHit.m_tHit = std::numeric_limits<float>::infinity();

                    SFVEC3F refractedColor = aBgColor;

                    if( m_accelerator->Intersect( refractedRay, refractedHit ) )
                    {
                        refractedColor = shadeHit( aBgColor, refractedRay, refractedHit,
                                                   !aIsInsideObject, aRecursiveLevel + 1, false );

                        const SFVEC3F absorbance = ( SFVEC3F(1.0f) - diffuseColorObj ) *
                                                   (1.0f - objTransparency ) *
                                                   objMaterial->GetAbsorvance() *
                                                   refractedHit.m_tHit;

                        const SFVEC3F transparency = 1.0f / ( absorbance + 1.0f );

                        sum_color += refractedColor * transparency;
                    }
                    else
                    {
                        sum_color += refractedColor;
                    }
                }

                outColor = outColor * ( 1.0f - objTransparency ) + objTransparency * sum_color
                         / SFVEC3F( (float) refractions_number_of_samples );
            }
            else
            {
                outColor = outColor * ( 1.0f - objTransparency ) + objTransparency * aBgColor;
            }
        }
    }

    return outColor;
}


void RENDER_3D_RAYTRACE::initializeNewWindowSize()
{
    initPbo();
}


void RENDER_3D_RAYTRACE::initPbo()
{
    if( GLEW_ARB_pixel_buffer_object )
    {
        m_openglSupportsVertexBufferObjects = true;

        // Try to delete vbo if it was already initialized
        deletePbo();

        // Learn about Pixel buffer objects at:
        // http://www.songho.ca/opengl/gl_pbo.html
        // http://web.eecs.umich.edu/~sugih/courses/eecs487/lectures/25-PBO+Mipmapping.pdf
        // "create 2 pixel buffer objects, you need to delete them when program exits.
        // glBufferDataARB with NULL pointer reserves only memory space."

        // This sets the number of RGBA pixels
        m_pboDataSize =  m_realBufferSize.x * m_realBufferSize.y * 4;

        glGenBuffersARB( 1, &m_pboId );
        glBindBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB, m_pboId );
        glBufferDataARB( GL_PIXEL_UNPACK_BUFFER_ARB, m_pboDataSize, 0, GL_STREAM_DRAW_ARB );
        glBindBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB, 0 );

        wxLogTrace( m_logTrace,
                    wxT( "RENDER_3D_RAYTRACE:: GLEW_ARB_pixel_buffer_object is supported" ) );
    }
}


bool RENDER_3D_RAYTRACE::initializeOpenGL()
{
    m_is_opengl_initialized = true;

    return true;
}


static float distance( const SFVEC2UI& a, const SFVEC2UI& b )
{
    const float dx = (float) a.x - (float) b.x;
    const float dy = (float) a.y - (float) b.y;
    return hypotf( dx, dy );
}


void RENDER_3D_RAYTRACE::initializeBlockPositions()
{
    m_realBufferSize = SFVEC2UI( 0 );

    // Calc block positions for fast preview mode
    m_blockPositionsFast.clear();

    unsigned int i = 0;

    while(1)
    {
        const unsigned int mX = DecodeMorton2X(i);
        const unsigned int mY = DecodeMorton2Y(i);

        i++;

        const SFVEC2UI blockPos( mX * 4 * RAYPACKET_DIM - mX * 4,
                                 mY * 4 * RAYPACKET_DIM - mY * 4 );

        if( ( blockPos.x >= ( (unsigned int)m_windowSize.x - ( 4 * RAYPACKET_DIM + 4 ) ) ) &&
            ( blockPos.y >= ( (unsigned int)m_windowSize.y - ( 4 * RAYPACKET_DIM + 4 ) ) ) )
            break;

        if( ( blockPos.x < ( (unsigned int)m_windowSize.x - ( 4 * RAYPACKET_DIM + 4 ) ) ) &&
            ( blockPos.y < ( (unsigned int)m_windowSize.y - ( 4 * RAYPACKET_DIM + 4 ) ) ) )
        {
            m_blockPositionsFast.push_back( blockPos );

            if( blockPos.x > m_realBufferSize.x )
                m_realBufferSize.x = blockPos.x;

            if( blockPos.y > m_realBufferSize.y )
                m_realBufferSize.y = blockPos.y;
        }
    }

    m_fastPreviewModeSize = m_realBufferSize;

    m_realBufferSize.x = ( ( m_realBufferSize.x + RAYPACKET_DIM * 4 ) & RAYPACKET_INVMASK );
    m_realBufferSize.y = ( ( m_realBufferSize.y + RAYPACKET_DIM * 4 ) & RAYPACKET_INVMASK );

    m_xoffset = ( m_windowSize.x - m_realBufferSize.x ) / 2;
    m_yoffset = ( m_windowSize.y - m_realBufferSize.y ) / 2;

    m_postShaderSsao.UpdateSize( m_realBufferSize );

    // Calc block positions for regular rendering. Choose an 'inside out' style of rendering.
    m_blockPositions.clear();
    const int blocks_x = m_realBufferSize.x / RAYPACKET_DIM;
    const int blocks_y = m_realBufferSize.y / RAYPACKET_DIM;
    m_blockPositions.reserve( blocks_x * blocks_y );

    for( int x = 0; x < blocks_x; ++x )
    {
        for( int y = 0; y < blocks_y; ++y )
            m_blockPositions.emplace_back( x * RAYPACKET_DIM, y * RAYPACKET_DIM );
    }

    const SFVEC2UI center( m_realBufferSize.x / 2, m_realBufferSize.y / 2 );
    std::sort( m_blockPositions.begin(), m_blockPositions.end(),
            [&]( const SFVEC2UI& a, const SFVEC2UI& b ) {
                // Sort order: inside out.
                return distance( a, center ) < distance( b, center );
            } );

    // Create m_shader buffer
    delete[] m_shaderBuffer;
    m_shaderBuffer = new SFVEC3F[m_realBufferSize.x * m_realBufferSize.y];

    initPbo();
}


BOARD_ITEM* RENDER_3D_RAYTRACE::IntersectBoardItem( const RAY& aRay )
{
    HITINFO hitInfo;
    hitInfo.m_tHit = std::numeric_limits<float>::infinity();

    if( m_accelerator )
    {
        if( m_accelerator->Intersect( aRay, hitInfo ) )
        {
            if( hitInfo.pHitObject )
                return hitInfo.pHitObject->GetBoardItem();
        }
    }

    return nullptr;
}
