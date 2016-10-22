/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file  cbvh_pbrt.h
 * @brief This BVH implementation is based on the source code implementation
 * from the book "Physically Based Rendering" (v2 and v3)
 *
 * Adaptions performed:
 *  - Types and class types adapted to KiCad project
 *  - Convert some source to build in the C++ specification of KiCad
 *  - Code style to match KiCad
 *  - Asserts converted
 *  - Use compare functions/structures for std::partition and std::nth_element
 *
 * The original source code have the following licence:
 *
 * "pbrt source code is Copyright(c) 1998-2015
 *                      Matt Pharr, Greg Humphreys, and Wenzel Jakob.
 *
 *  This file is part of pbrt.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 *  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 *  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
 *
 */


#ifndef _CBVH_PBRT_H_
#define _CBVH_PBRT_H_

#include "caccelerator.h"
#include <list>
#include <stdint.h>

// Forward Declarations
struct BVHBuildNode;
struct BVHPrimitiveInfo;
struct MortonPrimitive;

struct LinearBVHNode
{
    // 24 bytes
    CBBOX bounds;

    // 4 bytes
    union
    {
        int primitivesOffset;   ///< leaf
        int secondChildOffset;  ///< interior
    };

    // 4 bytes
    uint16_t nPrimitives;  ///< 0 -> interior node
    uint8_t  axis;         ///< interior node: xyz
    uint8_t  pad[1];       ///< ensure 32 byte total size
};


enum SPLITMETHOD
{
    SPLIT_MIDDLE,
    SPLIT_EQUALCOUNTS,
    SPLIT_SAH,
    SPLIT_HLBVH
};


class  CBVH_PBRT : public CGENERICACCELERATOR
{

public:
    CBVH_PBRT( const CGENERICCONTAINER &aObjectContainer,
               int aMaxPrimsInNode = 4,
               SPLITMETHOD aSplitMethod = SPLIT_SAH );

    ~CBVH_PBRT();

    // Imported from CGENERICACCELERATOR
    bool Intersect( const RAY &aRay, HITINFO &aHitInfo ) const override;
    bool Intersect( const RAY &aRay, HITINFO &aHitInfo, unsigned int aAccNodeInfo ) const override;
    bool Intersect( const RAYPACKET &aRayPacket, HITINFO_PACKET *aHitInfoPacket ) const override;
    bool IntersectP( const RAY &aRay, float aMaxDistance ) const override;

private:

    BVHBuildNode *recursiveBuild( std::vector<BVHPrimitiveInfo> &primitiveInfo,
                                  int start,
                                  int end,
                                  int *totalNodes,
                                  CONST_VECTOR_OBJECT &orderedPrims );

    BVHBuildNode *HLBVHBuild( const std::vector<BVHPrimitiveInfo> &primitiveInfo,
                              int *totalNodes,
                              CONST_VECTOR_OBJECT &orderedPrims );

    //!TODO: after implement memory arena, put const back to this functions
    BVHBuildNode *emitLBVH( BVHBuildNode *&buildNodes,
                            const std::vector<BVHPrimitiveInfo> &primitiveInfo,
                            MortonPrimitive *mortonPrims,
                            int nPrimitives,
                            int *totalNodes,
                            CONST_VECTOR_OBJECT &orderedPrims,
                            int *orderedPrimsOffset,
                            int bit );

    BVHBuildNode *buildUpperSAH( std::vector<BVHBuildNode *> &treeletRoots,
                                 int start,
                                 int end,
                                 int *totalNodes );

    int flattenBVHTree( BVHBuildNode *node,
                        uint32_t *offset );

    // BVH Private Data
    const int           m_maxPrimsInNode;
    SPLITMETHOD         m_splitMethod;
    CONST_VECTOR_OBJECT m_primitives;
    LinearBVHNode       *m_nodes;

    std::list<void *> m_addresses_pointer_to_mm_free;

    // Partition traversal
    unsigned int m_I[RAYPACKET_RAYS_PER_PACKET];
};

#endif  // _CBVH_PBRT_H_
