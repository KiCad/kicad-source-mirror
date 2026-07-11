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

#ifndef PCB_CONSTRAINT_H_
#define PCB_CONSTRAINT_H_

#include <optional>
#include <vector>

#include <board_item.h>


/**
 * The geometric relationship a #PCB_CONSTRAINT enforces between its members.
 *
 * The set mirrors the constraint families the planegcs adapter maps onto
 * (see pcbnew/constraints/board_constraint_adapter.*).  Ordinals are not part
 * of the file format; serialization is by token name, so values may be
 * reordered freely.
 */
enum class PCB_CONSTRAINT_TYPE
{
    UNDEFINED = 0,
    COINCIDENT,         ///< Two points are made to coincide.
    HORIZONTAL,         ///< A segment (or two points) is horizontal.
    VERTICAL,           ///< A segment (or two points) is vertical.
    PARALLEL,           ///< Two segments are parallel.
    PERPENDICULAR,      ///< Two segments are perpendicular.
    COLLINEAR,          ///< Two segments lie on the same line.
    SYMMETRIC,          ///< Two points are mirror images about an axis.
    EQUAL_LENGTH,       ///< Two segments have equal length.
    EQUAL_RADIUS,       ///< Two arcs/circles have equal radius.
    POINT_ON_LINE,      ///< A point lies on a segment's supporting line.
    MIDPOINT,           ///< A point is the midpoint of a segment.
    FIXED_POSITION,     ///< A point is locked at its current location.
    FIXED_LENGTH,       ///< A segment has a driving length value.
    CONCENTRIC,         ///< Two arcs/circles share a center.
    FIXED_RADIUS,       ///< An arc/circle has a driving radius value.
    ANGULAR_DIMENSION,  ///< An angle between members (driving or reference).
};


/**
 * Which feature of a referenced board item participates in a constraint.
 *
 * A bare KIID cannot say "the start point" vs "the end point" of a segment, so
 * each constraint member pairs the referenced item with one of these anchors.
 * START/END resolve the ambiguity the geometric-meaning bitmask POINT_TYPE
 * (libs/kimath/include/geometry/point_types.h) cannot with its single PT_END.
 */
enum class CONSTRAINT_ANCHOR
{
    WHOLE = 0,  ///< The item as a whole (a segment as a line, a circle).
    START,      ///< First endpoint of a segment or arc.
    END,        ///< Second endpoint of a segment or arc.
    MID,        ///< Midpoint of a segment or arc.
    CENTER,     ///< Center of an arc or circle.
    RADIUS,     ///< The radius (scalar feature) of an arc or circle.
};


/**
 * One participant in a constraint: a referenced board item plus the feature of
 * that item that participates.
 */
struct CONSTRAINT_MEMBER
{
    KIID              m_item;    ///< Referenced board item, usually a PCB_SHAPE.
    CONSTRAINT_ANCHOR m_anchor;  ///< Which feature of that item participates.

    CONSTRAINT_MEMBER() :
            m_item( niluuid ),
            m_anchor( CONSTRAINT_ANCHOR::WHOLE )
    {
    }

    CONSTRAINT_MEMBER( const KIID& aItem, CONSTRAINT_ANCHOR aAnchor ) :
            m_item( aItem ),
            m_anchor( aAnchor )
    {
    }

    bool operator==( const CONSTRAINT_MEMBER& aOther ) const = default;
};


/// Stable file-format token for a constraint type (e.g. "parallel").  Used by serialization.
const char* ConstraintTypeToken( PCB_CONSTRAINT_TYPE aType );

/// Parse a constraint-type token; returns UNDEFINED for an unknown token.
PCB_CONSTRAINT_TYPE ConstraintTypeFromToken( const wxString& aToken );

/// Stable file-format token for a member anchor (e.g. "start").
const char* ConstraintAnchorToken( CONSTRAINT_ANCHOR aAnchor );

/// Parse an anchor token; returns WHOLE for an unknown token.
CONSTRAINT_ANCHOR ConstraintAnchorFromToken( const wxString& aToken );

/// Translated name of the feature an anchor references (e.g. "start"); empty for WHOLE.  The
/// caller supplies any surrounding punctuation.
wxString ConstraintAnchorLabel( CONSTRAINT_ANCHOR aAnchor );

/// True if this type's value is a length in IU (serialized in mm); false for an angle in degrees.
bool ConstraintValueIsLength( PCB_CONSTRAINT_TYPE aType );

/// Human-readable name of a constraint type (e.g. "Parallel"), for menus and lists.
wxString ConstraintTypeLabel( PCB_CONSTRAINT_TYPE aType );

/// Compact glyph for a constraint type (e.g. "//", "=") for on-canvas badges.
wxString ConstraintTypeGlyph( PCB_CONSTRAINT_TYPE aType );

enum class EDA_UNITS : int;


/**
 * A geometric constraint between board items (issue #2329).
 *
 * A PCB_CONSTRAINT carries no geometry, layer, or net of its own.  It is a
 * BOARD_ITEM so that it inherits undo/redo, clone, copy/paste and the KIID
 * cache (mirroring PCB_GROUP, which is likewise a geometry-free container), but
 * it is deliberately excluded from selection, plotting, DRC, 3D, netlist and
 * connectivity.  Its members reference other board items by KIID; resolution
 * happens after load via the two-phase parser, exactly like PCB_GROUP.
 */
class PCB_CONSTRAINT : public BOARD_ITEM
{
public:
    PCB_CONSTRAINT( BOARD_ITEM* aParent = nullptr );
    PCB_CONSTRAINT( BOARD_ITEM* aParent, PCB_CONSTRAINT_TYPE aType );

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && PCB_CONSTRAINT_T == aItem->Type();
    }

    wxString GetClass() const override { return wxT( "PCB_CONSTRAINT" ); }

    PCB_CONSTRAINT_TYPE GetConstraintType() const { return m_type; }
    void                SetConstraintType( PCB_CONSTRAINT_TYPE aType ) { m_type = aType; }

    const std::vector<CONSTRAINT_MEMBER>& GetMembers() const { return m_members; }
    std::vector<CONSTRAINT_MEMBER>&       Members() { return m_members; }

    void AddMember( const KIID& aItem, CONSTRAINT_ANCHOR aAnchor = CONSTRAINT_ANCHOR::WHOLE )
    {
        m_members.emplace_back( aItem, aAnchor );
    }

    bool                  HasValue() const { return m_value.has_value(); }
    std::optional<double> GetValue() const { return m_value; }
    void                  SetValue( std::optional<double> aValue ) { m_value = aValue; }

    /// A driving constraint forces its value; a reference (non-driving) one only measures it.
    bool IsDriving() const { return m_driving; }
    void SetDriving( bool aDriving ) { m_driving = aDriving; }

    void Serialize( google::protobuf::Any& aContainer ) const override;
    bool Deserialize( const google::protobuf::Any& aContainer ) override;

    EDA_ITEM* Clone() const override;

    double Similarity( const BOARD_ITEM& aOther ) const override;
    bool   operator==( const PCB_CONSTRAINT& aOther ) const;
    bool   operator==( const BOARD_ITEM& aOther ) const override;

    // No-geometry surface: a constraint occupies no space and is never on a layer.
    VECTOR2I GetPosition() const override { return VECTOR2I(); }
    void     SetPosition( const VECTOR2I& ) override {}

    const BOX2I GetBoundingBox() const override { return BOX2I(); }

    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override { return false; }
    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy = 0 ) const override
    {
        return false;
    }

    std::vector<int> ViewGetLayers() const override { return {}; }

    // A constraint has no geometry, so the spatial transforms are no-ops (the base
    // implementations wxFAIL_MSG).  Its members move with their own host items.
    void Move( const VECTOR2I& aMoveVector ) override {}
    void Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle ) override {}
    void Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection ) override {}
    void Mirror( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection ) override {}

    /// Empty shape so any generic GetItemSet()/RunOnChildren consumer stays safe.
    std::shared_ptr<SHAPE> GetEffectiveShape( PCB_LAYER_ID aLayer = UNDEFINED_LAYER,
                                              FLASHING aFlash = FLASHING::DEFAULT ) const override;

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override;
    BITMAPS  GetMenuImage() const override;

    void RemapKIIDs( const std::map<KIID, KIID>& aIdMap ) override;

#if defined( DEBUG )
    void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif

protected:
    void swapData( BOARD_ITEM* aImage ) override;

private:
    PCB_CONSTRAINT_TYPE            m_type;
    std::vector<CONSTRAINT_MEMBER> m_members;
    std::optional<double>          m_value;
    bool                           m_driving;
};


/**
 * Display label for a constraint in lists and menus.
 *
 * A valued constraint reads "<type>: <value>" with the value formatted in @p aUnits (lengths)
 * or degrees (angles); a reference (non-driving) value is parenthesized, mirroring CAD reference
 * dimensions. A constraint with no value is just its type name.
 */
wxString ConstraintDisplayLabel( const PCB_CONSTRAINT& aConstraint, EDA_UNITS aUnits );


/**
 * Label for one constrained item in a list: the item's description with its anchored feature
 * appended, e.g. "Line ... (start)".  Shared by the board panel and the footprint-editor dialog.
 */
wxString ConstraintMemberLabel( BOARD_ITEM* aItem, CONSTRAINT_ANCHOR aAnchor,
                                UNITS_PROVIDER* aUnitsProvider );


/**
 * True if two constraints express the same relation: same type and the same members compared
 * without regard to order (so A-B equals B-A).  Value and driving are ignored, so a second
 * dimensional constraint on the same geometry -- which could only conflict -- also counts as a
 * duplicate.  Used to reject authoring an identical constraint.
 */
bool ConstraintsAreDuplicate( const PCB_CONSTRAINT& aA, const PCB_CONSTRAINT& aB );

#endif // PCB_CONSTRAINT_H_
