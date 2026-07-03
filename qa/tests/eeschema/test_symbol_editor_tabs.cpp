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

/// Headless tests for the Symbol Editor tabbed-document plumbing

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <symbol_library_manager.h>
#include <symbol_editor/symbol_edit_frame.h>
#include <symbol_editor/symbol_editor_tab_context.h>
#include <symbol_editor/symbol_editor_settings.h>

#include <lib_symbol.h>
#include <sch_screen.h>
#include <undo_redo_container.h>
#include <widgets/editor_tabs_panel.h>

#include <settings/json_settings_internals.h>

#include <nlohmann/json.hpp>


/// A LIB_SYMBOL that bumps a counter when destroyed, to prove the undo free path deletes its copies
class INSTRUMENTED_LIB_SYMBOL : public LIB_SYMBOL
{
public:
    INSTRUMENTED_LIB_SYMBOL( const wxString& aName, int* aDtorCounter ) :
            LIB_SYMBOL( aName ),
            m_dtorCounter( aDtorCounter )
    {
    }

    ~INSTRUMENTED_LIB_SYMBOL() override
    {
        if( m_dtorCounter )
            ( *m_dtorCounter )++;
    }

private:
    int* m_dtorCounter;
};


struct SYMBOL_EDITOR_TABS_TEST_FIXTURE
{
    SYMBOL_EDITOR_TABS_TEST_FIXTURE() {}

    /// Build a buffer over a fresh symbol and screen for the given name
    std::unique_ptr<SYMBOL_BUFFER> makeBuffer( const wxString& aName )
    {
        auto symbol = std::make_unique<LIB_SYMBOL>( aName );
        auto screen = std::make_unique<SCH_SCREEN>();

        return std::make_unique<SYMBOL_BUFFER>( std::move( symbol ), std::move( screen ) );
    }

    /// Push a UR_TRANSIENT-flagged symbol copy as a single-item command
    void pushTransientCommand( UNDO_REDO_CONTAINER& aList, LIB_SYMBOL* aTransient )
    {
        aTransient->SetFlags( UR_TRANSIENT );

        PICKED_ITEMS_LIST* cmd = new PICKED_ITEMS_LIST();
        cmd->PushItem( ITEM_PICKER( nullptr, aTransient, UNDO_REDO::LIBEDIT ) );
        aList.PushCommand( cmd );
    }

    /// Expose the private static free helper under test
    static void freeTransientUndoCommands( UNDO_REDO_CONTAINER& aList, const LIB_SYMBOL* aLiveSymbol )
    {
        SYMBOL_EDIT_FRAME::freeTransientUndoCommands( aList, aLiveSymbol );
    }

    /// Expose the library-tree visibility guard under test
    static bool libTreeAutoHiddenForSchematicEdit( bool aWasFromSchematic, bool aRestorePending,
                                                   bool aTreeShownNow )
    {
        return SYMBOL_EDIT_FRAME::libTreeAutoHiddenForSchematicEdit( aWasFromSchematic,
                                                                     aRestorePending, aTreeShownNow );
    }
};


BOOST_FIXTURE_TEST_SUITE( SymbolEditorTabs, SYMBOL_EDITOR_TABS_TEST_FIXTURE )


/// The context owns a working copy cloned from the buffer, seeds its dirty flag, and round-trips
/// the per-tab unit and body style
BOOST_AUTO_TEST_CASE( SymbolTabContextOwnsWorkingCopy )
{
    std::unique_ptr<SYMBOL_BUFFER> buf = makeBuffer( wxS( "R" ) );

    SYMBOL_EDITOR_TAB_CONTEXT ctx( wxS( "Device" ), wxS( "R" ), buf.get() );

    BOOST_CHECK_EQUAL( ctx.GetTabKey(), wxS( "Device:R" ) );
    BOOST_CHECK_EQUAL( ctx.GetDisplayName(), wxS( "R" ) );

    BOOST_REQUIRE( ctx.GetSymbol() != nullptr );
    BOOST_REQUIRE( ctx.GetScreen() != nullptr );
    BOOST_CHECK( ctx.GetSymbol() != &buf->GetSymbol() );
    BOOST_CHECK( ctx.GetScreen() != buf->GetScreen() );
    BOOST_CHECK_EQUAL( ctx.GetSymbol()->GetName(), wxS( "R" ) );

    BOOST_CHECK( !ctx.IsModified() );
    ctx.GetScreen()->SetContentModified();
    BOOST_CHECK( ctx.IsModified() );

    // Marking the buffer's screen must not affect the owned context.
    std::unique_ptr<SYMBOL_BUFFER> clean = makeBuffer( wxS( "C" ) );
    SYMBOL_EDITOR_TAB_CONTEXT      cleanCtx( wxS( "Device" ), wxS( "C" ), clean.get() );
    clean->GetScreen()->SetContentModified();
    BOOST_CHECK( !cleanCtx.IsModified() );

    BOOST_CHECK_EQUAL( ctx.GetUnit(), 1 );
    BOOST_CHECK_EQUAL( ctx.GetBodyStyle(), 1 );

    ctx.SetUnit( 2 );
    ctx.SetBodyStyle( 2 );
    BOOST_CHECK_EQUAL( ctx.GetUnit(), 2 );
    BOOST_CHECK_EQUAL( ctx.GetBodyStyle(), 2 );
}


/// An instance (schematic) tab owns its transient working symbol/screen, reports IsTransient(), and
/// keys off the source instance UUID in a namespace disjoint from library keys
BOOST_AUTO_TEST_CASE( InstanceTabContextIsTransientAndKeyedByUuid )
{
    KIID           sourceUuid;
    const wxString reference = wxS( "R5" );

    LIB_SYMBOL* symbol = new LIB_SYMBOL( wxS( "R" ) );
    SCH_SCREEN* screen = new SCH_SCREEN();

    SYMBOL_EDITOR_TAB_CONTEXT ctx( symbol, screen, sourceUuid, reference );

    BOOST_CHECK( ctx.IsTransient() );
    BOOST_CHECK( ctx.IsFromSchematic() );
    BOOST_CHECK_EQUAL( ctx.GetReference(), reference );
    BOOST_CHECK_EQUAL( ctx.GetSchematicSymbolUUID().AsString(), sourceUuid.AsString() );
    BOOST_CHECK_EQUAL( ctx.GetDisplayName(), reference );

    BOOST_CHECK_EQUAL( ctx.GetTabKey(),
                       SYMBOL_EDITOR_TAB_CONTEXT::MakeInstanceTabKey( sourceUuid ) );

    // The instance key cannot collide with any library:name key.
    BOOST_CHECK( ctx.GetTabKey() != SYMBOL_EDITOR_TAB_CONTEXT::MakeTabKey( wxS( "Device" ),
                                                                          wxS( "R" ) ) );
    BOOST_CHECK( ctx.GetTabKey().StartsWith( wxString( wxT( "\x01" ) ) ) );

    BOOST_CHECK( ctx.GetSymbol() == symbol );
    BOOST_CHECK( ctx.GetScreen() == screen );
}


/// Seeding from an already-modified buffer marks the new tab modified
BOOST_AUTO_TEST_CASE( SymbolTabContextSeedsDirtyFromBuffer )
{
    std::unique_ptr<SYMBOL_BUFFER> buf = makeBuffer( wxS( "R" ) );
    buf->GetScreen()->SetContentModified();

    SYMBOL_EDITOR_TAB_CONTEXT ctx( wxS( "Device" ), wxS( "R" ), buf.get() );
    BOOST_CHECK( ctx.IsModified() );
}


/// Open-tab persistence round-trips through the settings JSON, including the active-tab key
BOOST_AUTO_TEST_CASE( OpenTabsJsonRoundTrip )
{
    SYMBOL_EDITOR_SETTINGS source;

    source.m_OpenTabs.push_back( { wxS( "Device" ), wxS( "R" ), 1, 1 } );
    source.m_OpenTabs.push_back( { wxS( "Connector" ), wxS( "Conn_01x02" ), 2, 1 } );
    source.m_ActiveTabKey = wxS( "Connector:Conn_01x02" );

    BOOST_REQUIRE( source.Store() );

    std::string    serialized = source.FormatAsString();
    nlohmann::json reparsed = nlohmann::json::parse( serialized );

    BOOST_REQUIRE( reparsed.contains( "open_tabs" ) );
    BOOST_REQUIRE( reparsed["open_tabs"].is_array() );
    BOOST_CHECK_EQUAL( reparsed["open_tabs"].size(), 2u );

    SYMBOL_EDITOR_SETTINGS sink;

    // Seed a stale entry to catch a reader that fails to clear first.
    sink.m_OpenTabs.push_back( { wxS( "STALE" ), wxS( "X" ), 1, 1 } );

    JSON_SETTINGS_INTERNALS reparsedInternals;
    static_cast<nlohmann::json&>( reparsedInternals ) = reparsed;
    sink.Internals()->CloneFrom( reparsedInternals );
    sink.Load();

    BOOST_REQUIRE_EQUAL( sink.m_OpenTabs.size(), 2u );

    BOOST_CHECK_EQUAL( sink.m_OpenTabs[0].lib, wxS( "Device" ) );
    BOOST_CHECK_EQUAL( sink.m_OpenTabs[0].name, wxS( "R" ) );
    BOOST_CHECK_EQUAL( sink.m_OpenTabs[0].unit, 1 );

    BOOST_CHECK_EQUAL( sink.m_OpenTabs[1].lib, wxS( "Connector" ) );
    BOOST_CHECK_EQUAL( sink.m_OpenTabs[1].name, wxS( "Conn_01x02" ) );
    BOOST_CHECK_EQUAL( sink.m_OpenTabs[1].unit, 2 );

    BOOST_CHECK_EQUAL( sink.m_ActiveTabKey, wxS( "Connector:Conn_01x02" ) );
}


/// UR_TRANSIENT is flagged on the picked item, not the picker wrapper, so the container deleters that
/// gate on the wrapper flag leaked these copies. freeTransientUndoCommands walks the picked items and
/// deletes exactly those carrying UR_TRANSIENT, which this exercises across the undo and redo lists.
BOOST_AUTO_TEST_CASE( FreeTransientUndoCommandsDeletesCopies )
{
    int dtorCount = 0;

    UNDO_REDO_CONTAINER undo;
    UNDO_REDO_CONTAINER redo;

    pushTransientCommand( undo, new INSTRUMENTED_LIB_SYMBOL( wxS( "u1" ), &dtorCount ) );
    pushTransientCommand( undo, new INSTRUMENTED_LIB_SYMBOL( wxS( "u2" ), &dtorCount ) );
    pushTransientCommand( redo, new INSTRUMENTED_LIB_SYMBOL( wxS( "r1" ), &dtorCount ) );

    BOOST_REQUIRE_EQUAL( undo.m_CommandsList.size(), 2u );
    BOOST_REQUIRE_EQUAL( redo.m_CommandsList.size(), 1u );

    freeTransientUndoCommands( undo, nullptr );
    freeTransientUndoCommands( redo, nullptr );

    BOOST_CHECK_EQUAL( dtorCount, 3 );
    BOOST_CHECK_EQUAL( undo.m_CommandsList.size(), 0u );
    BOOST_CHECK_EQUAL( redo.m_CommandsList.size(), 0u );
}


/// The free helper must never delete the live working symbol, guarding the active-tab close path
/// against a double-free. The live item never carries UR_TRANSIENT, so it survives while a sibling
/// transient command is freed.
BOOST_AUTO_TEST_CASE( FreeTransientUndoCommandsSparesLiveSymbol )
{
    int dtorCount = 0;

    auto liveSymbol = std::make_unique<INSTRUMENTED_LIB_SYMBOL>( wxS( "live" ), &dtorCount );

    UNDO_REDO_CONTAINER undo;

    // A command referencing the live symbol without the transient flag, which the helper must skip.
    PICKED_ITEMS_LIST* liveCmd = new PICKED_ITEMS_LIST();
    liveCmd->PushItem( ITEM_PICKER( nullptr, liveSymbol.get(), UNDO_REDO::LIBEDIT ) );
    undo.PushCommand( liveCmd );

    pushTransientCommand( undo, new INSTRUMENTED_LIB_SYMBOL( wxS( "copy" ), &dtorCount ) );

    freeTransientUndoCommands( undo, liveSymbol.get() );

    BOOST_CHECK_EQUAL( dtorCount, 1 );
    BOOST_CHECK_EQUAL( undo.m_CommandsList.size(), 0u );

    liveSymbol.reset();
    BOOST_CHECK_EQUAL( dtorCount, 2 );
}


/// Saving a library clears the dirty state on both stores that drive the tab strip: the context's
/// screen flag (read on repaint) and the strip model entry (read on close).  A save that cleared only
/// the screen left the model starred, so the asterisk persisted until the next stray repaint.
BOOST_AUTO_TEST_CASE( ClearTabModifiedStateSyncsContextAndModel )
{
    std::unique_ptr<SYMBOL_BUFFER> buf = makeBuffer( wxS( "LMR16006" ) );

    SYMBOL_EDITOR_TAB_CONTEXT ctx( wxS( "proj_lib_01" ), wxS( "LMR16006" ), buf.get() );
    ctx.GetScreen()->SetContentModified();

    EDITOR_TABS_MODEL model;
    model.OpenDocument( ctx.GetTabKey(), /*asPreview*/ false );
    model.MarkModified( ctx.GetTabKey(), true );

    BOOST_REQUIRE( ctx.IsModified() );
    BOOST_REQUIRE( model.Entries()[0].modified );

    SYMBOL_EDIT_FRAME::clearTabModifiedState( ctx, model );

    BOOST_CHECK( !ctx.IsModified() );
    BOOST_CHECK( !model.Entries()[0].modified );
}


/// The library tree is auto-hidden while editing a symbol from the schematic (issue #23543). The
/// pending-restore flag must track that auto-hide so SaveSettings reinstates the tree, must survive
/// a chained schematic edit that finds the tree already hidden, and must not resurrect a tree the
/// user deliberately hid (a user toggle clears the flag before it reaches this helper).
BOOST_AUTO_TEST_CASE( LibTreeAutoHiddenRestoreAcrossSchematicEdit )
{
    // First schematic edit with the tree visible schedules its restore.
    BOOST_CHECK( libTreeAutoHiddenForSchematicEdit( false, false, true ) );

    // First schematic edit with the tree already hidden (no pending restore) schedules nothing.
    BOOST_CHECK( !libTreeAutoHiddenForSchematicEdit( false, false, false ) );

    // Chained schematic edit finds the tree auto-hidden by the previous one; the pending restore
    // must survive rather than drop.
    BOOST_CHECK( libTreeAutoHiddenForSchematicEdit( true, true, false ) );

    // A tree hidden without a pending restore (user toggled it off) stays unscheduled through a
    // chained edit, so SaveSettings will not resurrect it.
    BOOST_CHECK( !libTreeAutoHiddenForSchematicEdit( true, false, false ) );

    // A visible tree always schedules a restore regardless of the prior pending state.
    BOOST_CHECK( libTreeAutoHiddenForSchematicEdit( true, false, true ) );
}


BOOST_AUTO_TEST_SUITE_END()
