/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <sch_marker.h>
#include <sch_junction.h>
#include <sch_no_connect.h>
#include <sch_bus_entry.h>
#include <sch_line.h>
#include <sch_rule_area.h>
#include <sch_shape.h>
#include <sch_bitmap.h>
#include <sch_text.h>
#include <sch_textbox.h>
#include <sch_table.h>
#include <sch_tablecell.h>
#include <sch_field.h>
#include <sch_group.h>
#include <sch_symbol.h>
#include <sch_sheet_pin.h>
#include <sch_sheet.h>
#include <sch_pin.h>

#include <erc/erc_settings.h>

class TEST_EE_ITEM_FIXTURE
{
public:
    SCH_SHEET                 m_sheet;
    LIB_SYMBOL                m_symbol;
    SCH_PIN                   m_pin;
    SCH_TEXT                  m_text;
    std::shared_ptr<ERC_ITEM> m_ercItem;

    TEST_EE_ITEM_FIXTURE() :
            m_sheet(),
            m_symbol( "test symbol" ),
            m_pin( &m_symbol ),
            m_ercItem( ERC_ITEM::Create( ERCE_DRIVER_CONFLICT ) )
    {
        m_sheet.SetPosition( VECTOR2I( schIUScale.mmToIU( 5 ), schIUScale.mmToIU( 10 ) ) );
        m_sheet.SetSize( VECTOR2I( schIUScale.mmToIU( 50 ), schIUScale.mmToIU( 100 ) ) );
    }

    ~TEST_EE_ITEM_FIXTURE()
    {
        m_text.SetParentGroup( nullptr );
    }

    EDA_ITEM* Instantiate( KICAD_T aType )
    {
        if( !IsEeschemaType( aType ) )
            return nullptr;

        if( !IsInstantiableType( aType ) )
            return nullptr;

        switch( aType )
        {
        case SCH_MARKER_T:          return new SCH_MARKER( m_ercItem, VECTOR2I( 0, 0 ) );
        case SCH_JUNCTION_T:        return new SCH_JUNCTION();
        case SCH_NO_CONNECT_T:      return new SCH_NO_CONNECT();
        case SCH_BUS_WIRE_ENTRY_T:  return new SCH_BUS_WIRE_ENTRY();
        case SCH_BUS_BUS_ENTRY_T:   return new SCH_BUS_BUS_ENTRY();
        case SCH_LINE_T:            return new SCH_LINE();

        case SCH_RULE_AREA_T:
        {
            SHAPE_POLY_SET ruleShape;

            ruleShape.NewOutline();
            auto& outline = ruleShape.Outline( 0 );
            outline.Append( VECTOR2I( 20000, 20000) );
            outline.Append( VECTOR2I( 22000, 20000) );
            outline.Append( VECTOR2I( 22000, 22000) );
            outline.Append( VECTOR2I( 20000, 22000) );
            outline.SetClosed( true );
            outline.Simplify( true );

            SCH_RULE_AREA* ruleArea = new SCH_RULE_AREA();
            ruleArea->SetPolyShape( ruleShape );

            return ruleArea;
        }

        case SCH_SHAPE_T:           return new SCH_SHAPE( SHAPE_T::ARC, LAYER_NOTES );
        case SCH_BITMAP_T:          return new SCH_BITMAP();
        case SCH_TEXT_T:            return new SCH_TEXT( VECTOR2I( 0, 0 ), "test text" );
        case SCH_TEXTBOX_T:         return new SCH_TEXTBOX( LAYER_NOTES, 0, FILL_T::NO_FILL, "test textbox" );
        case SCH_TABLECELL_T:       return new SCH_TABLECELL();

        case SCH_TABLE_T:
        {
            SCH_TABLE* table = new SCH_TABLE( schIUScale.mmToIU( 0.1 ) );

            table->SetColCount( 2 );

            for( int ii = 0; ii < 4; ++ii )
                table->InsertCell( ii, new SCH_TABLECELL() );

            return table;
        }

        case SCH_LABEL_T:           return new SCH_LABEL( VECTOR2I( 0, 0 ), "test label" );
        case SCH_DIRECTIVE_LABEL_T: return new SCH_DIRECTIVE_LABEL( VECTOR2I( 0, 0 ) );
        case SCH_GLOBAL_LABEL_T:    return new SCH_GLOBALLABEL();
        case SCH_HIER_LABEL_T:      return new SCH_HIERLABEL();
        case SCH_FIELD_T:           return new SCH_FIELD( nullptr, FIELD_T::USER );
        case SCH_SYMBOL_T:          return new SCH_SYMBOL();

        case SCH_SHEET_PIN_T:
            // XXX: m_sheet pins currently have to have their initial positions calculated manually.
            return new SCH_SHEET_PIN( &m_sheet,
                                      VECTOR2I( m_sheet.GetPosition().x,
                                                m_sheet.GetPosition().y + schIUScale.mmToIU( 40 ) ),
                                      "test aPin" );

        case SCH_SHEET_T:           return new SCH_SHEET();
        case SCH_PIN_T:             return new SCH_PIN( &m_symbol );

        case SCH_GROUP_T:
        {
            SCH_GROUP* group = new SCH_GROUP();

            // Group position only makes sense if there's at least one item in the group.
            group->AddItem( &m_text );

            return group;
        }

        case SCHEMATIC_T:
        case LIB_SYMBOL_T:          return nullptr;

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
                    []( EDA_ITEM* aOriginalItem, VECTOR2I aRef )
                    {
                        auto item = std::unique_ptr<EDA_ITEM>( aOriginalItem->Clone() );
                        VECTOR2I originalPos = item->GetPosition();

                        SCH_ITEM* schItem = dynamic_cast<SCH_ITEM*>( item.get() );

                        // Move to a point, then go back.
                        // This has to be an identity transformation.

                        if( schItem != nullptr )
                        {
                            schItem->Move( aRef );
                            BOOST_CHECK_EQUAL( schItem->GetPosition(), originalPos + aRef );

                            schItem->Move( -aRef );
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
            // (warning: only for items having no autoplaced fields).

            if( item->GetClass() == "SCH_SHEET_PIN" )
            {
                auto newItem = std::unique_ptr<EDA_ITEM>( item->Clone() );

                SCH_ITEM* schItem = dynamic_cast<SCH_ITEM*>( newItem.get() );

                if( schItem != nullptr )
                {
                    schItem->SetFieldsAutoplaced( AUTOPLACE_NONE );
                    // Only rotating pins around the center of parent sheet works.
                    schItem->Rotate( m_sheet.GetBodyBoundingBox().GetCenter(), false );
                    schItem->Rotate( m_sheet.GetBodyBoundingBox().GetCenter(), false );
                    schItem->Rotate( m_sheet.GetBodyBoundingBox().GetCenter(), false );
                    schItem->Rotate( m_sheet.GetBodyBoundingBox().GetCenter(), false );
                }

                CompareItems( newItem.get(), item.get() );
            }
            else
            {
                IterateOverPositionsAndReferences<EDA_ITEM>(
                        item.get(),
                        []( EDA_ITEM* aOriginalItem, VECTOR2I aRef )
                        {
                            auto item = std::unique_ptr<EDA_ITEM>( aOriginalItem->Clone() );

                            SCH_ITEM* schItem = dynamic_cast<SCH_ITEM*>( item.get() );

                            if( schItem != nullptr )
                            {
                                schItem->SetFieldsAutoplaced( AUTOPLACE_NONE );
                                schItem->Rotate( aRef, false );
                                schItem->Rotate( aRef, false );
                                schItem->Rotate( aRef, false );
                                schItem->Rotate( aRef, false );
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
                    []( EDA_ITEM* aOriginalItem, VECTOR2I aRef )
                    {
                        auto item = std::unique_ptr<EDA_ITEM>( aOriginalItem->Clone() );

                        SCH_ITEM* schItem = dynamic_cast<SCH_ITEM*>( item.get() );

                        // Two mirrorings are an identity
                        // (warning: only for text items having no autoplaced fields).
                        if( schItem != nullptr )
                        {
                            schItem->SetFieldsAutoplaced( AUTOPLACE_NONE );
                            schItem->MirrorHorizontally( aRef.x );
                            schItem->MirrorHorizontally( aRef.x );
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
                    []( EDA_ITEM* aOriginalItem, VECTOR2I aRef )
                    {
                        auto item = std::unique_ptr<EDA_ITEM>( aOriginalItem->Clone() );

                        SCH_ITEM* schItem = dynamic_cast<SCH_ITEM*>( item.get() );

                        // Two mirrorings are an identity
                        // (warning only for text items having no autoplaced fields).

                        if( schItem != nullptr )
                        {
                            schItem->SetFieldsAutoplaced( AUTOPLACE_NONE );
                            schItem->MirrorVertically( aRef.y );
                            schItem->MirrorVertically( aRef.y );
                        }

                        CompareItems( item.get(), aOriginalItem );
                    } );
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
