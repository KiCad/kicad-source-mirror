/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <string_utils.h>
#include <footprint.h>
#include <fp_lib_table.h>
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


class FOOTPRINT_INFO_GENERATOR
{
    wxString      m_html;
    FP_LIB_TABLE* m_fp_lib_table;
    LIB_ID const  m_lib_id;

    const FOOTPRINT*    m_footprint;

public:
    FOOTPRINT_INFO_GENERATOR( FP_LIB_TABLE* aFpLibTable, LIB_ID const& aLibId )
        : m_html( DescriptionFormat ),
          m_fp_lib_table( aFpLibTable ),
          m_lib_id( aLibId ),
          m_footprint( nullptr )
    { }

    /**
     * Generate the HTML internally.
     */
    void GenerateHtml()
    {
        wxCHECK_RET( m_fp_lib_table, wxT( "Footprint library table pointer is not valid" ) );

        if( !m_lib_id.IsValid() )
            return;

        try
        {
            m_footprint = m_fp_lib_table->GetEnumeratedFootprint( m_lib_id.GetLibNickname(),
                                                                  m_lib_id.GetLibItemName() );
        }
        catch( const IO_ERROR& ioe )
        {
            wxLogError( _( "Error loading footprint %s from library '%s'." ) + wxS( "\n%s" ),
                        m_lib_id.GetLibItemName().wx_str(),
                        m_lib_id.GetLibNickname().wx_str(),
                        ioe.What() );
            return;
        }

        if( m_footprint )
        {
            wxString name = m_lib_id.GetLibItemName();
            wxString desc = m_footprint->GetDescription();
            wxString keywords = m_footprint->GetKeywords();
            wxString doc;

            // It is currently common practice to store a documentation link in the description.
            int idx = desc.find( wxT( "http:" ) );

            if( idx < 0 )
                idx = desc.find( wxT( "https:" ) );

            if( idx >= 0 )
            {
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

                    doc += ch;
                }

                    desc.Replace( doc, _( "doc url" ) );
            }

            m_html.Replace( "__NAME__", EscapeHTML( name ) );
            m_html.Replace( "__DESC__", EscapeHTML( desc ) );

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

};


wxString GenerateFootprintInfo( FP_LIB_TABLE* aFpLibTable, LIB_ID const& aLibId )
{
    FOOTPRINT_INFO_GENERATOR gen( aFpLibTable, aLibId );
    gen.GenerateHtml();
    return gen.GetHtml();
}
