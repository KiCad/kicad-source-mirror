/*
* This program source code file is part of KICAD, a free EDA CAD application.
*
* Copyright (C) 2016 Kicad Developers, see change_log.txt for contributors.
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

#ifndef GAL_GL_BUILTIN_SHADERS_H__
#define GAL_GL_BUILTIN_SHADERS_H__

namespace KIGFX {

    namespace BUILTIN_SHADERS {

        extern const char kicad_vertex_shader[];
        extern const char kicad_fragment_shader[];

        extern const char ssaa_x4_vertex_shader[];
        extern const char ssaa_x4_fragment_shader[];

        extern const char smaa_base_shader_p1[];
        extern const char smaa_base_shader_p2[];
        extern const char smaa_base_shader_p3[];
        extern const char smaa_base_shader_p4[];

        extern const char smaa_pass_1_vertex_shader[];
        extern const char smaa_pass_1_fragment_shader[];
        extern const char smaa_pass_2_vertex_shader[];
        extern const char smaa_pass_2_fragment_shader[];
        extern const char smaa_pass_3_vertex_shader[];
        extern const char smaa_pass_3_fragment_shader[];

    }
}

#endif
