/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Wayne Stambaugh, stambaughw@gmail.com
 * Copyright (C) 2016-2017 KiCad Developers, see change_log.txt for contributors.
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
#include <sch_connection.h>
#include <sch_validators.h>
#include <template_fieldnames.h>


SCH_FIELD_VALIDATOR::SCH_FIELD_VALIDATOR(  bool aIsLibEditor, int aFieldId, wxString* aValue ) :
    wxTextValidator( wxFILTER_EXCLUDE_CHAR_LIST, aValue )
{
    m_fieldId = aFieldId;
    m_isLibEditor = aIsLibEditor;

    // Fields cannot contain carriage returns, line feeds, or tabs.
    wxString excludes( "\r\n\t" );

    // The reference field cannot contain spaces.
    if( aFieldId == REFERENCE )
        excludes += " ";
    else if( aFieldId == VALUE && m_isLibEditor )
        excludes += " :/\\";

    long style = GetStyle();

    // The reference and value fields cannot be empty.
    if( aFieldId == REFERENCE || aFieldId == VALUE || aFieldId == FIELD_NAME )
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
    case FIELD_NAME:  fieldName = _( "field name" );         break;
    case FIELD_VALUE: fieldName = _( "field value" );        break;
    case REFERENCE:   fieldName = _( "reference field" );    break;
    case VALUE:       fieldName = _( "value field" );        break;
    case FOOTPRINT:   fieldName = _( "footprint field" );    break;
    case DATASHEET:   fieldName = _( "datasheet field" );    break;
    default:          fieldName = _( "user defined field" ); break;
    };

    wxString msg;

    // We can only do some kinds of validation once the input is complete, so
    // check for them here:
    if( HasFlag( wxFILTER_EMPTY ) && val.empty() )
        msg.Printf( _( "The %s cannot be empty." ), fieldName );
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

        msg.Printf( _( "The %s cannot contain %s characters." ), fieldName, badChars );
    }

    if ( !msg.empty() )
    {
        m_validatorWindow->SetFocus();

        wxMessageBox( msg, _( "Field Validation Error" ), wxOK | wxICON_EXCLAMATION, aParent );

        return false;
    }

    return true;
}


SCH_NETNAME_VALIDATOR::SCH_NETNAME_VALIDATOR( wxString *aVal ) :
        wxValidator(), m_allowSpaces( false )
{
}


SCH_NETNAME_VALIDATOR::SCH_NETNAME_VALIDATOR( const SCH_NETNAME_VALIDATOR& aValidator ) :
        m_allowSpaces( aValidator.m_allowSpaces )
{
}


SCH_NETNAME_VALIDATOR::SCH_NETNAME_VALIDATOR( bool aAllowSpaces ) :
        wxValidator(), m_allowSpaces( aAllowSpaces )
{
}


wxTextEntry *SCH_NETNAME_VALIDATOR::GetTextEntry()
{
#if wxUSE_TEXTCTRL
    if( wxDynamicCast( m_validatorWindow, wxTextCtrl ) )
        return static_cast<wxTextCtrl*>( m_validatorWindow );
#endif

#if wxUSE_COMBOBOX
    if( wxDynamicCast( m_validatorWindow, wxComboBox ) )
        return static_cast<wxComboBox*>( m_validatorWindow );
#endif

#if wxUSE_COMBOCTRL
    if( wxDynamicCast( m_validatorWindow, wxComboCtrl ) )
        return static_cast<wxComboCtrl*>( m_validatorWindow );
#endif

    wxFAIL_MSG( "SCH_NETNAME_VALIDATOR can only be used with wxTextCtrl, wxComboBox, or wxComboCtrl" );
    return nullptr;
}


bool SCH_NETNAME_VALIDATOR::Validate( wxWindow *aParent )
{
    // If window is disabled, simply return
    if ( !m_validatorWindow->IsEnabled() )
        return true;

    wxTextEntry * const text = GetTextEntry();

    if ( !text )
        return false;

    const wxString& errormsg = IsValid( text->GetValue() );

    if( !errormsg.empty() )
    {
        m_validatorWindow->SetFocus();
        wxMessageBox( errormsg, _( "Invalid signal name" ), wxOK | wxICON_EXCLAMATION, aParent );
        return false;
    }

    return true;
}


wxString SCH_NETNAME_VALIDATOR::IsValid( const wxString& str ) const
{
    if( SCH_CONNECTION::IsBusGroupLabel( str ) )
        return wxString();

    if( str.Contains( '{' ) || str.Contains( '}' ) )
        return _( "Signal name contains '{' or '}' but is not a valid group bus name" );

    if( ( str.Contains( '[' ) || str.Contains( ']' ) ) &&
        !SCH_CONNECTION::IsBusVectorLabel( str ) )
        return _( "Signal name contains '[' or ']' but is not a valid vector bus name" );

    if( str.Contains( '\r' ) || str.Contains( '\n' ) )
        return _( "Signal names cannot contain CR or LF characters" );

    if( !m_allowSpaces && ( str.Contains( ' ' ) || str.Contains( '\t' ) ) )
        return _( "Signal names cannot contain spaces" );

    return wxString();
}
