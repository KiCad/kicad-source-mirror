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

#ifndef RHI_GAL_PIPELINES_H
#define RHI_GAL_PIPELINES_H

#include <rhi/rhi_handle.h>
#include <rhi/rhi_bind_group.h>
#include <rhi/layer_draw_plan.h>

#include <array>

class RHI_DEVICE;
struct RHI_SHADER_BLOB;


/**
 * Owns the shaders, pipelines and bind group layouts for 2D GAL rendering.
 *
 * Instanced pipelines read RHI_VERTEX from slot 0 and RHI_INSTANCE from slot 1 and differ only in
 * their shaders. Fullscreen pipelines such as the compositor read only slot 0.
 */
class RHI_GAL_PIPELINES
{
public:
    explicit RHI_GAL_PIPELINES( RHI_DEVICE& aDevice );
    ~RHI_GAL_PIPELINES();

    RHI_GAL_PIPELINES( const RHI_GAL_PIPELINES& ) = delete;
    RHI_GAL_PIPELINES& operator=( const RHI_GAL_PIPELINES& ) = delete;

    /**
     * Create all shader modules and pipeline objects.
     * @return true if all pipelines were created successfully.
     */
    bool Initialize();

    /**
     * Release all GPU resources. Safe to call multiple times.
     */
    void Destroy();

    bool IsInitialized() const { return m_initialized; }

    /**
     * The pipeline drawing a shape in a draw mode. ONCE fails the depth test for a second fragment
     * at the layer's own depth so a pixel takes the layer once. MARK writes depth but no colour so
     * what is later drawn behind it in the same layer stays open. Polygons have no MARK pipeline.
     */
    RHI_PipelineHandle GetShapePipeline( KIGFX::RHI_SHAPE aShape, KIGFX::LAYER_DRAW_MODE aMode ) const
    {
        return m_shapePipelines[aShape][static_cast<int>( aMode )];
    }

    RHI_PipelineHandle GetWirePipeline() const { return m_wirePipeline; }
    RHI_PipelineHandle GetWireOncePipeline() const { return m_wireOncePipeline; }
    RHI_PipelineHandle GetGridPipeline() const { return m_gridPipeline; }

    /**
     * The cursor drawn straight into the composite, whose target has no depth.
     */
    RHI_PipelineHandle GetCursorCompositePipeline() const { return m_cursorCompositePipeline; }
    RHI_PipelineHandle GetCompositorPipeline() const { return m_compositorPipeline; }
    RHI_PipelineHandle GetInlineCompositorPipeline() const { return m_inlineCompositorPipeline; }

    /**
     * Replaces a target's colour with its difference from a diff layer, see LAYER_RANGE::difference.
     * It takes GetDifferenceLayout() at set 0 with the layer at binding 0 and a copy of the target
     * at binding 1, and does not blend.
     */
    RHI_PipelineHandle        GetDifferencePipeline() const { return m_differencePipeline; }
    RHI_BindGroupLayoutHandle GetDifferenceLayout() const { return m_differenceLayout; }

    RHI_PipelineHandle GetBitmapPipeline() const { return m_bitmapPipeline; }

    /**
     * The variant of a pipeline for content drawn into the overlay target.
     *
     * The legacy canvas blends alpha by alpha on its transparent overlay, so an element of alpha a
     * covers what is under the overlay by a*a. The variant blends alpha the same way. A pipeline
     * whose alpha blending would not differ, such as a knockout or mark, is its own variant.
     */
    RHI_PipelineHandle GetOverlayVariant( RHI_PipelineHandle aPipeline ) const;

    /**
     * The variant of a pipeline for a cached layer that the legacy canvas draws on its overlay.
     *
     * Its elements are flagged RHI_INSTANCE_OVERLAY_ALPHA and leave colour c*a with alpha a*a,
     * which the variant adds over what is below to give c*a + d*(1 - a*a) as the legacy overlay
     * does. A pipeline without such a variant is returned unchanged.
     */
    RHI_PipelineHandle GetPremultipliedVariant( RHI_PipelineHandle aPipeline ) const;

    /**
     * The bind group layout of the font atlas and glyph table the track pipelines read at set 1.
     */
    RHI_BindGroupLayoutHandle GetGlyphLayout() const { return m_glyphLayout; }

    /**
     * True for a pipeline that expects the glyph bind group at set 1.
     */
    bool UsesGlyphAtlas( RHI_PipelineHandle aPipeline ) const;

    RHI_BindGroupLayoutHandle GetFrameUBOLayout() const { return m_frameUBOLayout; }
    RHI_BindGroupLayoutHandle GetFullscreenUBOLayout() const { return m_fullscreenUBOLayout; }
    RHI_BindGroupLayoutHandle GetTextureSamplerLayout() const { return m_textureSamplerLayout; }

private:
    struct ShaderPair
    {
        RHI_ShaderHandle vert;
        RHI_ShaderHandle frag;
    };

    ShaderPair loadShaderPair( const RHI_SHADER_BLOB& aVert, const RHI_SHADER_BLOB& aFrag );

    /**
     * Build a pipeline reading RHI_VERTEX from slot 0 and, with @p aHasInstanceBinding, RHI_INSTANCE
     * from slot 1. The polygon shader reads only slot 0 and must be built without the instance
     * binding, or validation flags every polygon draw for the unbound slot.
     */
    RHI_PipelineHandle createInstancedPipeline( const ShaderPair& aShaders,
                                                bool aNegativeBlend = false,
                                                bool aHasInstanceBinding = true,
                                                bool aSingleCoverage = false,
                                                bool aKnockoutMark = false,
                                                bool aSquareAlpha = false,
                                                bool aPremultiplied = false );
    RHI_PipelineHandle createFullscreenPipeline(
        const ShaderPair& aShaders, bool aBlendEnabled,
        const std::vector<RHI_BIND_GROUP_LAYOUT_DESC>& aBindGroupLayouts = {},
        bool aPremultipliedAlpha = false,
        bool aRenderPassHasDepthStencil = false,
        bool aSquareAlpha = false );

    RHI_DEVICE& m_device;
    bool        m_initialized = false;

    RHI_BindGroupLayoutHandle  m_frameUBOLayout;
    RHI_BIND_GROUP_LAYOUT_DESC m_frameUBOLayoutDesc;

    RHI_BindGroupLayoutHandle  m_glyphLayout;
    RHI_BIND_GROUP_LAYOUT_DESC m_glyphLayoutDesc;
    std::vector<RHI_PipelineHandle> m_glyphPipelines;

    RHI_BindGroupLayoutHandle  m_fullscreenUBOLayout;
    RHI_BindGroupLayoutHandle  m_textureSamplerLayout;
    RHI_BindGroupLayoutHandle  m_differenceLayout;

    static constexpr int DRAW_MODES = static_cast<int>( KIGFX::LAYER_DRAW_MODE::MARK ) + 1;

    std::array<std::array<RHI_PipelineHandle, DRAW_MODES>, KIGFX::RHI_SHAPES> m_shapePipelines;

    RHI_PipelineHandle m_wirePipeline;
    RHI_PipelineHandle m_wireOncePipeline;
    RHI_PipelineHandle m_gridPipeline;
    RHI_PipelineHandle m_cursorCompositePipeline;
    RHI_PipelineHandle m_compositorPipeline;
    RHI_PipelineHandle m_inlineCompositorPipeline;
    RHI_PipelineHandle m_differencePipeline;
    RHI_PipelineHandle m_bitmapPipeline;

    /**
     * Shader modules kept alive for the lifetime of the pipelines.
     */
    std::vector<ShaderPair> m_shaderPairs;

    /**
     * Each pipeline that has an overlay variant, paired with it.
     */
    std::vector<std::pair<RHI_PipelineHandle, RHI_PipelineHandle>> m_overlayVariants;
    std::vector<std::pair<RHI_PipelineHandle, RHI_PipelineHandle>> m_premultipliedVariants;
};


#endif // RHI_GAL_PIPELINES_H
