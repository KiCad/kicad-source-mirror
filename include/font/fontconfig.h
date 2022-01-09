/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski
 * Copyright (C) 2021-2022 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KICAD_FONTCONFIG_H
#define KICAD_FONTCONFIG_H

#include <fontconfig/fontconfig.h>
#include <wx/string.h>
#include <vector>
#include <map>
#include <font/fontinfo.h>

namespace fontconfig
{

class FONTCONFIG
{
public:
    FONTCONFIG();

    /**
     * Given a fully-qualified font name ("Times:Bold:Italic") find the closest matching font
     * and return its filepath in \a aFontFile.
     *
     * A return value of false indicates a serious error in the font system.
     */
    bool FindFont( const wxString& aFontName, wxString& aFontFile );

    /**
     * List the current available font families.
     */
    void ListFonts( std::vector<std::string>& aFonts );

private:
    std::map<std::string, FONTINFO> m_fonts;
};

} // namespace fontconfig


fontconfig::FONTCONFIG* Fontconfig();


#endif //KICAD_FONTCONFIG_H
