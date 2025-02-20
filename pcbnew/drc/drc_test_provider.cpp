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


DRC_TEST_PROVIDER::DRC_TEST_PROVIDER() :
        UNITS_PROVIDER( pcbIUScale, EDA_UNITS::MILLIMETRES ), m_drcEngine( nullptr )
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
const wxString DRC_TEST_PROVIDER::GetDescription() const { return wxEmptyString; }


void DRC_TEST_PROVIDER::reportViolation( std::shared_ptr<DRC_ITEM>& item,
                                         const VECTOR2I& aMarkerPos, int aMarkerLayer,
                                         DRC_CUSTOM_MARKER_HANDLER* aCustomHandler )
{
    std::lock_guard<std::mutex> lock( m_statsMutex );
    if( item->GetViolatingRule() )
        accountCheck( item->GetViolatingRule() );

    item->SetViolatingTest( this );
    m_drcEngine->ReportViolation( item, aMarkerPos, aMarkerLayer, aCustomHandler );
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
    reportAux( aMessage );
    return m_drcEngine->ReportPhase( aMessage );
}


void DRC_TEST_PROVIDER::reportAux( const wxChar* fmt, ... )
{
    va_list vargs;
    va_start( vargs, fmt );
    wxString str;
    str.PrintfV( fmt, vargs );
    va_end( vargs );
    m_drcEngine->ReportAux( str );
}


void DRC_TEST_PROVIDER::accountCheck( const DRC_RULE* ruleToTest )
{
    auto it = m_stats.find( ruleToTest );

    if( it == m_stats.end() )
        m_stats[ ruleToTest ] = 1;
    else
        m_stats[ ruleToTest ] += 1;
}


void DRC_TEST_PROVIDER::accountCheck( const DRC_CONSTRAINT& constraintToTest )
{
    accountCheck( constraintToTest.GetParentRule() );
}


void DRC_TEST_PROVIDER::reportRuleStatistics()
{
    if( !m_isRuleDriven )
        return;

    m_drcEngine->ReportAux( wxT( "Rule hit statistics: " ) );

    for( const std::pair<const DRC_RULE* const, int>& stat : m_stats )
    {
        if( stat.first )
        {
            m_drcEngine->ReportAux( wxString::Format( wxT( " - rule '%s': %d hits " ),
                                                      stat.first->m_Name,
                                                      stat.second ) );
        }
    }
}


int DRC_TEST_PROVIDER::forEachGeometryItem( const std::vector<KICAD_T>& aTypes, LSET aLayers,
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
                                       double aConstraint, double aActual )
{
    wxString constraint_str = MessageTextFromValue( aConstraint );
    wxString actual_str = MessageTextFromValue( aActual );

    if( constraint_str == actual_str )
    {
        // Use more precise formatting if the message-text strings were equal.
        constraint_str = StringFromValue( aConstraint, true );
        actual_str = StringFromValue( aActual, true );
    }

    return wxString::Format( aFormatString, aSource, constraint_str, actual_str );
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

    return wxString::Format( aFormatString, aSource, constraint_str, actual_str );
}
