/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Mario Luzeiro <mrluzeiro@gmail.com>
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
 */

#ifndef _VRML_AUX_H
#define _VRML_AUX_H

#include <fctsys.h>
#include <common.h>
#include <macros.h>
#include <base_struct.h>
#include <gal/opengl/glm/glm.hpp>
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

int read_NotImplemented( FILE* File, char closeChar);
int parseVertexList( FILE* File, std::vector< glm::vec3 > &dst_vector);
int parseVertex( FILE* File, glm::vec3 &dst_vertex );
int parseFloat( FILE* File, float *dst_float );
char* GetNextTag( FILE* File, char* tag );

#endif
