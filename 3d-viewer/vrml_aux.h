/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2015 Mario Luzeiro <mrluzeiro@gmail.com>
 * Copyright (C) 1992-2014 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file vrml_aux.h
 * @brief auxiliar functions to parse VRML files
 */

#ifndef _VRML_AUX_H
#define _VRML_AUX_H

#include <fctsys.h>
#include <common.h>
#include <macros.h>
#include <base_struct.h>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <vector>
#include <kicad_string.h>
#include <info3d_visu.h>
#ifdef __WXMAC__
#  ifdef __DARWIN__
#    include <OpenGL/glu.h>
#  else
#    include <glu.h>
#  endif
#else
#  include <GL/glu.h>
#endif
#include <wx/glcanvas.h>

/**
 * Function GetEpoxyThicknessBIU
 * skip a VRML block and eventualy internal blocks until it find the close char
 * @param File file to read from
 * @param closeChar the expected close char of the block
 * @return int - -1 if failed, 0 if OK
 */
int Read_NotImplemented( FILE* File, char closeChar);


/**
 * Function ParseVertexList
 * parse a vertex list
 * @param File file to read from
 * @param dst_vector destination vector list
 * @return int - -1 if failed, 0 if OK
 */
int ParseVertexList( FILE* File, std::vector< glm::vec3 > &dst_vector);


/**
 * Function ParseVertex
 * parse a vertex
 * @param File file to read from
 * @param dst_vertex destination vector
 * @return bool - return true if the 3 elements are read
 */
bool ParseVertex( FILE* File, glm::vec3 &dst_vertex );


/**
 * Function ParseFloat
 * parse a float value
 * @param aFile file to read from
 * @param aDstFloat destination float
 * @param aDefaultValue = the default value, when the actual value cannot be read
 * @return bool - Return true if the float was read without error
 */
bool ParseFloat( FILE* aFile, float *aDstFloat, float aDefaultValue );

/**
 * Function GetNextTag
 * parse the next tag
 * @param File file to read from
 * @param tag destination pointer
 * @param len max length of storage
 * @return bool - true if succeeded, false if EOF
 */
bool GetNextTag( FILE* File, char* tag, size_t len );

/**
 * Function GetString
 * parse a string, it expects starting by " and end with "
 * @param File file to read from
 * @param aDstString destination pointer
 * @param maxDstLen max length of storage
 * @return bool - true if successful read the string, false if failed to get a string
 */
bool GetString( FILE* File, char* aDstString, size_t maxDstLen );

#endif
