/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2021 KiCad Developers, see change_log.txt for contributors.
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
 * @file validators.h
 * @brief Custom text control validator definitions.
 */

#ifndef VALIDATORS_H
#define VALIDATORS_H

#include <wx/valtext.h>
#include <wx/grid.h>
#include <wx/regex.h>

#include <lib_id.h>

/**
 * This class works around a bug in wxGrid where the first keystroke doesn't get sent through
 * the validator if the editor wasn't already open.
 */
class GRID_CELL_TEXT_EDITOR : public wxGridCellTextEditor
{
public:
    GRID_CELL_TEXT_EDITOR();

    virtual void SetValidator(const wxValidator& validator) override;
    virtual void StartingKey(wxKeyEvent& event) override;

protected:
    wxScopedPtr<wxValidator> m_validator;
};


/**
 * This class provides a custom wxValidator object for limiting the allowable characters when
 * defining footprint names.  Since the introduction of the PRETTY footprint library format,
 * footprint names cannot have any characters that would prevent file creation on any platform.
 * The characters \/:*?|"<> are illegal and filtered by the validator.
 */
class FOOTPRINT_NAME_VALIDATOR : public wxTextValidator
{
public:
    FOOTPRINT_NAME_VALIDATOR( wxString* aValue = NULL );
};


/**
 * This class provides a custom wxValidator object for limiting the allowable characters when
 * defining file names with path, for instance in schematic sheet file names.
 * The characters *?|"<> are illegal and filtered by the validator,
 * but /\: are valid (\ and : only on Windows.)
 */
class FILE_NAME_WITH_PATH_CHAR_VALIDATOR : public wxTextValidator
{
public:
    FILE_NAME_WITH_PATH_CHAR_VALIDATOR( wxString* aValue = NULL );
};


/**
 * This class provides a custom wxValidator object for limiting the allowable characters
 * when defining an environment variable name in a text edit control.  Only uppercase,
 * numbers, and underscore (_) characters are valid and the first character of the name
 * cannot start with a number.  This is according to IEEE Std 1003.1-2001.  Even though
 * most systems support other characters, these characters guarantee compatibility for
 * all shells.
 */
class ENV_VAR_NAME_VALIDATOR : public wxTextValidator
{
public:
    ENV_VAR_NAME_VALIDATOR( wxString* aValue = NULL );
    ENV_VAR_NAME_VALIDATOR( const ENV_VAR_NAME_VALIDATOR& val );

    virtual ~ENV_VAR_NAME_VALIDATOR();

    // Make a clone of this validator (or return NULL) - currently necessary
    // if you're passing a reference to a validator.
    virtual wxObject *Clone() const override
    {
        return new ENV_VAR_NAME_VALIDATOR( *this );
    }

    void OnChar( wxKeyEvent& event );

    void OnTextChanged( wxCommandEvent& event );
};


/**
 * Custom validator that checks verifies that a string *exactly* matches a
 * regular expression.
 */
class REGEX_VALIDATOR : public wxTextValidator
{
public:
    /**
     * @param aRegEx is a regular expression to validate strings.
     * @param aValue is a pointer to a wxString containing the value to validate.
     */
    REGEX_VALIDATOR( const wxString& aRegEx, wxString* aValue = NULL )
        : wxTextValidator( wxFILTER_NONE, aValue )
    {
        compileRegEx( aRegEx, wxRE_DEFAULT );
    }

    /**
     * @param aRegEx is a regular expression to validate strings.
     * @param aFlags are compilation flags (normally wxRE_DEFAULT).
     * @param aValue is a pointer to a wxString containing the value to validate.
     */
    REGEX_VALIDATOR( const wxString& aRegEx, int aFlags, wxString* aValue = NULL )
        : wxTextValidator( wxFILTER_NONE, aValue )
    {
        compileRegEx( aRegEx, aFlags );
    }

    REGEX_VALIDATOR( const REGEX_VALIDATOR& aOther ) : wxTextValidator( aOther )
    {
        compileRegEx( aOther.m_regExString, aOther.m_regExFlags );
    }

    virtual wxObject* Clone() const override
    {
        return new REGEX_VALIDATOR( *this );
    }

    bool Validate( wxWindow* aParent ) override;

protected:
    ///< Compiles and stores a regular expression
    void compileRegEx( const wxString& aRegEx, int aFlags );

    ///< Original regular expression (for copy constructor)
    wxString m_regExString;

    ///< Original compilation flags (for copy constructor)
    int m_regExFlags;

    ///< Compiled regex
    wxRegEx m_regEx;
};

/**
 * Custom validator that verifies that a string defines a valid #LIB_ID.
 *
 * The default validation allows empty #LIB_ID strings to allow the #LIB_ID to be cleared.
 * Use SetStyle( wxFILTER_EMPTY ) to force a valid #LIB_ID string.
 */
class LIB_ID_VALIDATOR : public wxTextValidator
{
public:
    /**
     * @param aLibIdType is the type of #LIB_ID object to validate.
     * @param aValue is a pointer to a wxString containing the value to validate.
     */
    LIB_ID_VALIDATOR( wxString* aValue = NULL ) :
        wxTextValidator( wxFILTER_EXCLUDE_CHAR_LIST, aValue )
    {
        SetCharExcludes( "\r\n\t" );
    }

    virtual wxObject* Clone() const override
    {
        return new LIB_ID_VALIDATOR( *this );
    }

    bool Validate( wxWindow* aParent ) override;
};


class NETNAME_VALIDATOR : public wxTextValidator
{
public:
    NETNAME_VALIDATOR( wxString *aVal = nullptr );

    NETNAME_VALIDATOR( bool aAllowSpaces );

    NETNAME_VALIDATOR( const NETNAME_VALIDATOR& aValidator );

    virtual wxObject* Clone() const override { return new NETNAME_VALIDATOR( *this ); }

    virtual bool TransferToWindow() override { return true; }

    virtual bool TransferFromWindow() override { return true; }

    virtual bool Validate( wxWindow *aParent ) override;

protected:
    // returns the error message if the contents of 'val' are invalid
    wxString IsValid( const wxString& aVal ) const override;

private:
    bool m_allowSpaces;
};


namespace KIUI
{
/**
 * Call a text validator's TransferDataToWindow method without firing
 * a text change event.
 *
 * This is useful when you want to keep a validator in sync with other data,
 * but the act of changing it should not trigger other updates. It is the
 * validator equivalent of ChangeValue() compared to SetValue().
 *
 * This function blocks all events, but the same technique can be used to
 * selectively block events.
 *
 * @param aValidator the validator to update the control of
 */
void ValidatorTransferToWindowWithoutEvents( wxValidator& aValidator );

} // namespace KIUI

#endif  // #ifndef VALIDATORS_H
