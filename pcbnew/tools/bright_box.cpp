/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include "bright_box.h"
#include <gal/graphics_abstraction_layer.h>
#include <class_track.h>

using namespace KIGFX;

const double BRIGHT_BOX::LINE_WIDTH = 100000.0;
const COLOR4D BRIGHT_BOX::BOX_COLOR = KIGFX::COLOR4D( 0.0, 1.0, 0.0, 1.0 );

BRIGHT_BOX::BRIGHT_BOX() :
    EDA_ITEM( NOT_USED ),    // this item is never added to a BOARD so it needs no type
    m_item( nullptr )
{
}


void BRIGHT_BOX::ViewDraw( int aLayer, KIGFX::VIEW* aView ) const
{
    if( !m_item )
        return;

    auto gal = aView->GetGAL();

    gal->SetIsStroke( true );
    gal->SetIsFill( false );
    gal->SetLineWidth( LINE_WIDTH );
    gal->SetStrokeColor( BOX_COLOR );


    if( m_item->Type() == PCB_TRACE_T )
    {
        const TRACK* track = static_cast<const TRACK*>( m_item );

        gal->DrawSegment( track->GetStart(), track->GetEnd(), track->GetWidth() );
    }
    else
    {
        BOX2I box = m_item->ViewBBox();

        gal->DrawRectangle( box.GetOrigin(), box.GetOrigin() + box.GetSize() );
    }
}


void BRIGHT_BOX::SetItem( BOARD_ITEM* aItem )
{
    m_item = aItem;
}
