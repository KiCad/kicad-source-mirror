/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "pads_sch_binary_parser.h"

#include "pads_sch_sdb.h"

#include <algorithm>
#include <set>

#include <ki_exception.h>

namespace PADS_SCH_BINARY
{

namespace
{

    template <typename Item>
    bool idsAreUnique( const std::vector<Item>& aItems )
    {
        std::set<decltype( aItems.front().id )> ids;

        for( const Item& item : aItems )
        {
            if( !item.id.IsValid() || !ids.insert( item.id ).second )
                return false;
        }

        return true;
    }


    template <typename Item, typename Id>
    bool containsId( const std::vector<Item>& aItems, Id aId )
    {
        return std::ranges::any_of( aItems,
                                    [&]( const Item& aItem )
                                    {
                                        return aItem.id == aId;
                                    } );
    }


    [[noreturn]] void throwValidationError( const SOURCE_PROVENANCE& aSource, const wxString& aMessage )
    {
        THROW_IO_ERROR( wxString::Format( wxS( "%s at offset 0x%llX, controller %d, sheet %d: %s" ), aSource.file,
                                          static_cast<unsigned long long>( aSource.offset ), aSource.controller,
                                          aSource.sheet, aMessage ) );
    }

} // namespace


bool PADS_SCH_MODEL::HasUniqueTypedIds() const
{
    if( !idsAreUnique( sheets ) || !idsAreUnique( definitions ) || !idsAreUnique( partTypes )
        || !idsAreUnique( placements ) || !idsAreUnique( nets ) || !idsAreUnique( buses ) )
    {
        return false;
    }

    std::vector<MODEL_GATE>           gates;
    std::vector<MODEL_PIN_DEFINITION> pins;

    for( const MODEL_PART_TYPE& partType : partTypes )
        gates.insert( gates.end(), partType.gates.begin(), partType.gates.end() );

    for( const MODEL_SYMBOL_DEFINITION& definition : definitions )
        pins.insert( pins.end(), definition.pins.begin(), definition.pins.end() );

    return idsAreUnique( gates ) && idsAreUnique( pins );
}


bool PADS_SCH_MODEL::AllReferencesResolved() const
{
    for( const MODEL_SHEET& sheet : sheets )
    {
        if( sheet.parent && !containsId( sheets, *sheet.parent ) )
            return false;
    }

    for( const MODEL_PART_TYPE& partType : partTypes )
    {
        for( const MODEL_GATE& gate : partType.gates )
        {
            auto definition = std::ranges::find_if( definitions,
                                                    [&]( const MODEL_SYMBOL_DEFINITION& aDefinition )
                                                    {
                                                        return aDefinition.id == gate.definition;
                                                    } );

            if( definition == definitions.end() )
                return false;

            for( PIN_ID pin : gate.pins )
            {
                if( !containsId( definition->pins, pin ) )
                    return false;
            }
        }
    }

    for( const MODEL_PLACEMENT& placement : placements )
    {
        if( !containsId( sheets, placement.sheet ) || !containsId( partTypes, placement.partType ) )
            return false;

        if( placement.gate )
        {
            auto partType = std::ranges::find_if( partTypes,
                                                  [&]( const MODEL_PART_TYPE& aPartType )
                                                  {
                                                      return aPartType.id == placement.partType;
                                                  } );

            if( partType == partTypes.end() || !containsId( partType->gates, *placement.gate ) )
                return false;
        }
    }

    for( const MODEL_NET& net : nets )
    {
        if( !containsId( sheets, net.sheet ) )
            return false;

        for( const MODEL_CONNECTION& connection : net.connections )
        {
            for( const MODEL_CONNECTION_ENDPOINT& endpoint : connection.endpoints )
            {
                if( endpoint.placement && !containsId( placements, *endpoint.placement ) )
                    return false;
            }
        }
    }

    for( const MODEL_BUS& bus : buses )
    {
        if( !containsId( sheets, bus.sheet ) )
            return false;
    }

    for( const MODEL_LABEL& label : labels )
    {
        if( !containsId( sheets, label.sheet ) )
            return false;
    }

    for( const MODEL_JUNCTION& junction : junctions )
    {
        if( !containsId( sheets, junction.sheet ) )
            return false;
    }

    for( const MODEL_TEXT& text : texts )
    {
        if( !containsId( sheets, text.sheet ) )
            return false;
    }

    return true;
}


void PADS_SCH_MODEL::ValidateOrThrow() const
{
    if( !HasUniqueTypedIds() )
        throwValidationError( source, wxS( "duplicate or invalid typed controller ID" ) );

    if( !AllReferencesResolved() )
        throwValidationError( source, wxS( "unresolved or wrong-class controller reference" ) );

    for( const MODEL_SHEET& sheet : sheets )
    {
        std::set<SHEET_ID> ancestors;
        const MODEL_SHEET* current = &sheet;

        while( current->parent )
        {
            if( !ancestors.insert( current->id ).second )
                throwValidationError( current->source, wxS( "cyclic sheet hierarchy" ) );

            auto parent = std::ranges::find_if( sheets,
                                                [&]( const MODEL_SHEET& aSheet )
                                                {
                                                    return aSheet.id == *current->parent;
                                                } );

            if( parent == sheets.end() )
                break;

            current = &*parent;
        }
    }
}


PADS_SCH_MODEL PADS_SCH_BINARY_PARSER::Parse( const std::vector<uint8_t>& aBytes, const wxString& aSourceName ) const
{
    PADS_SCH_SDB sdb;
    sdb.Load( aBytes );

    PADS_SCH_MODEL model;
    model.version = sdb.Version();
    model.source = { aSourceName, 0, aBytes.size(), -1, -1 };
    model.settings.source = model.source;

    for( const SCH_SDB_BLOCK& block : sdb.Blocks() )
    {
        if( block.kind != SCH_SDB_BLOCK_KIND::SHEET )
            continue;

        const size_t      index = model.sheets.size();
        SOURCE_PROVENANCE provenance{ aSourceName, block.offset, block.bytes, block.controller,
                                      static_cast<int>( index ) };
        SOURCE_STRING     name{ {}, {}, STRING_ENCODING_STATUS::UTF8, provenance };
        model.sheets.push_back(
                { SHEET_ID( static_cast<uint32_t>( index ) ), index, provenance, std::move( name ), std::nullopt } );
    }

    model.ValidateOrThrow();
    return model;
}

} // namespace PADS_SCH_BINARY
