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

#include <generate_alias_info.h>
#include <kicad_string.h>
#include <template_fieldnames.h>

static const wxString DescriptionFormat =
    "<b>__NAME__</b>"
    "__ALIASOF__"
    "__DESC__"
    "__KEY__"
    "<hr><table border=0>"
    "__FIELDS__"
    "</table>";

static const wxString AliasOfFormat =   "<br><i>" + _( "Alias of " ) + "%s</i>";
static const wxString DescFormat =      "<br>%s";
static const wxString KeywordsFormat =  "<br>" + _( "Keywords:" ) + " %s";
static const wxString FieldFormat =
    "<tr>"
    "   <td><b>__NAME__</b></td>"
    "   <td>__VALUE__</td>"
    "</tr>";
static const wxString DatasheetLinkFormat = "<a href=\"__VALUE__\">__VALUE__</a>";


class ALIAS_INFO_GENERATOR
{
    wxString m_html;
    LIB_ALIAS const * m_part;
    int m_unit;

public:
    ALIAS_INFO_GENERATOR( LIB_ALIAS const * aAlias, int aUnit )
        : m_html( DescriptionFormat ),
          m_part( aAlias ),
          m_unit( aUnit )
    { }

    /**
     * Generate the HTML internally.
     */
    void GenerateHtml()
    {
        SetHtmlName();
        SetHtmlAliasOf();
        SetHtmlDesc();
        SetHtmlKeywords();
        SetHtmlFieldTable();
    }

    /**
     * Return the generated HTML.
     */
    wxString GetHtml()
    {
        return m_html;
    }

protected:
    void SetHtmlName()
    {
        m_html.Replace( "__NAME__", EscapedHTML( m_part->GetName() ) );
    }


    void SetHtmlAliasOf()
    {
        if( m_part->IsRoot() )
        {
            m_html.Replace( "__ALIASOF__", wxEmptyString );
        }
        else
        {
            LIB_PART* root = m_part->GetPart();
            const wxString root_name = ( root ? root->GetName() : _( "Unknown" ) );
            m_html.Replace(
                "__ALIASOF__", wxString::Format( AliasOfFormat, EscapedHTML( root_name ) ) );
        }
    }


    void SetHtmlDesc()
    {
        wxString raw_desc;

        if( m_part->IsRoot() )
        {
            raw_desc = m_part->GetDescription();
        }
        else
        {
            LIB_PART* root = m_part->GetPart();

            for( size_t i = 0; i < root->GetAliasCount(); ++i )
            {
                LIB_ALIAS* alias = root->GetAlias( i );

                if( alias && !alias->GetDescription().empty() )
                {
                    raw_desc = alias->GetDescription();
                    break;
                }
            }
        }

        m_html.Replace( "__DESC__", wxString::Format( DescFormat, EscapedHTML( raw_desc ) ) );
    }


    void SetHtmlKeywords()
    {
        wxString keywords = m_part->GetKeyWords();

        if( keywords.empty() )
            m_html.Replace( "__KEY__", wxEmptyString );
        else
            m_html.Replace( "__KEY__",
                    wxString::Format( KeywordsFormat, EscapedHTML( keywords ) ) );
    }


    wxString GetHtmlFieldRow( LIB_FIELD const & aField )
    {
        wxString name = aField.GetName();
        wxString text = aField.GetFullText( m_unit > 0 ? m_unit : 1 );
        wxString fieldhtml = FieldFormat;

        fieldhtml.Replace( "__NAME__", EscapedHTML( name ) );

        switch( aField.GetId() )
        {
        case DATASHEET:
            {
                wxString datasheetlink = DatasheetLinkFormat;
                datasheetlink.Replace( "__VALUE__", EscapedHTML( text ) );
                fieldhtml.Replace( "__VALUE__", datasheetlink );
            }
            break;

        default:
            fieldhtml.Replace( "__VALUE__", EscapedHTML( text ) );
        }

        return fieldhtml;
    }


    void SetHtmlFieldTable()
    {
        wxString fieldtable;
        LIB_FIELDS fields;
        m_part->GetPart()->GetFields( fields );

        for( auto const & field: fields )
        {
            fieldtable += GetHtmlFieldRow( field );
        }

        m_html.Replace( "__FIELDS__", fieldtable );
    }
};


wxString GenerateAliasInfo( LIB_ALIAS const * aAlias, int aUnit )
{
    ALIAS_INFO_GENERATOR gen( aAlias, aUnit );
    gen.GenerateHtml();
    return gen.GetHtml();
}
