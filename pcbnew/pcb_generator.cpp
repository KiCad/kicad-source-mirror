/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Alex Shvartzkop <dudesuchamazing@gmail.com>
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "pcb_generator.h"

#include <board.h>


PCB_GENERATOR::PCB_GENERATOR( BOARD_ITEM* aParent, PCB_LAYER_ID aLayer ) :
        PCB_GROUP( aParent, PCB_GENERATOR_T, aLayer )
{
}


PCB_GENERATOR::~PCB_GENERATOR()
{
}


PCB_GENERATOR* PCB_GENERATOR::DeepClone() const
{
    // Use copy constructor to get the same uuid and other fields
    PCB_GENERATOR* newGenerator = static_cast<PCB_GENERATOR*>( Clone() );
    newGenerator->m_items.clear();

    for( EDA_ITEM* member : m_items )
    {
        if( member->Type() == PCB_GROUP_T )
            newGenerator->AddItem( static_cast<PCB_GROUP*>( member )->DeepClone() );
        else
            newGenerator->AddItem( static_cast<BOARD_ITEM*>( member->Clone() ) );
    }

    return newGenerator;
}


std::vector<EDA_ITEM*> PCB_GENERATOR::GetPreviewItems( GENERATOR_TOOL* aTool,
                                                       PCB_BASE_EDIT_FRAME* aFrame,
                                                       bool aStatusItemsOnly )
{
    return std::vector<EDA_ITEM*>();
}


bool PCB_GENERATOR::MakeEditPoints( EDIT_POINTS& aEditPoints ) const
{
    return true;
}


bool PCB_GENERATOR::UpdateFromEditPoints( EDIT_POINTS& aEditPoints )
{
    return true;
}


bool PCB_GENERATOR::UpdateEditPoints( EDIT_POINTS& aEditPoints )
{
    return true;
}


const BOX2I PCB_GENERATOR::GetBoundingBox() const
{
    BOX2I bbox;
    return bbox;
}


void PCB_GENERATOR::Move( const VECTOR2I& aMoveVector )
{
    m_origin += aMoveVector;

    PCB_GROUP::Move( aMoveVector );
}

void PCB_GENERATOR::Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle )
{
    RotatePoint( m_origin, aRotCentre, aAngle );

    PCB_GROUP::Rotate( aRotCentre, aAngle );
}

void PCB_GENERATOR::Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    baseMirror( aCentre, aFlipDirection );

    SetLayer( GetBoard()->FlipLayer( GetLayer() ) );

    PCB_GROUP::Flip( aCentre, aFlipDirection );
}

void PCB_GENERATOR::Mirror( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    baseMirror( aCentre, aFlipDirection );

    PCB_GROUP::Mirror( aCentre, aFlipDirection );
}

void PCB_GENERATOR::baseMirror( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    if( aFlipDirection == FLIP_DIRECTION::TOP_BOTTOM )
        MIRROR( m_origin.y, aCentre.y );
    else
        MIRROR( m_origin.x, aCentre.x );
}

LSET PCB_GENERATOR::GetLayerSet() const
{
    return PCB_GROUP::GetLayerSet() | LSET( { GetLayer() } );
}


void PCB_GENERATOR::SetLayer( PCB_LAYER_ID aLayer )
{
    m_layer = aLayer;
}


wxString PCB_GENERATOR::GetGeneratorType() const
{
    return m_generatorType;
}


const STRING_ANY_MAP PCB_GENERATOR::GetProperties() const
{
    STRING_ANY_MAP props( pcbIUScale.IU_PER_MM );

#ifdef GENERATOR_ORDER
    props.set( "update_order", m_updateOrder );
#endif

    props.set( "origin", m_origin );

    return props;
}


void PCB_GENERATOR::SetProperties( const STRING_ANY_MAP& aProps )
{
#ifdef GENERATOR_ORDER
    aProps.get_to( "update_order", m_updateOrder );
#endif

    aProps.get_to( "origin", m_origin );
}


std::vector<std::pair<wxString, wxVariant>> PCB_GENERATOR::GetRowData()
{
#ifdef GENERATOR_ORDER
    return { { _HKI( "Update Order" ), wxString::FromCDouble( GetUpdateOrder() ) } };
#else
    return { {} };
#endif
}


wxString PCB_GENERATOR::GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const
{
    return wxString( _( "Generator" ) );
}


wxString PCB_GENERATOR::GetClass() const
{
    return wxS( "PCB_GENERATOR" );
}


bool PCB_GENERATOR::ClassOf( const EDA_ITEM* aItem )
{
    return aItem && PCB_GENERATOR_T == aItem->Type();
}


#ifdef GENERATOR_ORDER
static struct PCB_GENERATOR_DESC
{
    PCB_GENERATOR_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_GENERATOR );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_GENERATOR, BOARD_ITEM> );
        propMgr.InheritsAfter( TYPE_HASH( PCB_GENERATOR ), TYPE_HASH( BOARD_ITEM ) );

        const wxString groupTab = _HKI( "Generator Properties" );

        propMgr.AddProperty( new PROPERTY<PCB_GENERATOR, int>( _HKI( "Update Order" ),
                                                               &PCB_GENERATOR::SetUpdateOrder,
                                                               &PCB_GENERATOR::GetUpdateOrder ),
                             groupTab );
    }
} _PCB_GENERATOR_DESC;
#endif
