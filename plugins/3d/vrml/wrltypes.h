/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file wrltypes.h
 * declares some compound types used for VRML
 */


#ifndef WRLTYPES_H
#define WRLTYPES_H

#include <wx/defs.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

// Mask for VRML tracing.
extern const wxChar* const traceVrmlPlugin;

// version of the VRML file being parsed
enum class WRLVERSION
{
    VRML_INVALID = 0,   // not a valid VRML file
    VRML_V1,
    VRML_V2
};


// VRML1 Node Types
// These are used to look up node names and to quickly
// determine what routine to invoke to read a section of
// a file.
enum class WRL1NODES
{
    WRL1_BASE = 0,  // not really a VRML node but we need a top level virtual node
    WRL1_BEGIN,
    WRL1_ASCIITEXT = WRL1_BEGIN,
    WRL1_CONE,
    WRL1_COORDINATE3,
    WRL1_CUBE,
    WRL1_CYLINDER,
    WRL1_DIRECTIONALLIGHT,
    WRL1_FONTSTYLE,
    WRL1_GROUP,
    WRL1_INDEXEDFACESET,
    WRL1_INDEXEDLINESET,
    WRL1_INFO,
    WRL1_LOD,
    WRL1_MATERIAL,
    WRL1_MATERIALBINDING,
    WRL1_MATRIXTRANSFORM,
    WRL1_NORMAL,
    WRL1_NORMALBINDING,
    WRL1_ORTHOCAMERA,
    WRL1_PERSPECTIVECAMERA,
    WRL1_POINTLIGHT,
    WRL1_POINTSET,
    WRL1_ROTATION,
    WRL1_SCALE,
    WRL1_SEPARATOR,
    WRL1_SHAPEHINTS,
    WRL1_SPHERE,
    WRL1_SPOTLIGHT,
    WRL1_SWITCH,
    WRL1_TEXTURE2,
    WRL1_TEXTURE2TRANSFORM,
    WRL1_TEXTURECOORDINATE2,
    WRL1_TRANSFORM,
    WRL1_TRANSLATION,
    WRL1_WWWANCHOR,
    WRL1_WWWINLINE,
    WRL1_INVALID,
    WRL1_END = WRL1_INVALID
};

// VRML1 Material/Normal Binding values
// note: PART/FACE have the same meaning in the specification
enum class WRL1_BINDING
{
    BIND_DEFAULT = 0,
    BIND_OVERALL,
    BIND_PER_PART,
    BIND_PER_FACE = BIND_PER_PART,
    BIND_PER_PART_INDEXED,
    BIND_PER_FACE_INDEXED = BIND_PER_PART_INDEXED,
    BIND_PER_VERTEX,
    BIND_PER_VERTEX_INDEXED,
    BIND_END
};

enum class WRL1_ORDER
{
    ORD_UNKNOWN = 0,
    ORD_CLOCKWISE,
    ORD_CCW
};

// VRML2 Node Types
// These are used to look up node names and to quickly
// determine what routine to invoke to read a section of
// a file.
enum class WRL2NODES
{
    WRL2_BASE = 0,  // not really a VRML node but we need a top level virtual node
    WRL2_BEGIN,
    WRL2_ANCHOR = WRL2NODES::WRL2_BEGIN,
    WRL2_APPEARANCE,
    WRL2_AUDIOCLIP,
    WRL2_BACKGROUND,
    WRL2_BILLBOARD,
    WRL2_BOX,
    WRL2_COLLISION,
    WRL2_COLOR,
    WRL2_COLORINTERPOLATOR,
    WRL2_CONE,
    WRL2_COORDINATE,
    WRL2_COORDINATEINTERPOLATOR,
    WRL2_CYLINDER,
    WRL2_CYLINDERSENSOR,
    WRL2_DIRECTIONALLIGHT,
    WRL2_ELEVATIONGRID,
    WRL2_EXTRUSION,
    WRL2_FOG,
    WRL2_FONTSTYLE,
    WRL2_GROUP,
    WRL2_IMAGETEXTURE,
    WRL2_INDEXEDFACESET,
    WRL2_INDEXEDLINESET,
    WRL2_INLINE,
    WRL2_LOD,
    WRL2_MATERIAL,
    WRL2_MOVIETEXTURE,
    WRL2_NAVIGATIONINFO,
    WRL2_NORMAL,
    WRL2_NORMALINTERPOLATOR,
    WRL2_ORIENTATIONINTERPOLATOR,
    WRL2_PIXELTEXTURE,
    WRL2_PLANESENSOR,
    WRL2_POINTLIGHT,
    WRL2_POINTSET,
    WRL2_POSITIONINTERPOLATOR,
    WRL2_PROXIMITYSENSOR,
    WRL2_SCALARINTERPOLATOR,
    WRL2_SCRIPT,
    WRL2_SHAPE,
    WRL2_SOUND,
    WRL2_SPHERE,
    WRL2_SPHERESENSOR,
    WRL2_SPOTLIGHT,
    WRL2_SWITCH,
    WRL2_TEXT,
    WRL2_TEXTURECOORDINATE,
    WRL2_TEXTURETRANSFORM,
    WRL2_TIMESENSOR,
    WRL2_TOUCHSENSOR,
    WRL2_TRANSFORM,
    WRL2_VIEWPOINT,
    WRL2_VISIBILITYSENSOR,
    WRL2_WORLDINFO,
    WRL2_INVALID,
    WRL2_END = WRL2_INVALID
};


typedef glm::vec2 WRLVEC2F;
typedef glm::vec3 WRLVEC3F;
typedef glm::vec4 WRLROTATION;

#endif  // WRLTYPES_H
