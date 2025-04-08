/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Jon Evans <jon@craftyjon.com>
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
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sch_pin.h>
#include <lib_symbol.h>
#include <sch_bitmap.h>
#include <sch_bus_entry.h>
#include <sch_field.h>
#include <sch_group.h>
#include <sch_junction.h>
#include <sch_label.h>
#include <sch_line.h>
#include <sch_no_connect.h>
#include <sch_shape.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <sch_table.h>
#include <sch_tablecell.h>
#include <sch_text.h>
#include <sch_textbox.h>

#include "api_sch_utils.h"


std::unique_ptr<EDA_ITEM> CreateItemForType( KICAD_T aType, EDA_ITEM* aContainer )
{
    SCH_ITEM* parentSchItem = dynamic_cast<SCH_ITEM*>( aContainer );

    switch( aType )
    {
    case SCH_JUNCTION_T:        return std::make_unique<SCH_JUNCTION>();
    case SCH_NO_CONNECT_T:      return std::make_unique<SCH_NO_CONNECT>();
    case SCH_BUS_WIRE_ENTRY_T:  return std::make_unique<SCH_BUS_WIRE_ENTRY>();
    case SCH_BUS_BUS_ENTRY_T:   return std::make_unique<SCH_BUS_BUS_ENTRY>();
    case SCH_LINE_T:            return std::make_unique<SCH_LINE>();
    case SCH_SHAPE_T:           return std::make_unique<SCH_SHAPE>();
    case SCH_BITMAP_T:          return std::make_unique<SCH_BITMAP>();
    case SCH_TEXTBOX_T:         return std::make_unique<SCH_TEXTBOX>();
    case SCH_TEXT_T:            return std::make_unique<SCH_TEXT>();
    case SCH_TABLE_T:           return std::make_unique<SCH_TABLE>();
    case SCH_TABLECELL_T:       return std::make_unique<SCH_TABLECELL>();
    case SCH_LABEL_T:           return std::make_unique<SCH_LABEL>();
    case SCH_GLOBAL_LABEL_T:    return std::make_unique<SCH_GLOBALLABEL>();
    case SCH_HIER_LABEL_T:      return std::make_unique<SCH_HIERLABEL>();
    case SCH_DIRECTIVE_LABEL_T: return std::make_unique<SCH_DIRECTIVE_LABEL>();
    case SCH_FIELD_T:           return std::make_unique<SCH_FIELD>( parentSchItem );
    case SCH_GROUP_T:           return std::make_unique<SCH_GROUP>();

    case SCH_SYMBOL_T:
    {
        // TODO: constructing currently requires more than just a "container" LIB_SYMBOL
        return nullptr;
    }

    case SCH_SHEET_PIN_T:
    {
        if( aContainer && aContainer->Type() == SCH_SHEET_T )
            return std::make_unique<SCH_SHEET_PIN>( static_cast<SCH_SHEET*>( aContainer ) );

        return nullptr;
    }

    case SCH_SHEET_T:           return std::make_unique<SCH_SHEET>();

    case SCH_PIN_T:
    {
        if( aContainer && aContainer->Type() == LIB_SYMBOL_T )
            return std::make_unique<SCH_PIN>( static_cast<LIB_SYMBOL*>( aContainer ) );

        return nullptr;
    }

    case LIB_SYMBOL_T:          return nullptr; // TODO: ctor currently requires non-null name

    default:
        return nullptr;
    }
}
