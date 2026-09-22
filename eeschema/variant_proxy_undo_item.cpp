/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
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

#include <variant_proxy_undo_item.h>
#include <sch_edit_frame.h>


VARIANT_PROXY_UNDO_ITEM::VARIANT_PROXY_UNDO_ITEM( const SCHEMATIC* aSchematic ) :
        EDA_ITEM( WS_PROXY_UNDO_ITEM_T )
{
    for( const wxString& variant : aSchematic->GetVariantNames() )
        m_variants[variant] = aSchematic->GetVariantDescription( variant );
}


void VARIANT_PROXY_UNDO_ITEM::Restore( SCHEMATIC* aSchematic )
{
    std::set<wxString> currentVariants = aSchematic->GetVariantNames();

    for( const auto& [variantName, variantDescription] : m_variants )
    {
        if( currentVariants.contains( variantName ) )
            currentVariants.erase( variantName );
        else
            aSchematic->AddVariant( variantName );

        aSchematic->SetVariantDescription( variantName, variantDescription );
    }

    for( const wxString& excessVariant : currentVariants )
        aSchematic->DeleteVariant( excessVariant );
}

