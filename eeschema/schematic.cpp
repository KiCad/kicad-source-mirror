/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2023, 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <bus_alias.h>
#include <commit.h>
#include <connection_graph.h>
#include <core/ignore.h>
#include <core/kicad_algo.h>
#include <ee_collectors.h>
#include <erc/erc_settings.h>
#include <font/outline_font.h>
#include <netlist_exporter_spice.h>
#include <project.h>
#include <project/net_settings.h>
#include <project/project_file.h>
#include <schematic.h>
#include <sch_junction.h>
#include <sch_label.h>
#include <sch_line.h>
#include <sch_marker.h>
#include <sch_screen.h>
#include <sim/spice_settings.h>
#include <sim/spice_value.h>

#include <wx/log.h>

bool SCHEMATIC::m_IsSchematicExists = false;

SCHEMATIC::SCHEMATIC( PROJECT* aPrj ) :
          EDA_ITEM( nullptr, SCHEMATIC_T ),
          m_project( nullptr ),
          m_rootSheet( nullptr )
{
    m_currentSheet    = new SCH_SHEET_PATH();
    m_connectionGraph = new CONNECTION_GRAPH( this );
    m_IsSchematicExists = true;

    SetProject( aPrj );

    PROPERTY_MANAGER::Instance().RegisterListener( TYPE_HASH( SCH_FIELD ),
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
                        case VALUE_FIELD:
                        {
                            if( aCommit )
                                aCommit->Modify( otherUnit, sheet.LastScreen() );

                            otherUnit->SetValueFieldText( newValue );
                            break;
                        }

                        case FOOTPRINT_FIELD:
                        {
                            if( aCommit )
                                aCommit->Modify( otherUnit, sheet.LastScreen() );

                            otherUnit->SetFootprintFieldText( newValue );
                            break;
                        }

                        case DATASHEET_FIELD:
                        {
                            if( aCommit )
                                aCommit->Modify( otherUnit, sheet.LastScreen() );

                            otherUnit->GetField( DATASHEET_FIELD )->SetText( newValue );
                            break;
                        }

                        default:
                            break;
                        }
                    }
                }
            } );
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
    if( m_project )
    {
        PROJECT_FILE& project = m_project->GetProjectFile();

        // d'tor will save settings to file
        delete project.m_ErcSettings;
        project.m_ErcSettings = nullptr;

        // d'tor will save settings to file
        delete project.m_SchematicSettings;
        project.m_SchematicSettings = nullptr;

        m_project = nullptr; // clear the project, so we don't do this again when setting a new one
    }

    delete m_rootSheet;

    m_rootSheet = nullptr;

    m_connectionGraph->Reset();
    m_currentSheet->clear();
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
        PROJECT_FILE& project       = m_project->GetProjectFile();
        project.m_ErcSettings       = new ERC_SETTINGS( &project, "erc" );
        project.m_SchematicSettings = new SCHEMATIC_SETTINGS( &project, "schematic" );

        project.m_SchematicSettings->LoadFromFile();
        project.m_SchematicSettings->m_NgspiceSettings->LoadFromFile();
        project.m_ErcSettings->LoadFromFile();
    }
}


void SCHEMATIC::SetRoot( SCH_SHEET* aRootSheet )
{
    wxCHECK_RET( aRootSheet, wxS( "Call to SetRoot with null SCH_SHEET!" ) );

    m_rootSheet = aRootSheet;

    m_currentSheet->clear();
    m_currentSheet->push_back( m_rootSheet );

    m_hierarchy = BuildSheetListSortedByPageNumbers();
    m_connectionGraph->Reset();
}


SCH_SCREEN* SCHEMATIC::RootScreen() const
{
    return IsValid() ? m_rootSheet->GetScreen() : nullptr;
}


SCH_SHEET_LIST SCHEMATIC::Hierarchy() const
{
    wxCHECK( !m_hierarchy.empty(), m_hierarchy );

    return m_hierarchy;
}


void SCHEMATIC::RefreshHierarchy()
{
    m_hierarchy = BuildSheetListSortedByPageNumbers();
}


void SCHEMATIC::GetContextualTextVars( wxArrayString* aVars ) const
{
    auto add =
            [&]( const wxString& aVar )
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

    if( !CurrentSheet().empty() )
        CurrentSheet().LastScreen()->GetTitleBlock().GetContextualTextVars( aVars );

    for( std::pair<wxString, wxString> entry : Prj().GetTextVars() )
        add( entry.first );
}


bool SCHEMATIC::ResolveTextVar( const SCH_SHEET_PATH* aSheetPath, wxString* token,
                                int aDepth ) const
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
        *token = Prj().GetProjectName();
        return true;
    }

    if( aSheetPath->LastScreen()->GetTitleBlock().TextVarResolver( token, m_project ) )
        return true;

    if( Prj().TextVarResolver( token ) )
        return true;

    return false;
}


wxString SCHEMATIC::GetFileName() const
{
    return IsValid() ? m_rootSheet->GetScreen()->GetFileName() : wxString( wxEmptyString );
}


SCHEMATIC_SETTINGS& SCHEMATIC::Settings() const
{
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
    ERC_SETTINGS&  settings  = ErcSettings();

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

            if(    settingsKey != wxT( "pin_to_pin" )
                && settingsKey != wxT( "hier_label_mismatch" )
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
    for( const SCH_SHEET_PATH& sheet : Hierarchy() )
    {
        for( const std::shared_ptr<BUS_ALIAS>& alias : sheet.LastScreen()->GetBusAliases() )
        {
            if( alias->GetName() == aLabel )
                return alias;
        }
    }

    return nullptr;
}


std::set<wxString> SCHEMATIC::GetNetClassAssignmentCandidates()
{
    std::set<wxString> names;

    for( const auto& [ key, subgraphList ] : m_connectionGraph->GetNetMap() )
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
    SCH_SHEET_PATH sheetPath;
    SCH_ITEM*      refItem = GetItem( KIID( ref ), &sheetPath );

    if( refItem && refItem->Type() == SCH_SYMBOL_T )
    {
        SCH_SYMBOL* refSymbol = static_cast<SCH_SYMBOL*>( refItem );

        if( refSymbol->ResolveTextVar( &sheetPath, &remainder, aDepth + 1 ) )
            *token = remainder;
        else
            *token = refSymbol->GetRef( &sheetPath, true ) + wxS( ":" ) + remainder;

        return true;    // Cross-reference is resolved whether or not the actual textvar was
    }
    else if( refItem && refItem->Type() == SCH_SHEET_T )
    {
        SCH_SHEET* refSheet = static_cast<SCH_SHEET*>( refItem );

        sheetPath.push_back( refSheet );

        if( refSheet->ResolveTextVar( &sheetPath, &remainder, aDepth + 1 ) )
            *token = remainder;

        return true;    // Cross-reference is resolved whether or not the actual textvar was
    }

    return false;
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
        if( aSource[i] == '$' && i + 1 < sourceLen && aSource[i+1] == '{' )
        {
            wxString token;
            bool     isCrossRef = false;
            int      nesting = 0;

            for( i = i + 2; i < sourceLen; ++i )
            {
                if( aSource[i] == '{'
                        && ( aSource[i-1] == '_' || aSource[i-1] == '^' || aSource[i-1] == '~' ) )
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
                    SCH_SYMBOL* refSymbol = references[ jj ].GetSymbol();

                    if( ref == refSymbol->GetRef( &references[ jj ].GetSheetPath(), true ) )
                    {
                        token = refSymbol->m_Uuid.AsString() + wxS( ":" ) + remainder;
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
        if( aSource[i] == '$' && i + 1 < sourceLen && aSource[i+1] == '{' )
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
                wxString remainder;
                wxString ref = token.BeforeFirst( ':', &remainder );

                SCH_SHEET_PATH refSheetPath;
                SCH_ITEM* refItem = GetItem( KIID( ref ), &refSheetPath );

                if( refItem && refItem->Type() == SCH_SYMBOL_T )
                {
                    SCH_SYMBOL* refSymbol = static_cast<SCH_SYMBOL*>( refItem );
                    token = refSymbol->GetRef( &refSheetPath, true ) + wxS( ":" ) + remainder;
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

    wxFileName rootFn( CurrentSheet().at( 0 )->GetFileName() );
    wxString   filename = rootFn.GetName();

    for( unsigned i = 1; i < CurrentSheet().size(); i++ )
        filename += wxT( "-" ) + CurrentSheet().at( i )->GetName();

    return filename;
}


void SCHEMATIC::SetSheetNumberAndCount()
{
    SCH_SCREEN* screen;
    SCH_SCREENS s_list( Root() );

    // Set the sheet count, and the sheet number (1 for root sheet)
    int              sheet_count = Root().CountSheets();
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


void SCHEMATIC::RecomputeIntersheetRefs( const std::function<void( SCH_GLOBALLABEL* )>& aItemCallback )
{
    std::map<wxString, std::set<int>>& pageRefsMap = GetPageRefsMap();

    pageRefsMap.clear();

    for( const SCH_SHEET_PATH& sheet : Hierarchy() )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_GLOBAL_LABEL_T ) )
        {
            SCH_GLOBALLABEL* global = static_cast<SCH_GLOBALLABEL*>( item );
            wxString         resolvedLabel = global->GetShownText( &sheet, false );

            pageRefsMap[ resolvedLabel ].insert( sheet.GetVirtualPageNumber() );
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
            aItemCallback( globalLabel );
        }
    }
}


wxString SCHEMATIC::GetOperatingPoint( const wxString& aNetName, int aPrecision,
                                       const wxString& aRange )
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


void SCHEMATIC::FixupJunctions()
{
    SCH_SCREENS screens( Root() );

    for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
    {
        std::deque<EDA_ITEM*> allItems;

        for( auto item : screen->Items() )
            allItems.push_back( item );

        // Add missing junctions and breakup wires as needed
        for( const VECTOR2I& point : screen->GetNeededJunctions( allItems ) )
        {
            SCH_JUNCTION* junction = new SCH_JUNCTION( point );
            screen->Append( junction );

            // Breakup wires
            for( SCH_LINE* wire : screen->GetBusesAndWires( point, true ) )
            {
                SCH_LINE* newSegment = wire->BreakAt( point );
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
                ercSettings.m_ErcExclusionComments[ serialized ] = marker->GetComment();
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
        ignore_unused( sheetList.GetItem( marker->GetRCItem()->GetMainItemID(), &errorPath ) );

        if( errorPath.LastScreen() )
            errorPath.LastScreen()->Append( marker );
        else
            RootScreen()->Append( marker );
    }
}


EMBEDDED_FILES* SCHEMATIC::GetEmbeddedFiles()
{
    return static_cast<EMBEDDED_FILES*>( this );
}


const EMBEDDED_FILES* SCHEMATIC::GetEmbeddedFiles() const
{
    return static_cast<const EMBEDDED_FILES*>( this );
}


void SCHEMATIC::EmbedFonts()
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
    SCH_SCREENS screens( m_rootSheet );

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
