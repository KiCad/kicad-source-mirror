/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef GL_RESET_BUDGET_H
#define GL_RESET_BUDGET_H

#include <chrono>
#include <cstddef>
#include <deque>

namespace KIGFX
{

/**
 * Limit how often the OpenGL canvases are rebuilt after GPU resets.
 *
 * A driver that resets on every frame would otherwise rebuild the canvases forever, so once
 * more than the allowed number of resets land inside the window the caller should stop using
 * OpenGL.
 */
class GL_RESET_BUDGET
{
public:
    using CLOCK = std::chrono::steady_clock;

    GL_RESET_BUDGET( size_t aMaxResets, CLOCK::duration aWindow ) :
            m_maxResets( aMaxResets ),
            m_window( aWindow )
    {
    }

    /**
     * Record a reset.
     *
     * @return true if rebuilding the OpenGL canvases is still worthwhile.
     */
    bool RecordReset( CLOCK::time_point aNow )
    {
        while( !m_resets.empty() && aNow - m_resets.front() >= m_window )
            m_resets.pop_front();

        m_resets.push_back( aNow );

        return m_resets.size() <= m_maxResets;
    }

private:
    size_t                        m_maxResets;
    CLOCK::duration               m_window;
    std::deque<CLOCK::time_point> m_resets;
};

} // namespace KIGFX

#endif // GL_RESET_BUDGET_H
