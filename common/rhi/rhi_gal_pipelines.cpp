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

#include <rhi/rhi_gal_pipelines.h>
#include <rhi/rhi_device.h>
#include <rhi/rhi_bind_group.h>
#include <rhi/rhi_pipeline.h>
#include <rhi/rhi_vertex.h>
#include <rhi/rhi_glyph_atlas.h>
#include <rhi/rhi_instance.h>

#include <algorithm>
#include <cstdio>

#include <rhi_shaders.h>


RHI_GAL_PIPELINES::RHI_GAL_PIPELINES( RHI_DEVICE& aDevice ) :
    m_device( aDevice )
{
}


RHI_GAL_PIPELINES::~RHI_GAL_PIPELINES()
{
    Destroy();
}


RHI_GAL_PIPELINES::ShaderPair RHI_GAL_PIPELINES::loadShaderPair( const RHI_SHADER_BLOB& aVert,
                                                                 const RHI_SHADER_BLOB& aFrag )
{
    // Vulkan uses SPIR-V bytecode; Metal uses the cross-compiled MSL text
    const bool useSource = m_device.GetBackend() != RHI_BACKEND::VULKAN && aVert.msl && aFrag.msl;

    auto create = [&]( RHI_SHADER_TYPE aType, const RHI_SHADER_BLOB& aBlob )
    {
        RHI_SHADER_DESC desc;
        desc.type      = aType;
        desc.bytecode  = useSource ? reinterpret_cast<const uint8_t*>( aBlob.msl ) : aBlob.spirv;
        desc.sizeBytes = useSource ? aBlob.mslSize : aBlob.spirvSize;
        return m_device.CreateShader( desc );
    };

    return { create( RHI_SHADER_TYPE::VERTEX, aVert ), create( RHI_SHADER_TYPE::FRAGMENT, aFrag ) };
}


RHI_PipelineHandle RHI_GAL_PIPELINES::createInstancedPipeline( const ShaderPair& aShaders,
                                                               bool aNegativeBlend,
                                                               bool aHasInstanceBinding,
                                                               bool aSingleCoverage,
                                                               bool aKnockoutMark,
                                                               bool aSquareAlpha,
                                                               bool aPremultiplied )
{
    if( !aShaders.vert || !aShaders.frag )
        return {};

    RHI_PIPELINE_DESC desc;
    desc.vertexShader   = aShaders.vert;
    desc.fragmentShader = aShaders.frag;

    desc.vertexBuffers.push_back( RhiStandardVertexLayout() );

    if( aHasInstanceBinding )
    {
        desc.vertexBuffers.push_back( RhiInstanceLayout() );
    }

    desc.topology = RHI_PRIMITIVE_TOPOLOGY::TRIANGLE_LIST;

    RHI_BLEND_STATE blend;
    blend.enabled = true;

    if( aKnockoutMark )
    {
        // Colour stays as it is, only depth is written
        blend.srcColor = RHI_BLEND_FACTOR::ZERO;
        blend.dstColor = RHI_BLEND_FACTOR::ONE;
        blend.colorOp  = RHI_BLEND_OP::ADD;
        blend.srcAlpha = RHI_BLEND_FACTOR::ZERO;
        blend.dstAlpha = RHI_BLEND_FACTOR::ONE;
        blend.alphaOp  = RHI_BLEND_OP::ADD;
    }
    else if( aNegativeBlend )
    {
        // Erases the destination under the source like CAIRO_OPERATOR_CLEAR for negative gerber polarity
        blend.srcColor = RHI_BLEND_FACTOR::ZERO;
        blend.dstColor = RHI_BLEND_FACTOR::ONE_MINUS_SRC_ALPHA;
        blend.colorOp  = RHI_BLEND_OP::ADD;
        blend.srcAlpha = RHI_BLEND_FACTOR::ZERO;
        blend.dstAlpha = RHI_BLEND_FACTOR::ONE_MINUS_SRC_ALPHA;
        blend.alphaOp  = RHI_BLEND_OP::ADD;
    }
    else
    {
        blend.srcColor = aPremultiplied ? RHI_BLEND_FACTOR::ONE : RHI_BLEND_FACTOR::SRC_ALPHA;
        blend.dstColor = RHI_BLEND_FACTOR::ONE_MINUS_SRC_ALPHA;
        blend.colorOp  = RHI_BLEND_OP::ADD;
        blend.srcAlpha = aSquareAlpha ? RHI_BLEND_FACTOR::SRC_ALPHA : RHI_BLEND_FACTOR::ONE;
        blend.dstAlpha = RHI_BLEND_FACTOR::ONE_MINUS_SRC_ALPHA;
        blend.alphaOp  = RHI_BLEND_OP::ADD;
    }

    desc.blendStates.push_back( blend );

    // Depth test and write enabled for layer ordering
    desc.depthStencil.depthTestEnable  = true;
    desc.depthStencil.depthWriteEnable = true;
    desc.depthStencil.depthCompareOp    = RHI_COMPARE_OP::LESS_EQUAL;

    // The first fragment of a layer writes the layer's depth, which a second one then fails
    if( aSingleCoverage )
        desc.depthStencil.depthCompareOp = RHI_COMPARE_OP::LESS;

    if( m_frameUBOLayout )
        desc.bindGroupLayouts.push_back( m_frameUBOLayoutDesc );

    // Only the track shaders draw glyphs of the bitmap font
    const bool glyphs = &aShaders == &m_shaderPairs[0];

    if( glyphs )
        desc.bindGroupLayouts.push_back( m_glyphLayoutDesc );

    desc.colorFormat = RHI_TEXTURE_FORMAT::RGBA8;
    desc.renderPassHasDepthStencil = true;

    desc.dynamicStates.push_back( RHI_DYNAMIC_STATE::VIEWPORT );
    desc.dynamicStates.push_back( RHI_DYNAMIC_STATE::SCISSOR );

    RHI_PipelineHandle pipeline = m_device.CreatePipeline( desc );

    if( glyphs && pipeline )
        m_glyphPipelines.push_back( pipeline );

    return pipeline;
}


bool RHI_GAL_PIPELINES::UsesGlyphAtlas( RHI_PipelineHandle aPipeline ) const
{
    return std::find( m_glyphPipelines.begin(), m_glyphPipelines.end(), aPipeline ) != m_glyphPipelines.end();
}


RHI_PipelineHandle RHI_GAL_PIPELINES::GetOverlayVariant( RHI_PipelineHandle aPipeline ) const
{
    for( const auto& [pipeline, variant] : m_overlayVariants )
    {
        if( pipeline == aPipeline )
            return variant;
    }

    return aPipeline;
}


RHI_PipelineHandle RHI_GAL_PIPELINES::GetPremultipliedVariant( RHI_PipelineHandle aPipeline ) const
{
    for( const auto& [pipeline, variant] : m_premultipliedVariants )
    {
        if( pipeline == aPipeline )
            return variant;
    }

    return aPipeline;
}


RHI_PipelineHandle RHI_GAL_PIPELINES::createFullscreenPipeline(
        const ShaderPair& aShaders, bool aBlendEnabled,
        const std::vector<RHI_BIND_GROUP_LAYOUT_DESC>& aBindGroupLayouts,
        bool aPremultipliedAlpha, bool aRenderPassHasDepthStencil, bool aSquareAlpha )
{
    if( !aShaders.vert || !aShaders.frag )
        return {};

    RHI_PIPELINE_DESC desc;
    desc.vertexShader   = aShaders.vert;
    desc.fragmentShader = aShaders.frag;

    // Fullscreen quads use slot 0 only
    desc.vertexBuffers.push_back( RhiStandardVertexLayout() );

    desc.topology = RHI_PRIMITIVE_TOPOLOGY::TRIANGLE_LIST;

    RHI_BLEND_STATE blend;
    blend.enabled = aBlendEnabled;

    if( aBlendEnabled )
    {
        blend.srcColor = aPremultipliedAlpha ? RHI_BLEND_FACTOR::ONE
                                             : RHI_BLEND_FACTOR::SRC_ALPHA;
        blend.dstColor = RHI_BLEND_FACTOR::ONE_MINUS_SRC_ALPHA;
        blend.colorOp  = RHI_BLEND_OP::ADD;
        blend.srcAlpha = aSquareAlpha ? RHI_BLEND_FACTOR::SRC_ALPHA : RHI_BLEND_FACTOR::ONE;
        blend.dstAlpha = RHI_BLEND_FACTOR::ONE_MINUS_SRC_ALPHA;
        blend.alphaOp  = RHI_BLEND_OP::ADD;
    }

    desc.blendStates.push_back( blend );

    desc.depthStencil.depthTestEnable  = false;
    desc.depthStencil.depthWriteEnable = false;

    desc.colorFormat = RHI_TEXTURE_FORMAT::RGBA8;
    desc.renderPassHasDepthStencil = aRenderPassHasDepthStencil;

    for( const auto& bgl : aBindGroupLayouts )
        desc.bindGroupLayouts.push_back( bgl );

    desc.dynamicStates.push_back( RHI_DYNAMIC_STATE::VIEWPORT );
    desc.dynamicStates.push_back( RHI_DYNAMIC_STATE::SCISSOR );

    return m_device.CreatePipeline( desc );
}


bool RHI_GAL_PIPELINES::Initialize()
{
    if( m_initialized )
        return true;

    RHI_BIND_GROUP_LAYOUT_DESC uboLayoutDesc;
    uboLayoutDesc.frequency = RHI_BIND_FREQUENCY::PER_FRAME;
    uboLayoutDesc.entries.push_back(
            { RHI_UBO_BINDING, RHI_BINDING_TYPE::UNIFORM_BUFFER, RHI_SHADER_STAGE::ALL } );

    m_frameUBOLayoutDesc = uboLayoutDesc;
    m_frameUBOLayout = m_device.CreateBindGroupLayout( uboLayoutDesc );

    if( !m_frameUBOLayout )
        return false;

    m_glyphLayoutDesc = {};
    m_glyphLayoutDesc.frequency = RHI_BIND_FREQUENCY::PER_PASS;
    m_glyphLayoutDesc.entries.push_back(
            { 0, RHI_BINDING_TYPE::COMBINED_TEXTURE_SAMPLER, RHI_SHADER_STAGE::FRAGMENT } );
    m_glyphLayoutDesc.entries.push_back(
            { RHI_GLYPH_TABLE_BINDING, RHI_BINDING_TYPE::UNIFORM_BUFFER, RHI_SHADER_STAGE::VERTEX } );

    m_glyphLayout = m_device.CreateBindGroupLayout( m_glyphLayoutDesc );

    if( !m_glyphLayout )
        return false;

    struct SHADER_DEF
    {
        const RHI_SHADER_BLOB& vert;
        const RHI_SHADER_BLOB& frag;
    };

    const SHADER_DEF shaderDefs[] = {
        { k_pcb_track_vert, k_pcb_track_frag },
        { k_pcb_pad_vert, k_pcb_pad_frag },
        { k_pcb_arc_vert, k_pcb_arc_frag },
        { k_pcb_zone_vert, k_pcb_zone_frag },
        { k_sch_wire_vert, k_sch_wire_frag },
        // The grid and the cursor each cover the whole target with the same quad
        { k_cursor_vert, k_grid_frag },
        { k_cursor_vert, k_cursor_frag },
        { k_compositor_vert, k_compositor_frag },
        { k_bitmap_vert, k_bitmap_frag },
        { k_compositor_vert, k_compositor_difference_frag },
    };

    constexpr size_t SHADER_COUNT = std::size( shaderDefs );

    m_shaderPairs.resize( SHADER_COUNT );

    for( size_t i = 0; i < SHADER_COUNT; i++ )
    {
        m_shaderPairs[i] = loadShaderPair( shaderDefs[i].vert, shaderDefs[i].frag );

        if( !m_shaderPairs[i].vert || !m_shaderPairs[i].frag )
        {
            Destroy();
            return false;
        }
    }

    auto addOverlayVariant = [&]( RHI_PipelineHandle aPipeline, RHI_PipelineHandle aVariant )
    {
        if( aPipeline && aVariant )
            m_overlayVariants.emplace_back( aPipeline, aVariant );
        else if( aVariant )
            m_device.DestroyPipeline( aVariant );
    };

    auto addPremultipliedVariant = [&]( RHI_PipelineHandle aPipeline, const ShaderPair& aShaders,
                                        bool aSingleCoverage )
    {
        RHI_PipelineHandle variant = createInstancedPipeline( aShaders, false, true, aSingleCoverage, false, false,
                                                              true );

        if( aPipeline && variant )
            m_premultipliedVariants.emplace_back( aPipeline, variant );
        else if( variant )
            m_device.DestroyPipeline( variant );
    };

    // Overlay content blends its alpha by its alpha as well, see GetOverlayVariant
    auto addOverlayVariants = [&]( RHI_PipelineHandle aBlend, RHI_PipelineHandle aOnce, const ShaderPair& aShaders,
                                   bool aInstanced )
    {
        addOverlayVariant( aBlend, createInstancedPipeline( aShaders, false, aInstanced, false, false, true ) );
        addOverlayVariant( aOnce, createInstancedPipeline( aShaders, false, aInstanced, true, false, true ) );
    };

    // The shaderDefs entry of each RHI_SHAPE
    static constexpr int shapeShaders[KIGFX::RHI_SHAPES] = { 0, 2, 1, 3 };

    for( int shape = 0; shape < KIGFX::RHI_SHAPES; ++shape )
    {
        using enum KIGFX::LAYER_DRAW_MODE;

        const ShaderPair& shaders = m_shaderPairs[shapeShaders[shape]];
        auto&             pipelines = m_shapePipelines[shape];

        // Polygons are pre-tessellated triangles with no instance data, so they must not declare slot 1
        const bool instanced = shape != KIGFX::RHI_SHAPE_POLYGON;

        pipelines[static_cast<int>( BLEND )] = createInstancedPipeline( shaders, false, instanced );
        pipelines[static_cast<int>( NEGATIVE )] = createInstancedPipeline( shaders, true, instanced );
        pipelines[static_cast<int>( ONCE )] = createInstancedPipeline( shaders, false, instanced, true );

        if( instanced )
            pipelines[static_cast<int>( MARK )] = createInstancedPipeline( shaders, false, true, false, true );

        addOverlayVariants( pipelines[static_cast<int>( BLEND )], pipelines[static_cast<int>( ONCE )], shaders,
                            instanced );

        if( instanced )
        {
            addPremultipliedVariant( pipelines[static_cast<int>( BLEND )], shaders, false );
            addPremultipliedVariant( pipelines[static_cast<int>( ONCE )], shaders, true );
        }
    }

    m_wirePipeline = createInstancedPipeline( m_shaderPairs[4] );
    m_wireOncePipeline = createInstancedPipeline( m_shaderPairs[4], false, true, true );
    addOverlayVariants( m_wirePipeline, m_wireOncePipeline, m_shaderPairs[4], true );

    // The grid and cursor read their uniforms from a UBO at set 0
    RHI_BIND_GROUP_LAYOUT_DESC fsUboLayoutDesc;
    fsUboLayoutDesc.frequency = RHI_BIND_FREQUENCY::PER_FRAME;
    fsUboLayoutDesc.entries.push_back(
            { RHI_UBO_BINDING, RHI_BINDING_TYPE::UNIFORM_BUFFER, RHI_SHADER_STAGE::FRAGMENT } );

    m_fullscreenUBOLayout = m_device.CreateBindGroupLayout( fsUboLayoutDesc );

    if( !m_fullscreenUBOLayout )
    {
        fprintf( stderr, "RHI: fullscreen UBO layout creation FAILED\n" );
        Destroy();
        return false;
    }

    m_gridPipeline = createFullscreenPipeline( m_shaderPairs[5], true, { fsUboLayoutDesc },
                                               false, true );

    if( !m_gridPipeline )
        fprintf( stderr, "RHI: grid pipeline creation FAILED\n" );

    m_cursorCompositePipeline = createFullscreenPipeline( m_shaderPairs[6], true, { fsUboLayoutDesc }, false, false );

    // Post-process pipelines read their input texture at set 0
    RHI_BIND_GROUP_LAYOUT_DESC texLayoutDesc;
    texLayoutDesc.frequency = RHI_BIND_FREQUENCY::PER_FRAME;
    texLayoutDesc.entries.push_back(
            { 0, RHI_BINDING_TYPE::COMBINED_TEXTURE_SAMPLER, RHI_SHADER_STAGE::FRAGMENT } );

    m_textureSamplerLayout = m_device.CreateBindGroupLayout( texLayoutDesc );

    if( !m_textureSamplerLayout )
    {
        fprintf( stderr, "RHI: texture sampler layout creation FAILED\n" );
        Destroy();
        return false;
    }

    m_compositorPipeline = createFullscreenPipeline( m_shaderPairs[7], true, { texLayoutDesc },
                                                     true, false );

    m_inlineCompositorPipeline = createFullscreenPipeline( m_shaderPairs[7], true,
                                                           { texLayoutDesc }, true, true );

    if( RHI_PipelineHandle variant = createFullscreenPipeline( m_shaderPairs[7], true, { texLayoutDesc }, true,
                                                               true, true ) )
    {
        if( m_inlineCompositorPipeline )
            m_overlayVariants.emplace_back( m_inlineCompositorPipeline, variant );
        else
            m_device.DestroyPipeline( variant );
    }

    RHI_BIND_GROUP_LAYOUT_DESC differenceLayoutDesc = texLayoutDesc;
    differenceLayoutDesc.entries.push_back(
            { 1, RHI_BINDING_TYPE::COMBINED_TEXTURE_SAMPLER, RHI_SHADER_STAGE::FRAGMENT } );

    m_differenceLayout = m_device.CreateBindGroupLayout( differenceLayoutDesc );

    // Without it diff layers are drawn as ordinary ones
    if( m_differenceLayout )
    {
        m_differencePipeline =
                createFullscreenPipeline( m_shaderPairs[9], false, { differenceLayoutDesc }, false, true );
    }

    if( !m_differencePipeline )
        fprintf( stderr, "RHI: difference pipeline creation FAILED\n" );

    // The bitmap pipeline reads the FrameUBO at set 0 and its texture at set 1, and depth tests to stay
    // in layer order with other geometry
    {
        const ShaderPair& bmpShaders = m_shaderPairs[8];

        if( bmpShaders.vert && bmpShaders.frag )
        {
            RHI_PIPELINE_DESC desc;
            desc.vertexShader   = bmpShaders.vert;
            desc.fragmentShader = bmpShaders.frag;

            desc.vertexBuffers.push_back( RhiStandardVertexLayout() );
            desc.topology = RHI_PRIMITIVE_TOPOLOGY::TRIANGLE_LIST;

            RHI_BLEND_STATE blend;
            blend.enabled  = true;
            blend.srcColor = RHI_BLEND_FACTOR::SRC_ALPHA;
            blend.dstColor = RHI_BLEND_FACTOR::ONE_MINUS_SRC_ALPHA;
            blend.colorOp  = RHI_BLEND_OP::ADD;
            blend.srcAlpha = RHI_BLEND_FACTOR::ONE;
            blend.dstAlpha = RHI_BLEND_FACTOR::ONE_MINUS_SRC_ALPHA;
            blend.alphaOp  = RHI_BLEND_OP::ADD;
            desc.blendStates.push_back( blend );

            desc.depthStencil.depthTestEnable  = true;
            desc.depthStencil.depthWriteEnable = true;
            desc.depthStencil.depthCompareOp   = RHI_COMPARE_OP::LESS_EQUAL;

            desc.bindGroupLayouts.push_back( m_frameUBOLayoutDesc );
            desc.bindGroupLayouts.push_back( texLayoutDesc );

            desc.colorFormat = RHI_TEXTURE_FORMAT::RGBA8;
            desc.renderPassHasDepthStencil = true;

            desc.dynamicStates.push_back( RHI_DYNAMIC_STATE::VIEWPORT );
            desc.dynamicStates.push_back( RHI_DYNAMIC_STATE::SCISSOR );

            m_bitmapPipeline = m_device.CreatePipeline( desc );

            if( !m_bitmapPipeline )
            {
                fprintf( stderr, "RHI: bitmap pipeline creation FAILED\n" );
                Destroy();
                return false;
            }

            desc.blendStates.front().srcAlpha = RHI_BLEND_FACTOR::SRC_ALPHA;
            addOverlayVariant( m_bitmapPipeline, m_device.CreatePipeline( desc ) );
        }
    }

    bool complete = m_wirePipeline && m_wireOncePipeline && m_gridPipeline && m_compositorPipeline
                    && m_inlineCompositorPipeline;

    for( int shape = 0; shape < KIGFX::RHI_SHAPES; ++shape )
    {
        for( int mode = 0; mode < DRAW_MODES; ++mode )
        {
            const bool wanted = shape != KIGFX::RHI_SHAPE_POLYGON
                                || mode != static_cast<int>( KIGFX::LAYER_DRAW_MODE::MARK );

            complete = complete && ( m_shapePipelines[shape][mode] || !wanted );
        }
    }

    if( !complete )
    {
        Destroy();
        return false;
    }

    m_initialized = true;
    return true;
}


void RHI_GAL_PIPELINES::Destroy()
{
    for( auto& pipelines : m_shapePipelines )
    {
        for( RHI_PipelineHandle& pipeline : pipelines )
            m_device.Release( pipeline );
    }

    for( RHI_PipelineHandle* pipeline : { &m_wirePipeline, &m_wireOncePipeline, &m_gridPipeline,
                                          &m_cursorCompositePipeline, &m_compositorPipeline,
                                          &m_inlineCompositorPipeline, &m_differencePipeline, &m_bitmapPipeline } )
    {
        m_device.Release( *pipeline );
    }

    for( auto& [pipeline, variant] : m_overlayVariants )
        m_device.Release( variant );

    for( auto& [pipeline, variant] : m_premultipliedVariants )
        m_device.Release( variant );

    for( ShaderPair& pair : m_shaderPairs )
    {
        m_device.Release( pair.vert );
        m_device.Release( pair.frag );
    }

    for( RHI_BindGroupLayoutHandle* layout : { &m_glyphLayout, &m_fullscreenUBOLayout, &m_textureSamplerLayout,
                                               &m_differenceLayout, &m_frameUBOLayout } )
    {
        m_device.Release( *layout );
    }

    m_overlayVariants.clear();
    m_premultipliedVariants.clear();
    m_shaderPairs.clear();
    m_glyphPipelines.clear();
    m_initialized = false;
}
