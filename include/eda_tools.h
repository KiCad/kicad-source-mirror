/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2021 KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2018 CERN
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
 * @file eda_tools.h
 * @brief Enums and utilities for different EDA tools.
 */


#ifndef EDA_TOOLS_H
#define EDA_TOOLS_H

#include <wx/filename.h>

/**
 * Enumeration of tools
 */
enum class EDA_TOOLS
{
    EAGLE
};

/**
 * Check if aFileName is a aTool file
 * As an example, can be used to check if a .sch file is an EAGLE file
 * (may be a legacy KICAD file)
 * @param aFileName name of file to check. Must be given with full path
 * @param aTool EDA tool
 * @return true if the file is an EDA_TOOL file type, false if not or file does not exist
 */
bool IsFileFromEDATool( const wxFileName& aFileName, const EDA_TOOLS aTool );

#endif
