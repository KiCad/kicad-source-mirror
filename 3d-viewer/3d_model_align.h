/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KICAD_3D_MODEL_ALIGN_H
#define KICAD_3D_MODEL_ALIGN_H

#include <glm/glm.hpp>
#include <array>
#include <string>
#include <vector>

struct S3DMODEL;

namespace MODEL_ALIGN
{
struct REGION
{
    std::vector<std::array<unsigned int, 2>> sourceTriangles; ///< Mesh/triangle pairs, including reversed duplicates.
    std::vector<glm::dvec3>                  vertices;        ///< Scaled model coordinates, millimetres.
    std::vector<std::array<unsigned int, 3>> triangles;
    unsigned int                             material = 0;
    double                                   area = 0;
    glm::dvec3                               centroid{ 0.0 };
    glm::dvec3                               normal{ 0.0 };
    glm::dvec3                               extents{ 0.0 }; ///< Sorted PCA extents, largest first.
    double                                   normalMagnitude = 0;
    bool                                     planar = false;
    bool                                     twoSided = false;
};

enum class PAD_ATTRIBUTE
{
    SMD,
    THROUGH_HOLE,
    OTHER
};

struct PAD
{
    std::string   number;
    PAD_ATTRIBUTE attribute = PAD_ATTRIBUTE::SMD;
    glm::dvec2    position{ 0.0 }; ///< Footprint-local millimetres, Y up.
    glm::dvec2    size{ 0.0 };
    glm::dvec2    drill{ 0.0 };
    double        rotation = 0; ///< Degrees in the Y-up frame.
};

enum class SOLUTION_KIND
{
    FULL,
    SUBSET,
    PARTIAL,
    SINGLE_PAIR
};

struct ALIGN_SOLUTION
{
    glm::dvec3    rotation{ 0.0 }; ///< GUI degrees of Rz * Ry * Rx; negate when storing in FP_3DMODEL.
    glm::dvec3    offset{ 0.0 };   ///< Footprint-local millimetres, Y up.
    unsigned int  matched = 0;
    unsigned int  total = 0;
    SOLUTION_KIND kind = SOLUTION_KIND::FULL;
};

std::vector<REGION> BuildRegions( const S3DMODEL& aModel, const glm::dvec3& aScale );

/** Return the clicked pad first, followed by its congruent, deduplicated peers. */
std::vector<PAD> BuildPadGroup( const std::vector<PAD>& aPads, size_t aClickedPad );

/** The first element of aPadGroup must be the clicked pad; all coordinates are millimetres, Y up. */
std::vector<ALIGN_SOLUTION> SolveAlignment( const std::vector<REGION>& aRegions, size_t aSeed,
                                            const std::vector<PAD>& aPadGroup, const std::vector<PAD>& aAllPads,
                                            const glm::dvec3& aCurrentRotation );
} // namespace MODEL_ALIGN

#endif
