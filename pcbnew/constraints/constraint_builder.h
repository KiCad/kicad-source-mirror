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

#ifndef CONSTRAINT_BUILDER_H_
#define CONSTRAINT_BUILDER_H_

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <vector>

#include <math/vector2d.h>
#include <geometry/eda_angle.h>

#include <constraints/pcb_constraint.h>

class BOARD;
class BOARD_ITEM;
class PCB_DIMENSION_BASE;
class PCB_SHAPE;
class SEG;


/**
 * The corner angle between two segments, in the closed range [0, 180] degrees.
 *
 * The vertex is the nearest pair of endpoints (one per segment); the angle is measured between the
 * two rays running from that vertex toward each segment's other endpoint.  Defining both rays "away
 * from the vertex" makes the value invariant to each segment's stored START/END order and to member
 * order, and it reads the obtuse/acute corner the user sees (a 120 degree corner stays 120, not 60).
 * Nearest-endpoint pairing avoids SEG::IntersectLines, so near-parallel segments stay stable.
 */
EDA_ANGLE MeasureCornerAngle( const SEG& aA, const SEG& aB );


/// A selectable feature of a shape (a segment endpoint, arc centre, ...) and its location.
struct CONSTRAINT_ANCHOR_POINT
{
    CONSTRAINT_ANCHOR anchor;
    VECTOR2I          pos;
    int               index = -1;   ///< Vertex ordinal; only meaningful for the VERTEX anchor.
};


/**
 * True when polygon has one non empty hole free arc free outline making it solver eligible
 * Shared by adapter ingestion and anchor enumeration so picker matches solver
 */
bool ConstraintPolygonIsModelable( const PCB_SHAPE* aShape );


/**
 * Enumerate a shape constraint anchors with positions segment and arc endpoints arc centre
 * circle centre rectangle and polygon vertices empty list if none
 * Shared by anchor picking and the endpoint selection overlay so both agree what is bindable
 */
std::vector<CONSTRAINT_ANCHOR_POINT> ConstraintShapeAnchors( const PCB_SHAPE* aShape );


/**
 * VERTEX anchor at ordinal @p aIndex of a rectangle or eligible polygon or std::nullopt if the
 * shape has no such vertex bad ordinal arc bearing polygon or wrong shape kind
 *
 * Point editor identifies a dragged corner or vertex by ordinal Returned position is the
 * authoritative solve target since a clamped drag can overshoot it so never match the cursor
 */
std::optional<CONSTRAINT_ANCHOR_POINT> ConstraintShapeVertex( const PCB_SHAPE* aShape, int aIndex );


/// Every PCB_SHAPE on the board (drawings plus footprint graphics) -- the candidates constraints
/// can reference.  Shared by the anchor and segment hit-tests so the board walk lives in one place.
std::vector<PCB_SHAPE*> CollectConstraintShapes( BOARD* aBoard );


/// Every constrainable item on the board -- shapes plus dimensions -- for board-wide anchor picking.
std::vector<BOARD_ITEM*> CollectConstrainableItems( BOARD* aBoard );


/**
 * The {shape, anchor} member nearest @p aPos within @p aMaxDist among @p aShapes, or std::nullopt.
 * Shared hit-test kernel for board-wide anchor picking and the selection-scoped endpoint overlay.
 */
std::optional<CONSTRAINT_MEMBER> NearestAnchorAmong( const std::vector<PCB_SHAPE*>& aShapes,
                                                     const VECTOR2I& aPos, double aMaxDist );


/**
 * Build a constraint of @p aType from a set of selected board items, or nullptr if the selection
 * does not fit the type (wrong count or kind of items).
 *
 * The authoring tool works on whole selected shapes, so every member uses the WHOLE anchor.  For
 * the fixed-length / fixed-radius families the driving value is measured from the current
 * geometry, so the constraint pins the shape at its present size until the user edits the value.
 *
 * This is split out from the tool so the selection-to-constraint logic is unit-testable without
 * the interactive tool framework.
 */
std::unique_ptr<PCB_CONSTRAINT> BuildConstraintFromItems( BOARD_ITEM* aParent,
                                                          PCB_CONSTRAINT_TYPE aType,
                                                          const std::vector<BOARD_ITEM*>& aItems );


/**
 * Find the constrainable-item anchor (a shape's segment/arc endpoint or centre, or a dimension's
 * feature point) nearest @p aPos within @p aMaxDist, for point-anchored constraint authoring
 * (coincident, midpoint, ...).
 *
 * Anchors in @p aExclude are skipped by member identity (item KIID plus anchor), so the previously
 * picked handle is passed over and the next-nearest is returned.  Two distinct shapes whose
 * endpoints coincide in space stay separately pickable, since only the exact handle is excluded.
 *
 * @return the {item, anchor} member, or std::nullopt if no anchor is close enough.
 */
std::optional<CONSTRAINT_MEMBER> NearestConstraintAnchor( BOARD* aBoard, const VECTOR2I& aPos,
                                                          double aMaxDist,
                                                          const std::vector<CONSTRAINT_MEMBER>& aExclude = {} );


/**
 * The board item a constraint may reference: a PCB_SHAPE or a dimension, or nullptr for anything
 * else (or a deleted item).  Constraints bind these two families; everything else is unconstrainable.
 */
BOARD_ITEM* ResolveConstrainableItem( BOARD* aBoard, const KIID& aId );


/// One of a dimension feature points bound coincident to an object anchor by draw time auto
/// constrain @p dimAnchor is the dimension START or END @p target is the anchor it binds to
struct DIMENSION_ENDPOINT_BINDING
{
    CONSTRAINT_ANCHOR dimAnchor;
    CONSTRAINT_MEMBER target;
};


/**
 * Choose the coincident bindings a freshly drawn dimension endpoints should take so it tracks
 * the geometry it measures
 *
 * Prefers one object near both endpoints binding START and END to it else binds each endpoint to
 * its own nearest anchor independently possibly on different objects and partial binding is fine
 * Excludes the dimension own anchors @p aEnd is std::nullopt for a leader or centre mark which
 * binds only START
 */
std::vector<DIMENSION_ENDPOINT_BINDING>
SelectDimensionEndpointBindings( BOARD* aBoard, const KIID& aDimension, const VECTOR2I& aStart,
                                 const std::optional<VECTOR2I>& aEnd, double aMaxDist );


/**
 * The constraint anchors an item exposes.  Delegates to ConstraintShapeAnchors for a shape; for a
 * dimension it returns the measured feature points (START/END for aligned/orthogonal/radial, START
 * only for leader/center -- their other point is a control point, not a feature).
 */
std::vector<CONSTRAINT_ANCHOR_POINT> ConstraintItemAnchors( const BOARD_ITEM* aItem );


/**
 * Current location of a constraint member's anchor (its shape's START/END/CENTER, or a dimension's
 * feature point), or std::nullopt if the referenced item is gone or exposes no such anchor.
 */
std::optional<VECTOR2I> ConstraintAnchorPosition( BOARD* aBoard, const CONSTRAINT_MEMBER& aMember );


/**
 * Value a freshly authored constraint dialog should open with
 *
 * Defaults to the geometry the tool just measured Once the user types a value for a type this
 * session that value sticks for future same type constraints instead of the new measurement
 * Keyed by type so length and angle values never mix
 */
double InitialConstraintValue( PCB_CONSTRAINT_TYPE aType, double aMeasured,
                               const std::map<PCB_CONSTRAINT_TYPE, double>& aRemembered );


/**
 * The candidate shape whose outline @p aPos hits within @p aMaxDist, or std::nullopt.  Pure over an
 * explicit candidate set (the caller passes the constrained shapes), so hovering an unconstrained
 * shape lying closer is ignored, and the helper stays unit-testable without the tool.
 */
std::optional<KIID> NearestConstrainedShape( const std::vector<PCB_SHAPE*>& aCandidates,
                                             const VECTOR2I& aPos, int aMaxDist );


/**
 * Single circle or arc a radial dimension binds to or std::nullopt
 *
 * Auto constrains only when @p aCenter and @p aRim both land on the same object centre and
 * circumference within @p aMaxDist Caller then binds centre coincident and rim point on circle
 * when no single circle or arc plays both roles nothing is bound
 */
std::optional<KIID> SelectRadialDimensionTarget( BOARD* aBoard, const KIID& aDimension,
                                                 const VECTOR2I& aCenter, const VECTOR2I& aRim,
                                                 double aMaxDist );


/**
 * True when both of @p aDimension measured endpoints are bound to anchors that still resolve a
 * coincident per endpoint for aligned or orthogonal or a centre and rim pair for radial the pair
 * the draw time auto constrain authors
 * A stale or deleted target does not count so the Driving mode this predicate gates degrades
 * rather than drive a dimension whose link to the geometry is broken
 */
bool DimensionEndpointsBound( BOARD* aBoard, const PCB_DIMENSION_BASE* aDimension );


/// Mode a value bearing dimension value is in Driven mirrors measured geometry Driving forces
/// geometry to the entered length Arbitrary shows custom text
enum class DIM_VALUE_MODE : int
{
    DRIVEN,
    DRIVING,
    ARBITRARY
};


/// True for dimension types with a measured value aligned orthogonal or radial offering the
/// Driven Driving or Arbitrary mode Centre and leader marks have none
bool DimensionHasValueMode( const PCB_DIMENSION_BASE* aDimension );


/**
 * Self FIXED_LENGTH constraint whose members are exactly @p aDimension START and END or nullptr
 * the driving length constraint the value mode owns
 * Scans board level and footprint parented constraints since SetDimensionValueMode parents to
 * the owning dimension footprint not necessarily the first
 */
PCB_CONSTRAINT* FindDimensionLengthConstraint( BOARD* aBoard, const PCB_DIMENSION_BASE* aDimension );


/**
 * True when Driving mode may be offered for @p aDimension needs both endpoints bound via
 * DimensionEndpointsBound or an existing driving length so removed bindings never silently drop it
 */
bool DimensionCanDrive( BOARD* aBoard, const PCB_DIMENSION_BASE* aDimension );


/**
 * The value mode @p aDimension is in, derived from state: a self driving length means Driving, else
 * enabled override text means Arbitrary, else the default Driven (the value mirrors the measured
 * geometry).
 */
DIM_VALUE_MODE DimensionValueMode( BOARD* aBoard, const PCB_DIMENSION_BASE* aDimension );


/**
 * Apply a value mode transition to @p aDimension Driving creates or updates the driving length
 * with @p aDrivingLengthIU Driven and Arbitrary drop any driving length Arbitrary also sets the
 * override text from @p aOverrideText when provided
 *
 * A Driving transition rejects leaving the board untouched when DimensionCanDrive fails or the
 * length is absent or not positive so an unbound dimension never grows a constraint its geometry
 * cannot follow
 *
 * Callers pass their commit staging operations @p aBeforeModify runs before an item changes
 * @p aStageAdd receives a fresh constraint to join the board and @p aBeforeRemove receives one to
 * retire unedited
 *
 * @return the live driving constraint for the caller to re solve once it joins the board or
 *         nullptr when the transition leaves no driving length
 */
PCB_CONSTRAINT* SetDimensionValueMode( BOARD* aBoard, PCB_DIMENSION_BASE* aDimension, DIM_VALUE_MODE aMode,
                                       std::optional<int>                        aDrivingLengthIU,
                                       const std::optional<wxString>&            aOverrideText,
                                       const std::function<void( BOARD_ITEM* )>& aBeforeModify,
                                       const std::function<void( BOARD_ITEM* )>& aStageAdd,
                                       const std::function<void( BOARD_ITEM* )>& aBeforeRemove );


/**
 * Repoint persisted VERTEX constraint members after an outline edit of polygon @p aPoly inserts
 * or removes a vertex at ordinal @p aChangedIndex Members at or past that point shift by
 * @p aDelta so each keeps naming the corner it was authored on
 *
 * A member on the removed vertex itself cannot be repaired since every solver form is fixed
 * arity so its whole constraint retires through @p aBeforeRemove a constraint about to be
 * reindexed is staged through @p aBeforeModify first Callers pass the geometry commit Modify and
 * Remove so undo restores outline indices and retired constraints together across board and
 * footprints mirroring DimensionEndpointsBound
 */
void RemapPolygonVertexMembers( BOARD* aBoard, const KIID& aPoly, int aChangedIndex, int aDelta,
                                const std::function<void( BOARD_ITEM* )>& aBeforeModify,
                                const std::function<void( BOARD_ITEM* )>& aBeforeRemove );

#endif // CONSTRAINT_BUILDER_H_
