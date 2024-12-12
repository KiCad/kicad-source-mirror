/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include "pcb_table.h"
#include <drc/drc_item.h>
#include <drc/drc_test_provider.h>
#include <pcb_track.h>
#include <footprint.h>
#include <pad.h>
#include <zone.h>
#include <pcb_text.h>


// A list of all basic (ie: non-compound) board geometry items
std::vector<KICAD_T> DRC_TEST_PROVIDER::s_allBasicItems;
std::vector<KICAD_T> DRC_TEST_PROVIDER::s_allBasicItemsButZones;


DRC_TEST_PROVIDER_REGISTRY::~DRC_TEST_PROVIDER_REGISTRY()
{
    for( DRC_TEST_PROVIDER* provider : m_providers )
        delete provider;
}


DRC_SHOWMATCHES_PROVIDER_REGISTRY::~DRC_SHOWMATCHES_PROVIDER_REGISTRY()
{
    for( DRC_TEST_PROVIDER* provider : m_providers )
        delete provider;
}


DRC_TEST_PROVIDER::DRC_TEST_PROVIDER() :
        UNITS_PROVIDER( pcbIUScale, EDA_UNITS::MM ),
        m_drcEngine( nullptr ),
        m_board( nullptr )
{
}


void DRC_TEST_PROVIDER::Init()
{
    if( s_allBasicItems.size() == 0 )
    {
        for( int i = 0; i < MAX_STRUCT_TYPE_ID; i++ )
        {
            if( i != PCB_FOOTPRINT_T && i != PCB_GROUP_T )
            {
                s_allBasicItems.push_back( (KICAD_T) i );

                if( i != PCB_ZONE_T )
                    s_allBasicItemsButZones.push_back( (KICAD_T) i );
            }
        }
    }
}


const wxString DRC_TEST_PROVIDER::GetName() const { return wxT( "<no name test>" ); }


void DRC_TEST_PROVIDER::reportViolation( std::shared_ptr<DRC_ITEM>& item,
                                         const VECTOR2I& aMarkerPos, int aMarkerLayer,
                                         const std::function<void( PCB_MARKER* )>& aPathGenerator )
{
    item->SetViolatingTest( this );
    m_drcEngine->ReportViolation( item, aMarkerPos, aMarkerLayer, aPathGenerator );
}


void DRC_TEST_PROVIDER::reportTwoPointGeometry( std::shared_ptr<DRC_ITEM>& aDrcItem, const VECTOR2I& aMarkerPos,
                                                const VECTOR2I& ptA, const VECTOR2I& ptB, PCB_LAYER_ID aLayer )
{
    PCB_SHAPE ptAShape( nullptr, SHAPE_T::SEGMENT );
    ptAShape.SetStart( ptA );
    ptAShape.SetEnd( ptB );

    reportViolation( aDrcItem, aMarkerPos, aLayer,
                     [&]( PCB_MARKER* aMarker )
                     {
                         aMarker->SetPath( { ptAShape }, ptA, ptB );
                     } );
}


void DRC_TEST_PROVIDER::reportTwoShapeGeometry( std::shared_ptr<DRC_ITEM>& aDrcItem, const VECTOR2I& aMarkerPos,
                                                const SHAPE* aShape1, const SHAPE* aShape2, PCB_LAYER_ID aLayer,
                                                int aDistance )
{
    VECTOR2I ptA, ptB;

    if( aDistance == 0 )
    {
        reportTwoPointGeometry( aDrcItem, aMarkerPos, aMarkerPos, aMarkerPos, aLayer );
    }
    else if( aShape1->NearestPoints( aShape2, ptA, ptB ) )
    {
        reportTwoPointGeometry( aDrcItem, aMarkerPos, ptA, ptB, aLayer );
    }
    else
    {
        reportViolation( aDrcItem, aMarkerPos, aLayer );
    }
}


void DRC_TEST_PROVIDER::reportTwoItemGeometry( std::shared_ptr<DRC_ITEM>& aDrcItem, const VECTOR2I& aMarkerPos,
                                               const BOARD_ITEM* aItem1, const BOARD_ITEM* aItem2,
                                               PCB_LAYER_ID aLayer, int aDistance )
{
    std::shared_ptr<SHAPE> aShape1 = aItem1->GetEffectiveShape( aLayer );
    std::shared_ptr<SHAPE> aShape2 = aItem2->GetEffectiveShape( aLayer );

    reportTwoShapeGeometry( aDrcItem, aMarkerPos, aShape1.get(), aShape2.get(), aLayer, aDistance );
}


bool DRC_TEST_PROVIDER::reportProgress( size_t aCount, size_t aSize, size_t aDelta )
{
    if( ( aCount % aDelta ) == 0 || aCount == aSize -  1 )
    {
        if( !m_drcEngine->ReportProgress( static_cast<double>( aCount ) / aSize ) )
            return false;
    }

    return true;
}


bool DRC_TEST_PROVIDER::reportPhase( const wxString& aMessage )
{
    REPORT_AUX( aMessage );
    return m_drcEngine->ReportPhase( aMessage );
}


int DRC_TEST_PROVIDER::forEachGeometryItem( const std::vector<KICAD_T>& aTypes, const LSET& aLayers,
                                            const std::function<bool( BOARD_ITEM*)>& aFunc )
{
    BOARD *brd = m_drcEngine->GetBoard();
    std::bitset<MAX_STRUCT_TYPE_ID> typeMask;
    int n = 0;

    if( aTypes.size() == 0 )
    {
        for( int i = 0; i < MAX_STRUCT_TYPE_ID; i++ )
            typeMask[ i ] = true;
    }
    else
    {
        for( KICAD_T aType : aTypes )
            typeMask[ aType ] = true;
    }

    for( PCB_TRACK* item : brd->Tracks() )
    {
        if( (item->GetLayerSet() & aLayers).any() )
        {
            if( typeMask[ PCB_TRACE_T ] && item->Type() == PCB_TRACE_T )
            {
                aFunc( item );
                n++;
            }
            else if( typeMask[ PCB_VIA_T ] && item->Type() == PCB_VIA_T )
            {
                aFunc( item );
                n++;
            }
            else if( typeMask[ PCB_ARC_T ] && item->Type() == PCB_ARC_T )
            {
                aFunc( item );
                n++;
            }
        }
    }

    for( BOARD_ITEM* item : brd->Drawings() )
    {
        if( (item->GetLayerSet() & aLayers).any() )
        {
            if( typeMask[ PCB_DIMENSION_T ] && BaseType( item->Type() ) == PCB_DIMENSION_T )
            {
                if( !aFunc( item ) )
                    return n;

                n++;
            }
            else if( typeMask[ PCB_SHAPE_T ] && item->Type() == PCB_SHAPE_T )
            {
                if( !aFunc( item ) )
                    return n;

                n++;
            }
            else if( typeMask[ PCB_TEXT_T ] && item->Type() == PCB_TEXT_T )
            {
                if( !aFunc( item ) )
                    return n;

                n++;
            }
            else if( typeMask[ PCB_TEXTBOX_T ] && item->Type() == PCB_TEXTBOX_T )
            {
                if( !aFunc( item ) )
                    return n;

                n++;
            }
            else if( typeMask[ PCB_TARGET_T ] && item->Type() == PCB_TARGET_T )
            {
                if( !aFunc( item ) )
                    return n;

                n++;
            }
            else if( item->Type() == PCB_TABLE_T )
            {
                if( typeMask[ PCB_TABLE_T ] )
                {
                    if( !aFunc( item ) )
                        return n;

                    n++;
                }

                if( typeMask[ PCB_TABLECELL_T ] )
                {
                    for( PCB_TABLECELL* cell : static_cast<PCB_TABLE*>( item )->GetCells() )
                    {
                        if( !aFunc( cell ) )
                            return n;

                        n++;
                    }
                }
            }
            else if( typeMask[ PCB_BARCODE_T ] && item->Type() == PCB_BARCODE_T )
            {
                if( !aFunc( item ) )
                    return n;

                n++;
            }
        }
    }

    if( typeMask[ PCB_ZONE_T ] )
    {
        for( ZONE* item : brd->Zones() )
        {
            if( ( item->GetLayerSet() & aLayers ).any() )
            {
                if( !aFunc( item ) )
                    return n;

                n++;
            }
        }
    }

    for( FOOTPRINT* footprint : brd->Footprints() )
    {
        if( typeMask[ PCB_FIELD_T ] )
        {
            for( PCB_FIELD* field : footprint->GetFields() )
            {
                if( ( field->GetLayerSet() & aLayers ).any() )
                {
                    if( !aFunc( field ) )
                        return n;

                    n++;
                }
            }
        }

        if( typeMask[ PCB_PAD_T ] )
        {
            for( PAD* pad : footprint->Pads() )
            {
                // Careful: if a pad has a hole then it pierces all layers
                if( pad->HasHole() || ( pad->GetLayerSet() & aLayers ).any() )
                {
                    if( !aFunc( pad ) )
                        return n;

                    n++;
                }
            }
        }

        for( BOARD_ITEM* dwg : footprint->GraphicalItems() )
        {
            if( (dwg->GetLayerSet() & aLayers).any() )
            {
                if( typeMask[ PCB_DIMENSION_T ] && BaseType( dwg->Type() ) == PCB_DIMENSION_T )
                {
                    if( !aFunc( dwg ) )
                        return n;

                    n++;
                }
                else if( typeMask[ PCB_TEXT_T ] && dwg->Type() == PCB_TEXT_T )
                {
                    if( !aFunc( dwg ) )
                        return n;

                    n++;
                }
                else if( typeMask[ PCB_TEXTBOX_T ] && dwg->Type() == PCB_TEXTBOX_T )
                {
                    if( !aFunc( dwg ) )
                        return n;

                    n++;
                }
                else if( typeMask[ PCB_SHAPE_T ] && dwg->Type() == PCB_SHAPE_T )
                {
                    if( !aFunc( dwg ) )
                        return n;

                    n++;
                }
            }
        }

        if( typeMask[ PCB_ZONE_T ] )
        {
            for( ZONE* zone : footprint->Zones() )
            {
                if( (zone->GetLayerSet() & aLayers).any() )
                {
                    if( !aFunc( zone ) )
                        return n;

                    n++;
                }
            }
        }

        if( typeMask[ PCB_FOOTPRINT_T ] )
        {
            if( !aFunc( footprint ) )
                return n;

            n++;
        }
    }

    return n;
}


bool DRC_TEST_PROVIDER::isInvisibleText( const BOARD_ITEM* aItem ) const
{
    if( const PCB_FIELD* field = dynamic_cast<const PCB_FIELD*>( aItem ) )
    {
        if( !field->IsVisible() )
            return true;
    }

    return false;
}


wxString DRC_TEST_PROVIDER::formatMsg( const wxString& aFormatString, const wxString& aSource,
                                       double aConstraint, double aActual, EDA_DATA_TYPE aType )
{
    wxString constraint_str = MessageTextFromValue( aConstraint, true, aType );
    wxString actual_str = MessageTextFromValue( aActual, true, aType );

    if( constraint_str == actual_str )
    {
        // Use more precise formatting if the message-text strings were equal.
        constraint_str = StringFromValue( aConstraint, true, aType );
        actual_str = StringFromValue( aActual, true, aType );
    }

    return wxString::Format( aFormatString, aSource, std::move( constraint_str ), std::move( actual_str ) );
}

wxString DRC_TEST_PROVIDER::formatMsg( const wxString& aFormatString, const wxString& aSource,
                                       const EDA_ANGLE& aConstraint, const EDA_ANGLE& aActual )
{
    wxString constraint_str = MessageTextFromValue( aConstraint );
    wxString actual_str = MessageTextFromValue( aActual );

    if( constraint_str == actual_str )
    {
        // Use more precise formatting if the message-text strings were equal.
        constraint_str = StringFromValue( aConstraint, true );
        actual_str = StringFromValue( aActual, true );
    }

    return wxString::Format( aFormatString, aSource, std::move( constraint_str ), std::move( actual_str ) );
}
