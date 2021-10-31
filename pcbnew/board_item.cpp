/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pybind11/pybind11.h>

#include <wx/debug.h>
#include <wx/msgdlg.h>
#include <i18n_utility.h>
#include <macros.h>
#include <board.h>
#include <pcb_group.h>


const BOARD* BOARD_ITEM::GetBoard() const
{
    if( Type() == PCB_T )
        return static_cast<const BOARD*>( this );

    BOARD_ITEM* parent = GetParent();

    if( parent )
        return parent->GetBoard();

    return nullptr;
}


BOARD* BOARD_ITEM::GetBoard()
{
    if( Type() == PCB_T )
        return static_cast<BOARD*>( this );

    BOARD_ITEM* parent = GetParent();

    if( parent )
        return parent->GetBoard();

    return nullptr;
}


bool BOARD_ITEM::IsLocked() const
{
    if( GetParentGroup() )
        return GetParentGroup()->IsLocked();

    const BOARD* board = GetBoard();

    return board && board->GetBoardUse() != BOARD_USE::FPHOLDER && GetState( LOCKED );
}


wxString BOARD_ITEM::GetLayerName() const
{
    const BOARD* board = GetBoard();

    if( board )
        return board->GetLayerName( m_layer );

    // If no parent, return standard name
    return BOARD::GetStandardLayerName( m_layer );
}


wxString BOARD_ITEM::layerMaskDescribe() const
{
    const BOARD* board = GetBoard();
    LSET         layers = GetLayerSet();

    // Try to be smart and useful.  Check all copper first.
    if( layers[F_Cu] && layers[B_Cu] )
        return _( "all copper layers" );

    LSET copperLayers = layers & board->GetEnabledLayers().AllCuMask();
    LSET techLayers = layers & board->GetEnabledLayers().AllTechMask();

    for( LSET testLayers : { copperLayers, techLayers, layers } )
    {
        for( int bit = PCBNEW_LAYER_ID_START; bit < PCB_LAYER_ID_COUNT; ++bit )
        {
            if( testLayers[ bit ] )
            {
                wxString layerInfo = board->GetLayerName( static_cast<PCB_LAYER_ID>( bit ) );

                if( testLayers.count() > 1 )
                    layerInfo << wxS( " " ) + _( "and others" );

                return layerInfo;
            }
        }
    }

    // No copper, no technicals: no layer
    return _( "no layers" );
}


void BOARD_ITEM::ViewGetLayers( int aLayers[], int& aCount ) const
{
    // Basic fallback
    aCount = 1;
    aLayers[0] = m_layer;
}


void BOARD_ITEM::DeleteStructure()
{
    BOARD_ITEM_CONTAINER* parent = GetParent();

    if( parent )
        parent->Remove( this );

    delete this;
}


void BOARD_ITEM::SwapData( BOARD_ITEM* aImage )
{
}


void BOARD_ITEM::TransformShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                                       PCB_LAYER_ID aLayer, int aClearanceValue,
                                                       int aError, ERROR_LOC aErrorLoc,
                                                       bool ignoreLineWidth ) const
{
    wxASSERT_MSG( false,
                  "Called TransformShapeWithClearanceToPolygon() on unsupported BOARD_ITEM." );
};


bool BOARD_ITEM::ptr_cmp::operator() ( const BOARD_ITEM* a, const BOARD_ITEM* b ) const
{
    if( a->Type() != b->Type() )
        return a->Type() < b->Type();

    if( a->GetLayer() != b->GetLayer() )
        return a->GetLayer() < b->GetLayer();

    if( a->m_Uuid != b->m_Uuid )    // should be always the case for valid boards
        return a->m_Uuid < b->m_Uuid;

    return a < b;
}


std::shared_ptr<SHAPE> BOARD_ITEM::GetEffectiveShape( PCB_LAYER_ID aLayer ) const
{
    std::shared_ptr<SHAPE> shape;

    UNIMPLEMENTED_FOR( GetClass() );

    return shape;
}


void BOARD_ITEM::Rotate( const wxPoint& aRotCentre, double aAngle )
{
    wxMessageBox( wxT( "virtual BOARD_ITEM::Rotate used, should not occur" ), GetClass() );
}


void BOARD_ITEM::Flip( const wxPoint& aCentre, bool aFlipLeftRight )
{
    wxMessageBox( wxT( "virtual BOARD_ITEM::Flip used, should not occur" ), GetClass() );
}


static struct BOARD_ITEM_DESC
{
    BOARD_ITEM_DESC()
    {
        ENUM_MAP<PCB_LAYER_ID>& layerEnum = ENUM_MAP<PCB_LAYER_ID>::Instance();

        if( layerEnum.Choices().GetCount() == 0 )
        {
            layerEnum.Undefined( UNDEFINED_LAYER );

            for( LSEQ seq = LSET::AllLayersMask().Seq(); seq; ++seq )
                layerEnum.Map( *seq, LSET::Name( *seq ) );
        }

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( BOARD_ITEM );
        propMgr.InheritsAfter( TYPE_HASH( BOARD_ITEM ), TYPE_HASH( EDA_ITEM ) );

        propMgr.AddProperty( new PROPERTY<BOARD_ITEM, int>( _HKI( "Position X" ),
                    &BOARD_ITEM::SetX, &BOARD_ITEM::GetX, PROPERTY_DISPLAY::DISTANCE ) );
        propMgr.AddProperty( new PROPERTY<BOARD_ITEM, int>( _HKI( "Position Y" ),
                    &BOARD_ITEM::SetY, &BOARD_ITEM::GetY, PROPERTY_DISPLAY::DISTANCE ) );
        propMgr.AddProperty( new PROPERTY_ENUM<BOARD_ITEM, PCB_LAYER_ID>( _HKI( "Layer" ),
                    &BOARD_ITEM::SetLayer, &BOARD_ITEM::GetLayer ) );
        propMgr.AddProperty( new PROPERTY<BOARD_ITEM, bool>( _HKI( "Locked" ),
                    &BOARD_ITEM::SetLocked, &BOARD_ITEM::IsLocked ) );
    }
} _BOARD_ITEM_DESC;

IMPLEMENT_ENUM_TO_WXANY( PCB_LAYER_ID )
