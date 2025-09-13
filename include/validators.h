/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <template_fieldnames.h>


/**
 * Provide a custom wxValidator object for limiting the allowable characters when
 * defining footprint names.
 *
 * Since the introduction of the PRETTY footprint library format, footprint names cannot have
 * any characters that would prevent file creation on any platform.  The characters \/:*?|"<>
 * are illegal and filtered by the validator.
 */
class FOOTPRINT_NAME_VALIDATOR : public wxTextValidator
{
public:
    FOOTPRINT_NAME_VALIDATOR( wxString* aValue = nullptr );
};


/**
 * Provide a custom wxValidator object for limiting the allowable characters when defining an
 * environment variable name in a text edit control.
 *
 * Only uppercase, numbers, and underscore (_) characters are valid and the first character of
 * the name cannot start with a number.  This is according to IEEE Std 1003.1-2001.  Even though
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

    /// @return the error message if the contents of @a aVal are invalid.
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
    FIELD_VALIDATOR( FIELD_T aFieldId, wxString* aValue = nullptr );

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
    FIELD_T m_fieldId;
};

/**
 * Return the error message if @a aValue is invalid for @a aFieldId.
 * Returns an empty string when the value is valid.
 */
wxString GetFieldValidationErrorMessage( FIELD_T aFieldId, const wxString& aValue );


#endif  // #ifndef VALIDATORS_H
