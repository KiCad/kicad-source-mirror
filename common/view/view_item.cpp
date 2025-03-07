/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2016 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include <view/view_item.h>
#include <view/view.h>
#include <gal/painter.h>

using namespace KIGFX;

VIEW_ITEM::~VIEW_ITEM()
{
    VIEW::OnDestroy( this );
    m_viewPrivData = nullptr;
}


double VIEW_ITEM::lodScaleForThreshold( const VIEW* aView, int aWhatIu, int aThresholdIu )
{
    if( aView->GetPainter()->GetSettings()->IsPrinting() )
        return LOD_SHOW;

    if( aWhatIu == 0 )
        return LOD_HIDE;

    return double( aThresholdIu ) / aWhatIu;
}
