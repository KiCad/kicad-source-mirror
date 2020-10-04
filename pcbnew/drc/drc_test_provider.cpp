/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see change_log.txt for contributors.
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

#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_test_provider.h>
#include <class_track.h>
#include <class_module.h>
#include <class_pad.h>
#include <class_zone.h>
#include <class_pcb_text.h>

DRC_TEST_PROVIDER::DRC_TEST_PROVIDER() :
    m_drcEngine( nullptr )
{
}


const wxString DRC_TEST_PROVIDER::GetName() const { return "<no name test>"; }
const wxString DRC_TEST_PROVIDER::GetDescription() const { return ""; }


void DRC_TEST_PROVIDER::reportViolation( std::shared_ptr<DRC_ITEM>& item, wxPoint aMarkerPos )
{
    item->SetViolatingTest( this );
    m_drcEngine->ReportViolation( item, aMarkerPos );
}


bool DRC_TEST_PROVIDER::reportProgress( int aCount, int aSize, int aDelta )
{
    if( ( aCount % aDelta ) == 0 || aCount == aSize -  1 )
    {
        if( !m_drcEngine->ReportProgress( (double) aCount / (double) aSize ) )
            return false;
    }

    return true;
}


bool DRC_TEST_PROVIDER::reportPhase( const wxString& aMessage )
{
    reportAux( aMessage );
    return m_drcEngine->ReportPhase( aMessage );
}


void DRC_TEST_PROVIDER::reportAux( wxString fmt, ... )
{
    va_list vargs;
    va_start( vargs, fmt );
    wxString str;
    str.PrintfV( fmt, vargs );
    va_end( vargs );
    m_drcEngine->ReportAux( str );
}


EDA_UNITS DRC_TEST_PROVIDER::userUnits() const
{
    return m_drcEngine->UserUnits();
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

    m_drcEngine->ReportAux( "Rule hit statistics: " );

    for( const std::pair<const DRC_RULE* const, int>& stat : m_stats )
    {
        if( stat.first )
        {
            m_drcEngine->ReportAux( wxString::Format( " - rule '%s': %d hits ",
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

    for( TRACK* item : brd->Tracks() )
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
            if( typeMask[PCB_DIMENSION_T] && BaseType( item->Type() ) == PCB_DIMENSION_T )
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
            else if( typeMask[ PCB_TARGET_T ] && item->Type() == PCB_TARGET_T )
            {
                if( !aFunc( item ) )
                    return n;

                n++;
            }
        }
    }

    for( ZONE_CONTAINER* item : brd->Zones() )
    {
        if( (item->GetLayerSet() & aLayers).any() )
        {
            if( typeMask[ PCB_ZONE_AREA_T ] && item->Type() == PCB_ZONE_AREA_T )
            {
                if( !aFunc( item ) )
                    return n;

                n++;
            }
        }
    }

    for( MODULE* mod : brd->Modules() )
    {
        if( typeMask[ PCB_FP_TEXT_T ] )
        {
            if( (mod->Reference().GetLayerSet() & aLayers).any() )
            {
                if( !aFunc( &mod->Reference() ) )
                    return n;

                n++;
            }

            if( (mod->Value().GetLayerSet() & aLayers).any() )
            {
                if( !aFunc( &mod->Value() ) )
                    return n;

                n++;
            }
        }

        for( D_PAD* pad : mod->Pads() )
        {
            if( (pad->GetLayerSet() & aLayers).any() )
            {
                if( typeMask[ PCB_PAD_T ] && pad->Type() == PCB_PAD_T )
                {
                    if( !aFunc( pad ) )
                        return n;

                    n++;
                }
            }
        }

        for( BOARD_ITEM* dwg : mod->GraphicalItems() )
        {
            if( (dwg->GetLayerSet() & aLayers).any() )
            {
                if( typeMask[ PCB_FP_TEXT_T ] && dwg->Type() == PCB_FP_TEXT_T )
                {
                    if( !aFunc( dwg ) )
                        return n;

                    n++;
                }
                else if( typeMask[ PCB_FP_SHAPE_T ] && dwg->Type() == PCB_FP_SHAPE_T )
                {
                    if( !aFunc( dwg ) )
                        return n;

                    n++;
                }
            }
        }

        for( ZONE_CONTAINER* zone : mod->Zones() )
        {
            if( (zone->GetLayerSet() & aLayers).any() )
            {
                if( typeMask[ PCB_FP_ZONE_AREA_T ] && zone->Type() == PCB_FP_ZONE_AREA_T )
                {
                    if( ! aFunc( zone ) )
                        return n;

                    n++;
                }
            }
        }
    }

    return n;
}


bool DRC_TEST_PROVIDER::isInvisibleText( const BOARD_ITEM* aItem ) const
{

    if( auto text = dyn_cast<const TEXTE_MODULE*>( aItem ) )
    {
        if( !text->IsVisible() )
            return true;
    }

    if( auto text = dyn_cast<const TEXTE_PCB*>( aItem ) )
    {
        if( !text->IsVisible() )
            return true;
    }

    return false;
}