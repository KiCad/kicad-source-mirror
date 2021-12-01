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
#include <board.h>
#include <board_item.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_shape.h>
#include <pcb_text.h>
#include <fp_text.h>
#include <fp_shape.h>
#include <zone.h>
#include <pcb_track.h>
#include <pcb_marker.h>
#include <pcb_dimension.h>
#include <pcb_target.h>
#include <netinfo.h>
#include <pcb_group.h>

class TEST_BOARD_ITEM_FIXTURE
{
public:
    BOARD                     m_board;
    FOOTPRINT                 m_footprint;
    std::shared_ptr<DRC_ITEM> m_drcItem;
    PCB_TEXT                  m_text;

    TEST_BOARD_ITEM_FIXTURE() :
            m_board(),
            m_footprint( &m_board ),
            m_drcItem( DRC_ITEM::Create( DRCE_MALFORMED_COURTYARD ) ),
            m_text( &m_board )
    {
    }

    BOARD_ITEM* Instantiate( KICAD_T aType )
    {
        if( !IsPcbnewType( aType ) )
            return nullptr;

        if( !IsInstantiableType( aType ) )
            return nullptr;

        switch( aType )
        {
        case PCB_FOOTPRINT_T:      return new FOOTPRINT( &m_board );
        case PCB_PAD_T:            return new PAD( &m_footprint );
        case PCB_SHAPE_T:          return new PCB_SHAPE( &m_board );
        case PCB_TEXT_T:           return new PCB_TEXT( &m_board );
        case PCB_FP_TEXT_T:        return new FP_TEXT( &m_footprint );
        case PCB_FP_SHAPE_T:       return new FP_SHAPE( &m_footprint );
        case PCB_FP_ZONE_T:
        {
            FP_ZONE* fpZone = new FP_ZONE( &m_footprint );

            fpZone->AppendCorner( wxPoint( Millimeter2iu( -100 ), Millimeter2iu( -50 ) ), -1 );
            fpZone->AppendCorner( wxPoint( Millimeter2iu( -100 ), Millimeter2iu( 50 ) ), -1 );
            fpZone->AppendCorner( wxPoint( Millimeter2iu( 100 ), Millimeter2iu( 50 ) ), -1 );
            fpZone->AppendCorner( wxPoint( Millimeter2iu( 100 ), Millimeter2iu( -50 ) ), -1 );

            return fpZone;
        }

        case PCB_TRACE_T:          return new PCB_TRACK( &m_board );
        case PCB_VIA_T:            return new PCB_VIA( &m_board );
        case PCB_ARC_T:            return new PCB_ARC( &m_board );
        case PCB_MARKER_T:         return new PCB_MARKER( m_drcItem, wxPoint( 0, 0 ) );
        case PCB_DIM_ALIGNED_T:    return new PCB_DIM_ALIGNED( &m_board );
        case PCB_DIM_LEADER_T:     return new PCB_DIM_LEADER( &m_board );
        case PCB_DIM_CENTER_T:     return new PCB_DIM_CENTER( &m_board );
        case PCB_DIM_ORTHOGONAL_T: return new PCB_DIM_ORTHOGONAL( &m_board );
        case PCB_TARGET_T:         return new PCB_TARGET( &m_board );
        case PCB_ZONE_T:
        {
            ZONE* zone = new ZONE( &m_board );

            zone->AppendCorner( wxPoint( Millimeter2iu( -100 ), Millimeter2iu( -50 ) ), -1 );
            zone->AppendCorner( wxPoint( Millimeter2iu( -100 ), Millimeter2iu( 50 ) ), -1 );
            zone->AppendCorner( wxPoint( Millimeter2iu( 100 ), Millimeter2iu( 50 ) ), -1 );
            zone->AppendCorner( wxPoint( Millimeter2iu( 100 ), Millimeter2iu( -50 ) ), -1 );

            return zone;
        }
        case PCB_GROUP_T:
        {
            PCB_GROUP* group = new PCB_GROUP( &m_board );

            // Group position only makes sense if there's at least one item in the group.
            group->AddItem( &m_text );

            return group;
        }

        case PCB_T:
        case PCB_ITEM_LIST_T:
        case PCB_NETINFO_T:
            return nullptr;

        default:
            BOOST_FAIL( wxString::Format(
                    "Unhandled type: %d "
                    "(if you created a new type you need to handle it in this switch statement)",
                    aType ) );
            return nullptr;
        }
    }

    static void CompareItems( BOARD_ITEM* aItem, BOARD_ITEM* aOriginalItem )
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


BOOST_FIXTURE_TEST_SUITE( PcbItem, TEST_BOARD_ITEM_FIXTURE )


BOOST_AUTO_TEST_CASE( Move )
{
    for( int i = 0; i < MAX_STRUCT_TYPE_ID; i++ )
    {
        KICAD_T type = static_cast<KICAD_T>( i );

        auto item = std::unique_ptr<BOARD_ITEM>( Instantiate( type ) );

        if( item == nullptr )
            continue;

        BOOST_TEST_CONTEXT( "Class: " << item->GetClass() )
        {
            IterateOverPositionsAndReferences<BOARD_ITEM>(
                    item.get(),
                    []( BOARD_ITEM* aOriginalItem, wxPoint aRef )
                    {
                        FP_SHAPE* originalFpShape = dynamic_cast<FP_SHAPE*>( aOriginalItem );

                        if( originalFpShape != nullptr )
                            originalFpShape->SetDrawCoord();

                        // FIXME: Update() has to be called after SetPosition() to update dimension
                        // shapes.
                        PCB_DIMENSION_BASE* originalDimension =
                                dynamic_cast<PCB_DIMENSION_BASE*>( aOriginalItem );

                        if( originalDimension != nullptr )
                            originalDimension->Update();

                        auto    item = std::unique_ptr<BOARD_ITEM>( aOriginalItem->Duplicate() );
                        wxPoint originalPos = item->GetPosition();

                        // Move to a point, then go back.
                        // This has to be an identity transformation.

                        item->Move( aRef );

                        FP_SHAPE* fpShape = dynamic_cast<FP_SHAPE*>( item.get() );
                        if( fpShape != nullptr )
                            fpShape->SetDrawCoord();

                        BOOST_CHECK_EQUAL( item->GetPosition(), originalPos + aRef );

                        item->Move( -aRef );

                        if( fpShape != nullptr )
                            fpShape->SetDrawCoord();

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

        auto item = std::unique_ptr<BOARD_ITEM>( Instantiate( type ) );

        if( item == nullptr )
            continue;

        BOOST_TEST_CONTEXT( "Class: " << item->GetClass() )
        {
            // Four same 90 degree rotations are an identity.

            IterateOverPositionsAndReferences<BOARD_ITEM>(
                    item.get(),
                    []( BOARD_ITEM* aOriginalItem, wxPoint aRef )
                    {
                        FP_SHAPE* originalFpShape = dynamic_cast<FP_SHAPE*>( aOriginalItem );

                        if( originalFpShape != nullptr )
                            originalFpShape->SetDrawCoord();

                        // FIXME: Update() has to be called after SetPosition() to update dimension
                        // shapes.
                        PCB_DIMENSION_BASE* originalDimension =
                                dynamic_cast<PCB_DIMENSION_BASE*>( aOriginalItem );

                        if( originalDimension != nullptr )
                            originalDimension->Update();

                        auto item = std::unique_ptr<BOARD_ITEM>( aOriginalItem->Duplicate() );

                        // Four equivalent 90 degree rotations are an identity.

                        item->Rotate( aRef, 900 );
                        item->Rotate( aRef, 900 );
                        item->Rotate( aRef, 900 );
                        item->Rotate( aRef, 900 );

                        FP_SHAPE* fpShape = dynamic_cast<FP_SHAPE*>( aOriginalItem );

                        if( fpShape != nullptr )
                            fpShape->SetDrawCoord();

                        CompareItems( item.get(), aOriginalItem );
                    } );
        }
    }
}


BOOST_AUTO_TEST_CASE( FlipLeftRight )
{
    for( int i = 0; i < MAX_STRUCT_TYPE_ID; i++ )
    {
        KICAD_T type = static_cast<KICAD_T>( i );

        auto item = std::unique_ptr<BOARD_ITEM>( Instantiate( type ) );

        if( item == nullptr )
            continue;

        BOOST_TEST_CONTEXT( "Class: " << item->GetClass() )
        {
            IterateOverPositionsAndReferences<BOARD_ITEM>(
                    item.get(),
                    []( BOARD_ITEM* aOriginalItem, wxPoint aRef )
                    {
                        FP_SHAPE* originalFpShape = dynamic_cast<FP_SHAPE*>( aOriginalItem );

                        if( originalFpShape != nullptr )
                            originalFpShape->SetDrawCoord();

                        // FIXME: Update() has to be called after SetPosition() to update dimension
                        // shapes.
                        PCB_DIMENSION_BASE* originalDimension =
                                dynamic_cast<PCB_DIMENSION_BASE*>( aOriginalItem );

                        if( originalDimension != nullptr )
                            originalDimension->Update();

                        auto item = std::unique_ptr<BOARD_ITEM>( aOriginalItem->Duplicate() );

                        // Two equivalent flips are an identity.

                        item->Flip( aRef, true );
                        item->Flip( aRef, true );

                        FP_SHAPE* fpShape = dynamic_cast<FP_SHAPE*>( aOriginalItem );

                        if( fpShape != nullptr )
                            fpShape->SetDrawCoord();

                        CompareItems( item.get(), aOriginalItem );
                    } );
        }
    }
}


BOOST_AUTO_TEST_CASE( FlipUpDown )
{
    for( int i = 0; i < MAX_STRUCT_TYPE_ID; i++ )
    {
        KICAD_T type = static_cast<KICAD_T>( i );

        auto item = std::unique_ptr<BOARD_ITEM>( Instantiate( type ) );

        if( item == nullptr )
            continue;

        BOOST_TEST_CONTEXT( "Class: " << item->GetClass() )
        {
            IterateOverPositionsAndReferences<BOARD_ITEM>(
                    item.get(),
                    []( BOARD_ITEM* aOriginalItem, wxPoint aRef )
                    {
                        FP_SHAPE* originalFpShape = dynamic_cast<FP_SHAPE*>( aOriginalItem );

                        if( originalFpShape != nullptr )
                            originalFpShape->SetDrawCoord();

                        // FIXME: Update() has to be called after SetPosition() to update dimension
                        // shapes.
                        PCB_DIMENSION_BASE* originalDimension =
                                dynamic_cast<PCB_DIMENSION_BASE*>( aOriginalItem );

                        if( originalDimension != nullptr )
                            originalDimension->Update();

                        auto item = std::unique_ptr<BOARD_ITEM>( aOriginalItem->Duplicate() );

                        // Two equivalent flips are an identity.

                        item->Flip( aRef, false );
                        item->Flip( aRef, false );

                        FP_SHAPE* fpShape = dynamic_cast<FP_SHAPE*>( aOriginalItem );

                        if( fpShape != nullptr )
                            fpShape->SetDrawCoord();

                        CompareItems( item.get(), aOriginalItem );
                    } );
        }
    }
}


BOOST_AUTO_TEST_SUITE_END()
