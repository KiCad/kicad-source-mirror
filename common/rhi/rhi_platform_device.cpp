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

#include <rhi/rhi_platform_device.h>
#include <rhi/rhi_device.h>

#ifdef RHI_HAS_VULKAN
#include <rhi/rhi_vulkan_device.h>
#endif

#ifdef RHI_HAS_METAL
#include <rhi/rhi_metal_device.h>
#endif


std::unique_ptr<RHI_DEVICE> RhiCreatePlatformDevice( bool aEnableVulkanValidation )
{
    std::unique_ptr<RHI_DEVICE> device;

#if defined( RHI_HAS_METAL )
    (void) aEnableVulkanValidation;
    device = RhiCreateMetalDevice();
#elif defined( RHI_HAS_VULKAN )
    device = RhiCreateVulkanDevice( aEnableVulkanValidation );
#else
    (void) aEnableVulkanValidation;
#endif

    if( device && device->IsInitialized() )
        return device;

    return nullptr;
}
