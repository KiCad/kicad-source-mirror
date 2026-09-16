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

/**
 * @file conn_overview.h
 * Architecture documentation for the schematic connectivity engine.
 */

/**
 * @page schematic_connectivity Schematic Connectivity Engine
 *
 * @tableofcontents
 *
 * @section sch_conn_model Model
 *
 * The engine calculates nets and buses for the complete schematic hierarchy. It lives in the
 * SCH_CONNECTIVITY namespace. SCHEMATIC owns one SCH_CONNECTIVITY::FACADE and exposes it through
 * SCHEMATIC::Connectivity(). The advanced configuration flag ADVANCED_CFG::m_ConnectivityEngine
 * selects this engine instead of CONNECTION_GRAPH.
 *
 * The engine divides the work into two layers.
 *
 * SCH_CONNECTIVITY::FACADE connects the engine to the editor model. It captures the text context,
 * runs the engine, writes results back to schematic items and notifies listeners. It also resolves
 * published keys back to live SCH_ITEM pointers.
 *
 * SCH_CONNECTIVITY::ENGINE runs the stage pipeline. After input capture, no stage reads the editor
 * model. Each stage reads values that the previous stage owns. This lets the engine run the pure
 * folds on worker threads, and it lets each stage cache results by version.
 *
 * All stages use the same key session, SCH_CONNECTIVITY::SESSION_KEYS. It converts sheet paths,
 * names and graph nodes to integer handles.
 *
 * Each cache entry has a version from SCH_CONNECTIVITY::CACHE_VERSIONS. A stage compares input
 * versions to decide which work to repeat. The engine does not keep a list of dirty items. The
 * only change signal from the model is SCH_SCREEN::ConnectivityRevision().
 *
 * SCH_CONNECTIVITY::PUBLICATION holds the only state that consumers read. A query goes through
 * SCH_CONNECTIVITY::FACADE, which rejects rows from a screen whose revision changed after the last
 * update. A consumer should never read a net that does not agree with the display.
 *
 * The engine runs on the main thread. SCH_CONNECTIVITY::ParallelFor in conn_tasks.h runs only the
 * pure per-partition folds on the KiCad thread pool.
 *
 * If a stage throws, the engine discards all stage outputs and the publication, then rethrows.
 * SCH_EDIT_FRAME::RecalculateConnections() catches the error and shows it in the info bar. An
 * empty publication is safe because every query then reports "unconnected".
 *
 *
 * @section sch_conn_glossary Glossary and keys
 *
 * @subsection sch_conn_glossary_terms Glossary
 *
 * | Term           | Meaning                                                                             |
 * |----------------|-------------------------------------------------------------------------------------|
 * | screen         | The drawing content that one or more sheet instances show.                          |
 * | instance       | One sheet path in the hierarchy, which a KIID_PATH identifies.                      |
 * | frame          | The captured hierarchy, with one entry for each instance that has a screen.         |
 * | instance scope | The handle, display path, depth and child instances of one instance.                |
 * | fact           | A value copy of model data that holds no model pointers.                            |
 * | item fact      | The copy of one connectable item, or of one group of merged pins.                   |
 * | instance fact  | The text that one instance resolves for the items of its screen.                    |
 * | port           | A connection point of an item, or the name scope that joins a sheet pin to a child. |
 * | island         | The smallest set of items on one screen that the drawing connects.                  |
 * | record         | The kind, claims, name edges and ERC atoms of one island in one instance.           |
 * | claim          | The request of one item to give its name to its island.                             |
 * | kind           | The electrical type of a record or component, either a net or a bus.                |
 * | stratum        | One partition pass over the nodes of one kind, first buses and then nets.           |
 * | node           | One vertex of the union-find graph, which the session interns from a value key.     |
 * | partition      | One connected set of nodes, with the node versions and an anchor.                   |
 * | component      | The cached evaluation of one partition, which its anchor node identifies.           |
 * | bundle         | A component of kind BUNDLE, which is a bus.                                         |
 * | signal         | A component of kind SIGNAL, which is a net.                                         |
 * | slot           | One member position of a bundle, which can join the nets of that member.            |
 * | bus schema     | The parsed form of one bus text.                                                    |
 * | leaf           | One flattened member net of a bus schema.                                           |
 * | row            | The published result for one item in one instance.                                  |
 * | publication    | The only engine state that consumers read.                                          |
 * | auxiliary      | The published data that does not depend on net identity.                            |
 * | net code       | A positive number that a net keeps while its successors continue it.                |
 * | subgraph code  | A positive number that a named net or bus keeps in the same way.                    |
 * | text epoch     | A counter that makes all instances resolve their text again when it changes.        |
 * | revision       | A screen counter that changes when a connectivity source on the screen changes.     |
 * | session        | The key tables and cache versions that one engine owns for its lifetime.            |
 *
 *
 *
 * @section sch_conn_full Full update
 *
 * A full update discards all stage caches and recalculates every stage again. It runs when loading
 * a schematic, when the sheet hierarchy changes, on any GLOBAL_CLEANUP request, and whenever
 * ADVANCED_CFG::m_IncrementalConnectivity is off.
 *
 * @code{.unparsed}
 *   [1]  SCHEMATIC::RebuildConnectivity                          schematic.cpp
 *          |
 *          v
 *   [2]  FACADE::Recalculate( aRebuild = true )                  conn_facade.cpp
 *          |
 *          v
 *   [3]  FACADE::updateModel                                     conn_facade.cpp
 *          |  text context, text epoch, BUS_ALIASES
 *   =======|================== ENGINE::Update =================  conn_engine.cpp
 *          v
 *   [4]  ENGINE::clearStages( retain source values )
 *          |
 *          v
 *   [5]  INPUT_STORE::Capture                                    conn_inputs.h
 *          |  SCREEN_FACTS, INSTANCE_FACTS, SCREEN_ISLANDS       conn_facts.h, conn_islands.h
 *          v
 *   [6]  RECORD_STORE::Update                                    conn_records.h
 *          |  one ISLAND_RECORD per island                       conn_claims.h
 *          v
 *   [7]  PARTITIONER::Build( buses )  -->  BindBundle            conn_partition.h
 *          |  one BUNDLE_BINDING per bus                         conn_bindings.h
 *          v
 *   [8]  SLOT_STORE::Update                                      conn_bindings.h
 *          |  one SLOT_INPUT per bus member
 *          v
 *   [9]  PARTITIONER::Build( nets )  -->  DeriveSignal           conn_signals.h
 *          |  one SIGNAL_RESULT per net                          conn_summary.h
 *          v
 *   [10] PUBLICATION::Update  +  AUXILIARY::Update               conn_publish.h
 *          |  ITEM_RESULT rows, CHANGE_SET                       conn_auxiliary.h
 *   =======|===================================================
 *          v
 *   [11] FACADE::Update records live screen sources              conn_facade.cpp
 *          |
 *          v
 *   [12] NETCHAIN_MANAGER::Rebuild                               conn_netchain_manager.h
 *          |
 *          v
 *   [13] FACADE::ApplyNetclasses, dangling and dirty flags       conn_facade.cpp
 *          |
 *          v
 *   [14] item handler, then SUBSCRIPTION listeners               conn_subscription.h
 *
 *   An exception in [3] to [12]  -->  FACADE::Clear()  -->  rethrow with an empty publication
 * @endcode
 *
 * @par 1. Prepare the model (SCHEMATIC::RebuildConnectivity in schematic.cpp)
 * The function clears the cached netclass lookups in NET_SETTINGS. It then calls
 * SCH_RULE_AREA::UpdateRuleAreasInScreens() for every screen.
 *
 * @par 2. Start the facade (SCH_CONNECTIVITY::FACADE::Recalculate in conn_facade.cpp)
 * The facade builds the sheet list in page number order.
 *
 * @par 3. Capture the text context (FACADE::updateModel in conn_facade.cpp)
 * Label text can contain variables that read the project, the title block or other sheets. A
 * screen revision does not change when these sources change. The facade thus must capture the
 * complete text context and compare it against the previous one. It also calls
 * SCH_CONNECTIVITY::ENGINE::ExternalSourcesChanged() for sources outside the model (e.g. environment
 * variables).
 *
 * @par 4. Clear the stages (SCH_CONNECTIVITY::ENGINE::Update in conn_engine.cpp)
 * This rebuild clears the record, bundle, slot and signal caches.
 *
 * @par 5. Capture inputs (SCH_CONNECTIVITY::INPUT_STORE::Capture in conn_inputs.cpp)
 * SCH_CONNECTIVITY::CaptureHierarchy() in conn_frame.h makes one SCH_CONNECTIVITY::FRAME_INSTANCE
 * for each sheet instance. SCH_CONNECTIVITY::ExtractScreenFacts() in conn_facts.h copies each
 * screen once into SCH_CONNECTIVITY::SCREEN_FACTS and each instance then resolves its own text
 * for SCH_CONNECTIVITY::INSTANCE_FACTS.
 *
 * @par 6. Build island records (SCH_CONNECTIVITY::RECORD_STORE::Update in conn_records.cpp)
 * An island is the smallest set of items that the drawing connects. Each item in an island can
 * claim a name.
 *
 * @par 7. Bind buses (SCH_CONNECTIVITY::PARTITIONER and SCH_CONNECTIVITY::BindBundle)
 * SCH_CONNECTIVITY::RecordNodes() in conn_components.h converts bus records and their name edges
 * into graph nodes.
 *
 * @par 8. Version member slots (SCH_CONNECTIVITY::SLOT_STORE::Update in conn_bindings.cpp)
 * Each bus member becomes a SCH_CONNECTIVITY::SLOT_INPUT. A slot joins a bus member to the nets
 * that use the same name.
 *
 * @par 9. Derive nets (SCH_CONNECTIVITY::DeriveSignal in conn_signals.cpp)
 * SCH_CONNECTIVITY::SignalNodes() combines net records and slots into graph nodes, and the
 * partitioner unites them again. SCH_CONNECTIVITY::SUMMARY in conn_summary.h then selects the
 * strongest claim by SCH_CONNECTIVITY::PRIORITY.
 *
 * @par 10. Publish (SCH_CONNECTIVITY::PUBLICATION::Update in conn_publish.cpp)
 * The publication compares the new components to the previous ones. It finds renamed, merged and
 * split nets, and it gives each successor the net code of its predecessor. The result of the
 * comparison is a SCH_CONNECTIVITY::CHANGE_SET in conn_changes.h.
 *
 * @par 11. Record live sources (FACADE::Update in conn_facade.cpp)
 * The facade stores a weak pointer to SCH_SCREEN::CONNECTIVITY_SOURCE for each instance. Any later
 * query can use it to detect a deleted screen.
 *
 * @par 12. Rebuild net chains (SCH_CONNECTIVITY::NETCHAIN_MANAGER::Rebuild)
 * Net chains join nets through pass-through components. They depend on published net names, so
 * the facade rebuilds them after publication and before it applies netclasses.
 *
 * @par 13. Apply results (SCH_CONNECTIVITY::FACADE::ApplyNetclasses in conn_facade.cpp)
 * The facade writes the label netclass assignments to NET_SETTINGS and clears only the changed
 * net caches.
 *
 * @par 14. Notify (SCH_CONNECTIVITY::SUBSCRIPTION in conn_subscription.h)
 * The facade sends the SCH_CONNECTIVITY::CHANGE_SET to each subscriber. This repaints the screen,
 * updates highlights and the net navigator.
 *
 *
 * @section sch_conn_modify Modification
 *
 * Modifications use the same pipeline without step 4 of the full update. Each stage compares
 * input versions and calculates only the entries that changed. The union-find collects all nodes
 * on every update, so each step keeps a complete record.
 *
 * @code{.unparsed}
 *   [1]  tool edits an item through SCH_COMMIT                   sch_commit.cpp
 *          |
 *          v
 *   [2]  SCH_COMMIT::Stage  -->  BumpConnectivityRevision        sch_screen.h
 *          |  the facade now rejects queries on this screen
 *          v
 *   [3]  SCH_COMMIT::pushSchEdit collects changed screens        sch_commit.cpp
 *          |
 *          v
 *   [4]  SCHEMATIC::CleanUpConnections( changed screens )        schematic.cpp
 *          |
 *          v
 *   [5]  SCHEMATIC::RecalculateConnections                       schematic.cpp
 *          |
 *          +---- GLOBAL_CLEANUP or incremental off ---->  full update
 *          |
 *          v
 *   [6]  FACADE::Recalculate( aRebuild = false )                 conn_facade.cpp
 *          |
 *   =======|================== ENGINE::Update =================  conn_engine.cpp
 *          v
 *   [7]  INPUT_STORE extracts screens whose revision changed     conn_inputs.cpp
 *          |
 *          v
 *   [8]  RECORD_STORE folds records whose inputs changed         conn_records.cpp
 *          |
 *          v
 *   [9]  PARTITIONER unites all nodes                            conn_partition.h
 *        COMPONENT_CACHE evaluates cache misses only             conn_component_cache.h
 *          |
 *          v
 *   [10] PUBLICATION keeps successor codes, builds CHANGE_SET    conn_publish.cpp
 *   =======|===================================================
 *          v
 *   [11] FACADE applies results, notifies handler and listeners  conn_facade.cpp
 *          |
 *          v
 *   [12] SCH_EDIT_FRAME::RefreshConnectivity( CHANGE_SET )       sch_edit_frame.cpp
 * @endcode
 *
 * @par 1. Stage the edit (SCH_COMMIT::Stage in sch_commit.cpp)
 *
 * @par 2. Bump the revision (SCH_SCREEN::BumpConnectivityRevision in sch_screen.h)
 * The commit increments the revision of the screen that holds a connectivity source. A batch that
 * must read connectivity while it stages edits holds a SCH_CONNECTIVITY::PUBLICATION_HOLD.
 *
 * @par 3. Collect changed screens (SCH_COMMIT::pushSchEdit in sch_commit.cpp)
 *
 * @par 4. Clean up (SCHEMATIC::CleanUpConnections in schematic.cpp)
 *
 * @par 5. Select the update (SCHEMATIC::RecalculateConnections in schematic.cpp)
 *
 * @par 6. Update without a rebuild (SCH_CONNECTIVITY::FACADE::Recalculate)
 * The facade does steps 2 and 3 of the full update. The text epoch changes only if the text
 * context changed, so most edits keep all cached instance text.
 *
 * @par 7. Extract changed screens (SCH_CONNECTIVITY::INPUT_STORE in conn_inputs.cpp)
 * INPUT_STORE::Screen() extracts a screen again only if its revision changed.
 *
 * @par 8. Rebuild changed records (SCH_CONNECTIVITY::RECORD_STORE in conn_records.cpp)
 * The record store skips each instance whose fact and text versions did not change.
 *
 * @par 9. Partition and evaluate (SCH_CONNECTIVITY::COMPONENT_CACHE in conn_component_cache.h)
 * The partitioner unites all bus nodes and then all net nodes again.
 *
 * @par 10. Publish with continuity (SCH_CONNECTIVITY::PUBLICATION in conn_publish.cpp)
 * An unchanged component keeps its content pointer, name and net code. For a changed component,
 * the publication finds the old component with the largest shared membership. The new component
 * continues that net code. The SCH_CONNECTIVITY::CHANGE_SET lists modified items.
 *
 * @par 11. Apply and notify (SCH_CONNECTIVITY::FACADE::Recalculate)
 * The facade does steps 11 to 14 of the full update. The item handler sees only changed rows and
 * changed dangling state, so an edit repaints only the items that it affects.
 *
 * @par 12. Refresh the editor (SCH_EDIT_FRAME::RefreshConnectivity in sch_edit_frame.cpp)
 */