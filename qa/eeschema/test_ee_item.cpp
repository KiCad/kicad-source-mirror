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

#include <eda_item.h>
#include <sch_item.h>
#include <lib_item.h>

#include <sch_marker.h>
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

#include <lib_shape.h>
#include <lib_text.h>
#include <lib_pin.h>
#include <lib_field.h>

#include <erc_settings.h>

class TEST_EE_ITEM_FIXTURE
{
public:
    SCH_SHEET                 m_sheet;
    LIB_SYMBOL                m_symbol;
    LIB_PIN                   m_pin;
    std::shared_ptr<ERC_ITEM> m_ercItem;

    TEST_EE_ITEM_FIXTURE() :
            m_sheet(),
            m_symbol( "test symbol" ),
            m_pin( &m_symbol ),
            m_ercItem( ERC_ITEM::Create( ERCE_DRIVER_CONFLICT ) )
    {
        m_sheet.SetPosition( wxPoint( Millimeter2iu( 5 ), Millimeter2iu( 10 ) ) );
        m_sheet.SetSize( wxSize( Millimeter2iu( 50 ), Millimeter2iu( 100 ) ) );
    }

    EDA_ITEM* Instantiate( KICAD_T aType )
    {
        if( !IsEeschemaType( aType ) )
            return nullptr;

        if( !IsInstantiableType( aType ) )
            return nullptr;

        switch( aType )
        {
        case SCH_MARKER_T:         return new SCH_MARKER( m_ercItem, wxPoint( 0, 0 ) );
        case SCH_JUNCTION_T:       return new SCH_JUNCTION();
        case SCH_NO_CONNECT_T:     return new SCH_NO_CONNECT();
        case SCH_BUS_WIRE_ENTRY_T: return new SCH_BUS_WIRE_ENTRY();
        case SCH_BUS_BUS_ENTRY_T:  return new SCH_BUS_BUS_ENTRY();
        case SCH_LINE_T:           return new SCH_LINE();
        case SCH_BITMAP_T:         return new SCH_BITMAP();
        case SCH_TEXT_T:           return new SCH_TEXT( wxPoint( 0, 0 ), "test text" );
        case SCH_LABEL_T:          return new SCH_LABEL( wxPoint( 0, 0 ), "test label" );
        case SCH_GLOBAL_LABEL_T:   return new SCH_GLOBALLABEL();
        case SCH_HIER_LABEL_T:     return new SCH_HIERLABEL();
        case SCH_FIELD_T:          return new SCH_FIELD( wxPoint( 0, 0 ), 0, nullptr );
        case SCH_SYMBOL_T:         return new SCH_SYMBOL();

        case SCH_SHEET_PIN_T:
            // XXX: m_sheet pins currently have to have their initial positions calculated manually.
            return new SCH_SHEET_PIN( &m_sheet,
                                      wxPoint( m_sheet.GetPosition().x,
                                               m_sheet.GetPosition().y + Millimeter2iu( 40 ) ),
                                      "test aPin" );

        case SCH_SHEET_T:          return new SCH_SHEET();
        case LIB_SHAPE_T:          return new LIB_SHAPE( &m_symbol, SHAPE_T::ARC );
        case LIB_TEXT_T:           return new LIB_TEXT( &m_symbol );
        case LIB_PIN_T:            return new LIB_PIN( &m_symbol );
        case LIB_FIELD_T:          return new LIB_FIELD( &m_symbol );

        case SCHEMATIC_T:
        case SCH_PIN_T:
        case LIB_SYMBOL_T:
        case LIB_ALIAS_T:
            return nullptr;

        default:
            BOOST_FAIL( wxString::Format(
                    "Unhandled type: %d "
                    "(if you created a new type you need to handle it in this switch statement)",
                    aType ) );
            return nullptr;
        }
    }

    static void CompareItems( EDA_ITEM* aItem, EDA_ITEM* aOriginalItem )
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
};


BOOST_FIXTURE_TEST_SUITE( EeItem, TEST_EE_ITEM_FIXTURE )


BOOST_AUTO_TEST_CASE( Move )
{
    for( int i = 0; i < MAX_STRUCT_TYPE_ID; i++ )
    {
        KICAD_T type = static_cast<KICAD_T>( i );

        auto item = std::unique_ptr<EDA_ITEM>( Instantiate( type ) );

        if( item == nullptr )
            continue;

        BOOST_TEST_CONTEXT( "Class: " << item->GetClass() )
        {
            IterateOverPositionsAndReferences<EDA_ITEM>(
                    item.get(),
                    []( EDA_ITEM* aOriginalItem, wxPoint aRef )
                    {
                        auto item = std::unique_ptr<EDA_ITEM>( aOriginalItem->Clone() );
                        wxPoint originalPos = item->GetPosition();

                        SCH_ITEM* schItem = dynamic_cast<SCH_ITEM*>( item.get() );
                        LIB_ITEM* libItem = dynamic_cast<LIB_ITEM*>( item.get() );

                        // Move to a point, then go back.
                        // This has to be an identity transformation.

                        if( schItem != nullptr )
                        {
                            schItem->Move( aRef );
                            BOOST_CHECK_EQUAL( schItem->GetPosition(), originalPos + aRef );

                            schItem->Move( -aRef );
                        }

                        if( libItem != nullptr )
                        {
                            libItem->MoveTo( libItem->GetPosition() + aRef );
                            BOOST_CHECK_EQUAL( libItem->GetPosition(), originalPos + aRef );

                            libItem->MoveTo( libItem->GetPosition() - aRef );
                        }

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

        auto item = std::unique_ptr<EDA_ITEM>( Instantiate( type ) );

        if( item == nullptr )
            continue;

        BOOST_TEST_CONTEXT( "Class: " << item->GetClass() )
        {
            // Four equivalent 90 degree rotations are an identity.

            if( item->GetClass() == "SCH_SHEET_PIN" )
            {
                auto newItem = std::unique_ptr<EDA_ITEM>( item->Clone() );

                SCH_ITEM* schItem = dynamic_cast<SCH_ITEM*>( newItem.get() );
                LIB_ITEM* libItem = dynamic_cast<LIB_ITEM*>( newItem.get() );

                if( schItem != nullptr )
                {
                    // Only rotating pins around the center of parent sheet works.
                    schItem->Rotate( m_sheet.GetBodyBoundingBox().GetCenter() );
                    schItem->Rotate( m_sheet.GetBodyBoundingBox().GetCenter() );
                    schItem->Rotate( m_sheet.GetBodyBoundingBox().GetCenter() );
                    schItem->Rotate( m_sheet.GetBodyBoundingBox().GetCenter() );
                }

                if( libItem != nullptr )
                {
                    libItem->Rotate( m_sheet.GetBodyBoundingBox().GetCenter() );
                    libItem->Rotate( m_sheet.GetBodyBoundingBox().GetCenter() );
                    libItem->Rotate( m_sheet.GetBodyBoundingBox().GetCenter() );
                    libItem->Rotate( m_sheet.GetBodyBoundingBox().GetCenter() );
                }

                CompareItems( newItem.get(), item.get() );
            }
            else
            {
                IterateOverPositionsAndReferences<EDA_ITEM>(
                        item.get(),
                        []( EDA_ITEM* aOriginalItem, wxPoint aRef )
                        {
                            auto item = std::unique_ptr<EDA_ITEM>( aOriginalItem->Clone() );

                            SCH_ITEM* schItem = dynamic_cast<SCH_ITEM*>( item.get() );
                            LIB_ITEM* libItem = dynamic_cast<LIB_ITEM*>( item.get() );

                            if( schItem != nullptr )
                            {
                                schItem->Rotate( aRef );
                                schItem->Rotate( aRef );
                                schItem->Rotate( aRef );
                                schItem->Rotate( aRef );
                            }

                            if( libItem != nullptr )
                            {
                                libItem->Rotate( aRef );
                                libItem->Rotate( aRef );
                                libItem->Rotate( aRef );
                                libItem->Rotate( aRef );
                            }

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

        auto item = std::unique_ptr<EDA_ITEM>( Instantiate( type ) );

        if( item == nullptr )
            continue;

        BOOST_TEST_CONTEXT( "Class: " << item->GetClass() )
        {
            IterateOverPositionsAndReferences<EDA_ITEM>(
                    item.get(),
                    []( EDA_ITEM* aOriginalItem, wxPoint aRef )
                    {
                        auto item = std::unique_ptr<EDA_ITEM>( aOriginalItem->Clone() );

                        SCH_ITEM* schItem = dynamic_cast<SCH_ITEM*>( item.get() );
                        LIB_ITEM* libItem = dynamic_cast<LIB_ITEM*>( item.get() );

                        // Two mirrorings are an identity.

                        if( schItem != nullptr )
                        {
                            schItem->MirrorHorizontally( aRef.x );
                            schItem->MirrorHorizontally( aRef.x );
                        }

                        if( libItem != nullptr )
                        {
                            libItem->MirrorHorizontal( aRef );
                            libItem->MirrorHorizontal( aRef );
                        }

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

        auto item = std::unique_ptr<EDA_ITEM>( Instantiate( type ) );

        if( item == nullptr )
            continue;

        BOOST_TEST_CONTEXT( "Class: " << item->GetClass() )
        {
            IterateOverPositionsAndReferences<EDA_ITEM>(
                    item.get(),
                    []( EDA_ITEM* aOriginalItem, wxPoint aRef )
                    {
                        auto item = std::unique_ptr<EDA_ITEM>( aOriginalItem->Clone() );

                        SCH_ITEM* schItem = dynamic_cast<SCH_ITEM*>( item.get() );
                        LIB_ITEM* libItem = dynamic_cast<LIB_ITEM*>( item.get() );

                        // Two mirrorings are an identity.

                        if( schItem != nullptr )
                        {
                            schItem->MirrorVertically( aRef.y );
                            schItem->MirrorVertically( aRef.y );
                        }

                        if( libItem != nullptr )
                        {
                            libItem->MirrorVertical( aRef );
                            libItem->MirrorVertical( aRef );
                        }

                        CompareItems( item.get(), aOriginalItem );
                    } );
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
