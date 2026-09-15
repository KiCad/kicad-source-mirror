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

#include "conn_facade.h"
#include "conn_netchain_input.h"
#include "conn_netchain_manager.h"
#include "conn_text.h"
#include <algorithm>
#include <array>
#include <iterator>
#include <ranges>
#include <span>
#include <limits>
#include <stdexcept>
#include <utility>
#include <schematic.h>
#include <bus_alias.h>
#include <sch_screen.h>
#include <sch_sheet_path.h>
#include <sch_sheet.h>
#include <sch_line.h>
#include <sch_bus_entry.h>
#include <sch_label.h>
#include <sch_pin.h>
#include <sch_rule_area.h>
#include <project/project_file.h>
#include <project/net_settings.h>
#include <wx/filefn.h>
#include <text_eval/text_eval_vcs.h>
#include <wx/thread.h>
#include <drawing_sheet/ds_data_model.h>

namespace SCH_CONNECTIVITY
{
namespace
{
std::optional<TEXT_EVAL_VCS::CONTEXT_PATH_SCOPE> sourceProjectContext( const SCHEMATIC& aSchematic )
{
    if( aSchematic.IsValid() )
    {
        if( const wxString path = aSchematic.Project().GetProjectPath(); !path.IsEmpty() )
            return std::optional<TEXT_EVAL_VCS::CONTEXT_PATH_SCOPE>( std::in_place, path );
    }

    return std::nullopt;
}

struct INSTANCE_TEXT_CONTEXT
{
    KIID_PATH             path;
    SCREEN_ID             screen = 0;
    wxString              page;
    wxString              humanPath;
    wxString              sheetName;
    wxString              filename;
    std::vector<wxString> title;
    wxString              paper;
    VECTOR2D              pageSize;
    bool                  portrait = false;
    bool                  operator==( const INSTANCE_TEXT_CONTEXT& ) const = default;
};

struct VARIANT_TEXT_CONTEXT
{
    std::map<wxString, wxString> fields;
    std::array<bool, 5>          attributes{};
    bool                         operator==( const VARIANT_TEXT_CONTEXT& ) const = default;
};

struct SHEET_TEXT_CONTEXT
{
    std::vector<std::tuple<FIELD_T, wxString, wxString>> fields;
    std::array<bool, 5> attributes{};
    std::map<KIID_PATH, std::map<wxString, VARIANT_TEXT_CONTEXT>> variants;
    bool operator==( const SHEET_TEXT_CONTEXT& ) const = default;
};

struct MODEL_TEXT_CONTEXT
{
    wxString                                projectFile;
    wxString                                projectName;
    wxString                                rootFile;
    wxString                                workingDirectory;
    wxString                                drawingLayout;
    bool                                    allowEmptyLayout = false;
    wxString                                vcsContext;
    bool                                    vcsContextIsFile = false;
    wxString                                variant;
    wxString                                variantDescription;
    int                                     sheetCount = 0;
    std::map<wxString, wxString>            variables;
    std::vector<wxString>                   templateFields;
    std::vector<INSTANCE_TEXT_CONTEXT>      instances;
    std::map<KIID_PATH, SHEET_TEXT_CONTEXT> sheets;
    bool                                    operator==( const MODEL_TEXT_CONTEXT& ) const = default;
};

std::optional<MODEL_TEXT_CONTEXT> CaptureTextContext( SCHEMATIC& aSchematic, const SCH_SHEET_LIST& aPaths )
{
    MODEL_TEXT_CONTEXT result;

    if( !aSchematic.IsValid() )
        return std::nullopt;

    PROJECT& project = aSchematic.Project();
    result.projectFile = project.GetProjectFullName();
    result.projectName = project.GetProjectName();
    result.rootFile = aSchematic.GetFileName();
    result.workingDirectory = wxGetCwd();

    if( project.GetProjectPath().empty() )
    {
        result.vcsContext = TEXT_EVAL_VCS::GetContextPath();
        result.vcsContextIsFile = TEXT_EVAL_VCS::GetContextIsFile();
    }

    DS_DATA_MODEL& layout = DS_DATA_MODEL::GetTheInstance();
    layout.SaveInString( &result.drawingLayout );
    result.allowEmptyLayout = layout.VoidListAllowed();
    result.variant = aSchematic.GetCurrentVariant();
    result.variantDescription = aSchematic.GetVariantDescription( result.variant );
    result.sheetCount = aSchematic.Root().CountSheets();
    result.variables = project.GetTextVars();

    for( const auto& field : project.GetProjectFile().m_TemplateFieldNames.GetResolvedTemplateFieldNames() )
        result.templateFields.push_back( field.m_Name );

    for( const SCH_SHEET_PATH& path : aPaths )
    {
        SCH_SCREEN* screen = path.LastScreen();

        if( !screen )
            continue;

        const TITLE_BLOCK& title = screen->GetTitleBlock();
        INSTANCE_TEXT_CONTEXT instance{ path.Path(), screen->ConnectivityId(),
                                        path.GetPageNumber(), path.PathHumanReadable(), path.Last()->GetName(),
                                        screen->GetFileName(),
                                        { title.GetTitle(), title.GetDate(), title.GetRevision(), title.GetCompany() },
                                        screen->GetPageSettings().GetTypeAsString(),
                                        screen->GetPageSettings().GetSizeMils(),
                                        screen->GetPageSettings().IsPortrait() };

        for( int comment = 0; comment < 9; ++comment )
            instance.title.push_back( title.GetComment( comment ) );

        result.instances.push_back( std::move( instance ) );
        KIID_PATH sheetPath;

        for( size_t depth = 0; depth < path.size(); ++depth )
        {
            const SCH_SHEET* sheet = path.at( depth );
            sheetPath.push_back( sheet->m_Uuid );

            if( result.sheets.contains( sheetPath ) )
                continue;

            SHEET_TEXT_CONTEXT source;
            source.attributes = { sheet->GetDNP(), sheet->GetExcludedFromSim(), sheet->GetExcludedFromBOM(),
                                  sheet->GetExcludedFromBoard(), sheet->GetExcludedFromPosFiles() };

            for( const SCH_FIELD& field : sheet->GetFields() )
                source.fields.emplace_back( field.GetId(), field.GetName( false ), field.GetText() );

            for( const SCH_SHEET_INSTANCE& sheetInstance : sheet->GetInstances() )
            {
                auto& variants = source.variants[sheetInstance.m_Path];

                for( const auto& [name, variant] : sheetInstance.m_Variants )
                {
                    variants.emplace( name, VARIANT_TEXT_CONTEXT{
                            variant.m_Fields,
                            { variant.m_DNP, variant.m_ExcludedFromSim, variant.m_ExcludedFromBOM,
                              variant.m_ExcludedFromBoard, variant.m_ExcludedFromPosFiles } } );
                }
            }

            result.sheets.emplace( sheetPath, std::move( source ) );
        }
    }

    std::ranges::sort( result.instances, {}, &INSTANCE_TEXT_CONTEXT::path );
    return result;
}
} // namespace

struct FACADE_STATE
{
    struct NET_ITEMS
    {
        std::shared_ptr<const COMPONENT_CONTENT> content;
        std::map<INST_ID, std::vector<KIID>>     instances;
    };

    struct TEXT_CHECKS
    {
        std::vector<TEXT_CHECK_FACT>                items;
        std::optional<std::vector<TEXT_CHECK_FACT>> drawingSheet;
    };

    ENGINE   engine;
    uint64_t resetGeneration = 0;
    uint64_t dispatchGeneration = 0;
    uint64_t nextListener = 0;
    std::map<uint64_t, std::function<void( const CHANGE_SET& )>> listeners;
    uint64_t                                                                textEpoch = 0;
    std::optional<MODEL_TEXT_CONTEXT>                                       textContext;
    bool                                                                    publicationApplied = false;
    std::optional<std::map<KIID_PATH, TEXT_CHECKS>>                         textChecks;
    std::optional<std::map<KIID_PATH, std::vector<SIMULATION_MODEL_FACT>>>  simulationModels;
    std::map<INST_ID, std::weak_ptr<const SCH_SCREEN::CONNECTIVITY_SOURCE>> sources;
    std::map<NODE_ID, std::shared_ptr<const NET_ITEMS>>                     netItems;
    unsigned                                                                publicationHolds = 0;

    SCH_SHEET_LIST SourcePaths( SCHEMATIC& aSchematic ) const
    {
        if( !publicationApplied || INPUT_TEXT_SCOPE::Active() )
            throw std::logic_error( "ERC sources require applied connectivity outside input capture" );

        auto paths = aSchematic.BuildSheetListSortedByPageNumbers();

        for( const SCH_SHEET_PATH& path : paths )
        {
            if( !path.LastScreen() )
                continue;

            const auto instance = engine.Keys().FindInstance( path.PathRef() );

            if( !instance || Screen( *instance ) != path.LastScreen() )
                throw std::logic_error( "ERC sources require current connectivity" );
        }

        return paths;
    }

    void RefreshNetItems()
    {
        decltype( netItems ) current;

        for( const auto& [node, component] : engine.Published().Components() )
        {
            const auto old = netItems.find( node );

            if( old != netItems.end() && old->second->content == component.content )
            {
                current.emplace( node, old->second );
                continue;
            }

            auto membership = std::make_shared<NET_ITEMS>();
            membership->content = component.content;

            for( const ITEM_KEY& item : component.content->items )
                membership->instances[item.inst].push_back( item.item );

            for( const SLOT_KEY& slot : component.content->slots )
                membership->instances.try_emplace( slot.bundleDriver.inst );

            current.emplace( node, std::move( membership ) );
        }

        netItems.swap( current );
    }

    std::vector<INST_ID> LiveInstances() const
    {
        std::vector<INST_ID> live;
        live.reserve( sources.size() );

        for( const auto& [instance, source] : sources )
        {
            if( Screen( source ) )
                live.push_back( instance );
        }

        return live;
    }

    bool AnyLive( NODE_ID aNode, std::span<const INST_ID> aLive ) const
    {
        const auto found = netItems.find( aNode );
        return found != netItems.end() && std::any_of( found->second->instances.begin(),
                                                       found->second->instances.end(), [&]( const auto& entry )
        {
            return std::binary_search( aLive.begin(), aLive.end(), entry.first );
        } );
    }

    std::optional<NODE_ID> BusNode( const wxString& aName ) const
    {
        const auto node = engine.Published().FindByName( aName );
        return node && engine.Published().Components().at( *node ).content->kind == KIND::BUNDLE
                       ? node : std::nullopt;
    }

    std::optional<NET_GROUP> Group( NODE_ID aNode, std::span<const INST_ID> aLive,
                                   const std::weak_ptr<const FACADE_STATE>& aState ) const
    {
        const auto component = engine.Published().Components().find( aNode );

        if( component == engine.Published().Components().end() || component->second.name == INVALID_ID )
            return std::nullopt;

        std::vector<INST_ID> instances;

        for( const auto& [instance, items] : netItems.at( aNode )->instances )
        {
            if( std::binary_search( aLive.begin(), aLive.end(), instance ) )
                instances.push_back( instance );
        }

        if( instances.empty() )
            return std::nullopt;

        std::ranges::sort( instances, std::less<>{}, std::bind_front( &SESSION_KEYS::Instance, &engine.Keys() ) );
        NET_GROUP result{ engine.Keys().Name( component->second.name ), {} };
        result.instances.reserve( instances.size() );

        for( INST_ID instance : instances )
            result.instances.push_back( NET_VIEW( aState, aNode, instance ) );

        return result;
    }

    std::vector<NET_GROUP> Groups( std::vector<NODE_ID> aNodes, std::span<const INST_ID> aLive,
                                   const std::weak_ptr<const FACADE_STATE>& aState ) const
    {
        // Unnamed components have no name to sort by, and Group drops them anyway
        std::erase_if( aNodes,
                       [&]( NODE_ID aNode )
                       {
                           return engine.Published().Components().at( aNode ).name == INVALID_ID;
                       } );
        std::ranges::sort( aNodes, NAME_LESS{ &engine.Keys() },
                           [&]( NODE_ID aNode )
                           {
                               return engine.Published().Components().at( aNode ).name;
                           } );
        std::vector<NET_GROUP> result;
        result.reserve( aNodes.size() );

        for( NODE_ID node : aNodes )
        {
            if( auto group = Group( node, aLive, aState ) )
                result.push_back( std::move( *group ) );
        }

        return result;
    }

    const PUBLISHED_COMPONENT* Net( NODE_ID aNode, INST_ID aInstance ) const
    {
        wxASSERT( wxThread::IsMain() );
        const auto found = netItems.find( aNode );

        if( found == netItems.end() || !found->second->instances.contains( aInstance ) || !Screen( aInstance ) )
            return nullptr;

        return &engine.Published().Components().at( aNode );
    }

    wxString Name( const PUBLISHED_COMPONENT& aComponent, bool aIgnoreSheet ) const
    {
        if( aComponent.name == INVALID_ID )
            return {};

        const wxString& name = engine.Keys().Name( aComponent.name );

        if( aIgnoreSheet && aComponent.content->best )
        {
            const NAME_ID path = aComponent.content->best->path;

            if( path != INVALID_ID )
                return name.Mid( engine.Keys().Name( path ).length() );
        }

        return name;
    }

    KIID_PATH Sheet( const std::optional<ITEM_KEY>& aDriver ) const
    {
        return aDriver && Resolve( *aDriver ) ? engine.Keys().Instance( aDriver->inst ) : KIID_PATH();
    }

    BUS_MEMBERS Members( NODE_ID aNode, INST_ID aInstance,
                         const std::weak_ptr<const FACADE_STATE>& aState, const ITEM_RESULT* aRow = nullptr ) const
    {
        const auto& component = engine.Published().Components().at( aNode );
        const auto& content = *component.content;

        if( content.kind != KIND::BUNDLE || !content.best || !Resolve( content.best->source ) )
            return {};

        BUS_MEMBERS result;
        const auto* local = aRow && aRow->busSource ? &*aRow->busSource : nullptr;

        if( local && !Resolve( local->source ) )
            return {};

        result.schema = local ? local->schema : content.best->schema;
        result.tree = engine.FindBusTree( engine.Keys().Name( local ? aRow->localName : content.best->name ) );
        wxASSERT( result.tree && content.best->schema->leaves.size() == content.members.size() );
        std::span<const SLOT_KEY> slots = content.members;
        std::vector<SLOT_KEY> localSlots;

        if( local )
        {
            localSlots.resize( result.schema->leaves.size() );
            // Keep local lookup aligned with BindBundle's matching and extra-slot identities
            const BUS_ALIGNMENT alignment = Align( *result.schema, *content.best->schema );

            for( const auto& [from, to] : alignment.matched )
                localSlots[from] = content.members.at( to );

            for( size_t from : alignment.unmappedLeft )
                localSlots[from] = SLOT_KEY{ local->source, static_cast<uint32_t>( from ) };

            slots = localSlots;
        }

        result.leaves.reserve( slots.size() );

        for( const SLOT_KEY& slot : slots )
        {
            const NODE_ID leaf = engine.Published().SlotComponents().at( slot );
            const auto& instances = netItems.at( leaf )->instances;
            const INST_ID instance = instances.contains( aInstance ) ? aInstance : slot.bundleDriver.inst;
            result.leaves.push_back( NET_VIEW( aState, leaf, instance ) );
        }

        return result;
    }

    SCH_SCREEN* Screen( INST_ID aInstance ) const
    {
        const auto found = sources.find( aInstance );

        if( found == sources.end() )
            return nullptr;

        return Screen( found->second );
    }

    SCH_SCREEN* Screen( const std::weak_ptr<const SCH_SCREEN::CONNECTIVITY_SOURCE>& aSource ) const
    {
        const auto source = aSource.lock();

        if( !source || !source->screen )
            return nullptr;

        SCH_SCREEN* screen = source->screen;
        const auto& revisions = engine.Published().Auxiliary().ScreenRevisions();
        const auto revision = revisions.find( screen->ConnectivityId() );

        if( revision == revisions.end() )
            return nullptr;

        if( revision->second == screen->ConnectivityRevision() || publicationHolds > 0 )
            return screen;

        return RENDER_SCOPE::Active() ? screen : nullptr;
    }

    bool Current( const SCH_SCREEN& aScreen ) const
    {
        const auto& revisions = engine.Published().Auxiliary().ScreenRevisions();
        const auto  revision = revisions.find( aScreen.ConnectivityId() );
        return revision != revisions.end() && revision->second == aScreen.ConnectivityRevision();
    }

    const ITEM_RESULT* Row( const ITEM_KEY& aItem ) const
    {
        wxASSERT( wxThread::IsMain() );

        const SCH_SCREEN* screen = Screen( aItem.inst );

        if( !screen )
            return nullptr;

        const auto& rows = engine.Published().Rows();
        const auto row = rows.find( aItem );

        if( row == rows.end() )
            return nullptr;

        // A canvas read of a changed screen must not revive the row of a deleted item
        if( publicationHolds == 0 && !Current( *screen ) && !screen->GetConnectivityItem( aItem.item ) )
            return nullptr;

        return &row->second;
    }

    SCH_ITEM* Resolve( const ITEM_KEY& aItem ) const
    {
        SCH_SCREEN* screen = Screen( aItem.inst );
        return screen ? screen->GetConnectivityItem( aItem.item ) : nullptr;
    }
};

SUBSCRIPTION::SUBSCRIPTION( std::weak_ptr<FACADE_STATE> aState, uint64_t aId ) :
        m_state( std::move( aState ) ), m_id( aId )
{
}

SUBSCRIPTION::~SUBSCRIPTION() { Reset(); }

SUBSCRIPTION::SUBSCRIPTION( SUBSCRIPTION&& aOther ) noexcept :
        m_state( std::move( aOther.m_state ) ), m_id( std::exchange( aOther.m_id, 0 ) )
{
}

SUBSCRIPTION& SUBSCRIPTION::operator=( SUBSCRIPTION&& aOther ) noexcept
{
    if( this != &aOther )
    {
        Reset();
        m_state = std::move( aOther.m_state );
        m_id = std::exchange( aOther.m_id, 0 );
    }

    return *this;
}

void SUBSCRIPTION::Reset()
{
    wxASSERT( wxThread::IsMain() );

    if( auto state = m_state.lock() )
        state->listeners.erase( m_id );

    m_state.reset();
    m_id = 0;
}

FACADE::FACADE() : m_state( std::make_shared<FACADE_STATE>() ) {}

FACADE::~FACADE()
{
    Clear();
    m_state->listeners.clear();
}

SUBSCRIPTION FACADE::Subscribe( std::function<void( const CHANGE_SET& )> aListener )
{
    wxASSERT( wxThread::IsMain() );

    if( !aListener )
        throw std::invalid_argument( "Connectivity listener is empty" );

    if( m_state->nextListener == std::numeric_limits<uint64_t>::max() )
        throw std::overflow_error( "Connectivity listener identities exhausted" );

    const uint64_t id = ++m_state->nextListener;
    m_state->listeners.emplace( id, std::move( aListener ) );
    return SUBSCRIPTION( m_state, id );
}

void FACADE::Recalculate( SCHEMATIC& aSchematic, bool aRebuild,
                           const std::function<void( SCH_ITEM* )>& aChangedHandler )
{
    wxASSERT( wxThread::IsMain() );

    if( !aSchematic.IsValid() )
        throw std::invalid_argument( "Cannot apply connectivity without a schematic project" );

    const auto state = m_state;
    const auto handler = aChangedHandler;
    const auto paths = aSchematic.BuildSheetListSortedByPageNumbers();

    try
    {
        updateModel( aSchematic, paths, aRebuild );

        NETCHAIN_INPUT chains;
        chains.sheets.reserve( paths.size() );
        struct SHEET_SOURCE
        {
            NETCHAIN_INPUT::SHEET* input;
            SCH_SCREEN*            screen;
        };
        std::map<INST_ID, SHEET_SOURCE> sheets;

        for( const SCH_SHEET_PATH& path : paths )
        {
            if( !path.LastScreen() )
                continue;

            const auto instance = Keys().FindInstance( path.PathRef() );

            if( !instance )
                throw std::logic_error( "Missing sheet instance while rebuilding netchains" );

            SCH_SCREEN* screen = state->Screen( *instance );

            if( !screen )
                throw std::logic_error( "Stale screen while rebuilding netchains" );

            chains.sheets.emplace_back( path, &chains.storage );
            sheets.emplace( *instance, SHEET_SOURCE{ &chains.sheets.back(), screen } );
        }

        for( const auto& [key, row] : Published().Rows() )
        {
            const auto sheet = sheets.find( key.inst );
            SCH_ITEM* item = sheet != sheets.end()
                                 ? sheet->second.screen->GetConnectivityItem( key.item ) : nullptr;

            if( !item )
                throw std::logic_error( "Stale item while rebuilding netchains" );

            if( row.kind != KIND::SIGNAL || ( item->Type() != SCH_PIN_T && item->Type() != SCH_LABEL_T ) )
                continue;

            NETCHAIN_INPUT::NET net{ row.name == INVALID_ID ? wxString() : Keys().Name( row.name ), {} };
            net.key = SCH_NETCHAIN::MakeKey( net.name, row.component );
            sheet->second.input->nets.emplace( item, std::move( net ) );
        }

        aSchematic.NetChains().Rebuild( chains );
    }
    catch( ... )
    {
        Clear();
        throw;
    }

    std::map<SCREEN_ID, INST_ID> displayed;

    for( const SCH_SHEET_PATH& path : paths )
    {
        if( const auto instance = Keys().FindInstance( path.PathRef() ) )
        {
            if( SCH_SCREEN* screen = state->Screen( *instance ) )
                displayed[screen->ConnectivityId()] = *instance;
        }
    }

    if( const auto current = Keys().FindInstance( aSchematic.CurrentSheet().PathRef() ) )
    {
        if( SCH_SCREEN* screen = state->Screen( *current ) )
            displayed[screen->ConnectivityId()] = *current;
    }

    std::set<ITEM_KEY, KEY_LESS> changed( KEY_LESS{ Keys() } );
    const auto& publishedChanges = Published().Changes().changedItems;
    changed.insert( publishedChanges.begin(), publishedChanges.end() );
    ApplyNetclasses( *aSchematic.Project().GetProjectFile().NetSettings() );
    aSchematic.NetChains().ApplyNetChainNetclasses();

    const auto applyEnds = []( auto* aItem, auto aDangling )
    {
        const bool differs = aItem->IsStartDangling() != bool( aDangling & 1 )
                             || aItem->IsEndDangling() != bool( aDangling & 2 );
        aItem->SetDanglingState( aDangling & 1, aDangling & 2 );
        return differs;
    };

    for( const auto& [key, island] : Published().Auxiliary().Islands() )
    {
        SCH_SCREEN* screen = state->Screen( key.inst );

        if( !screen )
            continue;

        const auto instance = displayed.find( screen->ConnectivityId() );

        if( instance == displayed.end() || instance->second != key.inst )
            continue;

        for( const auto& [id, dangling] : island.value.dangling )
        {
            SCH_ITEM* item = screen->GetConnectivityItem( id );
            bool differs = false;

            if( !item )
                continue;

            switch( item->Type() )
            {
            case SCH_LINE_T:
                differs = applyEnds( static_cast<SCH_LINE*>( item ), dangling );
                break;
            case SCH_BUS_WIRE_ENTRY_T:
            case SCH_BUS_BUS_ENTRY_T:
                differs = applyEnds( static_cast<SCH_BUS_ENTRY_BASE*>( item ), dangling );
                break;
            case SCH_LABEL_T:
            case SCH_GLOBAL_LABEL_T:
            case SCH_HIER_LABEL_T:
            case SCH_DIRECTIVE_LABEL_T:
            case SCH_SHEET_PIN_T:
            {
                auto* label = static_cast<SCH_LABEL_BASE*>( item );
                differs = label->SCH_LABEL_BASE::IsDangling() != bool( dangling );
                label->SetIsDangling( dangling );

                if( label->Type() == SCH_DIRECTIVE_LABEL_T )
                {
                    auto* directive = static_cast<SCH_DIRECTIVE_LABEL*>( label );
                    std::unordered_set<SCH_RULE_AREA*> areas;
                    const auto& auxiliary = Published().Auxiliary();
                    const auto attached = auxiliary.RuleAreasOf().find( { id, key.inst } );

                    if( attached != auxiliary.RuleAreasOf().end() )
                    {
                        for( const KIID& areaId : attached->second )
                        {
                            if( auto* live = dynamic_cast<SCH_RULE_AREA*>( screen->GetConnectivityItem( areaId ) ) )
                                areas.insert( live );
                        }
                    }

                    if( directive->GetConnectedRuleAreas() != areas )
                    {
                        directive->ClearConnectedRuleAreas();

                        for( SCH_RULE_AREA* area : areas )
                            directive->AddConnectedRuleArea( area );

                        differs = true;
                    }
                }
                break;
            }
            case SCH_PIN_T:
                differs = static_cast<SCH_PIN*>( item )->SetIsDangling( dangling );
                break;
            default:
                break;
            }

            if( differs )
                changed.insert( { id, key.inst } );
        }
    }

    for( const auto& [id, instance] : displayed )
    {
        SCH_SCREEN* screen = state->Screen( instance );

        if( !screen )
            continue;

        for( SCH_ITEM* item : screen->Items() )
        {
            item->SetConnectivityDirty( false );
            item->RunOnChildren( []( SCH_ITEM* child ) { child->SetConnectivityDirty( false ); },
                                 RECURSE_MODE::NO_RECURSE );
        }
    }

    state->publicationApplied = true;

    const uint64_t generation = state->resetGeneration;
    const uint64_t dispatch = ++state->dispatchGeneration;
    std::vector<uint64_t> listeners;
    CHANGE_SET notification;

    if( !state->listeners.empty() )
    {
        notification = state->engine.Published().Changes();
        notification.changedItems.assign( changed.begin(), changed.end() );
        std::ranges::copy( state->listeners | std::views::keys, std::back_inserter( listeners ) );
    }

    // A callback that clears or recalculates makes the rest of this delivery obsolete
    const auto superseded = [&]()
    {
        return state->resetGeneration != generation || state->dispatchGeneration != dispatch;
    };

    // The publication is already applied, so one failing callback must not starve the others
    const auto deliver = []( const char* aKind, const auto& aCall )
    {
        try
        {
            aCall();
        }
        catch( const std::exception& error )
        {
            wxLogTrace( "KICAD_CONNECTIVITY", "Connectivity %s failed: %s", aKind, error.what() );
        }
        catch( ... )
        {
            wxLogTrace( "KICAD_CONNECTIVITY", "Connectivity %s failed with an unknown exception", aKind );
        }
    };

    if( handler )
    {
        for( const ITEM_KEY& key : changed )
        {
            if( superseded() )
                break;

            if( SCH_ITEM* item = state->Resolve( key ) )
                deliver( "change handler",
                         [&]()
                         {
                             handler( item );
                         } );
        }
    }

    if( !notification.Empty() )
    {
        for( uint64_t id : listeners )
        {
            if( superseded() )
                break;

            const auto found = state->listeners.find( id );

            if( found != state->listeners.end() )
            {
                const auto listener = found->second;
                deliver( "listener",
                         [&]()
                         {
                             listener( notification );
                         } );
            }
        }
    }
}


void FACADE::Update( SCHEMATIC& aSchematic, bool aRebuild )
{
    wxASSERT( wxThread::IsMain() );

    try
    {
        updateModel( aSchematic, aSchematic.BuildSheetListSortedByPageNumbers(), aRebuild );
    }
    catch( ... )
    {
        Clear();
        throw;
    }
}

bool FACADE::UpdateShadow( SCHEMATIC& aSchematic, const SCH_SHEET_LIST& aPaths, bool aRebuild )
{
    try
    {
        updateModel( aSchematic, aPaths, aRebuild );
        return true;
    }
    catch( const std::exception& error )
    {
        Clear();
        wxLogTrace( "CONN_SHADOW", "Shadow connectivity failed: %s", error.what() );
    }
    catch( ... )
    {
        Clear();
        wxLogTrace( "CONN_SHADOW", "Shadow connectivity failed with an unknown exception" );
    }

    return false;
}


void FACADE::ApplyNetclasses( NET_SETTINGS& aSettings ) const
{
    wxASSERT( wxThread::IsMain() );
    std::map<wxString, std::set<wxString>> assignments;

    for( const auto& [name, classes] : Published().Netclasses() )
    {
        auto& names = assignments[Keys().Name( name )];

        for( NAME_ID netclass : *classes )
            names.insert( Keys().Name( netclass ) );
    }

    std::vector<wxString> retired;

    for( const auto& [name, classes] : aSettings.GetNetclassLabelAssignments() )
    {
        if( !assignments.contains( name ) )
            retired.push_back( name );
    }

    for( const wxString& name : retired )
    {
        aSettings.ClearCacheForNet( name );
        aSettings.ClearNetclassLabelAssignment( name );
    }

    const auto& previous = aSettings.GetNetclassLabelAssignments();

    for( const auto& [name, classes] : assignments )
    {
        const auto old = previous.find( name );

        if( old == previous.end() || old->second != classes )
        {
            aSettings.SetNetclassLabelAssignment( name, classes );
            aSettings.ClearCacheForNet( name );
        }
    }
}


void FACADE::updateModel( SCHEMATIC& aSchematic, const SCH_SHEET_LIST& aPaths, bool aRebuild )
{
    wxASSERT( wxThread::IsMain() );

    TEXT_ENVIRONMENT environment;
    INPUT_TEXT_SCOPE frame( environment );
    auto context = CaptureTextContext( aSchematic, aPaths );
    const bool changed = !context || m_state->textContext != context
                         || m_state->engine.ExternalSourcesChanged( aPaths, true );

    if( changed && m_state->textEpoch == std::numeric_limits<uint64_t>::max() )
        throw std::overflow_error( "Connectivity text epoch exhausted" );

    BUS_ALIASES aliases;

    for( const auto& alias : aSchematic.GetAllBusAliases() )
    {
        if( alias )
            aliases.insert_or_assign( alias->GetName(), alias->Members() );
    }

    Update( aPaths, m_state->textEpoch + ( changed ? 1 : 0 ), aliases, aRebuild );
    m_state->textContext = std::move( context );
}

void FACADE::Update( const SCH_SHEET_LIST& aPaths, uint64_t aTextEpoch, const BUS_ALIASES& aAliases, bool aRebuild )
{
    wxASSERT( wxThread::IsMain() );

    m_state->textContext.reset();
    m_state->publicationApplied = false;
    m_state->textChecks.reset();
    m_state->simulationModels.reset();

    try
    {
        m_state->engine.Update( aPaths, aTextEpoch, aAliases, aRebuild );
        decltype( m_state->sources ) sources;

        for( const SCH_SHEET_PATH& path : aPaths )
        {
            if( SCH_SCREEN* screen = path.LastScreen() )
            {
                if( const auto instance = Keys().FindInstance( path.PathRef() ) )
                    sources.emplace( *instance, screen->ConnectivitySource() );
            }
        }

        m_state->RefreshNetItems();
        m_state->sources.swap( sources );
        m_state->textEpoch = aTextEpoch;
    }
    catch( ... )
    {
        Clear();
        throw;
    }
}

void FACADE::Clear()
{
    m_state->publicationApplied = false;
    m_state->textChecks.reset();
    m_state->simulationModels.reset();
    ++m_state->resetGeneration;
    m_state->sources.clear();
    m_state->netItems.clear();
    m_state->engine.Clear();
    m_state->textEpoch = 0;
    m_state->textContext.reset();
}

PUBLICATION_HOLD::PUBLICATION_HOLD( const FACADE& aFacade ) : m_state( aFacade.m_state )
{
    wxASSERT( wxThread::IsMain() );
    ++aFacade.m_state->publicationHolds;
}

PUBLICATION_HOLD::~PUBLICATION_HOLD()
{
    if( const auto state = m_state.lock() )
        --state->publicationHolds;
}

const PUBLICATION& FACADE::Published() const { return m_state->engine.Published(); }
const SESSION_KEYS& FACADE::Keys() const { return m_state->engine.Keys(); }
const ENGINE& FACADE::Engine() const { return m_state->engine; }

void FACADE::PrepareTextChecks( SCHEMATIC& aSchematic, bool aIncludeDrawingSheet )
{
    wxASSERT( wxThread::IsMain() );
    m_state->textChecks.reset();
    const auto paths = m_state->SourcePaths( aSchematic );
    const auto vcs = sourceProjectContext( aSchematic );
    std::optional<DS_DATA_MODEL> isolated;
    struct RESTORE_LAYOUT
    {
        DS_DATA_MODEL* previous = nullptr;
        ~RESTORE_LAYOUT()
        {
            if( previous )
                DS_DATA_MODEL::SetAltInstance( previous );
        }
    } restoreLayout;

    if( aIncludeDrawingSheet )
    {
        // Rendering replaces draw objects that the canvas may still reference
        DS_DATA_MODEL& original = DS_DATA_MODEL::GetTheInstance();
        wxString layout;
        original.SaveInString( &layout );
        isolated.emplace();
        restoreLayout.previous = &original;
        DS_DATA_MODEL::SetAltInstance( &*isolated );
        isolated->SetPageLayout( layout.utf8_str() );
        isolated->AllowVoidList( original.VoidListAllowed() );
    }

    std::map<KIID_PATH, FACADE_STATE::TEXT_CHECKS> captured;

    for( const SCH_SHEET_PATH& path : paths )
    {
        auto& checks = captured[path.PathRef()];

        if( aIncludeDrawingSheet )
            checks.drawingSheet.emplace();

        if( const SCH_SCREEN* screen = path.LastScreen() )
        {
            checks.items = ExtractTextChecks( *screen, path );

            if( aIncludeDrawingSheet )
                checks.drawingSheet = ExtractDrawingSheetTextChecks( *screen, path );
        }
    }

    m_state->textChecks = std::move( captured );
}

std::vector<TEXT_CHECK_FACT> FACADE::TextChecks( const KIID_PATH& aPath, bool aDrawingSheet ) const
{
    wxASSERT( wxThread::IsMain() );

    if( !m_state->textChecks )
        throw std::logic_error( "Text sources have not been prepared" );

    const auto& captured = m_state->textChecks->at( aPath );

    if( aDrawingSheet && !captured.drawingSheet )
        throw std::logic_error( "Drawing sheet text sources have not been prepared" );

    return aDrawingSheet ? *captured.drawingSheet : captured.items;
}

void FACADE::PrepareSimulationModels( SCHEMATIC& aSchematic )
{
    wxASSERT( wxThread::IsMain() );
    m_state->simulationModels.reset();
    const auto vcs = sourceProjectContext( aSchematic );

    std::map<KIID_PATH, std::vector<SIMULATION_MODEL_FACT>> captured;
    const wxString variant = aSchematic.GetCurrentVariant();

    for( const SCH_SHEET_PATH& path : m_state->SourcePaths( aSchematic ) )
        captured.emplace( path.PathRef(), ExtractSimulationModelFacts( path, variant ) );

    m_state->simulationModels = std::move( captured );
}

std::vector<SIMULATION_MODEL_FACT> FACADE::SimulationModels( const KIID_PATH& aPath ) const
{
    wxASSERT( wxThread::IsMain() );

    if( !m_state->simulationModels )
        throw std::logic_error( "Simulation sources have not been prepared" );

    return m_state->simulationModels->at( aPath );
}

std::optional<ITEM_VIEW> FACADE::Connection( const KIID& aItem, const KIID_PATH& aPath ) const
{
    const auto instance = Keys().FindInstance( aPath );

    if( !instance )
        return std::nullopt;

    const ITEM_KEY key{ aItem, *instance };

    if( !m_state->Row( key ) )
        return std::nullopt;

    return ITEM_VIEW( m_state, key );
}

wxString ITEM_VIEW::Name( bool aIgnoreSheet ) const
{
    const auto state = m_state.lock();
    const auto* row = state ? state->Row( m_item ) : nullptr;
    return row ? state->Name( state->engine.Published().Components().at( row->component ), aIgnoreSheet ) : wxString();
}

wxString ITEM_VIEW::LocalName() const
{
    const auto state = m_state.lock();
    const auto* row = state ? state->Row( m_item ) : nullptr;
    return row && row->localName != INVALID_ID ? state->engine.Keys().Name( row->localName ) : wxString();
}

wxString ITEM_VIEW::FullLocalName() const
{
    const auto state = m_state.lock();
    const auto* row = state ? state->Row( m_item ) : nullptr;
    return row && row->fullLocalName != INVALID_ID ? state->engine.Keys().Name( row->fullLocalName ) : wxString();
}

bool ITEM_VIEW::IsBus() const
{
    const auto state = m_state.lock();
    const auto* row = state ? state->Row( m_item ) : nullptr;
    return row && row->kind == KIND::BUNDLE;
}

bool ITEM_VIEW::IsNet() const
{
    const auto state = m_state.lock();
    const auto* row = state ? state->Row( m_item ) : nullptr;
    return row && row->kind == KIND::SIGNAL;
}

bool ITEM_VIEW::IsUnconnected() const
{
    const auto state = m_state.lock();
    const auto* row = state ? state->Row( m_item ) : nullptr;
    return !row || !row->driver;
}

int ITEM_VIEW::NetCode() const
{
    const auto state = m_state.lock();
    const auto* row = state ? state->Row( m_item ) : nullptr;
    return row ? row->netCode : 0;
}

uint32_t ITEM_VIEW::SubgraphCode() const
{
    const auto state = m_state.lock();
    const auto* row = state ? state->Row( m_item ) : nullptr;
    return row ? row->subgraphCode : 0;
}

SCH_ITEM* ITEM_VIEW::Driver() const
{
    const auto state = m_state.lock();
    const auto* row = state ? state->Row( m_item ) : nullptr;
    return row && row->driver ? state->Resolve( *row->driver ) : nullptr;
}

std::vector<SCH_ITEM*> ITEM_VIEW::ConnectedItems() const
{
    std::vector<SCH_ITEM*> result;
    const auto state = m_state.lock();

    if( !state || !state->Row( m_item ) )
        return result;

    const auto& neighbors = state->engine.Published().Auxiliary().NeighborsOf();
    const auto found = neighbors.find( m_item );

    if( found == neighbors.end() )
        return result;

    result.reserve( found->second.size() );

    for( const KIID& id : found->second )
    {
        if( SCH_ITEM* item = state->Resolve( { id, m_item.inst } ) )
            result.push_back( item );
    }

    return result;
}

KIID_PATH ITEM_VIEW::Sheet() const
{
    const auto state = m_state.lock();
    const auto* row = state ? state->Row( m_item ) : nullptr;
    return row ? state->Sheet( row->driver ) : KIID_PATH();
}

BUS_MEMBERS ITEM_VIEW::Members() const
{
    const auto state = m_state.lock();
    const auto* row = state ? state->Row( m_item ) : nullptr;
    return row ? state->Members( row->component, m_item.inst, m_state, row ) : BUS_MEMBERS();
}

std::optional<NET_VIEW> FACADE::GetSubgraphForItem( const KIID& aItem, const KIID_PATH& aPath ) const
{
    const auto instance = Keys().FindInstance( aPath );

    if( !instance )
        return std::nullopt;

    const auto* row = m_state->Row( { aItem, *instance } );

    if( !row )
        return std::nullopt;

    return NET_VIEW( m_state, row->component, *instance );
}

std::vector<NET_GROUP> FACADE::GetNetMap() const
{
    const auto live = m_state->LiveInstances();
    std::vector<NET_GROUP> result;
    result.reserve( Published().ByName().size() );

    for( const auto& [name, node] : Published().ByName() )
    {
        if( auto group = m_state->Group( node, live, m_state ) )
            result.push_back( std::move( *group ) );
    }

    return result;
}

std::vector<wxString> FACADE::NetNames() const
{
    const auto live = m_state->LiveInstances();
    std::vector<wxString> result;
    result.reserve( Published().ByName().size() );

    for( const auto& [name, node] : Published().ByName() )
    {
        if( m_state->AnyLive( node, live ) )
            result.push_back( Keys().Name( name ) );
    }

    return result;
}


std::optional<NET_GROUP> FACADE::NetByName( const wxString& aName ) const
{
    const auto node = Published().FindByName( aName );

    if( !node )
        return std::nullopt;

    return m_state->Group( *node, m_state->LiveInstances(), m_state );
}


std::vector<NET_GROUP> FACADE::BusWithMembers( const wxString& aName ) const
{
    const auto node = m_state->BusNode( aName );

    if( !node )
        return {};

    const auto live = m_state->LiveInstances();

    if( !m_state->AnyLive( *node, live ) )
        return {};

    const auto members = Published().MembersOf( *node );
    std::vector<NODE_ID> nodes( members.begin(), members.end() );
    nodes.push_back( *node );
    return m_state->Groups( std::move( nodes ), live, m_state );
}

std::vector<NET_GROUP> FACADE::BundlesOf( NODE_ID aSignal ) const
{
    const auto parents = Published().ParentsOf( aSignal );

    if( parents.empty() )
        return {};

    const auto live = m_state->LiveInstances();

    if( !m_state->AnyLive( aSignal, live ) )
        return {};

    return m_state->Groups( { parents.begin(), parents.end() }, live, m_state );
}

std::vector<wxString> FACADE::GetEquivalentBusNames( const wxString& aName ) const
{
    const auto node = m_state->BusNode( aName );

    if( !node )
        return {};

    const auto live = m_state->LiveInstances();

    if( !m_state->AnyLive( *node, live ) )
        return {};

    auto names = Published().EquivalentBusNames( aName );
    std::erase_if( names, [&]( const wxString& name )
    {
        const auto equivalent = Published().FindByName( name );
        return !equivalent || !m_state->AnyLive( *equivalent, live );
    } );
    return names;
}

KIID_PATH NET_VIEW::Instance() const
{
    const auto state = m_state.lock();
    return state ? state->engine.Keys().Instance( m_instance ) : KIID_PATH();
}

wxString NET_VIEW::Name( bool aIgnoreSheet ) const
{
    const auto state = m_state.lock();
    const auto* net = state ? state->Net( m_component, m_instance ) : nullptr;
    return net ? state->Name( *net, aIgnoreSheet ) : wxString();
}

bool NET_VIEW::IsBus() const
{
    const auto state = m_state.lock();
    const auto* net = state ? state->Net( m_component, m_instance ) : nullptr;
    return net && net->content->kind == KIND::BUNDLE;
}

bool NET_VIEW::IsNet() const
{
    const auto state = m_state.lock();
    const auto* net = state ? state->Net( m_component, m_instance ) : nullptr;
    return net && net->content->kind == KIND::SIGNAL;
}

SCH_ITEM* NET_VIEW::Driver() const
{
    const auto state = m_state.lock();
    const auto* net = state ? state->Net( m_component, m_instance ) : nullptr;
    return net && net->content->best ? state->Resolve( net->content->best->source ) : nullptr;
}

KIID_PATH NET_VIEW::Sheet() const
{
    const auto state = m_state.lock();
    const auto* net = state ? state->Net( m_component, m_instance ) : nullptr;
    return net && net->content->best ? state->Sheet( net->content->best->source ) : KIID_PATH();
}

BUS_MEMBERS NET_VIEW::Members() const
{
    const auto state = m_state.lock();
    return state && state->Net( m_component, m_instance )
                   ? state->Members( m_component, m_instance, m_state ) : BUS_MEMBERS();
}

std::vector<SCH_ITEM*> NET_VIEW::Items() const
{
    std::vector<SCH_ITEM*> result;
    const auto state = m_state.lock();

    if( !state || !state->Net( m_component, m_instance ) )
        return result;

    const auto& ids = state->netItems.at( m_component )->instances.at( m_instance );
    result.reserve( ids.size() );

    for( const KIID& id : ids )
    {
        if( SCH_ITEM* item = state->Resolve( { id, m_instance } ) )
            result.push_back( item );
    }

    return result;
}
} // namespace SCH_CONNECTIVITY
