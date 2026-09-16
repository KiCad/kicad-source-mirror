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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <cstdint>
#include <memory>

namespace SCH_CONNECTIVITY
{
struct FACADE_STATE;
struct CHANGE_SET;
class FACADE;

/**
 * Main-thread registration owner. Resetting or destroying it disconnects the listener. A listener
 * can reset any subscription, and the facade skips a listener that was reset during a batch.
 */
class SUBSCRIPTION
{
public:
    SUBSCRIPTION() = default;
    ~SUBSCRIPTION();
    SUBSCRIPTION( const SUBSCRIPTION& ) = delete;
    SUBSCRIPTION& operator=( const SUBSCRIPTION& ) = delete;
    SUBSCRIPTION( SUBSCRIPTION&& aOther ) noexcept;
    SUBSCRIPTION& operator=( SUBSCRIPTION&& aOther ) noexcept;
    void          Reset();

private:
    friend class FACADE;
    SUBSCRIPTION( std::weak_ptr<FACADE_STATE> aState, uint64_t aId );
    std::weak_ptr<FACADE_STATE> m_state;
    uint64_t                    m_id = 0;
};
} // namespace SCH_CONNECTIVITY
