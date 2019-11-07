/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
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

#include <class_module.h>

namespace KIGFX {
PCB_VIEW::PCB_VIEW( bool aIsDynamic ) :
    VIEW( aIsDynamic )
{
    // Set m_boundary to define the max area size. The default value
    // is acceptable for Pcbnew and Gerbview.
    // However, ensure this area has the right size (max size allowed by integer coordinates)
    // in case of the default value is changed.
    // Could be a size depending on the worksheet size.
    typedef std::numeric_limits<int> coord_limits;
    double pos = coord_limits::lowest() / 2 + coord_limits::epsilon();
    double size = coord_limits::max() - coord_limits::epsilon();
    m_boundary.SetOrigin( pos, pos );
    m_boundary.SetSize( size, size );
}


PCB_VIEW::~PCB_VIEW()
{
}


void PCB_VIEW::Add( KIGFX::VIEW_ITEM* aItem, int aDrawPriority )
{
    auto item = static_cast<BOARD_ITEM*>( aItem );

    if( item->Type() == PCB_MODULE_T )
    {
        auto mod = static_cast<MODULE*>( item );
        mod->RunOnChildren([this] ( BOARD_ITEM* aModItem ) {
                VIEW::Add( aModItem );
            } );
    }

    VIEW::Add( item, aDrawPriority );
}


void PCB_VIEW::Remove( KIGFX::VIEW_ITEM* aItem )
{
    auto item = static_cast<BOARD_ITEM*>( aItem );


    if( item->Type() == PCB_MODULE_T )
    {
        auto mod = static_cast<MODULE*>( item );
        mod->RunOnChildren([this] ( BOARD_ITEM* aModItem ) {
                VIEW::Remove( aModItem );
            } );
    }

    VIEW::Remove( item );
}


void PCB_VIEW::Update( KIGFX::VIEW_ITEM* aItem, int aUpdateFlags )
{
    auto item = static_cast<BOARD_ITEM*>( aItem );

    if( item->Type() == PCB_MODULE_T )
    {
        auto mod = static_cast<MODULE*>( item );
        mod->RunOnChildren([this, aUpdateFlags] ( BOARD_ITEM* aModItem ) {
                VIEW::Update( aModItem, aUpdateFlags );
            } );
    }

    VIEW::Update( item, aUpdateFlags );
}


void PCB_VIEW::Update( KIGFX::VIEW_ITEM* aItem )
{
    PCB_VIEW::Update( aItem, KIGFX::ALL );
}


void PCB_VIEW::UpdateDisplayOptions( const PCB_DISPLAY_OPTIONS& aOptions )
{
    auto    painter     = static_cast<KIGFX::PCB_PAINTER*>( GetPainter() );
    auto    settings    = static_cast<KIGFX::PCB_RENDER_SETTINGS*>( painter->GetSettings() );

    settings->LoadDisplayOptions( aOptions, settings->GetShowPageLimits() );
}
}
