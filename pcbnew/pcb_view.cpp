/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2023 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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


#include <functional>
using namespace std::placeholders;

#include <pcb_view.h>
#include <pcb_display_options.h>
#include <pcb_painter.h>
#include <footprint.h>

namespace KIGFX {
PCB_VIEW::PCB_VIEW( bool aIsDynamic ) :
    VIEW( aIsDynamic )
{
    // Set m_boundary to define the max area size. The default value is acceptable for Pcbnew
    // and Gerbview.
    // However, ensure this area has the right size (max size allowed by integer coordinates) in
    // case of the default value is changed. Could be a size depending on the drawing-sheet size.
    typedef std::numeric_limits<int> coord_limits;
    double pos = coord_limits::lowest() + coord_limits::epsilon();
    double size = static_cast<double>( coord_limits::max() - coord_limits::epsilon() )
                  - static_cast<double>( coord_limits::min() + coord_limits::epsilon() );
    m_boundary.SetOrigin( pos, pos );
    m_boundary.SetSize( size, size );
}


PCB_VIEW::~PCB_VIEW()
{
}


void PCB_VIEW::Add( KIGFX::VIEW_ITEM* aItem, int aDrawPriority )
{
    if( FOOTPRINT* footprint = dynamic_cast<FOOTPRINT*>( aItem ) )
        footprint->RunOnChildren( std::bind( &PCB_VIEW::Add, this, _1, aDrawPriority ) );

    VIEW::Add( aItem, aDrawPriority );
}


void PCB_VIEW::Remove( KIGFX::VIEW_ITEM* aItem )
{
    if( FOOTPRINT* footprint = dynamic_cast<FOOTPRINT*>( aItem ) )
        footprint->RunOnChildren( std::bind( &PCB_VIEW::Remove, this, _1 ) );

    VIEW::Remove( aItem );
}


void PCB_VIEW::Update( const KIGFX::VIEW_ITEM* aItem, int aUpdateFlags ) const
{
    if( const BOARD_ITEM* boardItem = dynamic_cast<const BOARD_ITEM*>( aItem ) )
    {
        boardItem->RunOnChildren(
                [this, aUpdateFlags]( BOARD_ITEM* child )
                {
                    VIEW::Update( child, aUpdateFlags );
                } );
    }

    VIEW::Update( aItem, aUpdateFlags );
}


void PCB_VIEW::Update( const KIGFX::VIEW_ITEM* aItem ) const
{
    PCB_VIEW::Update( aItem, KIGFX::ALL );
}


void PCB_VIEW::UpdateDisplayOptions( const PCB_DISPLAY_OPTIONS& aOptions )
{
    KIGFX::PCB_PAINTER*         painter  = static_cast<KIGFX::PCB_PAINTER*>( GetPainter() );
    KIGFX::PCB_RENDER_SETTINGS* settings = painter->GetSettings();

    settings->LoadDisplayOptions( aOptions );
}
}
