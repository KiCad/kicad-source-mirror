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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef __OPENGL_UTILS_H
#define __OPENGL_UTILS_H

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

#endif /* __OPENGL_ERROR_H */
