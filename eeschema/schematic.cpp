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
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <advanced_config.h>
#include <algorithm>
#include <common.h>
#include <set>
#include <bus_alias.h>
#include <commit.h>
#include <connection_graph.h>
#include <core/ignore.h>
#include <core/kicad_algo.h>
#include <core/profile.h>
#include <sch_collectors.h>
#include <erc/erc_settings.h>
#include <font/outline_font.h>
#include <netlist_exporter_spice.h>
#include <progress_reporter.h>
#include <project.h>
#include <project/net_settings.h>
#include <project/project_file.h>
#include <refdes_tracker.h>
#include <schematic.h>
#include <sch_bus_entry.h>
#include <sch_commit.h>
#include <sch_junction.h>
#include <sch_label.h>
#include <sch_line.h>
#include <sch_marker.h>
#include <sch_no_connect.h>
#include <sch_rule_area.h>
#include <sch_screen.h>
#include <sch_sheet_pin.h>
#include <sch_selection_tool.h>
#include <sim/spice_settings.h>
#include <sim/spice_value.h>
#include <trace_helpers.h>
#include <string_utils.h>
#include <text_eval/text_eval_wrapper.h>
#include <tool/tool_manager.h>
#include <undo_redo_container.h>
#include <local_history.h>
#include <sch_io/sch_io_mgr.h>
#include <sch_io/sch_io.h>

#include <wx/log.h>

bool SCHEMATIC::m_IsSchematicExists = false;

SCHEMATIC::SCHEMATIC( PROJECT* aPrj ) :
        EDA_ITEM( nullptr, SCHEMATIC_T ),
        m_project( nullptr ),
        m_rootSheet( nullptr ),
        m_schematicHolder( nullptr )
{
    m_currentSheet = new SCH_SHEET_PATH();
    m_connectionGraph = new CONNECTION_GRAPH( this );
    m_IsSchematicExists = true;

    SetProject( aPrj );

    PROPERTY_MANAGER::Instance().RegisterListener(
            TYPE_HASH( SCH_FIELD ),
            [&]( INSPECTABLE* aItem, PROPERTY_BASE* aProperty, COMMIT* aCommit )
            {
                // Special case: propagate value, footprint, and datasheet fields to other units
                // of a given symbol if they aren't in the selection

                SCH_FIELD* field = dynamic_cast<SCH_FIELD*>( aItem );

                if( !field || !IsValid() )
                    return;

                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( field->GetParent() );

                if( !symbol || aProperty->Name() != _HKI( "Text" ) )
                    return;

                // TODO(JE) This will need to get smarter to enable API access
                SCH_SHEET_PATH sheetPath = CurrentSheet();

                wxString newValue = aItem->Get<wxString>( aProperty );

                if( field->GetId() == FIELD_T::REFERENCE )
                {
                    symbol->SetRef( &sheetPath, newValue );

                    // The user might want to change all the units to the new ref.  Or they
                    // might not.  Since we have no way of knowing, we default to the most
                    // concrete action (change only the selected reference).
                    return;
                }

                wxString ref = symbol->GetRef( &sheetPath );
                int      unit = symbol->GetUnit();
                LIB_ID   libId = symbol->GetLibId();

                for( SCH_SHEET_PATH& sheet : Hierarchy() )
                {
                    std::vector<SCH_SYMBOL*> otherUnits;

                    CollectOtherUnits( ref, unit, libId, sheet, &otherUnits );

                    for( SCH_SYMBOL* otherUnit : otherUnits )
                    {
                        switch( field->GetId() )
                        {
                        case FIELD_T::VALUE:
                        case FIELD_T::FOOTPRINT:
                        case FIELD_T::DATASHEET:
                        {
                            if( aCommit )
                                aCommit->Modify( otherUnit, sheet.LastScreen() );

                            otherUnit->GetField( field->GetId() )->SetText( newValue );
                            break;
                        }

                        default: break;
                        }
                    }
                }
            } );

    Reset();
}


SCHEMATIC::~SCHEMATIC()
{
    PROPERTY_MANAGER::Instance().UnregisterListeners( TYPE_HASH( SCH_FIELD ) );

    delete m_currentSheet;
    delete m_connectionGraph;

    m_IsSchematicExists = false;
}


void SCHEMATIC::Reset()
{
    delete m_rootSheet;

    m_rootSheet = nullptr;
    m_topLevelSheets.clear();
    m_hierarchy.clear();

    m_connectionGraph->Reset();
    m_currentSheet->clear();

    m_busAliases.clear();

    ensureVirtualRoot();
    ensureDefaultTopLevelSheet();
}


void SCHEMATIC::SetProject( PROJECT* aPrj )
{
    if( m_project )
    {
        PROJECT_FILE& project = m_project->GetProjectFile();

        // d'tor will save settings to file
        delete project.m_ErcSettings;
        project.m_ErcSettings = nullptr;

        // d'tor will save settings to file
        delete project.m_SchematicSettings;
        project.m_SchematicSettings = nullptr;
    }

    m_project = aPrj;

    if( m_project )
    {
        PROJECT_FILE& project = m_project->GetProjectFile();
        project.m_ErcSettings = new ERC_SETTINGS( &project, "erc" );
        project.m_SchematicSettings = new SCHEMATIC_SETTINGS( &project, "schematic" );

        project.m_SchematicSettings->LoadFromFile();
        project.m_SchematicSettings->m_NgspiceSettings->LoadFromFile();
        project.m_ErcSettings->LoadFromFile();

        loadBusAliasesFromProject();
    }
}


void SCHEMATIC::CacheExistingAnnotation()
{
    wxASSERT( m_project );

    // Cache all existing annotations in the REFDES_TRACKER
    std::shared_ptr<REFDES_TRACKER> refdesTracker = m_project->GetProjectFile().m_SchematicSettings->m_refDesTracker;

    SCH_SHEET_LIST     sheets = Hierarchy();
    SCH_REFERENCE_LIST references;

    sheets.GetSymbols( references );

    for( const SCH_REFERENCE& ref : references )
    {
        refdesTracker->Insert( ref.GetFullRef( false ).ToStdString() );
    }
}


bool SCHEMATIC::Contains( const SCH_REFERENCE& aRef ) const
{
    SCH_SHEET_LIST     sheets = Hierarchy();
    SCH_REFERENCE_LIST references;

    /// TODO(snh): This is horribly inefficient, we should be using refdesTracker for this.
    /// REFDES_TRACKER will need to be extended to track if a reference is currently present in the schematic
    /// as well as the units.  For now, this is relatively fast for reasonably sized schematics
    /// Famous last words...
    sheets.GetSymbols( references );

    return std::any_of( references.begin(), references.end(),
                        [&]( const SCH_REFERENCE& ref )
                        {
                            return ref.GetFullRef( true ) == aRef.GetFullRef( true );
                        } );
}


void SCHEMATIC::ensureVirtualRoot()
{
    if( m_rootSheet && m_rootSheet->m_Uuid == niluuid )
    {
        if( !m_rootSheet->GetScreen() )
            m_rootSheet->SetScreen( new SCH_SCREEN( this ) );

        return;
    }

    SCH_SHEET* previousRoot = m_rootSheet;

    m_rootSheet = new SCH_SHEET( this );
    const_cast<KIID&>( m_rootSheet->m_Uuid ) = niluuid;
    m_rootSheet->SetScreen( new SCH_SCREEN( this ) );

    if( previousRoot )
    {
        previousRoot->SetParent( m_rootSheet );

        if( m_rootSheet->GetScreen() )
            m_rootSheet->GetScreen()->Append( previousRoot );

        m_topLevelSheets.clear();
        m_topLevelSheets.push_back( previousRoot );
    }
}


void SCHEMATIC::ensureDefaultTopLevelSheet()
{
    // Early exit if we're already in the process of setting top-level sheets to avoid recursion
    if( m_settingTopLevelSheets )
        return;

    ensureVirtualRoot();

    if( !m_topLevelSheets.empty() )
        return;

    SCH_SHEET*  rootSheet = new SCH_SHEET( this );
    SCH_SCREEN* rootScreen = new SCH_SCREEN( this );

    const_cast<KIID&>( rootSheet->m_Uuid ) = rootScreen->GetUuid();
    rootSheet->SetScreen( rootScreen );

    SetTopLevelSheets( { rootSheet } );

    SCH_SHEET_PATH rootSheetPath;
    rootSheetPath.push_back( m_rootSheet );
    rootSheetPath.push_back( rootSheet );
    rootSheetPath.SetPageNumber( wxT( "1" ) );
}


void SCHEMATIC::ensureCurrentSheetIsTopLevel()
{
    if( m_topLevelSheets.empty() )
        return;

    if( m_currentSheet->empty() || !IsTopLevelSheet( m_currentSheet->at( 0 ) ) )
    {
        m_currentSheet->clear();
        m_currentSheet->push_back( m_topLevelSheets[0] );
    }
}


void SCHEMATIC::rebuildHierarchyState( bool aResetConnectionGraph )
{
    RefreshHierarchy();

    if( aResetConnectionGraph && m_project )
        m_connectionGraph->Reset();

    m_variantNames.clear();

    if( m_rootSheet && m_rootSheet->GetScreen() )
    {
        SCH_SCREENS        screens( m_rootSheet );
        std::set<wxString> variantNames = screens.GetVariantNames();
        m_variantNames.insert( variantNames.begin(), variantNames.end() );
    }
}


void SCHEMATIC::SetTopLevelSheets( const std::vector<SCH_SHEET*>& aSheets )
{
    wxCHECK_RET( !aSheets.empty(), wxS( "Cannot set empty top-level sheets!" ) );

    // Set the recursion guard early before any calls to ensureDefaultTopLevelSheet()
    bool wasAlreadySetting = m_settingTopLevelSheets;
    m_settingTopLevelSheets = true;

    std::vector<SCH_SHEET*> validSheets;
    validSheets.reserve( aSheets.size() );

    for( SCH_SHEET* sheet : aSheets )
    {
        // Skip null sheets and virtual roots (which have niluuid)
        if( sheet && sheet->m_Uuid != niluuid )
            validSheets.push_back( sheet );
    }

    if( validSheets.empty() )
    {
        // Guard against re-entry to prevent infinite recursion
        if( !wasAlreadySetting )
            ensureDefaultTopLevelSheet();

        m_settingTopLevelSheets = wasAlreadySetting;
        return;
    }

    ensureVirtualRoot();

    std::set<SCH_SHEET*> desiredSheets( validSheets.begin(), validSheets.end() );

    if( m_rootSheet->GetScreen() )
    {
        for( SCH_ITEM* item : m_rootSheet->GetScreen()->Items() )
        {
            SCH_SHEET* sheet = dynamic_cast<SCH_SHEET*>( item );

            if( sheet && !desiredSheets.contains( sheet ) )
                delete sheet;
        }

        m_rootSheet->GetScreen()->Clear( false );
    }

    m_currentSheet->clear();
    m_topLevelSheets.clear();

    for( SCH_SHEET* sheet : validSheets )
    {
        sheet->SetParent( m_rootSheet );

        if( m_rootSheet->GetScreen() )
            m_rootSheet->GetScreen()->Append( sheet );

        m_topLevelSheets.push_back( sheet );
    }

    ensureCurrentSheetIsTopLevel();
    rebuildHierarchyState( true );

    m_settingTopLevelSheets = wasAlreadySetting;
}


SCH_SCREEN* SCHEMATIC::RootScreen() const
{
    // Virtual root's screen is just a container - return the first top-level sheet's screen
    // which is what callers actually want
    if( !m_topLevelSheets.empty() && m_topLevelSheets[0] )
        return m_topLevelSheets[0]->GetScreen();

    return nullptr;
}


SCH_SHEET_LIST SCHEMATIC::Hierarchy() const
{
    wxCHECK( !m_hierarchy.empty(), m_hierarchy );

    return m_hierarchy;
}


void SCHEMATIC::RefreshHierarchy()
{
    ensureDefaultTopLevelSheet();
    m_hierarchy = BuildSheetListSortedByPageNumbers();
}


void SCHEMATIC::GetContextualTextVars( wxArrayString* aVars ) const
{
    auto add = [&]( const wxString& aVar )
    {
        if( !alg::contains( *aVars, aVar ) )
            aVars->push_back( aVar );
    };

    add( wxT( "#" ) );
    add( wxT( "##" ) );
    add( wxT( "SHEETPATH" ) );
    add( wxT( "SHEETNAME" ) );
    add( wxT( "FILENAME" ) );
    add( wxT( "FILEPATH" ) );
    add( wxT( "PROJECTNAME" ) );
    add( wxT( "VARIANT" ) );
    add( wxT( "VARIANT_DESC" ) );

    if( !CurrentSheet().empty() )
        CurrentSheet().LastScreen()->GetTitleBlock().GetContextualTextVars( aVars );

    for( std::pair<wxString, wxString> entry : m_project->GetTextVars() )
        add( entry.first );
}


bool SCHEMATIC::ResolveTextVar( const SCH_SHEET_PATH* aSheetPath, wxString* token, int aDepth ) const
{
    wxCHECK( aSheetPath, false );

    if( token->IsSameAs( wxT( "#" ) ) )
    {
        *token = aSheetPath->GetPageNumber();
        return true;
    }
    else if( token->IsSameAs( wxT( "##" ) ) )
    {
        *token = wxString::Format( "%i", Root().CountSheets() );
        return true;
    }
    else if( token->IsSameAs( wxT( "SHEETPATH" ) ) )
    {
        *token = aSheetPath->PathHumanReadable();
        return true;
    }
    else if( token->IsSameAs( wxT( "SHEETNAME" ) ) )
    {
        *token = aSheetPath->Last()->GetName();
        return true;
    }
    else if( token->IsSameAs( wxT( "FILENAME" ) ) )
    {
        wxFileName fn( GetFileName() );
        *token = fn.GetFullName();
        return true;
    }
    else if( token->IsSameAs( wxT( "FILEPATH" ) ) )
    {
        wxFileName fn( GetFileName() );
        *token = fn.GetFullPath();
        return true;
    }
    else if( token->IsSameAs( wxT( "PROJECTNAME" ) ) )
    {
        *token = m_project->GetProjectName();
        return true;
    }
    else if( token->IsSameAs( wxT( "VARIANTNAME" ) ) || token->IsSameAs( wxT( "VARIANT" ) ) )
    {
        *token = m_currentVariant;
        return true;
    }
    else if( token->IsSameAs( wxT( "VARIANT_DESC" ) ) )
    {
        *token = GetVariantDescription( m_currentVariant );
        return true;
    }

    // aSheetPath->LastScreen() can be null during schematic loading
    if( aSheetPath->LastScreen() && aSheetPath->LastScreen()->GetTitleBlock().TextVarResolver( token, m_project ) )
        return true;

    if( m_project->TextVarResolver( token ) )
        return true;

    return false;
}


wxString SCHEMATIC::GetFileName() const
{
    // With virtual root pattern, m_rootSheet is the virtual root with no file
    // Return filename from first top-level sheet if available
    if( !IsValid() )
        return wxString( wxEmptyString );

    if( !m_topLevelSheets.empty() && m_topLevelSheets[0]->GetScreen() )
        return m_topLevelSheets[0]->GetScreen()->GetFileName();

    return wxString( wxEmptyString );
}


SCHEMATIC_SETTINGS& SCHEMATIC::Settings() const
{
    if( !m_project )
    {
        static SCHEMATIC_SETTINGS defaultSettings( nullptr, "schematic" );
        return defaultSettings;
    }
    wxASSERT( m_project );
    return *m_project->GetProjectFile().m_SchematicSettings;
}


ERC_SETTINGS& SCHEMATIC::ErcSettings() const
{
    wxASSERT( m_project );
    return *m_project->GetProjectFile().m_ErcSettings;
}


std::vector<SCH_MARKER*> SCHEMATIC::ResolveERCExclusions()
{
    SCH_SHEET_LIST sheetList = Hierarchy();
    ERC_SETTINGS&  settings = ErcSettings();

    // Migrate legacy marker exclusions to new format to ensure exclusion matching functions across
    // file versions. Silently drops any legacy exclusions which can not be mapped to the new format
    // without risking an incorrect exclusion - this is preferable to silently dropping
    // new ERC errors / warnings due to an incorrect match between a legacy and new
    // marker serialization format
    std::set<wxString> migratedExclusions;

    for( auto it = settings.m_ErcExclusions.begin(); it != settings.m_ErcExclusions.end(); )
    {
        SCH_MARKER* testMarker = SCH_MARKER::DeserializeFromString( sheetList, *it );

        if( !testMarker )
        {
            it = settings.m_ErcExclusions.erase( it );
            continue;
        }

        if( testMarker->IsLegacyMarker() )
        {
            const wxString settingsKey = testMarker->GetRCItem()->GetSettingsKey();

            if( settingsKey != wxT( "pin_to_pin" ) && settingsKey != wxT( "hier_label_mismatch" )
                && settingsKey != wxT( "different_unit_net" ) )
            {
                migratedExclusions.insert( testMarker->SerializeToString() );
            }

            it = settings.m_ErcExclusions.erase( it );
        }
        else
        {
            ++it;
        }

        delete testMarker;
    }

    settings.m_ErcExclusions.insert( migratedExclusions.begin(), migratedExclusions.end() );

    // End of legacy exclusion removal / migrations

    for( const SCH_SHEET_PATH& sheet : sheetList )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_MARKER_T ) )
        {
            SCH_MARKER*                  marker = static_cast<SCH_MARKER*>( item );
            wxString                     serialized = marker->SerializeToString();
            std::set<wxString>::iterator it = settings.m_ErcExclusions.find( serialized );

            if( it != settings.m_ErcExclusions.end() )
            {
                marker->SetExcluded( true, settings.m_ErcExclusionComments[serialized] );
                settings.m_ErcExclusions.erase( it );
            }
        }
    }

    std::vector<SCH_MARKER*> newMarkers;

    for( const wxString& serialized : settings.m_ErcExclusions )
    {
        SCH_MARKER* marker = SCH_MARKER::DeserializeFromString( sheetList, serialized );

        if( marker )
        {
            marker->SetExcluded( true, settings.m_ErcExclusionComments[serialized] );
            newMarkers.push_back( marker );
        }
    }

    settings.m_ErcExclusions.clear();

    return newMarkers;
}


std::shared_ptr<BUS_ALIAS> SCHEMATIC::GetBusAlias( const wxString& aLabel ) const
{
    for( const std::shared_ptr<BUS_ALIAS>& alias : m_busAliases )
    {
        if( alias && alias->GetName() == aLabel )
            return alias;
    }

    return nullptr;
}


void SCHEMATIC::AddBusAlias( std::shared_ptr<BUS_ALIAS> aAlias )
{
    if( !aAlias )
        return;

    auto sameDefinition = [&]( const std::shared_ptr<BUS_ALIAS>& candidate ) -> bool
    {
        return candidate && candidate->GetName() == aAlias->GetName() && candidate->Members() == aAlias->Members();
    };

    auto it = std::find_if( m_busAliases.begin(), m_busAliases.end(), sameDefinition );

    if( it != m_busAliases.end() )
        return;

    m_busAliases.push_back( aAlias );

    updateProjectBusAliases();
}


void SCHEMATIC::SetBusAliases( const std::vector<std::shared_ptr<BUS_ALIAS>>& aAliases )
{
    m_busAliases.clear();

    for( const std::shared_ptr<BUS_ALIAS>& alias : aAliases )
    {
        if( !alias )
            continue;

        std::shared_ptr<BUS_ALIAS> clone = alias->Clone();

        auto sameDefinition = [&]( const std::shared_ptr<BUS_ALIAS>& candidate ) -> bool
        {
            return candidate && candidate->GetName() == clone->GetName() && candidate->Members() == clone->Members();
        };

        if( std::find_if( m_busAliases.begin(), m_busAliases.end(), sameDefinition ) != m_busAliases.end() )
            continue;

        m_busAliases.push_back( clone );
    }

    updateProjectBusAliases();
}


void SCHEMATIC::loadBusAliasesFromProject()
{
    m_busAliases.clear();

    if( !m_project )
        return;

    const auto& projectAliases = m_project->GetProjectFile().m_BusAliases;

    for( const auto& alias : projectAliases )
    {
        std::shared_ptr<BUS_ALIAS> busAlias = std::make_shared<BUS_ALIAS>();

        busAlias->SetName( alias.first );
        busAlias->Members() = alias.second;

        m_busAliases.push_back( busAlias );
    }
}


void SCHEMATIC::updateProjectBusAliases()
{
    if( !m_project )
        return;

    auto& projectAliases = m_project->GetProjectFile().m_BusAliases;

    projectAliases.clear();

    std::set<wxString> seen;

    for( const std::shared_ptr<BUS_ALIAS>& alias : m_busAliases )
    {
        if( !alias )
            continue;

        if( !seen.insert( alias->GetName() ).second )
            continue;

        projectAliases.emplace( alias->GetName(), alias->Members() );
    }
}


std::set<wxString> SCHEMATIC::GetNetClassAssignmentCandidates()
{
    std::set<wxString> names;

    for( const auto& [key, subgraphList] : m_connectionGraph->GetNetMap() )
    {
        CONNECTION_SUBGRAPH* firstSubgraph = subgraphList[0];

        if( !firstSubgraph->GetDriverConnection()->IsBus()
            && firstSubgraph->GetDriverPriority() >= CONNECTION_SUBGRAPH::PRIORITY::PIN )
        {
            names.insert( key.Name );
        }
    }

    return names;
}


bool SCHEMATIC::ResolveCrossReference( wxString* token, int aDepth ) const
{
    wxString       remainder;
    wxString       ref = token->BeforeFirst( ':', &remainder );
    KIID_PATH      path( ref );
    KIID           uuid = path.back();
    SCH_SHEET_PATH sheetPath;
    SCH_ITEM*      refItem = ResolveItem( KIID( uuid ), &sheetPath, true );

    if( path.size() > 1 )
    {
        path.pop_back();
        sheetPath = Hierarchy().GetSheetPathByKIIDPath( path ).value_or( sheetPath );
    }

    // Parse optional variant name from syntax ${REF:FIELD:VARIANT}
    // remainder is "FIELD" or "FIELD:VARIANT"
    wxString variantName;
    wxString fieldName = remainder;
    int      colonPos = remainder.Find( ':' );

    if( colonPos != wxNOT_FOUND )
    {
        fieldName = remainder.Left( colonPos );
        variantName = remainder.Mid( colonPos + 1 );
    }

    // Note: We don't expand nested variables or evaluate math expressions here.
    // The multi-pass loop in GetShownText handles all variable and expression resolution
    // before cross-references are resolved. This ensures table cell variables like ${ROW}
    // are expanded correctly.

    if( refItem && refItem->Type() == SCH_SYMBOL_T )
    {
        SCH_SYMBOL* refSymbol = static_cast<SCH_SYMBOL*>( refItem );

        bool resolved = refSymbol->ResolveTextVar( &sheetPath, &fieldName, variantName, aDepth + 1 );

        if( resolved )
        {
            *token = std::move( fieldName );
        }
        else
        {
            // Field/function not found on symbol
            *token = wxString::Format( wxT( "<Unresolved: %s:%s>" ), refSymbol->GetRef( &sheetPath, false ), fieldName );
        }

        return true;
    }
    else if( refItem && refItem->Type() == SCH_SHEET_T )
    {
        SCH_SHEET* refSheet = static_cast<SCH_SHEET*>( refItem );

        sheetPath.push_back( refSheet );

        wxString remainderBefore = remainder;

        if( refSheet->ResolveTextVar( &sheetPath, &remainder, aDepth + 1 ) )
            *token = std::move( remainder );

        // If the remainder still contains unresolved variables or expressions,
        // return false so ExpandTextVars keeps the ${...} wrapper
        if( remainderBefore.Contains( wxT( "${" ) ) || remainderBefore.Contains( wxT( "@{" ) ) )
            return false;

        return true; // Cross-reference is resolved
    }

    // If UUID resolution failed, try to resolve by reference designator
    // This handles both exact matches (J601A) and parent references for multi-unit symbols (J601)
    if( !refItem )
    {
        SCH_REFERENCE_LIST refs;
        Hierarchy().GetSymbols( refs );

        SCH_SYMBOL*    foundSymbol = nullptr;
        SCH_SHEET_PATH foundPath;

        for( int ii = 0; ii < (int) refs.GetCount(); ii++ )
        {
            SCH_REFERENCE& reference = refs[ii];
            wxString       symbolRef = reference.GetSymbol()->GetRef( &reference.GetSheetPath(), false );

            // Try exact match first
            if( symbolRef == ref )
            {
                foundSymbol = reference.GetSymbol();
                foundPath = reference.GetSheetPath();
                break;
            }

            // For multi-unit symbols, try matching parent reference (e.g., J601 matches J601A)
            if( symbolRef.StartsWith( ref ) && symbolRef.Length() == ref.Length() + 1 )
            {
                wxChar lastChar = symbolRef.Last();
                if( lastChar >= 'A' && lastChar <= 'Z' )
                {
                    foundSymbol = reference.GetSymbol();
                    foundPath = reference.GetSheetPath();
                    // Don't break - continue looking for exact match
                }
            }
        }

        if( foundSymbol )
        {
            bool resolved = foundSymbol->ResolveTextVar( &foundPath, &fieldName, variantName, aDepth + 1 );

            if( resolved )
            {
                *token = std::move( fieldName );
            }
            else
            {
                // Field/function not found on symbol
                *token = wxString::Format( wxT( "<Unresolved: %s:%s>" ), foundSymbol->GetRef( &foundPath, false ),
                                           fieldName );
            }

            return true;
        }

        // Symbol not found - set unresolved error
        *token = wxString::Format( wxT( "<Unresolved: %s>" ), ref );
        return true;
    }

    // Reference not found - show error message
    *token = wxString::Format( wxT( "<Unknown reference: %s>" ), ref );
    return true;
}


std::map<int, wxString> SCHEMATIC::GetVirtualPageToSheetNamesMap() const
{
    std::map<int, wxString> namesMap;

    for( const SCH_SHEET_PATH& sheet : Hierarchy() )
    {
        if( sheet.size() == 1 )
            namesMap[sheet.GetVirtualPageNumber()] = _( "<root sheet>" );
        else
            namesMap[sheet.GetVirtualPageNumber()] = sheet.Last()->GetName();
    }

    return namesMap;
}


std::map<int, wxString> SCHEMATIC::GetVirtualPageToSheetPagesMap() const
{
    std::map<int, wxString> pagesMap;

    for( const SCH_SHEET_PATH& sheet : Hierarchy() )
        pagesMap[sheet.GetVirtualPageNumber()] = sheet.GetPageNumber();

    return pagesMap;
}


wxString SCHEMATIC::ConvertRefsToKIIDs( const wxString& aSource ) const
{
    wxString newbuf;
    size_t   sourceLen = aSource.length();

    for( size_t i = 0; i < sourceLen; ++i )
    {
        // Check for escaped expressions: \${ or \@{
        // These should be copied verbatim without any ref→KIID conversion
        if( aSource[i] == '\\' && i + 2 < sourceLen && aSource[i + 2] == '{' &&
            ( aSource[i + 1] == '$' || aSource[i + 1] == '@' ) )
        {
            // Copy the escape sequence and the entire escaped expression
            newbuf.append( aSource[i] );     // backslash
            newbuf.append( aSource[i + 1] ); // $ or @
            newbuf.append( aSource[i + 2] ); // {
            i += 2;

            // Find and copy everything until the matching closing brace
            int braceDepth = 1;
            for( i = i + 1; i < sourceLen && braceDepth > 0; ++i )
            {
                if( aSource[i] == '{' )
                    braceDepth++;
                else if( aSource[i] == '}' )
                    braceDepth--;

                newbuf.append( aSource[i] );
            }
            i--; // Back up one since the for loop will increment
            continue;
        }

        if( aSource[i] == '$' && i + 1 < sourceLen && aSource[i + 1] == '{' )
        {
            wxString token;
            bool     isCrossRef = false;
            int      nesting = 0;

            for( i = i + 2; i < sourceLen; ++i )
            {
                if( aSource[i] == '{' && ( aSource[i - 1] == '_' || aSource[i - 1] == '^' || aSource[i - 1] == '~' ) )
                {
                    nesting++;
                }

                if( aSource[i] == '}' )
                {
                    nesting--;

                    if( nesting < 0 )
                        break;
                }

                if( aSource[i] == ':' )
                    isCrossRef = true;

                token.append( aSource[i] );
            }

            if( isCrossRef )
            {
                wxString           remainder;
                wxString           ref = token.BeforeFirst( ':', &remainder );
                SCH_REFERENCE_LIST references;

                Hierarchy().GetSymbols( references );

                for( size_t jj = 0; jj < references.GetCount(); jj++ )
                {
                    SCH_SYMBOL* refSymbol = references[jj].GetSymbol();

                    if( ref == refSymbol->GetRef( &references[jj].GetSheetPath(), true ) )
                    {
                        KIID_PATH path = references[jj].GetSheetPath().Path();
                        path.push_back( refSymbol->m_Uuid );

                        token = path.AsString() + wxS( ":" ) + remainder;
                        break;
                    }
                }
            }

            newbuf.append( wxS( "${" ) + token + wxS( "}" ) );
        }
        else
        {
            newbuf.append( aSource[i] );
        }
    }

    return newbuf;
}


wxString SCHEMATIC::ConvertKIIDsToRefs( const wxString& aSource ) const
{
    wxString newbuf;
    size_t   sourceLen = aSource.length();

    for( size_t i = 0; i < sourceLen; ++i )
    {
        // Check for escaped expressions: \${ or \@{
        // These should be copied verbatim without any KIID→ref conversion
        if( aSource[i] == '\\' && i + 2 < sourceLen && aSource[i + 2] == '{' &&
            ( aSource[i + 1] == '$' || aSource[i + 1] == '@' ) )
        {
            // Copy the escape sequence and the entire escaped expression
            newbuf.append( aSource[i] );     // backslash
            newbuf.append( aSource[i + 1] ); // $ or @
            newbuf.append( aSource[i + 2] ); // {
            i += 2;

            // Find and copy everything until the matching closing brace
            int braceDepth = 1;
            for( i = i + 1; i < sourceLen && braceDepth > 0; ++i )
            {
                if( aSource[i] == '{' )
                    braceDepth++;
                else if( aSource[i] == '}' )
                    braceDepth--;

                newbuf.append( aSource[i] );
            }
            i--; // Back up one since the for loop will increment
            continue;
        }

        if( aSource[i] == '$' && i + 1 < sourceLen && aSource[i + 1] == '{' )
        {
            wxString token;
            bool     isCrossRef = false;

            for( i = i + 2; i < sourceLen; ++i )
            {
                if( aSource[i] == '}' )
                    break;

                if( aSource[i] == ':' )
                    isCrossRef = true;

                token.append( aSource[i] );
            }

            if( isCrossRef )
            {
                wxString       remainder;
                wxString       ref = token.BeforeFirst( ':', &remainder );
                KIID_PATH      path( ref );
                KIID           uuid = path.back();
                SCH_SHEET_PATH sheetPath;
                SCH_ITEM*      refItem = ResolveItem( uuid, &sheetPath, true );

                if( path.size() > 1 )
                {
                    path.pop_back();
                    sheetPath = Hierarchy().GetSheetPathByKIIDPath( path ).value_or( sheetPath );
                }

                if( refItem && refItem->Type() == SCH_SYMBOL_T )
                {
                    SCH_SYMBOL* refSymbol = static_cast<SCH_SYMBOL*>( refItem );
                    token = refSymbol->GetRef( &sheetPath, true ) + wxS( ":" ) + remainder;
                }
            }

            newbuf.append( wxS( "${" ) + token + wxS( "}" ) );
        }
        else
        {
            newbuf.append( aSource[i] );
        }
    }

    return newbuf;
}


void SCHEMATIC::SetLegacySymbolInstanceData()
{
    SCH_SCREENS screens( m_rootSheet );

    screens.SetLegacySymbolInstanceData();
}


wxString SCHEMATIC::GetUniqueFilenameForCurrentSheet()
{
    // Filename is rootSheetName-sheetName-...-sheetName
    // Note that we need to fetch the rootSheetName out of its filename, as the root SCH_SHEET's
    // name is just a timestamp.

    // Skip virtual root if present
    size_t startIdx = 0;
    if( CurrentSheet().size() > 0 && CurrentSheet().at( 0 )->IsVirtualRootSheet() )
        startIdx = 1;

    // Handle the case where we only have a virtual root (shouldn't happen in practice)
    if( startIdx >= CurrentSheet().size() )
        return wxEmptyString;

    wxFileName rootFn( CurrentSheet().at( startIdx )->GetFileName() );
    wxString   filename = rootFn.GetName();

    for( unsigned i = startIdx + 1; i < CurrentSheet().size(); i++ )
        filename += wxT( "-" ) + CurrentSheet().at( i )->GetName();

    return filename;
}


void SCHEMATIC::SetSheetNumberAndCount()
{
    SCH_SCREEN* screen;
    SCH_SCREENS s_list( Root() );

    // Set the sheet count, and the sheet number (1 for root sheet)
    int sheet_count;

    // Handle virtual root case
    if( Root().m_Uuid == niluuid )
    {
        // Virtual root: count all top-level sheets
        sheet_count = 0;

        for( const SCH_SHEET* topSheet : m_topLevelSheets )
        {
            if( topSheet )
                sheet_count += topSheet->CountSheets();
        }
    }
    else
    {
        // Traditional single root
        sheet_count = Root().CountSheets();
    }

    int              sheet_number = 1;
    const KIID_PATH& current_sheetpath = CurrentSheet().Path();

    // @todo Remove all pseudo page number system is left over from prior to real page number
    //       implementation.
    for( const SCH_SHEET_PATH& sheet : Hierarchy() )
    {
        if( sheet.Path() == current_sheetpath ) // Current sheet path found
            break;

        sheet_number++; // Not found, increment before this current path
    }

    for( screen = s_list.GetFirst(); screen != nullptr; screen = s_list.GetNext() )
        screen->SetPageCount( sheet_count );

    CurrentSheet().SetVirtualPageNumber( sheet_number );
    CurrentSheet().LastScreen()->SetVirtualPageNumber( sheet_number );
    CurrentSheet().LastScreen()->SetPageNumber( CurrentSheet().GetPageNumber() );
}


void SCHEMATIC::RecomputeIntersheetRefs()
{
    std::map<wxString, std::set<int>>& pageRefsMap = GetPageRefsMap();

    pageRefsMap.clear();

    for( const SCH_SHEET_PATH& sheet : Hierarchy() )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_GLOBAL_LABEL_T ) )
        {
            SCH_GLOBALLABEL* global = static_cast<SCH_GLOBALLABEL*>( item );
            wxString         resolvedLabel = global->GetShownText( &sheet, false );

            pageRefsMap[resolvedLabel].insert( sheet.GetVirtualPageNumber() );
        }
    }

    bool show = Settings().m_IntersheetRefsShow;

    // Refresh all visible global labels.  Note that we have to collect them first as the
    // SCH_SCREEN::Update() call is going to invalidate the RTree iterator.

    std::vector<SCH_GLOBALLABEL*> currentSheetGlobalLabels;

    for( EDA_ITEM* item : CurrentSheet().LastScreen()->Items().OfType( SCH_GLOBAL_LABEL_T ) )
        currentSheetGlobalLabels.push_back( static_cast<SCH_GLOBALLABEL*>( item ) );

    for( SCH_GLOBALLABEL* globalLabel : currentSheetGlobalLabels )
    {
        std::vector<SCH_FIELD>& fields = globalLabel->GetFields();

        fields[0].SetVisible( show );

        if( show )
        {
            if( fields.size() == 1 && fields[0].GetTextPos() == globalLabel->GetPosition() )
                globalLabel->AutoplaceFields( CurrentSheet().LastScreen(), AUTOPLACE_AUTO );

            CurrentSheet().LastScreen()->Update( globalLabel );

            for( SCH_FIELD& field : globalLabel->GetFields() )
                field.ClearBoundingBoxCache();

            globalLabel->ClearBoundingBoxCache();

            if( m_schematicHolder )
                m_schematicHolder->IntersheetRefUpdate( globalLabel );
        }
    }
}


wxString SCHEMATIC::GetOperatingPoint( const wxString& aNetName, int aPrecision, const wxString& aRange )
{
    wxString spiceNetName( aNetName.Lower() );
    NETLIST_EXPORTER_SPICE::ConvertToSpiceMarkup( &spiceNetName );

    if( spiceNetName == wxS( "gnd" ) || spiceNetName == wxS( "0" ) )
        return wxEmptyString;

    auto it = m_operatingPoints.find( spiceNetName );

    if( it != m_operatingPoints.end() )
        return SPICE_VALUE( it->second ).ToString( { aPrecision, aRange } );
    else if( m_operatingPoints.empty() )
        return wxS( "--" );
    else
        return wxS( "?" );
}


void SCHEMATIC::FixupJunctionsAfterImport()
{
    SCH_SCREENS screens( Root() );

    for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
    {
        std::deque<EDA_ITEM*> allItems;

        for( SCH_ITEM* item : screen->Items() )
            allItems.push_back( item );

        // Add missing junctions and breakup wires as needed
        for( const VECTOR2I& point : screen->GetNeededJunctions( allItems ) )
        {
            SCH_JUNCTION* junction = new SCH_JUNCTION( point );
            screen->Append( junction );

            // Breakup wires
            for( SCH_LINE* wire : screen->GetBusesAndWires( point, true ) )
            {
                SCH_LINE* newSegment = wire->NonGroupAware_BreakAt( point );
                screen->Append( newSegment );
            }
        }
    }
}


void SCHEMATIC::OnItemsAdded( std::vector<SCH_ITEM*>& aNewItems )
{
    InvokeListeners( &SCHEMATIC_LISTENER::OnSchItemsAdded, *this, aNewItems );
}


void SCHEMATIC::OnItemsRemoved( std::vector<SCH_ITEM*>& aRemovedItems )
{
    InvokeListeners( &SCHEMATIC_LISTENER::OnSchItemsRemoved, *this, aRemovedItems );
}


void SCHEMATIC::OnItemsChanged( std::vector<SCH_ITEM*>& aItems )
{
    InvokeListeners( &SCHEMATIC_LISTENER::OnSchItemsChanged, *this, aItems );
}


void SCHEMATIC::OnSchSheetChanged()
{
    InvokeListeners( &SCHEMATIC_LISTENER::OnSchSheetChanged, *this );
}


void SCHEMATIC::AddListener( SCHEMATIC_LISTENER* aListener )
{
    if( !alg::contains( m_listeners, aListener ) )
        m_listeners.push_back( aListener );
}


void SCHEMATIC::RemoveListener( SCHEMATIC_LISTENER* aListener )
{
    auto i = std::find( m_listeners.begin(), m_listeners.end(), aListener );

    if( i != m_listeners.end() )
    {
        std::iter_swap( i, m_listeners.end() - 1 );
        m_listeners.pop_back();
    }
}


void SCHEMATIC::RemoveAllListeners()
{
    m_listeners.clear();
}


void SCHEMATIC::RecordERCExclusions()
{
    // Use a sorted sheetList to reduce file churn
    SCH_SHEET_LIST sheetList = Hierarchy();
    ERC_SETTINGS&  ercSettings = ErcSettings();

    ercSettings.m_ErcExclusions.clear();
    ercSettings.m_ErcExclusionComments.clear();

    for( unsigned i = 0; i < sheetList.size(); i++ )
    {
        for( SCH_ITEM* item : sheetList[i].LastScreen()->Items().OfType( SCH_MARKER_T ) )
        {
            SCH_MARKER* marker = static_cast<SCH_MARKER*>( item );

            if( marker->IsExcluded() )
            {
                wxString serialized = marker->SerializeToString();
                ercSettings.m_ErcExclusions.insert( serialized );
                ercSettings.m_ErcExclusionComments[serialized] = marker->GetComment();
            }
        }
    }
}


void SCHEMATIC::ResolveERCExclusionsPostUpdate()
{
    SCH_SHEET_LIST sheetList = Hierarchy();

    for( SCH_MARKER* marker : ResolveERCExclusions() )
    {
        SCH_SHEET_PATH errorPath;
        ignore_unused( sheetList.ResolveItem( marker->GetRCItem()->GetMainItemID(), &errorPath ) );

        if( errorPath.LastScreen() )
            errorPath.LastScreen()->Append( marker );
        else
            RootScreen()->Append( marker );
    }

    // Once we have the ERC Exclusions, record them in the project file so that
    // they are retained even before the schematic is saved (PCB Editor can also save the project)
    RecordERCExclusions();
}


EMBEDDED_FILES* SCHEMATIC::GetEmbeddedFiles()
{
    return static_cast<EMBEDDED_FILES*>( this );
}


const EMBEDDED_FILES* SCHEMATIC::GetEmbeddedFiles() const
{
    return static_cast<const EMBEDDED_FILES*>( this );
}


void SCHEMATIC::RunOnNestedEmbeddedFiles( const std::function<void( EMBEDDED_FILES* )>& aFunction )
{
    SCH_SCREENS screens( Root() );

    for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
    {
        for( auto& [name, libSym] : screen->GetLibSymbols() )
            aFunction( libSym->GetEmbeddedFiles() );
    }
}


std::set<KIFONT::OUTLINE_FONT*> SCHEMATIC::GetFonts() const
{
    std::set<KIFONT::OUTLINE_FONT*> fonts;

    SCH_SHEET_LIST sheetList = Hierarchy();

    for( const SCH_SHEET_PATH& sheet : sheetList )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items() )
        {
            if( EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( item ) )
            {
                KIFONT::FONT* font = text->GetFont();

                if( !font || font->IsStroke() )
                    continue;

                using EMBEDDING_PERMISSION = KIFONT::OUTLINE_FONT::EMBEDDING_PERMISSION;
                auto* outline = static_cast<KIFONT::OUTLINE_FONT*>( font );

                if( outline->GetEmbeddingPermission() == EMBEDDING_PERMISSION::EDITABLE
                    || outline->GetEmbeddingPermission() == EMBEDDING_PERMISSION::INSTALLABLE )
                {
                    fonts.insert( outline );
                }
            }
        }
    }

    return fonts;
}


void SCHEMATIC::EmbedFonts()
{
    std::set<KIFONT::OUTLINE_FONT*> fonts = GetFonts();

    for( KIFONT::OUTLINE_FONT* font : fonts )
    {
        auto file = GetEmbeddedFiles()->AddFile( font->GetFileName(), false );

        if( !file )
        {
            wxLogTrace( "EMBED", "Failed to add font file: %s", font->GetFileName() );
            continue;
        }

        file->type = EMBEDDED_FILES::EMBEDDED_FILE::FILE_TYPE::FONT;
    }
}


std::set<const SCH_SCREEN*> SCHEMATIC::GetSchematicsSharedByMultipleProjects() const
{
    std::set<const SCH_SCREEN*> retv;

    wxCHECK( m_rootSheet, retv );

    SCH_SHEET_LIST hierarchy( m_rootSheet );
    SCH_SCREENS    screens( m_rootSheet );

    for( const SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
    {
        for( const SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            const SCH_SYMBOL* symbol = static_cast<const SCH_SYMBOL*>( item );

            const std::vector<SCH_SYMBOL_INSTANCE> symbolInstances = symbol->GetInstances();

            for( const SCH_SYMBOL_INSTANCE& instance : symbolInstances )
            {
                if( !hierarchy.HasPath( instance.m_Path ) )
                {
                    retv.insert( screen );
                    break;
                }
            }

            if( retv.count( screen ) )
                break;
        }
    }

    return retv;
}


bool SCHEMATIC::IsComplexHierarchy() const
{
    wxCHECK( m_rootSheet, false );

    SCH_SCREENS screens( m_rootSheet );

    for( const SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
    {
        wxCHECK2( screen, continue );

        if( screen->GetRefCount() > 1 )
            return true;
    }

    return false;
}


void SCHEMATIC::CleanUp( SCH_COMMIT* aCommit, SCH_SCREEN* aScreen )
{
    SCH_SELECTION_TOOL*          selectionTool = m_schematicHolder ? m_schematicHolder->GetSelectionTool() : nullptr;
    std::vector<SCH_LINE*>       lines;
    std::vector<SCH_JUNCTION*>   junctions;
    std::vector<SCH_NO_CONNECT*> ncs;
    std::vector<SCH_ITEM*>       items_to_remove;
    bool                         changed = true;

    if( aScreen == nullptr )
        aScreen = GetCurrentScreen();

    auto remove_item = [&]( SCH_ITEM* aItem ) -> void
    {
        changed = true;

        if( !( aItem->GetFlags() & STRUCT_DELETED ) )
        {
            aItem->SetFlags( STRUCT_DELETED );

            if( aItem->IsSelected() && selectionTool )
                selectionTool->RemoveItemFromSel( aItem, true /*quiet mode*/ );

            if( m_schematicHolder )
            {
                m_schematicHolder->RemoveFromScreen( aItem, aScreen );
            }
            aCommit->Removed( aItem, aScreen );
        }
    };


    for( SCH_ITEM* item : aScreen->Items().OfType( SCH_JUNCTION_T ) )
    {
        if( !aScreen->IsExplicitJunction( item->GetPosition() ) )
        {
            if( item->IsSelected() || item->HasFlag( SELECTED_BY_DRAG ) )
                continue;

            items_to_remove.push_back( item );
        }
        else
            junctions.push_back( static_cast<SCH_JUNCTION*>( item ) );
    }

    for( SCH_ITEM* item : items_to_remove )
        remove_item( item );

    for( SCH_ITEM* item : aScreen->Items().OfType( SCH_NO_CONNECT_T ) )
        ncs.push_back( static_cast<SCH_NO_CONNECT*>( item ) );

    alg::for_all_pairs( junctions.begin(), junctions.end(),
                        [&]( SCH_JUNCTION* aFirst, SCH_JUNCTION* aSecond )
                        {
                            if( ( aFirst->GetEditFlags() & STRUCT_DELETED )
                                || ( aSecond->GetEditFlags() & STRUCT_DELETED ) )
                            {
                                return;
                            }

                            if( aFirst->GetPosition() == aSecond->GetPosition() )
                                remove_item( aSecond );
                        } );

    alg::for_all_pairs( ncs.begin(), ncs.end(),
                        [&]( SCH_NO_CONNECT* aFirst, SCH_NO_CONNECT* aSecond )
                        {
                            if( ( aFirst->GetEditFlags() & STRUCT_DELETED )
                                || ( aSecond->GetEditFlags() & STRUCT_DELETED ) )
                            {
                                return;
                            }

                            if( aFirst->GetPosition() == aSecond->GetPosition() )
                                remove_item( aSecond );
                        } );


    auto minX = []( const SCH_LINE* l )
    {
        return std::min( l->GetStartPoint().x, l->GetEndPoint().x );
    };

    auto maxX = []( const SCH_LINE* l )
    {
        return std::max( l->GetStartPoint().x, l->GetEndPoint().x );
    };

    auto minY = []( const SCH_LINE* l )
    {
        return std::min( l->GetStartPoint().y, l->GetEndPoint().y );
    };

    auto maxY = []( const SCH_LINE* l )
    {
        return std::max( l->GetStartPoint().y, l->GetEndPoint().y );
    };

    // Would be nice to put lines in a canonical form here by swapping
    //  start <-> end as needed but I don't know what swapping breaks.
    while( changed )
    {
        changed = false;
        lines.clear();

        for( SCH_ITEM* item : aScreen->Items().OfType( SCH_LINE_T ) )
        {
            if( item->GetLayer() == LAYER_WIRE || item->GetLayer() == LAYER_BUS )
                lines.push_back( static_cast<SCH_LINE*>( item ) );
        }

        // Sort by minimum X position
        std::sort( lines.begin(), lines.end(),
                   [&]( const SCH_LINE* a, const SCH_LINE* b )
                   {
                       return minX( a ) < minX( b );
                   } );

        for( auto it1 = lines.begin(); it1 != lines.end(); ++it1 )
        {
            SCH_LINE* firstLine = *it1;

            if( firstLine->GetEditFlags() & STRUCT_DELETED )
                continue;

            if( firstLine->IsNull() )
            {
                remove_item( firstLine );
                continue;
            }

            int  firstRightXEdge = maxX( firstLine );
            auto it2 = it1;

            for( ++it2; it2 != lines.end(); ++it2 )
            {
                SCH_LINE* secondLine = *it2;
                int       secondLeftXEdge = minX( secondLine );

                // impossible to overlap remaining lines
                if( secondLeftXEdge > firstRightXEdge )
                    break;

                // No Y axis overlap
                if( !( std::max( minY( firstLine ), minY( secondLine ) )
                       <= std::min( maxY( firstLine ), maxY( secondLine ) ) ) )
                {
                    continue;
                }

                if( secondLine->GetFlags() & STRUCT_DELETED )
                    continue;

                if( !secondLine->IsParallel( firstLine ) || !secondLine->IsStrokeEquivalent( firstLine )
                    || secondLine->GetLayer() != firstLine->GetLayer() )
                {
                    continue;
                }

                // Remove identical lines
                if( firstLine->IsEndPoint( secondLine->GetStartPoint() )
                    && firstLine->IsEndPoint( secondLine->GetEndPoint() ) )
                {
                    remove_item( secondLine );
                    continue;
                }

                // See if we can merge an overlap (or two colinear touching segments with
                // no junction where they meet).
                SCH_LINE* mergedLine = secondLine->MergeOverlap( aScreen, firstLine, true );

                if( mergedLine != nullptr )
                {
                    remove_item( firstLine );
                    remove_item( secondLine );

                    if( m_schematicHolder )
                    {
                        m_schematicHolder->AddToScreen( mergedLine, aScreen );
                    }

                    aCommit->Added( mergedLine, aScreen );

                    if( selectionTool && ( firstLine->IsSelected() || secondLine->IsSelected() ) )
                        selectionTool->AddItemToSel( mergedLine, true /*quiet mode*/ );

                    break;
                }
            }
        }
    }
}


void SCHEMATIC::RecalculateConnections( SCH_COMMIT* aCommit, SCH_CLEANUP_FLAGS aCleanupFlags,
                                        TOOL_MANAGER* aToolManager, PROGRESS_REPORTER* aProgressReporter,
                                        KIGFX::SCH_VIEW*                  aSchView,
                                        std::function<void( SCH_ITEM* )>* aChangedItemHandler,
                                        PICKED_ITEMS_LIST*                aLastChangeList )
{
    SCHEMATIC_SETTINGS& settings = Settings();
    RefreshHierarchy();
    SCH_SHEET_LIST list = Hierarchy();
    SCH_COMMIT     localCommit( aToolManager );

    if( !aCommit )
        aCommit = &localCommit;

    PROF_TIMER timer;

    // Ensure schematic graph is accurate
    if( aCleanupFlags == LOCAL_CLEANUP )
    {
        CleanUp( aCommit, GetCurrentScreen() );
    }
    else if( aCleanupFlags == GLOBAL_CLEANUP )
    {
        for( const SCH_SHEET_PATH& sheet : list )
            CleanUp( aCommit, sheet.LastScreen() );
    }

    timer.Stop();
    wxLogTrace( "CONN_PROFILE", "SchematicCleanUp() %0.4f ms", timer.msecs() );

    if( settings.m_IntersheetRefsShow )
        RecomputeIntersheetRefs();

    if( !ADVANCED_CFG::GetCfg().m_IncrementalConnectivity || aCleanupFlags == GLOBAL_CLEANUP
        || aLastChangeList == nullptr || ConnectionGraph()->IsMinor() )
    {
        // Clear all resolved netclass caches in case labels have changed
        m_project->GetProjectFile().NetSettings()->ClearAllCaches();

        // Update all rule areas so we can cascade implied connectivity changes
        std::unordered_set<SCH_SCREEN*> all_screens;

        for( const SCH_SHEET_PATH& path : list )
            all_screens.insert( path.LastScreen() );

        SCH_RULE_AREA::UpdateRuleAreasInScreens( all_screens, aSchView );

        // Recalculate all connectivity
        ConnectionGraph()->Recalculate( list, true, aChangedItemHandler, aProgressReporter );
    }
    else
    {
        struct CHANGED_ITEM
        {
            SCH_ITEM*   item;
            SCH_ITEM*   linked_item;
            SCH_SCREEN* screen;
        };

        // Final change sets
        std::set<SCH_ITEM*>                            changed_items;
        std::set<VECTOR2I>                             pts;
        std::set<std::pair<SCH_SHEET_PATH, SCH_ITEM*>> item_paths;

        // Working change sets
        std::unordered_set<SCH_SCREEN*>                  changed_screens;
        std::set<std::pair<SCH_RULE_AREA*, SCH_SCREEN*>> changed_rule_areas;
        std::vector<CHANGED_ITEM>                        changed_connectable_items;

        // Lambda to add an item to the connectivity update sets
        auto addItemToChangeSet = [&changed_items, &pts, &item_paths]( CHANGED_ITEM itemData )
        {
            std::vector<SCH_SHEET_PATH>& paths = itemData.screen->GetClientSheetPaths();

            std::vector<VECTOR2I> tmp_pts = itemData.item->GetConnectionPoints();
            pts.insert( tmp_pts.begin(), tmp_pts.end() );
            changed_items.insert( itemData.item );

            for( SCH_SHEET_PATH& path : paths )
                item_paths.insert( std::make_pair( path, itemData.item ) );

            if( !itemData.linked_item || !itemData.linked_item->IsConnectable() )
                return;

            tmp_pts = itemData.linked_item->GetConnectionPoints();
            pts.insert( tmp_pts.begin(), tmp_pts.end() );
            changed_items.insert( itemData.linked_item );

            // We have to directly add the pins here because the link may not exist on the schematic
            // anymore and so won't be picked up by GetScreen()->Items().Overlapping() below.
            if( SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( itemData.linked_item ) )
            {
                std::vector<SCH_PIN*> pins = symbol->GetPins();
                changed_items.insert( pins.begin(), pins.end() );
            }

            for( SCH_SHEET_PATH& path : paths )
                item_paths.insert( std::make_pair( path, itemData.linked_item ) );
        };

        // Get all changed connectable items and determine all changed screens
        for( unsigned ii = 0; ii < aLastChangeList->GetCount(); ++ii )
        {
            switch( aLastChangeList->GetPickedItemStatus( ii ) )
            {
            // Only care about changed, new, and deleted items, the other
            // cases are not connectivity-related
            case UNDO_REDO::CHANGED:
            case UNDO_REDO::NEWITEM:
            case UNDO_REDO::DELETED: break;

            default: continue;
            }

            SCH_ITEM* item = dynamic_cast<SCH_ITEM*>( aLastChangeList->GetPickedItem( ii ) );

            if( item )
            {
                SCH_SCREEN* screen = static_cast<SCH_SCREEN*>( aLastChangeList->GetScreenForItem( ii ) );
                changed_screens.insert( screen );

                if( item->Type() == SCH_RULE_AREA_T )
                {
                    SCH_RULE_AREA* ruleArea = static_cast<SCH_RULE_AREA*>( item );
                    changed_rule_areas.insert( { ruleArea, screen } );
                }
                else if( item->IsConnectable() )
                {
                    SCH_ITEM* linked_item = dynamic_cast<SCH_ITEM*>( aLastChangeList->GetPickedItemLink( ii ) );
                    changed_connectable_items.push_back( { item, linked_item, screen } );
                }
            }
        }

        // Update rule areas in changed screens to propagate any directive connectivity changes
        std::vector<std::pair<SCH_RULE_AREA*, SCH_SCREEN*>> forceUpdateRuleAreas =
                SCH_RULE_AREA::UpdateRuleAreasInScreens( changed_screens, aSchView );

        std::for_each( forceUpdateRuleAreas.begin(), forceUpdateRuleAreas.end(),
                       [&]( std::pair<SCH_RULE_AREA*, SCH_SCREEN*>& updatedRuleArea )
                       {
                           changed_rule_areas.insert( updatedRuleArea );
                       } );

        // If a SCH_RULE_AREA was changed, we need to add all past and present contained items to
        // update their connectivity
        std::map<KIID, EDA_ITEM*> itemMap;
        list.FillItemMap( itemMap );

        auto addPastAndPresentContainedItems = [&]( SCH_RULE_AREA* changedRuleArea, SCH_SCREEN* screen )
        {
            for( const KIID& pastItem : changedRuleArea->GetPastContainedItems() )
            {
                if( itemMap.contains( pastItem ) )
                    addItemToChangeSet( { static_cast<SCH_ITEM*>( itemMap[pastItem] ), nullptr, screen } );
            }

            for( SCH_ITEM* containedItem : changedRuleArea->GetContainedItems() )
                addItemToChangeSet( { containedItem, nullptr, screen } );
        };

        for( const auto& [changedRuleArea, screen] : changed_rule_areas )
            addPastAndPresentContainedItems( changedRuleArea, screen );

        // Add all changed items, and associated items, to the change set
        for( CHANGED_ITEM& changed_item_data : changed_connectable_items )
        {
            addItemToChangeSet( changed_item_data );

            // If a SCH_DIRECTIVE_LABEL was changed which is attached to a SCH_RULE_AREA, we need
            // to add the contained items to the change set to force update of their connectivity
            if( changed_item_data.item->Type() == SCH_DIRECTIVE_LABEL_T )
            {
                const std::vector<VECTOR2I> labelConnectionPoints = changed_item_data.item->GetConnectionPoints();

                auto candidateRuleAreas = changed_item_data.screen->Items().Overlapping(
                        SCH_RULE_AREA_T, changed_item_data.item->GetBoundingBox() );

                for( SCH_ITEM* candidateRuleArea : candidateRuleAreas )
                {
                    SCH_RULE_AREA*      ruleArea = static_cast<SCH_RULE_AREA*>( candidateRuleArea );
                    std::vector<SHAPE*> borderShapes = ruleArea->MakeEffectiveShapes( true );

                    if( ruleArea->GetPolyShape().CollideEdge( labelConnectionPoints[0], nullptr, 5 ) )
                        addPastAndPresentContainedItems( ruleArea, changed_item_data.screen );
                }
            }
        }

        for( const VECTOR2I& pt : pts )
        {
            for( SCH_ITEM* item : GetCurrentScreen()->Items().Overlapping( pt ) )
            {
                // Leave this check in place.  Overlapping items are not necessarily connectable.
                if( !item->IsConnectable() )
                    continue;

                if( item->Type() == SCH_LINE_T )
                {
                    if( item->HitTest( pt ) )
                        changed_items.insert( item );
                }
                else if( item->Type() == SCH_SYMBOL_T && item->IsConnected( pt ) )
                {
                    SCH_SYMBOL*           symbol = static_cast<SCH_SYMBOL*>( item );
                    std::vector<SCH_PIN*> pins = symbol->GetPins();

                    changed_items.insert( pins.begin(), pins.end() );
                }
                else if( item->Type() == SCH_SHEET_T )
                {
                    SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );

                    wxCHECK2( sheet, continue );

                    std::vector<SCH_SHEET_PIN*> sheetPins = sheet->GetPins();
                    changed_items.insert( sheetPins.begin(), sheetPins.end() );
                }
                else
                {
                    if( item->IsConnected( pt ) )
                        changed_items.insert( item );
                }
            }
        }

        std::set<std::pair<SCH_SHEET_PATH, SCH_ITEM*>> all_items =
                ConnectionGraph()->ExtractAffectedItems( changed_items );

        all_items.insert( item_paths.begin(), item_paths.end() );

        CONNECTION_GRAPH new_graph( this );

        new_graph.SetLastCodes( ConnectionGraph() );

        std::shared_ptr<NET_SETTINGS> netSettings = m_project->GetProjectFile().NetSettings();

        std::set<wxString> affectedNets;

        for( auto& [path, item] : all_items )
        {
            wxCHECK2( item, continue );
            item->SetConnectivityDirty();
            SCH_CONNECTION* conn = item->Connection();

            if( conn )
                affectedNets.insert( conn->Name() );
        }

        // For label items, also capture the old net name from the linked item (original state).
        // This ensures cache is cleared for both old and new net names when a label is renamed.
        for( const CHANGED_ITEM& changedItem : changed_connectable_items )
        {
            if( SCH_LABEL_BASE* label = dynamic_cast<SCH_LABEL_BASE*>( changedItem.item ) )
            {
                const wxString& driverName = label->GetCachedDriverName();

                if( !driverName.IsEmpty() )
                    affectedNets.insert( driverName );
            }

            if( SCH_LABEL_BASE* linkedLabel = dynamic_cast<SCH_LABEL_BASE*>( changedItem.linked_item ) )
            {
                const wxString& driverName = linkedLabel->GetCachedDriverName();

                if( !driverName.IsEmpty() )
                    affectedNets.insert( driverName );
            }
        }

        // Reset resolved netclass cache for this connection
        for( const wxString& netName : affectedNets )
            netSettings->ClearCacheForNet( netName );

        new_graph.Recalculate( list, false, aChangedItemHandler, aProgressReporter );
        ConnectionGraph()->Merge( new_graph );
    }

    if( !localCommit.Empty() )
        localCommit.Push( _( "Schematic Cleanup" ) );
}


void SCHEMATIC::CreateDefaultScreens()
{
    Reset();

    ensureVirtualRoot();

    // Create the actual first top-level sheet
    SCH_SHEET*  rootSheet = new SCH_SHEET( this );
    SCH_SCREEN* rootScreen = new SCH_SCREEN( this );

    const_cast<KIID&>( rootSheet->m_Uuid ) = rootScreen->GetUuid();
    rootSheet->SetScreen( rootScreen );
    rootScreen->SetFileName( "untitled.kicad_sch" ); // Set default filename to avoid conflicts
    rootScreen->SetPageNumber( wxT( "1" ) );

    // Don't leave root page number empty
    SCH_SHEET_PATH rootSheetPath;
    rootSheetPath.push_back( m_rootSheet );
    rootSheetPath.push_back( rootSheet );
    rootSheetPath.SetPageNumber( wxT( "1" ) );

    SetTopLevelSheets( { rootSheet } );
}


std::vector<SCH_SHEET*> SCHEMATIC::GetTopLevelSheets() const
{
    return m_topLevelSheets;
}

SCH_SHEET* SCHEMATIC::GetTopLevelSheet( int aIndex ) const
{
    if( aIndex < 0 )
        return nullptr;

    size_t index = static_cast<size_t>( aIndex );

    if( index >= m_topLevelSheets.size() )
        return nullptr;

    return m_topLevelSheets[index];
}

void SCHEMATIC::AddTopLevelSheet( SCH_SHEET* aSheet )
{
    wxCHECK_RET( aSheet, wxS( "Cannot add null sheet!" ) );
    wxCHECK_RET( aSheet->GetScreen(), wxS( "Cannot add virtual root as top-level sheet!" ) );

    ensureVirtualRoot();

    // Set parent to virtual root
    aSheet->SetParent( m_rootSheet );

    // Add to the virtual root's screen if it exists
    if( m_rootSheet->GetScreen() )
    {
        m_rootSheet->GetScreen()->Append( aSheet );
    }

    // Add to our list
    m_topLevelSheets.push_back( aSheet );

    ensureCurrentSheetIsTopLevel();
    rebuildHierarchyState( true );
}

bool SCHEMATIC::RemoveTopLevelSheet( SCH_SHEET* aSheet )
{
    auto it = std::find( m_topLevelSheets.begin(), m_topLevelSheets.end(), aSheet );

    if( it == m_topLevelSheets.end() )
        return false;

    if( m_topLevelSheets.size() == 1 )
        return false;

    m_topLevelSheets.erase( it );

    if( m_rootSheet && m_rootSheet->GetScreen() )
        m_rootSheet->GetScreen()->Items().remove( aSheet );

    // If we're removing the current sheet, switch to another one
    if( !m_currentSheet->empty() && m_currentSheet->at( 0 ) == aSheet )
    {
        m_currentSheet->clear();
        if( !m_topLevelSheets.empty() )
        {
            m_currentSheet->push_back( m_topLevelSheets[0] );
        }
    }

    rebuildHierarchyState( true );
    return true;
}


bool SCHEMATIC::IsTopLevelSheet( const SCH_SHEET* aSheet ) const
{
    return std::find( m_topLevelSheets.begin(), m_topLevelSheets.end(), aSheet ) != m_topLevelSheets.end();
}


SCH_SHEET_LIST SCHEMATIC::BuildSheetListSortedByPageNumbers() const
{
    SCH_SHEET_LIST hierarchy;

    wxLogTrace( traceSchSheetPaths, "BuildSheetListSortedByPageNumbers: %zu top-level sheets",
                m_topLevelSheets.size() );

    // Can't build hierarchy without top-level sheets
    if( m_topLevelSheets.empty() )
        return hierarchy;

    // For each top-level sheet, build its hierarchy
    for( SCH_SHEET* sheet : m_topLevelSheets )
    {
        if( sheet )
        {
            wxLogTrace( traceSchSheetPaths, "  Top-level sheet: '%s' (UUID=%s, isVirtualRoot=%d)", sheet->GetName(),
                        sheet->m_Uuid.AsString(), sheet->m_Uuid == niluuid ? 1 : 0 );

            // Build the sheet list for this top-level sheet
            SCH_SHEET_LIST sheetList;
            sheetList.BuildSheetList( sheet, false );

            // Add all sheets from this top-level sheet's hierarchy
            for( const SCH_SHEET_PATH& path : sheetList )
            {
                hierarchy.push_back( path );
            }
        }
    }

    hierarchy.SortByPageNumbers();

    return hierarchy;
}


SCH_SHEET_LIST SCHEMATIC::BuildUnorderedSheetList() const
{
    SCH_SHEET_LIST sheets;

    // For each top-level sheet, build its hierarchy
    for( SCH_SHEET* sheet : m_topLevelSheets )
    {
        if( sheet )
        {
            SCH_SHEET_LIST sheetList;
            sheetList.BuildSheetList( sheet, false );

            // Add all sheets from this top-level sheet's hierarchy
            for( const SCH_SHEET_PATH& path : sheetList )
            {
                sheets.push_back( path );
            }
        }
    }

    return sheets;
}


wxArrayString SCHEMATIC::GetVariantNamesForUI() const
{
    wxArrayString variantNames;

    // There is no default variant name.  This is just a place holder for UI controls.
    variantNames.Add( GetDefaultVariantName() );

    for( const wxString& name : m_variantNames )
        variantNames.Add( name );

    variantNames.Sort( SortVariantNames );

    return variantNames;
}


wxString SCHEMATIC::GetCurrentVariant() const
{
    if( m_currentVariant.IsEmpty() || ( m_currentVariant == GetDefaultVariantName() ) )
        return wxEmptyString;

    return m_currentVariant;
}


void SCHEMATIC::SetCurrentVariant( const wxString& aVariantName )
{
    // Internally an empty string is the default variant.  Set to default if the variant name doesn't exist.
    if( ( aVariantName == GetDefaultVariantName() ) || !m_variantNames.contains( aVariantName ) )
        m_currentVariant = wxEmptyString;
    else
        m_currentVariant = aVariantName;
}


void SCHEMATIC::AddVariant( const wxString& aVariantName )
{
    m_variantNames.emplace( aVariantName );

    // Ensure the variant is registered in the project file
    auto& descriptions = Settings().m_VariantDescriptions;

    if( descriptions.find( aVariantName ) == descriptions.end() )
        descriptions[aVariantName] = wxEmptyString;
}


void SCHEMATIC::DeleteVariant( const wxString& aVariantName, SCH_COMMIT* aCommit )
{
    wxCHECK( m_rootSheet, /* void */ );

    SCH_SCREENS allScreens( m_rootSheet );

    allScreens.DeleteVariant( aVariantName, aCommit );

    m_variantNames.erase( aVariantName );
    Settings().m_VariantDescriptions.erase( aVariantName );
}


void SCHEMATIC::RenameVariant( const wxString& aOldName, const wxString& aNewName,
                               SCH_COMMIT* aCommit )
{
    wxCHECK( m_rootSheet, /* void */ );
    wxCHECK( !aOldName.IsEmpty() && !aNewName.IsEmpty(), /* void */ );
    wxCHECK( m_variantNames.contains( aOldName ), /* void */ );

    m_variantNames.erase( aOldName );
    m_variantNames.insert( aNewName );

    auto& descriptions = Settings().m_VariantDescriptions;

    if( descriptions.count( aOldName ) )
    {
        descriptions[aNewName] = descriptions[aOldName];
        descriptions.erase( aOldName );
    }

    if( m_currentVariant == aOldName )
        m_currentVariant = aNewName;

    SCH_SCREENS allScreens( m_rootSheet );
    allScreens.RenameVariant( aOldName, aNewName, aCommit );
}


void SCHEMATIC::CopyVariant( const wxString& aSourceVariant, const wxString& aNewVariant,
                             SCH_COMMIT* aCommit )
{
    wxCHECK( m_rootSheet, /* void */ );
    wxCHECK( !aSourceVariant.IsEmpty() && !aNewVariant.IsEmpty(), /* void */ );
    wxCHECK( m_variantNames.contains( aSourceVariant ), /* void */ );
    wxCHECK( !m_variantNames.contains( aNewVariant ), /* void */ );

    AddVariant( aNewVariant );

    auto& descriptions = Settings().m_VariantDescriptions;

    if( descriptions.count( aSourceVariant ) )
        descriptions[aNewVariant] = descriptions[aSourceVariant];

    SCH_SCREENS allScreens( m_rootSheet );
    allScreens.CopyVariant( aSourceVariant, aNewVariant, aCommit );
}


wxString SCHEMATIC::GetVariantDescription( const wxString& aVariantName ) const
{
    const auto& descriptions = Settings().m_VariantDescriptions;
    auto it = descriptions.find( aVariantName );

    if( it != descriptions.end() )
        return it->second;

    return wxEmptyString;
}


void SCHEMATIC::SetVariantDescription( const wxString& aVariantName, const wxString& aDescription )
{
    auto& descriptions = Settings().m_VariantDescriptions;

    if( aDescription.IsEmpty() )
        descriptions.erase( aVariantName );
    else
        descriptions[aVariantName] = aDescription;
}


void SCHEMATIC::LoadVariants()
{
    if( m_rootSheet->GetScreen() )
    {
        SCH_SCREENS        screens( m_rootSheet );
        std::set<wxString> variantNames = screens.GetVariantNames();
        m_variantNames.insert( variantNames.begin(), variantNames.end() );

        // Register any unknown variants to the project file with empty descriptions
        auto& descriptions = Settings().m_VariantDescriptions;

        for( const wxString& name : variantNames )
        {
            if( descriptions.find( name ) == descriptions.end() )
                descriptions[name] = wxEmptyString;
        }
    }
}


void SCHEMATIC::SaveToHistory( const wxString& aProjectPath, std::vector<wxString>& aFiles )
{
    wxString projPath = m_project->GetProjectPath();

    if( projPath.IsEmpty() )
        return; // no project yet

    // Verify we're saving for the correct project
    if( !projPath.IsSameAs( aProjectPath ) )
    {
        wxLogTrace( traceAutoSave, wxS( "[history] sch saver skipping - project path mismatch: %s vs %s" ), projPath,
                    aProjectPath );
        return;
    }

    // Ensure project path has trailing separator for StartsWith tests & Mid calculations.
    if( !projPath.EndsWith( wxFILE_SEP_PATH ) )
        projPath += wxFILE_SEP_PATH;

    wxFileName historyRoot( projPath, wxEmptyString );
    historyRoot.AppendDir( wxS( ".history" ) );
    wxString historyRootPath = historyRoot.GetPath();

    // Iterate full schematic hierarchy (all sheets & their screens).
    SCH_SHEET_LIST sheetList = Hierarchy();

    // Acquire plugin once.
    IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );

    for( const SCH_SHEET_PATH& path : sheetList )
    {
        SCH_SHEET*  sheet = path.Last();
        SCH_SCREEN* screen = path.LastScreen();

        if( !sheet || !screen )
            continue;

        wxFileName abs = m_project->AbsolutePath( screen->GetFileName() );

        if( !abs.IsOk() )
            continue; // no filename

        wxString absPath = abs.GetFullPath();

        if( absPath.IsEmpty() || !absPath.StartsWith( projPath ) )
            continue; // external / unsaved subsheet

        wxString rel = absPath.Mid( projPath.length() );

        // Destination mirrors project-relative path under .history
        wxFileName dst( rel );

        if( dst.IsRelative() )
            dst.MakeAbsolute( historyRootPath );
        else
            dst.SetPath( historyRootPath );

        // Ensure destination directory exists
        wxFileName dstDir( dst );
        dstDir.SetFullName( wxEmptyString );

        if( !dstDir.DirExists() )
            wxFileName::Mkdir( dstDir.GetPath(), 0777, wxPATH_MKDIR_FULL );

        try
        {
            pi->SaveSchematicFile( dst.GetFullPath(), sheet, this );
            aFiles.push_back( dst.GetFullPath() );
            wxLogTrace( traceAutoSave, wxS( "[history] sch saver exported sheet '%s' -> '%s'" ), absPath,
                        dst.GetFullPath() );
        }
        catch( const IO_ERROR& ioe )
        {
            wxLogTrace( traceAutoSave, wxS( "[history] sch saver export failed for '%s': %s" ), absPath,
                        wxString::FromUTF8( ioe.What() ) );
        }
    }
}
