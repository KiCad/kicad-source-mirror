/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2014 Kicad Developers, see change_log.txt for contributors.
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

#ifndef BITMAP2COMPONENT_H
#define BITMAP2COMPONENT_H

// for consistency this enum should conform to the
// indices in m_radioBoxFormat from bitmap2cmp_gui.cpp
enum OUTPUT_FMT_ID
{
    EESCHEMA_FMT = 0,
    PCBNEW_KICAD_MOD,
    POSTSCRIPT_FMT,
    KICAD_LOGO,
    FINAL_FMT = KICAD_LOGO
};

#endif  // BITMAP2COMPONENT_H
