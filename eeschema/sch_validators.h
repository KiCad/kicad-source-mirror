/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Wayne Stambaugh, stambaughw@gmail.com
 * Copyright (C) 2016-2021 KiCad Developers, see change_log.txt for contributors.
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
 * @file sch_validators.h
 * @brief Definitions of control validators for schematic dialogs.
 */

#ifndef _SCH_VALIDATORS_H_
#define _SCH_VALIDATORS_H_

#include <wx/valtext.h>
#include <validators.h>

#define FIELD_NAME  -1
#define FIELD_VALUE -2

#define SHEETNAME_V      100    // We can't use SHEETNAME and SHEETFILENAME because they
#define SHEETFILENAME_V  101    //   overlap with REFERENCE_FIELD and VALUE_FIELD
#define SHEETUSERFIELD_V 102

/**
 * A text control validator used for validating the text allowed in library and
 * schematic symbol fields.
 *
 * - The reference field does not accept spaces.
 * - The value field does not accept spaces in the symbol library editor because in symbol
 *   libraries, the value field is the symbol name in the library.
 */
class SCH_FIELD_VALIDATOR : public wxTextValidator
{
public:
    SCH_FIELD_VALIDATOR( bool aIsLibEditor, int aFieldId, wxString* aValue = nullptr );

    SCH_FIELD_VALIDATOR( const SCH_FIELD_VALIDATOR& aValidator );

    virtual wxObject* Clone() const override { return new SCH_FIELD_VALIDATOR( *this ); }

    /**
     * Override the default Validate() function provided by wxTextValidator to provide
     * better error messages.
     *
     * @param aParent is the parent window of the error message dialog.
     * @return true if the text in the control is valid otherwise false.
     */
    virtual bool Validate( wxWindow *aParent ) override;

private:
    int  m_fieldId;
    bool m_isLibEditor;
};


/*
 * A refinement of the NETNAME_VALIDATOR which also allows (and checks) bus definitions.
 */
class SCH_NETNAME_VALIDATOR : public NETNAME_VALIDATOR
{
public:
    SCH_NETNAME_VALIDATOR( wxString *aVal = nullptr ) :
            NETNAME_VALIDATOR( aVal )
    { }

    SCH_NETNAME_VALIDATOR( bool aAllowSpaces ) :
            NETNAME_VALIDATOR( aAllowSpaces )
    { }

    SCH_NETNAME_VALIDATOR( const SCH_NETNAME_VALIDATOR& aValidator ) :
            NETNAME_VALIDATOR( aValidator )
    { }

    virtual wxObject* Clone() const override { return new SCH_NETNAME_VALIDATOR( *this ); }

protected:
    /// @return the error message if the contents of \a aVal are invalid.
    wxString IsValid( const wxString& aVal ) const override;

private:
    static wxRegEx m_busGroupRegex;
};

#endif // _SCH_VALIDATORS_H_
