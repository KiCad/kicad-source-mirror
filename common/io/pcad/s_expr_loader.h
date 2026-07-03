/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Alexander Lunev <al.lunev@yahoo.com>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file s_expr_loader.h
 */

#ifndef S_EXPR_LOADER_H_
#define S_EXPR_LOADER_H_

#include <string>

class wxString;
class wxXmlDocument;

namespace PCAD2KICAD
{
    void LoadInputFile( const wxString& aFileName, wxXmlDocument* aXmlDoc );

    /**
     * Check that a file is ACCEL_ASCII and, when @a aSection is not empty,
     * that it contains the given section token (e.g. "(pcbDesign" or
     * "(schematicDesign").  P-CAD ASCII saves are extension-agnostic, so
     * format detection is by content.
     */
    bool FileMatchesFormat( const wxString& aFileName, const std::string& aSection );
}

#endif    // S_EXPR_LOADER_H_
