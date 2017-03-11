/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 KiCad Developers, see change_log.txt for contributors.
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

#include "pcb_bright_box.h"
#include <gal/graphics_abstraction_layer.h>
#include <class_track.h>

using namespace KIGFX;

const double PCB_BRIGHT_BOX::PCB_LINE_WIDTH = 100000.0;


PCB_BRIGHT_BOX::PCB_BRIGHT_BOX() :
    BRIGHT_BOX()
{
    SetLineWidth( PCB_LINE_WIDTH );
}


void PCB_BRIGHT_BOX::ViewDraw( int aLayer, KIGFX::VIEW* aView ) const
{
    if( !m_item )
        return;

    if( m_item->Type() == PCB_TRACE_T )
    {
        const TRACK* track = static_cast<const TRACK*>( m_item );

        auto gal = aView->GetGAL();

        gal->SetIsStroke( true );
        gal->SetIsFill( false );
        gal->SetLineWidth( m_lineWidth );
        gal->SetStrokeColor( m_color );

        gal->DrawSegment( track->GetStart(), track->GetEnd(), track->GetWidth() );
    }
    else
    {
        BRIGHT_BOX::ViewDraw( aLayer, aView );
    }
}
