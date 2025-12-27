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

#ifndef KICAD_SINGLETON_H
#define KICAD_SINGLETON_H

#include <cstdint>
#include <advanced_config.h>

class GL_CONTEXT_MANAGER;
namespace BS
{
template <std::uint8_t>
class thread_pool;

using priority_thread_pool = thread_pool<1>;
}

class KICAD_SINGLETON
{
public:
    KICAD_SINGLETON() :
            m_ThreadPool( nullptr ),
            m_GLContextManager( nullptr )
    {};

    ~KICAD_SINGLETON();

    /**
     * Explicitly shut down and destroy the thread pool and GL context manager.
     *
     * This must be called before static destruction begins to avoid crashes on macOS
     * where the thread pool destructor tries to wait on condition variables during
     * static destruction, after other statics have already been destroyed.
     *
     * After calling Shutdown(), the destructor becomes a no-op.
     */
    void Shutdown();

    void Init();

public:
    BS::priority_thread_pool* m_ThreadPool;
    GL_CONTEXT_MANAGER* m_GLContextManager;
};


#endif // KICAD_SINGLETON_H
