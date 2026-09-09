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

#include <nanosvg.h>

#include <wx/mstream.h>
#include <wx/xml/xml.h>

#include <fast_float/fast_float.h>


tl::expected<wxXmlDocument, wxString> KI_TEST::LoadSvg( const wxString& aSvg )
{
    const wxCharBuffer  utf8 = aSvg.ToUTF8();
    wxMemoryInputStream input( utf8.data(), utf8.length() );
    wxXmlDocument       doc;

    if( !doc.Load( input ) )
        return tl::make_unexpected( wxString( wxS( "Failed to load SVG XML" ) ) );

    if( doc.GetRoot()->GetName() != wxT( "svg" ) )
        return tl::make_unexpected( wxString( wxS( "Root element is not <svg>" ) ) );

    return doc;
}


tl::expected<KI_TEST::SVG_VIEWBOX, wxString> KI_TEST::ParseViewBox( const wxXmlNode& aRoot )
{
    if( aRoot.GetName() != wxT( "svg" ) )
        return tl::make_unexpected( wxS( "Root element is not <svg>" ) );

    wxString viewBox;

    if( !aRoot.GetAttribute( wxT( "viewBox" ), &viewBox ) )
        return tl::make_unexpected( wxS( "Missing viewBox attribute" ) );

    // The viewBox is four whitespace-separated numbers: min-x, min-y, width, height.
    const std::string values = viewBox.ToStdString();
    SVG_VIEWBOX       parsed{};
    double* const     valuesToParse[] = { &parsed.m_X, &parsed.m_Y, &parsed.m_Width, &parsed.m_Height };
    size_t            pos = 0;

    for( double* value : valuesToParse )
    {
        while( pos < values.size() && ( values[pos] == ' ' || values[pos] == '\t' ) )
            pos++;

        if( pos >= values.size() )
            return tl::make_unexpected( wxS( "viewBox must contain four numbers" ) );

        const auto [ptr, ec] = fast_float::from_chars( values.data() + pos, values.data() + values.size(), *value );

        if( ec != std::errc() )
            return tl::make_unexpected( wxS( "Invalid viewBox number" ) );

        pos = ptr - values.data();
    }

    while( pos < values.size() && ( values[pos] == ' ' || values[pos] == '\t' ) )
        pos++;

    if( pos != values.size() )
        return tl::make_unexpected( wxS( "viewBox must contain exactly four numbers" ) );

    return parsed;
}


const wxXmlNode* KI_TEST::FindFirstRect( const wxXmlNode& aNode )
{
    for( const wxXmlNode* child = aNode.GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == wxT( "rect" ) )
            return child;

        if( const wxXmlNode* rect = FindFirstRect( *child ) )
            return rect;
    }

    return nullptr;
}