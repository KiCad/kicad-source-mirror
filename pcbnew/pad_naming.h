/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PAD_NAMING_H
#define PAD_NAMING_H

#include <class_pad.h>

/**
 * The PAD_NAMING namespace contains helper functions for common operations
 * to do with naming of #D_PAD objects.
 */
namespace PAD_NAMING
{

/**
 * Check if a pad should be named.
 *
 * For example, NPTH or paste apertures normally do not have names, as they
 * cannot be assigned to a netlist.
 *
 * @param  aPad the pad to check
 * @return      true if the pad gets a name
 */
bool PadCanHaveName( const D_PAD& aPad );

} // namespace PAD_NAMING

#endif // PAD_NAMING_H