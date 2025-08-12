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

#include <core/spinlock.h>
#include <geometry/shape_poly_set.h>
#include <geometry/shape_line_chain.h>
#include <sch_commit.h>
#include <sch_edit_frame.h>
#include <sch_rule_area.h>
#include <tool/tool_manager.h>
#include <tools/sch_actions.h>
#include <tools/rule_area_create_helper.h>


RULE_AREA_CREATE_HELPER::RULE_AREA_CREATE_HELPER( KIGFX::VIEW& aView, SCH_EDIT_FRAME* aFrame,
                                                  TOOL_MANAGER* aMgr ) :
        m_parentView( aView ),
        m_frame( aFrame ), m_toolManager( aMgr )
{
    m_parentView.Add( &m_previewItem );
}


RULE_AREA_CREATE_HELPER::~RULE_AREA_CREATE_HELPER()
{
    // remove the preview from the view
    m_parentView.SetVisible( &m_previewItem, false );
    m_parentView.Remove( &m_previewItem );
}


std::unique_ptr<SCH_RULE_AREA> RULE_AREA_CREATE_HELPER::createNewRuleArea()
{
    std::unique_ptr<SCH_RULE_AREA> ruleArea = std::make_unique<SCH_RULE_AREA>();
    ruleArea->SetLineStyle( LINE_STYLE::DASH );
    ruleArea->SetLineColor( COLOR4D::UNSPECIFIED );

    return ruleArea;
}


void RULE_AREA_CREATE_HELPER::commitRuleArea( std::unique_ptr<SCH_RULE_AREA> aRuleArea )
{
    SCH_COMMIT commit( m_toolManager );

    SCH_RULE_AREA* ruleArea = aRuleArea.release();

    commit.Add( ruleArea, m_frame->GetScreen() );
    commit.Push( _( "Draw Rule Area" ) );

    m_toolManager->RunAction<EDA_ITEM*>( ACTIONS::selectItem, ruleArea );

    m_parentView.ClearPreview();
}


bool RULE_AREA_CREATE_HELPER::OnFirstPoint( POLYGON_GEOM_MANAGER& aMgr )
{
    m_rule_area = createNewRuleArea();

    if( m_rule_area )
    {
        m_toolManager->RunAction( ACTIONS::selectionClear );

        SCH_RENDER_SETTINGS renderSettings;
        COLOR_SETTINGS*     colorSettings = m_frame->GetColorSettings();
        renderSettings.LoadColors( colorSettings );

        COLOR4D color = renderSettings.GetLayerColor( LAYER_RULE_AREAS );
        m_previewItem.SetLineColor( color );
        m_previewItem.SetLeaderColor( color );
        m_previewItem.SetFillColor( color.WithAlpha( 0.2 ) );

        m_parentView.SetVisible( &m_previewItem, true );

        aMgr.SetLeaderMode( LEADER_MODE::DEG45 );
    }

    return m_rule_area != nullptr;
}


void RULE_AREA_CREATE_HELPER::OnGeometryChange( const POLYGON_GEOM_MANAGER& aMgr )
{
    // Handle a cancel-interactive
    if( m_rule_area && !aMgr.IsPolygonInProgress() )
    {
        m_rule_area = nullptr;
        m_parentView.SetVisible( &m_previewItem, false );
        return;
    }

    // send the points to the preview item
    m_previewItem.SetPoints( aMgr.GetLockedInPoints(), aMgr.GetLeaderLinePoints(),
                             aMgr.GetLoopLinePoints() );
    m_parentView.Update( &m_previewItem, KIGFX::GEOMETRY );
}


void RULE_AREA_CREATE_HELPER::OnComplete( const POLYGON_GEOM_MANAGER& aMgr )
{
    auto& finalPoints = aMgr.GetLockedInPoints();

    if( finalPoints.PointCount() < 3 )
    {
        // Just scrap the rule area in progress
        m_rule_area = nullptr;
    }
    else
    {
        SHAPE_POLY_SET ruleShape;

        ruleShape.NewOutline();
        auto& outline = ruleShape.Outline( 0 );

        for( int i = 0; i < finalPoints.PointCount(); ++i )
            outline.Append( finalPoints.CPoint( i ) );

        // In DEG45 mode, we may have intermediate points in the leader that should be included
        // as they are shown in the preview.  These typically maintain the 45 constraint
        if( aMgr.GetLeaderMode() == LEADER_MODE::DEG45 || aMgr.GetLeaderMode() == LEADER_MODE::DEG90 )
        {
            const SHAPE_LINE_CHAIN leaderPts = aMgr.GetLeaderLinePoints();
            for( int i = 1; i < leaderPts.PointCount(); i++ )
                outline.Append( leaderPts.CPoint( i ) );

            const SHAPE_LINE_CHAIN loopPts = aMgr.GetLoopLinePoints();
            for( int i = 1; i < loopPts.PointCount() - 1; i++ )
                outline.Append( loopPts.CPoint( i ) );
        }

        outline.SetClosed( true );
        outline.Simplify( true );

        // Remove the start point if it lies on the line between neighbouring points.
        // Simplify doesn't handle that currently.
        if( outline.PointCount() >= 3 )
        {
            SEG seg( outline.CLastPoint(), outline.CPoint( 1 ) );

            if( seg.LineDistance( outline.CPoint( 0 ) ) <= 1 )
                outline.Remove( 0 );
        }

        m_rule_area->SetPolyShape( ruleShape );

        // hand the rule area over to the committer
        commitRuleArea( std::move( m_rule_area ) );
        m_rule_area = nullptr;
    }

    m_parentView.SetVisible( &m_previewItem, false );
}


