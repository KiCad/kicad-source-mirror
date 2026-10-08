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

#include <downgrade/sch_downgrade.h>

#include <downgrade_scan.h>

#include <cctype>
#include <climits>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <base_units.h>
#include <common.h>
#include <bus_alias.h>
#include <eda_text.h>
#include <project/net_settings.h>
#include <project.h>
#include <sch_file_versions.h>
#include <schematic.h>
#include <sch_screen.h>
#include <sch_line.h>
#include <sch_label.h>
#include <sch_shape.h>
#include <sch_sheet.h>
#include <sch_symbol.h>
#include <sch_group.h>
#include <sch_pin.h>
#include <sch_rule_area.h>
#include <sch_sheet_path.h>
#include <lib_symbol.h>
#include <pin_map.h>
#include <bitmap_base.h>
#include <sch_bitmap.h>
#include <string_utils.h>
#include <eda_shape.h>
#include <geometry/shape_ellipse.h>
#include <geometry/shape_poly_set.h>


// Schematic format date each feature landed. A rule fires only when the target predates this.
static constexpr int SCH_VER_SHAPE_HATCH = 20250222;
static constexpr int SCH_VER_LOCAL_POWER = 20250227;
static constexpr int SCH_VER_JUMPER_PINS = 20250324;
static constexpr int SCH_VER_GROUPS = 20250513;
static constexpr int SCH_VER_RULE_AREA_FLAGS = 20250610;
static constexpr int SCH_VER_ROUNDED_RECT = 20250829;
static constexpr int SCH_VER_LOCKING = 20260326;
static constexpr int SCH_VER_ELLIPSE = 20260508;
static constexpr int SCH_VER_STACKED_PINS = 20250901;
static constexpr int SCH_VER_STACKED_PIN_ESCAPES = 20260622;
static constexpr int SCH_VER_PIN_MAPS = 20260629;
static constexpr int SCH_VER_BODY_STYLES = 20250827;
static constexpr int SCH_VER_POSITION_EXCLUSIONS = 20260101;
static constexpr int SCH_VER_IMAGE_PPI = 20260623;
static constexpr int SCH_VER_LINE_ENDINGS = 20260818;
static constexpr int SYMLIB_VER_LINE_ENDINGS = 20260710;
static constexpr int SCH_VER_ALTERNATE_SYMBOLS = 20260722;
static constexpr int SCH_VER_CUSTOM_PROPERTIES = 20260830;

// The formats this feature was built for. If a version moves past its constant, a new feature
// landed and the rules or denylist may be stale. Review the keywords, then update these.
static constexpr int SCH_DOWNGRADE_COVERED = 20260830;
static constexpr int SYMLIB_DOWNGRADE_COVERED = 20260830;


static bool isEllipse( const SCH_ITEM* aItem )
{
    if( aItem->Type() != SCH_SHAPE_T )
        return false;

    SHAPE_T shape = static_cast<const SCH_SHAPE*>( aItem )->GetShape();
    return shape == SHAPE_T::ELLIPSE || shape == SHAPE_T::ELLIPSE_ARC;
}


static bool isHatchedFillItem( const SCH_ITEM& aItem )
{
    if( aItem.Type() != SCH_SHAPE_T && aItem.Type() != SCH_TEXTBOX_T )
        return false;

    return static_cast<const SCH_SHAPE&>( aItem ).IsHatchedFill();
}


// A schematic polyline is only closed by repeating the first point.
static void closePolyline( std::vector<VECTOR2I>& aPoints )
{
    if( !aPoints.empty() && aPoints.front() != aPoints.back() )
        aPoints.push_back( aPoints.front() );
}


static void ellipseToPolygon( SCH_SHAPE* aShape )
{
    int  maxError = schIUScale.mmToIU( 0.01 );
    bool isArc = aShape->GetShape() == SHAPE_T::ELLIPSE_ARC;

    SHAPE_ELLIPSE ellipse = isArc ? SHAPE_ELLIPSE( aShape->GetEllipseCenter(), aShape->GetEllipseMajorRadius(),
                                                   aShape->GetEllipseMinorRadius(), aShape->GetEllipseRotation(),
                                                   aShape->GetEllipseStartAngle(), aShape->GetEllipseEndAngle() )
                                  : SHAPE_ELLIPSE( aShape->GetEllipseCenter(), aShape->GetEllipseMajorRadius(),
                                                   aShape->GetEllipseMinorRadius(), aShape->GetEllipseRotation() );

    SHAPE_LINE_CHAIN chain = ellipse.ConvertToPolyline( maxError );

    std::vector<VECTOR2I> points( chain.CPoints().begin(), chain.CPoints().end() );

    // An arc stays open.
    if( !isArc )
        closePolyline( points );

    aShape->SetShape( SHAPE_T::POLY );
    aShape->SetPolyPoints( points );
}


static bool isRoundedRect( const SCH_ITEM* aItem )
{
    if( aItem->Type() != SCH_SHAPE_T )
        return false;

    const SCH_SHAPE* shape = static_cast<const SCH_SHAPE*>( aItem );
    return shape->GetShape() == SHAPE_T::RECTANGLE && shape->GetCornerRadius() > 0;
}


static void roundedRectToPolygon( SCH_SHAPE* aShape )
{
    int maxError = schIUScale.mmToIU( 0.01 );
    int radius = aShape->GetCornerRadius();

    SHAPE_POLY_SET poly;
    poly.NewOutline();

    for( const VECTOR2I& corner : aShape->GetRectCorners() )
        poly.Append( corner );

    poly = poly.Fillet( radius, maxError );

    std::vector<VECTOR2I> points( poly.Outline( 0 ).CPoints().begin(), poly.Outline( 0 ).CPoints().end() );

    closePolyline( points );

    aShape->SetCornerRadius( 0 );
    aShape->SetShape( SHAPE_T::POLY );
    aShape->SetPolyPoints( points );
}


// The escaping only exists since 20260622. KiCad 10.0 splits the notation without honouring
// the backslashes, and 9.0 reads the whole bracket text as one literal number. Either way the
// pins are silently renamed.
static bool hasEscapedStackedPinNumber( const wxString& aNumber )
{
    if( !aNumber.StartsWith( wxT( "[" ) ) )
        return false;

    // Invalid notation is read as one literal number by every KiCad, so it never blocks.
    bool valid = false;
    ExpandStackedPinNotation( aNumber, &valid );

    if( !valid )
        return false;

    for( size_t i = 0; i + 1 < aNumber.length(); ++i )
    {
        if( aNumber[i] != '\\' )
            continue;

        // A backslash escapes exactly the characters EscapeStackedPinItem escapes, so this
        // cannot drift from the notation's structural set.
        wxString next( aNumber[i + 1] );

        if( EscapeStackedPinItem( next ) != next )
            return true;
    }

    return false;
}


// Stacked notation itself only exists since 20250901. A target older than that reads the
// bracket text as one literal pin number, which silently changes the netlist. Escaped numbers
// are counted by their own rule.
static bool usesPlainStackedPinNumber( const wxString& aNumber )
{
    if( hasEscapedStackedPinNumber( aNumber ) )
        return false;

    bool                  valid = false;
    std::vector<wxString> expanded = ExpandStackedPinNotation( aNumber, &valid );

    return valid && !( expanded.size() == 1 && expanded.front() == aNumber );
}


// Per-symbol rule scopes. The screen apply functions delegate to these for the library cache,
// and the standalone .kicad_sym path runs them from the same rule table, so they cannot drift.
static int countSymbolEllipses( const LIB_SYMBOL* aSymbol )
{
    int count = 0;

    for( const SCH_ITEM& item : aSymbol->GetDrawItems() )
    {
        if( isEllipse( &item ) )
            count++;
    }

    return count;
}


static void lowerSymbolEllipses( LIB_SYMBOL* aSymbol )
{
    std::vector<SCH_SHAPE*> ellipses;

    for( SCH_ITEM& item : aSymbol->GetDrawItems() )
    {
        if( isEllipse( &item ) )
            ellipses.push_back( static_cast<SCH_SHAPE*>( &item ) );
    }

    for( SCH_SHAPE* shape : ellipses )
        ellipseToPolygon( shape );
}


static int countSymbolHatchedFills( const LIB_SYMBOL* aSymbol )
{
    int count = 0;

    for( const SCH_ITEM& item : aSymbol->GetDrawItems() )
    {
        if( isHatchedFillItem( item ) )
            count++;
    }

    return count;
}


static void lowerSymbolHatchedFills( LIB_SYMBOL* aSymbol )
{
    for( SCH_ITEM& item : aSymbol->GetDrawItems() )
    {
        if( isHatchedFillItem( item ) )
            static_cast<SCH_SHAPE&>( item ).SetFillMode( FILL_T::FILLED_SHAPE );
    }
}


static int countSymbolPinMaps( const LIB_SYMBOL* aSymbol )
{
    return ( aSymbol->GetPinMaps() != PIN_MAP_SET() || !aSymbol->GetAssociatedFootprints().empty() ) ? 1 : 0;
}


static void dropSymbolPinMaps( LIB_SYMBOL* aSymbol )
{
    aSymbol->SetPinMaps( PIN_MAP_SET() );
    aSymbol->SetAssociatedFootprints( {} );
}


static int countSymbolRoundedRects( const LIB_SYMBOL* aSymbol )
{
    int count = 0;

    for( const SCH_ITEM& item : aSymbol->GetDrawItems() )
    {
        if( isRoundedRect( &item ) )
            count++;
    }

    return count;
}


static void lowerSymbolRoundedRects( LIB_SYMBOL* aSymbol )
{
    std::vector<SCH_SHAPE*> rects;

    for( SCH_ITEM& item : aSymbol->GetDrawItems() )
    {
        if( isRoundedRect( &item ) )
            rects.push_back( static_cast<SCH_SHAPE*>( &item ) );
    }

    for( SCH_SHAPE* rect : rects )
        roundedRectToPolygon( rect );
}


static int countSymbolLocalPower( const LIB_SYMBOL* aSymbol )
{
    return aSymbol->IsLocalPower() ? 1 : 0;
}


static int countSymbolEscapedStackedPins( const LIB_SYMBOL* aSymbol )
{
    int count = 0;

    for( const SCH_PIN* pin : aSymbol->GetGraphicalPins() )
    {
        if( hasEscapedStackedPinNumber( pin->GetNumber() ) )
            count++;
    }

    return count;
}


static int countSymbolStackedPins( const LIB_SYMBOL* aSymbol )
{
    int count = 0;

    for( const SCH_PIN* pin : aSymbol->GetGraphicalPins() )
    {
        if( usesPlainStackedPinNumber( pin->GetNumber() ) )
            count++;
    }

    return count;
}


static int countSymbolJumperPins( const LIB_SYMBOL* aSymbol )
{
    return ( !aSymbol->JumperPinGroups().IsEmpty() || aSymbol->GetDuplicatePinNumbersAreJumpers() ) ? 1 : 0;
}


static void dropSymbolJumperPins( LIB_SYMBOL* aSymbol )
{
    aSymbol->JumperPinGroups().Clear();
    aSymbol->SetDuplicatePinNumbersAreJumpers( false );
}


// DeMorgan pairs survive as the old convert notation. Custom style names and the ability to
// select styles beyond the first two do not; used higher styles are baked separately below.
static int countSymbolNamedBodyStyles( const LIB_SYMBOL* aSymbol )
{
    return ( aSymbol->IsMultiBodyStyle() && !aSymbol->HasDeMorganBodyStyles() ) ? 1 : 0;
}


static void dropSymbolBodyStyleNames( LIB_SYMBOL* aSymbol )
{
    aSymbol->SetBodyStyleNames( {} );
}


static int countSelectedCustomBodyStyles( const SCH_SCREEN* aScreen )
{
    int count = 0;

    for( SCH_ITEM* item : aScreen->Items().OfType( SCH_SYMBOL_T ) )
    {
        if( static_cast<const SCH_SYMBOL*>( item )->GetBodyStyle() > BODY_STYLE::DEMORGAN )
            count++;
    }

    return count;
}


static int countUnresolvedCustomBodyStyles( const SCH_SCREEN* aScreen )
{
    int count = 0;

    for( SCH_ITEM* item : aScreen->Items().OfType( SCH_SYMBOL_T ) )
    {
        const SCH_SYMBOL* symbol = static_cast<const SCH_SYMBOL*>( item );

        if( symbol->GetBodyStyle() > BODY_STYLE::DEMORGAN
            && ( !symbol->GetLibSymbolRef() || symbol->IsMissingLibSymbol() ) )
        {
            count++;
        }
    }

    return count;
}


static void bakeSelectedCustomBodyStyles( SCH_SCREEN* aScreen )
{
    std::vector<SCH_SYMBOL*> symbols;

    for( SCH_ITEM* item : aScreen->Items().OfType( SCH_SYMBOL_T ) )
    {
        if( static_cast<SCH_SYMBOL*>( item )->GetBodyStyle() > BODY_STYLE::DEMORGAN )
            symbols.push_back( static_cast<SCH_SYMBOL*>( item ) );
    }

    std::map<std::pair<wxString, int>, LIB_SYMBOL*> bakedSymbols;

    for( SCH_SYMBOL* symbol : symbols )
    {
        wxCHECK_RET( symbol->GetLibSymbolRef() && !symbol->IsMissingLibSymbol(),
                     wxT( "Cannot bake a body style without its library symbol." ) );

        const wxString sourceName = symbol->GetSchSymbolLibraryName();
        const int      bodyStyle = symbol->GetBodyStyle();
        const auto     key = std::make_pair( sourceName, bodyStyle );
        LIB_SYMBOL*&   baked = bakedSymbols[key];

        if( !baked )
        {
            // Work on a private flattened cache entry, not the source library. Instances
            // selecting other styles must keep their own drawings and pin positions.
            baked = symbol->GetLibSymbolRef()->Flatten().release();
            std::vector<SCH_ITEM*> remove;

            for( SCH_ITEM& item : baked->GetDrawItems() )
            {
                if( item.GetBodyStyle() && item.GetBodyStyle() != bodyStyle )
                    remove.push_back( &item );
                else if( item.GetBodyStyle() == bodyStyle )
                    item.SetBodyStyle( BODY_STYLE::BASE );
            }

            for( SCH_ITEM* item : remove )
                baked->RemoveDrawItem( item );

            baked->SetBodyStyleNames( {} );
            baked->SetHasDeMorganBodyStyles( false );

            const wxString baseName = wxString::Format( wxT( "%s__KiCad9_body_%d" ), sourceName, bodyStyle );
            wxString       name = baseName;

            for( int suffix = 1; aScreen->GetLibSymbols().count( name ); ++suffix )
                name = wxString::Format( wxT( "%s_%d" ), baseName, suffix );

            baked->SetName( name );
            baked->SetLibId( LIB_ID( wxEmptyString, name ) );
            aScreen->AddLibSymbol( baked );
        }

        aScreen->Remove( symbol );
        symbol->SetSchSymbolLibraryName( baked->GetLibId().Format().wx_str() );

        // Change the selector without rebuilding pins against the OLD base style. Rebuild
        // once against the baked symbol so the selected pins keep their numbers and UUIDs.
        symbol->SCH_ITEM::SetBodyStyle( BODY_STYLE::BASE );
        symbol->SetLibSymbol( new LIB_SYMBOL( *baked ) );
        aScreen->Append( symbol );
    }
}


static int countSymbolPositionExclusions( const LIB_SYMBOL* aSymbol )
{
    return aSymbol->GetExcludedFromPosFiles() ? 1 : 0;
}


static void dropSymbolPositionExclusions( LIB_SYMBOL* aSymbol )
{
    aSymbol->SetExcludedFromPosFiles( false );
}


static int countPositionExclusions( const SCH_SCREEN* aScreen )
{
    int count = 0;

    for( SCH_ITEM* item : aScreen->Items().OfType( SCH_SYMBOL_T ) )
    {
        if( static_cast<const SCH_SYMBOL*>( item )->GetExcludedFromPosFiles() )
            count++;
    }

    for( const auto& [name, symbol] : aScreen->GetLibSymbols() )
    {
        if( symbol )
            count += countSymbolPositionExclusions( symbol );
    }

    return count;
}


static void dropPositionExclusions( SCH_SCREEN* aScreen )
{
    for( SCH_ITEM* item : aScreen->Items().OfType( SCH_SYMBOL_T ) )
        static_cast<SCH_SYMBOL*>( item )->SetExcludedFromPosFiles( false );

    for( const auto& [name, symbol] : aScreen->GetLibSymbols() )
    {
        if( symbol )
            dropSymbolPositionExclusions( symbol );
    }
}


// Group locks shipped with group support (20250513) and predate general item locking, so
// groups are the group rule's business, not this one's. A lock inherited from a locked group
// survives through the group itself and is not a loss.
static bool hasOwnLock( const SCH_ITEM* aItem )
{
    if( aItem->Type() == SCH_GROUP_T )
        return false;

    if( EDA_GROUP* group = aItem->GetParentGroup() )
    {
        if( group->AsEdaItem()->IsLocked() )
            return false;
    }

    return aItem->IsLocked();
}


static int countLockedItems( const SCH_SCREEN* aScreen )
{
    int count = 0;

    for( SCH_ITEM* item : aScreen->Items() )
    {
        if( hasOwnLock( item ) )
            count++;
    }

    return count;
}


static void unlockItems( SCH_SCREEN* aScreen )
{
    for( SCH_ITEM* item : aScreen->Items() )
    {
        if( hasOwnLock( item ) )
            item->SetLocked( false );
    }
}


static int countEllipses( const SCH_SCREEN* aScreen )
{
    int count = 0;

    for( SCH_ITEM* item : aScreen->Items().OfType( SCH_SHAPE_T ) )
    {
        if( isEllipse( item ) )
            count++;
    }

    // Symbol bodies are written from the screen library cache, so ellipses there count too.
    for( const auto& [name, symbol] : aScreen->GetLibSymbols() )
    {
        if( symbol )
            count += countSymbolEllipses( symbol );
    }

    return count;
}


static void lowerEllipses( SCH_SCREEN* aScreen )
{
    // Collect first, then mutate. Changing items while iterating the R-tree is not safe.
    std::vector<SCH_SHAPE*> ellipses;

    for( SCH_ITEM* item : aScreen->Items().OfType( SCH_SHAPE_T ) )
    {
        if( isEllipse( item ) )
            ellipses.push_back( static_cast<SCH_SHAPE*>( item ) );
    }

    for( SCH_SHAPE* shape : ellipses )
        ellipseToPolygon( shape );

    for( const auto& [name, symbol] : aScreen->GetLibSymbols() )
    {
        if( symbol )
            lowerSymbolEllipses( symbol );
    }
}


// Every fill-bearing schematic item derives from SCH_SHAPE but has its own type id.
static constexpr KICAD_T FILLABLE_TYPES[] = { SCH_SHAPE_T, SCH_TEXTBOX_T, SCH_RULE_AREA_T };


static int countHatchedFills( const SCH_SCREEN* aScreen )
{
    int count = 0;

    for( KICAD_T type : FILLABLE_TYPES )
    {
        for( SCH_ITEM* item : aScreen->Items().OfType( type ) )
        {
            if( static_cast<SCH_SHAPE*>( item )->IsHatchedFill() )
                count++;
        }
    }

    for( const auto& [name, symbol] : aScreen->GetLibSymbols() )
    {
        if( symbol )
            count += countSymbolHatchedFills( symbol );
    }

    return count;
}


static void lowerHatchedFills( SCH_SCREEN* aScreen )
{
    for( KICAD_T type : FILLABLE_TYPES )
    {
        for( SCH_ITEM* item : aScreen->Items().OfType( type ) )
        {
            SCH_SHAPE* shape = static_cast<SCH_SHAPE*>( item );

            if( shape->IsHatchedFill() )
                shape->SetFillMode( FILL_T::FILLED_SHAPE );
        }
    }

    for( const auto& [name, symbol] : aScreen->GetLibSymbols() )
    {
        if( symbol )
            lowerSymbolHatchedFills( symbol );
    }
}


static bool hasPinMaps( const SCH_SYMBOL* aSymbol )
{
    if( !aSymbol->GetPinMapOverride().IsDefault() )
        return true;

    for( const SCH_SYMBOL_INSTANCE& instance : aSymbol->GetInstances() )
    {
        for( const auto& [name, variant] : instance.m_Variants )
        {
            if( !variant.m_PinMapOverride.IsDefault() )
                return true;
        }
    }

    const std::unique_ptr<LIB_SYMBOL>& lib = aSymbol->GetLibSymbolRef();
    return lib && ( lib->GetPinMaps() != PIN_MAP_SET() || !lib->GetAssociatedFootprints().empty() );
}


static int countPinMaps( const SCH_SCREEN* aScreen )
{
    int count = 0;

    for( SCH_ITEM* item : aScreen->Items().OfType( SCH_SYMBOL_T ) )
    {
        if( hasPinMaps( static_cast<const SCH_SYMBOL*>( item ) ) )
            count++;
    }

    // The transform strips the library cache too, so cache-only maps count as well.
    for( const auto& [name, symbol] : aScreen->GetLibSymbols() )
    {
        if( symbol )
            count += countSymbolPinMaps( symbol );
    }

    return count;
}


static void dropPinMaps( SCH_SCREEN* aScreen )
{
    for( SCH_ITEM* item : aScreen->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        symbol->ClearPinMapOverrides();

        if( symbol->GetLibSymbolRef() )
            dropSymbolPinMaps( symbol->GetLibSymbolRef().get() );
    }

    for( const auto& [name, symbol] : aScreen->GetLibSymbols() )
    {
        if( symbol )
            dropSymbolPinMaps( symbol );
    }
}


static int countNamedBodyStyles( const SCH_SCREEN* aScreen )
{
    int count = 0;

    for( const auto& [name, symbol] : aScreen->GetLibSymbols() )
    {
        if( symbol )
            count += countSymbolNamedBodyStyles( symbol );
    }

    return count;
}


static void dropNamedBodyStyles( SCH_SCREEN* aScreen )
{
    for( const auto& [name, symbol] : aScreen->GetLibSymbols() )
    {
        if( symbol )
            dropSymbolBodyStyleNames( symbol );
    }
}


static int countGroups( const SCH_SCREEN* aScreen )
{
    auto groups = aScreen->Items().OfType( SCH_GROUP_T );
    return static_cast<int>( std::distance( groups.begin(), groups.end() ) );
}


static void ungroupAll( SCH_SCREEN* aScreen )
{
    std::vector<SCH_GROUP*> groups;

    for( SCH_ITEM* item : aScreen->Items().OfType( SCH_GROUP_T ) )
        groups.push_back( static_cast<SCH_GROUP*>( item ) );

    // Detach every member first. Groups can nest, and deleting an inner group while an outer
    // group still lists it would leave the outer group holding a freed pointer.
    for( SCH_GROUP* group : groups )
        group->RemoveAll();

    for( SCH_GROUP* group : groups )
    {
        aScreen->Remove( group );
        delete group;
    }
}


static int countJumperPinGroups( const SCH_SCREEN* aScreen )
{
    int count = 0;

    for( const auto& [name, symbol] : aScreen->GetLibSymbols() )
    {
        if( symbol )
            count += countSymbolJumperPins( symbol );
    }

    return count;
}


static void dropJumperPinGroups( SCH_SCREEN* aScreen )
{
    for( const auto& [name, symbol] : aScreen->GetLibSymbols() )
    {
        if( symbol )
            dropSymbolJumperPins( symbol );
    }
}


static int countRoundedRects( const SCH_SCREEN* aScreen )
{
    int count = 0;

    for( SCH_ITEM* item : aScreen->Items().OfType( SCH_SHAPE_T ) )
    {
        if( isRoundedRect( item ) )
            count++;
    }

    for( const auto& [name, symbol] : aScreen->GetLibSymbols() )
    {
        if( symbol )
            count += countSymbolRoundedRects( symbol );
    }

    return count;
}


static void lowerRoundedRects( SCH_SCREEN* aScreen )
{
    // Collect first, then mutate. Changing items while iterating the R-tree is not safe.
    std::vector<SCH_SHAPE*> rects;

    for( SCH_ITEM* item : aScreen->Items().OfType( SCH_SHAPE_T ) )
    {
        if( isRoundedRect( item ) )
            rects.push_back( static_cast<SCH_SHAPE*>( item ) );
    }

    for( SCH_SHAPE* rect : rects )
        roundedRectToPolygon( rect );

    for( const auto& [name, symbol] : aScreen->GetLibSymbols() )
    {
        if( symbol )
            lowerSymbolRoundedRects( symbol );
    }
}


static int countLocalPowerSymbols( const SCH_SCREEN* aScreen )
{
    int count = 0;

    for( const auto& [name, symbol] : aScreen->GetLibSymbols() )
    {
        if( symbol )
            count += countSymbolLocalPower( symbol );
    }

    return count;
}


static bool hasRuleAreaFlags( const SCH_RULE_AREA* aArea )
{
    return aArea->GetDNP() || aArea->GetExcludedFromSim() || aArea->GetExcludedFromBOM()
           || aArea->GetExcludedFromBoard();
}


static int countRuleAreaFlags( const SCH_SCREEN* aScreen )
{
    int count = 0;

    for( SCH_ITEM* item : aScreen->Items().OfType( SCH_RULE_AREA_T ) )
    {
        if( hasRuleAreaFlags( static_cast<const SCH_RULE_AREA*>( item ) ) )
            count++;
    }

    return count;
}


static void dropRuleAreaFlags( SCH_SCREEN* aScreen )
{
    for( SCH_ITEM* item : aScreen->Items().OfType( SCH_RULE_AREA_T ) )
    {
        SCH_RULE_AREA* area = static_cast<SCH_RULE_AREA*>( item );

        area->SetDNP( false );
        area->SetExcludedFromSim( false );
        area->SetExcludedFromBOM( false );
        area->SetExcludedFromBoard( false );
    }
}


static int countEscapedStackedPins( const SCH_SCREEN* aScreen )
{
    int count = 0;

    for( const auto& [name, symbol] : aScreen->GetLibSymbols() )
    {
        if( symbol )
            count += countSymbolEscapedStackedPins( symbol );
    }

    return count;
}


static int countStackedPins( const SCH_SCREEN* aScreen )
{
    int count = 0;

    for( const auto& [name, symbol] : aScreen->GetLibSymbols() )
    {
        if( symbol )
            count += countSymbolStackedPins( symbol );
    }

    return count;
}


static void omitSymbolGraphics( LIB_SYMBOL* aSymbol, const std::function<bool( const SCH_ITEM* )>& aMatches )
{
    std::vector<SCH_ITEM*> remove;

    for( SCH_ITEM& item : aSymbol->GetDrawItems() )
    {
        if( aMatches( &item ) )
            remove.push_back( &item );
    }

    // RemoveDrawItem owns and deletes library drawings.
    for( SCH_ITEM* item : remove )
        aSymbol->RemoveDrawItem( item );
}


static void omitScreenItems( SCH_SCREEN* aScreen, const std::function<bool( const SCH_ITEM* )>& aMatches )
{
    std::vector<SCH_ITEM*> remove;

    for( SCH_ITEM* item : aScreen->Items() )
    {
        if( aMatches( item ) )
            remove.push_back( item );
    }

    for( SCH_ITEM* item : remove )
    {
        // SCH_SCREEN::Remove does not detach group membership.
        if( EDA_GROUP* group = item->GetParentGroup() )
            group->RemoveItem( item );

        aScreen->Remove( item );
        delete item;
    }
}


static void omitSelectedCustomBodyStyles( SCH_SCREEN* aScreen )
{
    omitScreenItems( aScreen,
                     []( const SCH_ITEM* aItem )
                     {
                         return aItem->Type() == SCH_SYMBOL_T && aItem->GetBodyStyle() > BODY_STYLE::DEMORGAN;
                     } );
}


static void omitSymbolEllipses( LIB_SYMBOL* aSymbol )
{
    omitSymbolGraphics( aSymbol, isEllipse );
}
static void omitSymbolRoundedRects( LIB_SYMBOL* aSymbol )
{
    omitSymbolGraphics( aSymbol, isRoundedRect );
}


static void omitEllipses( SCH_SCREEN* aScreen )
{
    omitScreenItems( aScreen, isEllipse );

    for( const auto& [name, symbol] : aScreen->GetLibSymbols() )
    {
        if( symbol )
            omitSymbolEllipses( symbol );
    }
}


static void omitRoundedRects( SCH_SCREEN* aScreen )
{
    omitScreenItems( aScreen, isRoundedRect );

    for( const auto& [name, symbol] : aScreen->GetLibSymbols() )
    {
        if( symbol )
            omitSymbolRoundedRects( symbol );
    }
}


static void omitSymbolHatchedFills( LIB_SYMBOL* aSymbol )
{
    for( SCH_ITEM& item : aSymbol->GetDrawItems() )
    {
        if( isHatchedFillItem( item ) )
            static_cast<SCH_SHAPE&>( item ).SetFillMode( FILL_T::NO_FILL );
    }
}


static void omitHatchedFills( SCH_SCREEN* aScreen )
{
    for( KICAD_T type : FILLABLE_TYPES )
    {
        for( SCH_ITEM* item : aScreen->Items().OfType( type ) )
        {
            SCH_SHAPE* shape = static_cast<SCH_SHAPE*>( item );

            if( shape->IsHatchedFill() )
                shape->SetFillMode( FILL_T::NO_FILL );
        }
    }

    for( const auto& [name, symbol] : aScreen->GetLibSymbols() )
    {
        if( symbol )
            omitSymbolHatchedFills( symbol );
    }
}


static void visitSymbolItems( const LIB_SYMBOL* aSymbol, const std::function<void( EDA_ITEM* )>& aVisit )
{
    aVisit( const_cast<LIB_SYMBOL*>( aSymbol ) );

    for( const SCH_ITEM& item : aSymbol->GetDrawItems() )
        aVisit( const_cast<SCH_ITEM*>( &item ) );
}


static void visitScreenItems( const SCH_SCREEN* aScreen, const std::function<void( EDA_ITEM* )>& aVisit )
{
    std::set<EDA_ITEM*> visited;
    auto                visit = [&]( EDA_ITEM* item )
    {
        if( visited.insert( item ).second )
            aVisit( item );
    };

    for( SCH_ITEM* item : aScreen->Items() )
    {
        visit( item );
        item->RunOnChildren(
                [&]( SCH_ITEM* child )
                {
                    visit( child );
                },
                RECURSE );
    }

    for( const auto& [name, symbol] : aScreen->GetLibSymbols() )
    {
        if( symbol )
            visitSymbolItems( symbol, visit );
    }
}


static bool isPostTargetTextVar( const wxString& aToken )
{
    const wxString token = aToken.AfterLast( ':' );
    return token.StartsWith( wxT( "PROPERTY." ) ) || token == wxT( "SYMBOL_IS_POWER" )
           || token == wxT( "SYMBOL_IS_LOCAL_POWER" );
}


static bool hasPostTargetTextVars( const wxString& aText )
{
    bool                             found = false;
    std::function<bool( wxString* )> scan = [&]( wxString* token )
    {
        found |= isPostTargetTextVar( *token );
        return false;
    };
    ExpandTextVars( aText, &scan, INTERNAL );
    return found;
}


static bool hasActiveExpression( const wxString& aText )
{
    std::function<bool( wxString* )> preserve = []( wxString* )
    {
        return false;
    };
    return ExpandTextVars( aText, &preserve, INTERNAL ).Contains( wxT( "@{" ) );
}


static bool hasEscapedTextControl( const wxString& aText )
{
    return aText.Contains( wxT( "\\${" ) ) || aText.Contains( wxT( "\\@{" ) );
}


static bool textNeedsDowngrade( const wxString& aText, const PROJECT* aProject, bool aLegacyExpressions,
                                std::set<wxString>& aVisiting, const TITLE_BLOCK* aTitles = nullptr )
{
    bool found = aLegacyExpressions && ( hasActiveExpression( aText ) || hasEscapedTextControl( aText ) );
    std::function<bool( wxString* )> scan = [&]( wxString* token )
    {
        found |= isPostTargetTextVar( *token );

        if( aProject )
        {
            const auto& vars = aProject->GetTextVars();
            auto        value = vars.find( *token );

            if( value != vars.end() && aVisiting.insert( *token ).second )
            {
                found |= textNeedsDowngrade( value->second, aProject, aLegacyExpressions, aVisiting, aTitles );
                aVisiting.erase( *token );
            }
        }

        if( aTitles && aVisiting.insert( *token ).second )
        {
            wxString value;

            if( *token == wxT( "TITLE" ) )
                value = aTitles->GetTitle();
            else if( *token == wxT( "ISSUE_DATE" ) )
                value = aTitles->GetDate();
            else if( *token == wxT( "REVISION" ) )
                value = aTitles->GetRevision();
            else if( *token == wxT( "COMPANY" ) )
                value = aTitles->GetCompany();
            else if( token->length() == 8 && token->StartsWith( wxT( "COMMENT" ) ) && ( *token )[7] >= '1'
                     && ( *token )[7] <= '9' )
                value = aTitles->GetComment( ( *token )[7].GetValue() - '1' );

            found |= textNeedsDowngrade( value, aProject, aLegacyExpressions, aVisiting, aTitles );
            aVisiting.erase( *token );
        }

        return false;
    };
    ExpandTextVars( aText, &scan, INTERNAL );
    return found;
}


static wxString bakeLegacyExpressions( wxString aText, const SCH_ITEM* aItem, const SCH_SHEET_PATH* aPath,
                                       bool& aFailed )
{
    for( size_t pos = 0; pos + 1 < aText.length(); ++pos )
    {
        if( aText[pos] == '\\' && ( aText[pos + 1] == '$' || aText[pos + 1] == '@' ) )
        {
            ++pos;
            continue;
        }

        if( aText[pos] != '@' || aText[pos + 1] != '{' )
            continue;

        size_t end = pos + 2;
        int    depth = 1;

        for( ; end < aText.length() && depth; ++end )
        {
            if( aText[end] == '{' )
                ++depth;
            else if( aText[end] == '}' )
                --depth;
        }

        if( depth )
        {
            aFailed = true;
            return aText;
        }

        const wxString expression = aText.Mid( pos, end - pos );
        wxString       value = aItem->ResolveText( expression, aPath, 0, wxEmptyString );
        FinalizeTextVarExpansion( value, INTERNAL );

        if( value == expression || hasActiveExpression( value ) || hasPostTargetTextVars( value )
            || !ExtractTextVarReferences( value ).empty() || value.Contains( wxT( "<Unresolved:" ) ) )
        {
            aFailed = true;
            return aText;
        }

        aText = aText.Left( pos ) + value + aText.Mid( end );
        pos += value.length();

        if( pos )
            --pos;
    }

    return aText;
}


struct TEXT_DOWNGRADE_PLAN
{
    std::map<EDA_TEXT*, wxString> m_replacements;
    int                           m_blocked = 0;
};


static void planTextDowngrade( EDA_ITEM* aItem, const std::vector<SCH_SHEET_PATH>& aPaths, bool aLibrary,
                               bool aLegacyExpressions, TEXT_DOWNGRADE_PLAN& aPlan,
                               const TITLE_BLOCK* aTitles = nullptr )
{
    EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( aItem );
    SCH_ITEM* item = dynamic_cast<SCH_ITEM*>( aItem );

    if( !text || !item )
        return;

    const SCHEMATIC*   schematic = item->Schematic();
    const PROJECT*     project = schematic && schematic->IsValid() ? &schematic->Project() : nullptr;
    std::set<wxString> visiting;

    if( !textNeedsDowngrade( text->GetText(), project, aLegacyExpressions, visiting, aTitles ) )
        return;

    if( aLegacyExpressions && hasEscapedTextControl( text->GetText() ) )
    {
        ++aPlan.m_blocked;
        return;
    }

    if( !aLibrary && ( aPaths.empty() || !project ) )
    {
        ++aPlan.m_blocked;
        return;
    }

    std::optional<wxString> replacement;
    bool                    failed = false;
    auto                    resolve = [&]( const SCH_SHEET_PATH* path )
    {
        std::function<bool( wxString* )> resolver = [&]( wxString* token )
        {
            std::set<wxString> dependencies;

            if( !textNeedsDowngrade( wxT( "${" ) + *token + wxT( "}" ), project, aLegacyExpressions, dependencies,
                                     aTitles ) )
                return false;

            const wxString reference = wxT( "${" ) + *token + wxT( "}" );
            wxString       value = item->ResolveText( reference, path, 0, wxEmptyString );
            FinalizeTextVarExpansion( value, INTERNAL );

            if( value == reference || hasPostTargetTextVars( value ) || !ExtractTextVarReferences( value ).empty()
                || value.Contains( wxT( "<Unresolved:" ) ) || ( aLegacyExpressions && hasEscapedTextControl( value ) ) )
            {
                failed = true;
                return false;
            }

            *token = value;
            return true;
        };
        wxString value = ExpandTextVars( text->GetText(), &resolver, INTERNAL );

        if( aLegacyExpressions )
            value = bakeLegacyExpressions( value, item, path, failed );

        FinalizeTextVarExpansion( value, INTERNAL );

        if( hasPostTargetTextVars( value ) || ( replacement && *replacement != value ) )
            failed = true;

        replacement = value;
    };

    if( aLibrary )
        resolve( nullptr );
    else
    {
        for( const SCH_SHEET_PATH& path : aPaths )
            resolve( &path );
    }

    if( failed || !replacement )
        ++aPlan.m_blocked;
    else
        aPlan.m_replacements.emplace( text, *replacement );
}


static TEXT_DOWNGRADE_PLAN symbolTextPlan( const LIB_SYMBOL* aSymbol, bool aLegacyExpressions )
{
    TEXT_DOWNGRADE_PLAN plan;
    visitSymbolItems( aSymbol,
                      [&]( EDA_ITEM* item )
                      {
                          planTextDowngrade( item, {}, true, aLegacyExpressions, plan );
                      } );
    return plan;
}


static TEXT_DOWNGRADE_PLAN screenTextPlan( const SCH_SCREEN* aScreen, bool aLegacyExpressions )
{
    TEXT_DOWNGRADE_PLAN         plan;
    std::vector<SCH_SHEET_PATH> paths;

    const SCHEMATIC* owner = aScreen->GetParent() && aScreen->GetParent()->Type() == SCHEMATIC_T
                                     ? static_cast<const SCHEMATIC*>( aScreen->GetParent() )
                                     : nullptr;

    if( const SCHEMATIC* schematic = owner )
    {
        for( const SCH_SHEET_PATH& path : schematic->BuildUnorderedSheetList() )
        {
            if( path.LastScreen() == aScreen )
                paths.push_back( path );
        }
    }

    std::set<EDA_ITEM*> visited;
    auto                planItem = [&]( SCH_ITEM* item )
    {
        if( visited.insert( item ).second )
            planTextDowngrade( item, paths, false, aLegacyExpressions, plan, &aScreen->GetTitleBlock() );
    };

    for( SCH_ITEM* item : aScreen->Items() )
    {
        planItem( item );
        item->RunOnChildren(
                [&]( SCH_ITEM* child )
                {
                    planItem( child );
                },
                RECURSE );
    }

    for( const auto& [name, symbol] : aScreen->GetLibSymbols() )
    {
        if( symbol )
            visitSymbolItems( symbol,
                              [&]( EDA_ITEM* item )
                              {
                                  planTextDowngrade( item, {}, true, aLegacyExpressions, plan );
                              } );
    }

    const SCHEMATIC*      schematic = owner;
    const PROJECT*        project = schematic && schematic->IsValid() ? &schematic->Project() : nullptr;
    const TITLE_BLOCK&    titles = aScreen->GetTitleBlock();
    std::vector<wxString> borderText = { titles.GetTitle(), titles.GetDate(), titles.GetRevision(),
                                         titles.GetCompany() };

    for( int i = 0; i < 9; ++i )
        borderText.push_back( titles.GetComment( i ) );

    for( const wxString& value : borderText )
    {
        std::set<wxString> visiting;

        if( textNeedsDowngrade( value, project, aLegacyExpressions, visiting, &titles ) )
            ++plan.m_blocked;
    }

    return plan;
}


static void reportTextPlan( const TEXT_DOWNGRADE_PLAN& aPlan, COMPATIBILITY_REPORT& aReport )
{
    if( !aPlan.m_replacements.empty() )
    {
        aReport.Add( DOWNGRADE_BUCKET::DROP, _( "Post-target text variables" ),
                     _( "Dynamic evaluation of unsupported references and expressions is removed. Resolved literal "
                        "values are kept." ),
                     aPlan.m_replacements.size() );
    }

    if( aPlan.m_blocked )
    {
        aReport.Add( DOWNGRADE_BUCKET::BLOCK, _( "Unresolved post-target text variables" ),
                     _( "The target cannot preserve these references, literal escapes or page-border expressions and a "
                        "reliable compatible value is unavailable." ),
                     aPlan.m_blocked );
    }
}


static void applyTextPlan( const TEXT_DOWNGRADE_PLAN& aPlan )
{
    for( const auto& [text, value] : aPlan.m_replacements )
        text->SetText( value );
}


static bool hasPaddedBusVector( const wxString& aText )
{
    if( NET_SETTINGS::ParseBusVector( aText, nullptr, nullptr ) )
    {
        const wxString range = aText.AfterFirst( '[' ).BeforeFirst( ']' );
        const wxString begin = range.BeforeFirst( '.' );
        const wxString end = range.AfterLast( '.' );
        return ( begin.length() > 1 && begin[0] == '0' ) || ( end.length() > 1 && end[0] == '0' );
    }

    std::vector<wxString> members;

    if( NET_SETTINGS::ParseBusGroup( aText, nullptr, &members ) )
    {
        for( const wxString& member : members )
        {
            if( hasPaddedBusVector( member ) )
                return true;
        }
    }

    return false;
}


struct FROZEN_BUS_VECTOR
{
    bool     m_valid = false;
    wxString m_prefix;
    long     m_begin = 0;
    long     m_end = 0;
};


// Acceptance and prefix logic from the released parsers, without their unsafe expansion loop.
static FROZEN_BUS_VECTOR frozenBusVector( const wxString& aBus, bool aPreTen )
{
    FROZEN_BUS_VECTOR result;
    const size_t      length = aBus.length();
    size_t            i = 0;
    int               nesting = 0;
    bool              quoted = false;
    auto              digit = []( wxUniChar c )
    {
        return c >= '0' && c <= '9';
    };
    auto formatting = []( wxUniChar c )
    {
        return c == '_' || c == '^' || c == '~';
    };
    auto escaped = [&]( size_t pos )
    {
        size_t slashes = 0;
        for( ; pos && aBus[pos - 1] == '\\'; --pos )
            ++slashes;
        return slashes % 2 == 1;
    };

    for( ; i < length; ++i )
    {
        if( !aPreTen && aBus[i] == '"' && !escaped( i ) )
        {
            quoted = !quoted;
            continue;
        }

        if( quoted )
        {
            if( aBus[i] == '\\' && i + 1 < length )
                result.m_prefix += aBus[++i];
            else
                result.m_prefix += aBus[i];
            continue;
        }

        if( aBus[i] == '{' )
        {
            if( i == 0 || !formatting( aBus[i - 1] ) )
                return result;
            ++nesting;
            if( !aPreTen )
            {
                if( !result.m_prefix.IsEmpty() )
                    result.m_prefix.RemoveLast();
                continue;
            }
        }
        else if( aBus[i] == '}' )
        {
            --nesting;
            if( !aPreTen )
                continue;
        }

        if( !aPreTen && aBus[i] == '\\' && i + 1 < length && aBus[i + 1] == ' ' )
        {
            result.m_prefix += aBus[++i];
            continue;
        }

        if( aBus[i] == ' ' || aBus[i] == ']' )
            return result;
        if( aBus[i] == '[' )
            break;
        result.m_prefix += aBus[i];
    }

    if( ++i >= length )
        return result;
    wxString number;

    for( ; i < length; ++i )
    {
        if( aBus[i] == '.' && i + 1 < length && aBus[i + 1] == '.' )
        {
            number.ToLong( &result.m_begin );
            i += 2;
            break;
        }
        if( !digit( aBus[i] ) )
            return result;
        number += aBus[i];
    }

    if( i >= length )
        return result;
    number.clear();

    for( ; i < length; ++i )
    {
        if( aBus[i] == ']' )
        {
            number.ToLong( &result.m_end );
            ++i;
            break;
        }
        if( !digit( aBus[i] ) )
            return result;
        number += aBus[i];
    }

    for( ; i < length; ++i )
    {
        if( aBus[i] == '}' )
            --nesting;
        else if( aPreTen || ( aBus[i] != '+' && aBus[i] != '-' && aBus[i] != 'P' && aBus[i] != 'N' ) )
            return result;
    }

    result.m_valid = nesting == 0 && result.m_begin != result.m_end;
    return result;
}


static bool hasIncompatibleBusVector( const wxString& aText, bool aPreTen )
{
    wxString                currentPrefix;
    bool                    current = NET_SETTINGS::ParseBusVector( aText, &currentPrefix, nullptr );
    const FROZEN_BUS_VECTOR legacy = frozenBusVector( aText, aPreTen );

    if( current != legacy.m_valid || ( legacy.m_valid && ( legacy.m_begin == LONG_MAX || legacy.m_end == LONG_MAX ) )
        || ( current && legacy.m_prefix != currentPrefix ) )
        return true;

    std::vector<wxString> members;
    if( NET_SETTINGS::ParseBusGroup( aText, nullptr, &members ) )
    {
        for( const wxString& member : members )
        {
            if( hasIncompatibleBusVector( member, aPreTen ) )
                return true;
        }
    }
    return false;
}


static int countIncompatibleBusLabels( const SCH_SCREEN* aScreen, bool aPreTen )
{
    int              count = 0;
    const SCHEMATIC* schematic = aScreen->GetParent() && aScreen->GetParent()->Type() == SCHEMATIC_T
                                         ? static_cast<const SCHEMATIC*>( aScreen->GetParent() )
                                         : nullptr;
    visitScreenItems( aScreen,
                      [&]( EDA_ITEM* item )
                      {
                          if( item->Type() != SCH_LABEL_T && item->Type() != SCH_GLOBAL_LABEL_T
                              && item->Type() != SCH_HIER_LABEL_T && item->Type() != SCH_SHEET_PIN_T )
                              return;
                          const EDA_TEXT*       text = dynamic_cast<const EDA_TEXT*>( item );
                          const SCH_LABEL_BASE* label = dynamic_cast<const SCH_LABEL_BASE*>( item );
                          if( !text || !label )
                              return;
                          bool incompatible = hasIncompatibleBusVector( text->GetText(), aPreTen );
                          if( schematic && schematic->IsValid() )
                          {
                              for( const SCH_SHEET_PATH& path : schematic->BuildUnorderedSheetList() )
                              {
                                  if( path.LastScreen() != aScreen )
                                      continue;
                                  wxString resolved = label->GetShownText( &path, FOR_NETNAME );
                                  incompatible |= hasIncompatibleBusVector( resolved, aPreTen )
                                                  || hasPaddedBusVector( resolved );
                              }
                          }
                          count += incompatible;
                      } );
    return count;
}


static int countPaddedBusLabels( const SCH_SCREEN* aScreen )
{
    int count = 0;
    visitScreenItems( aScreen,
                      [&]( EDA_ITEM* item )
                      {
                          if( item->Type() == SCH_LABEL_T || item->Type() == SCH_GLOBAL_LABEL_T
                              || item->Type() == SCH_HIER_LABEL_T || item->Type() == SCH_DIRECTIVE_LABEL_T
                              || item->Type() == SCH_SHEET_PIN_T )
                          {
                              if( EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( item ) )
                                  count += hasPaddedBusVector( text->GetText() );
                          }
                      } );
    return count;
}


static void reportPaddedBuses( int aCount, COMPATIBILITY_REPORT& aReport )
{
    if( aCount )
    {
        aReport.Add( DOWNGRADE_BUCKET::BLOCK, _( "Zero-padded bus vectors" ),
                     _( "The target removes the leading zeros from bus members and would change their net names." ),
                     aCount );
    }
}


static void reportIncompatibleBuses( int aCount, COMPATIBILITY_REPORT& aReport )
{
    if( aCount )
        aReport.Add( DOWNGRADE_BUCKET::BLOCK, _( "Incompatible bus-vector parsing" ),
                     _( "The target would reinterpret a literal as a bus, rename formatted bus members or overflow "
                        "while expanding the range." ),
                     aCount );
}


static int countSymbolCustomProperties( const LIB_SYMBOL* aSymbol )
{
    int count = 0;
    visitSymbolItems( aSymbol,
                      [&]( EDA_ITEM* item )
                      {
                          count += item->GetCustomProperties().size();
                      } );
    return count;
}


static int countCustomProperties( const SCH_SCREEN* aScreen )
{
    int count = 0;
    visitScreenItems( aScreen,
                      [&]( EDA_ITEM* item )
                      {
                          count += item->GetCustomProperties().size();
                      } );
    return count;
}


static void dropSymbolCustomProperties( LIB_SYMBOL* aSymbol )
{
    visitSymbolItems( aSymbol,
                      []( EDA_ITEM* item )
                      {
                          item->ClearCustomProperties();
                      } );
}


static void dropCustomProperties( SCH_SCREEN* aScreen )
{
    visitScreenItems( aScreen,
                      []( EDA_ITEM* item )
                      {
                          item->ClearCustomProperties();
                      } );

    for( SCH_ITEM* item : aScreen->Items().OfType( SCH_SYMBOL_T ) )
    {
        if( LIB_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item )->GetLibSymbolRef().get() )
            dropSymbolCustomProperties( symbol );
    }
}


static int countAlternateSymbols( const SCH_SCREEN* aScreen )
{
    int count = 0;

    for( SCH_ITEM* item : aScreen->Items().OfType( SCH_SYMBOL_T ) )
    {
        for( const SCH_SYMBOL_INSTANCE& instance : static_cast<SCH_SYMBOL*>( item )->GetInstances() )
        {
            for( const auto& [name, variant] : instance.m_Variants )
                count += variant.m_SymbolOverride.has_value();
        }
    }

    return count;
}


static void dropAlternateSymbols( SCH_SCREEN* aScreen )
{
    for( SCH_ITEM* item : aScreen->Items().OfType( SCH_SYMBOL_T ) )
    {
        auto& instances =
                const_cast<std::vector<SCH_SYMBOL_INSTANCE>&>( static_cast<SCH_SYMBOL*>( item )->GetInstances() );

        for( SCH_SYMBOL_INSTANCE& instance : instances )
        {
            for( auto& [name, variant] : instance.m_Variants )
                variant.m_SymbolOverride.reset();
        }
    }
}


static bool hasLineEndings( const SCH_ITEM* aItem )
{
    if( const SCH_SHAPE* shape = dynamic_cast<const SCH_SHAPE*>( aItem ) )
        return shape->GetStartEndingStyle() != LINE_ENDING_STYLE::NONE
               || shape->GetEndEndingStyle() != LINE_ENDING_STYLE::NONE;

    if( const SCH_LINE* line = dynamic_cast<const SCH_LINE*>( aItem ) )
        return line->GetStartEndingStyle() != LINE_ENDING_STYLE::NONE
               || line->GetEndEndingStyle() != LINE_ENDING_STYLE::NONE;

    return false;
}


static SCH_SHAPE lineEndingShape( const SCH_ITEM* aItem )
{
    if( const SCH_SHAPE* shape = dynamic_cast<const SCH_SHAPE*>( aItem ) )
        return *shape;

    const SCH_LINE* line = static_cast<const SCH_LINE*>( aItem );
    SCH_SHAPE       shape( SHAPE_T::SEGMENT );
    shape.SetStart( line->GetStartPoint() );
    shape.SetEnd( line->GetEndPoint() );
    STROKE_PARAMS stroke = line->GetStroke();
    stroke.SetWidth( line->GetPenWidth() );
    shape.SetStroke( stroke );
    shape.SetStartEnding( line->GetStartEnding() );
    shape.SetEndEnding( line->GetEndEnding() );
    shape.SetLayer( line->GetLayer() );
    return shape;
}


static bool canLowerLineEndings( const SCH_ITEM* aItem )
{
    SCH_SHAPE shape = lineEndingShape( aItem );

    if( shape.IsClosed() )
        return true;

    if( aItem->Type() == SCH_LINE_T && aItem->GetLayer() != LAYER_NOTES )
        return false;

    if( shape.GetFillMode() != FILL_T::NO_FILL )
        return false;

    auto solid = []( LINE_STYLE style )
    {
        return style == LINE_STYLE::SOLID || style == LINE_STYLE::DEFAULT;
    };

    if( !solid( shape.GetLineStyle() ) )
        return false;

    for( const LINE_ENDING* ending : { &shape.GetStartEnding(), &shape.GetEndEnding() } )
    {
        if( ending->GetStyle() == LINE_ENDING_STYLE::NONE )
            continue;

        if( !solid( ending->GetStroke().GetLineStyle() ) )
            return false;

        const auto& color = ending->GetStroke().GetColor();

        if( color != COLOR4D::UNSPECIFIED && color != shape.GetStroke().GetColor() )
            return false;
    }

    return shape.GetEffectiveWidth() > 0;
}


static void clearLineEndings( SCH_ITEM* aItem )
{
    if( SCH_SHAPE* shape = dynamic_cast<SCH_SHAPE*>( aItem ) )
    {
        shape->SetStartEnding( LINE_ENDING() );
        shape->SetEndEnding( LINE_ENDING() );
    }
    else if( SCH_LINE* line = dynamic_cast<SCH_LINE*>( aItem ) )
    {
        line->SetStartEnding( LINE_ENDING() );
        line->SetEndEnding( LINE_ENDING() );
    }
}


static int countSymbolLineEndings( const LIB_SYMBOL* aSymbol )
{
    int count = 0;

    for( const SCH_ITEM& item : aSymbol->GetDrawItems() )
        count += hasLineEndings( &item );

    return count;
}


static int countLineEndings( const SCH_SCREEN* aScreen )
{
    int count = 0;
    visitScreenItems( aScreen,
                      [&]( EDA_ITEM* item )
                      {
                          if( SCH_ITEM* schItem = dynamic_cast<SCH_ITEM*>( item ) )
                              count += hasLineEndings( schItem );
                      } );
    return count;
}


static std::vector<SCH_SHAPE*> lineEndingPolygons( SCH_ITEM* aItem )
{
    SCH_SHAPE     source = lineEndingShape( aItem );
    STROKE_PARAMS stroke = source.GetStroke();
    stroke.SetWidth( source.GetEffectiveWidth() );
    source.SetStroke( stroke );
    SHAPE_POLY_SET polygons;
    source.TransformWithLineEndingsToPolygon( polygons, 0, schIUScale.mmToIU( 0.01 ), ERROR_INSIDE );
    polygons.Simplify();
    polygons.Fracture();
    std::vector<SCH_SHAPE*> result;

    for( int i = 0; i < polygons.OutlineCount(); ++i )
    {
        SCH_SHAPE* shape = new SCH_SHAPE( source );
        shape->SetShape( SHAPE_T::POLY );
        shape->SetPolyShape( SHAPE_POLY_SET( polygons.Outline( i ) ) );
        shape->SetStroke( STROKE_PARAMS( -1, LINE_STYLE::SOLID ) );
        shape->SetFillMode( stroke.GetColor() == COLOR4D::UNSPECIFIED ? FILL_T::FILLED_SHAPE
                                                                      : FILL_T::FILLED_WITH_COLOR );
        shape->SetFillColor( stroke.GetColor() );
        shape->ClearCustomProperties();
        clearLineEndings( shape );
        const_cast<KIID&>( shape->m_Uuid ) = i == 0 ? aItem->m_Uuid : KIID();
        result.push_back( shape );
    }

    return result;
}


static void lowerSymbolLineEndings( LIB_SYMBOL* aSymbol )
{
    std::vector<SCH_ITEM*> items;

    for( SCH_ITEM& item : aSymbol->GetDrawItems() )
    {
        if( hasLineEndings( &item ) )
            items.push_back( &item );
    }

    for( SCH_ITEM* item : items )
    {
        if( lineEndingShape( item ).IsClosed() )
            clearLineEndings( item );
        else if( canLowerLineEndings( item ) )
        {
            std::vector<SCH_SHAPE*> shapes = lineEndingPolygons( item );

            if( shapes.empty() )
            {
                clearLineEndings( item );
                continue;
            }

            for( SCH_SHAPE* shape : shapes )
            {
                shape->SetParent( aSymbol );
                aSymbol->AddDrawItem( shape );
            }

            aSymbol->RemoveDrawItem( item );
        }
    }
}


static void lowerLineEndings( SCH_SCREEN* aScreen )
{
    std::vector<SCH_ITEM*> items;

    for( SCH_ITEM* item : aScreen->Items() )
    {
        if( hasLineEndings( item ) )
            items.push_back( item );
    }

    for( SCH_ITEM* item : items )
    {
        if( lineEndingShape( item ).IsClosed() )
            clearLineEndings( item );
        else if( canLowerLineEndings( item ) )
        {
            SCH_GROUP*              group = static_cast<SCH_GROUP*>( item->GetParentGroup() );
            std::vector<SCH_SHAPE*> shapes = lineEndingPolygons( item );

            if( shapes.empty() )
            {
                clearLineEndings( item );
                continue;
            }

            if( group )
                group->RemoveItem( item );

            aScreen->Remove( item );

            for( SCH_SHAPE* shape : shapes )
            {
                shape->SetParentGroup( nullptr );
                aScreen->Append( shape );

                if( group )
                    group->AddItem( shape );
            }

            delete item;
        }
    }

    for( const auto& [name, symbol] : aScreen->GetLibSymbols() )
    {
        if( symbol )
            lowerSymbolLineEndings( symbol );
    }
}


static void dropSymbolLineEndings( LIB_SYMBOL* aSymbol )
{
    for( SCH_ITEM& item : aSymbol->GetDrawItems() )
        clearLineEndings( &item );
}


static void dropLineEndings( SCH_SCREEN* aScreen )
{
    visitScreenItems( aScreen,
                      []( EDA_ITEM* item )
                      {
                          if( SCH_ITEM* schItem = dynamic_cast<SCH_ITEM*>( item ) )
                              clearLineEndings( schItem );
                      } );
}


// Same pattern as board_downgrade.cpp. One list drives both the report and the transform.
struct SCREEN_RULE
{
    int                                     m_introducedIn;
    DOWNGRADE_BUCKET                        m_bucket;
    wxString                                m_feature;
    wxString                                m_detail;
    std::function<int( const SCH_SCREEN* )> m_count;
    std::function<void( SCH_SCREEN* )>      m_apply;
    std::function<int( const LIB_SYMBOL* )> m_countSymbol = {}; ///< Symbol-scoped part, for .kicad_sym files
    std::function<void( LIB_SYMBOL* )>      m_applySymbol = {};
    std::function<void( SCH_SCREEN* )>      m_omit = {};
    std::function<void( LIB_SYMBOL* )>      m_omitSymbol = {};
    wxString                                m_omitDetail = {};
};


static std::vector<SCREEN_RULE> screenRules()
{
    return {
        { SCH_VER_CUSTOM_PROPERTIES, DOWNGRADE_BUCKET::DROP, _( "Custom user properties" ),
          _( "Custom user-property metadata is removed." ), countCustomProperties, dropCustomProperties,
          countSymbolCustomProperties, dropSymbolCustomProperties },
        { SCH_VER_ALTERNATE_SYMBOLS, DOWNGRADE_BUCKET::DROP, _( "Alternate variant symbols" ),
          _( "Alternate-symbol choices in retained variant definitions are removed. The base symbol is unchanged." ),
          countAlternateSymbols, dropAlternateSymbols },
        // Bake before the cache-wide lowering rules so the new entries receive those too.
        { SCH_VER_BODY_STYLES,
          DOWNGRADE_BUCKET::LOWER,
          _( "Selected custom body styles" ),
          _( "The selected drawing and pins are baked into a separate base-style symbol." ),
          countSelectedCustomBodyStyles,
          bakeSelectedCustomBodyStyles,
          {},
          {},
          omitSelectedCustomBodyStyles,
          {},
          _( "Symbols selecting a custom body style are omitted, including their pins. Connectivity may change." ) },
        { SCH_VER_BODY_STYLES, DOWNGRADE_BUCKET::BLOCK, _( "Unresolved custom body styles" ),
          _( "The selected drawing cannot be preserved without its library symbol." ), countUnresolvedCustomBodyStyles,
          nullptr },
        { SCH_VER_LOCKING,
          DOWNGRADE_BUCKET::LOWER,
          _( "Locked items" ),
          _( "The lock is not kept." ),
          countLockedItems,
          unlockItems,
          {},
          {},
          unlockItems,
          {},
          _( "Lock metadata is removed; the supported items are kept." ) },
        { SCH_VER_LINE_ENDINGS, DOWNGRADE_BUCKET::LOWER, _( "Line endings" ),
          _( "Solid line bodies and endings are approximated by filled polygons." ), countLineEndings, lowerLineEndings,
          countSymbolLineEndings, lowerSymbolLineEndings, dropLineEndings, dropSymbolLineEndings,
          _( "Line-ending embellishments are removed. The original supported body is kept." ) },
        { SCH_VER_ELLIPSE, DOWNGRADE_BUCKET::LOWER, _( "Ellipse graphics" ), _( "Approximated by a polygon." ),
          countEllipses, lowerEllipses, countSymbolEllipses, lowerSymbolEllipses, omitEllipses, omitSymbolEllipses,
          _( "Ellipse graphics are omitted instead of approximated." ) },
        { SCH_VER_SHAPE_HATCH, DOWNGRADE_BUCKET::LOWER, _( "Hatched shape fills" ), _( "Converted to a solid fill." ),
          countHatchedFills, lowerHatchedFills, countSymbolHatchedFills, lowerSymbolHatchedFills, omitHatchedFills,
          omitSymbolHatchedFills, _( "The hatch fill is omitted; the supported outline is kept." ) },
        { SCH_VER_PIN_MAPS, DOWNGRADE_BUCKET::DROP, _( "Pin-to-pad maps" ),
          _( "No equivalent in the target. The map is removed." ), countPinMaps, dropPinMaps, countSymbolPinMaps,
          dropSymbolPinMaps },
        { SCH_VER_GROUPS,
          DOWNGRADE_BUCKET::LOWER,
          _( "Groups" ),
          _( "Grouping is removed; the items are kept." ),
          countGroups,
          ungroupAll,
          {},
          {},
          ungroupAll,
          {},
          _( "Group metadata is removed; the supported items are kept." ) },
        { SCH_VER_JUMPER_PINS, DOWNGRADE_BUCKET::DROP, _( "Jumper pin groups" ),
          _( "Jumper pin groupings are removed." ), countJumperPinGroups, dropJumperPinGroups, countSymbolJumperPins,
          dropSymbolJumperPins },
        { SCH_VER_BODY_STYLES, DOWNGRADE_BUCKET::DROP, _( "Named body styles" ),
          _( "Custom names are removed. The target can select only the base and De Morgan styles." ),
          countNamedBodyStyles, dropNamedBodyStyles, countSymbolNamedBodyStyles, dropSymbolBodyStyleNames },
        { SCH_VER_POSITION_EXCLUSIONS, DOWNGRADE_BUCKET::DROP, _( "Position file exclusions" ),
          _( "The target cannot exclude schematic symbols from position files." ), countPositionExclusions,
          dropPositionExclusions, countSymbolPositionExclusions, dropSymbolPositionExclusions },
        { SCH_VER_ROUNDED_RECT, DOWNGRADE_BUCKET::LOWER, _( "Rounded rectangles" ), _( "Approximated by a polygon." ),
          countRoundedRects, lowerRoundedRects, countSymbolRoundedRects, lowerSymbolRoundedRects, omitRoundedRects,
          omitSymbolRoundedRects, _( "Rounded rectangles are omitted instead of approximated." ) },
        { SCH_VER_LOCAL_POWER, DOWNGRADE_BUCKET::BLOCK, _( "Local power symbols" ),
          _( "The target would treat it as global power and merge nets across sheets." ), countLocalPowerSymbols,
          nullptr, countSymbolLocalPower },
        { SCH_VER_RULE_AREA_FLAGS, DOWNGRADE_BUCKET::DROP, _( "Rule area flags" ),
          _( "DNP and exclusion flags on rule areas are removed." ), countRuleAreaFlags, dropRuleAreaFlags },
        { SCH_VER_STACKED_PIN_ESCAPES, DOWNGRADE_BUCKET::BLOCK, _( "Escaped stacked pin numbers" ),
          _( "The target misreads the escaping and would rename the pins." ), countEscapedStackedPins, nullptr,
          countSymbolEscapedStackedPins },
        { SCH_VER_STACKED_PINS, DOWNGRADE_BUCKET::BLOCK, _( "Stacked pin numbers" ),
          _( "The target reads the brackets as one literal pin number and the netlist changes." ), countStackedPins,
          nullptr, countSymbolStackedPins },
    };
}


COMPATIBILITY_REPORT ClassifyScreenForDowngrade( const SCH_SCREEN* aScreen, const DOWNGRADE_TARGET& aTarget,
                                                 bool aDropInsteadOfApproximate )
{
    // The rules were written against a specific format. Warn developers when the format has
    // moved on, since a frozen writer silently omits features that have no rule yet.
    wxASSERT_MSG( SEXPR_SCHEMATIC_FILE_VERSION == SCH_DOWNGRADE_COVERED,
                  wxT( "Schematic format changed. Review the downgrade rules and denylist, then "
                       "update SCH_DOWNGRADE_COVERED." ) );

    COMPATIBILITY_REPORT report;

    if( aTarget.m_schVersion < SCH_VER_CUSTOM_PROPERTIES )
    {
        reportTextPlan( screenTextPlan( aScreen, aTarget.m_schVersion < SCH_WRITER_V10::FORMAT_VERSION ), report );
        reportPaddedBuses( countPaddedBusLabels( aScreen ), report );
        reportIncompatibleBuses(
                countIncompatibleBusLabels( aScreen, aTarget.m_schVersion < SCH_WRITER_V10::FORMAT_VERSION ), report );
    }

    if( aTarget.m_schVersion < SCH_VER_LINE_ENDINGS && !aDropInsteadOfApproximate )
    {
        int blocked = 0;
        visitScreenItems( aScreen,
                          [&]( EDA_ITEM* item )
                          {
                              if( SCH_ITEM* schItem = dynamic_cast<SCH_ITEM*>( item ) )
                                  blocked += hasLineEndings( schItem ) && !canLowerLineEndings( schItem );
                          } );

        if( blocked )
            report.Add( DOWNGRADE_BUCKET::BLOCK, _( "Styled line endings" ),
                        _( "Dashed, filled, differently colored or non-graphical endings cannot be approximated "
                           "faithfully. Explicitly dropping approximations keeps the supported body." ),
                        blocked );
    }

    for( const SCREEN_RULE& rule : screenRules() )
    {
        if( aTarget.m_schVersion >= rule.m_introducedIn )
            continue;

        if( int count = rule.m_count( aScreen ); count > 0 )
        {
            bool omit = aDropInsteadOfApproximate && rule.m_bucket == DOWNGRADE_BUCKET::LOWER;
            report.Add( omit ? DOWNGRADE_BUCKET::DROP : rule.m_bucket, rule.m_feature,
                        omit ? rule.m_omitDetail : rule.m_detail, count );
        }
    }

    return report;
}


// The stored scale compensates for the truncated pixels/cm PPI that targets before 20260623
// compute, so it must be rewound. The catalog keeps images with a silent correction, so this
// is not a table rule and adds no report row.
static void compensateReferenceImageScales( SCH_SCREEN* aScreen )
{
    for( SCH_ITEM* item : aScreen->Items().OfType( SCH_BITMAP_T ) )
    {
        REFERENCE_IMAGE& ref = static_cast<SCH_BITMAP*>( item )->GetReferenceImage();
        int              legacyPPI = ref.GetImage().GetLegacyPPI();

        if( legacyPPI > 0 && ref.GetImage().GetPPI() != legacyPPI )
            ref.SetImageScale( ref.GetImageScale() * legacyPPI / ref.GetImage().GetPPI() );
    }
}


COMPATIBILITY_REPORT ClassifySchematicStructureForDowngrade( const SCHEMATIC&        aSchematic,
                                                             const DOWNGRADE_TARGET& aTarget )
{
    COMPATIBILITY_REPORT report;

    // A flat hierarchy (20251012) has several top-level sheets. The target opens a single root
    // sheet only, so refuse rather than write a project it cannot represent.
    if( aTarget.m_schVersion < SCH_VER_FLAT_HIERARCHY && aSchematic.GetTopLevelSheets().size() > 1 )
    {
        report.Add( DOWNGRADE_BUCKET::BLOCK, _( "Multiple top-level sheets" ),
                    _( "The target can only open a single root sheet." ), 1 );
    }

    // A variant that selects a different library symbol cannot be represented by a target that
    // keeps variants but not the choice. The variant would survive and quietly resolve to the base
    // symbol, changing that variant's pins. A target that removes variants loses the choice along
    // with them, which the screen rule reports as a drop.
    if( aTarget.m_schVersion >= SCH_VER_VARIANTS && aTarget.m_schVersion < SCH_VER_ALTERNATE_SYMBOLS )
    {
        SCH_SCREENS screens( aSchematic.Root() );
        int         count = 0;

        for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
            count += countAlternateSymbols( screen );

        if( count > 0 )
        {
            report.Add( DOWNGRADE_BUCKET::BLOCK, _( "Unrepresentable alternate variant symbols" ),
                        _( "The target keeps variants but cannot store a variant's alternate symbol, so the "
                           "variant would resolve to the base symbol." ),
                        count );
        }
    }

    if( aTarget.m_schVersion < SCH_VER_CUSTOM_PROPERTIES )
    {
        int count = 0;
        int incompatible = 0;

        for( const auto& alias : aSchematic.GetAllBusAliases() )
        {
            for( const wxString& member : alias->Members() )
            {
                count += hasPaddedBusVector( member );
                incompatible +=
                        hasIncompatibleBusVector( member, aTarget.m_schVersion < SCH_WRITER_V10::FORMAT_VERSION );
            }
        }

        reportPaddedBuses( count, report );
        reportIncompatibleBuses( incompatible, report );
    }

    return report;
}


void DowngradeScreenInPlace( SCH_SCREEN* aScreen, const DOWNGRADE_TARGET& aTarget, bool aDropInsteadOfApproximate )
{
    if( aTarget.m_schVersion < SCH_VER_CUSTOM_PROPERTIES )
        applyTextPlan( screenTextPlan( aScreen, aTarget.m_schVersion < SCH_WRITER_V10::FORMAT_VERSION ) );

    if( aTarget.m_schVersion < SCH_VER_IMAGE_PPI )
        compensateReferenceImageScales( aScreen );

    for( const SCREEN_RULE& rule : screenRules() )
    {
        if( aTarget.m_schVersion >= rule.m_introducedIn )
            continue;

        bool        omit = aDropInsteadOfApproximate && rule.m_bucket == DOWNGRADE_BUCKET::LOWER;
        const auto& apply = omit ? rule.m_omit : rule.m_apply;
        wxASSERT_MSG( !omit || apply, wxT( "A LOWER rule needs an omission transform." ) );

        if( apply )
            apply( aScreen );
    }
}


// Symbol-scoped subset for standalone .kicad_sym files the screen pass never sees.
COMPATIBILITY_REPORT ClassifyLibSymbolForDowngrade( const LIB_SYMBOL* aSymbol, const DOWNGRADE_TARGET& aTarget,
                                                    bool aDropInsteadOfApproximate )
{
    wxASSERT_MSG( SEXPR_SYMBOL_LIB_FILE_VERSION == SYMLIB_DOWNGRADE_COVERED,
                  wxT( "Symbol library format changed. Review the symbol downgrade path, then "
                       "update SYMLIB_DOWNGRADE_COVERED." ) );

    COMPATIBILITY_REPORT report;

    if( aTarget.m_symLibVersion < SCH_VER_CUSTOM_PROPERTIES )
        reportTextPlan( symbolTextPlan( aSymbol, aTarget.m_schVersion < SCH_WRITER_V10::FORMAT_VERSION ), report );

    if( aTarget.m_symLibVersion < SYMLIB_VER_LINE_ENDINGS && !aDropInsteadOfApproximate )
    {
        int blocked = 0;

        for( const SCH_ITEM& item : aSymbol->GetDrawItems() )
            blocked += hasLineEndings( &item ) && !canLowerLineEndings( &item );

        if( blocked )
            report.Add( DOWNGRADE_BUCKET::BLOCK, _( "Styled line endings" ),
                        _( "Dashed, filled or differently colored endings cannot be approximated faithfully. "
                           "Explicitly dropping approximations keeps the supported body." ),
                        blocked );
    }

    for( const SCREEN_RULE& rule : screenRules() )
    {
        if( aTarget.m_schVersion >= rule.m_introducedIn || !rule.m_countSymbol )
            continue;

        if( int count = rule.m_countSymbol( aSymbol ); count > 0 )
        {
            bool omit = aDropInsteadOfApproximate && rule.m_bucket == DOWNGRADE_BUCKET::LOWER;
            report.Add( omit ? DOWNGRADE_BUCKET::DROP : rule.m_bucket, rule.m_feature,
                        omit ? rule.m_omitDetail : rule.m_detail, count );
        }
    }

    return report;
}


void DowngradeLibSymbolInPlace( LIB_SYMBOL* aSymbol, const DOWNGRADE_TARGET& aTarget, bool aDropInsteadOfApproximate )
{
    if( aTarget.m_symLibVersion < SCH_VER_CUSTOM_PROPERTIES )
        applyTextPlan( symbolTextPlan( aSymbol, aTarget.m_schVersion < SCH_WRITER_V10::FORMAT_VERSION ) );

    for( const SCREEN_RULE& rule : screenRules() )
    {
        if( aTarget.m_schVersion >= rule.m_introducedIn )
            continue;

        bool        omit = aDropInsteadOfApproximate && rule.m_bucket == DOWNGRADE_BUCKET::LOWER;
        const auto& apply = omit ? rule.m_omitSymbol : rule.m_applySymbol;
        wxASSERT_MSG( !omit || !rule.m_applySymbol || apply,
                      wxT( "A symbol LOWER rule needs an omission transform." ) );

        if( apply )
            apply( aSymbol );
    }
}


// Node heads dated by the format that introduced them. The versioned writers cannot emit these,
// so this gate is a safety net against a wrong or miswired writer. To add a token, add a row
// with its format version.
static constexpr DATED_TOKEN SCH_TOKENS[] = {
    { "symbol_override", SCH_VER_ALTERNATE_SYMBOLS },
    { "start_shape", SYMLIB_VER_LINE_ENDINGS },
    { "end_shape", SYMLIB_VER_LINE_ENDINGS },
    { "custom_property", SCH_VER_CUSTOM_PROPERTIES },
    { "jumper_pin_groups", SCH_VER_JUMPER_PINS },
    { "group", SCH_VER_GROUPS },
    { "body_style", SCH_VER_BODY_STYLES },
    { "body_styles", SCH_VER_BODY_STYLES },
    { "variant", SCH_VER_VARIANTS },
    { "field", SCH_VER_VARIANTS },
    // Dated by the group lock, the oldest legal use. The unlock rule strips all others.
    { "locked", SCH_VER_GROUPS },
    { "ellipse", SCH_VER_ELLIPSE },
    { "ellipse_arc", SCH_VER_ELLIPSE },
    { "rotation_angle", SCH_VER_ELLIPSE },
    { "net_chain", SCH_VER_NET_CHAINS },
    { "net_class", SCH_VER_NET_CHAINS },
    { "nets", SCH_VER_NET_CHAINS },
    { "passthrough", SCH_VER_NET_CHAINS },
    { "pin_map", SCH_VER_PIN_MAPS },
    { "pin_maps", SCH_VER_PIN_MAPS },
    { "pin_map_override", SCH_VER_PIN_MAPS },
    { "named_map", SCH_VER_PIN_MAPS },
    { "associated_footprints", SCH_VER_PIN_MAPS },
    { "entry", SCH_VER_PIN_MAPS },
    { "map", SCH_VER_PIN_MAPS },
    { "edit", SCH_VER_PIN_MAPS },
};

// Bareword values the target cannot parse. The node scan cannot see these.
static constexpr DATED_TOKEN SCH_VALUES[] = {
    { "hatch", SCH_VER_SHAPE_HATCH },
    { "reverse_hatch", SCH_VER_SHAPE_HATCH },
    { "cross_hatch", SCH_VER_SHAPE_HATCH },
};


int SymbolLibDowngradeCoveredVersion()
{
    return SYMLIB_DOWNGRADE_COVERED;
}


int SchDowngradeCoveredVersion()
{
    return SCH_DOWNGRADE_COVERED;
}


wxString FindUnsupportedSchToken( const wxString& aSerialized, const DOWNGRADE_TARGET& aTarget )
{
    if( aTarget.m_schVersion >= SEXPR_SCHEMATIC_FILE_VERSION )
        return wxEmptyString;

    return FindUnsupportedToken( aSerialized, SCH_TOKENS, std::size( SCH_TOKENS ), SCH_VALUES, std::size( SCH_VALUES ),
                                 aTarget.m_schVersion );
}
