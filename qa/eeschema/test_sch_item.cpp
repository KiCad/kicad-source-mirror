/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <eda_item_test_utils.h>
#include <core/typeinfo.h>

// Code under test
#include <sch_item.h>
//#include <sch_marker.h>
#include <sch_junction.h>
#include <sch_no_connect.h>
#include <sch_bus_entry.h>
#include <sch_line.h>
#include <sch_bitmap.h>
#include <sch_text.h>
#include <sch_field.h>
#include <sch_symbol.h>
#include <sch_sheet_pin.h>
#include <sch_sheet.h>


class TEST_SCH_ITEM_FIXTURE
{
public:
    TEST_SCH_ITEM_FIXTURE() : m_sheet()
    {
        m_sheet.SetPosition( wxPoint( Millimeter2iu( 5 ), Millimeter2iu( 10 ) ) );
        m_sheet.SetSize( wxSize( Millimeter2iu( 50 ), Millimeter2iu( 100 ) ) );
    }

    SCH_SHEET m_sheet;
};


static SCH_ITEM* Instatiate( KICAD_T aType, SCH_SHEET* sheet )
{
    if( !IsEeschemaType( aType ) )
        return nullptr;

    if( !IsInstatiableType( aType ) )
        return nullptr;

    switch( aType )
    {
    case SCH_MARKER_T: return nullptr;
    case SCH_JUNCTION_T: return new SCH_JUNCTION();
    case SCH_NO_CONNECT_T: return new SCH_NO_CONNECT();
    case SCH_BUS_WIRE_ENTRY_T: return new SCH_BUS_WIRE_ENTRY();
    case SCH_BUS_BUS_ENTRY_T: return new SCH_BUS_BUS_ENTRY();
    case SCH_LINE_T: return new SCH_LINE();
    case SCH_BITMAP_T: return new SCH_BITMAP();
    case SCH_TEXT_T: return new SCH_TEXT( wxPoint( 0, 0 ), "test text" );
    case SCH_LABEL_T: return new SCH_LABEL( wxPoint( 0, 0 ), "test label" );
    case SCH_GLOBAL_LABEL_T: return new SCH_GLOBALLABEL();
    case SCH_HIER_LABEL_T: return new SCH_HIERLABEL();
    case SCH_FIELD_T: return new SCH_FIELD( wxPoint( 0, 0 ), 0, nullptr );
    case SCH_SYMBOL_T: return new SCH_SYMBOL();

    case SCH_SHEET_PIN_T:
        // XXX (?): Sheet pins currently have to have their initial positions calculated manually.
        return new SCH_SHEET_PIN(
                sheet,
                wxPoint( sheet->GetPosition().x, sheet->GetPosition().y + Millimeter2iu( 40 ) ),
                "test pin" );

    case SCH_SHEET_T:
    {
        SCH_SHEET* sheet = new SCH_SHEET();
        sheet->SetSize( wxSize( Millimeter2iu( 100 ), Millimeter2iu( 50 ) ) );

        // XXX (?): Sheet fields currently have to be positioned with an additional method call.
        sheet->AutoplaceFields( nullptr, false );
        return sheet;
    }

    case SCH_PIN_T:
    case SCHEMATIC_T:
        // TODO
        return nullptr;

    // `LIB_ITEM`s aren't handled in this module.
    case LIB_SYMBOL_T:
    case LIB_ALIAS_T:
    case LIB_SHAPE_T:
    case LIB_TEXT_T:
    case LIB_PIN_T:
    case LIB_FIELD_T:
        return nullptr;

    default:
        BOOST_FAIL( wxString::Format( "Unhandled type: %d", aType ) );
        return nullptr;
    }
}


static void CompareItems( SCH_ITEM* aItem, SCH_ITEM* aOriginalItem )
{
    BOOST_CHECK_EQUAL( aItem->GetPosition(), aOriginalItem->GetPosition() );
    BOOST_CHECK_EQUAL( aItem->GetBoundingBox().GetTop(),
                       aOriginalItem->GetBoundingBox().GetTop() );
    BOOST_CHECK_EQUAL( aItem->GetBoundingBox().GetLeft(),
                       aOriginalItem->GetBoundingBox().GetLeft() );
    BOOST_CHECK_EQUAL( aItem->GetBoundingBox().GetBottom(),
                       aOriginalItem->GetBoundingBox().GetBottom() );
    BOOST_CHECK_EQUAL( aItem->GetBoundingBox().GetRight(),
                       aOriginalItem->GetBoundingBox().GetRight() );
}


/**
 * Declare the test suite
 */
BOOST_FIXTURE_TEST_SUITE( SchItem, TEST_SCH_ITEM_FIXTURE )


BOOST_AUTO_TEST_CASE( Move )
{
    for( int i = 0; i < MAX_STRUCT_TYPE_ID; i++ )
    {
        KICAD_T type = static_cast<KICAD_T>( i );

        auto item = std::unique_ptr<SCH_ITEM>( Instatiate( type, &m_sheet ) );

        if( item == nullptr )
            continue;

        BOOST_TEST_CONTEXT( "Class: " << item->GetClass() )
        {
            IterateOverPositionsAndReferences<SCH_ITEM>(
                    item.get(),
                    []( SCH_ITEM* aOriginalItem, wxPoint aOffset )
                    {
                        auto item = std::unique_ptr<SCH_ITEM>( aOriginalItem->Duplicate() );

                        item->Move( aOffset );
                        item->Move( -aOffset );

                        CompareItems( item.get(), aOriginalItem );
                    } );
        }
    }
}


BOOST_AUTO_TEST_CASE( Rotate )
{
    for( int i = 0; i < MAX_STRUCT_TYPE_ID; i++ )
    {
        KICAD_T type = static_cast<KICAD_T>( i );

        auto item = std::unique_ptr<SCH_ITEM>( Instatiate( type, &m_sheet ) );

        if( item == nullptr )
            continue;

        BOOST_TEST_CONTEXT( "Class: " << item->GetClass() )
        {
            if( item->GetClass() == "SCH_SHEET_PIN" )
            {
                auto newItem = std::unique_ptr<SCH_ITEM>( item->Duplicate() );

                // Only rotating pins around the center of parent sheet works.
                item->Rotate( m_sheet.GetBodyBoundingBox().GetCenter() );
                item->Rotate( m_sheet.GetBodyBoundingBox().GetCenter() );
                item->Rotate( m_sheet.GetBodyBoundingBox().GetCenter() );
                item->Rotate( m_sheet.GetBodyBoundingBox().GetCenter() );

                CompareItems( newItem.get(), item.get() );
            }
            else
            {
                IterateOverPositionsAndReferences<SCH_ITEM>(
                        item.get(),
                        []( SCH_ITEM* aOriginalItem, wxPoint aRef )
                        {
                            auto item = std::unique_ptr<SCH_ITEM>( aOriginalItem->Duplicate() );

                            // Four rotations are an identity.
                            item->Rotate( aRef );
                            item->Rotate( aRef );
                            item->Rotate( aRef );
                            item->Rotate( aRef );

                            CompareItems( item.get(), aOriginalItem );
                        } );
            }
        }
    }
}


BOOST_AUTO_TEST_CASE( MirrorHorizontally )
{
    for( int i = 0; i < MAX_STRUCT_TYPE_ID; i++ )
    {
        KICAD_T type = static_cast<KICAD_T>( i );

        auto item = std::unique_ptr<SCH_ITEM>( Instatiate( type, &m_sheet ) );

        if( item == nullptr )
            continue;

        BOOST_TEST_CONTEXT( "Class: " << item->GetClass() )
        {
            IterateOverPositionsAndReferences<SCH_ITEM>(
                    item.get(),
                    []( SCH_ITEM* aOriginalItem, wxPoint aRef )
                    {
                        auto item = std::unique_ptr<SCH_ITEM>( aOriginalItem->Duplicate() );

                        // Two mirrorings are an identity.
                        item->MirrorHorizontally( aRef.x );
                        item->MirrorHorizontally( aRef.x );

                        CompareItems( item.get(), aOriginalItem );
                    } );
        }
    }
}


BOOST_AUTO_TEST_CASE( MirrorVertically )
{
    for( int i = 0; i < MAX_STRUCT_TYPE_ID; i++ )
    {
        KICAD_T type = static_cast<KICAD_T>( i );

        auto item = std::unique_ptr<SCH_ITEM>( Instatiate( type, &m_sheet ) );
        auto originalItem = std::unique_ptr<SCH_ITEM>( Instatiate( type, &m_sheet ) );

        if( item == nullptr )
            continue;

        BOOST_TEST_CONTEXT( "Class: " << item->GetClass() )
        {
            IterateOverPositionsAndReferences<SCH_ITEM>(
                    item.get(),
                    []( SCH_ITEM* aOriginalItem, wxPoint aRef )
                    {
                        auto item = std::unique_ptr<SCH_ITEM>( aOriginalItem->Duplicate() );

                        // Two mirrorings are an identity.
                        item->MirrorVertically( aRef.x );
                        item->MirrorVertically( aRef.x );

                        CompareItems( item.get(), aOriginalItem );
                    } );
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
