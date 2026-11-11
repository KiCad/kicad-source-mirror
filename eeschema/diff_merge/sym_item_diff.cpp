/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 */

#include <diff_merge/sym_item_diff.h>
#include <diff_merge/auto_resolution.h>
#include <diff_merge/property_diff.h>
#include <diff_merge/kicad_merge_engine.h>
#include <diff_merge/sym_lib_differ.h>

#include <lib_symbol.h>
#include <richio.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <sch_field.h>
#include <sch_item.h>
#include <sch_pin.h>

#include <algorithm>
#include <map>
#include <set>


namespace KICAD_DIFF
{

namespace
{
    wxString graphicTypeName( const EDA_ITEM* aItem )
    {
        switch( aItem->Type() )
        {
        case SCH_SHAPE_T:   return _( "Graphic" );
        case SCH_TEXT_T:    return _( "Text" );
        case SCH_TEXTBOX_T: return _( "Text Box" );
        default:            return _( "Item" );
        }
    }


    std::map<wxString, const EDA_ITEM*> pinsByNumber( const LIB_SYMBOL* aSym )
    {
        std::map<wxString, const EDA_ITEM*> out;

        if( aSym )
        {
            for( SCH_PIN* pin : aSym->GetGraphicalPins( ALL_UNITS, ALL_BODY_STYLES ) )
            {
                if( pin )
                    out[pin->GetNumber()] = pin;
            }
        }

        return out;
    }


    std::map<wxString, const EDA_ITEM*> fieldsByName( const LIB_SYMBOL* aSym )
    {
        std::map<wxString, const EDA_ITEM*> out;

        if( aSym )
        {
            std::vector<SCH_FIELD*> fields;
            aSym->GetFields( fields );

            for( SCH_FIELD* field : fields )
            {
                if( field )
                    out[field->GetName()] = field;
            }
        }

        return out;
    }


    std::vector<const EDA_ITEM*> graphics( const LIB_SYMBOL* aSym )
    {
        std::vector<const EDA_ITEM*> out;

        if( aSym )
        {
            for( const SCH_ITEM& item : aSym->GetDrawItems() )
            {
                if( item.Type() != SCH_PIN_T && item.Type() != SCH_FIELD_T )
                    out.push_back( &item );
            }
        }

        return out;
    }
} // namespace


std::vector<LIB_ELEMENT> DiffSymbolElements( const LIB_SYMBOL* aBefore, const LIB_SYMBOL* aAfter )
{
    std::vector<LIB_ELEMENT> out;

    DiffKeyedElements( out, _( "Pin" ), pinsByNumber( aBefore ), pinsByNumber( aAfter ) );
    DiffKeyedElements( out, _( "Field" ), fieldsByName( aBefore ), fieldsByName( aAfter ) );
    DiffUnkeyedElements( out, graphics( aBefore ), graphics( aAfter ), graphicTypeName );

    return out;
}


std::unique_ptr<LIB_SYMBOL> CombineSymbolEdits( const LIB_SYMBOL& aAncestor, const LIB_SYMBOL& aOurs,
                                                const LIB_SYMBOL& aTheirs, const ITEM_CHANGE& aOurChange,
                                                const ITEM_CHANGE& aTheirChange )
{
    // Edits are compared on flattened symbols, so an inherited field cannot be copied back safely
    if( aOurs.IsDerived() || aTheirs.IsDerived() )
        return nullptr;

    auto onlyFields = []( const ITEM_CHANGE& aChange )
    {
        return std::ranges::all_of( aChange.children,
                                    []( const ITEM_CHANGE& aChild )
                                    {
                                        return aChild.typeName == _( "Field" ) && aChild.refdes
                                               && aChild.kind != CHANGE_KIND::REMOVED;
                                    } );
    };

    const bool theirsDonates = onlyFields( aTheirChange );

    if( !theirsDonates && !onlyFields( aOurChange ) )
        return nullptr;

    const std::set<wxString> ourNames = EditedNames( aOurChange );
    const std::set<wxString> theirNames = EditedNames( aTheirChange );

    // A rename moves the symbol to another key, which a field copy cannot follow
    if( ourNames.contains( wxS( "Name" ) ) || theirNames.contains( wxS( "Name" ) ) )
        return nullptr;

    if( std::ranges::any_of( ourNames, [&]( const wxString& aName ) { return theirNames.contains( aName ); } ) )
        return nullptr;

    const LIB_SYMBOL&  donor = theirsDonates ? aTheirs : aOurs;
    const ITEM_CHANGE& donation = theirsDonates ? aTheirChange : aOurChange;

    auto donate = [&]( const LIB_SYMBOL& aBase ) -> std::unique_ptr<LIB_SYMBOL>
    {
        auto target = std::make_unique<LIB_SYMBOL>( aBase );

        for( const ITEM_CHANGE& child : donation.children )
        {
            const SCH_FIELD* from = donor.GetField( *child.refdes );

            if( !from )
                return nullptr;

            if( SCH_FIELD* to = target->GetField( *child.refdes ) )
                *to = *from;
            else
                target->AddField( new SCH_FIELD( *from ) );
        }

        std::vector<PROPERTY_RESOLUTION> props;

        for( const PROPERTY_DELTA& delta : donation.properties )
        {
            PROPERTY_RESOLUTION prop;
            prop.name = delta.name;
            prop.kind = PROP_RES::THEIRS;
            props.push_back( std::move( prop ) );
        }

        if( ApplyPropertyResolutions( target.get(), props, nullptr, &donor, nullptr ).failed > 0 )
            return nullptr;

        return target;
    };

    // The saved form holds everything a symbol carries, including state no diff reports
    auto saved = []( const LIB_SYMBOL& aSymbol )
    {
        LIB_SYMBOL       copy( aSymbol );
        STRING_FORMATTER formatter;

        SCH_IO_KICAD_SEXPR::FormatLibSymbol( &copy, formatter );
        return formatter.GetString();
    };

    // Judge the merge as it will be written, since a symbol can hold state its file drops
    auto reload = [&]( const LIB_SYMBOL& aSymbol ) -> std::unique_ptr<LIB_SYMBOL>
    {
        std::string text = saved( aSymbol );

        try
        {
            std::vector<LIB_SYMBOL*> parsed = SCH_IO_KICAD_SEXPR::ParseLibSymbols( text, "combined symbol" );
            std::vector<std::unique_ptr<LIB_SYMBOL>> owned( parsed.begin(), parsed.end() );

            if( owned.size() == 1 )
                return std::move( owned.front() );

            return nullptr;
        }
        catch( const IO_ERROR& )
        {
            return nullptr;
        }
    };

    std::unique_ptr<LIB_SYMBOL> replay = donate( aAncestor );

    if( !replay || saved( *replay ) != saved( donor ) )
        return nullptr;

    std::unique_ptr<LIB_SYMBOL> merged = donate( theirsDonates ? aOurs : aTheirs );

    if( merged )
        merged = reload( *merged );

    const wxString& name = aAncestor.GetName();

    // A donated property can undo the other side's edit, as a lower unit count drops its pins
    if( !merged
        || !SYM_LIB_DIFFER::MergeKeepsBothSides( { { name, &aAncestor } }, { { name, &aOurs } },
                                                 { { name, &aTheirs } }, { { name, merged.get() } } ) )
    {
        return nullptr;
    }

    return merged;
}

} // namespace KICAD_DIFF
