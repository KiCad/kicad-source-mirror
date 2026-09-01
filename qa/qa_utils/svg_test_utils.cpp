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

#include "qa_utils/svg_test_utils.h"

#include <wx/mstream.h>
#include <wx/xml/xml.h>

#include <fast_float/fast_float.h>


std::vector<double> KI_TEST::ParseViewBox( const wxString& aSvg )
{
    const wxCharBuffer  utf8 = aSvg.ToUTF8();
    wxMemoryInputStream input( utf8.data(), utf8.length() );
    wxXmlDocument       doc;

    if( !doc.Load( input ) )
        return {};

    const wxXmlNode* root = doc.GetRoot();

    if( !root || root->GetName() != wxT( "svg" ) )
        return {};

    wxString viewBox;

    if( !root->GetAttribute( wxT( "viewBox" ), &viewBox ) )
        return {};

    // The viewBox is four whitespace-separated numbers: min-x, min-y, width, height.
    const std::string   values = viewBox.ToStdString();
    std::vector<double> parsed;
    size_t              pos = 0;

    while( pos < values.size() )
    {
        while( pos < values.size() && ( values[pos] == ' ' || values[pos] == '\t' ) )
            pos++;

        if( pos >= values.size() )
            break;

        double val = 0.0;

        const auto [ptr, ec] = fast_float::from_chars( values.data() + pos, values.data() + values.size(), val );

        if( ec != std::errc() )
            return {};

        parsed.push_back( val );
        pos = ptr - values.data();
    }

    // We expect exactly four numbers. Anything else is a parse failure.
    if( parsed.size() != 4 )
        return {};

    return parsed;
}
