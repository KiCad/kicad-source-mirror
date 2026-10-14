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

#ifndef RHI_COMPOSITOR_H
#define RHI_COMPOSITOR_H

#include <rhi/rhi_bind_group.h>
#include <rhi/rhi_handle.h>

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <vector>


class RHI_DEVICE;


/**
 * One layer to composite. Output position u samples the layer at u * uvScale + uvOffset, which
 * selects the visible part of a layer drawn larger than the output.
 */
struct COMPOSITOR_LAYER
{
    RHI_FramebufferHandle fbo;
    float                 alpha = 1.0f;
    float                 uvScale[2] = { 1.0f, 1.0f };
    float                 uvOffset[2] = { 0.0f, 0.0f };
};


/**
 * Matches the PushConstants block of the compositor shader.
 */
struct COMPOSITOR_PUSH_CONSTANTS
{
    float alpha = 1.0f;
    float _pad0 = 0.0f;
    float uvScale[2] = { 1.0f, 1.0f };
    float uvOffset[2] = { 0.0f, 0.0f };
};


/**
 * Blends layer FBOs back-to-front into an accumulation FBO, drawing each as a fullscreen quad
 * with its own alpha.
 */
class RHI_COMPOSITOR
{
public:
    RHI_COMPOSITOR( RHI_DEVICE& aDevice );
    ~RHI_COMPOSITOR();

    /**
     * Create the clear pass and fullscreen quad. Must be called before Composite().
     *
     * @param aSingleTexLayout is the layout baked into the fullscreen pipelines. Without one the
     *                         compositor makes its own.
     */
    bool Initialize( RHI_BindGroupLayoutHandle aSingleTexLayout = {} );

    /**
     * Release all GPU resources. Safe to call multiple times.
     */
    void Destroy();

    /**
     * Set the pipeline that draws each layer as an alpha-blended fullscreen quad.
     */
    void SetCompositorPipeline( RHI_PipelineHandle aPipeline );

    /**
     * Blend aLayers back-to-front into aAccumulationFBO, which is cleared to aBackgroundColor
     * first.
     *
     * @param aDrawOnTop draws after the layers in the same pass with its own pipelines, which is
     *                   how the cursor is drawn.
     * @return aAccumulationFBO, or a null handle when nothing could be drawn.
     */
    RHI_FramebufferHandle Composite( RHI_FramebufferHandle aAccumulationFBO, uint32_t aWidth, uint32_t aHeight,
                                     const std::vector<COMPOSITOR_LAYER>& aLayers,
                                     const float* aBackgroundColor = nullptr,
                                     const std::function<void()>& aDrawOnTop = {} );

    RHI_BindGroupLayoutHandle GetSingleTexLayout() const { return m_singleTexLayout; }
    RHI_BufferHandle          GetQuadVB() const { return m_quadVB; }

    bool IsInitialized() const { return m_initialized; }

    /**
     * Drop the cached bind groups over layer FBO textures. Call before destroying layer FBOs the
     * compositor has sampled so stale descriptors do not accumulate across resizes.
     */
    void InvalidateCachedBindGroups() { destroyFBOBindGroups(); }

private:
    void compositeLayer( const COMPOSITOR_LAYER& aLayer );

    RHI_BindGroupHandle bindFBOTexture( RHI_FramebufferHandle aFBO,
                                        RHI_BindGroupLayoutHandle aLayout,
                                        uint32_t aGroupIndex );

    RHI_DEVICE& m_device;

    RHI_RenderPassHandle      m_clearRenderPass;
    RHI_BufferHandle          m_quadVB;
    RHI_BindGroupLayoutHandle m_singleTexLayout;
    bool                      m_ownsSingleTexLayout = true;

    RHI_PipelineHandle m_compositorPipeline;

    bool m_initialized = false;

    /**
     * Bind groups over FBO color textures, keyed by texture and layout handles. A recreated FBO
     * has a new texture handle, so stale entries are never hit and only wait for the next clear.
     */
    std::unordered_map<uint64_t, RHI_BindGroupHandle> m_fboBindGroups;

    void destroyFBOBindGroups();
};


#endif // RHI_COMPOSITOR_H
