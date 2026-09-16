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

#pragma once

#include "conn_engine.h"
#include "conn_subscription.h"
#include <memory>
#include <functional>

class SCH_ITEM;
class SCHEMATIC;
class NET_SETTINGS;

namespace SCH_CONNECTIVITY
{
struct FACADE_STATE;
struct BUS_MEMBERS;

// Main-thread value view; every read resolves current publication and source lifetime.
class ITEM_VIEW
{
public:
    wxString Name( bool aIgnoreSheet = false ) const;
    wxString LocalName() const;
    wxString FullLocalName() const;
    bool     IsBus() const;
    bool     IsNet() const;
    // Missing or stale rows are unconnected; Connection() rejects them when acquiring a view.
    // Inside a RENDER_SCOPE, a stale row of a live item still counts (see sch_conn_revisions_render).
    bool      IsUnconnected() const;
    int       NetCode() const;
    uint32_t  SubgraphCode() const;
    SCH_ITEM* Driver() const;
    // Winning driver instance; empty without a driver or when either source is stale.
    KIID_PATH   Sheet() const;
    BUS_MEMBERS Members() const;
    // Direct physical neighbors on this instance, resolved from published adjacency.
    std::vector<SCH_ITEM*> ConnectedItems() const;

private:
    friend class FACADE;
    ITEM_VIEW( std::weak_ptr<const FACADE_STATE> aState, ITEM_KEY aItem ) :
            m_state( std::move( aState ) ), m_item( aItem )
    {}

    std::weak_ptr<const FACADE_STATE> m_state;
    ITEM_KEY                          m_item;
};

// A published component on one physical or bus-member instance.
class NET_VIEW
{
public:
    NODE_ID Component() const { return m_component; }
    KIID_PATH Instance() const;
    wxString  Name( bool aIgnoreSheet = false ) const;
    bool      IsBus() const;
    bool      IsNet() const;
    // Winning driver for the whole component; it may belong to another instance.
    SCH_ITEM* Driver() const;
    // Winning driver instance; empty without a driver or when either source is stale.
    KIID_PATH   Sheet() const;
    BUS_MEMBERS Members() const;
    // Physical items on this instance; bus-member instances may have none.
    std::vector<SCH_ITEM*> Items() const;

private:
    friend class FACADE;
    friend struct FACADE_STATE;
    NET_VIEW( std::weak_ptr<const FACADE_STATE> aState, NODE_ID aComponent, INST_ID aInstance ) :
            m_state( std::move( aState ) ), m_component( aComponent ), m_instance( aInstance )
    {}

    std::weak_ptr<const FACADE_STATE> m_state;
    NODE_ID                           m_component;
    INST_ID                           m_instance;
};

/**
 * Query-time snapshot; reacquire after Update. An absent tree means empty leaves.
 * Tree and schema leaf ordinals index signal views; schema retains declaration-local names.
 * Item views preserve same-kind local declarations; mixed kinds and net views use the canonical declaration.
 * Nested buses exist only in tree, not in leaves.
 * Walk tree for recursive presentation. Leaves prefer the querying instance when present,
 * otherwise the canonical bus driver's instance, including itemless members.
 */
struct BUS_MEMBERS
{
    std::shared_ptr<const BUS_SCHEMA::NODE> tree;
    std::shared_ptr<const BUS_SCHEMA>       schema;
    std::vector<NET_VIEW>                   leaves;
};

struct NET_GROUP
{
    wxString              name;
    std::vector<NET_VIEW> instances;
};

/**
 * Model-owned source resolver. Graph stages retain only keys and extracted values.
 *
 * @see @ref schematic_connectivity for the full update, modification and overlay flows.
 */
class FACADE
{
public:
    FACADE();
    ~FACADE();
    FACADE( const FACADE& ) = delete;
    FACADE& operator=( const FACADE& ) = delete;

    /**
     * Update the complete model and apply netclasses, dangling flags and dirty-state clearing.
     * The current instance controls shared-screen display state; callbacks run after application.
     * A callback that clears or destroys the model retires the remaining notifications in its batch.
     */
    void Recalculate( SCHEMATIC& aSchematic, bool aRebuild = false,
                      const std::function<void( SCH_ITEM* )>& aChangedHandler = {} );
    // Applied, nonempty deltas only. Registrations survive Clear; new listeners start with the next batch.
    [[nodiscard]] SUBSCRIPTION Subscribe( std::function<void( const CHANGE_SET& )> aListener );
    // Refresh current model context, including text inputs without revision hooks.
    void Update( SCHEMATIC& aSchematic, bool aRebuild = false );
    // Production shadow boundary that traces failures and clears the publication without disturbing legacy flow.
    bool UpdateShadow( SCHEMATIC& aSchematic, const SCH_SHEET_LIST& aPaths, bool aRebuild = false );
    void Update( const SCH_SHEET_LIST& aPaths, uint64_t aTextEpoch, const BUS_ALIASES& aAliases,
                 bool aRebuild = false );
    void              Clear();
    /**
     * Replace label-derived assignments from a complete publication; invalidate changed net caches.
     * Call after Update for authoritative model application. Shadow updates never call this.
     */
    void                     ApplyNetclasses( NET_SETTINGS& aSettings ) const;
    std::optional<ITEM_VIEW> Connection( const KIID& aItem, const KIID_PATH& aPath ) const;
    std::optional<NET_VIEW>  GetSubgraphForItem( const KIID& aItem, const KIID_PATH& aPath ) const;
    /**
     * Owned query-time groups in canonical name order, with unique instances in KIID_PATH order.
     * Reacquire the collection after Update; individual views resolve current publication.
     */
    std::vector<NET_GROUP>   GetNetMap() const;
    std::vector<wxString>    NetNames() const;
    std::optional<NET_GROUP> NetByName( const wxString& aName ) const;
    // Current bus queries in canonical name and instance order; members include unmapped slots.
    std::vector<NET_GROUP> BusWithMembers( const wxString& aName ) const;
    // Signal handle from this facade session; Clear never recycles session handles.
    std::vector<NET_GROUP> BundlesOf( NODE_ID aSignal ) const;
    std::vector<wxString>  GetEquivalentBusNames( const wxString& aName ) const;
    // Explicit ERC text capture after Recalculate; updates retire the owned snapshot.
    void                         PrepareTextChecks( SCHEMATIC& aSchematic, bool aIncludeDrawingSheet = false );
    std::vector<TEXT_CHECK_FACT> TextChecks( const KIID_PATH& aPath, bool aDrawingSheet = false ) const;
    // Explicit ERC source supplement after Recalculate; ordinary updates retire the batch.
    void PrepareSimulationModels( SCHEMATIC& aSchematic );
    // Owned snapshot; unavailable batches and paths outside the captured hierarchy throw.
    std::vector<SIMULATION_MODEL_FACT> SimulationModels( const KIID_PATH& aPath ) const;
    const PUBLICATION&                 Published() const;
    const ENGINE&                      Engine() const;
    const SESSION_KEYS&                Keys() const;

private:
    friend class PUBLICATION_HOLD;

    /**
     * Capture the text context and advance the text epoch when it or an external source changed.
     */
    void                          updateModel( SCHEMATIC& aSchematic, const SCH_SHEET_LIST& aPaths, bool aRebuild );
    std::shared_ptr<FACADE_STATE> m_state;
};

/**
 * Keep the last publication readable while a batch stages edits before its single commit.
 * Staging bumps screen revisions, which otherwise hides every later query on that screen.
 * Release the hold before recalculating or pushing the commit.
 *
 * @see @ref sch_conn_modify
 */
class PUBLICATION_HOLD
{
public:
    explicit PUBLICATION_HOLD( const FACADE& aFacade );
    ~PUBLICATION_HOLD();

    PUBLICATION_HOLD( const PUBLICATION_HOLD& ) = delete;
    PUBLICATION_HOLD& operator=( const PUBLICATION_HOLD& ) = delete;

private:
    std::weak_ptr<FACADE_STATE> m_state;
};
} // namespace SCH_CONNECTIVITY
