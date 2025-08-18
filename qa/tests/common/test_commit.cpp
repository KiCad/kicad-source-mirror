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

#include <boost/test/unit_test.hpp>
#include <commit.h>
#include <undo_redo_container.h>
#include <eda_item.h>

// Minimal EDA_ITEM for testing
class TEST_EDA_ITEM : public EDA_ITEM
{
public:
    TEST_EDA_ITEM( KICAD_T aType ) : EDA_ITEM( aType ) {}

    wxString GetClass() const override { return wxT( "TEST_EDA_ITEM" ); }

    EDA_ITEM* Clone() const override { return new TEST_EDA_ITEM( Type() ); }
};

// Simple COMMIT implementation for testing
class TEST_COMMIT : public COMMIT
{
public:
    void Push( const wxString&, int ) override {}
    void Revert() override {}

private:
    EDA_ITEM* undoLevelItem( EDA_ITEM* aItem ) const override { return aItem; }
    EDA_ITEM* makeImage( EDA_ITEM* aItem ) const override { return aItem->Clone(); }
};

BOOST_AUTO_TEST_SUITE( Commit )

BOOST_AUTO_TEST_CASE( StageAndStatus )
{
    TEST_COMMIT commit;
    TEST_EDA_ITEM itemAdd( PCB_T );
    TEST_EDA_ITEM itemRemove( PCB_T );
    TEST_EDA_ITEM itemModify( PCB_T );

    commit.Add( &itemAdd );
    BOOST_CHECK_EQUAL( commit.GetStatus( &itemAdd ), CHT_ADD );

    commit.Remove( &itemRemove );
    BOOST_CHECK_EQUAL( commit.GetStatus( &itemRemove ), CHT_REMOVE );

    commit.Modify( &itemModify );
    TEST_EDA_ITEM* copy = static_cast<TEST_EDA_ITEM*>( itemModify.Clone() );
    commit.Modified( &itemModify, copy );
    BOOST_CHECK_EQUAL( commit.GetStatus( &itemModify ), CHT_MODIFY );
}

BOOST_AUTO_TEST_CASE( StageContainers )
{
    TEST_COMMIT commit;
    TEST_EDA_ITEM a( PCB_T );
    TEST_EDA_ITEM b( PCB_T );
    std::vector<EDA_ITEM*> items = { &a, &b };

    commit.Stage( items, CHT_ADD );

    BOOST_CHECK_EQUAL( commit.GetStatus( &a ), CHT_ADD );
    BOOST_CHECK_EQUAL( commit.GetStatus( &b ), CHT_ADD );
}

BOOST_AUTO_TEST_CASE( StagePickedItemsList )
{
    TEST_COMMIT commit;
    TEST_EDA_ITEM newItem( PCB_T );
    TEST_EDA_ITEM modItem( PCB_T );
    TEST_EDA_ITEM* modCopy = static_cast<TEST_EDA_ITEM*>( modItem.Clone() );

    PICKED_ITEMS_LIST list;
    ITEM_PICKER p1( nullptr, &newItem, UNDO_REDO::NEWITEM );
    list.PushItem( p1 );

    ITEM_PICKER p2( nullptr, &modItem, UNDO_REDO::CHANGED );
    p2.SetLink( modCopy );
    list.PushItem( p2 );

    commit.Stage( list );

    BOOST_CHECK_EQUAL( commit.GetStatus( &newItem ), CHT_ADD );
    BOOST_CHECK_EQUAL( commit.GetStatus( &modItem ), CHT_MODIFY );
}

BOOST_AUTO_TEST_CASE( UnstageRemovesNewItem )
{
    TEST_COMMIT commit;
    TEST_EDA_ITEM* item = new TEST_EDA_ITEM( PCB_T );
    item->SetFlags( IS_NEW );

    commit.Add( item );
    commit.Unstage( item, nullptr );

    BOOST_CHECK( commit.Empty() );
}

BOOST_AUTO_TEST_SUITE_END()

