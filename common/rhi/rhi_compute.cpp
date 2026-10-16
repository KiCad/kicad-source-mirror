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

#include <rhi/rhi_compute.h>

#include <array>
#include <unordered_set>


bool RhiValidateComputeBindGroupLayout( const RHI_BIND_GROUP_LAYOUT_DESC& aLayout )
{
    if( !aLayout.Validate() )
        return false;

    for( const RHI_BINDING_ENTRY& entry : aLayout.entries )
    {
        if( !( entry.stages & RHI_SHADER_STAGE::COMPUTE ) )
            return false;

        if( entry.type != RHI_BINDING_TYPE::COMBINED_TEXTURE_SAMPLER && entry.binding == 0 )
            return false;
    }

    return true;
}


bool RhiValidateComputePipelineDesc( const RHI_COMPUTE_PIPELINE_DESC& aDesc )
{
    if( !aDesc.workgroupX || !aDesc.workgroupY || aDesc.pushConstantBytes > RHI_MAX_PUSH_CONSTANT_BYTES
        || aDesc.pushConstantBytes % 4 || aDesc.bindGroupLayouts.size() > RHI_MAX_BIND_GROUPS )
    {
        return false;
    }

    std::unordered_set<uint32_t> ids;

    for( const RHI_SPECIALIZATION_CONSTANT& constant : aDesc.constants )
    {
        if( constant.id < RHI_SPEC_ID_USER_FIRST || !ids.insert( constant.id ).second )
            return false;
    }

    // Metal flattens sets, so binding numbers must be unique across all of them
    std::unordered_set<uint32_t> bindings;

    for( const RHI_BIND_GROUP_LAYOUT_DESC& layout : aDesc.bindGroupLayouts )
    {
        if( !RhiValidateComputeBindGroupLayout( layout ) )
            return false;

        for( const RHI_BINDING_ENTRY& entry : layout.entries )
        {
            if( !bindings.insert( entry.binding ).second )
                return false;
        }
    }

    return true;
}


bool RhiValidateComputeJob( const RHI_COMPUTE_JOB& aJob, const RHI_COMPUTE_DEVICE_LIMITS& aLimits,
                            const RHI_COMPUTE_JOB_LOOKUP& aLookup )
{
    using OP = RHI_COMPUTE_JOB::OP;

    if( !aJob.IsValid() )
        return false;

    const RHI_COMPUTE_PIPELINE_DESC*                     pipeline = nullptr;
    std::array<RHI_BindGroupHandle, RHI_MAX_BIND_GROUPS> bound{};
    bool                                                 pushed = false;
    bool                                                 groupsChecked = false;

    for( const RHI_COMPUTE_JOB::COMMAND& command : aJob.Commands() )
    {
        switch( command.op )
        {
        case OP::UPLOAD:
        case OP::READBACK:
            if( !aLookup.storageRange( command.buffer, command.offset, command.bytes ) )
                return false;

            break;

        case OP::BIND:
            pipeline = aLookup.pipeline( command.pipeline );

            if( !pipeline )
                return false;

            pushed = false;
            groupsChecked = false;
            break;

        case OP::BIND_GROUP:
            if( command.index >= bound.size() || !aLookup.groupLayout( command.group ) )
                return false;

            bound[command.index] = command.group;
            groupsChecked = false;
            break;

        case OP::PUSH_CONSTANTS:
            if( !pipeline || command.bytes != pipeline->pushConstantBytes )
                return false;

            pushed = true;
            break;

        case OP::DISPATCH:
            if( !pipeline || !command.groupsX || !command.groupsY || command.groupsX > aLimits.maxDispatchGroups[0]
                || command.groupsY > aLimits.maxDispatchGroups[1] || ( pipeline->pushConstantBytes && !pushed ) )
            {
                return false;
            }

            // Layouts only need rechecking after a pipeline or group change
            if( !groupsChecked )
            {
                for( size_t i = 0; i < pipeline->bindGroupLayouts.size(); ++i )
                {
                    const RHI_BIND_GROUP_LAYOUT_DESC& expected = pipeline->bindGroupLayouts[i];

                    if( expected.entries.empty() )
                        continue;

                    const RHI_BIND_GROUP_LAYOUT_DESC* actual = aLookup.groupLayout( bound[i] );

                    if( !actual || !actual->IsCompatible( expected ) )
                        return false;
                }

                groupsChecked = true;
            }

            break;

        case OP::BARRIER: break;
        }
    }

    return true;
}


static std::vector<uint8_t> copyBytes( const void* aData, size_t aBytes )
{
    const uint8_t* bytes = static_cast<const uint8_t*>( aData );
    return aData ? std::vector<uint8_t>( bytes, bytes + aBytes ) : std::vector<uint8_t>();
}


void RHI_COMPUTE_JOB::Upload( RHI_BufferHandle aBuffer, const void* aData, size_t aBytes, size_t aOffset )
{
    m_valid = m_valid && ( aData || !aBytes );
    m_commands.push_back( { .op = OP::UPLOAD,
                            .buffer = aBuffer,
                            .offset = aOffset,
                            .bytes = aBytes,
                            .data = copyBytes( aData, aBytes ) } );
}


void RHI_COMPUTE_JOB::Bind( RHI_ComputePipelineHandle aPipeline )
{
    m_commands.push_back( { .op = OP::BIND, .pipeline = aPipeline } );
}


void RHI_COMPUTE_JOB::BindGroup( uint32_t aIndex, RHI_BindGroupHandle aGroup )
{
    m_commands.push_back( { .op = OP::BIND_GROUP, .group = aGroup, .index = aIndex } );
}


void RHI_COMPUTE_JOB::PushConstants( const void* aData, size_t aBytes )
{
    m_valid = m_valid && aBytes <= RHI_MAX_PUSH_CONSTANT_BYTES && aBytes % 4 == 0 && ( aData || !aBytes );
    m_commands.push_back( { .op = OP::PUSH_CONSTANTS, .bytes = aBytes, .data = copyBytes( aData, aBytes ) } );
}


void RHI_COMPUTE_JOB::Dispatch( uint32_t aGroupsX, uint32_t aGroupsY )
{
    m_commands.push_back( { .op = OP::DISPATCH, .groupsX = aGroupsX, .groupsY = aGroupsY } );
}


void RHI_COMPUTE_JOB::Barrier()
{
    m_commands.push_back( { .op = OP::BARRIER } );
}


void RHI_COMPUTE_JOB::Readback( RHI_BufferHandle aBuffer, size_t aOffset, size_t aBytes, void* aDestination )
{
    m_valid = m_valid && ( aDestination || !aBytes );
    m_commands.push_back( { .op = OP::READBACK,
                            .buffer = aBuffer,
                            .offset = aOffset,
                            .bytes = aBytes,
                            .destination = aDestination } );
}
