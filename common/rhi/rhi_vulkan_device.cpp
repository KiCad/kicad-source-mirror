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

#include <rhi/rhi_vulkan_device.h>
#include <rhi/rhi_present_mode.h>

#include <wx/dynlib.h>

// Each window system is compiled in only when the build found its development files
#ifdef RHI_VULKAN_HAS_X11
#define VK_USE_PLATFORM_XLIB_KHR
#define VK_USE_PLATFORM_XCB_KHR
#include <X11/Xlib.h>
#include <xcb/xcb.h>
#endif

#ifdef RHI_VULKAN_HAS_WAYLAND
#define VK_USE_PLATFORM_WAYLAND_KHR
#include <wayland-client.h>
#endif

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <vulkan/vulkan.h>
#include <trace_helpers.h>
#include <core/profile.h>

#include <array>
#include <cstdio>
#include <cstdlib>

// Windows headers pulled in by VK_USE_PLATFORM_WIN32_KHR re-define ERROR,
// which collides with RHI_FRAME_OP_RESULT::ERROR in rhi_device.h.
#undef ERROR

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include <vk_mem_alloc.h>

#include <wx/log.h>

#include <magic_enum.hpp>

#include <present_blit_vert.h>
#include <present_blit_frag.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <deque>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>


static const char* VkResultToString( VkResult aResult )
{
    switch( aResult )
    {
    case VK_SUCCESS:                 return "VK_SUCCESS";
    case VK_NOT_READY:               return "VK_NOT_READY";
    case VK_TIMEOUT:                 return "VK_TIMEOUT";
    case VK_EVENT_SET:               return "VK_EVENT_SET";
    case VK_EVENT_RESET:             return "VK_EVENT_RESET";
    case VK_INCOMPLETE:              return "VK_INCOMPLETE";
    case VK_ERROR_OUT_OF_HOST_MEMORY:return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED:return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_DEVICE_LOST:       return "VK_ERROR_DEVICE_LOST";
    case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT:return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT:return "VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER:return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_SURFACE_LOST_KHR:  return "VK_ERROR_SURFACE_LOST_KHR";
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
    case VK_SUBOPTIMAL_KHR:          return "VK_SUBOPTIMAL_KHR";
    case VK_ERROR_OUT_OF_DATE_KHR:   return "VK_ERROR_OUT_OF_DATE_KHR";
    default:                         return "VK_RESULT_UNKNOWN";
    }
}


// Whole milliseconds a timer has run, as the device traces print them
static long long VkElapsedMs( PROF_TIMER& aTimer )
{
    return static_cast<long long>( aTimer.msecs() );
}

#ifdef VK_USE_PLATFORM_XLIB_KHR
// The render thread's own connection to the X server, through xcb or Xlib as the surface was made
struct X11_CONNECTION_OWNER
{
    xcb_connection_t* xcb    = nullptr;
    Display*          xlib   = nullptr;
    int               screen = 0;

    // True when a compositing manager owns this screen's _NET_WM_CM_Sn selection
    bool HasCompositor() const
    {
        const std::string selection = "_NET_WM_CM_S" + std::to_string( screen );

        if( xcb )
        {
            xcb_intern_atom_reply_t* atomReply = xcb_intern_atom_reply(
                    xcb, xcb_intern_atom( xcb, 1, selection.size(), selection.c_str() ), nullptr );

            if( !atomReply )
                return false;

            const xcb_atom_t atom = atomReply->atom;
            free( atomReply );

            if( atom == XCB_ATOM_NONE )
                return false;

            xcb_get_selection_owner_reply_t* ownerReply =
                    xcb_get_selection_owner_reply( xcb, xcb_get_selection_owner( xcb, atom ), nullptr );

            if( !ownerReply )
                return false;

            const bool owned = ownerReply->owner != XCB_WINDOW_NONE;
            free( ownerReply );
            return owned;
        }

        if( xlib )
        {
            const Atom atom = XInternAtom( xlib, selection.c_str(), True );
            return atom != None && XGetSelectionOwner( xlib, atom ) != None;
        }

        return false;
    }

    void Close()
    {
        if( xcb )
            xcb_disconnect( xcb );

        if( xlib )
            XCloseDisplay( xlib );

        xcb  = nullptr;
        xlib = nullptr;
    }
};
#endif

static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT aSeverity,
        VkDebugUtilsMessageTypeFlagsEXT aType,
        const VkDebugUtilsMessengerCallbackDataEXT* aCallbackData,
        void* aUserData );


// Dynamically loaded Vulkan function pointers
struct VulkanFunctions
{
    // Loader / instance level
    PFN_vkGetInstanceProcAddr                    GetInstanceProcAddr = nullptr;
    PFN_vkCreateInstance                         CreateInstance = nullptr;
    PFN_vkEnumerateInstanceExtensionProperties   EnumerateInstanceExtensionProperties = nullptr;
    PFN_vkEnumerateInstanceLayerProperties       EnumerateInstanceLayerProperties = nullptr;
    PFN_vkDestroyInstance                        DestroyInstance = nullptr;
    PFN_vkEnumeratePhysicalDevices               EnumeratePhysicalDevices = nullptr;
    PFN_vkEnumerateDeviceExtensionProperties     EnumerateDeviceExtensionProperties = nullptr;
    PFN_vkGetPhysicalDeviceProperties            GetPhysicalDeviceProperties = nullptr;
    PFN_vkGetPhysicalDeviceFeatures2             GetPhysicalDeviceFeatures2 = nullptr;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties GetPhysicalDeviceQueueFamilyProperties = nullptr;
    PFN_vkCreateDevice                           CreateDevice = nullptr;
    PFN_vkGetDeviceProcAddr                      GetDeviceProcAddr = nullptr;

    // Device level
    PFN_vkDestroyDevice              DestroyDevice = nullptr;
    PFN_vkGetDeviceQueue             GetDeviceQueue = nullptr;
    PFN_vkDeviceWaitIdle             DeviceWaitIdle = nullptr;
    PFN_vkCreateImageView            CreateImageView = nullptr;
    PFN_vkDestroyImageView           DestroyImageView = nullptr;
    PFN_vkCreateSampler              CreateSampler = nullptr;
    PFN_vkDestroySampler             DestroySampler = nullptr;
    PFN_vkCreateCommandPool          CreateCommandPool = nullptr;
    PFN_vkDestroyCommandPool         DestroyCommandPool = nullptr;
    PFN_vkAllocateCommandBuffers     AllocateCommandBuffers = nullptr;
    PFN_vkFreeCommandBuffers         FreeCommandBuffers = nullptr;
    PFN_vkBeginCommandBuffer         BeginCommandBuffer = nullptr;
    PFN_vkEndCommandBuffer           EndCommandBuffer = nullptr;
    PFN_vkCmdPipelineBarrier         CmdPipelineBarrier = nullptr;
    PFN_vkCmdCopyBuffer              CmdCopyBuffer = nullptr;
    PFN_vkCmdCopyBufferToImage       CmdCopyBufferToImage = nullptr;
    PFN_vkQueueSubmit                QueueSubmit = nullptr;
    PFN_vkQueueWaitIdle              QueueWaitIdle = nullptr;
    PFN_vkCreateShaderModule         CreateShaderModule = nullptr;
    PFN_vkDestroyShaderModule        DestroyShaderModule = nullptr;
    PFN_vkCreatePipelineLayout       CreatePipelineLayout = nullptr;
    PFN_vkDestroyPipelineLayout      DestroyPipelineLayout = nullptr;
    PFN_vkCreatePipelineCache        CreatePipelineCache = nullptr;
    PFN_vkDestroyPipelineCache       DestroyPipelineCache = nullptr;
    PFN_vkCreateGraphicsPipelines    CreateGraphicsPipelines = nullptr;
    PFN_vkDestroyPipeline            DestroyPipeline = nullptr;
    PFN_vkCreateRenderPass           CreateRenderPass = nullptr;
    PFN_vkDestroyRenderPass          DestroyRenderPass = nullptr;
    PFN_vkCreateFramebuffer          CreateFramebuffer = nullptr;
    PFN_vkDestroyFramebuffer         DestroyFramebuffer = nullptr;
    PFN_vkCmdBeginRenderPass         CmdBeginRenderPass = nullptr;
    PFN_vkCmdEndRenderPass           CmdEndRenderPass = nullptr;
    PFN_vkCmdClearColorImage         CmdClearColorImage = nullptr;
    PFN_vkCmdCopyImage               CmdCopyImage = nullptr;
    PFN_vkCmdCopyImageToBuffer       CmdCopyImageToBuffer = nullptr;
    PFN_vkCmdSetViewport             CmdSetViewport = nullptr;
    PFN_vkCmdSetScissor              CmdSetScissor = nullptr;
    PFN_vkCmdBindPipeline            CmdBindPipeline = nullptr;
    PFN_vkCmdPushConstants           CmdPushConstants = nullptr;
    PFN_vkCmdBindVertexBuffers       CmdBindVertexBuffers = nullptr;
    PFN_vkCmdBindIndexBuffer         CmdBindIndexBuffer = nullptr;
    PFN_vkCmdDraw                    CmdDraw = nullptr;
    PFN_vkCmdDrawIndexed             CmdDrawIndexed = nullptr;
    PFN_vkCmdBindDescriptorSets      CmdBindDescriptorSets = nullptr;
    PFN_vkCreateDescriptorSetLayout  CreateDescriptorSetLayout = nullptr;
    PFN_vkDestroyDescriptorSetLayout DestroyDescriptorSetLayout = nullptr;
    PFN_vkCreateDescriptorPool       CreateDescriptorPool = nullptr;
    PFN_vkDestroyDescriptorPool      DestroyDescriptorPool = nullptr;
    PFN_vkAllocateDescriptorSets     AllocateDescriptorSets = nullptr;
    PFN_vkFreeDescriptorSets         FreeDescriptorSets = nullptr;
    PFN_vkUpdateDescriptorSets       UpdateDescriptorSets = nullptr;
    PFN_vkCreateFence                CreateFence = nullptr;
    PFN_vkDestroyFence               DestroyFence = nullptr;
    PFN_vkWaitForFences              WaitForFences = nullptr;
    PFN_vkResetFences                ResetFences = nullptr;
    PFN_vkAcquireNextImageKHR        AcquireNextImageKHR = nullptr;
    PFN_vkQueuePresentKHR            QueuePresentKHR = nullptr;
    PFN_vkWaitForPresentKHR          WaitForPresentKHR = nullptr;
    PFN_vkCreateSwapchainKHR         CreateSwapchainKHR = nullptr;
    PFN_vkDestroySwapchainKHR        DestroySwapchainKHR = nullptr;
    PFN_vkGetSwapchainImagesKHR      GetSwapchainImagesKHR = nullptr;
    PFN_vkCreateSemaphore            CreateSemaphore = nullptr;
    PFN_vkDestroySemaphore           DestroySemaphore = nullptr;

    // Instance level surface functions
    PFN_vkDestroySurfaceKHR                       DestroySurfaceKHR = nullptr;
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR      GetPhysicalDeviceSurfaceSupportKHR = nullptr;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR GetPhysicalDeviceSurfaceCapabilitiesKHR = nullptr;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR      GetPhysicalDeviceSurfaceFormatsKHR = nullptr;
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR GetPhysicalDeviceSurfacePresentModesKHR = nullptr;

#ifdef VK_USE_PLATFORM_XLIB_KHR
    PFN_vkCreateXlibSurfaceKHR                    CreateXlibSurfaceKHR = nullptr;
#endif

#ifdef VK_USE_PLATFORM_XCB_KHR
    PFN_vkCreateXcbSurfaceKHR                     CreateXcbSurfaceKHR = nullptr;
#endif

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    PFN_vkCreateWaylandSurfaceKHR                 CreateWaylandSurfaceKHR = nullptr;
#endif

#ifdef VK_USE_PLATFORM_WIN32_KHR
    PFN_vkCreateWin32SurfaceKHR                   CreateWin32SurfaceKHR = nullptr;
#endif

    PFN_vkCreateDebugUtilsMessengerEXT            CreateDebugUtilsMessengerEXT = nullptr;
    PFN_vkDestroyDebugUtilsMessengerEXT           DestroyDebugUtilsMessengerEXT = nullptr;
};


// One attachment of a render pass made by RHI_VULKAN_DEVICE::makeRenderPass()
struct VK_ATTACHMENT
{
    VkFormat            format;
    VkAttachmentLoadOp  loadOp;
    VkAttachmentStoreOp storeOp;
    VkImageLayout       initialLayout;
    VkImageLayout       finalLayout;
};


class RHI_VULKAN_DEVICE : public RHI_DEVICE
{
public:
    RHI_VULKAN_DEVICE( bool enableValidation );
    ~RHI_VULKAN_DEVICE() override;

    const char*     GetName() const override;
    RHI_BACKEND     GetBackend() const override;
    uint32_t        MaxTextureSize() const override { return m_maxTextureSize; }
    bool            IsInitialized() const override;

    RHI_BufferHandle  ReserveBufferHandle() override;
    bool              RealizeBuffer( RHI_BufferHandle handle, const RHI_BUFFER_DESC& desc ) override;
    void              DestroyBuffer( RHI_BufferHandle handle ) override;
    void              UpdateBuffer( RHI_BufferHandle handle, const void* data,
                                    size_t sizeBytes, size_t offset = 0 ) override;

    RHI_TextureHandle ReserveTextureHandle() override;
    bool              RealizeTexture( RHI_TextureHandle handle, const RHI_TEXTURE_DESC& desc ) override;
    void              DestroyTexture( RHI_TextureHandle handle ) override;
    void              UploadTexture( RHI_TextureHandle handle, const void* data,
                                     size_t sizeBytes ) override;

    RHI_ShaderHandle  CreateShader( const RHI_SHADER_DESC& desc ) override;
    void              DestroyShader( RHI_ShaderHandle handle ) override;

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
    bool WaitForPresentSlot( RHI_SwapchainHandle aSwapchain, std::chrono::milliseconds aTimeout,
                             int aAllowed ) override;
    RHI_SURFACE_EXTENT_POLL_RESULT PollSurfaceExtent( RHI_SwapchainHandle aSwapchain ) override;
    bool PresentWaitsForDisplay( RHI_SwapchainHandle aSwapchain ) const override;
    bool PresentReplacesQueued( RHI_SwapchainHandle aSwapchain ) const override;
    RHI_FRAME_OP_RESULT Present( RHI_SwapchainHandle handle, RHI_FramebufferHandle aSrcFBO = {},
                                 const RHI_SCISSOR_RECT* aSourceRect = nullptr ) override;

    bool ReadbackFramebufferPixels( RHI_FramebufferHandle handle,
                                    uint32_t attachmentIndex,
                                    void* buffer, size_t bufferSize ) override;
    bool CopyFramebufferDepth( RHI_FramebufferHandle aSource, RHI_FramebufferHandle aDestination,
                               uint32_t aSourceX = 0, uint32_t aSourceY = 0 ) override;

    void BeginUploadBatch() override;
    void EndUploadBatch() override;

private:
    bool InitLoader();
    bool CreateVkInstance( bool enableValidation );
    bool SelectPhysicalDevice();
    bool CreateLogicalDevice();
    void LoadDeviceFunctions();
    bool InitVMA();
    bool InitCommandPool();
    void ReadDeviceProperties();

    void* LoadVulkanFunction( const char* name );
    void* LoadInstanceFunction( const char* name );

    // A single-subpass render pass drawing into aColor and, when given, aDepth
    VkRenderPass makeRenderPass( const std::vector<VK_ATTACHMENT>&  aColor,
                                 const std::optional<VK_ATTACHMENT>& aDepth );
    void* LoadDeviceFunction( const char* name );

    static VkBufferUsageFlags  TranslateBufferUsage( RHI_BUFFER_TYPE type );
    static VkFormat            TranslateTextureFormat( RHI_TEXTURE_FORMAT format );
    static VkFilter            TranslateFilter( RHI_FILTER filter );

    // Every command buffer of the device pool goes through these steps under the device mutex
    bool     allocateCommandBuffer( VkCommandBuffer& aCmd );
    VkResult beginOneTimeCommands( VkCommandBuffer aCmd );
    VkResult submitCommands( VkCommandBuffer aCmd );

    // The batch's command buffer on the thread that owns the batch, otherwise a one-shot buffer under aLock
    VkCommandBuffer beginUploadCommands( std::unique_lock<std::recursive_mutex>& aLock );

    // Returns true once the copy has run and the caller's staging memory is free again. A batch
    // holds that memory until EndUploadBatch().
    bool endUploadCommands( VkCommandBuffer aCmd, const std::unique_lock<std::recursive_mutex>& aLock );

    // Returns true once the commands have finished on the GPU
    bool SubmitOneShot( VkCommandBuffer cmd );
    VkCommandBuffer EnsureUploadCmdBuf();
    bool CurrentThreadOwnsUploadBatch() const;

    struct BufferAllocation
    {
        VkBuffer        buffer     = VK_NULL_HANDLE;
        VmaAllocation   allocation = VK_NULL_HANDLE;
        void*           mappedPtr  = nullptr;
        size_t          size       = 0;
        bool            mappable   = false;

        // Serial of the last submitted pass that reads this buffer, see SyncSubmittedPasses()
        uint64_t        lastUseSerial = 0;
    };

    struct TextureAllocation
    {
        VkImage              image         = VK_NULL_HANDLE;
        VmaAllocation        allocation    = VK_NULL_HANDLE;
        VkImageView          imageView     = VK_NULL_HANDLE;
        VkSampler            sampler       = VK_NULL_HANDLE;
        uint32_t             width         = 0;
        uint32_t             height        = 0;
        RHI_TEXTURE_FORMAT   format        = RHI_TEXTURE_FORMAT::RGBA8;
        bool                 ownedByFBO    = false;
        VkImageLayout        currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    };

    wxDynamicLibrary           m_vulkanLib;
    VkInstance                  m_instance  = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT   m_debugMessenger = VK_NULL_HANDLE;
    VkPhysicalDevice           m_physicalDevice = VK_NULL_HANDLE;
    VkDevice           m_device    = VK_NULL_HANDLE;
    VkQueue            m_graphicsQueue = VK_NULL_HANDLE;
    uint32_t           m_graphicsQueueFamily = UINT32_MAX;
    VmaAllocator       m_allocator      = VK_NULL_HANDLE;
    VkCommandPool      m_commandPool    = VK_NULL_HANDLE;

    // Queues and command pools need external synchronization, and the main and render threads share this device.
    // Recursive because EndUploadBatch() holds it while calling SubmitOneShot().
    std::recursive_mutex m_deviceMutex;

    VkPipelineCache    m_pipelineCache  = VK_NULL_HANDLE;
    double             m_totalPipelineCreateMs = 0.0;

    // Every Vulkan device takes 2D images at least this large
    uint32_t           m_maxTextureSize   = 4096;

    VulkanFunctions    m_vk;
    std::string        m_deviceName;
    bool               m_initialized      = false;

    // Tracked by thread id so main-thread uploads take the locked one-shot path while the render
    // thread is inside its own batch
    std::atomic<std::thread::id> m_uploadBatchOwner{ std::thread::id{} };
    VkCommandBuffer             m_uploadCmdBuf  = VK_NULL_HANDLE;

    // Kept between uploads because mapping fresh pages for the tens of megabytes a rebuilt scene
    // uploads costs more than the copy
    struct StagingBuffer
    {
        VkBuffer      buffer     = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        void*         mapped     = nullptr;
        size_t        size       = 0;
        bool          inUse      = false;
    };

    static constexpr size_t STAGING_MIN_BYTES  = 64 * 1024;
    static constexpr size_t STAGING_POOL_BYTES = 256 * 1024 * 1024;

    std::vector<StagingBuffer> m_stagingPool;
    std::vector<size_t>        m_batchPooledStaging;

    int  AcquireStaging( size_t aSizeBytes );
    void ReleaseStaging( int aIndex );

    // Copy aData into a pooled staging buffer and return its index, or -1. aBuffer receives the buffer.
    int stageData( const void* aData, size_t aSizeBytes, VkBuffer& aBuffer );

    // End upload commands that read a staging buffer, which is released once they are submitted
    void endStagedUpload( VkCommandBuffer aCmd, std::unique_lock<std::recursive_mutex>& aLock, int aStaging );
    void DestroyStagingPool();

    RHI_HANDLE_POOL<RHI_TAG_BUFFER>                   m_bufferPool;
    std::unordered_map<uint32_t, BufferAllocation>     m_buffers;

    RHI_HANDLE_POOL<RHI_TAG_TEXTURE>                   m_texturePool;
    std::unordered_map<uint32_t, TextureAllocation>     m_textures;

    struct ShaderAllocation
    {
        VkShaderModule module = VK_NULL_HANDLE;
    };

    RHI_HANDLE_POOL<RHI_TAG_SHADER>                    m_shaderPool;
    std::unordered_map<uint32_t, ShaderAllocation>      m_shaders;

    struct PipelineAllocation
    {
        VkPipeline                         pipeline   = VK_NULL_HANDLE;
        VkPipelineLayout                   layout     = VK_NULL_HANDLE;
        VkRenderPass                       renderPass = VK_NULL_HANDLE;
        std::vector<VkDescriptorSetLayout> dsLayouts;
    };

    RHI_HANDLE_POOL<RHI_TAG_PIPELINE>                   m_pipelinePool;
    std::unordered_map<uint32_t, PipelineAllocation>     m_pipelines;

    struct BindGroupLayoutAllocation
    {
        VkDescriptorSetLayout     layout    = VK_NULL_HANDLE;
        RHI_BIND_GROUP_LAYOUT_DESC desc;
    };

    RHI_HANDLE_POOL<RHI_TAG_BIND_GROUP_LAYOUT>                 m_bindGroupLayoutPool;
    std::unordered_map<uint32_t, BindGroupLayoutAllocation>     m_bindGroupLayouts;

    struct DescriptorPoolEntry
    {
        VkDescriptorPool pool          = VK_NULL_HANDLE;
        uint32_t         allocatedSets = 0;
        uint32_t         maxSets       = 128;
    };

    std::vector<DescriptorPoolEntry> m_descriptorPools;

    VkDescriptorPool AcquireDescriptorPool();
    void             ReleaseDescriptorSet( VkDescriptorPool pool,
                                           VkDescriptorSet dset = VK_NULL_HANDLE );
    void             TransitionTexture( VkCommandBuffer cmd, TextureAllocation& tex,
                                        VkImageLayout newLayout,
                                        VkPipelineStageFlags srcStage,
                                        VkPipelineStageFlags dstStage );

    struct BindGroupAllocation
    {
        VkDescriptorSet           descriptorSet = VK_NULL_HANDLE;
        VkDescriptorPool          pool          = VK_NULL_HANDLE;

        // Pool indices of the buffers this group binds, marked in use when the group is bound
        std::vector<uint32_t>     bufferIndices;
    };

    RHI_HANDLE_POOL<RHI_TAG_BIND_GROUP>                m_bindGroupPool;
    std::unordered_map<uint32_t, BindGroupAllocation>   m_bindGroups;

    struct RenderPassAllocation
    {
        VkRenderPass                        renderPass = VK_NULL_HANDLE;
        std::vector<RHI_COLOR_ATTACHMENT_DESC> colorDescs;
        RHI_DEPTH_STENCIL_ATTACHMENT_DESC   depthStencilDesc;
    };

    RHI_HANDLE_POOL<RHI_TAG_RENDER_PASS>                m_renderPassPool;
    std::unordered_map<uint32_t, RenderPassAllocation>   m_renderPasses;

    struct FramebufferAllocation
    {
        VkFramebuffer                       framebuffer        = VK_NULL_HANDLE;
        VkRenderPass                        internalRenderPass = VK_NULL_HANDLE;
        std::vector<VkImage>                colorImages;
        std::vector<VmaAllocation>          colorAllocations;
        std::vector<VkImageView>            colorViews;
        VkImage                             depthImage         = VK_NULL_HANDLE;
        VmaAllocation                       depthAllocation    = VK_NULL_HANDLE;
        VkImageView                         depthView          = VK_NULL_HANDLE;
        bool                                depthRendered      = false;   // left in attachment layout
        std::vector<RHI_TEXTURE_FORMAT>     colorFormats;
        std::vector<RHI_TextureHandle>      colorTextureHandles;
        uint32_t                            width  = 0;
        uint32_t                            height = 0;
    };

    RHI_HANDLE_POOL<RHI_TAG_FRAMEBUFFER>                 m_framebufferPool;
    std::unordered_map<uint32_t, FramebufferAllocation>   m_framebuffers;

    // Destroy a framebuffer's Vulkan objects and images, leaving its texture handles alone
    void destroyFramebufferObjects( FramebufferAllocation& aFb );
    void destroyTexture( TextureAllocation& aTexture );

    // A single-level 2D image in device memory. Returns false and makes nothing on failure.
    bool createImage2D( VkFormat aFormat, uint32_t aWidth, uint32_t aHeight, VkImageUsageFlags aUsage,
                        bool aDedicated, VkImage& aImage, VmaAllocation& aAllocation );

    // A view of all of a 2D image, or VK_NULL_HANDLE
    VkImageView createView( VkImage aImage, VkFormat aFormat, VkImageAspectFlags aAspect );

    // A sampler clamping to the edge, or VK_NULL_HANDLE
    VkSampler createSampler( VkFilter aFilter );

    // A pass's command buffer, reused when one has come back from the queue
    bool acquirePassCmd( VkCommandBuffer& aCmd );

    // Submit a pass's ended command buffer without waiting. A failed submit returns it for reuse.
    VkResult submitPassCmd( VkCommandBuffer aCmd );

    // The descriptor set layout of a bind group layout, or VK_NULL_HANDLE
    VkDescriptorSetLayout createSetLayout( const RHI_BIND_GROUP_LAYOUT_DESC& aDesc );

    // A graphics pipeline with one viewport, no culling and no multisampling, the rest as given
    VkResult buildPipeline( VkShaderModule aVertex, VkShaderModule aFragment,
                            const VkPipelineVertexInputStateCreateInfo&             aVertexInput,
                            VkPrimitiveTopology                                     aTopology,
                            const std::vector<VkPipelineColorBlendAttachmentState>& aBlend,
                            const VkPipelineDepthStencilStateCreateInfo&            aDepthStencil,
                            const std::vector<VkDynamicState>& aDynamicStates, VkPipelineLayout aLayout,
                            VkRenderPass aRenderPass, VkPipeline& aPipeline );
    void destroyPipeline( PipelineAllocation& aPipeline );

    struct SurfaceAllocation
    {
        VkSurfaceKHR       surface         = VK_NULL_HANDLE;
        bool               wayland         = false;
#ifdef VK_USE_PLATFORM_XLIB_KHR
        X11_CONNECTION_OWNER x11Owner;
#endif
    };

    RHI_HANDLE_POOL<RHI_TAG_SURFACE>                     m_surfacePool;
    std::unordered_map<uint32_t, SurfaceAllocation>       m_surfaces;

    struct SwapchainAllocation
    {
        VkSwapchainKHR              swapchain           = VK_NULL_HANDLE;
        RHI_SurfaceHandle           surface;
        VkFormat                    format              = VK_FORMAT_R8G8B8A8_UNORM;
        VkColorSpaceKHR             colorSpace          = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        VkExtent2D                  extent              = { 0, 0 };
        VkExtent2D                  requestedExtent     = { 0, 0 };
        float                       dpiScale            = 1.0f;
        VkPresentModeKHR            presentMode         = VK_PRESENT_MODE_FIFO_KHR;
        std::vector<VkImage>        images;
        std::vector<VkImageView>    imageViews;
        VkSemaphore                 imageAvailable      = VK_NULL_HANDLE;
        VkFence                     inFlightFence       = VK_NULL_HANDLE;

        // One per image because the fence does not show that a present has stopped waiting on its semaphore
        std::vector<VkSemaphore>    renderFinished;
        uint64_t                    lastPresentId       = 0;    // id of the last present, see WaitForPresentSlot
        uint32_t                    currentImageIndex   = 0;
        VkCommandBuffer             lastPresentCmd      = VK_NULL_HANDLE;
        bool                        needsResize         = false;
        bool                        waitingForSurfaceExtent = false;
        std::chrono::steady_clock::time_point nextSurfaceExtentCheck{};
        VkExtent2D                  lastLoggedExtent    = { 0, 0 };
        uint64_t                    presentAttemptCount = 0;

        // Objects of the present blit
        VkRenderPass                presentRenderPass   = VK_NULL_HANDLE;
        std::vector<VkFramebuffer>  presentFramebuffers;
        VkDescriptorPool            presentDescPool     = VK_NULL_HANDLE;
        VkDescriptorSet             presentDescSet      = VK_NULL_HANDLE;
    };

    RHI_HANDLE_POOL<RHI_TAG_SWAPCHAIN>                    m_swapchainPool;
    std::unordered_map<uint32_t, SwapchainAllocation>      m_swapchains;

    void ExecuteDeferredReleases() override;
    void ExecuteDeferredRelease( const RHI_DEFERRED_RELEASE_ENTRY& aEntry );

    bool FindPresentQueueFamily( VkSurfaceKHR vkSurface, uint32_t& outFamily );
    VkPresentModeKHR ChoosePresentMode( const SurfaceAllocation& aSurface );
    bool CreateSwapchainResources( SwapchainAllocation& swa );
    void DestroySwapchainResources( SwapchainAllocation& swa, bool aKeepSwapchain = false );

    // Destroy everything a swapchain owns, the sync objects included
    void destroySwapchainObjects( SwapchainAllocation& aSwa );

    struct VkSwapchainTry
    {
        VkSwapchainKHR   swapchain     = VK_NULL_HANDLE;
        VkResult         result        = VK_ERROR_INITIALIZATION_FAILED;
        uint32_t         imageCount    = 0;                         // the count first asked for
        VkPresentModeKHR preferredMode = VK_PRESENT_MODE_FIFO_KHR;
        VkPresentModeKHR presentMode   = VK_PRESENT_MODE_FIFO_KHR;  // the mode that succeeded
    };

    // Create the VkSwapchainKHR of a window surface, retrying with FIFO and then with the minimum
    // image count when the driver refuses what was asked for
    VkSwapchainTry createVkSwapchain( const SurfaceAllocation& aSurface, const VkSurfaceCapabilitiesKHR& aCaps,
                                      VkSurfaceFormatKHR aFormat, VkExtent2D aExtent, VkSwapchainKHR aOld,
                                      bool aResize );


    // The present draws the compositor output into the swapchain image instead of using a transfer copy
    // Kept per format until device teardown since another swapchain's submitted frame may still use one
    std::unordered_map<VkFormat, VkPipeline> m_presentBlitPipelines;
    VkPipelineLayout                         m_presentBlitLayout   = VK_NULL_HANDLE;
    VkDescriptorSetLayout                    m_presentBlitDSLayout = VK_NULL_HANDLE;

    // Returns VK_NULL_HANDLE when the pipeline cannot be built
    VkPipeline EnsurePresentBlit( VkFormat aSwapchainFormat );
    void DestroyPresentBlit();

    // Active render pass state
    VkCommandBuffer       m_currentCmd       = VK_NULL_HANDLE;

    // Passes are submitted without waiting, so their command buffers are held here until the queue is
    // next known idle
    std::vector<VkCommandBuffer> m_submittedCmds;
    std::vector<VkCommandBuffer> m_freeCmds;
    uint64_t                     m_submitSerial = 0;
    uint64_t                     m_syncedSerial = 0;

    void MarkBufferInUse( BufferAllocation& aBuffer ) { aBuffer.lastUseSerial = m_submitSerial + 1; }
    void OnQueueIdle();
    void SyncSubmittedPasses();
    bool                  m_renderPassActive = false;
    bool                  m_presentWait      = false;   // presents carry ids that can be waited for
    RHI_PipelineHandle    m_activePipeline;
};


// ---- Dynamic loader ----

void* RHI_VULKAN_DEVICE::LoadVulkanFunction( const char* name )
{
    if( !m_vulkanLib.IsLoaded() )
        return nullptr;

    return m_vulkanLib.RawGetSymbol( wxString::FromAscii( name ) );
}


void* RHI_VULKAN_DEVICE::LoadInstanceFunction( const char* name )
{
    if( m_vk.GetInstanceProcAddr && m_instance )
        return reinterpret_cast<void*>( m_vk.GetInstanceProcAddr( m_instance, name ) );

    return nullptr;
}


void* RHI_VULKAN_DEVICE::LoadDeviceFunction( const char* name )
{
    if( m_vk.GetDeviceProcAddr && m_device )
        return reinterpret_cast<void*>( m_vk.GetDeviceProcAddr( m_device, name ) );

    return nullptr;
}


bool RHI_VULKAN_DEVICE::InitLoader()
{
    // Quiet, as a missing loader only means this canvas is unavailable
    const int flags = wxDL_VERBATIM | wxDL_LAZY | wxDL_QUIET;

#ifdef _WIN32
    m_vulkanLib.Load( wxS( "vulkan-1.dll" ), flags );
#else
    if( !m_vulkanLib.Load( wxS( "libvulkan.so.1" ), flags ) )
        m_vulkanLib.Load( wxS( "libvulkan.so" ), flags );
#endif

    m_vk.GetInstanceProcAddr =
            reinterpret_cast<PFN_vkGetInstanceProcAddr>( LoadVulkanFunction( "vkGetInstanceProcAddr" ) );

    if( !m_vk.GetInstanceProcAddr )
        return false;

    auto loadGlobal = [this]( const char* aName )
    {
        return reinterpret_cast<void*>( m_vk.GetInstanceProcAddr( VK_NULL_HANDLE, aName ) );
    };

    m_vk.CreateInstance = reinterpret_cast<PFN_vkCreateInstance>( loadGlobal( "vkCreateInstance" ) );
    m_vk.EnumerateInstanceLayerProperties = reinterpret_cast<PFN_vkEnumerateInstanceLayerProperties>(
            loadGlobal( "vkEnumerateInstanceLayerProperties" ) );
    m_vk.EnumerateInstanceExtensionProperties = reinterpret_cast<PFN_vkEnumerateInstanceExtensionProperties>(
            loadGlobal( "vkEnumerateInstanceExtensionProperties" ) );

    return m_vk.CreateInstance != nullptr;
}


bool RHI_VULKAN_DEVICE::CreateVkInstance( bool enableValidation )
{
    VkApplicationInfo appInfo{};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "KiCad";
    appInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
    appInfo.pEngineName        = "KiCad RHI";
    appInfo.engineVersion      = VK_MAKE_VERSION( 1, 0, 0 );
    appInfo.apiVersion         = VK_API_VERSION_1_1;

    std::vector<const char*> layers;

    if( enableValidation && m_vk.EnumerateInstanceLayerProperties )
    {
        uint32_t layerCount = 0;
        m_vk.EnumerateInstanceLayerProperties( &layerCount, nullptr );
        std::vector<VkLayerProperties> available( layerCount );
        m_vk.EnumerateInstanceLayerProperties( &layerCount, available.data() );

        for( const auto& layer : available )
        {
            if( std::strcmp( layer.layerName, "VK_LAYER_KHRONOS_validation" ) == 0 )
            {
                layers.push_back( "VK_LAYER_KHRONOS_validation" );
                break;
            }
        }
    }

    if( enableValidation && layers.empty() )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: validation requested but "
                                    "VK_LAYER_KHRONOS_validation not available" ) );
    }

    std::vector<const char*> extensions;
    extensions.push_back( VK_KHR_SURFACE_EXTENSION_NAME );

#ifdef VK_USE_PLATFORM_XLIB_KHR
    extensions.push_back( VK_KHR_XLIB_SURFACE_EXTENSION_NAME );
#endif

#ifdef VK_USE_PLATFORM_XCB_KHR
    extensions.push_back( VK_KHR_XCB_SURFACE_EXTENSION_NAME );
#endif

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    extensions.push_back( VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME );
#endif

#ifdef VK_USE_PLATFORM_WIN32_KHR
    extensions.push_back( VK_KHR_WIN32_SURFACE_EXTENSION_NAME );
#endif

    bool debugUtilsAvailable = false;

    if( enableValidation && m_vk.EnumerateInstanceExtensionProperties )
    {
        uint32_t extCount = 0;
        m_vk.EnumerateInstanceExtensionProperties( nullptr, &extCount, nullptr );
        std::vector<VkExtensionProperties> availExts( extCount );
        m_vk.EnumerateInstanceExtensionProperties( nullptr, &extCount, availExts.data() );

        for( const auto& ext : availExts )
        {
            if( std::strcmp( ext.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME ) == 0 )
            {
                extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
                debugUtilsAvailable = true;
                break;
            }
        }
    }

    if( enableValidation && !debugUtilsAvailable )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: validation requested but "
                                    "VK_EXT_debug_utils not available" ) );
    }

    for( const auto& ext : extensions )
        wxLogTrace( traceRhi, wxS( "VulkanDevice: requesting instance extension %s" ), ext );

    for( const auto& layer : layers )
        wxLogTrace( traceRhi, wxS( "VulkanDevice: requesting layer %s" ), layer );

    VkInstanceCreateInfo createInfo{};
    createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo        = &appInfo;
    createInfo.enabledLayerCount       = static_cast<uint32_t>( layers.size() );
    createInfo.ppEnabledLayerNames     = layers.empty() ? nullptr : layers.data();
    createInfo.enabledExtensionCount   = static_cast<uint32_t>( extensions.size() );
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkResult result = m_vk.CreateInstance( &createInfo, nullptr, &m_instance );

    if( result != VK_SUCCESS )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: vkCreateInstance failed (%d)" ),
                    static_cast<int>( result ) );
        return false;
    }

#define LOAD_INST_FN( fn ) \
    m_vk.fn = reinterpret_cast<PFN_vk##fn>( LoadInstanceFunction( "vk" #fn ) )

    LOAD_INST_FN( DestroyInstance );
    LOAD_INST_FN( EnumeratePhysicalDevices );
    LOAD_INST_FN( EnumerateDeviceExtensionProperties );
    LOAD_INST_FN( GetPhysicalDeviceProperties );
    LOAD_INST_FN( GetPhysicalDeviceFeatures2 );
    LOAD_INST_FN( GetPhysicalDeviceQueueFamilyProperties );
    LOAD_INST_FN( CreateDevice );
    LOAD_INST_FN( GetDeviceProcAddr );
    LOAD_INST_FN( DestroySurfaceKHR );
    LOAD_INST_FN( GetPhysicalDeviceSurfaceSupportKHR );
    LOAD_INST_FN( GetPhysicalDeviceSurfaceCapabilitiesKHR );
    LOAD_INST_FN( GetPhysicalDeviceSurfaceFormatsKHR );
    LOAD_INST_FN( GetPhysicalDeviceSurfacePresentModesKHR );

#ifdef VK_USE_PLATFORM_XLIB_KHR
    LOAD_INST_FN( CreateXlibSurfaceKHR );
#endif

#ifdef VK_USE_PLATFORM_XCB_KHR
    LOAD_INST_FN( CreateXcbSurfaceKHR );
#endif

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    LOAD_INST_FN( CreateWaylandSurfaceKHR );
#endif

#ifdef VK_USE_PLATFORM_WIN32_KHR
    LOAD_INST_FN( CreateWin32SurfaceKHR );
#endif

    if( debugUtilsAvailable )
    {
        LOAD_INST_FN( CreateDebugUtilsMessengerEXT );
        LOAD_INST_FN( DestroyDebugUtilsMessengerEXT );

        if( m_vk.CreateDebugUtilsMessengerEXT )
        {
            VkDebugUtilsMessengerCreateInfoEXT dbgInfo{};
            dbgInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            dbgInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                                      | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
                                      | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                      | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
            dbgInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                                   | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                                   | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            dbgInfo.pfnUserCallback = VulkanDebugCallback;

            VkResult dbgResult = m_vk.CreateDebugUtilsMessengerEXT(
                    m_instance, &dbgInfo, nullptr, &m_debugMessenger );

            wxLogTrace( traceRhi, wxS( "VulkanDevice: debug messenger %s" ),
                        dbgResult == VK_SUCCESS ? "created" : "FAILED" );
        }
    }

#undef LOAD_INST_FN

    return true;
}


bool RHI_VULKAN_DEVICE::SelectPhysicalDevice()
{
    if( !m_vk.EnumeratePhysicalDevices )
        return false;

    uint32_t deviceCount = 0;
    m_vk.EnumeratePhysicalDevices( m_instance, &deviceCount, nullptr );

    if( deviceCount == 0 )
        return false;

    std::vector<VkPhysicalDevice> devices( deviceCount );
    m_vk.EnumeratePhysicalDevices( m_instance, &deviceCount, devices.data() );

    // Prefer discrete GPU, fall back to integrated, then any
    VkPhysicalDevice discrete   = VK_NULL_HANDLE;
    VkPhysicalDevice integrated = VK_NULL_HANDLE;

    for( VkPhysicalDevice dev : devices )
    {
        VkPhysicalDeviceProperties props;
        m_vk.GetPhysicalDeviceProperties( dev, &props );

        uint32_t queueFamilyCount = 0;
        m_vk.GetPhysicalDeviceQueueFamilyProperties( dev, &queueFamilyCount, nullptr );
        std::vector<VkQueueFamilyProperties> queueFamilies( queueFamilyCount );
        m_vk.GetPhysicalDeviceQueueFamilyProperties( dev, &queueFamilyCount,
                                                     queueFamilies.data() );

        bool hasGraphics = false;

        for( const auto& qf : queueFamilies )
        {
            if( qf.queueFlags & VK_QUEUE_GRAPHICS_BIT )
            {
                hasGraphics = true;
                break;
            }
        }

        if( !hasGraphics )
            continue;

        if( props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && !discrete )
            discrete = dev;
        else if( props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU && !integrated )
            integrated = dev;
    }

    if( discrete )
        m_physicalDevice = discrete;
    else if( integrated )
        m_physicalDevice = integrated;
    else
        m_physicalDevice = devices[0];

    if( m_physicalDevice != VK_NULL_HANDLE )
    {
        VkPhysicalDeviceProperties props;
        m_vk.GetPhysicalDeviceProperties( m_physicalDevice, &props );
        wxLogTrace( traceRhi, wxS( "VulkanDevice: selected GPU '%s' (type=%d) from %u device(s)" ),
                    props.deviceName, static_cast<int>( props.deviceType ), deviceCount );
    }

    return m_physicalDevice != VK_NULL_HANDLE;
}


bool RHI_VULKAN_DEVICE::CreateLogicalDevice()
{
    if( !m_vk.CreateDevice || !m_physicalDevice )
        return false;

    uint32_t queueFamilyCount = 0;
    m_vk.GetPhysicalDeviceQueueFamilyProperties( m_physicalDevice, &queueFamilyCount, nullptr );
    std::vector<VkQueueFamilyProperties> queueFamilies( queueFamilyCount );
    m_vk.GetPhysicalDeviceQueueFamilyProperties( m_physicalDevice, &queueFamilyCount,
                                                 queueFamilies.data() );

    m_graphicsQueueFamily = UINT32_MAX;

    for( uint32_t i = 0; i < queueFamilyCount; i++ )
    {
        if( queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT )
        {
            m_graphicsQueueFamily = i;
            break;
        }
    }

    if( m_graphicsQueueFamily == UINT32_MAX )
        return false;

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = m_graphicsQueueFamily;
    queueInfo.queueCount       = 1;
    queueInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures features{};

    std::vector<const char*> deviceExtensions;

    // Requesting an unadvertised extension fails device creation, which would rule out offscreen
    // rendering on headless or software Vulkan without WSI
    bool swapchainSupported = true; // assume supported when enumeration is unavailable

    std::vector<VkExtensionProperties> avail;

    auto has = [&]( const char* aName )
    {
        return std::any_of( avail.begin(), avail.end(),
                            [&]( const VkExtensionProperties& e )
                            {
                                return std::strcmp( e.extensionName, aName ) == 0;
                            } );
    };

    if( m_vk.EnumerateDeviceExtensionProperties )
    {
        uint32_t extCount = 0;
        m_vk.EnumerateDeviceExtensionProperties( m_physicalDevice, nullptr, &extCount, nullptr );

        avail.resize( extCount );
        m_vk.EnumerateDeviceExtensionProperties( m_physicalDevice, nullptr, &extCount, avail.data() );

        swapchainSupported = has( VK_KHR_SWAPCHAIN_EXTENSION_NAME );
    }

    if( swapchainSupported )
        deviceExtensions.push_back( VK_KHR_SWAPCHAIN_EXTENSION_NAME );
    else
        wxLogTrace( traceRhi, wxS( "VulkanDevice: swapchain extension unsupported; offscreen only" ) );

    // Waiting for a present to reach the display needs present ids
    VkPhysicalDevicePresentIdFeaturesKHR   presentIdFeatures{};
    VkPhysicalDevicePresentWaitFeaturesKHR presentWaitFeatures{};

    presentIdFeatures.sType   = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR;
    presentWaitFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_WAIT_FEATURES_KHR;
    presentIdFeatures.pNext   = &presentWaitFeatures;

    if( swapchainSupported && m_vk.GetPhysicalDeviceFeatures2 && has( VK_KHR_PRESENT_ID_EXTENSION_NAME )
        && has( VK_KHR_PRESENT_WAIT_EXTENSION_NAME ) )
    {
        VkPhysicalDeviceFeatures2 query{};
        query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        query.pNext = &presentIdFeatures;
        m_vk.GetPhysicalDeviceFeatures2( m_physicalDevice, &query );

        m_presentWait = presentIdFeatures.presentId && presentWaitFeatures.presentWait;
    }

    if( m_presentWait )
    {
        deviceExtensions.push_back( VK_KHR_PRESENT_ID_EXTENSION_NAME );
        deviceExtensions.push_back( VK_KHR_PRESENT_WAIT_EXTENSION_NAME );
    }

    wxLogTrace( traceRhi, wxS( "VulkanDevice: present wait %s" ), m_presentWait ? "on" : "off (fence)" );

    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pNext                   = m_presentWait ? &presentIdFeatures : nullptr;
    deviceInfo.queueCreateInfoCount    = 1;
    deviceInfo.pQueueCreateInfos       = &queueInfo;
    deviceInfo.pEnabledFeatures        = &features;
    deviceInfo.enabledExtensionCount   = static_cast<uint32_t>( deviceExtensions.size() );
    deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();

    VkResult result = m_vk.CreateDevice( m_physicalDevice, &deviceInfo, nullptr, &m_device );

    if( result != VK_SUCCESS )
        return false;

    LoadDeviceFunctions();

    if( m_vk.GetDeviceQueue )
        m_vk.GetDeviceQueue( m_device, m_graphicsQueueFamily, 0, &m_graphicsQueue );

    return true;
}


void RHI_VULKAN_DEVICE::LoadDeviceFunctions()
{
#define LOAD_DEV_FN( fn ) \
    m_vk.fn = reinterpret_cast<PFN_vk##fn>( LoadDeviceFunction( "vk" #fn ) )

    LOAD_DEV_FN( DestroyDevice );
    LOAD_DEV_FN( GetDeviceQueue );
    LOAD_DEV_FN( DeviceWaitIdle );
    LOAD_DEV_FN( CreateImageView );
    LOAD_DEV_FN( DestroyImageView );
    LOAD_DEV_FN( CreateSampler );
    LOAD_DEV_FN( DestroySampler );
    LOAD_DEV_FN( CreateCommandPool );
    LOAD_DEV_FN( DestroyCommandPool );
    LOAD_DEV_FN( AllocateCommandBuffers );
    LOAD_DEV_FN( FreeCommandBuffers );
    LOAD_DEV_FN( BeginCommandBuffer );
    LOAD_DEV_FN( EndCommandBuffer );
    LOAD_DEV_FN( CmdPipelineBarrier );
    LOAD_DEV_FN( CmdCopyBuffer );
    LOAD_DEV_FN( CmdCopyBufferToImage );
    LOAD_DEV_FN( QueueSubmit );
    LOAD_DEV_FN( QueueWaitIdle );
    LOAD_DEV_FN( CreateShaderModule );
    LOAD_DEV_FN( DestroyShaderModule );
    LOAD_DEV_FN( CreatePipelineLayout );
    LOAD_DEV_FN( DestroyPipelineLayout );
    LOAD_DEV_FN( CreatePipelineCache );
    LOAD_DEV_FN( DestroyPipelineCache );
    LOAD_DEV_FN( CreateGraphicsPipelines );
    LOAD_DEV_FN( DestroyPipeline );
    LOAD_DEV_FN( CreateRenderPass );
    LOAD_DEV_FN( DestroyRenderPass );
    LOAD_DEV_FN( CreateFramebuffer );
    LOAD_DEV_FN( DestroyFramebuffer );
    LOAD_DEV_FN( CmdBeginRenderPass );
    LOAD_DEV_FN( CmdEndRenderPass );
    LOAD_DEV_FN( CmdClearColorImage );
    LOAD_DEV_FN( CmdCopyImage );
    LOAD_DEV_FN( CmdCopyImageToBuffer );
    LOAD_DEV_FN( CmdSetViewport );
    LOAD_DEV_FN( CmdSetScissor );
    LOAD_DEV_FN( CmdBindPipeline );
    LOAD_DEV_FN( CmdPushConstants );
    LOAD_DEV_FN( CmdBindVertexBuffers );
    LOAD_DEV_FN( CmdBindIndexBuffer );
    LOAD_DEV_FN( CmdDraw );
    LOAD_DEV_FN( CmdDrawIndexed );
    LOAD_DEV_FN( CmdBindDescriptorSets );
    LOAD_DEV_FN( CreateDescriptorSetLayout );
    LOAD_DEV_FN( DestroyDescriptorSetLayout );
    LOAD_DEV_FN( CreateDescriptorPool );
    LOAD_DEV_FN( DestroyDescriptorPool );
    LOAD_DEV_FN( AllocateDescriptorSets );
    LOAD_DEV_FN( FreeDescriptorSets );
    LOAD_DEV_FN( UpdateDescriptorSets );
    LOAD_DEV_FN( CreateFence );
    LOAD_DEV_FN( DestroyFence );
    LOAD_DEV_FN( WaitForFences );
    LOAD_DEV_FN( ResetFences );
    LOAD_DEV_FN( AcquireNextImageKHR );
    LOAD_DEV_FN( QueuePresentKHR );

    if( m_presentWait )
    {
        LOAD_DEV_FN( WaitForPresentKHR );
        m_presentWait = m_vk.WaitForPresentKHR != nullptr;
    }
    LOAD_DEV_FN( CreateSwapchainKHR );
    LOAD_DEV_FN( DestroySwapchainKHR );
    LOAD_DEV_FN( GetSwapchainImagesKHR );
    LOAD_DEV_FN( CreateSemaphore );
    LOAD_DEV_FN( DestroySemaphore );

#undef LOAD_DEV_FN
}


void RHI_VULKAN_DEVICE::ReadDeviceProperties()
{
    VkPhysicalDeviceProperties props;
    m_vk.GetPhysicalDeviceProperties( m_physicalDevice, &props );

    m_deviceName = std::string( "Vulkan (" ) + props.deviceName + ")";
    m_maxTextureSize = props.limits.maxImageDimension2D;
}


bool RHI_VULKAN_DEVICE::InitVMA()
{
    VmaVulkanFunctions vmaFuncs{};
    vmaFuncs.vkGetInstanceProcAddr = m_vk.GetInstanceProcAddr;
    vmaFuncs.vkGetDeviceProcAddr   = m_vk.GetDeviceProcAddr;

    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_1;
    allocatorInfo.physicalDevice   = m_physicalDevice;
    allocatorInfo.device           = m_device;
    allocatorInfo.instance         = m_instance;
    allocatorInfo.pVulkanFunctions = &vmaFuncs;

    return vmaCreateAllocator( &allocatorInfo, &m_allocator ) == VK_SUCCESS;
}


VkBufferUsageFlags RHI_VULKAN_DEVICE::TranslateBufferUsage( RHI_BUFFER_TYPE type )
{
    switch( type )
    {
    case RHI_BUFFER_TYPE::VERTEX:   return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
                                           | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    case RHI_BUFFER_TYPE::INDEX:    return VK_BUFFER_USAGE_INDEX_BUFFER_BIT
                                           | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    case RHI_BUFFER_TYPE::UNIFORM:  return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
                                           | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }

    return VK_BUFFER_USAGE_TRANSFER_DST_BIT;
}


VkFormat RHI_VULKAN_DEVICE::TranslateTextureFormat( RHI_TEXTURE_FORMAT format )
{
    switch( format )
    {
    case RHI_TEXTURE_FORMAT::RGBA8: return VK_FORMAT_R8G8B8A8_UNORM;
    }

    return VK_FORMAT_R8G8B8A8_UNORM;
}


static VkFormat RhiDefaultDepthFormat()
{
    return VK_FORMAT_D32_SFLOAT;
}


VkFilter RHI_VULKAN_DEVICE::TranslateFilter( RHI_FILTER filter )
{
    switch( filter )
    {
    case RHI_FILTER::BILINEAR: return VK_FILTER_LINEAR;
    }

    return VK_FILTER_LINEAR;
}


static VkBlendFactor TranslateBlendFactor( RHI_BLEND_FACTOR factor )
{
    switch( factor )
    {
    case RHI_BLEND_FACTOR::ZERO:                return VK_BLEND_FACTOR_ZERO;
    case RHI_BLEND_FACTOR::ONE:                 return VK_BLEND_FACTOR_ONE;
    case RHI_BLEND_FACTOR::SRC_ALPHA:           return VK_BLEND_FACTOR_SRC_ALPHA;
    case RHI_BLEND_FACTOR::ONE_MINUS_SRC_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    }

    return VK_BLEND_FACTOR_ONE;
}


static VkBlendOp TranslateBlendOp( RHI_BLEND_OP op )
{
    switch( op )
    {
    case RHI_BLEND_OP::ADD: return VK_BLEND_OP_ADD;
    }

    return VK_BLEND_OP_ADD;
}


static VkCompareOp TranslateCompareOp( RHI_COMPARE_OP op )
{
    switch( op )
    {
    case RHI_COMPARE_OP::LESS:       return VK_COMPARE_OP_LESS;
    case RHI_COMPARE_OP::LESS_EQUAL: return VK_COMPARE_OP_LESS_OR_EQUAL;
    }

    return VK_COMPARE_OP_LESS;
}


static VkFormat TranslateVertexFormat( RHI_VERTEX_FORMAT format )
{
    switch( format )
    {
    case RHI_VERTEX_FORMAT::FLOAT32X2:   return VK_FORMAT_R32G32_SFLOAT;
    case RHI_VERTEX_FORMAT::FLOAT32X4:   return VK_FORMAT_R32G32B32A32_SFLOAT;
    case RHI_VERTEX_FORMAT::FLOAT16X2:   return VK_FORMAT_R16G16_SFLOAT;
    case RHI_VERTEX_FORMAT::FLOAT16X4:   return VK_FORMAT_R16G16B16A16_SFLOAT;
    case RHI_VERTEX_FORMAT::UINT8X4_NORM: return VK_FORMAT_R8G8B8A8_UNORM;
    case RHI_VERTEX_FORMAT::UINT16X2:    return VK_FORMAT_R16G16_UINT;
    }

    return VK_FORMAT_R32G32B32A32_SFLOAT;
}


static VkPrimitiveTopology TranslateTopology( RHI_PRIMITIVE_TOPOLOGY topology )
{
    switch( topology )
    {
    case RHI_PRIMITIVE_TOPOLOGY::TRIANGLE_LIST: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }

    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}


static VkDynamicState TranslateDynamicState( RHI_DYNAMIC_STATE state )
{
    switch( state )
    {
    case RHI_DYNAMIC_STATE::VIEWPORT: return VK_DYNAMIC_STATE_VIEWPORT;
    case RHI_DYNAMIC_STATE::SCISSOR:  return VK_DYNAMIC_STATE_SCISSOR;
    }

    return VK_DYNAMIC_STATE_VIEWPORT;
}


bool RHI_VULKAN_DEVICE::InitCommandPool()
{
    if( !m_vk.CreateCommandPool )
        return false;

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = m_graphicsQueueFamily;

    return m_vk.CreateCommandPool( m_device, &poolInfo, nullptr, &m_commandPool ) == VK_SUCCESS;
}


bool RHI_VULKAN_DEVICE::allocateCommandBuffer( VkCommandBuffer& aCmd )
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = m_commandPool;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    return m_vk.AllocateCommandBuffers( m_device, &allocInfo, &aCmd ) == VK_SUCCESS;
}


VkResult RHI_VULKAN_DEVICE::beginOneTimeCommands( VkCommandBuffer aCmd )
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    return m_vk.BeginCommandBuffer( aCmd, &beginInfo );
}


VkResult RHI_VULKAN_DEVICE::submitCommands( VkCommandBuffer aCmd )
{
    VkSubmitInfo submitInfo{};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &aCmd;

    return m_vk.QueueSubmit( m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
}


bool RHI_VULKAN_DEVICE::SubmitOneShot( VkCommandBuffer cmd )
{
    std::lock_guard<std::recursive_mutex> lock( m_deviceMutex );

    bool ok = false;

    if( m_vk.EndCommandBuffer( cmd ) == VK_SUCCESS && submitCommands( cmd ) == VK_SUCCESS )
    {
        ok = m_vk.QueueWaitIdle( m_graphicsQueue ) == VK_SUCCESS;

        if( ok )
            OnQueueIdle();
    }
    else
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: one-shot command submit failed" ) );
    }

    m_vk.FreeCommandBuffers( m_device, m_commandPool, 1, &cmd );
    return ok;
}


bool RHI_VULKAN_DEVICE::acquirePassCmd( VkCommandBuffer& aCmd )
{
    if( m_freeCmds.empty() )
        return allocateCommandBuffer( aCmd );

    aCmd = m_freeCmds.back();
    m_freeCmds.pop_back();
    return true;
}


VkResult RHI_VULKAN_DEVICE::submitPassCmd( VkCommandBuffer aCmd )
{
    const VkResult result = submitCommands( aCmd );

    if( result != VK_SUCCESS )
    {
        m_freeCmds.push_back( aCmd );
        return result;
    }

    m_submittedCmds.push_back( aCmd );
    m_submitSerial++;
    return result;
}


void RHI_VULKAN_DEVICE::OnQueueIdle()
{
    m_freeCmds.insert( m_freeCmds.end(), m_submittedCmds.begin(), m_submittedCmds.end() );
    m_submittedCmds.clear();
    m_syncedSerial = m_submitSerial;
}


void RHI_VULKAN_DEVICE::SyncSubmittedPasses()
{
    std::lock_guard<std::recursive_mutex> lock( m_deviceMutex );

    if( m_syncedSerial == m_submitSerial )
        return;

    VkResult idleResult = m_vk.QueueWaitIdle( m_graphicsQueue );

    if( idleResult != VK_SUCCESS )
        wxLogTrace( traceRhi, wxS( "VulkanDevice: SyncSubmittedPasses idle=%d" ), idleResult );

    // On device loss the command buffers are dead either way, so recycling them is still right
    OnQueueIdle();
}


VkCommandBuffer RHI_VULKAN_DEVICE::EnsureUploadCmdBuf()
{
    std::lock_guard<std::recursive_mutex> lock( m_deviceMutex );

    if( m_uploadCmdBuf != VK_NULL_HANDLE )
        return m_uploadCmdBuf;

    if( !allocateCommandBuffer( m_uploadCmdBuf ) )
    {
        m_uploadCmdBuf = VK_NULL_HANDLE;
        return VK_NULL_HANDLE;
    }

    beginOneTimeCommands( m_uploadCmdBuf );

    return m_uploadCmdBuf;
}


VkCommandBuffer RHI_VULKAN_DEVICE::beginUploadCommands( std::unique_lock<std::recursive_mutex>& aLock )
{
    if( CurrentThreadOwnsUploadBatch() )
        return EnsureUploadCmdBuf();

    // Held from the allocation to the end of the submit so the render thread cannot race this
    // one on the shared command pool and queue
    aLock.lock();

    VkCommandBuffer cmd = VK_NULL_HANDLE;

    if( !allocateCommandBuffer( cmd ) )
        return VK_NULL_HANDLE;

    beginOneTimeCommands( cmd );
    return cmd;
}


bool RHI_VULKAN_DEVICE::endUploadCommands( VkCommandBuffer aCmd,
                                           const std::unique_lock<std::recursive_mutex>& aLock )
{
    if( !aLock.owns_lock() )
        return false;

    SubmitOneShot( aCmd );
    return true;
}


void RHI_VULKAN_DEVICE::BeginUploadBatch()
{
    std::lock_guard<std::recursive_mutex> lock( m_deviceMutex );

    m_uploadBatchOwner.store( std::this_thread::get_id(), std::memory_order_release );
    m_uploadCmdBuf = VK_NULL_HANDLE;
}


void RHI_VULKAN_DEVICE::EndUploadBatch()
{
    std::lock_guard<std::recursive_mutex> lock( m_deviceMutex );

    m_uploadBatchOwner.store( std::thread::id{}, std::memory_order_release );

    if( m_uploadCmdBuf != VK_NULL_HANDLE )
    {
        SubmitOneShot( m_uploadCmdBuf );
        m_uploadCmdBuf = VK_NULL_HANDLE;
    }

    for( size_t index : m_batchPooledStaging )
        ReleaseStaging( static_cast<int>( index ) );

    m_batchPooledStaging.clear();
}


int RHI_VULKAN_DEVICE::AcquireStaging( size_t aSizeBytes )
{
    std::lock_guard<std::recursive_mutex> lock( m_deviceMutex );

    int best = -1;

    for( size_t i = 0; i < m_stagingPool.size(); ++i )
    {
        const StagingBuffer& candidate = m_stagingPool[i];

        if( candidate.inUse || candidate.size < aSizeBytes )
            continue;

        if( best < 0 || candidate.size < m_stagingPool[best].size )
            best = static_cast<int>( i );
    }

    if( best >= 0 )
    {
        m_stagingPool[best].inUse = true;
        return best;
    }

    // Round up so a scene that grows a little each frame keeps fitting the same buffer
    size_t size = STAGING_MIN_BYTES;

    while( size < aSizeBytes )
        size *= 2;

    VkBufferCreateInfo stagingInfo{};
    stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingInfo.size  = size;
    stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo stagingAllocInfo{};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
    stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    StagingBuffer     staging;
    VmaAllocationInfo mapInfo{};

    if( vmaCreateBuffer( m_allocator, &stagingInfo, &stagingAllocInfo, &staging.buffer,
                         &staging.allocation, &mapInfo ) != VK_SUCCESS )
    {
        return -1;
    }

    staging.mapped = mapInfo.pMappedData;
    staging.size   = size;
    staging.inUse  = true;

    // Reuse a dead slot so indices held by an open batch stay valid
    for( size_t i = 0; i < m_stagingPool.size(); ++i )
    {
        if( m_stagingPool[i].buffer == VK_NULL_HANDLE )
        {
            m_stagingPool[i] = staging;
            return static_cast<int>( i );
        }
    }

    m_stagingPool.push_back( staging );
    return static_cast<int>( m_stagingPool.size() - 1 );
}


int RHI_VULKAN_DEVICE::stageData( const void* aData, size_t aSizeBytes, VkBuffer& aBuffer )
{
    const int index = AcquireStaging( aSizeBytes );

    if( index < 0 )
        return -1;

    // Copy the handle out. The pool vector may grow under another thread's acquire.
    std::lock_guard<std::recursive_mutex> lock( m_deviceMutex );
    const StagingBuffer&                  staging = m_stagingPool[index];

    aBuffer = staging.buffer;
    std::memcpy( staging.mapped, aData, aSizeBytes );
    vmaFlushAllocation( m_allocator, staging.allocation, 0, aSizeBytes );
    return index;
}


void RHI_VULKAN_DEVICE::endStagedUpload( VkCommandBuffer aCmd, std::unique_lock<std::recursive_mutex>& aLock,
                                         int aStaging )
{
    if( endUploadCommands( aCmd, aLock ) )
        ReleaseStaging( aStaging );
    else
        m_batchPooledStaging.push_back( static_cast<size_t>( aStaging ) );
}


void RHI_VULKAN_DEVICE::ReleaseStaging( int aIndex )
{
    std::lock_guard<std::recursive_mutex> lock( m_deviceMutex );

    if( aIndex < 0 || aIndex >= static_cast<int>( m_stagingPool.size() ) )
        return;

    m_stagingPool[aIndex].inUse = false;

    size_t total = 0;

    for( const StagingBuffer& staging : m_stagingPool )
        total += staging.size;

    // Drop idle buffers, largest first, once a burst has left the pool over its budget
    while( total > STAGING_POOL_BYTES )
    {
        int largest = -1;

        for( size_t i = 0; i < m_stagingPool.size(); ++i )
        {
            const StagingBuffer& candidate = m_stagingPool[i];

            if( candidate.inUse || candidate.buffer == VK_NULL_HANDLE )
                continue;

            if( largest < 0 || candidate.size > m_stagingPool[largest].size )
                largest = static_cast<int>( i );
        }

        if( largest < 0 )
            break;

        StagingBuffer& victim = m_stagingPool[largest];
        vmaDestroyBuffer( m_allocator, victim.buffer, victim.allocation );
        total -= victim.size;
        victim = StagingBuffer{};
    }
}


void RHI_VULKAN_DEVICE::DestroyStagingPool()
{
    for( StagingBuffer& staging : m_stagingPool )
    {
        if( m_allocator && staging.buffer != VK_NULL_HANDLE )
            vmaDestroyBuffer( m_allocator, staging.buffer, staging.allocation );
    }

    m_stagingPool.clear();
    m_batchPooledStaging.clear();
}


bool RHI_VULKAN_DEVICE::CurrentThreadOwnsUploadBatch() const
{
    return m_uploadBatchOwner.load( std::memory_order_acquire )
           == std::this_thread::get_id();
}


static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT aSeverity,
        VkDebugUtilsMessageTypeFlagsEXT aType,
        const VkDebugUtilsMessengerCallbackDataEXT* aCallbackData,
        void* )
{
    const char* severityStr = "UNKNOWN";

    if( aSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT )
        severityStr = "ERROR";
    else if( aSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT )
        severityStr = "WARNING";
    else if( aSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT )
        severityStr = "INFO";
    else if( aSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT )
        severityStr = "VERBOSE";

    const char* typeStr = "GENERAL";

    if( aType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT )
        typeStr = "VALIDATION";
    else if( aType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT )
        typeStr = "PERFORMANCE";

    wxLogTrace( traceRhi, wxS( "Vulkan %s [%s]: %s" ),
                severityStr, typeStr, aCallbackData->pMessage );

    return VK_FALSE;
}


RHI_VULKAN_DEVICE::RHI_VULKAN_DEVICE( bool enableValidation )
{
    wxLogTrace( traceRhi, wxS( "VulkanDevice: init (validation=%s)" ),
                enableValidation ? "ON" : "OFF" );

    if( !InitLoader() )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: InitLoader FAILED" ) );
        return;
    }

    if( !CreateVkInstance( enableValidation ) )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: CreateVkInstance FAILED" ) );
        return;
    }

    if( !SelectPhysicalDevice() )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: SelectPhysicalDevice FAILED" ) );
        return;
    }

    if( !CreateLogicalDevice() )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: CreateLogicalDevice FAILED" ) );
        return;
    }

    if( !InitVMA() )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: InitVMA FAILED" ) );
        return;
    }

    if( !InitCommandPool() )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: InitCommandPool FAILED" ) );
        return;
    }

    if( m_vk.CreatePipelineCache )
    {
        VkPipelineCacheCreateInfo cacheInfo{};
        cacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        m_vk.CreatePipelineCache( m_device, &cacheInfo, nullptr, &m_pipelineCache );
    }

    ReadDeviceProperties();
    m_initialized = true;

    wxLogTrace( traceRhi, wxS( "VulkanDevice: initialized OK (%s)" ), m_deviceName );
}


RHI_VULKAN_DEVICE::~RHI_VULKAN_DEVICE()
{
    if( m_device )
    {
        m_vk.DeviceWaitIdle( m_device );

        // Command pool destruction frees all allocated command buffers implicitly
        m_currentCmd = VK_NULL_HANDLE;

        DestroyStagingPool();
        m_vk.DestroyPipelineCache( m_device, m_pipelineCache, nullptr );
        m_vk.DestroyCommandPool( m_device, m_commandPool, nullptr );

        m_deferredReleaseQueue.FlushAll(
                [this]( const RHI_DEFERRED_RELEASE_ENTRY& entry )
                {
                    ExecuteDeferredRelease( entry );
                } );

        DestroyPresentBlit();

        // Destroy swapchains before surfaces
        for( auto& [index, swa] : m_swapchains )
            destroySwapchainObjects( swa );

        for( auto& [index, sa] : m_surfaces )
        {
            m_vk.DestroySurfaceKHR( m_instance, sa.surface, nullptr );

#ifdef VK_USE_PLATFORM_XLIB_KHR
            // As in DestroySurface, close the X11 connection this surface opened
            sa.x11Owner.Close();
#endif
        }

        // A framebuffer's color textures are left to it, so it goes before them
        for( auto& [index, fb] : m_framebuffers )
            destroyFramebufferObjects( fb );

        for( auto& [index, rp] : m_renderPasses )
            m_vk.DestroyRenderPass( m_device, rp.renderPass, nullptr );

        for( auto& poolEntry : m_descriptorPools )
            m_vk.DestroyDescriptorPool( m_device, poolEntry.pool, nullptr );

        for( auto& [index, bgl] : m_bindGroupLayouts )
            m_vk.DestroyDescriptorSetLayout( m_device, bgl.layout, nullptr );

        for( auto& [index, pipe] : m_pipelines )
            destroyPipeline( pipe );

        for( auto& [index, shader] : m_shaders )
            m_vk.DestroyShaderModule( m_device, shader.module, nullptr );

        for( auto& [index, tex] : m_textures )
            destroyTexture( tex );

        if( m_allocator )
        {
            for( auto& [index, buf] : m_buffers )
                vmaDestroyBuffer( m_allocator, buf.buffer, buf.allocation );

            vmaDestroyAllocator( m_allocator );
        }

        m_vk.DestroyDevice( m_device, nullptr );
    }

    if( m_debugMessenger )
        m_vk.DestroyDebugUtilsMessengerEXT( m_instance, m_debugMessenger, nullptr );

    if( m_instance )
        m_vk.DestroyInstance( m_instance, nullptr );

    m_vulkanLib.Unload();

    wxLogTrace( traceRhi, wxS( "VulkanDevice: destroyed" ) );
}


bool RHI_VULKAN_DEVICE::createImage2D( VkFormat aFormat, uint32_t aWidth, uint32_t aHeight,
                                       VkImageUsageFlags aUsage, bool aDedicated, VkImage& aImage,
                                       VmaAllocation& aAllocation )
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.format        = aFormat;
    imageInfo.extent        = { aWidth, aHeight, 1 };
    imageInfo.mipLevels     = 1;
    imageInfo.arrayLayers   = 1;
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage         = aUsage;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    if( aDedicated )
        allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    return vmaCreateImage( m_allocator, &imageInfo, &allocInfo, &aImage, &aAllocation, nullptr ) == VK_SUCCESS;
}


VkImageView RHI_VULKAN_DEVICE::createView( VkImage aImage, VkFormat aFormat, VkImageAspectFlags aAspect )
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType                       = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image                       = aImage;
    viewInfo.viewType                    = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format                      = aFormat;
    viewInfo.subresourceRange.aspectMask = aAspect;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView view = VK_NULL_HANDLE;

    if( m_vk.CreateImageView( m_device, &viewInfo, nullptr, &view ) != VK_SUCCESS )
        return VK_NULL_HANDLE;

    return view;
}


VkSampler RHI_VULKAN_DEVICE::createSampler( VkFilter aFilter )
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter    = aFilter;
    samplerInfo.minFilter    = aFilter;
    samplerInfo.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

    VkSampler sampler = VK_NULL_HANDLE;

    if( m_vk.CreateSampler( m_device, &samplerInfo, nullptr, &sampler ) != VK_SUCCESS )
        return VK_NULL_HANDLE;

    return sampler;
}


void RHI_VULKAN_DEVICE::destroyTexture( TextureAllocation& aTexture )
{
    m_vk.DestroySampler( m_device, aTexture.sampler, nullptr );

    // A framebuffer's color texture shares its image and view, which go with the framebuffer
    if( aTexture.ownedByFBO )
        return;

    m_vk.DestroyImageView( m_device, aTexture.imageView, nullptr );

    if( m_allocator )
        vmaDestroyImage( m_allocator, aTexture.image, aTexture.allocation );
}


void RHI_VULKAN_DEVICE::destroyPipeline( PipelineAllocation& aPipeline )
{
    m_vk.DestroyPipeline( m_device, aPipeline.pipeline, nullptr );
    m_vk.DestroyPipelineLayout( m_device, aPipeline.layout, nullptr );

    for( VkDescriptorSetLayout dsLayout : aPipeline.dsLayouts )
        m_vk.DestroyDescriptorSetLayout( m_device, dsLayout, nullptr );

    m_vk.DestroyRenderPass( m_device, aPipeline.renderPass, nullptr );
}


void RHI_VULKAN_DEVICE::destroyFramebufferObjects( FramebufferAllocation& aFb )
{
    m_vk.DestroyFramebuffer( m_device, aFb.framebuffer, nullptr );
    m_vk.DestroyRenderPass( m_device, aFb.internalRenderPass, nullptr );
    m_vk.DestroyImageView( m_device, aFb.depthView, nullptr );

    for( VkImageView view : aFb.colorViews )
        m_vk.DestroyImageView( m_device, view, nullptr );

    if( m_allocator )
    {
        vmaDestroyImage( m_allocator, aFb.depthImage, aFb.depthAllocation );

        for( size_t i = 0; i < aFb.colorImages.size(); i++ )
            vmaDestroyImage( m_allocator, aFb.colorImages[i], aFb.colorAllocations[i] );
    }

    aFb = FramebufferAllocation();
}


void RHI_VULKAN_DEVICE::ExecuteDeferredReleases()
{
    // Retirement counts CPU frames, and submitted passes may still be reading these resources
    if( m_deferredReleaseQueue.HasRetired( m_frameCounter ) )
        SyncSubmittedPasses();

    m_deferredReleaseQueue.ProcessRetired( m_frameCounter,
            [this]( const RHI_DEFERRED_RELEASE_ENTRY& entry )
            {
                ExecuteDeferredRelease( entry );
            } );
}


void RHI_VULKAN_DEVICE::ExecuteDeferredRelease( const RHI_DEFERRED_RELEASE_ENTRY& aEntry )
{
    uint32_t idx = aEntry.handleValue & RHI_HANDLE_INDEX_MASK;

    switch( aEntry.type )
    {
    case RHI_DEFERRED_RESOURCE_TYPE::BUFFER:
    {
        auto it = m_buffers.find( idx );

        if( it != m_buffers.end() )
        {
            vmaDestroyBuffer( m_allocator, it->second.buffer, it->second.allocation );
            m_buffers.erase( it );
        }

        m_bufferPool.Release( RHI_BufferHandle{ aEntry.handleValue } );
        break;
    }

    case RHI_DEFERRED_RESOURCE_TYPE::TEXTURE:
    {
        auto it = m_textures.find( idx );

        if( it != m_textures.end() )
        {
            destroyTexture( it->second );
            m_textures.erase( it );
        }

        m_texturePool.Release( RHI_TextureHandle{ aEntry.handleValue } );
        break;
    }

    case RHI_DEFERRED_RESOURCE_TYPE::SHADER:
    {
        auto it = m_shaders.find( idx );

        if( it != m_shaders.end() )
        {
            m_vk.DestroyShaderModule( m_device, it->second.module, nullptr );
            m_shaders.erase( it );
        }

        m_shaderPool.Release( RHI_ShaderHandle{ aEntry.handleValue } );
        break;
    }

    case RHI_DEFERRED_RESOURCE_TYPE::PIPELINE:
    {
        auto it = m_pipelines.find( idx );

        if( it != m_pipelines.end() )
        {
            destroyPipeline( it->second );
            m_pipelines.erase( it );
        }

        m_pipelinePool.Release( RHI_PipelineHandle{ aEntry.handleValue } );
        break;
    }

    case RHI_DEFERRED_RESOURCE_TYPE::BIND_GROUP_LAYOUT:
    {
        auto it = m_bindGroupLayouts.find( idx );

        if( it != m_bindGroupLayouts.end() )
        {
            m_vk.DestroyDescriptorSetLayout( m_device, it->second.layout, nullptr );
            m_bindGroupLayouts.erase( it );
        }

        m_bindGroupLayoutPool.Release( RHI_BindGroupLayoutHandle{ aEntry.handleValue } );
        break;
    }

    case RHI_DEFERRED_RESOURCE_TYPE::BIND_GROUP:
    {
        auto it = m_bindGroups.find( idx );

        if( it != m_bindGroups.end() )
        {
            ReleaseDescriptorSet( it->second.pool, it->second.descriptorSet );
            m_bindGroups.erase( it );
        }

        m_bindGroupPool.Release( RHI_BindGroupHandle{ aEntry.handleValue } );
        break;
    }

    case RHI_DEFERRED_RESOURCE_TYPE::RENDER_PASS:
    {
        auto it = m_renderPasses.find( idx );

        if( it != m_renderPasses.end() )
        {
            m_vk.DestroyRenderPass( m_device, it->second.renderPass, nullptr );
            m_renderPasses.erase( it );
        }

        m_renderPassPool.Release( RHI_RenderPassHandle{ aEntry.handleValue } );
        break;
    }

    case RHI_DEFERRED_RESOURCE_TYPE::FRAMEBUFFER:
    {
        auto it = m_framebuffers.find( idx );

        if( it != m_framebuffers.end() )
        {
            FramebufferAllocation& fba = it->second;

            for( RHI_TextureHandle texHandle : fba.colorTextureHandles )
            {
                if( !texHandle )
                    continue;

                // FBO color textures own their sampler, which would otherwise leak until device teardown
                auto texIt = m_textures.find( RhiHandleIndex( texHandle ) );

                if( texIt != m_textures.end() )
                {
                    destroyTexture( texIt->second );
                    m_textures.erase( texIt );
                }

                m_texturePool.Release( texHandle );
            }

            destroyFramebufferObjects( fba );
            m_framebuffers.erase( it );
        }

        m_framebufferPool.Release( RHI_FramebufferHandle{ aEntry.handleValue } );
        break;
    }
    }
}


const char* RHI_VULKAN_DEVICE::GetName() const
{
    return m_deviceName.c_str();
}


RHI_BACKEND RHI_VULKAN_DEVICE::GetBackend() const
{
    return RHI_BACKEND::VULKAN;
}


bool RHI_VULKAN_DEVICE::IsInitialized() const
{
    return m_initialized;
}


// ---- Buffer operations (VMA-backed) ----

RHI_BufferHandle RHI_VULKAN_DEVICE::ReserveBufferHandle()
{
    return m_bufferPool.Allocate();
}


bool RHI_VULKAN_DEVICE::RealizeBuffer( RHI_BufferHandle handle, const RHI_BUFFER_DESC& desc )
{
    if( !handle )
        return false;

    // Release the reserved slot on every failure so a create command that cannot realize does not leak it
    if( desc.sizeBytes == 0 || !m_allocator )
    {
        m_bufferPool.Release( handle );
        return false;
    }

    VkBufferCreateInfo bufInfo{};
    bufInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufInfo.size        = desc.sizeBytes;
    bufInfo.usage       = TranslateBufferUsage( desc.type );
    bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    if( desc.mappable )
    {
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                          | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    }

    VkBuffer          buffer     = VK_NULL_HANDLE;
    VmaAllocation     allocation = VK_NULL_HANDLE;
    VmaAllocationInfo allocationInfo{};

    VkResult result = vmaCreateBuffer( m_allocator, &bufInfo, &allocInfo,
                                       &buffer, &allocation, &allocationInfo );

    if( result != VK_SUCCESS )
    {
        m_bufferPool.Release( handle );
        return false;
    }

    uint32_t idx = RhiHandleIndex( handle );

    BufferAllocation& ba = m_buffers[idx];
    ba.buffer     = buffer;
    ba.allocation = allocation;
    ba.mappedPtr  = desc.mappable ? allocationInfo.pMappedData : nullptr;
    ba.size       = desc.sizeBytes;
    ba.mappable   = desc.mappable;

    return true;
}


void RHI_VULKAN_DEVICE::DestroyBuffer( RHI_BufferHandle handle )
{
    if( !m_allocator )
        return;

    deferDestroy( m_bufferPool, RHI_DEFERRED_RESOURCE_TYPE::BUFFER, handle );
}


void RHI_VULKAN_DEVICE::UpdateBuffer( RHI_BufferHandle handle, const void* data,
                                       size_t sizeBytes, size_t offset )
{
    if( !m_bufferPool.IsValid( handle ) || !data || sizeBytes == 0 )
        return;

    uint32_t idx = RhiHandleIndex( handle );
    auto     it  = m_buffers.find( idx );

    if( it == m_buffers.end() )
        return;

    if( !RhiRangeWithinBuffer( offset, sizeBytes, it->second.size ) )
        return;

    if( it->second.mappable && it->second.mappedPtr )
    {
        // A CPU write lands immediately, so a pass still queued on the GPU would read the
        // new contents. Staged copies below are ordered on the queue and need no wait.
        if( it->second.lastUseSerial > m_syncedSerial )
            SyncSubmittedPasses();

        std::memcpy( static_cast<uint8_t*>( it->second.mappedPtr ) + offset, data, sizeBytes );
        vmaFlushAllocation( m_allocator, it->second.allocation, offset, sizeBytes );
        return;
    }

    VkBuffer  stagingBuffer = VK_NULL_HANDLE;
    const int stagingIndex = stageData( data, sizeBytes, stagingBuffer );

    if( stagingIndex < 0 )
        return;

    std::unique_lock<std::recursive_mutex> oneShotLock( m_deviceMutex, std::defer_lock );
    VkCommandBuffer                        cmd = beginUploadCommands( oneShotLock );

    if( cmd == VK_NULL_HANDLE )
    {
        ReleaseStaging( stagingIndex );
        return;
    }

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = offset;
    copyRegion.size      = sizeBytes;

    m_vk.CmdCopyBuffer( cmd, stagingBuffer, it->second.buffer, 1, &copyRegion );
    endStagedUpload( cmd, oneShotLock, stagingIndex );
}


// ---- Texture operations (VMA-backed) ----

RHI_TextureHandle RHI_VULKAN_DEVICE::ReserveTextureHandle()
{
    return m_texturePool.Allocate();
}


bool RHI_VULKAN_DEVICE::RealizeTexture( RHI_TextureHandle handle, const RHI_TEXTURE_DESC& desc )
{
    if( !handle )
        return false;

    if( desc.width == 0 || desc.height == 0 || !m_allocator )
    {
        m_texturePool.Release( handle );
        return false;
    }

    const VkFormat    vkFormat = TranslateTextureFormat( desc.format );
    TextureAllocation ta;

    if( createImage2D( vkFormat, desc.width, desc.height, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                       false, ta.image, ta.allocation ) )
    {
        ta.imageView = createView( ta.image, vkFormat, VK_IMAGE_ASPECT_COLOR_BIT );

        if( ta.imageView )
            ta.sampler = createSampler( TranslateFilter( desc.filter ) );
    }

    if( !ta.sampler )
    {
        destroyTexture( ta );
        m_texturePool.Release( handle );
        return false;
    }

    ta.width  = desc.width;
    ta.height = desc.height;
    ta.format = desc.format;

    m_textures[RhiHandleIndex( handle )] = ta;

    return true;
}


void RHI_VULKAN_DEVICE::DestroyTexture( RHI_TextureHandle handle )
{
    deferDestroy( m_texturePool, RHI_DEFERRED_RESOURCE_TYPE::TEXTURE, handle );
}


void RHI_VULKAN_DEVICE::UploadTexture( RHI_TextureHandle handle, const void* data,
                                        size_t sizeBytes )
{
    if( !m_texturePool.IsValid( handle ) || !data || sizeBytes == 0 || !m_allocator )
        return;

    uint32_t idx = RhiHandleIndex( handle );
    auto     it  = m_textures.find( idx );

    if( it == m_textures.end() )
        return;

    TextureAllocation& tex = it->second;
    uint32_t bpp           = RhiTextureBytesPerPixel( tex.format );
    size_t   expected      = static_cast<size_t>( tex.width ) * tex.height * bpp;

    if( sizeBytes < expected )
        return;

    VkBuffer  stagingBuffer = VK_NULL_HANDLE;
    const int stagingIndex = stageData( data, expected, stagingBuffer );

    if( stagingIndex < 0 )
        return;

    std::unique_lock<std::recursive_mutex> oneShotLock( m_deviceMutex, std::defer_lock );
    VkCommandBuffer                        cmd = beginUploadCommands( oneShotLock );

    if( cmd == VK_NULL_HANDLE )
    {
        ReleaseStaging( stagingIndex );
        return;
    }

    TransitionTexture( cmd, tex, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT );

    VkBufferImageCopy region{};
    region.bufferOffset      = 0;
    region.bufferRowLength   = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel       = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount     = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { tex.width, tex.height, 1 };

    m_vk.CmdCopyBufferToImage( cmd, stagingBuffer, tex.image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region );

    TransitionTexture( cmd, tex, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT );

    endStagedUpload( cmd, oneShotLock, stagingIndex );
}


// ---- Shader and pipeline operations ----

RHI_ShaderHandle RHI_VULKAN_DEVICE::CreateShader( const RHI_SHADER_DESC& desc )
{
    if( !desc.bytecode || desc.sizeBytes == 0 || !m_vk.CreateShaderModule )
        return {};

    // SPIR-V must be a multiple of 4 bytes
    if( desc.sizeBytes % 4 != 0 )
        return {};

    VkShaderModuleCreateInfo moduleInfo{};
    moduleInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleInfo.codeSize = desc.sizeBytes;
    moduleInfo.pCode    = reinterpret_cast<const uint32_t*>( desc.bytecode );

    VkShaderModule module = VK_NULL_HANDLE;
    VkResult result = m_vk.CreateShaderModule( m_device, &moduleInfo, nullptr, &module );

    if( result != VK_SUCCESS )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: CreateShaderModule FAILED %s(%d) bytes=%zu" ),
                    VkResultToString( result ), static_cast<int>( result ), desc.sizeBytes );
        return {};
    }

    RHI_ShaderHandle handle = m_shaderPool.Allocate();

    if( !handle )
    {
        m_vk.DestroyShaderModule( m_device, module, nullptr );
        return {};
    }

    uint32_t idx = RhiHandleIndex( handle );

    ShaderAllocation& sa = m_shaders[idx];
    sa.module = module;

    return handle;
}


void RHI_VULKAN_DEVICE::DestroyShader( RHI_ShaderHandle handle )
{
    deferDestroy( m_shaderPool, RHI_DEFERRED_RESOURCE_TYPE::SHADER, handle );
}


static VkAccessFlags LayoutToAccessMask( VkImageLayout layout )
{
    switch( layout )
    {
    case VK_IMAGE_LAYOUT_UNDEFINED:                        return 0;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:             return VK_ACCESS_TRANSFER_WRITE_BIT;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:             return VK_ACCESS_TRANSFER_READ_BIT;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:         return VK_ACCESS_SHADER_READ_BIT;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:         return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:                  return 0;
    default:                                               return 0;
    }
}


// A barrier over the one mip level and array layer of an image
static VkImageMemoryBarrier imageBarrier( VkImage aImage, VkImageAspectFlags aAspect, VkImageLayout aOld,
                                          VkImageLayout aNew, VkAccessFlags aSrcAccess, VkAccessFlags aDstAccess )
{
    VkImageMemoryBarrier barrier{};
    barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcAccessMask       = aSrcAccess;
    barrier.dstAccessMask       = aDstAccess;
    barrier.oldLayout           = aOld;
    barrier.newLayout           = aNew;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image               = aImage;
    barrier.subresourceRange    = { aAspect, 0, 1, 0, 1 };
    return barrier;
}


static VkDescriptorType TranslateBindingType( RHI_BINDING_TYPE type )
{
    switch( type )
    {
    case RHI_BINDING_TYPE::UNIFORM_BUFFER:          return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case RHI_BINDING_TYPE::COMBINED_TEXTURE_SAMPLER: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    }

    return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
}


static VkShaderStageFlags TranslateShaderStage( RHI_SHADER_STAGE stages )
{
    VkShaderStageFlags flags = 0;

    if( ( stages & RHI_SHADER_STAGE::VERTEX ) != RHI_SHADER_STAGE::NONE )
        flags |= VK_SHADER_STAGE_VERTEX_BIT;

    if( ( stages & RHI_SHADER_STAGE::FRAGMENT ) != RHI_SHADER_STAGE::NONE )
        flags |= VK_SHADER_STAGE_FRAGMENT_BIT;

    return flags;
}


// A swapchain image drawn whole and then presented
static VK_ATTACHMENT presentAttachment( VkFormat aFormat )
{
    return { aFormat, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED,
             VK_IMAGE_LAYOUT_PRESENT_SRC_KHR };
}


VkRenderPass RHI_VULKAN_DEVICE::makeRenderPass( const std::vector<VK_ATTACHMENT>&  aColor,
                                                const std::optional<VK_ATTACHMENT>& aDepth )
{
    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference>   colorRefs;

    auto describe = []( const VK_ATTACHMENT& aAttachment )
    {
        VkAttachmentDescription att{};
        att.format         = aAttachment.format;
        att.samples        = VK_SAMPLE_COUNT_1_BIT;
        att.loadOp         = aAttachment.loadOp;
        att.storeOp        = aAttachment.storeOp;
        att.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        att.initialLayout  = aAttachment.initialLayout;
        att.finalLayout    = aAttachment.finalLayout;
        return att;
    };

    for( const VK_ATTACHMENT& color : aColor )
    {
        const uint32_t index = static_cast<uint32_t>( attachments.size() );

        colorRefs.push_back( { index, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL } );
        attachments.push_back( describe( color ) );
    }

    const VkAttachmentReference depthRef{ static_cast<uint32_t>( attachments.size() ),
                                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

    if( aDepth )
        attachments.push_back( describe( *aDepth ) );

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = static_cast<uint32_t>( colorRefs.size() );
    subpass.pColorAttachments    = colorRefs.data();

    if( aDepth )
        subpass.pDepthStencilAttachment = &depthRef;

    VkRenderPassCreateInfo rpInfo{};
    rpInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpInfo.attachmentCount = static_cast<uint32_t>( attachments.size() );
    rpInfo.pAttachments    = attachments.data();
    rpInfo.subpassCount    = 1;
    rpInfo.pSubpasses      = &subpass;

    VkRenderPass renderPass = VK_NULL_HANDLE;

    if( m_vk.CreateRenderPass( m_device, &rpInfo, nullptr, &renderPass ) != VK_SUCCESS )
        return VK_NULL_HANDLE;

    return renderPass;
}


RHI_PipelineHandle RHI_VULKAN_DEVICE::CreatePipeline( const RHI_PIPELINE_DESC& desc )
{
    PROF_TIMER createTimer;

    if( !m_vk.CreateGraphicsPipelines || !m_vk.CreatePipelineLayout || !m_vk.CreateRenderPass )
        return {};

    if( !desc.vertexShader || !desc.fragmentShader )
        return {};

    if( !m_shaderPool.IsValid( desc.vertexShader ) || !m_shaderPool.IsValid( desc.fragmentShader ) )
        return {};

    auto vertIt = m_shaders.find( RhiHandleIndex( desc.vertexShader ) );
    auto fragIt = m_shaders.find( RhiHandleIndex( desc.fragmentShader ) );

    if( vertIt == m_shaders.end() || fragIt == m_shaders.end() )
        return {};

    const bool useDepthStencil = RhiPipelineUsesDepthStencil( desc );

    std::optional<VK_ATTACHMENT> depth;

    if( useDepthStencil )
    {
        depth = VK_ATTACHMENT{ RhiDefaultDepthFormat(), VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
    }

    const VkRenderPass renderPass =
            makeRenderPass( { { TranslateTextureFormat( desc.colorFormat ), VK_ATTACHMENT_LOAD_OP_CLEAR,
                                VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL } },
                            depth );

    if( !renderPass )
        return {};

    std::vector<VkDescriptorSetLayout> dsLayouts;

    // The pipeline owns these until they move into its PipelineAllocation
    auto destroyDsLayouts = [&]()
    {
        for( VkDescriptorSetLayout dsLayout : dsLayouts )
            m_vk.DestroyDescriptorSetLayout( m_device, dsLayout, nullptr );
    };

    for( const auto& bglDesc : desc.bindGroupLayouts )
    {
        const VkDescriptorSetLayout dsLayout = createSetLayout( bglDesc );

        if( !dsLayout )
        {
            destroyDsLayouts();
            m_vk.DestroyRenderPass( m_device, renderPass, nullptr );
            return {};
        }

        dsLayouts.push_back( dsLayout );
    }

    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushRange.offset     = 0;
    pushRange.size       = RHI_MAX_PUSH_CONSTANT_BYTES;

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount         = static_cast<uint32_t>( dsLayouts.size() );
    layoutInfo.pSetLayouts            = dsLayouts.empty() ? nullptr : dsLayouts.data();
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges    = &pushRange;

    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

    if( m_vk.CreatePipelineLayout( m_device, &layoutInfo, nullptr, &pipelineLayout ) != VK_SUCCESS )
    {
        destroyDsLayouts();
        m_vk.DestroyRenderPass( m_device, renderPass, nullptr );
        return {};
    }

    std::vector<VkVertexInputBindingDescription>   bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;

    for( uint32_t b = 0; b < desc.vertexBuffers.size(); b++ )
    {
        const auto& vb = desc.vertexBuffers[b];

        VkVertexInputBindingDescription binding{};
        binding.binding   = b;
        binding.stride    = vb.stride;
        binding.inputRate = ( vb.stepMode == RHI_VERTEX_STEP_MODE::INSTANCE )
                                    ? VK_VERTEX_INPUT_RATE_INSTANCE
                                    : VK_VERTEX_INPUT_RATE_VERTEX;
        bindings.push_back( binding );

        for( const auto& attr : vb.attributes )
        {
            VkVertexInputAttributeDescription a{};
            a.location = attr.location;
            a.binding  = b;
            a.format   = TranslateVertexFormat( attr.format );
            a.offset   = attr.offset;
            attributes.push_back( a );
        }
    }

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount   = static_cast<uint32_t>( bindings.size() );
    vertexInput.pVertexBindingDescriptions      = bindings.data();
    vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>( attributes.size() );
    vertexInput.pVertexAttributeDescriptions    = attributes.data();

    std::vector<VkPipelineColorBlendAttachmentState> blendAttachments;

    if( desc.blendStates.empty() )
    {
        VkPipelineColorBlendAttachmentState defaultBlend{};
        defaultBlend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                                      | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        blendAttachments.push_back( defaultBlend );
    }
    else
    {
        for( const auto& bs : desc.blendStates )
        {
            VkPipelineColorBlendAttachmentState vkBlend{};
            vkBlend.blendEnable         = bs.enabled ? VK_TRUE : VK_FALSE;
            vkBlend.srcColorBlendFactor = TranslateBlendFactor( bs.srcColor );
            vkBlend.dstColorBlendFactor = TranslateBlendFactor( bs.dstColor );
            vkBlend.colorBlendOp        = TranslateBlendOp( bs.colorOp );
            vkBlend.srcAlphaBlendFactor = TranslateBlendFactor( bs.srcAlpha );
            vkBlend.dstAlphaBlendFactor = TranslateBlendFactor( bs.dstAlpha );
            vkBlend.alphaBlendOp        = TranslateBlendOp( bs.alphaOp );
            vkBlend.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                                          | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            blendAttachments.push_back( vkBlend );
        }
    }

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable       = desc.depthStencil.depthTestEnable ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable      = desc.depthStencil.depthWriteEnable ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp        = TranslateCompareOp( desc.depthStencil.depthCompareOp );
    depthStencil.stencilTestEnable     = VK_FALSE;

    // Viewport and scissor are always dynamic
    std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT,
                                                   VK_DYNAMIC_STATE_SCISSOR };

    for( auto ds : desc.dynamicStates )
    {
        VkDynamicState vkDs = TranslateDynamicState( ds );

        if( vkDs != VK_DYNAMIC_STATE_VIEWPORT && vkDs != VK_DYNAMIC_STATE_SCISSOR )
            dynamicStates.push_back( vkDs );
    }

    VkPipeline     pipeline = VK_NULL_HANDLE;
    const VkResult result = buildPipeline( vertIt->second.module, fragIt->second.module, vertexInput,
                                           TranslateTopology( desc.topology ), blendAttachments, depthStencil,
                                           dynamicStates, pipelineLayout, renderPass, pipeline );

    if( result != VK_SUCCESS )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: CreateGraphicsPipelines FAILED %s(%d)" ),
                    VkResultToString( result ), static_cast<int>( result ) );
        m_vk.DestroyPipelineLayout( m_device, pipelineLayout, nullptr );
        m_vk.DestroyRenderPass( m_device, renderPass, nullptr );
        destroyDsLayouts();
        return {};
    }

    RHI_PipelineHandle handle = m_pipelinePool.Allocate();

    if( !handle )
    {
        m_vk.DestroyPipeline( m_device, pipeline, nullptr );
        m_vk.DestroyPipelineLayout( m_device, pipelineLayout, nullptr );
        m_vk.DestroyRenderPass( m_device, renderPass, nullptr );
        destroyDsLayouts();
        return {};
    }

    uint32_t idx = RhiHandleIndex( handle );

    PipelineAllocation& pa = m_pipelines[idx];
    pa.pipeline   = pipeline;
    pa.layout     = pipelineLayout;
    pa.renderPass = renderPass;
    pa.dsLayouts  = std::move( dsLayouts );

    const double ms = createTimer.msecs();
    m_totalPipelineCreateMs += ms;

    if( ms > 10.0 )
    {
        wxLogTrace( traceRhi, wxS( "%s: CreatePipeline took %.1fms (total %.1fms)" ),
                    GetName(), ms, m_totalPipelineCreateMs );
    }

    return handle;
}


void RHI_VULKAN_DEVICE::DestroyPipeline( RHI_PipelineHandle handle )
{
    deferDestroy( m_pipelinePool, RHI_DEFERRED_RESOURCE_TYPE::PIPELINE, handle );
}


VkResult RHI_VULKAN_DEVICE::buildPipeline( VkShaderModule aVertex, VkShaderModule aFragment,
                                           const VkPipelineVertexInputStateCreateInfo&             aVertexInput,
                                           VkPrimitiveTopology                                     aTopology,
                                           const std::vector<VkPipelineColorBlendAttachmentState>& aBlend,
                                           const VkPipelineDepthStencilStateCreateInfo&            aDepthStencil,
                                           const std::vector<VkDynamicState>& aDynamicStates,
                                           VkPipelineLayout aLayout, VkRenderPass aRenderPass,
                                           VkPipeline& aPipeline )
{
    VkPipelineShaderStageCreateInfo stages[2] = {};
    stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = aVertex;
    stages[0].pName  = "main";
    stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = aFragment;
    stages[1].pName  = "main";

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = aTopology;

    // Viewport and scissor are dynamic, so only the counts are set here
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount  = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode    = VK_CULL_MODE_NONE;
    rasterizer.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.lineWidth   = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisample{};
    multisample.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendStateCreateInfo blendState{};
    blendState.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blendState.attachmentCount = static_cast<uint32_t>( aBlend.size() );
    blendState.pAttachments    = aBlend.data();

    VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
    dynamicStateInfo.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>( aDynamicStates.size() );
    dynamicStateInfo.pDynamicStates    = aDynamicStates.data();

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount          = 2;
    pipelineInfo.pStages             = stages;
    pipelineInfo.pVertexInputState   = &aVertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisample;
    pipelineInfo.pColorBlendState    = &blendState;
    pipelineInfo.pDepthStencilState  = &aDepthStencil;
    pipelineInfo.pDynamicState       = &dynamicStateInfo;
    pipelineInfo.layout              = aLayout;
    pipelineInfo.renderPass          = aRenderPass;

    return m_vk.CreateGraphicsPipelines( m_device, m_pipelineCache, 1, &pipelineInfo, nullptr, &aPipeline );
}


VkDescriptorSetLayout RHI_VULKAN_DEVICE::createSetLayout( const RHI_BIND_GROUP_LAYOUT_DESC& aDesc )
{
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    for( const RHI_BINDING_ENTRY& entry : aDesc.entries )
    {
        bindings.push_back( { entry.binding, TranslateBindingType( entry.type ), 1,
                              TranslateShaderStage( entry.stages ), nullptr } );
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>( bindings.size() );
    layoutInfo.pBindings    = bindings.data();

    VkDescriptorSetLayout layout = VK_NULL_HANDLE;

    if( m_vk.CreateDescriptorSetLayout( m_device, &layoutInfo, nullptr, &layout ) != VK_SUCCESS )
        return VK_NULL_HANDLE;

    return layout;
}


RHI_BindGroupLayoutHandle RHI_VULKAN_DEVICE::CreateBindGroupLayout(
        const RHI_BIND_GROUP_LAYOUT_DESC& desc )
{
    if( !desc.Validate() )
        return {};

    if( !m_vk.CreateDescriptorSetLayout )
        return {};

    const VkDescriptorSetLayout vkLayout = createSetLayout( desc );

    if( !vkLayout )
        return {};

    RHI_BindGroupLayoutHandle handle = m_bindGroupLayoutPool.Allocate();

    if( !handle )
    {
        m_vk.DestroyDescriptorSetLayout( m_device, vkLayout, nullptr );
        return {};
    }

    uint32_t idx = RhiHandleIndex( handle );

    m_bindGroupLayouts[idx].layout = vkLayout;
    m_bindGroupLayouts[idx].desc   = desc;

    return handle;
}


void RHI_VULKAN_DEVICE::DestroyBindGroupLayout( RHI_BindGroupLayoutHandle handle )
{
    deferDestroy( m_bindGroupLayoutPool, RHI_DEFERRED_RESOURCE_TYPE::BIND_GROUP_LAYOUT, handle );
}


VkDescriptorPool RHI_VULKAN_DEVICE::AcquireDescriptorPool()
{
    std::lock_guard<std::recursive_mutex> lock( m_deviceMutex );

    for( auto& entry : m_descriptorPools )
    {
        if( entry.allocatedSets < entry.maxSets )
        {
            entry.allocatedSets++;
            return entry.pool;
        }
    }

    // Every pool is full. FREE_DESCRIPTOR_SET_BIT lets a destroyed bind group free its own set.
    const uint32_t setsPerPool = 128;

    VkDescriptorPoolSize poolSizes[] = {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         setsPerPool * 2 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  setsPerPool },
    };

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets       = setsPerPool;
    poolInfo.poolSizeCount = static_cast<uint32_t>( std::size( poolSizes ) );
    poolInfo.pPoolSizes    = poolSizes;

    VkDescriptorPool pool = VK_NULL_HANDLE;

    if( m_vk.CreateDescriptorPool( m_device, &poolInfo, nullptr, &pool ) != VK_SUCCESS )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: AcquireDescriptorPool FAILED" ) );
        return VK_NULL_HANDLE;
    }

    m_descriptorPools.push_back( { pool, 1, setsPerPool } );
    return pool;
}


void RHI_VULKAN_DEVICE::ReleaseDescriptorSet( VkDescriptorPool pool, VkDescriptorSet dset )
{
    std::lock_guard<std::recursive_mutex> lock( m_deviceMutex );

    for( auto& entry : m_descriptorPools )
    {
        if( entry.pool == pool )
        {
            if( dset != VK_NULL_HANDLE && m_vk.FreeDescriptorSets )
                m_vk.FreeDescriptorSets( m_device, pool, 1, &dset );

            if( entry.allocatedSets > 0 )
                entry.allocatedSets--;

            return;
        }
    }
}


void RHI_VULKAN_DEVICE::TransitionTexture( VkCommandBuffer cmd, TextureAllocation& tex,
                                            VkImageLayout newLayout,
                                            VkPipelineStageFlags srcStage,
                                            VkPipelineStageFlags dstStage )
{
    if( tex.currentLayout == newLayout )
        return;

    const VkImageMemoryBarrier barrier = imageBarrier( tex.image, VK_IMAGE_ASPECT_COLOR_BIT, tex.currentLayout,
                                                       newLayout, LayoutToAccessMask( tex.currentLayout ),
                                                       LayoutToAccessMask( newLayout ) );

    m_vk.CmdPipelineBarrier( cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier );
    tex.currentLayout = newLayout;
}


RHI_BindGroupHandle RHI_VULKAN_DEVICE::CreateBindGroup( const RHI_BIND_GROUP_DESC& desc )
{
    if( !m_bindGroupLayoutPool.IsValid( desc.layout ) )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: CreateBindGroup invalid layout=%u" ),
                    static_cast<unsigned>( desc.layout.value ) );
        return {};
    }

    if( !m_vk.CreateDescriptorPool || !m_vk.AllocateDescriptorSets || !m_vk.UpdateDescriptorSets )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: CreateBindGroup missing descriptor funcs" ) );
        return {};
    }

    std::lock_guard<std::recursive_mutex> lock( m_deviceMutex );

    uint32_t layoutIdx = RhiHandleIndex( desc.layout );
    auto     layoutIt  = m_bindGroupLayouts.find( layoutIdx );

    if( layoutIt == m_bindGroupLayouts.end() )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: CreateBindGroup layout not found=%u" ),
                    static_cast<unsigned>( desc.layout.value ) );
        return {};
    }

    const auto& layoutDesc = layoutIt->second.desc;

    VkDescriptorPool pool = AcquireDescriptorPool();

    if( pool == VK_NULL_HANDLE )
        return {};

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts        = &layoutIt->second.layout;

    VkDescriptorSet dset = VK_NULL_HANDLE;

    if( m_vk.AllocateDescriptorSets( m_device, &allocInfo, &dset ) != VK_SUCCESS )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: CreateBindGroup descriptor set alloc FAILED "
                                    "layout=%u" ),
                    static_cast<unsigned>( desc.layout.value ) );
        ReleaseDescriptorSet( pool );
        return {};
    }

    std::vector<VkWriteDescriptorSet>   writes;
    std::vector<VkDescriptorBufferInfo> bufferInfos;
    std::vector<VkDescriptorImageInfo>  imageInfos;
    bufferInfos.reserve( desc.entries.size() );
    imageInfos.reserve( desc.entries.size() );

    for( const auto& res : desc.entries )
    {
        const RHI_BINDING_ENTRY* layoutEntry = layoutDesc.Find( res.binding );
        const RHI_BINDING_TYPE   bindType = layoutEntry ? layoutEntry->type : RHI_BINDING_TYPE::UNIFORM_BUFFER;

        VkWriteDescriptorSet write{};
        write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet          = dset;
        write.dstBinding      = res.binding;
        write.dstArrayElement = 0;
        write.descriptorCount = 1;
        write.descriptorType  = TranslateBindingType( bindType );

        if( bindType == RHI_BINDING_TYPE::UNIFORM_BUFFER )
        {
            if( !m_bufferPool.IsValid( res.bufferHandle ) )
            {
                wxLogTrace( traceRhi, wxS( "VulkanDevice: CreateBindGroup missing buffer "
                                            "layout=%u binding=%u handle=%u" ),
                            static_cast<unsigned>( desc.layout.value ), res.binding,
                            static_cast<unsigned>( res.bufferHandle.value ) );
                continue;
            }

            auto bufIt = m_buffers.find( RhiHandleIndex( res.bufferHandle ) );

            if( bufIt == m_buffers.end() )
            {
                wxLogTrace( traceRhi, wxS( "VulkanDevice: CreateBindGroup buffer lookup FAILED "
                                            "layout=%u binding=%u handle=%u" ),
                            static_cast<unsigned>( desc.layout.value ), res.binding,
                            static_cast<unsigned>( res.bufferHandle.value ) );
                continue;
            }

            VkDescriptorBufferInfo bi{};
            bi.buffer = bufIt->second.buffer;
            bi.offset = res.offset;
            bi.range  = ( res.size > 0 ) ? res.size : bufIt->second.size;
            bufferInfos.push_back( bi );

            write.pBufferInfo = &bufferInfos.back();
        }
        else
        {
            if( !m_texturePool.IsValid( res.textureHandle ) )
            {
                wxLogTrace( traceRhi, wxS( "VulkanDevice: CreateBindGroup missing texture "
                                            "layout=%u binding=%u handle=%u" ),
                            static_cast<unsigned>( desc.layout.value ), res.binding,
                            static_cast<unsigned>( res.textureHandle.value ) );
                continue;
            }

            auto texIt = m_textures.find( RhiHandleIndex( res.textureHandle ) );

            if( texIt == m_textures.end() )
            {
                wxLogTrace( traceRhi, wxS( "VulkanDevice: CreateBindGroup texture lookup FAILED "
                                            "layout=%u binding=%u handle=%u" ),
                            static_cast<unsigned>( desc.layout.value ), res.binding,
                            static_cast<unsigned>( res.textureHandle.value ) );
                continue;
            }

            VkDescriptorImageInfo ii{};
            ii.sampler     = texIt->second.sampler;
            ii.imageView   = texIt->second.imageView;
            ii.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfos.push_back( ii );

            write.pImageInfo = &imageInfos.back();
        }

        writes.push_back( write );
    }

    // An unwritten descriptor set would fail validation on its first draw
    if( !desc.entries.empty() && writes.empty() )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: CreateBindGroup produced no descriptor writes "
                                    "layout=%u entries=%zu" ),
                    static_cast<unsigned>( desc.layout.value ), desc.entries.size() );
        ReleaseDescriptorSet( pool, dset );
        return {};
    }

    if( !writes.empty() )
    {
        m_vk.UpdateDescriptorSets( m_device, static_cast<uint32_t>( writes.size() ),
                                   writes.data(), 0, nullptr );
    }

    RHI_BindGroupHandle handle = m_bindGroupPool.Allocate();

    if( !handle )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: CreateBindGroup handle alloc FAILED" ) );
        ReleaseDescriptorSet( pool, dset );
        return {};
    }

    uint32_t idx = RhiHandleIndex( handle );

    BindGroupAllocation& bga = m_bindGroups[idx];
    bga.descriptorSet = dset;
    bga.pool          = pool;
    bga.bufferIndices.clear();

    for( const auto& entry : desc.entries )
    {
        if( entry.bufferHandle )
            bga.bufferIndices.push_back( RhiHandleIndex( entry.bufferHandle ) );
    }

    return handle;
}


void RHI_VULKAN_DEVICE::DestroyBindGroup( RHI_BindGroupHandle handle )
{
    deferDestroy( m_bindGroupPool, RHI_DEFERRED_RESOURCE_TYPE::BIND_GROUP, handle );
}


void RHI_VULKAN_DEVICE::BindGroup( uint32_t groupIndex, RHI_BindGroupHandle group )
{
    std::lock_guard<std::recursive_mutex> lock( m_deviceMutex );

    if( !m_renderPassActive || !m_currentCmd || !m_vk.CmdBindDescriptorSets )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: BindGroup skipped renderPassActive=%d "
                                    "cmd=%p group=%u handle=%u" ),
                    m_renderPassActive ? 1 : 0, m_currentCmd, groupIndex,
                    static_cast<unsigned>( group.value ) );
        return;
    }

    if( groupIndex >= static_cast<uint32_t>( RHI_MAX_BIND_GROUPS ) )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: BindGroup invalid group index=%u" ),
                    groupIndex );
        return;
    }

    if( !m_bindGroupPool.IsValid( group ) )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: BindGroup invalid handle=%u" ),
                    static_cast<unsigned>( group.value ) );
        return;
    }

    auto bgIt = m_bindGroups.find( RhiHandleIndex( group ) );

    if( bgIt == m_bindGroups.end() )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: BindGroup lookup FAILED handle=%u" ),
                    static_cast<unsigned>( group.value ) );
        return;
    }

    if( !m_pipelinePool.IsValid( m_activePipeline ) )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: BindGroup missing active pipeline "
                                    "group=%u handle=%u active=%u" ),
                    groupIndex, static_cast<unsigned>( group.value ),
                    static_cast<unsigned>( m_activePipeline.value ) );
        return;
    }

    auto pipeIt = m_pipelines.find( RhiHandleIndex( m_activePipeline ) );

    if( pipeIt == m_pipelines.end() )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: BindGroup pipeline lookup FAILED active=%u" ),
                    static_cast<unsigned>( m_activePipeline.value ) );
        return;
    }

    for( uint32_t bufferIndex : bgIt->second.bufferIndices )
    {
        auto bufIt = m_buffers.find( bufferIndex );

        if( bufIt != m_buffers.end() )
            MarkBufferInUse( bufIt->second );
    }

    m_vk.CmdBindDescriptorSets( m_currentCmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipeIt->second.layout, groupIndex, 1,
                                &bgIt->second.descriptorSet, 0, nullptr );
}


static VkAttachmentLoadOp TranslateLoadOp( RHI_LOAD_OP op )
{
    switch( op )
    {
    case RHI_LOAD_OP::LOAD:      return VK_ATTACHMENT_LOAD_OP_LOAD;
    case RHI_LOAD_OP::CLEAR:     return VK_ATTACHMENT_LOAD_OP_CLEAR;
    case RHI_LOAD_OP::DONT_CARE: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    }

    return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
}


static VkAttachmentStoreOp TranslateStoreOp( RHI_STORE_OP op )
{
    switch( op )
    {
    case RHI_STORE_OP::STORE:     return VK_ATTACHMENT_STORE_OP_STORE;
    case RHI_STORE_OP::DONT_CARE: return VK_ATTACHMENT_STORE_OP_DONT_CARE;
    }

    return VK_ATTACHMENT_STORE_OP_DONT_CARE;
}


RHI_RenderPassHandle RHI_VULKAN_DEVICE::CreateRenderPass( const RHI_RENDER_PASS_DESC& desc )
{
    if( !m_vk.CreateRenderPass )
        return {};

    if( desc.colorAttachments.empty() && !desc.depthStencilAttachment.enabled )
        return {};

    // After rendering a color target is left ready to sample. CLEAR and DONT_CARE discard old
    // contents, so their initial layout is irrelevant.
    std::vector<VK_ATTACHMENT> colors;

    for( const auto& ca : desc.colorAttachments )
    {
        colors.push_back( { TranslateTextureFormat( ca.format ), TranslateLoadOp( ca.loadOp ),
                            TranslateStoreOp( ca.storeOp ),
                            ca.loadOp == RHI_LOAD_OP::LOAD ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                                                           : VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } );
    }

    std::optional<VK_ATTACHMENT> depth;
    const auto&               ds = desc.depthStencilAttachment;

    if( ds.enabled )
    {
        depth = VK_ATTACHMENT{ RhiDefaultDepthFormat(), TranslateLoadOp( ds.depthLoadOp ),
                            TranslateStoreOp( ds.depthStoreOp ),
                            ds.depthLoadOp == RHI_LOAD_OP::LOAD ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                                                                : VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
    }

    const VkRenderPass renderPass = makeRenderPass( colors, depth );

    if( !renderPass )
        return {};

    RHI_RenderPassHandle handle = m_renderPassPool.Allocate();

    if( !handle )
    {
        m_vk.DestroyRenderPass( m_device, renderPass, nullptr );
        return {};
    }

    uint32_t idx = RhiHandleIndex( handle );

    RenderPassAllocation& rpa = m_renderPasses[idx];
    rpa.renderPass       = renderPass;
    rpa.colorDescs       = desc.colorAttachments;
    rpa.depthStencilDesc = desc.depthStencilAttachment;

    return handle;
}


void RHI_VULKAN_DEVICE::DestroyRenderPass( RHI_RenderPassHandle handle )
{
    deferDestroy( m_renderPassPool, RHI_DEFERRED_RESOURCE_TYPE::RENDER_PASS, handle );
}


RHI_FramebufferHandle RHI_VULKAN_DEVICE::CreateFramebuffer( const RHI_FRAMEBUFFER_DESC& desc )
{
    if( desc.width == 0 || desc.height == 0 || !m_allocator || !m_vk.CreateRenderPass )
        return {};

    if( desc.colorFormats.empty() && !desc.hasDepthStencil )
        return {};

    FramebufferAllocation fb;
    fb.colorFormats = desc.colorFormats;
    fb.width        = desc.width;
    fb.height       = desc.height;

    auto fail = [&]()
    {
        destroyFramebufferObjects( fb );
        return RHI_FramebufferHandle{};
    };

    for( RHI_TEXTURE_FORMAT format : desc.colorFormats )
    {
        const VkFormat vkFormat = TranslateTextureFormat( format );
        VkImage        image = VK_NULL_HANDLE;
        VmaAllocation  allocation = VK_NULL_HANDLE;

        if( !createImage2D( vkFormat, desc.width, desc.height,
                            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                                    | VK_IMAGE_USAGE_SAMPLED_BIT,
                            true, image, allocation ) )
        {
            return fail();
        }

        fb.colorImages.push_back( image );
        fb.colorAllocations.push_back( allocation );
        fb.colorViews.push_back( createView( image, vkFormat, VK_IMAGE_ASPECT_COLOR_BIT ) );

        if( !fb.colorViews.back() )
            return fail();
    }

    if( desc.hasDepthStencil )
    {
        if( !createImage2D( RhiDefaultDepthFormat(), desc.width, desc.height,
                            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                                    | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                            false, fb.depthImage, fb.depthAllocation ) )
        {
            return fail();
        }

        fb.depthView = createView( fb.depthImage, RhiDefaultDepthFormat(), VK_IMAGE_ASPECT_DEPTH_BIT );

        if( !fb.depthView )
            return fail();
    }

    // An internal render pass used only for VkFramebuffer compatibility
    std::vector<VK_ATTACHMENT> colors;

    for( RHI_TEXTURE_FORMAT format : desc.colorFormats )
    {
        colors.push_back( { TranslateTextureFormat( format ), VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                            VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL } );
    }

    std::optional<VK_ATTACHMENT> depth;

    if( desc.hasDepthStencil )
    {
        depth = VK_ATTACHMENT{ RhiDefaultDepthFormat(), VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                            VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
    }

    fb.internalRenderPass = makeRenderPass( colors, depth );

    if( !fb.internalRenderPass )
        return fail();

    std::vector<VkImageView> framebufferAttachments = fb.colorViews;

    if( fb.depthView )
        framebufferAttachments.push_back( fb.depthView );

    VkFramebufferCreateInfo fbInfo{};
    fbInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbInfo.renderPass      = fb.internalRenderPass;
    fbInfo.attachmentCount = static_cast<uint32_t>( framebufferAttachments.size() );
    fbInfo.pAttachments    = framebufferAttachments.data();
    fbInfo.width           = desc.width;
    fbInfo.height          = desc.height;
    fbInfo.layers          = 1;

    if( m_vk.CreateFramebuffer( m_device, &fbInfo, nullptr, &fb.framebuffer ) != VK_SUCCESS )
        return fail();

    RHI_FramebufferHandle handle = m_framebufferPool.Allocate();

    if( !handle )
        return fail();

    FramebufferAllocation& fba = m_framebuffers[RhiHandleIndex( handle )];
    fba = std::move( fb );

    for( uint32_t i = 0; i < fba.colorViews.size(); i++ )
    {
        RHI_TextureHandle texHandle = m_texturePool.Allocate();

        if( texHandle )
        {
            // The sampler lets the compositor bind FBO color textures as combined image samplers
            const VkSampler sampler = createSampler( VK_FILTER_LINEAR );

            if( !sampler )
            {
                // Skip the texture rather than store a null sampler that a later bind would dereference
                wxLogTrace( traceRhi, wxS( "VulkanDevice: FBO color sampler creation failed" ) );
                m_texturePool.Release( texHandle );
                fba.colorTextureHandles.push_back( RHI_TextureHandle{} );
                continue;
            }

            TextureAllocation& texAlloc = m_textures[RhiHandleIndex( texHandle )];
            texAlloc.image      = fba.colorImages[i];
            texAlloc.allocation = fba.colorAllocations[i];
            texAlloc.imageView  = fba.colorViews[i];
            texAlloc.sampler    = sampler;
            texAlloc.width      = desc.width;
            texAlloc.height     = desc.height;
            texAlloc.format     = desc.colorFormats[i];
            texAlloc.ownedByFBO = true;
        }

        fba.colorTextureHandles.push_back( texHandle );
    }

    // The compositor may sample these images before anything renders into them
    if( !fba.colorTextureHandles.empty() && m_commandPool )
    {
        std::lock_guard<std::recursive_mutex> lock( m_deviceMutex );

        VkCommandBuffer initCmd = VK_NULL_HANDLE;

        if( allocateCommandBuffer( initCmd ) )
        {
            beginOneTimeCommands( initCmd );

            for( RHI_TextureHandle texHandle : fba.colorTextureHandles )
            {
                // A null handle marks a failed registration, and RhiHandleIndex({}) would alias slot 0
                if( !texHandle )
                    continue;

                auto texIt = m_textures.find( RhiHandleIndex( texHandle ) );

                if( texIt != m_textures.end() )
                {
                    TransitionTexture( initCmd, texIt->second,
                                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                       VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT );
                }
            }

            m_vk.EndCommandBuffer( initCmd );

            // The queue runs the transitions ahead of any pass that uses the images, so waiting
            // for them would only stall this thread behind every frame still queued
            if( submitCommands( initCmd ) == VK_SUCCESS )
            {
                m_submittedCmds.push_back( initCmd );
                m_submitSerial++;
            }
            else
            {
                wxLogTrace( traceRhi, wxS( "VulkanDevice: FBO init-transition submit failed" ) );
                m_vk.FreeCommandBuffers( m_device, m_commandPool, 1, &initCmd );
            }
        }
    }

    return handle;
}


void RHI_VULKAN_DEVICE::DestroyFramebuffer( RHI_FramebufferHandle handle )
{
    deferDestroy( m_framebufferPool, RHI_DEFERRED_RESOURCE_TYPE::FRAMEBUFFER, handle );
}


RHI_TextureHandle RHI_VULKAN_DEVICE::GetFramebufferColorTexture(
        RHI_FramebufferHandle handle, uint32_t attachmentIndex ) const
{
    if( !handle )
        return {};

    auto it = m_framebuffers.find( RhiHandleIndex( handle ) );

    if( it == m_framebuffers.end() )
        return {};

    if( attachmentIndex >= it->second.colorTextureHandles.size() )
        return {};

    return it->second.colorTextureHandles[attachmentIndex];
}


bool RHI_VULKAN_DEVICE::BeginRenderPass( const RHI_RENDER_PASS_BEGIN_DESC& desc )
{
    if( m_renderPassActive )
        return false;

    if( !m_renderPassPool.IsValid( desc.renderPass ) )
        return false;

    if( !m_framebufferPool.IsValid( desc.framebuffer ) )
        return false;

    auto rpIt = m_renderPasses.find( RhiHandleIndex( desc.renderPass ) );
    auto fbIt = m_framebuffers.find( RhiHandleIndex( desc.framebuffer ) );

    if( rpIt == m_renderPasses.end() || fbIt == m_framebuffers.end() )
        return false;

    RenderPassAllocation&  rpa = rpIt->second;
    FramebufferAllocation& fba = fbIt->second;

    // Command pools are externally synchronized, and this one is shared with main-thread uploads
    std::lock_guard<std::recursive_mutex> lock( m_deviceMutex );

    // A submitted buffer stays pending until the queue idles, so each pass takes its own
    if( !m_currentCmd && !acquirePassCmd( m_currentCmd ) )
        return false;

    if( beginOneTimeCommands( m_currentCmd ) != VK_SUCCESS )
        return false;

    // Every render pass leaves its depth attachment in the attachment layout
    if( fba.depthImage && rpa.depthStencilDesc.enabled )
        fba.depthRendered = true;

    std::vector<VkClearValue> clearValues;
    clearValues.reserve( rpa.colorDescs.size() + 1 );

    for( const auto& ca : rpa.colorDescs )
    {
        VkClearValue cv{};

        if( desc.hasClearColorOverride )
        {
            cv.color.float32[0] = desc.clearColorOverride[0];
            cv.color.float32[1] = desc.clearColorOverride[1];
            cv.color.float32[2] = desc.clearColorOverride[2];
            cv.color.float32[3] = desc.clearColorOverride[3];
        }
        else
        {
            cv.color.float32[0] = ca.clearColor[0];
            cv.color.float32[1] = ca.clearColor[1];
            cv.color.float32[2] = ca.clearColor[2];
            cv.color.float32[3] = ca.clearColor[3];
        }

        clearValues.push_back( cv );
    }

    if( rpa.depthStencilDesc.enabled
        && ( rpa.depthStencilDesc.depthLoadOp == RHI_LOAD_OP::CLEAR
             || rpa.depthStencilDesc.stencilLoadOp == RHI_LOAD_OP::CLEAR ) )
    {
        VkClearValue depthStencilClear{};
        depthStencilClear.depthStencil.depth = rpa.depthStencilDesc.clearDepth;
        depthStencilClear.depthStencil.stencil = rpa.depthStencilDesc.clearStencil;
        clearValues.push_back( depthStencilClear );
    }

    VkRenderPassBeginInfo rpBeginInfo{};
    rpBeginInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBeginInfo.renderPass      = rpa.renderPass;
    rpBeginInfo.framebuffer     = fba.framebuffer;
    rpBeginInfo.renderArea      = { { 0, 0 }, { fba.width, fba.height } };
    rpBeginInfo.clearValueCount = static_cast<uint32_t>( clearValues.size() );
    rpBeginInfo.pClearValues    = clearValues.data();

    m_vk.CmdBeginRenderPass( m_currentCmd, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE );

    m_renderPassActive = true;

    // BindPipeline skips a matching pipeline, so the first bind of each pass must not match
    m_activePipeline = {};

    return true;
}


bool RHI_VULKAN_DEVICE::EndRenderPass()
{
    std::lock_guard<std::recursive_mutex> lock( m_deviceMutex );

    if( !m_renderPassActive || !m_currentCmd )
        return false;

    m_vk.CmdEndRenderPass( m_currentCmd );
    m_vk.EndCommandBuffer( m_currentCmd );

    // No wait, as the queue runs passes in order and paths that need the GPU caught up call SyncSubmittedPasses()
    if( VkResult result = submitPassCmd( m_currentCmd ); result != VK_SUCCESS )
        wxLogTrace( traceRhi, wxS( "VulkanDevice: EndRenderPass submit=%d" ), result );

    m_currentCmd = VK_NULL_HANDLE;

    m_renderPassActive = false;
    m_activePipeline   = {};

    return true;
}


void RHI_VULKAN_DEVICE::SetViewport( const RHI_VIEWPORT& viewport )
{
    std::lock_guard<std::recursive_mutex> lock( m_deviceMutex );

    if( !m_renderPassActive || !m_currentCmd || !m_vk.CmdSetViewport )
        return;

    VkViewport vp{};
    vp.x        = viewport.x;
    vp.y        = viewport.y;
    vp.width    = viewport.width;
    vp.height   = viewport.height;
    vp.minDepth = viewport.minDepth;
    vp.maxDepth = viewport.maxDepth;

    m_vk.CmdSetViewport( m_currentCmd, 0, 1, &vp );
}


void RHI_VULKAN_DEVICE::SetScissor( const RHI_SCISSOR_RECT& scissor )
{
    std::lock_guard<std::recursive_mutex> lock( m_deviceMutex );

    if( !m_renderPassActive || !m_currentCmd || !m_vk.CmdSetScissor )
        return;

    VkRect2D rect{};
    rect.offset = { scissor.x, scissor.y };
    rect.extent = { scissor.width, scissor.height };

    m_vk.CmdSetScissor( m_currentCmd, 0, 1, &rect );
}


void RHI_VULKAN_DEVICE::BindPipeline( RHI_PipelineHandle pipeline )
{
    std::lock_guard<std::recursive_mutex> lock( m_deviceMutex );

    if( !m_renderPassActive || !m_currentCmd || !m_vk.CmdBindPipeline )
        return;

    if( !m_pipelinePool.IsValid( pipeline ) )
        return;

    if( pipeline == m_activePipeline )
        return;

    auto it = m_pipelines.find( RhiHandleIndex( pipeline ) );

    if( it == m_pipelines.end() )
        return;

    m_vk.CmdBindPipeline( m_currentCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, it->second.pipeline );
    m_activePipeline = pipeline;
}


void RHI_VULKAN_DEVICE::SetPushConstants( const void* data, size_t sizeBytes )
{
    std::lock_guard<std::recursive_mutex> lock( m_deviceMutex );

    if( !m_renderPassActive || !m_currentCmd || !m_vk.CmdPushConstants )
        return;

    if( !data || sizeBytes == 0 || sizeBytes > RHI_MAX_PUSH_CONSTANT_BYTES )
        return;

    if( !m_pipelinePool.IsValid( m_activePipeline ) )
        return;

    auto it = m_pipelines.find( RhiHandleIndex( m_activePipeline ) );

    if( it == m_pipelines.end() )
        return;

    m_vk.CmdPushConstants( m_currentCmd, it->second.layout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, static_cast<uint32_t>( sizeBytes ), data );
}


void RHI_VULKAN_DEVICE::BindVertexBuffer( uint32_t slot, RHI_BufferHandle buffer )
{
    std::lock_guard<std::recursive_mutex> lock( m_deviceMutex );

    if( !m_renderPassActive || !m_currentCmd || !m_vk.CmdBindVertexBuffers )
        return;

    if( !m_bufferPool.IsValid( buffer ) )
        return;

    auto it = m_buffers.find( RhiHandleIndex( buffer ) );

    if( it == m_buffers.end() )
        return;

    MarkBufferInUse( it->second );

    VkBuffer     vkBuf    = it->second.buffer;
    VkDeviceSize vkOffset = 0;

    m_vk.CmdBindVertexBuffers( m_currentCmd, slot, 1, &vkBuf, &vkOffset );
}


void RHI_VULKAN_DEVICE::BindIndexBuffer( RHI_BufferHandle buffer )
{
    std::lock_guard<std::recursive_mutex> lock( m_deviceMutex );

    if( !m_renderPassActive || !m_currentCmd || !m_vk.CmdBindIndexBuffer )
        return;

    if( !m_bufferPool.IsValid( buffer ) )
        return;

    auto it = m_buffers.find( RhiHandleIndex( buffer ) );

    if( it == m_buffers.end() )
        return;

    MarkBufferInUse( it->second );

    m_vk.CmdBindIndexBuffer( m_currentCmd, it->second.buffer, 0, VK_INDEX_TYPE_UINT16 );
}


void RHI_VULKAN_DEVICE::Draw( uint32_t vertexCount, uint32_t firstVertex )
{
    std::lock_guard<std::recursive_mutex> lock( m_deviceMutex );

    if( !m_renderPassActive || !m_currentCmd || !m_vk.CmdDraw )
        return;

    m_vk.CmdDraw( m_currentCmd, vertexCount, 1, firstVertex, 0 );
}


void RHI_VULKAN_DEVICE::DrawInstanced( uint32_t vertexCount, uint32_t instanceCount,
                                        uint32_t firstVertex, int32_t /*vertexOffset*/,
                                        uint32_t firstInstance )
{
    std::lock_guard<std::recursive_mutex> lock( m_deviceMutex );

    if( !m_renderPassActive || !m_currentCmd || !m_vk.CmdDraw )
        return;

    m_vk.CmdDraw( m_currentCmd, vertexCount, instanceCount, firstVertex, firstInstance );
}


void RHI_VULKAN_DEVICE::DrawIndexedInstanced( uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
                                               int32_t vertexOffset, uint32_t firstInstance )
{
    std::lock_guard<std::recursive_mutex> lock( m_deviceMutex );

    if( !m_renderPassActive || !m_currentCmd || !m_vk.CmdDrawIndexed )
        return;

    m_vk.CmdDrawIndexed( m_currentCmd, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance );
}


bool RHI_VULKAN_DEVICE::FindPresentQueueFamily( VkSurfaceKHR vkSurface, uint32_t& outFamily )
{
    if( !m_vk.GetPhysicalDeviceSurfaceSupportKHR )
        return false;

    // Present goes through the graphics queue, the only queue the device creates
    VkBool32 presentSupport = VK_FALSE;

    if( m_vk.GetPhysicalDeviceSurfaceSupportKHR( m_physicalDevice, m_graphicsQueueFamily, vkSurface,
                                                 &presentSupport ) != VK_SUCCESS
        || !presentSupport )
    {
        return false;
    }

    outFamily = m_graphicsQueueFamily;
    return true;
}


VkPipeline RHI_VULKAN_DEVICE::EnsurePresentBlit( VkFormat aSwapchainFormat )
{
    if( auto it = m_presentBlitPipelines.find( aSwapchainFormat ); it != m_presentBlitPipelines.end() )
        return it->second;

    if( !m_presentBlitDSLayout )
    {
        VkDescriptorSetLayoutBinding dsBinding{};
        dsBinding.binding         = 0;
        dsBinding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        dsBinding.descriptorCount = 1;
        dsBinding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo dsLayoutInfo{};
        dsLayoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        dsLayoutInfo.bindingCount = 1;
        dsLayoutInfo.pBindings    = &dsBinding;

        if( m_vk.CreateDescriptorSetLayout( m_device, &dsLayoutInfo, nullptr, &m_presentBlitDSLayout )
            != VK_SUCCESS )
        {
            m_presentBlitDSLayout = VK_NULL_HANDLE;
            return VK_NULL_HANDLE;
        }
    }

    if( !m_presentBlitLayout )
    {
        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = 1;
        layoutInfo.pSetLayouts    = &m_presentBlitDSLayout;

        if( m_vk.CreatePipelineLayout( m_device, &layoutInfo, nullptr, &m_presentBlitLayout ) != VK_SUCCESS )
        {
            m_presentBlitLayout = VK_NULL_HANDLE;
            return VK_NULL_HANDLE;
        }
    }

    VkShaderModuleCreateInfo vertModuleInfo{};
    vertModuleInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertModuleInfo.codeSize = k_present_blit_vert.spirvSize;
    vertModuleInfo.pCode    = reinterpret_cast<const uint32_t*>( k_present_blit_vert.spirv );

    VkShaderModule vertModule = VK_NULL_HANDLE;

    if( m_vk.CreateShaderModule( m_device, &vertModuleInfo, nullptr, &vertModule ) != VK_SUCCESS )
        return VK_NULL_HANDLE;

    VkShaderModuleCreateInfo fragModuleInfo{};
    fragModuleInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragModuleInfo.codeSize = k_present_blit_frag.spirvSize;
    fragModuleInfo.pCode    = reinterpret_cast<const uint32_t*>( k_present_blit_frag.spirv );

    VkShaderModule fragModule = VK_NULL_HANDLE;

    if( m_vk.CreateShaderModule( m_device, &fragModuleInfo, nullptr, &fragModule ) != VK_SUCCESS )
    {
        m_vk.DestroyShaderModule( m_device, vertModule, nullptr );
        return VK_NULL_HANDLE;
    }

    // Render pass compatible with the swapchain format
    const VkRenderPass pipelineRenderPass = makeRenderPass( { presentAttachment( aSwapchainFormat ) }, std::nullopt );

    if( !pipelineRenderPass )
    {
        m_vk.DestroyShaderModule( m_device, vertModule, nullptr );
        m_vk.DestroyShaderModule( m_device, fragModule, nullptr );
        return VK_NULL_HANDLE;
    }

    // The fullscreen triangle comes from gl_VertexIndex, so there is no vertex input, blending or depth
    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineColorBlendAttachmentState blendAtt{};
    blendAtt.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                              | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

    VkPipeline     pipeline = VK_NULL_HANDLE;
    const VkResult result = buildPipeline( vertModule, fragModule, vertexInput, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                                           { blendAtt }, depthStencil,
                                           { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR },
                                           m_presentBlitLayout, pipelineRenderPass, pipeline );

    m_vk.DestroyShaderModule( m_device, vertModule, nullptr );
    m_vk.DestroyShaderModule( m_device, fragModule, nullptr );
    m_vk.DestroyRenderPass( m_device, pipelineRenderPass, nullptr );

    if( result != VK_SUCCESS )
        return VK_NULL_HANDLE;

    m_presentBlitPipelines[aSwapchainFormat] = pipeline;

    wxLogTrace( traceRhi, wxS( "VulkanDevice: present blit pipeline created for format=%d" ),
                static_cast<int>( aSwapchainFormat ) );

    return pipeline;
}


void RHI_VULKAN_DEVICE::DestroyPresentBlit()
{
    for( const auto& [format, pipeline] : m_presentBlitPipelines )
        m_vk.DestroyPipeline( m_device, pipeline, nullptr );

    m_vk.DestroyPipelineLayout( m_device, m_presentBlitLayout, nullptr );
    m_vk.DestroyDescriptorSetLayout( m_device, m_presentBlitDSLayout, nullptr );

    m_presentBlitPipelines.clear();
    m_presentBlitLayout   = VK_NULL_HANDLE;
    m_presentBlitDSLayout = VK_NULL_HANDLE;
}


VkPresentModeKHR RHI_VULKAN_DEVICE::ChoosePresentMode( const SurfaceAllocation& aSurface )
{
    std::vector<RHI_PRESENT_MODE> offered;
    bool                          composited = aSurface.wayland;

#ifdef VK_USE_PLATFORM_XLIB_KHR
    // Asked on every swapchain build since a compositing manager can start or stop at any time
    composited = composited || aSurface.x11Owner.HasCompositor();
#endif

    if( m_vk.GetPhysicalDeviceSurfacePresentModesKHR )
    {
        uint32_t modeCount = 0;
        m_vk.GetPhysicalDeviceSurfacePresentModesKHR( m_physicalDevice, aSurface.surface, &modeCount, nullptr );
        std::vector<VkPresentModeKHR> modes( modeCount );
        m_vk.GetPhysicalDeviceSurfacePresentModesKHR( m_physicalDevice, aSurface.surface, &modeCount, modes.data() );

        for( VkPresentModeKHR mode : modes )
        {
            wxLogTrace( traceRhi, wxS( "VulkanDevice: present mode available=%d" ), static_cast<int>( mode ) );

            if( mode == VK_PRESENT_MODE_MAILBOX_KHR )
                offered.push_back( RHI_PRESENT_MODE::MAILBOX );
            else if( mode == VK_PRESENT_MODE_IMMEDIATE_KHR )
                offered.push_back( RHI_PRESENT_MODE::IMMEDIATE );
        }
    }

#if defined( __linux__ )
    const bool immediateUnderCompositor = true;
#else
    const bool immediateUnderCompositor = false;
#endif

    const RHI_PRESENT_MODE chosen = RhiChoosePresentMode( offered, composited, immediateUnderCompositor );

    wxLogTrace( traceRhi, wxS( "VulkanDevice: present mode %s (compositor %s)" ),
                std::string( magic_enum::enum_name( chosen ) ), composited ? "owns the present" : "absent" );

    switch( chosen )
    {
    case RHI_PRESENT_MODE::MAILBOX:   return VK_PRESENT_MODE_MAILBOX_KHR;
    case RHI_PRESENT_MODE::IMMEDIATE: return VK_PRESENT_MODE_IMMEDIATE_KHR;
    case RHI_PRESENT_MODE::FIFO:      break;
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}


bool RHI_VULKAN_DEVICE::CreateSwapchainResources( SwapchainAllocation& swa )
{
    if( !m_vk.GetSwapchainImagesKHR || !m_vk.CreateImageView )
        return false;

    uint32_t imageCount = 0;
    m_vk.GetSwapchainImagesKHR( m_device, swa.swapchain, &imageCount, nullptr );

    if( imageCount == 0 )
        return false;

    swa.images.resize( imageCount );
    m_vk.GetSwapchainImagesKHR( m_device, swa.swapchain, &imageCount, swa.images.data() );

    swa.imageViews.resize( imageCount );

    for( uint32_t i = 0; i < imageCount; i++ )
    {
        swa.imageViews[i] = createView( swa.images[i], swa.format, VK_IMAGE_ASPECT_COLOR_BIT );

        if( !swa.imageViews[i] )
            return false;
    }

    // Kept across resizes and destroyed with the swapchain, so only a larger image count adds semaphores
    VkSemaphoreCreateInfo semInfo{};
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    while( swa.renderFinished.size() < imageCount )
    {
        VkSemaphore semaphore = VK_NULL_HANDLE;

        if( !m_vk.CreateSemaphore || m_vk.CreateSemaphore( m_device, &semInfo, nullptr, &semaphore ) != VK_SUCCESS )
            return false;

        swa.renderFinished.push_back( semaphore );
    }

    // The present pass takes each swapchain image from UNDEFINED to PRESENT_SRC
    EnsurePresentBlit( swa.format );
    swa.presentRenderPass = makeRenderPass( { presentAttachment( swa.format ) }, std::nullopt );

    if( !swa.presentRenderPass )
    {
        return false;
    }

    swa.presentFramebuffers.resize( imageCount );

    for( uint32_t i = 0; i < imageCount; i++ )
    {
        VkFramebufferCreateInfo fbInfo{};
        fbInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass      = swa.presentRenderPass;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments    = &swa.imageViews[i];
        fbInfo.width           = swa.extent.width;
        fbInfo.height          = swa.extent.height;
        fbInfo.layers          = 1;

        if( m_vk.CreateFramebuffer( m_device, &fbInfo, nullptr,
                                     &swa.presentFramebuffers[i] ) != VK_SUCCESS )
        {
            return false;
        }
    }

    // The present blit has its own descriptor pool because rendering can exhaust the shared pool's samplers
    if( m_presentBlitDSLayout )
    {
        VkDescriptorPoolSize presentPoolSize{};
        presentPoolSize.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        presentPoolSize.descriptorCount = 1;

        VkDescriptorPoolCreateInfo presentPoolInfo{};
        presentPoolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        presentPoolInfo.maxSets       = 1;
        presentPoolInfo.poolSizeCount = 1;
        presentPoolInfo.pPoolSizes    = &presentPoolSize;

        if( m_vk.CreateDescriptorPool( m_device, &presentPoolInfo, nullptr,
                                        &swa.presentDescPool ) != VK_SUCCESS )
        {
            return false;
        }

        VkDescriptorSetAllocateInfo dsAlloc{};
        dsAlloc.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        dsAlloc.descriptorPool     = swa.presentDescPool;
        dsAlloc.descriptorSetCount = 1;
        dsAlloc.pSetLayouts        = &m_presentBlitDSLayout;

        if( m_vk.AllocateDescriptorSets( m_device, &dsAlloc,
                                          &swa.presentDescSet ) != VK_SUCCESS )
        {
            return false;
        }
    }

    return true;
}


void RHI_VULKAN_DEVICE::DestroySwapchainResources( SwapchainAllocation& swa, bool aKeepSwapchain )
{
    if( m_device && m_vk.DeviceWaitIdle )
        m_vk.DeviceWaitIdle( m_device );

    // Device idle does not cover the presentation engine's wait on renderFinished, which is destroyed
    // with the swapchain. Resizes reuse the semaphores, so only a full destroy waits.
    if( !aKeepSwapchain && m_presentWait && swa.swapchain && swa.lastPresentId > 0 )
        m_vk.WaitForPresentKHR( m_device, swa.swapchain, swa.lastPresentId, 100'000'000 );

    // A VkFramebuffer still references its image views, so it goes first
    for( VkFramebuffer fb : swa.presentFramebuffers )
        m_vk.DestroyFramebuffer( m_device, fb, nullptr );

    m_vk.DestroyRenderPass( m_device, swa.presentRenderPass, nullptr );

    for( VkImageView view : swa.imageViews )
        m_vk.DestroyImageView( m_device, view, nullptr );

    m_vk.DestroyDescriptorPool( m_device, swa.presentDescPool, nullptr );

    swa.presentFramebuffers.clear();
    swa.imageViews.clear();
    swa.presentRenderPass = VK_NULL_HANDLE;
    swa.presentDescPool   = VK_NULL_HANDLE;
    swa.presentDescSet    = VK_NULL_HANDLE;

    swa.images.clear();

    if( !aKeepSwapchain && swa.swapchain && m_vk.DestroySwapchainKHR )
    {
        m_vk.DestroySwapchainKHR( m_device, swa.swapchain, nullptr );
        swa.swapchain = VK_NULL_HANDLE;
    }
}


RHI_SurfaceHandle RHI_VULKAN_DEVICE::CreateSurface( const RHI_SURFACE_DESC& desc )
{
    if( !m_initialized || !desc.window.nativeHandle )
        return {};

    VkSurfaceKHR vkSurface      = VK_NULL_HANDLE;
    bool         waylandSurface = false;
#ifdef VK_USE_PLATFORM_XLIB_KHR
    X11_CONNECTION_OWNER ownedX11;

    if( vkSurface == VK_NULL_HANDLE && desc.window.nativeDisplay
        && ( desc.window.displayType == RHI_DISPLAY_TYPE::X11
             || desc.window.displayType == RHI_DISPLAY_TYPE::UNKNOWN )
        && ( m_vk.CreateXcbSurfaceKHR || m_vk.CreateXlibSurfaceKHR ) )
    {
        // GTK owns the UI thread's Display, so the render thread opens its own connection to the
        // widget's display, which need not be $DISPLAY
        Display*    uiDisplay   = static_cast<Display*>( desc.window.nativeDisplay );
        const char* displayName = ( desc.window.displayType == RHI_DISPLAY_TYPE::X11 )
                                          ? XDisplayString( uiDisplay )
                                          : nullptr;
        ::Window    window      = reinterpret_cast<::Window>( desc.window.nativeHandle );

#ifdef VK_USE_PLATFORM_XCB_KHR
        // Connect through xcb, never XOpenDisplay. Xlib connects under its process-wide lock,
        // and GDK takes that lock while holding a server grab, which deadlocks both threads.
        if( m_vk.CreateXcbSurfaceKHR )
        {
            xcb_connection_t* connection = xcb_connect( displayName, &ownedX11.screen );

            if( connection && !xcb_connection_has_error( connection ) )
            {
                VkXcbSurfaceCreateInfoKHR xcbInfo{};
                xcbInfo.sType      = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
                xcbInfo.connection = connection;
                xcbInfo.window     = static_cast<xcb_window_t>( window );

                VkResult xcbResult =
                        m_vk.CreateXcbSurfaceKHR( m_instance, &xcbInfo, nullptr, &vkSurface );

                wxLogTrace( traceRhi, wxS( "VulkanDevice: CreateXcbSurface conn=%p "
                                            "window=0x%x result=%d surface=%p" ),
                            static_cast<void*>( xcbInfo.connection ),
                            static_cast<unsigned>( xcbInfo.window ),
                            static_cast<int>( xcbResult ),
                            static_cast<void*>( vkSurface ) );
            }
            else
            {
                wxLogTrace( traceRhi, wxS( "VulkanDevice: CreateSurface xcb_connect failed" ) );
            }

            if( vkSurface != VK_NULL_HANDLE )
                ownedX11.xcb = connection;
            else if( connection )
                xcb_disconnect( connection );
        }
#endif

        // Only drivers without VK_KHR_xcb_surface get here, so the Xlib lock hazard remains
        if( vkSurface == VK_NULL_HANDLE && m_vk.CreateXlibSurfaceKHR )
        {
            Display* ownedDisplay   = XOpenDisplay( displayName );
            Display* surfaceDisplay = ownedDisplay ? ownedDisplay : uiDisplay;

            VkXlibSurfaceCreateInfoKHR xlibInfo{};
            xlibInfo.sType  = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
            xlibInfo.dpy    = surfaceDisplay;
            xlibInfo.window = window;

            VkResult xlibResult =
                    m_vk.CreateXlibSurfaceKHR( m_instance, &xlibInfo, nullptr, &vkSurface );
            wxLogTrace( traceRhi, wxS( "VulkanDevice: CreateXlibSurface dpy=%p window=0x%lx "
                                        "result=%d surface=%p" ),
                        static_cast<void*>( xlibInfo.dpy ),
                        static_cast<unsigned long>( xlibInfo.window ),
                        static_cast<int>( xlibResult ),
                        static_cast<void*>( vkSurface ) );

            if( vkSurface != VK_NULL_HANDLE && ownedDisplay )
            {
                ownedX11.xlib   = ownedDisplay;
                ownedX11.screen = DefaultScreen( ownedDisplay );
            }
            else if( ownedDisplay )
                XCloseDisplay( ownedDisplay );
        }
    }
#endif

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    if( vkSurface == VK_NULL_HANDLE && desc.window.nativeDisplay
        && ( desc.window.displayType == RHI_DISPLAY_TYPE::WAYLAND
             || desc.window.displayType == RHI_DISPLAY_TYPE::UNKNOWN )
        && m_vk.CreateWaylandSurfaceKHR )
    {
        VkWaylandSurfaceCreateInfoKHR waylandInfo{};
        waylandInfo.sType   = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
        waylandInfo.display = static_cast<struct wl_display*>( desc.window.nativeDisplay );
        waylandInfo.surface = static_cast<struct wl_surface*>( desc.window.nativeHandle );

        VkResult waylandResult =
                m_vk.CreateWaylandSurfaceKHR( m_instance, &waylandInfo, nullptr, &vkSurface );
        waylandSurface = vkSurface != VK_NULL_HANDLE;
        wxLogTrace( traceRhi, wxS( "VulkanDevice: CreateWaylandSurface display=%p surface=%p "
                                    "result=%d vkSurface=%p" ),
                    desc.window.nativeDisplay, desc.window.nativeHandle,
                    static_cast<int>( waylandResult ),
                    static_cast<void*>( vkSurface ) );
    }
#endif

#ifdef VK_USE_PLATFORM_WIN32_KHR
    if( vkSurface == VK_NULL_HANDLE
        && ( desc.window.displayType == RHI_DISPLAY_TYPE::WINDOWS
             || desc.window.displayType == RHI_DISPLAY_TYPE::UNKNOWN )
        && m_vk.CreateWin32SurfaceKHR )
    {
        VkWin32SurfaceCreateInfoKHR win32Info{};
        win32Info.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        win32Info.hinstance = GetModuleHandle( nullptr );
        win32Info.hwnd      = static_cast<HWND>( desc.window.nativeHandle );

        VkResult win32Result =
                m_vk.CreateWin32SurfaceKHR( m_instance, &win32Info, nullptr, &vkSurface );
        wxLogTrace( traceRhi, wxS( "VulkanDevice: CreateWin32Surface hwnd=%p "
                                    "result=%d vkSurface=%p" ),
                    desc.window.nativeHandle,
                    static_cast<int>( win32Result ),
                    static_cast<void*>( vkSurface ) );
    }
#endif

    if( vkSurface == VK_NULL_HANDLE )
        return {};

    uint32_t presentFamily = UINT32_MAX;

    // Only success keeps the render thread's X11 connection for teardown, so every failure closes it
    auto closeOwnedDisplay = [&]()
    {
#ifdef VK_USE_PLATFORM_XLIB_KHR
        ownedX11.Close();
#endif
    };

    if( !FindPresentQueueFamily( vkSurface, presentFamily ) )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: CreateSurface FAILED - no present queue family "
                                    "supports this surface" ) );

        if( m_vk.DestroySurfaceKHR )
            m_vk.DestroySurfaceKHR( m_instance, vkSurface, nullptr );

        closeOwnedDisplay();
        return {};
    }

    wxLogTrace( traceRhi, wxS( "VulkanDevice: CreateSurface presentFamily=%u graphicsFamily=%u" ),
                presentFamily, m_graphicsQueueFamily );

    RHI_SurfaceHandle handle = m_surfacePool.Allocate();

    if( !handle )
    {
        if( m_vk.DestroySurfaceKHR )
            m_vk.DestroySurfaceKHR( m_instance, vkSurface, nullptr );

        closeOwnedDisplay();
        return {};
    }

    SurfaceAllocation& sa = m_surfaces[RhiHandleIndex( handle )];
    sa.surface = vkSurface;
    sa.wayland = waylandSurface;
#ifdef VK_USE_PLATFORM_XLIB_KHR
    sa.x11Owner = ownedX11;
#endif

    wxLogTrace( traceRhi, wxS( "VulkanDevice: CreateSurface OK (handle=%u)" ),
                RhiHandleIndex( handle ) );

    return handle;
}


void RHI_VULKAN_DEVICE::DestroySurface( RHI_SurfaceHandle handle )
{
    if( !m_surfacePool.IsValid( handle ) )
        return;

    auto it = m_surfaces.find( RhiHandleIndex( handle ) );

    if( it == m_surfaces.end() )
        return;

    if( it->second.surface && m_vk.DestroySurfaceKHR )
        m_vk.DestroySurfaceKHR( m_instance, it->second.surface, nullptr );

#ifdef VK_USE_PLATFORM_XLIB_KHR
    it->second.x11Owner.Close();
#endif

    m_surfaces.erase( it );
    m_surfacePool.Release( handle );
}


static VkExtent2D swapchainExtent( const VkSurfaceCapabilitiesKHR& aCaps, uint32_t aWidth, uint32_t aHeight )
{
    if( aCaps.currentExtent.width != UINT32_MAX )
        return aCaps.currentExtent;

    return { std::max( aCaps.minImageExtent.width, std::min( aCaps.maxImageExtent.width, aWidth ) ),
             std::max( aCaps.minImageExtent.height, std::min( aCaps.maxImageExtent.height, aHeight ) ) };
}


RHI_VULKAN_DEVICE::VkSwapchainTry RHI_VULKAN_DEVICE::createVkSwapchain( const SurfaceAllocation&        aSurface,
                                                                        const VkSurfaceCapabilitiesKHR& aCaps,
                                                                        VkSurfaceFormatKHR aFormat, VkExtent2D aExtent,
                                                                        VkSwapchainKHR aOld,
                                                                        bool aResize )
{
    VkSwapchainTry made;

    uint32_t imageCount = aCaps.minImageCount + 1;

    if( aCaps.maxImageCount > 0 && imageCount > aCaps.maxImageCount )
        imageCount = aCaps.maxImageCount;

    VkPresentModeKHR presentMode = ChoosePresentMode( aSurface );

    VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    const VkCompositeAlphaFlagBitsKHR alphaPreference[] = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR
    };

    for( auto alpha : alphaPreference )
    {
        if( aCaps.supportedCompositeAlpha & alpha )
        {
            compositeAlpha = alpha;
            break;
        }
    }

    if( !aResize )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: compositeAlpha=0x%x" ),
                    static_cast<unsigned>( compositeAlpha ) );
    }

    VkSurfaceTransformFlagBitsKHR preTransform =
            ( aCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR )
                    ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
                    : aCaps.currentTransform;

    wxLogTrace( traceRhi, wxS( "VulkanDevice: %spreTransform current=0x%x chosen=0x%x "
                                "supported=0x%x" ),
                aResize ? "ResizeSwapchain " : "",
                static_cast<unsigned>( aCaps.currentTransform ),
                static_cast<unsigned>( preTransform ),
                static_cast<unsigned>( aCaps.supportedTransforms ) );

    VkSwapchainCreateInfoKHR swapInfo{};
    swapInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapInfo.surface          = aSurface.surface;
    swapInfo.minImageCount    = imageCount;
    swapInfo.imageFormat      = aFormat.format;
    swapInfo.imageColorSpace  = aFormat.colorSpace;
    swapInfo.imageExtent      = aExtent;
    swapInfo.imageArrayLayers = 1;
    swapInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                                | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    swapInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapInfo.preTransform     = preTransform;
    swapInfo.compositeAlpha   = compositeAlpha;
    swapInfo.presentMode      = presentMode;
    swapInfo.clipped          = VK_TRUE;
    swapInfo.oldSwapchain     = aOld;

    if( !aResize )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: swapInfo surface=%p minImages=%u format=%d "
                                    "colorSpace=%d extent=%ux%u layers=%u usage=0x%x "
                                    "sharingMode=%d preTransform=0x%x compositeAlpha=0x%x "
                                    "presentMode=%d clipped=%d oldSwapchain=%p" ),
                    static_cast<void*>( swapInfo.surface ),
                    swapInfo.minImageCount,
                    static_cast<int>( swapInfo.imageFormat ),
                    static_cast<int>( swapInfo.imageColorSpace ),
                    swapInfo.imageExtent.width, swapInfo.imageExtent.height,
                    swapInfo.imageArrayLayers,
                    swapInfo.imageUsage,
                    static_cast<int>( swapInfo.imageSharingMode ),
                    static_cast<unsigned>( swapInfo.preTransform ),
                    static_cast<unsigned>( swapInfo.compositeAlpha ),
                    static_cast<int>( swapInfo.presentMode ),
                    static_cast<int>( swapInfo.clipped ),
                    static_cast<void*>( swapInfo.oldSwapchain ) );
    }

    const char* who = aResize ? "ResizeSwapchain" : "CreateSwapchain";

    auto tryCreate = [&]( VkPresentModeKHR aMode, uint32_t aImageCount, const char* aLabel ) -> bool
    {
        swapInfo.presentMode   = aMode;
        swapInfo.minImageCount = aImageCount;

        wxLogTrace( traceRhi, wxS( "VulkanDevice: %s attempt=%s "
                                    "presentMode=%d minImages=%u" ),
                    who, aLabel, static_cast<int>( aMode ), aImageCount );

        made.result = m_vk.CreateSwapchainKHR( m_device, &swapInfo, nullptr, &made.swapchain );

        wxLogTrace( traceRhi, wxS( "VulkanDevice: %s result=%s(%d) "
                                    "attempt=%s" ),
                    who, VkResultToString( made.result ), static_cast<int>( made.result ),
                    aLabel );
        return made.result == VK_SUCCESS;
    };

    made.imageCount    = imageCount;
    made.preferredMode = presentMode;

    if( tryCreate( presentMode, imageCount, "preferred" )
        || ( presentMode != VK_PRESENT_MODE_FIFO_KHR
             && tryCreate( VK_PRESENT_MODE_FIFO_KHR, imageCount, "fifo" ) )
        || ( imageCount != aCaps.minImageCount
             && tryCreate( presentMode, aCaps.minImageCount, "min-images" ) )
        || ( presentMode != VK_PRESENT_MODE_FIFO_KHR
             && imageCount != aCaps.minImageCount
             && tryCreate( VK_PRESENT_MODE_FIFO_KHR, aCaps.minImageCount, "fifo-min-images" ) ) )
    {
        made.presentMode = swapInfo.presentMode;
    }

    return made;
}


RHI_SwapchainHandle RHI_VULKAN_DEVICE::CreateSwapchain( const RHI_SWAPCHAIN_DESC& desc )
{
    if( !m_initialized || !m_surfacePool.IsValid( desc.surface ) )
        return {};

    if( desc.width == 0 || desc.height == 0 )
        return {};

    auto surfIt = m_surfaces.find( RhiHandleIndex( desc.surface ) );

    if( surfIt == m_surfaces.end() )
        return {};

    VkSurfaceKHR vkSurface = surfIt->second.surface;

    if( !m_vk.GetPhysicalDeviceSurfaceCapabilitiesKHR
        || !m_vk.GetPhysicalDeviceSurfaceFormatsKHR
        || !m_vk.CreateSwapchainKHR )
    {
        return {};
    }

    VkSurfaceCapabilitiesKHR surfCaps{};

    if( m_vk.GetPhysicalDeviceSurfaceCapabilitiesKHR( m_physicalDevice, vkSurface,
                                                       &surfCaps ) != VK_SUCCESS )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: GetSurfaceCapabilities failed" ) );
        return {};
    }

    wxLogTrace( traceRhi, wxS( "VulkanDevice: surfCaps extent=%ux%u (min=%ux%u max=%ux%u) "
                                "requested=%ux%u supportedComposite=0x%x "
                                "supportedUsage=0x%x minImages=%u maxImages=%u" ),
                surfCaps.currentExtent.width, surfCaps.currentExtent.height,
                surfCaps.minImageExtent.width, surfCaps.minImageExtent.height,
                surfCaps.maxImageExtent.width, surfCaps.maxImageExtent.height,
                desc.width, desc.height,
                surfCaps.supportedCompositeAlpha,
                surfCaps.supportedUsageFlags,
                surfCaps.minImageCount, surfCaps.maxImageCount );

    uint32_t formatCount = 0;
    m_vk.GetPhysicalDeviceSurfaceFormatsKHR( m_physicalDevice, vkSurface,
                                              &formatCount, nullptr );

    if( formatCount == 0 )
        return {};

    std::vector<VkSurfaceFormatKHR> surfFormats( formatCount );
    m_vk.GetPhysicalDeviceSurfaceFormatsKHR( m_physicalDevice, vkSurface,
                                              &formatCount, surfFormats.data() );

    VkSurfaceFormatKHR chosenFormat = surfFormats[0];

    wxLogTrace( traceRhi, wxS( "VulkanDevice: %u surface formats available:" ), formatCount );

    for( uint32_t i = 0; i < formatCount; i++ )
    {
        wxLogTrace( traceRhi, wxS( "  format[%u]: format=%d colorSpace=%d" ),
                    i, static_cast<int>( surfFormats[i].format ),
                    static_cast<int>( surfFormats[i].colorSpace ) );
    }

    // R8G8B8A8 matches the compositor FBO, so the present blit needs no conversion
    static const VkFormat formatPrefs[] = {
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_B8G8R8A8_UNORM,
    };

    for( VkFormat preferred : formatPrefs )
    {
        bool found = false;

        for( const auto& fmt : surfFormats )
        {
            if( fmt.format == preferred
                && fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
            {
                chosenFormat = fmt;
                found = true;
                break;
            }
        }

        if( found )
            break;
    }

    wxLogTrace( traceRhi, wxS( "VulkanDevice: chosen format=%d colorSpace=%d" ),
                static_cast<int>( chosenFormat.format ),
                static_cast<int>( chosenFormat.colorSpace ) );

    const VkExtent2D     extent = swapchainExtent( surfCaps, desc.width, desc.height );
    const VkSwapchainTry made = createVkSwapchain( surfIt->second, surfCaps, chosenFormat, extent, VK_NULL_HANDLE,
                                                   false );

    if( made.result != VK_SUCCESS )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: vkCreateSwapchainKHR failed (%d)" ),
                    static_cast<int>( made.result ) );
        return {};
    }

    VkSwapchainKHR vkSwapchain = made.swapchain;

    wxLogTrace( traceRhi, wxS( "VulkanDevice: swapchain created %ux%u format=%d images=%u "
                                "present=%d" ),
                extent.width, extent.height, static_cast<int>( chosenFormat.format ),
                made.imageCount, static_cast<int>( made.preferredMode ) );

    RHI_SwapchainHandle handle = m_swapchainPool.Allocate();

    if( !handle )
    {
        m_vk.DestroySwapchainKHR( m_device, vkSwapchain, nullptr );
        return {};
    }

    SwapchainAllocation& swa = m_swapchains[RhiHandleIndex( handle )];
    swa.swapchain = vkSwapchain;
    swa.surface   = desc.surface;
    swa.format    = chosenFormat.format;
    swa.colorSpace = chosenFormat.colorSpace;
    swa.extent    = extent;
    swa.requestedExtent = { desc.width, desc.height };
    swa.waitingForSurfaceExtent = surfCaps.currentExtent.width != UINT32_MAX
                                  && ( extent.width != desc.width || extent.height != desc.height );
    swa.dpiScale  = desc.dpiScale;

    // The attempt that succeeded may have fallen back to FIFO
    swa.presentMode = made.presentMode;

    if( !CreateSwapchainResources( swa ) )
    {
        destroySwapchainObjects( swa );
        m_swapchains.erase( RhiHandleIndex( handle ) );
        m_swapchainPool.Release( handle );
        return {};
    }

    VkSemaphoreCreateInfo semInfo{};
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if( !m_vk.CreateSemaphore || !m_vk.CreateFence
        || m_vk.CreateSemaphore( m_device, &semInfo, nullptr,
                                  &swa.imageAvailable ) != VK_SUCCESS
        || m_vk.CreateFence( m_device, &fenceInfo, nullptr,
                              &swa.inFlightFence ) != VK_SUCCESS )
    {
        destroySwapchainObjects( swa );
        m_swapchains.erase( RhiHandleIndex( handle ) );
        m_swapchainPool.Release( handle );
        return {};
    }

    return handle;
}


bool RHI_VULKAN_DEVICE::ResizeSwapchain( RHI_SwapchainHandle handle, uint32_t width,
                                          uint32_t height, float dpiScale )
{
    wxLogTrace( traceRhi, wxS( "VulkanDevice: ResizeSwapchain %ux%u dpi=%.1f" ),
                width, height, dpiScale );

    if( !m_swapchainPool.IsValid( handle ) || width == 0 || height == 0 )
        return false;

    auto it = m_swapchains.find( RhiHandleIndex( handle ) );

    if( it == m_swapchains.end() )
        return false;

    SwapchainAllocation& swa = it->second;
    swa.requestedExtent = { width, height };

    if( !m_surfacePool.IsValid( swa.surface ) )
        return false;

    auto surfIt = m_surfaces.find( RhiHandleIndex( swa.surface ) );

    if( surfIt == m_surfaces.end() )
        return false;

    VkSurfaceKHR vkSurface = surfIt->second.surface;

    VkSurfaceCapabilitiesKHR surfCaps{};
    VkResult capsResult = m_vk.GetPhysicalDeviceSurfaceCapabilitiesKHR(
            m_physicalDevice, vkSurface, &surfCaps );

    if( capsResult != VK_SUCCESS )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: ResizeSwapchain GetSurfaceCaps FAILED (%d)" ),
                    static_cast<int>( capsResult ) );
        return false;
    }

    const VkExtent2D extent = swapchainExtent( surfCaps, width, height );

    wxLogTrace( traceRhi, wxS( "VulkanDevice: ResizeSwapchain surfExtent=%ux%u "
                                "requested=%ux%u minExtent=%ux%u maxExtent=%ux%u" ),
                extent.width, extent.height, width, height,
                surfCaps.minImageExtent.width, surfCaps.minImageExtent.height,
                surfCaps.maxImageExtent.width, surfCaps.maxImageExtent.height );

    if( extent.width == 0 || extent.height == 0 )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: ResizeSwapchain SKIPPED (zero extent)" ) );
        return false;
    }

    // Keep the swapchain so it can be passed as oldSwapchain
    DestroySwapchainResources( swa, true );

    VkSwapchainKHR oldSwapchain = swa.swapchain;

    const VkSwapchainTry made = createVkSwapchain( surfIt->second, surfCaps,
                                                   { swa.format, swa.colorSpace }, extent,
                                                   oldSwapchain, true );

    if( made.result != VK_SUCCESS )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: ResizeSwapchain CreateSwapchain FAILED (%d) "
                                    "extent=%ux%u" ),
                    static_cast<int>( made.result ), extent.width, extent.height );
        return false;
    }

    m_vk.DestroySwapchainKHR( m_device, oldSwapchain, nullptr );

    swa.swapchain = made.swapchain;
    swa.presentMode = made.presentMode;
    swa.lastPresentId = 0;
    swa.extent    = extent;
    swa.dpiScale  = dpiScale;

    bool resourcesOk = CreateSwapchainResources( swa );

    wxLogTrace( traceRhi, wxS( "VulkanDevice: ResizeSwapchain %s extent=%ux%u images=%zu" ),
                resourcesOk ? "OK" : "RESOURCES_FAILED",
                extent.width, extent.height, swa.images.size() );

    // Release what a failed CreateSwapchainResources left behind, keeping the swapchain handle
    if( !resourcesOk )
        DestroySwapchainResources( swa, /* aKeepSwapchain */ true );
    else
    {
        swa.needsResize = false;
        swa.waitingForSurfaceExtent = surfCaps.currentExtent.width != UINT32_MAX
                                      && ( extent.width != width || extent.height != height );
    }

    if( swa.waitingForSurfaceExtent )
        swa.nextSurfaceExtentCheck = {};

    return resourcesOk;
}


RHI_SURFACE_EXTENT_POLL_RESULT RHI_VULKAN_DEVICE::PollSurfaceExtent( RHI_SwapchainHandle aSwapchain )
{
    if( !m_swapchainPool.IsValid( aSwapchain ) )
        return RHI_SURFACE_EXTENT_POLL_RESULT::IDLE;

    auto it = m_swapchains.find( RhiHandleIndex( aSwapchain ) );

    if( it == m_swapchains.end() || !it->second.waitingForSurfaceExtent )
        return RHI_SURFACE_EXTENT_POLL_RESULT::IDLE;

    SwapchainAllocation& swa = it->second;
    auto now = std::chrono::steady_clock::now();

    if( now < swa.nextSurfaceExtentCheck )
        return RHI_SURFACE_EXTENT_POLL_RESULT::WAITING;

    swa.nextSurfaceExtentCheck = now + std::chrono::milliseconds( 50 );

    std::lock_guard<std::recursive_mutex> lock( m_deviceMutex );
    auto surfIt = m_surfaces.find( RhiHandleIndex( swa.surface ) );

    if( surfIt == m_surfaces.end() )
        return RHI_SURFACE_EXTENT_POLL_RESULT::FAILED;

    VkSurfaceCapabilitiesKHR caps{};
    VkResult capsResult = m_vk.GetPhysicalDeviceSurfaceCapabilitiesKHR(
            m_physicalDevice, surfIt->second.surface, &caps );

    if( capsResult != VK_SUCCESS )
        return RHI_SURFACE_EXTENT_POLL_RESULT::FAILED;

    if( caps.currentExtent.width != UINT32_MAX
        && ( caps.currentExtent.width != swa.requestedExtent.width
             || caps.currentExtent.height != swa.requestedExtent.height ) )
    {
        return RHI_SURFACE_EXTENT_POLL_RESULT::WAITING;
    }

    wxLogTrace( traceRhi, wxS( "VulkanDevice: surface reached requested extent %ux%u; "
                                "resizing swapchain" ),
                swa.requestedExtent.width, swa.requestedExtent.height );

    if( !ResizeSwapchain( aSwapchain, swa.requestedExtent.width,
                          swa.requestedExtent.height, swa.dpiScale ) )
    {
        return RHI_SURFACE_EXTENT_POLL_RESULT::FAILED;
    }

    return swa.waitingForSurfaceExtent ? RHI_SURFACE_EXTENT_POLL_RESULT::WAITING
                                       : RHI_SURFACE_EXTENT_POLL_RESULT::RESIZED;
}


void RHI_VULKAN_DEVICE::DestroySwapchain( RHI_SwapchainHandle handle )
{
    if( !m_swapchainPool.IsValid( handle ) )
        return;

    auto it = m_swapchains.find( RhiHandleIndex( handle ) );

    if( it == m_swapchains.end() )
        return;

    destroySwapchainObjects( it->second );

    m_swapchains.erase( it );
    m_swapchainPool.Release( handle );
}


void RHI_VULKAN_DEVICE::destroySwapchainObjects( SwapchainAllocation& aSwa )
{
    DestroySwapchainResources( aSwa );

    // The destructor gets here after the pool is gone, which already freed the buffer
    if( aSwa.lastPresentCmd && m_commandPool && m_vk.FreeCommandBuffers )
    {
        std::lock_guard<std::recursive_mutex> lock( m_deviceMutex );
        m_vk.FreeCommandBuffers( m_device, m_commandPool, 1, &aSwa.lastPresentCmd );
    }

    aSwa.lastPresentCmd = VK_NULL_HANDLE;

    m_vk.DestroySemaphore( m_device, aSwa.imageAvailable, nullptr );
    m_vk.DestroyFence( m_device, aSwa.inFlightFence, nullptr );

    for( VkSemaphore semaphore : aSwa.renderFinished )
        m_vk.DestroySemaphore( m_device, semaphore, nullptr );

    aSwa.imageAvailable = VK_NULL_HANDLE;
    aSwa.inFlightFence  = VK_NULL_HANDLE;
    aSwa.renderFinished.clear();
}


bool RHI_VULKAN_DEVICE::WaitForPresentSlot( RHI_SwapchainHandle aSwapchain, std::chrono::milliseconds aTimeout,
                                            int aAllowed )
{
    auto it = m_swapchains.find( RhiHandleIndex( aSwapchain ) );

    if( !m_swapchainPool.IsValid( aSwapchain ) || it == m_swapchains.end() )
        return false;

    SwapchainAllocation& swa = it->second;
    const uint64_t       timeoutNs =
            static_cast<uint64_t>( std::chrono::duration_cast<std::chrono::nanoseconds>( aTimeout ).count() );

    // With present ids, wait until all but the allowed presents are on the display. Without them, wait for
    // the GPU to finish the last frame so the CPU cannot run ahead.
    const uint64_t allowed = static_cast<uint64_t>( std::max( aAllowed, 1 ) );

    if( !swa.swapchain || swa.lastPresentId == 0 )
        return false;

    if( m_presentWait )
    {
        if( swa.lastPresentId < allowed )
            return false;

        const uint64_t waitFor = swa.lastPresentId + 1 - allowed;
        const VkResult result = m_vk.WaitForPresentKHR( m_device, swa.swapchain, waitFor, timeoutNs );

        // An out of date or lost swapchain is reported by the next present
        return result == VK_SUCCESS || result == VK_TIMEOUT;
    }

    if( !swa.inFlightFence || !m_vk.WaitForFences )
        return false;

    return m_vk.WaitForFences( m_device, 1, &swa.inFlightFence, VK_TRUE, timeoutNs ) == VK_SUCCESS;
}


bool RHI_VULKAN_DEVICE::PresentWaitsForDisplay( RHI_SwapchainHandle aSwapchain ) const
{
    auto it = m_swapchains.find( RhiHandleIndex( aSwapchain ) );

    return it == m_swapchains.end() || it->second.presentMode == VK_PRESENT_MODE_FIFO_KHR
           || it->second.presentMode == VK_PRESENT_MODE_FIFO_RELAXED_KHR;
}


bool RHI_VULKAN_DEVICE::PresentReplacesQueued( RHI_SwapchainHandle aSwapchain ) const
{
    auto it = m_swapchains.find( RhiHandleIndex( aSwapchain ) );

    return it != m_swapchains.end() && it->second.presentMode == VK_PRESENT_MODE_MAILBOX_KHR;
}


RHI_FRAME_OP_RESULT RHI_VULKAN_DEVICE::Present( RHI_SwapchainHandle handle, RHI_FramebufferHandle aSrcFBO,
                                                const RHI_SCISSOR_RECT* aSourceRect )
{
    if( !m_swapchainPool.IsValid( handle ) )
        return RHI_FRAME_OP_RESULT::ERROR;

    auto it = m_swapchains.find( RhiHandleIndex( handle ) );

    if( it == m_swapchains.end() )
        return RHI_FRAME_OP_RESULT::ERROR;

    SwapchainAllocation& swa = it->second;
    uint64_t attempt = ++swa.presentAttemptCount;

    wxLogTrace( traceRhi, wxS( "VulkanDevice: Present begin attempt=%llu handle=%u "
                                "extent=%ux%u srcFBO=%s needsResize=%s" ),
                static_cast<unsigned long long>( attempt ),
                static_cast<unsigned>( RhiHandleIndex( handle ) ),
                swa.extent.width, swa.extent.height,
                aSrcFBO ? "valid" : "null",
                swa.needsResize ? "yes" : "no" );

    VkImage srcImage = VK_NULL_HANDLE;

    if( aSrcFBO )
    {
        auto fbIt = m_framebuffers.find( RhiHandleIndex( aSrcFBO ) );

        if( fbIt != m_framebuffers.end() && !fbIt->second.colorImages.empty() )
            srcImage = fbIt->second.colorImages[0];
    }

    if( !m_vk.AcquireNextImageKHR || !m_vk.QueuePresentKHR
        || !m_vk.WaitForFences || !m_vk.ResetFences )
    {
        return RHI_FRAME_OP_RESULT::ERROR;
    }

    // A failed resize can leave a valid swapchain with no images, which the caller must recreate
    if( swa.images.empty() )
        return RHI_FRAME_OP_RESULT::OUT_OF_DATE;

    PROF_TIMER waitTimer;
    PROF_TIMER waitLogTimer;
    VkResult   waitResult = VK_TIMEOUT;

    while( waitResult == VK_TIMEOUT )
    {
        waitResult = m_vk.WaitForFences( m_device, 1, &swa.inFlightFence, VK_TRUE,
                                         250000000ULL );

        if( waitResult == VK_TIMEOUT && VkElapsedMs( waitLogTimer ) >= 1000 )
        {
            wxLogTrace( traceRhi, wxS( "VulkanDevice: WaitForFences still waiting "
                                        "attempt=%llu elapsed=%lldms" ),
                        static_cast<unsigned long long>( attempt ),
                        VkElapsedMs( waitTimer ) );
            waitLogTimer.Start();
        }
    }

    waitTimer.Stop();

    wxLogTrace( traceRhi, wxS( "VulkanDevice: WaitForFences attempt=%llu result=%s(%d) "
                                "elapsed=%lldms" ),
                static_cast<unsigned long long>( attempt ),
                VkResultToString( waitResult ), static_cast<int>( waitResult ),
                VkElapsedMs( waitTimer ) );

    if( waitResult != VK_SUCCESS )
        return RHI_FRAME_OP_RESULT::ERROR;

    // Held through QueuePresentKHR because the main thread shares the command pool and queue
    std::unique_lock<std::recursive_mutex> presentLock( m_deviceMutex );

    const VkPipeline blitPipeline = EnsurePresentBlit( swa.format );

    // Free the previous frame's command buffer now that the GPU is done with it
    if( swa.lastPresentCmd )
    {
        m_vk.FreeCommandBuffers( m_device, m_commandPool, 1, &swa.lastPresentCmd );
        swa.lastPresentCmd = VK_NULL_HANDLE;
    }

    // Allocate before acquiring so a failure cannot strand an acquired image with a signaled semaphore
    VkCommandBuffer cmd = VK_NULL_HANDLE;

    if( !allocateCommandBuffer( cmd ) )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: AllocateCommandBuffers FAILED "
                                    "attempt=%llu" ),
                    static_cast<unsigned long long>( attempt ) );
        return RHI_FRAME_OP_RESULT::ERROR;
    }

    if( swa.needsResize )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: Present deferred resize %ux%u" ),
                    swa.extent.width, swa.extent.height );

        bool resizeOk = ResizeSwapchain( handle, swa.requestedExtent.width,
                                         swa.requestedExtent.height, swa.dpiScale );
        swa.needsResize = false;

        // A failed resize leaves no images to acquire, so the caller must recreate the swapchain
        if( !resizeOk || swa.images.empty() )
        {
            m_vk.FreeCommandBuffers( m_device, m_commandPool, 1, &cmd );
            return RHI_FRAME_OP_RESULT::OUT_OF_DATE;
        }
    }

    uint32_t imageIndex = 0;
    PROF_TIMER acquireTimer;
    VkResult acquireResult = m_vk.AcquireNextImageKHR( m_device, swa.swapchain, UINT64_MAX,
                                                       swa.imageAvailable, VK_NULL_HANDLE,
                                                       &imageIndex );
    acquireTimer.Stop();

    wxLogTrace( traceRhi, wxS( "VulkanDevice: AcquireNextImage attempt=%llu result=%s(%d) "
                                "image=%u elapsed=%lldms" ),
                static_cast<unsigned long long>( attempt ),
                VkResultToString( acquireResult ), static_cast<int>( acquireResult ),
                imageIndex, VkElapsedMs( acquireTimer ) );

    if( acquireResult == VK_ERROR_OUT_OF_DATE_KHR )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: AcquireNextImage out-of-date, "
                                    "resizing swapchain" ) );

        if( !ResizeSwapchain( handle, swa.requestedExtent.width,
                              swa.requestedExtent.height, swa.dpiScale ) )
        {
            wxLogTrace( traceRhi, wxS( "VulkanDevice: ResizeSwapchain after OUT_OF_DATE FAILED" ) );
            m_vk.FreeCommandBuffers( m_device, m_commandPool, 1, &cmd );
            return RHI_FRAME_OP_RESULT::OUT_OF_DATE;
        }

        acquireTimer.Start();
        acquireResult = m_vk.AcquireNextImageKHR( m_device, swa.swapchain, UINT64_MAX,
                                                  swa.imageAvailable, VK_NULL_HANDLE,
                                                  &imageIndex );
        acquireTimer.Stop();

        if( acquireResult != VK_SUCCESS )
        {
            wxLogTrace( traceRhi, wxS( "VulkanDevice: AcquireNextImage retry FAILED "
                                        "attempt=%llu result=%s(%d) image=%u elapsed=%lldms" ),
                        static_cast<unsigned long long>( attempt ),
                        VkResultToString( acquireResult ), static_cast<int>( acquireResult ),
                        imageIndex, VkElapsedMs( acquireTimer ) );
            m_vk.FreeCommandBuffers( m_device, m_commandPool, 1, &cmd );
            return RHI_FRAME_OP_RESULT::OUT_OF_DATE;
        }

        wxLogTrace( traceRhi, wxS( "VulkanDevice: AcquireNextImage retry OK "
                                    "attempt=%llu image=%u elapsed=%lldms" ),
                    static_cast<unsigned long long>( attempt ),
                    imageIndex, VkElapsedMs( acquireTimer ) );
    }
    else if( acquireResult == VK_SUBOPTIMAL_KHR )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: AcquireNextImage suboptimal "
                                    "attempt=%llu (continuing)" ),
                    static_cast<unsigned long long>( attempt ) );
    }
    else if( acquireResult != VK_SUCCESS )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: AcquireNextImage FAILED attempt=%llu "
                                    "result=%s(%d)" ),
                    static_cast<unsigned long long>( attempt ),
                    VkResultToString( acquireResult ), static_cast<int>( acquireResult ) );
        m_vk.FreeCommandBuffers( m_device, m_commandPool, 1, &cmd );

        if( acquireResult == VK_ERROR_DEVICE_LOST )
            return RHI_FRAME_OP_RESULT::DEVICE_LOST;

        if( acquireResult == VK_ERROR_SURFACE_LOST_KHR )
            return RHI_FRAME_OP_RESULT::SURFACE_LOST;

        return RHI_FRAME_OP_RESULT::ERROR;
    }

    // Reset the fence only after acquire succeeds, or an early exit would leave it never signaled
    m_vk.ResetFences( m_device, 1, &swa.inFlightFence );

    VkSemaphore renderFinished = swa.renderFinished[imageIndex];

    swa.currentImageIndex = imageIndex;

    beginOneTimeCommands( cmd );

    // Sample the FBO in a render pass because vkCmdBlitImage and vkCmdCopyImage intermittently read
    // zeros on some NVIDIA drivers
    bool presentBlitOk = false;

    if( srcImage && blitPipeline && swa.presentRenderPass
        && swa.presentDescSet && imageIndex < swa.presentFramebuffers.size() )
    {
        auto fbIt = m_framebuffers.find( RhiHandleIndex( aSrcFBO ) );
        RHI_TextureHandle srcTexHandle;

        if( fbIt != m_framebuffers.end() && !fbIt->second.colorTextureHandles.empty() )
            srcTexHandle = fbIt->second.colorTextureHandles[0];

        auto texIt = srcTexHandle ? m_textures.find( RhiHandleIndex( srcTexHandle ) )
                                  : m_textures.end();

        if( texIt != m_textures.end() )
        {
            // NVIDIA may not flush attachment writes for texture reads in a later submission, so a
            // barrier forces it before sampling
            const VkImageMemoryBarrier fboBarrier = imageBarrier(
                    srcImage, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,
                    VK_ACCESS_SHADER_READ_BIT );

            m_vk.CmdPipelineBarrier( cmd,
                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                      0, 0, nullptr, 0, nullptr, 1, &fboBarrier );

            // The fence wait above means the previous frame no longer reads this descriptor set
            VkDescriptorImageInfo imgInfo{};
            imgInfo.sampler     = texIt->second.sampler;
            imgInfo.imageView   = texIt->second.imageView;
            imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkWriteDescriptorSet write{};
            write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet          = swa.presentDescSet;
            write.dstBinding      = 0;
            write.descriptorCount = 1;
            write.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write.pImageInfo      = &imgInfo;

            m_vk.UpdateDescriptorSets( m_device, 1, &write, 0, nullptr );

            VkRenderPassBeginInfo rpBegin{};
            rpBegin.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            rpBegin.renderPass        = swa.presentRenderPass;
            rpBegin.framebuffer       = swa.presentFramebuffers[imageIndex];
            rpBegin.renderArea.extent = swa.extent;

            m_vk.CmdBeginRenderPass( cmd, &rpBegin, VK_SUBPASS_CONTENTS_INLINE );

            VkViewport vp{};
            vp.width    = static_cast<float>( swa.extent.width );
            vp.height   = static_cast<float>( swa.extent.height );
            vp.maxDepth = 1.0f;

            // The blit draws the whole source across the viewport, so a part of it is shown by
            // growing the viewport until that part covers the image. The scissor clips the rest.
            if( aSourceRect && aSourceRect->width > 0 && aSourceRect->height > 0 )
            {
                const float scaleX = vp.width / static_cast<float>( aSourceRect->width );
                const float scaleY = vp.height / static_cast<float>( aSourceRect->height );

                vp.x      = -static_cast<float>( aSourceRect->x ) * scaleX;
                vp.y      = -static_cast<float>( aSourceRect->y ) * scaleY;
                vp.width  = static_cast<float>( fbIt->second.width ) * scaleX;
                vp.height = static_cast<float>( fbIt->second.height ) * scaleY;
            }

            m_vk.CmdSetViewport( cmd, 0, 1, &vp );

            VkRect2D scissor{};
            scissor.extent = swa.extent;
            m_vk.CmdSetScissor( cmd, 0, 1, &scissor );

            m_vk.CmdBindPipeline( cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, blitPipeline );

            m_vk.CmdBindDescriptorSets( cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                         m_presentBlitLayout,
                                         0, 1, &swa.presentDescSet, 0, nullptr );

            m_vk.CmdDraw( cmd, 3, 1, 0, 0 );

            m_vk.CmdEndRenderPass( cmd );

            presentBlitOk = true;
        }
    }

    // With no source texture or the pipeline not ready, clear to magenta
    if( !presentBlitOk )
    {
        const VkImageMemoryBarrier toDst = imageBarrier( swa.images[imageIndex], VK_IMAGE_ASPECT_COLOR_BIT,
                                                         VK_IMAGE_LAYOUT_UNDEFINED,
                                                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0,
                                                         VK_ACCESS_TRANSFER_WRITE_BIT );

        m_vk.CmdPipelineBarrier( cmd,
                                  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                  VK_PIPELINE_STAGE_TRANSFER_BIT,
                                  0, 0, nullptr, 0, nullptr, 1, &toDst );

        VkClearColorValue magenta = { { 1.0f, 0.0f, 1.0f, 1.0f } };
        m_vk.CmdClearColorImage( cmd, swa.images[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &magenta, 1,
                                 &toDst.subresourceRange );

        const VkImageMemoryBarrier toPresent = imageBarrier( swa.images[imageIndex], VK_IMAGE_ASPECT_COLOR_BIT,
                                                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                             VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                                             VK_ACCESS_TRANSFER_WRITE_BIT, 0 );

        m_vk.CmdPipelineBarrier( cmd,
                                  VK_PIPELINE_STAGE_TRANSFER_BIT,
                                  VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                  0, 0, nullptr, 0, nullptr, 1, &toPresent );
    }

    m_vk.EndCommandBuffer( cmd );

    // Waiting only at COLOR_ATTACHMENT_OUTPUT lets the fragment shader read a stale FBO on NVIDIA
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    VkSubmitInfo submitInfo{};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = &swa.imageAvailable;
    submitInfo.pWaitDstStageMask    = &waitStage;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &cmd;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = &renderFinished;

    PROF_TIMER submitTimer;
    VkResult submitResult = m_vk.QueueSubmit( m_graphicsQueue, 1, &submitInfo,
                                              swa.inFlightFence );
    submitTimer.Stop();

    wxLogTrace( traceRhi, wxS( "VulkanDevice: QueueSubmit attempt=%llu result=%s(%d) "
                                "image=%u elapsed=%lldms" ),
                static_cast<unsigned long long>( attempt ),
                VkResultToString( submitResult ), static_cast<int>( submitResult ),
                imageIndex, VkElapsedMs( submitTimer ) );

    if( submitResult != VK_SUCCESS )
    {
        m_vk.FreeCommandBuffers( m_device, m_commandPool, 1, &cmd );

        // The fence was reset but never submitted, so signal it again or the next wait deadlocks
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VkFence newFence = VK_NULL_HANDLE;

        if( m_vk.CreateFence( m_device, &fenceInfo, nullptr, &newFence ) == VK_SUCCESS )
        {
            m_vk.DestroyFence( m_device, swa.inFlightFence, nullptr );
            swa.inFlightFence = newFence;
        }

        if( submitResult == VK_ERROR_DEVICE_LOST )
            return RHI_FRAME_OP_RESULT::DEVICE_LOST;

        // No submit consumed the acquired image and its signaled semaphore, so the caller must recreate
        // the swapchain rather than acquire against that state
        return RHI_FRAME_OP_RESULT::OUT_OF_DATE;
    }


    VkPresentInfoKHR presentInfo{};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &renderFinished;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &swa.swapchain;
    presentInfo.pImageIndices      = &imageIndex;

    VkPresentIdKHR presentId{};
    const uint64_t thisPresent = swa.lastPresentId + 1;

    if( m_presentWait )
    {
        presentId.sType          = VK_STRUCTURE_TYPE_PRESENT_ID_KHR;
        presentId.swapchainCount = 1;
        presentId.pPresentIds    = &thisPresent;
        presentInfo.pNext        = &presentId;
    }

    PROF_TIMER presentTimer;
    VkResult presentResult = m_vk.QueuePresentKHR( m_graphicsQueue, &presentInfo );
    presentTimer.Stop();

    if( presentResult == VK_SUCCESS || presentResult == VK_SUBOPTIMAL_KHR )
    {
        swa.lastPresentId = thisPresent;
    }

    swa.lastPresentCmd = cmd;

    // Past the last queue call, so the main thread need not wait while stats are logged
    presentLock.unlock();

    wxLogTrace( traceRhi, wxS( "VulkanDevice: QueuePresent attempt=%llu result=%s(%d) "
                                "image=%u elapsed=%lldms" ),
                static_cast<unsigned long long>( attempt ),
                VkResultToString( presentResult ), static_cast<int>( presentResult ),
                imageIndex, VkElapsedMs( presentTimer ) );

    if( presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: QueuePresent %s (will resize next frame)" ),
                    presentResult == VK_ERROR_OUT_OF_DATE_KHR ? "out-of-date" : "suboptimal" );
        swa.needsResize = true;
        return RHI_FRAME_OP_RESULT::OK;
    }

    if( presentResult == VK_ERROR_DEVICE_LOST )
        return RHI_FRAME_OP_RESULT::DEVICE_LOST;

    if( presentResult == VK_ERROR_SURFACE_LOST_KHR )
        return RHI_FRAME_OP_RESULT::SURFACE_LOST;

    if( presentResult != VK_SUCCESS )
        return RHI_FRAME_OP_RESULT::ERROR;

    if( swa.extent.width != swa.lastLoggedExtent.width
        || swa.extent.height != swa.lastLoggedExtent.height )
    {
        wxLogTrace( traceRhi, wxS( "VulkanDevice: Present OK extent=%ux%u srcFBO=%s" ),
                    swa.extent.width, swa.extent.height,
                    aSrcFBO ? "valid" : "null" );
        swa.lastLoggedExtent = swa.extent;
    }

    return RHI_FRAME_OP_RESULT::OK;
}


bool RHI_VULKAN_DEVICE::ReadbackFramebufferPixels( RHI_FramebufferHandle handle,
                                                    uint32_t attachmentIndex,
                                                    void* buffer, size_t bufferSize )
{
    if( !m_framebufferPool.IsValid( handle ) || !buffer || bufferSize == 0 || !m_allocator )
        return false;

    auto it = m_framebuffers.find( RhiHandleIndex( handle ) );

    if( it == m_framebuffers.end() )
        return false;

    FramebufferAllocation& fba = it->second;

    if( attachmentIndex >= fba.colorImages.size() )
        return false;

    uint32_t bpp      = RhiTextureBytesPerPixel( fba.colorFormats[attachmentIndex] );
    size_t   required = static_cast<size_t>( fba.width ) * fba.height * bpp;

    if( bufferSize < required )
        return false;

    VkImage srcImage = fba.colorImages[attachmentIndex];

    VkBufferCreateInfo stagingBufInfo{};
    stagingBufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufInfo.size  = required;
    stagingBufInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo stagingAllocInfo{};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT
                             | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkBuffer          stagingBuffer     = VK_NULL_HANDLE;
    VmaAllocation     stagingAllocation = VK_NULL_HANDLE;
    VmaAllocationInfo stagingInfo{};

    if( vmaCreateBuffer( m_allocator, &stagingBufInfo, &stagingAllocInfo,
                         &stagingBuffer, &stagingAllocation, &stagingInfo ) != VK_SUCCESS )
    {
        return false;
    }

    // Held from allocation through the wait so main-thread uploads cannot race the readback
    std::lock_guard<std::recursive_mutex> lock( m_deviceMutex );

    VkCommandBuffer cmd = VK_NULL_HANDLE;

    if( !allocateCommandBuffer( cmd ) )
    {
        vmaDestroyBuffer( m_allocator, stagingBuffer, stagingAllocation );
        return false;
    }

    beginOneTimeCommands( cmd );

    // The writes of earlier passes must be made visible to the copy, as in Present()
    VkImageMemoryBarrier barrier = imageBarrier(
            srcImage, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_ACCESS_TRANSFER_READ_BIT );

    m_vk.CmdPipelineBarrier( cmd,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &barrier );

    VkBufferImageCopy region{};
    region.bufferOffset                    = 0;
    region.bufferRowLength                 = 0;
    region.bufferImageHeight               = 0;
    region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel       = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount     = 1;
    region.imageOffset                     = { 0, 0, 0 };
    region.imageExtent                     = { fba.width, fba.height, 1 };

    m_vk.CmdCopyImageToBuffer( cmd, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                stagingBuffer, 1, &region );

    barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    m_vk.CmdPipelineBarrier( cmd,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &barrier );

    if( !SubmitOneShot( cmd ) )
    {
        vmaDestroyBuffer( m_allocator, stagingBuffer, stagingAllocation );
        return false;
    }

    vmaInvalidateAllocation( m_allocator, stagingAllocation, 0, VK_WHOLE_SIZE );
    std::memcpy( buffer, stagingInfo.pMappedData, required );

    vmaDestroyBuffer( m_allocator, stagingBuffer, stagingAllocation );
    return true;
}


bool RHI_VULKAN_DEVICE::CopyFramebufferDepth( RHI_FramebufferHandle aSource, RHI_FramebufferHandle aDestination,
                                              uint32_t aSourceX, uint32_t aSourceY )
{
    std::lock_guard<std::recursive_mutex> lock( m_deviceMutex );

    if( m_renderPassActive || !m_framebufferPool.IsValid( aSource ) || !m_framebufferPool.IsValid( aDestination ) )
        return false;

    auto srcIt = m_framebuffers.find( RhiHandleIndex( aSource ) );
    auto dstIt = m_framebuffers.find( RhiHandleIndex( aDestination ) );

    if( srcIt == m_framebuffers.end() || dstIt == m_framebuffers.end() )
        return false;

    FramebufferAllocation& src = srcIt->second;
    FramebufferAllocation& dst = dstIt->second;

    if( !src.depthImage || !dst.depthImage || !src.depthRendered || aSourceX + dst.width > src.width
        || aSourceY + dst.height > src.height )
    {
        return false;
    }

    VkCommandBuffer cmd = VK_NULL_HANDLE;

    if( !acquirePassCmd( cmd ) )
        return false;

    if( beginOneTimeCommands( cmd ) != VK_SUCCESS )
    {
        m_freeCmds.push_back( cmd );
        return false;
    }


    const VkPipelineStageFlags depthStages = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
                                             | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    const VkAccessFlags        depthAccess = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
                                             | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    auto barrier = [&]( VkImage aImage, VkImageLayout aOld, VkImageLayout aNew, VkAccessFlags aSrcAccess,
                        VkAccessFlags aDstAccess )
    {
        return imageBarrier( aImage, VK_IMAGE_ASPECT_DEPTH_BIT, aOld, aNew, aSrcAccess, aDstAccess );
    };

    // The destination's old content is overwritten, so its layout before does not matter
    VkImageMemoryBarrier before[2] = {
        barrier( src.depthImage, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, depthAccess, VK_ACCESS_TRANSFER_READ_BIT ),
        barrier( dst.depthImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, depthAccess,
                 VK_ACCESS_TRANSFER_WRITE_BIT )
    };

    m_vk.CmdPipelineBarrier( cmd, depthStages, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 2,
                             before );

    VkImageCopy region{};
    region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    region.srcSubresource.layerCount = 1;
    region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    region.dstSubresource.layerCount = 1;
    region.srcOffset                 = { static_cast<int32_t>( aSourceX ), static_cast<int32_t>( aSourceY ), 0 };
    region.extent                    = { dst.width, dst.height, 1 };

    m_vk.CmdCopyImage( cmd, src.depthImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst.depthImage,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region );

    VkImageMemoryBarrier after[2] = {
        barrier( src.depthImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, depthAccess ),
        barrier( dst.depthImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, depthAccess )
    };

    m_vk.CmdPipelineBarrier( cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, depthStages, 0, 0, nullptr, 0, nullptr, 2,
                             after );

    m_vk.EndCommandBuffer( cmd );

    if( submitPassCmd( cmd ) != VK_SUCCESS )
        return false;

    dst.depthRendered = true;
    return true;
}


// ---- Factory function ----

std::unique_ptr<RHI_DEVICE> RhiCreateVulkanDevice( bool enableValidation )
{
    auto device = std::make_unique<RHI_VULKAN_DEVICE>( enableValidation );

    if( !device->IsInitialized() )
        return nullptr;

    return device;
}
