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

#include <boost/test/unit_test.hpp>
#include <rhi/gpu_compute.h>
#include <checkerboard_comp.h>
#include <workgroup_comp.h>
#include <reduction_comp.h>
#include <multipass_comp.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>


namespace
{
RHI_BIND_GROUP_LAYOUT_DESC StorageLayout( bool aReduction = false )
{
    RHI_BIND_GROUP_LAYOUT_DESC layout;
    layout.entries.push_back(
            { 1, aReduction ? RHI_BINDING_TYPE::STORAGE_BUFFER_READONLY : RHI_BINDING_TYPE::STORAGE_BUFFER,
              RHI_SHADER_STAGE::COMPUTE } );

    if( aReduction )
        layout.entries.push_back( { 2, RHI_BINDING_TYPE::STORAGE_BUFFER, RHI_SHADER_STAGE::COMPUTE } );

    return layout;
}


struct RESOURCES
{
    std::shared_ptr<GPU_COMPUTE>           gpu;
    std::vector<RHI_BufferHandle>          buffers;
    std::vector<RHI_ComputePipelineHandle> pipelines;
    RHI_BindGroupLayoutHandle              layout;
    RHI_BindGroupHandle                    group;

    ~RESOURCES()
    {
        gpu->DestroyBindGroup( group );
        gpu->DestroyBindGroupLayout( layout );

        for( auto pipeline : pipelines )
            gpu->DestroyPipeline( pipeline );

        for( auto buffer : buffers )
            gpu->DestroyBuffer( buffer );
    }

    RHI_BufferHandle Buffer( size_t aBytes )
    {
        auto buffer = gpu->CreateStorageBuffer( aBytes );
        BOOST_REQUIRE( buffer );
        buffers.push_back( buffer );
        return buffer;
    }

    RHI_ComputePipelineHandle Pipeline( const RHI_COMPUTE_PIPELINE_DESC& aDesc )
    {
        auto pipeline = gpu->CreatePipeline( aDesc );
        BOOST_REQUIRE( pipeline );
        pipelines.push_back( pipeline );
        return pipeline;
    }

    void Group( const RHI_BIND_GROUP_LAYOUT_DESC& aLayout, const std::vector<RHI_BIND_GROUP_RESOURCE>& aEntries )
    {
        layout = gpu->CreateBindGroupLayout( aLayout );
        BOOST_REQUIRE( layout );
        group = gpu->CreateBindGroup( { layout, aEntries } );
        BOOST_REQUIRE( group );
    }
};
} // namespace


BOOST_AUTO_TEST_SUITE( RhiCompute )

BOOST_AUTO_TEST_CASE( DescriptorValidation )
{
    RHI_COMPUTE_PIPELINE_DESC desc;
    desc.bindGroupLayouts = { StorageLayout() };
    BOOST_CHECK( RhiValidateComputePipelineDesc( desc ) );
    desc.constants = { { 0, 1 } };
    BOOST_CHECK( !RhiValidateComputePipelineDesc( desc ) );
    desc.constants = { { 1, 1 } };
    BOOST_CHECK( !RhiValidateComputePipelineDesc( desc ) );
    desc.constants = { { 2, 0 }, { 2, 1 } };
    BOOST_CHECK( !RhiValidateComputePipelineDesc( desc ) );
    desc.constants.clear();
    desc.bindGroupLayouts[0].entries[0].binding = 0;
    BOOST_CHECK( desc.bindGroupLayouts[0].Validate() );
    BOOST_CHECK( !RhiValidateComputeBindGroupLayout( desc.bindGroupLayouts[0] ) );
    BOOST_CHECK( !RhiValidateComputePipelineDesc( desc ) );
    desc.bindGroupLayouts = { StorageLayout() };
    desc.workgroupX = 0;
    BOOST_CHECK( !RhiValidateComputePipelineDesc( desc ) );
    desc.workgroupX = 8;
    desc.workgroupY = 0;
    BOOST_CHECK( !RhiValidateComputePipelineDesc( desc ) );
    desc.workgroupY = 8;
    desc.pushConstantBytes = RHI_MAX_PUSH_CONSTANT_BYTES + 4;
    BOOST_CHECK( !RhiValidateComputePipelineDesc( desc ) );
    desc.pushConstantBytes = 0;
    desc.bindGroupLayouts.push_back( StorageLayout() );
    BOOST_CHECK( !RhiValidateComputePipelineDesc( desc ) );
}

BOOST_AUTO_TEST_CASE( RecordedBytesAreOwned )
{
    std::array<uint32_t, 2> data{ 37, 19 };
    RHI_COMPUTE_JOB         job;
    job.Upload( RHI_BufferHandle{ 1 }, data.data(), sizeof( data ), 4 );
    job.PushConstants( data.data(), sizeof( data ) );
    data.fill( 0 );
    const std::array<uint32_t, 2> expected{ 37, 19 };
    const auto*                   bytes = reinterpret_cast<const uint8_t*>( expected.data() );
    BOOST_CHECK( job.IsValid() );
    BOOST_REQUIRE_EQUAL( job.Commands().size(), 2 );

    for( const auto& command : job.Commands() )
        BOOST_CHECK_EQUAL_COLLECTIONS( command.data.begin(), command.data.end(), bytes, bytes + sizeof( expected ) );

    RHI_COMPUTE_JOB invalid;
    invalid.Upload( RHI_BufferHandle{ 1 }, nullptr, 4 );
    BOOST_CHECK( !invalid.IsValid() );
}


BOOST_AUTO_TEST_CASE( OrderedByteTransfers )
{
    auto gpu = GPU_COMPUTE::Get();

    if( !gpu )
    {
        BOOST_TEST_MESSAGE( "Compute unavailable; skipping ordered transfers" );
        return;
    }

    auto buffer = gpu->CreateStorageBuffer( 11 );
    BOOST_REQUIRE( buffer );
    std::array<uint8_t, 11>      initial{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    std::array<uint8_t, 7>       replacement{ 31, 32, 33, 34, 35, 36, 37 };
    std::array<uint8_t, 11>      first{};
    std::array<uint8_t, 8>       second{};
    const std::array<uint8_t, 8> expected{ 2, 31, 32, 33, 34, 35, 36, 37 };
    RHI_COMPUTE_JOB              job;
    job.Upload( buffer, initial.data(), initial.size() );
    job.Readback( buffer, 0, first.size(), first.data() );
    job.Upload( buffer, replacement.data(), replacement.size(), 3 );
    replacement.fill( 0 );
    job.Readback( buffer, 2, second.size(), second.data() );
    BOOST_REQUIRE( gpu->Submit( job ) == RHI_COMPUTE_RESULT::OK );
    gpu->DestroyBuffer( buffer );
    BOOST_CHECK_EQUAL_COLLECTIONS( first.begin(), first.end(), initial.begin(), initial.end() );
    BOOST_CHECK_EQUAL_COLLECTIONS( second.begin(), second.end(), expected.begin(), expected.end() );
}


BOOST_AUTO_TEST_CASE( CheckerboardSpecializations )
{
    auto gpu = GPU_COMPUTE::Get();

    if( !gpu )
    {
        BOOST_TEST_MESSAGE( "Compute unavailable; skipping checkerboard" );
        return;
    }

    RESOURCES                     resources{ gpu };
    constexpr uint32_t            width = 37, height = 19;
    const std::array<uint32_t, 2> params{ width, height };
    const auto                    layout = StorageLayout();
    auto                          buffer = resources.Buffer( width * height * sizeof( uint32_t ) );
    resources.Group( layout, { { 1, buffer } } );

    for( auto workgroup : { std::array<uint32_t, 2>{ 8, 8 }, std::array<uint32_t, 2>{ 16, 4 } } )
    {
        for( uint32_t rb = 0; rb < 2; ++rb )
        {
            RHI_COMPUTE_PIPELINE_DESC desc;
            desc.shader = k_checkerboard_comp;
            desc.bindGroupLayouts = { layout };
            desc.constants = { { 2, rb } };
            desc.workgroupX = workgroup[0];
            desc.workgroupY = workgroup[1];
            desc.pushConstantBytes = sizeof( params );
            auto                  pipeline = resources.Pipeline( desc );
            std::vector<uint32_t> cells( width * height, 0xfeedu );
            RHI_COMPUTE_JOB       job;
            job.Upload( buffer, cells.data(), cells.size() * sizeof( cells[0] ) );
            std::fill( cells.begin(), cells.end(), 0u );
            job.Bind( pipeline );
            job.BindGroup( 0, resources.group );
            job.PushConstants( params.data(), sizeof( params ) );
            job.Dispatch( ( width + workgroup[0] - 1 ) / workgroup[0], ( height + workgroup[1] - 1 ) / workgroup[1] );
            job.Readback( buffer, 0, cells.size() * sizeof( cells[0] ), cells.data() );
            BOOST_REQUIRE( gpu->Submit( job ) == RHI_COMPUTE_RESULT::OK );

            for( uint32_t y = 0; y < height; ++y )
            {
                for( uint32_t x = 0; x < width; ++x )
                    BOOST_CHECK_EQUAL( cells[y * width + x], ( ( x + y ) & 1 ) == rb ? 17u + rb : 0xfeedu );
            }
        }
    }
}

BOOST_AUTO_TEST_CASE( InjectedWorkgroupSize )
{
    auto gpu = GPU_COMPUTE::Get();

    if( !gpu )
    {
        BOOST_TEST_MESSAGE( "Compute unavailable; skipping injected workgroup" );
        return;
    }

    RESOURCES  resources{ gpu };
    auto       buffer = resources.Buffer( 2 * sizeof( uint32_t ) );
    const auto layout = StorageLayout();
    resources.Group( layout, { { 1, buffer } } );

    for( auto size : { std::array<uint32_t, 2>{ 8, 8 }, std::array<uint32_t, 2>{ 16, 4 } } )
    {
        RHI_COMPUTE_PIPELINE_DESC desc;
        desc.shader = k_workgroup_comp;
        desc.bindGroupLayouts = { layout };
        desc.workgroupX = size[0];
        desc.workgroupY = size[1];
        std::array<uint32_t, 2> result{};
        RHI_COMPUTE_JOB         job;
        job.Upload( buffer, result.data(), sizeof( result ) );
        job.Bind( resources.Pipeline( desc ) );
        job.BindGroup( 0, resources.group );
        job.Dispatch( 1 );
        job.Readback( buffer, 0, sizeof( result ), result.data() );
        BOOST_REQUIRE( gpu->Submit( job ) == RHI_COMPUTE_RESULT::OK );
        BOOST_CHECK_EQUAL( result[0], size[0] );
        BOOST_CHECK_EQUAL( result[1], size[1] );
    }
}

BOOST_AUTO_TEST_CASE( SpecializedSharedReduction )
{
    auto gpu = GPU_COMPUTE::Get();

    if( !gpu )
    {
        BOOST_TEST_MESSAGE( "Compute unavailable; skipping shared reduction" );
        return;
    }

    RESOURCES          resources{ gpu };
    constexpr uint32_t groups = 16, localSize = 64;
    std::vector<float> input( groups * localSize );

    for( size_t i = 0; i < input.size(); ++i )
        input[i] = static_cast<float>( static_cast<int>( ( i * 37 ) % 2003 ) - 1001 ) / 8.0f;

    auto       in = resources.Buffer( input.size() * sizeof( float ) );
    auto       out = resources.Buffer( groups * sizeof( float ) );
    const auto layout = StorageLayout( true );
    resources.Group( layout, { { 1, in }, { 2, out } } );
    RHI_COMPUTE_PIPELINE_DESC desc;
    desc.shader = k_reduction_comp;
    desc.bindGroupLayouts = { layout };
    desc.workgroupX = localSize;
    desc.workgroupY = 1;
    std::array<float, groups> result;
    result.fill( -1.0f );
    RHI_COMPUTE_JOB job;
    job.Upload( in, input.data(), input.size() * sizeof( float ) );
    job.Upload( out, result.data(), sizeof( result ) );
    job.Bind( resources.Pipeline( desc ) );
    job.BindGroup( 0, resources.group );
    job.Dispatch( groups );
    job.Readback( out, 0, sizeof( result ), result.data() );
    BOOST_REQUIRE( gpu->Submit( job ) == RHI_COMPUTE_RESULT::OK );

    for( uint32_t group = 0; group < groups; ++group )
    {
        float expected = 0;

        for( uint32_t local = 0; local < localSize; ++local )
            expected = std::max( expected, std::abs( input[group * localSize + local] ) );

        BOOST_CHECK_EQUAL( result[group], expected );
    }
}

BOOST_AUTO_TEST_CASE( DependentPassesInOneJob )
{
    auto gpu = GPU_COMPUTE::Get();

    if( !gpu )
    {
        BOOST_TEST_MESSAGE( "Compute unavailable; skipping dependent passes" );
        return;
    }

    RESOURCES               resources{ gpu };
    constexpr uint32_t      width = 37, height = 19, iterations = 5;
    std::array<uint32_t, 2> params{ width, height };
    auto                    buffer = resources.Buffer( width * height * sizeof( uint32_t ) );
    const auto              layout = StorageLayout();
    resources.Group( layout, { { 1, buffer } } );
    std::array<RHI_ComputePipelineHandle, 3> pipelines;

    for( uint32_t pass = 0; pass < pipelines.size(); ++pass )
    {
        RHI_COMPUTE_PIPELINE_DESC desc;
        desc.shader = k_multipass_comp;
        desc.bindGroupLayouts = { layout };
        desc.constants = { { 2, pass } };
        desc.pushConstantBytes = sizeof( params );
        pipelines[pass] = resources.Pipeline( desc );
    }

    std::vector<uint32_t> expected( width * height ), result( width * height, 0xfeedu );

    for( uint32_t y = 0; y < height; ++y )
    {
        for( uint32_t x = 0; x < width; ++x )
            expected[y * width + x] = ( x + 3 * y ) % 11;
    }

    RHI_COMPUTE_JOB job;
    job.Upload( buffer, result.data(), result.size() * sizeof( uint32_t ) );
    job.BindGroup( 0, resources.group );
    const auto pass = [&]( uint32_t aPass )
    {
        job.Bind( pipelines[aPass] );
        job.PushConstants( params.data(), sizeof( params ) );
        job.Dispatch( ( width + 7 ) / 8, ( height + 7 ) / 8 );
        job.Barrier();
    };
    pass( 2 );

    for( uint32_t iteration = 0; iteration < iterations; ++iteration )
    {
        for( uint32_t rb = 0; rb < 2; ++rb )
        {
            pass( rb );

            for( uint32_t y = 0; y < height; ++y )
            {
                for( uint32_t x = 0; x < width; ++x )
                {
                    if( ( ( x + y ) & 1 ) == rb )
                    {
                        auto i = y * width + x;
                        expected[i] += 1 + expected[x == 0 ? i + 1 : i - 1] % 3;
                    }
                }
            }
        }
    }

    job.Readback( buffer, 0, result.size() * sizeof( uint32_t ), result.data() );
    BOOST_REQUIRE( gpu->Submit( job ) == RHI_COMPUTE_RESULT::OK );
    BOOST_CHECK_EQUAL_COLLECTIONS( result.begin(), result.end(), expected.begin(), expected.end() );
}

BOOST_AUTO_TEST_SUITE_END()
