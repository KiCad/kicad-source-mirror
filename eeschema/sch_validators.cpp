/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Wayne Stambaugh, stambaughw@gmail.com
 * Copyright (C) 2016-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

    // The reference, sheetname and sheetfilename fields cannot be empty.
    if( aFieldId == REFERENCE_FIELD
      || aFieldId == SHEETNAME_V
      || aFieldId == SHEETFILENAME_V )
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
    wxString msg;

    if( HasFlag( wxFILTER_EMPTY ) && val.empty() )
        msg.Printf( _( "The value of the field cannot be empty." ) );

    if( HasFlag( wxFILTER_EXCLUDE_CHAR_LIST ) && ContainsExcludedCharacters( val ) )
    {
        wxArrayString badCharsFound;

#if wxCHECK_VERSION( 3, 1, 3 )
        for( const wxUniCharRef& excludeChar : GetCharExcludes() )
        {
            if( val.Find( excludeChar ) != wxNOT_FOUND )
            {
                if( excludeChar == '\r' )
                    badCharsFound.Add( _( "carriage return" ) );
                else if( excludeChar == '\n' )
                    badCharsFound.Add( _( "line feed" ) );
                else if( excludeChar == '\t' )
                    badCharsFound.Add( _( "tab" ) );
                else if( excludeChar == ' ' )
                    badCharsFound.Add( _( "space" ) );
                else
                    badCharsFound.Add( wxString::Format( wxT( "'%c'" ), excludeChar ) );
            }
        }
#else
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
#endif

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

        switch( m_fieldId )
        {
        case REFERENCE_FIELD:
            msg.Printf( _( "The reference designator cannot contain %s character(s)." ), badChars );
            break;

        case VALUE_FIELD:
            msg.Printf( _( "The value field cannot contain %s character(s)." ), badChars );
            break;

        case FOOTPRINT_FIELD:
            msg.Printf( _( "The footprint field cannot contain %s character(s)." ), badChars );
            break;

        case DATASHEET_FIELD:
            msg.Printf( _( "The datasheet field cannot contain %s character(s)." ), badChars );
            break;

        case SHEETNAME_V:
            msg.Printf( _( "The sheet name cannot contain %s character(s)." ), badChars );
            break;

        case SHEETFILENAME_V:
            msg.Printf( _( "The sheet filename cannot contain %s character(s)." ), badChars );
            break;

        default:
            msg.Printf( _( "The field cannot contain %s character(s)." ), badChars );
            break;
        };
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


// Match opening curly brace, preceeded by start-of-line or by a character not including $_^~
wxRegEx SCH_NETNAME_VALIDATOR::m_busGroupRegex( R"((^|[^$_\^~]){)", wxRE_ADVANCED );


wxString SCH_NETNAME_VALIDATOR::IsValid( const wxString& str ) const
{
    wxString msg = NETNAME_VALIDATOR::IsValid( str );

    if( !msg.IsEmpty() )
        return msg;

    // We don't do single-character validation here
    if( str.Length() == 1 )
        return wxString();

    // Figuring out if the user "meant" to make a bus group is somewhat tricky because curly
    // braces are also used for formatting and variable expansion

    if( m_busGroupRegex.Matches( str ) && str.Contains( '}' ) )
    {
        if( !NET_SETTINGS::ParseBusGroup( str, nullptr, nullptr ) )
            return _( "Signal name contains '{' and '}' but is not a valid bus name" );
    }
    else if( str.Contains( '[' ) || str.Contains( ']' ) )
    {
        if( !NET_SETTINGS::ParseBusVector( str, nullptr, nullptr ) )
            return _( "Signal name contains '[' or ']' but is not a valid bus name." );
    }

    return wxString();
}
