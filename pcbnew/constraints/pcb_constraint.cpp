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

#include <constraints/pcb_constraint.h>

#include <algorithm>

#include <base_units.h>
#include <bitmaps.h>
#include <eda_units.h>
#include <geometry/eda_angle.h>
#include <geometry/shape_compound.h>
#include <i18n_utility.h>
#include <pcb_shape.h>
#include <properties/property.h>
#include <properties/property_mgr.h>

#include <api/api_enums.h>
#include <api/board/board_types.pb.h>
#include <google/protobuf/any.pb.h>


const char* ConstraintTypeToken( PCB_CONSTRAINT_TYPE aType )
{
    switch( aType )
    {
    case PCB_CONSTRAINT_TYPE::COINCIDENT:        return "coincident";
    case PCB_CONSTRAINT_TYPE::HORIZONTAL:        return "horizontal";
    case PCB_CONSTRAINT_TYPE::VERTICAL:          return "vertical";
    case PCB_CONSTRAINT_TYPE::PARALLEL:          return "parallel";
    case PCB_CONSTRAINT_TYPE::PERPENDICULAR:     return "perpendicular";
    case PCB_CONSTRAINT_TYPE::COLLINEAR:         return "collinear";
    case PCB_CONSTRAINT_TYPE::SYMMETRIC:         return "symmetric";
    case PCB_CONSTRAINT_TYPE::EQUAL_LENGTH:      return "equal_length";
    case PCB_CONSTRAINT_TYPE::EQUAL_RADIUS:      return "equal_radius";
    case PCB_CONSTRAINT_TYPE::POINT_ON_LINE:     return "point_on_line";
    case PCB_CONSTRAINT_TYPE::MIDPOINT:          return "midpoint";
    case PCB_CONSTRAINT_TYPE::FIXED_POSITION:    return "fixed_position";
    case PCB_CONSTRAINT_TYPE::FIXED_LENGTH:      return "fixed_length";
    case PCB_CONSTRAINT_TYPE::CONCENTRIC:        return "concentric";
    case PCB_CONSTRAINT_TYPE::FIXED_RADIUS:      return "fixed_radius";
    case PCB_CONSTRAINT_TYPE::ANGULAR_DIMENSION: return "angular_dimension";
    case PCB_CONSTRAINT_TYPE::TANGENT:           return "tangent";
    case PCB_CONSTRAINT_TYPE::ARC_ANGLE:         return "arc_angle";
    case PCB_CONSTRAINT_TYPE::UNDEFINED:         return "undefined";
    }

    return "undefined";
}


PCB_CONSTRAINT_TYPE ConstraintTypeFromToken( const wxString& aToken )
{
    for( int i = 0; i <= static_cast<int>( PCB_CONSTRAINT_TYPE::ARC_ANGLE ); ++i )
    {
        PCB_CONSTRAINT_TYPE type = static_cast<PCB_CONSTRAINT_TYPE>( i );

        if( aToken == ConstraintTypeToken( type ) )
            return type;
    }

    return PCB_CONSTRAINT_TYPE::UNDEFINED;
}


bool ConstraintValueIsLength( PCB_CONSTRAINT_TYPE aType )
{
    switch( aType )
    {
    case PCB_CONSTRAINT_TYPE::FIXED_LENGTH:
    case PCB_CONSTRAINT_TYPE::FIXED_RADIUS:
        return true;
    default:
        return false;   // the angular types are angles; everything else is valueless
    }
}


const char* ConstraintAnchorToken( CONSTRAINT_ANCHOR aAnchor )
{
    switch( aAnchor )
    {
    case CONSTRAINT_ANCHOR::WHOLE:  return "whole";
    case CONSTRAINT_ANCHOR::START:  return "start";
    case CONSTRAINT_ANCHOR::END:    return "end";
    case CONSTRAINT_ANCHOR::MID:    return "mid";
    case CONSTRAINT_ANCHOR::CENTER: return "center";
    case CONSTRAINT_ANCHOR::RADIUS: return "radius";
    case CONSTRAINT_ANCHOR::VERTEX: return "vertex";
    }

    return "whole";
}


CONSTRAINT_ANCHOR ConstraintAnchorFromToken( const wxString& aToken )
{
    for( int i = 0; i <= static_cast<int>( CONSTRAINT_ANCHOR::VERTEX ); ++i )
    {
        CONSTRAINT_ANCHOR anchor = static_cast<CONSTRAINT_ANCHOR>( i );

        if( aToken == ConstraintAnchorToken( anchor ) )
            return anchor;
    }

    return CONSTRAINT_ANCHOR::WHOLE;
}


wxString ConstraintTypeGlyph( PCB_CONSTRAINT_TYPE aType )
{
    // Single-codepoint marks carried by both the newstroke stroke font and the OpenGL glyph atlas
    // so every GAL backend renders them without a missing-glyph box.
    switch( aType )
    {
    case PCB_CONSTRAINT_TYPE::COINCIDENT:        return wxT( "●" );
    case PCB_CONSTRAINT_TYPE::HORIZONTAL:        return wxT( "↔" );
    case PCB_CONSTRAINT_TYPE::VERTICAL:          return wxT( "↕" );
    case PCB_CONSTRAINT_TYPE::PARALLEL:          return wxT( "∥" );
    case PCB_CONSTRAINT_TYPE::PERPENDICULAR:     return wxT( "⊥" );
    case PCB_CONSTRAINT_TYPE::COLLINEAR:         return wxT( "─" );
    case PCB_CONSTRAINT_TYPE::SYMMETRIC:         return wxT( "⧓" );
    case PCB_CONSTRAINT_TYPE::EQUAL_LENGTH:      return wxT( "=" );
    case PCB_CONSTRAINT_TYPE::EQUAL_RADIUS:      return wxT( "⊜" );
    case PCB_CONSTRAINT_TYPE::POINT_ON_LINE:     return wxT( "∈" );
    case PCB_CONSTRAINT_TYPE::MIDPOINT:          return wxT( "½" );
    case PCB_CONSTRAINT_TYPE::FIXED_POSITION:    return wxT( "⌖" );
    case PCB_CONSTRAINT_TYPE::FIXED_LENGTH:      return wxT( "⟷" );
    case PCB_CONSTRAINT_TYPE::CONCENTRIC:        return wxT( "◎" );
    case PCB_CONSTRAINT_TYPE::FIXED_RADIUS:      return wxT( "r" );
    case PCB_CONSTRAINT_TYPE::ANGULAR_DIMENSION: return wxT( "∠" );
    case PCB_CONSTRAINT_TYPE::TANGENT:           return wxT( "T" );
    case PCB_CONSTRAINT_TYPE::ARC_ANGLE:         return wxT( "α" );
    case PCB_CONSTRAINT_TYPE::UNDEFINED:         return wxEmptyString;
    }

    return wxEmptyString;
}


BITMAPS ConstraintTypeBitmap( PCB_CONSTRAINT_TYPE aType )
{
    switch( aType )
    {
    case PCB_CONSTRAINT_TYPE::COINCIDENT: return BITMAPS::constraint_coincident;
    case PCB_CONSTRAINT_TYPE::HORIZONTAL: return BITMAPS::constraint_horizontal;
    case PCB_CONSTRAINT_TYPE::VERTICAL: return BITMAPS::constraint_vertical;
    case PCB_CONSTRAINT_TYPE::PARALLEL: return BITMAPS::constraint_parallel;
    case PCB_CONSTRAINT_TYPE::PERPENDICULAR: return BITMAPS::constraint_perpendicular;
    case PCB_CONSTRAINT_TYPE::COLLINEAR: return BITMAPS::constraint_collinear;
    case PCB_CONSTRAINT_TYPE::SYMMETRIC: return BITMAPS::constraint_symmetric;
    case PCB_CONSTRAINT_TYPE::EQUAL_LENGTH: return BITMAPS::constraint_equal_length;
    case PCB_CONSTRAINT_TYPE::EQUAL_RADIUS: return BITMAPS::constraint_equal_radius;
    case PCB_CONSTRAINT_TYPE::POINT_ON_LINE: return BITMAPS::constraint_point_on_line;
    case PCB_CONSTRAINT_TYPE::MIDPOINT: return BITMAPS::constraint_midpoint;
    case PCB_CONSTRAINT_TYPE::FIXED_POSITION: return BITMAPS::constraint_fixed_position;
    case PCB_CONSTRAINT_TYPE::FIXED_LENGTH: return BITMAPS::constraint_fixed_length;
    case PCB_CONSTRAINT_TYPE::CONCENTRIC: return BITMAPS::constraint_concentric;
    case PCB_CONSTRAINT_TYPE::FIXED_RADIUS: return BITMAPS::constraint_fixed_radius;
    case PCB_CONSTRAINT_TYPE::ANGULAR_DIMENSION: return BITMAPS::constraint_angular_dimension;
    case PCB_CONSTRAINT_TYPE::TANGENT: return BITMAPS::constraint_tangent;
    case PCB_CONSTRAINT_TYPE::ARC_ANGLE: return BITMAPS::constraint_arc_angle;
    case PCB_CONSTRAINT_TYPE::UNDEFINED: return BITMAPS::INVALID_BITMAP;
    }

    return BITMAPS::INVALID_BITMAP;
}


wxString ConstraintAnchorLabel( CONSTRAINT_ANCHOR aAnchor )
{
    switch( aAnchor )
    {
    case CONSTRAINT_ANCHOR::WHOLE:  return wxEmptyString;
    case CONSTRAINT_ANCHOR::START:  return _( "start" );
    case CONSTRAINT_ANCHOR::END:    return _( "end" );
    case CONSTRAINT_ANCHOR::MID:    return _( "mid" );
    case CONSTRAINT_ANCHOR::CENTER: return _( "center" );
    case CONSTRAINT_ANCHOR::RADIUS: return _( "radius" );
    case CONSTRAINT_ANCHOR::VERTEX: return _( "vertex" );
    }

    return wxEmptyString;
}


wxString ConstraintTypeLabel( PCB_CONSTRAINT_TYPE aType )
{
    switch( aType )
    {
    case PCB_CONSTRAINT_TYPE::COINCIDENT:        return _( "Coincident" );
    case PCB_CONSTRAINT_TYPE::HORIZONTAL:        return _( "Horizontal" );
    case PCB_CONSTRAINT_TYPE::VERTICAL:          return _( "Vertical" );
    case PCB_CONSTRAINT_TYPE::PARALLEL:          return _( "Parallel" );
    case PCB_CONSTRAINT_TYPE::PERPENDICULAR:     return _( "Perpendicular" );
    case PCB_CONSTRAINT_TYPE::COLLINEAR:         return _( "Collinear" );
    case PCB_CONSTRAINT_TYPE::SYMMETRIC:         return _( "Symmetric" );
    case PCB_CONSTRAINT_TYPE::EQUAL_LENGTH:      return _( "Equal length" );
    case PCB_CONSTRAINT_TYPE::EQUAL_RADIUS:      return _( "Equal radius" );
    case PCB_CONSTRAINT_TYPE::POINT_ON_LINE:     return _( "Point on line" );
    case PCB_CONSTRAINT_TYPE::MIDPOINT:          return _( "Midpoint" );
    case PCB_CONSTRAINT_TYPE::FIXED_POSITION:    return _( "Fixed position" );
    case PCB_CONSTRAINT_TYPE::FIXED_LENGTH:      return _( "Fixed length" );
    case PCB_CONSTRAINT_TYPE::CONCENTRIC:        return _( "Concentric" );
    case PCB_CONSTRAINT_TYPE::FIXED_RADIUS:      return _( "Fixed radius" );
    case PCB_CONSTRAINT_TYPE::ANGULAR_DIMENSION: return _( "Angular dimension" );
    case PCB_CONSTRAINT_TYPE::TANGENT:           return _( "Tangent" );
    case PCB_CONSTRAINT_TYPE::ARC_ANGLE:         return _( "Arc angle" );
    case PCB_CONSTRAINT_TYPE::UNDEFINED:         return _( "Undefined" );
    }

    return _( "Undefined" );
}


PCB_CONSTRAINT::PCB_CONSTRAINT( BOARD_ITEM* aParent ) :
        PCB_CONSTRAINT( aParent, PCB_CONSTRAINT_TYPE::UNDEFINED )
{
}


PCB_CONSTRAINT::PCB_CONSTRAINT( BOARD_ITEM* aParent, PCB_CONSTRAINT_TYPE aType ) :
        BOARD_ITEM( aParent, PCB_CONSTRAINT_T, UNDEFINED_LAYER ),
        m_type( aType ),
        m_driving( true )
{
}


void PCB_CONSTRAINT::Serialize( google::protobuf::Any& aContainer ) const
{
    using namespace kiapi::board::types;
    Constraint constraint;

    constraint.mutable_id()->set_value( m_Uuid.AsStdString() );

    constraint.set_type( ToProtoEnum<PCB_CONSTRAINT_TYPE, ConstraintType>( m_type ) );
    constraint.set_driving( m_driving );

    if( m_value.has_value() )
        constraint.set_value( *m_value );

    for( const CONSTRAINT_MEMBER& member : m_members )
    {
        ConstraintMember* m = constraint.add_members();
        m->mutable_item()->set_value( member.m_item.AsStdString() );
        m->set_anchor( ToProtoEnum<CONSTRAINT_ANCHOR, ConstraintAnchor>( member.m_anchor ) );

        // Only vertex anchors carry an index leaving it absent elsewhere keeps the negative one
        // sentinel out of the wire format
        if( member.m_anchor == CONSTRAINT_ANCHOR::VERTEX )
            m->set_index( member.m_index );
    }

    aContainer.PackFrom( constraint );
}


bool PCB_CONSTRAINT::Deserialize( const google::protobuf::Any& aContainer )
{
    using namespace kiapi::board::types;
    Constraint constraint;

    if( !aContainer.UnpackTo( &constraint ) )
        return false;

    SetUuidDirect( KIID( constraint.id().value() ) );

    m_type = FromProtoEnum<PCB_CONSTRAINT_TYPE, ConstraintType>( constraint.type() );
    m_driving = constraint.driving();

    if( constraint.has_value() )
        m_value = constraint.value();
    else
        m_value = std::nullopt;

    m_members.clear();

    // Members reference items by KIID, so they need no board to resolve (resolved on use).
    for( const auto& m : constraint.members() )
    {
        CONSTRAINT_ANCHOR anchor = FromProtoEnum<CONSTRAINT_ANCHOR, ConstraintAnchor>( m.anchor() );

        // A vertex member without an explicit index gets the negative one sentinel rather than a
        // silent vertex zero no other anchor may carry an index at all
        int index = anchor == CONSTRAINT_ANCHOR::VERTEX && m.has_index() ? m.index() : -1;

        m_members.emplace_back( KIID( m.item().value() ), anchor, index );
    }

    return true;
}


EDA_ITEM* PCB_CONSTRAINT::Clone() const
{
    // Copy constructor preserves the uuid and all members, matching PCB_GROUP.
    return new PCB_CONSTRAINT( *this );
}


void PCB_CONSTRAINT::swapData( BOARD_ITEM* aImage )
{
    assert( aImage->Type() == PCB_CONSTRAINT_T );

    std::swap( *this, *static_cast<PCB_CONSTRAINT*>( aImage ) );
}


void PCB_CONSTRAINT::RemapKIIDs( const std::map<KIID, KIID>& aIdMap )
{
    for( CONSTRAINT_MEMBER& member : m_members )
    {
        if( auto it = aIdMap.find( member.m_item ); it != aIdMap.end() )
            member.m_item = it->second;
    }
}


bool PCB_CONSTRAINT::operator==( const BOARD_ITEM& aOther ) const
{
    if( aOther.Type() != Type() )
        return false;

    return *this == static_cast<const PCB_CONSTRAINT&>( aOther );
}


bool PCB_CONSTRAINT::operator==( const PCB_CONSTRAINT& aOther ) const
{
    // Members are positional (a symmetric constraint's axis is not interchangeable with its
    // mirrored points), so compare them in order.
    return m_type == aOther.m_type
            && m_driving == aOther.m_driving
            && m_value == aOther.m_value
            && m_members == aOther.m_members;
}


double PCB_CONSTRAINT::Similarity( const BOARD_ITEM& aOther ) const
{
    if( aOther.Type() != Type() )
        return 0.0;

    const PCB_CONSTRAINT& other = static_cast<const PCB_CONSTRAINT&>( aOther );

    if( m_type != other.m_type )
        return 0.0;

    double similarity = 0.5;

    if( m_members == other.m_members )
        similarity += 0.3;

    if( m_driving == other.m_driving && m_value == other.m_value )
        similarity += 0.2;

    return similarity;
}


wxString PCB_CONSTRAINT::GetItemDescription( UNITS_PROVIDER*, bool ) const
{
    return wxString::Format( _( "Constraint: %s (%zu members)" ),
                             ConstraintTypeLabel( m_type ), m_members.size() );
}


wxString ConstraintDisplayLabel( const PCB_CONSTRAINT& aConstraint, EDA_UNITS aUnits )
{
    wxString label = ConstraintTypeLabel( aConstraint.GetConstraintType() );

    if( !aConstraint.HasValue() )
        return label;

    double   value = *aConstraint.GetValue();
    wxString text;

    if( ConstraintValueIsLength( aConstraint.GetConstraintType() ) )
        text = EDA_UNIT_UTILS::UI::MessageTextFromValue( pcbIUScale, aUnits, value );
    else
        text = EDA_UNIT_UTILS::UI::MessageTextFromValue( EDA_ANGLE( value, DEGREES_T ) );

    // Reference (non-driving) dimensions only measure, so show them parenthesized like CAD does.
    return wxString::Format( aConstraint.IsDriving() ? wxT( "%s: %s" ) : wxT( "%s: (%s)" ), label, text );
}


wxString ConstraintMemberLabel( BOARD_ITEM* aItem, const CONSTRAINT_MEMBER& aMember,
                                UNITS_PROVIDER* aUnitsProvider )
{
    if( !aItem )
        return _( "(missing)" );

    wxString text = aItem->GetItemDescription( aUnitsProvider, false );
    wxString anchor;

    if( aMember.m_anchor == CONSTRAINT_ANCHOR::VERTEX && aMember.m_index >= 0 )
    {
        const PCB_SHAPE* shape = dynamic_cast<const PCB_SHAPE*>( aItem );

        // Ordinals read 1 based matching the vertex editor pane grid row numbering Widened so a
        // hostile INT_MAX index from a file cannot overflow
        if( shape && shape->GetShape() == SHAPE_T::RECTANGLE )
            anchor = wxString::Format( _( "corner %lld" ), static_cast<long long>( aMember.m_index ) + 1 );
        else
            anchor = wxString::Format( _( "vertex %lld" ), static_cast<long long>( aMember.m_index ) + 1 );
    }
    else
    {
        anchor = ConstraintAnchorLabel( aMember.m_anchor );
    }

    if( !anchor.IsEmpty() )
        text += wxString::Format( wxT( " (%s)" ), anchor );

    return text;
}


bool ConstraintsAreDuplicate( const PCB_CONSTRAINT& aA, const PCB_CONSTRAINT& aB )
{
    if( aA.GetConstraintType() != aB.GetConstraintType() )
        return false;

    const std::vector<CONSTRAINT_MEMBER>& a = aA.GetMembers();
    const std::vector<CONSTRAINT_MEMBER>& b = aB.GetMembers();

    if( a.empty() || a.size() != b.size() )
        return false;

    // Match members as a multiset so member order does not matter (A-B is the same as B-A).
    // Roles survive this because a member's anchor encodes them; in SYMMETRIC and the point
    // families the mirror/probe points carry endpoint anchors while the axis/line is WHOLE, so
    // two constraints with the same multiset genuinely relate the same geometry the same way.
    return std::is_permutation( a.begin(), a.end(), b.begin(), b.end() );
}


BITMAPS PCB_CONSTRAINT::GetMenuImage() const
{
    return BITMAPS::measurement;
}


std::shared_ptr<SHAPE> PCB_CONSTRAINT::GetEffectiveShape( PCB_LAYER_ID, FLASHING ) const
{
    return std::make_shared<SHAPE_COMPOUND>();
}


static struct PCB_CONSTRAINT_DESC
{
    PCB_CONSTRAINT_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_CONSTRAINT );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_CONSTRAINT, BOARD_ITEM> );
        propMgr.InheritsAfter( TYPE_HASH( PCB_CONSTRAINT ), TYPE_HASH( BOARD_ITEM ) );

        // A constraint has no geometry, so hide the inherited positional/layer properties.
        propMgr.Mask( TYPE_HASH( PCB_CONSTRAINT ), TYPE_HASH( BOARD_ITEM ), _HKI( "Position X" ) );
        propMgr.Mask( TYPE_HASH( PCB_CONSTRAINT ), TYPE_HASH( BOARD_ITEM ), _HKI( "Position Y" ) );
        propMgr.Mask( TYPE_HASH( PCB_CONSTRAINT ), TYPE_HASH( BOARD_ITEM ), _HKI( "Layer" ) );
        propMgr.Mask( TYPE_HASH( PCB_CONSTRAINT ), TYPE_HASH( BOARD_ITEM ), _HKI( "Locked" ) );

        const wxString constraintTab = _HKI( "Constraint Properties" );

        // Driving vs reference is the one freely-editable property (the "convert to reference"
        // toggle); type and members are intrinsic and set at creation.
        propMgr.AddProperty( new PROPERTY<PCB_CONSTRAINT, bool>( _HKI( "Driving" ),
                             &PCB_CONSTRAINT::SetDriving, &PCB_CONSTRAINT::IsDriving ),
                             constraintTab );
    }
} _PCB_CONSTRAINT_DESC;
