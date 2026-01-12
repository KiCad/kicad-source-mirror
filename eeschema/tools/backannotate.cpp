/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Alexander Shuklin <Jasuramme@gmail.com>
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


#include <backannotate.h>
#include <boost/property_tree/ptree.hpp>
#include <confirm.h>
#include <common.h>
#include <dsnlexer.h>
#include <ptree.h>
#include <reporter.h>
#include <sch_edit_frame.h>
#include <sch_sheet_path.h>
#include <sch_label.h>
#include <lib_symbol.h>
#include <schematic.h>
#include <sch_commit.h>
#include <string_utils.h>
#include <kiface_base.h>
#include <wildcards_and_files_ext.h>
#include <connection_graph.h>
#include <limits>
#include <set>
#include <tool/tool_manager.h>
#include <tools/sch_line_wire_bus_tool.h>
#include <tools/sch_selection.h>
#include <tools/sch_tool_utils.h>
#include <wx/log.h>
#include <fmt.h>
#include <fmt/ranges.h>
#include <fmt/xchar.h>


BACK_ANNOTATE::BACK_ANNOTATE( SCH_EDIT_FRAME* aFrame, REPORTER& aReporter, bool aRelinkFootprints,
                              bool aProcessFootprints, bool aProcessValues, bool aProcessReferences,
                              bool aProcessNetNames, bool aProcessAttributes, bool aProcessOtherFields,
                              bool aPreferUnitSwaps, bool aPreferPinSwaps, bool aDryRun ) :
        m_reporter( aReporter ),
        m_matchByReference( aRelinkFootprints ),
        m_processFootprints( aProcessFootprints ),
        m_processValues( aProcessValues ),
        m_processReferences( aProcessReferences ),
        m_processNetNames( aProcessNetNames ),
        m_processAttributes( aProcessAttributes ),
        m_processOtherFields( aProcessOtherFields ),
        m_preferUnitSwaps( aPreferUnitSwaps ),
        m_preferPinSwaps( aPreferPinSwaps ),
        m_dryRun( aDryRun ),
        m_frame( aFrame ),
        m_changesCount( 0 )
{ }


bool BACK_ANNOTATE::BackAnnotateSymbols( const std::string& aNetlist )
{
    m_changesCount = 0;

    if( !m_matchByReference && !m_processValues && !m_processFootprints && !m_processReferences
        && !m_processNetNames && !m_processAttributes && !m_processOtherFields )
    {
        m_reporter.ReportTail( _( "Select at least one property to back annotate." ), RPT_SEVERITY_ERROR );
        return false;
    }

    getPcbModulesFromString( aNetlist );

    SCH_SHEET_LIST sheets = m_frame->Schematic().Hierarchy();
    sheets.GetSymbols( m_refs, false );
    sheets.GetMultiUnitSymbols( m_multiUnitsRefs );

    getChangeList();
    checkForUnusedSymbols();

    applyChangelist();
    return true;
}


bool BACK_ANNOTATE::FetchNetlistFromPCB( std::string& aNetlist )
{
    if( Kiface().IsSingle() )
    {
        DisplayErrorMessage( m_frame, _( "Cannot fetch PCB netlist because Schematic Editor is opened in "
                                         "stand-alone mode.\n"
                                         "You must launch the KiCad project manager and create a project." ) );
        return false;
    }

    KIWAY_PLAYER* frame = m_frame->Kiway().Player( FRAME_PCB_EDITOR, false );

    if( !frame )
    {
        wxFileName fn( m_frame->Prj().GetProjectFullName() );
        fn.SetExt( FILEEXT::PcbFileExtension );

        frame = m_frame->Kiway().Player( FRAME_PCB_EDITOR, true );
        frame->OpenProjectFiles( std::vector<wxString>( 1, fn.GetFullPath() ) );
    }

    m_frame->Kiway().ExpressMail( FRAME_PCB_EDITOR, MAIL_PCB_GET_NETLIST, aNetlist );
    return true;
}


void BACK_ANNOTATE::PushNewLinksToPCB()
{
    std::string nullPayload;

    m_frame->Kiway().ExpressMail( FRAME_PCB_EDITOR, MAIL_PCB_UPDATE_LINKS, nullPayload );
}


void BACK_ANNOTATE::getPcbModulesFromString( const std::string& aPayload )
{
    auto getStr = []( const PTREE& pt ) -> wxString
                  {
                      return UTF8( pt.front().first );
                  };

    DSNLEXER lexer( aPayload, From_UTF8( __func__ ) );
    PTREE    doc;

    // NOTE: KiCad's PTREE scanner constructs a property *name* tree, not a property tree.
    // Every token in the s-expr is stored as a property name; the property's value is then
    // either the nested s-exprs or an empty PTREE; there are *no* literal property values.

    Scan( &doc, &lexer );

    PTREE&   tree = doc.get_child( "pcb_netlist" );
    wxString msg;
    m_pcbFootprints.clear();

    for( const std::pair<const std::string, PTREE>& item : tree )
    {
        wxString path, value, footprint;
        bool     dnp = false, exBOM = false;
        std::map<wxString, wxString> pinNetMap, fieldsMap;
        wxASSERT( item.first == "ref" );
        wxString ref = getStr( item.second );

        try
        {
            if( m_matchByReference )
                path = ref;
            else
                path = getStr( item.second.get_child( "timestamp" ) );

            if( path == "" )
            {
                msg.Printf( _( "Footprint '%s' has no assigned symbol." ), DescribeRef( ref ) );
                m_reporter.ReportHead( msg, RPT_SEVERITY_WARNING );
                continue;
            }

            footprint = getStr( item.second.get_child( "fpid" ) );
            value     = getStr( item.second.get_child( "value" ) );

            // Get child PTREE of fields
            boost::optional<const PTREE&> fields = item.second.get_child_optional( "fields" );

            // Parse each field out of the fields string
            if( fields )
            {
                for( const std::pair<const std::string, PTREE>& field : fields.get() )
                {
                    if( field.first != "field" )
                        continue;

                    // Fields are of the format "(field (name "name") "12345")
                    const auto&        fieldName = field.second.get_child_optional( "name" );
                    const std::string& fieldValue = field.second.back().first;

                    if( !fieldName )
                        continue;

                    fieldsMap[getStr( fieldName.get() )] = wxString::FromUTF8( fieldValue );
                }
            }


            // Get DNP and Exclude from BOM out of the properties if they exist
            for( const auto& child : item.second )
            {
                if( child.first != "property" )
                    continue;

                auto property = child.second;
                auto name = property.get_child_optional( "name" );

                if( !name )
                    continue;

                if( name.get().front().first == "dnp" )
                    dnp = true;
                else if( name.get().front().first == "exclude_from_bom" )
                    exBOM = true;
            }

            boost::optional<const PTREE&> nets = item.second.get_child_optional( "nets" );

            if( nets )
            {
                for( const std::pair<const std::string, PTREE>& pin_net : nets.get() )
                {
                    wxASSERT( pin_net.first == "pin_net" );
                    wxString pinNumber = UTF8( pin_net.second.front().first );
                    wxString netName = UTF8( pin_net.second.back().first );
                    pinNetMap[ pinNumber ] = netName;
                }
            }
        }
        catch( ... )
        {
            wxLogWarning( "Cannot parse PCB netlist for back-annotation." );
        }

        // Use lower_bound for not to iterate over map twice
        auto nearestItem = m_pcbFootprints.lower_bound( path );

        if( nearestItem != m_pcbFootprints.end() && nearestItem->first == path )
        {
            // Module with this path already exists - generate error
            msg.Printf( _( "Footprints '%s' and '%s' linked to same symbol." ),
                        DescribeRef( nearestItem->second->m_ref ),
                        DescribeRef( ref ) );
            m_reporter.ReportHead( msg, RPT_SEVERITY_ERROR );
        }
        else
        {
            // Add footprint to the map
            std::shared_ptr<PCB_FP_DATA> data = std::make_shared<PCB_FP_DATA>( ref, footprint, value, dnp,
                                                                               exBOM, pinNetMap, fieldsMap );
            m_pcbFootprints.insert( nearestItem, std::make_pair( path, data ) );
        }
    }
}


void BACK_ANNOTATE::getChangeList()
{
    for( const auto& [pcbPath, pcbData] : m_pcbFootprints )
    {
        int  refIndex;
        bool foundInMultiunit = false;

        for( const auto& [_, refList] : m_multiUnitsRefs )
        {
            if( m_matchByReference )
                refIndex = refList.FindRef( pcbPath );
            else
                refIndex = refList.FindRefByFullPath( pcbPath );

            if( refIndex >= 0 )
            {
                // If footprint linked to multi unit symbol, we add all symbol's units to
                // the change list
                foundInMultiunit = true;

                for( size_t i = 0; i < refList.GetCount(); ++i )
                {
                    refList[ i ].GetSymbol()->ClearFlags(SKIP_STRUCT );
                    m_changelist.emplace_back( CHANGELIST_ITEM( refList[i], pcbData ) );
                }

                break;
            }
        }

        if( foundInMultiunit )
            continue;

        if( m_matchByReference )
            refIndex = m_refs.FindRef( pcbPath );
        else
            refIndex = m_refs.FindRefByFullPath( pcbPath );

        if( refIndex >= 0 )
        {
            m_refs[ refIndex ].GetSymbol()->ClearFlags( SKIP_STRUCT );
            m_changelist.emplace_back( CHANGELIST_ITEM( m_refs[refIndex], pcbData ) );
        }
        else
        {
            // Haven't found linked symbol in multiunits or common refs. Generate error
            m_reporter.ReportTail( wxString::Format( _( "Cannot find symbol for footprint '%s'." ),
                                                     DescribeRef( pcbData->m_ref ) ),
                                   RPT_SEVERITY_ERROR );
        }
    }
}

void BACK_ANNOTATE::checkForUnusedSymbols()
{
    m_refs.SortByTimeStamp();

    std::sort( m_changelist.begin(), m_changelist.end(),
               []( const CHANGELIST_ITEM& a, const CHANGELIST_ITEM& b )
               {
                   return SCH_REFERENCE_LIST::sortByTimeStamp( a.first, b.first );
               } );

    size_t i = 0;

    for( const std::pair<SCH_REFERENCE, std::shared_ptr<PCB_FP_DATA>>& item : m_changelist )
    {
        // Refs and changelist are both sorted by paths, so we just go over m_refs and
        // generate errors before we will find m_refs member to which item linked
        while( i < m_refs.GetCount() && m_refs[i].GetPath() != item.first.GetPath() )
        {
            const SCH_REFERENCE& ref = m_refs[i];

            if( ref.GetSymbol()->GetExcludedFromBoard() )
            {
                m_reporter.ReportTail( wxString::Format( _( "Footprint '%s' is not present on PCB. "
                                                            "Corresponding symbols in schematic must be "
                                                            "manually deleted (if desired)." ),
                                                         DescribeRef( m_refs[i].GetRef() ) ),
                                       RPT_SEVERITY_WARNING );
            }

            ++i;
        }

        ++i;
    }

    if( m_matchByReference && !m_frame->ReadyToNetlist( _( "Re-linking footprints requires a fully "
                                                           "annotated schematic." ) ) )
    {
        m_reporter.ReportTail( _( "Footprint re-linking canceled by user." ), RPT_SEVERITY_ERROR );
    }
}


void BACK_ANNOTATE::applyChangelist()
{
    SCH_COMMIT commit( m_frame );
    wxString   msg;

    std::set<CHANGELIST_ITEM*> unitSwapItems;

    // First, optionally handle unit swaps across multi-unit symbols where possible
    // This needs to happen before the rest of the normal changelist processing,
    // because the swaps will modify the changelist
    if( m_processNetNames && m_preferUnitSwaps )
    {
        REPORTER& debugReporter = NULL_REPORTER::GetInstance(); // Change to m_reporter for debugging

        // Group changelist items by shared PCB footprint data pointer
        std::map<std::shared_ptr<PCB_FP_DATA>, std::vector<CHANGELIST_ITEM*>> changesPerFp;

        msg.Printf( wxT( "DEBUG(unit-swap): grouped changelist into %zu footprint(s) for unit-swap processing:" ),
                    changesPerFp.size() );

        // The changelist will have multiple entries for multi-unit symbols, each unit ref, e.g. U1A, U1B
        // will point to the same PCB_FP_DATA. Grouping by pointer allows us to handle all units of a symbol
        // together by working on all changes to the same PCB footprint at once.
        for( CHANGELIST_ITEM& item : m_changelist )
        {
            changesPerFp[item.second].push_back( &item );
            msg += wxT( " " ) + item.first.GetRef();
        }

        debugReporter.ReportHead( msg, RPT_SEVERITY_INFO );

        // Handle all changes per footprint
        for( auto& fpChangelistPair : changesPerFp )
        {
            std::set<SCH_SYMBOL*>        swappedSymbols;
            std::set<CHANGELIST_ITEM*>   swappedItems;
            std::shared_ptr<PCB_FP_DATA> fp = fpChangelistPair.first;
            auto&                        changedFpItems = fpChangelistPair.second;

            // Build symbol unit list for this footprint (multi-unit candidates)
            // Snapshot of one schematic unit instance (ref + sheet path + nets) for matching
            struct SYM_UNIT
            {
                SCH_SYMBOL*           sym = nullptr;
                SCH_SCREEN*           screen = nullptr;
                const SCH_SHEET_PATH* sheetPath = nullptr;
                wxString              ref;
                int                   currentUnit = 0;
                // Track these so we avoid re-applying label/field changes to swapped units
                CHANGELIST_ITEM*             changeItem = nullptr;
                std::map<wxString, wxString> schNetsByPin;   // pinNumber -> net (schematic)
                std::map<wxString, wxString> pcbNetsByPin;   // pinNumber -> net (from PCB pin map)
                std::vector<wxString>        unitPinNumbers; // library-defined pin numbers per unit
                std::vector<wxString>        schNetsInUnitOrder;
                std::vector<wxString>        pcbNetsInUnitOrder;
            };

            // All symbol units for this footprint
            std::vector<SYM_UNIT> symbolUnits;

            std::map<LIB_SYMBOL*, std::vector<LIB_SYMBOL::UNIT_PIN_INFO>> unitPinsByLibSymbol;

            auto getUnitPins =
                [&]( SCH_SYMBOL* symbol, int unitNumber ) -> std::vector<wxString>
                {
                    if( unitNumber <= 0 )
                        return {};

                    if( !symbol )
                        return {};

                    LIB_SYMBOL* libSymbol = symbol->GetLibSymbolRef().get();

                    if( !libSymbol )
                        return {};

                    auto found = unitPinsByLibSymbol.find( libSymbol );

                    if( found == unitPinsByLibSymbol.end() )
                        found = unitPinsByLibSymbol.emplace( libSymbol, libSymbol->GetUnitPinInfo() ).first;

                    const std::vector<LIB_SYMBOL::UNIT_PIN_INFO>& unitInfos = found->second;

                    if( unitNumber > static_cast<int>( unitInfos.size() ) )
                        return {};

                    return unitInfos[unitNumber - 1].m_pinNumbers;
                };

            // Compare nets in the deterministic library order so diode arrays and similar map cleanly
            auto netsInUnitOrder =
                   []( const std::vector<wxString>& pins,
                       const std::map<wxString, wxString>& netByPin )
                   {
                       std::vector<wxString> nets;

                       if( !pins.empty() )
                       {
                           nets.reserve( pins.size() );

                           for( const wxString& pinNum : pins )
                           {
                               auto it = netByPin.find( pinNum );
                               nets.push_back( it != netByPin.end() ? it->second : wxString() );
                           }
                       }
                       else
                       {
                           nets.reserve( netByPin.size() );

                           for( const std::pair<const wxString, wxString>& kv : netByPin )
                               nets.push_back( kv.second );
                       }

                       return nets;
                   };

            for( CHANGELIST_ITEM* changedItem : changedFpItems )
            {
                SCH_REFERENCE& ref    = changedItem->first;
                SCH_SYMBOL*    symbol = ref.GetSymbol();
                SCH_SCREEN*    screen = ref.GetSheetPath().LastScreen();

                if( !symbol )
                    continue;

                // Collect nets keyed by pin number. Ordering by XY is intentionally avoided
                // here; see comment in SYM_UNIT above.
                SYM_UNIT symbolUnit;
                symbolUnit.sym = symbol;
                symbolUnit.screen = screen;
                symbolUnit.ref = symbol->GetRef( &ref.GetSheetPath(), true );
                symbolUnit.sheetPath = &ref.GetSheetPath();
                symbolUnit.changeItem = changedItem;

                int currentUnit = ref.GetUnit();

                if( currentUnit <= 0 )
                    currentUnit = symbol->GetUnitSelection( &ref.GetSheetPath() );

                if( currentUnit <= 0 )
                    currentUnit = symbol->GetUnit();

                symbolUnit.currentUnit = currentUnit;
                symbolUnit.unitPinNumbers = getUnitPins( symbol, symbolUnit.currentUnit );

                const SCH_SHEET_PATH& sheetPath = ref.GetSheetPath();

                for( SCH_PIN* pin : symbol->GetPins( &ref.GetSheetPath() ) )
                {
                    const wxString& pinNum = pin->GetNumber();

                    // PCB nets from footprint pin map
                    auto it = fp->m_pinMap.find( pinNum );
                    symbolUnit.pcbNetsByPin[pinNum] = ( it != fp->m_pinMap.end() ) ? it->second : wxString();

                    // Schematic nets from connections
                    if( SCH_PIN* p = symbol->GetPin( pinNum ) )
                    {
                        if( SCH_CONNECTION* connection = p->Connection( &sheetPath ) )
                            symbolUnit.schNetsByPin[pinNum] = connection->Name( true );
                        else
                            symbolUnit.schNetsByPin[pinNum] = wxString();
                    }
                    else
                        symbolUnit.schNetsByPin[pinNum] = wxString();
                }

                symbolUnit.pcbNetsInUnitOrder = netsInUnitOrder( symbolUnit.unitPinNumbers, symbolUnit.pcbNetsByPin );
                symbolUnit.schNetsInUnitOrder = netsInUnitOrder( symbolUnit.unitPinNumbers, symbolUnit.schNetsByPin );

                symbolUnits.push_back( symbolUnit );
            }

            auto vectorToString =
                    []( const std::vector<wxString>& values ) -> wxString
                    {
                        return fmt::format( L"{}", fmt::join( values, L", " ) );
                    };

            auto mapToString =
                    [vectorToString]( const std::map<wxString, wxString>& pinMap ) -> wxString
                    {
                        std::vector<wxString> entries;

                        for( const std::pair<const wxString, wxString>& pin : pinMap )
                            entries.push_back( pin.first + '=' + pin.second );

                        return vectorToString( entries );
                    };

            msg.Printf( wxT( "DEBUG(unit-swap): footprint %s processed (%zu units, dryRun=%d)." ), fp->m_ref,
                        symbolUnits.size(), m_dryRun ? 1 : 0 );
            debugReporter.ReportHead( msg, RPT_SEVERITY_INFO );

            // For debugging, sort the symbol units by ref
            std::sort( symbolUnits.begin(), symbolUnits.end(),
                       []( const SYM_UNIT& a, const SYM_UNIT& b )
                       {
                           return a.ref < b.ref;
                       } );

            // Store addresses so identical SCH_SYMBOLs on different sheet instances stay unique
            std::vector<SYM_UNIT*> symbolUnitPtrs;

            for( SYM_UNIT& su : symbolUnits )
                symbolUnitPtrs.push_back( &su );

            for( const SYM_UNIT& su : symbolUnits )
            {
                wxString pcbPins = mapToString( su.pcbNetsByPin );
                wxString schPins = mapToString( su.schNetsByPin );
                wxString unitPins = vectorToString( su.unitPinNumbers );
                wxString pcbUnitNetSeq = vectorToString( su.pcbNetsInUnitOrder );
                wxString schUnitNetSeq = vectorToString( su.schNetsInUnitOrder );

                msg.Printf( wxT( "DEBUG(unit-swap):   unit %d: %s pcbPins[%s] schPins[%s] unitPins[%s] pcbUnitNets[%s] "
                                 "schUnitNets[%s]." ),
                            su.currentUnit, su.ref, pcbPins, schPins, unitPins, pcbUnitNetSeq, schUnitNetSeq );
                debugReporter.ReportHead( msg, RPT_SEVERITY_INFO );
            }

            if( symbolUnits.size() < 2 )
                continue;

            // For each symbol unit, find the target unit whose schematic nets match this unit's
            // PCB nets when treated as a multiset of net names.  This allows units with different
            // pin numbering (e.g. diode arrays) to be matched while still requiring every net in
            // the unit to move as a group.
            // desiredTarget maps each PCB unit to the schematic unit whose nets it matches
            std::map<SYM_UNIT*, SYM_UNIT*> desiredTarget;
            std::set<SYM_UNIT*>            usedTargets;
            bool                           mappingOk = true;

            for( SYM_UNIT* symbolUnit : symbolUnitPtrs )
            {
                SYM_UNIT* match = nullptr;

                for( SYM_UNIT* potentialMatch : symbolUnitPtrs )
                {
                    if( usedTargets.count( potentialMatch ) )
                        continue;

                    if( symbolUnit->pcbNetsInUnitOrder == potentialMatch->schNetsInUnitOrder )
                    {
                        match = potentialMatch;
                        break;
                    }
                }

                if( !match )
                {
                    wxString pcbPins = mapToString( symbolUnit->pcbNetsByPin );
                    wxString schPins = mapToString( symbolUnit->schNetsByPin );
                    wxString unitPins = vectorToString( symbolUnit->unitPinNumbers );
                    wxString pcbUnitNetSeq = vectorToString( symbolUnit->pcbNetsInUnitOrder );
                    wxString schUnitNetSeq = vectorToString( symbolUnit->schNetsInUnitOrder );

                    msg.Printf( wxT( "DEBUG(unit-swap): no schematic match for %s (%s) pcbPins[%s] schPins[%s] "
                                     "unitPins[%s] pcbUnitNets[%s] schUnitNets[%s]." ),
                                symbolUnit->ref, fp->m_ref, pcbPins, schPins, unitPins, pcbUnitNetSeq, schUnitNetSeq );
                    debugReporter.ReportHead( msg, RPT_SEVERITY_INFO );
                    mappingOk = false;
                    break;
                }

                msg.Printf( wxT( "DEBUG(unit-swap): %s matches %s for footprint %s." ), symbolUnit->ref, match->ref,
                            fp->m_ref );
                debugReporter.ReportHead( msg, RPT_SEVERITY_INFO );
                desiredTarget[symbolUnit] = match;
                usedTargets.insert( match );
            }

            if( !mappingOk )
            {
                msg.Printf( wxT( "DEBUG(unit-swap): mapping failed for footprint %s." ), fp->m_ref );
                debugReporter.ReportHead( msg, RPT_SEVERITY_INFO );
                continue;
            }

            // Check if mapping is identity (no swap needed)
            bool isIdentity = true;

            for( SYM_UNIT* su : symbolUnitPtrs )
            {
                auto it = desiredTarget.find( su );

                if( it == desiredTarget.end() || it->second != su )
                {
                    isIdentity = false;
                    break;
                }
            }

            if( isIdentity )
            {
                msg.Printf( wxT( "DEBUG(unit-swap): footprint %s already aligned (identity mapping)." ), fp->m_ref );
                debugReporter.ReportHead( msg, RPT_SEVERITY_INFO );
                continue;
            }

            // Decompose into cycles over the symbols and perform swaps along cycles
            std::set<SYM_UNIT*> visited;

            for( SYM_UNIT* symbolUnit : symbolUnitPtrs )
            {
                SYM_UNIT* start = symbolUnit;

                if( visited.count( start ) )
                    continue;

                // Build cycle starting at 'start'
                std::vector<SYM_UNIT*> cycle;
                SYM_UNIT*              cur = start;

                while( !visited.count( cur ) )
                {
                    visited.insert( cur );
                    cycle.push_back( cur );

                    auto nextIt = desiredTarget.find( cur );

                    if( nextIt == desiredTarget.end() )
                        break;

                    SYM_UNIT* nextSym = nextIt->second;

                    if( !nextSym || nextSym == cur )
                        break;

                    cur = nextSym;
                }

                if( cycle.size() < 2 )
                {
                    msg.Printf( wxT( "DEBUG(unit-swap): cycle length %zu for footprint %s starting at %s." ),
                                cycle.size(), fp->m_ref, start->ref );
                    debugReporter.ReportHead( msg, RPT_SEVERITY_INFO );
                    continue;
                }

                // Apply swaps along the cycle from end to start,
                // modify all symbol units in the cycle
                if( !m_dryRun )
                {
                    for( SYM_UNIT* s : cycle )
                        commit.Modify( s->sym, s->screen );
                }

                for( size_t i = cycle.size() - 1; i > 0; --i )
                {
                    SYM_UNIT* a = cycle[i - 1];
                    SYM_UNIT* b = cycle[i];
                    int       aUnit = a->currentUnit;
                    int       bUnit = b->currentUnit;

                    // Swap unit numbers between a and b
                    if( !m_dryRun )
                    {
                        a->sym->SetUnit( bUnit );
                        b->sym->SetUnit( aUnit );

                        if( const SCH_SHEET_PATH* sheet = a->sheetPath )
                            a->sym->SetUnitSelection( sheet, bUnit );

                        if( const SCH_SHEET_PATH* sheet = b->sheetPath )
                            b->sym->SetUnitSelection( sheet, aUnit );
                    }

                    a->currentUnit = bUnit;
                    b->currentUnit = aUnit;

                    swappedSymbols.insert( a->sym );
                    swappedSymbols.insert( b->sym );

                    // Track which changelist entries participated so later net-label updates skip them
                    if( a->changeItem )
                    {
                        swappedItems.insert( a->changeItem );
                        unitSwapItems.insert( a->changeItem );
                    }

                    if( b->changeItem )
                    {
                        swappedItems.insert( b->changeItem );
                        unitSwapItems.insert( b->changeItem );
                    }

                    wxString baseRef = a->sym->GetRef( a->sheetPath, false );
                    wxString unitAString = a->sym->SubReference( aUnit, false );
                    wxString unitBString = b->sym->SubReference( bUnit, false );

                    if( unitAString.IsEmpty() )
                        unitAString.Printf( wxT( "%d" ), aUnit );

                    if( unitBString.IsEmpty() )
                        unitBString.Printf( wxT( "%d" ), bUnit );

                    msg.Printf( _( "Swap %s unit %s with unit %s." ),
                                DescribeRef( baseRef ),
                                unitAString,
                                unitBString );
                    m_reporter.ReportHead( msg, RPT_SEVERITY_ACTION );
                    ++m_changesCount;
                }

                msg.Printf( wxT( "DEBUG(unit-swap): applied %zu swap steps for footprint %s." ), cycle.size() - 1,
                            fp->m_ref );
                debugReporter.ReportHead( msg, RPT_SEVERITY_INFO );
            }

            // Remove label updates for swapped symbols by marking their SKIP_STRUCT flag
            if( !m_dryRun )
            {
                for( CHANGELIST_ITEM* changedItem : changedFpItems )
                {
                    SCH_SYMBOL* symbol = changedItem->first.GetSymbol();

                    if( !symbol )
                        continue;

                    if( swappedItems.count( changedItem ) || swappedSymbols.count( symbol ) )
                        symbol->SetFlags( SKIP_STRUCT );

                    int updatedUnit = symbol->GetUnitSelection( &changedItem->first.GetSheetPath() );
                    changedItem->first.SetUnit( updatedUnit );
                }
            }
        }
    }

    // Apply changes from change list
    for( CHANGELIST_ITEM& item : m_changelist )
    {
        SCH_REFERENCE& ref = item.first;
        PCB_FP_DATA&   fpData = *item.second;
        SCH_SYMBOL*    symbol = ref.GetSymbol();
        SCH_SCREEN*    screen = ref.GetSheetPath().LastScreen();
        wxString       oldFootprint = ref.GetFootprint();
        wxString       oldValue = ref.GetValue();
        bool           oldDNP = ref.GetSymbol()->GetDNP();
        bool           oldExBOM = ref.GetSymbol()->GetExcludedFromBOM();
        // Skip prevents us from re-applying label/field changes to units we just swapped
        bool skip = ( ref.GetSymbol()->GetFlags() & SKIP_STRUCT ) > 0 || unitSwapItems.count( &item ) > 0;

        auto boolString =
                []( bool b ) -> wxString
                {
                    return b ? _( "true" ) : _( "false" );
                };

        if( !m_dryRun )
            commit.Modify( symbol, screen, RECURSE_MODE::NO_RECURSE );

        if( m_processReferences && ref.GetRef() != fpData.m_ref && !skip
                && !symbol->GetField( FIELD_T::REFERENCE )->HasTextVars() )
        {
            ++m_changesCount;
            msg.Printf( _( "Change %s reference designator to '%s'." ),
                        DescribeRef( ref.GetRef() ),
                        fpData.m_ref );

            if( !m_dryRun )
                symbol->SetRef( &ref.GetSheetPath(), fpData.m_ref );

            m_reporter.ReportHead( msg, RPT_SEVERITY_ACTION );
        }

        if( m_processFootprints && oldFootprint != fpData.m_footprint && !skip
                && !symbol->GetField( FIELD_T::FOOTPRINT )->HasTextVars() )
        {
            ++m_changesCount;
            msg.Printf( _( "Change %s footprint assignment from '%s' to '%s'." ),
                        DescribeRef( ref.GetRef() ),
                        EscapeHTML( oldFootprint ),
                        EscapeHTML( fpData.m_footprint ) );

            if( !m_dryRun )
                symbol->SetFootprintFieldText( fpData.m_footprint );

            m_reporter.ReportHead( msg, RPT_SEVERITY_ACTION );
        }

        if( m_processValues && oldValue != fpData.m_value && !skip
                && !symbol->GetField( FIELD_T::VALUE )->HasTextVars() )
        {
            ++m_changesCount;
            msg.Printf( _( "Change %s value from '%s' to '%s'." ),
                        DescribeRef( ref.GetRef() ),
                        EscapeHTML( oldValue ),
                        EscapeHTML( fpData.m_value ) );

            if( !m_dryRun )
                symbol->SetValueFieldText( fpData.m_value );

            m_reporter.ReportHead( msg, RPT_SEVERITY_ACTION );
        }

        if( m_processAttributes && oldDNP != fpData.m_DNP && !skip )
        {
            ++m_changesCount;
            msg.Printf( _( "Change %s 'Do not populate' from '%s' to '%s'." ),
                        DescribeRef( ref.GetRef() ),
                        boolString( oldDNP ),
                        boolString( fpData.m_DNP ) );

            if( !m_dryRun )
                symbol->SetDNP( fpData.m_DNP );

            m_reporter.ReportHead( msg, RPT_SEVERITY_ACTION );
        }

        if( m_processAttributes && oldExBOM != fpData.m_excludeFromBOM && !skip )
        {
            ++m_changesCount;
            msg.Printf( _( "Change %s 'Exclude from bill of materials' from '%s' to '%s'." ),
                        DescribeRef( ref.GetRef() ),
                        boolString( oldExBOM ),
                        boolString( fpData.m_excludeFromBOM ) );

            if( !m_dryRun )
                symbol->SetExcludedFromBOM( fpData.m_excludeFromBOM );

            m_reporter.ReportHead( msg, RPT_SEVERITY_ACTION );
        }

        std::set<wxString> swappedPins;

        // Try to satisfy footprint pad net swaps by moving symbol pins before falling back to labels.
        if( m_preferPinSwaps && m_processNetNames && !skip )
            swappedPins = applyPinSwaps( symbol, ref, fpData, &commit );

        if( m_processNetNames && !skip )
        {
            for( const std::pair<const wxString, wxString>& entry : fpData.m_pinMap )
            {
                const wxString& pinNumber = entry.first;
                const wxString& shortNetName = entry.second;
                SCH_PIN*        pin = symbol->GetPin( pinNumber );

                // Skip pins that the user preferred to handle with pin swaps in applyPreferredPinSwaps()
                if( swappedPins.count( pinNumber ) > 0 )
                    continue;

                if( !pin )
                {
                    msg.Printf( _( "Cannot find %s pin '%s'." ),
                                DescribeRef( ref.GetRef() ),
                                EscapeHTML( pinNumber ) );
                    m_reporter.ReportHead( msg, RPT_SEVERITY_ERROR );

                    continue;
                }

                SCH_CONNECTION* connection = pin->Connection( &ref.GetSheetPath() );

                if( connection && connection->Name( true ) != shortNetName )
                {
                    processNetNameChange( &commit, ref.GetRef(), pin, connection,
                                          connection->Name( true ), shortNetName );
                }
            }
        }

        if( m_processOtherFields )
        {
            // Need to handle three cases: existing field, new field, deleted field
            for( const std::pair<const wxString, wxString>& field : fpData.m_fieldsMap )
            {
                const wxString& fpFieldName = field.first;
                const wxString& fpFieldValue = field.second;
                SCH_FIELD*      symField = symbol->GetField( fpFieldName );

                // Skip fields that are individually controlled
                if( fpFieldName == GetCanonicalFieldName( FIELD_T::REFERENCE )
                    || fpFieldName == GetCanonicalFieldName( FIELD_T::VALUE ) )
                {
                    continue;
                }

                // 1. Existing fields has changed value
                // PCB Field value is checked against the shown text because this is the value
                // with all the variables resolved. The footprints field value gets the symbol's
                // resolved value when the PCB is updated from the schematic.
                if( symField
                    && !symField->HasTextVars()
                    && symField->GetShownText( &ref.GetSheetPath(), false ) != fpFieldValue )
                {
                    m_changesCount++;
                    msg.Printf( _( "Change %s field '%s' value to '%s'." ),
                                DescribeRef( ref.GetRef() ),
                                EscapeHTML( symField->GetCanonicalName() ),
                                EscapeHTML( fpFieldValue ) );

                    if( !m_dryRun )
                        symField->SetText( fpFieldValue );

                    m_reporter.ReportHead( msg, RPT_SEVERITY_ACTION );
                }

                // 2. New field has been added to footprint and needs to be added to symbol
                if( symField == nullptr )
                {
                    m_changesCount++;
                    msg.Printf( _( "Add %s field '%s' with value '%s'." ),
                                DescribeRef( ref.GetRef() ),
                                EscapeHTML( fpFieldName ),
                                EscapeHTML( fpFieldValue ) );

                    if( !m_dryRun )
                    {
                        SCH_FIELD newField( symbol, FIELD_T::USER, fpFieldName );
                        newField.SetText( fpFieldValue );
                        newField.SetTextPos( symbol->GetPosition() );
                        newField.SetVisible( false ); // Don't clutter up the schematic
                        symbol->AddField( newField );
                    }

                    m_reporter.ReportHead( msg, RPT_SEVERITY_ACTION );
                }
            }

            // 3. Existing field has been deleted from footprint and needs to be deleted from symbol
            // Check all symbol fields for existence in the footprint field map
            for( SCH_FIELD& field : symbol->GetFields() )
            {
                // Never delete mandatory fields
                if( field.IsMandatory() )
                    continue;

                if( fpData.m_fieldsMap.find( field.GetCanonicalName() ) == fpData.m_fieldsMap.end() )
                {
                    // Field not found in footprint field map, delete it
                    m_changesCount++;
                    msg.Printf( _( "Delete %s field '%s.'" ),
                                DescribeRef( ref.GetRef() ),
                                EscapeHTML( field.GetName() ) );

                    if( !m_dryRun )
                        symbol->RemoveField( symbol->GetField( field.GetName() ) );

                    m_reporter.ReportHead( msg, RPT_SEVERITY_ACTION );
                }
            }
        }

        if( symbol->GetFlags() & SKIP_STRUCT )
            symbol->ClearFlags( SKIP_STRUCT );

        unitSwapItems.erase( &item );

        // TODO: back-annotate netclass changes?
    }

    if( !m_dryRun )
    {
        m_frame->RecalculateConnections( &commit, NO_CLEANUP );
        m_frame->UpdateNetHighlightStatus();

        commit.Push( _( "Update Schematic from PCB" ) );
    }
}


static SPIN_STYLE orientLabel( SCH_PIN* aPin )
{
    SPIN_STYLE spin = SPIN_STYLE::RIGHT;

    // Initial orientation from the pin
    switch( aPin->GetLibPin()->GetOrientation() )
    {
    default:
    case PIN_ORIENTATION::PIN_RIGHT: spin = SPIN_STYLE::LEFT;   break;
    case PIN_ORIENTATION::PIN_UP:    spin = SPIN_STYLE::BOTTOM; break;
    case PIN_ORIENTATION::PIN_DOWN:  spin = SPIN_STYLE::UP;     break;
    case PIN_ORIENTATION::PIN_LEFT:  spin = SPIN_STYLE::RIGHT;  break;
    }

    // Reorient based on the actual symbol orientation now
    struct ORIENT
    {
        int flag;
        int n_rots;
        int mirror_x;
        int mirror_y;
    }
    orientations[] =
    {
        { SYM_ORIENT_0,                  0, 0, 0 },
        { SYM_ORIENT_90,                 1, 0, 0 },
        { SYM_ORIENT_180,                2, 0, 0 },
        { SYM_ORIENT_270,                3, 0, 0 },
        { SYM_MIRROR_X + SYM_ORIENT_0,   0, 1, 0 },
        { SYM_MIRROR_X + SYM_ORIENT_90,  1, 1, 0 },
        { SYM_MIRROR_Y,                  0, 0, 1 },
        { SYM_MIRROR_X + SYM_ORIENT_270, 3, 1, 0 },
        { SYM_MIRROR_Y + SYM_ORIENT_0,   0, 0, 1 },
        { SYM_MIRROR_Y + SYM_ORIENT_90,  1, 0, 1 },
        { SYM_MIRROR_Y + SYM_ORIENT_180, 2, 0, 1 },
        { SYM_MIRROR_Y + SYM_ORIENT_270, 3, 0, 1 }
    };

    ORIENT o = orientations[ 0 ];

    const SCH_SYMBOL* parentSymbol = static_cast<const SCH_SYMBOL*>( aPin->GetParentSymbol() );

    if( !parentSymbol )
        return spin;

    int symbolOrientation = parentSymbol->GetOrientation();

    for( const ORIENT& i : orientations )
    {
        if( i.flag == symbolOrientation )
        {
            o = i;
            break;
        }
    }

    for( int i = 0; i < o.n_rots; i++ )
        spin = spin.RotateCCW();

    if( o.mirror_x )
        spin = spin.MirrorX();

    if( o.mirror_y )
        spin = spin.MirrorY();

    return spin;
}


void addConnections( SCH_ITEM* aItem, const SCH_SHEET_PATH& aSheetPath, std::set<SCH_ITEM*>& connectedItems )
{
    if( connectedItems.insert( aItem ).second )
    {
        for( SCH_ITEM* connectedItem : aItem->ConnectedItems( aSheetPath ) )
            addConnections( connectedItem, aSheetPath, connectedItems );
    }
}


std::set<wxString> BACK_ANNOTATE::applyPinSwaps( SCH_SYMBOL* aSymbol, const SCH_REFERENCE& aReference,
                                                 const PCB_FP_DATA& aFpData, SCH_COMMIT* aCommit )
{
    // Tracks pin numbers that we end up swapping so that the caller can skip any
    // duplicate label-only handling for those pins.
    std::set<wxString> swappedPins;

    if( !aSymbol )
        return swappedPins;

    SCH_SCREEN* screen = aReference.GetSheetPath().LastScreen();

    if( !screen )
        return swappedPins;

    wxCHECK( m_frame, swappedPins );

    // Used to build the list of schematic pins whose current net assignment does not match the PCB.
    struct PIN_CHANGE
    {
        SCH_PIN* pin;
        wxString pinNumber;
        wxString currentNet;
        wxString targetNet;
    };

    std::vector<PIN_CHANGE> mismatches;

    // Helper map that lets us find all pins currently on a given net.
    std::map<wxString, std::vector<size_t>> pinsByCurrentNet;

    // Build the mismatch list by inspecting each footprint pin that differs from the schematic.
    for( const std::pair<const wxString, wxString>& entry : aFpData.m_pinMap )
    {
        const wxString& pinNumber = entry.first;
        const wxString& desiredNet = entry.second;

        SCH_PIN* pin = aSymbol->GetPin( pinNumber );

        // Ignore power pins and anything marked as non-connectable. Power pins can map to
        // hidden/global references, e.g. implicit power connections on logic symbols.
        // KiCad pins are currently always connectable, but the extra guard keeps the
        // logic robust if alternate pin types (e.g. explicit mechanical/NC pins)
        // ever start reporting false.
        if( !pin || pin->IsPower() || !pin->IsConnectable() )
            continue;

        SCH_CONNECTION* connection = pin->Connection( &aReference.GetSheetPath() );
        wxString        currentNet = connection ? connection->Name( true ) : wxString();

        if( desiredNet.IsEmpty() || currentNet.IsEmpty() )
            continue;

        if( desiredNet == currentNet )
            continue;

        size_t idx = mismatches.size();
        mismatches.push_back( { pin, pinNumber, currentNet, desiredNet } );
        pinsByCurrentNet[currentNet].push_back( idx );
    }

    if( mismatches.size() < 2 )
        return swappedPins;

    // Track which mismatch entries we have already consumed, and the underlying pin objects
    // we still need to clean up wiring around once the geometry swap is done.
    std::vector<bool>     handled( mismatches.size(), false );
    std::vector<SCH_PIN*> swappedPinObjects;
    bool                  swappedLibPins = false;
    wxString              msg;

    bool allowPinSwaps = false;
    wxString currentProjectName = m_frame->Prj().GetProjectName();

    if( m_frame->eeconfig() )
        allowPinSwaps = m_frame->eeconfig()->m_Input.allow_unconstrained_pin_swaps;

    std::set<wxString> sharedSheetPaths;
    std::set<wxString> sharedProjectNames;
    bool               sharedSheetSymbol = SymbolHasSheetInstances( *aSymbol, currentProjectName,
                                                                    &sharedSheetPaths, &sharedProjectNames );

    std::set<wxString> friendlySheetNames;

    if( sharedSheetSymbol && !sharedSheetPaths.empty() )
        friendlySheetNames = GetSheetNamesFromPaths( sharedSheetPaths, m_frame->Schematic() );

    // Check each mismatch and try to find a partner whose desired net matches our current net
    // (i.e. the two pins have been swapped on the PCB).
    for( size_t i = 0; i < mismatches.size(); ++i )
    {
        // Skip entries already handled by a previous successful swap.
        if( handled[i] )
            continue;

        PIN_CHANGE& change = mismatches[i];

        // Find candidate pins whose current net equals the net we want to move this pin to.
        auto range = pinsByCurrentNet.find( change.targetNet );

        // Nobody currently on the desired net, so there’s no reciprocal swap to apply.
        if( range == pinsByCurrentNet.end() )
            continue;

        // Track the best partner index in case we discover a reciprocal mismatch below.
        size_t partnerIdx = std::numeric_limits<size_t>::max();

        // Examine every pin that presently lives on the net we want to move to.
        for( size_t candidateIdx : range->second )
        {
            if( candidateIdx == i || handled[candidateIdx] )
                continue;

            PIN_CHANGE& candidate = mismatches[candidateIdx];

            // Potential swap partner must want to move to our current net, i.e. the PCB swapped the
            // two nets between these pins.
            if( candidate.targetNet == change.currentNet )
            {
                partnerIdx = candidateIdx;
                break;
            }
        }

        // No viable partner found; either there was no reciprocal net mismatch or it was already
        // consumed. Leave this entry for label-based handling.
        if( partnerIdx == std::numeric_limits<size_t>::max() )
            continue;

        PIN_CHANGE& partner = mismatches[partnerIdx];

        // Sanity check: both pins must belong to the same schematic symbol before we swap geometry;
        // this prevents us from moving pin outlines between different units of a multi-unit symbol.
        if( change.pin->GetParentSymbol() != partner.pin->GetParentSymbol() )
            continue;

        if( !allowPinSwaps || sharedSheetSymbol )
        {
            if( !sharedProjectNames.empty() )
            {
                std::vector<wxString> otherProjects;

                for( const wxString& name : sharedProjectNames )
                {
                    if( !currentProjectName.IsEmpty() && name.IsSameAs( currentProjectName ) )
                        continue;

                    otherProjects.push_back( name );
                }

                wxString projects = AccumulateDescriptions( otherProjects );

                if( projects.IsEmpty() )
                {
                    msg.Printf( _( "Would swap %s pins %s and %s to match PCB, but the symbol is shared across other projects." ),
                                DescribeRef( aReference.GetRef() ),
                                EscapeHTML( change.pin->GetShownNumber() ),
                                EscapeHTML( partner.pin->GetShownNumber() ) );
                }
                else
                {
                    msg.Printf( _( "Would swap %s pins %s and %s to match PCB, but the symbol is shared across other "
                                   "projects (%s)." ),
                                aReference.GetRef(), EscapeHTML( change.pin->GetShownNumber() ),
                                EscapeHTML( partner.pin->GetShownNumber() ), projects );
                }
            }
            else if( !friendlySheetNames.empty() )
            {
                wxString sheets = AccumulateDescriptions( friendlySheetNames );

                msg.Printf( _( "Would swap %s pins %s and %s to match PCB, but the symbol is used by multiple sheet "
                               "instances (%s)." ),
                            DescribeRef( aReference.GetRef() ),
                            EscapeHTML( change.pin->GetShownNumber() ),
                            EscapeHTML( partner.pin->GetShownNumber() ), sheets );
            }
            else if( sharedSheetSymbol )
            {
                msg.Printf( _( "Would swap %s pins %s and %s to match PCB, but the symbol is shared." ),
                            DescribeRef( aReference.GetRef() ),
                            EscapeHTML( change.pin->GetShownNumber() ),
                            EscapeHTML( partner.pin->GetShownNumber() ) );
            }
            else
            {
                msg.Printf( _( "Would swap %s pins %s and %s to match PCB, but unconstrained pin swaps are disabled in "
                               "the schematic preferences." ),
                            DescribeRef( aReference.GetRef() ),
                            EscapeHTML( change.pin->GetShownNumber() ),
                            EscapeHTML( partner.pin->GetShownNumber() ) );
            }
            m_reporter.ReportHead( msg, RPT_SEVERITY_INFO );

            handled[i] = true;
            handled[partnerIdx] = true;
            continue;
        }

        if( !m_dryRun )
        {
            wxCHECK2( aCommit, continue );

            // Record the two pins in the commit and physically swap their local geometry.
            aCommit->Modify( change.pin, screen, RECURSE_MODE::RECURSE );
            aCommit->Modify( partner.pin, screen, RECURSE_MODE::RECURSE );

            swappedLibPins |= SwapPinGeometry( change.pin, partner.pin );
        }

        // Don’t pick either entry again for another pairing.
        handled[i] = true;
        handled[partnerIdx] = true;

        // Remember which pin numbers we touched so the caller can suppress duplicate work.
        swappedPins.insert( change.pinNumber );
        swappedPins.insert( partner.pinNumber );
        swappedPinObjects.push_back( change.pin );
        swappedPinObjects.push_back( partner.pin );

        ++m_changesCount;

        msg.Printf( _( "Swap %s pins %s and %s to match PCB." ),
                    DescribeRef( aReference.GetRef() ),
                    EscapeHTML( change.pin->GetShownNumber() ),
                    EscapeHTML( partner.pin->GetShownNumber() ) );
        m_reporter.ReportHead( msg, RPT_SEVERITY_ACTION );
    }

    // Nothing left to do if we didn't find a valid swap pair when we were in dry-run mode.
    if( swappedPinObjects.empty() )
        return swappedPins;

    if( !m_dryRun )
    {
        if( swappedLibPins )
            aSymbol->UpdatePins();

        m_frame->UpdateItem( aSymbol, false, true );

        if( TOOL_MANAGER* toolMgr = m_frame->GetToolManager() )
        {
            if( SCH_LINE_WIRE_BUS_TOOL* lwbTool = toolMgr->GetTool<SCH_LINE_WIRE_BUS_TOOL>() )
            {
                SCH_SELECTION cleanupSelection( screen );

                // Make sure we tidy up any wires connected to the pins whose geometry just moved.
                for( SCH_PIN* swappedPin : swappedPinObjects )
                    cleanupSelection.Add( swappedPin );

                lwbTool->TrimOverLappingWires( aCommit, &cleanupSelection );
                lwbTool->AddJunctionsIfNeeded( aCommit, &cleanupSelection );
            }
        }

        m_frame->Schematic().CleanUp( aCommit );
    }

    return swappedPins;
}


void BACK_ANNOTATE::processNetNameChange( SCH_COMMIT* aCommit, const wxString& aRef, SCH_PIN* aPin,
                                          const SCH_CONNECTION* aConnection,
                                          const wxString& aOldName, const wxString& aNewName )
{
    wxString msg;

    // Find a physically-connected driver.  We can't use the SCH_CONNECTION's m_driver because
    // it has already been resolved by merging subgraphs with the same label, etc., and our
    // name change may cause that resolution to change.

    std::set<SCH_ITEM*>           connectedItems;
    SCH_ITEM*                     driver = nullptr;
    CONNECTION_SUBGRAPH::PRIORITY driverPriority = CONNECTION_SUBGRAPH::PRIORITY::NONE;

    addConnections( aPin, aConnection->Sheet(), connectedItems );

    for( SCH_ITEM* item : connectedItems )
    {
        CONNECTION_SUBGRAPH::PRIORITY priority = CONNECTION_SUBGRAPH::GetDriverPriority( item );

        if( priority > driverPriority )
        {
            driver = item;
            driverPriority = priority;
        }
    }

    switch( driver->Type() )
    {
    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIER_LABEL_T:
    case SCH_SHEET_PIN_T:
        ++m_changesCount;

        msg.Printf( _( "Change %s pin %s net label from '%s' to '%s'." ),
                    DescribeRef( aRef ),
                    EscapeHTML( aPin->GetShownNumber() ),
                    EscapeHTML( aOldName ),
                    EscapeHTML( aNewName ) );

        if( !m_dryRun )
        {
            aCommit->Modify( driver, aConnection->Sheet().LastScreen() );
            static_cast<SCH_LABEL_BASE*>( driver )->SetText( aNewName );
        }

        m_reporter.ReportHead( msg, RPT_SEVERITY_ACTION );
        break;

    case SCH_PIN_T:
        if( static_cast<SCH_PIN*>( driver )->IsPower() )
        {
            msg.Printf( _( "Net %s cannot be changed to %s because it is driven by a power pin." ),
                        EscapeHTML( aOldName ),
                        EscapeHTML( aNewName ) );

            m_reporter.ReportHead( msg, RPT_SEVERITY_ERROR );
            break;
        }

        ++m_changesCount;
        msg.Printf( _( "Add label '%s' to %s pin %s net." ),
                    EscapeHTML( aNewName ),
                    DescribeRef( aRef ),
                    EscapeHTML( aPin->GetShownNumber() ) );

        if( !m_dryRun )
        {
            SCHEMATIC_SETTINGS& settings = m_frame->Schematic().Settings();
            SCH_LABEL* label = new SCH_LABEL( driver->GetPosition(), aNewName );
            label->SetParent( &m_frame->Schematic() );
            label->SetTextSize( VECTOR2I( settings.m_DefaultTextSize, settings.m_DefaultTextSize ) );
            label->SetSpinStyle( orientLabel( static_cast<SCH_PIN*>( driver ) ) );
            label->SetFlags( IS_NEW );

            SCH_SCREEN* screen = aConnection->Sheet().LastScreen();
            aCommit->Add( label, screen );
        }

        m_reporter.ReportHead( msg, RPT_SEVERITY_ACTION );
        break;

    default:
        break;
    }
}
