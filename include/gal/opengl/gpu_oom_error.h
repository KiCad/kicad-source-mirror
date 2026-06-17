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

#ifndef GPU_OOM_ERROR_H_
#define GPU_OOM_ERROR_H_

#include <stdexcept>
#include <string>

namespace KIGFX
{

/**
 * Raised when a GPU buffer allocation is predicted to exceed the available video memory.
 *
 * Throwing this before the allocation is issued lets the canvas fall back to software
 * rendering instead of being killed by the graphics driver (e.g. the NVIDIA "Error code: 6"
 * abort, which terminates the process before glGetError() can report GL_OUT_OF_MEMORY).
 */
class GPU_OOM_ERROR : public std::runtime_error
{
public:
    explicit GPU_OOM_ERROR( const std::string& aMessage ) :
            std::runtime_error( aMessage )
    {
    }
};

} // namespace KIGFX

#endif /* GPU_OOM_ERROR_H_ */
