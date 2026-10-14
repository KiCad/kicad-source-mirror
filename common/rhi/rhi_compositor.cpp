/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <rhi/rhi_compositor.h>
#include <rhi/rhi_device.h>

#include <trace_helpers.h>
#include <wx/log.h>

#include <utility>


RHI_COMPOSITOR::RHI_COMPOSITOR( RHI_DEVICE& aDevice ) :
    m_device( aDevice )
{
}


RHI_COMPOSITOR::~RHI_COMPOSITOR()
{
    Destroy();
}


bool RHI_COMPOSITOR::Initialize( RHI_BindGroupLayoutHandle aSingleTexLayout )
{
    RHI_RENDER_PASS_DESC clearRpDesc;
    RHI_COLOR_ATTACHMENT_DESC clearAttach;
    clearAttach.format    = RHI_TEXTURE_FORMAT::RGBA8;
    clearAttach.loadOp    = RHI_LOAD_OP::CLEAR;
    clearAttach.storeOp   = RHI_STORE_OP::STORE;
    clearAttach.clearColor[0] = 0.0f;
    clearAttach.clearColor[1] = 0.0f;
    clearAttach.clearColor[2] = 0.0f;
    clearAttach.clearColor[3] = 0.0f;
    clearRpDesc.colorAttachments.push_back( clearAttach );

    m_clearRenderPass = m_device.CreateRenderPass( clearRpDesc );

    if( !m_clearRenderPass )
    {
        Destroy();
        return false;
    }

    // The shared 16-byte vertex layout, of which the compositor uses only position and UV
    struct QuadVertex
    {
        float    x, y;
        uint8_t  r, g, b, a;
        uint16_t u, v;
    };

    // Half-float encodings of 0.0 and 1.0
    constexpr uint16_t H0 = 0x0000;
    constexpr uint16_t H1 = 0x3C00;

    QuadVertex verts[6] = {
        { -1.0f, -1.0f, 255, 255, 255, 255, H0, H0 },
        {  1.0f, -1.0f, 255, 255, 255, 255, H1, H0 },
        {  1.0f,  1.0f, 255, 255, 255, 255, H1, H1 },
        { -1.0f, -1.0f, 255, 255, 255, 255, H0, H0 },
        {  1.0f,  1.0f, 255, 255, 255, 255, H1, H1 },
        { -1.0f,  1.0f, 255, 255, 255, 255, H0, H1 },
    };

    RHI_BUFFER_DESC bufDesc;
    bufDesc.type      = RHI_BUFFER_TYPE::VERTEX;
    bufDesc.sizeBytes = sizeof( verts );
    bufDesc.mappable  = false;

    m_quadVB = m_device.CreateBuffer( bufDesc );

    if( !m_quadVB )
    {
        Destroy();
        return false;
    }

    m_device.UpdateBuffer( m_quadVB, verts, sizeof( verts ) );

    // Vulkan descriptor sets must match the layout baked into the fullscreen pipelines
    m_singleTexLayout = aSingleTexLayout;
    m_ownsSingleTexLayout = !aSingleTexLayout;

    if( !m_singleTexLayout )
    {
        RHI_BIND_GROUP_LAYOUT_DESC singleTexLayoutDesc;
        singleTexLayoutDesc.frequency = RHI_BIND_FREQUENCY::PER_DRAW;
        singleTexLayoutDesc.entries.push_back(
                { 0, RHI_BINDING_TYPE::COMBINED_TEXTURE_SAMPLER, RHI_SHADER_STAGE::FRAGMENT } );

        m_singleTexLayout = m_device.CreateBindGroupLayout( singleTexLayoutDesc );

        if( !m_singleTexLayout )
        {
            Destroy();
            return false;
        }
    }

    m_initialized = true;

    return true;
}


void RHI_COMPOSITOR::Destroy()
{
    destroyFBOBindGroups();

    if( m_ownsSingleTexLayout )
        m_device.Release( m_singleTexLayout );

    m_singleTexLayout = {};
    m_ownsSingleTexLayout = true;
    m_device.Release( m_quadVB );
    m_device.Release( m_clearRenderPass );

    m_initialized = false;
}


void RHI_COMPOSITOR::SetCompositorPipeline( RHI_PipelineHandle aPipeline )
{
    m_compositorPipeline = aPipeline;
}


RHI_FramebufferHandle RHI_COMPOSITOR::Composite( RHI_FramebufferHandle aAccumulationFBO, uint32_t aWidth,
                                                 uint32_t aHeight, const std::vector<COMPOSITOR_LAYER>& aLayers,
                                                 const float* aBackgroundColor,
                                                 const std::function<void()>& aDrawOnTop )
{
    if( !m_initialized )
        return {};

    if( !m_compositorPipeline || !m_quadVB )
        return {};

    RHI_RENDER_PASS_BEGIN_DESC beginDesc;
    beginDesc.renderPass  = m_clearRenderPass;
    beginDesc.framebuffer = aAccumulationFBO;

    if( aBackgroundColor )
    {
        beginDesc.hasClearColorOverride = true;
        beginDesc.clearColorOverride[0] = aBackgroundColor[0];
        beginDesc.clearColorOverride[1] = aBackgroundColor[1];
        beginDesc.clearColorOverride[2] = aBackgroundColor[2];
        beginDesc.clearColorOverride[3] = aBackgroundColor[3];
    }

    bool rpOk = m_device.BeginRenderPass( beginDesc );

    if( !rpOk )
        return {};

    RHI_VIEWPORT viewport;
    viewport.width  = static_cast<float>( aWidth );
    viewport.height = static_cast<float>( aHeight );
    m_device.SetViewport( viewport );

    RHI_SCISSOR_RECT scissor;
    scissor.width  = aWidth;
    scissor.height = aHeight;
    m_device.SetScissor( scissor );

    if( m_compositorPipeline )
        m_device.BindPipeline( m_compositorPipeline );

    if( m_quadVB )
        m_device.BindVertexBuffer( 0, m_quadVB );

    for( const auto& layer : aLayers )
    {
        if( layer.fbo )
            compositeLayer( layer );
    }

    if( aDrawOnTop )
        aDrawOnTop();

    m_device.EndRenderPass();

    return aAccumulationFBO;
}


RHI_BindGroupHandle RHI_COMPOSITOR::bindFBOTexture( RHI_FramebufferHandle aFBO,
                                                    RHI_BindGroupLayoutHandle aLayout,
                                                    uint32_t aGroupIndex )
{
    RHI_TextureHandle tex = m_device.GetFramebufferColorTexture( aFBO, 0 );

    if( !tex )
    {
        wxLogTrace( traceRhi, wxS( "Compositor: missing FBO color texture fbo=%u layout=%u" ),
                    static_cast<unsigned>( aFBO.value ),
                    static_cast<unsigned>( aLayout.value ) );
        return {};
    }

    uint64_t key = ( static_cast<uint64_t>( tex.value ) << 32 ) | aLayout.value;
    auto     it  = m_fboBindGroups.find( key );

    if( it != m_fboBindGroups.end() )
    {
        m_device.BindGroup( aGroupIndex, it->second );
        return it->second;
    }

    RHI_BIND_GROUP_DESC bgDesc;
    bgDesc.layout = aLayout;
    bgDesc.entries.push_back( { 0, {}, tex } );

    RHI_BindGroupHandle bg = m_device.CreateBindGroup( bgDesc );

    if( !bg )
    {
        wxLogTrace( traceRhi, wxS( "Compositor: CreateBindGroup FAILED fbo=%u tex=%u layout=%u" ),
                    static_cast<unsigned>( aFBO.value ),
                    static_cast<unsigned>( tex.value ),
                    static_cast<unsigned>( aLayout.value ) );
        return {};
    }

    m_fboBindGroups.emplace( key, bg );
    m_device.BindGroup( aGroupIndex, bg );

    return bg;
}


void RHI_COMPOSITOR::compositeLayer( const COMPOSITOR_LAYER& aLayer )
{
    RHI_BindGroupHandle bg = bindFBOTexture( aLayer.fbo, m_singleTexLayout, 0 );

    if( !bg )
        return;

    COMPOSITOR_PUSH_CONSTANTS pc;
    pc.alpha = aLayer.alpha;
    pc.uvScale[0] = aLayer.uvScale[0];
    pc.uvScale[1] = aLayer.uvScale[1];
    pc.uvOffset[0] = aLayer.uvOffset[0];
    pc.uvOffset[1] = aLayer.uvOffset[1];

    m_device.SetPushConstants( &pc, sizeof( pc ) );
    m_device.Draw( 6 );
}


void RHI_COMPOSITOR::destroyFBOBindGroups()
{
    for( auto& [key, bg] : m_fboBindGroups )
        m_device.DestroyBindGroup( bg );

    m_fboBindGroups.clear();
}
