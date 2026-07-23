/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef SNAP_INFERENCE_H
#define SNAP_INFERENCE_H

#include <snap/snap_resolver.h>

#include <geometry/intersection.h>

#include <optional>
#include <vector>


struct SNAP_OBJECT_PATH
{
    SNAP_STABLE_ID     id;
    INTERSECTABLE_GEOM geometry;
    bool               intrinsic = false;
    bool               activeExtension = false;
};


struct SNAP_OBJECT_BOUNDS
{
    SNAP_STABLE_ID                id;
    BOX2I                         bounds;
    std::optional<SNAP_TARGET_ID> parent;
};


/// A named point a parent object offers for alignment, such as a pad center or a pin end.
struct SNAP_ALIGNMENT_POINT
{
    SNAP_STABLE_ID                id;
    VECTOR2I                      position;
    std::optional<SNAP_TARGET_ID> parent;
};


class SNAP_INFERENCE_PROVIDER
{
public:
    void AddPath( SNAP_OBJECT_PATH aPath );
    void AddBounds( SNAP_OBJECT_BOUNDS aBounds );
    void AddAlignmentPoint( SNAP_ALIGNMENT_POINT aPoint );
    void Clear();

    void ActivateExtension( const SNAP_STABLE_ID& aId );
    void ClearExtensions();

    std::vector<SNAP_CANDIDATE> CollectObjectGeometry( const SNAP_SOURCE_CONTEXT& aContext, int aRadius ) const;
    std::vector<SNAP_CANDIDATE> CollectTangentNormal( const SNAP_SOURCE_CONTEXT& aContext, int aRadius,
                                                      bool aTangentEnabled, bool aNormalEnabled ) const;
    std::vector<SNAP_CANDIDATE> CollectAlignment( const SNAP_SOURCE_CONTEXT& aContext, int aRadius ) const;
    std::vector<SNAP_CANDIDATE> CollectEqualSpacing( const SNAP_SOURCE_CONTEXT& aContext, int aRadius ) const;

private:
    bool eligible( const SNAP_SOURCE_CONTEXT& aContext, const SNAP_STABLE_ID& aId ) const;
    bool eligible( const SNAP_SOURCE_CONTEXT& aContext, const SNAP_OBJECT_BOUNDS& aBounds ) const;
    bool eligible( const SNAP_SOURCE_CONTEXT& aContext, const SNAP_ALIGNMENT_POINT& aPoint ) const;

    std::vector<SNAP_OBJECT_PATH>     m_paths;
    std::vector<SNAP_OBJECT_BOUNDS>   m_bounds;
    std::vector<SNAP_ALIGNMENT_POINT> m_alignmentPoints;
    std::vector<SNAP_STABLE_ID>       m_activeExtensions;
};

#endif
