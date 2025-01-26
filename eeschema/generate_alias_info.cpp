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

#include <generate_alias_info.h>
#include <ki_exception.h>
#include <string_utils.h>
#include <template_fieldnames.h>
#include <lib_symbol.h>
#include <libraries/symbol_library_manager_adapter.h>
#include <wx/log.h>

static const wxString DescriptionFormat = wxS(
    "<b>__NAME__</b>"
    "__ALIASOF__"
    "__DESC__"
    "__KEY__"
    "<hr><table border=0>"
    "__FIELDS__"
    "</table>" );

static const wxString AliasOfFormat =   wxS( "<br><i>" ) + _( "Derived from" ) +
                                        wxS( " %s (%s)</i>" );
static const wxString DescFormat =      wxS( "<br>%s" );
static const wxString KeywordsFormat =  wxS( "<br>" ) + _( "Keywords" ) + wxS( ": %s" );
static const wxString FieldFormat = wxS(
    "<tr>"
    "   <td><b>__NAME__</b></td>"
    "   <td>__VALUE__</td>"
    "</tr>" );
static const wxString LinkFormat = wxS( "<a href=\"__HREF__\">__TEXT__</a>" );


class FOOTPRINT_INFO_GENERATOR
{
public:
    FOOTPRINT_INFO_GENERATOR( SYMBOL_LIBRARY_MANAGER_ADAPTER* aLibs, LIB_ID const& aLibId, int aUnit ) :
            m_html( DescriptionFormat ),
            m_libs( aLibs ),
            m_lib_id( aLibId ),
            m_symbol( nullptr ),
            m_unit( aUnit )
    { }

    /**
     * Generate the HTML internally.
     */
    void GenerateHtml()
    {
        wxCHECK_RET( m_libs, "Symbol library manager adapter pointer is not valid" );

        if( !m_lib_id.IsValid() )
            return;

        try
        {
            m_symbol = m_libs->LoadSymbol( m_lib_id );
        }
        catch( const IO_ERROR& ioe )
        {
            wxLogError( _( "Error loading symbol %s from library '%s'." ) + wxS( "\n%s" ),
                        m_lib_id.GetLibItemName().wx_str(),
                        m_lib_id.GetLibNickname().wx_str(),
                        ioe.What() );
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
        m_html.Replace( wxS( "__NAME__" ), EscapeHTML( UnescapeString( m_symbol->GetName() ) ) );
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
            wxString root_desc = wxS( "" );

            std::shared_ptr< LIB_SYMBOL > parent = m_symbol->GetParent().lock();

            if( parent )
            {
                root_name = parent->GetName();
                root_desc = parent->GetDesc();
            }

            m_html.Replace( wxS( "__ALIASOF__" ), wxString::Format( AliasOfFormat,
                                                                    EscapeHTML( UnescapeString( root_name ) ),
                                                                    EscapeHTML( root_desc ) ) );
        }
    }

    void SetHtmlDesc()
    {
        wxString esc_desc = EscapeHTML( UnescapeString( m_symbol->GetDescription() ) );

        // Add line breaks
        esc_desc.Replace( wxS( "\n" ), wxS( "<br>" ) );

        // Add links
        esc_desc = LinkifyHTML( esc_desc );

        m_html.Replace( wxS( "__DESC__" ), wxString::Format( DescFormat, esc_desc ) );
    }

    void SetHtmlKeywords()
    {
        wxString keywords = m_symbol->GetKeyWords();

        if( keywords.empty() )
            m_html.Replace( wxS( "__KEY__" ), wxEmptyString );
        else
            m_html.Replace( wxS( "__KEY__" ), wxString::Format( KeywordsFormat, EscapeHTML( keywords ) ) );
    }

    wxString GetHtmlFieldRow( const SCH_FIELD& aField ) const
    {
        wxString name = aField.GetCanonicalName();
        wxString text;
        wxString fieldhtml = FieldFormat;

        fieldhtml.Replace( wxS( "__NAME__" ), EscapeHTML( name ) );

        switch( aField.GetId() )
        {
        case FIELD_T::DATASHEET:
            text = m_symbol->GetDatasheetField().GetShownText( false );

            if( text.IsEmpty() || text == wxT( "~" ) )
            {
                fieldhtml.Replace( wxS( "__VALUE__" ), text );
            }
            else
            {
                wxString datasheetlink = LinkFormat;
                datasheetlink.Replace( wxS( "__HREF__" ), EscapeHTML( text ) );

                if( text.Length() > 75 )
                    text = text.Left( 72 ) + wxT( "..." );

                datasheetlink.Replace( wxS( "__TEXT__" ), EscapeHTML( text ) );

                fieldhtml.Replace( wxS( "__VALUE__" ), datasheetlink );
            }

            break;

        case FIELD_T::VALUE:
            // showing the value just repeats the name, so that's not much use...
            return wxEmptyString;

        case FIELD_T::REFERENCE:
            text = aField.GetFullText( m_unit > 0 ? m_unit : 1 );
            fieldhtml.Replace( wxS( "__VALUE__" ), EscapeHTML( text ) );
            break;

        default:
            text = aField.GetShownText( false );

            if( aField.IsHypertext() )
            {
                wxString link = LinkFormat;
                link.Replace( wxS( "__HREF__" ), EscapeHTML( text ) );

                if( text.Length() > 75 )
                    text = text.Left( 72 ) + wxT( "..." );

                link.Replace( wxS( "__TEXT__" ), EscapeHTML( text ) );

                fieldhtml.Replace( wxS( "__VALUE__" ), link );
            }
            else
            {
                fieldhtml.Replace( wxS( "__VALUE__" ), EscapeHTML( text ) );
            }
        }

        return fieldhtml;
    }


    void SetHtmlFieldTable()
    {
        wxString                fieldtable;
        std::vector<SCH_FIELD*> fields;

        m_symbol->GetFields( fields );

        for( const SCH_FIELD* field: fields )
            fieldtable += GetHtmlFieldRow( *field );

        if( m_symbol->IsDerived() )
        {
            std::shared_ptr<LIB_SYMBOL> parent = m_symbol->GetParent().lock();

            // Append all of the unique parent fields if this is a derived symbol.
            if( parent )
            {
                std::vector<SCH_FIELD*> parentFields;

                parent->GetFields( parentFields );

                for( const SCH_FIELD* parentField : parentFields )
                {
                    if( m_symbol->GetField( parentField->GetCanonicalName() ) )
                        continue;

                    fieldtable += GetHtmlFieldRow( *parentField );
                }
            }
        }

        m_html.Replace( wxS( "__FIELDS__" ), fieldtable );
    }

private:
    wxString                        m_html;
    SYMBOL_LIBRARY_MANAGER_ADAPTER* m_libs;
    LIB_ID const                    m_lib_id;
    LIB_SYMBOL*                     m_symbol;
    int                             m_unit;
};


wxString GenerateAliasInfo( SYMBOL_LIBRARY_MANAGER_ADAPTER* aLibs, LIB_ID const& aLibId, int aUnit )
{
    FOOTPRINT_INFO_GENERATOR gen( aLibs, aLibId, aUnit );
    gen.GenerateHtml();
    return gen.GetHtml();
}
