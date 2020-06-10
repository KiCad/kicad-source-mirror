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
