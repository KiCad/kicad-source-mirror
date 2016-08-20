/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Wayne Stambaugh, stambaughw@gmail.com
 * Copyright (C) 2016 KiCad Developers, see change_log.txt for contributors.
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

/**
 * @file sch_validators.cpp
 * @brief Implementation of control validators for schematic dialogs.
 */

#include <sch_validators.h>
#include <template_fieldnames.h>

SCH_FIELD_VALIDATOR::SCH_FIELD_VALIDATOR(  bool aIsCmplibEditor,
                                           int aFieldId, wxString* aValue ) :
    wxTextValidator( wxFILTER_EXCLUDE_CHAR_LIST, aValue )
{
    m_fieldId = aFieldId;
    m_isLibEditor = aIsCmplibEditor;

    // Fields cannot contain carriage returns, line feeds, or tabs.
    wxString excludes( "\r\n\t" );

    // The reference field cannot contain spaces.
    if( aFieldId == REFERENCE )
        excludes += " ";
    else if( aFieldId == VALUE && m_isLibEditor )
        excludes += " ";

    long style = GetStyle();

    // The reference and value fields cannot be empty.
    if( aFieldId == REFERENCE || aFieldId == VALUE )
        style |= wxFILTER_EMPTY;

    SetStyle( style );
    SetCharExcludes( excludes );
}


SCH_FIELD_VALIDATOR::SCH_FIELD_VALIDATOR( const SCH_FIELD_VALIDATOR& aValidator ) :
    wxTextValidator( aValidator )
{
    m_fieldId = aValidator.m_fieldId;
    m_isLibEditor = aValidator.m_isLibEditor;
}


bool SCH_FIELD_VALIDATOR::Validate( wxWindow *aParent )
{
    // If window is disabled, simply return
    if( !m_validatorWindow->IsEnabled() )
        return true;

    wxTextEntry * const text = GetTextEntry();

    if( !text )
        return false;

    wxString val( text->GetValue() );
    wxString tmp = val.Clone();           // For trailing and leading white space tests.
    wxString fieldName;

    switch( m_fieldId )
    {
    case REFERENCE: fieldName = _( "reference designator" ); break;
    case VALUE:     fieldName = _( "value" );                break;
    case FOOTPRINT: fieldName = _( "footprint" );            break;
    case DATASHEET: fieldName = _( "data sheet" );           break;
    default:        fieldName = _( "user defined" );         break;
    };

    wxString errorMsg;

    // We can only do some kinds of validation once the input is complete, so
    // check for them here:
    if( HasFlag( wxFILTER_EMPTY ) && val.empty() )
        errorMsg.Printf( _( "The %s field cannot be empty." ), fieldName );
    else if( HasFlag( wxFILTER_EXCLUDE_CHAR_LIST ) && ContainsExcludedCharacters( val ) )
    {
        wxArrayString whiteSpace;
        bool spaceIllegal = ( m_fieldId == REFERENCE ) ||
                            ( m_fieldId == VALUE && m_isLibEditor );

        if( val.Find( '\r' ) != wxNOT_FOUND )
            whiteSpace.Add( _( "carriage return" ) );
        if( val.Find( '\n' ) != wxNOT_FOUND )
            whiteSpace.Add( _( "line feed" ) );
        if( val.Find( '\t' ) != wxNOT_FOUND )
            whiteSpace.Add( _( "tab" ) );
        if( spaceIllegal && (val.Find( ' ' ) != wxNOT_FOUND) )
            whiteSpace.Add( _( "space" ) );

        wxString badChars;

        if( whiteSpace.size() == 1 )
            badChars = whiteSpace[0];
        else if( whiteSpace.size() == 2 )
            badChars.Printf( _( "%s or %s" ), whiteSpace[0], whiteSpace[1] );
        else if( whiteSpace.size() == 3 )
            badChars.Printf( _( "%s, %s, or %s" ), whiteSpace[0], whiteSpace[1], whiteSpace[2] );
        else if( whiteSpace.size() == 4 )
            badChars.Printf( _( "%s, %s, %s, or %s" ),
                             whiteSpace[0], whiteSpace[1], whiteSpace[2], whiteSpace[3] );
        else
            wxCHECK_MSG( false, true, wxT( "Invalid illegal character in field validator." ) );

        errorMsg.Printf( _( "The %s field cannot contain %s characters." ), fieldName, badChars );
    }
    else if( (tmp.Trim().Length() != val.Length()) || (tmp.Trim( false ).Length() != val.Length()) )
    {
        errorMsg.Printf( _( "The %s field cannot contain leading and/or trailing white space." ),
                         fieldName );
    }

    if ( !errorMsg.empty() )
    {
        m_validatorWindow->SetFocus();

        wxMessageBox( errorMsg, _( "Field Validation Error" ),
                      wxOK | wxICON_EXCLAMATION, aParent );

        return false;
    }

    return true;
}
