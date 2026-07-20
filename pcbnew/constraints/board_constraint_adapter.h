/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef BOARD_CONSTRAINT_ADAPTER_H_
#define BOARD_CONSTRAINT_ADAPTER_H_

#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <vector>

#include <kiid.h>
#include <math/vector2d.h>

#include <constraints/pcb_constraint.h>

class BOARD;
class BOARD_ITEM;
class PCB_DIMENSION_BASE;
class PCB_DIM_ORTHOGONAL;
class PCB_SHAPE;

// planegcs is an Eigen-heavy implementation detail; keep its header out of this one (PIMPL).
namespace GCS
{
class System;
}


/**
 * The outcome of a constraint solve, in plain data so callers need not know planegcs.
 */
struct CONSTRAINT_DIAGNOSIS
{
    bool              solved = false;   ///< Solver reached Success or Converged.
    int               freeDof = -1;     ///< Remaining degrees of freedom (-1 if not diagnosed).
    std::vector<KIID> conflicting;      ///< Constraints the solver reports as over-constraining.
    std::vector<KIID> redundant;        ///< Constraints the solver reports as redundant.

    bool IsWellConstrained() const { return solved && freeDof == 0; }
    bool IsUnderConstrained() const { return freeDof > 0; }
    bool IsOverConstrained() const { return !conflicting.empty(); }
};


/// The constraint state of a shape, for the diagnostics overlay tint.
enum class CONSTRAINT_STATE
{
    UNCONSTRAINED,       ///< Not referenced by any constraint.
    UNDER_CONSTRAINED,   ///< In a cluster with remaining free degrees of freedom.
    WELL_CONSTRAINED,    ///< In a fully-determined cluster (zero free DOF).
    OVER_CONSTRAINED,    ///< In a cluster the solver reports as conflicting.
};


/**
 * Board-wide diagnostics for the constraint overlay and info bar.  Holds each constrained shape's
 * state, the conflicting and redundant constraints, and the total remaining free degrees of freedom.
 */
struct BOARD_CONSTRAINT_DIAGNOSTICS
{
    int                              totalFreeDof = 0;
    std::map<KIID, CONSTRAINT_STATE> shapeStates;
    std::vector<KIID>                conflicting;
    std::vector<KIID>                redundant;
    std::vector<KIID>                errored;   ///< Invalid constraints (member missing, deleted,
                                                ///< or of a kind incompatible with the type).
};


/**
 * Translates KiCad board geometry to and from the planegcs solver (issue #2329).
 *
 * Build() translates a cluster of #PCB_SHAPEs and the #PCB_CONSTRAINTs among them into a
 * planegcs system, scaled into a millimetre frame because raw IU (nanometres) squared in the
 * residuals is badly conditioned.  Solve() runs the solver (optionally pinning a dragged anchor
 * to a cursor position), Apply() writes the solution back to the shapes in IU, and Diagnose()
 * reports the constraint state.
 *
 * The adapter owns a pointer-stable std::deque<double> backing store for every value the solver
 * touches, both point coordinates and driving constants (lengths, pin targets).  planegcs holds
 * raw double* into this deque, so it must never reallocate, erase, or reorder while the
 * GCS::System is alive.  std::deque keeps element pointers stable across push_back, the only
 * growth used.
 */
class BOARD_CONSTRAINT_ADAPTER
{
public:
    BOARD_CONSTRAINT_ADAPTER();
    ~BOARD_CONSTRAINT_ADAPTER();

    BOARD_CONSTRAINT_ADAPTER( const BOARD_CONSTRAINT_ADAPTER& ) = delete;
    BOARD_CONSTRAINT_ADAPTER& operator=( const BOARD_CONSTRAINT_ADAPTER& ) = delete;

    /**
     * Translate a cluster into a planegcs system.
     *
     * @param aShapes the shapes referenced by the constraints (resolved by the caller).
     * @param aConstraints the constraints relating those shapes.
     * @param aFixedShapes shapes to treat as immovable references (their whole span is frozen).
     * @param aDimensions dimensions referenced by the constraints; each contributes its two feature
     *                  points, so a coincident constraint can drag a dimension along with a shape.
     * @return true if the system was built; unmappable constraints are skipped rather than fatal,
     *         and are listed by UnmappedConstraints() so the caller can flag them.
     */
    bool Build( const std::vector<PCB_SHAPE*>& aShapes, const std::vector<PCB_CONSTRAINT*>& aConstraints,
                const std::set<KIID>* aFixedShapes = nullptr,
                const std::vector<PCB_DIMENSION_BASE*>& aDimensions = {} );

    /// Constraints from the last Build() that could not be mapped onto a solver primitive (wrong
    /// member count/kind for their type, e.g. a parallel whose shape was changed to a circle), so
    /// they are silently not enforced.  Callers surface these as errored.
    const std::vector<KIID>& UnmappedConstraints() const { return m_unmapped; }

    /**
     * Solve the system, pinning a dragged anchor to a cursor position.
     *
     * The pin is a temporary (negatively tagged) constraint, so it yields to the real ones and an
     * over-constrained drag leaves the geometry where the rules allow.
     *
     * @param aDragged the anchor being dragged.
     * @param aCursor the cursor target, in IU.
     * @param aEdited other shapes edited alongside the dragged one excluded from stay-put pins
     * @param aCoDragged second anchor moved by the same handle with its own target pinned at the
     *                same weight since a polygon edge drag needs two independent points
     */
    /// @p aStabilize holds free segment lengths so an angle constraint rotates a segment instead of
    /// collapsing it. Off for live dragging.
    bool Solve( const CONSTRAINT_MEMBER& aDragged, const VECTOR2I& aCursor, bool aStabilize = false,
                const std::set<KIID>& aEdited = {},
                const std::optional<std::pair<CONSTRAINT_MEMBER, VECTOR2I>>& aCoDragged = std::nullopt );

    /// Solve with no drag pin. @p aStabilize holds free segment lengths (see the dragged overload).
    bool Solve( bool aStabilize = false );

    /// Solve after a resize. Holds the resized radius, pins its centre (yielding), and holds the
    /// other radii (yielding), so neighbours translate and the resized shape moves only if forced.
    bool SolveAfterResize( const KIID& aResizedShape );

    /**
     * Write the solved coordinates back into the shapes, de-normalized to IU.
     *
     * Only shapes whose geometry actually changed are written.  @p aBeforeWrite, if set, is
     * invoked with each such shape immediately before it is mutated, so a caller can stage it in
     * an undo commit (BOARD_COMMIT::Modify) before the change.
     *
     * @return the shapes that were changed.
     */
    std::vector<PCB_SHAPE*> Apply( const std::function<void( BOARD_ITEM* )>& aBeforeWrite = {} );

    /**
     * Propagate solved reference (non-driving) constraint values back into their m_value so a
     * reference dimension tracks the geometry it measures.  Call after Apply(); the value is
     * measured from the written shape geometry (not the raw solver param, which rounds separately),
     * so a settled solve re-measures the same value and stages nothing.  The value is always
     * written when it changed; @p aBeforeWrite, if set, stages that constraint (BOARD_COMMIT::Modify)
     * first so the write is undoable.  Driving constraints are never touched.
     */
    void ApplyReferenceValues( const std::function<void( BOARD_ITEM* )>& aBeforeWrite = {} );

    /// Report degrees of freedom and conflicting/redundant constraints.  The residual check reads
    /// the last solve's params; a contradictory cluster diverges under the stabilize holds, and the
    /// finite residuals it leaves behind are what flag the conflict.
    CONSTRAINT_DIAGNOSIS Diagnose();

private:
    enum class SHAPE_KIND
    {
        SEGMENT,
        BEZIER,      ///< A cubic bezier only its start and end endpoints are exposed as free points
        CIRCLE,
        ARC,
        ELLIPSE,
        ELLIPSE_ARC,
        POINT_PAIR,  ///< A dimension's two feature points (start + end); no line/curve geometry.
        RECT,        ///< An axis-aligned rectangle whose four corners alias the two stored corners
                     ///< params so rectness holds by construction with no extra DOF
        POLYGON      ///< A single hole-free outline with one free param pair per vertex since
                     ///< write-back rebuilds one outline only
    };

    /// Per-shape indices into m_params.  A segment stores start/end; a circle stores center
    /// (startX) + radius; an arc stores center (startX) + radius + endpoints + sweep angles, all
    /// tied together by an ArcRules solver constraint.  An ellipse stores center (startX) + first
    /// focus + minor radius (the GCS parameterization); an elliptical arc adds endpoints + sweep
    /// angles tied by ArcOfEllipseRules.
    struct SHAPE_VARS
    {
        PCB_SHAPE*          shape = nullptr;
        PCB_DIMENSION_BASE* dimension = nullptr;   ///< set instead of shape for a POINT_PAIR
        SHAPE_KIND kind = SHAPE_KIND::SEGMENT;
        int        startX = -1;        ///< start.x (segment) / center.x (circle, arc, ellipse).
        int        endX = -1;          ///< end.x (segment only).
        int        radius = -1;        ///< radius scalar (circle, arc) / minor radius (ellipse).
        int        focusX = -1;        ///< first focus.x (ellipse kinds only).
        int        arcStartX = -1;     ///< arc start-point.x.
        int        arcEndX = -1;       ///< arc end-point.x.
        int        startAngle = -1;    ///< arc start angle (radians).
        int        endAngle = -1;      ///< arc end angle (radians).
        int                 fixedLengthParam = -1; ///< param index of a driving fixed-length target, or -1.

        /// Outline-0 vertex count of a POLYGON vertex i x param is startX plus 2 times i
        int vertexCount = 0;

        /// Rect corner roles frozen at Build so VERTEX 0 to 3 as TL TR BR BL bind the same physical
        /// corners whatever the stored start end order
        bool startIsLeft = true;
        bool startIsTop = true;
    };

    /// Param indices of an anchor coordinates a rect corner aliases mixed start end params
    /// so y is not always x plus 1
    struct ANCHOR_PARAMS
    {
        int x = -1;
        int y = -1;

        bool IsValid() const { return x >= 0 && y >= 0; }
    };

    /// Append a normalized coordinate to the backing store, returning its stable index.
    int pushParam( double aValue );

    /// Note a non-driving valued constraint so its measured value can be read back after a solve.
    /// A driving constraint is ignored here -- its value is an input, never overwritten.
    void recordReferenceValue( PCB_CONSTRAINT* aConstraint );

    /// Length hold on the free segments in @p aShapes tagged @p aTag so only those shapes are
    /// protected while a merely dragged-along neighbour keeps its own stay-put pins instead
    void holdFreeSegmentLengths( int aTag, const std::set<KIID>& aShapes );

    /// Radius hold on the free arcs in @p aShapes tagged @p aTag so an angle change rotates an
    /// endpoint instead of collapsing the arc a real FIXED_RADIUS still wins
    void holdFreeArcRadii( int aTag, const std::set<KIID>& aShapes );

    /// Hold @p aVars's arc at its current radius (tagged @p aTag).
    void holdArcRadius( const SHAPE_VARS& aVars, int aTag );

    /// Pin the point at @p aPoint where it sits tagged @p aTag with @p aWeight rescaling the tier
    /// or the default weight overriding the stay-put tier when unset an invalid point is ignored
    void softPinPoint( const ANCHOR_PARAMS& aPoint, int aTag, std::optional<double> aWeight = std::nullopt );

    /// As above for the point whose x param is @p aPointX and whose y param follows it
    void softPinPoint( int aPointX, int aTag, std::optional<double> aWeight = std::nullopt );

    /// Soft-pin every vertex of each POLYGON in @p aShapes tagged @p aTag for edited shapes
    /// pinUneditedShapes excludes so unbound vertices keep their null-space minimal-movement pins
    void holdPolygonVertices( const std::set<KIID>& aShapes, int aTag );

    /// Hold the parts of the dragged shape meant to stay put tagged @p aTag a segment holds its far
    /// endpoint an arc holds centre radius and far endpoint or both endpoints on a centre drag
    void pinDraggedShapeRest( const CONSTRAINT_MEMBER& aDragged, int aTag,
                              const CONSTRAINT_MEMBER* aCoDragged = nullptr );

    /// Soft-pin every cluster shape not in @p aEdited at its current geometry tagged @p aTag for a
    /// minimal-movement solve @p aEdited must list every edited shape or a neighbour gets held back
    void pinUneditedShapes( const std::set<KIID>& aEdited, int aTag );

    /// Decide whether a solve reached a usable result a raw Success or Converged always qualifies
    /// while a Failed code is accepted only when every driving hard constraint is still satisfied
    bool solveSucceeded( int aSolveResult );

    /// Indices into m_params of the coordinates an anchor maps to invalid if the shape has no
    /// such anchor for example a circle has no endpoints
    ANCHOR_PARAMS anchorParams( const CONSTRAINT_MEMBER& aMember ) const;

    /// The orthogonal dimension a two-point length constraint drives or nullptr requires both
    /// members to be the same dimension START and END in either order
    PCB_DIM_ORTHOGONAL* orthogonalDimensionForMembers( const std::vector<CONSTRAINT_MEMBER>& aMembers ) const;

    /// IU <-> normalized (millimetre, cluster-centred) frame, per axis.
    double normalizeX( int aIU ) const { return ( aIU - m_originX ) * m_invScale; }
    double normalizeY( int aIU ) const { return ( aIU - m_originY ) * m_invScale; }
    double denormalizeX( double aNorm ) const { return aNorm * m_scale + m_originX; }
    double denormalizeY( double aNorm ) const { return aNorm * m_scale + m_originY; }

    std::unique_ptr<GCS::System> m_gcs;

    // Pointer-stable backing store for every solver double (see class comment).
    std::deque<double> m_params;

    std::map<KIID, SHAPE_VARS>    m_shapeVars;
    std::map<int, KIID>           m_tagToConstraint;
    std::set<int>                 m_nonDrivingTags;   ///< Measurement-only; excluded from conflict residuals.
    std::map<int, std::vector<KIID>> m_tagMembers;    ///< Member items per tag, for collapse attribution.
    std::vector<PCB_CONSTRAINT*>  m_referenceConstraints;   ///< Non-driving valued, read back after a solve.
    std::vector<KIID>             m_unmapped;   ///< Constraints Build() could not map (not enforced).

    /// Shapes a direction or angle constraint could collapse to a point only these get a
    /// stabilize length or radius hold
    std::set<KIID>                m_angleConstrainedShapes;

    double m_originX = 0.0;    ///< IU offset subtracted from x before scaling (cluster anchor).
    double m_originY = 0.0;    ///< IU offset subtracted from y before scaling.
    double m_scale = 1.0;      ///< IU per normalized unit.
    double m_invScale = 1.0;   ///< 1 / m_scale.

    int m_dragTargetX = -1;    ///< Stable backing slot for the drag pin's x target (-1 = unset).
    int m_dragTargetY = -1;    ///< Stable backing slot for the drag pin's y target.
    int m_coDragTargetX = -1;  ///< Backing slot for the co-dragged pin x target
    int m_coDragTargetY = -1;  ///< Backing slot for the co-dragged pin y target

    // Hold tags reserved by Build() just past the mapped constraints' tags, so a hold can never
    // collide with (and clear) a real constraint on a large cluster.
    int m_lengthHoldTag = -1;
    int m_resizeRadiusTag = -1;

    bool m_built = false;
};


/**
 * Gather the cluster of shapes transitively constrained with the dragged shape, solve with the
 * dragged anchor pinned to the cursor, and apply the result.
 *
 * Kept free-standing so the interactive drag hook stays thin and this logic is headless-testable.
 *
 * @param aBoard the board (or footprint-holder board) owning the constraints.
 * @param aDragged the anchor being dragged.
 * @param aCursor the cursor target, in IU.
 * @param aModified [out] the neighbor shapes whose geometry the solve changed (excludes the
 *                  dragged shape itself), for the caller to stage in a commit.
 * @param aBeforeModify if set, invoked with each item (neighbor shape or reference constraint whose
 *                  measured value changed) immediately before it is modified, so the caller can
 *                  BOARD_COMMIT::Modify it for a clean undo.  The dragged shape is excluded unless
 *                  @p aIncludeDragged, since the caller stages it itself.
 * @param aIncludeDragged if true, the dragged shape is also reported in @p aModified and passed to
 *                  @p aBeforeModify -- for callers (e.g. solve-on-create) that pin where the shape
 *                  already is and do not stage it themselves.
 * @param aEdited other shapes in this cluster edited alongside the dragged one left free instead
 *                  of stay-pinned back
 * @param aCoDragged second anchor moved by the same handle with its own target for a polygon edge
 *                  drag pinned at the same weight as the primary
 * @return the diagnosis; .solved is false if the cluster could not be built or did not converge.
 */
CONSTRAINT_DIAGNOSIS SolveCluster( BOARD* aBoard, const CONSTRAINT_MEMBER& aDragged, const VECTOR2I& aCursor,
                                   std::vector<PCB_SHAPE*>*                  aModified = nullptr,
                                   const std::function<void( BOARD_ITEM* )>& aBeforeModify = {},
                                   bool aIncludeDragged = false, bool aStabilize = false,
                                   const std::set<KIID>& aEdited = {},
                                   const std::optional<std::pair<CONSTRAINT_MEMBER, VECTOR2I>>& aCoDragged =
                                           std::nullopt );


/**
 * Solve a just-created constraint's cluster so the geometry snaps to satisfy it (SolidWorks-style),
 * pinning the constraint's first member at its current position and letting the solver move the
 * rest.  @p aConstraint must already be on the board.  A failed solve leaves geometry untouched.
 *
 * @param aModified [out] the shapes the solve moved, for the caller to stage in a commit.
 * @param aBeforeModify if set, invoked with each moved shape and each reference constraint whose
 *                  value changed, just before it changes, so the caller can stage it in the commit.
 */
CONSTRAINT_DIAGNOSIS ApplyConstraintImmediately(
        BOARD* aBoard, const PCB_CONSTRAINT* aConstraint,
        std::vector<PCB_SHAPE*>* aModified = nullptr,
        const std::function<void( BOARD_ITEM* )>& aBeforeModify = {} );


/// True if the board or any of its footprints carries at least one geometric constraint. Cheap,
/// short-circuiting guard so an unconstrained board pays nothing on every edit.
bool BoardHasConstraints( BOARD* aBoard );


/// True when the solver must treat @p aItem as immovable, either locked itself or living inside a
/// locked footprint.  BOARD_ITEM::IsLocked() alone misses the parent-footprint case because most
/// edit tools filter footprint children out before their lock checks run, while the solver
/// collects footprint graphics directly.
bool ConstraintItemIsLocked( const BOARD_ITEM* aItem );


/// Re-solve the clusters of shapes edited outside the solver, e.g. a whole-shape move. Pins one
/// anchor of each edited shape at its new position. A failed solve leaves geometry untouched.
void ReSolveShapeClusters( BOARD* aBoard, const std::vector<PCB_SHAPE*>& aShapes,
                           std::vector<PCB_SHAPE*>*                  aModified = nullptr,
                           const std::function<void( BOARD_ITEM* )>& aBeforeModify = {} );


/// Re-solve clusters whose new geometry is authoritative holding every edited shape fully fixed
/// so only constrained neighbors move a failed solve leaves that cluster untouched
void ReSolveShapeClustersHoldingEdited( BOARD* aBoard, const std::vector<PCB_SHAPE*>& aEditedShapes,
                                        std::vector<PCB_SHAPE*>*                  aModified = nullptr,
                                        const std::function<void( BOARD_ITEM* )>& aBeforeModify = {} );


/// Re-solve after a resize, e.g. a circle radius edit. Holds aShape fixed so its neighbors adjust.
void ReSolveAfterShapeResize( BOARD* aBoard, PCB_SHAPE* aShape, std::vector<PCB_SHAPE*>* aModified = nullptr,
                              const std::function<void( BOARD_ITEM* )>& aBeforeModify = {} );


/**
 * Diagnose every constraint cluster on the board (validate only -- geometry is not changed) and
 * return the per-shape state plus the conflicting/redundant constraints and total free DOF, for
 * the diagnostics overlay and info bar.
 */
BOARD_CONSTRAINT_DIAGNOSTICS DiagnoseBoardConstraints( BOARD* aBoard );


/**
 * One cluster's diagnosis, the unit DiagnoseBoardConstraints assembles the board-wide result from
 * and #BOARD_CONSTRAINT_DIAGNOSER caches state applies to every shape and dimension in the cluster
 */
struct CLUSTER_DIAGNOSIS
{
    CONSTRAINT_STATE  state = CONSTRAINT_STATE::WELL_CONSTRAINED;
    std::vector<KIID> shapeIds;          ///< Cluster shapes in the order the state is written
    std::vector<KIID> dimensionIds;      ///< Cluster dimensions
    int               freeDof = 0;       ///< Remaining free DOF folded into the board total
    std::vector<KIID> conflicting;
    std::vector<KIID> redundant;
    std::vector<KIID> erroredUnmapped;   ///< Constraints Build could not map and so not enforced
};


/**
 * An incremental #DiagnoseBoardConstraints for the interactive edit path
 *
 * Caches each cluster diagnosis keyed by constraint-id set with a hash of every solve input so a
 * model change re-solves only the affected clusters matching DiagnoseBoardConstraints exactly
 */
class BOARD_CONSTRAINT_DIAGNOSER
{
public:
    /// Diagnose every cluster reusing cached per-cluster results whose solve inputs are unchanged
    BOARD_CONSTRAINT_DIAGNOSTICS Diagnose( BOARD* aBoard );

    /// Drop the cache call when the board or view reloads and item-identity assumptions break
    void Clear();

    /// Clusters actually re-solved across this diagnoser lifetime for testing cache isolation
    std::size_t SolveCount() const { return m_solveCount; }

private:
    struct CACHE_ENTRY
    {
        std::size_t       hash = 0;
        CLUSTER_DIAGNOSIS result;
    };

    // Keyed by the cluster sorted constraint-id set which is stable across a geometry or value
    // edit and changes only when a constraint is added to or removed from the cluster
    std::map<std::vector<KIID>, CACHE_ENTRY> m_cache;
    std::size_t                              m_solveCount = 0;
};

#endif // BOARD_CONSTRAINT_ADAPTER_H_
