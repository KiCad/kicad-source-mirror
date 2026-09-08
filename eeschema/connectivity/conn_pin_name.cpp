/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "conn_pin_name.h"

#include <string_utils.h>
#include <kiid.h>

namespace SCH_CONNECTIVITY
{
wxString RenderPinNetName( const PIN_NAME_FACT& aPin, const PIN_NAME_REFERENCE& aReference,
                          bool aForceNoConnect )
{
    const bool unconnected = aForceNoConnect || aPin.noConnect;
    wxString name = unconnected ? "unconnected-(" : "Net-(";

    if( aReference.reference.IsEmpty() || aReference.reference.Last() == '?' )
    {
        wxString number = aPin.number;

        if( aPin.padNumber != aPin.shownNumber && !aPin.padNumber.IsEmpty() )
            number = aPin.padNumber;

        if( aReference.reference.IsEmpty() )
            name << aReference.symbolUuid;
        else
            name << aReference.reference
                 << wxString::Format( wxS( "-%08x" ),
                                      static_cast<unsigned>( KIID( aReference.symbolUuid ).Hash() & 0xFFFFFFFF ) );

        name << "-Pad" << number << ")";
    }
    else if( !aPin.name.IsEmpty() && aPin.name != aPin.shownNumber )
    {
        // Pin names may repeat across units; pad numbers are unique across the symbol.
        name << aReference.referenceWithUnit << "-" << EscapeString( aPin.name, CTX_NETNAME );

        if( unconnected || aPin.hasDuplicateName )
            name << "-Pad" << EscapeString( aPin.padNumber, CTX_NETNAME );

        name << ")";
    }
    else
    {
        name << aReference.reference << "-Pad" << EscapeString( aPin.padNumber, CTX_NETNAME ) << ")";
    }

    return name;
}
}
