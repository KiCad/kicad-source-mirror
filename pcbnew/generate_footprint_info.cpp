/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Chris Pavlina <pavlina.chris@gmail.com>
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

#include <generate_footprint_info.h>
#include <ki_exception.h>
#include <string_utils.h>
#include <footprint.h>
#include <footprint_library_adapter.h>
#include <trace_helpers.h>
#include <wx/log.h>


static const wxString DescriptionFormat = wxT(
    "<b>__NAME__</b>"
    "<br>__DESC__"
    "<hr><table border=0>"
    "__FIELDS__"
    "</table>" );

static const wxString KeywordsFormat = wxT(
        "<tr>"
        "   <td><b>" + _( "Keywords" ) + "</b></td>"
        "   <td>__KEYWORDS__</td>"
        "</tr>" );

static const wxString DocFormat = wxT(
        "<tr>"
        "   <td><b>" + _( "Documentation" ) + "</b></td>"
        "   <td><a href=\"__HREF__\">__TEXT__</a></td>"
        "</tr>" );


std::optional<wxString> GetFootprintDocumentationURL( const FOOTPRINT& aFootprint )
{
    // Footprints have now a field (FIELD_T::DATASHEET) containing the url datasheet
    // But old footprints did not have this field, so this fiels can be empty.
    // So we use this field is not empty, and if empty see if the documentation has an URL
    const PCB_FIELD* data_field = aFootprint.GetField( FIELD_T::DATASHEET );
    wxString url = data_field->GetText();

    if( !url.IsEmpty() )
        return url;

    wxString desc = aFootprint.GetLibDescription();

    // It is (or was) currently common practice to store a documentation link in the description.
    size_t idx = desc.find( wxT( "http:" ) );

    if( idx == wxString::npos )
        idx = desc.find( wxT( "https:" ) );

    if( idx == wxString::npos )
        return std::nullopt;

    int nesting = 0;

    for( auto chit = desc.begin() + idx; chit != desc.end(); ++chit )
    {
        int ch = *chit;

        // Break on invalid URI characters
        if( ch <= 0x20 || ch >= 0x7F || ch == '"' )
            break;

        // Check for nesting parentheses, e.g. (Body style from: https://this.url/part.pdf)
        if( ch == '(' )
            ++nesting;
        else if( ch == ')' && --nesting < 0 )
            break;

        url += ch;
    }

    // Trim trailing punctuation
    static wxString punct = wxS( ".,:;" );

    if( punct.find( url.Last() ) != wxString::npos )
        url = url.Left( url.Length() - 1 );

    if( url.IsEmpty() )
        return std::nullopt;

    return url;
}


class FOOTPRINT_INFO_GENERATOR
{
public:
    FOOTPRINT_INFO_GENERATOR( FOOTPRINT_LIBRARY_ADAPTER* aAdapter, LIB_ID const& aLibId ) :
            m_html( DescriptionFormat ),
            m_adapter( aAdapter ),
            m_lib_id( aLibId ),
            m_footprint( nullptr )
    { }

    /**
     * Generate the HTML internally.
     */
    void GenerateHtml()
    {
        wxCHECK_RET( m_adapter, wxT( "Footprint library table pointer is not valid" ) );

        if( !m_lib_id.IsValid() )
            return;

        try
        {
            m_footprint = m_adapter->LoadFootprint( m_lib_id, false );
        }
        catch( const IO_ERROR& ioe )
        {
            // Log to trace instead of error to avoid popup dialogs
            wxLogTrace( traceLibraries, "Error loading footprint %s from library '%s': %s",
                        m_lib_id.GetLibItemName().wx_str(),
                        m_lib_id.GetLibNickname().wx_str(),
                        ioe.What() );
            return;
        }

        if( m_footprint )
        {
            wxString name = m_lib_id.GetLibItemName();
            wxString desc = m_footprint->GetLibDescription();
            wxString keywords = m_footprint->GetKeywords();
            wxString doc;

            if( std::optional<wxString> url = GetFootprintDocumentationURL( *m_footprint ) )
                doc = *url;

            wxString esc_desc = EscapeHTML( UnescapeString( desc ) );

            // Add line breaks
            esc_desc.Replace( wxS( "\n" ), wxS( "<br>" ) );

            // Add links
            esc_desc = LinkifyHTML( esc_desc );

            m_html.Replace( "__DESC__", esc_desc );
            m_html.Replace( "__NAME__", EscapeHTML( name ) );

            wxString keywordsHtml = KeywordsFormat;
            keywordsHtml.Replace( "__KEYWORDS__", EscapeHTML( keywords ) );

            wxString docHtml = DocFormat;
            docHtml.Replace( "__HREF__", EscapeHTML( doc ) );

            if( doc.Length() > 75 )
                doc = doc.Left( 72 ) + wxT( "..." );

            docHtml.Replace( "__TEXT__", EscapeHTML( doc ) );

            m_html.Replace( "__FIELDS__", keywordsHtml + docHtml );
        }
    }

    /**
     * Return the generated HTML.
     */
    wxString GetHtml()
    {
        return m_html;
    }

private:
    wxString                   m_html;
    FOOTPRINT_LIBRARY_ADAPTER* m_adapter;
    LIB_ID const               m_lib_id;

    const FOOTPRINT* m_footprint;
};


wxString GenerateFootprintInfo( FOOTPRINT_LIBRARY_ADAPTER* aAdapter, LIB_ID const& aLibId )
{
    FOOTPRINT_INFO_GENERATOR gen( aAdapter, aLibId );
    gen.GenerateHtml();
    return gen.GetHtml();
}
