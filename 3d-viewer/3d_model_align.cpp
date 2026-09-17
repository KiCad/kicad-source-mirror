/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "3d_model_align.h"
#include <core/union_find.h>
#include <plugins/3dapi/c3dmodel.h>
#include <Eigen/Dense>
#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <map>
#include <numeric>
#include <utility>

namespace MODEL_ALIGN
{
namespace
{
    using V2 = Eigen::Vector2d;
    using V3 = Eigen::Vector3d;
    using M3 = Eigen::Matrix3d;
    constexpr double PI = 3.14159265358979323846;
    constexpr double INF = std::numeric_limits<double>::infinity();

    V3 vec( const glm::dvec3& aValue )
    {
        return V3( aValue.x, aValue.y, aValue.z );
    }
    V2 vec( const glm::dvec2& aValue )
    {
        return V2( aValue.x, aValue.y );
    }
    glm::dvec3 vec( const V3& aValue )
    {
        return { aValue.x(), aValue.y(), aValue.z() };
    }

    using BOX2 = Eigen::AlignedBox2d;
    using BOX3 = Eigen::AlignedBox3d;

    struct CONTACT
    {
        V3     position;
        double weight;
    };

    struct ALIGN_CANDIDATE
    {
        M3            rotation;
        V3            offset;
        unsigned int  matched;
        unsigned int  total;
        SOLUTION_KIND kind;
        double        copper = 0;
        double        turn = 0;
    };

    void describe( REGION& aRegion )
    {
        V3 centroid = V3::Zero();
        V3 normal = V3::Zero();

        for( const auto& triangle : aRegion.triangles )
        {
            V3     a = vec( aRegion.vertices[triangle[0]] );
            V3     b = vec( aRegion.vertices[triangle[1]] );
            V3     c = vec( aRegion.vertices[triangle[2]] );
            V3     n = ( b - a ).cross( c - a ) / 2;
            double area = n.norm();
            aRegion.area += area;
            centroid += area * ( a + b + c ) / 3;
            normal += n;
        }

        if( aRegion.area <= 0 || !std::isfinite( aRegion.area ) || !centroid.allFinite() || !normal.allFinite() )
        {
            aRegion.area = 0;
            return;
        }

        centroid /= aRegion.area;
        M3 covariance = M3::Zero();

        for( const auto& vertex : aRegion.vertices )
        {
            V3 delta = vec( vertex ) - centroid;
            covariance += delta * delta.transpose();
        }

        if( !covariance.allFinite() )
        {
            aRegion.area = 0;
            return;
        }

        Eigen::SelfAdjointEigenSolver<M3> eigen( covariance );

        if( eigen.info() != Eigen::Success )
        {
            aRegion.area = 0;
            return;
        }

        BOX3 box;

        for( const auto& vertex : aRegion.vertices )
            box.extend( V3( eigen.eigenvectors().transpose() * ( vec( vertex ) - centroid ) ) );

        V3 extents = box.sizes();
        std::sort( extents.data(), extents.data() + 3, std::greater<double>() );
        aRegion.centroid = vec( centroid );
        aRegion.extents = vec( extents );
        aRegion.planar = extents[2] < 1e-3 + 1e-3 * extents[0];
        aRegion.normalMagnitude = normal.norm() / aRegion.area;

        if( normal.norm() > 0 )
            aRegion.normal = vec( V3( normal.normalized() ) );
    }

    bool congruent( const REGION& aLeft, const REGION& aRight, double aLength )
    {
        if( aLeft.material != aRight.material || aLeft.planar != aRight.planar
            || std::abs( aLeft.area - aRight.area ) > 0.05 * std::max( aLeft.area, aRight.area ) )
            return false;

        for( int axis = 0; axis < 3; ++axis )
        {
            if( std::abs( aLeft.extents[axis] - aRight.extents[axis] )
                > std::max( 0.05 * std::max( aLeft.extents[axis], aRight.extents[axis] ), 1e-3 * aLength ) )
                return false;
        }

        return true;
    }

    V2 padLocal( const PAD& aPad, const V2& aPoint )
    {
        return Eigen::Rotation2Dd( -aPad.rotation * PI / 180 ) * ( aPoint - vec( aPad.position ) );
    }

    bool overDrill( const PAD& aPad, const V2& aPoint, double aGrow = 0 )
    {
        if( aPad.drill.x <= 0 || aPad.drill.y <= 0 )
            return false;

        V2     local = padLocal( aPad, aPoint );
        int    major = aPad.drill.x >= aPad.drill.y ? 0 : 1;
        double radius = std::min( aPad.drill.x, aPad.drill.y ) / 2 + aGrow;
        double halfSegment = std::abs( aPad.drill.x - aPad.drill.y ) / 2;
        local[major] -= std::clamp( local[major], -halfSegment, halfSegment );
        return local.squaredNorm() <= radius * radius;
    }

    bool padContains( const PAD& aPad, const V2& aPoint, double aGrow )
    {
        if( aPad.attribute == PAD_ATTRIBUTE::THROUGH_HOLE )
            return overDrill( aPad, aPoint, aGrow );

        V2 local = padLocal( aPad, aPoint );
        return std::abs( local.x() ) <= aPad.size.x / 2 + aGrow && std::abs( local.y() ) <= aPad.size.y / 2 + aGrow;
    }

    std::vector<int> match( const std::vector<V2>& aPoints, const std::vector<PAD>& aPads, bool aContainment,
                            double aGrow )
    {
        std::vector<int>  assignment( aPoints.size(), -1 );
        std::vector<bool> used( aPads.size(), false );

        for( size_t i = 0; i < aPoints.size(); ++i )
        {
            int    best = -1;
            double distance = INF;

            for( size_t j = 0; j < aPads.size(); ++j )
            {
                double trial = ( aPoints[i] - vec( aPads[j].position ) ).squaredNorm();

                if( trial < distance )
                {
                    best = static_cast<int>( j );
                    distance = trial;
                }
            }

            if( best >= 0 && !used[best] && ( !aContainment || padContains( aPads[best], aPoints[i], aGrow ) ) )
            {
                assignment[i] = best;
                used[best] = true;
            }
        }

        return assignment;
    }

    size_t countMatches( const std::vector<int>& aAssignment )
    {
        return std::count_if( aAssignment.begin(), aAssignment.end(),
                              []( int aIndex )
                              {
                                  return aIndex >= 0;
                              } );
    }

    BOX2 bodyBox( const std::vector<REGION>& aRegions, const M3& aRotation, const V3& aOffset )
    {
        BOX2 box;

        for( const REGION& region : aRegions )
        {
            for( const auto& vertex : region.vertices )
                box.extend( V3( aRotation * vec( vertex ) + aOffset ).head<2>() );
        }

        return box;
    }

    bool geometryGate( ALIGN_CANDIDATE& aCandidate, const std::vector<REGION>& aRegions, const std::vector<PAD>& aPads,
                       const std::vector<PAD>& aAllPads, const std::vector<CONTACT>& aContacts, double aLevel,
                       const V3& aDown, double aEpsilon, double aGrow )
    {
        bool   throughHole = aPads.front().attribute == PAD_ATTRIBUTE::THROUGH_HOLE;
        double bottom = INF;

        auto hole = [&]( const V3& aPoint, double aMargin )
        {
            return std::any_of( aAllPads.begin(), aAllPads.end(),
                                [&]( const PAD& aPad )
                                {
                                    return overDrill( aPad, aPoint.head<2>(), aMargin );
                                } );
        };

        if( throughHole )
        {
            for( const REGION& region : aRegions )
            {
                for( const auto& vertex : region.vertices )
                {
                    V3 point = aCandidate.rotation * vec( vertex ) + aCandidate.offset;

                    if( !hole( point, 0 ) )
                        bottom = std::min( bottom, point.z() );
                }
            }
        }
        else
        {
            bottom = ( aCandidate.rotation * ( aDown * aLevel ) ).z();
        }

        if( !std::isfinite( bottom ) )
            return false;

        aCandidate.offset.z() -= bottom;
        double below = 0;
        double total = std::accumulate( aRegions.begin(), aRegions.end(), 0.0,
                                        []( double aTotal, const REGION& aRegion )
                                        {
                                            return aTotal + aRegion.area;
                                        } );
        double bodyZ = 0;

        for( const REGION& region : aRegions )
        {
            V3 point = aCandidate.rotation * vec( region.centroid ) + aCandidate.offset;
            bodyZ += region.area * point.z();

            if( throughHole )
                continue;

            for( const auto& triangle : region.triangles )
            {
                std::array<V3, 3> vertices;

                for( int corner = 0; corner < 3; ++corner )
                    vertices[corner] =
                            aCandidate.rotation * vec( region.vertices[triangle[corner]] ) + aCandidate.offset;

                std::array<V3, 4> clipped;
                size_t            clippedCount = 0;

                for( int edge = 0; edge < 3; ++edge )
                {
                    const V3& a = vertices[edge];
                    const V3& b = vertices[( edge + 1 ) % 3];
                    bool      aBelow = a.z() < -aEpsilon;
                    bool      bBelow = b.z() < -aEpsilon;

                    if( aBelow )
                        clipped[clippedCount++] = a;

                    if( aBelow != bBelow )
                        clipped[clippedCount++] = a + ( b - a ) * ( ( -aEpsilon - a.z() ) / ( b.z() - a.z() ) );
                }

                for( size_t corner = 1; corner + 1 < clippedCount; ++corner )
                {
                    V3 centre = ( clipped[0] + clipped[corner] + clipped[corner + 1] ) / 3;

                    if( !hole( centre, aEpsilon ) )
                        below += ( clipped[corner] - clipped[0] ).cross( clipped[corner + 1] - clipped[0] ).norm() / 2;

                    if( below > 0.02 * total )
                        return false;
                }
            }
        }

        if( total <= 0 || ( !throughHole && below > 0.02 * total ) || ( throughHole && bodyZ < 0 ) )
            return false;

        std::vector<V2> finalPoints;

        for( const CONTACT& contact : aContacts )
        {
            V3 point = aCandidate.rotation * contact.position + aCandidate.offset;
            finalPoints.push_back( point.head<2>() );

            if( throughHole && point.z() > -aEpsilon )
                return false;
        }

        aCandidate.matched = countMatches( match( finalPoints, aPads, true, aGrow ) );
        return aCandidate.kind == SOLUTION_KIND::PARTIAL ? aCandidate.matched >= std::ceil( 0.9 * aContacts.size() )
                                                         : aCandidate.matched == aContacts.size();
    }

    void centreSlack( ALIGN_CANDIDATE& aCandidate, const std::vector<REGION>& aRegions,
                      const std::vector<CONTACT>& aContacts, const std::vector<PAD>& aPads, const V2& aPadsCentre,
                      const std::vector<int>& aAssignment )
    {
        V2 low = V2::Constant( -INF );
        V2 high = V2::Constant( INF );

        for( size_t i = 0; i < aContacts.size(); ++i )
        {
            const PAD& pad = aPads[aAssignment[i]];
            V3         point = aCandidate.rotation * aContacts[i].position + aCandidate.offset;
            double     angle = std::remainder( pad.rotation, 90.0 );

            // Axis-aligned translation boxes are exact for the normal footprint pad orientations.
            if( std::abs( angle ) > 1e-6 )
                return;

            bool swap = std::abs( std::remainder( pad.rotation, 180.0 ) ) > 45;
            V2   half( ( swap ? pad.size.y : pad.size.x ) / 2, ( swap ? pad.size.x : pad.size.y ) / 2 );

            if( pad.attribute == PAD_ATTRIBUTE::THROUGH_HOLE )
                half.setZero();

            low = low.cwiseMax( vec( pad.position ) - half - point.head<2>() );
            high = high.cwiseMin( vec( pad.position ) + half - point.head<2>() );
        }

        if( ( low.array() <= high.array() ).all() )
        {
            V2 modelCentre = bodyBox( aRegions, aCandidate.rotation, aCandidate.offset ).center();
            aCandidate.offset.head<2>() += ( aPadsCentre - modelCentre ).cwiseMax( low ).cwiseMin( high );
        }
    }
} // namespace

std::vector<REGION> BuildRegions( const S3DMODEL& aModel, const glm::dvec3& aScale )
{
    std::vector<REGION> result;

    if( !aModel.m_Meshes || !std::isfinite( aScale.x ) || !std::isfinite( aScale.y ) || !std::isfinite( aScale.z )
        || aScale.x == 0 || aScale.y == 0 || aScale.z == 0 )
        return result;

    using POINT_KEY = std::array<double, 3>;
    using TRIANGLE_KEY = std::array<POINT_KEY, 3>;
    std::map<std::pair<unsigned int, std::vector<TRIANGLE_KEY>>, size_t> known;
    auto makeSignature = []( const REGION& aRegion, bool aOriented, bool aReverse )
    {
        std::vector<TRIANGLE_KEY> signature;
        signature.reserve( aRegion.triangles.size() );

        for( const auto& triangle : aRegion.triangles )
        {
            TRIANGLE_KEY key;

            for( unsigned int corner = 0; corner < 3; ++corner )
            {
                const auto& point = aRegion.vertices[triangle[corner]];
                key[corner] = { point.x, point.y, point.z };
            }

            if( aReverse )
                std::swap( key[1], key[2] );

            if( aOriented )
                std::rotate( key.begin(), std::min_element( key.begin(), key.end() ), key.end() );
            else
                std::sort( key.begin(), key.end() );

            signature.push_back( key );
        }

        std::sort( signature.begin(), signature.end() );
        return signature;
    };

    for( unsigned int meshIndex = 0; meshIndex < aModel.m_MeshesSize; ++meshIndex )
    {
        const SMESH& mesh = aModel.m_Meshes[meshIndex];

        if( !mesh.m_Positions || !mesh.m_FaceIdx )
            continue;

        KI_UNION_FIND             connected( mesh.m_VertexSize );
        std::vector<unsigned int> valid;

        for( unsigned int offset = 0; offset + 2 < mesh.m_FaceIdxSize; offset += 3 )
        {
            if( !IsTriangleInRange( mesh.m_FaceIdx, offset, mesh.m_VertexSize ) )
                continue;

            bool finite = true;

            for( unsigned int corner = 0; corner < 3; ++corner )
            {
                const auto& point = mesh.m_Positions[mesh.m_FaceIdx[offset + corner]];
                finite &= std::isfinite( point.x * aScale.x ) && std::isfinite( point.y * aScale.y )
                          && std::isfinite( point.z * aScale.z );
            }

            if( !finite )
                continue;

            valid.push_back( offset );
            connected.Unite( mesh.m_FaceIdx[offset], mesh.m_FaceIdx[offset + 1] );
            connected.Unite( mesh.m_FaceIdx[offset], mesh.m_FaceIdx[offset + 2] );
        }

        std::map<size_t, std::vector<unsigned int>> components;

        for( unsigned int offset : valid )
            components[connected.FindCompress( mesh.m_FaceIdx[offset] )].push_back( offset );

        for( const auto& [component, offsets] : components )
        {
            REGION region;
            region.material = mesh.m_MaterialIdx;
            std::map<unsigned int, unsigned int> indices;

            for( unsigned int offset : offsets )
            {
                std::array<unsigned int, 3> triangle;

                for( unsigned int corner = 0; corner < 3; ++corner )
                {
                    unsigned int source = mesh.m_FaceIdx[offset + corner];
                    auto [entry, inserted] = indices.emplace( source, indices.size() );

                    if( inserted )
                    {
                        const auto& point = mesh.m_Positions[source];
                        region.vertices.push_back( glm::dvec3( point.x, point.y, point.z ) * aScale );
                    }

                    triangle[corner] = entry->second;
                }

                if( aScale.x * aScale.y * aScale.z < 0 )
                    std::swap( triangle[1], triangle[2] );

                region.triangles.push_back( triangle );
                region.sourceTriangles.push_back( { meshIndex, offset / 3 } );
            }

            describe( region );

            if( region.area <= 0 )
                continue;

            auto [previous, inserted] =
                    known.emplace( std::make_pair( region.material, makeSignature( region, false, false ) ),
                                   result.size() );

            if( !inserted )
            {
                REGION& original = result[previous->second];
                original.twoSided |= makeSignature( original, true, true ) == makeSignature( region, true, false );
                original.sourceTriangles.insert( original.sourceTriangles.end(), region.sourceTriangles.begin(),
                                                 region.sourceTriangles.end() );
                continue;
            }

            result.push_back( std::move( region ) );
        }
    }

    return result;
}

std::vector<PAD> BuildPadGroup( const std::vector<PAD>& aPads, size_t aClickedPad )
{
    std::vector<PAD> result;

    if( aClickedPad >= aPads.size() || aPads[aClickedPad].attribute == PAD_ATTRIBUTE::OTHER )
        return result;

    const PAD& seed = aPads[aClickedPad];
    result.push_back( seed );
    auto dimensions = []( const PAD& aPad )
    {
        glm::dvec2 size = aPad.attribute == PAD_ATTRIBUTE::THROUGH_HOLE ? aPad.drill : aPad.size;
        return V2( std::max( size.x, size.y ), std::min( size.x, size.y ) );
    };

    for( const PAD& pad : aPads )
    {
        if( pad.attribute != seed.attribute || ( dimensions( pad ) - dimensions( seed ) ).norm() > 1e-6 )
            continue;

        bool duplicate = std::any_of( result.begin(), result.end(),
                                      [&]( const PAD& aPrevious )
                                      {
                                          return pad.number == aPrevious.number
                                                 && glm::length( pad.position - aPrevious.position ) < 1e-6;
                                      } );

        if( !duplicate )
            result.push_back( pad );
    }

    return result;
}

std::vector<ALIGN_SOLUTION> SolveAlignment( const std::vector<REGION>& aRegions, size_t aSeed,
                                            const std::vector<PAD>& aPadGroup, const std::vector<PAD>& aAllPads,
                                            const glm::dvec3& aCurrentRotation )
{
    std::vector<ALIGN_SOLUTION> result;

    if( aSeed >= aRegions.size() || aPadGroup.empty() || aAllPads.empty() )
        return result;

    const REGION& seed = aRegions[aSeed];
    BOX3          modelBox;
    BOX2          allPadBox;

    for( const REGION& region : aRegions )
    {
        for( const auto& vertex : region.vertices )
            modelBox.extend( vec( vertex ) );
    }

    for( const PAD& pad : aAllPads )
        allPadBox.extend( vec( pad.position ) );

    double length = modelBox.sizes().norm();

    if( !std::isfinite( length ) )
        return result;

    double epsilon = std::max( 1e-3 * length, 0.01 );
    double grow = seed.extents.y / 2;
    V2     padCentre = V2::Zero();
    BOX2   padBox;

    for( const PAD& pad : aPadGroup )
    {
        padCentre += vec( pad.position );
        padBox.extend( vec( pad.position ) );
    }

    padCentre /= aPadGroup.size();
    std::vector<V3> directions{ V3::UnitX(), -V3::UnitX(), V3::UnitY(), -V3::UnitY(), V3::UnitZ(), -V3::UnitZ() };
    V3              normal = vec( seed.normal );

    if( normal.norm() > 0 && normal.cwiseAbs().maxCoeff() < std::cos( 0.5 * PI / 180 ) )
    {
        directions.push_back( normal );

        if( seed.twoSided )
            directions.push_back( -normal );
    }

    std::vector<ALIGN_CANDIDATE> full;
    std::vector<ALIGN_CANDIDATE> partial;
    std::vector<ALIGN_CANDIDATE> single;
    double                       minimumPitch = -1;

    for( const V3& down : directions )
    {
        double facing = normal.dot( down );

        if( seed.twoSided )
            facing = std::abs( facing );

        if( facing < 0.25 && ( seed.planar || seed.normalMagnitude > 0.3 ) )
            continue;

        std::vector<double> levels;

        for( const REGION& region : aRegions )
        {
            double level = -INF;

            for( const auto& vertex : region.vertices )
                level = std::max( level, vec( vertex ).dot( down ) );

            levels.push_back( level );
        }

        double               level = levels[aSeed];
        std::vector<CONTACT> contacts;
        V3                   seedContact = vec( seed.centroid );

        for( size_t i = 0; i < aRegions.size(); ++i )
        {
            const REGION& region = aRegions[i];

            double peerFacing = vec( region.normal ).dot( down );

            if( region.twoSided )
                peerFacing = std::abs( peerFacing );

            if( std::abs( levels[i] - level ) > epsilon || !congruent( seed, region, length )
                || ( region.planar && peerFacing < 0.25 ) )
                continue;

            V3     accumulator = V3::Zero();
            double weight = 0;

            for( const auto& triangle : region.triangles )
            {
                V3 a = vec( region.vertices[triangle[0]] );
                V3 b = vec( region.vertices[triangle[1]] );
                V3 c = vec( region.vertices[triangle[2]] );

                if( std::min( { a.dot( down ), b.dot( down ), c.dot( down ) } ) < level - epsilon )
                    continue;

                double area = ( b - a ).cross( c - a ).norm() / 2;
                accumulator += area * ( a + b + c ) / 3;
                weight += area;
            }

            if( weight <= 0 )
            {
                for( const auto& vertex : region.vertices )
                {
                    if( vec( vertex ).dot( down ) >= level - epsilon )
                    {
                        accumulator += vec( vertex );
                        weight += 1;
                    }
                }
            }

            if( weight <= 0 )
                continue;

            CONTACT contact{ accumulator / weight, region.area };

            if( i == aSeed )
                seedContact = contact.position;

            auto peer = std::find_if( contacts.begin(), contacts.end(),
                                      [&]( const CONTACT& aContact )
                                      {
                                          return ( aContact.position - contact.position ).norm() < seed.extents.x / 2;
                                      } );

            if( peer == contacts.end() )
            {
                contacts.push_back( contact );
            }
            else
            {
                peer->position = ( peer->position * peer->weight + contact.position * contact.weight )
                                 / ( peer->weight + contact.weight );
                peer->weight += contact.weight;
            }
        }

        if( contacts.empty() )
            continue;

        M3 seat = down.z() > 1 - 1e-12
                          ? Eigen::AngleAxisd( PI, V3::UnitX() ).toRotationMatrix()
                          : Eigen::Quaterniond::FromTwoVectors( down, V3( 0, 0, -1 ) ).toRotationMatrix();
        BOX2            contactBox;
        V2              centre = V2::Zero();
        std::vector<V2> points;

        for( const CONTACT& contact : contacts )
        {
            V3 point = seat * contact.position;
            points.push_back( point.head<2>() );
            contactBox.extend( points.back() );
            centre += points.back();
        }

        centre /= points.size();
        V2 contactExtents = contactBox.sizes();
        V2 padExtents = padBox.sizes();
        std::sort( contactExtents.data(), contactExtents.data() + 2 );
        std::sort( padExtents.data(), padExtents.data() + 2 );
        bool subset = contacts.size() < aPadGroup.size() && contacts.size() >= 3
                      && ( contactExtents - padExtents ).cwiseAbs().maxCoeff() <= seed.extents.x / 2;

        if( ( contacts.size() != aPadGroup.size() && !subset ) || aPadGroup.size() == 1 )
        {
            double          bestDistance = INF;
            ALIGN_CANDIDATE best;
            bool            found = false;

            for( double angle : { 0.0, PI / 2, PI, -PI / 2 } )
            {
                M3 rotation = Eigen::AngleAxisd( angle, V3::UnitZ() ).toRotationMatrix() * seat;
                V3 offset = -rotation * seedContact;
                offset.head<2>() += vec( aPadGroup.front().position );
                offset.z() = 0;
                ALIGN_CANDIDATE candidate{ rotation, offset, 1, 1, SOLUTION_KIND::SINGLE_PAIR };

                if( !geometryGate( candidate, aRegions, aPadGroup, aAllPads, { { seedContact, 1 } }, level, down,
                                   epsilon, grow ) )
                    continue;

                V2     centre = bodyBox( aRegions, rotation, candidate.offset ).center();
                double distance = ( centre - allPadBox.center() ).squaredNorm();

                if( distance < bestDistance )
                {
                    bestDistance = distance;
                    best = candidate;
                    best.turn = std::abs( angle );
                    found = true;
                }
            }

            if( found )
                single.push_back( best );

            continue;
        }

        if( subset )
            centre = contactBox.center();

        V2 target = subset ? padBox.center() : padCentre;

        for( V2& point : points )
            point -= centre;

        std::vector<double> angles;

        if( down.cwiseAbs().maxCoeff() > std::cos( 0.5 * PI / 180 ) )
        {
            angles = { 0, PI / 2, PI, -PI / 2 };
        }
        else
        {
            auto   farthest = std::max_element( points.begin(), points.end(),
                                                []( const V2& a, const V2& b )
                                                {
                                                  return a.squaredNorm() < b.squaredNorm();
                                              } );
            double maxPad = 0;

            for( const PAD& pad : aPadGroup )
                maxPad = std::max( { maxPad, pad.size.x, pad.size.y } );

            for( const PAD& pad : aPadGroup )
            {
                V2 delta = vec( pad.position ) - target;

                if( std::abs( delta.norm() - farthest->norm() ) > maxPad / 2 )
                    continue;

                double angle = std::atan2( delta.y(), delta.x() ) - std::atan2( farthest->y(), farthest->x() );

                if( std::none_of( angles.begin(), angles.end(),
                                  [&]( double aOther )
                                  {
                                      return std::abs( std::remainder( angle - aOther, 2 * PI ) ) < PI / 180;
                                  } ) )
                    angles.push_back( angle );
            }
        }

        std::vector<const REGION*> copperRegions;

        for( size_t i = 0; i < aRegions.size(); ++i )
        {
            if( std::abs( levels[i] - level ) <= epsilon && !congruent( seed, aRegions[i], length ) )
                copperRegions.push_back( &aRegions[i] );
        }

        for( bool isPartial : { false, true } )
        {
            if( isPartial && !full.empty() )
                break;

            for( double angle : angles )
            {
                std::vector<V2> transformed;

                for( const V2& point : points )
                    transformed.push_back( Eigen::Rotation2Dd( angle ) * point + target );

                std::vector<int> assignment = match( transformed, aPadGroup, false, grow );

                if( !isPartial && countMatches( assignment ) < points.size() )
                    continue;

                V2 shift = V2::Zero();

                if( isPartial )
                {
                    for( int iteration = 0; iteration < 4; ++iteration )
                    {
                        std::vector<V2> shifted;

                        for( const V2& point : transformed )
                            shifted.push_back( point + shift );

                        assignment = match( shifted, aPadGroup, true, grow );
                        V2     accumulator = V2::Zero();
                        size_t count = countMatches( assignment );

                        for( size_t i = 0; i < shifted.size(); ++i )
                        {
                            if( assignment[i] >= 0 )
                                accumulator += vec( aPadGroup[assignment[i]].position ) - shifted[i];
                        }

                        if( count )
                            shift += accumulator / count;
                    }

                    if( minimumPitch < 0 )
                    {
                        minimumPitch = INF;

                        for( size_t i = 0; i < aPadGroup.size(); ++i )
                        {
                            for( size_t j = i + 1; j < aPadGroup.size(); ++j )
                            {
                                double distance = glm::length( aPadGroup[i].position - aPadGroup[j].position );

                                if( distance > 1e-6 )
                                    minimumPitch = std::min( minimumPitch, distance );
                            }
                        }
                    }

                    if( countMatches( assignment ) < std::ceil( 0.9 * points.size() )
                        || shift.norm() >= minimumPitch / 2 )
                        continue;
                }
                else
                {
                    double cross = 0;
                    double dot = 0;

                    for( size_t i = 0; i < points.size(); ++i )
                    {
                        V2 delta = vec( aPadGroup[assignment[i]].position ) - target;
                        cross += points[i].x() * delta.y() - points[i].y() * delta.x();
                        dot += points[i].dot( delta );
                    }

                    angle = std::atan2( cross, dot );
                    double snapped = std::round( angle / ( PI / 2 ) ) * ( PI / 2 );

                    if( std::abs( angle - snapped ) < 0.5 * PI / 180 )
                        angle = snapped;
                }

                M3              inPlane = Eigen::AngleAxisd( angle, V3::UnitZ() ).toRotationMatrix();
                ALIGN_CANDIDATE candidate{ inPlane * seat,
                                           V3( target.x() + shift.x(), target.y() + shift.y(), 0 )
                                                   - inPlane * V3( centre.x(), centre.y(), 0 ),
                                           0, static_cast<unsigned int>( contacts.size() ),
                                           isPartial ? SOLUTION_KIND::PARTIAL
                                           : subset  ? SOLUTION_KIND::SUBSET
                                                     : SOLUTION_KIND::FULL };
                candidate.turn = std::abs( std::remainder( angle, 2 * PI ) );

                if( !isPartial )
                    centreSlack( candidate, aRegions, contacts, aPadGroup, allPadBox.center(), assignment );

                if( !geometryGate( candidate, aRegions, aPadGroup, aAllPads, contacts, level, down, epsilon, grow ) )
                    continue;

                for( const REGION* region : copperRegions )
                {
                    V3 point = candidate.rotation * vec( region->centroid ) + candidate.offset;

                    if( std::any_of( aAllPads.begin(), aAllPads.end(),
                                     [&]( const PAD& aPad )
                                     {
                                         return padContains( aPad, point.head<2>(), 0 );
                                     } ) )
                        candidate.copper += region->area;
                }

                ( isPartial ? partial : full ).push_back( candidate );
            }
        }
    }

    std::vector<ALIGN_CANDIDATE>& candidates = !full.empty() ? full : !partial.empty() ? partial : single;
    std::vector<ALIGN_CANDIDATE>  unique;

    for( const ALIGN_CANDIDATE& candidate : candidates )
    {
        if( !candidate.rotation.allFinite() || !candidate.offset.allFinite() )
            continue;

        bool duplicate = std::any_of( unique.begin(), unique.end(),
                                      [&]( const ALIGN_CANDIDATE& aPrevious )
                                      {
                                          return ( candidate.rotation - aPrevious.rotation ).norm() < 1e-6
                                                 && ( candidate.offset - aPrevious.offset ).norm() < 1e-3;
                                      } );

        if( !duplicate )
            unique.push_back( candidate );
    }

    std::stable_sort( unique.begin(), unique.end(),
                      []( const ALIGN_CANDIDATE& a, const ALIGN_CANDIDATE& b )
                      {
                          return a.copper > b.copper;
                      } );

    // Fixed groups avoid the non-transitive pairwise 10% comparator.
    for( auto first = unique.begin(); first != unique.end(); )
    {
        auto last = std::find_if( first, unique.end(),
                                  [&]( const ALIGN_CANDIDATE& aCandidate )
                                  {
                                      return aCandidate.copper < 0.9 * first->copper;
                                  } );
        std::stable_sort( first, last,
                          []( const ALIGN_CANDIDATE& a, const ALIGN_CANDIDATE& b )
                          {
                              return a.turn < b.turn;
                          } );
        first = last;
    }

    for( const ALIGN_CANDIDATE& candidate : unique )
    {
        // Eigen::eulerAngles() confines the first angle to [0, pi], which gives unfamiliar UI angles.
        const M3&  rotation = candidate.rotation;
        glm::dvec3 angles;
        angles.z = std::atan2( rotation( 1, 0 ), rotation( 0, 0 ) );
        angles.y = std::atan2( -rotation( 2, 0 ), std::hypot( rotation( 2, 1 ), rotation( 2, 2 ) ) );
        double sine = std::sin( angles.z );
        double cosine = std::cos( angles.z );
        angles.x = std::atan2( sine * rotation( 0, 2 ) - cosine * rotation( 1, 2 ),
                               cosine * rotation( 1, 1 ) - sine * rotation( 0, 1 ) );
        angles *= 180 / PI;

        for( int axis = 0; axis < 3; ++axis )
        {
            double snapped = std::round( angles[axis] / 90 ) * 90;

            if( std::abs( angles[axis] - snapped ) < 1e-6 )
                angles[axis] = snapped;

            angles[axis] += 360 * std::round( ( aCurrentRotation[axis] - angles[axis] ) / 360 );
        }

        result.push_back( { angles, vec( candidate.offset ), candidate.matched, candidate.total, candidate.kind } );
    }

    return result;
}
} // namespace MODEL_ALIGN
