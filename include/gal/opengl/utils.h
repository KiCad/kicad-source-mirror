/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016-2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef __OPENGL_UTILS_H
#define __OPENGL_UTILS_H

#include <gal/gal.h>

#include <cstddef>
#include <string>

/**
 * Check if a recent OpenGL operation has failed. If so, display the appropriate message
 * starting with \a aInfo string to give more details.
 *
 * @param aInfo is the beginning of the error message.
 * @param aFile is the file where the error occurred defined by the C __FILE__ variable.
 * @param aLine is the line in \a aFile where the error occurred defined by the C __LINE__
 *              variable.
 * @param aThrow an exception is thrown when true, otherwise only an error message is displayed.
 * @return GL_NO_ERROR in case of no errors or one of GL_ constants returned by glGetError().
 */
int checkGlError( const std::string& aInfo, const char* aFile, int aLine, bool aThrow = true );

/**
 * Enable or disable OpenGL driver messages output.
 *
 * @param aEnable decides whether the message should be shown.
 */
void enableGlDebug( bool aEnable );


namespace KIGFX
{

/**
 * Strategy for growing a GPU vertex buffer, trading copy speed against peak video memory.
 */
enum class VRAM_RESIZE_STRATEGY
{
    GPU_COPY,  ///< Fast GPU-side copy; the old and new buffers are briefly co-resident.
    RAM_STAGE, ///< Stage through host memory so only the larger of the two buffers is resident.
    REFUSE     ///< Neither path fits; the caller should fall back to software rendering.
};

/**
 * Query the amount of free video memory the driver reports.
 *
 * Uses GL_NVX_gpu_memory_info (NVIDIA) or GL_ATI_meminfo (AMD) when present. Requires a
 * current OpenGL context.
 *
 * @return the free video memory in bytes, or 0 when no driver query is available.
 */
GAL_API size_t queryFreeVideoMemoryBytes();

/**
 * Decide how to grow a GPU vertex buffer given the free video memory budget.
 *
 * A doubling resize would otherwise hold the old and new buffers in VRAM at the same time,
 * which can exceed the budget on a large board and trip a fatal driver out-of-memory abort.
 *
 * @param aFreeVRAM is the free video memory in bytes, or 0 when unknown.
 * @param aOldBytes is the size of the existing buffer in bytes.
 * @param aNewBytes is the size of the requested buffer in bytes.
 * @param aMarginFrac is the headroom kept free, as a fraction of the allocation size.
 * @return the resize strategy to use.
 */
GAL_API VRAM_RESIZE_STRATEGY chooseResizeStrategy( size_t aFreeVRAM, size_t aOldBytes,
                                                   size_t aNewBytes, double aMarginFrac );

} // namespace KIGFX

#endif /* __OPENGL_ERROR_H */
