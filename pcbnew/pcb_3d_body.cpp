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

#include <pcb_3d_body.h>

#include <api/api_enums.h>
#include <api/api_utils.h>
#include <api/board/board_types.pb.h>
#include <google/protobuf/any.pb.h>
#include <i18n_utility.h>
#include <properties/property.h>
#include <properties/property_mgr.h>
#include <board.h>
#include <footprint.h>


PCB_3D_BODY::PCB_3D_BODY( BOARD_ITEM* aParent ) :
        BOARD_ITEM( aParent, PCB_3D_BODY_T, UNDEFINED_LAYER )
{
}


void PCB_3D_BODY::Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    if( m_extruded.m_layer == UNDEFINED_LAYER )
        return;

    if( BOARD* board = GetBoard() )
        m_extruded.m_layer = board->FlipLayer( m_extruded.m_layer );
    else
        m_extruded.m_layer = ::FlipLayer( m_extruded.m_layer );
}


void PCB_3D_BODY::CopyFrom( const BOARD_ITEM* aOther )
{
    wxCHECK( aOther && aOther->Type() == PCB_3D_BODY_T, /* void */ );
    *this = *static_cast<const PCB_3D_BODY*>( aOther );
}


double PCB_3D_BODY::Similarity( const BOARD_ITEM& aOther ) const
{
    if( aOther.Type() != Type() )
        return 0.0;

    const PCB_3D_BODY& other = static_cast<const PCB_3D_BODY&>( aOther );

    if( m_kind != other.m_kind )
        return 0.0;

    return m_extruded == other.m_extruded ? 1.0 : 0.5;
}


bool PCB_3D_BODY::operator==( const PCB_3D_BODY& aOther ) const
{
    return m_kind == aOther.m_kind && m_extruded == aOther.m_extruded;
}


bool PCB_3D_BODY::operator==( const BOARD_ITEM& aOther ) const
{
    if( aOther.Type() != Type() )
        return false;

    return *this == static_cast<const PCB_3D_BODY&>( aOther );
}


void PCB_3D_BODY::swapData( BOARD_ITEM* aImage )
{
    assert( aImage->Type() == PCB_3D_BODY_T );

    std::swap( *( (PCB_3D_BODY*) this ), *( (PCB_3D_BODY*) aImage ) );
}


void PCB_3D_BODY::Serialize( kiapi::board::types::Body3D& aOutput ) const
{
    using namespace kiapi::board;

    aOutput.mutable_id()->set_value( m_Uuid.AsStdString() );

    if( FOOTPRINT* parent = GetParentFootprint() )
        aOutput.mutable_parent()->set_value( parent->m_Uuid.AsStdString() );
    else if( const BOARD* board = GetBoard() )
        aOutput.mutable_parent()->set_value( board->m_Uuid.AsStdString() );

    aOutput.set_kind( types::Body3DKind::BDK_EXTRUDED );

    kiapi::common::PackVector3D( *aOutput.mutable_scale(), m_extruded.m_scale );
    kiapi::common::PackVector3D( *aOutput.mutable_rotation(), m_extruded.m_rotation );
    kiapi::common::PackVector3D( *aOutput.mutable_offset(), m_extruded.m_offset );
    aOutput.set_visible( m_extruded.m_show );

    types::ExtrudedBody* extruded = aOutput.mutable_extruded();
    kiapi::common::PackDistance( *extruded->mutable_height(), m_extruded.m_height );
    kiapi::common::PackDistance( *extruded->mutable_standoff(), m_extruded.m_standoff );
    extruded->set_outline_layer( ToProtoEnum<PCB_LAYER_ID, types::BoardLayer>( m_extruded.m_layer ) );

    switch( m_extruded.m_material )
    {
    case EXTRUSION_MATERIAL::PLASTIC: extruded->set_material( types::EM_PLASTIC ); break;
    case EXTRUSION_MATERIAL::MATTE:   extruded->set_material( types::EM_MATTE );   break;
    case EXTRUSION_MATERIAL::METAL:   extruded->set_material( types::EM_METAL );   break;
    case EXTRUSION_MATERIAL::COPPER:  extruded->set_material( types::EM_COPPER );  break;
    }

    if( m_extruded.m_color != KIGFX::COLOR4D::UNSPECIFIED )
        kiapi::common::PackColor( *extruded->mutable_color(), m_extruded.m_color );
}


void PCB_3D_BODY::Serialize( google::protobuf::Any& aContainer ) const
{
    kiapi::board::types::Body3D msg;
    Serialize( msg );
    aContainer.PackFrom( msg );
}


bool PCB_3D_BODY::Deserialize( const kiapi::board::types::Body3D& aInput )
{
    using namespace kiapi::board;

    if( aInput.kind() != types::BDK_EXTRUDED || !aInput.has_extruded() )
        return false;

    SetUuidDirect( KIID( aInput.id().value() ) );

    m_extruded.m_scale = kiapi::common::UnpackVector3D( aInput.scale() );
    m_extruded.m_rotation = kiapi::common::UnpackVector3D( aInput.rotation() );
    m_extruded.m_offset = kiapi::common::UnpackVector3D( aInput.offset() );
    m_extruded.m_show = aInput.visible();

    const types::ExtrudedBody& extruded = aInput.extruded();
    m_extruded.m_height = kiapi::common::UnpackDistance( extruded.height() );
    m_extruded.m_standoff = kiapi::common::UnpackDistance( extruded.standoff() );
    m_extruded.m_layer = FromProtoEnum<PCB_LAYER_ID, types::BoardLayer>( extruded.outline_layer() );

    switch( extruded.material() )
    {
    default:
    case types::EM_UNKNOWN: m_extruded.m_material = EXTRUSION_MATERIAL::PLASTIC; break;
    case types::EM_PLASTIC: m_extruded.m_material = EXTRUSION_MATERIAL::PLASTIC; break;
    case types::EM_MATTE:   m_extruded.m_material = EXTRUSION_MATERIAL::MATTE;   break;
    case types::EM_METAL:   m_extruded.m_material = EXTRUSION_MATERIAL::METAL;   break;
    case types::EM_COPPER:  m_extruded.m_material = EXTRUSION_MATERIAL::COPPER;  break;
    }

    if( extruded.has_color() )
        m_extruded.m_color = kiapi::common::UnpackColor( extruded.color() );
    else
        m_extruded.m_color = KIGFX::COLOR4D::UNSPECIFIED;

    return true;
}


bool PCB_3D_BODY::Deserialize( const google::protobuf::Any& aContainer )
{
    kiapi::board::types::Body3D msg;

    if( !aContainer.UnpackTo( &msg ) )
        return false;

    return Deserialize( msg );
}


static struct PCB_3D_BODY_DESC
{
    PCB_3D_BODY_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_3D_BODY );
        propMgr.InheritsAfter( TYPE_HASH( PCB_3D_BODY ), TYPE_HASH( BOARD_ITEM ) );
    }
} _PCB_3D_BODY_DESC;
