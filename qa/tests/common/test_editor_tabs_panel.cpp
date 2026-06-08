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

#define BOOST_TEST_NO_MAIN
#include <boost/test/unit_test.hpp>

#include <algorithm>

#include <wx/display.h>
#include <wx/frame.h>
#include <wx/utils.h>

#include <widgets/editor_tabs_panel.h>
#include <widgets/kicad_tab_art.h>


/// The panel owns real tab-control child windows, so it needs a display
static bool displayAvailable()
{
#ifdef __WXGTK__
    // Probing the display with no session emits GDK noise, so check the environment first.
    if( !wxGetEnv( wxT( "DISPLAY" ), nullptr ) && !wxGetEnv( wxT( "WAYLAND_DISPLAY" ), nullptr ) )
        return false;

    return wxDisplay::GetCount() > 0;
#else
    return true;
#endif
}

BOOST_AUTO_TEST_SUITE( EditorTabsPanel )


BOOST_AUTO_TEST_CASE( TabVisualState )
{
    BOOST_CHECK( ResolveTabVisualState( /*preview*/ true, false, false ).italic );

    TAB_VISUAL_STATE mod = ResolveTabVisualState( false, /*modified*/ true, false );
    BOOST_CHECK( mod.bold && mod.showAsterisk && !mod.italic );

    // Pinning carries no glyph, so a pinned unmodified tab is plain.
    TAB_VISUAL_STATE pinned = ResolveTabVisualState( false, false, /*pinned*/ true );
    BOOST_CHECK( !pinned.italic && !pinned.bold && !pinned.showAsterisk );

    TAB_VISUAL_STATE plain = ResolveTabVisualState( false, false, false );
    BOOST_CHECK( !plain.italic && !plain.bold && !plain.showAsterisk );
}


BOOST_AUTO_TEST_CASE( TabVisualStateModifiedPreviewIsModified )
{
    // A modified document is never shown as a preview, so modified wins over italic.
    TAB_VISUAL_STATE st = ResolveTabVisualState( /*preview*/ true, /*modified*/ true, false );

    BOOST_CHECK( st.bold );
    BOOST_CHECK( st.showAsterisk );
    BOOST_CHECK( !st.italic );
}


BOOST_AUTO_TEST_CASE( PreviewReuse )
{
    EDITOR_TABS_MODEL m;
    int               a = m.OpenDocument( "lib:A", /*asPreview*/ true );
    int               b = m.OpenDocument( "lib:B", /*asPreview*/ true );

    BOOST_CHECK_EQUAL( a, b );
    BOOST_CHECK_EQUAL( m.Entries().size(), 1u );
    BOOST_CHECK_EQUAL( m.Entries()[0].key, "lib:B" );
    BOOST_CHECK( m.Entries()[0].preview );
}


BOOST_AUTO_TEST_CASE( EditPromotesPreview )
{
    EDITOR_TABS_MODEL m;
    m.OpenDocument( "lib:A", true );
    m.MarkModified( "lib:A", true );

    int c = m.OpenDocument( "lib:B", true ); // must not replace the now-modified A

    BOOST_CHECK_EQUAL( m.Entries().size(), 2u );
    BOOST_CHECK_EQUAL( c, 1 );
    BOOST_CHECK( !m.Entries()[0].preview );
}


BOOST_AUTO_TEST_CASE( PinExemptsFromReplacement )
{
    EDITOR_TABS_MODEL m;
    m.OpenDocument( "lib:A", true );
    m.Pin( "lib:A", true );
    m.OpenDocument( "lib:B", true );

    BOOST_CHECK_EQUAL( m.Entries().size(), 2u );
}


BOOST_AUTO_TEST_CASE( PromoteClearsPreviewWithoutPinning )
{
    EDITOR_TABS_MODEL m;
    m.OpenDocument( "lib:A", true );

    BOOST_CHECK( m.Entries()[0].preview );
    BOOST_CHECK_EQUAL( m.PreviewIndex(), 0 );

    m.Promote( "lib:A" );

    BOOST_CHECK( !m.Entries()[0].preview );
    BOOST_CHECK( !m.Entries()[0].pinned );
    BOOST_CHECK_EQUAL( m.PreviewIndex(), -1 );

    // A promoted tab is no longer reused, so the next preview opens its own tab.
    m.OpenDocument( "lib:B", true );
    BOOST_CHECK_EQUAL( m.Entries().size(), 2u );
}


BOOST_AUTO_TEST_CASE( ModifiedNeedsPrompt )
{
    EDITOR_TABS_MODEL m;
    m.OpenDocument( "lib:A", false );
    BOOST_CHECK( m.CanCloseWithoutPrompt( "lib:A" ) );

    m.MarkModified( "lib:A", true );
    BOOST_CHECK( !m.CanCloseWithoutPrompt( "lib:A" ) );
}


BOOST_AUTO_TEST_CASE( ReopenExistingKeyDoesNotDuplicate )
{
    EDITOR_TABS_MODEL m;
    int               a = m.OpenDocument( "lib:A", false );
    int               again = m.OpenDocument( "lib:A", false );

    BOOST_CHECK_EQUAL( a, again );
    BOOST_CHECK_EQUAL( m.Entries().size(), 1u );
}


BOOST_AUTO_TEST_CASE( PermanentOpenDoesNotReusePreview )
{
    EDITOR_TABS_MODEL m;
    m.OpenDocument( "lib:A", /*asPreview*/ true );
    int b = m.OpenDocument( "lib:B", /*asPreview*/ false );

    BOOST_CHECK_EQUAL( m.Entries().size(), 2u );
    BOOST_CHECK_EQUAL( b, 1 );
    BOOST_CHECK( !m.Entries()[1].preview );
}


BOOST_AUTO_TEST_CASE( ModifiedPreviewBecomesPermanentForReuse )
{
    EDITOR_TABS_MODEL m;
    m.OpenDocument( "lib:A", true );
    m.MarkModified( "lib:A", true );
    m.MarkModified( "lib:A", false );

    // The tab stays permanent once promoted, even after the dirty flag clears.
    int b = m.OpenDocument( "lib:B", true );
    BOOST_CHECK_EQUAL( m.Entries().size(), 2u );
    BOOST_CHECK_EQUAL( b, 1 );
}


BOOST_AUTO_TEST_CASE( CloseRemovesEntryAndClearsPreviewIndex )
{
    EDITOR_TABS_MODEL m;
    m.OpenDocument( "lib:A", true );
    m.CloseDocument( "lib:A" );

    BOOST_CHECK_EQUAL( m.Entries().size(), 0u );

    // The preview slot is freed, so the next preview opens fresh.
    int b = m.OpenDocument( "lib:B", true );
    BOOST_CHECK_EQUAL( b, 0 );
    BOOST_CHECK_EQUAL( m.Entries().size(), 1u );
}


BOOST_AUTO_TEST_CASE( PreviewReuseForgetsOldMruKey )
{
    if( !displayAvailable() )
        return;

    wxFrame*           frame = new wxFrame( nullptr, wxID_ANY, "tabs-test" );
    EDITOR_TABS_PANEL* panel = new EDITOR_TABS_PANEL( frame, /*aSharedCanvas*/ nullptr );

    // Reusing the preview slot must drop the old key from the MRU so Ctrl+Tab never lands on a
    // tab that no longer exists.
    panel->AddTab( "lib:A", "A", /*aAsPreview*/ true );
    panel->AddTab( "lib:B", "B", /*aAsPreview*/ true );

    const std::vector<wxString>& mru = panel->MruForTest();

    BOOST_CHECK( std::find( mru.begin(), mru.end(), wxString( "lib:A" ) ) == mru.end() );
    BOOST_CHECK( std::find( mru.begin(), mru.end(), wxString( "lib:B" ) ) != mru.end() );
    BOOST_CHECK_EQUAL( panel->Model().Entries().size(), 1u );

    frame->Destroy();
}


BOOST_AUTO_TEST_CASE( CloseToRightGuardsNegativeAnchor )
{
    if( !displayAvailable() )
        return;

    wxFrame*           frame = new wxFrame( nullptr, wxID_ANY, "tabs-test" );
    EDITOR_TABS_PANEL* panel = new EDITOR_TABS_PANEL( frame, /*aSharedCanvas*/ nullptr );

    panel->AddTab( "lib:A", "A", false );
    panel->AddTab( "lib:B", "B", false );

    // A negative anchor must be a no-op, not a close-everything sweep.
    panel->CloseToRight( -1 );

    BOOST_CHECK_EQUAL( panel->Model().Entries().size(), 2u );

    panel->CloseToRight( 0 );
    BOOST_CHECK_EQUAL( panel->Model().Entries().size(), 1u );

    frame->Destroy();
}


BOOST_AUTO_TEST_CASE( SuppressActivateOnCloseSkipsFallbackActivation )
{
    if( !displayAvailable() )
        return;

    wxFrame*           frame = new wxFrame( nullptr, wxID_ANY, "tabs-test" );
    EDITOR_TABS_PANEL* panel = new EDITOR_TABS_PANEL( frame, /*aSharedCanvas*/ nullptr );

    int  activateCount = 0;
    int  lastActivated = -1;
    int  closeRequests = 0;

    panel->onActivateTab =
            [&]( int aIdx )
            {
                ++activateCount;
                lastActivated = aIdx;
            };

    panel->onCloseTabRequested =
            [&]( int ) -> bool
            {
                ++closeRequests;
                return true;
            };

    // When the host installs the successor itself, the panel must not also re-activate the fallback.
    panel->SetSuppressActivateOnClose( true );

    panel->AddTab( "lib:A", "A", false );
    panel->AddTab( "lib:B", "B", false );

    // AddTab activates each opened tab, so reset the counters to isolate the close behaviour.
    activateCount = 0;
    lastActivated = -1;

    panel->CloseTab( 1 );

    BOOST_CHECK_EQUAL( closeRequests, 1 );
    BOOST_CHECK_EQUAL( activateCount, 0 );
    BOOST_CHECK_EQUAL( panel->Model().Entries().size(), 1u );

    // The remaining tab is still selected visually even though no host activation fired.
    BOOST_CHECK_EQUAL( panel->GetActiveTab(), 0 );

    frame->Destroy();
}


BOOST_AUTO_TEST_CASE( SuppressActivateOnCloseKeepsActiveTabOnNonActiveClose )
{
    if( !displayAvailable() )
        return;

    wxFrame*           frame = new wxFrame( nullptr, wxID_ANY, "tabs-test" );
    EDITOR_TABS_PANEL* panel = new EDITOR_TABS_PANEL( frame, /*aSharedCanvas*/ nullptr );

    int activateCount = 0;

    panel->onActivateTab = [&]( int ) { ++activateCount; };
    panel->onCloseTabRequested = []( int ) -> bool { return true; };
    panel->SetSuppressActivateOnClose( true );

    panel->AddTab( "lib:A", "A", false );
    panel->AddTab( "lib:B", "B", false );
    panel->AddTab( "lib:C", "C", false );

    BOOST_REQUIRE_EQUAL( panel->GetActiveTab(), 2 );

    activateCount = 0;
    panel->CloseTab( 0 );

    BOOST_CHECK_EQUAL( activateCount, 0 );
    BOOST_REQUIRE_EQUAL( panel->Model().Entries().size(), 2u );
    BOOST_CHECK_EQUAL( panel->Model().Entries()[panel->GetActiveTab()].key, wxString( "lib:C" ) );

    frame->Destroy();
}


BOOST_AUTO_TEST_CASE( DefaultCloseStillActivatesFallback )
{
    if( !displayAvailable() )
        return;

    wxFrame*           frame = new wxFrame( nullptr, wxID_ANY, "tabs-test" );
    EDITOR_TABS_PANEL* panel = new EDITOR_TABS_PANEL( frame, /*aSharedCanvas*/ nullptr );

    int activateCount = 0;
    int lastActivated = -1;

    panel->onActivateTab =
            [&]( int aIdx )
            {
                ++activateCount;
                lastActivated = aIdx;
            };

    panel->onCloseTabRequested = []( int ) -> bool { return true; };

    // Default contract: the panel re-activates the fallback after a close so the host can install
    // the successor lazily from that activation.
    panel->AddTab( "lib:A", "A", false );
    panel->AddTab( "lib:B", "B", false );

    activateCount = 0;
    lastActivated = -1;

    panel->CloseTab( 1 );

    BOOST_CHECK_EQUAL( activateCount, 1 );
    BOOST_CHECK_EQUAL( lastActivated, 0 );

    frame->Destroy();
}


BOOST_AUTO_TEST_CASE( PinChangeNotifiesHost )
{
    if( !displayAvailable() )
        return;

    wxFrame*           frame = new wxFrame( nullptr, wxID_ANY, "tabs-test" );
    EDITOR_TABS_PANEL* panel = new EDITOR_TABS_PANEL( frame, /*aSharedCanvas*/ nullptr );

    int  calls = 0;
    int  lastIdx = -1;
    bool lastPinned = false;

    panel->onPinChanged =
            [&]( int aIdx, bool aPinned )
            {
                ++calls;
                lastIdx = aIdx;
                lastPinned = aPinned;
            };

    panel->AddTab( "lib:A", "A", false );

    // Every pin change funnels through SetPinned, so the host is notified once and can mirror the flag
    // into its persisted tab context. The right-click menu and the pin action share this path.
    panel->SetPinned( 0, true );

    BOOST_CHECK_EQUAL( calls, 1 );
    BOOST_CHECK_EQUAL( lastIdx, 0 );
    BOOST_CHECK( lastPinned );
    BOOST_CHECK( panel->Model().Entries()[0].pinned );

    panel->SetPinned( 0, false );

    BOOST_CHECK_EQUAL( calls, 2 );
    BOOST_CHECK( !lastPinned );
    BOOST_CHECK( !panel->Model().Entries()[0].pinned );

    frame->Destroy();
}


BOOST_AUTO_TEST_SUITE_END()
