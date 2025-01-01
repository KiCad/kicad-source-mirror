/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007, 2008 Lubo Racko <developer@lura.sk>
 * Copyright (C) 2007, 2008, 2012 Alexander Lunev <al.lunev@yahoo.com>
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

class PCAD_PCB_COMPONENT;
class PCAD_NET;
class PCAD_NET_NODE;
class PCAD_PAD_SHAPE;

WX_DEFINE_ARRAY( PCAD_PCB_COMPONENT*, PCAD_COMPONENTS_ARRAY );
WX_DEFINE_ARRAY( PCAD_NET*, PCAD_NETS_ARRAY );
WX_DEFINE_ARRAY( PCAD_NET_NODE*, PCAD_NET_NODES_ARRAY );
WX_DEFINE_ARRAY( PCAD_PAD_SHAPE*, PCAD_PAD_SHAPES_ARRAY );
}

#endif
