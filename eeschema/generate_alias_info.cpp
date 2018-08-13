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
#include <class_libentry.h>
#include <symbol_lib_table.h>


static const wxString DescriptionFormat =
    "<b>__NAME__</b>"
    "__ALIASOF__"
    "__DESC__"
    "__KEY__"
    "<hr><table border=0>"
    "__FIELDS__"
    "</table>";

static const wxString AliasOfFormat =   "<br><i>" + _( "Alias of" ) + " %s (%s)</i>";
static const wxString DescFormat =      "<br>%s";
static const wxString KeywordsFormat =  "<br>" + _( "Key words:" ) + " %s";
static const wxString FieldFormat =
    "<tr>"
    "   <td><b>__NAME__</b></td>"
    "   <td>__VALUE__</td>"
    "</tr>";
static const wxString DatasheetLinkFormat = "<a href=\"__HREF__\">__TEXT__</a>";


class FOOTPRINT_INFO_GENERATOR
{
    wxString m_html;
    SYMBOL_LIB_TABLE* m_sym_lib_table;
    LIB_ID const m_lib_id;
    LIB_ALIAS* m_module;
    int m_unit;

public:
    FOOTPRINT_INFO_GENERATOR( SYMBOL_LIB_TABLE* aSymbolLibTable, LIB_ID const& aLibId, int aUnit )
        : m_html( DescriptionFormat ),
          m_sym_lib_table( aSymbolLibTable ),
          m_lib_id( aLibId ),
          m_module( nullptr ),
          m_unit( aUnit )
    { }

    /**
     * Generate the HTML internally.
     */
    void GenerateHtml()
    {
        wxCHECK_RET( m_sym_lib_table, "Symbol library table pointer is not valid" );

        if( !m_lib_id.IsValid() )
            return;

        try
        {
            m_module = const_cast< LIB_ALIAS* >( m_sym_lib_table->LoadSymbol( m_lib_id ) );
        }
        catch( const IO_ERROR& ioe )
        {
            wxLogError( wxString::Format( _( "Error occurred loading symbol %s from library %s."
                                             "\n\n%s" ),
                                          m_lib_id.GetLibItemName().wx_str(),
                                          m_lib_id.GetLibNickname().wx_str(),
                                          ioe.What() ) );
            return;
        }

        if( m_module )
        {
            SetHtmlName();
            SetHtmlAliasOf();
            SetHtmlDesc();
            SetHtmlKeywords();
            SetHtmlFieldTable();
        }
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
        m_html.Replace( "__NAME__", EscapedHTML( m_module->GetName() ) );
    }


    void SetHtmlAliasOf()
    {
        if( m_module->IsRoot() )
        {
            m_html.Replace( "__ALIASOF__", wxEmptyString );
        }
        else
        {
            wxString root_name = _( "Unknown" );
            wxString root_desc = "";

            LIB_PART* root = m_module->GetPart();
            LIB_ALIAS* root_alias = root ? root->GetAlias( 0 ) : nullptr;

            if( root )
                root_name = root->GetName();

            if( root_alias )
                root_desc = root_alias->GetDescription();

            m_html.Replace(
                "__ALIASOF__", wxString::Format(
                    AliasOfFormat, EscapedHTML( root_name ), EscapedHTML( root_desc ) ) );
        }
    }


    void SetHtmlDesc()
    {
        wxString raw_desc = m_module->GetDescription();

        m_html.Replace( "__DESC__", wxString::Format( DescFormat, EscapedHTML( raw_desc ) ) );
    }


    void SetHtmlKeywords()
    {
        wxString keywords = m_module->GetKeyWords();

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
                text = m_module->GetDocFileName();

                if( text.IsEmpty() || text == wxT( "~" ) )
                {
                    fieldhtml.Replace( "__VALUE__", text );
                }
                else
                {
                    wxString datasheetlink = DatasheetLinkFormat;
                    datasheetlink.Replace( "__HREF__", EscapedHTML( text ) );

                    if( text.Length() > 75 )
                        text = text.Left( 72 ) + wxT( "..." );

                    datasheetlink.Replace( "__TEXT__", EscapedHTML( text ) );

                    fieldhtml.Replace( "__VALUE__", datasheetlink );
                }
            }
            break;

        case VALUE:
            // showing the value just repeats the name, so that's not much use...
            return wxEmptyString;

        default:
            fieldhtml.Replace( "__VALUE__", EscapedHTML( text ) );
        }

        return fieldhtml;
    }


    void SetHtmlFieldTable()
    {
        wxString fieldtable;
        LIB_FIELDS fields;
        m_module->GetPart()->GetFields( fields );

        for( auto const & field: fields )
        {
            fieldtable += GetHtmlFieldRow( field );
        }

        m_html.Replace( "__FIELDS__", fieldtable );
    }
};


wxString GenerateAliasInfo( SYMBOL_LIB_TABLE* aSymLibTable, LIB_ID const& aLibId, int aUnit )
{
    FOOTPRINT_INFO_GENERATOR gen( aSymLibTable, aLibId, aUnit );
    gen.GenerateHtml();
    return gen.GetHtml();
}
