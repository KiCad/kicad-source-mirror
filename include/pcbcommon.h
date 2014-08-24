/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2014 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file pcbcommon.h
 */

#ifndef PCBCOMMON_H_
#define PCBCOMMON_H_

class PGM_BASE;

/**
 * attempts to set (when not set or valid) the environment variable given by aKiSys3Dmod
 * (typically "KISYS3DMOD" ) to a valid path.
 * If the environment variable is already set, then it left as is to respect
 * the wishes of the user.
 *
 * The path is determined by attempting to find the path modules/packages3d
 * files in kicad tree.
 * This may or may not be the best path but it provides the best solution for
 * backwards compatibility with the previous 3D shapes search path implementation.
 *
 * @note This must be called after #SetBinDir() is called at least on Windows.
 * Otherwise, the kicad path is not known (Windows specific)
 *
 * @param aKiSys3Dmod = the value of environment variable, typically "KISYS3DMOD"
 * @param aProcess = the current process
 * @return false if the aKiSys3Dmod path is not valid.
 */
bool Set3DShapesDefaultPath( const wxString& aKiSys3Dmod, const PGM_BASE* aProcess );


/// Utility for comma separated lists
inline void AccumulateDescription( wxString &aDesc, const wxString &aItem )
{
    if( !aDesc.IsEmpty() )
        aDesc << wxT(", ");
    aDesc << aItem;
}

#endif  // PCBCOMMON_H_
