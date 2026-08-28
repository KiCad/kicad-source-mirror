/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "template_fieldnames.h"

#include <mutex>

#include <template_fieldnames_lexer.h>
#include <string_utils.h>

using namespace TFIELD_T;

// N.B. Do not change these values without transitioning the file format
#define REFERENCE_UNTRANSLATED "Reference"
#define VALUE_UNTRANSLATED "Value"
#define FOOTPRINT_UNTRANSLATED "Footprint"
#define DATASHEET_UNTRANSLATED "Datasheet"
#define DESCRIPTION_UNTRANSLATED "Description"
#define SHEET_NAME_UNTRANSLATED "Sheetname"
#define SHEET_FILE_UNTRANSLATED "Sheetfile"
#define INTERSHEET_REFS_UNTRANSLATED "Intersheetrefs"
#define USER_FIELD_UNTRANSLATED_FORMAT "Field%d"

static wxString s_UntranslatedReference( REFERENCE_UNTRANSLATED );
static wxString s_UntranslatedValue( VALUE_UNTRANSLATED );
static wxString s_UntranslatedFootprint( FOOTPRINT_UNTRANSLATED );
static wxString s_UntranslatedDatasheet( DATASHEET_UNTRANSLATED );
static wxString s_UntranslatedDescription( DESCRIPTION_UNTRANSLATED );
static wxString s_UntranslatedSheetName( SHEET_NAME_UNTRANSLATED );
static wxString s_UntranslatedSheetFile( SHEET_FILE_UNTRANSLATED );
static wxString s_UntranslatedIntersheetRefs( INTERSHEET_REFS_UNTRANSLATED );


wxString GetDefaultFieldName( FIELD_T aFieldId, TRANSLATION aTranslation )
{
    if( aTranslation == UNTRANSLATED )
    {
        switch( aFieldId )
        {
        case FIELD_T::REFERENCE:       return s_UntranslatedReference;   // The symbol reference, R1, C1, etc.
        case FIELD_T::VALUE:           return s_UntranslatedValue;       // The symbol value
        case FIELD_T::FOOTPRINT:       return s_UntranslatedFootprint;   // The footprint for use with Pcbnew
        case FIELD_T::DATASHEET:       return s_UntranslatedDatasheet;   // Link to a datasheet for symbol
        case FIELD_T::DESCRIPTION:     return s_UntranslatedDescription; // The symbol description
        case FIELD_T::SHEET_NAME:      return s_UntranslatedSheetName;
        case FIELD_T::SHEET_FILENAME:  return s_UntranslatedSheetFile;
        case FIELD_T::INTERSHEET_REFS: return s_UntranslatedIntersheetRefs;
        default:                       return GetUserFieldName( 42, aTranslation );
        }
    }
    else
    {
        switch( aFieldId )
        {
        case FIELD_T::REFERENCE:       return _( REFERENCE_UNTRANSLATED );   // The symbol reference, R1, C1, etc.
        case FIELD_T::VALUE:           return _( VALUE_UNTRANSLATED );       // The symbol value
        case FIELD_T::FOOTPRINT:       return _( FOOTPRINT_UNTRANSLATED );   // The footprint for use with Pcbnew
        case FIELD_T::DATASHEET:       return _( DATASHEET_UNTRANSLATED );   // Link to a datasheet for symbol
        case FIELD_T::DESCRIPTION:     return _( DESCRIPTION_UNTRANSLATED ); // The symbol description
        case FIELD_T::SHEET_NAME:      return _( SHEET_NAME_UNTRANSLATED );
        case FIELD_T::SHEET_FILENAME:  return _( SHEET_FILE_UNTRANSLATED );
        case FIELD_T::INTERSHEET_REFS: return _( INTERSHEET_REFS_UNTRANSLATED );
        default:                       return GetUserFieldName( 42, aTranslation );
        }
    }
}


wxString GetUserFieldName( int aFieldNdx, TRANSLATION aTranslation )
{
    if( aTranslation == UNTRANSLATED )
        return wxString::Format( wxS( USER_FIELD_UNTRANSLATED_FORMAT ), aFieldNdx );
    else
        return wxString::Format( _( USER_FIELD_UNTRANSLATED_FORMAT ), aFieldNdx );
}


bool FieldNamesAreDuplicates( const wxString& aLhs, const wxString& aRhs,
                              std::initializer_list<FIELD_T> aMandatoryFields )
{
    if( aLhs == aRhs )
        return true;

    // If they don't even match case-insensitively they can't both be variants of the same
    // untranslated mandatory field name.
    if( aLhs.CmpNoCase( aRhs ) != 0 )
        return false;

    // Mandatory field names are folded case-insensitively by the s-expression parser, so any
    // case variant of an untranslated mandatory name collides with that mandatory field.
    for( FIELD_T fieldId : aMandatoryFields )
    {
        if( aLhs.CmpNoCase( GetDefaultFieldName( fieldId, UNTRANSLATED ) ) == 0 )
            return true;
    }

    return false;
}


bool FieldNamesAreDuplicates( const wxString& aLhs, const wxString& aRhs )
{
    return FieldNamesAreDuplicates( aLhs, aRhs, MANDATORY_FIELDS );
}


void TEMPLATE_FIELDNAME::Format( OUTPUTFORMATTER* out ) const
{
    out->Print( "(field (name %s)",  out->Quotew( m_Name ).c_str() );

    if( m_Visible )
        out->Print( " visible" );

    if( m_URL )
        out->Print( " url" );

    out->Print( ")" );
}


void TEMPLATE_FIELDNAME::Parse( TEMPLATE_FIELDNAMES_LEXER* in )
{
    T    tok;

    in->NeedLEFT();     // begin (name ...)

    if( ( tok = in->NextTok() ) != T_name )
        in->Expecting( T_name );

    in->NeedSYMBOLorNUMBER();

    m_Name = From_UTF8( in->CurText() );

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


void TEMPLATES::Format( OUTPUTFORMATTER* out, SCOPE aScope ) const
{
    // We'll keep this general, and include the \n, even though the only known
    // use at this time will not want the newlines or the indentation.
    out->Print( "(templatefields" );

    const std::vector<TEMPLATE_FIELDNAME>& source = aScope == SCOPE::GLOBAL ? m_globals : m_project;

    for( const TEMPLATE_FIELDNAME& temp : source )
    {
        if( !temp.m_Name.IsEmpty() )
            temp.Format( out );
    }

    out->Print( ")" );
}


void TEMPLATES::parse( TEMPLATE_FIELDNAMES_LEXER* in, SCOPE aScope )
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
                if( !field.m_Name.IsEmpty() )
                    AddTemplateFieldName( field, aScope );
            }
            break;

        default:
            in->Unexpected( in->CurText() );
            break;
        }
    }
}


/**
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
        bool overriddenInProject = false;

        for( const TEMPLATE_FIELDNAME& project : m_project )
        {
            if( global.m_Name == project.m_Name )
            {
                overriddenInProject = true;
                break;
            }
        }

        if( !overriddenInProject )
            m_resolved.push_back( global );
    }

    m_resolvedDirty = false;
}


void TEMPLATES::AddTemplateFieldName( const TEMPLATE_FIELDNAME& aFieldName, SCOPE aScope )
{
    // Reject any case variant of a mandatory fieldname; the s-expression parser folds those onto
    // the mandatory field with the matching untranslated name, so they can never become a
    // distinct user field.
    for( FIELD_T fieldId : MANDATORY_FIELDS )
    {
        if( GetDefaultFieldName( fieldId, UNTRANSLATED ).CmpNoCase( aFieldName.m_Name ) == 0 )
            return;
    }

    std::vector<TEMPLATE_FIELDNAME>& target = aScope == SCOPE::GLOBAL ? m_globals : m_project;

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


void TEMPLATES::AddTemplateFieldNames( const wxString& aSerializedFieldNames, SCOPE aScope )
{
    TEMPLATE_FIELDNAMES_LEXER field_lexer( TO_UTF8( aSerializedFieldNames ) );

    try
    {
        parse( &field_lexer, aScope );
    }
    catch( const IO_ERROR& )
    {
    }
}


void TEMPLATES::DeleteFieldNameTemplates( SCOPE aScope )
{
    if( aScope == SCOPE::GLOBAL )
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


const std::vector<TEMPLATE_FIELDNAME>& TEMPLATES::GetResolvedTemplateFieldNames()
{
    if( m_resolvedDirty )
        resolveTemplates();

    return m_resolved;
}


const std::vector<TEMPLATE_FIELDNAME>& TEMPLATES::GetTemplateFieldNames( SCOPE aScope )
{
    if( aScope == SCOPE::GLOBAL )
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
