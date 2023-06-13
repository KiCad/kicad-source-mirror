/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2019 KiCad Developers, see change_log.txt for contributors.
 * Copyright (C) 2018 CERN
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
 * @file validators.cpp
 * @brief Custom text control validator implementations.
 */

#include <string_utils.h>
#include <confirm.h>
#include <validators.h>
#include <template_fieldnames.h>

#include <wx/grid.h>
#include <wx/textctrl.h>
#include <wx/textentry.h>
#include <wx/log.h>
#include <wx/combo.h>

GRID_CELL_TEXT_EDITOR::GRID_CELL_TEXT_EDITOR() : wxGridCellTextEditor()
{
}


void GRID_CELL_TEXT_EDITOR::SetValidator( const wxValidator& validator )
{
    // keep our own copy because wxGridCellTextEditor's is annoyingly private
    m_validator.reset( static_cast<wxValidator*>( validator.Clone() ) );

    wxGridCellTextEditor::SetValidator( *m_validator );
}


void GRID_CELL_TEXT_EDITOR::StartingKey( wxKeyEvent& event )
{
    if( m_validator )
    {
        m_validator.get()->SetWindow( Text() );
        m_validator.get()->ProcessEvent( event );
    }

    if( event.GetSkipped() )
    {
        wxGridCellTextEditor::StartingKey( event );
        event.Skip( false );
    }
}


FOOTPRINT_NAME_VALIDATOR::FOOTPRINT_NAME_VALIDATOR( wxString* aValue ) :
    wxTextValidator( wxFILTER_EXCLUDE_CHAR_LIST, aValue )
{
    // This list of characters follows the string from footprint.cpp which, in turn mimics the
    // strings from lib_id.cpp
    // TODO: Unify forbidden character lists
    wxString illegalChars = wxS( "%$<>\t\n\r\"\\/:" );
    SetCharExcludes( illegalChars );
 }


FILE_NAME_WITH_PATH_CHAR_VALIDATOR::FILE_NAME_WITH_PATH_CHAR_VALIDATOR( wxString* aValue ) :
    wxTextValidator( wxFILTER_EXCLUDE_CHAR_LIST | wxFILTER_EMPTY, aValue )
{
    // The Windows (DOS) file system forbidden characters already include the forbidden
    // file name characters for both Posix and OSX systems.  The characters *?|"<> are
    // illegal and filtered by the validator, but /\: are valid (\ and : only on Windows.
    wxString illegalChars = wxFileName::GetForbiddenChars( wxPATH_DOS );
    wxTextValidator nameValidator( wxFILTER_EXCLUDE_CHAR_LIST );
    wxArrayString illegalCharList;

    for( unsigned i = 0;  i < illegalChars.size();  i++ )
    {
        if( illegalChars[i] == '/' )
            continue;

#if defined (__WINDOWS__)
        if( illegalChars[i] == '\\' || illegalChars[i] == ':' )
            continue;
#endif
        illegalCharList.Add( wxString( illegalChars[i] ) );
    }

    SetExcludes( illegalCharList );
}


ENV_VAR_NAME_VALIDATOR::ENV_VAR_NAME_VALIDATOR( wxString* aValue ) :
    wxTextValidator()
{
    Connect( wxEVT_CHAR, wxKeyEventHandler( ENV_VAR_NAME_VALIDATOR::OnChar ) );
}


ENV_VAR_NAME_VALIDATOR::ENV_VAR_NAME_VALIDATOR( const ENV_VAR_NAME_VALIDATOR& val )
    : wxTextValidator()
{
    wxValidator::Copy( val );

    Connect( wxEVT_CHAR, wxKeyEventHandler( ENV_VAR_NAME_VALIDATOR::OnChar ) );
}


ENV_VAR_NAME_VALIDATOR::~ENV_VAR_NAME_VALIDATOR()
{
    Disconnect( wxEVT_CHAR, wxKeyEventHandler( ENV_VAR_NAME_VALIDATOR::OnChar ) );
}


void ENV_VAR_NAME_VALIDATOR::OnChar( wxKeyEvent& aEvent  )
{
    if( !m_validatorWindow )
    {
        aEvent.Skip();
        return;
    }

    int keyCode = aEvent.GetKeyCode();

    // we don't filter special keys and delete
    if( keyCode < WXK_SPACE || keyCode == WXK_DELETE || keyCode >= WXK_START )
    {
        aEvent.Skip();
        return;
    }

    wxUniChar c = (wxUChar) keyCode;

    if( c == wxT( '_' ) )
    {
        // OK anywhere
        aEvent.Skip();
    }
    else if( wxIsdigit( c ) )
    {
        // not as first character
        long from, to;
        GetTextEntry()->GetSelection( &from, &to );

        if( from < 1 )
            wxBell();
        else
            aEvent.Skip();
    }
    else if( wxIsalpha( c ) )
    {
        // Capitals only.

        if( wxIslower( c ) )
        {
            // You may wonder why this scope is so twisted, so make yourself comfortable and read:
            // 1. Changing the keyCode and/or uniChar in the event and passing it on
            // doesn't work.  Some platforms look at the original copy as long as the event
            // isn't vetoed.
            // 2. Inserting characters by hand does not move the cursor, meaning either you insert
            // text backwards (lp:#1798869) or always append, no matter where is the cursor.
            // wxTextEntry::{Get/Set}InsertionPoint() do not work at all here.
            // 3. There is wxTextEntry::ForceUpper(), but it is not yet available in common
            // wxWidgets packages.
            //
            // So here we are, with a command event handler that converts
            // the text to upper case upon every change.
            wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( GetTextEntry() );

            if( textCtrl )
            {
                textCtrl->Connect( textCtrl->GetId(), wxEVT_COMMAND_TEXT_UPDATED,
                        wxCommandEventHandler( ENV_VAR_NAME_VALIDATOR::OnTextChanged ) );
            }
        }

        aEvent.Skip();
    }
    else
    {
        wxBell();
    }
}


void ENV_VAR_NAME_VALIDATOR::OnTextChanged( wxCommandEvent& event )
{
    wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( event.GetEventObject() );

    if( textCtrl )
    {
        if( !textCtrl->IsModified() )
            return;

        long insertionPoint = textCtrl->GetInsertionPoint();
        textCtrl->ChangeValue( textCtrl->GetValue().Upper() );
        textCtrl->SetInsertionPoint( insertionPoint );
        textCtrl->Disconnect( textCtrl->GetId(), wxEVT_COMMAND_TEXT_UPDATED );
    }

    event.Skip();
}


bool REGEX_VALIDATOR::Validate( wxWindow* aParent )
{
    // If window is disabled, simply return
    if( !m_validatorWindow->IsEnabled() )
        return true;

    wxTextEntry* const textEntry = GetTextEntry();

    if( !textEntry )
        return false;

    bool valid = true;
    const wxString& value = textEntry->GetValue();

    if( m_regEx.Matches( value ) )
    {
        size_t start, len;
        m_regEx.GetMatch( &start, &len );

        if( start != 0 || len != value.Length() ) // whole string must match
            valid = false;
    }
    else    // no match at all
    {
        valid = false;
    }

    if( !valid )
    {
        m_validatorWindow->SetFocus();
        DisplayError( aParent, wxString::Format( _( "Incorrect value: %s" ), value ) );
        return false;
    }

    return true;
}


void REGEX_VALIDATOR::compileRegEx( const wxString& aRegEx, int aFlags )
{
    if( !m_regEx.Compile( aRegEx, aFlags ) )
    {
        throw std::runtime_error( "REGEX_VALIDATOR: Invalid regular expression: "
                + aRegEx.ToStdString() );
    }

    m_regExString = aRegEx;
    m_regExFlags = aFlags;
}


NETNAME_VALIDATOR::NETNAME_VALIDATOR( wxString *aVal ) :
         wxTextValidator(),
         m_allowSpaces( false )
{
}


NETNAME_VALIDATOR::NETNAME_VALIDATOR( const NETNAME_VALIDATOR& aValidator ) :
        wxTextValidator( aValidator ),
        m_allowSpaces( aValidator.m_allowSpaces )
{
}


NETNAME_VALIDATOR::NETNAME_VALIDATOR( bool aAllowSpaces ) :
        wxTextValidator(),
        m_allowSpaces( aAllowSpaces )
{
}


bool NETNAME_VALIDATOR::Validate( wxWindow *aParent )
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


wxString NETNAME_VALIDATOR::IsValid( const wxString& str ) const
{
    if( str.Contains( '\r' ) || str.Contains( '\n' ) )
        return _( "Signal names cannot contain CR or LF characters" );

    if( !m_allowSpaces && ( str.Contains( ' ' ) || str.Contains( '\t' ) ) )
        return _( "Signal names cannot contain spaces" );

    return wxString();
}


void KIUI::ValidatorTransferToWindowWithoutEvents( wxValidator& aValidator )
{
    wxWindow* ctrl = aValidator.GetWindow();

    wxCHECK_RET( ctrl != nullptr, wxS( "Transferring validator data without a control" ) );

    wxEventBlocker orient_update_blocker( ctrl, wxEVT_ANY );
    aValidator.TransferToWindow();
}


FIELD_VALIDATOR::FIELD_VALIDATOR( int aFieldId, wxString* aValue ) :
        wxTextValidator( wxFILTER_EXCLUDE_CHAR_LIST, aValue ), m_fieldId( aFieldId )
{
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
    if( aFieldId == REFERENCE_FIELD || aFieldId == SHEETNAME_V || aFieldId == SHEETFILENAME_V )
    {
        style |= wxFILTER_EMPTY;
    }

    SetStyle( style );
    SetCharExcludes( excludes );
}


FIELD_VALIDATOR::FIELD_VALIDATOR( const FIELD_VALIDATOR& aValidator ) :
        wxTextValidator( aValidator ), m_fieldId( aValidator.m_fieldId )
{
}


bool FIELD_VALIDATOR::Validate( wxWindow* aParent )
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
                    badCharsFound.Add( wxString::Format( wxT( "'%s'" ), excludeChar ) );
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

    if( !msg.empty() )
    {
        m_validatorWindow->SetFocus();

        wxMessageBox( msg, _( "Field Validation Error" ), wxOK | wxICON_EXCLAMATION, aParent );

        return false;
    }

    return true;
}
