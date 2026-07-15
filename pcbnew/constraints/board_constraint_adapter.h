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
#include <set>
#include <vector>

#include <kiid.h>
#include <math/vector2d.h>

#include <constraints/pcb_constraint.h>

class BOARD;
class BOARD_ITEM;
class PCB_DIMENSION_BASE;
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
     */
    /// @p aStabilize holds free segment lengths so an angle constraint rotates a segment instead of
    /// collapsing it. Off for live dragging.
    bool Solve( const CONSTRAINT_MEMBER& aDragged, const VECTOR2I& aCursor, bool aStabilize = false );

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
        CIRCLE,
        ARC,
        ELLIPSE,
        ELLIPSE_ARC,
        POINT_PAIR   ///< A dimension's two feature points (start + end); no line/curve geometry.
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
    };

    /// Append a normalized coordinate to the backing store, returning its stable index.
    int pushParam( double aValue );

    /// Note a non-driving valued constraint so its measured value can be read back after a solve.
    /// A driving constraint is ignored here -- its value is an input, never overwritten.
    void recordReferenceValue( PCB_CONSTRAINT* aConstraint );

    /// Add a length hold on every free segment (tagged @p aTag), cleared by tag after the solve.
    void holdFreeSegmentLengths( int aTag );

    /// Add a radius hold on every free arc (tagged @p aTag) so an angle change rotates an endpoint
    /// about a stable radius instead of collapsing the arc.  A real FIXED_RADIUS still wins.
    void holdFreeArcRadii( int aTag );

    /// Hold @p aVars's arc at its current radius (tagged @p aTag).
    void holdArcRadius( const SHAPE_VARS& aVars, int aTag );

    /// Hold the parts of the dragged shape the edit means to keep (tagged @p aTag), so grabbing one
    /// handle moves only that handle instead of letting the solver drift the rest.  A segment holds
    /// its far endpoint; an arc endpoint drag holds the centre, radius and far endpoint (only the
    /// dragged endpoint sweeps); an arc centre drag holds both endpoints.  The holds are temporary,
    /// so a real constraint still wins.
    void pinDraggedShapeRest( const CONSTRAINT_MEMBER& aDragged, int aTag );

    /// Index into m_params of the x-coordinate an anchor maps to, or -1 if the shape has no such
    /// anchor (e.g. a circle has no endpoints).  The y-coordinate is always the next index.
    int anchorParamIndex( const CONSTRAINT_MEMBER& aMember ) const;

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

    double m_originX = 0.0;    ///< IU offset subtracted from x before scaling (cluster anchor).
    double m_originY = 0.0;    ///< IU offset subtracted from y before scaling.
    double m_scale = 1.0;      ///< IU per normalized unit.
    double m_invScale = 1.0;   ///< 1 / m_scale.

    int m_dragTargetX = -1;    ///< Stable backing slot for the drag pin's x target (-1 = unset).
    int m_dragTargetY = -1;    ///< Stable backing slot for the drag pin's y target.

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
 * @return the diagnosis; .solved is false if the cluster could not be built or did not converge.
 */
CONSTRAINT_DIAGNOSIS SolveCluster( BOARD* aBoard, const CONSTRAINT_MEMBER& aDragged, const VECTOR2I& aCursor,
                                   std::vector<PCB_SHAPE*>*                  aModified = nullptr,
                                   const std::function<void( BOARD_ITEM* )>& aBeforeModify = {},
                                   bool aIncludeDragged = false, bool aStabilize = false );


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
 * One-line summary of the board's overall constraint state for the info bar, e.g. "Fully
 * constrained" or "Under-constrained (3 degrees of freedom)".  @p aOverConstrained is set true for
 * an error/conflict state so the caller can pick a warning icon.
 */
wxString ConstraintStateSummary( const BOARD_CONSTRAINT_DIAGNOSTICS& aDiag, bool* aOverConstrained = nullptr );

#endif // BOARD_CONSTRAINT_ADAPTER_H_
