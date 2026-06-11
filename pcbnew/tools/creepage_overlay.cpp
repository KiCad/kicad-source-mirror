/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers.
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

#include <tools/creepage_overlay.h>

#include <algorithm>
#include <chrono>

#include <advanced_config.h>
#include <base_units.h>
#include <eda_units.h>
#include <board.h>
#include <board_connected_item.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <netinfo.h>
#include <pad.h>
#include <pcb_track.h>
#include <pcb_shape.h>

#include <drc/drc_engine.h>
#include <drc/drc_rule.h>

#include <view/view.h>
#include <view/view_overlay.h>


CREEPAGE_OVERLAY::CREEPAGE_OVERLAY( BOARD* aBoard, std::shared_ptr<DRC_ENGINE> aDrcEngine,
                                    KIGFX::VIEW* aView ) :
        m_board( aBoard ),
        m_drcEngine( std::move( aDrcEngine ) ),
        m_view( aView ),
        m_minGrooveWidth( 0 ),
        m_enabled( false ),
        m_active( false ),
        m_skipFrames( 0 )
{
    m_enabled = ADVANCED_CFG::GetCfg().m_RealtimeCreepage && m_board && m_drcEngine && m_view
                && m_drcEngine->HasRulesForConstraintType( CREEPAGE_CONSTRAINT );

    if( ADVANCED_CFG::GetCfg().m_EnableCreepageSlot && m_board )
        m_minGrooveWidth = m_board->GetDesignSettings().m_MinGrooveWidth;
}


CREEPAGE_OVERLAY::~CREEPAGE_OVERLAY()
{
    Stop();
}


void CREEPAGE_OVERLAY::clearOverlay()
{
    if( m_overlay )
    {
        m_overlay->Clear();
        m_view->Update( m_overlay.get() );
    }
}


void CREEPAGE_OVERLAY::Start( const std::vector<BOARD_ITEM*>& aMovingItems )
{
    if( !m_enabled || aMovingItems.empty() )
        return;

    std::map<PCB_LAYER_ID, std::set<int>> netsByLayer;
    std::set<const BOARD_ITEM*>           movingItems;
    LSET                                  copper = LSET::AllCuMask( m_board->GetCopperLayerCount() );

    auto addConnected = [&]( BOARD_CONNECTED_ITEM* aItem )
    {
        if( !aItem || aItem->GetNetCode() <= 0 )
            return;

        for( PCB_LAYER_ID layer : copper.Seq() )
        {
            if( aItem->IsOnLayer( layer ) )
                netsByLayer[layer].insert( aItem->GetNetCode() );
        }
    };

    for( BOARD_ITEM* item : aMovingItems )
    {
        if( !item )
            continue;

        movingItems.insert( item );

        if( item->IsConnected() )
            addConnected( static_cast<BOARD_CONNECTED_ITEM*>( item ) );

        if( item->Type() == PCB_FOOTPRINT_T )
        {
            for( PAD* pad : static_cast<FOOTPRINT*>( item )->Pads() )
                addConnected( pad );
        }
    }

    if( netsByLayer.empty() )
        return;

    BOARD*                      board = m_board;
    std::shared_ptr<DRC_ENGINE> drcEngine = m_drcEngine;

    for( const auto& [layer, affectedNets] : netsByLayer )
    {
        // Capturing by value keeps the lambda valid for the whole drag
        auto constraintFn = [board, drcEngine, layer]( int aNetA, int aNetB ) -> double
        {
            PCB_TRACK a( board );
            PCB_TRACK b( board );
            a.SetNetCode( aNetA );
            b.SetNetCode( aNetB );
            a.SetLayer( layer );
            b.SetLayer( layer );

            DRC_CONSTRAINT c = drcEngine->EvalRules( CREEPAGE_CONSTRAINT, &a, &b, layer );
            return c.Value().Min();
        };

        // The largest constraint for any affected pair sets the search radius and near band
        double maxConstraint = 0.0;

        for( int affected : affectedNets )
        {
            for( const auto& [netCode, net] : m_board->GetNetInfo().NetsByNetcode() )
            {
                if( netCode <= 0 || netCode == affected )
                    continue;

                maxConstraint = std::max( maxConstraint, constraintFn( affected, netCode ) );
            }
        }

        if( maxConstraint <= 0 )
            continue;

        int nearMargin = static_cast<int>( maxConstraint / 2 );
        int margin = static_cast<int>( maxConstraint ) + nearMargin;

        auto engine = std::make_unique<CREEPAGE_ENGINE>( *m_board );
        engine->SetMinGrooveWidth( m_minGrooveWidth );
        engine->BeginInteractive( layer, affectedNets, movingItems, margin, constraintFn );
        m_engines[layer] = { std::move( engine ), nearMargin };
    }

    if( m_engines.empty() )
        return;

    if( !m_overlay )
    {
        m_overlay = m_view->MakeOverlay();
        m_view->Add( m_overlay.get() );
    }

    m_active = true;
}


void CREEPAGE_OVERLAY::Update()
{
    if( !m_active || !m_overlay )
        return;

    if( m_skipFrames > 0 )
    {
        --m_skipFrames;
        return;
    }

    auto                         t0 = std::chrono::steady_clock::now();
    std::vector<CREEPAGE_RESULT> results;

    for( auto& [layer, le] : m_engines )
    {
        std::vector<CREEPAGE_RESULT> layerResults = le.m_engine->Update( le.m_nearMargin );
        results.insert( results.end(), layerResults.begin(), layerResults.end() );
    }

    double elapsedMs = std::chrono::duration<double, std::milli>(
                               std::chrono::steady_clock::now() - t0 )
                               .count();

    // Budget one 60 Hz frame; a heavier Update amortizes by skipping proportional motion frames
    constexpr double frameBudgetMs = 16.0;
    m_skipFrames = static_cast<int>( elapsedMs / frameBudgetMs );

    m_overlay->Clear();

    const COLOR4D violationColor( 0.92, 0.18, 0.18, 1.0 );
    const COLOR4D nearColor( 0.95, 0.62, 0.10, 0.85 );
    const double  lineWidth = pcbIUScale.mmToIU( 0.12 );

    for( const CREEPAGE_RESULT& r : results )
    {
        m_overlay->SetStrokeColor( r.m_violation ? violationColor : nearColor );
        m_overlay->SetLineWidth( lineWidth );

        for( const PCB_SHAPE& shape : r.m_path )
        {
            if( shape.GetShape() == SHAPE_T::ARC )
            {
                EDA_ANGLE start( shape.GetStart() - shape.GetCenter() );
                m_overlay->Arc( shape.GetCenter(), shape.GetRadius(), start,
                                start + shape.GetArcAngle() );
            }
            else
            {
                m_overlay->Segment( shape.GetStart(), shape.GetEnd(), lineWidth );
            }
        }

        m_overlay->SetGlyphSize( VECTOR2I( pcbIUScale.mmToIU( 0.6 ), pcbIUScale.mmToIU( 0.6 ) ) );
        m_overlay->BitmapText( EDA_UNIT_UTILS::UI::MessageTextFromValue( pcbIUScale, EDA_UNITS::MM,
                                                                         r.m_distance ),
                               r.m_start, ANGLE_0 );
    }

    m_view->Update( m_overlay.get() );
}


void CREEPAGE_OVERLAY::Stop()
{
    for( auto& [layer, le] : m_engines )
    {
        if( le.m_engine )
            le.m_engine->EndInteractive();
    }

    m_engines.clear();
    m_active = false;

    if( m_overlay )
    {
        clearOverlay();
        m_view->Remove( m_overlay.get() );
        m_overlay.reset();
    }
}
