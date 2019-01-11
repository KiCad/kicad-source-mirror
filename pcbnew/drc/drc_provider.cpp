/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <drc/drc_provider.h>


DRC_PROVIDER::DRC_PROVIDER( const DRC_MARKER_FACTORY& aMarkerMaker, MARKER_HANDLER aMarkerHandler )
        : m_marker_factory( aMarkerMaker ), m_marker_handler( aMarkerHandler )
{
}


const DRC_MARKER_FACTORY& DRC_PROVIDER::GetMarkerFactory() const
{
    return m_marker_factory;
}


void DRC_PROVIDER::HandleMarker( std::unique_ptr<MARKER_PCB> aMarker ) const
{
    // The marker hander currently takes a raw pointer,
    // but it also assumes ownership
    m_marker_handler( aMarker.release() );
}
