/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Wayne Stambaugh, stambaughw@gmail.com
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file sch_validators.h
 * @brief Definitions of control validators for schematic dialogs.
 */

#ifndef _SCH_VALIDATORS_H_
#define _SCH_VALIDATORS_H_

#include <wx/valtext.h>
#include <validators.h>

/*
 * A refinement of the NETNAME_VALIDATOR which also allows (and checks) bus definitions.
 */
class SCH_NETNAME_VALIDATOR : public NETNAME_VALIDATOR
{
public:
    SCH_NETNAME_VALIDATOR( wxString* aVal = nullptr ) :
            NETNAME_VALIDATOR( aVal )
    { }

    SCH_NETNAME_VALIDATOR( bool aAllowSpaces ) :
            NETNAME_VALIDATOR( aAllowSpaces )
    { }

    SCH_NETNAME_VALIDATOR( const SCH_NETNAME_VALIDATOR& aValidator ) :
            NETNAME_VALIDATOR( aValidator )
    { }

    virtual wxObject* Clone() const override { return new SCH_NETNAME_VALIDATOR( *this ); }

    /// @return the error message if the contents of \a aVal are invalid.
    wxString IsValid( const wxString& aVal ) const override;

private:
    static wxRegEx m_busGroupRegex;
};

#endif // _SCH_VALIDATORS_H_
