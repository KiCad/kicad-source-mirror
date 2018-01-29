/**
 * @file class_pl_editor_layout.cpp
 * @brief  PL_EDITOR_LAYOUT class functions.
 */
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
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

#include <limits.h>
#include <algorithm>

#include <fctsys.h>
#include <common.h>
#include <class_eda_rect.h>
#include <class_pl_editor_layout.h>

PL_EDITOR_LAYOUT::PL_EDITOR_LAYOUT()
{
    PAGE_INFO pageInfo( wxT( "A4" ) );
    SetPageSettings( pageInfo );
}


PL_EDITOR_LAYOUT::~PL_EDITOR_LAYOUT()
{
}

EDA_RECT PL_EDITOR_LAYOUT::ComputeBoundingBox()
{
    EDA_RECT bbox;

    SetBoundingBox( bbox );
    return bbox;
}
