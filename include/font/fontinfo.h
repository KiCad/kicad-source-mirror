/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef FONT_FONTINFO_H
#define FONT_FONTINFO_H

#include <string>
#include <vector>

namespace fontconfig
{

class FONTINFO
{
public:
    FONTINFO( std::string aFile, std::string aStyle, std::string aFamily ) :
            m_file( std::move( aFile ) ),
            m_style( std::move( aStyle ) ),
            m_family( std::move( aFamily ) )
    {
    }

    const std::string& File() const   { return m_file; }
    const std::string& Style() const  { return m_style; }
    const std::string& Family() const { return m_family; }

    std::vector<FONTINFO>& Children() { return m_children; }

private:
    std::string           m_file;
    std::string           m_style;
    std::string           m_family;

    std::vector<FONTINFO> m_children;
};

} // namespace fontconfig

#endif //FONT_FONTINFO_H
