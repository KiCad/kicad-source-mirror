/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2015-2020 KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <mutex>

#include <macros.h>
#include <template_fieldnames.h>
#include <pgm_base.h>

using namespace TFIELD_T;

#define REFERENCE_CANONICAL "Reference"
#define VALUE_CANONICAL "Value"
#define FOOTPRINT_CANONICAL "Footprint"
#define DATASHEET_CANONICAL "Datasheet"

static std::mutex s_defaultFieldMutex;

const wxString TEMPLATE_FIELDNAME::GetDefaultFieldName( int aFieldNdx, bool aTranslate )
{
    static void* locale = nullptr;
    static wxString referenceDefault;
    static wxString valueDefault;
    static wxString footprintDefault;
    static wxString datasheetDefault;
    static wxString fieldDefault;

    if( !aTranslate )
    {
        switch( aFieldNdx )
        {
        case  REFERENCE_FIELD: return REFERENCE_CANONICAL;   // The symbol reference, R1, C1, etc.
        case  VALUE_FIELD:     return VALUE_CANONICAL;       // The symbol value
        case  FOOTPRINT_FIELD: return FOOTPRINT_CANONICAL;   // The footprint for use with Pcbnew
        case  DATASHEET_FIELD: return DATASHEET_CANONICAL;   // Link to a datasheet for symbol
        }
    }

    // Mutex protection is needed so that multiple loader threads don't write to the static
    // variables at once
    std::lock_guard<std::mutex> lock( s_defaultFieldMutex );

    // Fetching translations can take a surprising amount of time when loading libraries,
    // so only do it when necessary.
    if( Pgm().GetLocale() != locale )
    {
        referenceDefault = _( REFERENCE_CANONICAL );
        valueDefault     = _( VALUE_CANONICAL );
        footprintDefault = _( FOOTPRINT_CANONICAL );
        datasheetDefault = _( DATASHEET_CANONICAL );
        fieldDefault     = _( "Field%d" );
        locale = Pgm().GetLocale();
    }

    // Fixed values for the mandatory fields
    switch( aFieldNdx )
    {
    case  REFERENCE_FIELD: return referenceDefault;   // The symbol reference, R1, C1, etc.
    case  VALUE_FIELD:     return valueDefault;       // The symbol value
    case  FOOTPRINT_FIELD: return footprintDefault;   // The footprint for use with Pcbnew
    case  DATASHEET_FIELD: return datasheetDefault;   // Link to a datasheet for symbol
    default:               return wxString::Format( fieldDefault, aFieldNdx );
    }

}

#undef REFERENCE_CANONICAL
#undef VALUE_CANONICAL
#undef FOOTPRINT_CANONICAL
#undef DATASHEET_CANONICAL


void TEMPLATE_FIELDNAME::Format( OUTPUTFORMATTER* out, int nestLevel ) const
{
    out->Print( nestLevel, "(field (name %s)",  out->Quotew( m_Name ).c_str() );

    if( m_Visible )
        out->Print( 0, " visible" );

    if( m_URL )
        out->Print( 0, " url" );

    out->Print( 0, ")\n" );
}


void TEMPLATE_FIELDNAME::Parse( TEMPLATE_FIELDNAMES_LEXER* in )
{
    T    tok;

    in->NeedLEFT();     // begin (name ...)

    if( ( tok = in->NextTok() ) != T_name )
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
            // older format; silently skip
            in->NeedSYMBOLorNUMBER();
            in->NeedRIGHT();
            break;

        case T_visible:
            m_Visible = true;
            break;

        case T_url:
            m_URL = true;
            break;

        default:
            in->Expecting( "value|url|visible" );
            break;
        }
    }
}


void TEMPLATES::Format( OUTPUTFORMATTER* out, int nestLevel, bool aGlobal ) const
{
    // We'll keep this general, and include the \n, even though the only known
    // use at this time will not want the newlines or the indentation.
    out->Print( nestLevel, "(templatefields" );

    const TEMPLATE_FIELDNAMES& source = aGlobal ? m_globals : m_project;

    for( const TEMPLATE_FIELDNAME& temp : source )
        temp.Format( out, nestLevel+1 );

    out->Print( 0, ")\n" );
}


void TEMPLATES::Parse( TEMPLATE_FIELDNAMES_LEXER* in, bool aGlobal )
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
                AddTemplateFieldName( field, aGlobal );
            }
            break;

        default:
            in->Unexpected( in->CurText() );
            break;
        }
    }
}


/*
 * Flatten project and global templates into a single list.  (Project templates take
 * precedence.)
 */
void TEMPLATES::resolveTemplates()
{
    m_resolved = m_project;

    // Note: order N^2 algorithm.  Would need changing if fieldname template sets ever
    // get large.

    for( const TEMPLATE_FIELDNAME& global : m_globals )
    {
        for( const TEMPLATE_FIELDNAME& project : m_project )
        {
            if( global.m_Name == project.m_Name )
                continue;
        }

        m_resolved.push_back( global );
    }

    m_resolvedDirty = false;
}


void TEMPLATES::AddTemplateFieldName( const TEMPLATE_FIELDNAME& aFieldName, bool aGlobal )
{
    // Ensure that the template fieldname does not match a fixed fieldname.
    for( int i = 0; i < MANDATORY_FIELDS; ++i )
    {
        if( TEMPLATE_FIELDNAME::GetDefaultFieldName( i ) == aFieldName.m_Name )
            return;
    }

    TEMPLATE_FIELDNAMES& target = aGlobal ? m_globals : m_project;

    // ensure uniqueness, overwrite any template fieldname by the same name.
    for( TEMPLATE_FIELDNAME& temp : target )
    {
        if( temp.m_Name == aFieldName.m_Name )
        {
            temp = aFieldName;
            m_resolvedDirty = true;
            return;
        }
    }

    // the name is legal and not previously added to the config container, append
    // it and return its index within the container.
    target.push_back( aFieldName );
    m_resolvedDirty = true;
}


void TEMPLATES::DeleteAllFieldNameTemplates( bool aGlobal )
{
    if( aGlobal )
    {
        m_globals.clear();
        m_resolved = m_project;
    }
    else
    {
        m_project.clear();
        m_resolved = m_globals;
    }

    m_resolvedDirty = false;
}


const TEMPLATE_FIELDNAMES& TEMPLATES::GetTemplateFieldNames()
{
    if( m_resolvedDirty )
        resolveTemplates();

    return m_resolved;
}


const TEMPLATE_FIELDNAMES& TEMPLATES::GetTemplateFieldNames( bool aGlobal )
{
    if( aGlobal )
        return m_globals;
    else
        return m_project;
}


const TEMPLATE_FIELDNAME* TEMPLATES::GetFieldName( const wxString& aName )
{
    if( m_resolvedDirty )
        resolveTemplates();

    for( const TEMPLATE_FIELDNAME& field : m_resolved )
    {
        if( field.m_Name == aName )
            return &field;
    }

    return nullptr;
}

