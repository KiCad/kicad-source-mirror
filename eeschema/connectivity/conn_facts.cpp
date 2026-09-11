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

#include "conn_facts.h"
#include "conn_pin_name.h"
#include "conn_text.h"

#include <algorithm>
#include <common.h>
#include <drawing_sheet/ds_draw_item.h>
#include <map>
#include <geometry/shape_rect.h>
#include <geometry/shape_segment.h>
#include <stdexcept>
#include <tuple>
#include <unordered_set>
#include <string_utils.h>
#include <wx/thread.h>
#include <wx/regex.h>
#include <lib_symbol.h>
#include <sch_bus_entry.h>
#include <sch_field.h>
#include <sch_label.h>
#include <sch_line.h>
#include <sch_pin.h>
#include <sch_rule_area.h>
#include <sch_screen.h>
#include <sch_shape.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <sch_text.h>
#include <sch_textbox.h>
#include <schematic.h>
#include <sim/sim_lib_mgr.h>
#include <project.h>
#include <text_eval/text_eval_vcs.h>

namespace SCH_CONNECTIVITY
{
namespace
{
    class INSTANCE_FIELD : public SCH_FIELD
    {
    public:
        INSTANCE_FIELD( const SCH_FIELD& aField, const SCH_SHEET_PATH& aPath ) :
                SCH_FIELD( aField ),
                m_path( aPath )
        {
            ClearBoundingBoxCache();
        }

        wxString GetShownText( RESOLUTION_CONTEXT aContext, int aDepth = 0 ) const override
        {
            const wxString variant = Schematic() ? Schematic()->GetCurrentVariant() : wxString();
            return SCH_FIELD::GetShownText( &m_path, aContext, variant, aDepth );
        }

    private:
        const SCH_SHEET_PATH& m_path;
    };

    template <typename T>
    void SortUnique( std::vector<T>& aValues )
    {
        std::sort( aValues.begin(), aValues.end() );
        aValues.erase( std::unique( aValues.begin(), aValues.end() ), aValues.end() );
    }

    constexpr auto ById = []( const auto& a, const auto& b )
    {
        return a.id < b.id;
    };

    void ReadLabel( ITEM_FACT& aFact, const SCH_LABEL_BASE& aLabel )
    {
        aFact.rawText = aLabel.GetText();
        aFact.outputShape = aLabel.GetShape() == L_OUTPUT;

        for( const SCH_FIELD& field : aLabel.GetFields() )
        {
            if( field.GetUntranslatedName() == wxS( "Netclass" ) )
                aFact.netclassFields.push_back( field.GetText() );
        }
    }
} // namespace

LIBRARY_FIELD_FACT ExtractLibraryFieldFact( const SCH_FIELD& aField )
{
    LIBRARY_FIELD_FACT result{ aField.GetId(), aField.IsMandatory(), aField.GetName(), aField.GetText(),
            aField.GetPosition(), aField.IsVisible(), aField.IsNameShown(), aField.IsPrivate(),
            reinterpret_cast<uintptr_t>( aField.GetFont() ), aField.GetAttributes() };

    // Fonts are process-interned resources; comparison needs identity, never a deferred dereference
    result.style.m_Font = nullptr;
    return result;
}

bool LIBRARY_FIELD_FACT::Matches( const LIBRARY_FIELD_FACT& aOther, int aCompareFlags ) const
{
    using FLAGS = SCH_ITEM::COMPARE_FLAGS;

    if( ( aCompareFlags & FLAGS::FIELD_TEXT ) && id != FIELD_T::REFERENCE && text != aOther.text )
        return false;

    if( ( aCompareFlags & FLAGS::FIELD_SIZE_AND_STYLE )
        && ( fontIdentity != aOther.fontIdentity || style != aOther.style ) )
    {
        return false;
    }

    if( ( aCompareFlags & FLAGS::FIELD_VISIBILITY )
        && ( visible != aOther.visible || nameShown != aOther.nameShown ) )
    {
        return false;
    }

    return isPrivate == aOther.isPrivate
           && ( !( aCompareFlags & FLAGS::FIELD_POSITIONS ) || position == aOther.position );
}

std::vector<SIMULATION_MODEL_FACT> ExtractSimulationModelFacts( const SCH_SHEET_PATH& aPath,
                                                               const wxString& aVariantName )
{
    wxASSERT( wxThread::IsMain() );

    if( INPUT_TEXT_SCOPE::Active() )
        throw std::logic_error( "Simulation sources require published connectivity text" );

    std::vector<SIMULATION_MODEL_FACT> result;
    SCH_SCREEN* screen = aPath.LastScreen();

    if( !screen || aPath.GetExcludedFromSim( aVariantName ) )
        return result;

    for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
    {
        const auto* symbol = static_cast<const SCH_SYMBOL*>( item );

        if( symbol->GetRef( &aPath ).StartsWith( '#' ) || symbol->ResolveExcludedFromSim( &aPath, aVariantName ) )
            continue;

        result.push_back( { symbol->m_Uuid, symbol->GetPosition(),
                            SIM_LIB_MGR::CaptureModelInput( &aPath, *symbol, 0, aVariantName ) } );
    }

    std::ranges::sort( result, std::less<KIID>{}, &SIMULATION_MODEL_FACT::id );
    return result;
}

std::optional<VARIANT_SYMBOL_FACT> ExtractVariantSymbolFact( const SCH_SYMBOL& aSymbol,
                                                           const SCH_SHEET_PATH& aPath )
{
    wxASSERT( wxIsMainThread() );
    SCH_SYMBOL_INSTANCE instance;

    if( !aSymbol.GetInstance( instance, aPath.Path() ) )
        return std::nullopt;

    VARIANT_SYMBOL_FACT result;
    result.id = aSymbol.m_Uuid;

    for( const auto& [name, variant] : instance.m_Variants )
    {
        if( variant.m_SymbolOverride )
            result.overrides.emplace( name, *variant.m_SymbolOverride );
    }

    if( result.overrides.empty() )
        return std::nullopt;

    return result;
}

LIBRARY_SYMBOL_FACT ExtractLibrarySymbolFact( const LIB_SYMBOL& aSymbol )
{
    wxASSERT( wxIsMainThread() );
    LIBRARY_SYMBOL_FACT result;
    result.hasEmbeddedSymbol = true;
    result.unitCount = aSymbol.GetUnitCount();
    result.bodyStyleCount = aSymbol.GetBodyStyleCount();
    result.attributes = aSymbol.ComparisonAttributes();

    for( const SCH_ITEM& field : aSymbol.GetDrawItems()[SCH_FIELD_T] )
        result.fields.push_back( ExtractLibraryFieldFact( static_cast<const SCH_FIELD&>( field ) ) );

    for( const SCH_ITEM& pin : aSymbol.GetDrawItems()[SCH_PIN_T] )
        result.pins.push_back( static_cast<const SCH_PIN&>( pin ).ComparisonData() );

    for( const SCH_ITEM& drawing : aSymbol.GetDrawItems()[SCH_SHAPE_T] )
    {
        const auto& shape = static_cast<const SCH_SHAPE&>( drawing );
        result.shapes.push_back( { shape.m_Uuid, shape.GetUnit(), shape.GetBodyStyle(), shape.IsPrivate(),
                                  shape.GetPosition(), shape } );
    }

    if( aSymbol.IsDerived() )
    {
        result.inheritedPins.emplace();

        for( const SCH_PIN* pin : aSymbol.GetGraphicalPins( 0, 0 ) )
            result.inheritedPins->push_back( pin->ComparisonData() );
    }

    return result;
}

bool LIBRARY_SYMBOL_FACT::HasDuplicatePins() const
{
    const auto& source = inheritedPins ? *inheritedPins : pins;
    std::vector<std::pair<wxString, size_t>> logical;

    for( size_t i = 0; i < source.size(); ++i )
    {
        bool valid = false;
        auto numbers = ExpandStackedPinNotation( source[i].number, &valid );

        if( !valid || numbers.empty() )
            numbers = { source[i].number };

        for( const auto& number : numbers )
            logical.emplace_back( number, i );
    }

    std::sort( logical.begin(), logical.end(), [&]( const auto& a, const auto& b )
    {
        return std::tie( a.first, source[a.second].bodyStyle, source[a.second].unit, a.second )
               < std::tie( b.first, source[b.second].bodyStyle, source[b.second].unit, b.second );
    } );

    const auto clash = [&]( const auto& a, const auto& b )
    {
        const int first = source[a.second].bodyStyle;
        const int second = source[b.second].bodyStyle;
        return a.first == b.first && a.second != b.second && ( !first || !second || first == second );
    };

    return std::ranges::adjacent_find( logical, clash ) != logical.end();
}

bool LIBRARY_SYMBOL_FACT::Matches( const LIBRARY_SYMBOL_FACT& aOther, int aCompareFlags ) const
{
    using FLAGS = SCH_ITEM::COMPARE_FLAGS;
    wxCHECK_MSG( !( aCompareFlags & FLAGS::IDENTITY ), false,
                 "Captured library comparison requires content-only flags" );

    if( hasEmbeddedSymbol != aOther.hasEmbeddedSymbol )
        return false;

    if( !hasEmbeddedSymbol )
        return true;

    if( !attributes.Matches( aOther.attributes, aCompareFlags ) )
        return false;

    auto lessShape = []( const LIBRARY_SHAPE_FACT* a, const LIBRARY_SHAPE_FACT* b )
    {
        return a->Compare( *b, ~FLAGS::UUID ) < 0;
    };
    std::set<const LIBRARY_SHAPE_FACT*, decltype( lessShape )> lhsShapes( lessShape );
    std::set<const LIBRARY_SHAPE_FACT*, decltype( lessShape )> rhsShapes( lessShape );

    for( const auto& shape : shapes )
        lhsShapes.insert( &shape );

    for( const auto& shape : aOther.shapes )
        rhsShapes.insert( &shape );

    if( lhsShapes.size() != rhsShapes.size() )
        return false;

    for( auto lhs = lhsShapes.begin(), rhs = rhsShapes.begin(); lhs != lhsShapes.end(); ++lhs, ++rhs )
    {
        if( ( *lhs )->Compare( **rhs, aCompareFlags ) != 0 )
            return false;
    }

    auto findPin = []( const LIBRARY_SYMBOL_FACT& aSource, const PIN_COMPARISON_DATA& aPin )
            -> const PIN_COMPARISON_DATA*
    {
        const auto& candidates = aSource.inheritedPins ? *aSource.inheritedPins : aSource.pins;

        for( const auto& candidate : candidates )
        {
            if( candidate.number == aPin.number
                && ( !aPin.unit || !candidate.unit || aPin.unit == candidate.unit )
                && ( !aPin.bodyStyle || !candidate.bodyStyle || aPin.bodyStyle == candidate.bodyStyle ) )
            {
                return &candidate;
            }
        }

        return nullptr;
    };

    for( const auto& pin : pins )
    {
        const auto* other = findPin( aOther, pin );

        if( !other || pin.Compare( *other, aCompareFlags ) != 0 )
            return false;
    }

    for( const auto& pin : aOther.pins )
    {
        if( !findPin( *this, pin ) )
            return false;
    }

    auto findField = []( const LIBRARY_SYMBOL_FACT& aSource, const LIBRARY_FIELD_FACT& aField )
            -> const LIBRARY_FIELD_FACT*
    {
        for( const auto& candidate : aSource.fields )
        {
            if( aField.mandatory ? candidate.id == aField.id : candidate.name == aField.name )
                return &candidate;
        }

        return nullptr;
    };

    for( const auto& field : fields )
    {
        const auto* other = findField( aOther, field );

        if( !other )
        {
            if( aCompareFlags & FLAGS::EXTRA_FIELDS )
                return false;
        }
        else if( !field.Matches( *other, aCompareFlags ) )
        {
            return false;
        }
    }

    if( aCompareFlags & FLAGS::MISSING_FIELDS )
    {
        for( const auto& field : aOther.fields )
        {
            if( !findField( *this, field ) )
                return false;
        }
    }

    return true;
}

int LIBRARY_SHAPE_FACT::Compare( const LIBRARY_SHAPE_FACT& aOther, int aCompareFlags ) const
{
    // Subtraction can overflow at extreme coordinates and flip the order the shape sets rely on
    const auto sign = []( int aLeft, int aRight )
    {
        return ( aLeft > aRight ) - ( aLeft < aRight );
    };

    if( aCompareFlags & SCH_ITEM::COMPARE_FLAGS::UNIT )
    {
        if( unit != aOther.unit )
            return sign( unit, aOther.unit );

        if( bodyStyle != aOther.bodyStyle )
            return sign( bodyStyle, aOther.bodyStyle );
    }

    if( isPrivate != aOther.isPrivate )
        return isPrivate ? 1 : -1;

    if( aCompareFlags & SCH_ITEM::COMPARE_FLAGS::POSITION )
    {
        if( position.x != aOther.position.x )
            return sign( position.x, aOther.position.x );

        if( position.y != aOther.position.y )
            return sign( position.y, aOther.position.y );
    }

    if( int difference = geometry.Compare( &aOther.geometry ) )
        return difference;

    if( ( aCompareFlags & SCH_ITEM::COMPARE_FLAGS::UUID ) && id != aOther.id )
        return id < aOther.id ? -1 : 1;

    return 0;
}

bool LIBRARY_SHAPE_FACT::operator==( const LIBRARY_SHAPE_FACT& aOther ) const
{
    // Cache equality must retain edits smaller than the library comparison tolerance
    return id == aOther.id && unit == aOther.unit && bodyStyle == aOther.bodyStyle
           && isPrivate == aOther.isPrivate && position == aOther.position
           && geometry == aOther.geometry && geometry.GetStart() == aOther.geometry.GetStart()
           && geometry.GetEnd() == aOther.geometry.GetEnd()
           && ( geometry.GetShape() != SHAPE_T::ARC || geometry.GetArcMid() == aOther.geometry.GetArcMid() )
           && geometry.GetBezierPoints() == aOther.geometry.GetBezierPoints()
           && geometry.Compare( &aOther.geometry ) == 0;
}

bool RULE_AREA_FACT::operator==( const RULE_AREA_FACT& aOther ) const
{
    if( id != aOther.id || containedItems != aOther.containedItems || attachedDirectives != aOther.attachedDirectives
        || polygon.OutlineCount() != aOther.polygon.OutlineCount() )
    {
        return false;
    }

    for( int i = 0; i < polygon.OutlineCount(); ++i )
    {
        const auto& first = polygon.CPolygon( i );
        const auto& second = aOther.polygon.CPolygon( i );

        if( first.size() != second.size() )
            return false;

        for( size_t j = 0; j < first.size(); ++j )
        {
            if( first[j].CPoints() != second[j].CPoints() || first[j].CArcs() != second[j].CArcs()
                || first[j].CShapes() != second[j].CShapes() || first[j].IsClosed() != second[j].IsClosed() )
            {
                return false;
            }
        }
    }

    return true;
}

const LIBRARY_SYMBOL_FACTS& SCREEN_FACTS::LibrarySymbols() const
{
    static const LIBRARY_SYMBOL_FACTS empty;
    return librarySymbols ? *librarySymbols : empty;
}

bool SCREEN_FACTS::operator==( const SCREEN_FACTS& aOther ) const
{
    return ( librarySymbols == aOther.librarySymbols || LibrarySymbols() == aOther.LibrarySymbols() )
           && std::tie( footprints, pinMaps, multiUnits, items, ruleAreas, netclassOwners, invalidFieldNames )
              == std::tie( aOther.footprints, aOther.pinMaps, aOther.multiUnits, aOther.items,
                           aOther.ruleAreas, aOther.netclassOwners, aOther.invalidFieldNames );
}

SCREEN_FACTS ExtractScreenFacts( const SCH_SCREEN& aScreen, std::shared_ptr<const LIBRARY_SYMBOL_FACTS> aLibrarySymbols,
                                 const SCREEN_FACTS* aUnchangedNonLines )
{
    wxASSERT( wxIsMainThread() );
    const EE_RTREE&                   items = aScreen.Items();
    SCREEN_FACTS                      result;
    std::vector<const SCH_RULE_AREA*> areas;
    std::unordered_set<KIID>          identities;

    // Rule-area membership depends on line geometry
    if( aUnchangedNonLines && ( !aUnchangedNonLines->ruleAreas.empty() || !items.OfType( SCH_RULE_AREA_T ).empty() ) )
        aUnchangedNonLines = nullptr;

    if( aUnchangedNonLines )
    {
        result = *aUnchangedNonLines;
        std::erase_if( result.items, []( const ITEM_FACT& fact ) { return fact.type == SCH_LINE_T; } );
        aLibrarySymbols = result.librarySymbols;
    }

    identities.reserve( items.size() );
    result.items.reserve( items.size() );
    auto libraries = aLibrarySymbols ? nullptr : std::make_shared<LIBRARY_SYMBOL_FACTS>();
    result.librarySymbols = aLibrarySymbols ? std::move( aLibrarySymbols ) : libraries;

    if( libraries )
    {
        libraries->reserve( std::count_if( items.begin(), items.end(), []( const SCH_ITEM* item )
        {
            return item->Type() == SCH_SYMBOL_T;
        } ) );
    }

    const auto checkIdentity = [&]( const KIID& aId )
    {
        if( !identities.insert( aId ).second )
            throw std::runtime_error( "Duplicate connectivity source identity" );
    };

    for( SCH_ITEM* item : items )
    {
        checkIdentity( item->m_Uuid );

        if( aUnchangedNonLines && item->Type() != SCH_LINE_T )
        {
            if( item->Type() == SCH_SYMBOL_T )
            {
                for( const auto& pin : static_cast<const SCH_SYMBOL*>( item )->GetRawPins() )
                    checkIdentity( pin->m_Uuid );
            }
            else if( item->Type() == SCH_SHEET_T )
            {
                for( const SCH_SHEET_PIN* pin : static_cast<const SCH_SHEET*>( item )->GetPins() )
                    checkIdentity( pin->m_Uuid );
            }

            continue;
        }

        bool hasNetclass = false;
        item->RunOnChildren(
                [&]( SCH_ITEM* child )
                {
                    if( child->Type() != SCH_FIELD_T )
                        return true;

                    const auto* field = static_cast<SCH_FIELD*>( child );

                    if( field->GetUntranslatedName() == wxS( "Netclass" ) )
                        hasNetclass = true;

                    if( item->Type() == SCH_SYMBOL_T || item->Type() == SCH_SHEET_T )
                    {
                        const wxString name = field->GetName();
                        wxString trimmed = name;

                        if( trimmed.Trim( false ).Trim( true ) != name )
                        {
                            result.invalidFieldNames.push_back(
                                    { item->m_Uuid, field->m_Uuid, field->GetPosition(), name } );
                        }
                    }

                    return true;
                },
                RECURSE_MODE::NO_RECURSE );

        if( hasNetclass )
            result.netclassOwners.push_back( item->m_Uuid );

        if( item->Type() == SCH_RULE_AREA_T )
        {
            areas.push_back( static_cast<const SCH_RULE_AREA*>( item ) );
            continue;
        }

        if( item->Type() == SCH_SYMBOL_T )
        {
            const auto& symbol = *static_cast<const SCH_SYMBOL*>( item );
            const auto& library = symbol.GetLibSymbolRef();

            if( libraries )
            {
                auto& librarySource = libraries->emplace_back();

                if( library )
                    librarySource = ExtractLibrarySymbolFact( *library );

                librarySource.id = symbol.m_Uuid;
                librarySource.position = symbol.GetPosition();
                librarySource.library = symbol.GetLibId();
            }

            if( library && library->GetUnitCount() > 1 )
            {
                result.multiUnits.push_back( { symbol.m_Uuid, symbol.GetPosition(), ExtractUnitFacts( *library ) } );
            }

            result.footprints.push_back( { symbol.m_Uuid, symbol.GetPosition(),
                    library ? library->GetFPFilters() : wxArrayString() } );

            if( auto maps = ExtractPinMapFacts( symbol ) )
                result.pinMaps.push_back( std::move( *maps ) );

            std::map<std::tuple<wxString, int, int>, size_t> groups;
            std::map<wxString, std::vector<size_t>> numbers;
            const bool hasJumperGroups = library && !library->JumperPinGroups().empty();

            // Unjumpered duplicates join only when stacked, so ERC can report pins wired to different nets
            const bool joinApart = library && library->GetDuplicatePinNumbersAreJumpers();

            for( const std::unique_ptr<SCH_PIN>& pin : symbol.GetRawPins() )
            {
                checkIdentity( pin->m_Uuid );
                const bool nc = pin->GetType() == ELECTRICAL_PINTYPE::PT_NC;
                const VECTOR2I contact = joinApart ? VECTOR2I() : pin->GetPosition();
                const auto key = std::make_tuple( pin->GetNumber(), contact.x, contact.y );
                auto       found = nc ? groups.end() : groups.find( key );
                size_t     index;

                if( found == groups.end() )
                {
                    index = result.items.size();
                    ITEM_FACT fact;
                    fact.id = pin->m_Uuid;
                    fact.type = SCH_PIN_T;
                    fact.owner = symbol.m_Uuid;
                    fact.multiUnit = symbol.IsMultiUnit();
                    fact.rawText = symbol.GetField( FIELD_T::VALUE )->GetText();
                    result.items.push_back( std::move( fact ) );

                    if( !nc )
                        groups.emplace( key, index );
                }
                else
                {
                    index = found->second;
                    result.items[index].id = std::min( result.items[index].id, pin->m_Uuid );
                }

                ITEM_FACT& fact = result.items[index];
                const bool valid = pin->GetLibPin() && library && !symbol.IsMissingLibSymbol();
                PIN_FACT   member;
                member.id = pin->m_Uuid;
                member.position = pin->GetPosition();
                member.unit = pin->GetUnit();
                member.type = pin->GetType();
                member.canDrive = valid;
                member.globalPower = valid && pin->IsGlobalPower();
                member.localPower = valid && pin->IsLocalPower();
                member.invisible = !pin->IsVisible();
                member.globalPowerParent = symbol.IsGlobalPower();
                member.localPowerParent = symbol.IsLocalPower();
                member.name = pin->GetName();
                member.shownName = pin->GetShownName();
                member.shownNumber = pin->GetShownNumber();
                member.number = pin->GetNumber();

                if( const SCH_PIN* libPin = pin->GetLibPin() )
                {
                    member.libraryName = libPin->GetShownName();
                    member.padNumber = libPin->GetSmallestStackedPadNumber();
                }

                fact.pins.push_back( std::move( member ) );

                if( !nc && hasJumperGroups )
                {
                    numbers[pin->GetNumber()].push_back( index );

                    for( const wxString& number : pin->GetStackedPinNumbers() )
                        numbers[number].push_back( index );
                }
            }

            if( hasJumperGroups )
            {
                for( const std::set<wxString>& group : library->JumperPinGroups() )
                {
                    std::vector<size_t> indices;

                    for( const wxString& number : group )
                    {
                        if( auto it = numbers.find( number ); it != numbers.end() )
                            indices.insert( indices.end(), it->second.begin(), it->second.end() );
                    }

                    SortUnique( indices );

                    for( size_t first : indices )
                    {
                        for( size_t second : indices )
                        {
                            if( first != second )
                                result.items[first].jumperedWith.push_back( result.items[second].id );
                        }
                    }
                }
            }

            continue;
        }

        if( item->Type() == SCH_SHEET_T )
        {
            for( SCH_SHEET_PIN* pin : static_cast<SCH_SHEET*>( item )->GetPins() )
            {
                checkIdentity( pin->m_Uuid );
                ITEM_FACT fact;
                fact.id = pin->m_Uuid;
                fact.type = pin->Type();
                fact.owner = item->m_Uuid;
                fact.ports.push_back( { pin->GetPosition(), PORT_KIND::ANCHOR } );
                ReadLabel( fact, *pin );
                result.items.push_back( std::move( fact ) );
            }

            continue;
        }

        if( !item->IsConnectable() )
            continue;

        ITEM_FACT fact;
        fact.id = item->m_Uuid;
        fact.type = item->Type();
        PORT_KIND kind = PORT_KIND::ANCHOR;

        if( item->Type() == SCH_LINE_T )
        {
            const auto& line = *static_cast<const SCH_LINE*>( item );
            kind = line.GetLayer() == LAYER_BUS ? PORT_KIND::BUS : PORT_KIND::WIRE;
            fact.segment.emplace( line.GetStartPoint(), line.GetEndPoint() );
            fact.lineWidth = line.GetLineWidth();
        }

        if( item->Type() == SCH_BUS_WIRE_ENTRY_T || item->Type() == SCH_BUS_BUS_ENTRY_T )
        {
            const auto& entry = *static_cast<const SCH_BUS_ENTRY_BASE*>( item );
            kind = item->Type() == SCH_BUS_WIRE_ENTRY_T ? PORT_KIND::ENTRY : PORT_KIND::ENTRY_BUS;
            fact.ports = { { entry.GetPosition(), kind }, { entry.GetEnd(), kind } };
        }
        else
        {
            for( const VECTOR2I& point : item->GetConnectionPoints() )
                fact.ports.push_back( { point, kind } );
        }

        if( auto* label = dynamic_cast<const SCH_LABEL_BASE*>( item ) )
            ReadLabel( fact, *label );

        result.items.push_back( std::move( fact ) );
    }

    // Spatial-index traversal order changes when unrelated items are reindexed
    if( libraries )
        std::ranges::sort( *libraries, ById );

    if( !aUnchangedNonLines )
    {
        std::ranges::sort( result.footprints, ById );
        std::ranges::sort( result.pinMaps, ById );
        std::ranges::sort( result.multiUnits, ById );
    }

    SortUnique( result.netclassOwners );
    std::ranges::sort( result.invalidFieldNames,
                       []( const auto& a, const auto& b )
                       {
                           return std::tie( a.owner, a.field ) < std::tie( b.owner, b.field );
                       } );

    for( ITEM_FACT& fact : result.items )
    {
        std::ranges::sort( fact.pins, ById );
        SortUnique( fact.jumperedWith );
    }

    std::ranges::sort( result.items, ById );

    for( const SCH_RULE_AREA* area : areas )
    {
        RULE_AREA_FACT fact;
        fact.id = area->m_Uuid;
        fact.polygon = area->GetPolyShape();
        const auto& polygon = fact.polygon;

        for( const ITEM_FACT& item : result.items )
        {
            if( item.type == SCH_DIRECTIVE_LABEL_T )
            {
                if( !item.ports.empty() && polygon.CollideEdge( item.ports.front().position, nullptr, 5 ) )
                    fact.attachedDirectives.push_back( item.id );
            }
            else if( item.segment )
            {
                const SHAPE_SEGMENT segment( *item.segment, item.lineWidth );

                if( polygon.Collide( &segment ) )
                    fact.containedItems.push_back( item.id );
            }
            else
            {
                for( const PORT_FACT& port : item.ports )
                {
                    if( polygon.Collide( port.position ) )
                        fact.containedItems.push_back( item.id );
                }

                for( const PIN_FACT& pin : item.pins )
                {
                    if( polygon.Collide( pin.position ) )
                        fact.containedItems.push_back( pin.id );
                }
            }
        }

        SortUnique( fact.containedItems );
        SortUnique( fact.attachedDirectives );
        result.ruleAreas.push_back( std::move( fact ) );
    }

    std::ranges::sort( result.ruleAreas, ById );
    return result;
}

std::vector<TEXT_ASSERTION> ExtractTextAssertions( const wxString& aText )
{
    wxASSERT( wxThread::IsMain() );
    std::vector<TEXT_ASSERTION> result;

    if( !aText.Contains( wxS( "${" ) ) )
        return result;

    static wxRegEx warningExpr( wxS( "(^|[^\\\\])\\$\\{ERC_WARNING\\s*([^}]*)\\}" ) );
    static wxRegEx errorExpr( wxS( "(^|[^\\\\])\\$\\{ERC_ERROR\\s*([^}]*)\\}" ) );

    for( bool warning : { true, false } )
    {
        wxRegEx& expression = warning ? warningExpr : errorExpr;
        wxString remaining = aText;

        while( expression.Matches( remaining ) )
        {
            result.push_back( { warning, expression.GetMatch( remaining, 2 ) } );
            size_t start = 0;
            size_t length = 0;

            if( !expression.GetMatch( &start, &length, 0 ) || length == 0 )
                break;

            remaining = remaining.Mid( start + length );
        }
    }

    return result;
}

std::vector<TEXT_CHECK_FACT> ExtractDrawingSheetTextChecks( const SCH_SCREEN& aScreen, const SCH_SHEET_PATH& aPath )
{
    wxASSERT( wxThread::IsMain() );
    std::vector<TEXT_CHECK_FACT> result;

    if( const SCHEMATIC* schematic = aScreen.Schematic(); schematic && schematic->IsValid() )
    {
        DS_DRAW_ITEM_LIST drawing( schIUScale, FOR_ERC_DRC );
        drawing.SetPageNumber( aPath.GetPageNumber() );
        drawing.SetSheetCount( schematic->Hierarchy().size() );
        drawing.SetFileName( aScreen.GetFileName() );
        drawing.SetSheetName( aPath.Last()->GetName() );
        drawing.SetSheetPath( aPath.PathHumanReadable() );
        drawing.SetIsFirstPage( aPath.GetVirtualPageNumber() == 1 );
        drawing.SetVariantName( schematic->GetCurrentVariant() );
        drawing.SetVariantDesc( schematic->GetVariantDescription( schematic->GetCurrentVariant() ) );
        drawing.SetSheetLayer( wxS( "dummyLayer" ) );
        drawing.SetProject( &schematic->Project() );
        drawing.BuildDrawItemsList( aScreen.GetPageSettings(), aScreen.GetTitleBlock() );

        for( DS_DRAW_ITEM_BASE* item = drawing.GetFirst(); item; item = drawing.GetNext() )
        {
            const auto* text = dynamic_cast<const DS_DRAW_ITEM_TEXT*>( item );

            if( !text )
                continue;

            auto assertions = ExtractTextAssertions( text->GetText() );
            const wxString shown = assertions.empty() ? text->GetShownText( FOR_ERC_DRC ) : wxString();

            if( !assertions.empty() || shown.Contains( wxS( "${" ) ) )
            {
                result.push_back( { niluuid, niluuid, text->GetPosition(), text->GetPosition(),
                                                       std::move( assertions ), shown } );
            }
        }
    }

    return result;
}

std::vector<TEXT_CHECK_FACT> ExtractTextChecks( const SCH_SCREEN& aScreen, const SCH_SHEET_PATH& aPath )
{
    wxASSERT( wxThread::IsMain() );
    std::vector<TEXT_CHECK_FACT> result;
    auto append = [&]( const KIID& assertionItem, const KIID& item, const VECTOR2I& assertionPosition,
                       const wxString& raw, bool expandEnvironment, auto&& resolve )
    {
        auto assertions = ExtractTextAssertions( raw );

        if( !assertions.empty() )
        {
            result.push_back( { assertionItem, item, assertionPosition, assertionPosition,
                                std::move( assertions ), wxString() } );
            return;
        }

        auto [shown, position] = resolve();
        const auto* schematic = aScreen.Schematic();

        if( expandEnvironment && shown.find_first_of( wxS( "$%" ) ) != wxString::npos )
            shown = ExpandEnvVarSubstitutions( shown, schematic ? &schematic->Project() : nullptr );

        if( shown.Contains( wxS( "${" ) ) )
            result.push_back( { assertionItem, item, assertionPosition, position, {}, std::move( shown ) } );
    };
    auto fields = [&]( const auto& owner )
    {
        for( const SCH_FIELD& field : owner.GetFields() )
        {
            append( field.m_Uuid, owner.m_Uuid, field.GetPosition(), field.GetText(), true, [&]
            {
                return std::make_pair( field.GetShownText( &aPath, FOR_ERC_DRC ), field.GetPosition() );
            } );
        }
    };

    for( SCH_ITEM* item : aScreen.Items().OfType( SCH_LOCATE_ANY_T ) )
    {
        if( item->Type() == SCH_SYMBOL_T )
        {
            const auto& symbol = *static_cast<SCH_SYMBOL*>( item );
            fields( symbol );

            if( const auto& library = symbol.GetLibSymbolRef() )
            {
                library->RunOnChildren( [&]( SCH_ITEM* child )
                {
                    auto text = [&]( const auto& source, auto&& shown )
                    {
                        append( symbol.m_Uuid, symbol.m_Uuid, source.GetPosition(), source.GetText(), true, [&]
                        {
                            wxString shownText = shown();
                            const BOX2I box = symbol.GetTransform().TransformCoordinate( source.GetBoundingBox() );
                            return std::make_pair( std::move( shownText ), box.Centre() + symbol.GetPosition() );
                        } );
                    };

                    if( child->Type() == SCH_TEXT_T )
                    {
                        const auto& source = *static_cast<SCH_TEXT*>( child );
                        text( source, [&] { return source.GetShownText( &aPath, FOR_ERC_DRC ); } );
                    }
                    else if( child->Type() == SCH_TEXTBOX_T )
                    {
                        const auto& source = *static_cast<SCH_TEXTBOX*>( child );
                        text( source, [&] { return source.GetShownText( nullptr, &aPath, FOR_ERC_DRC ); } );
                    }
                }, RECURSE_MODE::NO_RECURSE );
            }
        }
        else if( const auto* label = dynamic_cast<SCH_LABEL_BASE*>( item ) )
        {
            fields( *label );
        }
        else if( item->Type() == SCH_SHEET_T )
        {
            auto& sheet = *static_cast<SCH_SHEET*>( item );
            fields( sheet );
            SCH_SHEET_PATH childPath = aPath;
            childPath.push_back( &sheet );

            for( const SCH_SHEET_PIN* pin : sheet.GetPins() )
            {
                append( niluuid, pin->m_Uuid, pin->GetPosition(), wxString(), false, [&]
                {
                    return std::make_pair( pin->GetShownText( &childPath, FOR_ERC_DRC ), pin->GetPosition() );
                } );
            }
        }
        else if( const auto* text = dynamic_cast<SCH_TEXT*>( item ) )
        {
            append( text->m_Uuid, text->m_Uuid, text->GetPosition(), text->GetText(), false, [&]
            {
                return std::make_pair( text->GetShownText( &aPath, FOR_ERC_DRC ), text->GetPosition() );
            } );
        }
        else if( const auto* textbox = dynamic_cast<SCH_TEXTBOX*>( item ) )
        {
            append( textbox->m_Uuid, textbox->m_Uuid, textbox->GetPosition(), textbox->GetText(), false, [&]
            {
                return std::make_pair( textbox->GetShownText( nullptr, &aPath, FOR_ERC_DRC ), textbox->GetPosition() );
            } );
        }
    }

    return result;
}


std::optional<PIN_MAP_FACT> ExtractPinMapFacts( const SCH_SYMBOL& aSymbol )
{
    const auto& lib = aSymbol.GetLibSymbolRef();

    if( !lib )
        return std::nullopt;

    const auto& maps = lib->GetEffectivePinMaps();
    const auto& footprints = lib->GetEffectiveAssociatedFootprints();

    if( maps.IsEmpty() && footprints.empty() )
        return std::nullopt;

    PIN_MAP_FACT result;
    result.id = aSymbol.m_Uuid;
    result.position = aSymbol.GetPosition();
    result.maps = maps;
    result.footprints = footprints;
    result.jumperGroups = lib->JumperPinGroups();

    for( const SCH_PIN* pin : lib->GetPins() )
        result.pinNumbers.insert( pin->GetNumber() );

    return result;
}

std::vector<UNIT_FACT> ExtractUnitFacts( const LIB_SYMBOL& aSymbol )
{
    std::vector<UNIT_FACT> result;

    for( int unit = 1; unit <= aSymbol.GetUnitCount(); ++unit )
    {
        UNIT_FACT fact;
        fact.name = aSymbol.GetUnitDisplayName( unit, false );

        for( const SCH_PIN* pin : aSymbol.GetGraphicalPins( unit, 0 ) )
        {
            fact.powerInput |= pin->GetType() == ELECTRICAL_PINTYPE::PT_POWER_IN;
            fact.input |= pin->GetType() == ELECTRICAL_PINTYPE::PT_INPUT;
            fact.bidirectional |= pin->GetType() == ELECTRICAL_PINTYPE::PT_BIDI;
        }

        result.push_back( std::move( fact ) );
    }

    return result;
}

INSTANCE_FACTS ExtractInstanceFacts( const SCREEN_FACTS& aFacts, const SCH_SCREEN& aScreen,
                                     const SCH_SHEET_PATH& aPath )
{
    wxASSERT( wxIsMainThread() );

    if( aPath.LastScreen() != &aScreen )
        throw std::invalid_argument( "Connectivity instance does not reference its screen" );

    std::optional<TEXT_EVAL_VCS::CONTEXT_PATH_SCOPE> vcs;

    if( const auto* schematic = aScreen.Schematic(); schematic && schematic->IsValid() )
    {
        if( const wxString path = schematic->Project().GetProjectPath(); !path.IsEmpty() )
            vcs.emplace( path );
    }

    struct SYMBOL_TEXT
    {
        const SCH_SYMBOL*                      symbol = nullptr;
        PIN_NAME_REFERENCE                     reference;
        wxString                               plainReference;
        std::map<wxString, std::set<wxString>> numbersByName;
    };

    INPUT_TEXT_SCOPE             inputText;
    INSTANCE_FACTS               result;
    std::map<KIID, int>          units;
    std::map<KIID, SYMBOL_TEXT>  symbols;
    std::set<KIID>               inactivePins;
    const wxString               variant = aScreen.Schematic() ? aScreen.Schematic()->GetCurrentVariant() : wxString();

    result.pageOrder = aPath.GetVirtualPageNumber();

    for( const FOOTPRINT_FACT& fact : aFacts.footprints )
    {
        const auto* symbol = dynamic_cast<const SCH_SYMBOL*>( aScreen.GetConnectivityItem( fact.id ) );

        if( !symbol )
            throw std::runtime_error( "Footprint symbol disappeared during extraction" );

        result.footprints.emplace_back( fact.id, symbol->GetFootprintFieldText( &aPath, RESOLVED ) );

        if( auto source = ExtractVariantSymbolFact( *symbol, aPath ) )
            result.variantSymbols.push_back( std::move( *source ) );
    }

    for( const PIN_MAP_FACT& fact : aFacts.pinMaps )
    {
        if( fact.footprints.empty() )
            continue;

        const auto* symbol = dynamic_cast<const SCH_SYMBOL*>( aScreen.GetConnectivityItem( fact.id ) );

        if( !symbol )
            throw std::runtime_error( "Pin-map symbol disappeared during extraction" );

        const wxString footprint = symbol->GetFootprintFieldText( &aPath, RESOLVED );
        LIB_ID footprintId;

        if( footprint.IsEmpty() || footprintId.Parse( footprint, true ) >= 0 )
            continue;

        for( const SCH_PIN* pin : symbol->GetPins( &aPath ) )
        {
            SCH_PIN::PAD_RESOLUTION state;
            pin->GetEffectivePadNumber( aPath, variant, footprintId, nullptr, &state );

            if( state == SCH_PIN::PAD_RESOLUTION::MAPPED )
                continue;

            const auto type = pin->GetType();
            result.pinMapCandidates.push_back( { pin->m_Uuid, pin->GetPosition(), pin->GetNumber(), footprint,
                    type == ELECTRICAL_PINTYPE::PT_NC || type == ELECTRICAL_PINTYPE::PT_NIC } );
        }
    }

    for( const MULTI_UNIT_FACT& fact : aFacts.multiUnits )
    {
        const auto* symbol = dynamic_cast<const SCH_SYMBOL*>( aScreen.GetConnectivityItem( fact.id ) );

        if( !symbol )
            throw std::runtime_error( "Multiunit symbol disappeared during extraction" );

        result.multiUnits.push_back( { fact.id, symbol->GetRef( &aPath ), symbol->GetRef( &aPath, true ),
                symbol->GetFootprintFieldText( &aPath, RESOLVED ), symbol->GetUnitSelection( &aPath ) } );
    }

    // Screen order fixes the main and auxiliary witnesses in saved ERC exclusions
    for( SCH_ITEM* item : aScreen.Items().OfType( SCH_SHEET_T ) )
    {
        const auto* sheet = static_cast<const SCH_SHEET*>( item );
        result.childSheets.push_back( { sheet->m_Uuid, sheet->GetPosition(),
                sheet->GetField( FIELD_T::SHEET_NAME )->GetShownText( &aPath, RESOLVED, variant ) } );
    }

    for( const KIID& owner : aFacts.netclassOwners )
    {
        SCH_ITEM* item = aScreen.GetConnectivityItem( owner );

        if( !item )
            throw std::invalid_argument( "Netclass field owner is missing from its captured screen" );

        item->RunOnChildren(
                [&]( SCH_ITEM* child )
                {
                    if( child->Type() != SCH_FIELD_T )
                        return true;

                    const auto* field = static_cast<SCH_FIELD*>( child );

                    if( field->GetUntranslatedName() == wxS( "Netclass" ) )
                    {
                        wxString name = field->GetShownText( &aPath, FOR_NETNAME, variant );

                        if( !name.empty() )
                            result.netclassReferences.push_back(
                                    { aPath.PathRef(), owner, item->GetPosition(), std::move( name ) } );
                    }

                    return true;
                },
                RECURSE_MODE::NO_RECURSE );
    }

    for( const ITEM_FACT& fact : aFacts.items )
    {
        if( fact.type == SCH_PIN_T )
        {
            auto [entry, inserted] = symbols.try_emplace( fact.owner );
            SYMBOL_TEXT& cached = entry->second;

            if( inserted )
            {
                cached.symbol = dynamic_cast<const SCH_SYMBOL*>( aScreen.GetConnectivityItem( fact.owner ) );

                if( !cached.symbol )
                    throw std::runtime_error( "Connectivity symbol disappeared during extraction" );

                units.emplace( fact.owner, cached.symbol->GetUnitSelection( &aPath ) );
                SCH_SYMBOL_INSTANCE instance;
                const bool hasInstance = cached.symbol->GetInstance( instance, aPath.Path() );
                cached.reference.symbolUuid = cached.symbol->m_Uuid.AsString();

                if( hasInstance )
                {
                    cached.reference.reference = instance.m_Reference;
                    cached.reference.referenceWithUnit = cached.symbol->GetRef( &aPath, true );
                }

                cached.plainReference = cached.symbol->GetRef( &aPath );

                for( const SCH_PIN* pin : cached.symbol->GetPins( &aPath ) )
                {
                    if( pin->GetType() != ELECTRICAL_PINTYPE::PT_NC )
                        cached.numbersByName[pin->GetShownName()].insert( pin->GetShownNumber() );
                }
            }

            const SCH_SYMBOL*         symbol = cached.symbol;
            const int                 unit = units.at( fact.owner );
            const PIN_NAME_REFERENCE& reference = cached.reference;

            for( const PIN_FACT& member : fact.pins )
            {
                if( unit && member.unit && unit != member.unit )
                {
                    inactivePins.insert( member.id );
                    continue;
                }

                const auto* pin = dynamic_cast<const SCH_PIN*>( aScreen.GetConnectivityItem( member.id ) );

                if( !pin )
                    throw std::runtime_error( "Connectivity pin disappeared during extraction" );

                ITEM_TEXT_FACT text;
                text.id = member.id;
                text.canDrive = member.canDrive;
                text.reference = cached.plainReference;

                if( member.globalPower || member.localPower )
                {
                    text.name = EscapeString( member.globalPowerParent || member.localPowerParent
                                                      ? symbol->GetValue( &aPath, FOR_NETNAME, variant )
                                                      : pin->GetLibPin()->GetName(),
                                              CTX_NETNAME );
                    text.ncName = text.name;
                }
                else
                {
                    const SCH_PIN* libPin = pin->GetLibPin();
                    PIN_NAME_FACT  name;
                    name.name = libPin ? libPin->GetShownName() : wxString( "??" );
                    name.shownNumber = libPin ? libPin->GetShownNumber() : wxString( "??" );
                    name.number = libPin ? libPin->GetNumber() : wxString( "??" );
                    name.padNumber = libPin ? libPin->GetSmallestStackedPadNumber() : name.shownNumber;
                    name.noConnect = member.type == ELECTRICAL_PINTYPE::PT_NC;

                    const auto names = cached.numbersByName.find( pin->GetShownName() );

                    if( names != cached.numbersByName.end() )
                    {
                        const auto& numbers = names->second;
                        name.hasDuplicateName = numbers.size() > 1 || !numbers.contains( pin->GetShownNumber() );
                    }

                    text.name = RenderPinNetName( name, reference );
                    text.ncName = RenderPinNetName( name, reference, true );
                    text.canDrive &= symbol->IsInNetlist() && !symbol->GetExcludedFromBoard( &aPath, variant )
                                     && !reference.reference.StartsWith( wxS( "#" ) );
                }

                result.items.push_back( std::move( text ) );
            }

            continue;
        }

        const auto* label = dynamic_cast<const SCH_LABEL_BASE*>( aScreen.GetConnectivityItem( fact.id ) );

        if( !label )
            continue;

        SCH_SHEET_PATH path = aPath;

        if( fact.type == SCH_SHEET_PIN_T )
            path.push_back( static_cast<const SCH_SHEET_PIN*>( label )->GetParent() );

        ITEM_TEXT_FACT text;
        text.id = fact.id;
        text.canDrive = fact.type != SCH_DIRECTIVE_LABEL_T;
        text.name = EscapeString( label->GetShownText( &path, FOR_NETNAME ), CTX_NETNAME );
        text.ncName = text.name;

        for( const SCH_FIELD& field : label->GetFields() )
        {
            if( field.GetUntranslatedName() == wxS( "Netclass" ) )
            {
                wxString value = field.GetShownText( &path, FOR_NETNAME, variant );

                if( !value.empty() )
                    text.netclasses.push_back( std::move( value ) );
            }
        }

        SortUnique( text.netclasses );
        result.items.push_back( std::move( text ) );
    }

    std::map<KIID, BOX2I> ownerBoxes;

    if( !aFacts.ruleAreas.empty() )
    {
        for( SCH_ITEM* item : aScreen.Items() )
        {
            if( item->Type() == SCH_SYMBOL_T )
            {
                const auto& symbol = *static_cast<const SCH_SYMBOL*>( item );
                const int   unit = symbol.GetUnitSelection( &aPath );
                units.emplace( symbol.m_Uuid, unit );
                const LIB_SYMBOL* library = symbol.GetEffectiveLibSymbol( &aPath );

                if( !library )
                    library = LIB_SYMBOL::GetDummy();

                BOX2I box = library->GetBodyBoundingBox( unit, symbol.GetBodyStyle(), true, false );
                box = symbol.GetTransform().TransformCoordinate( box );
                box.Normalize();
                box.Offset( symbol.GetPosition() );

                for( const SCH_FIELD& field : symbol.GetFields() )
                {
                    if( field.IsVisible() )
                        box.Merge( INSTANCE_FIELD( field, aPath ).GetBoundingBox() );
                }

                ownerBoxes.emplace( symbol.m_Uuid, box );
            }
            else if( item->Type() == SCH_SHEET_T )
            {
                const auto& sheet = *static_cast<const SCH_SHEET*>( item );
                BOX2I       box = sheet.GetBodyBoundingBox();

                for( const SCH_FIELD& field : sheet.GetFields() )
                    box.Merge( INSTANCE_FIELD( field, aPath ).GetBoundingBox() );

                ownerBoxes.emplace( sheet.m_Uuid, box );
            }
        }
    }

    for( const RULE_AREA_FACT& area : aFacts.ruleAreas )
    {
        INSTANCE_RULE_AREA_FACT resolved;
        resolved.id = area.id;
        resolved.attachedDirectives = area.attachedDirectives;
        resolved.containedItems = area.containedItems;
        std::erase_if( resolved.containedItems, [&]( const KIID& id ) { return inactivePins.contains( id ); } );

        for( const auto& [id, box] : ownerBoxes )
        {
            const SHAPE_RECT rectangle( box );

            if( area.polygon.Collide( &rectangle ) )
                resolved.containedItems.push_back( id );
        }

        for( const ITEM_TEXT_FACT& text : result.items )
        {
            if( std::binary_search( area.attachedDirectives.begin(), area.attachedDirectives.end(), text.id ) )
                resolved.netclasses.insert( resolved.netclasses.end(), text.netclasses.begin(), text.netclasses.end() );
        }

        SortUnique( resolved.containedItems );
        SortUnique( resolved.netclasses );
        result.ruleAreas.push_back( std::move( resolved ) );
    }

    result.units.assign( units.begin(), units.end() );
    std::ranges::sort( result.items, ById );
    return result;
}
} // namespace SCH_CONNECTIVITY
