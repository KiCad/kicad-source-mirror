/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <dialogs/git/workspace_tree.h>


namespace
{
using KIND = WORKSPACE_TREE_NODE::KIND;


MDW_FOLDER folder( const wxString& aId, const wxString& aName, const wxString& aParent = wxEmptyString )
{
    MDW_FOLDER f;
    f.folder_id = aId;
    f.display_name = aName;
    f.parent_folder_id = aParent;
    return f;
}


MDW_ITEM_INFO item( const wxString& aId, const wxString& aName, const wxString& aFolder = wxEmptyString )
{
    MDW_ITEM_INFO i;
    i.item_id = aId;
    i.display_name = aName;
    i.folder_id = aFolder;
    return i;
}


std::vector<wxString> names( const WORKSPACE_TREE_NODE& aNode )
{
    std::vector<wxString> out;

    for( const WORKSPACE_TREE_NODE& child : aNode.children )
        out.push_back( child.name );

    return out;
}


size_t countKey( const WORKSPACE_TREE_NODE& aNode, const wxString& aKey )
{
    size_t n = aNode.key == aKey ? 1 : 0;

    for( const WORKSPACE_TREE_NODE& child : aNode.children )
        n += countKey( child, aKey );

    return n;
}
} // namespace


BOOST_AUTO_TEST_SUITE( WorkspaceTree )


BOOST_AUTO_TEST_CASE( NestsItemsUnderTheirFolders )
{
    WORKSPACE_TREE_NODE root = BuildWorkspaceTree( { folder( wxT( "e" ), wxT( "Elec" ) ),
                                                     folder( wxT( "s" ), wxT( "Sub" ), wxT( "e" ) ) },
                                                   { item( wxT( "b.kicad_pcb" ), wxT( "b.kicad_pcb" ), wxT( "s" ) ) },
                                                   {}, wxEmptyString );

    BOOST_REQUIRE_EQUAL( root.children.size(), 1u );
    BOOST_CHECK( root.children[0].kind == WORKSPACE_TREE_NODE::KIND::FOLDER );
    BOOST_CHECK_EQUAL( root.children[0].key, wxT( "e" ) );

    BOOST_REQUIRE_EQUAL( root.children[0].children.size(), 1u );
    const WORKSPACE_TREE_NODE& sub = root.children[0].children[0];
    BOOST_CHECK( sub.kind == WORKSPACE_TREE_NODE::KIND::FOLDER );
    BOOST_CHECK_EQUAL( sub.key, wxT( "s" ) );

    BOOST_REQUIRE_EQUAL( sub.children.size(), 1u );
    BOOST_CHECK( sub.children[0].kind == WORKSPACE_TREE_NODE::KIND::ITEM );
    BOOST_CHECK_EQUAL( sub.children[0].item, 0u );
}


BOOST_AUTO_TEST_CASE( FoldersSortBeforeItemsNaturally )
{
    WORKSPACE_TREE_NODE root = BuildWorkspaceTree( { folder( wxT( "z" ), wxT( "Z" ) ) },
                                                   { item( wxT( "1" ), wxT( "R10" ) ),
                                                     item( wxT( "2" ), wxT( "R2" ) ) },
                                                   {}, wxEmptyString );

    BOOST_CHECK( names( root ) == ( std::vector<wxString>{ wxT( "Z" ), wxT( "R2" ), wxT( "R10" ) } ) );
}


BOOST_AUTO_TEST_CASE( UnknownFolderGoesToRoot )
{
    WORKSPACE_TREE_NODE root =
            BuildWorkspaceTree( {}, { item( wxT( "1" ), wxT( "a.kicad_sch" ), wxT( "gone" ) ) }, {}, wxEmptyString );

    BOOST_REQUIRE_EQUAL( root.children.size(), 1u );
    BOOST_CHECK( root.children[0].kind == WORKSPACE_TREE_NODE::KIND::ITEM );
}


BOOST_AUTO_TEST_CASE( FolderCycleDoesNotHang )
{
    WORKSPACE_TREE_NODE root = BuildWorkspaceTree( { folder( wxT( "a" ), wxT( "A" ), wxT( "b" ) ),
                                                     folder( wxT( "b" ), wxT( "B" ), wxT( "a" ) ) },
                                                   { item( wxT( "1" ), wxT( "x.kicad_pcb" ), wxT( "a" ) ) }, {},
                                                   wxEmptyString );

    BOOST_CHECK_EQUAL( countKey( root, wxT( "1" ) ), 1 );

    // Both loop members sit at the root, the item stays in its folder
    BOOST_REQUIRE_EQUAL( root.children.size(), 2u );
    BOOST_CHECK_EQUAL( root.children[0].key, wxT( "a" ) );
    BOOST_CHECK_EQUAL( root.children[1].key, wxT( "b" ) );
    BOOST_CHECK_EQUAL( root.children[0].children.size(), 1u );
    BOOST_CHECK( root.children[1].children.empty() );
}


BOOST_AUTO_TEST_CASE( SelfParentFolderGoesToRoot )
{
    WORKSPACE_TREE_NODE root =
            BuildWorkspaceTree( { folder( wxT( "a" ), wxT( "A" ), wxT( "a" ) ) }, {}, {}, wxEmptyString );

    BOOST_REQUIRE_EQUAL( root.children.size(), 1u );
    BOOST_CHECK_EQUAL( root.children[0].key, wxT( "a" ) );
    BOOST_CHECK( root.children[0].children.empty() );
}


BOOST_AUTO_TEST_CASE( TailIntoACycleStaysNested )
{
    // c hangs off the a/b loop and is not part of it, and comes first so the walk starts on the tail
    WORKSPACE_TREE_NODE root = BuildWorkspaceTree( { folder( wxT( "c" ), wxT( "C" ), wxT( "a" ) ),
                                                     folder( wxT( "a" ), wxT( "A" ), wxT( "b" ) ),
                                                     folder( wxT( "b" ), wxT( "B" ), wxT( "a" ) ) },
                                                   {}, {}, wxEmptyString );

    BOOST_REQUIRE_EQUAL( root.children.size(), 2u );
    BOOST_CHECK_EQUAL( root.children[0].key, wxT( "a" ) );
    BOOST_CHECK_EQUAL( root.children[1].key, wxT( "b" ) );
    BOOST_REQUIRE_EQUAL( root.children[0].children.size(), 1u );
    BOOST_CHECK_EQUAL( root.children[0].children[0].key, wxT( "c" ) );
}


BOOST_AUTO_TEST_CASE( DuplicateFolderIdKeepsTheFirst )
{
    WORKSPACE_TREE_NODE root = BuildWorkspaceTree( { folder( wxT( "d" ), wxT( "First" ) ),
                                                     folder( wxT( "d" ), wxT( "Second" ) ) },
                                                   {}, {}, wxEmptyString );

    BOOST_REQUIRE_EQUAL( root.children.size(), 1u );
    BOOST_CHECK_EQUAL( root.children[0].name, wxT( "First" ) );
}


BOOST_AUTO_TEST_CASE( EmptyFolderKeptWithoutFilterDroppedWithOne )
{
    std::vector<MDW_FOLDER>    folders = { folder( wxT( "e" ), wxT( "Elec" ) ) };
    std::vector<MDW_ITEM_INFO> items = { item( wxT( "1" ), wxT( "a.md" ) ) };

    BOOST_CHECK_EQUAL( BuildWorkspaceTree( folders, items, {}, wxEmptyString ).children.size(), 2u );

    WORKSPACE_TREE_NODE filtered = BuildWorkspaceTree( folders, items, {}, wxT( "a.md" ) );

    BOOST_REQUIRE_EQUAL( filtered.children.size(), 1u );
    BOOST_CHECK_EQUAL( filtered.children[0].key, wxT( "1" ) );
}


BOOST_AUTO_TEST_CASE( FilterMatchesALibraryMember )
{
    WORKSPACE_TREE_LIBRARY lib{ wxT( "fp:Res" ), wxT( "Res" ), true, { wxT( "R_0603" ), wxT( "C_0603" ) } };
    WORKSPACE_TREE_LIBRARY other{ wxT( "fp:Cap" ), wxT( "Cap" ), true, { wxT( "C_0805" ) } };

    WORKSPACE_TREE_NODE root = BuildWorkspaceTree( {}, {}, { lib, other }, wxT( "r_06" ) );

    BOOST_REQUIRE_EQUAL( root.children.size(), 1u );
    BOOST_CHECK( root.children[0].kind == WORKSPACE_TREE_NODE::KIND::LIBRARIES );
    BOOST_REQUIRE_EQUAL( root.children[0].children.size(), 1u );
    BOOST_CHECK_EQUAL( root.children[0].children[0].key, wxT( "fp:Res" ) );
    BOOST_REQUIRE_EQUAL( root.children[0].children[0].children.size(), 1u );
    BOOST_CHECK_EQUAL( root.children[0].children[0].children[0].key, wxT( "fp:Res/R_0603" ) );
}


BOOST_AUTO_TEST_CASE( FilterKeepsAncestorsOfMatches )
{
    std::vector<MDW_FOLDER> folders = { folder( wxT( "e" ), wxT( "Elec" ) ),
                                        folder( wxT( "s" ), wxT( "Sub" ), wxT( "e" ) ) };
    std::vector<MDW_ITEM_INFO> items = { item( wxT( "board" ), wxT( "board.kicad_pcb" ), wxT( "s" ) ),
                                         item( wxT( "notes" ), wxT( "notes.md" ), wxT( "s" ) ) };

    WORKSPACE_TREE_NODE root = BuildWorkspaceTree( folders, items, {}, wxT( "BOARD" ) );

    BOOST_REQUIRE_EQUAL( root.children.size(), 1u );
    BOOST_REQUIRE_EQUAL( root.children[0].children.size(), 1u );
    const WORKSPACE_TREE_NODE& sub = root.children[0].children[0];
    BOOST_REQUIRE_EQUAL( sub.children.size(), 1u );
    BOOST_CHECK_EQUAL( sub.children[0].key, wxT( "board" ) );
    BOOST_CHECK_EQUAL( countKey( root, wxT( "notes" ) ), 0 );
}


BOOST_AUTO_TEST_CASE( UnnamedFolderShowsAndFiltersByItsId )
{
    std::vector<MDW_FOLDER>    folders = { folder( wxT( "Elec" ), wxEmptyString ) };
    std::vector<MDW_ITEM_INFO> items = { item( wxT( "1" ), wxT( "a.md" ), wxT( "Elec" ) ) };

    WORKSPACE_TREE_NODE root = BuildWorkspaceTree( folders, items, {}, wxT( "elec" ) );

    BOOST_REQUIRE_EQUAL( root.children.size(), 1u );
    BOOST_CHECK_EQUAL( root.children[0].name, wxT( "Elec" ) );
    BOOST_CHECK_EQUAL( countKey( root, wxT( "1" ) ), 1 );
}


BOOST_AUTO_TEST_CASE( FilterOnFolderKeepsItsSubtree )
{
    WORKSPACE_TREE_NODE root = BuildWorkspaceTree( { folder( wxT( "e" ), wxT( "Elec" ) ),
                                                     folder( wxT( "s" ), wxT( "Sub" ), wxT( "e" ) ),
                                                     folder( wxT( "o" ), wxT( "Other" ) ) },
                                                   { item( wxT( "1" ), wxT( "a.md" ), wxT( "s" ) ),
                                                     item( wxT( "2" ), wxT( "b.md" ), wxT( "e" ) ),
                                                     item( wxT( "3" ), wxT( "c.md" ), wxT( "o" ) ) },
                                                   {}, wxT( "elec" ) );

    BOOST_REQUIRE_EQUAL( root.children.size(), 1u );
    BOOST_CHECK_EQUAL( root.children[0].key, wxT( "e" ) );
    BOOST_CHECK_EQUAL( countKey( root, wxT( "1" ) ), 1 );
    BOOST_CHECK_EQUAL( countKey( root, wxT( "2" ) ), 1 );
    BOOST_CHECK_EQUAL( countKey( root, wxT( "3" ) ), 0 );
}


BOOST_AUTO_TEST_CASE( LibrariesAppearUnderOneGroup )
{
    WORKSPACE_TREE_LIBRARY zeta{ wxT( "sym:Zeta" ), wxT( "Zeta" ), false, {} };
    WORKSPACE_TREE_LIBRARY alpha{ wxT( "fp:Alpha" ), wxT( "Alpha" ), true, { wxT( "R_0603" ), wxT( "R_0402" ) } };

    WORKSPACE_TREE_NODE root =
            BuildWorkspaceTree( {}, { item( wxT( "1" ), wxT( "a.md" ) ) }, { zeta, alpha }, wxEmptyString );

    BOOST_REQUIRE_EQUAL( root.children.size(), 2u );
    const WORKSPACE_TREE_NODE& group = root.children.back();
    BOOST_CHECK( group.kind == WORKSPACE_TREE_NODE::KIND::LIBRARIES );
    BOOST_CHECK_EQUAL( group.name, wxT( "Team libraries" ) );

    BOOST_REQUIRE_EQUAL( group.children.size(), 2u );
    BOOST_CHECK( group.children[0].kind == WORKSPACE_TREE_NODE::KIND::LIBRARY );
    BOOST_CHECK_EQUAL( group.children[0].key, wxT( "fp:Alpha" ) );
    BOOST_CHECK_EQUAL( group.children[1].key, wxT( "sym:Zeta" ) );

    BOOST_REQUIRE_EQUAL( group.children[0].children.size(), 2u );
    BOOST_CHECK( group.children[0].children[0].kind == WORKSPACE_TREE_NODE::KIND::MEMBER );
    BOOST_CHECK_EQUAL( group.children[0].children[0].key, wxT( "fp:Alpha/R_0402" ) );
    BOOST_REQUIRE_EQUAL( group.children[1].children.size(), 1u );
    BOOST_CHECK( group.children[1].children[0].kind == WORKSPACE_TREE_NODE::KIND::UNLISTED );

    WORKSPACE_TREE_NODE bare = BuildWorkspaceTree( {}, { item( wxT( "1" ), wxT( "a.md" ) ) }, {}, wxEmptyString );

    BOOST_REQUIRE_EQUAL( bare.children.size(), 1u );
    BOOST_CHECK( bare.children[0].kind == WORKSPACE_TREE_NODE::KIND::ITEM );
}


// A source's search matched the files on the server, and the team libraries it does not reach are filtered here
BOOST_AUTO_TEST_CASE( MatchedItemsStillFilterLibraries )
{
    WORKSPACE_TREE_LIBRARY passives{ wxT( "sym:passives" ), wxT( "passives" ), true, { wxT( "R" ), wxT( "C" ) } };
    WORKSPACE_TREE_NODE    root = BuildWorkspaceTree( {}, { item( wxT( "1" ), wxT( "amp.kicad_sch" ) ) }, { passives },
                                                      wxT( "R" ), true );

    BOOST_REQUIRE_EQUAL( root.children.size(), 2u );
    BOOST_CHECK_EQUAL( root.children[0].name, wxT( "amp.kicad_sch" ) );

    const WORKSPACE_TREE_NODE& group = root.children[1];

    BOOST_REQUIRE_EQUAL( group.children.size(), 1u );
    BOOST_CHECK( names( group.children[0] ) == std::vector<wxString>{ wxT( "R" ) } );
}


BOOST_AUTO_TEST_CASE( PathLeadsToANestedItem )
{
    WORKSPACE_TREE_NODE root = BuildWorkspaceTree( { folder( wxT( "e" ), wxT( "Elec" ) ),
                                                     folder( wxT( "s" ), wxT( "Sub" ), wxT( "e" ) ) },
                                                   { item( wxT( "b.kicad_pcb" ), wxT( "b.kicad_pcb" ), wxT( "s" ) ) },
                                                   {}, wxEmptyString );

    std::vector<wxString> keys;

    for( const WORKSPACE_TREE_NODE* node : WorkspaceTreePath( root, { KIND::ITEM, wxT( "b.kicad_pcb" ) } ) )
        keys.push_back( node->key );

    BOOST_CHECK( keys == ( std::vector<wxString>{ wxT( "e" ), wxT( "s" ), wxT( "b.kicad_pcb" ) } ) );
    BOOST_CHECK( WorkspaceTreePath( root, { KIND::ITEM, wxT( "missing" ) } ).empty() );
}


// A file named like a member's key must not take the member's place when the selection comes back after a rebuild
BOOST_AUTO_TEST_CASE( PathTellsAFileFromAMemberWithItsKey )
{
    const wxString         key = wxT( "sym:passives/R" );
    WORKSPACE_TREE_LIBRARY passives{ wxT( "sym:passives" ), wxT( "passives" ), true, { wxT( "R" ) } };
    WORKSPACE_TREE_NODE    root = BuildWorkspaceTree( {}, { item( key, wxT( "R" ) ) }, { passives }, wxEmptyString );

    std::vector<const WORKSPACE_TREE_NODE*> member = WorkspaceTreePath( root, { KIND::MEMBER, key } );
    std::vector<const WORKSPACE_TREE_NODE*> file = WorkspaceTreePath( root, { KIND::ITEM, key } );

    BOOST_REQUIRE_EQUAL( member.size(), 3u );
    BOOST_CHECK( member.back()->kind == KIND::MEMBER );
    BOOST_CHECK( member.front()->kind == KIND::LIBRARIES );

    BOOST_REQUIRE_EQUAL( file.size(), 1u );
    BOOST_CHECK( file.back()->kind == KIND::ITEM );
}


BOOST_AUTO_TEST_SUITE_END()
