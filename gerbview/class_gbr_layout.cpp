/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2014 Jean-Pierre Charras  jp.charras at wanadoo.fr
 * Copyright (C) 1992-2014 KiCad Developers, see change_log.txt for contributors.
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

/**
 * @file class_gbr_layout.cpp
 * @brief  GBR_LAYOUT class functions.
 */

#include <limits.h>
#include <algorithm>

#include <fctsys.h>
#include <common.h>
#include <class_gbr_layout.h>

GBR_LAYOUT::GBR_LAYOUT()
{
    PAGE_INFO pageInfo( wxT( "GERBER" ) );
    SetPageSettings( pageInfo );

    m_printLayersMask.set();
}


GBR_LAYOUT::~GBR_LAYOUT()
{
}


EDA_RECT GBR_LAYOUT::ComputeBoundingBox()
{
    EDA_RECT bbox;

    for( GERBER_DRAW_ITEM* gerb_item = m_Drawings; gerb_item; gerb_item = gerb_item->Next() )
        bbox.Merge( gerb_item->GetBoundingBox() );

    SetBoundingBox( bbox );
    return bbox;
}
