/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <tl/expected.hpp>

#include <wx/string.h>
#include <wx/xml/xml.h>

namespace KI_TEST
{
struct SVG_VIEWBOX
{
    double m_X;
    double m_Y;
    double m_Width;
    double m_Height;
};


tl::expected<wxXmlDocument, wxString> LoadSvg( const wxString& aSvg );

/**
 * Parse the four numbers of the SVG viewBox attribute (min-x, min-y, width, height).
 * Returns an error if the attribute is missing or cannot be parsed.
 */
tl::expected<SVG_VIEWBOX, wxString> ParseViewBox( const wxXmlNode& aRoot );

/**
 * Find the first <rect> element in the given XML node's descendants.
 * Returns nullptr if no <rect> element is found.
 */
const wxXmlNode* FindFirstRect( const wxXmlNode& aNode );

} // namespace KI_TEST