/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007, 2008 Lubo Racko <developer@lura.sk>
 * Copyright (C) 2007, 2008, 2012 Alexander Lunev <al.lunev@yahoo.com>
 * Copyright (C) 2012-2020 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#ifndef PCAD_ITEM_TYPES_H_
#define PCAD_ITEM_TYPES_H_

#include <wx/dynarray.h>

class wxRealPoint;

WX_DEFINE_ARRAY( wxRealPoint*, VERTICES_ARRAY );
WX_DEFINE_ARRAY( VERTICES_ARRAY*, ISLANDS_ARRAY );

namespace PCAD2KICAD {

class PCB_COMPONENT;
class PCB_NET;
class PCB_NET_NODE;
class PCB_PAD_SHAPE;

WX_DEFINE_ARRAY( PCB_COMPONENT*, PCB_COMPONENTS_ARRAY );
WX_DEFINE_ARRAY( PCB_NET*, PCB_NETS_ARRAY );
WX_DEFINE_ARRAY( PCB_NET_NODE*, PCB_NET_NODES_ARRAY );
WX_DEFINE_ARRAY( PCB_PAD_SHAPE*, PCB_PAD_SHAPES_ARRAY );
}

#endif
