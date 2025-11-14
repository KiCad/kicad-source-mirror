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

#include "tool/grid_helper.h"

#include <functional>
#include <cmath>
#include <limits>

#include <advanced_config.h>
#include <trace_helpers.h>
#include <wx/log.h>
#include <gal/graphics_abstraction_layer.h>
#include <math/util.h>      // for KiROUND
#include <math/vector2d.h>
#include <tool/tool_manager.h>
#include <tool/tools_holder.h>
#include <view/view.h>
#include <settings/app_settings.h>


GRID_HELPER::GRID_HELPER() :
        m_toolMgr( nullptr ), m_snapManager( m_constructionGeomPreview )
{
    m_maskTypes = ALL;
    m_enableSnap = true;
    m_enableSnapLine = true;
    m_enableGrid = true;
    m_snapItem = std::nullopt;

    m_manualGrid = VECTOR2D( 1, 1 );
    m_manualVisibleGrid = VECTOR2D( 1, 1 );
    m_manualOrigin = VECTOR2I( 0, 0 );
    m_manualGridSnapping = true;
}


GRID_HELPER::GRID_HELPER( TOOL_MANAGER* aToolMgr, int aConstructionLayer ) :
        GRID_HELPER()
{
    m_toolMgr = aToolMgr;

    if( !m_toolMgr )
        return;

    KIGFX::VIEW* view = m_toolMgr->GetView();
    wxUnusedVar( aConstructionLayer );

    view->Add( &m_constructionGeomPreview );
    view->SetVisible( &m_constructionGeomPreview, false );

    m_snapManager.SetUpdateCallback(
            [view, this]( bool aAnythingShown )
            {
                const bool currentlyVisible = view->IsVisible( &m_constructionGeomPreview );

                if( currentlyVisible && aAnythingShown )
                {
                    view->Update( &m_constructionGeomPreview, KIGFX::GEOMETRY );
                }
                else
                {
                    view->SetVisible( &m_constructionGeomPreview, aAnythingShown );
                }

                m_toolMgr->GetToolHolder()->RefreshCanvas();
            } );

    // Initialise manual values from view for compatibility
    m_manualGrid = view->GetGAL()->GetGridSize();
    m_manualVisibleGrid = view->GetGAL()->GetVisibleGridSize();
    m_manualOrigin = VECTOR2I( view->GetGAL()->GetGridOrigin() );
    m_manualGridSnapping = view->GetGAL()->GetGridSnapping();
}


GRID_HELPER::~GRID_HELPER()
{
    if( !m_toolMgr )
        return;

    KIGFX::VIEW& view = *m_toolMgr->GetView();
    view.Remove( &m_constructionGeomPreview );

    if( m_anchorDebug )
        view.Remove( m_anchorDebug.get() );
}


KIGFX::ANCHOR_DEBUG* GRID_HELPER::enableAndGetAnchorDebug()
{
    static bool permitted = ADVANCED_CFG::GetCfg().m_EnableSnapAnchorsDebug;

    if( !m_toolMgr )
        return nullptr;

    if( permitted && !m_anchorDebug )
    {
        KIGFX::VIEW& view = *m_toolMgr->GetView();
        m_anchorDebug = std::make_unique<KIGFX::ANCHOR_DEBUG>();
        view.Add( m_anchorDebug.get() );
        view.SetVisible( m_anchorDebug.get(), true );
    }

    return m_anchorDebug.get();
}


void GRID_HELPER::showConstructionGeometry( bool aShow )
{
    if( m_toolMgr )
        m_toolMgr->GetView()->SetVisible( &m_constructionGeomPreview, aShow );
}


void GRID_HELPER::SetSnapLineDirections( const std::vector<VECTOR2I>& aDirections )
{
    m_snapManager.GetSnapLineManager().SetDirections( aDirections );
}


void GRID_HELPER::SetSnapLineOrigin( const VECTOR2I& aOrigin )
{
    m_snapManager.GetSnapLineManager().SetSnapLineOrigin( aOrigin );
}

void GRID_HELPER::SetSnapLineEnd( const std::optional<VECTOR2I>& aEnd )
{
    m_snapManager.GetSnapLineManager().SetSnapLineEnd( aEnd );
}

void GRID_HELPER::ClearSnapLine()
{
    m_snapManager.GetSnapLineManager().ClearSnapLine();
}


std::optional<VECTOR2I> GRID_HELPER::SnapToConstructionLines( const VECTOR2I& aPoint,
                                                              const VECTOR2I& aNearestGrid,
                                                              const VECTOR2D& aGrid,
                                                              double aSnapRange ) const
{
    const SNAP_LINE_MANAGER& snapLineManager = m_snapManager.GetSnapLineManager();
    const OPT_VECTOR2I&      snapOrigin = snapLineManager.GetSnapLineOrigin();

    wxLogTrace( traceSnap, "SnapToConstructionLines: aPoint=(%d, %d), nearestGrid=(%d, %d), snapRange=%.1f",
                aPoint.x, aPoint.y, aNearestGrid.x, aNearestGrid.y, aSnapRange );

    if( !snapOrigin || snapLineManager.GetDirections().empty() )
    {
        wxLogTrace( traceSnap, "  No snap origin or no directions, returning nullopt" );
        return std::nullopt;
    }

    const VECTOR2I& origin = *snapOrigin;

    wxLogTrace( traceSnap, "  snapOrigin=(%d, %d), directions count=%zu",
                origin.x, origin.y, snapLineManager.GetDirections().size() );

    const std::vector<VECTOR2I>& directions = snapLineManager.GetDirections();
    const std::optional<int>     activeDirection = snapLineManager.GetActiveDirection();

    if( activeDirection )
        wxLogTrace( traceSnap, "  activeDirection=%d", *activeDirection );

    const VECTOR2D originVec( origin );
    const VECTOR2D cursorVec( aPoint );
    const VECTOR2D delta = cursorVec - originVec;

    std::optional<VECTOR2I> bestPoint;
    double                  bestPerp = std::numeric_limits<double>::max();
    double                  bestDistance = std::numeric_limits<double>::max();

    for( size_t ii = 0; ii < directions.size(); ++ii )
    {
        const VECTOR2I& dir = directions[ii];
        VECTOR2D        dirVector( dir );
        double          dirLength = dirVector.EuclideanNorm();

        if( dirLength == 0.0 )
        {
            wxLogTrace( traceSnap, "    Direction %zu: zero length, skipping", ii );
            continue;
        }

        VECTOR2D dirUnit = dirVector / dirLength;

        double    distanceAlong = delta.Dot( dirUnit );
        VECTOR2D  projection = originVec + dirUnit * distanceAlong;
        VECTOR2D  offset = delta - dirUnit * distanceAlong;
        double    perpDistance = offset.EuclideanNorm();

        double snapThreshold = aSnapRange;

        if( activeDirection && *activeDirection == static_cast<int>( ii ) )
        {
            snapThreshold *= 1.5;
            wxLogTrace( traceSnap, "    Direction %zu: ACTIVE, increased snapThreshold=%.1f", ii, snapThreshold );
        }

        wxLogTrace( traceSnap, "    Direction %zu: dir=(%d, %d), perpDist=%.1f, threshold=%.1f",
                    ii, dir.x, dir.y, perpDistance, snapThreshold );

        if( perpDistance > snapThreshold )
        {
            wxLogTrace( traceSnap, "      perpDistance > threshold, skipping" );
            continue;
        }

        VECTOR2D candidate = projection;

        if( canUseGrid() )
        {
            if( dir.x == 0 && dir.y != 0 )
            {
                // Vertical construction line: snap to grid intersection
                candidate.x = origin.x;
                candidate.y = aNearestGrid.y;
                wxLogTrace( traceSnap, "      Vertical snap: candidate=(%d, %d)",
                            (int)candidate.x, (int)candidate.y );
            }
            else if( dir.y == 0 && dir.x != 0 )
            {
                // Horizontal construction line: snap to grid intersection
                candidate.x = aNearestGrid.x;
                candidate.y = origin.y;
                wxLogTrace( traceSnap, "      Horizontal snap: candidate=(%d, %d)",
                            (int)candidate.x, (int)candidate.y );
            }
            else
            {
                // Diagonal construction line: find nearest grid intersection along the line
                // We need to find grid points near the projection point and pick the closest
                // one that lies on the construction line

                // Get the grid origin for proper alignment
                VECTOR2D gridOrigin( GetOrigin() );

                // Calculate the projection point relative to grid
                VECTOR2D relProjection = projection - gridOrigin;

                // Find nearby grid points (check 9 points in a 3x3 grid around the projection)
                std::vector<VECTOR2D> gridPoints;
                for( int dx = -1; dx <= 1; ++dx )
                {
                    for( int dy = -1; dy <= 1; ++dy )
                    {
                        double gridX = std::round( relProjection.x / aGrid.x ) * aGrid.x + dx * aGrid.x;
                        double gridY = std::round( relProjection.y / aGrid.y ) * aGrid.y + dy * aGrid.y;
                        gridPoints.push_back( VECTOR2D( gridX + gridOrigin.x, gridY + gridOrigin.y ) );
                    }
                }

                // Find the grid point closest to the construction line
                double   bestGridDist = std::numeric_limits<double>::max();
                VECTOR2D bestGridPt = projection;

                for( const VECTOR2D& gridPt : gridPoints )
                {
                    // Calculate perpendicular distance from grid point to construction line
                    VECTOR2D gridDelta = gridPt - originVec;
                    double   gridDistAlong = gridDelta.Dot( dirUnit );
                    VECTOR2D gridProjection = originVec + dirUnit * gridDistAlong;
                    double   gridPerpDist = ( gridPt - gridProjection ).EuclideanNorm();

                    // Also consider distance from cursor
                    double distFromCursor = ( gridPt - cursorVec ).EuclideanNorm();

                    // Prefer grid points that are close to the line and close to cursor
                    double score = gridPerpDist + distFromCursor * 0.1;

                    if( score < bestGridDist )
                    {
                        bestGridDist = score;
                        bestGridPt = gridPt;
                    }
                }

                candidate = bestGridPt;
                wxLogTrace( traceSnap, "      Diagonal snap: candidate=(%.1f, %.1f), perpDist=%.1f",
                            candidate.x, candidate.y, bestGridDist );
            }
        }
        else
        {
            wxLogTrace( traceSnap, "      Grid disabled, using projection candidate=(%.1f, %.1f)",
                        candidate.x, candidate.y );
        }

        VECTOR2I candidateInt = KiROUND( candidate );

        if( candidateInt == m_skipPoint )
        {
            wxLogTrace( traceSnap, "      candidateInt matches m_skipPoint, skipping" );
            continue;
        }

        VECTOR2D candidateDelta( candidateInt.x - aPoint.x, candidateInt.y - aPoint.y );
        double    candidateDistance = candidateDelta.EuclideanNorm();

        wxLogTrace( traceSnap, "      candidateInt=(%d, %d), candidateDist=%.1f",
                    candidateInt.x, candidateInt.y, candidateDistance );

        if( perpDistance < bestPerp
                || ( std::abs( perpDistance - bestPerp ) < 1e-9 && candidateDistance < bestDistance ) )
        {
            wxLogTrace( traceSnap, "      NEW BEST: perpDist=%.1f, candDist=%.1f", perpDistance, candidateDistance );
            bestPerp = perpDistance;
            bestDistance = candidateDistance;
            bestPoint = candidateInt;
        }
    }

    if( bestPoint )
    {
        wxLogTrace( traceSnap, "  RETURNING bestPoint=(%d, %d)", bestPoint->x, bestPoint->y );
    }
    else
    {
        wxLogTrace( traceSnap, "  RETURNING nullopt (no valid snap found)" );
    }

    return bestPoint;
}


void GRID_HELPER::updateSnapPoint( const TYPED_POINT2I& aPoint )
{
    if( !m_toolMgr )
        return;

    m_viewSnapPoint.SetPosition( aPoint.m_point );
    m_viewSnapPoint.SetSnapTypes( aPoint.m_types );

    if( m_toolMgr->GetView()->IsVisible( &m_viewSnapPoint ) )
        m_toolMgr->GetView()->Update( &m_viewSnapPoint, KIGFX::GEOMETRY );
    else
        m_toolMgr->GetView()->SetVisible( &m_viewSnapPoint, true );
}


VECTOR2I GRID_HELPER::GetGrid() const
{
    VECTOR2D size = m_toolMgr ? m_toolMgr->GetView()->GetGAL()->GetGridSize() : m_manualGrid;
    return VECTOR2I( KiROUND( size.x ), KiROUND( size.y ) );
}


VECTOR2D GRID_HELPER::GetVisibleGrid() const
{
    return m_toolMgr ? m_toolMgr->GetView()->GetGAL()->GetVisibleGridSize() : m_manualVisibleGrid;
}


VECTOR2I GRID_HELPER::GetOrigin() const
{
    if( m_toolMgr )
    {
        VECTOR2D origin = m_toolMgr->GetView()->GetGAL()->GetGridOrigin();
        return VECTOR2I( origin );
    }

    return m_manualOrigin;
}


GRID_HELPER_GRIDS GRID_HELPER::GetSelectionGrid( const SELECTION& aSelection ) const
{
    GRID_HELPER_GRIDS grid = GetItemGrid( aSelection.Front() );

    // Find the largest grid of all the items and use that
    for( EDA_ITEM* item : aSelection )
    {
        GRID_HELPER_GRIDS itemGrid = GetItemGrid( item );

        if( GetGridSize( itemGrid ) > GetGridSize( grid ) )
            grid = itemGrid;
    }

    return grid;
}


VECTOR2D GRID_HELPER::GetGridSize( GRID_HELPER_GRIDS aGrid ) const
{
    return m_toolMgr ? m_toolMgr->GetView()->GetGAL()->GetGridSize() : m_manualGrid;
}


void GRID_HELPER::SetAuxAxes( bool aEnable, const VECTOR2I& aOrigin )
{
    if( aEnable )
    {
        m_auxAxis = aOrigin;
        m_viewAxis.SetPosition( aOrigin );
        if( m_toolMgr )
            m_toolMgr->GetView()->SetVisible( &m_viewAxis, true );
    }
    else
    {
        m_auxAxis = std::optional<VECTOR2I>();
        if( m_toolMgr )
            m_toolMgr->GetView()->SetVisible( &m_viewAxis, false );
    }
}


VECTOR2I GRID_HELPER::AlignGrid( const VECTOR2I& aPoint ) const
{
    return computeNearest( aPoint, GetGrid(), GetOrigin() );
}


VECTOR2I GRID_HELPER::AlignGrid( const VECTOR2I& aPoint, const VECTOR2D& aGrid,
                                 const VECTOR2D& aOffset ) const
{
    return computeNearest( aPoint, aGrid, aOffset );
}


VECTOR2I GRID_HELPER::computeNearest( const VECTOR2I& aPoint, const VECTOR2I& aGrid,
                                      const VECTOR2I& aOffset ) const
{
    return VECTOR2I( KiROUND( (double) ( aPoint.x - aOffset.x ) / aGrid.x ) * aGrid.x + aOffset.x,
                     KiROUND( (double) ( aPoint.y - aOffset.y ) / aGrid.y ) * aGrid.y + aOffset.y );
}


VECTOR2I GRID_HELPER::Align( const VECTOR2I& aPoint ) const
{
    return Align( aPoint, GetGrid(), GetOrigin() );
}


VECTOR2I GRID_HELPER::Align( const VECTOR2I& aPoint, const VECTOR2D& aGrid,
                             const VECTOR2D& aOffset ) const
{
    if( !canUseGrid() )
        return aPoint;

    VECTOR2I nearest = AlignGrid( aPoint, aGrid, aOffset );

    if( !m_auxAxis )
        return nearest;

    if( std::abs( m_auxAxis->x - aPoint.x ) < std::abs( nearest.x - aPoint.x ) )
        nearest.x = m_auxAxis->x;

    if( std::abs( m_auxAxis->y - aPoint.y ) < std::abs( nearest.y - aPoint.y ) )
        nearest.y = m_auxAxis->y;

    return nearest;
}


bool GRID_HELPER::canUseGrid() const
{
    return m_enableGrid && ( m_toolMgr ? m_toolMgr->GetView()->GetGAL()->GetGridSnapping()
                                       : m_manualGridSnapping );
}


std::optional<VECTOR2I> GRID_HELPER::GetSnappedPoint() const
{
    if( m_snapItem )
        return m_snapItem->pos;

    return std::nullopt;
}
