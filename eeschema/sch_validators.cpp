/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Wayne Stambaugh, stambaughw@gmail.com
 * Copyright (C) 2016-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/combo.h>
#include <wx/msgdlg.h>

#include <sch_connection.h>
#include <sch_validators.h>
#include <project/net_settings.h>
#include <template_fieldnames.h>


SCH_FIELD_VALIDATOR::SCH_FIELD_VALIDATOR(  bool aIsLibEditor, int aFieldId, wxString* aValue ) :
    wxTextValidator( wxFILTER_EXCLUDE_CHAR_LIST, aValue )
{
    m_fieldId = aFieldId;
    m_isLibEditor = aIsLibEditor;

    // Fields cannot contain carriage returns, line feeds, or tabs.
    wxString excludes( wxT( "\r\n\t" ) );

    // The reference and sheet name fields cannot contain spaces.
    if( aFieldId == REFERENCE_FIELD )
    {
        excludes += wxT( " " );
    }
    else if( m_fieldId == SHEETNAME_V )
    {
        excludes += wxT( "/" );
    }

    long style = GetStyle();

    // The reference, value sheetname and sheetfilename fields cannot be empty.
    if( aFieldId == REFERENCE_FIELD
      || aFieldId == VALUE_FIELD
      || aFieldId == SHEETNAME_V
      || aFieldId == SHEETFILENAME_V
      || aFieldId == FIELD_NAME )
    {
        style |= wxFILTER_EMPTY;
    }

    SetStyle( style );
    SetCharExcludes( excludes );
}


SCH_FIELD_VALIDATOR::SCH_FIELD_VALIDATOR( const SCH_FIELD_VALIDATOR& aValidator ) :
    wxTextValidator( aValidator )
{
    m_fieldId = aValidator.m_fieldId;
    m_isLibEditor = aValidator.m_isLibEditor;
}


bool SCH_FIELD_VALIDATOR::Validate( wxWindow* aParent )
{
    // If window is disabled, simply return
    if( !m_validatorWindow->IsEnabled() )
        return true;

    wxTextEntry* const text = GetTextEntry();

    if( !text )
        return false;

    wxString val( text->GetValue() );

    // The format of the error message for not allowed chars
    wxString fieldCharError;

    switch( m_fieldId )
    {
    case REFERENCE_FIELD:
        fieldCharError = _( "The reference designator cannot contain %s character(s)." );
        break;

    case VALUE_FIELD:
        fieldCharError = _( "The value field cannot contain %s character(s)." );
        break;

    case FOOTPRINT_FIELD:
        fieldCharError = _( "The footprint field cannot contain %s character(s)." );
        break;

    case DATASHEET_FIELD:
        fieldCharError = _( "The datasheet field cannot contain %s character(s)." );
        break;

    case SHEETNAME_V:
        fieldCharError = _( "The sheet name cannot contain %s character(s)." );
        break;

    case SHEETFILENAME_V:
        fieldCharError = _( "The sheet filename cannot contain %s character(s)." );
        break;

    default:
        fieldCharError = _( "The field cannot contain %s character(s)." );
        break;
    };

    wxString msg;

    // We can only do some kinds of validation once the input is complete, so
    // check for them here:
    if( HasFlag( wxFILTER_EMPTY ) && val.empty() )
    {
        // Some fields cannot have an empty value, and user fields require a name:
        if( m_fieldId == FIELD_NAME )
            msg.Printf( _( "The name of the field cannot be empty." ) );
        else    // the FIELD_VALUE id or REFERENCE_FIELD or VALUE_FIELD
            msg.Printf( _( "The value of the field cannot be empty." ) );
    }
    else if( HasFlag( wxFILTER_EXCLUDE_CHAR_LIST ) && ContainsExcludedCharacters( val ) )
    {
        wxArrayString badCharsFound;

        for( const wxString& excludeChar : GetExcludes() )
        {
            if( val.Find( excludeChar ) != wxNOT_FOUND )
            {
                if( excludeChar == wxT( "\r" ) )
                    badCharsFound.Add( _( "carriage return" ) );
                else if( excludeChar == wxT( "\n" ) )
                    badCharsFound.Add( _( "line feed" ) );
                else if( excludeChar == wxT( "\t" ) )
                    badCharsFound.Add( _( "tab" ) );
                else if( excludeChar == wxT( " " ) )
                    badCharsFound.Add( _( "space" ) );
                else
                    badCharsFound.Add( wxString::Format( wxT( "'%s'" ), excludeChar ) );
            }
        }

        wxString badChars;

        for( size_t i = 0; i < badCharsFound.GetCount(); i++ )
        {
            if( !badChars.IsEmpty() )
            {
                if( badCharsFound.GetCount() == 2 )
                {
                    badChars += _( " or " );
                }
                else
                {
                    if( i < badCharsFound.GetCount() - 2 )
                        badChars += _( ", or " );
                    else
                        badChars += wxT( ", " );
                }
            }

            badChars += badCharsFound.Item( i );
        }

        msg.Printf( fieldCharError, badChars );
    }
    else if( m_fieldId == REFERENCE_FIELD && val.Contains( wxT( "${" ) ) )
    {
        msg.Printf( _( "The reference designator cannot contain text variable references" ) );
    }

    if ( !msg.empty() )
    {
        m_validatorWindow->SetFocus();

        wxMessageBox( msg, _( "Field Validation Error" ), wxOK | wxICON_EXCLAMATION, aParent );

        return false;
    }

    return true;
}


wxString SCH_NETNAME_VALIDATOR::IsValid( const wxString& str ) const
{
    if( NET_SETTINGS::ParseBusGroup( str, nullptr, nullptr ) )
        return wxString();

    if( ( str.Contains( '[' ) || str.Contains( ']' ) ) &&
        !NET_SETTINGS::ParseBusVector( str, nullptr, nullptr ) )
        return _( "Signal name contains '[' or ']' but is not a valid vector bus name" );

    return NETNAME_VALIDATOR::IsValid( str );
}
