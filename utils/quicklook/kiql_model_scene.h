/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-3.0.html
 */

#ifndef KIQL_MODEL_SCENE_H
#define KIQL_MODEL_SCENE_H

#import <SceneKit/SceneKit.h>

struct S3DMODEL;

/**
 * A built scene plus the framing the preview and the thumbnail both need.
 *
 * SceneKit owns the geometry once the scene is built, so the imported model may be released as
 * soon as KIQL_BuildScene() returns.
 */
struct KIQL_PRESENTATION
{
    SCNScene*  scene = nil;
    SCNNode*   camera = nil;
    SCNVector3 center = SCNVector3Make( 0, 0, 0 );
    CGFloat    radius = 0;

    explicit operator bool() const { return scene != nil && camera != nil && radius > 0; }
};

/**
 * Convert an imported model into a SceneKit scene.
 *
 * Returns an empty presentation when the model has no renderable geometry.  S3D::ImportModel has
 * already rejected non-finite vertices and out-of-range indices, so this does not re-check them.
 */
KIQL_PRESENTATION KIQL_BuildScene( const S3DMODEL& aModel );

#endif // KIQL_MODEL_SCENE_H
