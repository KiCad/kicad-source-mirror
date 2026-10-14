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

#include <rhi/rhi_metal_device.h>
#include <rhi/rhi_types.h>
#include <rhi/rhi_handle.h>
#include <rhi/rhi_present_mode.h>

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <QuartzCore/CATransaction.h>

#include <chrono>
#include <condition_variable>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <trace_helpers.h>
#include <wx/log.h>


class RHI_METAL_DEVICE : public RHI_DEVICE
{
public:
    RHI_METAL_DEVICE();
    ~RHI_METAL_DEVICE() override;

    const char*     GetName() const override;
    RHI_BACKEND     GetBackend() const override;
    bool            IsInitialized() const override;

    RHI_BufferHandle ReserveBufferHandle() override;
    bool             RealizeBuffer( RHI_BufferHandle handle, const RHI_BUFFER_DESC& desc ) override;
    void             DestroyBuffer( RHI_BufferHandle handle ) override;
    void             UpdateBuffer( RHI_BufferHandle handle, const void* data,
                                   size_t sizeBytes, size_t offset = 0 ) override;

    RHI_TextureHandle ReserveTextureHandle() override;
    bool              RealizeTexture( RHI_TextureHandle handle, const RHI_TEXTURE_DESC& desc ) override;
    void              DestroyTexture( RHI_TextureHandle handle ) override;
    void              UploadTexture( RHI_TextureHandle handle, const void* data,
                                     size_t sizeBytes ) override;

    RHI_ShaderHandle CreateShader( const RHI_SHADER_DESC& desc ) override;
    void             DestroyShader( RHI_ShaderHandle handle ) override;

    RHI_PipelineHandle CreatePipeline( const RHI_PIPELINE_DESC& desc ) override;
    void               DestroyPipeline( RHI_PipelineHandle handle ) override;

    RHI_BindGroupLayoutHandle CreateBindGroupLayout(
            const RHI_BIND_GROUP_LAYOUT_DESC& desc ) override;
    void DestroyBindGroupLayout( RHI_BindGroupLayoutHandle handle ) override;
    RHI_BindGroupHandle CreateBindGroup( const RHI_BIND_GROUP_DESC& desc ) override;
    void DestroyBindGroup( RHI_BindGroupHandle handle ) override;
    void BindGroup( uint32_t groupIndex, RHI_BindGroupHandle group ) override;

    RHI_RenderPassHandle  CreateRenderPass( const RHI_RENDER_PASS_DESC& desc ) override;
    void                  DestroyRenderPass( RHI_RenderPassHandle handle ) override;
    RHI_FramebufferHandle CreateFramebuffer( const RHI_FRAMEBUFFER_DESC& desc ) override;
    void                  DestroyFramebuffer( RHI_FramebufferHandle handle ) override;
    RHI_TextureHandle     GetFramebufferColorTexture(
                                  RHI_FramebufferHandle handle,
                                  uint32_t attachmentIndex = 0 ) const override;

    bool BeginRenderPass( const RHI_RENDER_PASS_BEGIN_DESC& desc ) override;
    bool EndRenderPass() override;

    void SetViewport( const RHI_VIEWPORT& viewport ) override;
    void SetScissor( const RHI_SCISSOR_RECT& scissor ) override;
    void BindPipeline( RHI_PipelineHandle pipeline ) override;
    void SetPushConstants( const void* data, size_t sizeBytes ) override;
    void BindVertexBuffer( uint32_t slot, RHI_BufferHandle buffer ) override;
    void BindIndexBuffer( RHI_BufferHandle buffer ) override;

    void Draw( uint32_t vertexCount, uint32_t firstVertex = 0 ) override;
    void DrawInstanced( uint32_t indexCount, uint32_t instanceCount,
                        uint32_t firstIndex = 0, int32_t vertexOffset = 0,
                        uint32_t firstInstance = 0 ) override;
    void DrawIndexedInstanced( uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex = 0,
                               int32_t vertexOffset = 0, uint32_t firstInstance = 0 ) override;

    RHI_SurfaceHandle   CreateSurface( const RHI_SURFACE_DESC& desc ) override;
    void                DestroySurface( RHI_SurfaceHandle handle ) override;
    RHI_SwapchainHandle CreateSwapchain( const RHI_SWAPCHAIN_DESC& desc ) override;
    bool                ResizeSwapchain( RHI_SwapchainHandle handle, uint32_t width,
                                         uint32_t height, float dpiScale ) override;
    void                DestroySwapchain( RHI_SwapchainHandle handle ) override;
    RHI_FRAME_OP_RESULT Present( RHI_SwapchainHandle handle, RHI_FramebufferHandle aSrcFBO = {},
                                 const RHI_SCISSOR_RECT* aSourceRect = nullptr ) override;

    bool              ReadbackFramebufferPixels( RHI_FramebufferHandle handle,
                                                  uint32_t attachmentIndex,
                                                  void* buffer, size_t bufferSize ) override;
    bool              CopyFramebufferDepth( RHI_FramebufferHandle aSource, RHI_FramebufferHandle aDestination,
                                            uint32_t aSourceX = 0, uint32_t aSourceY = 0 ) override;
    bool              WaitForPresentSlot( RHI_SwapchainHandle aSwapchain, std::chrono::milliseconds aTimeout,
                                          int aAllowed ) override;
    bool              PresentWaitsForDisplay( RHI_SwapchainHandle aSwapchain ) const override;

    std::optional<std::chrono::steady_clock::time_point>
    LastDisplayRefresh( RHI_SwapchainHandle aSwapchain ) const override;

private:
    bool InitMetal();

    static MTLPixelFormat TextureFormatToMTL( RHI_TEXTURE_FORMAT format );
    static MTLBlendFactor TranslateBlendFactor( RHI_BLEND_FACTOR factor );
    static MTLBlendOperation TranslateBlendOp( RHI_BLEND_OP op );
    static MTLCompareFunction TranslateCompareOp( RHI_COMPARE_OP op );
    static MTLPrimitiveType TranslateTopology( RHI_PRIMITIVE_TOPOLOGY topology );
    static MTLVertexFormat TranslateVertexFormat( RHI_VERTEX_FORMAT fmt );

    void EncodeBindGroup( id<MTLRenderCommandEncoder> encoder, uint32_t groupIndex,
                          RHI_BindGroupHandle group );

    // Clamped bilinear sampler shared by every texture binding
    id<MTLSamplerState> TextureSampler();

    static MTLLoadAction TranslateLoadOp( RHI_LOAD_OP op );
    static MTLStoreAction TranslateStoreOp( RHI_STORE_OP op );

    void ExecuteDeferredReleases() override;
    void FreeNativeResource( const RHI_DEFERRED_RELEASE_ENTRY& entry );

    // Builds on first use the pipeline that resolves a framebuffer onto a swapchain of another size or format
    bool EnsureBlitPipeline( MTLPixelFormat aTargetFormat );

    // Draws aSrcTex, or the part aSourceRect selects, scaled across the whole of aDstTex
    bool BlitTextureToTarget( id<MTLCommandBuffer> aCmdBuf, id<MTLTexture> aSrcTex,
                              id<MTLTexture> aDstTex, const RHI_SCISSOR_RECT* aSourceRect = nullptr );

    struct MetalBufferAllocation
    {
        id<MTLBuffer>    buffer = nil;
        RHI_BUFFER_DESC  desc;

        // Serial of the last committed pass that reads this buffer, see SyncSubmittedPasses()
        uint64_t         lastUseSerial = 0;
    };

    struct MetalTextureAllocation
    {
        id<MTLTexture>    texture = nil;
        RHI_TEXTURE_DESC  desc;
    };

    struct MetalShaderAllocation
    {
        id<MTLFunction>  function = nil;
        id<MTLLibrary>   library  = nil;
        RHI_SHADER_TYPE  type     = RHI_SHADER_TYPE::VERTEX;
    };

    struct MetalPipelineAllocation
    {
        id<MTLRenderPipelineState> pipelineState  = nil;
        id<MTLDepthStencilState>   depthStencil   = nil;
        RHI_PIPELINE_DESC         desc;
        MTLPrimitiveType           primitiveType   = MTLPrimitiveTypeTriangle;
    };

    struct MetalRenderPassAllocation
    {
        RHI_RENDER_PASS_DESC desc;
    };

    struct MetalFramebufferAllocation
    {
        RHI_FRAMEBUFFER_DESC            desc;
        std::vector<id<MTLTexture>>     colorTextures;
        std::vector<RHI_TextureHandle>  colorTextureHandles;
        id<MTLTexture>                  depthStencilTexture = nil;
    };

    struct MetalBindGroupLayoutAllocation
    {
        RHI_BIND_GROUP_LAYOUT_DESC desc;
    };

    struct MetalBindGroupAllocation
    {
        RHI_BIND_GROUP_DESC desc;
    };

    struct MetalSurfaceAllocation
    {
        CAMetalLayer*  layer = nil;
    };

    // Presents committed for a swapchain that have not reached the display yet
    struct PRESENTS_IN_FLIGHT
    {
        std::mutex              mutex;
        std::condition_variable presented;
        int                     count = 0;
        double                  lastPresentedTime = 0.0;    // CACurrentMediaTime of the newest shown
    };

    struct MetalSwapchainAllocation
    {
        std::shared_ptr<PRESENTS_IN_FLIGHT> inFlight = std::make_shared<PRESENTS_IN_FLIGHT>();
        RHI_SurfaceHandle       surface;
        uint32_t                width       = 0;
        uint32_t                height      = 0;
        float                   dpiScale    = 1.0f;
        bool                    displaySync = true;     // Presents wait for the next refresh

        id<CAMetalDrawable>     currentDrawable = nil;
    };

    id<MTLDevice>        m_device       = nil;
    id<MTLCommandQueue>  m_commandQueue = nil;

    // Framebuffer resolve pipeline, built for the swapchain format on first use
    id<MTLRenderPipelineState> m_blitPipeline       = nil;
    id<MTLSamplerState>        m_textureSampler     = nil;
    MTLPixelFormat             m_blitPipelineFormat = MTLPixelFormatInvalid;

    bool                 m_initialized  = false;
    double               m_totalPipelineCreateMs = 0.0;
    std::string          m_deviceName;

    RHI_HANDLE_POOL<RHI_TAG_BUFFER>                                m_bufferPool;
    std::unordered_map<uint32_t, MetalBufferAllocation>             m_buffers;

    RHI_HANDLE_POOL<RHI_TAG_TEXTURE>                               m_texturePool;
    std::unordered_map<uint32_t, MetalTextureAllocation>            m_textures;

    RHI_HANDLE_POOL<RHI_TAG_SHADER>                                m_shaderPool;
    std::unordered_map<uint32_t, MetalShaderAllocation>             m_shaders;

    RHI_HANDLE_POOL<RHI_TAG_PIPELINE>                              m_pipelinePool;
    std::unordered_map<uint32_t, MetalPipelineAllocation>           m_pipelines;

    RHI_HANDLE_POOL<RHI_TAG_RENDER_PASS>                           m_renderPassPool;
    std::unordered_map<uint32_t, MetalRenderPassAllocation>         m_renderPasses;

    RHI_HANDLE_POOL<RHI_TAG_FRAMEBUFFER>                           m_framebufferPool;
    std::unordered_map<uint32_t, MetalFramebufferAllocation>        m_framebuffers;

    RHI_HANDLE_POOL<RHI_TAG_BIND_GROUP_LAYOUT>                     m_bindGroupLayoutPool;
    std::unordered_map<uint32_t, MetalBindGroupLayoutAllocation>    m_bindGroupLayouts;

    RHI_HANDLE_POOL<RHI_TAG_BIND_GROUP>                            m_bindGroupPool;
    std::unordered_map<uint32_t, MetalBindGroupAllocation>          m_bindGroups;

    RHI_HANDLE_POOL<RHI_TAG_SURFACE>                               m_surfacePool;
    std::unordered_map<uint32_t, MetalSurfaceAllocation>            m_surfaces;

    RHI_HANDLE_POOL<RHI_TAG_SWAPCHAIN>                             m_swapchainPool;
    std::unordered_map<uint32_t, MetalSwapchainAllocation>          m_swapchains;

    bool                            m_renderPassActive = false;
    id<MTLCommandBuffer>            m_activeCommandBuffer  = nil;
    id<MTLRenderCommandEncoder>     m_activeEncoder        = nil;

    // Passes are committed without waiting. The queue runs them in order, so waiting on the newest
    // one is enough to know every earlier pass has finished.
    std::mutex                      m_submitMutex;
    id<MTLCommandBuffer>            m_lastCommitted = nil;
    uint64_t                        m_submitSerial  = 0;
    uint64_t                        m_syncedSerial  = 0;

    void MarkBufferInUse( MetalBufferAllocation& aBuffer )
    {
        aBuffer.lastUseSerial = m_submitSerial + 1;
    }

    void SyncSubmittedPasses();

    // Cached at bind time to avoid per-draw hash lookups
    RHI_PipelineHandle  m_boundPipeline;
    MTLPrimitiveType    m_cachedPrimitiveType = MTLPrimitiveTypeTriangle;
    id<MTLBuffer>       m_cachedIndexBuffer   = nil;

    static constexpr int MAX_VB_SLOTS = 8;

    // spirv-cross puts push constants at buffer 0 and every other resource at its binding number, so
    // vertex buffers use a separate high range starting at VB_BASE_INDEX to avoid colliding with them.
    static constexpr uint32_t PUSH_CONSTANT_BUFFER_INDEX = 0;
    static constexpr uint32_t VB_BASE_INDEX              = 16;
};


// Metal enum translation helpers

MTLPixelFormat RHI_METAL_DEVICE::TextureFormatToMTL( RHI_TEXTURE_FORMAT format )
{
    switch( format )
    {
    case RHI_TEXTURE_FORMAT::RGBA8: return MTLPixelFormatRGBA8Unorm;
    }

    return MTLPixelFormatRGBA8Unorm;
}


MTLBlendFactor RHI_METAL_DEVICE::TranslateBlendFactor( RHI_BLEND_FACTOR factor )
{
    switch( factor )
    {
    case RHI_BLEND_FACTOR::ZERO:                return MTLBlendFactorZero;
    case RHI_BLEND_FACTOR::ONE:                 return MTLBlendFactorOne;
    case RHI_BLEND_FACTOR::SRC_ALPHA:           return MTLBlendFactorSourceAlpha;
    case RHI_BLEND_FACTOR::ONE_MINUS_SRC_ALPHA: return MTLBlendFactorOneMinusSourceAlpha;
    }

    return MTLBlendFactorZero;
}


MTLBlendOperation RHI_METAL_DEVICE::TranslateBlendOp( RHI_BLEND_OP op )
{
    switch( op )
    {
    case RHI_BLEND_OP::ADD: return MTLBlendOperationAdd;
    }

    return MTLBlendOperationAdd;
}


MTLCompareFunction RHI_METAL_DEVICE::TranslateCompareOp( RHI_COMPARE_OP op )
{
    switch( op )
    {
    case RHI_COMPARE_OP::LESS:       return MTLCompareFunctionLess;
    case RHI_COMPARE_OP::LESS_EQUAL: return MTLCompareFunctionLessEqual;
    }

    return MTLCompareFunctionAlways;
}


MTLPrimitiveType RHI_METAL_DEVICE::TranslateTopology( RHI_PRIMITIVE_TOPOLOGY topology )
{
    switch( topology )
    {
    case RHI_PRIMITIVE_TOPOLOGY::TRIANGLE_LIST: return MTLPrimitiveTypeTriangle;
    }

    return MTLPrimitiveTypeTriangle;
}


MTLVertexFormat RHI_METAL_DEVICE::TranslateVertexFormat( RHI_VERTEX_FORMAT fmt )
{
    switch( fmt )
    {
    case RHI_VERTEX_FORMAT::FLOAT32X2:    return MTLVertexFormatFloat2;
    case RHI_VERTEX_FORMAT::FLOAT32X4:    return MTLVertexFormatFloat4;
    case RHI_VERTEX_FORMAT::FLOAT16X2:    return MTLVertexFormatHalf2;
    case RHI_VERTEX_FORMAT::FLOAT16X4:    return MTLVertexFormatHalf4;
    case RHI_VERTEX_FORMAT::UINT8X4_NORM: return MTLVertexFormatUChar4Normalized;
    case RHI_VERTEX_FORMAT::UINT16X2:     return MTLVertexFormatUShort2;
    }

    return MTLVertexFormatFloat4;
}


MTLLoadAction RHI_METAL_DEVICE::TranslateLoadOp( RHI_LOAD_OP op )
{
    switch( op )
    {
    case RHI_LOAD_OP::LOAD:      return MTLLoadActionLoad;
    case RHI_LOAD_OP::CLEAR:     return MTLLoadActionClear;
    case RHI_LOAD_OP::DONT_CARE: return MTLLoadActionDontCare;
    }

    return MTLLoadActionDontCare;
}


MTLStoreAction RHI_METAL_DEVICE::TranslateStoreOp( RHI_STORE_OP op )
{
    switch( op )
    {
    case RHI_STORE_OP::STORE:     return MTLStoreActionStore;
    case RHI_STORE_OP::DONT_CARE: return MTLStoreActionDontCare;
    }

    return MTLStoreActionDontCare;
}


// Device lifecycle

RHI_METAL_DEVICE::RHI_METAL_DEVICE()
{
    m_initialized = InitMetal();
}


RHI_METAL_DEVICE::~RHI_METAL_DEVICE()
{
    if( m_activeEncoder )
    {
        [m_activeEncoder endEncoding];
        m_activeEncoder = nil;
    }

    if( m_activeCommandBuffer )
    {
        [m_activeCommandBuffer commit];
        [m_activeCommandBuffer waitUntilCompleted];
        m_activeCommandBuffer = nil;
    }

    m_deferredReleaseQueue.FlushAll(
            [this]( const RHI_DEFERRED_RELEASE_ENTRY& entry )
            {
                FreeNativeResource( entry );
            } );

    // Clearing the tables releases what they hold. A surface's layer can outlive the device, so it
    // lets go of the device first.
    for( auto& [key, surf] : m_surfaces )
        surf.layer.device = nil;

    m_swapchains.clear();
    m_surfaces.clear();
    m_framebuffers.clear();
    m_pipelines.clear();
    m_shaders.clear();
    m_textures.clear();
    m_buffers.clear();

    m_blitPipeline   = nil;
    m_textureSampler = nil;

    m_commandQueue = nil;
    m_device = nil;
}


bool RHI_METAL_DEVICE::InitMetal()
{
    @autoreleasepool
    {
        m_device = MTLCreateSystemDefaultDevice();

        if( !m_device )
            return false;

        m_commandQueue = [m_device newCommandQueue];

        if( !m_commandQueue )
        {
            m_device = nil;
            return false;
        }

        m_deviceName = std::string( "Metal (" ) + [[m_device name] UTF8String] + ")";

        return true;
    }
}


const char* RHI_METAL_DEVICE::GetName() const
{
    return m_deviceName.c_str();
}


RHI_BACKEND RHI_METAL_DEVICE::GetBackend() const
{
    return RHI_BACKEND::METAL;
}


bool RHI_METAL_DEVICE::IsInitialized() const
{
    return m_initialized;
}


// Buffer operations

RHI_BufferHandle RHI_METAL_DEVICE::ReserveBufferHandle()
{
    return m_bufferPool.Allocate();
}


bool RHI_METAL_DEVICE::RealizeBuffer( RHI_BufferHandle handle, const RHI_BUFFER_DESC& desc )
{
    if( !handle )
        return false;

    if( desc.sizeBytes == 0 )
    {
        m_bufferPool.Release( handle );
        return false;
    }

    @autoreleasepool
    {
        // Managed keeps a CPU-visible mirror so UpdateBuffer can write through the contents pointer
        // while Metal synchronises the GPU copy.
        MTLResourceOptions options = desc.mappable ? MTLResourceStorageModeShared
                                                   : MTLResourceStorageModeManaged;

        id<MTLBuffer> mtlBuffer = [m_device newBufferWithLength:desc.sizeBytes
                                                        options:options];

        if( !mtlBuffer )
        {
            m_bufferPool.Release( handle );
            return false;
        }

        uint32_t idx = RhiHandleIndex( handle );

        MetalBufferAllocation alloc;
        alloc.buffer = mtlBuffer;
        alloc.desc   = desc;
        m_buffers[idx] = alloc;

        return true;
    }
}


void RHI_METAL_DEVICE::DestroyBuffer( RHI_BufferHandle handle )
{
    deferDestroy( m_bufferPool, RHI_DEFERRED_RESOURCE_TYPE::BUFFER, handle );
}


void RHI_METAL_DEVICE::UpdateBuffer( RHI_BufferHandle handle, const void* data,
                                      size_t sizeBytes, size_t offset )
{
    if( !m_bufferPool.IsValid( handle ) || !data || sizeBytes == 0 )
        return;

    uint32_t idx = RhiHandleIndex( handle );
    auto it = m_buffers.find( idx );

    if( it == m_buffers.end() )
        return;

    MetalBufferAllocation& alloc = it->second;

    if( !RhiRangeWithinBuffer( offset, sizeBytes, alloc.desc.sizeBytes ) )
        return;

    void* contents = [alloc.buffer contents];

    if( !contents )
        return;

    // The GPU reads buffer contents when a pass executes, not when it was encoded
    if( alloc.lastUseSerial > m_syncedSerial )
        SyncSubmittedPasses();

    std::memcpy( static_cast<uint8_t*>( contents ) + offset, data, sizeBytes );

    if( [alloc.buffer storageMode] == MTLStorageModeManaged )
    {
        [alloc.buffer didModifyRange:NSMakeRange( offset, sizeBytes )];
    }
}


void RHI_METAL_DEVICE::SyncSubmittedPasses()
{
    id<MTLCommandBuffer> pending = nil;
    uint64_t             serial = 0;

    {
        std::lock_guard<std::mutex> lock( m_submitMutex );

        if( m_syncedSerial == m_submitSerial )
            return;

        pending = m_lastCommitted;
        serial = m_submitSerial;
    }

    [pending waitUntilCompleted];

    std::lock_guard<std::mutex> lock( m_submitMutex );

    if( serial > m_syncedSerial )
        m_syncedSerial = serial;

    if( m_syncedSerial == m_submitSerial )
        m_lastCommitted = nil;
}


// Texture operations

RHI_TextureHandle RHI_METAL_DEVICE::ReserveTextureHandle()
{
    return m_texturePool.Allocate();
}


bool RHI_METAL_DEVICE::RealizeTexture( RHI_TextureHandle handle, const RHI_TEXTURE_DESC& desc )
{
    if( !handle )
        return false;

    if( desc.width == 0 || desc.height == 0 )
    {
        m_texturePool.Release( handle );
        return false;
    }

    @autoreleasepool
    {
        MTLTextureDescriptor* texDesc = [MTLTextureDescriptor
                texture2DDescriptorWithPixelFormat:TextureFormatToMTL( desc.format )
                                             width:desc.width
                                            height:desc.height
                                         mipmapped:NO];

        texDesc.usage = MTLTextureUsageShaderRead;
        texDesc.storageMode = MTLStorageModeManaged;

        id<MTLTexture> mtlTex = [m_device newTextureWithDescriptor:texDesc];

        if( !mtlTex )
        {
            m_texturePool.Release( handle );
            return false;
        }

        uint32_t idx = RhiHandleIndex( handle );

        MetalTextureAllocation alloc;
        alloc.texture = mtlTex;
        alloc.desc    = desc;
        m_textures[idx] = alloc;

        return true;
    }
}


void RHI_METAL_DEVICE::DestroyTexture( RHI_TextureHandle handle )
{
    deferDestroy( m_texturePool, RHI_DEFERRED_RESOURCE_TYPE::TEXTURE, handle );
}


void RHI_METAL_DEVICE::UploadTexture( RHI_TextureHandle handle, const void* data,
                                       size_t sizeBytes )
{
    if( !m_texturePool.IsValid( handle ) || !data )
        return;

    uint32_t idx = RhiHandleIndex( handle );
    auto it = m_textures.find( idx );

    if( it == m_textures.end() )
        return;

    MetalTextureAllocation& alloc = it->second;
    uint32_t bpp = RhiTextureBytesPerPixel( alloc.desc.format );
    size_t bytesPerRow = alloc.desc.width * bpp;
    size_t expectedSize = bytesPerRow * alloc.desc.height;

    if( sizeBytes < expectedSize )
        return;

    // replaceRegion writes at once, under any queued pass that still samples this texture
    SyncSubmittedPasses();

    MTLRegion region = MTLRegionMake2D( 0, 0, alloc.desc.width, alloc.desc.height );
    [alloc.texture replaceRegion:region
                     mipmapLevel:0
                       withBytes:data
                     bytesPerRow:bytesPerRow];
}


// Shader operations

RHI_ShaderHandle RHI_METAL_DEVICE::CreateShader( const RHI_SHADER_DESC& desc )
{
    @autoreleasepool
    {
        if( !desc.bytecode || desc.sizeBytes == 0 )
            return {};

        NSString* source = [[NSString alloc] initWithBytes:desc.bytecode
                                                    length:desc.sizeBytes
                                                  encoding:NSUTF8StringEncoding];

        if( !source )
            return {};

        NSError* error = nil;
        id<MTLLibrary> library = [m_device newLibraryWithSource:source
                                                        options:nil
                                                          error:&error];

        if( !library )
            return {};

        // spirv-cross renames the GLSL main, which MSL reserves
        id<MTLFunction> function = [library newFunctionWithName:@"main0"];

        if( !function )
            return {};

        RHI_ShaderHandle handle = m_shaderPool.Allocate();

        if( !handle )
            return {};

        uint32_t idx = RhiHandleIndex( handle );

        MetalShaderAllocation alloc;
        alloc.function = function;
        alloc.library  = library;
        alloc.type     = desc.type;
        m_shaders[idx] = alloc;

        return handle;
    }
}


void RHI_METAL_DEVICE::DestroyShader( RHI_ShaderHandle handle )
{
    deferDestroy( m_shaderPool, RHI_DEFERRED_RESOURCE_TYPE::SHADER, handle );
}


// Pipeline operations

RHI_PipelineHandle RHI_METAL_DEVICE::CreatePipeline( const RHI_PIPELINE_DESC& desc )
{
    auto t0 = std::chrono::steady_clock::now();

    @autoreleasepool
    {
        auto vsIt = m_shaders.find( RhiHandleIndex( desc.vertexShader ) );
        auto fsIt = m_shaders.find( RhiHandleIndex( desc.fragmentShader ) );

        if( vsIt == m_shaders.end() || fsIt == m_shaders.end() )
            return {};

        MTLRenderPipelineDescriptor* pipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
        pipelineDesc.vertexFunction   = vsIt->second.function;
        pipelineDesc.fragmentFunction = fsIt->second.function;

        RHI_TEXTURE_FORMAT colorFmt = desc.colorFormat;

        if( !desc.blendStates.empty() )
        {
            const RHI_BLEND_STATE& blend = desc.blendStates[0];
            MTLRenderPipelineColorAttachmentDescriptor* colorAttach =
                    pipelineDesc.colorAttachments[0];
            colorAttach.pixelFormat = TextureFormatToMTL( colorFmt );
            colorAttach.blendingEnabled = blend.enabled;

            if( blend.enabled )
            {
                colorAttach.sourceRGBBlendFactor      = TranslateBlendFactor( blend.srcColor );
                colorAttach.destinationRGBBlendFactor  = TranslateBlendFactor( blend.dstColor );
                colorAttach.rgbBlendOperation          = TranslateBlendOp( blend.colorOp );
                colorAttach.sourceAlphaBlendFactor     = TranslateBlendFactor( blend.srcAlpha );
                colorAttach.destinationAlphaBlendFactor = TranslateBlendFactor( blend.dstAlpha );
                colorAttach.alphaBlendOperation        = TranslateBlendOp( blend.alphaOp );
            }
        }
        else
        {
            pipelineDesc.colorAttachments[0].pixelFormat = TextureFormatToMTL( colorFmt );
        }

        // A pipeline drawn in a depth-attached pass needs the formats even when it does not test depth
        if( RhiPipelineUsesDepthStencil( desc ) )
        {
            pipelineDesc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
            pipelineDesc.stencilAttachmentPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
        }

        MTLVertexDescriptor* vertexDesc = [[MTLVertexDescriptor alloc] init];

        for( uint32_t bufIdx = 0; bufIdx < desc.vertexBuffers.size(); bufIdx++ )
        {
            // See VB_BASE_INDEX
            uint32_t mtlBufIdx = VB_BASE_INDEX + bufIdx;
            const RHI_VERTEX_BUFFER_LAYOUT_DESC& vbLayout = desc.vertexBuffers[bufIdx];
            vertexDesc.layouts[mtlBufIdx].stride = vbLayout.stride;

            if( vbLayout.stepMode == RHI_VERTEX_STEP_MODE::INSTANCE )
                vertexDesc.layouts[mtlBufIdx].stepFunction = MTLVertexStepFunctionPerInstance;
            else
                vertexDesc.layouts[mtlBufIdx].stepFunction = MTLVertexStepFunctionPerVertex;

            vertexDesc.layouts[mtlBufIdx].stepRate = 1;

            for( const auto& attr : vbLayout.attributes )
            {
                uint32_t loc = attr.location;
                vertexDesc.attributes[loc].format      = TranslateVertexFormat( attr.format );
                vertexDesc.attributes[loc].offset      = attr.offset;
                vertexDesc.attributes[loc].bufferIndex = mtlBufIdx;
            }
        }

        pipelineDesc.vertexDescriptor = vertexDesc;

        NSError* error = nil;
        id<MTLRenderPipelineState> pipelineState =
                [m_device newRenderPipelineStateWithDescriptor:pipelineDesc error:&error];

        if( !pipelineState )
            return {};

        id<MTLDepthStencilState> depthStencilState = nil;

        if( desc.depthStencil.depthTestEnable )
        {
            MTLDepthStencilDescriptor* dsDesc = [[MTLDepthStencilDescriptor alloc] init];
            dsDesc.depthCompareFunction = TranslateCompareOp( desc.depthStencil.depthCompareOp );
            dsDesc.depthWriteEnabled = desc.depthStencil.depthWriteEnable;

            depthStencilState = [m_device newDepthStencilStateWithDescriptor:dsDesc];
        }

        RHI_PipelineHandle handle = m_pipelinePool.Allocate();

        if( !handle )
            return {};

        uint32_t idx = RhiHandleIndex( handle );

        MetalPipelineAllocation alloc;
        alloc.pipelineState    = pipelineState;
        alloc.depthStencil     = depthStencilState;
        alloc.desc             = desc;
        alloc.primitiveType    = TranslateTopology( desc.topology );
        m_pipelines[idx] = alloc;

        auto t1 = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>( t1 - t0 ).count();
        m_totalPipelineCreateMs += ms;

        if( ms > 10.0 )
        {
            wxLogTrace( traceRhi, wxS( "%s: CreatePipeline took %.1fms (total %.1fms)" ),
                        GetName(), ms, m_totalPipelineCreateMs );
        }

        return handle;
    }
}


void RHI_METAL_DEVICE::DestroyPipeline( RHI_PipelineHandle handle )
{
    deferDestroy( m_pipelinePool, RHI_DEFERRED_RESOURCE_TYPE::PIPELINE, handle );
}


// Bind group operations

RHI_BindGroupLayoutHandle RHI_METAL_DEVICE::CreateBindGroupLayout(
        const RHI_BIND_GROUP_LAYOUT_DESC& desc )
{
    if( !desc.Validate() )
        return {};

    RHI_BindGroupLayoutHandle handle = m_bindGroupLayoutPool.Allocate();

    if( !handle )
        return {};

    uint32_t idx = RhiHandleIndex( handle );
    m_bindGroupLayouts.erase( idx );

    MetalBindGroupLayoutAllocation alloc;
    alloc.desc = desc;
    m_bindGroupLayouts[idx] = alloc;

    return handle;
}


void RHI_METAL_DEVICE::DestroyBindGroupLayout( RHI_BindGroupLayoutHandle handle )
{
    deferDestroy( m_bindGroupLayoutPool, RHI_DEFERRED_RESOURCE_TYPE::BIND_GROUP_LAYOUT, handle );
}


RHI_BindGroupHandle RHI_METAL_DEVICE::CreateBindGroup( const RHI_BIND_GROUP_DESC& desc )
{
    if( !m_bindGroupLayoutPool.IsValid( desc.layout ) )
        return {};

    RHI_BindGroupHandle handle = m_bindGroupPool.Allocate();

    if( !handle )
        return {};

    uint32_t idx = RhiHandleIndex( handle );
    m_bindGroups.erase( idx );

    MetalBindGroupAllocation alloc;
    alloc.desc = desc;
    m_bindGroups[idx] = alloc;

    return handle;
}


void RHI_METAL_DEVICE::DestroyBindGroup( RHI_BindGroupHandle handle )
{
    deferDestroy( m_bindGroupPool, RHI_DEFERRED_RESOURCE_TYPE::BIND_GROUP, handle );
}


void RHI_METAL_DEVICE::BindGroup( uint32_t groupIndex, RHI_BindGroupHandle group )
{
    if( !m_renderPassActive || !m_activeEncoder )
        return;

    if( !m_bindGroupPool.IsValid( group ) )
        return;

    EncodeBindGroup( m_activeEncoder, groupIndex, group );
}


void RHI_METAL_DEVICE::EncodeBindGroup( id<MTLRenderCommandEncoder> encoder,
                                         uint32_t groupIndex,
                                         RHI_BindGroupHandle group )
{
    auto bgIt = m_bindGroups.find( RhiHandleIndex( group ) );

    if( bgIt == m_bindGroups.end() )
        return;

    const RHI_BIND_GROUP_DESC& bgDesc = bgIt->second.desc;

    auto layoutIt = m_bindGroupLayouts.find( RhiHandleIndex( bgDesc.layout ) );

    if( layoutIt == m_bindGroupLayouts.end() )
        return;

    const RHI_BIND_GROUP_LAYOUT_DESC& layoutDesc = layoutIt->second.desc;

    // The MSL keeps each resource's binding number as its index, see RHI_UBO_BINDING
    for( const RHI_BIND_GROUP_RESOURCE& resource : bgDesc.entries )
    {
        const RHI_BINDING_ENTRY* layoutEntry = layoutDesc.Find( resource.binding );
        const RHI_BINDING_TYPE   type = layoutEntry ? layoutEntry->type : RHI_BINDING_TYPE::UNIFORM_BUFFER;
        const RHI_SHADER_STAGE   stages = layoutEntry ? layoutEntry->stages : RHI_SHADER_STAGE::ALL;

        const bool     vertex = !!( stages & RHI_SHADER_STAGE::VERTEX );
        const bool     fragment = !!( stages & RHI_SHADER_STAGE::FRAGMENT );
        const uint32_t index = resource.binding;

        if( type == RHI_BINDING_TYPE::UNIFORM_BUFFER )
        {
            auto bufIt = m_buffers.find( RhiHandleIndex( resource.bufferHandle ) );

            if( bufIt == m_buffers.end() )
                continue;

            MarkBufferInUse( bufIt->second );

            if( vertex )
                [encoder setVertexBuffer:bufIt->second.buffer offset:resource.offset atIndex:index];

            if( fragment )
                [encoder setFragmentBuffer:bufIt->second.buffer offset:resource.offset atIndex:index];

            continue;
        }

        auto texIt = m_textures.find( RhiHandleIndex( resource.textureHandle ) );

        if( texIt == m_textures.end() )
            continue;

        // A combined image sampler needs its sampler set as well, or the draw samples through none
        id<MTLSamplerState> sampler = TextureSampler();

        if( vertex )
        {
            [encoder setVertexTexture:texIt->second.texture atIndex:index];
            [encoder setVertexSamplerState:sampler atIndex:index];
        }

        if( fragment )
        {
            [encoder setFragmentTexture:texIt->second.texture atIndex:index];
            [encoder setFragmentSamplerState:sampler atIndex:index];
        }
    }
}


id<MTLSamplerState> RHI_METAL_DEVICE::TextureSampler()
{
    if( m_textureSampler )
        return m_textureSampler;

    MTLSamplerDescriptor* sd = [[MTLSamplerDescriptor alloc] init];
    sd.minFilter    = MTLSamplerMinMagFilterLinear;
    sd.magFilter    = MTLSamplerMinMagFilterLinear;
    sd.sAddressMode = MTLSamplerAddressModeClampToEdge;
    sd.tAddressMode = MTLSamplerAddressModeClampToEdge;

    m_textureSampler = [m_device newSamplerStateWithDescriptor:sd];
    return m_textureSampler;
}


// Render pass and framebuffer operations

RHI_RenderPassHandle RHI_METAL_DEVICE::CreateRenderPass( const RHI_RENDER_PASS_DESC& desc )
{
    RHI_RenderPassHandle handle = m_renderPassPool.Allocate();

    if( !handle )
        return {};

    uint32_t idx = RhiHandleIndex( handle );
    m_renderPasses.erase( idx );

    MetalRenderPassAllocation alloc;
    alloc.desc = desc;
    m_renderPasses[idx] = alloc;

    return handle;
}


void RHI_METAL_DEVICE::DestroyRenderPass( RHI_RenderPassHandle handle )
{
    deferDestroy( m_renderPassPool, RHI_DEFERRED_RESOURCE_TYPE::RENDER_PASS, handle );
}


RHI_FramebufferHandle RHI_METAL_DEVICE::CreateFramebuffer( const RHI_FRAMEBUFFER_DESC& desc )
{
    @autoreleasepool
    {
        if( desc.width == 0 || desc.height == 0 )
            return {};

        MetalFramebufferAllocation alloc;
        alloc.desc = desc;

        // The colour textures registered so far would otherwise hold their pool slots for good
        auto fail = [&]() -> RHI_FramebufferHandle
        {
            for( RHI_TextureHandle texHandle : alloc.colorTextureHandles )
                RhiReleasePooledResource( m_textures, m_texturePool, texHandle.value );

            return {};
        };

        for( const auto& fmt : desc.colorFormats )
        {
            MTLTextureDescriptor* texDesc = [MTLTextureDescriptor
                    texture2DDescriptorWithPixelFormat:TextureFormatToMTL( fmt )
                                                 width:desc.width
                                                height:desc.height
                                             mipmapped:NO];

            texDesc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
            texDesc.storageMode = MTLStorageModePrivate;

            id<MTLTexture> tex = [m_device newTextureWithDescriptor:texDesc];

            if( !tex )
                return fail();

            // The compositor samples these through texture handles
            RHI_TextureHandle texHandle = m_texturePool.Allocate();

            if( !texHandle )
                return fail();

            MetalTextureAllocation texAlloc;
            texAlloc.texture    = tex;
            texAlloc.desc.width  = desc.width;
            texAlloc.desc.height = desc.height;
            texAlloc.desc.format = fmt;
            m_textures[RhiHandleIndex( texHandle )] = texAlloc;

            alloc.colorTextures.push_back( tex );
            alloc.colorTextureHandles.push_back( texHandle );
        }

        if( desc.hasDepthStencil )
        {
            MTLTextureDescriptor* dsDesc = [MTLTextureDescriptor
                    texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float_Stencil8
                                                 width:desc.width
                                                height:desc.height
                                             mipmapped:NO];

            dsDesc.usage = MTLTextureUsageRenderTarget;
            dsDesc.storageMode = MTLStorageModePrivate;

            alloc.depthStencilTexture = [m_device newTextureWithDescriptor:dsDesc];

            if( !alloc.depthStencilTexture )
                return fail();
        }

        RHI_FramebufferHandle handle = m_framebufferPool.Allocate();

        if( !handle )
            return fail();

        m_framebuffers[RhiHandleIndex( handle )] = alloc;
        return handle;
    }
}


void RHI_METAL_DEVICE::DestroyFramebuffer( RHI_FramebufferHandle handle )
{
    deferDestroy( m_framebufferPool, RHI_DEFERRED_RESOURCE_TYPE::FRAMEBUFFER, handle );
}


RHI_TextureHandle RHI_METAL_DEVICE::GetFramebufferColorTexture(
        RHI_FramebufferHandle handle, uint32_t attachmentIndex ) const
{
    if( !m_framebufferPool.IsValid( handle ) )
        return {};

    uint32_t idx = RhiHandleIndex( handle );
    auto it = m_framebuffers.find( idx );

    if( it == m_framebuffers.end() )
        return {};

    if( attachmentIndex >= it->second.colorTextureHandles.size() )
        return {};

    return it->second.colorTextureHandles[attachmentIndex];
}


// Render pass scope and draw commands

bool RHI_METAL_DEVICE::BeginRenderPass( const RHI_RENDER_PASS_BEGIN_DESC& desc )
{
    @autoreleasepool
    {
        if( m_renderPassActive )
            return false;

        if( !m_renderPassPool.IsValid( desc.renderPass ) )
            return false;

        if( !m_framebufferPool.IsValid( desc.framebuffer ) )
            return false;

        auto rpIt = m_renderPasses.find( RhiHandleIndex( desc.renderPass ) );

        if( rpIt == m_renderPasses.end() )
            return false;

        auto fbIt = m_framebuffers.find( RhiHandleIndex( desc.framebuffer ) );

        if( fbIt == m_framebuffers.end() )
            return false;

        const RHI_RENDER_PASS_DESC& rpDesc = rpIt->second.desc;
        const MetalFramebufferAllocation& fb = fbIt->second;

        MTLRenderPassDescriptor* passDesc = [MTLRenderPassDescriptor renderPassDescriptor];

        for( uint32_t i = 0; i < fb.colorTextures.size() && i < rpDesc.colorAttachments.size();
             i++ )
        {
            const RHI_COLOR_ATTACHMENT_DESC& attach = rpDesc.colorAttachments[i];
            passDesc.colorAttachments[i].texture     = fb.colorTextures[i];
            passDesc.colorAttachments[i].loadAction  = TranslateLoadOp( attach.loadOp );
            passDesc.colorAttachments[i].storeAction = TranslateStoreOp( attach.storeOp );

            // The compositor clears to the editor's background colour instead of the pass's static one
            if( desc.hasClearColorOverride && i == 0 )
            {
                passDesc.colorAttachments[i].clearColor = MTLClearColorMake(
                        desc.clearColorOverride[0], desc.clearColorOverride[1],
                        desc.clearColorOverride[2], desc.clearColorOverride[3] );
            }
            else
            {
                passDesc.colorAttachments[i].clearColor = MTLClearColorMake(
                        attach.clearColor[0], attach.clearColor[1],
                        attach.clearColor[2], attach.clearColor[3] );
            }
        }

        if( fb.depthStencilTexture )
        {
            const RHI_DEPTH_STENCIL_ATTACHMENT_DESC& dsAttach = rpDesc.depthStencilAttachment;

            passDesc.depthAttachment.texture     = fb.depthStencilTexture;
            passDesc.depthAttachment.loadAction  = TranslateLoadOp( dsAttach.depthLoadOp );
            passDesc.depthAttachment.storeAction = TranslateStoreOp( dsAttach.depthStoreOp );
            passDesc.depthAttachment.clearDepth  = dsAttach.clearDepth;

            passDesc.stencilAttachment.texture      = fb.depthStencilTexture;
            passDesc.stencilAttachment.loadAction   = TranslateLoadOp( dsAttach.stencilLoadOp );
            passDesc.stencilAttachment.storeAction  = TranslateStoreOp( dsAttach.stencilStoreOp );
            passDesc.stencilAttachment.clearStencil = dsAttach.clearStencil;
        }

        m_activeCommandBuffer = [m_commandQueue commandBuffer];

        if( !m_activeCommandBuffer )
            return false;

        m_activeEncoder = [m_activeCommandBuffer renderCommandEncoderWithDescriptor:passDesc];

        if( !m_activeEncoder )
        {
            m_activeCommandBuffer = nil;
            return false;
        }

        m_renderPassActive = true;
        m_boundPipeline    = {};

        return true;
    }
}


bool RHI_METAL_DEVICE::EndRenderPass()
{
    if( !m_renderPassActive )
        return false;

    [m_activeEncoder endEncoding];
    m_activeEncoder = nil;

    // No wait here. Paths that write memory a queued pass still reads call SyncSubmittedPasses().
    [m_activeCommandBuffer commit];

    {
        std::lock_guard<std::mutex> lock( m_submitMutex );
        m_lastCommitted = m_activeCommandBuffer;
        m_submitSerial++;
    }

    m_activeCommandBuffer = nil;

    m_renderPassActive = false;
    m_boundPipeline = {};

    return true;
}


void RHI_METAL_DEVICE::SetViewport( const RHI_VIEWPORT& viewport )
{
    if( !m_activeEncoder )
        return;

    MTLViewport vp;
    vp.originX = viewport.x;
    vp.width   = viewport.width;
    vp.znear   = viewport.minDepth;
    vp.zfar    = viewport.maxDepth;

    // The RHI projection flips Y, so the viewport is inverted too or Metal's top-left origin would leave
    // screen-space effects upside down relative to the geometry.
    vp.originY = viewport.y + viewport.height;
    vp.height  = -viewport.height;

    [m_activeEncoder setViewport:vp];
}


void RHI_METAL_DEVICE::SetScissor( const RHI_SCISSOR_RECT& scissor )
{
    if( !m_activeEncoder )
        return;

    MTLScissorRect rect;
    rect.x      = std::max( 0, (int)scissor.x );
    rect.y      = std::max( 0, (int)scissor.y );
    rect.width  = scissor.width;
    rect.height = scissor.height;

    [m_activeEncoder setScissorRect:rect];
}


void RHI_METAL_DEVICE::BindPipeline( RHI_PipelineHandle pipeline )
{
    if( !m_activeEncoder )
        return;

    if( !m_pipelinePool.IsValid( pipeline ) )
        return;

    auto it = m_pipelines.find( RhiHandleIndex( pipeline ) );

    if( it == m_pipelines.end() )
        return;

    [m_activeEncoder setRenderPipelineState:it->second.pipelineState];

    // Nil restores the default state so a depth-less pipeline does not inherit the previous pipeline's test
    [m_activeEncoder setDepthStencilState:it->second.depthStencil];

    m_boundPipeline = pipeline;
    m_cachedPrimitiveType = it->second.primitiveType;
}


void RHI_METAL_DEVICE::SetPushConstants( const void* data, size_t sizeBytes )
{
    if( !m_activeEncoder || !data || sizeBytes == 0 )
        return;

    if( sizeBytes > RHI_MAX_PUSH_CONSTANT_BYTES )
        sizeBytes = RHI_MAX_PUSH_CONSTANT_BYTES;

    // Push constants take buffer 0, which no binding uses. setVertexBytes copies, so no staging buffer is needed.
    [m_activeEncoder setVertexBytes:data length:sizeBytes atIndex:PUSH_CONSTANT_BUFFER_INDEX];
    [m_activeEncoder setFragmentBytes:data length:sizeBytes atIndex:PUSH_CONSTANT_BUFFER_INDEX];
}


void RHI_METAL_DEVICE::BindVertexBuffer( uint32_t slot, RHI_BufferHandle buffer )
{
    if( !m_activeEncoder || slot >= MAX_VB_SLOTS )
        return;

    if( !m_bufferPool.IsValid( buffer ) )
        return;

    auto it = m_buffers.find( RhiHandleIndex( buffer ) );

    if( it == m_buffers.end() )
        return;

    MarkBufferInUse( it->second );

    [m_activeEncoder setVertexBuffer:it->second.buffer offset:0 atIndex:VB_BASE_INDEX + slot];
}


void RHI_METAL_DEVICE::BindIndexBuffer( RHI_BufferHandle buffer )
{
    if( !m_bufferPool.IsValid( buffer ) )
        return;

    auto it = m_buffers.find( RhiHandleIndex( buffer ) );

    if( it == m_buffers.end() )
        return;

    MarkBufferInUse( it->second );

    m_cachedIndexBuffer = it->second.buffer;
}


void RHI_METAL_DEVICE::Draw( uint32_t vertexCount, uint32_t firstVertex )
{
    if( !m_activeEncoder || !m_boundPipeline )
        return;

    [m_activeEncoder drawPrimitives:m_cachedPrimitiveType
                        vertexStart:firstVertex
                        vertexCount:vertexCount];
}


void RHI_METAL_DEVICE::DrawInstanced( uint32_t vertexCount, uint32_t instanceCount,
                                       uint32_t firstVertex, int32_t /*vertexOffset*/,
                                       uint32_t firstInstance )
{
    // Callers bind only vertex buffers, never an index buffer, so this is a non-indexed draw to match
    // vkCmdDraw in the Vulkan backend.
    if( !m_activeEncoder || !m_boundPipeline )
        return;

    [m_activeEncoder drawPrimitives:m_cachedPrimitiveType
                        vertexStart:firstVertex
                        vertexCount:vertexCount
                      instanceCount:instanceCount
                       baseInstance:firstInstance];
}


void RHI_METAL_DEVICE::DrawIndexedInstanced( uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
                                              int32_t vertexOffset, uint32_t firstInstance )
{
    if( !m_activeEncoder || !m_boundPipeline || !m_cachedIndexBuffer )
        return;

    [m_activeEncoder drawIndexedPrimitives:m_cachedPrimitiveType
                                indexCount:indexCount
                                 indexType:MTLIndexTypeUInt16
                               indexBuffer:m_cachedIndexBuffer
                         indexBufferOffset:firstIndex * sizeof( uint16_t )
                             instanceCount:instanceCount
                                baseVertex:vertexOffset
                              baseInstance:firstInstance];
}


// Surface and swapchain operations

RHI_SurfaceHandle RHI_METAL_DEVICE::CreateSurface( const RHI_SURFACE_DESC& desc )
{
    @autoreleasepool
    {
        if( !desc.window.IsValid() || desc.window.displayType != RHI_DISPLAY_TYPE::COCOA )
            return {};

        RHI_SurfaceHandle handle = m_surfacePool.Allocate();

        if( !handle )
            return {};

        CAMetalLayer* metalLayer = (__bridge CAMetalLayer*) desc.window.nativeDisplay;
        metalLayer.device = m_device;

        MetalSurfaceAllocation alloc;
        alloc.layer = metalLayer;
        m_surfaces[RhiHandleIndex( handle )] = alloc;

        return handle;
    }
}


void RHI_METAL_DEVICE::DestroySurface( RHI_SurfaceHandle handle )
{
    if( !m_surfacePool.IsValid( handle ) )
        return;

    uint32_t idx = RhiHandleIndex( handle );
    auto it = m_surfaces.find( idx );

    if( it != m_surfaces.end() )
    {
        it->second.layer.device = nil;
        it->second.layer = nil;
        m_surfaces.erase( it );
    }

    m_surfacePool.Release( handle );
}


// Implicit actions are off so the change applies at once from the render thread. The size is already
// in backing pixels, and the auto-resizing host view sizes the frame.
static void sizeLayer( CAMetalLayer* aLayer, uint32_t aWidth, uint32_t aHeight, float aDpiScale )
{
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    aLayer.drawableSize = CGSizeMake( aWidth, aHeight );
    aLayer.contentsScale = aDpiScale;
    [CATransaction commit];
}


RHI_SwapchainHandle RHI_METAL_DEVICE::CreateSwapchain( const RHI_SWAPCHAIN_DESC& desc )
{
    @autoreleasepool
    {
        if( !m_surfacePool.IsValid( desc.surface ) )
            return {};

        auto surfIt = m_surfaces.find( RhiHandleIndex( desc.surface ) );

        if( surfIt == m_surfaces.end() )
            return {};

        MetalSurfaceAllocation& surf = surfIt->second;

        RHI_SwapchainHandle handle = m_swapchainPool.Allocate();

        if( !handle )
            return {};

        MetalSwapchainAllocation alloc;
        alloc.surface  = desc.surface;
        alloc.width    = desc.width;
        alloc.height   = desc.height;
        alloc.dpiScale = desc.dpiScale;

        sizeLayer( surf.layer, desc.width, desc.height, desc.dpiScale );

        // WindowServer composites every window, so a present that skips the refresh cannot tear
        alloc.displaySync = RhiChoosePresentMode( { RHI_PRESENT_MODE::IMMEDIATE }, true )
                            == RHI_PRESENT_MODE::FIFO;
        surf.layer.displaySyncEnabled = alloc.displaySync ? YES : NO;

        static bool s_logged = false;

        if( !s_logged )
        {
            wxLogTrace( traceRhi, wxS( "MetalDevice: display sync %s" ), alloc.displaySync ? "on" : "off" );
            s_logged = true;
        }

        m_swapchains[RhiHandleIndex( handle )] = alloc;
        return handle;
    }
}


bool RHI_METAL_DEVICE::ResizeSwapchain( RHI_SwapchainHandle handle, uint32_t width,
                                         uint32_t height, float dpiScale )
{
    @autoreleasepool
    {
        if( !m_swapchainPool.IsValid( handle ) || width == 0 || height == 0 )
            return false;

        auto it = m_swapchains.find( RhiHandleIndex( handle ) );

        if( it == m_swapchains.end() )
            return false;

        MetalSwapchainAllocation& sc = it->second;
        sc.width    = width;
        sc.height   = height;
        sc.dpiScale = dpiScale;

        auto surfIt = m_surfaces.find( RhiHandleIndex( sc.surface ) );

        if( surfIt == m_surfaces.end() )
            return false;

        sizeLayer( surfIt->second.layer, width, height, dpiScale );

        return true;
    }
}


void RHI_METAL_DEVICE::DestroySwapchain( RHI_SwapchainHandle handle )
{
    if( !m_swapchainPool.IsValid( handle ) )
        return;

    uint32_t idx = RhiHandleIndex( handle );
    auto it = m_swapchains.find( idx );

    if( it != m_swapchains.end() )
    {
        it->second.currentDrawable = nil;
        m_swapchains.erase( it );
    }

    m_swapchainPool.Release( handle );
}


bool RHI_METAL_DEVICE::EnsureBlitPipeline( MTLPixelFormat aTargetFormat )
{
    if( m_blitPipeline && m_blitPipelineFormat == aTargetFormat )
        return true;

    @autoreleasepool
    {
        // Texture (0,0) lands at the top-left of the target with no vertical flip, matching the Vulkan
        // present blit.
        NSString* src =
            @"#include <metal_stdlib>\n"
             "using namespace metal;\n"
             "struct BlitVSOut { float4 position [[position]]; float2 uv; };\n"
             "vertex BlitVSOut kicad_rhi_blit_vs( uint vid [[vertex_id]] ) {\n"
             "    float2 pos = float2( vid == 1 ? 3.0 : -1.0, vid == 2 ? 3.0 : -1.0 );\n"
             "    BlitVSOut out;\n"
             "    out.position = float4( pos, 0.0, 1.0 );\n"
             "    out.uv = float2( ( pos.x + 1.0 ) * 0.5, ( 1.0 - pos.y ) * 0.5 );\n"
             "    return out;\n"
             "}\n"
             "fragment float4 kicad_rhi_blit_fs( BlitVSOut in [[stage_in]],\n"
             "        texture2d<float> tex [[texture(0)]], sampler samp [[sampler(0)]],\n"
             "        constant float4& rect [[buffer(0)]] ) {\n"
             "    return tex.sample( samp, rect.xy + in.uv * rect.zw );\n"
             "}\n";

        NSError*       error = nil;
        id<MTLLibrary> lib   = [m_device newLibraryWithSource:src options:nil error:&error];

        if( !lib )
        {
            wxLogTrace( traceRhi, wxS( "MetalDevice: blit shader compile failed: %s" ),
                        error.localizedDescription.UTF8String );
            return false;
        }

        id<MTLFunction> vs = [lib newFunctionWithName:@"kicad_rhi_blit_vs"];
        id<MTLFunction> fs = [lib newFunctionWithName:@"kicad_rhi_blit_fs"];

        if( !vs || !fs )
            return false;

        MTLRenderPipelineDescriptor* pd = [[MTLRenderPipelineDescriptor alloc] init];
        pd.vertexFunction                  = vs;
        pd.fragmentFunction                = fs;
        pd.colorAttachments[0].pixelFormat = aTargetFormat;

        id<MTLRenderPipelineState> pso =
                [m_device newRenderPipelineStateWithDescriptor:pd error:&error];

        if( !pso )
        {
            wxLogTrace( traceRhi, wxS( "MetalDevice: blit pipeline creation failed: %s" ),
                        error.localizedDescription.UTF8String );
            return false;
        }

        m_blitPipeline       = pso;
        m_blitPipelineFormat = aTargetFormat;
        return true;
    }
}


bool RHI_METAL_DEVICE::BlitTextureToTarget( id<MTLCommandBuffer> aCmdBuf, id<MTLTexture> aSrcTex,
                                            id<MTLTexture> aDstTex, const RHI_SCISSOR_RECT* aSourceRect )
{
    if( !aCmdBuf || !aSrcTex || !aDstTex )
        return false;

    if( !EnsureBlitPipeline( aDstTex.pixelFormat ) )
        return false;

    MTLRenderPassDescriptor* rp = [MTLRenderPassDescriptor renderPassDescriptor];
    rp.colorAttachments[0].texture     = aDstTex;
    rp.colorAttachments[0].loadAction  = MTLLoadActionDontCare;
    rp.colorAttachments[0].storeAction = MTLStoreActionStore;

    id<MTLRenderCommandEncoder> enc = [aCmdBuf renderCommandEncoderWithDescriptor:rp];

    if( !enc )
        return false;

    // Origin and size of the part shown, as a share of the texture
    float rect[4] = { 0.0f, 0.0f, 1.0f, 1.0f };

    if( aSourceRect && aSrcTex.width > 0 && aSrcTex.height > 0 )
    {
        rect[0] = static_cast<float>( aSourceRect->x ) / aSrcTex.width;
        rect[1] = static_cast<float>( aSourceRect->y ) / aSrcTex.height;
        rect[2] = static_cast<float>( aSourceRect->width ) / aSrcTex.width;
        rect[3] = static_cast<float>( aSourceRect->height ) / aSrcTex.height;
    }

    [enc setRenderPipelineState:m_blitPipeline];
    [enc setFragmentTexture:aSrcTex atIndex:0];
    [enc setFragmentSamplerState:TextureSampler() atIndex:0];
    [enc setFragmentBytes:rect length:sizeof( rect ) atIndex:0];
    [enc drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
    [enc endEncoding];
    return true;
}


bool RHI_METAL_DEVICE::PresentWaitsForDisplay( RHI_SwapchainHandle aSwapchain ) const
{
    auto it = m_swapchains.find( RhiHandleIndex( aSwapchain ) );

    return it == m_swapchains.end() || it->second.displaySync;
}


std::optional<std::chrono::steady_clock::time_point>
RHI_METAL_DEVICE::LastDisplayRefresh( RHI_SwapchainHandle aSwapchain ) const
{
    auto it = m_swapchains.find( RhiHandleIndex( aSwapchain ) );

    if( it == m_swapchains.end() )
        return std::nullopt;

    double presented = 0.0;

    {
        std::lock_guard<std::mutex> lock( it->second.inFlight->mutex );
        presented = it->second.inFlight->lastPresentedTime;
    }

    if( presented <= 0.0 )
        return std::nullopt;

    // steady_clock and Core Animation's media time both count mach_absolute_time
    return std::chrono::steady_clock::time_point( std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            std::chrono::duration<double>( presented ) ) );
}


bool RHI_METAL_DEVICE::WaitForPresentSlot( RHI_SwapchainHandle aSwapchain, std::chrono::milliseconds aTimeout,
                                           int aAllowed )
{
    std::shared_ptr<PRESENTS_IN_FLIGHT> inFlight;

    {
        auto it = m_swapchains.find( RhiHandleIndex( aSwapchain ) );

        if( !m_swapchainPool.IsValid( aSwapchain ) || it == m_swapchains.end() )
            return false;

        inFlight = it->second.inFlight;
    }

    // The next frame may start once fewer presents than allowed are still waiting for the display
    const int allowed = aAllowed;

    std::unique_lock<std::mutex> lock( inFlight->mutex );

    if( inFlight->count < allowed )
        return false;

    inFlight->presented.wait_for( lock, aTimeout,
                                  [&]()
                                  {
                                      return inFlight->count < allowed;
                                  } );
    return true;
}


RHI_FRAME_OP_RESULT RHI_METAL_DEVICE::Present( RHI_SwapchainHandle handle, RHI_FramebufferHandle aSrcFBO,
                                               const RHI_SCISSOR_RECT* aSourceRect )
{
    @autoreleasepool
    {
        if( !m_swapchainPool.IsValid( handle ) )
            return RHI_FRAME_OP_RESULT::ERROR;

        auto it = m_swapchains.find( RhiHandleIndex( handle ) );

        if( it == m_swapchains.end() )
            return RHI_FRAME_OP_RESULT::ERROR;

        MetalSwapchainAllocation& sc = it->second;
        auto surfIt = m_surfaces.find( RhiHandleIndex( sc.surface ) );

        if( surfIt == m_surfaces.end() )
            return RHI_FRAME_OP_RESULT::ERROR;

        CAMetalLayer* layer = surfIt->second.layer;

        if( !layer )
            return RHI_FRAME_OP_RESULT::ERROR;

        // The source may be supersampled, so the blit downscales it onto the drawable
        id<MTLTexture> srcTex = nil;

        if( aSrcFBO )
        {
            auto fbIt = m_framebuffers.find( RhiHandleIndex( aSrcFBO ) );

            if( fbIt != m_framebuffers.end() && !fbIt->second.colorTextures.empty() )
                srcTex = fbIt->second.colorTextures[0];
        }

        // Nothing to show yet. Succeeding keeps the render loop from tearing down the swapchain.
        if( !srcTex )
            return RHI_FRAME_OP_RESULT::OK;

        id<CAMetalDrawable> drawable = [layer nextDrawable];

        // nextDrawable returns nil while every drawable is in flight or before layout. A failure keeps the
        // frame unacknowledged so the render thread retries with backoff.
        if( !drawable )
            return RHI_FRAME_OP_RESULT::ERROR;

        id<MTLCommandBuffer> cmdBuf = [m_commandQueue commandBuffer];
        cmdBuf.label = @"present";

        if( !BlitTextureToTarget( cmdBuf, srcTex, drawable.texture, aSourceRect ) )
            return RHI_FRAME_OP_RESULT::ERROR;

        std::shared_ptr<PRESENTS_IN_FLIGHT> inFlight = sc.inFlight;

        {
            std::lock_guard<std::mutex> lock( inFlight->mutex );
            inFlight->count++;
        }

        // A drawable that is never shown still reports, with a presented time of zero
        [drawable addPresentedHandler:^( id<MTLDrawable> aDrawable )
                  {
                      std::lock_guard<std::mutex> lock( inFlight->mutex );

                      if( aDrawable.presentedTime > inFlight->lastPresentedTime )
                          inFlight->lastPresentedTime = aDrawable.presentedTime;

                      inFlight->count--;
                      inFlight->presented.notify_all();
                  }];

        [cmdBuf presentDrawable:drawable];
        [cmdBuf commit];

        return RHI_FRAME_OP_RESULT::OK;
    }
}


// Framebuffer copy and readback

bool RHI_METAL_DEVICE::CopyFramebufferDepth( RHI_FramebufferHandle aSource, RHI_FramebufferHandle aDestination,
                                             uint32_t aSourceX, uint32_t aSourceY )
{
    @autoreleasepool
    {
        if( m_renderPassActive || !m_framebufferPool.IsValid( aSource ) || !m_framebufferPool.IsValid( aDestination ) )
            return false;

        auto srcIt = m_framebuffers.find( RhiHandleIndex( aSource ) );
        auto dstIt = m_framebuffers.find( RhiHandleIndex( aDestination ) );

        if( srcIt == m_framebuffers.end() || dstIt == m_framebuffers.end() )
            return false;

        id<MTLTexture> src = srcIt->second.depthStencilTexture;
        id<MTLTexture> dst = dstIt->second.depthStencilTexture;

        if( !src || !dst || aSourceX + dst.width > src.width || aSourceY + dst.height > src.height )
            return false;

        // Its own command buffer, committed after the passes before it as they are
        id<MTLCommandBuffer>      commandBuffer = [m_commandQueue commandBuffer];
        id<MTLBlitCommandEncoder> blit = [commandBuffer blitCommandEncoder];

        commandBuffer.label = @"depth copy";
        blit.label = @"depth copy";

        [blit copyFromTexture:src
                  sourceSlice:0
                  sourceLevel:0
                 sourceOrigin:MTLOriginMake( aSourceX, aSourceY, 0 )
                   sourceSize:MTLSizeMake( dst.width, dst.height, 1 )
                    toTexture:dst
             destinationSlice:0
             destinationLevel:0
            destinationOrigin:MTLOriginMake( 0, 0, 0 )];
        [blit endEncoding];

        [commandBuffer commit];

        {
            std::lock_guard<std::mutex> lock( m_submitMutex );
            m_lastCommitted = commandBuffer;
            m_submitSerial++;
        }

        return true;
    }
}


bool RHI_METAL_DEVICE::ReadbackFramebufferPixels( RHI_FramebufferHandle handle,
                                                    uint32_t attachmentIndex,
                                                    void* buffer, size_t bufferSize )
{
    // Read back what the queued passes drew, not what was there before them
    SyncSubmittedPasses();

    @autoreleasepool
    {
        if( !m_framebufferPool.IsValid( handle ) || !buffer )
            return false;

        auto it = m_framebuffers.find( RhiHandleIndex( handle ) );

        if( it == m_framebuffers.end() )
            return false;

        const MetalFramebufferAllocation& fb = it->second;

        if( attachmentIndex >= fb.colorTextures.size() )
            return false;

        id<MTLTexture> srcTex = fb.colorTextures[attachmentIndex];

        if( !srcTex )
            return false;

        uint32_t width  = fb.desc.width;
        uint32_t height = fb.desc.height;
        uint32_t bpp    = 4;

        if( fb.desc.colorFormats.size() > attachmentIndex )
            bpp = RhiTextureBytesPerPixel( fb.desc.colorFormats[attachmentIndex] );

        size_t bytesPerRow = width * bpp;
        size_t requiredSize = bytesPerRow * height;

        if( bufferSize < requiredSize )
            return false;

        // Private storage textures need a blit to a managed buffer first
        if( [srcTex storageMode] == MTLStorageModePrivate )
        {
            MTLTextureDescriptor* readDesc = [MTLTextureDescriptor
                    texture2DDescriptorWithPixelFormat:[srcTex pixelFormat]
                                                 width:width
                                                height:height
                                             mipmapped:NO];
            readDesc.storageMode = MTLStorageModeManaged;
            readDesc.usage = MTLTextureUsageShaderRead;

            id<MTLTexture> readbackTex = [m_device newTextureWithDescriptor:readDesc];

            if( !readbackTex )
                return false;

            id<MTLCommandBuffer> cmdBuf = [m_commandQueue commandBuffer];
            id<MTLBlitCommandEncoder> blit = [cmdBuf blitCommandEncoder];

            [blit copyFromTexture:srcTex
                      sourceSlice:0
                      sourceLevel:0
                     sourceOrigin:MTLOriginMake( 0, 0, 0 )
                       sourceSize:MTLSizeMake( width, height, 1 )
                        toTexture:readbackTex
                 destinationSlice:0
                 destinationLevel:0
                destinationOrigin:MTLOriginMake( 0, 0, 0 )];

            [blit synchronizeTexture:readbackTex slice:0 level:0];
            [blit endEncoding];
            [cmdBuf commit];
            [cmdBuf waitUntilCompleted];

            MTLRegion region = MTLRegionMake2D( 0, 0, width, height );
            [readbackTex getBytes:buffer bytesPerRow:bytesPerRow fromRegion:region mipmapLevel:0];
        }
        else
        {
            MTLRegion region = MTLRegionMake2D( 0, 0, width, height );
            [srcTex getBytes:buffer bytesPerRow:bytesPerRow fromRegion:region mipmapLevel:0];
        }

        return true;
    }
}


// Deferred resource release

void RHI_METAL_DEVICE::FreeNativeResource( const RHI_DEFERRED_RELEASE_ENTRY& entry )
{
    uint32_t idx = entry.handleValue & RHI_HANDLE_INDEX_MASK;

    switch( entry.type )
    {
    case RHI_DEFERRED_RESOURCE_TYPE::BUFFER:
        RhiReleasePooledResource( m_buffers, m_bufferPool, entry.handleValue );
        break;

    // A framebuffer's color texture stays alive while its framebuffer holds it
    case RHI_DEFERRED_RESOURCE_TYPE::TEXTURE:
        RhiReleasePooledResource( m_textures, m_texturePool, entry.handleValue );
        break;

    case RHI_DEFERRED_RESOURCE_TYPE::SHADER:
        RhiReleasePooledResource( m_shaders, m_shaderPool, entry.handleValue );
        break;

    case RHI_DEFERRED_RESOURCE_TYPE::PIPELINE:
        RhiReleasePooledResource( m_pipelines, m_pipelinePool, entry.handleValue );
        break;

    case RHI_DEFERRED_RESOURCE_TYPE::BIND_GROUP_LAYOUT:
        RhiReleasePooledResource( m_bindGroupLayouts, m_bindGroupLayoutPool, entry.handleValue );
        break;

    case RHI_DEFERRED_RESOURCE_TYPE::BIND_GROUP:
        RhiReleasePooledResource( m_bindGroups, m_bindGroupPool, entry.handleValue );
        break;

    case RHI_DEFERRED_RESOURCE_TYPE::RENDER_PASS:
        RhiReleasePooledResource( m_renderPasses, m_renderPassPool, entry.handleValue );
        break;

    case RHI_DEFERRED_RESOURCE_TYPE::FRAMEBUFFER:
    {
        auto it = m_framebuffers.find( idx );

        if( it != m_framebuffers.end() )
        {
            for( RHI_TextureHandle texHandle : it->second.colorTextureHandles )
            {
                if( texHandle )
                    RhiReleasePooledResource( m_textures, m_texturePool, texHandle.value );
            }

            m_framebuffers.erase( it );
        }

        m_framebufferPool.Release( RHI_FramebufferHandle{ entry.handleValue } );
        break;
    }
    }
}


void RHI_METAL_DEVICE::ExecuteDeferredReleases()
{
    m_deferredReleaseQueue.ProcessRetired( m_frameCounter,
            [this]( const RHI_DEFERRED_RELEASE_ENTRY& entry )
            {
                FreeNativeResource( entry );
            } );
}


// Factory function

std::unique_ptr<RHI_DEVICE> RhiCreateMetalDevice()
{
    auto device = std::make_unique<RHI_METAL_DEVICE>();

    if( !device->IsInitialized() )
        return nullptr;

    return device;
}
