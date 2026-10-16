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

#pragma once

#include <rhi/rhi_bind_group.h>
#include <rhi/rhi_command.h>
#include <rhi/rhi_shader.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

/** Specialization IDs reserved for the workgroup dimensions; Z is always one. */
constexpr uint32_t RHI_SPEC_ID_WORKGROUP_X = 0;
constexpr uint32_t RHI_SPEC_ID_WORKGROUP_Y = 1;
constexpr uint32_t RHI_SPEC_ID_USER_FIRST = 2;

/** A 32-bit specialization value represented as raw bits. */
struct RHI_SPECIALIZATION_CONSTANT
{
    uint32_t id;
    uint32_t value;
};

struct RHI_COMPUTE_PIPELINE_DESC
{
    RHI_SHADER_BLOB                          shader;
    std::vector<RHI_BIND_GROUP_LAYOUT_DESC>  bindGroupLayouts;
    std::vector<RHI_SPECIALIZATION_CONSTANT> constants;
    uint32_t                                 workgroupX = 8;
    uint32_t                                 workgroupY = 8;
    uint32_t                                 pushConstantBytes = 0;
};

/** Validate the portable contract without accessing a GPU or inspecting shader bytecode. */
bool RhiValidateComputePipelineDesc( const RHI_COMPUTE_PIPELINE_DESC& aDesc );

/**
 * Validate a compute bind group layout. Every entry must be visible to compute and no buffer may use
 * binding 0, which Metal reserves for push constants.
 */
bool RhiValidateComputeBindGroupLayout( const RHI_BIND_GROUP_LAYOUT_DESC& aLayout );

struct RHI_COMPUTE_DEVICE_LIMITS
{
    std::array<uint32_t, 3> maxWorkgroupSize{};
    uint32_t                maxWorkgroupInvocations = 0;
    std::array<uint32_t, 3> maxDispatchGroups{};
    uint64_t                maxSharedMemoryBytes = 0;
    uint64_t                maxStorageBufferRange = 0;
    uint64_t                minStorageBufferOffsetAlignment = 0;
};

struct RHI_COMPUTE_PIPELINE_LIMITS
{
    uint32_t maxThreadsPerWorkgroup = 0;
    uint32_t preferredWorkgroupMultiple = 0;
};

enum class RHI_COMPUTE_RESULT
{
    OK,
    DEVICE_LOST,
    INVALID
};

/**
 * Plain owning command data, recordable on any thread without device access.
 * Readback destinations must remain valid until Submit() returns.
 * Handles belong to the GPU_COMPUTE instance that created them.
 */
class RHI_COMPUTE_JOB
{
public:
    enum class OP
    {
        UPLOAD,
        BIND,
        BIND_GROUP,
        PUSH_CONSTANTS,
        DISPATCH,
        BARRIER,
        READBACK
    };

    /** Backend command representation; payload bytes are owned by the job. */
    struct COMMAND
    {
        OP                        op = OP::BARRIER;
        RHI_BufferHandle          buffer{};
        RHI_ComputePipelineHandle pipeline{};
        RHI_BindGroupHandle       group{};
        uint32_t                  index = 0;
        uint32_t                  groupsX = 0;
        uint32_t                  groupsY = 0;
        size_t                    offset = 0;
        size_t                    bytes = 0;
        void*                     destination = nullptr;
        std::vector<uint8_t>      data{};
    };

    /** Copy upload bytes immediately. Invalid pointers mark the job invalid. */
    void Upload( RHI_BufferHandle aBuffer, const void* aData, size_t aBytes, size_t aOffset = 0 );
    void Bind( RHI_ComputePipelineHandle aPipeline );
    void BindGroup( uint32_t aIndex, RHI_BindGroupHandle aGroup );
    void PushConstants( const void* aData, size_t aBytes );
    void Dispatch( uint32_t aGroupsX, uint32_t aGroupsY = 1 );
    void Barrier();
    void Readback( RHI_BufferHandle aBuffer, size_t aOffset, size_t aBytes, void* aDestination );

    bool                        IsValid() const { return m_valid; }
    const std::vector<COMMAND>& Commands() const { return m_commands; }

private:
    bool                 m_valid = true;
    std::vector<COMMAND> m_commands;
};


/** Backend lookups for RhiValidateComputeJob. Each returns nullptr or false for a stale handle. */
struct RHI_COMPUTE_JOB_LOOKUP
{
    std::function<const RHI_COMPUTE_PIPELINE_DESC*( RHI_ComputePipelineHandle )> pipeline;
    std::function<const RHI_BIND_GROUP_LAYOUT_DESC*( RHI_BindGroupHandle )>      groupLayout;
    std::function<bool( RHI_BufferHandle, size_t aOffset, size_t aBytes )>       storageRange;
};

/**
 * Check a whole job against live resources before any of it executes, so a bad job has no partial
 * effect. Push constants must be pushed in full after each Bind() and before the next Dispatch().
 */
bool RhiValidateComputeJob( const RHI_COMPUTE_JOB& aJob, const RHI_COMPUTE_DEVICE_LIMITS& aLimits,
                            const RHI_COMPUTE_JOB_LOOKUP& aLookup );
