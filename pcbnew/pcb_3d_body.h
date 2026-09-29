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

#ifndef PCB_3D_BODY_H
#define PCB_3D_BODY_H

#include <bitmaps/bitmaps_list.h>
#include <board_item.h>
#include <gal/color4d.h>
#include <math/vector3.h>

class PCB_3D_BODY;

enum class EXTRUSION_MATERIAL
{
    PLASTIC = 0,
    MATTE,
    METAL,
    COPPER
};

class EXTRUDED_3D_BODY
{
public:
    EXTRUDED_3D_BODY() = default;

    int                m_height = 0;
    int                m_standoff = 0;
    PCB_LAYER_ID       m_layer = UNDEFINED_LAYER;
    KIGFX::COLOR4D     m_color = KIGFX::COLOR4D::UNSPECIFIED;
    EXTRUSION_MATERIAL m_material = EXTRUSION_MATERIAL::PLASTIC;
    bool               m_show = true;

    VECTOR3D m_scale{ 1.0, 1.0, 1.0 };
    VECTOR3D m_rotation{ 0.0, 0.0, 0.0 };
    VECTOR3D m_offset{ 0.0, 0.0, 0.0 };

    static KIGFX::COLOR4D GetDefaultColor( EXTRUSION_MATERIAL aMaterial )
    {
        switch( aMaterial )
        {
        default:
        case EXTRUSION_MATERIAL::PLASTIC: return KIGFX::COLOR4D( 0.2, 0.2, 0.2, 1.0 );
        case EXTRUSION_MATERIAL::MATTE: return KIGFX::COLOR4D( 0.4, 0.4, 0.4, 1.0 );
        case EXTRUSION_MATERIAL::METAL: return KIGFX::COLOR4D( 0.7, 0.7, 0.7, 1.0 );
        case EXTRUSION_MATERIAL::COPPER: return KIGFX::COLOR4D( 0.72, 0.45, 0.2, 1.0 );
        }
    }

    bool operator==( const EXTRUDED_3D_BODY& aOther ) const
    {
        return m_height == aOther.m_height && m_standoff == aOther.m_standoff && m_layer == aOther.m_layer
               && m_color == aOther.m_color && m_material == aOther.m_material && m_scale == aOther.m_scale
               && m_rotation == aOther.m_rotation && m_offset == aOther.m_offset && m_show == aOther.m_show;
    }

    static uint32_t PackColorKey( const KIGFX::COLOR4D& aColor )
    {
        return ( (uint8_t) ( aColor.r * 255 ) << 24 ) | ( (uint8_t) ( aColor.g * 255 ) << 16 )
               | ( (uint8_t) ( aColor.b * 255 ) << 8 ) | (uint8_t) ( aColor.a * 255 );
    }
};

namespace kiapi::board::types
{
class Body3D;
}

/**
 * A 3D body attached to a footprint (or, later, placed on a board).
 *
 * The payload is either an extruded outline (today) or a referenced model
 * (reserved for later). The body has no 2D presence: its layer is semantic
 * (which outline source to extrude), not a geometric placement.
 */
class PCB_3D_BODY : public BOARD_ITEM
{
public:
    enum class KIND
    {
        EXTRUDED = 0,
        MODEL // reserved; not constructible yet
    };

    explicit PCB_3D_BODY( BOARD_ITEM* aParent = nullptr );

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && PCB_3D_BODY_T == aItem->Type();
    }

    wxString GetClass() const override { return wxT( "PCB_3D_BODY" ); }

    wxString GetFriendlyName() const override { return _( "3D Body" ); }

    KIND GetKind() const { return m_kind; }

    const EXTRUDED_3D_BODY* GetExtruded() const { return &m_extruded; }
    EXTRUDED_3D_BODY*       MutableExtruded() { return &m_extruded; }

    /**
     * The payload layer names the outline source (auto, pad bbox, or a real
     * layer), so it is semantic rather than geometric: sentinel layers are
     * valid and the item must not be confined to a board's enabled layers.
     */
    PCB_LAYER_ID GetLayer() const override { return m_extruded.m_layer; }
    void         SetLayer( PCB_LAYER_ID aLayer ) override { m_extruded.m_layer = aLayer; }

    /// @copydoc BOARD_ITEM::IsLayerAgnostic
    bool IsLayerAgnostic() const override { return true; }

    void Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection ) override;

    void Serialize( google::protobuf::Any& aContainer ) const override;
    bool Deserialize( const google::protobuf::Any& aContainer ) override;
    void Serialize( kiapi::board::types::Body3D& aOutput ) const;
    bool Deserialize( const kiapi::board::types::Body3D& aInput );

    EDA_ITEM* Clone() const override { return new PCB_3D_BODY( *this ); }

    void CopyFrom( const BOARD_ITEM* aOther ) override;

    std::vector<int> ViewGetLayers() const override
    {
        return {};
    }

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override
    {
        return _( "3D Body" );
    }

    BITMAPS GetMenuImage() const override
    {
        return BITMAPS::dummy_item;
    }

    double Similarity( const BOARD_ITEM& aOther ) const override;

    bool operator==( const PCB_3D_BODY& aOther ) const;
    bool operator==( const BOARD_ITEM& aOther ) const override;

#if defined( DEBUG )
    void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif

protected:
    void swapData( BOARD_ITEM* aImage ) override;

private:
    KIND             m_kind = KIND::EXTRUDED;
    EXTRUDED_3D_BODY m_extruded;
};

#endif // PCB_3D_BODY_H
