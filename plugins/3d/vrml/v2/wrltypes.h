/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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


// version of the VRML file being parsed
enum WRLVERSION
{
    VRML_INVALID = 0,   // not a valid VRML file
    VRML_V1,
    VRML_V2
};


// VRML Node Types
// These are used to look up node names and to quickly
// determine what routine to invoke to read a section of
// a file.
enum WRLNODES
{
    WRL2_BASE = 0,  // not really a VRML node but we need a top level virtual node
    WRL2_BEGIN,
    WRL2_ANCHOR = WRL2_BEGIN,
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
    WRL2_END
};


struct WRLVEC2F
{
    float x;
    float y;
};

struct WRLVEC3F
{
    float x;
    float y;
    float z;
};

struct WRLROTATION
{
    float x;
    float y;
    float z;
    float w;
};

#endif  // WRLTYPES_H
