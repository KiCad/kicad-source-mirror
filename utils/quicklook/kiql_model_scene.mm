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

#import "kiql_model_scene.h"

#import <AppKit/AppKit.h>

#include <algorithm>
#include <cmath>
#include <limits>

#include <plugins/3dapi/c3dmodel.h>

namespace
{
NSColor* toColor( const SFVEC3F& aColor )
{
    return [NSColor colorWithSRGBRed:aColor.x green:aColor.y blue:aColor.z alpha:1.0];
}


/// SFVEC3F is three packed floats, so SceneKit can read the importer's arrays without a copy
/// into a wider stride.
SCNGeometrySource* vectorSource( const SFVEC3F* aVectors, unsigned int aCount, SCNGeometrySourceSemantic aSemantic )
{
    NSData* data = [NSData dataWithBytes:aVectors length:aCount * sizeof( SFVEC3F )];

    return [SCNGeometrySource geometrySourceWithData:data
                                            semantic:aSemantic
                                         vectorCount:aCount
                                     floatComponents:YES
                                 componentsPerVector:3
                                   bytesPerComponent:sizeof( float )
                                          dataOffset:0
                                          dataStride:sizeof( SFVEC3F )];
}


/// A camera-attached headlight leaves flat parts uniformly shaded, so the preview gets an
/// explicit rig: a key light off the camera axis for shape, a weaker fill opposite it to keep the
/// shadow side readable, and ambient so nothing is pure black.
void addLighting( SCNScene* aScene, SCNVector3 aCenter, CGFloat aRadius )
{
    SCNLight* ambient = [SCNLight light];
    ambient.type = SCNLightTypeAmbient;
    ambient.color = [NSColor colorWithWhite:0.45 alpha:1.0];
    SCNNode* ambientNode = [SCNNode node];
    ambientNode.light = ambient;
    [aScene.rootNode addChildNode:ambientNode];

    const CGFloat distance = std::max<CGFloat>( aRadius * 4.0, 1.0 );

    struct LIGHT_PLACEMENT
    {
        CGFloat x, y, z, intensity;
    };

    const LIGHT_PLACEMENT placements[] = { { -0.6, 0.5, 1.0, 1000 }, { 0.8, -0.4, 0.4, 350 } };

    for( const LIGHT_PLACEMENT& placement : placements )
    {
        SCNLight* light = [SCNLight light];
        light.type = SCNLightTypeDirectional;
        light.intensity = placement.intensity;

        SCNNode* target = [SCNNode node];
        target.position = aCenter;
        [aScene.rootNode addChildNode:target];

        SCNNode* node = [SCNNode node];
        node.light = light;
        node.position = SCNVector3Make( aCenter.x + placement.x * distance, aCenter.y + placement.y * distance,
                                        aCenter.z + placement.z * distance );
        node.constraints = @[ [SCNLookAtConstraint lookAtConstraintWithTarget:target] ];
        [aScene.rootNode addChildNode:node];
    }
}


/// Frame the model from three quarters.  Looking straight down an axis renders most board models
/// as a flat silhouette, which tells the user nothing about the part.
SCNNode* makeCameraNode( SCNScene* aScene, SCNVector3 aCenter, CGFloat aRadius )
{
    SCNCamera* camera = [SCNCamera camera];
    camera.zNear = std::max( aRadius * 0.001, 0.001 );
    camera.zFar = std::max( aRadius * 20.0, 100.0 );

    const CGFloat offset = aRadius * 2.5 / std::sqrt( 3.0 );

    SCNNode* target = [SCNNode node];
    target.position = aCenter;
    [aScene.rootNode addChildNode:target];

    SCNLookAtConstraint* lookAt = [SCNLookAtConstraint lookAtConstraintWithTarget:target];
    lookAt.gimbalLockEnabled = YES;

    SCNNode* node = [SCNNode node];
    node.camera = camera;
    node.position = SCNVector3Make( aCenter.x + offset, aCenter.y - offset, aCenter.z + offset );
    node.constraints = @[ lookAt ];
    [aScene.rootNode addChildNode:node];

    return node;
}


SCNMaterial* toMaterial( const SMATERIAL& aMaterial, bool aHasVertexColors )
{
    SCNMaterial* material = [SCNMaterial material];
    material.lightingModelName = SCNLightingModelBlinn;
    material.ambient.contents = toColor( aMaterial.m_Ambient );
    material.diffuse.contents = aHasVertexColors ? [NSColor whiteColor] : toColor( aMaterial.m_Diffuse );
    material.emission.contents = toColor( aMaterial.m_Emissive );
    material.specular.contents = toColor( aMaterial.m_Specular );
    material.shininess = std::clamp( aMaterial.m_Shininess, 0.0f, 1.0f ) * 128.0;
    material.transparency = 1.0 - std::clamp( aMaterial.m_Transparency, 0.0f, 1.0f );
    material.doubleSided = YES;

    return material;
}
} // namespace


KIQL_PRESENTATION KIQL_BuildScene( const S3DMODEL& aModel )
{
    KIQL_PRESENTATION presentation;

    if( !aModel.m_Meshes || !aModel.m_MeshesSize || !aModel.m_Materials || !aModel.m_MaterialsSize )
        return presentation;

    SCNScene* scene = [SCNScene scene];
    SFVEC3F   minimum( std::numeric_limits<float>::infinity() );
    SFVEC3F   maximum( -std::numeric_limits<float>::infinity() );
    bool      hasGeometry = false;

    for( unsigned int meshIndex = 0; meshIndex < aModel.m_MeshesSize; ++meshIndex )
    {
        const SMESH& mesh = aModel.m_Meshes[meshIndex];

        if( !mesh.m_Positions || !mesh.m_Normals || !mesh.m_FaceIdx || !mesh.m_VertexSize
            || !mesh.m_FaceIdxSize || mesh.m_FaceIdxSize % 3 || mesh.m_MaterialIdx >= aModel.m_MaterialsSize )
        {
            continue;
        }

        NSMutableArray<SCNGeometrySource*>* sources = [NSMutableArray arrayWithObjects:
                vectorSource( mesh.m_Positions, mesh.m_VertexSize, SCNGeometrySourceSemanticVertex ),
                vectorSource( mesh.m_Normals, mesh.m_VertexSize, SCNGeometrySourceSemanticNormal ), nil];

        if( mesh.m_Color )
            [sources addObject:vectorSource( mesh.m_Color, mesh.m_VertexSize, SCNGeometrySourceSemanticColor )];

        NSData* indexData = [NSData dataWithBytes:mesh.m_FaceIdx
                                           length:mesh.m_FaceIdxSize * sizeof( unsigned int )];
        SCNGeometryElement* element =
                [SCNGeometryElement geometryElementWithData:indexData
                                              primitiveType:SCNGeometryPrimitiveTypeTriangles
                                             primitiveCount:mesh.m_FaceIdxSize / 3
                                              bytesPerIndex:sizeof( unsigned int )];

        SCNGeometry* geometry = [SCNGeometry geometryWithSources:sources elements:@[ element ]];
        geometry.materials = @[ toMaterial( aModel.m_Materials[mesh.m_MaterialIdx], mesh.m_Color != nullptr ) ];
        [scene.rootNode addChildNode:[SCNNode nodeWithGeometry:geometry]];

        for( unsigned int vertex = 0; vertex < mesh.m_VertexSize; ++vertex )
        {
            minimum = glm::min( minimum, mesh.m_Positions[vertex] );
            maximum = glm::max( maximum, mesh.m_Positions[vertex] );
        }

        hasGeometry = true;
    }

    if( !hasGeometry )
        return presentation;

    const SFVEC3F center = ( minimum + maximum ) * 0.5f;
    const float   radius = glm::length( maximum - minimum ) * 0.5f;

    if( !std::isfinite( center.x ) || !std::isfinite( center.y ) || !std::isfinite( center.z )
        || !std::isfinite( radius ) || radius <= 0.0f )
    {
        return presentation;
    }

    presentation.scene = scene;
    presentation.center = SCNVector3Make( center.x, center.y, center.z );
    presentation.radius = radius;

    // Built once here so neither consumer can add a second camera or light rig to a scene it
    // does not own.
    addLighting( scene, presentation.center, radius );
    presentation.camera = makeCameraNode( scene, presentation.center, radius );

    return presentation;
}
