/**
 * @file class_board_item.cpp
 * @brief Class BOARD_ITEM definition and  some basic functions.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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


void BOARD_ITEM::UnLink()
{
    DLIST<BOARD_ITEM>* list = (DLIST<BOARD_ITEM>*) GetList();
    wxASSERT( list );

    if( list )
        list->Remove( this );
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


void BOARD_ITEM::ViewGetLayers( int aLayers[], int& aCount ) const
{
    // Basic fallback
    aCount = 1;
    aLayers[0] = m_Layer;
}


int BOARD_ITEM::getTrailingInt( const wxString& aStr )
{
    int number = 0;
    int base = 1;

    // Trim and extract the trailing numeric part
    int index = aStr.Len() - 1;
    while( index >= 0 )
    {
        const char chr = aStr.GetChar( index );

        if( chr < '0' || chr > '9' )
            break;

        number += ( chr - '0' ) * base;
        base *= 10;
        index--;
    }

    return number;
}


int BOARD_ITEM::getNextNumberInSequence( const std::set<int>& aSeq, bool aFillSequenceGaps)
{
    if( aSeq.empty() )
        return 1;

    // By default go to the end of the sequence
    int candidate = *aSeq.rbegin();

    // Filling in gaps in pad numbering
    if( aFillSequenceGaps )
    {
        // start at the beginning
        candidate = *aSeq.begin();

        for( auto it : aSeq )
        {
            if( it - candidate > 1 )
                break;

            candidate = it;
        }
    }

    ++candidate;

    return candidate;
}


void BOARD_ITEM::DeleteStructure()
{
    auto parent = GetParent();

    if( parent && GetList() )
        parent->Remove( this );

    delete this;
}


void BOARD_ITEM::SwapData( BOARD_ITEM* aImage )
{

}

void BOARD_ITEM::TransformShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                           int aClearanceValue,
                                           int aCircleToSegmentsCount,
                                           double aCorrectionFactor,
                                           bool ignoreLineWidth ) const
{
    wxASSERT_MSG( false, "Called TransformShapeWithClearanceToPolygon() on unsupported BOARD_ITEM." );
};
