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
 * @file  c3d_render_raytracing.cpp
 * @brief
 */

#include <GL/glew.h>
#include <climits>

#include "c3d_render_raytracing.h"
#include "mortoncodes.h"
#include "../ccolorrgb.h"
#include "3d_fastmath.h"
#include "3d_math.h"
#include "../common_ogl/ogl_utils.h"

#ifdef _OPENMP
#include <omp.h>
#endif

C3D_RENDER_RAYTRACING::C3D_RENDER_RAYTRACING( CINFO3D_VISU &aSettings ) :
                       C3D_RENDER_BASE( aSettings ),
                       m_postshader_ssao( aSettings.CameraGet() )
{
    wxLogTrace( m_logTrace, wxT( "C3D_RENDER_RAYTRACING::C3D_RENDER_RAYTRACING" ) );

    m_opengl_support_vertex_buffer_object = false;
    m_pboId       = GL_NONE;
    m_pboDataSize = 0;
    m_accelerator = NULL;
    m_stats_converted_dummy_to_plane = 0;
    m_stats_converted_roundsegment2d_to_roundsegment = 0;
    m_oldWindowsSize.x = 0;
    m_oldWindowsSize.y = 0;
    m_outlineBoard2dObjects = NULL;
    m_firstHitinfo = NULL;
    m_shaderBuffer = NULL;
    m_camera_light = NULL;

    m_xoffset = 0;
    m_yoffset = 0;

    m_isPreview = false;
    m_rt_render_state = RT_RENDER_STATE_MAX; // Set to an initial invalid state
}


C3D_RENDER_RAYTRACING::~C3D_RENDER_RAYTRACING()
{
    wxLogTrace( m_logTrace, wxT( "C3D_RENDER_RAYTRACING::~C3D_RENDER_RAYTRACING" ) );

    delete m_accelerator;
    m_accelerator = NULL;

    delete m_outlineBoard2dObjects;
    m_outlineBoard2dObjects = NULL;

    delete m_shaderBuffer;
    m_shaderBuffer = NULL;

    opengl_delete_pbo();
}


int C3D_RENDER_RAYTRACING::GetWaitForEditingTimeOut()
{
    return 1000; // ms
}


void C3D_RENDER_RAYTRACING::opengl_delete_pbo()
{
    // Delete PBO if it was created
    if( m_opengl_support_vertex_buffer_object )
    {
        if( glIsBufferARB( m_pboId ) )
            glDeleteBuffers( 1, &m_pboId );

        m_pboId = GL_NONE;
    }
}


void C3D_RENDER_RAYTRACING::SetCurWindowSize( const wxSize &aSize )
{
    if( m_windowSize != aSize )
    {
        m_windowSize = aSize;
        glViewport( 0, 0, m_windowSize.x, m_windowSize.y );

        initializeNewWindowSize();
    }
}


void C3D_RENDER_RAYTRACING::restart_render_state()
{
    m_stats_start_rendering_time = GetRunningMicroSecs();

    m_rt_render_state = RT_RENDER_STATE_TRACING;
    m_nrBlocksRenderProgress = 0;

    m_postshader_ssao.InitFrame();

    m_blockPositionsWasProcessed.resize( m_blockPositions.size() );

    // Mark the blocks not processed yet
    std::fill( m_blockPositionsWasProcessed.begin(),
               m_blockPositionsWasProcessed.end(),
               false );
}


static inline void SetPixel( GLubyte *p, const CCOLORRGB &v )
{
    p[0] = v.c[0]; p[1] = v.c[1]; p[2] = v.c[2]; p[3] = 255;
}


bool C3D_RENDER_RAYTRACING::Redraw( bool aIsMoving, REPORTER *aStatusTextReporter )
{
    bool requestRedraw = false;

    // Initialize openGL if need
    // /////////////////////////////////////////////////////////////////////////
    if( !m_is_opengl_initialized )
    {
        if( !initializeOpenGL() )
            return false;

        //aIsMoving = true;
        requestRedraw = true;

        // It will assign the first time the windows size, so it will now
        // revert to preview mode the first time the Redraw is called
        m_oldWindowsSize = m_windowSize;
        initialize_block_positions();
    }


    // Reload board if it was requested
    // /////////////////////////////////////////////////////////////////////////
    if( m_reloadRequested )
    {
        if( aStatusTextReporter )
            aStatusTextReporter->Report( _( "Loading..." ) );

        //aIsMoving = true;
        requestRedraw = true;
        reload( aStatusTextReporter );
    }


    // Recalculate constants if windows size was changed
    // /////////////////////////////////////////////////////////////////////////
    if( m_windowSize != m_oldWindowsSize )
    {
        m_oldWindowsSize = m_windowSize;
        aIsMoving = true;
        requestRedraw = true;

        initialize_block_positions();
    }


    // Clear buffers
    // /////////////////////////////////////////////////////////////////////////
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


    const bool was_camera_changed = m_settings.CameraGet().ParametersChanged();

    if( requestRedraw || aIsMoving || was_camera_changed )
        m_rt_render_state = RT_RENDER_STATE_MAX; // Set to an invalid state,
                                                 // so it will restart again latter


    // This will only render if need, otherwise it will redraw the PBO on the screen again
    if( aIsMoving || was_camera_changed )
    {
        // Set head light (camera view light) with the oposite direction of the camera
        if( m_camera_light )
            m_camera_light->SetDirection( -m_settings.CameraGet().GetDir() );

        OGL_DrawBackground( SFVEC3F(m_settings.m_BgColorTop),
                            SFVEC3F(m_settings.m_BgColorBot) );

        // Bind PBO
        glBindBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB, m_pboId );

        // Get the PBO pixel pointer to write the data
        GLubyte *ptrPBO = (GLubyte *)glMapBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB,
                                                     GL_WRITE_ONLY_ARB );

        if( ptrPBO )
        {
            render_preview( ptrPBO );

            // release pointer to mapping buffer, this initialize the coping to PBO
            glUnmapBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB );
        }

        glWindowPos2i( m_xoffset, m_yoffset );
    }
    else
    {
        // Bind PBO
        glBindBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB, m_pboId );

        if( m_rt_render_state != RT_RENDER_STATE_FINISH )
        {
            // Get the PBO pixel pointer to write the data
            GLubyte *ptrPBO = (GLubyte *)glMapBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB,
                                                         GL_WRITE_ONLY_ARB );

            if( ptrPBO )
            {
                render( ptrPBO, aStatusTextReporter );

                if( m_rt_render_state != RT_RENDER_STATE_FINISH )
                    requestRedraw = true;

                // release pointer to mapping buffer, this initialize the coping to PBO
                glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
            }
        }

        if( m_rt_render_state == RT_RENDER_STATE_FINISH )
        {
            glClear( GL_COLOR_BUFFER_BIT );
            // Options if we want draw background instead
            //OGL_DrawBackground( SFVEC3F(m_settings.m_BgColorTop),
            //                    SFVEC3F(m_settings.m_BgColorBot) );
        }

        glWindowPos2i( m_xoffset, m_yoffset );
    }

    // This way it will blend the progress rendering with the last buffer. eg:
    // if it was called after a openGL.
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable( GL_ALPHA_TEST );

    glDrawPixels( m_realBufferSize.x,
                  m_realBufferSize.y,
                  GL_RGBA,
                  GL_UNSIGNED_BYTE,
                  0 );

    glBindBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB, 0 );

    return requestRedraw;
}


void C3D_RENDER_RAYTRACING::render( GLubyte *ptrPBO , REPORTER *aStatusTextReporter )
{
    if( (m_rt_render_state == RT_RENDER_STATE_FINISH) ||
        (m_rt_render_state >= RT_RENDER_STATE_MAX) )
    {
        restart_render_state();

        if( m_camera_light )
            m_camera_light->SetDirection( -m_settings.CameraGet().GetDir() );

        if( m_settings.RenderEngineGet() == RENDER_ENGINE_OPENGL_LEGACY )
        {
            // Set all pixels of PBO transparent (Alpha to 0)
            // This way it will draw the full buffer but only shows the updated (
            // already calculated) squares
            // /////////////////////////////////////////////////////////////////////
            unsigned int nPixels = m_realBufferSize.x * m_realBufferSize.y;
            GLubyte *tmp_ptrPBO = ptrPBO + 3;   // PBO is RGBA

            for( unsigned int i = 0; i < nPixels; ++i )
            {
                *tmp_ptrPBO = 0;
                tmp_ptrPBO += 4;                // PBO is RGBA
            }
        }
    }

    switch( m_rt_render_state )
    {
    case RT_RENDER_STATE_TRACING:
            rt_render_tracing( ptrPBO, aStatusTextReporter );
        break;

    case RT_RENDER_STATE_POST_PROCESS_SHADE:
            rt_render_post_process_shade( ptrPBO, aStatusTextReporter );
        break;

    case RT_RENDER_STATE_POST_PROCESS_BLUR_AND_FINISH:
            rt_render_post_process_blur_finish( ptrPBO, aStatusTextReporter );
        break;

    default:
        wxASSERT_MSG( false, "Invalid state on m_rt_render_state");
        restart_render_state();
        break;
    }

    if( aStatusTextReporter && (m_rt_render_state == RT_RENDER_STATE_FINISH) )
    {
        // Calculation time in seconds
        const double calculation_time = (double)( GetRunningMicroSecs() -
                                                  m_stats_start_rendering_time ) / 1e6;

        aStatusTextReporter->Report( wxString::Format( _( "Rendering time %.3f s" ),
                                                       calculation_time ) );
    }
}


void C3D_RENDER_RAYTRACING::rt_render_tracing( GLubyte *ptrPBO ,
                                               REPORTER *aStatusTextReporter )
{
    m_isPreview = false;
    wxASSERT( m_blockPositions.size() <= LONG_MAX );

    const long nrBlocks = (long) m_blockPositions.size();
    const unsigned startTime = GetRunningMicroSecs();
    bool breakLoop = false;
    int numBlocksRendered = 0;

    #pragma omp parallel for schedule(dynamic) shared(breakLoop) \
        firstprivate(ptrPBO, nrBlocks, startTime) reduction(+:numBlocksRendered) default(none)
    for( long iBlock = 0; iBlock < nrBlocks; iBlock++ )
    {

        #pragma omp flush(breakLoop)
        if( !breakLoop )
        {
            bool process_block;

            // std::vector<bool> stuffs eight bools to each byte, so access to
            // them can never be natively atomic.
            #pragma omp critical(checkProcessBlock)
            {
                process_block = !m_blockPositionsWasProcessed[iBlock];
                m_blockPositionsWasProcessed[iBlock] = true;
            }

            if( process_block )
            {
                rt_render_trace_block( ptrPBO, iBlock );
                numBlocksRendered++;


                // Check if it spend already some time render and request to exit
                // to display the progress
                #ifdef _OPENMP
                if( omp_get_thread_num() == 0 )
                #endif
                    if( (GetRunningMicroSecs() - startTime) > 150000 )
                    {
                        breakLoop = true;
                        #pragma omp flush(breakLoop)
                    }
            }
        }
    }

    m_nrBlocksRenderProgress += numBlocksRendered;

    if( aStatusTextReporter )
        aStatusTextReporter->Report( wxString::Format( _( "Rendering: %.0f %%" ),
                                                       (float)(m_nrBlocksRenderProgress * 100) /
                                                       (float)nrBlocks ) );

    // Check if it finish the rendering and if should continue to a post processing
    // or mark it as finished
    if( m_nrBlocksRenderProgress >= nrBlocks )
    {
        if( m_settings.GetFlag( FL_RENDER_RAYTRACING_POST_PROCESSING ) )
            m_rt_render_state = RT_RENDER_STATE_POST_PROCESS_SHADE;
        else
        {
            m_rt_render_state = RT_RENDER_STATE_FINISH;
        }
    }
}


void C3D_RENDER_RAYTRACING::rt_render_trace_block( GLubyte *ptrPBO ,
                                                   signed int iBlock )
{
    // Initialize ray packets
    // /////////////////////////////////////////////////////////////////////////
    const SFVEC2UI &blockPos = m_blockPositions[iBlock];
    const SFVEC2I blockPosI = SFVEC2I( blockPos.x + m_xoffset,
                                       blockPos.y + m_yoffset );

    RAYPACKET blockPacket( m_settings.CameraGet(), blockPosI );

    HITINFO_PACKET hitPacket[RAYPACKET_RAYS_PER_PACKET];

    // Initialize hitPacket with a "not hit" information
    for( unsigned int i = 0; i < RAYPACKET_RAYS_PER_PACKET; ++i )
    {
        hitPacket[i].m_HitInfo.m_tHit = std::numeric_limits<float>::infinity();
        hitPacket[i].m_HitInfo.m_acc_node_info = 0;
        hitPacket[i].m_hitresult = false;
        hitPacket[i].m_HitInfo.m_HitNormal = SFVEC3F();
        hitPacket[i].m_HitInfo.m_ShadowFactor = 1.0f;
    }

    // Calculate background gradient color
    // /////////////////////////////////////////////////////////////////////////
    SFVEC3F bgColor[RAYPACKET_DIM];// Store a vertical gradient color

    for( unsigned int y = 0; y < RAYPACKET_DIM; ++y )
    {
        const float posYfactor = (float)(blockPosI.y + y) / (float)m_windowSize.y;

        bgColor[y] = (SFVEC3F)m_settings.m_BgColorTop * SFVEC3F(posYfactor) +
                     (SFVEC3F)m_settings.m_BgColorBot * ( SFVEC3F(1.0f) - SFVEC3F(posYfactor) );
    }

    // Intersect ray packets (calculate the intersection with rays and objects)
    // /////////////////////////////////////////////////////////////////////////
    if( !m_accelerator->Intersect( blockPacket, hitPacket ) )
    {

        // If block is empty then set shades and continue
        if( m_settings.GetFlag( FL_RENDER_RAYTRACING_POST_PROCESSING ) )
        {
            for( unsigned int y = 0; y < RAYPACKET_DIM; ++y )
            {
                const SFVEC3F &outColor = bgColor[y];

                const unsigned int yBlockPos = blockPos.y + y;

                for( unsigned int x = 0; x < RAYPACKET_DIM; ++x )
                {
                        m_postshader_ssao.SetPixelData( blockPos.x + x,
                                                        yBlockPos,
                                                        SFVEC3F(),
                                                        outColor,
                                                        SFVEC3F(),
                                                        0,
                                                        1.0f );
                }
            }
        }

        // This will set the output color to be displayed
        // If post processing is enabled, it will not reflect the final result
        // (as the final color will be computed on post processing)
        // but it is used for report progress
        for( unsigned int y = 0; y < RAYPACKET_DIM; ++y )
        {
            const SFVEC3F &outColor = bgColor[y];

            const unsigned int yConst = blockPos.x +
                                        ( (y + blockPos.y) * m_realBufferSize.x);

            for( unsigned int x = 0; x < RAYPACKET_DIM; ++x )
            {
                GLubyte *ptr = &ptrPBO[ (yConst + x) * 4 ];

                ptr[0] = (unsigned int)glm::clamp( (int)(outColor.r * 255), 0, 255 );
                ptr[1] = (unsigned int)glm::clamp( (int)(outColor.g * 255), 0, 255 );
                ptr[2] = (unsigned int)glm::clamp( (int)(outColor.b * 255), 0, 255 );
                ptr[3] = 255;
            }
        }

        // There is nothing more here to do.. there are no hits ..
        // just background so continue
        return;
    }


    // Shade hits ("paint" the intersected objects)
    // /////////////////////////////////////////////////////////////////////////

    SFVEC3F hitColor[RAYPACKET_RAYS_PER_PACKET];

    for( unsigned int y = 0, i = 0; y < RAYPACKET_DIM; ++y )
    {
        for( unsigned int x = 0; x < RAYPACKET_DIM; ++x, ++i )
        {
            if( hitPacket[i].m_hitresult == true )
            {
                hitColor[i] = shadeHit( bgColor[y],
                                        blockPacket.m_ray[i],
                                        hitPacket[i].m_HitInfo,
                                        false,
                                        0 );
            }
            else
            {
                hitColor[i] = bgColor[y];
            }
        }
    }


    // This code was a tentative to retrace the block but using small changes
    // on ray direction (to work as a random anti-aliasing)
    // but it was not producing good results with low passes,
    // parked for future use / to be implemented
    // /////////////////////////////////////////////////////////////////////////

    /*
    //if( m_settings.GetFlag( FL_RENDER_RAYTRACING_ANTI_ALIASING ) )
    if(0)
    {
        SFVEC3F absDirDiff;
        absDirDiff.x = glm::abs( blockPacket.m_ray[                1].m_Dir.x - blockPacket.m_ray[0].m_Dir.x ) * 0.45f;
        absDirDiff.y = glm::abs( blockPacket.m_ray[RAYPACKET_DIM + 0].m_Dir.y - blockPacket.m_ray[0].m_Dir.y ) * 0.45f;
        absDirDiff.z = glm::abs( blockPacket.m_ray[RAYPACKET_DIM + 1].m_Dir.z - blockPacket.m_ray[0].m_Dir.z ) * 0.45f;

        const unsigned int number_of_passes = 16;

        for( unsigned int aaPasses = 0; aaPasses < number_of_passes; ++aaPasses )
        {

            HITINFO_PACKET hitPacketAA[RAYPACKET_RAYS_PER_PACKET];

            // Initialize hitPacket with a "not hit" information
            for( unsigned int i = 0; i < RAYPACKET_RAYS_PER_PACKET; ++i )
            {
                hitPacketAA[i].m_HitInfo.m_tHit = std::numeric_limits<float>::infinity();
                hitPacketAA[i].m_HitInfo.m_acc_node_info = 0;
                hitPacketAA[i].m_hitresult = false;
                hitPacketAA[i].m_HitInfo.m_HitNormal = SFVEC3F();
                hitPacketAA[i].m_HitInfo.m_ShadowFactor = 1.0f;
            }

            RAYPACKET blockPacketAA( m_settings.CameraGet(), blockPosI, absDirDiff );

            if( m_accelerator->Intersect( blockPacketAA, hitPacketAA ) )
            {
                for( unsigned int y = 0, i = 0; y < RAYPACKET_DIM; ++y )
                {
                    for( unsigned int x = 0; x < RAYPACKET_DIM; ++x, ++i )
                    {
                        if( hitPacketAA[i].m_hitresult == true )
                        {
                            hitColor[i] += shadeHit( bgColor[y],
                                                     blockPacketAA.m_ray[i],
                                                     hitPacketAA[i].m_HitInfo,
                                                     false,
                                                     0 );
                        }
                        else
                        {
                            hitColor[i] += bgColor[y];
                        }
                    }
                }
            }
        }

        const float aaPasses_inv = 1.0f / (float)(number_of_passes + 1);

        for( unsigned int i = 0; i < RAYPACKET_RAYS_PER_PACKET; ++i )
            hitColor[i] *= aaPasses_inv;
    }
    */

    // If anti-aliasing is enabled, trace a new random rays over the hits
    // already calculated.
    // On this pass it will calculate pixels near the hitted ray,
    // so it will reuse the nodes found on that hits
    // /////////////////////////////////////////////////////////////////////

    if( m_settings.GetFlag( FL_RENDER_RAYTRACING_ANTI_ALIASING ) )
    {

        SFVEC3F hitColorAA[RAYPACKET_RAYS_PER_PACKET];

        for( unsigned int i = 0; i < RAYPACKET_RAYS_PER_PACKET; ++i )
            hitColorAA[i] = SFVEC3F(0.0f);

        // This just get some difference between two pixels
        // There is not logic on this approach, it trys to guess the xyz increments
        SFVEC3F absDirDiff;
        absDirDiff.x = glm::abs( blockPacket.m_ray[RAYPACKET_DIM + 0].m_Dir.x -
                                 blockPacket.m_ray[0].m_Dir.x ) * 0.55f;

        absDirDiff.y = glm::abs( blockPacket.m_ray[RAYPACKET_DIM + 0].m_Dir.y -
                                 blockPacket.m_ray[0].m_Dir.y ) * 0.55f;

        absDirDiff.z = glm::abs( blockPacket.m_ray[RAYPACKET_DIM + 0].m_Dir.z -
                                 blockPacket.m_ray[0].m_Dir.z ) * 0.55f;

        const unsigned int number_of_passes = 3;

        for( unsigned int aaPasses = 0; aaPasses < number_of_passes; ++aaPasses )
        {
            for( unsigned int y = 0, i = 0; y < RAYPACKET_DIM; ++y )
            {
                for( unsigned int x = 0; x < RAYPACKET_DIM; ++x, ++i )
                {
                    if( hitPacket[i].m_hitresult == true )
                    {
                        const SFVEC3F randVector = SFVEC3F( Fast_RandFloat() * absDirDiff.x,
                                                            Fast_RandFloat() * absDirDiff.y,
                                                            Fast_RandFloat() * absDirDiff.z );

                        RAY rayAA;
                        rayAA.Init( blockPacket.m_ray[i].m_Origin,
                                    glm::normalize( blockPacket.m_ray[i].m_Dir +
                                                    randVector ) );

                        HITINFO hitAA;
                        hitAA.m_tHit = std::numeric_limits<float>::infinity();
                        hitAA.m_acc_node_info = 0;
                        hitAA.m_ShadowFactor = 1.0f;

                        bool hitted = false;

                        const unsigned int idx0y1 = ( x + 0 ) + RAYPACKET_DIM * ( y + 1 );
                        const unsigned int idx1y1 = ( x + 1 ) + RAYPACKET_DIM * ( y + 1 );

                        // Gets the node info from the hit. If there was no hit, return 0
                        const unsigned int nodex0y0 = (hitPacket[ i ].m_hitresult == false)?0:
                                                       hitPacket[ i ].m_HitInfo.m_acc_node_info;

                        unsigned int nodex1y0 = 0;

                        if( x < (RAYPACKET_DIM - 1) )
                            nodex1y0 = (hitPacket[ i + 1 ].m_hitresult == false)?0:
                                        hitPacket[ i + 1 ].m_HitInfo.m_acc_node_info;

                        unsigned int nodex0y1 = 0;

                        if( y < (RAYPACKET_DIM - 1) )
                                nodex0y1 = (hitPacket[ idx0y1 ].m_hitresult == false)?0:
                                            hitPacket[ idx0y1 ].m_HitInfo.m_acc_node_info;

                        unsigned int nodex1y1 = 0;

                        if(  ((x < (RAYPACKET_DIM - 1)) &&
                              (y < (RAYPACKET_DIM - 1))) )
                            nodex1y1 = (hitPacket[ idx1y1 ].m_hitresult == false)?0:
                                        hitPacket[ idx1y1 ].m_HitInfo.m_acc_node_info;


                        if( (nodex0y0 == nodex1y0) &&   //
                            (nodex0y0 == nodex0y1) &&
                            (nodex0y0 == nodex1y1) )
                        {
                            // Option 1
                            // This option will give a very good quality on reflections (slow)
                            /*
                            if( m_accelerator->Intersect( rayAA, hitAA, nodex0y0 ) )
                            {
                                hitColorAA[i] += shadeHit( bgColor[y], rayAA, hitAA, false, 0 );
                            }
                            else
                            {
                                if( m_accelerator->Intersect( rayAA, hitAA ) )
                                    hitColorAA[i] += shadeHit( bgColor[y], rayAA, hitAA, false, 0 );
                                else
                                    hitColorAA[i] += hitColor[i];
                            }
                            */

                            // Option 2
                            // Trace again with the same node,
                            // then if miss just give the same color as before
                            //if( m_accelerator->Intersect( rayAA, hitAA, nodex0y0 ) )
                            //    hitColorAA[i] += shadeHit( bgColor[y], rayAA, hitAA, false, 0 );
                            //else
                                // This option will give the same color as the hit before (faster)
                                hitColorAA[i] += hitColor[i];
                        }
                        else
                        {
                            // Try to intersect the different nodes
                            // It tests the possible combination of hitted or not hitted points
                            // This will try to get the best hit for this ray

                            if( nodex0y0 != 0 )
                                hitted |= m_accelerator->Intersect( rayAA, hitAA, nodex0y0 );

                            if( nodex1y0 != 0 )
                                if( ( nodex0y0 != nodex1y0 ) || ( nodex0y0 == 0 ) )
                                    hitted |= m_accelerator->Intersect( rayAA, hitAA, nodex1y0 );

                            if( nodex0y1 != 0 )
                                if( ( ( nodex0y0 != nodex0y1 ) || ( nodex0y0 == 0 ) ) &&
                                    ( ( nodex1y0 != nodex0y1 ) || ( nodex1y0 == 0 ) ) )
                                    hitted |= m_accelerator->Intersect( rayAA, hitAA, nodex0y1 );

                            if( nodex1y1 != 0 )
                                if( ( ( nodex0y0 != nodex1y1 ) || ( nodex0y0 == 0 ) ) &&
                                    ( ( nodex0y1 != nodex1y1 ) || ( nodex0y1 == 0 ) ) &&
                                    ( ( nodex1y0 != nodex1y1 ) || ( nodex1y0 == 0 ) ) )
                                    hitted |= m_accelerator->Intersect( rayAA, hitAA, nodex1y1 );

                            if( hitted )
                            {
                                // If he got any result, shade it
                                hitColorAA[i] += shadeHit( bgColor[y], rayAA, hitAA, false, 0 );
                            }
                            else
                            {
                                // It was missed the 'last nodes' so, trace a ray from the beginning
                                if( m_accelerator->Intersect( rayAA, hitAA ) )
                                    hitColorAA[i] += shadeHit( bgColor[y], rayAA, hitAA, false, 0 );
                                else
                                    hitColorAA[i] += hitColor[i];
                            }
                        }
                    }
                    else
                    {
                        hitColorAA[i] += hitColor[i];
                    }
                }
            }
        }

        const float aaPasses_inv = 1.0f / (float)number_of_passes;

        for( unsigned int i = 0; i < RAYPACKET_RAYS_PER_PACKET; ++i )
            hitColor[i] = hitColor[i] * 0.40f + (hitColorAA[i] * aaPasses_inv) * 0.60f;
    }


    // Trace adaptative anti-aliasing middle pixels
    // /////////////////////////////////////////////////////////////////////

    HITINFO_PACKET hitPacketAA[ (RAYPACKET_DIM-1) * (RAYPACKET_DIM-1) ];
    RAY            raysAA[ (RAYPACKET_DIM-1) * (RAYPACKET_DIM-1) ];
    SFVEC3F        hitColorAA[ (RAYPACKET_DIM-1) * (RAYPACKET_DIM-1) ];
    bool           hittedAA[ (RAYPACKET_DIM-1) * (RAYPACKET_DIM-1) ];

    if( m_settings.GetFlag( FL_RENDER_RAYTRACING_ANTI_ALIASING ) )
    {
        for( unsigned int y = 0, i = 0; y < (RAYPACKET_DIM - 1); ++y )
        {
            for( unsigned int x = 0; x < (RAYPACKET_DIM - 1); ++x, ++i )
            {
                hitColorAA[i] = bgColor[y];
                hittedAA[i] = false;

                const unsigned int idx0y0 = (x + 0) + RAYPACKET_DIM * (y + 0);
                const unsigned int idx1y0 = (x + 1) + RAYPACKET_DIM * (y + 0);
                const unsigned int idx0y1 = (x + 0) + RAYPACKET_DIM * (y + 1);
                const unsigned int idx1y1 = (x + 1) + RAYPACKET_DIM * (y + 1);

                // Evaluate if we can skip the pixel from trace if the adjacent
                // pixels are similar (hitted, similar color and normal)

                if( hitPacket[ idx0y0 ].m_hitresult &&
                    hitPacket[ idx1y0 ].m_hitresult &&
                    hitPacket[ idx0y1 ].m_hitresult &&
                    hitPacket[ idx1y1 ].m_hitresult )
                {
                    // Calc the average gray scale

                    // Average
                    /*
                    const float gray_idx0y0 =
                            (hitColor[idx0y0].r + hitColor[idx0y0].g + hitColor[idx0y0].b ) / 3.0f;
                    const float gray_idx1y0 =
                            (hitColor[idx1y0].r + hitColor[idx1y0].g + hitColor[idx1y0].b ) / 3.0f;
                    const float gray_idx0y1 =
                            (hitColor[idx0y1].r + hitColor[idx0y1].g + hitColor[idx0y1].b ) / 3.0f;
                    const float gray_idx1y1 =
                            (hitColor[idx1y1].r + hitColor[idx1y1].g + hitColor[idx1y1].b ) / 3.0f;
                    */


                    // Luminance
                    const float gray_idx0y0 = (hitColor[idx0y0].r * 0.2126f +
                                               hitColor[idx0y0].g * 0.7152f +
                                               hitColor[idx0y0].b * 0.0722f);

                    const float gray_idx1y0 = (hitColor[idx1y0].r * 0.2126f +
                                               hitColor[idx1y0].g * 0.7152f +
                                               hitColor[idx1y0].b * 0.0722f);

                    const float gray_idx0y1 = (hitColor[idx0y1].r * 0.2126f +
                                               hitColor[idx0y1].g * 0.7152f +
                                               hitColor[idx0y1].b * 0.0722f);

                    const float gray_idx1y1 = (hitColor[idx1y1].r * 0.2126f +
                                               hitColor[idx1y1].g * 0.7152f +
                                               hitColor[idx1y1].b * 0.0722f);

                    const float threshould_color = 0.070f;

                    // Check if there are no big difference, if not,
                    // it will continue and not process anti-aliasing
                    if( ( glm::abs( gray_idx0y0 - gray_idx1y0) < threshould_color ) &&
                        ( glm::abs( gray_idx0y0 - gray_idx0y1) < threshould_color ) &&
                        ( glm::abs( gray_idx0y1 - gray_idx1y1) < threshould_color ) &&
                        ( glm::abs( gray_idx1y1 - gray_idx1y0) < threshould_color ) )
                    {
                            continue;
                    }
                }


                // Use this code if you want to see (debug) the pixels that are interpolated
                // hitColorAA[i] = SFVEC3F(1.0f, 0.0f, 1.0f);
                // hittedAA[i] = true;
                // continue;



                // Initialize a ray that is in the middle of the 4 pixeis and
                // have an average direction of the 4 pixels
                raysAA[i].Init( ( blockPacket.m_ray[idx0y0].m_Origin +
                                  blockPacket.m_ray[idx1y0].m_Origin +
                                  blockPacket.m_ray[idx0y1].m_Origin +
                                  blockPacket.m_ray[idx1y1].m_Origin ) / 4.0f,
                                glm::normalize( ( blockPacket.m_ray[idx0y0].m_Dir +
                                                  blockPacket.m_ray[idx1y0].m_Dir +
                                                  blockPacket.m_ray[idx0y1].m_Dir +
                                                  blockPacket.m_ray[idx1y1].m_Dir ) ) );

                hitPacketAA[i].m_HitInfo.m_tHit = std::numeric_limits<float>::infinity();
                hitPacketAA[i].m_HitInfo.m_acc_node_info = 0;
                hitPacketAA[i].m_HitInfo.m_ShadowFactor = 1.0f;
                hitPacketAA[i].m_hitresult = false;

                bool hitted = false;

                // Gets the node info from the hit. If there was no hit, return 0
                const unsigned int nodex0y0 = (hitPacket[ idx0y0 ].m_hitresult == false)?0:
                                               hitPacket[ idx0y0 ].m_HitInfo.m_acc_node_info;

                const unsigned int nodex1y0 = (hitPacket[ idx1y0 ].m_hitresult == false)?0:
                                               hitPacket[ idx1y0 ].m_HitInfo.m_acc_node_info;

                const unsigned int nodex0y1 = (hitPacket[ idx0y1 ].m_hitresult == false)?0:
                                               hitPacket[ idx0y1 ].m_HitInfo.m_acc_node_info;

                const unsigned int nodex1y1 = (hitPacket[ idx1y1 ].m_hitresult == false)?0:
                                               hitPacket[ idx1y1 ].m_HitInfo.m_acc_node_info;

                // Try to intersect the different nodes
                // It tests the possible combination of hitted or not hitted points
                // This will try to get the best hit for this ray

                if( nodex0y0 != 0 )
                    hitted |= m_accelerator->Intersect( raysAA[i],
                                                        hitPacketAA[i].m_HitInfo,
                                                        nodex0y0 );

                if( nodex1y0 != 0 )
                    if( ( nodex0y0 != nodex1y0 ) || ( nodex0y0 == 0 ) )
                        hitted |= m_accelerator->Intersect( raysAA[i],
                                                            hitPacketAA[i].m_HitInfo,
                                                            nodex1y0 );

                if( nodex0y1 != 0 )
                    if( ( ( nodex0y0 != nodex0y1 ) || ( nodex0y0 == 0 ) ) &&
                        ( ( nodex1y0 != nodex0y1 ) || ( nodex1y0 == 0 ) ) )
                        hitted |= m_accelerator->Intersect( raysAA[i],
                                                            hitPacketAA[i].m_HitInfo,
                                                            nodex0y1 );

                if( nodex1y1 != 0 )
                    if( ( ( nodex0y0 != nodex1y1 ) || ( nodex0y0 == 0 ) ) &&
                        ( ( nodex0y1 != nodex1y1 ) || ( nodex0y1 == 0 ) ) &&
                        ( ( nodex1y0 != nodex1y1 ) || ( nodex1y0 == 0 ) ) )
                        hitted |= m_accelerator->Intersect( raysAA[i],
                                                            hitPacketAA[i].m_HitInfo,
                                                            nodex1y1 );

                if( hitted )
                {
                    // If he got any result, shade it
                    hitColorAA[i] = shadeHit( bgColor[y],
                                              raysAA[i],
                                              hitPacketAA[i].m_HitInfo,
                                              false,
                                              0 );
                }
                else
                {
                    // It was missed the 'last nodes' so,
                    // trace a ray from the beginning
                    if( m_accelerator->Intersect( raysAA[i], hitPacketAA[i].m_HitInfo ) )
                        hitColorAA[i] = shadeHit( bgColor[y],
                                                  raysAA[i],
                                                  hitPacketAA[i].m_HitInfo,
                                                  false,
                                                  0 );
                }

                hittedAA[i] = true;
            }
        }
    }


    // Blend original hitted pixels with anti-alised pixels
    // /////////////////////////////////////////////////////////////////////
    for( unsigned int y = 0, i = 0; y < RAYPACKET_DIM; ++y )
    {
        for( unsigned int x = 0; x < RAYPACKET_DIM; ++x, ++i )
        {
            SFVEC3F hColor = hitColor[i];

            if( m_settings.GetFlag( FL_RENDER_RAYTRACING_ANTI_ALIASING ) )
            {
                SFVEC3F aaColor = bgColor[y];

                if( (x > 0) &&
                    (y > 0) &&
                    ( x < (RAYPACKET_DIM - 1) ) &&
                    ( y < (RAYPACKET_DIM - 1) ) )
                {
                    // It makes a blur of the hitColor
                    const SFVEC3F averageHitColor =
                             hitColor[ (x - 1) + RAYPACKET_DIM * (y - 1) ] * 0.0625f +
                             hitColor[ (x + 0) + RAYPACKET_DIM * (y - 1) ] * 0.1250f +
                             hitColor[ (x + 1) + RAYPACKET_DIM * (y - 1) ] * 0.0625f +
                             hitColor[ (x - 1) + RAYPACKET_DIM * (y + 0) ] * 0.1250f +
                             hitColor[ (x + 0) + RAYPACKET_DIM * (y + 0) ] * 0.2500f +
                             hitColor[ (x + 1) + RAYPACKET_DIM * (y + 0) ] * 0.1250f +
                             hitColor[ (x - 1) + RAYPACKET_DIM * (y + 1) ] * 0.0625f +
                             hitColor[ (x + 0) + RAYPACKET_DIM * (y + 1) ] * 0.1250f +
                             hitColor[ (x + 1) + RAYPACKET_DIM * (y + 1) ] * 0.0625f;


                    const unsigned x0y0 = (x - 1) + (RAYPACKET_DIM - 1) * (y - 1);
                    const unsigned x1y0 = (x - 0) + (RAYPACKET_DIM - 1) * (y - 1);
                    const unsigned x0y1 = (x - 1) + (RAYPACKET_DIM - 1) * (y - 0);
                    const unsigned x1y1 = (x - 0) + (RAYPACKET_DIM - 1) * (y - 0);

                    aaColor = (hittedAA[ x0y0 ]? hitColorAA[ x0y0 ]: averageHitColor) +
                              (hittedAA[ x1y0 ]? hitColorAA[ x1y0 ]: averageHitColor) +
                              (hittedAA[ x0y1 ]? hitColorAA[ x0y1 ]: averageHitColor) +
                              (hittedAA[ x1y1 ]? hitColorAA[ x1y1 ]: averageHitColor);

                    aaColor /= 4.0f;
                }
                else
                {
                    if( (x == 0) && (y == 0) )
                    {
                        const unsigned x0y0 = (x - 0) + (RAYPACKET_DIM - 1) * (y - 0);
                        aaColor = (hittedAA[ x0y0 ]? hitColorAA[ x0y0 ]: hColor);
                    }
                    else
                    {
                        if( (x == (RAYPACKET_DIM - 1)) && (y == (RAYPACKET_DIM - 1)) )
                        {
                            const unsigned x0y0 = (x - 1) + (RAYPACKET_DIM - 1) * (y - 1);
                            aaColor = (hittedAA[ x0y0 ]? hitColorAA[ x0y0 ]: hColor);
                        }
                        else
                        {
                            if( (x == (RAYPACKET_DIM - 1)) && (y == 0) )
                            {
                                const unsigned x0y0 = (x - 1) + (RAYPACKET_DIM - 1) * (y - 0);
                                aaColor = (hittedAA[ x0y0 ]? hitColorAA[ x0y0 ]: hColor);
                            }
                            else
                            {
                                if( (x == 0) && (y == (RAYPACKET_DIM - 1)) )
                                {
                                    const unsigned x0y0 = (x - 0) + (RAYPACKET_DIM - 1) * (y - 1);
                                    aaColor = (hittedAA[ x0y0 ]? hitColorAA[ x0y0 ]: hColor);
                                }
                                else
                                {
                                    if( ( y == 0 ) && ( x < (RAYPACKET_DIM - 1) ) )
                                    {
                                        const unsigned x0y0 = (x - 1) + (RAYPACKET_DIM - 1) *
                                                              (y - 0);
                                        const unsigned x1y0 = (x - 0) + (RAYPACKET_DIM - 1) *
                                                              (y - 0);

                                        aaColor = (hittedAA[ x0y0 ]? hitColorAA[ x0y0 ]: hColor) +
                                                  (hittedAA[ x1y0 ]? hitColorAA[ x1y0 ]: hColor);

                                        aaColor = aaColor / 2.0f;
                                    }
                                    else
                                    {
                                        if( ( y == (RAYPACKET_DIM - 1) ) &&
                                            ( x < (RAYPACKET_DIM - 1) ) )
                                        {
                                            const unsigned x0y0 = (x - 1) + (RAYPACKET_DIM - 1) *
                                                                  (y - 1);
                                            const unsigned x1y0 = (x - 0) + (RAYPACKET_DIM - 1) *
                                                                  (y - 1);

                                            aaColor = (hittedAA[ x0y0 ]?hitColorAA[ x0y0 ]:hColor) +
                                                      (hittedAA[ x1y0 ]?hitColorAA[ x1y0 ]:hColor);

                                            aaColor = aaColor / 2.0f;
                                        }
                                        else
                                        {
                                            if( ( x == 0 ) && ( y < (RAYPACKET_DIM - 1) ) )
                                            {
                                                const unsigned x0y0 = (x - 0) +
                                                                      (RAYPACKET_DIM - 1) * (y - 1);
                                                const unsigned x0y1 = (x - 0) +
                                                                      (RAYPACKET_DIM - 1) * (y - 0);

                                                aaColor = (hittedAA[x0y0]?hitColorAA[x0y0]:hColor) +
                                                          (hittedAA[x0y1]?hitColorAA[x0y1]:hColor);

                                                aaColor = aaColor / 2.0f;
                                            }
                                            else
                                            {
                                                if( ( x == (RAYPACKET_DIM - 1) ) &&
                                                    ( y < (RAYPACKET_DIM - 1) ) )
                                                {
                                                    const unsigned x0y0 = (x - 1) +
                                                                          (RAYPACKET_DIM - 1) *
                                                                          (y - 1);

                                                    const unsigned x0y1 = (x - 1) +
                                                                          (RAYPACKET_DIM - 1) *
                                                                          (y - 0);

                                                    aaColor =
                                                        (hittedAA[x0y0]?hitColorAA[x0y0]:hColor) +
                                                        (hittedAA[x0y1]?hitColorAA[x0y1]:hColor);

                                                    aaColor = aaColor / 2.0f;
                                                }
                                                else
                                                {
                                                    aaColor = SFVEC3F(1.0f, 0.0f, 1.0f ); // Invalid
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // Calculate the final blend color. It gives more importance
                // to the original color
                hColor = hColor * 0.60f + aaColor * 0.40f;
            }

            if( m_settings.GetFlag( FL_RENDER_RAYTRACING_POST_PROCESSING ) )
            {
                if( hitPacket[i].m_hitresult == true )
                    m_postshader_ssao.SetPixelData( blockPos.x + x, blockPos.y + y,
                                                    hitPacket[i].m_HitInfo.m_HitNormal,
                                                    hColor,
                                                    blockPacket.m_ray[i].at(
                                                        hitPacket[i].m_HitInfo.m_tHit ),
                                                    hitPacket[i].m_HitInfo.m_tHit,
                                                    hitPacket[i].m_HitInfo.m_ShadowFactor );
                else
                    m_postshader_ssao.SetPixelData( blockPos.x + x, blockPos.y + y,
                                                    SFVEC3F(),
                                                    hColor,
                                                    SFVEC3F(),
                                                    0,
                                                    1.0f );

            }

            // This will set the output color to be displayed
            // If post processing is enabled, it will not reflect the final result
            // (as the final color will be computed on post processing)
            // but it is used for report progress
            GLubyte *ptr = &ptrPBO[ ( blockPos.x + x +
                                      ((y + blockPos.y) * m_realBufferSize.x) ) * 4 ];

            ptr[0] = (unsigned int)glm::clamp( (int)(hColor.r * 255), 0, 255 );
            ptr[1] = (unsigned int)glm::clamp( (int)(hColor.g * 255), 0, 255 );
            ptr[2] = (unsigned int)glm::clamp( (int)(hColor.b * 255), 0, 255 );
            ptr[3] = 255;
        }
    }
}


void C3D_RENDER_RAYTRACING::rt_render_post_process_shade( GLubyte *ptrPBO,
                                                          REPORTER *aStatusTextReporter )
{
    (void)ptrPBO; // unused

    if( m_settings.GetFlag( FL_RENDER_RAYTRACING_POST_PROCESSING ) )
    {
        if( aStatusTextReporter )
            aStatusTextReporter->Report( _("Rendering: Post processing shader") );

        // Compute the shader value
        #pragma omp parallel for schedule(dynamic)
        for( signed int y = 0; y < (int)m_realBufferSize.y; ++y )
        {
            SFVEC3F *ptr = &m_shaderBuffer[ y * m_realBufferSize.x ];

            for( signed int x = 0; x < (int)m_realBufferSize.x; ++x )
            {
                *ptr = m_postshader_ssao.Shade( SFVEC2I( x, y ) );
                ptr++;
            }
        }

        // Wait for all threads to finish
        #pragma omp barrier

        // Set next state
        m_rt_render_state = RT_RENDER_STATE_POST_PROCESS_BLUR_AND_FINISH;
    }
    else
    {
        // As this was an invalid state, set to finish
        m_rt_render_state = RT_RENDER_STATE_FINISH;
    }
}


void C3D_RENDER_RAYTRACING::rt_render_post_process_blur_finish( GLubyte *ptrPBO,
                                                                REPORTER *aStatusTextReporter )
{
    (void)aStatusTextReporter; //unused

    if( m_settings.GetFlag( FL_RENDER_RAYTRACING_POST_PROCESSING ) )
    {
        // Now blurs the shader result and compute the final color
        #pragma omp parallel for schedule(dynamic)
        for( signed int y = 0; y < (int)m_realBufferSize.y; ++y )
        {
            GLubyte *ptr = &ptrPBO[ y * m_realBufferSize.x * 4 ];

            SFVEC3F *ptrShaderY0 =
                    &m_shaderBuffer[ glm::max((int)y - 2, 0) * m_realBufferSize.x ];
            SFVEC3F *ptrShaderY1 =
                    &m_shaderBuffer[ glm::max((int)y - 1, 0) * m_realBufferSize.x ];
            SFVEC3F *ptrShaderY2 =
                    &m_shaderBuffer[ y * m_realBufferSize.x ];
            SFVEC3F *ptrShaderY3 =
                    &m_shaderBuffer[ glm::min((int)y + 1, (int)(m_realBufferSize.y - 1)) *
                                     m_realBufferSize.x ];
            SFVEC3F *ptrShaderY4 =
                    &m_shaderBuffer[ glm::min((int)y + 2, (int)(m_realBufferSize.y - 1)) *
                                     m_realBufferSize.x ];

            for( signed int x = 0; x < (int)m_realBufferSize.x; ++x )
            {
// This #if should be 1, it is here that can be used for debug proposes during development
#if 1

                SFVEC3F bluredShadeColor = (*ptrShaderY0) * 1.0f / 273.0f +
                                           (*ptrShaderY1) * 4.0f / 273.0f +
                                           (*ptrShaderY2) * 7.0f / 273.0f +
                                           (*ptrShaderY3) * 4.0f / 273.0f +
                                           (*ptrShaderY4) * 1.0f / 273.0f;
                if( x > 1 )
                {
                    ptrShaderY0++;
                    ptrShaderY1++;
                    ptrShaderY2++;
                    ptrShaderY3++;
                    ptrShaderY4++;
                }

                bluredShadeColor += (*ptrShaderY0) * 4.0f / 273.0f +
                                    (*ptrShaderY1) *16.0f / 273.0f +
                                    (*ptrShaderY2) *26.0f / 273.0f +
                                    (*ptrShaderY3) *16.0f / 273.0f +
                                    (*ptrShaderY4) * 4.0f / 273.0f;

                if( x > 0 )
                {
                    ptrShaderY0++;
                    ptrShaderY1++;
                    ptrShaderY2++;
                    ptrShaderY3++;
                    ptrShaderY4++;
                }

                bluredShadeColor += (*ptrShaderY0) * 7.0f / 273.0f +
                                    (*ptrShaderY1) *26.0f / 273.0f +
                                    (*ptrShaderY2) *41.0f / 273.0f +
                                    (*ptrShaderY3) *26.0f / 273.0f +
                                    (*ptrShaderY4) * 7.0f / 273.0f;

                if( x < ((int)m_realBufferSize.x - 1) )
                {
                    ptrShaderY0++;
                    ptrShaderY1++;
                    ptrShaderY2++;
                    ptrShaderY3++;
                    ptrShaderY4++;
                }

                bluredShadeColor += (*ptrShaderY0) * 4.0f / 273.0f +
                                    (*ptrShaderY1) *16.0f / 273.0f +
                                    (*ptrShaderY2) *26.0f / 273.0f +
                                    (*ptrShaderY3) *16.0f / 273.0f +
                                    (*ptrShaderY4) * 4.0f / 273.0f;

                if( x < ((int)m_realBufferSize.x - 2) )
                {
                    ptrShaderY0++;
                    ptrShaderY1++;
                    ptrShaderY2++;
                    ptrShaderY3++;
                    ptrShaderY4++;
                }

                bluredShadeColor += (*ptrShaderY0) * 1.0f / 273.0f +
                                    (*ptrShaderY1) * 4.0f / 273.0f +
                                    (*ptrShaderY2) * 7.0f / 273.0f +
                                    (*ptrShaderY3) * 4.0f / 273.0f +
                                    (*ptrShaderY4) * 1.0f / 273.0f;

                ptrShaderY0-= 3;
                ptrShaderY1-= 3;
                ptrShaderY2-= 3;
                ptrShaderY3-= 3;
                ptrShaderY4-= 3;

                const float grayBluredColor = ( bluredShadeColor.r +
                                                bluredShadeColor.g +
                                                bluredShadeColor.b ) / 3.0f;

                const SFVEC3F shadedColor = m_postshader_ssao.GetColorAtNotProtected(
                                            SFVEC2I( x, y ) ) * ( SFVEC3F(1.0f) -
                                                                  bluredShadeColor ) -
                                            ( bluredShadeColor - grayBluredColor * 0.5f );

                // Debug code
                //const SFVEC3F shadedColor =  ( bluredShadeColor - grayBluredColor * 0.5f);
                //const SFVEC3F shadedColor =  - glm::min( bluredShadeColor, SFVEC3F(0.0f) );
                //const SFVEC3F shadedColor =  0.5f * (SFVEC3F(1.0f) - bluredShadeColor) -
                //                             glm::min( bluredShadeColor, SFVEC3F(0.0f) );
                //const SFVEC3F shadedColor =  bluredShadeColor;
#else
                // Debug code
                //const SFVEC3F shadedColor =  SFVEC3F( 1.0f ) -
                //                             m_shaderBuffer[ y * m_realBufferSize.x + x];
                const SFVEC3F shadedColor =  m_shaderBuffer[ y * m_realBufferSize.x + x ];
#endif
                ptr[0] = (unsigned int)glm::clamp( (int)(shadedColor.r * 255), 0, 255 );
                ptr[1] = (unsigned int)glm::clamp( (int)(shadedColor.g * 255), 0, 255 );
                ptr[2] = (unsigned int)glm::clamp( (int)(shadedColor.b * 255), 0, 255 );
                ptr[3] = 255;
                ptr += 4;
            }
        }

        // Wait for all threads to finish
        #pragma omp barrier

        // Debug code
        //m_postshader_ssao.DebugBuffersOutputAsImages();
    }

    // End rendering
    m_rt_render_state = RT_RENDER_STATE_FINISH;
}


void C3D_RENDER_RAYTRACING::render_preview( GLubyte *ptrPBO )
{
    m_isPreview = true;

    unsigned int nrBlocks = m_blockPositionsFast.size();

    #pragma omp parallel for schedule(dynamic)
    for( signed int iBlock = 0; iBlock < (int)nrBlocks; iBlock++ )
    {
        const SFVEC2UI &windowPosUI = m_blockPositionsFast[ iBlock ];
        const SFVEC2I windowsPos = SFVEC2I( windowPosUI.x + m_xoffset,
                                            windowPosUI.y + m_yoffset );

        RAYPACKET blockPacket( m_settings.CameraGet(), windowsPos, 4 );

        HITINFO_PACKET hitPacket[RAYPACKET_RAYS_PER_PACKET];

        // Initialize hitPacket with a "not hit" information
        for( unsigned int i = 0; i < RAYPACKET_RAYS_PER_PACKET; ++i )
        {
            hitPacket[i].m_HitInfo.m_tHit = std::numeric_limits<float>::infinity();
            hitPacket[i].m_HitInfo.m_acc_node_info = 0;
            hitPacket[i].m_hitresult = false;
        }

        //  Intersect packet block
        m_accelerator->Intersect( blockPacket, hitPacket );


        // Calculate background gradient color
        // /////////////////////////////////////////////////////////////////////
        SFVEC3F bgColor[RAYPACKET_DIM];

        for( unsigned int y = 0; y < RAYPACKET_DIM; ++y )
        {
            const float posYfactor = (float)(windowsPos.y + y * 4.0f) / (float)m_windowSize.y;

            bgColor[y] = (SFVEC3F)m_settings.m_BgColorTop * SFVEC3F(posYfactor) +
                         (SFVEC3F)m_settings.m_BgColorBot * ( SFVEC3F(1.0f) - SFVEC3F(posYfactor) );
        }

        CCOLORRGB hitColorShading[RAYPACKET_RAYS_PER_PACKET];

        for( unsigned int i = 0; i < RAYPACKET_RAYS_PER_PACKET; ++i )
        {
            const SFVEC3F bhColorY = bgColor[i/RAYPACKET_DIM];

            if( hitPacket[i].m_hitresult == true )
            {
                const SFVEC3F hitColor = shadeHit( bhColorY,
                                                   blockPacket.m_ray[i],
                                                   hitPacket[i].m_HitInfo,
                                                   false,
                                                   0 );

                hitColorShading[i] = CCOLORRGB( hitColor );
            }
            else
                hitColorShading[i] = bhColorY;
        }

        CCOLORRGB cLRB_old[(RAYPACKET_DIM - 1)];

        for( unsigned int y = 0; y < (RAYPACKET_DIM - 1); ++y )
        {

            const SFVEC3F     bgColorY = bgColor[y];
            const CCOLORRGB   bgColorYRGB = CCOLORRGB( bgColorY );

            // This stores cRTB from the last block to be reused next time in a cLTB pixel
            CCOLORRGB cRTB_old;

            //RAY       cRTB_ray;
            //HITINFO   cRTB_hitInfo;

            for( unsigned int x = 0; x < (RAYPACKET_DIM - 1); ++x )
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

                const unsigned int iLT = ((x + 0) + RAYPACKET_DIM * (y + 0));
                const unsigned int iRT = ((x + 1) + RAYPACKET_DIM * (y + 0));
                const unsigned int iLB = ((x + 0) + RAYPACKET_DIM * (y + 1));
                const unsigned int iRB = ((x + 1) + RAYPACKET_DIM * (y + 1));

                // !TODO: skip when there are no hits


                const CCOLORRGB &cLT = hitColorShading[ iLT ];
                const CCOLORRGB &cRT = hitColorShading[ iRT ];
                const CCOLORRGB &cLB = hitColorShading[ iLB ];
                const CCOLORRGB &cRB = hitColorShading[ iRB ];

                // Trace and shade cC
                // /////////////////////////////////////////////////////////////
                CCOLORRGB cC = bgColorYRGB;

                const SFVEC3F &oriLT = blockPacket.m_ray[ iLT ].m_Origin;
                const SFVEC3F &oriRB = blockPacket.m_ray[ iRB ].m_Origin;

                const SFVEC3F &dirLT = blockPacket.m_ray[ iLT ].m_Dir;
                const SFVEC3F &dirRB = blockPacket.m_ray[ iRB ].m_Dir;

                SFVEC3F oriC;
                SFVEC3F dirC;

                HITINFO centerHitInfo;
                centerHitInfo.m_tHit = std::numeric_limits<float>::infinity();

                bool hittedC = false;

                if( (hitPacket[ iLT ].m_hitresult == true) ||
                    (hitPacket[ iRT ].m_hitresult == true) ||
                    (hitPacket[ iLB ].m_hitresult == true) ||
                    (hitPacket[ iRB ].m_hitresult == true) )
                {

                    oriC = ( oriLT + oriRB ) * 0.5f;
                    dirC = glm::normalize( ( dirLT + dirRB ) * 0.5f );

                    // Trace the center ray
                    RAY centerRay;
                    centerRay.Init( oriC, dirC );

                    const unsigned int nodeLT = (hitPacket[ iLT ].m_hitresult == false)?0:
                                                 hitPacket[ iLT ].m_HitInfo.m_acc_node_info;

                    const unsigned int nodeRT = (hitPacket[ iRT ].m_hitresult == false)?0:
                                                 hitPacket[ iRT ].m_HitInfo.m_acc_node_info;

                    const unsigned int nodeLB = (hitPacket[ iLB ].m_hitresult == false)?0:
                                                 hitPacket[ iLB ].m_HitInfo.m_acc_node_info;

                    const unsigned int nodeRB = (hitPacket[ iRB ].m_hitresult == false)?0:
                                                 hitPacket[ iRB ].m_HitInfo.m_acc_node_info;

                    if( nodeLT != 0 )
                        hittedC |= m_accelerator->Intersect( centerRay, centerHitInfo, nodeLT );

                    if( nodeRT != 0 )
                        if( ( nodeRT != nodeLT ) || ( nodeLT == 0 ) )
                            hittedC |= m_accelerator->Intersect( centerRay, centerHitInfo, nodeRT );

                    if( nodeLB != 0 )
                        if( ( ( nodeLB != nodeLT ) || ( nodeLT == 0 ) ) &&
                            ( ( nodeLB != nodeRT ) || ( nodeRT == 0 ) ) )
                            hittedC |= m_accelerator->Intersect( centerRay, centerHitInfo, nodeLB );

                    if( nodeRB != 0 )
                        if( ( ( nodeRB != nodeLB ) || ( nodeLB == 0 ) ) &&
                            ( ( nodeRB != nodeLT ) || ( nodeLT == 0 ) ) &&
                            ( ( nodeRB != nodeRT ) || ( nodeRT == 0 ) ) )
                            hittedC |= m_accelerator->Intersect( centerRay, centerHitInfo, nodeRB );

                    if( hittedC )
                        cC = CCOLORRGB( shadeHit( bgColorY, centerRay, centerHitInfo, false, 0 ) );
                    else
                    {
                        centerHitInfo.m_tHit = std::numeric_limits<float>::infinity();
                        hittedC = m_accelerator->Intersect( centerRay, centerHitInfo );

                        if( hittedC )
                            cC = CCOLORRGB( shadeHit( bgColorY,
                                                      centerRay,
                                                      centerHitInfo,
                                                      false,
                                                      0 ) );
                    }
                }

                // Trace and shade cLRT
                // /////////////////////////////////////////////////////////////
                CCOLORRGB cLRT = bgColorYRGB;

                const SFVEC3F &oriRT = blockPacket.m_ray[ iRT ].m_Origin;
                const SFVEC3F &dirRT = blockPacket.m_ray[ iRT ].m_Dir;

                if( y == 0 )
                {
                    // Trace the center ray
                    RAY rayLRT;
                    rayLRT.Init( ( oriLT + oriRT ) * 0.5f,
                                    glm::normalize( ( dirLT + dirRT ) * 0.5f ) );

                    HITINFO hitInfoLRT;
                    hitInfoLRT.m_tHit = std::numeric_limits<float>::infinity();

                    if( hitPacket[ iLT ].m_hitresult &&
                        hitPacket[ iRT ].m_hitresult &&
                        (hitPacket[ iLT ].m_HitInfo.pHitObject == hitPacket[ iRT ].m_HitInfo.pHitObject) )
                    {
                        hitInfoLRT.pHitObject = hitPacket[ iLT ].m_HitInfo.pHitObject;
                        hitInfoLRT.m_tHit = ( hitPacket[ iLT ].m_HitInfo.m_tHit +
                                              hitPacket[ iRT ].m_HitInfo.m_tHit ) * 0.5f;
                        hitInfoLRT.m_HitNormal =
                                glm::normalize( ( hitPacket[ iLT ].m_HitInfo.m_HitNormal +
                                                  hitPacket[ iRT ].m_HitInfo.m_HitNormal ) * 0.5f );

                        cLRT = CCOLORRGB( shadeHit( bgColorY, rayLRT, hitInfoLRT, false, 0 ) );
                        cLRT = BlendColor( cLRT, BlendColor( cLT, cRT) );
                    }
                    else
                    {
                        if( hitPacket[ iLT ].m_hitresult ||
                            hitPacket[ iRT ].m_hitresult )                  // If any hits
                        {
                            const unsigned int nodeLT = (hitPacket[ iLT ].m_hitresult == false)?0:
                                                         hitPacket[ iLT ].m_HitInfo.m_acc_node_info;

                            const unsigned int nodeRT = (hitPacket[ iRT ].m_hitresult == false)?0:
                                                         hitPacket[ iRT ].m_HitInfo.m_acc_node_info;

                            bool hittedLRT = false;

                            if( nodeLT != 0 )
                                hittedLRT |= m_accelerator->Intersect( rayLRT, hitInfoLRT, nodeLT );

                            if( nodeRT != 0 )
                                if( ( nodeRT != nodeLT ) || ( nodeLT == 0 ) )
                                    hittedLRT |= m_accelerator->Intersect( rayLRT,
                                                                           hitInfoLRT,
                                                                           nodeRT );

                            if( hittedLRT )
                                cLRT = CCOLORRGB( shadeHit( bgColorY,
                                                            rayLRT,
                                                            hitInfoLRT,
                                                            false,
                                                            0 ) );
                            else
                            {
                                hitInfoLRT.m_tHit = std::numeric_limits<float>::infinity();

                                if( m_accelerator->Intersect( rayLRT,hitInfoLRT ) )
                                    cLRT = CCOLORRGB( shadeHit( bgColorY,
                                                                rayLRT,
                                                                hitInfoLRT,
                                                                false,
                                                                0 ) );
                            }
                        }
                    }
                }
                else
                    cLRT = cLRB_old[x];


                // Trace and shade cLTB
                // /////////////////////////////////////////////////////////////
                CCOLORRGB cLTB = bgColorYRGB;

                if( x == 0 )
                {
                    const SFVEC3F &oriLB = blockPacket.m_ray[ iLB ].m_Origin;
                    const SFVEC3F &dirLB = blockPacket.m_ray[ iLB ].m_Dir;

                    // Trace the center ray
                    RAY rayLTB;
                    rayLTB.Init( ( oriLT + oriLB ) * 0.5f,
                                    glm::normalize( ( dirLT + dirLB ) * 0.5f ) );

                    HITINFO hitInfoLTB;
                    hitInfoLTB.m_tHit = std::numeric_limits<float>::infinity();

                    if( hitPacket[ iLT ].m_hitresult &&
                        hitPacket[ iLB ].m_hitresult &&
                        ( hitPacket[ iLT ].m_HitInfo.pHitObject ==
                          hitPacket[ iLB ].m_HitInfo.pHitObject ) )
                    {
                        hitInfoLTB.pHitObject = hitPacket[ iLT ].m_HitInfo.pHitObject;
                        hitInfoLTB.m_tHit = ( hitPacket[ iLT ].m_HitInfo.m_tHit +
                                              hitPacket[ iLB ].m_HitInfo.m_tHit ) * 0.5f;
                        hitInfoLTB.m_HitNormal =
                                glm::normalize( ( hitPacket[ iLT ].m_HitInfo.m_HitNormal +
                                                  hitPacket[ iLB ].m_HitInfo.m_HitNormal ) * 0.5f );
                        cLTB = CCOLORRGB( shadeHit( bgColorY, rayLTB, hitInfoLTB, false, 0 ) );
                        cLTB = BlendColor( cLTB, BlendColor( cLT, cLB) );
                    }
                    else
                    {
                        if( hitPacket[ iLT ].m_hitresult ||
                            hitPacket[ iLB ].m_hitresult )                  // If any hits
                        {
                            const unsigned int nodeLT = (hitPacket[ iLT ].m_hitresult == false)?0:
                                                         hitPacket[ iLT ].m_HitInfo.m_acc_node_info;

                            const unsigned int nodeLB = (hitPacket[ iLB ].m_hitresult == false)?0:
                                                         hitPacket[ iLB ].m_HitInfo.m_acc_node_info;

                            bool hittedLTB = false;

                            if( nodeLT != 0 )
                                hittedLTB |= m_accelerator->Intersect( rayLTB,
                                                                       hitInfoLTB,
                                                                       nodeLT );

                            if( nodeLB != 0 )
                                if( ( nodeLB != nodeLT ) || ( nodeLT == 0 ) )
                                    hittedLTB |= m_accelerator->Intersect( rayLTB,
                                                                           hitInfoLTB,
                                                                           nodeLB );

                            if( hittedLTB )
                                cLTB = CCOLORRGB( shadeHit( bgColorY,
                                                            rayLTB,
                                                            hitInfoLTB,
                                                            false,
                                                            0 ) );
                            else
                            {
                                hitInfoLTB.m_tHit = std::numeric_limits<float>::infinity();

                                if( m_accelerator->Intersect( rayLTB, hitInfoLTB ) )
                                    cLTB = CCOLORRGB( shadeHit( bgColorY,
                                                                rayLTB,
                                                                hitInfoLTB,
                                                                false,
                                                                0 ) );
                            }
                        }
                    }
                }
                else
                    cLTB = cRTB_old;


                // Trace and shade cRTB
                // /////////////////////////////////////////////////////////////
                CCOLORRGB cRTB = bgColorYRGB;

                // Trace the center ray
                RAY rayRTB;
                rayRTB.Init( ( oriRT + oriRB ) * 0.5f,
                                glm::normalize( ( dirRT + dirRB ) * 0.5f ) );

                HITINFO hitInfoRTB;
                hitInfoRTB.m_tHit = std::numeric_limits<float>::infinity();

                if( hitPacket[ iRT ].m_hitresult &&
                    hitPacket[ iRB ].m_hitresult &&
                    ( hitPacket[ iRT ].m_HitInfo.pHitObject ==
                      hitPacket[ iRB ].m_HitInfo.pHitObject ) )
                {
                    hitInfoRTB.pHitObject = hitPacket[ iRT ].m_HitInfo.pHitObject;

                    hitInfoRTB.m_tHit = ( hitPacket[ iRT ].m_HitInfo.m_tHit +
                                          hitPacket[ iRB ].m_HitInfo.m_tHit ) * 0.5f;

                    hitInfoRTB.m_HitNormal =
                            glm::normalize( ( hitPacket[ iRT ].m_HitInfo.m_HitNormal +
                                              hitPacket[ iRB ].m_HitInfo.m_HitNormal ) * 0.5f );

                    cRTB = CCOLORRGB( shadeHit( bgColorY, rayRTB, hitInfoRTB, false, 0 ) );
                    cRTB = BlendColor( cRTB, BlendColor( cRT, cRB) );
                }
                else
                {
                    if( hitPacket[ iRT ].m_hitresult ||
                        hitPacket[ iRB ].m_hitresult )                  // If any hits
                    {
                        const unsigned int nodeRT = (hitPacket[ iRT ].m_hitresult == false)?0:
                                                     hitPacket[ iRT ].m_HitInfo.m_acc_node_info;
                        const unsigned int nodeRB = (hitPacket[ iRB ].m_hitresult == false)?0:
                                                     hitPacket[ iRB ].m_HitInfo.m_acc_node_info;

                        bool hittedRTB = false;

                        if( nodeRT != 0 )
                            hittedRTB |= m_accelerator->Intersect( rayRTB, hitInfoRTB, nodeRT );

                        if( nodeRB != 0 )
                            if( ( nodeRB != nodeRT ) || ( nodeRT == 0 ) )
                                hittedRTB |= m_accelerator->Intersect( rayRTB, hitInfoRTB, nodeRB );

                        if( hittedRTB )
                            cRTB = CCOLORRGB( shadeHit( bgColorY,
                                                        rayRTB,
                                                        hitInfoRTB,
                                                        false,
                                                        0 ) );
                        else
                        {
                            hitInfoRTB.m_tHit = std::numeric_limits<float>::infinity();

                            if( m_accelerator->Intersect( rayRTB, hitInfoRTB ) )
                                cRTB = CCOLORRGB( shadeHit( bgColorY,
                                                            rayRTB,
                                                            hitInfoRTB,
                                                            false,
                                                            0 ) );
                        }
                    }
                }

                cRTB_old = cRTB;


                // Trace and shade cLRB
                // /////////////////////////////////////////////////////////////
                CCOLORRGB cLRB = bgColorYRGB;

                const SFVEC3F &oriLB = blockPacket.m_ray[ iLB ].m_Origin;
                const SFVEC3F &dirLB = blockPacket.m_ray[ iLB ].m_Dir;

                // Trace the center ray
                RAY rayLRB;
                rayLRB.Init( ( oriLB + oriRB ) * 0.5f,
                                glm::normalize( ( dirLB + dirRB ) * 0.5f ) );

                HITINFO hitInfoLRB;
                hitInfoLRB.m_tHit = std::numeric_limits<float>::infinity();

                if( hitPacket[ iLB ].m_hitresult &&
                    hitPacket[ iRB ].m_hitresult &&
                    ( hitPacket[ iLB ].m_HitInfo.pHitObject ==
                      hitPacket[ iRB ].m_HitInfo.pHitObject ) )
                {
                    hitInfoLRB.pHitObject = hitPacket[ iLB ].m_HitInfo.pHitObject;

                    hitInfoLRB.m_tHit = ( hitPacket[ iLB ].m_HitInfo.m_tHit +
                                          hitPacket[ iRB ].m_HitInfo.m_tHit ) * 0.5f;

                    hitInfoLRB.m_HitNormal =
                            glm::normalize( ( hitPacket[ iLB ].m_HitInfo.m_HitNormal +
                                              hitPacket[ iRB ].m_HitInfo.m_HitNormal ) * 0.5f );

                    cLRB = CCOLORRGB( shadeHit( bgColorY, rayLRB, hitInfoLRB, false, 0 ) );
                    cLRB = BlendColor( cLRB, BlendColor( cLB, cRB) );
                }
                else
                {
                    if( hitPacket[ iLB ].m_hitresult ||
                        hitPacket[ iRB ].m_hitresult )                  // If any hits
                    {
                        const unsigned int nodeLB = (hitPacket[ iLB ].m_hitresult == false)?0:
                                                     hitPacket[ iLB ].m_HitInfo.m_acc_node_info;
                        const unsigned int nodeRB = (hitPacket[ iRB ].m_hitresult == false)?0:
                                                     hitPacket[ iRB ].m_HitInfo.m_acc_node_info;

                        bool hittedLRB = false;

                        if( nodeLB != 0 )
                            hittedLRB |= m_accelerator->Intersect( rayLRB, hitInfoLRB, nodeLB );

                        if( nodeRB != 0 )
                            if( ( nodeRB != nodeLB ) || ( nodeLB == 0 ) )
                                hittedLRB |= m_accelerator->Intersect( rayLRB, hitInfoLRB, nodeRB );

                        if( hittedLRB )
                            cLRB = CCOLORRGB( shadeHit( bgColorY, rayLRB, hitInfoLRB, false, 0 ) );
                        else
                        {
                            hitInfoLRB.m_tHit = std::numeric_limits<float>::infinity();

                            if( m_accelerator->Intersect( rayLRB, hitInfoLRB ) )
                                cLRB = CCOLORRGB( shadeHit( bgColorY,
                                                            rayLRB,
                                                            hitInfoLRB,
                                                            false,
                                                            0 ) );
                        }
                    }
                }

                cLRB_old[x] = cLRB;


                // Trace and shade cLTC
                // /////////////////////////////////////////////////////////////
                CCOLORRGB cLTC = BlendColor( cLT , cC );

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
                    else
                        if( hitPacket[ iLT ].m_hitresult )
                            hitted = hitPacket[ iLT ].m_HitInfo.pHitObject->Intersect( rayLTC,
                                                                                       hitInfoLTC );

                    if( hitted )
                        cLTC = CCOLORRGB( shadeHit( bgColorY, rayLTC, hitInfoLTC, false, 0 ) );
                }


                // Trace and shade cRTC
                // /////////////////////////////////////////////////////////////
                CCOLORRGB cRTC = BlendColor( cRT , cC );

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
                    else
                        if( hitPacket[ iRT ].m_hitresult )
                            hitted = hitPacket[ iRT ].m_HitInfo.pHitObject->Intersect( rayRTC,
                                                                                       hitInfoRTC );

                    if( hitted )
                        cRTC = CCOLORRGB( shadeHit( bgColorY, rayRTC, hitInfoRTC, false, 0 ) );
                }


                // Trace and shade cLBC
                // /////////////////////////////////////////////////////////////
                CCOLORRGB cLBC = BlendColor( cLB , cC );

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
                    else
                        if( hitPacket[ iLB ].m_hitresult )
                            hitted = hitPacket[ iLB ].m_HitInfo.pHitObject->Intersect( rayLBC,
                                                                                       hitInfoLBC );

                    if( hitted )
                        cLBC = CCOLORRGB( shadeHit( bgColorY, rayLBC, hitInfoLBC, false, 0 ) );
                }


                // Trace and shade cRBC
                // /////////////////////////////////////////////////////////////
                CCOLORRGB cRBC = BlendColor( cRB , cC );

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
                    else
                        if( hitPacket[ iRB ].m_hitresult )
                            hitted = hitPacket[ iRB ].m_HitInfo.pHitObject->Intersect( rayRBC,
                                                                                       hitInfoRBC );

                    if( hitted )
                        cRBC = CCOLORRGB( shadeHit( bgColorY, rayRBC, hitInfoRBC, false, 0 ) );
                }


                // Set pixel colors
                // /////////////////////////////////////////////////////////////

                GLubyte *ptr = &ptrPBO[ (4 * x + m_blockPositionsFast[iBlock].x +
                                         m_realBufferSize.x *
                                         (m_blockPositionsFast[iBlock].y + 4 * y)) * 4 ];
                SetPixel( ptr +  0, cLT );
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

    // Wait for all threads to finish (not sure if this is need)
    #pragma omp barrier
}


#define USE_EXPERIMENTAL_SOFT_SHADOWS 1

SFVEC3F C3D_RENDER_RAYTRACING::shadeHit( const SFVEC3F &aBgColor,
                                         const RAY &aRay,
                                         HITINFO &aHitInfo,
                                         bool aIsInsideObject,
                                         unsigned int aRecursiveLevel ) const
{
    if( aRecursiveLevel > 2 )
        return SFVEC3F( 0.0f );

    SFVEC3F hitPoint;

    if( m_isPreview )
        hitPoint = aRay.at( aHitInfo.m_tHit );
    else
        hitPoint = aRay.at( aHitInfo.m_tHit ) +
                   aHitInfo.m_HitNormal * ( 0.5f * m_settings.GetNonCopperLayerThickness3DU() *
                                            glm::abs(Fast_RandFloat()) +
                                            0.5f * m_settings.GetNonCopperLayerThickness3DU() );

    const CMATERIAL *objMaterial = aHitInfo.pHitObject->GetMaterial();
    wxASSERT( objMaterial != NULL );

    const SFVEC3F diffuseColorObj = aHitInfo.pHitObject->GetDiffuseColor( aHitInfo );

    SFVEC3F outColor = objMaterial->GetEmissiveColor();

    const LIST_LIGHT &lightList = m_lights.GetList();

    const bool is_testShadow = m_settings.GetFlag( FL_RENDER_RAYTRACING_SHADOWS ) &&
                               (!m_isPreview);

#if USE_EXPERIMENTAL_SOFT_SHADOWS
    const bool is_aa_enabled = m_settings.GetFlag( FL_RENDER_RAYTRACING_ANTI_ALIASING ) &&
                               (!m_isPreview);
#endif

    float shadow_att_factor_sum = 0.0f;

    unsigned int nr_lights_that_can_cast_shadows = 0;

    for( LIST_LIGHT::const_iterator ii = lightList.begin();
         ii != lightList.end();
         ++ii )
    {
        const CLIGHT *light = (CLIGHT *)*ii;

        SFVEC3F vectorToLight;
        SFVEC3F colorOfLight;
        float   distToLight;

        light->GetLightParameters( hitPoint, vectorToLight, colorOfLight, distToLight );

        if( m_isPreview )
            colorOfLight = SFVEC3F( 1.0f );

        /*
        if( (!m_isPreview) &&
            // Little hack to make randomness to the shading and shadows
            m_settings.GetFlag( FL_RENDER_RAYTRACING_POST_PROCESSING ) )
            vectorToLight = glm::normalize( vectorToLight +
                                            UniformRandomHemisphereDirection() * 0.1f );
         */

        const float NdotL = glm::dot( aHitInfo.m_HitNormal, vectorToLight );

        // Only calc shade if the normal is facing the direction of light,
        // otherwise it is in the shadow
        if( NdotL >= FLT_EPSILON )
        {
            float   shadow_att_factor_light = 1.0f;

            if( is_testShadow && light->GetCastShadows() )
            {
                nr_lights_that_can_cast_shadows++;
#if USE_EXPERIMENTAL_SOFT_SHADOWS
                if( (!is_aa_enabled) ||

                    // For rays that are recursive, just calculate one hit shadow
                    (aRecursiveLevel > 0) ||

                    // Only use soft shadows if using post processing
                    (!m_settings.GetFlag( FL_RENDER_RAYTRACING_POST_PROCESSING ) )
                  )
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

                // Experimental softshadow calculation
                else
                {

                    const unsigned int shadow_number_of_samples = 2;
                    const float shadow_inc_factor = 1.0f / (float)(shadow_number_of_samples);

                    for( unsigned int i=0; i < shadow_number_of_samples; ++i )
                    {
                        const SFVEC3F unifVector = UniformRandomHemisphereDirection();
                        const SFVEC3F disturbed_vector_to_light = glm::normalize( vectorToLight +
                                                                                  unifVector *
                                                                                  0.05f );

                        RAY rayToLight;
                        rayToLight.Init( hitPoint, disturbed_vector_to_light );

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

            if( !m_settings.GetFlag( FL_RENDER_RAYTRACING_POST_PROCESSING ) )
            {
                outColor += objMaterial->Shade( aRay,
                                                aHitInfo,
                                                NdotL,
                                                diffuseColorObj,
                                                vectorToLight,
                                                colorOfLight,
                                                shadow_att_factor_light );
            }
            else
            {
                // This is a render hack in order to compensate for the lake of
                // ambient and too much darkness when using post process shadeer
                // It will calculate as it was not in shadow
                outColor += objMaterial->Shade( aRay,
                                                aHitInfo,
                                                NdotL,
                                                diffuseColorObj,
                                                vectorToLight,
                                                colorOfLight,
                                                1.0f );
            }
        }

        if( nr_lights_that_can_cast_shadows > 0 )
        {
            aHitInfo.m_ShadowFactor = shadow_att_factor_sum /
                                      (float)(nr_lights_that_can_cast_shadows * 1.0f);
        }
        else
        {
            aHitInfo.m_ShadowFactor = 1.0f;
        }

        // Only use the headlight for preview
        if( m_isPreview )
            break;
    }

    // Clamp color to not be brighter than 1.0f
    outColor = glm::min( outColor, SFVEC3F( 1.0f ) );

    if( !m_isPreview )
    {
        // Reflections
        // /////////////////////////////////////////////////////////////////////

        if( !aIsInsideObject &&
            (objMaterial->GetReflection() > 0.0f) &&
            m_settings.GetFlag( FL_RENDER_RAYTRACING_REFLECTIONS ) )
        {
            const unsigned int reflection_number_of_samples = 3;

            SFVEC3F sum_color = SFVEC3F(0.0f);

            const SFVEC3F reflectVector = aRay.m_Dir -
                                    2.0f * glm::dot( aRay.m_Dir, aHitInfo.m_HitNormal ) *
                                    aHitInfo.m_HitNormal;

            for( unsigned int i = 0; i < reflection_number_of_samples; ++i )
            {
                // If we want to apply some randomize to the reflected vector
                const SFVEC3F random_reflectVector =
                        glm::normalize( reflectVector +
                                        UniformRandomHemisphereDirection() *
                                        0.02f );

                RAY reflectedRay;
                reflectedRay.Init( hitPoint, random_reflectVector );

                HITINFO reflectedHit;
                reflectedHit.m_tHit = std::numeric_limits<float>::infinity();

                if( m_accelerator->Intersect( reflectedRay, reflectedHit ) )
                {
                    sum_color += objMaterial->GetReflection() *
                                 shadeHit( aBgColor,
                                           reflectedRay,
                                           reflectedHit,
                                           false,
                                           aRecursiveLevel + 1 ) *
                                 (1.0f / ( 1.0f + 0.75f * reflectedHit.m_tHit *
                                                          reflectedHit.m_tHit) ); // Falloff factor
                }
            }

            outColor += (sum_color / SFVEC3F( (float)reflection_number_of_samples) );
        }


        // Refractions
        // /////////////////////////////////////////////////////////////////////

        if( (objMaterial->GetTransparency() > 0.0f) &&
            m_settings.GetFlag( FL_RENDER_RAYTRACING_REFRACTIONS ) )
        {
            const float airIndex = 1.000293f;
            const float glassIndex = 1.49f;
            const float air_over_glass = airIndex / glassIndex;
            const float glass_over_air = glassIndex / airIndex;

            float refractionRatio = aIsInsideObject?glass_over_air:air_over_glass;

            SFVEC3F refractedVector;

            if( Refract( aRay.m_Dir,
                         aHitInfo.m_HitNormal,
                         refractionRatio,
                         refractedVector ) )
            {
                // If we want to apply some randomize to the refracted vector
                //refractedVector = refractedVector + UniformRandomHemisphereDirection() * 0.01f;
                refractedVector = glm::normalize( refractedVector );

                // This increase the start point by a "fixed" factor so it will work the
                // same for all distances
                const SFVEC3F startPoint = aRay.at( NextFloatUp(
                                                    NextFloatUp(
                                                    NextFloatUp( aHitInfo.m_tHit ) ) ) );

                RAY refractedRay;
                refractedRay.Init( startPoint, refractedVector );

                HITINFO refractedHit;
                refractedHit.m_tHit = std::numeric_limits<float>::infinity();

                SFVEC3F refractedColor = aBgColor;

                float objTransparency = objMaterial->GetTransparency();

                if( m_accelerator->Intersect( refractedRay, refractedHit ) )
                {
                    refractedColor = shadeHit( aBgColor,
                                               refractedRay,
                                               refractedHit,
                                               true,
                                               aRecursiveLevel + 1 );

                    const SFVEC3F absorbance = ( SFVEC3F(1.0f) - diffuseColorObj ) *
                                               (1.0f - objTransparency ) *
                                               1.0000f *  // Adjust falloff factor
                                               -refractedHit.m_tHit;

                    const SFVEC3F transparency = SFVEC3F( expf( absorbance.r ),
                                                          expf( absorbance.g ),
                                                          expf( absorbance.b ) );

                    outColor = outColor * (1.0f - objTransparency) +
                               refractedColor * transparency * objTransparency;
                }
                else
                {
                    outColor = outColor * (1.0f - objTransparency) +
                               refractedColor * objTransparency;
                }
            }
        }
    }

   //outColor += glm::max( -glm::dot( aHitInfo.m_HitNormal, aRay.m_Dir ), 0.0f ) *
   //            objMaterial->GetAmbientColor();

    return outColor;
}


void C3D_RENDER_RAYTRACING::initializeNewWindowSize()
{
    opengl_init_pbo();
}


void C3D_RENDER_RAYTRACING::opengl_init_pbo()
{
    if( GLEW_ARB_pixel_buffer_object )
    {
        m_opengl_support_vertex_buffer_object = true;

        // Try to delete vbo if it was already initialized
        opengl_delete_pbo();

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
                    wxT( "C3D_RENDER_RAYTRACING:: GLEW_ARB_pixel_buffer_object is supported" ) );
    }
}


bool C3D_RENDER_RAYTRACING::initializeOpenGL()
{
    m_is_opengl_initialized = true;

    return true;
}


void C3D_RENDER_RAYTRACING::initialize_block_positions()
{

    m_realBufferSize = SFVEC2UI();

    // Calc block positions for fast preview mode
    // /////////////////////////////////////////////////////////////////////
    m_blockPositionsFast.clear();

    unsigned int i = 0;

    while(1)
    {
        const unsigned int mX = DecodeMorton2X(i);
        const unsigned int mY = DecodeMorton2Y(i);

        i++;

        const SFVEC2UI blockPos( mX * 4 * RAYPACKET_DIM - mX * 4,
                                 mY * 4 * RAYPACKET_DIM - mY * 4);

        if( ( blockPos.x >= ( (unsigned int)m_windowSize.x - ( 4 * RAYPACKET_DIM + 4 ) ) ) &&
            ( blockPos.y >= ( (unsigned int)m_windowSize.y - ( 4 * RAYPACKET_DIM + 4 ) ) ) )
            break;

        if( ( blockPos.x < ( (unsigned int)m_windowSize.x - ( 4 * RAYPACKET_DIM + 4) ) ) &&
            ( blockPos.y < ( (unsigned int)m_windowSize.y - ( 4 * RAYPACKET_DIM + 4) ) ) )
        {
            m_blockPositionsFast.push_back( blockPos );

            if( blockPos.x > m_realBufferSize.x )
                m_realBufferSize.x = blockPos.x;

            if( blockPos.y > m_realBufferSize.y )
                m_realBufferSize.y = blockPos.y;
        }
    }

    m_fastPreviewModeSize = m_realBufferSize;

    m_realBufferSize.x = ((m_realBufferSize.x + RAYPACKET_DIM * 4) & RAYPACKET_INVMASK);
    m_realBufferSize.y = ((m_realBufferSize.y + RAYPACKET_DIM * 4) & RAYPACKET_INVMASK);

    m_xoffset = (m_windowSize.x - m_realBufferSize.x) / 2;
    m_yoffset = (m_windowSize.y - m_realBufferSize.y) / 2;

    m_postshader_ssao.UpdateSize( m_realBufferSize );


    // Calc block positions
    // /////////////////////////////////////////////////////////////////////
    m_blockPositions.clear();
    m_blockPositions.reserve( (m_realBufferSize.x / RAYPACKET_DIM) *
                              (m_realBufferSize.y / RAYPACKET_DIM) );

    i = 0;

    while(1)
    {
        SFVEC2UI blockPos( DecodeMorton2X(i) * RAYPACKET_DIM,
                           DecodeMorton2Y(i) * RAYPACKET_DIM );
        i++;

        if( (blockPos.x >= m_realBufferSize.x) && (blockPos.y >= m_realBufferSize.y) )
            break;

        if( (blockPos.x < m_realBufferSize.x) && (blockPos.y < m_realBufferSize.y) )
            m_blockPositions.push_back( blockPos );
    }

    // Create m_shader buffer
    delete m_shaderBuffer;
    m_shaderBuffer = new SFVEC3F[m_realBufferSize.x * m_realBufferSize.y];

    opengl_init_pbo();
}
