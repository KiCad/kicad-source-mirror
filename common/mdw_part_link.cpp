/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <mdw_part_link.h>

#include <wx/tokenzr.h>

#include <vector>


namespace
{

wxString escape( const wxString& aText )
{
    wxString out;

    for( wxUniChar c : aText )
    {
        if( c == '%' )
            out += wxS( "%25" );
        else if( c == '|' )
            out += wxS( "%7C" );
        else
            out += c;
    }

    return out;
}


std::optional<wxString> unescape( const wxString& aText )
{
    wxString out;

    for( size_t ii = 0; ii < aText.length(); ++ii )
    {
        if( aText[ii] != '%' )
        {
            out += aText[ii];
            continue;
        }

        wxString code = aText.Mid( ii + 1, 2 ).Upper();

        if( code == wxS( "25" ) )
            out += '%';
        else if( code == wxS( "7C" ) )
            out += '|';
        else
            return std::nullopt;

        ii += 2;
    }

    return out;
}

} // namespace


wxString MdwFormatPartField( const MDW_PART_IDENTITY& aPart )
{
    return escape( aPart.part.source ) + wxS( "|" ) + escape( aPart.part.item_id ) + wxS( "|" )
           + escape( aPart.revision_id );
}


std::optional<MDW_PART_IDENTITY> MdwParsePartField( const wxString& aText )
{
    std::vector<wxString> fields;
    wxStringTokenizer     tokens( aText, wxS( "|" ), wxTOKEN_RET_EMPTY_ALL );

    while( tokens.HasMoreTokens() )
    {
        std::optional<wxString> field = unescape( tokens.GetNextToken() );

        if( !field )
            return std::nullopt;

        fields.push_back( *field );
    }

    if( fields.size() != 3 || fields[0].IsEmpty() || fields[1].IsEmpty() )
        return std::nullopt;

    MDW_PART_IDENTITY part;
    part.part.source = fields[0];
    part.part.item_id = fields[1];
    part.revision_id = fields[2];
    return part;
}
