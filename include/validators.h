/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <memory>

#include <wx/valtext.h>
#include <wx/grid.h>
#include <wx/regex.h>

#include <lib_id.h>


#define FIELD_NAME  -1
#define FIELD_VALUE -2

#define SHEETNAME_V      100    // We can't use SHEETNAME and SHEETFILENAME because they
#define SHEETFILENAME_V  101    //   overlap with REFERENCE_FIELD and VALUE_FIELD
#define SHEETUSERFIELD_V 102

#define LABELUSERFIELD_V 200


/**
 * This class provides a custom wxValidator object for limiting the allowable characters when
 * defining footprint names.  Since the introduction of the PRETTY footprint library format,
 * footprint names cannot have any characters that would prevent file creation on any platform.
 * The characters \/:*?|"<> are illegal and filtered by the validator.
 */
class FOOTPRINT_NAME_VALIDATOR : public wxTextValidator
{
public:
    FOOTPRINT_NAME_VALIDATOR( wxString* aValue = nullptr );
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
    FILE_NAME_WITH_PATH_CHAR_VALIDATOR( wxString* aValue = nullptr );
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
    ENV_VAR_NAME_VALIDATOR( wxString* aValue = nullptr );
    ENV_VAR_NAME_VALIDATOR( const ENV_VAR_NAME_VALIDATOR& val );

    virtual ~ENV_VAR_NAME_VALIDATOR();

    // Make a clone of this validator (or return nullptr) - currently necessary
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
    REGEX_VALIDATOR( const wxString& aRegEx, wxString* aValue = nullptr )
        : wxTextValidator( wxFILTER_NONE, aValue )
    {
        compileRegEx( aRegEx, wxRE_DEFAULT );
    }

    /**
     * @param aRegEx is a regular expression to validate strings.
     * @param aFlags are compilation flags (normally wxRE_DEFAULT).
     * @param aValue is a pointer to a wxString containing the value to validate.
     */
    REGEX_VALIDATOR( const wxString& aRegEx, int aFlags, wxString* aValue = nullptr )
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

    const wxString& GetRegEx() const
    {
        return m_regExString;
    }

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

class NETNAME_VALIDATOR : public wxTextValidator
{
public:
    NETNAME_VALIDATOR( wxString* aVal = nullptr );

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


/**
 * A text control validator used for validating the text allowed in fields.
 *
 * - The reference field does not accept spaces.
 * - The value field does not accept spaces in the symbol library editor because in symbol
 *   libraries, the value field is the symbol name in the library.
 */
class FIELD_VALIDATOR : public wxTextValidator
{
public:
    FIELD_VALIDATOR( int aFieldId, wxString* aValue = nullptr );

    FIELD_VALIDATOR( const FIELD_VALIDATOR& aValidator );

    virtual wxObject* Clone() const override { return new FIELD_VALIDATOR( *this ); }

    /**
     * Override the default Validate() function provided by wxTextValidator to provide
     * better error messages.
     *
     * @param aParent is the parent window of the error message dialog.
     * @return true if the text in the control is valid otherwise false.
     */
    virtual bool Validate( wxWindow* aParent ) override;

    bool DoValidate( const wxString& aValue, wxWindow* aParent );

private:
    int m_fieldId;
};


#endif  // #ifndef VALIDATORS_H
