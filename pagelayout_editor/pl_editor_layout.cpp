/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#include <eda_draw_frame.h>
#include "pl_editor_layout.h"

PL_EDITOR_LAYOUT::PL_EDITOR_LAYOUT() :
    m_drawItemList( drawSheetIUScale )
{
    PAGE_INFO pageInfo( PAGE_SIZE_TYPE::A4 );
    SetPageSettings( pageInfo );
}


PL_EDITOR_LAYOUT::~PL_EDITOR_LAYOUT()
{
}


BOX2I PL_EDITOR_LAYOUT::ComputeBoundingBox()
{
    BOX2I bbox;

    SetBoundingBox( bbox );
    return bbox;
}
