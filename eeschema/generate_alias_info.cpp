/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2017-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <lib_part.h>
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
static const wxString KeywordsFormat =  "<br>" + _( "Keywords" ) + ": %s";
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
    LIB_PART* m_symbol;
    int m_unit;

public:
    FOOTPRINT_INFO_GENERATOR( SYMBOL_LIB_TABLE* aSymbolLibTable, LIB_ID const& aLibId, int aUnit )
        : m_html( DescriptionFormat ),
          m_sym_lib_table( aSymbolLibTable ),
          m_lib_id( aLibId ),
          m_symbol( nullptr ),
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
            m_symbol = m_sym_lib_table->LoadSymbol( m_lib_id );
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

        if( m_symbol )
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
    wxString GetHtml() const
    {
        return m_html;
    }

protected:
    void SetHtmlName()
    {
        m_html.Replace( "__NAME__", EscapeHTML( m_symbol->GetName() ) );
    }


    void SetHtmlAliasOf()
    {
        if( m_symbol->IsRoot() )
        {
            m_html.Replace( "__ALIASOF__", wxEmptyString );
        }
        else
        {
            wxString root_name = _( "Unknown" );
            wxString root_desc = "";

            std::shared_ptr< LIB_PART > parent = m_symbol->GetParent().lock();

            if( parent )
            {
                root_name = parent->GetName();
                root_desc = parent->GetDescription();
            }

            m_html.Replace( "__ALIASOF__", wxString::Format(  AliasOfFormat,
                                                              EscapeHTML( root_name ),
                                                              EscapeHTML( root_desc ) ) );
        }
    }


    void SetHtmlDesc()
    {
        wxString raw_desc = m_symbol->GetDescription();

        m_html.Replace( "__DESC__", wxString::Format( DescFormat, EscapeHTML( raw_desc ) ) );
    }


    void SetHtmlKeywords()
    {
        wxString keywords = m_symbol->GetKeyWords();

        if( keywords.empty() )
            m_html.Replace( "__KEY__", wxEmptyString );
        else
            m_html.Replace( "__KEY__", wxString::Format( KeywordsFormat, EscapeHTML( keywords ) ) );
    }


    wxString GetHtmlFieldRow( const LIB_FIELD& aField ) const
    {
        wxString name = aField.GetCanonicalName();
        wxString text = aField.GetFullText( m_unit > 0 ? m_unit : 1 );
        wxString fieldhtml = FieldFormat;

        fieldhtml.Replace( "__NAME__", EscapeHTML( name ) );

        switch( aField.GetId() )
        {
        case DATASHEET_FIELD:
            text = m_symbol->GetDatasheetField().GetText();

            if( text.IsEmpty() || text == wxT( "~" ) )
            {
                fieldhtml.Replace( "__VALUE__", text );
            }
            else
            {
                wxString datasheetlink = DatasheetLinkFormat;
                datasheetlink.Replace( "__HREF__", EscapeHTML( text ) );

                if( text.Length() > 75 )
                    text = text.Left( 72 ) + wxT( "..." );

                datasheetlink.Replace( "__TEXT__", EscapeHTML( text ) );

                fieldhtml.Replace( "__VALUE__", datasheetlink );
            }

            break;

        case VALUE_FIELD:
            // showing the value just repeats the name, so that's not much use...
            return wxEmptyString;

        default:
            fieldhtml.Replace( "__VALUE__", EscapeHTML( text ) );
        }

        return fieldhtml;
    }


    void SetHtmlFieldTable()
    {
        wxString                fieldtable;
        std::vector<LIB_FIELD*> fields;

        m_symbol->GetFields( fields );

        for( const LIB_FIELD* field: fields )
            fieldtable += GetHtmlFieldRow( *field );

        if( m_symbol->IsAlias() )
        {
            std::shared_ptr<LIB_PART> parent = m_symbol->GetParent().lock();

            // Append all of the unique parent fields if this is an alias.
            if( parent )
            {
                std::vector<LIB_FIELD*> parentFields;

                parent->GetFields( parentFields );

                for( const LIB_FIELD* parentField : parentFields )
                {
                    if( m_symbol->FindField( parentField->GetCanonicalName() ) )
                        continue;

                    fieldtable += GetHtmlFieldRow( *parentField );
                }
            }
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
