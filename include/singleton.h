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
}

class KICAD_SINGLETON
{
public:
    KICAD_SINGLETON() :
            m_ThreadPool( nullptr ),
            m_GLContextManager( nullptr )
    {};

    ~KICAD_SINGLETON();


    void Init();

public:
    BS::thread_pool<0>* m_ThreadPool;
    GL_CONTEXT_MANAGER* m_GLContextManager;
};


#endif // KICAD_SINGLETON_H