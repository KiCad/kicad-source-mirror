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
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "mdw_part_field.h"

#include <sch_field.h>
#include <sch_symbol.h>


void MdwWritePartField( SCH_SYMBOL& aSymbol, const MDW_PART_IDENTITY& aPart )
{
    const wxString name = wxString::FromUTF8( MDW_PART_FIELD );
    SCH_FIELD*     field = aSymbol.GetField( name );

    if( !field )
    {
        field = aSymbol.AddField( SCH_FIELD( &aSymbol, FIELD_T::USER, name ) );
        field->SetTextPos( aSymbol.GetPosition() );
    }

    // Identity only, so never shown even where a user made the field visible
    field->SetVisible( false );
    field->SetText( MdwFormatPartField( aPart ) );
}


std::optional<MDW_PART_IDENTITY> MdwReadPartField( const SCH_SYMBOL& aSymbol )
{
    const SCH_FIELD* field = aSymbol.GetField( wxString::FromUTF8( MDW_PART_FIELD ) );

    if( !field )
        return std::nullopt;

    return MdwParsePartField( field->GetText() );
}
