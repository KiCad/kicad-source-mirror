/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski
 * Copyright (C) 2021 Kicad Developers, see AUTHORS.txt for contributors.
 *
 * Font abstract base class
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

#ifndef FONT_H_
#define FONT_H_

#include <iostream>
#include <map>
#include <algorithm>
#include <wx/string.h>

#include <utf8.h>
#include <font/text_attributes.h>

namespace KIGFX
{
class GAL;
}


enum TEXT_STYLE
{
    BOLD = 1,
    ITALIC = 1 << 1,
    SUBSCRIPT = 1 << 2,
    SUPERSCRIPT = 1 << 3,
    OVERBAR = 1 << 4
};


using TEXT_STYLE_FLAGS = unsigned int;


inline bool IsBold( TEXT_STYLE_FLAGS aFlags )
{
    return aFlags & TEXT_STYLE::BOLD;
}


inline bool IsItalic( TEXT_STYLE_FLAGS aFlags )
{
    return aFlags & TEXT_STYLE::ITALIC;
}


inline bool IsSuperscript( TEXT_STYLE_FLAGS aFlags )
{
    return aFlags & TEXT_STYLE::SUPERSCRIPT;
}


inline bool IsSubscript( TEXT_STYLE_FLAGS aFlags )
{
    return aFlags & TEXT_STYLE::SUBSCRIPT;
}


inline bool IsOverbar( TEXT_STYLE_FLAGS aFlags )
{
    return aFlags & TEXT_STYLE::OVERBAR;
}


std::string TextStyleAsString( TEXT_STYLE_FLAGS aFlags );


namespace KIFONT
{
/**
 * FONT is an abstract base class for both outline and stroke fonts
 */
class FONT
{
public:
    explicit FONT();

    virtual ~FONT()
    { }

    virtual bool IsStroke() const { return false; }
    virtual bool IsOutline() const { return false; }
    virtual bool IsBold() const { return false; }
    virtual bool IsItalic() const { return false; }

    const wxString&    Name() const;
    inline const char* NameAsToken() const { return Name().utf8_str().data(); }

protected:
    wxString                         m_fontName;         ///< Font name
    wxString                         m_fontFileName;     ///< Font file name

private:
    static FONT*                     s_defaultFont;
    static std::map<wxString, FONT*> s_fontMap;
};
} //namespace KIFONT


inline std::ostream& operator<<(std::ostream& os, const KIFONT::FONT& aFont)
{
    os << "[Font \"" << aFont.Name() << "\"" << ( aFont.IsStroke() ? " stroke" : "" )
       << ( aFont.IsOutline() ? " outline" : "" ) << ( aFont.IsBold() ? " bold" : "" )
       << ( aFont.IsItalic() ? " italic" : "" ) << "]";
    return os;
}


inline std::ostream& operator<<(std::ostream& os, const KIFONT::FONT* aFont)
{
    os << *aFont;
    return os;
}

#endif // FONT_H_
