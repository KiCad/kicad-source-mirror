/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <template_fieldnames.h>
#include <dsnlexer.h>
#include <fctsys.h>
#include <macros.h>

using namespace TFIELD_T;

wxString TEMPLATE_FIELDNAME::GetDefaultFieldName( int aFieldNdx )
{
    // Fixed values for the first few default fields used by EESCHEMA
    static const wxString fixedNames[] = {
        _( "Reference" ),   // The component reference, R1, C1, etc.
        _( "Value" ),       // The component value + name
        _( "Footprint" ),   // The footprint for use with Pcbnew
        _( "Datasheet" ),   // Link to a datasheet for component
    };

    if( (unsigned) aFieldNdx < DIM(fixedNames) )
        return fixedNames[aFieldNdx];

    else
    {
        wxString fieldName = _( "Field" );

        fieldName << aFieldNdx;

        return fieldName;
    }
}

void TEMPLATE_FIELDNAME::Format( OUTPUTFORMATTER* out, int nestLevel ) const throw( IO_ERROR )
{
    out->Print( nestLevel, "(field (name %s)",  out->Quotew( m_Name ).c_str() );

    if( !m_Value.IsEmpty() )
        out->Print( 0, "(value %s)", out->Quotew( m_Value ).c_str() );

    if( m_Visible )
        out->Print( 0, " visible" );

    out->Print( 0, ")\n" );
}


void TEMPLATE_FIELDNAME::Parse( TEMPLATE_FIELDNAMES_LEXER* in ) throw( IO_ERROR )
{
    T    tok;

    in->NeedLEFT();     // begin (name ...)

    if( (tok = in->NextTok()) != T_name )
        in->Expecting( T_name );

    in->NeedSYMBOLorNUMBER();

    m_Name = FROM_UTF8( in->CurText() );

    in->NeedRIGHT();    // end (name ...)

    while( (tok = in->NextTok() ) != T_RIGHT && tok != T_EOF )
    {
        // "visible" has no '(' prefix, "value" does, so T_LEFT is optional.
        if( tok == T_LEFT )
            tok = in->NextTok();

        switch( tok )
        {
        case T_value:
            in->NeedSYMBOLorNUMBER();
            m_Value = FROM_UTF8( in->CurText() );
            in->NeedRIGHT();
            break;

        case T_visible:
            m_Visible = true;
            break;

        default:
            in->Expecting( "value|visible" );
            break;
        }
    }
}


void TEMPLATES::Format( OUTPUTFORMATTER* out, int nestLevel ) const throw( IO_ERROR )
{
    // We'll keep this general, and include the \n, even though the only known
    // use at this time will not want the newlines or the indentation.
    out->Print( nestLevel, "(templatefields" );

    for( unsigned i=0;  i<m_Fields.size();  ++i )
        m_Fields[i].Format( out, nestLevel+1 );

    out->Print( 0, ")\n" );
}


void TEMPLATES::Parse( TEMPLATE_FIELDNAMES_LEXER* in ) throw( IO_ERROR )
{
    T  tok;

    while( ( tok = in->NextTok() ) != T_RIGHT && tok != T_EOF )
    {
        if( tok == T_LEFT )
            tok = in->NextTok();

        switch( tok )
        {
        case T_templatefields:  // a token indicating class TEMPLATES.

            // Be flexible regarding the starting point of the TEMPLATE_FIELDNAMES_LEXER
            // stream.  Caller may not have read the first two tokens out of the
            // stream: T_LEFT and T_templatefields, so ignore them if seen here.
            break;

        case T_field:
            {
                // instantiate on stack, so if exception is thrown,
                // destructor runs
                TEMPLATE_FIELDNAME  field;

                field.Parse( in );

                // add the field
                AddTemplateFieldName( field );
            }
            break;

        default:
            in->Unexpected( in->CurText() );
            break;
        }
    }
}


int TEMPLATES::AddTemplateFieldName( const TEMPLATE_FIELDNAME& aFieldName )
{
    // Ensure that the template fieldname does not match a fixed fieldname.
    for( int i=0;  i<MANDATORY_FIELDS;  ++i )
    {
        if( TEMPLATE_FIELDNAME::GetDefaultFieldName( i ) == aFieldName.m_Name )
        {
            return -1;
        }
    }

    // ensure uniqueness, overwrite any template fieldname by the same name.
    for( unsigned i=0; i<m_Fields.size();  ++i )
    {
        if( m_Fields[i].m_Name == aFieldName.m_Name )
        {
            // DBG( printf( "inserting template fieldname:'%s' at %d\n",
            //            TO_UTF8( aFieldName.m_Name ), i ); )

            m_Fields[i] = aFieldName;
            return i;   // return the container index
        }
    }

    // DBG(printf("appending template fieldname:'%s'\n", aFieldName.m_Name.utf8_str() );)

    // the name is legal and not previously added to the config container, append
    // it and return its index within the container.
    m_Fields.push_back( aFieldName );

    return m_Fields.size() - 1; // return the index of insertion.
}


bool TEMPLATES::HasFieldName( const wxString& aName ) const
{
    for( size_t i=0; i<m_Fields.size();  ++i )
    {
        if( m_Fields[i].m_Name == aName )
            return true;
    }

    return false;
}

