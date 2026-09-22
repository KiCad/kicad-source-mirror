/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
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

#include <algorithm>
#include <map>
#include <numeric>
#include <set>

#include "connection_graph.h"
#include "kiface_ids.h"
#include <advanced_config.h>
#include <connectivity/conn_facade.h>
#include <common.h>     // for ExpandEnvVarSubstitutions
#include <erc/erc.h>
#include <erc/erc_sch_pin_context.h>
#include <gal/graphics_abstraction_layer.h>
#include <string_utils.h>
#include <sch_pin.h>
#include <connectivity/conn_netchain_manager.h>
#include <sch_netchain.h>
#include <project_sch.h>
#include <project/project_file.h>
#include <project/net_settings.h>
#include <sch_bus_entry.h>
#include <sch_edit_frame.h>
#include <sch_marker.h>
#include <sch_reference_list.h>
#include <sch_rule_area.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <sch_pin.h>
#include <sch_textbox.h>
#include <sch_line.h>
#include <schematic.h>
#include <lib_symbol.h>
#include <sch_symbol.h>
#include <pin_map.h>
#include <drawing_sheet/ds_draw_item.h>
#include <drawing_sheet/ds_proxy_view_item.h>
#include <vector>
#include <optional>
#include <wx/ffile.h>
#include <sim/sim_lib_mgr.h>
#include <progress_reporter.h>
#include <kiway.h>
#include <pgm_base.h>
#include <libraries/symbol_library_adapter.h>
#include <trace_helpers.h>
#include <variant_symbol_utils.h>

namespace
{
// Item paths participate in saved exclusion keys; preserve each check's serialized shape
enum class ITEM_PATHS
{
    NONE,
    MAIN,
    BOTH
};

// Capture membership before the first marker; checks may insert markers while traversing the result
class ERC_MARKER_PLACER
{
public:
    explicit ERC_MARKER_PLACER( const SCHEMATIC& aSchematic ) :
            m_schematic( aSchematic )
    {
    }

    const SCH_SHEET_PATH* Find( const KIID_PATH& aSheet )
    {
        if( !m_paths )
        {
            m_hierarchy = m_schematic.Hierarchy();
            m_paths.emplace();

            for( const SCH_SHEET_PATH& path : m_hierarchy )
                m_paths->emplace( path.PathRef(), &path );
        }

        if( const auto found = m_paths->find( aSheet ); found != m_paths->end() )
            return found->second;

        wxLogTrace( "KICAD_CONNECTIVITY", "ERC connectivity references unknown sheet instance %s", aSheet.AsString() );
        return nullptr;
    }

    void Place( std::shared_ptr<ERC_ITEM> aItem, const KIID_PATH& aSheet, const VECTOR2I& aPosition,
                ITEM_PATHS aItemPaths = ITEM_PATHS::NONE, bool aSheetSpecific = true )
    {
        const SCH_SHEET_PATH* sheet = Find( aSheet );

        if( !sheet )
            return;

        if( aItemPaths == ITEM_PATHS::BOTH && aItem->GetAuxItemID() != niluuid )
            aItem->SetItemsSheetPaths( *sheet, *sheet );
        else if( aItemPaths != ITEM_PATHS::NONE )
            aItem->SetItemsSheetPaths( *sheet );

        append( std::move( aItem ), *sheet, aPosition, aSheetSpecific );
    }

    void Place( std::shared_ptr<ERC_ITEM> aItem, const KIID_PATH& aSheet, const VECTOR2I& aPosition,
                const KIID_PATH& aMainItemSheet, const KIID_PATH& aAuxItemSheet )
    {
        const SCH_SHEET_PATH* sheet = Find( aSheet );
        const SCH_SHEET_PATH* main = Find( aMainItemSheet );
        const SCH_SHEET_PATH* aux = Find( aAuxItemSheet );

        if( !sheet || !main || !aux )
            return;

        aItem->SetItemsSheetPaths( *main, *aux );
        append( std::move( aItem ), *sheet, aPosition );
    }

    void Report( int aCode, const RC_ITEM::KIIDS& aItems, const KIID_PATH& aSheet, const VECTOR2I& aPosition,
                 ITEM_PATHS aItemPaths = ITEM_PATHS::NONE, const wxString& aMessage = wxEmptyString )
    {
        auto item = ERC_ITEM::Create( aCode );
        item->SetItems( aItems );

        if( !aMessage.empty() )
            item->SetErrorMessage( aMessage );

        Place( std::move( item ), aSheet, aPosition, aItemPaths );
    }

    int Count() const { return m_count; }

private:
    void append( std::shared_ptr<ERC_ITEM> aItem, const SCH_SHEET_PATH& aSheet, const VECTOR2I& aPosition,
                 bool aSheetSpecific = true )
    {
        if( aSheetSpecific )
            aItem->SetSheetSpecificPath( aSheet );

        aSheet.LastScreen()->Append( new SCH_MARKER( std::move( aItem ), aPosition ) );
        ++m_count;
    }

    const SCHEMATIC&                                          m_schematic;
    SCH_SHEET_LIST                                            m_hierarchy;
    std::optional<std::map<KIID_PATH, const SCH_SHEET_PATH*>> m_paths;
    int                                                       m_count = 0;
};

std::vector<SCH_CONNECTIVITY::FOOTPRINT_SOURCE> collectFootprints( SCHEMATIC& aSchematic )
{
    if( ADVANCED_CFG::GetCfg().m_ConnectivityEngine )
        return aSchematic.Connectivity().Engine().FootprintSources();

    std::vector<SCH_CONNECTIVITY::FOOTPRINT_SOURCE> result;

    for( const SCH_SHEET_PATH& path : aSchematic.Hierarchy() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            const auto& symbol = *static_cast<SCH_SYMBOL*>( item );
            const auto& library = symbol.GetLibSymbolRef();
            result.push_back( { path.PathRef(), symbol.m_Uuid, symbol.GetPosition(),
                    symbol.GetFootprintFieldText( &path, RESOLVED ),
                    library ? library->GetFPFilters() : wxArrayString() } );
        }
    }

    return result;
}

template <typename LOCATION>
int reportSourceErrors( SCHEMATIC& aSchematic, int aCode, const std::vector<LOCATION>& aSources )
{
    ERC_MARKER_PLACER markers( aSchematic );

    for( const auto& source : aSources )
        markers.Report( aCode, { source.item }, source.sheet, source.position, ITEM_PATHS::MAIN );

    return markers.Count();
}

std::vector<SCH_CONNECTIVITY::NAMED_ITEM> collectNamedItems( SCHEMATIC& aSchematic, bool aSimilar )
{
    // The engine compares resolved net names for every power pin, including hidden and alternate pins
    if( ADVANCED_CFG::GetCfg().m_ConnectivityEngine )
        return aSchematic.Connectivity().Engine().NamedItems();

    std::vector<SCH_CONNECTIVITY::NAMED_ITEM> result;

    for( const auto& [key, subgraphs] : aSchematic.ConnectionGraph()->GetNetMap() )
    {
        for( const CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            const SCH_SHEET_PATH& sheet = subgraph->GetSheet();

            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() == SCH_LABEL_T || item->Type() == SCH_GLOBAL_LABEL_T
                    || item->Type() == SCH_HIER_LABEL_T )
                {
                    auto* label = static_cast<SCH_LABEL_BASE*>( item );
                    result.push_back( { sheet.PathRef(), item->m_Uuid, item->GetPosition(),
                                        label->GetShownText( &sheet, FOR_NETNAME ), item->Type(),
                                        item->Type() == SCH_GLOBAL_LABEL_T } );
                }
                else if( item->Type() == SCH_PIN_T )
                {
                    auto* pin = static_cast<SCH_PIN*>( item );
                    SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                    if( !pin->IsPower() || !symbol )
                        continue;

                    const wxString name = !aSimilar && pin->IsGlobalPower() && !symbol->IsGlobalPower()
                                                  ? pin->GetShownName() : symbol->GetValue( &sheet, FOR_NETNAME );
                    result.push_back( { sheet.PathRef(), item->m_Uuid, item->GetPosition(), name,
                                        SCH_PIN_T, pin->IsGlobalPower() } );
                }
            }
        }
    }

    return result;
}

void reportNameConflict( ERC_MARKER_PLACER& aMarkers, ERC_SETTINGS& aSettings, ERCE_T aCode,
                         const SCH_CONNECTIVITY::NAMED_ITEM& aMain, const SCH_CONNECTIVITY::NAMED_ITEM& aAux )
{
    if( !aSettings.IsTestEnabled( aCode ) )
        return;

    auto item = ERC_ITEM::Create( aCode );
    item->SetItems( aMain.item, aAux.item );
    aMarkers.Place( std::move( item ), aMain.sheet, aMain.position, aMain.sheet, aAux.sheet );
}

}


/* ERC tests :
 *  1 - conflicts between connected pins ( example: 2 connected outputs )
 *  2 - minimal connections requirements ( 1 input *must* be connected to an
 * output, or a passive pin )
 */

/*
 *  Minimal ERC requirements:
 *  All pins *must* be connected (except ELECTRICAL_PINTYPE::PT_NC).
 *  When a pin is not connected in schematic, the user must place a "non
 * connected" symbol to this pin.
 *  This ensures a forgotten connection will be detected.
 */

// Messages for matrix rows:
const wxString CommentERC_H[] =
{
    _( "Input Pin" ),
    _( "Output Pin" ),
    _( "Bidirectional Pin" ),
    _( "Tri-State Pin" ),
    _( "Passive Pin" ),
    _( "Free Pin" ),
    _( "Unspecified Pin" ),
    _( "Power Input Pin" ),
    _( "Power Output Pin" ),
    _( "Open Collector" ),
    _( "Open Emitter" ),
    _( "No Connection" )
};

// Messages for matrix columns
const wxString CommentERC_V[] =
{
    _( "Input Pin" ),
    _( "Output Pin" ),
    _( "Bidirectional Pin" ),
    _( "Tri-State Pin" ),
    _( "Passive Pin" ),
    _( "Free Pin" ),
    _( "Unspecified Pin" ),
    _( "Power Input Pin" ),
    _( "Power Output Pin" ),
    _( "Open Collector" ),
    _( "Open Emitter" ),
    _( "No Connection" )
};


// List of pin types that are considered drivers for usual input pins
// i.e. pin type = ELECTRICAL_PINTYPE::PT_INPUT, but not PT_POWER_IN
// that need only a PT_POWER_OUT pin type to be driven
const std::set<ELECTRICAL_PINTYPE> DrivingPinTypes =
{
    ELECTRICAL_PINTYPE::PT_OUTPUT,
    ELECTRICAL_PINTYPE::PT_POWER_OUT,
    ELECTRICAL_PINTYPE::PT_PASSIVE,
    ELECTRICAL_PINTYPE::PT_TRISTATE,
    ELECTRICAL_PINTYPE::PT_BIDI
};

// List of pin types that are considered drivers for power pins
// In fact only a ELECTRICAL_PINTYPE::PT_POWER_OUT pin type can drive
// power input pins
const std::set<ELECTRICAL_PINTYPE> DrivingPowerPinTypes =
{
    ELECTRICAL_PINTYPE::PT_POWER_OUT
};

// List of pin types that require a driver elsewhere on the net
const std::set<ELECTRICAL_PINTYPE> DrivenPinTypes =
{
    ELECTRICAL_PINTYPE::PT_INPUT,
    ELECTRICAL_PINTYPE::PT_POWER_IN
};

extern void CheckDuplicatePins( LIB_SYMBOL* aSymbol, std::vector<wxString>& aMessages,
                                UNITS_PROVIDER* aUnitsProvider );

ERC_TESTER::ERC_TESTER( SCHEMATIC* aSchematic, bool aShowAllErrors ) :
        m_schematic( aSchematic ),
        m_settings( aSchematic->ErcSettings() ),
        m_sheetList( aSchematic->BuildSheetListSortedByPageNumbers() ),
        m_screens( aSchematic->Root() ),
        m_nets( aSchematic->ConnectionGraph()->GetNetMap() ),
        m_showAllErrors( aShowAllErrors )
{
    if( !ADVANCED_CFG::GetCfg().m_ConnectivityEngine )
        m_sheetList.GetMultiUnitSymbols( m_refMap, SYMBOL_FILTER_ALL );
}


int ERC_TESTER::TestDuplicateSheetNames( bool aCreateMarker )
{
    int err_count = 0;

    // Preflight callers validate fresh edits before connectivity has been rebuilt
    if( aCreateMarker && ADVANCED_CFG::GetCfg().m_ConnectivityEngine )
    {
        const auto errors = m_schematic->Connectivity().Engine().DuplicateSheetNames();
        ERC_MARKER_PLACER markers( *m_schematic );

        for( const auto& error : errors )
        {
            markers.Report( ERCE_DUPLICATE_SHEET_NAME, { error.main, error.auxiliary }, error.sheet, error.position,
                            ITEM_PATHS::BOTH );
        }

        return markers.Count();
    }

    for( const SCH_SHEET_PATH& path : m_schematic->Hierarchy() )
    {
        SCH_SCREEN* screen = path.LastScreen();
        std::vector<SCH_SHEET*> list;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SHEET_T ) )
            list.push_back( static_cast<SCH_SHEET*>( item ) );

        for( size_t i = 0; i < list.size(); i++ )
        {
            SCH_SHEET* sheet = list[i];

            for( size_t j = i + 1; j < list.size(); j++ )
            {
                SCH_SHEET* test_item = list[j];

                // We have found a second sheet: compare names
                // we are using case insensitive comparison to avoid mistakes between
                // similar names like Mysheet and mysheet
                const wxString variant = m_schematic->GetCurrentVariant();
                const wxString name =
                        sheet->GetField( FIELD_T::SHEET_NAME )->GetShownText( &path, RESOLVED, variant, 0 );
                const wxString other =
                        test_item->GetField( FIELD_T::SHEET_NAME )->GetShownText( &path, RESOLVED, variant, 0 );

                if( name.IsSameAs( other, false ) )
                {
                    if( aCreateMarker )
                    {
                        auto ercItem = ERC_ITEM::Create( ERCE_DUPLICATE_SHEET_NAME );
                        ercItem->SetItems( sheet, test_item );
                        ercItem->SetSheetSpecificPath( path );
                        ercItem->SetItemsSheetPaths( path, path );

                        SCH_MARKER* marker = new SCH_MARKER( std::move( ercItem ), sheet->GetPosition() );
                        screen->Append( marker );
                    }

                    err_count++;
                }
            }
        }
    }

    return err_count;
}


int ERC_TESTER::TestPinMap( KIFACE* aCvPcb, PROJECT* aProject )
{
    int        errors = 0;
    const bool checkStale = m_settings.IsTestEnabled( ERCE_PIN_MAP_STALE_PIN );
    const bool checkDuplicate = m_settings.IsTestEnabled( ERCE_PIN_MAP_DUPLICATE_PAD );
    const bool checkBadPad = m_settings.IsTestEnabled( ERCE_PIN_MAP_BAD_PAD );

    typedef void ( *PAD_NUMBERS_FN_PTR )( const wxString&, PROJECT*, std::set<wxString>& );

    PAD_NUMBERS_FN_PTR padFetcher =
            aCvPcb ? (PAD_NUMBERS_FN_PTR) aCvPcb->IfaceOrAddress( KIFACE_FOOTPRINT_PAD_NUMBERS ) : nullptr;

    std::map<wxString, std::set<wxString>> padCache;

    auto getPads = [&]( const wxString& aFootprintId ) -> const std::set<wxString>&
    {
        auto it = padCache.find( aFootprintId );

        if( it != padCache.end() )
            return it->second;

        std::set<wxString>& pads = padCache[aFootprintId];

        if( padFetcher && !aFootprintId.IsEmpty() )
            padFetcher( aFootprintId, aProject, pads );

        return pads;
    };

    // Pin maps and the symbol's pin numbers are library-symbol properties, so iterate unique
    // screens (not sheet paths) to avoid double-reporting on reused hierarchical sheets.
    for( SCH_SCREEN* screen = m_screens.GetFirst(); screen; screen = m_screens.GetNext() )
    {
        std::vector<SCH_CONNECTIVITY::PIN_MAP_FACT> sources;

        if( ADVANCED_CFG::GetCfg().m_ConnectivityEngine )
        {
            sources = m_schematic->Connectivity().Engine().PinMapSymbols( screen->ConnectivityId() );
        }
        else
        {
            for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
            {
                if( auto source = SCH_CONNECTIVITY::ExtractPinMapFacts( *static_cast<SCH_SYMBOL*>( item ) ) )
                    sources.push_back( std::move( *source ) );
            }
        }

        for( const auto& source : sources )
        {
            const PIN_MAP_SET& maps = source.maps;
            const auto& pinNumbers = source.pinNumbers;
            const auto& jumperGroups = source.jumperGroups;

            auto sharesJumperGroup = [&]( const wxString& aPinA, const wxString& aPinB )
            {
                for( const JUMPER_GROUP& group : jumperGroups.GetAll() )
                {
                    if( group.Contains( aPinA ) && group.Contains( aPinB ) )
                        return true;
                }

                return false;
            };

            for( const PIN_MAP& map : maps.GetAll() )
            {
                if( checkStale )
                {
                    for( const PIN_MAP_ENTRY& entry : map.GetEntries() )
                    {
                        if( pinNumbers.count( entry.m_PinNumber ) )
                            continue;

                        std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_PIN_MAP_STALE_PIN );
                        ercItem->SetItems( source.id );
                        ercItem->SetErrorMessage(
                                wxString::Format( _( "Pin map '%s' references unknown symbol pin '%s'" ), map.GetName(),
                                                  entry.m_PinNumber ) );
                        screen->Append( new SCH_MARKER( std::move( ercItem ), source.position ) );
                        errors++;
                    }
                }

                // A single pin stacked across several pads is allowed. Two pins on one pad is not.
                if( checkDuplicate )
                {
                    std::map<wxString, wxString> padToPin;

                    for( const PIN_MAP_ENTRY& entry : map.GetEntries() )
                    {
                        for( const wxString& pad : ExpandStackedPinNotation( entry.m_PadNumber ) )
                        {
                            auto it = padToPin.find( pad );

                            if( it == padToPin.end() )
                            {
                                padToPin[pad] = entry.m_PinNumber;
                            }
                            else if( it->second != entry.m_PinNumber
                                     && !sharesJumperGroup( it->second, entry.m_PinNumber ) )
                            {
                                std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_PIN_MAP_DUPLICATE_PAD );
                                ercItem->SetItems( source.id );
                                ercItem->SetErrorMessage(
                                        wxString::Format( _( "Symbol pins '%s' and '%s' both map to pad '%s'" ),
                                                          it->second, entry.m_PinNumber, pad ) );
                                screen->Append( new SCH_MARKER( std::move( ercItem ), source.position ) );
                                errors++;
                            }
                        }
                    }
                }
            }

            if( checkBadPad && padFetcher )
            {
                for( const ASSOCIATED_FOOTPRINT& assoc : source.footprints )
                {
                    const PIN_MAP* boundMap = maps.FindByName( assoc.m_MapName );

                    if( !boundMap )
                        continue;

                    const std::set<wxString>& pads = getPads( assoc.m_FootprintLibId.GetUniStringLibId() );

                    if( pads.empty() )
                        continue;

                    for( const PIN_MAP_ENTRY& entry : boundMap->GetEntries() )
                    {
                        for( const wxString& pad : ExpandStackedPinNotation( entry.m_PadNumber ) )
                        {
                            if( pads.count( pad ) )
                                continue;

                            std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_PIN_MAP_BAD_PAD );
                            ercItem->SetItems( source.id );
                            ercItem->SetErrorMessage( wxString::Format(
                                    _( "Pin map '%s' references pad '%s' not present on footprint '%s'" ),
                                    boundMap->GetName(), pad, assoc.m_FootprintLibId.GetUniStringLibId() ) );
                            screen->Append( new SCH_MARKER( std::move( ercItem ), source.position ) );
                            errors++;
                        }
                    }
                }
            }
        }
    }

    if( m_settings.IsTestEnabled( ERCE_PIN_MAP_UNMAPPED_PIN ) && padFetcher )
    {
        auto report = [&]( const SCH_SHEET_PATH& sheet, const KIID& pin, const VECTOR2I& position,
                           const wxString& number, const wxString& footprint )
        {
            auto error = ERC_ITEM::Create( ERCE_PIN_MAP_UNMAPPED_PIN );
            error->SetItems( pin );
            error->SetSheetSpecificPath( sheet );
            error->SetItemsSheetPaths( sheet );
            error->SetErrorMessage( wxString::Format(
                    _( "Pin '%s' is connected but maps to no pad on footprint '%s'" ), number, footprint ) );
            sheet.LastScreen()->Append( new SCH_MARKER( std::move( error ), position ) );
            ++errors;
        };

        if( ADVANCED_CFG::GetCfg().m_ConnectivityEngine )
        {
            ERC_MARKER_PLACER sheets( *m_schematic );

            for( const auto& pin : m_schematic->Connectivity().Engine().UnmappedPinCandidates() )
            {
                const SCH_SHEET_PATH* sheet = sheets.Find( pin.sheet );

                if( !sheet )
                    continue;

                LIB_ID footprint;
                footprint.Parse( pin.footprint, true );
                const auto& pads = getPads( footprint.GetUniStringLibId() );

                if( !pads.empty() && !SCH_PIN::HasIdentityPad( pin.number, pads ) )
                    report( *sheet, pin.item, pin.position, pin.number, pin.footprint );
            }

            return errors;
        }

        const wxString variant = m_schematic ? m_schematic->GetCurrentVariant() : wxString();

        for( SCH_SHEET_PATH& sheet : m_sheetList )
        {
            for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
                LIB_SYMBOL* lib = symbol->GetLibSymbolRef().get();

                if( !lib || lib->GetEffectiveAssociatedFootprints().empty() )
                    continue;

                wxString fpText = symbol->GetFootprintFieldText( &sheet, RESOLVED );
                LIB_ID   fpId;

                if( fpText.IsEmpty() || fpId.Parse( fpText, true ) >= 0 )
                    continue;

                const std::set<wxString>& pads = getPads( fpId.GetUniStringLibId() );

                if( pads.empty() )
                    continue;

                for( SCH_PIN* pin : symbol->GetPins( &sheet ) )
                {
                    if( pin->IsDangling() )
                        continue;

                    SCH_PIN::PAD_RESOLUTION state = SCH_PIN::PAD_RESOLUTION::MAPPED;
                    pin->GetEffectivePadNumber( sheet, variant, fpId, &pads, &state );

                    if( state != SCH_PIN::PAD_RESOLUTION::UNMAPPED )
                        continue;

                    report( sheet, pin->m_Uuid, pin->GetPosition(), pin->GetNumber(), fpText );
                }
            }
        }
    }

    return errors;
}


void ERC_TESTER::TestTextVars( DS_PROXY_VIEW_ITEM* aDrawingSheet )
{
    DS_DRAW_ITEM_LIST wsItems( schIUScale, FOR_ERC_DRC );

    // The leading "(^|[^\\\\])" group requires the marker to start the string or follow a non-backslash,
    // so `\${ERC_ERROR ...}` stays inert.  (The group is just to make it easier for a human to parse.)
    static wxRegEx varRefRegEx( wxT( "(^|[^\\\\])\\$\\{.*\\}.*" ) );

    auto reportAssertions =
            []( const KIID& item, const SCH_SHEET_PATH& sheet, SCH_SCREEN* screen,
                const std::vector<SCH_CONNECTIVITY::TEXT_ASSERTION>& assertions, const VECTOR2I& pos )
            {
                for( const auto& assertion : assertions )
                {
                    auto ercItem = ERC_ITEM::Create( assertion.warning ? ERCE_GENERIC_WARNING : ERCE_GENERIC_ERROR );
                    wxString message = assertion.message;

                    if( item != niluuid )
                    {
                        ercItem->SetItems( std::vector<KIID>{ item } );
                        ercItem->SetItemsSheetPaths( sheet );
                    }
                    else
                    {
                        message += _( " (in drawing sheet)" );
                    }

                    ercItem->SetSheetSpecificPath( sheet );
                    ercItem->SetErrorMessage( message );
                    screen->Append( new SCH_MARKER( std::move( ercItem ), pos ) );
                }

                return !assertions.empty();
            };

    if( aDrawingSheet && !ADVANCED_CFG::GetCfg().m_ConnectivityEngine )
    {
        wsItems.SetPageNumber( wxS( "1" ) );
        wsItems.SetSheetCount( 1 );
        wsItems.SetFileName( wxS( "dummyFilename" ) );
        wsItems.SetSheetName( wxS( "dummySheet" ) );
        wsItems.SetSheetLayer( wxS( "dummyLayer" ) );
        wsItems.SetProject( &m_schematic->Project() );
        wsItems.BuildDrawItemsList( aDrawingSheet->GetPageInfo(), aDrawingSheet->GetTitleBlock() );
    }

    for( const SCH_SHEET_PATH& sheet : m_sheetList )
    {
        SCH_SCREEN* screen = sheet.LastScreen();

        const auto sources = ADVANCED_CFG::GetCfg().m_ConnectivityEngine
                                                    ? m_schematic->Connectivity().TextChecks( sheet.PathRef() )
                                                    : SCH_CONNECTIVITY::ExtractTextChecks( *screen, sheet );

        for( const auto& source : sources )
        {
            if( reportAssertions( source.assertionItem, sheet, screen, source.assertions, source.assertionPosition ) )
                continue;

            if( !varRefRegEx.Matches( source.shownText ) )
                continue;

            std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_UNRESOLVED_VARIABLE );
            ercItem->SetItems( std::vector<KIID>{ source.item } );
            ercItem->SetSheetSpecificPath( sheet );
            ercItem->SetItemsSheetPaths( sheet );
            screen->Append( new SCH_MARKER( std::move( ercItem ), source.position ) );
        }

        if( aDrawingSheet && ADVANCED_CFG::GetCfg().m_ConnectivityEngine )
        {
            for( const auto& source : m_schematic->Connectivity().TextChecks( sheet.PathRef(), true ) )
            {
                if( reportAssertions( niluuid, sheet, screen, source.assertions, source.assertionPosition ) )
                    continue;

                if( varRefRegEx.Matches( source.shownText ) )
                {
                    std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_UNRESOLVED_VARIABLE );
                    ercItem->SetErrorMessage( _( "Unresolved text variable in drawing sheet" ) );
                    ercItem->SetSheetSpecificPath( sheet );
                    screen->Append( new SCH_MARKER( std::move( ercItem ), source.position ) );
                }
            }
        }

        for( DS_DRAW_ITEM_BASE* item = wsItems.GetFirst(); item; item = wsItems.GetNext() )
        {
            if( DS_DRAW_ITEM_TEXT* text = dynamic_cast<DS_DRAW_ITEM_TEXT*>( item ) )
            {
                if( reportAssertions( niluuid, sheet, screen,
                                      SCH_CONNECTIVITY::ExtractTextAssertions( text->GetText() ),
                                      text->GetPosition() ) )
                {
                    // Don't run unresolved test
                }
                else if( varRefRegEx.Matches( text->GetShownText( FOR_ERC_DRC ) ) )
                {
                    std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_UNRESOLVED_VARIABLE );
                    ercItem->SetErrorMessage( _( "Unresolved text variable in drawing sheet" ) );
                    ercItem->SetSheetSpecificPath( sheet );

                    SCH_MARKER* marker = new SCH_MARKER( std::move( ercItem ), text->GetPosition() );
                    screen->Append( marker );
                }
            }
        }
    }
}


int ERC_TESTER::TestEmptyLabelNames()
{
    if( ADVANCED_CFG::GetCfg().m_ConnectivityEngine )
    {
        return reportSourceErrors( *m_schematic, ERCE_EMPTY_LABEL_NAME,
                                   m_schematic->Connectivity().Engine().EmptyLabels() );
    }

    int errors = 0;

    // No directive labels, they carry no text (the netclass lives in a field)
    static const KICAD_T labelTypes[] = { SCH_LABEL_T, SCH_GLOBAL_LABEL_T, SCH_HIER_LABEL_T };

    for( const SCH_SHEET_PATH& sheet : m_sheetList )
    {
        SCH_SCREEN* screen = sheet.LastScreen();

        for( KICAD_T labelType : labelTypes )
        {
            for( SCH_ITEM* item : screen->Items().OfType( labelType ) )
            {
                SCH_LABEL_BASE* label = static_cast<SCH_LABEL_BASE*>( item );

                wxString text = label->GetText();
                text.Trim( false ).Trim( true );

                if( text.IsEmpty() )
                {
                    auto ercItem = ERC_ITEM::Create( ERCE_EMPTY_LABEL_NAME );
                    ercItem->SetItems( label );
                    ercItem->SetItemsSheetPaths( sheet );
                    ercItem->SetSheetSpecificPath( sheet );

                    SCH_MARKER* marker = new SCH_MARKER( std::move( ercItem ), label->GetPosition() );
                    screen->Append( marker );
                    errors++;
                }
            }
        }
    }

    return errors;
}


int ERC_TESTER::TestFieldNameWhitespace()
{
    int warnings = 0;

    if( ADVANCED_CFG::GetCfg().m_ConnectivityEngine )
    {
        const auto errors = m_schematic->Connectivity().Engine().InvalidFieldNames();
        ERC_MARKER_PLACER markers( *m_schematic );

        for( const auto& error : errors )
        {
            markers.Report( ERCE_FIELD_NAME_WHITESPACE, { error.field.owner, error.field.field }, error.sheet,
                            error.field.position, ITEM_PATHS::BOTH,
                            wxString::Format( _( "Field name has leading or trailing whitespace: '%s'" ),
                                              error.field.name ) );
        }

        return markers.Count();
    }

    for( const SCH_SHEET_PATH& sheet : m_sheetList )
    {
        SCH_SCREEN* screen = sheet.LastScreen();

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            for( SCH_FIELD& field : symbol->GetFields() )
            {
                wxString trimmedFieldName = field.GetName();
                trimmedFieldName.Trim();
                trimmedFieldName.Trim( false );

                if( field.GetName() != trimmedFieldName )
                {
                    auto ercItem = ERC_ITEM::Create( ERCE_FIELD_NAME_WHITESPACE );
                    ercItem->SetItems( symbol, &field );
                    ercItem->SetItemsSheetPaths( sheet, sheet );
                    ercItem->SetSheetSpecificPath( sheet );
                    ercItem->SetErrorMessage(
                            wxString::Format(
                                    _( "Field name has leading or trailing whitespace: '%s'" ),
                                    field.GetName() ) );

                    SCH_MARKER* marker = new SCH_MARKER( std::move( ercItem ), field.GetPosition() );
                    screen->Append( marker );
                    warnings++;
                }
            }
        }

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SHEET_T ) )
        {
            SCH_SHEET* subSheet = static_cast<SCH_SHEET*>( item );

            for( SCH_FIELD& field : subSheet->GetFields() )
            {
                wxString trimmedFieldName = field.GetName();
                trimmedFieldName.Trim();
                trimmedFieldName.Trim( false );

                if( field.GetName() != trimmedFieldName )
                {
                    auto ercItem = ERC_ITEM::Create( ERCE_FIELD_NAME_WHITESPACE );
                    ercItem->SetItems( subSheet, &field );
                    ercItem->SetItemsSheetPaths( sheet, sheet );
                    ercItem->SetSheetSpecificPath( sheet );
                    ercItem->SetErrorMessage(
                            wxString::Format(
                                    _( "Field name has leading or trailing whitespace: '%s'" ),
                                    field.GetName() ) );

                    SCH_MARKER* marker = new SCH_MARKER( std::move( ercItem ), field.GetPosition() );
                    screen->Append( marker );
                    warnings++;
                }
            }
        }
    }

    return warnings;
}


std::vector<SCH_CONNECTIVITY::MULTI_UNIT_GROUP> ERC_TESTER::multiUnitSources() const
{
    using namespace SCH_CONNECTIVITY;

    if( ADVANCED_CFG::GetCfg().m_ConnectivityEngine )
        return m_schematic->Connectivity().Engine().MultiUnitSymbols();

    std::vector<MULTI_UNIT_GROUP> result;

    for( const auto& [reference, refs] : m_refMap )
    {
        if( refs.GetCount() == 0 )
            continue;

        MULTI_UNIT_GROUP group;
        group.reference = reference;
        group.units = ExtractUnitFacts( *refs.GetItem( 0 ).GetLibPart() );

        for( size_t i = 0; i < refs.GetCount(); ++i )
        {
            const SCH_REFERENCE& ref = refs.GetItem( i );
            const SCH_SYMBOL* symbol = ref.GetSymbol();
            group.instances.push_back( { ref.GetSheetPath().PathRef(), symbol->m_Uuid,
                    symbol->GetPosition(), symbol->GetRef( &ref.GetSheetPath(), true ),
                    ref.GetFootprint(), ref.GetUnit() } );
        }

        result.push_back( std::move( group ) );
    }

    return result;
}


int ERC_TESTER::TestMultiunitFootprints()
{
    ERC_MARKER_PLACER markers( *m_schematic );

    for( const auto& group : multiUnitSources() )
    {
        const SCH_CONNECTIVITY::MULTI_UNIT_INSTANCE* reference = nullptr;

        for( const auto& unit : group.instances )
        {
            if( !unit.footprint.IsEmpty() )
            {
                reference = &unit;
                break;
            }
        }

        if( !reference )
            continue;

        // Equivalent reference witnesses must retain exclusions when pages are reordered
        for( const auto& unit : group.instances )
        {
            if( unit.item == reference->item && unit.footprint == reference->footprint
                && unit.name == reference->name && unit.sheet < reference->sheet )
            {
                reference = &unit;
            }
        }

        for( const auto& unit : group.instances )
        {
            if( unit.footprint.IsEmpty() || unit.footprint == reference->footprint )
                continue;

            auto error = ERC_ITEM::Create( ERCE_DIFFERENT_UNIT_FP );
            error->SetErrorMessage( wxString::Format( _( "Different footprints assigned to %s and %s" ),
                                                     reference->name, unit.name ) );
            error->SetItems( std::vector<KIID>{ reference->item, unit.item } );
            markers.Place( std::move( error ), unit.sheet, unit.position, reference->sheet, unit.sheet );
        }
    }

    return markers.Count();
}


int ERC_TESTER::TestMissingUnits()
{
    ERC_MARKER_PLACER markers( *m_schematic );

    for( const auto& group : multiUnitSources() )
    {
        if( group.instances.empty() )
            continue;

        const auto& base = group.instances.front();
        std::set<int> placed;

        for( const auto& instance : group.instances )
            placed.insert( instance.unit );

        std::set<int> missing;
        std::set<int> power;
        std::set<int> input;
        std::set<int> bidirectional;

        for( size_t i = 0; i < group.units.size(); ++i )
        {
            const int unit = static_cast<int>( i ) + 1;

            if( placed.contains( unit ) )
                continue;

            missing.insert( unit );

            if( group.units[i].powerInput )
                power.insert( unit );

            if( group.units[i].input )
                input.insert( unit );

            if( group.units[i].bidirectional )
                bidirectional.insert( unit );
        }

        const auto report =
                [&]( const std::set<int>& units, const wxString& message, int code )
                {
                    if( units.empty() || !m_settings.IsTestEnabled( code ) )
                        return;

                    wxString names = wxS( "[ " );
                    int      count = 0;

                    for( int unit : units )
                    {
                        if( count == 3 )
                            break;

                        if( count++ )
                            names += wxS( ", " );

                        names += group.units[unit - 1].name;
                    }

                    if( units.size() > 3 )
                        names += wxS( ", ..." );

                    names += wxS( " ]" );
                    markers.Report( code, { base.item }, base.sheet, base.position, ITEM_PATHS::MAIN,
                                    wxString::Format( message, group.reference, names ) );
                };

        report( missing, _( "Symbol %s has unplaced units %s" ),
                ERCE_MISSING_UNIT );
        report( power, _( "Symbol %s has input power pins in units %s that are not placed" ),
                ERCE_MISSING_POWER_INPUT_PIN );
        report( input, _( "Symbol %s has input pins in units %s that are not placed" ),
                ERCE_MISSING_INPUT_PIN );
        report( bidirectional, _( "Symbol %s has bidirectional pins in units %s that are not placed" ),
                ERCE_MISSING_BIDI_PIN );
    }

    return markers.Count();
}


int ERC_TESTER::TestMissingNetclasses()
{
    int                            err_count = 0;
    std::shared_ptr<NET_SETTINGS>& settings = m_schematic->Project().GetProjectFile().NetSettings();
    wxString                       defaultNetclass = settings->GetDefaultNetclass()->GetName();

    auto logError =
            [&]( const SCH_SHEET_PATH& sheet, const KIID& item, const VECTOR2I& position, const wxString& netclass )
            {
                err_count++;

                std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_UNDEFINED_NETCLASS );

                ercItem->SetItems( std::vector<KIID>{ item } );
                ercItem->SetSheetSpecificPath( sheet );
                ercItem->SetItemsSheetPaths( sheet );
                ercItem->SetErrorMessage( wxString::Format( _( "Netclass %s is not defined" ), netclass ) );

                SCH_MARKER* marker = new SCH_MARKER( std::move( ercItem ), position );
                sheet.LastScreen()->Append( marker );
            };

    if( ADVANCED_CFG::GetCfg().m_ConnectivityEngine )
    {
        ERC_MARKER_PLACER sheets( *m_schematic );

        for( const auto& reference : m_schematic->Connectivity().Engine().NetclassReferences() )
        {
            if( reference.name == defaultNetclass || settings->HasNetclass( reference.name ) )
                continue;

            if( const SCH_SHEET_PATH* sheet = sheets.Find( reference.sheet ) )
                logError( *sheet, reference.item, reference.position, reference.name );
        }

        return err_count;
    }

    for( const SCH_SHEET_PATH& sheet : m_sheetList )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items() )
        {
            item->RunOnChildren(
                    [&]( SCH_ITEM* aChild )
                    {
                        if( aChild->Type() == SCH_FIELD_T )
                        {
                            SCH_FIELD* field = static_cast<SCH_FIELD*>( aChild );

                            if( field->GetUntranslatedName() == wxT( "Netclass" ) )
                            {
                                wxString netclass = field->GetShownText( &sheet, FOR_NETNAME,
                                                                         m_schematic->GetCurrentVariant() );

                                if( !netclass.empty()
                                        && !netclass.IsSameAs( defaultNetclass )
                                        && !settings->HasNetclass( netclass ) )
                                {
                                    logError( sheet, item->m_Uuid, item->GetPosition(), netclass );
                                }
                            }
                        }

                        return true;
                    },
                RECURSE_MODE::NO_RECURSE );
        }
    }

    return err_count;
}


int ERC_TESTER::TestLabelMultipleWires()
{
    int err_count = 0;

    if( ADVANCED_CFG::GetCfg().m_ConnectivityEngine )
    {
        ERC_MARKER_PLACER markers( *m_schematic );

        for( const auto& conflict : m_schematic->Connectivity().Engine().LabelWireConflicts() )
        {
            std::vector<KIID> witnesses{ conflict.label };
            witnesses.insert( witnesses.end(), conflict.wires.begin(),
                              conflict.wires.begin() + std::min<std::ptrdiff_t>( 3, std::ssize( conflict.wires ) ) );
            markers.Report( ERCE_LABEL_MULTIPLE_WIRES, witnesses, conflict.sheet, conflict.position, ITEM_PATHS::NONE,
                            wxString::Format( _( "Label connects more than one wire at %d, %d" ),
                                              conflict.position.x, conflict.position.y ) );
        }

        return markers.Count();
    }

    for( const SCH_SHEET_PATH& sheet : m_sheetList )
    {
        std::map<VECTOR2I, std::vector<SCH_ITEM*>> connMap;

        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_LABEL_T ) )
        {
            SCH_LABEL* label = static_cast<SCH_LABEL*>( item );

            for( const VECTOR2I& pt : label->GetConnectionPoints() )
                connMap[pt].emplace_back( label );
        }

        for( const std::pair<const VECTOR2I, std::vector<SCH_ITEM*>>& pair : connMap )
        {
            std::vector<SCH_ITEM*> lines;

            for( SCH_ITEM* item : sheet.LastScreen()->Items().Overlapping( SCH_LINE_T, pair.first ) )
            {
                SCH_LINE* line = static_cast<SCH_LINE*>( item );

                if( line->IsGraphicLine() )
                    continue;

                // If the line is connected at the endpoint, then there will be a junction
                if( !line->IsEndPoint( pair.first ) )
                    lines.emplace_back( line );
            }

            if( lines.size() > 1 )
            {
                err_count++;
                lines.resize( 3 ); // Only show the first 3 lines and if there are only two, adds a nullptr

                std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_LABEL_MULTIPLE_WIRES );

                ercItem->SetItems( pair.second.front(), lines[0], lines[1], lines[2] );
                ercItem->SetErrorMessage( wxString::Format( _( "Label connects more than one wire at %d, %d" ),
                                                            pair.first.x, pair.first.y ) );
                ercItem->SetSheetSpecificPath( sheet );

                SCH_MARKER* marker = new SCH_MARKER( std::move( ercItem ), pair.first );
                sheet.LastScreen()->Append( marker );
            }
        }
    }

    return err_count;
}


int ERC_TESTER::TestFourWayJunction()
{
    int err_count = 0;

    if( ADVANCED_CFG::GetCfg().m_ConnectivityEngine )
    {
        ERC_MARKER_PLACER markers( *m_schematic );

        for( const auto& junction : m_schematic->Connectivity().Engine().FourWayJunctions() )
        {
            markers.Report( ERCE_FOUR_WAY_JUNCTION, { junction.items.begin(), junction.items.begin() + 4 },
                            junction.sheet, junction.position, ITEM_PATHS::NONE,
                            wxString::Format( _( "Four items connected at %d, %d" ),
                                              junction.position.x, junction.position.y ) );
        }

        return markers.Count();
    }

    auto pinStackAlreadyRepresented =
            []( SCH_PIN* pin, std::vector<SCH_ITEM*>& collection ) -> bool
            {
                for( SCH_ITEM*& item : collection )
                {
                    if( item->Type() == SCH_PIN_T && item->GetParentSymbol() == pin->GetParentSymbol() )
                    {
                        if( pin->IsVisible() && !static_cast<SCH_PIN*>( item )->IsVisible() )
                            item = pin;

                        return true;
                    }
                }

                return false;
            };

    for( const SCH_SHEET_PATH& sheet : m_sheetList )
    {
        std::map<VECTOR2I, std::vector<SCH_ITEM*>> connMap;
        SCH_SCREEN* screen = sheet.LastScreen();

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            for( SCH_PIN* pin : symbol->GetPins( &sheet ) )
            {
                std::vector<SCH_ITEM*>& entry = connMap[pin->GetPosition()];

                // Only one pin per pin-stack.
                if( pinStackAlreadyRepresented( pin, entry ) )
                    continue;

                entry.emplace_back( pin );
            }
        }

        for( SCH_ITEM* item : screen->Items().OfType( SCH_LINE_T ) )
        {
            SCH_LINE* line = static_cast<SCH_LINE*>( item );

            if( line->IsGraphicLine() )
                continue;

            for( const VECTOR2I& pt : line->GetConnectionPoints() )
                connMap[pt].emplace_back( line );
        }

        for( const std::pair<const VECTOR2I, std::vector<SCH_ITEM*>>& pair : connMap )
        {
            if( pair.second.size() >= 4 )
            {
                err_count++;

                std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_FOUR_WAY_JUNCTION );

                ercItem->SetItems( pair.second[0], pair.second[1], pair.second[2], pair.second[3] );

                ercItem->SetErrorMessage( wxString::Format( _( "Four items connected at %d, %d" ),
                                                            pair.first.x, pair.first.y ) );

                ercItem->SetSheetSpecificPath( sheet );

                SCH_MARKER* marker = new SCH_MARKER( std::move( ercItem ), pair.first );
                sheet.LastScreen()->Append( marker );
            }
        }
    }

    return err_count;
}


int ERC_TESTER::TestNoConnectPins()
{
    int err_count = 0;

    if( ADVANCED_CFG::GetCfg().m_ConnectivityEngine )
    {
        if( !m_settings.IsTestEnabled( ERCE_NOCONNECT_CONNECTED ) )
            return 0;

        ERC_MARKER_PLACER markers( *m_schematic );

        for( const auto& conflict : m_schematic->Connectivity().Engine().NoConnectPinConflicts() )
        {
            // Reserve a witness for the actual connection even when many NC pins coincide
            const auto witnessPins = std::min<std::ptrdiff_t>( 3, std::ssize( conflict.pins ) );
            std::vector<KIID> items( conflict.pins.begin(), conflict.pins.begin() + witnessPins );
            const auto witnessOthers = std::min<std::ptrdiff_t>( 4 - witnessPins, std::ssize( conflict.others ) );
            items.insert( items.end(), conflict.others.begin(), conflict.others.begin() + witnessOthers );
            markers.Report( ERCE_NOCONNECT_CONNECTED, items, conflict.sheet, conflict.position, ITEM_PATHS::NONE,
                            _( "Pin with 'no connection' type is connected" ) );
        }

        return markers.Count();
    }

    for( const SCH_SHEET_PATH& sheet : m_sheetList )
    {
        std::map<VECTOR2I, std::vector<SCH_ITEM*>> pinMap;

        auto addOther =
                [&]( const VECTOR2I& pt, SCH_ITEM* aOther )
                {
                    if( pinMap.count( pt ) )
                        pinMap[pt].emplace_back( aOther );
                };

        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            for( SCH_PIN* pin : symbol->GetPins( &sheet ) )
            {
                if( pin->GetType() == ELECTRICAL_PINTYPE::PT_NC )
                    pinMap[pin->GetPosition()].emplace_back( pin );
            }
        }

        for( SCH_ITEM* item : sheet.LastScreen()->Items() )
        {
            if( item->Type() == SCH_SYMBOL_T )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

                for( SCH_PIN* pin : symbol->GetPins( &sheet ) )
                {
                    if( pin->GetType() != ELECTRICAL_PINTYPE::PT_NC )
                        addOther( pin->GetPosition(), pin );
                }
            }
            else if( item->IsConnectable() && item->Type() != SCH_NO_CONNECT_T )
            {
                for( const VECTOR2I& pt : item->GetConnectionPoints() )
                    addOther( pt, item );
            }
        }

        for( const std::pair<const VECTOR2I, std::vector<SCH_ITEM*>>& pair : pinMap )
        {
            if( pair.second.size() > 1 )
            {
                bool all_nc = true;

                for( SCH_ITEM* item : pair.second )
                {
                    if( item->Type() != SCH_PIN_T )
                    {
                        all_nc = false;
                        break;
                    }

                    SCH_PIN* pin = static_cast<SCH_PIN*>( item );

                    if( pin->GetType() != ELECTRICAL_PINTYPE::PT_NC )
                    {
                        all_nc = false;
                        break;
                    }
                }

                if( all_nc )
                    continue;

                err_count++;

                std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_NOCONNECT_CONNECTED );

                ercItem->SetItems( pair.second[0], pair.second[1],
                                   pair.second.size() > 2 ? pair.second[2] : nullptr,
                                   pair.second.size() > 3 ? pair.second[3] : nullptr );
                ercItem->SetErrorMessage( _( "Pin with 'no connection' type is connected" ) );
                ercItem->SetSheetSpecificPath( sheet );

                SCH_MARKER* marker = new SCH_MARKER( std::move( ercItem ), pair.first );
                sheet.LastScreen()->Append( marker );
            }
        }
    }

    return err_count;
}


namespace
{
ERCE_T ercErrorCode( PIN_ERROR aPinError )
{
    return ( aPinError == PIN_ERROR::WARNING ) ? ERCE_PIN_TO_PIN_WARNING
                                               : ERCE_PIN_TO_PIN_ERROR;
}

int testCapturedPinToPin( SCHEMATIC& aSchematic, ERC_SETTINGS& aSettings, bool aShowAllErrors )
{
    using PIN = SCH_CONNECTIVITY::ERC_PIN;

    std::vector<SCH_CONNECTIVITY::ERC_PIN_NET>               nets = aSchematic.Connectivity().Engine().PinNets();
    const std::vector<std::unique_ptr<SCH_NETCHAIN>>&        chains = aSchematic.NetChains().GetCommittedNetChains();
    std::map<wxString, const SCH_CONNECTIVITY::ERC_PIN_NET*> byName;
    std::set<wxString>                                       powerDriven;

    ERC_MARKER_PLACER markers( aSchematic );

    const auto less =
            []( const PIN& a, const PIN& b )
            {
                int order = StrNumCmp( a.reference, b.reference );

                if( order == 0 )
                    order = StrNumCmp( a.pin.number, b.pin.number );

                if( order != 0 )
                    return order < 0;

                // Tuple comparison would bypass KIID_PATH's length-first ordering
                if( a.sheet != b.sheet )
                    return a.sheet < b.sheet;

                return a.pin.id < b.pin.id;
            };

    const auto mismatch =
            [&]( const PIN& a, const PIN& b )
            {
                const PIN_ERROR pinError = aSettings.GetPinMapValue( a.pin.type, b.pin.type );

                return aSettings.IsTestEnabled( ercErrorCode( pinError ) ) ? pinError : PIN_ERROR::OK;
            };

    const auto reportPair =
            [&]( const PIN& a, const PIN& b, PIN_ERROR pinError, const wxString& chain )
            {
                std::shared_ptr<ERC_ITEM> item = ERC_ITEM::Create( ercErrorCode( pinError ) );

                item->SetItems( std::vector<KIID>{ a.pin.id, b.pin.id } );

                if( chain.empty() )
                {
                    item->SetErrorMessage( wxString::Format( _( "Pins of type %s and %s are connected" ),
                                                             ElectricalPinTypeGetText( a.pin.type ),
                                                             ElectricalPinTypeGetText( b.pin.type ) ) );
                }
                else
                {
                    item->SetErrorMessage( wxString::Format( _( "Pins of type %s and %s are connected via "
                                                                "net chain %s" ),
                                                             ElectricalPinTypeGetText( a.pin.type ),
                                                             ElectricalPinTypeGetText( b.pin.type ),
                                                             chain ) );
                }
                markers.Place( std::move( item ), a.sheet, a.pin.position, a.sheet, b.sheet );
            };

    const auto passiveOrNic =
            []( ELECTRICAL_PINTYPE type )
            {
                return type == ELECTRICAL_PINTYPE::PT_PASSIVE || type == ELECTRICAL_PINTYPE::PT_NIC;
            };

    const bool heuristics = aSettings.GetERCSortingMetric() == ERC_PIN_SORTING_METRIC::SM_HEURISTICS;

    struct MISMATCH
    {
        size_t    first;
        size_t    second;
        PIN_ERROR error;
    };

    for( auto& net : nets )
    {
        std::sort( net.pins.begin(), net.pins.end(), less );
        byName.emplace( net.name, &net );

        if( net.powerDriven )
            powerDriven.insert( net.name );
    }

    for( const auto& net : nets )
    {
        const std::vector<PIN>& pins = net.pins;
        const bool              powerNet = std::ranges::any_of( pins,
                                                   []( const PIN& pin )
                                                   {
                                                       return pin.pin.type == ELECTRICAL_PINTYPE::PT_POWER_IN;
                                                   } );
        const PIN*              preferred = nullptr;
        std::vector<const PIN*> needsDriver;
        std::vector<const PIN*> nonPowerInputs;
        std::vector<const PIN*> powerInputs;
        std::vector<MISMATCH>   mismatches;
        std::map<size_t, int>   weights;
        bool                    hasDriver = net.powerDriven;

        for( size_t i = 0; i < pins.size(); ++i )
        {
            const PIN& pin = pins[i];
            hasDriver |= powerNet ? DrivingPowerPinTypes.contains( pin.pin.type )
                                  : DrivingPinTypes.contains( pin.pin.type );

            if( DrivenPinTypes.contains( pin.pin.type ) )
            {
                needsDriver.push_back( &pin );

                if( !pin.pin.globalPower && !pin.pin.localPower )
                    nonPowerInputs.push_back( &pin );

                if( pin.pin.type == ELECTRICAL_PINTYPE::PT_POWER_IN )
                    powerInputs.push_back( &pin );

                if( !preferred
                        || ( preferred->pin.invisible && !pin.pin.invisible )
                        || ( powerNet != ( preferred->pin.type == ELECTRICAL_PINTYPE::PT_POWER_IN )
                                 && powerNet == ( pin.pin.type == ELECTRICAL_PINTYPE::PT_POWER_IN ) ) )
                {
                    preferred = &pin;
                }
            }

            for( size_t j = i + 1; j < pins.size(); ++j )
            {
                const PIN& other = pins[j];
                const bool stacked = pin.sheet == other.sheet
                                    && pin.owner == other.owner
                                    && pin.pin.position == other.pin.position
                                    && pin.pin.name == other.pin.name
                                    && (   pin.pin.type == other.pin.type
                                        || passiveOrNic( pin.pin.type )
                                        || passiveOrNic( other.pin.type ) );

                if( stacked )
                    continue;

                const PIN_ERROR error = mismatch( pin, other );

                if( error == PIN_ERROR::OK )
                    continue;

                mismatches.push_back( { i, j, error } );

                if( heuristics )
                {
                    weights[i] = aSettings.GetPinTypeWeight( pin.pin.type );
                    weights[j] = aSettings.GetPinTypeWeight( other.pin.type );
                }
                else
                {
                    ++weights[i];
                    ++weights[j];
                }
            }
        }

        // Each pin reports once against its nearest mismatch so one bad pin does not flood the net
        std::vector<std::pair<int, size_t>> worst;

        for( const auto& [index, weight] : weights )
            worst.emplace_back( weight, index );

        std::stable_sort( worst.begin(), worst.end(),
                          []( const auto& a, const auto& b )
                          {
                              return a.first > b.first;
                          } );

        for( const auto& [weight, index] : worst )
        {
            const PIN& pin = pins[index];
            const PIN* nearest = nullptr;
            PIN_ERROR error = PIN_ERROR::OK;
            std::optional<double> nearestDistance;

            std::erase_if( mismatches,
                    [&]( const MISMATCH& candidate )
                    {
                        if( candidate.first != index && candidate.second != index )
                            return false;

                        const PIN& other = pins[candidate.first == index ? candidate.second : candidate.first];

                        // Cross-sheet partners have no distance and stand in only until a same-sheet partner appears
                        if( other.sheet != pin.sheet )
                        {
                            if( !nearestDistance )
                            {
                                nearest = &other;
                                error = candidate.error;
                            }
                        }
                        else if( const double distance = pin.pin.position.Distance( other.pin.position );
                                 !nearestDistance || distance < *nearestDistance )
                        {
                            nearestDistance = distance;
                            nearest = &other;
                            error = candidate.error;
                        }

                        return true;
                    } );

            if( nearest )
                reportPair( pin, *nearest, error, wxString() );
        }

        if( !preferred || hasDriver || net.noConnect )
            continue;

        bool chainDriver = false;

        if( powerNet )
        {
            for( const auto& chain : chains )
            {
                if( !chain || !chain->GetNets().contains( net.name ) )
                    continue;

                chainDriver = std::ranges::any_of( chain->GetNets(),
                                                   [&]( const wxString& name )
                                                   {
                                                       return name != net.name && powerDriven.contains( name );
                                                   } );
                break;
            }
        }

        const ERCE_T code = powerNet ? ERCE_POWERPIN_NOT_DRIVEN : ERCE_PIN_NOT_DRIVEN;

        if( chainDriver || !aSettings.IsTestEnabled( code ) )
            continue;

        if( aShowAllErrors )
        {
            const auto& selected = powerNet && !powerInputs.empty() ? powerInputs
                                                                    : !nonPowerInputs.empty() ? nonPowerInputs
                                                                                              : needsDriver;

            for( const PIN* pin : selected )
                markers.Report( code, { pin->pin.id }, pin->sheet, pin->pin.position, ITEM_PATHS::MAIN );
        }
        else
        {
            const PIN& pin = powerNet && !powerInputs.empty() ? *powerInputs.front()
                                                              : *preferred;
            markers.Report( code, { pin.pin.id }, pin.sheet, pin.pin.position, ITEM_PATHS::MAIN );
        }
    }

    for( const std::unique_ptr<SCH_NETCHAIN>& chain : chains )
    {
        if( !chain )
            continue;

        struct CHAIN_PIN
        {
            const PIN*      pin;
            const wxString* net;
        };
        std::vector<CHAIN_PIN> pins;

        for( const wxString& name : chain->GetNets() )
        {
            const auto net = byName.find( name );

            if( net == byName.end() )
                continue;

            for( const PIN& pin : net->second->pins )
                pins.push_back( { &pin, &net->first } );
        }

        std::sort( pins.begin(), pins.end(),
                   [&]( const CHAIN_PIN& a, const CHAIN_PIN& b )
                   {
                       return less( *a.pin, *b.pin );
                   } );

        for( size_t i = 0; i < pins.size(); ++i )
        {
            for( size_t j = i + 1; j < pins.size(); ++j )
            {
                if( *pins[i].net == *pins[j].net )
                    continue;

                if( const PIN_ERROR error = mismatch( *pins[i].pin, *pins[j].pin ); error != PIN_ERROR::OK )
                    reportPair( *pins[i].pin, *pins[j].pin, error, chain->GetName() );
            }
        }
    }

    return markers.Count();
}
}


int ERC_TESTER::TestPinToPin()
{
    if( ADVANCED_CFG::GetCfg().m_ConnectivityEngine )
        return testCapturedPinToPin( *m_schematic, m_settings, m_showAllErrors );

    int errors = 0;
    std::set<wxString> powerDrivenNets;

    if( !m_schematic->ConnectionGraph()->GetCommittedNetChains().empty() )
    {
        for( const auto& [key, subgraphs] : m_nets )
        {
            for( const CONNECTION_SUBGRAPH* subgraph : subgraphs )
            {
                for( SCH_ITEM* item : subgraph->GetItems() )
                {
                    if( item->Type() != SCH_PIN_T )
                        continue;

                    const SCH_PIN* pin = static_cast<SCH_PIN*>( item );

                    if( DrivingPowerPinTypes.contains( pin->GetType() ) )
                        powerDrivenNets.insert( key.Name );
                }
            }
        }
    }


    // Map each net name to the pins (with sheet context) found on that net so we can later
    // perform cross-net compatibility checks for grouped net chains.
    std::unordered_map<wxString, std::vector<ERC_SCH_PIN_CONTEXT>> netToPins;

    for( const std::pair<NET_NAME_CODE_CACHE_KEY, std::vector<CONNECTION_SUBGRAPH*>> net : m_nets )
    {
        using iterator_t = std::vector<ERC_SCH_PIN_CONTEXT>::iterator;
        std::vector<ERC_SCH_PIN_CONTEXT>           pins;
        std::unordered_map<EDA_ITEM*, SCH_SCREEN*> pinToScreenMap;
        bool has_noconnect = false;

        for( CONNECTION_SUBGRAPH* subgraph: net.second )
        {
            if( subgraph->GetNoConnect() )
                has_noconnect = true;

            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() == SCH_PIN_T )
                {
                    pins.emplace_back( static_cast<SCH_PIN*>( item ), subgraph->GetSheet() );
                    netToPins[ net.first.Name ].emplace_back( static_cast<SCH_PIN*>( item ), subgraph->GetSheet() );
                    pinToScreenMap[item] = subgraph->GetSheet().LastScreen();
                }
            }
        }

        std::sort( pins.begin(), pins.end(),
                   []( const ERC_SCH_PIN_CONTEXT& lhs, const ERC_SCH_PIN_CONTEXT& rhs )
                   {
                       int ret = StrNumCmp( lhs.Pin()->GetParentSymbol()->GetRef( &lhs.Sheet() ),
                                            rhs.Pin()->GetParentSymbol()->GetRef( &rhs.Sheet() ) );

                       if( ret == 0 )
                           ret = StrNumCmp( lhs.Pin()->GetNumber(), rhs.Pin()->GetNumber() );

                       return ret != 0 ? ret < 0 : lhs < rhs;
                   } );

        ERC_SCH_PIN_CONTEXT needsDriver;
        ELECTRICAL_PINTYPE  needsDriverType = ELECTRICAL_PINTYPE::PT_UNSPECIFIED;
        bool                hasDriver = false;
        std::vector<ERC_SCH_PIN_CONTEXT*> pinsNeedingDrivers;
        std::vector<ERC_SCH_PIN_CONTEXT*> nonPowerPinsNeedingDrivers;
        std::vector<ERC_SCH_PIN_CONTEXT*> powerInPinsNeedingDrivers;

        // We need different drivers for power nets and normal nets.
        // A power net has at least one pin having the ELECTRICAL_PINTYPE::PT_POWER_IN
        // and power nets can be driven only by ELECTRICAL_PINTYPE::PT_POWER_OUT pins
        bool     ispowerNet  = false;

        for( ERC_SCH_PIN_CONTEXT& refPin : pins )
        {
            if( refPin.Pin()->GetType() == ELECTRICAL_PINTYPE::PT_POWER_IN )
            {
                ispowerNet = true;
                break;
            }
        }

        std::vector<std::tuple<iterator_t, iterator_t, PIN_ERROR>> pin_mismatches;
        std::map<iterator_t, int>                                  pin_mismatch_counts;

        for( auto refIt = pins.begin(); refIt != pins.end(); ++refIt )
        {
            ERC_SCH_PIN_CONTEXT& refPin = *refIt;
            ELECTRICAL_PINTYPE refType = refPin.Pin()->GetType();

            if( DrivenPinTypes.contains( refType ) )
            {
                // needsDriver will be the pin shown in the error report eventually, so try to
                // upgrade to a "better" pin if possible: something visible and only a power symbol
                // if this net needs a power driver
                pinsNeedingDrivers.push_back( &refPin );

                if( !refPin.Pin()->IsPower() )
                    nonPowerPinsNeedingDrivers.push_back( &refPin );

                if( refType == ELECTRICAL_PINTYPE::PT_POWER_IN )
                    powerInPinsNeedingDrivers.push_back( &refPin );

                if( !needsDriver.Pin()
                    || ( !needsDriver.Pin()->IsVisible() && refPin.Pin()->IsVisible() )
                    || ( ispowerNet != ( needsDriverType == ELECTRICAL_PINTYPE::PT_POWER_IN )
                         && ispowerNet == ( refType == ELECTRICAL_PINTYPE::PT_POWER_IN ) ) )
                {
                    needsDriver = refPin;
                    needsDriverType = needsDriver.Pin()->GetType();
                }
            }

            if( ispowerNet )
                hasDriver |= ( DrivingPowerPinTypes.count( refType ) != 0 );
            else
                hasDriver |= ( DrivingPinTypes.count( refType ) != 0 );

            for( auto testIt = refIt + 1; testIt != pins.end(); ++testIt )
            {
                ERC_SCH_PIN_CONTEXT& testPin = *testIt;

                // Multiple pins in the same symbol that share a type,
                // name and position are considered
                // "stacked" and shouldn't trigger ERC errors
                if( refPin.Pin()->IsStacked( testPin.Pin() ) && refPin.Sheet() == testPin.Sheet() )
                    continue;

                ELECTRICAL_PINTYPE testType = testPin.Pin()->GetType();

                if( ispowerNet )
                    hasDriver |= DrivingPowerPinTypes.contains( testType );
                else
                    hasDriver |= DrivingPinTypes.contains( testType );

                PIN_ERROR pinError = m_settings.GetPinMapValue( refType, testType );

                if( pinError != PIN_ERROR::OK && m_settings.IsTestEnabled( ercErrorCode( pinError ) ) )
                {
                    pin_mismatches.emplace_back( std::tuple<iterator_t, iterator_t,
                                                 PIN_ERROR>{ refIt, testIt, pinError } );

                    if( m_settings.GetERCSortingMetric() == ERC_PIN_SORTING_METRIC::SM_HEURISTICS )
                    {
                        pin_mismatch_counts[refIt] = m_settings.GetPinTypeWeight( refIt->Pin()->GetType() );
                        pin_mismatch_counts[testIt] = m_settings.GetPinTypeWeight( testIt->Pin()->GetType() );
                    }
                    else
                    {
                        if( !pin_mismatch_counts.contains( testIt ) )
                            pin_mismatch_counts.emplace( testIt, 1 );
                        else
                            pin_mismatch_counts[testIt]++;

                        if( !pin_mismatch_counts.contains( refIt ) )
                            pin_mismatch_counts.emplace( refIt, 1 );
                        else
                            pin_mismatch_counts[refIt]++;
                    }
                }
            }
        }

        std::multimap<size_t, iterator_t, std::greater<size_t>> pins_dsc;

        std::transform( pin_mismatch_counts.begin(), pin_mismatch_counts.end(),
                        std::inserter( pins_dsc, pins_dsc.begin() ),
                        []( const auto& p )
                        {
                            return std::pair<size_t, iterator_t>( p.second, p.first );
                        } );

        for( const auto& [amount, pinItBind] : pins_dsc )
        {
            auto& pinIt = pinItBind;

            if( pin_mismatches.empty() )
                break;

            SCH_PIN* pin = pinIt->Pin();
            VECTOR2I position = pin->GetPosition();

            iterator_t nearest_pin = pins.end();
            double     smallest_distance = std::numeric_limits<double>::infinity();
            PIN_ERROR  pinError;

            std::erase_if(
                    pin_mismatches,
                    [&]( const auto& tuple )
                    {
                        iterator_t other;

                        if( pinIt == std::get<0>( tuple ) )
                            other = std::get<1>( tuple );
                        else if( pinIt == std::get<1>( tuple ) )
                            other = std::get<0>( tuple );
                        else
                            return false;

                        if( ( *pinIt ).Sheet().Cmp( ( *other ).Sheet() ) != 0 )
                        {
                            if( std::isinf( smallest_distance ) )
                            {
                                nearest_pin = other;
                                pinError = std::get<2>( tuple );
                            }
                        }
                        else
                        {
                            double distance = position.Distance( ( *other ).Pin()->GetPosition() );

                            if( std::isinf( smallest_distance ) || distance < smallest_distance )
                            {
                                smallest_distance = distance;
                                nearest_pin = other;
                                pinError = std::get<2>( tuple );
                            }
                        }

                        return true;
                    } );

            if( nearest_pin != pins.end() )
            {
                SCH_PIN*                  other_pin = nearest_pin->Pin();
                std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ercErrorCode( pinError ) );

                ercItem->SetItems( pin, other_pin );
                ercItem->SetSheetSpecificPath( pinIt->Sheet() );
                ercItem->SetItemsSheetPaths( pinIt->Sheet(), nearest_pin->Sheet() );

                ercItem->SetErrorMessage( wxString::Format( _( "Pins of type %s and %s are connected" ),
                                                            ElectricalPinTypeGetText( pin->GetType() ),
                                                            ElectricalPinTypeGetText( other_pin->GetType() ) ) );

                SCH_MARKER* marker = new SCH_MARKER( std::move( ercItem ), pin->GetPosition() );
                pinToScreenMap[pin]->Append( marker );
                errors++;
            }
        }

        if( needsDriver.Pin() && !hasDriver && !has_noconnect )
        {
            int err_code = ispowerNet ? ERCE_POWERPIN_NOT_DRIVEN : ERCE_PIN_NOT_DRIVEN;

            // NEW: For power nets, before reporting a not-driven error, look across the
            // net chain (multi-net chain formed via passives) to see if there is a
            // power driver pin on any other net in the same chain. If so, suppress the
            // error because the net chain as a whole is driven.
            bool suppressForNetChainDriver = false;

            if( ispowerNet && m_schematic && m_schematic->ConnectionGraph() )
            {
                const wxString& thisNetName = net.first.Name;
                const auto& netChains = m_schematic->ConnectionGraph()->GetCommittedNetChains();

                for( const auto& sig : netChains )
                {
                    if( !sig )
                        continue;

                    const auto& sigNets = sig->GetNets();
                    bool containsThisNet = std::find( sigNets.begin(), sigNets.end(), thisNetName ) != sigNets.end();

                    if( !containsThisNet )
                        continue;

                    // Look for a different net in this chain that has a power driver.
                    for( const wxString& otherNet : sigNets )
                    {
                        if( otherNet == thisNetName )
                            continue; // skip same net (we already know it lacks a driver)

                        if( powerDrivenNets.contains( otherNet ) )
                        {
                            suppressForNetChainDriver = true;
                            break;
                        }
                    }

                    break; // examined the containing chain
                }
            }

            if( !suppressForNetChainDriver && m_settings.IsTestEnabled( err_code ) )
            {
                std::vector<ERC_SCH_PIN_CONTEXT*> pinsToMark;

                // The marker should land on a pin matching the error message: for an
                // ERCE_POWERPIN_NOT_DRIVEN error mark a PT_POWER_IN pin (which is what the
                // error refers to), for ERCE_PIN_NOT_DRIVEN prefer a pin that is not on a
                // power symbol so the marker is anchored to the consuming pin rather than
                // a power flag.
                if( m_showAllErrors )
                {
                    if( ispowerNet && !powerInPinsNeedingDrivers.empty() )
                        pinsToMark = powerInPinsNeedingDrivers;
                    else if( !nonPowerPinsNeedingDrivers.empty() )
                        pinsToMark = nonPowerPinsNeedingDrivers;
                    else
                        pinsToMark = pinsNeedingDrivers;
                }
                else
                {
                    if( ispowerNet && !powerInPinsNeedingDrivers.empty() )
                        pinsToMark.push_back( powerInPinsNeedingDrivers.front() );
                    else
                        pinsToMark.push_back( &needsDriver );
                }

                for( ERC_SCH_PIN_CONTEXT* pinCtx : pinsToMark )
                {
                    std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( err_code );

                    ercItem->SetItems( pinCtx->Pin() );
                    ercItem->SetSheetSpecificPath( pinCtx->Sheet() );
                    ercItem->SetItemsSheetPaths( pinCtx->Sheet() );

                    SCH_MARKER* marker = new SCH_MARKER( std::move( ercItem ), pinCtx->Pin()->GetPosition() );
                    pinToScreenMap[pinCtx->Pin()]->Append( marker );
                    errors++;
                }
            }
        }
    }

    // --- Additional net-chain-level checking ---
    // If a pin participates in a grouped net chain (spanning multiple nets via passives), ensure
    // that all other pins reachable through that chain are electrically compatible even if
    // they reside on different nets.
    // We only consider pairs on DIFFERENT nets here to avoid duplicating existing net-level
    // mismatches already reported above.
    if( m_schematic && m_schematic->ConnectionGraph() )
    {
        auto& netChains = m_schematic->ConnectionGraph()->GetCommittedNetChains();
        wxLogTrace( traceSchNetChain, "ERC TestPinToPin: cross-chain phase start chains=%zu",
                    netChains.size() );

        for( const auto& sig : netChains )
        {
            if( !sig )
                continue;

            const wxString chainName = sig->GetName();
            const auto& sigNets = sig->GetNets();

            // Collect all pin contexts across the nets in this chain.
            struct CHAIN_PIN
            {
                ERC_SCH_PIN_CONTEXT context;
                const wxString* net;
            };
            std::vector<CHAIN_PIN> netChainPins;
            netChainPins.reserve( sigNets.size() * 4 );

            for( const wxString& n : sigNets )
            {
                auto it = netToPins.find( n );
                if( it != netToPins.end() )
                {
                    for( const ERC_SCH_PIN_CONTEXT& context : it->second )
                        netChainPins.push_back( { context, &it->first } );
                }
            }

            if( netChainPins.size() < 2 )
                continue; // nothing to compare

            wxLogTrace( traceSchNetChain,
                        "ERC TestPinToPin: chain '%s' nets=%zu collectedPins=%zu",
                        TO_UTF8( chainName ), sigNets.size(), netChainPins.size() );

            // For deterministic behavior, sort by reference/pin number similar to earlier pass.
            std::sort( netChainPins.begin(), netChainPins.end(),
                       []( const CHAIN_PIN& left, const CHAIN_PIN& right )
                       {
                           const auto& lhs = left.context;
                           const auto& rhs = right.context;
                           int ret = StrNumCmp( lhs.Pin()->GetParentSymbol()->GetRef( &lhs.Sheet() ),
                                                rhs.Pin()->GetParentSymbol()->GetRef( &rhs.Sheet() ) );
                           if( ret == 0 )
                               ret = StrNumCmp( lhs.Pin()->GetNumber(), rhs.Pin()->GetNumber() );
                           return ret != 0 ? ret < 0 : lhs < rhs;
                       } );

            for( size_t i = 0; i < netChainPins.size(); ++i )
            {
                const auto& aContext = netChainPins[i].context;
                SCH_PIN* aPin = aContext.Pin();
                ELECTRICAL_PINTYPE aType = aPin->GetType();
                const wxString& aNet = *netChainPins[i].net;

                for( size_t j = i + 1; j < netChainPins.size(); ++j )
                {
                    const auto& bContext = netChainPins[j].context;
                    SCH_PIN* bPin = bContext.Pin();
                    const wxString& bNet = *netChainPins[j].net;

                    if( aNet == bNet )
                        continue; // already handled at net-level

                    ELECTRICAL_PINTYPE bType = bPin->GetType();
                    PIN_ERROR          pinError = m_settings.GetPinMapValue( aType, bType );

                    if( pinError != PIN_ERROR::OK && m_settings.IsTestEnabled( ercErrorCode( pinError ) ) )
                    {
                        std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ercErrorCode( pinError ) );

                        ercItem->SetItems( aPin, bPin );
                        ercItem->SetSheetSpecificPath( aContext.Sheet() );
                        ercItem->SetItemsSheetPaths( aContext.Sheet(), bContext.Sheet() );
                        ercItem->SetErrorMessage( wxString::Format( _( "Pins of type %s and %s are connected via "
                                                                       "net chain %s" ),
                                                                    ElectricalPinTypeGetText( aType ),
                                                                    ElectricalPinTypeGetText( bType ), chainName ) );

                        SCH_MARKER* marker = new SCH_MARKER( std::move( ercItem ), aPin->GetPosition() );
                        aContext.Sheet().LastScreen()->Append( marker );
                        errors++;
                    }
                }
            }
        }
    }

    return errors;
}


int ERC_TESTER::TestMultUnitPinConflicts()
{
    int errors = 0;

    if( ADVANCED_CFG::GetCfg().m_ConnectivityEngine )
    {
        ERC_MARKER_PLACER markers( *m_schematic );

        for( const auto& conflict : m_schematic->Connectivity().Engine().MultiUnitPinConflicts() )
        {
            std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_DIFFERENT_UNIT_NET );
            ercItem->SetErrorMessage( wxString::Format( _( "Pin %s is connected to both %s and %s" ),
                                                        conflict.number,
                                                        conflict.otherNet,
                                                        conflict.firstNet ) );
            ercItem->SetItems( std::vector<KIID>{ conflict.other.item, conflict.first.item } );
            markers.Place( std::move( ercItem ), conflict.other.sheet, conflict.other.position, conflict.other.sheet,
                           conflict.first.sheet );
        }

        return markers.Count();
    }

    struct PIN_ON_NET
    {
        wxString       m_netName;
        SCH_PIN*       m_pin;
        SCH_SHEET_PATH m_sheet;
    };

    std::unordered_map<wxString, std::vector<PIN_ON_NET>> pinToNetMap;

    for( const std::pair<NET_NAME_CODE_CACHE_KEY, std::vector<CONNECTION_SUBGRAPH*>> net : m_nets )
    {
        const wxString& netName = net.first.Name;

        for( CONNECTION_SUBGRAPH* subgraph : net.second )
        {
            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() == SCH_PIN_T )
                {
                    SCH_PIN*              pin = static_cast<SCH_PIN*>( item );
                    const SCH_SHEET_PATH& sheet = subgraph->GetSheet();

                    if( !pin->GetParentSymbol()->IsMultiUnit() )
                        continue;

                    wxString name = pin->GetParentSymbol()->GetRef( &sheet ) + ":" + pin->GetShownNumber();

                    pinToNetMap[name].push_back( { netName, pin, sheet } );
                }
            }
        }
    }

    // Pick the reference net/pin deterministically, so the same conflict yields the same marker on every ERC run.
    std::vector<wxString> conflicted;

    for( const auto& [name, pins] : pinToNetMap )
    {
        for( const PIN_ON_NET& candidate : pins )
        {
            if( candidate.m_netName != pins.front().m_netName )
            {
                conflicted.push_back( name );
                break;
            }
        }
    }

    std::sort( conflicted.begin(), conflicted.end() );

    for( const wxString& name : conflicted )
    {
        std::vector<PIN_ON_NET> pins = pinToNetMap[name];

        std::sort( pins.begin(), pins.end(),
                   []( const PIN_ON_NET& a, const PIN_ON_NET& b )
                   {
                       if( a.m_netName != b.m_netName )
                           return a.m_netName < b.m_netName;

                       return a.m_pin->m_Uuid < b.m_pin->m_Uuid;
                   } );

        const PIN_ON_NET& first = pins.front();

        for( const PIN_ON_NET& other : pins )
        {
            if( other.m_netName == first.m_netName )
                continue;

            std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_DIFFERENT_UNIT_NET );

            ercItem->SetErrorMessage( wxString::Format( _( "Pin %s is connected to both %s and %s" ),
                                                        other.m_pin->GetShownNumber(),
                                                        other.m_netName,
                                                        first.m_netName ) );

            ercItem->SetItems( other.m_pin, first.m_pin );
            ercItem->SetSheetSpecificPath( other.m_sheet );
            ercItem->SetItemsSheetPaths( other.m_sheet, first.m_sheet );

            SCH_MARKER* marker = new SCH_MARKER( std::move( ercItem ), other.m_pin->GetPosition() );
            other.m_sheet.LastScreen()->Append( marker );
            errors += 1;
            break;
        }
    }

    return errors;
}


int ERC_TESTER::TestConnectivity( SCHEMATIC& aSchematic )
{
    const ERC_SETTINGS& settings = aSchematic.ErcSettings();
    const auto&         connectivity = aSchematic.Connectivity().Engine();
    ERC_MARKER_PLACER   markers( aSchematic );

    if( settings.IsTestEnabled( ERCE_WIRED_IMPLICIT_POWER ) )
    {
        for( const SCH_CONNECTIVITY::SOURCE_LOCATION& pin : connectivity.WiredImplicitPowerPins() )
        {
            std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_WIRED_IMPLICIT_POWER );
            ercItem->SetItems( pin.item );
            markers.Place( std::move( ercItem ), pin.sheet, pin.position, ITEM_PATHS::MAIN );
        }
    }

    if( settings.IsTestEnabled( ERCE_DRIVER_CONFLICT ) )
    {
        for( const SCH_CONNECTIVITY::DRIVER_CONFLICT& conflict : connectivity.DriverConflicts() )
        {
            std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_DRIVER_CONFLICT );
            ercItem->SetItems( conflict.first, conflict.second );
            ercItem->SetErrorMessage( wxString::Format( _( "Both %s and %s are attached to the same items; "
                                                           "%s will be used in the netlist" ),
                                                        conflict.firstName,
                                                        conflict.secondName,
                                                        conflict.firstName ) );
            markers.Place( std::move( ercItem ), conflict.sheet, conflict.position, ITEM_PATHS::BOTH );
        }
    }

    if( settings.IsTestEnabled( ERCE_UNCONNECTED_WIRE_ENDPOINT ) )
    {
        for( const SCH_CONNECTIVITY::WIRE_ENDPOINT& endpoint : connectivity.DanglingWireEndpoints() )
        {
            std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_UNCONNECTED_WIRE_ENDPOINT );
            ercItem->SetItems( endpoint.item );
            ercItem->SetErrorMessage( endpoint.busEntry ? _( "Unconnected wire to bus entry" )
                                                        : _( "Unconnected wire endpoint" ) );
            markers.Place( std::move( ercItem ), endpoint.sheet, endpoint.position );
        }
    }

    if( settings.IsTestEnabled( ERCE_BUS_TO_NET_CONFLICT ) )
    {
        for( const SCH_CONNECTIVITY::BUS_NET_CONFLICT& conflict : connectivity.BusNetConflicts() )
        {
            std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_BUS_TO_NET_CONFLICT );
            ercItem->SetItems( conflict.net, conflict.bus );
            markers.Place( std::move( ercItem ), conflict.sheet, conflict.position );
        }
    }

    if( settings.IsTestEnabled( ERCE_BUS_TO_BUS_CONFLICT ) )
    {
        for( const SCH_CONNECTIVITY::BUS_BUS_CONFLICT& conflict : connectivity.BusBusConflicts() )
        {
            std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_BUS_TO_BUS_CONFLICT );
            ercItem->SetItems( conflict.canonical, conflict.other );

            if( conflict.mixedShapes )
                ercItem->SetErrorMessage( _( "Bus vector and bus group are graphically connected" ) );

            markers.Place( std::move( ercItem ), conflict.sheet, conflict.position );
        }
    }

    if( settings.IsTestEnabled( ERCE_BUS_ENTRY_CONFLICT ) )
    {
        for( const SCH_CONNECTIVITY::BUS_ENTRY_CONFLICT& conflict : connectivity.BusEntryConflicts() )
        {
            std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_BUS_ENTRY_CONFLICT );
            ercItem->SetItems( conflict.entry, conflict.bus );
            ercItem->SetErrorMessage( wxString::Format( _( "Net %s is graphically connected to bus %s but is not a"
                                                           " member of that bus" ),
                                                         UnescapeString( conflict.netName ),
                                                         UnescapeString( conflict.busName ) ) );
            markers.Place( std::move( ercItem ), conflict.sheet, conflict.position );
        }
    }

    if( settings.IsTestEnabled( ERCE_NOCONNECT_CONNECTED ) || settings.IsTestEnabled( ERCE_NOCONNECT_NOT_CONNECTED ) )
    {
        for( const SCH_CONNECTIVITY::NO_CONNECT_FLAG_ERROR& diagnostic : connectivity.NoConnectFlagErrors() )
        {
            const int code = diagnostic.connected ? ERCE_NOCONNECT_CONNECTED : ERCE_NOCONNECT_NOT_CONNECTED;

            if( !settings.IsTestEnabled( code ) )
                continue;

            markers.Report( code,
                            diagnostic.pin == niluuid ? RC_ITEM::KIIDS{ diagnostic.flag }
                                                      : RC_ITEM::KIIDS{ diagnostic.pin, diagnostic.flag },
                            diagnostic.sheet, diagnostic.position, ITEM_PATHS::MAIN );
        }
    }

    if( settings.IsTestEnabled( ERCE_PIN_NOT_CONNECTED ) )
    {
        for( const SCH_CONNECTIVITY::UNCONNECTED_PIN& pin : connectivity.UnconnectedPins() )
            markers.Report( ERCE_PIN_NOT_CONNECTED, { pin.item }, pin.sheet, pin.position, ITEM_PATHS::MAIN );
    }

    if( settings.IsTestEnabled( ERCE_HIERACHICAL_LABEL ) || settings.IsTestEnabled( ERCE_PIN_NOT_CONNECTED ) )
    {
        using ERROR_KIND = SCH_CONNECTIVITY::HIERARCHY_ERROR::KIND;

        for( const SCH_CONNECTIVITY::HIERARCHY_ERROR& error : connectivity.HierarchyErrors() )
        {
            const bool mismatch = error.kind == ERROR_KIND::MISSING_LABEL || error.kind == ERROR_KIND::MISSING_PIN;
            const int code = mismatch ? ERCE_HIERACHICAL_LABEL : ERCE_PIN_NOT_CONNECTED;

            if( !settings.IsTestEnabled( code ) )
                continue;

            std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( code );
            ercItem->SetItems( std::vector<KIID>{ error.item } );
            const wxString name = UnescapeString( error.name );

            switch( error.kind )
            {
            case ERROR_KIND::ROOT_LABEL:
                ercItem->SetErrorMessage( wxString::Format( _( "Hierarchical label '%s' in root sheet cannot be "
                                                               "connected to non-existent parent sheet" ),
                                                            name ) );
                break;

            case ERROR_KIND::MISSING_LABEL:
                ercItem->SetErrorMessage( wxString::Format( _( "Sheet pin %s has no matching hierarchical label "
                                                               "inside the sheet" ),
                                                            name ) );
                break;

            case ERROR_KIND::MISSING_PIN:
                ercItem->SetErrorMessage( wxString::Format( _( "Hierarchical label %s has no matching sheet pin "
                                                               "in the parent sheet" ),
                                                            name ) );
                break;

            case ERROR_KIND::DANGLING_PIN:
                break;
            }

            // Root label markers are not sheet specific, and saved exclusions keep that shape
            const bool rootLabel = error.kind == ERROR_KIND::ROOT_LABEL;
            markers.Place( std::move( ercItem ), error.sheet, error.position,
                           rootLabel ? ITEM_PATHS::NONE : ITEM_PATHS::MAIN, !rootLabel );
        }
    }

    if( settings.IsTestEnabled( ERCE_LABEL_NOT_CONNECTED ) || settings.IsTestEnabled( ERCE_LABEL_SINGLE_PIN ) )
    {
        for( const SCH_CONNECTIVITY::LABEL_CONNECTION_ERROR& label : connectivity.LabelConnectionErrors() )
        {
            const int code = label.singlePin ? ERCE_LABEL_SINGLE_PIN : ERCE_LABEL_NOT_CONNECTED;

            if( settings.IsTestEnabled( code ) )
                markers.Report( code, { label.item }, label.sheet, label.position );
        }
    }

    if( settings.IsTestEnabled( ERCE_LABEL_NOT_CONNECTED ) )
    {
        for( const SCH_CONNECTIVITY::LABEL_LOCATION& label : connectivity.DanglingDirectives() )
            markers.Report( ERCE_LABEL_NOT_CONNECTED, { label.item }, label.sheet, label.position );
    }

    if( settings.IsTestEnabled( ERCE_SINGLE_GLOBAL_LABEL ) )
    {
        for( const SCH_CONNECTIVITY::LABEL_LOCATION& label : connectivity.SingleGlobalLabels() )
        {
            markers.Report( ERCE_SINGLE_GLOBAL_LABEL, { label.item }, label.sheet, label.position,
                            ITEM_PATHS::MAIN );
        }
    }

    if( settings.IsTestEnabled( ERCE_WIRE_DANGLING ) )
    {
        for( const SCH_CONNECTIVITY::FLOATING_WIRE& group : connectivity.FloatingWires() )
            markers.Report( ERCE_WIRE_DANGLING, group.items, group.sheet, group.position );
    }

    return markers.Count();
}


int ERC_TESTER::TestDuplicatePinNets()
{
    const bool engine = ADVANCED_CFG::GetCfg().m_ConnectivityEngine;
    int        errors = 0;

    for( const SCH_SHEET_PATH& sheet : m_sheetList )
    {
        SCH_SCREEN* screen = sheet.LastScreen();

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            LIB_SYMBOL* libSymbol = symbol->GetLibSymbolRef().get();

            if( !libSymbol )
                continue;

            if( libSymbol->GetDuplicatePinNumbersAreJumpers() )
                continue;

            std::vector<SCH_PIN*> pins = symbol->GetPins( &sheet );

            std::map<wxString, std::vector<std::pair<SCH_PIN*, wxString>>> pinsByNumber;

            for( SCH_PIN* pin : pins )
            {
                wxString netName;

                if( engine )
                    netName = pin->GetConnectionName( &sheet ).value_or( wxString() );
                else if( SCH_CONNECTION* conn = pin->Connection( &sheet ) )
                    netName = conn->GetNetName();

                pinsByNumber[pin->GetNumber()].emplace_back( pin, netName );
            }

            for( const auto& [pinNumber, pinNetPairs] : pinsByNumber )
            {
                if( pinNetPairs.size() < 2 )
                    continue;

                wxString firstNet = pinNetPairs[0].second;
                bool     hasDifferentNets = false;
                SCH_PIN* conflictPin = nullptr;

                for( size_t i = 1; i < pinNetPairs.size(); i++ )
                {
                    if( pinNetPairs[i].second != firstNet )
                    {
                        hasDifferentNets = true;
                        conflictPin = pinNetPairs[i].first;
                        break;
                    }
                }

                if( hasDifferentNets )
                {
                    std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_DUPLICATE_PIN_ERROR );
                    wxString msg;

                    msg.Printf( _( "Pin %s on symbol '%s' is connected to different nets: %s and %s" ),
                                pinNumber,
                                symbol->GetRef( &sheet ),
                                firstNet.IsEmpty() ? _( "<no net>" ) : firstNet,
                                pinNetPairs[1].second.IsEmpty() ? _( "<no net>" ) : pinNetPairs[1].second );

                    ercItem->SetErrorMessage( msg );
                    ercItem->SetItems( pinNetPairs[0].first, conflictPin );
                    ercItem->SetSheetSpecificPath( sheet );
                    ercItem->SetItemsSheetPaths( sheet, sheet );

                    SCH_MARKER* marker = new SCH_MARKER( std::move( ercItem ), pinNetPairs[0].first->GetPosition() );
                    screen->Append( marker );
                    errors++;
                }
            }
        }
    }

    return errors;
}


int ERC_TESTER::TestGroundPins()
{
    int errors = 0;

    if( ADVANCED_CFG::GetCfg().m_ConnectivityEngine )
    {
        ERC_MARKER_PLACER markers( *m_schematic );

        for( const auto& mismatch : m_schematic->Connectivity().Engine().GroundPinErrors() )
        {
            markers.Report( ERCE_GROUND_PIN_NOT_GROUND, { mismatch.pin }, mismatch.sheet, mismatch.position,
                            ITEM_PATHS::MAIN,
                            wxString::Format( _( "Pin %s not connected to ground net" ), mismatch.name ) );
        }

        return markers.Count();
    }

    auto isGround =
            []( const wxString& txt )
            {
                wxString upper = txt.Upper();

                return upper.Contains( wxT( "GND" ) )
                       || upper == wxT( "EARTH" )
                       || upper.StartsWith( wxT( "EARTH_" ) )
                       || upper == wxT( "VSS" )
                       || upper == wxT( "VSSA" );
            };

    for( const SCH_SHEET_PATH& sheet : m_sheetList )
    {
        SCH_SCREEN* screen = sheet.LastScreen();

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            bool        hasGroundNet = false;
            std::vector<SCH_PIN*> mismatched;

            for( SCH_PIN* pin : symbol->GetPins( &sheet ) )
            {
                // We are only interested in power pins
                if( pin->GetType() != ELECTRICAL_PINTYPE::PT_POWER_OUT
                    && pin->GetType() != ELECTRICAL_PINTYPE::PT_POWER_IN )
                {
                    continue;
                }

                const SCH_CONNECTION* conn = pin->Connection( &sheet );
                const wxString        net = conn ? conn->Name( true ) : wxString();
                const bool            netIsGround = isGround( net );

                if( netIsGround )
                    hasGroundNet = true;

                if( isGround( pin->GetShownName() ) && !netIsGround )
                    mismatched.push_back( pin );
            }

            if( hasGroundNet )
            {
                for( SCH_PIN* pin : mismatched )
                {
                    std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_GROUND_PIN_NOT_GROUND );

                    ercItem->SetErrorMessage( wxString::Format( _( "Pin %s not connected to ground net" ),
                                                                pin->GetShownName() ) );
                    ercItem->SetItems( pin );
                    ercItem->SetSheetSpecificPath( sheet );
                    ercItem->SetItemsSheetPaths( sheet );

                    SCH_MARKER* marker = new SCH_MARKER( std::move( ercItem ), pin->GetPosition() );
                    screen->Append( marker );
                    errors++;
                }
            }
        }
    }

    return errors;
}


int ERC_TESTER::TestStackedPinNotation()
{
    if( ADVANCED_CFG::GetCfg().m_ConnectivityEngine )
    {
        return reportSourceErrors( *m_schematic, ERCE_STACKED_PIN_SYNTAX,
                                   m_schematic->Connectivity().Engine().InvalidPinNotation() );
    }

    int warnings = 0;

    for( const SCH_SHEET_PATH& sheet : m_sheetList )
    {
        SCH_SCREEN* screen = sheet.LastScreen();

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            for( SCH_PIN* pin : symbol->GetPins( &sheet ) )
            {
                bool valid;
                pin->GetStackedPinNumbers( &valid );

                if( !valid )
                {
                    std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_STACKED_PIN_SYNTAX );
                    ercItem->SetItems( pin );
                    ercItem->SetSheetSpecificPath( sheet );
                    ercItem->SetItemsSheetPaths( sheet );

                    SCH_MARKER* marker = new SCH_MARKER( std::move( ercItem ), pin->GetPosition() );
                    screen->Append( marker );
                    warnings++;
                }
            }
        }
    }

    return warnings;
}


int ERC_TESTER::TestSameLocalGlobalLabel()
{
    using SCH_CONNECTIVITY::NAMED_ITEM;
    std::map<wxString, NAMED_ITEM> globalLabels;
    std::map<wxString, NAMED_ITEM> localLabels;

    for( const NAMED_ITEM& item : collectNamedItems( *m_schematic, false ) )
    {
        if( item.type == SCH_HIER_LABEL_T )
            continue;

        auto& labels = item.global ? globalLabels : localLabels;
        auto found = labels.find( item.name );

        // For one item, direct KIID_PATH comparison selects its shallowest instance
        if( found == labels.end()
            || item.item < found->second.item
            || ( item.item == found->second.item && item.sheet < found->second.sheet ) )
        {
            labels[item.name] = item;
        }
    }

    ERC_MARKER_PLACER markers( *m_schematic );

    for( const auto& [name, global] : globalLabels )
    {
        const auto local = localLabels.find( name );

        if( local == localLabels.end() )
            continue;

        const ERCE_T code = global.type == SCH_PIN_T && local->second.type == SCH_PIN_T ? ERCE_SAME_LOCAL_GLOBAL_POWER
                                                                                        : ERCE_SAME_LOCAL_GLOBAL_LABEL;
        reportNameConflict( markers, m_settings, code, global, local->second );
    }

    return markers.Count();
}


int ERC_TESTER::TestSimilarLabels()
{
    using SCH_CONNECTIVITY::NAMED_ITEM;
    std::map<wxString, std::vector<NAMED_ITEM>> generalMap;

    auto normalizeLabel =
            []( const wxString& aLabel )
            {
                wxString stripped = aLabel.Strip( wxString::leading ).Strip( wxString::trailing );
                wxString prefix;
                wxString value;
                wxString units;

                SplitString( stripped, &prefix, &value, &units );

                if( prefix == wxT( "+" ) )
                    prefix = wxEmptyString;

                /// Although the two 'μ's look the same, they are U+03BC and U+00B5
                units.Replace( wxS( "µ" ), 'u' );
                units.Replace( wxS( "μ" ), 'u' );

                return prefix.Lower() + value.Lower() + units.Lower();
            };

    for( const NAMED_ITEM& item : collectNamedItems( *m_schematic, true ) )
        generalMap[normalizeLabel( item.name )].push_back( item );

    ERC_MARKER_PLACER markers( *m_schematic );

    for( auto& [name, entries] : generalMap )
    {
        std::sort( entries.begin(), entries.end(), []( const NAMED_ITEM& a, const NAMED_ITEM& b )
        {
            // Saved exclusions record which item is main, so order by path text rather than depth
            if( a.sheet != b.sheet )
                return a.sheet.AsString() < b.sheet.AsString();

            return a.item < b.item;
        } );

        // Report each unordered pair whose shown text differs only in case only once.
        for( size_t ii = 0; ii < entries.size(); ++ii )
        {
            for( size_t jj = ii + 1; jj < entries.size(); ++jj )
            {
                const NAMED_ITEM& a = entries[ii];
                const NAMED_ITEM& b = entries[jj];

                if( a.name == b.name )
                    continue;

                // Similar local labels on different sheets are fine.
                if( a.type == SCH_LABEL_T && b.type == SCH_LABEL_T && a.sheet != b.sheet )
                    continue;

                ERCE_T code = ERCE_SIMILAR_LABELS;

                if( a.type == SCH_PIN_T && b.type == SCH_PIN_T )
                    code = ERCE_SIMILAR_POWER;
                else if( a.type == SCH_PIN_T || b.type == SCH_PIN_T )
                    code = ERCE_SIMILAR_LABEL_AND_POWER;

                reportNameConflict( markers, m_settings, code, a, b );
            }
        }
    }

    return markers.Count();
}


int ERC_TESTER::TestLibSymbolIssues()
{
    wxCHECK( m_schematic, 0 );

    LIBRARY_MANAGER&        manager = Pgm().GetLibraryManager();
    SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( &m_schematic->Project() );
    int                     err_count = 0;

    const bool captured = ADVANCED_CFG::GetCfg().m_ConnectivityEngine;

    struct LIBRARY_ISSUE
    {
        int      code;
        wxString message;
    };

    const auto check =
            [&]( const SCH_CONNECTIVITY::LIBRARY_SYMBOL_FACT& aSource, const SCH_SYMBOL* aSymbol )
            {
                std::vector<LIBRARY_ISSUE> issues;
                const wxString libName = aSource.library.GetLibNickname();
                std::optional<const LIBRARY_TABLE_ROW*> optRow = manager.GetRow( LIBRARY_TABLE_TYPE::SYMBOL, libName );

                if( !optRow || ( *optRow )->Disabled() )
                {
                    if( m_settings.IsTestEnabled( ERCE_LIB_SYMBOL_ISSUES ) )
                    {
                        issues.push_back( { ERCE_LIB_SYMBOL_ISSUES,
                                            wxString::Format( _( "The current configuration does not include the "
                                                                 "symbol library '%s'" ),
                                                              UnescapeString( libName ) )
                                          } );
                    }

                    return issues;
                }

                if( !adapter->IsLibraryLoaded( libName ) )
                {
                    if( m_settings.IsTestEnabled( ERCE_LIB_SYMBOL_ISSUES ) )
                    {
                        std::optional<wxString> uri = manager.GetFullURI( LIBRARY_TABLE_TYPE::SYMBOL, libName, true );
                        wxCHECK2( uri.has_value(), uri = wxEmptyString );
                        issues.push_back( { ERCE_LIB_SYMBOL_ISSUES,
                                            wxString::Format( _( "The symbol library '%s' was not found at '%s'" ),
                                                              UnescapeString( libName ), *uri )
                                          } );
                    }

                    return issues;
                }

                const wxString symbolName = aSource.library.GetLibItemName();
                LIB_SYMBOL*    libSymbol = adapter->LoadSymbol( aSource.library );

                if( !libSymbol )
                {
                    if( m_settings.IsTestEnabled( ERCE_LIB_SYMBOL_ISSUES ) )
                    {
                        issues.push_back( { ERCE_LIB_SYMBOL_ISSUES,
                                            wxString::Format( _( "Symbol '%s' not found in symbol library '%s'" ),
                                                              UnescapeString( symbolName ),
                                                              UnescapeString( libName ) )
                                          } );
                    }

                    return issues;
                }

                if( !m_settings.IsTestEnabled( ERCE_LIB_SYMBOL_MISMATCH ) )
                    return issues;

                std::unique_ptr<LIB_SYMBOL> flattenedSymbol = libSymbol->Flatten();
                const int                   flags = m_schematic->Settings().SymbolCompareFlags();
                bool                        mismatch = false;

                if( captured )
                {
                    if( aSource.attributes.duplicatePinNumbersAreJumpers || !aSource.HasDuplicatePins() )
                    {
                        mismatch = !SCH_CONNECTIVITY::ExtractLibrarySymbolFact( *flattenedSymbol )
                                            .Matches( aSource, flags );
                    }
                }
                else
                {
                    wxCHECK( aSymbol && aSymbol->GetLibSymbolRef(), issues );

                    LIB_SYMBOL*           embedded = aSymbol->GetLibSymbolRef().get();
                    std::vector<wxString> messages;

                    // Duplicate pins can be valid on the same net; TestDuplicatePinNets checks their nets
                    if( !embedded->GetDuplicatePinNumbersAreJumpers() )
                    {
                        UNITS_PROVIDER unitsProvider( schIUScale, EDA_UNITS::MILS );
                        CheckDuplicatePins( embedded, messages, &unitsProvider );
                    }

                    mismatch = messages.empty() && flattenedSymbol->Compare( *embedded, flags ) != 0;
                }

                if( mismatch )
                {
                    issues.push_back( { ERCE_LIB_SYMBOL_MISMATCH,
                                        wxString::Format( _( "Symbol '%s' doesn't match copy in library '%s'" ),
                                                          UnescapeString( symbolName ),
                                                          UnescapeString( libName ) )
                                      } );
                }

                return issues;
            };

    // Library results depend only on the shared screen, so every instance reuses one lookup
    std::map<std::pair<const SCH_SCREEN*, KIID>, std::vector<LIBRARY_ISSUE>> results;

    for( const SCH_SHEET_PATH& path : m_schematic->Hierarchy() )
    {
        SCH_SCREEN* screen = path.LastScreen();
        std::vector<SCH_CONNECTIVITY::LIBRARY_SYMBOL_FACT> sources;

        // Legacy sources keep their symbol because duplicate UUIDs make screen lookup ambiguous
        std::vector<const SCH_SYMBOL*> symbols;

        if( captured )
        {
            sources = m_schematic->Connectivity().Engine().LibrarySymbols( path.PathRef() );
        }
        else
        {
            for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
            {
                const SCH_SYMBOL*                      symbol = static_cast<SCH_SYMBOL*>( item );
                SCH_CONNECTIVITY::LIBRARY_SYMBOL_FACT& source = sources.emplace_back();

                source.id = symbol->m_Uuid;
                source.position = symbol->GetPosition();
                source.library = symbol->GetLibId();
                source.hasEmbeddedSymbol = bool( symbol->GetLibSymbolRef() );
                symbols.push_back( symbol );
            }
        }

        std::vector<SCH_MARKER*> markers;

        for( size_t ii = 0; ii < sources.size(); ++ii )
        {
            const auto& source = sources[ii];

            if( !source.hasEmbeddedSymbol )
                continue;

            auto result = results.find( { screen, source.id } );

            if( result == results.end() )
            {
                const SCH_SYMBOL* symbol = captured ? nullptr : symbols[ii];
                result = results.emplace( std::make_pair( screen, source.id ), check( source, symbol ) ).first;
            }

            for( const LIBRARY_ISSUE& issue : result->second )
            {
                std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( issue.code );
                ercItem->SetItems( std::vector<KIID>{ source.id } );
                ercItem->SetSheetSpecificPath( path );
                ercItem->SetItemsSheetPaths( path );
                ercItem->SetErrorMessage( issue.message );
                markers.emplace_back( new SCH_MARKER( std::move( ercItem ), source.position ) );
            }
        }

        for( SCH_MARKER* marker : markers )
        {
            screen->Append( marker );
            err_count += 1;
        }
    }

    return err_count;
}


int ERC_TESTER::TestFootprintLinkIssues( KIFACE* aCvPcb, PROJECT* aProject )
{
    wxCHECK( m_schematic, 0 );

    if( std::optional<LIBRARY_MANAGER_ADAPTER*> adapter =
                Pgm().GetLibraryManager().Adapter( LIBRARY_TABLE_TYPE::FOOTPRINT ) )
    {
        ( *adapter )->BlockUntilLoaded();
    }

    typedef int (*TESTER_FN_PTR)( const wxString&, PROJECT* );

    TESTER_FN_PTR linkTester = (TESTER_FN_PTR) aCvPcb->IfaceOrAddress( KIFACE_TEST_FOOTPRINT_LINK );
    ERC_MARKER_PLACER markers( *m_schematic );

    for( const auto& source : collectFootprints( *m_schematic ) )
    {
        if( source.footprint.IsEmpty() )
            continue;

        wxString msg;
        LIB_ID fpID;

        if( fpID.Parse( source.footprint, true ) >= 0 )
        {
            msg.Printf( _( "'%s' is not a valid footprint identifier" ), source.footprint );
        }
        else
        {
            const wxString libName = fpID.GetLibNickname();
            const wxString fpName = fpID.GetLibItemName();
            const int ret = linkTester( source.footprint, aProject );

            if( ret == KIFACE_TEST_FOOTPRINT_LINK_NO_LIBRARY )
            {
                msg.Printf( _( "The current configuration does not include the footprint library '%s'" ),
                            libName );
            }
            else if( ret == KIFACE_TEST_FOOTPRINT_LINK_LIBRARY_NOT_ENABLED )
            {
                msg.Printf( _( "The footprint library '%s' is not enabled in the current configuration" ),
                            libName );
            }
            else if( ret == KIFACE_TEST_FOOTPRINT_LINK_NO_FOOTPRINT )
            {
                msg.Printf( _( "Footprint '%s' not found in library '%s'" ), fpName, libName );
            }
            else
            {
                continue;
            }
        }

        markers.Report( ERCE_FOOTPRINT_LINK_ISSUES, { source.item }, source.sheet, source.position, ITEM_PATHS::MAIN,
                        msg );
    }

    return markers.Count();
}


int ERC_TESTER::TestFootprintFilters()
{
    wxCHECK( m_schematic, 0 );
    ERC_MARKER_PLACER markers( *m_schematic );

    for( const auto& source : collectFootprints( *m_schematic ) )
    {
        if( source.filters.empty() )
            continue;

        const wxString lowerId = source.footprint.Lower();
        LIB_ID footprint;

        // An id rejected at its first character is still checked and reported
        if( footprint.Parse( lowerId ) > 0 )
            continue;

        const wxString lowerItemName = footprint.GetUniStringLibItemName().Lower();
        const bool found = std::ranges::any_of( source.filters,
                            [&]( const wxString& filter )
                            {
                                // If the filter contains a ':' character, include the library name in the pattern
                                return filter.Contains( wxS( ":" ) ) ? lowerId.Matches( filter.Lower() )
                                                                     : lowerItemName.Matches( filter.Lower() );
                            } );

        if( found )
            continue;

        markers.Report( ERCE_FOOTPRINT_FILTERS, { source.item }, source.sheet, source.position, ITEM_PATHS::MAIN,
                        wxString::Format( _( "Assigned footprint (%s) doesn't match footprint filters (%s)" ),
                                          footprint.GetUniStringLibItemName(), wxJoin( source.filters, ' ' ) ) );
    }

    return markers.Count();
}


int ERC_TESTER::TestOffGridEndpoints()
{
    if( ADVANCED_CFG::GetCfg().m_ConnectivityEngine )
    {
        const int grid = m_schematic->Settings().m_ConnectionGridSize;
        return reportSourceErrors( *m_schematic, ERCE_ENDPOINT_OFF_GRID,
                                   m_schematic->Connectivity().Engine().OffGridEndpoints( grid ) );
    }

    const int gridSize = m_schematic->Settings().m_ConnectionGridSize;
    int       err_count = 0;

    for( SCH_SCREEN* screen = m_screens.GetFirst(); screen; screen = m_screens.GetNext() )
    {
        std::vector<SCH_MARKER*> markers;

        for( SCH_ITEM* item : screen->Items() )
        {
            if( item->Type() == SCH_LINE_T && item->IsConnectable() )
            {
                SCH_LINE* line = static_cast<SCH_LINE*>( item );

                if( ( line->GetStartPoint().x % gridSize ) != 0
                        || ( line->GetStartPoint().y % gridSize ) != 0 )
                {
                    std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_ENDPOINT_OFF_GRID );
                    ercItem->SetItems( line );

                    markers.emplace_back( new SCH_MARKER( std::move( ercItem ), line->GetStartPoint() ) );
                }
                else if( ( line->GetEndPoint().x % gridSize ) != 0
                            || ( line->GetEndPoint().y % gridSize ) != 0 )
                {
                    std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_ENDPOINT_OFF_GRID );
                    ercItem->SetItems( line );

                    markers.emplace_back( new SCH_MARKER( std::move( ercItem ), line->GetEndPoint() ) );
                }
            }
            if( item->Type() == SCH_BUS_WIRE_ENTRY_T )
            {
                SCH_BUS_WIRE_ENTRY* entry = static_cast<SCH_BUS_WIRE_ENTRY*>( item );

                for( const VECTOR2I& point : entry->GetConnectionPoints() )
                {
                    if( ( point.x % gridSize ) != 0
                        || ( point.y % gridSize ) != 0 )
                    {
                        std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_ENDPOINT_OFF_GRID );
                        ercItem->SetItems( entry );

                        markers.emplace_back( new SCH_MARKER( std::move( ercItem ), point ) );
                    }
                }
            }
            else if( item->Type() == SCH_SYMBOL_T )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

                for( SCH_PIN* pin : symbol->GetPins( nullptr ) )
                {
                    if( pin->GetType() == ELECTRICAL_PINTYPE::PT_NC )
                        continue;

                    VECTOR2I pinPos = pin->GetPosition();

                    if( ( pinPos.x % gridSize ) != 0 || ( pinPos.y % gridSize ) != 0 )
                    {
                        std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_ENDPOINT_OFF_GRID );
                        ercItem->SetItems( pin );

                        markers.emplace_back( new SCH_MARKER( std::move( ercItem ), pinPos ) );
                        break;
                    }
                }
            }
        }

        for( SCH_MARKER* marker : markers )
        {
            screen->Append( marker );
            err_count += 1;
        }
    }

    return err_count;
}


int ERC_TESTER::TestSimModelIssues()
{
    WX_STRING_REPORTER reporter;
    int                err_count = 0;
    SIM_LIB_MGR        libMgr( &m_schematic->Project() );
    wxString           variant = m_schematic->GetCurrentVariant();

    for( SCH_SHEET_PATH& sheet : m_sheetList )
    {
        const auto sources = ADVANCED_CFG::GetCfg().m_ConnectivityEngine
                                     ? m_schematic->Connectivity().SimulationModels( sheet.PathRef() )
                                     : SCH_CONNECTIVITY::ExtractSimulationModelFacts( sheet, variant );
        std::vector<SCH_MARKER*> markers;

        for( const auto& source : sources )
        {
            reporter.Clear();
            libMgr.CreateModel( source.input, true, reporter );

            if( reporter.HasMessage() )
            {
                wxString                  msg = reporter.GetMessages();
                std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_SIMULATION_MODEL );

                //Remove \n and \r at e.o.l if any:
                msg.Trim();

                ercItem->SetErrorMessage( msg );
                ercItem->SetItems( source.id );
                ercItem->SetSheetSpecificPath( sheet );
                ercItem->SetItemsSheetPaths( sheet );

                markers.emplace_back( new SCH_MARKER( std::move( ercItem ), source.position ) );
            }
        }

        for( SCH_MARKER* marker : markers )
        {
            sheet.LastScreen()->Append( marker );
            err_count += 1;
        }
    }

    return err_count;
}


int ERC_TESTER::TestVariantSymbols()
{
    wxCHECK( m_schematic, 0 );

    SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( &m_schematic->Project() );

    if( !adapter )
        return 0;

    int err_count = 0;

    // Flatten each alternate once per ERC run rather than once per referencing symbol.
    std::map<wxString, std::unique_ptr<LIB_SYMBOL>> flatAltCache;

    for( SCH_SHEET_PATH& sheet : m_sheetList )
    {
        SCH_SCREEN* screen = sheet.LastScreen();

        if( !screen )
            continue;

        std::vector<SCH_MARKER*> markers;
        std::vector<SCH_CONNECTIVITY::VARIANT_SYMBOL_FACT> sources;
        std::vector<SCH_CONNECTIVITY::LIBRARY_SYMBOL_FACT> libraries;

        if( ADVANCED_CFG::GetCfg().m_ConnectivityEngine )
        {
            sources = m_schematic->Connectivity().Engine().VariantSymbols( sheet.PathRef() );

            if( sources.empty() )
                continue;

            libraries = m_schematic->Connectivity().Engine().LibrarySymbols( sheet.PathRef() );
        }
        else
        {
            for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
            {
                const SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
                auto source = SCH_CONNECTIVITY::ExtractVariantSymbolFact( *symbol, sheet );

                if( !source )
                    continue;

                auto& base = libraries.emplace_back();

                if( symbol->GetLibSymbolRef() )
                    base = SCH_CONNECTIVITY::ExtractLibrarySymbolFact( *symbol->GetLibSymbolRef() );

                base.id = symbol->m_Uuid;
                base.position = symbol->GetPosition();
                base.library = symbol->GetLibId();
                sources.push_back( std::move( *source ) );
            }

            std::sort( libraries.begin(), libraries.end(),
                       []( const auto& a, const auto& b )
                       {
                           return a.id < b.id;
                       } );
        }

        for( const auto& source : sources )
        {
            const auto base = std::lower_bound( libraries.begin(), libraries.end(), source.id,
                                                []( const auto& a, const KIID& id )
                                                {
                                                    return a.id < id;
                                                } );

            if( base == libraries.end() || base->id != source.id )
                continue;

            auto addMarker =
                    [&]( int aErrorCode, const wxString& aMessage )
                    {
                        std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( aErrorCode );

                        ercItem->SetItems( std::vector<KIID>{ source.id } );
                        ercItem->SetSheetSpecificPath( sheet );
                        ercItem->SetItemsSheetPaths( sheet );
                        ercItem->SetErrorMessage( aMessage );

                        markers.emplace_back( new SCH_MARKER( std::move( ercItem ), base->position ) );
                        err_count++;
                    };

            for( const auto& [variantName, libId] : source.overrides )
            {
                wxString libIdStr = libId.Format();

                // A self-referencing override is ignored by resolution, so skip it here too.
                if( libId == base->library )
                    continue;

                auto cacheIt = flatAltCache.find( libIdStr );

                if( cacheIt == flatAltCache.end() )
                {
                    std::unique_ptr<LIB_SYMBOL> flatAlt;

                    try
                    {
                        if( LIB_SYMBOL* altSymbol = adapter->LoadSymbol( libId ) )
                            flatAlt = altSymbol->Flatten();
                    }
                    catch( const IO_ERROR& )
                    {
                    }

                    cacheIt = flatAltCache.emplace( libIdStr, std::move( flatAlt ) ).first;
                }

                const LIB_SYMBOL* flatAlt = cacheIt->second.get();

                if( !flatAlt )
                {
                    if( m_settings.IsTestEnabled( ERCE_VARIANT_SYMBOL_INVALID ) )
                    {
                        addMarker( ERCE_VARIANT_SYMBOL_INVALID,
                                   wxString::Format( _( "Variant '%s': symbol '%s' not found in libraries" ),
                                                     variantName, libIdStr ) );
                    }

                    continue;
                }

                if( !m_settings.IsTestEnabled( ERCE_VARIANT_SYMBOL_INCOMPATIBLE ) )
                    continue;

                if( !base->hasEmbeddedSymbol )
                    continue;

                std::vector<VARIANT_COMPAT_RESULT> issues =
                        ValidateVariantSymbolCompatibility( *base, *flatAlt );

                for( const VARIANT_COMPAT_RESULT& issue : issues )
                {
                    addMarker( ERCE_VARIANT_SYMBOL_INCOMPATIBLE,
                               wxString::Format( _( "Variant '%s', alternate '%s': %s" ),
                                                 variantName, libIdStr, issue.detail ) );
                }
            }
        }

        for( SCH_MARKER* marker : markers )
            screen->Append( marker );
    }

    return err_count;
}


void ERC_TESTER::RunTests( DS_PROXY_VIEW_ITEM* aDrawingSheet, SCH_EDIT_FRAME* aEditFrame,
                           KIFACE* aCvPcb, PROJECT* aProject, PROGRESS_REPORTER* aProgressReporter )
{
    if( !aEditFrame )
    {
        m_schematic->RecordERCExclusions();
        m_screens.DeleteAllMarkers( MARKER_BASE::MARKER_ERC, true );
    }

    m_sheetList.AnnotatePowerSymbols();

    if( aEditFrame )
    {
        if( ADVANCED_CFG::GetCfg().m_IncrementalConnectivity )
            aEditFrame->RecalculateConnections( nullptr, GLOBAL_CLEANUP );
        else
            aEditFrame->RecalculateConnections( nullptr, NO_CLEANUP );
    }
    else if( ADVANCED_CFG::GetCfg().m_ConnectivityEngine )
    {
        // Captured sources track revisions and text context, so an update refreshes job overrides
        m_schematic->Connectivity().Recalculate( *m_schematic );
    }
    else
    {
        m_schematic->RebuildConnectivity( nullptr, aProgressReporter );
    }

    if( ADVANCED_CFG::GetCfg().m_ConnectivityEngine
        && m_settings.IsTestEnabled( ERCE_SIMULATION_MODEL ) )
    {
        m_schematic->Connectivity().PrepareSimulationModels( *m_schematic );
    }

    if( ADVANCED_CFG::GetCfg().m_ConnectivityEngine
        && m_settings.IsTestEnabled( ERCE_UNRESOLVED_VARIABLE ) )
    {
        m_schematic->Connectivity().PrepareTextChecks( *m_schematic, aDrawingSheet != nullptr );
    }

    // Test duplicate sheet names inside a given sheet.  While one can have multiple references
    // to the same file, each must have a unique name.
    if( m_settings.IsTestEnabled( ERCE_DUPLICATE_SHEET_NAME ) )
    {
        if( aProgressReporter )
            aProgressReporter->AdvancePhase( _( "Checking sheet names..." ) );

        TestDuplicateSheetNames( true );
    }

    // Test pin-to-pad maps for stale pins, duplicate pad targets and bad pad references (issue #2282).
    if( m_settings.IsTestEnabled( ERCE_PIN_MAP_STALE_PIN ) || m_settings.IsTestEnabled( ERCE_PIN_MAP_DUPLICATE_PAD )
        || m_settings.IsTestEnabled( ERCE_PIN_MAP_BAD_PAD ) || m_settings.IsTestEnabled( ERCE_PIN_MAP_UNMAPPED_PIN ) )
    {
        if( aProgressReporter )
            aProgressReporter->AdvancePhase( _( "Checking pin maps..." ) );

        TestPinMap( aCvPcb, aProject );
    }

    if( aProgressReporter )
        aProgressReporter->AdvancePhase( _( "Checking conflicts..." ) );

    if( ADVANCED_CFG::GetCfg().m_ConnectivityEngine )
        TestConnectivity( *m_schematic );
    else
        m_schematic->ConnectionGraph()->RunERC();

    if( aProgressReporter )
        aProgressReporter->AdvancePhase( _( "Checking units..." ) );

    // Test is all units of each multiunit symbol have the same footprint assigned.
    if( m_settings.IsTestEnabled( ERCE_DIFFERENT_UNIT_FP ) )
    {
        if( aProgressReporter )
            aProgressReporter->AdvancePhase( _( "Checking footprints..." ) );

        TestMultiunitFootprints();
    }

    if( m_settings.IsTestEnabled( ERCE_MISSING_UNIT )
        || m_settings.IsTestEnabled( ERCE_MISSING_INPUT_PIN )
        || m_settings.IsTestEnabled( ERCE_MISSING_POWER_INPUT_PIN )
        || m_settings.IsTestEnabled( ERCE_MISSING_BIDI_PIN ) )
    {
        TestMissingUnits();
    }

    if( aProgressReporter )
        aProgressReporter->AdvancePhase( _( "Checking pins..." ) );

    if( m_settings.IsTestEnabled( ERCE_DIFFERENT_UNIT_NET ) )
        TestMultUnitPinConflicts();

    if( m_settings.IsTestEnabled( ERCE_DUPLICATE_PIN_ERROR ) )
        TestDuplicatePinNets();

    // Test pins on each net against the pin connection table
    if( m_settings.IsTestEnabled( ERCE_PIN_TO_PIN_ERROR )
        || m_settings.IsTestEnabled( ERCE_PIN_TO_PIN_WARNING )
        || m_settings.IsTestEnabled( ERCE_POWERPIN_NOT_DRIVEN )
        || m_settings.IsTestEnabled( ERCE_PIN_NOT_DRIVEN ) )
    {
        TestPinToPin();
    }

    if( m_settings.IsTestEnabled( ERCE_GROUND_PIN_NOT_GROUND ) )
        TestGroundPins();

    if( m_settings.IsTestEnabled( ERCE_STACKED_PIN_SYNTAX ) )
        TestStackedPinNotation();

    // Test similar labels (i;e. labels which are identical when
    // using case insensitive comparisons)
    if( m_settings.IsTestEnabled( ERCE_SIMILAR_LABELS )
        || m_settings.IsTestEnabled( ERCE_SIMILAR_POWER )
        || m_settings.IsTestEnabled( ERCE_SIMILAR_LABEL_AND_POWER ) )
    {
        if( aProgressReporter )
            aProgressReporter->AdvancePhase( _( "Checking similar labels..." ) );

        TestSimilarLabels();
    }

    if( m_settings.IsTestEnabled( ERCE_SAME_LOCAL_GLOBAL_LABEL )
        || m_settings.IsTestEnabled( ERCE_SAME_LOCAL_GLOBAL_POWER ) )
    {
        if( aProgressReporter )
            aProgressReporter->AdvancePhase( _( "Checking local and global labels..." ) );

        TestSameLocalGlobalLabel();
    }

    if( m_settings.IsTestEnabled( ERCE_UNRESOLVED_VARIABLE ) )
    {
        if( aProgressReporter )
            aProgressReporter->AdvancePhase( _( "Checking for unresolved variables..." ) );

        TestTextVars( aDrawingSheet );
    }

    if( m_settings.IsTestEnabled( ERCE_FIELD_NAME_WHITESPACE ) )
    {
        if( aProgressReporter )
            aProgressReporter->AdvancePhase( _( "Checking field names..." ) );

        TestFieldNameWhitespace();
    }

    if( m_settings.IsTestEnabled( ERCE_EMPTY_LABEL_NAME ) )
    {
        if( aProgressReporter )
            aProgressReporter->AdvancePhase( _( "Checking for empty label names..." ) );

        TestEmptyLabelNames();
    }

    if( m_settings.IsTestEnabled( ERCE_SIMULATION_MODEL ) )
    {
        if( aProgressReporter )
            aProgressReporter->AdvancePhase( _( "Checking SPICE models..." ) );

        TestSimModelIssues();
    }

    if( m_settings.IsTestEnabled( ERCE_NOCONNECT_CONNECTED ) )
    {
        if( aProgressReporter )
            aProgressReporter->AdvancePhase( _( "Checking no connect pins for connections..." ) );

        TestNoConnectPins();
    }

    if( m_settings.IsTestEnabled( ERCE_LIB_SYMBOL_ISSUES )
        || m_settings.IsTestEnabled( ERCE_LIB_SYMBOL_MISMATCH ) )
    {
        if( aProgressReporter )
            aProgressReporter->AdvancePhase( _( "Checking for library symbol issues..." ) );

        TestLibSymbolIssues();
    }

    if( m_settings.IsTestEnabled( ERCE_FOOTPRINT_LINK_ISSUES ) && aCvPcb )
    {
        if( aProgressReporter )
            aProgressReporter->AdvancePhase( _( "Checking for footprint link issues..." ) );

        TestFootprintLinkIssues( aCvPcb, aProject );
    }

    if( m_settings.IsTestEnabled( ERCE_FOOTPRINT_FILTERS ) )
    {
        if( aProgressReporter )
            aProgressReporter->AdvancePhase( _( "Checking footprint assignments against footprint filters..." ) );

        TestFootprintFilters();
    }

    if( m_settings.IsTestEnabled( ERCE_ENDPOINT_OFF_GRID ) )
    {
        if( aProgressReporter )
            aProgressReporter->AdvancePhase( _( "Checking for off grid pins and wires..." ) );

        TestOffGridEndpoints();
    }

    if( m_settings.IsTestEnabled( ERCE_FOUR_WAY_JUNCTION ) )
    {
        if( aProgressReporter )
            aProgressReporter->AdvancePhase( _( "Checking for four way junctions..." ) );

        TestFourWayJunction();
    }

    if( m_settings.IsTestEnabled( ERCE_LABEL_MULTIPLE_WIRES ) )
    {
        if( aProgressReporter )
            aProgressReporter->AdvancePhase( _( "Checking for labels on more than one wire..." ) );

        TestLabelMultipleWires();
    }

    if( m_settings.IsTestEnabled( ERCE_UNDEFINED_NETCLASS ) )
    {
        if( aProgressReporter )
            aProgressReporter->AdvancePhase( _( "Checking for undefined netclasses..." ) );

        TestMissingNetclasses();
    }

    if( m_settings.IsTestEnabled( ERCE_VARIANT_SYMBOL_INVALID )
        || m_settings.IsTestEnabled( ERCE_VARIANT_SYMBOL_INCOMPATIBLE ) )
    {
        if( aProgressReporter )
            aProgressReporter->AdvancePhase( _( "Checking variant symbols..." ) );

        TestVariantSymbols();
    }

    m_schematic->ResolveERCExclusionsPostUpdate();
}
