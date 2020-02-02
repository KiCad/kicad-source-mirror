/**
 * @file class_board_item.cpp
 * @brief Class BOARD_ITEM definition and  some basic functions.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <common.h>
#include <pcbnew.h>
#include <wx/debug.h>

#include <class_board.h>
#include <string>

wxString BOARD_ITEM::ShowShape( STROKE_T aShape )
{
    switch( aShape )
    {
    case S_SEGMENT:         return _( "Line" );
    case S_RECT:            return _( "Rect" );
    case S_ARC:             return _( "Arc" );
    case S_CIRCLE:          return _( "Circle" );
    case S_CURVE:           return _( "Bezier Curve" );
    case S_POLYGON:         return _( "Polygon" );
    default:                return wxT( "??" );
    }
}


BOARD* BOARD_ITEM::GetBoard() const
{
    if( Type() == PCB_T )
        return (BOARD*) this;

    BOARD_ITEM* parent = GetParent();

    if( parent )
        return parent->GetBoard();

    return NULL;
}


wxString BOARD_ITEM::GetLayerName() const
{
    BOARD*  board = GetBoard();

    if( board )
        return board->GetLayerName( m_Layer );

    // If no parent, return standard name
    return BOARD::GetStandardLayerName( m_Layer );
}


wxString BOARD_ITEM::LayerMaskDescribe( const BOARD* aBoard, LSET aMask )
{
    // Try to be smart and useful.  Check all copper first.
    if( aMask[F_Cu] && aMask[B_Cu] )
        return _( "All copper layers" );

    // Check for copper.
    auto layer = aBoard->GetEnabledLayers().AllCuMask() & aMask;

    for( int i = 0; i < 3; i++ )
    {
        for( int bit = PCBNEW_LAYER_ID_START; bit < PCB_LAYER_ID_COUNT; ++bit )
        {
            if( layer[ bit ] )
            {
                wxString layerInfo = aBoard->GetLayerName( static_cast<PCB_LAYER_ID>( bit ) );

                if( aMask.count() > 1 )
                    layerInfo << _( " and others" );

                return layerInfo;
            }
        }

        // No copper; first, check for technicals and then for all layers.
        if( i < 1 )     // first, check for technicals
            layer = aBoard->GetEnabledLayers().AllTechMask() & aMask;
        else            // then check for all other layers
            layer = aMask;
    }

    // No copper, no technicals: no layer
    return _( "no layers" );
}


void BOARD_ITEM::ViewGetLayers( int aLayers[], int& aCount ) const
{
    // Basic fallback
    aCount = 1;
    aLayers[0] = m_Layer;
}


void BOARD_ITEM::DeleteStructure()
{
    auto parent = GetParent();

    if( parent )
        parent->Remove( this );

    delete this;
}


void BOARD_ITEM::SwapData( BOARD_ITEM* aImage )
{
}


void BOARD_ITEM::TransformShapeWithClearanceToPolygon(
        SHAPE_POLY_SET& aCornerBuffer, int aClearanceValue, int aError, bool ignoreLineWidth ) const
{
    wxASSERT_MSG( false, "Called TransformShapeWithClearanceToPolygon() on unsupported BOARD_ITEM." );
};


static struct BOARD_ITEM_DESC
{
    BOARD_ITEM_DESC()
    {
        ENUM_MAP<PCB_LAYER_ID>::Instance()
                .Map( F_Cu, LSET::Name( F_Cu ) )
                .Map( In1_Cu, LSET::Name( In1_Cu ) )
                .Map( In2_Cu, LSET::Name( In2_Cu ) )
                .Map( In3_Cu, LSET::Name( In3_Cu ) )
                .Map( In4_Cu, LSET::Name( In4_Cu ) )
                .Map( In5_Cu, LSET::Name( In5_Cu ) )
                .Map( In6_Cu, LSET::Name( In6_Cu ) )
                .Map( In7_Cu, LSET::Name( In7_Cu ) )
                .Map( In8_Cu, LSET::Name( In8_Cu ) )
                .Map( In9_Cu, LSET::Name( In9_Cu ) )
                .Map( In10_Cu, LSET::Name( In10_Cu ) )
                .Map( In11_Cu, LSET::Name( In11_Cu ) )
                .Map( In12_Cu, LSET::Name( In12_Cu ) )
                .Map( In13_Cu, LSET::Name( In13_Cu ) )
                .Map( In14_Cu, LSET::Name( In14_Cu ) )
                .Map( In15_Cu, LSET::Name( In15_Cu ) )
                .Map( In16_Cu, LSET::Name( In16_Cu ) )
                .Map( In17_Cu, LSET::Name( In17_Cu ) )
                .Map( In18_Cu, LSET::Name( In18_Cu ) )
                .Map( In19_Cu, LSET::Name( In19_Cu ) )
                .Map( In20_Cu, LSET::Name( In20_Cu ) )
                .Map( In21_Cu, LSET::Name( In21_Cu ) )
                .Map( In22_Cu, LSET::Name( In22_Cu ) )
                .Map( In23_Cu, LSET::Name( In23_Cu ) )
                .Map( In24_Cu, LSET::Name( In24_Cu ) )
                .Map( In25_Cu, LSET::Name( In25_Cu ) )
                .Map( In26_Cu, LSET::Name( In26_Cu ) )
                .Map( In27_Cu, LSET::Name( In27_Cu ) )
                .Map( In28_Cu, LSET::Name( In28_Cu ) )
                .Map( In29_Cu, LSET::Name( In29_Cu ) )
                .Map( In30_Cu, LSET::Name( In30_Cu ) )
                .Map( B_Cu, LSET::Name( B_Cu ) )
                .Map( B_Adhes, LSET::Name( B_Adhes ) )
                .Map( F_Adhes, LSET::Name( F_Adhes ) )
                .Map( B_Paste, LSET::Name( B_Paste ) )
                .Map( F_Paste, LSET::Name( F_Paste ) )
                .Map( B_SilkS, LSET::Name( B_SilkS ) )
                .Map( F_SilkS, LSET::Name( F_SilkS ) )
                .Map( B_Mask, LSET::Name( B_Mask ) )
                .Map( F_Mask, LSET::Name( F_Mask ) )
                .Map( Dwgs_User, LSET::Name( Dwgs_User ) )
                .Map( Cmts_User, LSET::Name( Cmts_User ) )
                .Map( Eco1_User, LSET::Name( Eco1_User ) )
                .Map( Eco2_User, LSET::Name( Eco2_User ) )
                .Map( Edge_Cuts, LSET::Name( Edge_Cuts ) )
                .Map( Margin, LSET::Name( Margin ) )
                .Map( B_CrtYd, LSET::Name( B_CrtYd ) )
                .Map( F_CrtYd, LSET::Name( F_CrtYd ) )
                .Map( B_Fab, LSET::Name( B_Fab ) )
                .Map( F_Fab, LSET::Name( F_Fab ) );

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( BOARD_ITEM );
        propMgr.InheritsAfter( TYPE_HASH( BOARD_ITEM ), TYPE_HASH( EDA_ITEM ) );

        propMgr.AddProperty( new PROPERTY<BOARD_ITEM, int>( _( "Position X" ),
                    &BOARD_ITEM::SetX, &BOARD_ITEM::GetX, PROPERTY_DISPLAY::DISTANCE ) );
        propMgr.AddProperty( new PROPERTY<BOARD_ITEM, int>( _( "Position Y" ),
                    &BOARD_ITEM::SetY, &BOARD_ITEM::GetY, PROPERTY_DISPLAY::DISTANCE ) );
        propMgr.AddProperty( new PROPERTY_ENUM<BOARD_ITEM, PCB_LAYER_ID>( _( "Layer" ),
                    &BOARD_ITEM::SetLayer, &BOARD_ITEM::GetLayer ) );
        propMgr.AddProperty( new PROPERTY<BOARD_ITEM, bool>( _( "Locked" ),
                    &BOARD_ITEM::SetLocked, &BOARD_ITEM::IsLocked ) );
    }
} _BOARD_ITEM_DESC;

IMPLEMENT_ENUM_TO_WXANY( PCB_LAYER_ID )
