/*
 * Copyright (C) 2025 Mark Roszko <mark.roszko@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2007 Ecma International (original Java source)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * License info found here:
 * https://www.loc.gov/preservation/digital/formats/fdd/fdd000491.shtml
 */

#pragma once

#include <cstdint>
#include <vector>

namespace U3D
{
namespace BLOCK_TYPES
{
    constexpr uint32_t MODIFIER_CHAIN = 0xffffff14;
    constexpr uint32_t GROUP_NODE = 0xffffff21;
    constexpr uint32_t MODEL_NODE = 0xffffff22;
    constexpr uint32_t LIGHT_NODE = 0xffffff23;
    constexpr uint32_t MESH_DECLARATION = 0xffffff31;
    constexpr uint32_t MESH_CONTINUATION = 0xffffff3b;
    constexpr uint32_t SHADING_MODIFIER = 0xffffff45;
    constexpr uint32_t LIGHT_RESOURCE = 0xffffff51;
    constexpr uint32_t LIT_TEXTURE_SHADER = 0xffffff53;
    constexpr uint32_t MATERIAL_RESOURCE = 0xffffff54;
    constexpr uint32_t FILE_HEADER = 0x00443355;
} // namespace BLOCK_TYPES

namespace HEADER_PROFILE_FLAGS
{
    constexpr uint32_t DEFINED_UNITS = 0x00000008;
} // namespace HEADER_PROFILE_FLAGS

class DATA_BLOCK
{
public:
    DATA_BLOCK();

    uint32_t GetDataSize() const { return m_dataSize; }
    void     SetDataSize( uint32_t aSize );

    const std::vector<uint32_t>& GetData() const { return m_data; }
    void                  SetData( const std::vector<uint32_t>& aValue ) { m_data = aValue; }

    uint32_t GetMetaDataSize() const { return m_metaDataSize; }
    void     SetMetaDataSize( uint32_t aSize );

    const std::vector<uint32_t>& GetMetaData() const { return m_metaData; }
    void                  SetMetaData( const std::vector<uint32_t>& aValue );

    uint32_t GetBlockType() const { return m_blockType; }
    void     SetBlockType( uint32_t aType ) { m_blockType = aType; }

    uint32_t GetPriority() const { return m_priority; }
    void     SetPriority( uint32_t aPriority ) { m_priority = aPriority; }

private:
    std::vector<uint32_t> m_data;
    uint32_t              m_dataSize;
    std::vector<uint32_t> m_metaData;
    uint32_t              m_metaDataSize;
    uint32_t              m_priority;
    uint32_t              m_blockType;
};

} // namespace U3D