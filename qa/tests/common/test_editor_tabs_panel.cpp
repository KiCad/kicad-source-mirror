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

#include <widgets/editor_tabs_panel.h>
#include <widgets/kicad_tab_art.h>


BOOST_AUTO_TEST_SUITE( EditorTabsPanel )


BOOST_AUTO_TEST_CASE( TabVisualState )
{
    BOOST_CHECK( ResolveTabVisualState( /*preview*/ true, false ).italic );

    TAB_VISUAL_STATE mod = ResolveTabVisualState( false, /*modified*/ true );
    BOOST_CHECK( mod.bold && mod.showAsterisk && !mod.italic );

    TAB_VISUAL_STATE plain = ResolveTabVisualState( false, false );
    BOOST_CHECK( !plain.italic && !plain.bold && !plain.showAsterisk );
}


BOOST_AUTO_TEST_CASE( TabVisualStateModifiedPreviewIsModified )
{
    // A modified document is never shown as a preview, so modified wins over italic.
    TAB_VISUAL_STATE st = ResolveTabVisualState( /*preview*/ true, /*modified*/ true );

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


BOOST_AUTO_TEST_CASE( PromoteClearsPreview )
{
    EDITOR_TABS_MODEL m;
    m.OpenDocument( "lib:A", true );

    BOOST_CHECK( m.Entries()[0].preview );
    BOOST_CHECK_EQUAL( m.PreviewIndex(), 0 );

    m.Promote( "lib:A" );

    BOOST_CHECK( !m.Entries()[0].preview );
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


BOOST_AUTO_TEST_SUITE_END()
