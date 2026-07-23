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

#ifndef SNAP_RESOLVER_H
#define SNAP_RESOLVER_H

#include <math/vector2d.h>
#include <math/box2.h>
#include <geometry/intersection.h>

#include <array>
#include <cstdint>
#include <chrono>
#include <functional>
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>


/// Screen-space radius (px) inside which a candidate is snappable.  Editors scale this to world
/// units; it lives here so the resolver and its callers agree on what "normalized residual" means.
constexpr double SNAP_SCREEN_RADIUS = 25.0;

/// Default ranking stickiness, as a fraction of SNAP_SCREEN_RADIUS.
constexpr double SNAP_DEFAULT_RANKING_HYSTERESIS = 5.0 / SNAP_SCREEN_RADIUS;


enum class SNAP_EDITOR_PROFILE
{
    POINT_EDIT,
    CONSTRAINED_MOVE,
    DRAW,
    RIGID_PLACEMENT,
    PICKER,
    VIEWER,
    MEASUREMENT
};


enum class SNAP_PRIORITY_TIER
{
    AUTHORED_INTRINSIC,
    ANGLE,
    OBJECT,
    GRID,
    CURSOR
};


enum class SNAP_CANDIDATE_SUBTYPE
{
    INTRINSIC_ANCHOR,
    CONSTRUCTED_POINT,
    INTERSECTION,
    TANGENT_NORMAL,
    BBOX_LAYOUT,
    FINITE_MANIFOLD,
    ACTIVE_EXTENSION,
    ANGLE_BRANCH,
    GRID_AXIS,
    CURSOR
};


enum class SNAP_RELATION
{
    COINCIDENCE,
    X_COORDINATE,
    Y_COORDINATE,
    POINT_ON_LINE,
    POINT_ON_RAY,
    POINT_ON_SEGMENT,
    POINT_ON_CIRCLE,
    POINT_ON_ARC,
    ANGLE,
    TANGENT,
    NORMAL,
    BBOX_ALIGNMENT,
    BBOX_EQUAL_GAP,
    GRID_X,
    GRID_Y
};


enum class SNAP_RESULT_STATUS
{
    SUCCESS,
    BASE_CONFLICT,
    INCOMPATIBLE,
    NONCONVERGENT,
    INVALID_GEOMETRY,
    BUDGET_EXHAUSTED
};


std::ostream& operator<<( std::ostream& aStream, SNAP_RESULT_STATUS aStatus );


enum class SNAP_ID_KIND
{
    ITEM_GEOMETRY,
    INTRINSIC_ANCHOR,
    CONSTRUCTED_ANCHOR,
    SELF_SEGMENT,
    SELF_ENDPOINT,
    SELF_POINT,
    CONSTRUCTION,
    INTERSECTION,
    TANGENT,
    NORMAL,
    ANGLE_BRANCH,
    BOUNDS_X,
    BOUNDS_Y,
    ANCHOR_POINT_X,
    ANCHOR_POINT_Y,
    EQUAL_GAP_X,
    EQUAL_GAP_Y,
    COPY_GAP_X,
    COPY_GAP_Y,
    GRID_X,
    GRID_Y
};


// Preserve source byte order: lexicographic ordering participates in stable ID construction.
using SNAP_TARGET_ID = std::array<uint8_t, 16>;


struct SNAP_STABLE_ID
{
    SNAP_ID_KIND   kind = SNAP_ID_KIND::ITEM_GEOMETRY;
    SNAP_TARGET_ID target{};
    int            featureIndex = 0;
    int            solutionBranch = 0;

    bool operator==( const SNAP_STABLE_ID& aOther ) const = default;
    bool operator<( const SNAP_STABLE_ID& aOther ) const;
};


SNAP_STABLE_ID MakeDerivedSnapId( SNAP_ID_KIND aKind, const SNAP_STABLE_ID& aSource, int aFeatureIndex = 0,
                                  int aSolutionBranch = 0 );
SNAP_STABLE_ID MakeIntersectionSnapId( const SNAP_STABLE_ID& aFirst, const SNAP_STABLE_ID& aSecond,
                                       int aSolutionBranch );
SNAP_STABLE_ID MakePointSnapId( SNAP_ID_KIND aKind, const VECTOR2I& aPoint, int aFeatureIndex = 0 );
SNAP_STABLE_ID MakeCompositeSnapId( SNAP_ID_KIND aKind, const std::vector<SNAP_TARGET_ID>& aTargets,
                                    int aFeatureIndex = 0 );


enum class SNAP_GUIDE_STYLE
{
    SNAP_LINE,
    DIMENSION_BRACKET
};


struct SNAP_GUIDE
{
    VECTOR2I         start;
    VECTOR2I         end;
    SNAP_GUIDE_STYLE style = SNAP_GUIDE_STYLE::SNAP_LINE;
};


enum class SNAP_REFERENCE_KIND
{
    NONE,
    BOUNDS_FEATURE,
    ANCHOR_POINT
};


struct SNAP_REFERENCE_PREFERENCE
{
    SNAP_REFERENCE_KIND kind = SNAP_REFERENCE_KIND::NONE;
    int                 horizontalFeature = -1;
    int                 verticalFeature = -1;
};


struct SNAP_SOURCE_CONTEXT
{
    SNAP_EDITOR_PROFILE           profile = SNAP_EDITOR_PROFILE::PICKER;
    VECTOR2I                      sourcePoint;
    std::optional<VECTOR2I>       stationarySourceLeg;
    std::vector<SNAP_STABLE_ID>   movingFeatures;
    std::vector<SNAP_STABLE_ID>   stationarySelfFeatures;
    std::optional<SNAP_STABLE_ID> movingItem;
    std::optional<BOX2I>          movingBounds;
    // Reference point represented by the live bounds; inference projects them to sourcePoint.
    std::optional<VECTOR2I>   movingReferencePoint;
    SNAP_REFERENCE_PREFERENCE referencePreference;
};


struct SNAP_CANDIDATE
{
    SNAP_STABLE_ID                    id;
    SNAP_PRIORITY_TIER                priority = SNAP_PRIORITY_TIER::CURSOR;
    SNAP_CANDIDATE_SUBTYPE            subtype = SNAP_CANDIDATE_SUBTYPE::CURSOR;
    SNAP_RELATION                     relation = SNAP_RELATION::COINCIDENCE;
    VECTOR2D                          origin;
    VECTOR2D                          direction;
    double                            normalizedScreenResidual = 0.0;
    int                               referenceAffinity = 0;
    int                               consumedDof = 0;
    bool                              finite = false;
    double                            domainStart = 0.0;
    double                            domainEnd = 0.0;
    std::vector<SNAP_GUIDE>           guides;
    std::optional<INTERSECTABLE_GEOM> manifold;

    static SNAP_CANDIDATE Point( SNAP_STABLE_ID aId, SNAP_PRIORITY_TIER aPriority, SNAP_CANDIDATE_SUBTYPE aSubtype,
                                 const VECTOR2I& aPoint, double aResidual );
    static SNAP_CANDIDATE Line( SNAP_STABLE_ID aId, SNAP_PRIORITY_TIER aPriority, SNAP_CANDIDATE_SUBTYPE aSubtype,
                                const VECTOR2I& aOrigin, const VECTOR2D& aDirection, double aResidual );
    static SNAP_CANDIDATE AxisX( SNAP_STABLE_ID aId, SNAP_PRIORITY_TIER aPriority, SNAP_CANDIDATE_SUBTYPE aSubtype,
                                 int aCoordinate, double aResidual );
    static SNAP_CANDIDATE AxisY( SNAP_STABLE_ID aId, SNAP_PRIORITY_TIER aPriority, SNAP_CANDIDATE_SUBTYPE aSubtype,
                                 int aCoordinate, double aResidual );
};


struct SNAP_RESULT
{
    SNAP_RESULT_STATUS          status = SNAP_RESULT_STATUS::SUCCESS;
    std::vector<SNAP_STABLE_ID> accepted;
    VECTOR2I                    position;
    int                         remainingDof = 2;
    std::vector<double>         quantizedResiduals;
    std::vector<SNAP_GUIDE>     guides;

    bool Accepted( const SNAP_STABLE_ID& aId ) const;
};


class SNAP_RESOLVER
{
public:
    using CLOCK = std::chrono::steady_clock;
    using CLOCK_CALLBACK = std::function<CLOCK::time_point()>;
    // The candidate-vector reference is valid only during the synchronous callback and must not be retained.
    using FEASIBILITY_CALLBACK =
            std::function<SNAP_RESULT( const SNAP_SOURCE_CONTEXT&, const std::vector<SNAP_CANDIDATE>& )>;
    using TRACE_CALLBACK = std::function<void( const std::string& )>;

    void AddCandidate( SNAP_CANDIDATE aCandidate );
    void Clear();
    void SetRetainedCandidate( std::optional<SNAP_STABLE_ID> aId );

    /**
     * Bias the ranking toward candidates accepted on a previous resolve so the chosen snap stays
     * put until a competitor is clearly closer.  Unlike the retained candidate these are not forced
     * into the result, they only earn the ranking hysteresis that prevents frame-to-frame jitter
     * between near-equal solutions.
     */
    void SetStickyCandidates( std::vector<SNAP_STABLE_ID> aIds );

    /**
     * Set how strongly a candidate with hysteresis is favoured, as a fraction of the snap radius.
     */
    void SetRankingHysteresis( double aNormalizedResidual );

    void SetFeasibilityCallback( FEASIBILITY_CALLBACK aCallback );
    void SetTraceCallback( TRACE_CALLBACK aCallback );
    void SetClock( CLOCK_CALLBACK aClock );
    void SetDeadline( CLOCK::duration aDeadline );

    SNAP_RESULT Resolve( const SNAP_SOURCE_CONTEXT& aContext ) const;

private:
    /// True when a candidate should earn the ranking hysteresis (retained or sticky).
    bool hasHysteresis( const SNAP_STABLE_ID& aId ) const;

    std::vector<SNAP_CANDIDATE>   m_candidates;
    std::optional<SNAP_STABLE_ID> m_retainedCandidate;
    std::vector<SNAP_STABLE_ID>   m_stickyCandidates;
    double                        m_rankingHysteresis = SNAP_DEFAULT_RANKING_HYSTERESIS;
    FEASIBILITY_CALLBACK          m_feasibilityCallback;
    TRACE_CALLBACK                m_traceCallback;
    CLOCK_CALLBACK                m_clock = []
    {
        return CLOCK::now();
    };
    CLOCK::duration m_deadline = std::chrono::milliseconds( 8 );
};

#endif
