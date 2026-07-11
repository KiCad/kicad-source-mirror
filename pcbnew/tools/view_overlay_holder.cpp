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

#include <tools/view_overlay_holder.h>

#include <view/view.h>
#include <view/view_overlay.h>


VIEW_OVERLAY_HOLDER::VIEW_OVERLAY_HOLDER( KIGFX::VIEW* aView ) :
        m_view( aView ),
        m_overlay( aView->MakeOverlay() )
{
}


VIEW_OVERLAY_HOLDER::~VIEW_OVERLAY_HOLDER()
{
    if( m_overlay )
        m_view->Remove( m_overlay.get() );
}
