/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
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

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <io/pads/pads_binary_utils.h>

namespace PADS_SCH_BINARY
{

struct SCH_SDB_POOL
{
    uint32_t allocatedBytes = 0;
    uint32_t count = 0;
    uint32_t usedBytes = 0;
    uint32_t handle = 0;
};


class PADS_SCH_SDB
{
public:
    PADS_SCH_SDB() = default;

    PADS_SCH_SDB( const PADS_SCH_SDB& ) = delete;
    PADS_SCH_SDB& operator=( const PADS_SCH_SDB& ) = delete;

    void Load( std::vector<uint8_t> aBytes );

    static bool HasFamilyMagic( const std::vector<uint8_t>& aBytes );
    static bool IsSupportedVersion( uint16_t aVersion );

    uint16_t                            Version() const { return m_version; }
    const std::array<SCH_SDB_POOL, 20>& Pools() const { return m_pools; }
    const PADS_IO::BINARY_CURSOR&       Cursor() const { return m_cursor; }
    const std::vector<uint8_t>&         Bytes() const { return m_data; }
    size_t                              PayloadOffset() const { return m_payloadOffset; }
    size_t                              FooterOffset() const { return m_footerOffset; }

private:
    void              parseHeader();
    void              parseDirectory();
    void              verifyFooter();
    [[noreturn]] void throwAt( size_t aOffset, const wxString& aDetail ) const;

    std::vector<uint8_t>         m_data;
    PADS_IO::BINARY_CURSOR       m_cursor{ m_data };
    uint16_t                     m_version = 0;
    std::array<SCH_SDB_POOL, 20> m_pools{};
    size_t                       m_payloadOffset = 0;
    size_t                       m_footerOffset = 0;
};

} // namespace PADS_SCH_BINARY
