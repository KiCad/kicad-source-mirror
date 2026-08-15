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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <algorithm>
#include <set>
#include <trace_helpers.h>

#include <sch_pin.h>
#include <lib_symbol.h>
#include <schematic.h>
#include <sch_symbol.h>
#include <sch_bitmap.h>
#include <sch_bus_entry.h>
#include <sch_field.h>
#include <sch_group.h>
#include <sch_junction.h>
#include <sch_label.h>
#include <sch_line.h>
#include <sch_no_connect.h>
#include <sch_rule_area.h>
#include <sch_shape.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_screen.h>
#include <sch_sheet_pin.h>
#include <sch_table.h>
#include <sch_tablecell.h>
#include <sch_text.h>
#include <sch_textbox.h>

#include "api_sch_utils.h"

#include <api/api_utils.h>
#include <api/api_enums.h>


using namespace kiapi::common;


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
    case SCH_RULE_AREA_T:       return std::make_unique<SCH_RULE_AREA>();
    case SCH_FIELD_T:           return std::make_unique<SCH_FIELD>( parentSchItem );
    case SCH_GROUP_T:           return std::make_unique<SCH_GROUP>();
    case SCH_SYMBOL_T:          return std::make_unique<SCH_SYMBOL>();
    case LIB_SYMBOL_T:          return std::make_unique<LIB_SYMBOL>( wxEmptyString );
    case SCH_SHEET_T:
    {
        if( aContainer && aContainer->Type() == SCH_SCREEN_T )
            return std::make_unique<SCH_SHEET>( static_cast<SCH_SCREEN*>( aContainer ) );

        return nullptr;
    }


    case SCH_SHEET_PIN_T:
        if( aContainer && aContainer->Type() == SCH_SHEET_T )
            return std::make_unique<SCH_SHEET_PIN>( static_cast<SCH_SHEET*>( aContainer ) );

        return nullptr;

    case SCH_PIN_T:
        if( aContainer && aContainer->Type() == LIB_SYMBOL_T )
            return std::make_unique<SCH_PIN>( static_cast<LIB_SYMBOL*>( aContainer ) );

        return nullptr;

    default:
        return nullptr;
    }
}


void PackPinMapOverride( kiapi::schematic::types::PinMapInstanceOverride* aOutput,
                         const PIN_MAP_INSTANCE_OVERRIDE&                 aOverride )
{
    aOutput->Clear();
    aOutput->set_mode(
            ToProtoEnum<PIN_MAP_OVERRIDE_MODE, kiapi::schematic::types::PinMapOverrideMode>( aOverride.m_Mode ) );
    aOutput->set_active_map_name( aOverride.m_ActiveMapName.ToUTF8() );

    for( const PIN_MAP_ENTRY& edit : aOverride.m_Edits )
    {
        kiapi::schematic::types::PinMapEntry* e = aOutput->add_edits();
        e->set_pin_number( edit.m_PinNumber.ToUTF8() );
        e->set_pad_number( edit.m_PadNumber.ToUTF8() );
    }
}


PIN_MAP_INSTANCE_OVERRIDE UnpackPinMapOverride( const kiapi::schematic::types::PinMapInstanceOverride& aInput )
{
    PIN_MAP_INSTANCE_OVERRIDE override;
    override.m_Mode = FromProtoEnum<PIN_MAP_OVERRIDE_MODE>( aInput.mode() );
    override.m_ActiveMapName = wxString::FromUTF8( aInput.active_map_name() );

    for( const kiapi::schematic::types::PinMapEntry& e : aInput.edits() )
    {
        override.m_Edits.push_back( { wxString::FromUTF8( e.pin_number() ), wxString::FromUTF8( e.pad_number() ) } );
    }

    return override;
}


bool PackSymbol( kiapi::schematic::types::SchematicSymbolInstance* aOutput, const SCH_SYMBOL* aInput,
                 const SCH_SHEET_PATH& aPath )
{
    KIID_PATH path = aPath.Path();
    SCH_SYMBOL_INSTANCE instance;

    if( !aInput->GetInstance( instance, path ) )
    {
        wxLogTrace( traceApi, "error: instance data for symbol %s on %s is missing",
                     aInput->m_Uuid.AsString(), aPath.PathHumanReadable() );
        return false;
    }

    google::protobuf::Any any;
    aInput->Serialize( any );

    if( !any.UnpackTo( aOutput ) )
        return false;

    PackSheetPath( *aOutput->mutable_path(), path );
    aOutput->mutable_reference_field()->mutable_text()->set_text( instance.m_Reference.ToUTF8() );
    aOutput->mutable_unit()->set_unit( instance.m_Unit );

    kiapi::schematic::types::SchematicSymbol* def = aOutput->mutable_definition();

    std::vector<const SCH_PIN*> pins = aInput->GetPins( &aPath );

    std::ranges::sort( pins,
                       []( const SCH_PIN* a, const SCH_PIN* b )
                       {
                           return a->m_Uuid < b->m_Uuid;
                       } );

    for( const SCH_PIN* pin : pins )
    {
        kiapi::schematic::types::SchematicSymbolChild* item = def->add_items();
        item->mutable_unit()->set_unit( pin->GetUnit() );
        item->mutable_body_style()->set_style( pin->GetBodyStyle() );
        item->set_is_private( pin->IsPrivate() );
        pin->Serialize( *item->mutable_item() );
    }

    if( const LIB_SYMBOL* lib = aInput->GetLibSymbolRef().get() )
    {
        kiapi::schematic::types::SymbolPinMaps* pinMaps = def->mutable_pin_maps();

        for( const ASSOCIATED_FOOTPRINT& assoc : lib->GetEffectiveAssociatedFootprints() )
        {
            kiapi::schematic::types::AssociatedFootprint* a = pinMaps->add_associated_footprints();
            PackLibId( a->mutable_footprint(), assoc.m_FootprintLibId );
            a->set_map_name( assoc.m_MapName.ToUTF8() );
        }

        for( const PIN_MAP& map : lib->GetEffectivePinMaps().GetAll() )
        {
            kiapi::schematic::types::PinMap* m = pinMaps->add_pin_maps();
            m->set_name( map.GetName().ToUTF8() );

            for( const PIN_MAP_ENTRY& entry : map.GetEntries() )
            {
                kiapi::schematic::types::PinMapEntry* e = m->add_entries();
                e->set_pin_number( entry.m_PinNumber.ToUTF8() );
                e->set_pad_number( entry.m_PadNumber.ToUTF8() );
            }
        }
    }

    PIN_MAP_INSTANCE_OVERRIDE override = aInput->GetPinMapOverride( &aPath );

    if( !override.IsDefault() )
        PackPinMapOverride( aOutput->mutable_pin_map_override(), override );

    kiapi::schematic::types::SchematicSymbolAttributes* attributes = aOutput->mutable_attributes();

    attributes->set_exclude_from_simulation( instance.m_ExcludedFromSim );
    attributes->set_exclude_from_bill_of_materials( instance.m_ExcludedFromBOM );
    attributes->set_exclude_from_board( instance.m_ExcludedFromBoard );
    attributes->set_exclude_from_position_files( instance.m_ExcludedFromPosFiles );
    attributes->set_do_not_populate( instance.m_DNP );

    // Descriptions belong to the schematic's variant registry rather than to each record.
    SCHEMATIC* schematic = aInput->Schematic();

    // Always set, so that sending the response back describes the placement's variants in full.
    kiapi::schematic::types::SchematicSymbolVariants* variants = aOutput->mutable_variants();

    for( const auto& [name, variantInfo] : instance.m_Variants )
    {
        kiapi::schematic::types::SchematicSymbolVariant* variant = variants->add_variants();
        variant->set_name( name.ToUTF8() );

        if( schematic )
            variant->set_description( schematic->GetVariantDescription( name ).ToUTF8() );

        attributes = variant->mutable_attributes();
        attributes->set_exclude_from_simulation( variantInfo.m_ExcludedFromSim );
        attributes->set_exclude_from_bill_of_materials( variantInfo.m_ExcludedFromBOM );
        attributes->set_exclude_from_board( variantInfo.m_ExcludedFromBoard );
        attributes->set_exclude_from_position_files( variantInfo.m_ExcludedFromPosFiles );
        attributes->set_do_not_populate( variantInfo.m_DNP );

        for( const auto& [key, value] : variantInfo.m_Fields )
            ( *variant->mutable_fields() )[std::string( key.ToUTF8() )] = value.ToUTF8();
    }

    return true;
}


bool UnpackSymbol( SCH_SYMBOL* aOutput, const kiapi::schematic::types::SchematicSymbolInstance& aInput )
{
    using namespace kiapi::common::types;
    using namespace kiapi::schematic::types;

    google::protobuf::Any any;
    any.PackFrom( aInput );

    if( !aOutput->Deserialize( any ) )
        return false;

    if( aInput.has_attributes() )
    {
        const SchematicSymbolAttributes& attrs = aInput.attributes();

        aOutput->SetExcludedFromSim( attrs.exclude_from_simulation() );
        aOutput->SetExcludedFromBOM( attrs.exclude_from_bill_of_materials() );
        aOutput->SetExcludedFromBoard( attrs.exclude_from_board() );
        aOutput->SetExcludedFromPosFiles( attrs.exclude_from_position_files() );
        aOutput->SetDNP( attrs.do_not_populate() );
    }

    if( aInput.has_pin_map_override() )
        aOutput->SetPinMapOverride( UnpackPinMapOverride( aInput.pin_map_override() ) );

    return true;
}


/// Make the schematic aware of a variant a request named.  Accepts either variant message.
template <typename VariantProto>
static void registerVariant( SCHEMATIC* aSchematic, const wxString& aName,
                             const VariantProto& aInput )
{
    if( !aSchematic )
        return;

    aSchematic->AddVariant( aName );

    // An empty description clears the one the variant has, so honour presence rather than text.
    if( aInput.has_description() )
        aSchematic->SetVariantDescription( aName, wxString::FromUTF8( aInput.description() ) );
}


/// Make the set of variant records on one placement of a symbol match @a aInput, by name.
///
/// Each surviving record keeps what the message does not carry: the attributes it leaves unset,
/// and the alternate symbol and pin-map overrides the message has no field for.
static void applySymbolVariants( SCH_SYMBOL* aSymbol,
                                 const kiapi::schematic::types::SchematicSymbolVariants& aInput,
                                 const SCH_SHEET_PATH& aPath, SCHEMATIC* aSchematic )
{
    using namespace kiapi::schematic::types;

    std::set<wxString> requested;

    for( const SchematicSymbolVariant& variantProto : aInput.variants() )
    {
        wxString name = wxString::FromUTF8( variantProto.name() );

        // The empty name selects the default variant, whose values are the symbol's own.
        if( name.IsEmpty() )
            continue;

        requested.insert( name );
        registerVariant( aSchematic, name, variantProto );

        if( variantProto.has_attributes() )
        {
            const SchematicSymbolAttributes& attrs = variantProto.attributes();

            aSymbol->SetExcludedFromSim( attrs.exclude_from_simulation(), &aPath, name );
            aSymbol->SetExcludedFromBOM( attrs.exclude_from_bill_of_materials(), &aPath, name );
            aSymbol->SetExcludedFromBoard( attrs.exclude_from_board(), &aPath, name );
            aSymbol->SetExcludedFromPosFiles( attrs.exclude_from_position_files(), &aPath, name );
            aSymbol->SetDNP( attrs.do_not_populate(), &aPath, name );
        }

        // Overrides the request dropped go back to resolving as the symbol's own value.
        SCH_SYMBOL_INSTANCE stored;

        if( aSymbol->GetInstance( stored, aPath.Path() ) && stored.m_Variants.contains( name ) )
        {
            for( const auto& [fieldName, unused] : stored.m_Variants[name].m_Fields )
            {
                if( variantProto.fields().find( std::string( fieldName.ToUTF8() ) )
                    == variantProto.fields().end() )
                {
                    aSymbol->ClearVariantField( aPath.Path(), name, fieldName );
                }
            }
        }

        for( const auto& [key, value] : variantProto.fields() )
        {
            aSymbol->SetFieldText( wxString::FromUTF8( key ), wxString::FromUTF8( value ), &aPath,
                                   name );
        }
    }

    // Variants the request left out are removed from this placement only; the variant itself
    // stays registered with the schematic and other placements keep theirs.
    SCH_SYMBOL_INSTANCE stored;

    if( aSymbol->GetInstance( stored, aPath.Path() ) )
    {
        for( const auto& [name, unused] : stored.m_Variants )
        {
            if( !requested.contains( name ) )
                aSymbol->DeleteVariant( aPath.Path(), name );
        }
    }
}


void ApplySymbolInstance( SCH_SYMBOL* aSymbol,
                          const kiapi::schematic::types::SchematicSymbolInstance& aInput,
                          const SCH_SHEET_PATH& aPath, SCHEMATIC* aSchematic )
{
    wxString            reference = wxString::FromUTF8( aInput.reference_field().text().text() );
    int                 unit = aInput.has_unit() ? aInput.unit().unit() : 1;
    SCH_SYMBOL_INSTANCE existing;

    if( aSymbol->GetInstance( existing, aPath.Path() ) )
    {
        if( !reference.IsEmpty() )
            aSymbol->SetRef( &aPath, reference );

        aSymbol->SetUnitSelection( &aPath, unit );
    }
    else
    {
        SCH_SYMBOL_INSTANCE instance;
        instance.m_Path = aPath.Path();
        instance.m_Reference = reference;
        instance.m_Unit = unit;

        aSymbol->AddHierarchicalReference( instance );
    }

    // The displayed unit follows the placement the request targeted, as it does in the editor.
    aSymbol->SetUnit( unit );

    // A request that carries no variant set leaves the placement's variants as they are.
    if( aInput.has_variants() )
        applySymbolVariants( aSymbol, aInput.variants(), aPath, aSchematic );
}


bool PackSheet( kiapi::schematic::types::SheetSymbol* aOutput, const SCH_SHEET* aInput,
                const SCH_SHEET_PATH& aPath )
{
    google::protobuf::Any any;
    aInput->Serialize( any );

    if( !any.UnpackTo( aOutput ) )
        return false;

    PackSheetPath( *aOutput->mutable_path(), aPath.Path() );

    SCHEMATIC* schematic = aInput->Schematic();

    kiapi::schematic::types::SheetVariants* variants = aOutput->mutable_variants();

    if( const SCH_SHEET_INSTANCE* instance = aInput->GetInstance( aPath.Path() ) )
    {
        aOutput->set_page_number( instance->m_PageNumber.ToUTF8() );

        for( const auto& [name, variantInfo] : instance->m_Variants )
        {
            kiapi::schematic::types::SheetVariant* variant = variants->add_variants();
            variant->set_name( name.ToUTF8() );

            if( schematic )
                variant->set_description( schematic->GetVariantDescription( name ).ToUTF8() );

            variant->set_exclude_from_sim( variantInfo.m_ExcludedFromSim );
            variant->set_exclude_from_bom( variantInfo.m_ExcludedFromBOM );
            variant->set_dnp( variantInfo.m_DNP );

            for( const auto& [key, value] : variantInfo.m_Fields )
                ( *variant->mutable_fields() )[std::string( key.ToUTF8() )] = value.ToUTF8();
        }
    }

    return true;
}


/// Make the set of variant records on one placement of a sheet match @a aInput, by name.
///
/// Each surviving record keeps the attributes the message leaves unset.
static void applySheetVariants( SCH_SHEET* aSheet,
                                const kiapi::schematic::types::SheetVariants& aInput,
                                const SCH_SHEET_PATH& aParentPath, SCHEMATIC* aSchematic )
{
    using namespace kiapi::schematic::types;

    std::set<wxString> requested;

    for( const SheetVariant& variantProto : aInput.variants() )
    {
        wxString name = wxString::FromUTF8( variantProto.name() );

        // The empty name selects the default variant, whose values are the sheet's own.
        if( name.IsEmpty() )
            continue;

        requested.insert( name );
        registerVariant( aSchematic, name, variantProto );

        // Each attribute the request leaves out keeps the value this variant already has.
        if( variantProto.has_exclude_from_sim() )
            aSheet->SetExcludedFromSim( variantProto.exclude_from_sim(), &aParentPath, name );

        if( variantProto.has_exclude_from_bom() )
            aSheet->SetExcludedFromBOM( variantProto.exclude_from_bom(), &aParentPath, name );

        if( variantProto.has_dnp() )
            aSheet->SetDNP( variantProto.dnp(), &aParentPath, name );

        // Overrides the request dropped go back to resolving as the sheet's own value.
        if( const SCH_SHEET_INSTANCE* stored = aSheet->GetInstance( aParentPath.Path() );
            stored && stored->m_Variants.contains( name ) )
        {
            std::map<wxString, wxString> storedFields = stored->m_Variants.at( name ).m_Fields;

            for( const auto& [fieldName, unused] : storedFields )
            {
                if( variantProto.fields().find( std::string( fieldName.ToUTF8() ) )
                    == variantProto.fields().end() )
                {
                    aSheet->ClearVariantField( aParentPath.Path(), name, fieldName );
                }
            }
        }

        for( const auto& [key, value] : variantProto.fields() )
        {
            aSheet->SetFieldText( wxString::FromUTF8( key ), wxString::FromUTF8( value ),
                                  &aParentPath, name );
        }
    }

    // Remove variants from the sheet (not the schematic) that the client didn't send.
    if( const SCH_SHEET_INSTANCE* stored = aSheet->GetInstance( aParentPath.Path() ) )
    {
        std::vector<wxString> obsolete;

        for( const auto& [name, unused] : stored->m_Variants )
        {
            if( !requested.contains( name ) )
                obsolete.push_back( name );
        }

        for( const wxString& name : obsolete )
            aSheet->DeleteVariant( aParentPath.Path(), name );
    }
}


void ApplySheetInstance( SCH_SHEET* aSheet, const kiapi::schematic::types::SheetSymbol& aInput,
                         const SCH_SHEET_PATH& aParentPath, SCHEMATIC* aSchematic )
{
    wxString       pageNumber = wxString::FromUTF8( aInput.page_number() );
    SCH_SHEET_PATH path( aParentPath );

    path.push_back( aSheet );

    // Creates the placement record when it is missing and keeps an existing one's variants; an
    // empty page number in the request leaves the placement on the page it already has.
    path.SetPageNumber( pageNumber.IsEmpty() ? path.GetPageNumber() : pageNumber );

    // A request that carries no variant set leaves the placement's variants as they are.
    if( aInput.has_variants() )
        applySheetVariants( aSheet, aInput.variants(), aParentPath, aSchematic );
}


tl::expected<bool, ApiResponseStatus> UnpackSheet( SCH_SHEET* aOutput, const kiapi::schematic::types::SheetSymbol& aInput )
{
    google::protobuf::Any any;
    any.PackFrom( aInput );

    if( !aOutput->Deserialize( any ) )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "could not unpack SCH_SHEET from SheetSymbol in request" );
        return tl::unexpected( e );
    }

    return true;
}
