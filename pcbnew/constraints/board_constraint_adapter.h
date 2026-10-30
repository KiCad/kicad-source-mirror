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
#include <unordered_set>
#include <vector>

#include <constraints/constraint_system_2d.h>
#include <kiid.h>
#include <math/vector2d.h>
#include <snap/snap_resolver.h>

#include <constraints/pcb_constraint.h>

class BOARD;
class BOARD_ITEM;
class FOOTPRINT;
class PAD;
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


/// How a block edit solve turned out. Lets the caller tell an edit the constraints
/// forbid from a cluster the solver has nothing to say about.
enum class CONSTRAINT_BLOCK_RESULT
{
    SOLVED,      ///< The constraints hold and every pinned point is exactly where it was put.
    REFUSED,     ///< No arrangement fits the pinned points, and the geometry is untouched.
    NOT_MODELED, ///< Nothing the caller pinned belongs to this cluster.
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
 * Build() translates a cluster of #PCB_SHAPE objects and the #PCB_CONSTRAINT objects among them into a
 * planegcs system, scaled into a millimetre frame because raw IU (nanometres) squared in the
 * residuals is badly conditioned.  Solve() runs the solver (optionally pinning a dragged anchor
 * to a cursor position), Apply() writes the solution back to the shapes in IU, and Diagnose()
 * reports the constraint state.
 *
 * #CONSTRAINT_SYSTEM_2D owns the normalized coordinate frame, planegcs system, and pointer-stable
 * parameter storage.  This adapter maps board geometry and authored constraints into that shared
 * solver-neutral lifetime.
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
     * @param aPads pads referenced by the constraints; each contributes its centre point.
     * @return true if the system was built; unmappable constraints are skipped rather than fatal,
     *         and are listed by UnmappedConstraints() so the caller can flag them.
     *
     * Outside the footprint editor a footprint is a rigid body.  Its children in the cluster share
     * one free translation and write-back moves the footprint itself.  A pad never moves on its
     * own, so in the footprint editor it is grounded.
     */
    bool Build( const std::vector<PCB_SHAPE*>& aShapes, const std::vector<PCB_CONSTRAINT*>& aConstraints,
                const std::set<KIID>* aFixedShapes = nullptr,
                const std::vector<PCB_DIMENSION_BASE*>& aDimensions = {},
                const std::vector<PAD*>& aPads = {} );

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
     * @param aStabilize holds free segment lengths so an angle constraint rotates a segment instead of
     *                   collapsing it. Off for live dragging.
     * @param aEdited other shapes edited alongside the dragged one excluded from stay-put pins
     * @param aCoDragged second anchor moved by the same handle with its own target pinned at the
     *                   same weight since a polygon edge drag needs two independent points
     * @param aHoldDraggedRigid holds the dragged shape's own geometry, so it travels whole instead of
     *                          stretching to meet the relation.
     */
    bool Solve( const CONSTRAINT_MEMBER& aDragged, const VECTOR2I& aCursor, bool aStabilize = false,
                const std::set<KIID>&                                        aEdited = {},
                const std::optional<std::pair<CONSTRAINT_MEMBER, VECTOR2I>>& aCoDragged = std::nullopt,
                bool                                                         aHoldDraggedRigid = false );
    bool SolveSnapRelations( const CONSTRAINT_MEMBER& aDragged,
                             const std::vector<SNAP_CANDIDATE>& aCandidates,
                             const VECTOR2I& aCursor );
    bool SolveRigidTranslation( const std::set<KIID>& aEditedShapes,
                                const VECTOR2I& aTranslation );

    /**
     * Solve a block edit whose geometry the caller has already written.
     *
     * Every coordinate the caller names is held exactly where the caller put it. Every other
     * coordinate in the cluster is held with a weak pull back to where it was, so an anchor
     * of an edited shape that the user did not pick, and an untouched neighbour, each move
     * only as far as the constraints make them. The result is handed back for the caller to
     * apply.
     *
     * Some named coordinates cannot move at all, such as a locked shape's or a dimension
     * point no constraint binds. Those are read as constants, since the caller has already
     * put the item there. If a constraint then breaks, the solve is still refused.
     *
     * A point a FIXED_POSITION constraint pins is the exception. It is never read from the
     * caller: a pinned anchor is left uncommanded so the solve puts it back, and a shape
     * that would carry one along is refused outright.
     *
     * @param aPinnedAnchors the anchors the user placed, each with its target in IU.
     * @param aTranslatedShapes shapes that travelled whole, with all of their coordinates held.
     * @param aTranslation how far those shapes travelled from the current parameters.
     * @return SOLVED when the constraints hold with every pinned point exact, REFUSED when
     *         they cannot, and NOT_MODELED when nothing pinned belongs to this cluster.
     */
    CONSTRAINT_BLOCK_RESULT
    SolveBlockEdit( const std::vector<std::pair<CONSTRAINT_MEMBER, VECTOR2I>>& aPinnedAnchors,
                    const std::set<KIID>& aTranslatedShapes, const VECTOR2I& aTranslation );

    /// True when every shape in the cluster is in @p aEditedShapes and free to travel with it.
    bool TravelsWhole( const std::set<KIID>& aEditedShapes ) const;
    bool SolveRigidSnapRelations( const std::set<KIID>& aEditedShapes,
                                  const VECTOR2I& aReference,
                                  const std::vector<SNAP_CANDIDATE>& aCandidates,
                                  const VECTOR2I& aCursor, VECTOR2I& aResolvedCursor );

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
     * an undo commit (BOARD_COMMIT::Modify) before the change.  A rigid footprint is moved whole
     * and passed to @p aBeforeWrite, but its children are not reported as changed shapes.
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

    using SNAPSHOT = CONSTRAINT_SYSTEM_2D::SNAPSHOT;

    SNAPSHOT Snapshot() const { return m_system.Snapshot(); }
    bool     Restore( const SNAPSHOT& aSnapshot ) { return m_system.RestorePrefix( aSnapshot ); }
    std::optional<VECTOR2I> AnchorPosition( const CONSTRAINT_MEMBER& aMember ) const;
    bool QuantizedRelationsSatisfied();
    bool CurrentRelationsSatisfied() const { return hardRelationsSatisfied(); }

    /**
     * Remove the constraints the current geometry already fails to satisfy.
     *
     * A constraint that does not hold at the baseline records no live intent, and leaving it
     * in the system makes the whole cluster count as conflicted, which switches off the
     * constraints beside it that do hold. Returns how many were removed.
     */
    int DropStaleRelations();

private:
    enum class SHAPE_KIND
    {
        SEGMENT,
        BEZIER, ///< A cubic bezier only its start and end endpoints are exposed as free points
        CIRCLE,
        ARC,
        ELLIPSE,
        ELLIPSE_ARC,
        POINT_PAIR, ///< A dimension's two feature points (start + end); no line/curve geometry.
        RECTANGLE,  ///< An axis-aligned rectangle whose four corners alias the two stored corners
                    ///< params so rectness holds by construction with no extra DOF
        POLYGON,    ///< A single hole-free outline with one free param pair per vertex since
                    ///< write-back rebuilds one outline only
        PAD_CENTER  ///< A pad's centre point, which only ever moves with its footprint
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
        PAD*                pad = nullptr;         ///< set instead of shape for a PAD_CENTER
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

    struct RIGID_RADIUS_HOLD
    {
        int centerX;
        int radius;
    };

    struct RIGID_STATE
    {
        std::set<int>                  points;
        std::vector<RIGID_RADIUS_HOLD> radii;
    };

    /// A footprint moving as one body with its translation from buildPosition held in the params
    /// at translationX
    struct RIGID_FOOTPRINT
    {
        FOOTPRINT* footprint = nullptr;
        VECTOR2I   buildPosition;
        int        translationX = -1;
    };

    static BOARD_ITEM* itemOf( const SHAPE_VARS& aVars );

    /// Tie every mapped child of each footprint in @p aFootprints to one shared translation, or
    /// freeze them all when any is locked or in @p aFixedShapes, adding frozen params to @p aFixed
    void coupleRigidFootprints( const std::set<FOOTPRINT*>& aFootprints, const std::set<KIID>* aFixedShapes,
                                std::set<int>& aFixed );

    /// Append a normalized coordinate to the backing store, returning its stable index.
    int pushParam( double aValue );
    int temporaryParam( double aValue );
    void beginTemporaryParameters();
    bool addSnapRelations( const ANCHOR_PARAMS& aAnchor,
                           const std::vector<SNAP_CANDIDATE>& aCandidates,
                           const VECTOR2I& aOffset );
    RIGID_STATE collectRigidState( const std::set<KIID>& aEditedShapes ) const;
    void        holdRigidRadii( const std::vector<RIGID_RADIUS_HOLD>& aRadii, int aTag );

    /// Note a non-driving valued constraint so its measured value can be read back after a solve.
    /// A driving constraint is ignored here -- its value is an input, never overwritten.
    void recordReferenceValue( PCB_CONSTRAINT* aConstraint );

    /// Length hold on the free segments in @p aShapes tagged @p aTag so only those shapes are
    /// protected while a merely dragged-along neighbour keeps its own stay-put pins instead
    void holdFreeSegmentLengths( int aTag, const std::set<KIID>& aShapes );

    /// Radius hold on the free arcs in @p aShapes tagged @p aTag so an angle change rotates an
    /// endpoint instead of collapsing the arc a real FIXED_RADIUS still wins
    void holdFreeArcRadii( int aTag, const std::set<KIID>& aShapes );

    /// Hold the shapes in @p aShapes rigid, tagged @p aTag, so a shape the solve moves translates
    /// instead of stretching
    void holdShapesRigid( int aTag, const std::set<KIID>& aShapes );

    /// Hold @p aVars's arc at its current radius (tagged @p aTag).
    void holdArcRadius( const SHAPE_VARS& aVars, int aTag );

    /// Pin the point at @p aPoint where it sits tagged @p aTag with @p aWeight rescaling the tier
    /// or the default weight overriding the stay-put tier when unset an invalid point is ignored
    void softPinPoint( const ANCHOR_PARAMS& aPoint, int aTag, std::optional<double> aWeight = std::nullopt );

    /// As above for the point whose x param is @p aPointX and whose y param follows it
    void softPinPoint( int aPointX, int aTag, std::optional<double> aWeight = std::nullopt );

    /// Hold the one coordinate at @p aParam where it sits, tagged @p aTag, with @p aWeight
    /// scaling the tier. Used where an anchor's two coordinates are set one at a time, as a
    /// rectangle corner's shared pair is. A negative param is ignored.
    void softPinCoordinate( int aParam, int aTag, std::optional<double> aWeight = std::nullopt );

    /// Weakly hold every cluster coordinate outside @p aCommanded where it sits, tagged
    /// @p aTag, so nothing the caller left alone moves further than the constraints force.
    /// A shape with a held coordinate keeps its radii at full weight.
    void pinUncommandedCoordinates( const std::set<int>& aCommanded, int aTag );

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
    bool hardRelationsSatisfied() const;

    /// Driving constraint tags whose residual is already out of tolerance, ignoring dropped ones.
    std::vector<int> staleRelationTags() const;

    /// Indices into m_params of the coordinates an anchor maps to invalid if the shape has no
    /// such anchor for example a circle has no endpoints
    ANCHOR_PARAMS anchorParams( const CONSTRAINT_MEMBER& aMember ) const;

    /// The orthogonal dimension a two-point length constraint drives or nullptr requires both
    /// members to be the same dimension START and END in either order
    PCB_DIM_ORTHOGONAL* orthogonalDimensionForMembers( const std::vector<CONSTRAINT_MEMBER>& aMembers ) const;

    /// IU <-> normalized (millimetre, cluster-centred) frame, per axis.
    double normalizeX( int aIU ) const { return m_system.NormalizeX( aIU ); }
    double normalizeY( int aIU ) const { return m_system.NormalizeY( aIU ); }
    double denormalizeX( double aNorm ) const { return m_system.DenormalizeX( aNorm ); }
    double denormalizeY( double aNorm ) const { return m_system.DenormalizeY( aNorm ); }

    CONSTRAINT_SYSTEM_2D m_system;
    GCS::System*         m_gcs;

    // Compatibility alias while board geometry mapping remains in this adapter.
    std::deque<double>& m_params;

    std::map<KIID, SHAPE_VARS>       m_shapeVars;

    /// Parameter indices the solver may not move, so a rigid translation may not either.
    std::set<int>                    m_fixedParams;

    /// The subset of those grounded by a FIXED_POSITION constraint rather than by a lock
    /// or a driving constant, which a block edit may not command to a new place.
    std::set<int>                    m_groundedPositionParams;
    std::map<int, KIID>              m_tagToConstraint;
    std::set<int>                    m_droppedTags;          ///< Stale relations taken out of the system.
    std::set<int>                    m_nonDrivingTags;       ///< Measurement-only; excluded from conflict residuals.
    std::map<int, std::vector<KIID>> m_tagMembers;           ///< Member items per tag, for collapse attribution.
    std::vector<PCB_CONSTRAINT*>     m_referenceConstraints; ///< Non-driving valued, read back after a solve.
    std::vector<KIID>                m_unmapped;             ///< Constraints Build() could not map (not enforced).
    std::vector<RIGID_FOOTPRINT>     m_rigidFootprints;
    std::set<KIID>                   m_rigidMembers;         ///< Mapped items that move only with their footprint

    /// Shapes a direction or angle constraint could collapse to a point only these get a
    /// stabilize length or radius hold
    std::set<KIID> m_angleConstrainedShapes;

    double m_scale = 1.0;    ///< IU per normalized unit.
    double m_invScale = 1.0; ///< 1 / m_scale.

    int m_dragTargetX = -1;   ///< Stable backing slot for the drag pin's x target (-1 = unset).
    int m_dragTargetY = -1;   ///< Stable backing slot for the drag pin's y target.
    int m_coDragTargetX = -1; ///< Backing slot for the co-dragged pin x target
    int m_coDragTargetY = -1; ///< Backing slot for the co-dragged pin y target
    std::vector<int> m_temporaryParams;
    size_t           m_nextTemporaryParam = 0;

    // Hold tags reserved by Build() just past the mapped constraints' tags, so a hold can never
    // collide with (and clear) a real constraint on a large cluster.
    int m_lengthHoldTag = -1;
    int m_resizeRadiusTag = -1;

    bool m_built = false;
};


/**
 * One editing session over a constraint cluster.
 *
 * The session owns the cluster's adapter plus the baseline it rewinds to.  Speculative solves
 * always run from the baseline and restore it, so the board changes only when the caller applies
 * a result.  Subclasses differ in what they drag and how they judge a candidate; everything about
 * owning and rewinding the cluster is here.
 */
class BOARD_CONSTRAINT_SESSION
{
public:
    bool HasBaseConflict() const { return m_baseConflict; }

protected:
    /**
     * Assemble the adapter for one cluster and record its baseline.
     *
     * Sets the base-conflict flag when the cluster's relations were already unsatisfied, which
     * every solve then refuses rather than trying to repair an edit the user did not make.
     */
    bool buildCluster( BOARD* aBoard, const std::unordered_set<KIID>& aClusterShapes,
                       const std::vector<PCB_CONSTRAINT*>& aConstraints );

    /// Drop the built cluster, leaving the session unusable until the next successful build.
    void reset();

    bool usable() const { return m_adapter && !m_baseConflict; }

    /// Rewind to the state the next speculative solve starts from.
    bool rewind() { return m_adapter && m_adapter->Restore( m_baseline ); }

    /// Seed a snap result at the cursor, rejecting a cluster that was already broken.
    SNAP_RESULT startResult( const SNAP_SOURCE_CONTEXT& aContext ) const;

    /**
     * Record one candidate the session honoured.
     *
     * Only the residual and the freedom consumed belong here.  #SNAP_RESOLVER appends the id and
     * the guides to whatever a feasibility callback returns, so reporting them again would draw
     * every guide twice and enter the id in the sticky set twice.
     */
    static void accept( SNAP_RESULT& aResult, const SNAP_CANDIDATE& aCandidate, double aResidual );

    BOARD*                                    m_board = nullptr;
    std::unique_ptr<BOARD_CONSTRAINT_ADAPTER> m_adapter;
    BOARD_CONSTRAINT_ADAPTER::SNAPSHOT        m_baseline;
    bool                                      m_baseConflict = false;
};


class BOARD_CONSTRAINT_DRAG_SESSION : public BOARD_CONSTRAINT_SESSION
{
public:
    bool Build( BOARD* aBoard, const CONSTRAINT_MEMBER& aDragged );
    bool Matches( const CONSTRAINT_MEMBER& aDragged ) const;
    bool IsExactFeasible( const VECTOR2I& aTarget );
    SNAP_RESULT ResolveCandidates( const SNAP_SOURCE_CONTEXT& aContext,
                                   const std::vector<SNAP_CANDIDATE>& aCandidates );

    CONSTRAINT_DIAGNOSIS
    Solve( const VECTOR2I& aCursor, std::vector<PCB_SHAPE*>* aModified,
           const std::function<void( BOARD_ITEM* )>& aBeforeModify, bool aIncludeDragged,
           bool aStabilize, const std::set<KIID>& aEdited = {},
           const std::optional<std::pair<CONSTRAINT_MEMBER, VECTOR2I>>& aCoDragged = std::nullopt );

private:
    CONSTRAINT_MEMBER m_dragged;
    PCB_SHAPE*        m_draggedShape = nullptr;
};


class BOARD_CONSTRAINT_MOVE_SESSION : public BOARD_CONSTRAINT_SESSION
{
public:
    bool Build( BOARD* aBoard, const std::vector<PCB_SHAPE*>& aEditedShapes,
                const VECTOR2I& aReference );
    SNAP_RESULT ResolveCandidates( const SNAP_SOURCE_CONTEXT& aContext,
                                   const std::vector<SNAP_CANDIDATE>& aCandidates );
    bool Solve( const VECTOR2I& aTarget, std::vector<PCB_SHAPE*>* aModified,
                const std::function<void( BOARD_ITEM* )>& aBeforeModify );

private:
    bool feasibleAt( const VECTOR2I& aTarget );

    VECTOR2I       m_reference;
    std::set<KIID> m_edited;
};


/**
 * One block edit session, holding several anchors and one travel offset at once.
 *
 * A block drag reshapes some shapes by their picked anchors while carrying others whole, so
 * it needs more than the drag session's single anchor and more than the move session's one
 * rigid step. The session is built once, over every cluster the block touches, and every
 * frame solves from the same starting geometry, so the result depends only on where the
 * drag ends and not on the path the cursor took.
 */
class BOARD_CONSTRAINT_BLOCK_SESSION : public BOARD_CONSTRAINT_SESSION
{
public:
    /// Build one adapter over the clusters holding @p aBlockItems. False when none of them
    /// are constrained, which tells the caller to edit without the solver.
    bool Build( BOARD* aBoard, const std::vector<BOARD_ITEM*>& aBlockItems );

    /**
     * Solve one frame of the block edit and apply it.
     *
     * @param aPinnedAnchors the frame's anchors with their targets in IU.
     * @param aTranslatedShapes shapes the frame carried whole.
     * @param aTranslation how far they travelled from the starting geometry.
     * @param[out] aModified the shapes the solve moved, for the caller to stage and redraw.
     * @param aBeforeModify called with each item just before it is written, for the undo commit.
     * @return SOLVED after the geometry was written, REFUSED when an anchor could not be
     *         placed and the geometry is untouched, and NOT_MODELED when there was nothing
     *         to solve.
     */
    CONSTRAINT_BLOCK_RESULT
    Solve( const std::vector<std::pair<CONSTRAINT_MEMBER, VECTOR2I>>& aPinnedAnchors,
           const std::set<KIID>& aTranslatedShapes, const VECTOR2I& aTranslation, std::vector<PCB_SHAPE*>* aModified,
           const std::function<void( BOARD_ITEM* )>& aBeforeModify );
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
 * @param[out] aModified the neighbor shapes whose geometry the solve changed (excludes the
 *                       dragged shape itself), for the caller to stage in a commit.
 * @param aBeforeModify if set, invoked with each item (neighbor shape or reference constraint whose
 *                      measured value changed) immediately before it is modified, so the caller can
 *                      #BOARD_COMMIT::Modify it for a clean undo.  The dragged shape is excluded unless
 *                      @p aIncludeDragged, since the caller stages it itself.
 * @param aIncludeDragged if true, the dragged shape is also reported in @p aModified and passed to
 *                        @p aBeforeModify -- for callers (e.g. solve-on-create) that pin where the shape
 *                        already is and do not stage it themselves.
 * @param aStabilize is?
 * @param aEdited other shapes in this cluster edited alongside the dragged one left free instead
 *                of stay-pinned back.
 * @param aCoDragged second anchor moved by the same handle with its own target for a polygon edge
 *                   drag pinned at the same weight as the primary.
 * @param aFixedShapes shapes to freeze whole, so the solve moves the rest of the cluster around them
 * @param aHoldDraggedRigid holds the dragged shape's geometry, so it travels whole instead of
 *                          stretching to meet the relation.
 * @return the diagnosis; .solved is false if the cluster could not be built or did not converge.
 */
CONSTRAINT_DIAGNOSIS
SolveCluster( BOARD* aBoard, const CONSTRAINT_MEMBER& aDragged, const VECTOR2I& aCursor,
              std::vector<PCB_SHAPE*>*                  aModified = nullptr,
              const std::function<void( BOARD_ITEM* )>& aBeforeModify = {}, bool aIncludeDragged = false,
              bool aStabilize = false, const std::set<KIID>& aEdited = {},
              const std::optional<std::pair<CONSTRAINT_MEMBER, VECTOR2I>>& aCoDragged = std::nullopt,
              const std::set<KIID>& aFixedShapes = {}, bool aHoldDraggedRigid = false );


/**
 * The shapes a just-authored constraint should treat as an immovable reference, for the caller to
 * pass to ApplyConstraintImmediately().
 *
 * The point-on-line and midpoint pickers always take the point first, so the line is the reference
 * and the point is what should move.  Empty for every other type, and for the cases where freezing
 * would leave nothing able to move.
 *
 * Only the interactive authoring path may pass this.  The drawing tool's auto-bindings build the
 * same types with the same member roles, so nothing here can tell them apart -- they are safe only
 * because they never call this.  Do not wire it into snapAutoConstraints(): a corridor pin's line
 * is the shape just drawn, and freezing it would drag existing board geometry instead.
 */
std::set<KIID> ConstraintReferenceShapes( BOARD* aBoard, const PCB_CONSTRAINT* aConstraint );


/**
 * Solve a just-created constraint's cluster so the geometry snaps to satisfy it (SolidWorks-style),
 * pinning the constraint's first member at its current position and letting the solver move the
 * rest.  @p aConstraint must already be on the board.  A failed solve leaves geometry untouched.
 *
 * @param aBoard is the #BOARD object to constrain.
 * @param[in] aConstraint is the constraint to apply to \a aBoard.
 * @param[out] aModified are the shapes the solve moved, for the caller to stage in a commit.
 * @param aBeforeModify if set, invoked with each moved shape and each reference constraint whose
 *                      value changed, just before it changes, so the caller can stage it in the commit.
 * @param aFixedShapes shapes to freeze whole for this solve, for a caller that knows which member
 *                     is the reference the others should move to.
 */
CONSTRAINT_DIAGNOSIS ApplyConstraintImmediately( BOARD* aBoard, const PCB_CONSTRAINT* aConstraint,
                                                 std::vector<PCB_SHAPE*>*                  aModified = nullptr,
                                                 const std::function<void( BOARD_ITEM* )>& aBeforeModify = {},
                                                 const std::set<KIID>&                     aFixedShapes = {} );


/// True if @p aBoard resolves at least one geometric constraint (see CollectResolvedConstraints()).
/// Cheap, short-circuiting guard so an unconstrained board pays nothing on every edit.
bool BoardHasConstraints( BOARD* aBoard );


/// The distinct board positions that fixed positions pin on @p aItems. A transform of those items
/// has to leave every one of them where it is.
std::vector<VECTOR2I> ConstraintPinnedPositions( BOARD* aBoard, const std::set<KIID>& aItems );


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
bool ReSolveShapeClustersHoldingEdited( BOARD* aBoard, const std::vector<PCB_SHAPE*>& aEditedShapes,
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
    std::vector<KIID> padIds;            ///< Cluster pads
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
