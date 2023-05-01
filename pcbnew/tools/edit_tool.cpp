/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * Copyright (C) 2017-2023 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include <advanced_config.h>
#include <limits>
#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pcb_shape.h>
#include <pcb_group.h>
#include <pcb_target.h>
#include <pcb_text.h>
#include <pcb_textbox.h>
#include <collectors.h>
#include <pcb_edit_frame.h>
#include <drawing_sheet/ds_proxy_view_item.h>
#include <kiway.h>
#include <array_creator.h>
#include <status_popup.h>
#include <tool/selection_conditions.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <tools/edit_tool.h>
#include <tools/pcb_picker_tool.h>
#include <tools/tool_event_utils.h>
#include <tools/pcb_grid_helper.h>
#include <tools/pad_tool.h>
#include <view/view_controls.h>
#include <connectivity/connectivity_algo.h>
#include <connectivity/connectivity_items.h>
#include <core/kicad_algo.h>
#include <bitmaps.h>
#include <cassert>
#include <functional>
using namespace std::placeholders;
#include "kicad_clipboard.h"
#include <wx/hyperlink.h>
#include <router/router_tool.h>
#include <dialogs/dialog_move_exact.h>
#include <dialogs/dialog_track_via_properties.h>
#include <dialogs/dialog_unit_entry.h>
#include <board_commit.h>
#include <zone_filler.h>
#include <pcb_bitmap.h>

const unsigned int EDIT_TOOL::COORDS_PADDING = pcbIUScale.mmToIU( 20 );

EDIT_TOOL::EDIT_TOOL() :
        PCB_TOOL_BASE( "pcbnew.InteractiveEdit" ),
        m_selectionTool( nullptr ),
        m_dragging( false )
{
}


void EDIT_TOOL::Reset( RESET_REASON aReason )
{
    m_dragging = false;

    m_statusPopup = std::make_unique<STATUS_TEXT_POPUP>( getEditFrame<PCB_BASE_EDIT_FRAME>() );

    if( aReason != RUN )
        m_commit.reset( new BOARD_COMMIT( this ) );
}


POSITIONING_TOOLS_MENU::POSITIONING_TOOLS_MENU( TOOL_INTERACTIVE* aTool ) :
        CONDITIONAL_MENU( aTool )
{
    SetIcon( BITMAPS::special_tools );
    SetTitle( _( "Positioning Tools" ) );

    AddItem( PCB_ACTIONS::moveExact,         SELECTION_CONDITIONS::ShowAlways );
    AddItem( PCB_ACTIONS::moveWithReference, SELECTION_CONDITIONS::ShowAlways );
    AddItem( PCB_ACTIONS::copyWithReference, SELECTION_CONDITIONS::ShowAlways );
    AddItem( PCB_ACTIONS::positionRelative,  SELECTION_CONDITIONS::ShowAlways );
}


bool EDIT_TOOL::Init()
{
    // Find the selection tool, so they can cooperate
    m_selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();

    auto positioningToolsSubMenu = std::make_shared<POSITIONING_TOOLS_MENU>( this );
    m_selectionTool->GetToolMenu().RegisterSubMenu( positioningToolsSubMenu );

    auto propertiesCondition =
            [&]( const SELECTION& aSel )
            {
                if( aSel.GetSize() == 0 )
                {
                    if( getView()->IsLayerVisible( LAYER_SCHEMATIC_DRAWINGSHEET ) )
                    {
                        DS_PROXY_VIEW_ITEM* ds = canvas()->GetDrawingSheet();
                        VECTOR2D            cursor = getViewControls()->GetCursorPosition( false );

                        if( ds && ds->HitTestDrawingSheetItems( getView(), cursor ) )
                            return true;
                    }

                    return false;
                }

                if( aSel.GetSize() == 1 )
                    return true;

                for( EDA_ITEM* item : aSel )
                {
                    if( !dynamic_cast<PCB_TRACK*>( item ) )
                        return false;
                }

                return true;
            };

    auto inFootprintEditor =
            [ this ]( const SELECTION& aSelection )
            {
                return m_isFootprintEditor;
            };

    auto canMirror =
            [ this ]( const SELECTION& aSelection )
            {
                if( !m_isFootprintEditor
                    && SELECTION_CONDITIONS::OnlyTypes( { PCB_PAD_T } )( aSelection ) )
                {
                    return false;
                }

                return SELECTION_CONDITIONS::HasTypes( EDIT_TOOL::MirrorableItems )( aSelection );
            };

    auto singleFootprintCondition = SELECTION_CONDITIONS::OnlyTypes( { PCB_FOOTPRINT_T } )
                                        && SELECTION_CONDITIONS::Count( 1 );

    auto noActiveToolCondition =
            [ this ]( const SELECTION& aSelection )
            {
                return frame()->ToolStackIsEmpty();
            };

    auto notMovingCondition =
            [ this ]( const SELECTION& aSelection )
            {
                return !frame()->IsCurrentTool( PCB_ACTIONS::move )
                       && !frame()->IsCurrentTool( PCB_ACTIONS::moveWithReference )
                       && !frame()->IsCurrentTool( PCB_ACTIONS::moveIndividually );
            };

    auto noItemsCondition =
            [ this ]( const SELECTION& aSelections ) -> bool
            {
                return frame()->GetBoard() && !frame()->GetBoard()->IsEmpty();
            };

    auto isSkippable =
            [ this ]( const SELECTION& aSelection )
            {
                return frame()->IsCurrentTool( PCB_ACTIONS::moveIndividually );
            };

    static std::vector<KICAD_T> connectedTypes = { PCB_TRACE_T,
                                                   PCB_ARC_T,
                                                   PCB_VIA_T,
                                                   PCB_PAD_T,
                                                   PCB_ZONE_T };

    static std::vector<KICAD_T> unroutableTypes = { PCB_TRACE_T,
                                                    PCB_ARC_T,
                                                    PCB_VIA_T,
                                                    PCB_PAD_T,
                                                    PCB_FOOTPRINT_T };

    static std::vector<KICAD_T> trackTypes = { PCB_TRACE_T,
                                               PCB_ARC_T,
                                               PCB_VIA_T };

    static std::vector<KICAD_T> filletTypes = { PCB_SHAPE_LOCATE_POLY_T,
                                                PCB_SHAPE_LOCATE_RECT_T,
                                                PCB_SHAPE_LOCATE_SEGMENT_T };


    // Add context menu entries that are displayed when selection tool is active
    CONDITIONAL_MENU& menu = m_selectionTool->GetToolMenu().GetMenu();

    menu.AddItem( PCB_ACTIONS::move,              SELECTION_CONDITIONS::NotEmpty
                                                      && notMovingCondition );
    menu.AddItem( PCB_ACTIONS::unrouteSelected,   SELECTION_CONDITIONS::NotEmpty
                                                      && SELECTION_CONDITIONS::OnlyTypes( unroutableTypes )
                                                      && notMovingCondition
                                                      && !inFootprintEditor );
    menu.AddItem( PCB_ACTIONS::moveIndividually,  SELECTION_CONDITIONS::MoreThan( 1 )
                                                      && notMovingCondition );
    menu.AddItem( PCB_ACTIONS::skip,              isSkippable );
    menu.AddItem( PCB_ACTIONS::breakTrack,        SELECTION_CONDITIONS::Count( 1 )
                                                      && SELECTION_CONDITIONS::OnlyTypes( trackTypes ) );
    menu.AddItem( PCB_ACTIONS::drag45Degree,      SELECTION_CONDITIONS::Count( 1 )
                                                      && SELECTION_CONDITIONS::OnlyTypes( GENERAL_COLLECTOR::DraggableItems ) );
    menu.AddItem( PCB_ACTIONS::dragFreeAngle,     SELECTION_CONDITIONS::Count( 1 )
                                                      && SELECTION_CONDITIONS::OnlyTypes( GENERAL_COLLECTOR::DraggableItems )
                                                      && !SELECTION_CONDITIONS::OnlyTypes( { PCB_FOOTPRINT_T } ) );
    menu.AddItem( PCB_ACTIONS::filletTracks,      SELECTION_CONDITIONS::OnlyTypes( trackTypes ) );
    menu.AddItem( PCB_ACTIONS::filletLines,       SELECTION_CONDITIONS::OnlyTypes( filletTypes ) );
    menu.AddItem( PCB_ACTIONS::rotateCcw,         SELECTION_CONDITIONS::NotEmpty );
    menu.AddItem( PCB_ACTIONS::rotateCw,          SELECTION_CONDITIONS::NotEmpty );
    menu.AddItem( PCB_ACTIONS::flip,              SELECTION_CONDITIONS::NotEmpty );
    menu.AddItem( PCB_ACTIONS::mirrorH,           canMirror );
    menu.AddItem( PCB_ACTIONS::mirrorV,           canMirror );
    menu.AddItem( PCB_ACTIONS::swap,              SELECTION_CONDITIONS::MoreThan( 1 ) );
    menu.AddItem( PCB_ACTIONS::packAndMoveFootprints, SELECTION_CONDITIONS::MoreThan( 1 )
                                                      && SELECTION_CONDITIONS::HasType( PCB_FOOTPRINT_T ) );

    menu.AddItem( PCB_ACTIONS::properties,        propertiesCondition );

    menu.AddItem( PCB_ACTIONS::assignNetClass,    SELECTION_CONDITIONS::OnlyTypes( connectedTypes )
                                                      && !inFootprintEditor );
    menu.AddItem( PCB_ACTIONS::inspectClearance,  SELECTION_CONDITIONS::Count( 2 ) );

    // Footprint actions
    menu.AddSeparator();
    menu.AddItem( PCB_ACTIONS::editFpInFpEditor,  singleFootprintCondition );
    menu.AddItem( PCB_ACTIONS::updateFootprint,   singleFootprintCondition );
    menu.AddItem( PCB_ACTIONS::changeFootprint,   singleFootprintCondition );

    // Add the submenu for the special positioning tools
    menu.AddSeparator( 100 );
    menu.AddMenu( positioningToolsSubMenu.get(),  SELECTION_CONDITIONS::NotEmpty, 100 );

    menu.AddSeparator( 150 );
    menu.AddItem( ACTIONS::cut,                   SELECTION_CONDITIONS::NotEmpty, 150 );
    menu.AddItem( ACTIONS::copy,                  SELECTION_CONDITIONS::NotEmpty, 150 );

    // Selection tool handles the context menu for some other tools, such as the Picker.
    // Don't add things like Paste when another tool is active.
    menu.AddItem( ACTIONS::paste,                 noActiveToolCondition, 150 );
    menu.AddItem( ACTIONS::pasteSpecial,          noActiveToolCondition && !inFootprintEditor, 150 );
    menu.AddItem( ACTIONS::duplicate,             SELECTION_CONDITIONS::NotEmpty, 150 );
    menu.AddItem( ACTIONS::doDelete,              SELECTION_CONDITIONS::NotEmpty, 150 );

    menu.AddSeparator( 150 );
    menu.AddItem( ACTIONS::selectAll, noItemsCondition, 150 );

    return true;
}


int EDIT_TOOL::GetAndPlace( const TOOL_EVENT& aEvent )
{
    // GetAndPlace makes sense only in board editor, although it is also called
    // in fpeditor, that shares the same EDIT_TOOL list
    if( !getEditFrame<PCB_BASE_FRAME>()->IsType( FRAME_PCB_EDITOR ) )
        return 0;

    PCB_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    FOOTPRINT*          fp = getEditFrame<PCB_BASE_FRAME>()->GetFootprintFromBoardByReference();

    if( fp )
    {
        m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
        m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, (void*) fp );

        selectionTool->GetSelection().SetReferencePoint( fp->GetPosition() );
        m_toolMgr->RunAction( PCB_ACTIONS::move, false );
    }

    return 0;
}


bool EDIT_TOOL::invokeInlineRouter( int aDragMode )
{
    ROUTER_TOOL* theRouter = m_toolMgr->GetTool<ROUTER_TOOL>();

    if( !theRouter )
        return false;

    // don't allow switch from moving to dragging
    if( m_dragging )
    {
        wxBell();
        return false;
    }

    // make sure we don't accidentally invoke inline routing mode while the router is already
    // active!
    if( theRouter->IsToolActive() )
        return false;

    if( theRouter->CanInlineDrag( aDragMode ) )
    {
        m_toolMgr->RunAction( PCB_ACTIONS::routerInlineDrag, true,
                              static_cast<intptr_t>( aDragMode ) );
        return true;
    }

    return false;
}


bool EDIT_TOOL::isRouterActive() const
{
    ROUTER_TOOL* router = m_toolMgr->GetTool<ROUTER_TOOL>();

    return router && router->RoutingInProgress();
}


int EDIT_TOOL::Drag( const TOOL_EVENT& aEvent )
{
    if( !m_toolMgr->GetTool<ROUTER_TOOL>() )
        return false; // don't drag when no router tool (i.e. fp editor)

    if( m_toolMgr->GetTool<ROUTER_TOOL>()->IsToolActive() )
        return false; // don't drag when router is already active

    int mode = PNS::DM_ANY;

    if( aEvent.IsAction( &PCB_ACTIONS::dragFreeAngle ) )
        mode |= PNS::DM_FREE_ANGLE;

    PCB_SELECTION& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
                sTool->FilterCollectorForFreePads( aCollector );
                sTool->FilterCollectorForHierarchy( aCollector, true );

                if( aCollector.GetCount() > 1 )
                    sTool->GuessSelectionCandidates( aCollector, aPt );

                /*
                 * If we have a knee between two segments, or a via attached to two segments,
                 * then drop the selection to a single item.
                 */

                std::vector<PCB_TRACK*> tracks;
                std::vector<PCB_TRACK*> vias;

                for( EDA_ITEM* item : aCollector )
                {
                    if( PCB_TRACK* track = dynamic_cast<PCB_TRACK*>( item ) )
                    {
                        if( track->Type() == PCB_VIA_T )
                            vias.push_back( track );
                        else
                            tracks.push_back( track );
                    }
                }

                auto connected = []( PCB_TRACK* track, const VECTOR2I& pt )
                                 {
                                     return track->GetStart() == pt || track->GetEnd() == pt;
                                 };

                if( tracks.size() == 2 && vias.size() == 0 )
                {
                    if( connected( tracks[0], tracks[1]->GetStart() )
                            || connected( tracks[0], tracks[1]->GetEnd() ) )
                    {
                        aCollector.Remove( tracks[1] );
                    }
                }
                else if( tracks.size() == 2 && vias.size() == 1 )
                {
                    if( connected( tracks[0], vias[0]->GetPosition() )
                            && connected( tracks[1], vias[0]->GetPosition() ) )
                    {
                        aCollector.Remove( tracks[0] );
                        aCollector.Remove( tracks[1] );
                    }
                }
            },
            true /* prompt user regarding locked items */ );

    if( selection.Empty() )
        return 0;

    if( selection.Size() == 1 && selection.Front()->Type() == PCB_ARC_T )
    {
        // TODO: This really should be done in PNS to ensure DRC is maintained, but for now
        // it allows interactive editing of an arc track
        return DragArcTrack( aEvent );
    }
    else
    {
        invokeInlineRouter( mode );
    }

    return 0;
}


int EDIT_TOOL::DragArcTrack( const TOOL_EVENT& aEvent )
{
    PCB_SELECTION& selection = m_selectionTool->GetSelection();

    if( selection.Size() != 1 || selection.Front()->Type() != PCB_ARC_T )
        return 0;

    PCB_ARC*  theArc = static_cast<PCB_ARC*>( selection.Front() );
    EDA_ANGLE maxTangentDeviation( ADVANCED_CFG::GetCfg().m_MaxTangentAngleDeviation, DEGREES_T );

    if( theArc->GetAngle() + maxTangentDeviation >= ANGLE_180 )
    {
        wxString msg = wxString::Format( _( "Unable to resize arc tracks of %s or greater." ),
                                         EDA_UNIT_UTILS::UI::MessageTextFromValue( ANGLE_180 - maxTangentDeviation ) );
        frame()->ShowInfoBarError( msg );

        return 0; // don't bother with > 180 degree arcs
    }

    KIGFX::VIEW_CONTROLS* controls = getViewControls();

    Activate();
    // Must be done after Activate() so that it gets set into the correct context
    controls->ShowCursor( true );
    controls->SetAutoPan( true );

    bool     restore_state = false;
    VECTOR2I arcCenter = theArc->GetCenter();
    SEG      tanStart = SEG( arcCenter, theArc->GetStart() ).PerpendicularSeg( theArc->GetStart() );
    SEG      tanEnd = SEG( arcCenter, theArc->GetEnd() ).PerpendicularSeg( theArc->GetEnd() );

    //Ensure the tangent segments are in the correct orientation
    OPT_VECTOR2I tanIntersect = tanStart.IntersectLines( tanEnd );

    if( !tanIntersect )
        return 0;

    tanStart.A = *tanIntersect;
    tanStart.B = theArc->GetStart();
    tanEnd.A = *tanIntersect;
    tanEnd.B = theArc->GetEnd();

    auto getUniqueTrackAtAnchorCollinear =
            [&]( const VECTOR2I& aAnchor, const SEG& aCollinearSeg ) -> PCB_TRACK*
            {
                std::shared_ptr<CONNECTIVITY_DATA> conn = board()->GetConnectivity();

                // Allow items at a distance within the width of the arc track
                int allowedDeviation = theArc->GetWidth();

                std::vector<BOARD_CONNECTED_ITEM*> itemsOnAnchor;

                for( int i = 0; i < 3; i++ )
                {
                    itemsOnAnchor = conn->GetConnectedItemsAtAnchor( theArc, aAnchor,
                                                                     { PCB_PAD_T, PCB_VIA_T,
                                                                       PCB_TRACE_T, PCB_ARC_T },
                                                                     allowedDeviation );
                    allowedDeviation /= 2;

                    if( itemsOnAnchor.size() == 1 )
                        break;
                }

                PCB_TRACK* retval = nullptr;

                if( itemsOnAnchor.size() == 1 && itemsOnAnchor.front()->Type() == PCB_TRACE_T )
                {
                    retval = static_cast<PCB_TRACK*>( itemsOnAnchor.front() );
                    SEG trackSeg( retval->GetStart(), retval->GetEnd() );

                    // Allow deviations in colinearity as defined in ADVANCED_CFG
                    if( trackSeg.Angle( aCollinearSeg ) > maxTangentDeviation )
                        retval = nullptr;
                }

                if( !retval )
                {
                    retval = new PCB_TRACK( theArc->GetParent() );
                    retval->SetStart( aAnchor );
                    retval->SetEnd( aAnchor );
                    retval->SetNet( theArc->GetNet() );
                    retval->SetLayer( theArc->GetLayer() );
                    retval->SetWidth( theArc->GetWidth() );
                    retval->SetLocked( theArc->IsLocked() );
                    retval->SetFlags( IS_NEW );
                    getView()->Add( retval );
                }

                return retval;
            };

    PCB_TRACK* trackOnStart = getUniqueTrackAtAnchorCollinear( theArc->GetStart(), tanStart);
    PCB_TRACK* trackOnEnd = getUniqueTrackAtAnchorCollinear( theArc->GetEnd(), tanEnd );

    // Make copies of items to be edited
    PCB_ARC*   theArcCopy = new PCB_ARC( *theArc );
    PCB_TRACK* trackOnStartCopy = new PCB_TRACK( *trackOnStart );
    PCB_TRACK* trackOnEndCopy = new PCB_TRACK( *trackOnEnd );

    if( trackOnStart->GetLength() != 0 )
    {
        tanStart.A = trackOnStart->GetStart();
        tanStart.B = trackOnStart->GetEnd();
    }

    if( trackOnEnd->GetLength() != 0 )
    {
        tanEnd.A = trackOnEnd->GetStart();
        tanEnd.B = trackOnEnd->GetEnd();
    }

    // Recalculate intersection point
    if( tanIntersect = tanStart.IntersectLines( tanEnd ); !tanIntersect )
        return 0;

    auto isTrackStartClosestToArcStart =
            [&]( PCB_TRACK* aTrack ) -> bool
            {
                double trackStartToArcStart = GetLineLength( aTrack->GetStart(), theArc->GetStart() );
                double trackEndToArcStart = GetLineLength( aTrack->GetEnd(), theArc->GetStart() );

                return trackStartToArcStart < trackEndToArcStart;
            };

    bool isStartTrackOnStartPt = isTrackStartClosestToArcStart( trackOnStart );
    bool isEndTrackOnStartPt = isTrackStartClosestToArcStart( trackOnEnd );

    // Calculate constraints
    //======================
    // maxTanCircle is the circle with maximum radius that is tangent to the two adjacent straight
    // tracks and whose tangent points are constrained within the original tracks and their
    // projected intersection points.
    //
    // The cursor will be constrained first within the isosceles triangle formed by the segments
    // cSegTanStart, cSegTanEnd and cSegChord. After that it will be constrained to be outside
    // maxTanCircle.
    //
    //
    //                   ____________  <-cSegTanStart
    //                  /     *   . '   *
    //    cSegTanEnd-> /  *   . '           *
    //                /*  . ' <-cSegChord     *
    //               /. '
    //              /*                           *
    //
    //              *             c               *  <-maxTanCircle
    //
    //               *                           *
    //
    //                  *                     *
    //                    *                 *
    //                        *        *
    //

    auto getFurthestPointToTanInterstect =
            [&]( VECTOR2I& aPointA, VECTOR2I& aPointB ) -> VECTOR2I
            {
                if( ( aPointA - *tanIntersect ).EuclideanNorm()
                    > ( aPointB - *tanIntersect ).EuclideanNorm() )
                {
                    return aPointA;
                }
                else
                {
                    return aPointB;
                }
            };

    CIRCLE   maxTanCircle;
    VECTOR2I tanStartPoint = getFurthestPointToTanInterstect( tanStart.A, tanStart.B );
    VECTOR2I tanEndPoint = getFurthestPointToTanInterstect( tanEnd.A, tanEnd.B );
    VECTOR2I tempTangentPoint = tanEndPoint;

    if( getFurthestPointToTanInterstect( tanStartPoint, tanEndPoint ) == tanEndPoint )
        tempTangentPoint = tanStartPoint;

    maxTanCircle.ConstructFromTanTanPt( tanStart, tanEnd, tempTangentPoint );
    VECTOR2I maxTanPtStart = tanStart.LineProject( maxTanCircle.Center );
    VECTOR2I maxTanPtEnd = tanEnd.LineProject( maxTanCircle.Center );

    SEG cSegTanStart( maxTanPtStart, *tanIntersect );
    SEG cSegTanEnd( maxTanPtEnd, *tanIntersect );
    SEG cSegChord( maxTanPtStart, maxTanPtEnd );

    int cSegTanStartSide = cSegTanStart.Side( theArc->GetMid() );
    int cSegTanEndSide = cSegTanEnd.Side( theArc->GetMid() );
    int cSegChordSide = cSegChord.Side( theArc->GetMid() );

    bool eatFirstMouseUp = true;

    // Start the tool loop
    while( TOOL_EVENT* evt = Wait() )
    {
        m_cursor = controls->GetMousePosition();

        // Constrain cursor within the isosceles triangle
        if( cSegTanStartSide != cSegTanStart.Side( m_cursor )
            || cSegTanEndSide != cSegTanEnd.Side( m_cursor )
            || cSegChordSide != cSegChord.Side( m_cursor ) )
        {
            std::vector<VECTOR2I> possiblePoints;

            possiblePoints.push_back( cSegTanEnd.NearestPoint( m_cursor ) );
            possiblePoints.push_back( cSegChord.NearestPoint( m_cursor ) );

            VECTOR2I closest = cSegTanStart.NearestPoint( m_cursor );

            for( VECTOR2I candidate : possiblePoints )
            {
                if( ( candidate - m_cursor ).SquaredEuclideanNorm()
                    < ( closest - m_cursor ).SquaredEuclideanNorm() )
                {
                    closest = candidate;
                }
            }

            m_cursor = closest;
        }

        // Constrain cursor to be outside maxTanCircle
        if( ( m_cursor - maxTanCircle.Center ).EuclideanNorm() < maxTanCircle.Radius )
            m_cursor = maxTanCircle.NearestPoint( m_cursor );

        controls->ForceCursorPosition( true, m_cursor );

        // Calculate resulting object coordinates
        CIRCLE circlehelper;
        circlehelper.ConstructFromTanTanPt( cSegTanStart, cSegTanEnd, m_cursor );

        VECTOR2I newCenter = circlehelper.Center;
        VECTOR2I newStart = cSegTanStart.LineProject( newCenter );
        VECTOR2I newEnd = cSegTanEnd.LineProject( newCenter );
        VECTOR2I newMid = CalcArcMid( newStart, newEnd, newCenter );

        // Update objects
        theArc->SetStart( newStart );
        theArc->SetEnd( newEnd );
        theArc->SetMid( newMid );

        if( isStartTrackOnStartPt )
            trackOnStart->SetStart( newStart );
        else
            trackOnStart->SetEnd( newStart );

        if( isEndTrackOnStartPt )
            trackOnEnd->SetStart( newEnd );
        else
            trackOnEnd->SetEnd( newEnd );

        // Update view
        getView()->Update( trackOnStart );
        getView()->Update( trackOnEnd );
        getView()->Update( theArc );

        // Handle events
        if( evt->IsMotion() || evt->IsDrag( BUT_LEFT ) )
        {
            eatFirstMouseUp = false;
        }
        else if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            restore_state = true; // Canceling the tool means that items have to be restored
            break;                // Finish
        }
        else if( evt->IsAction( &ACTIONS::undo ) )
        {
            restore_state = true; // Perform undo locally
            break;                // Finish
        }
        else if( evt->IsMouseUp( BUT_LEFT ) || evt->IsClick( BUT_LEFT )
                || evt->IsDblClick( BUT_LEFT ) )
        {
            // Eat mouse-up/-click events that leaked through from the lock dialog
            if( eatFirstMouseUp && evt->Parameter<intptr_t>() != ACTIONS::CURSOR_CLICK )
            {
                eatFirstMouseUp = false;
                continue;
            }

            break; // Finish
        }
    }

    // Ensure we only do one commit operation on each object
    auto processTrack =
            [&]( PCB_TRACK* aTrack, PCB_TRACK* aTrackCopy, int aMaxLengthIU ) -> bool
            {
                if( aTrack->IsNew() )
                {
                    getView()->Remove( aTrack );

                    if( aTrack->GetLength() <= aMaxLengthIU )
                    {
                        aTrack->SetParentGroup( nullptr );
                        delete aTrack;
                        aTrack = nullptr;

                        aTrackCopy->SetParentGroup( nullptr );
                        delete aTrackCopy;
                        aTrackCopy = nullptr;

                        return false;
                    }
                    else
                    {
                        m_commit->Add( aTrack );

                        aTrackCopy->SetParentGroup( nullptr );
                        delete aTrackCopy;
                        aTrackCopy = nullptr;

                        return true;
                    }
                }
                else if( aTrack->GetLength() <= aMaxLengthIU )
                {
                    aTrack->SwapItemData( aTrackCopy ); //restore the original before notifying COMMIT
                    m_commit->Remove( aTrack );

                    aTrackCopy->SetParentGroup( nullptr );
                    delete aTrackCopy;
                    aTrackCopy = nullptr;

                    return false;
                }
                else
                {
                    m_commit->Modified( aTrack, aTrackCopy );
                }

                return true;
            };

    // Amend the end points of the arc if we delete the joining tracks
    VECTOR2I newStart = trackOnStart->GetStart();
    VECTOR2I newEnd = trackOnEnd->GetStart();

    if( isStartTrackOnStartPt )
        newStart = trackOnStart->GetEnd();

    if( isEndTrackOnStartPt )
        newEnd = trackOnEnd->GetEnd();

    int maxLengthIU =
            KiROUND( ADVANCED_CFG::GetCfg().m_MaxTrackLengthToKeep * pcbIUScale.IU_PER_MM );

    if( !processTrack( trackOnStart, trackOnStartCopy, maxLengthIU ) )
        theArc->SetStart( newStart );

    if( !processTrack( trackOnEnd, trackOnEndCopy, maxLengthIU ) )
        theArc->SetEnd( newEnd );

    processTrack( theArc, theArcCopy, 0 ); // only delete the arc if start and end points coincide

    // Should we commit?
    if( restore_state )
        m_commit->Revert();
    else
        m_commit->Push( _( "Drag Arc Track" ) );

    return 0;
}


int EDIT_TOOL::ChangeTrackWidth( const TOOL_EVENT& aEvent )
{
    const PCB_SELECTION& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
                // Iterate from the back so we don't have to worry about removals.
                for( int i = aCollector.GetCount() - 1; i >= 0; --i )
                {
                    BOARD_ITEM* item = aCollector[ i ];

                    if( !dynamic_cast<PCB_TRACK*>( item ) )
                        aCollector.Remove( item );
                }
            },
            true /* prompt user regarding locked items */ );

    for( EDA_ITEM* item : selection )
    {
        if( item->Type() == PCB_VIA_T )
        {
            PCB_VIA* via = static_cast<PCB_VIA*>( item );

            m_commit->Modify( via );

            int new_width;
            int new_drill;

            if( via->GetViaType() == VIATYPE::MICROVIA )
            {
                NETCLASS* netClass = via->GetEffectiveNetClass();

                new_width = netClass->GetuViaDiameter();
                new_drill = netClass->GetuViaDrill();
            }
            else
            {
                new_width = board()->GetDesignSettings().GetCurrentViaSize();
                new_drill = board()->GetDesignSettings().GetCurrentViaDrill();
            }

            via->SetDrill( new_drill );
            via->SetWidth( new_width );
        }
        else if( item->Type() == PCB_TRACE_T || item->Type() == PCB_ARC_T )
        {
            PCB_TRACK* track = dynamic_cast<PCB_TRACK*>( item );

            wxCHECK( track, 0 );

            m_commit->Modify( track );

            int new_width = board()->GetDesignSettings().GetCurrentTrackWidth();
            track->SetWidth( new_width );
        }
    }

    m_commit->Push( _( "Edit track width/via size" ) );

    if( selection.IsHover() )
    {
        m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

        // Notify other tools of the changes -- This updates the visual ratsnest
        m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );
    }

    return 0;
}


int EDIT_TOOL::FilletTracks( const TOOL_EVENT& aEvent )
{
    // Store last used fillet radius to allow pressing "enter" if repeat fillet is required
    static long long filletRadiusIU = 0;

    PCB_SELECTION& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
                // Iterate from the back so we don't have to worry about removals.
                for( int i = aCollector.GetCount() - 1; i >= 0; --i )
                {
                    BOARD_ITEM* item = aCollector[i];

                    if( !dynamic_cast<PCB_TRACK*>( item ) )
                        aCollector.Remove( item );
                }
            },
            true /* prompt user regarding locked items */ );

    if( selection.Size() < 2 )
    {
        frame()->ShowInfoBarMsg( _( "At least two straight track segments must be selected." ) );
        return 0;
    }

    WX_UNIT_ENTRY_DIALOG dia( frame(), _( "Enter fillet radius:" ), _( "Fillet Tracks" ),
                              filletRadiusIU );

    if( dia.ShowModal() == wxID_CANCEL )
        return 0;

    filletRadiusIU = dia.GetValue();

    if( filletRadiusIU == 0 )
    {
        frame()->ShowInfoBarMsg( _( "A radius of zero was entered.\n"
                                    "The fillet operation was not performed." ) );
        return 0;
    }

    struct FILLET_OP
    {
        PCB_TRACK* t1;
        PCB_TRACK* t2;
        // Start point of track is modified after PCB_ARC is added, otherwise the end point:
        bool       t1Start = true;
        bool       t2Start = true;
    };

    std::vector<FILLET_OP> filletOperations;
    bool                   operationPerformedOnAtLeastOne = false;
    bool                   didOneAttemptFail              = false;
    std::set<PCB_TRACK*>   processedTracks;

    for( EDA_ITEM* item : selection )
    {
        PCB_TRACK* track = dyn_cast<PCB_TRACK*>( item );

        if( !track || track->Type() != PCB_TRACE_T || track->GetLength() == 0 )
        {
            continue;
        }

        auto processFilletOp =
                [&]( bool aStartPoint )
                {
                    std::shared_ptr<CONNECTIVITY_DATA> c = board()->GetConnectivity();
                    VECTOR2I                           anchor = aStartPoint ? track->GetStart()
                                                                            : track->GetEnd();
                    std::vector<BOARD_CONNECTED_ITEM*> itemsOnAnchor;

                    itemsOnAnchor = c->GetConnectedItemsAtAnchor( track, anchor,
                                                                  { PCB_PAD_T, PCB_VIA_T,
                                                                    PCB_TRACE_T, PCB_ARC_T } );

                    if( itemsOnAnchor.size() > 0
                            && selection.Contains( itemsOnAnchor.at( 0 ) )
                            && itemsOnAnchor.at( 0 )->Type() == PCB_TRACE_T )
                    {
                        PCB_TRACK* trackOther = dyn_cast<PCB_TRACK*>( itemsOnAnchor.at( 0 ) );

                        // Make sure we don't fillet the same pair of tracks twice
                        if( processedTracks.find( trackOther ) == processedTracks.end() )
                        {
                            if( itemsOnAnchor.size() == 1 )
                            {
                                FILLET_OP filletOp;
                                filletOp.t1      = track;
                                filletOp.t2      = trackOther;
                                filletOp.t1Start = aStartPoint;
                                filletOp.t2Start = track->IsPointOnEnds( filletOp.t2->GetStart() );
                                filletOperations.push_back( filletOp );
                            }
                            else
                            {
                                // User requested to fillet these two tracks but not possible as
                                // there are other elements connected at that point
                                didOneAttemptFail = true;
                            }
                        }
                    }
                };

        processFilletOp( true ); // on the start point of track
        processFilletOp( false ); // on the end point of track

        processedTracks.insert( track );
    }

    std::vector<BOARD_ITEM*> itemsToAddToSelection;

    for( FILLET_OP filletOp : filletOperations )
    {
        PCB_TRACK* track1 = filletOp.t1;
        PCB_TRACK* track2 = filletOp.t2;

        bool trackOnStart = track1->IsPointOnEnds( track2->GetStart() );
        bool trackOnEnd   = track1->IsPointOnEnds( track2->GetEnd() );

        if( trackOnStart && trackOnEnd )
            continue; // Ignore duplicate tracks

        if( ( trackOnStart || trackOnEnd ) && track1->GetLayer() == track2->GetLayer() )
        {
            SEG t1Seg( track1->GetStart(), track1->GetEnd() );
            SEG t2Seg( track2->GetStart(), track2->GetEnd() );

            if( t1Seg.ApproxCollinear( t2Seg ) )
                continue;

            SHAPE_ARC sArc( t1Seg, t2Seg, filletRadiusIU );
            VECTOR2I  t1newPoint, t2newPoint;

            auto setIfPointOnSeg =
                    []( VECTOR2I& aPointToSet, SEG aSegment, VECTOR2I aVecToTest )
                    {
                        VECTOR2I segToVec = aSegment.NearestPoint( aVecToTest ) - aVecToTest;

                        // Find out if we are on the segment (minimum precision)
                        if( segToVec.EuclideanNorm() < SHAPE_ARC::MIN_PRECISION_IU )
                        {
                            aPointToSet.x = aVecToTest.x;
                            aPointToSet.y = aVecToTest.y;
                            return true;
                        }

                        return false;
                    };

            //Do not draw a fillet if the end points of the arc are not within the track segments
            if( !setIfPointOnSeg( t1newPoint, t1Seg, sArc.GetP0() )
                    && !setIfPointOnSeg( t2newPoint, t2Seg, sArc.GetP0() ) )
            {
                didOneAttemptFail = true;
                continue;
            }

            if( !setIfPointOnSeg( t1newPoint, t1Seg, sArc.GetP1() )
                    && !setIfPointOnSeg( t2newPoint, t2Seg, sArc.GetP1() ) )
            {
                didOneAttemptFail = true;
                continue;
            }

            PCB_ARC* tArc = new PCB_ARC( frame()->GetBoard(), &sArc );
            tArc->SetLayer( track1->GetLayer() );
            tArc->SetWidth( track1->GetWidth() );
            tArc->SetNet( track1->GetNet() );
            tArc->SetLocked( track1->IsLocked() );
            m_commit->Add( tArc );
            itemsToAddToSelection.push_back( tArc );

            m_commit->Modify( track1 );
            m_commit->Modify( track2 );

            if( filletOp.t1Start )
                track1->SetStart( t1newPoint );
            else
                track1->SetEnd( t1newPoint );

            if( filletOp.t2Start )
                track2->SetStart( t2newPoint );
            else
                track2->SetEnd( t2newPoint );

            operationPerformedOnAtLeastOne = true;
        }
    }

    m_commit->Push( _( "Fillet Tracks" ) );

    //select the newly created arcs
    for( BOARD_ITEM* item : itemsToAddToSelection )
        m_selectionTool->AddItemToSel( item );

    if( !operationPerformedOnAtLeastOne )
        frame()->ShowInfoBarMsg( _( "Unable to fillet the selected track segments." ) );
    else if( didOneAttemptFail )
        frame()->ShowInfoBarMsg( _( "Some of the track segments could not be filleted." ) );

    return 0;
}


int EDIT_TOOL::FilletLines( const TOOL_EVENT& aEvent )
{
    // Store last used fillet radius to allow pressing "enter" if repeat fillet is required
    static long long filletRadiusIU = 0;

    PCB_SELECTION& selection = m_selectionTool->RequestSelection(
        []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
        {

            std::vector<VECTOR2I> pts;

            // Iterate from the back so we don't have to worry about removals.
            for( int i = aCollector.GetCount() - 1; i >= 0; --i )
            {
                BOARD_ITEM* item = aCollector[i];

                // We've converted the polygon and rectangle to segments, so drop everything
                // that isn't a segment at this point
                if( !item->IsType( { PCB_SHAPE_LOCATE_SEGMENT_T,
                                     PCB_SHAPE_LOCATE_POLY_T,
                                     PCB_SHAPE_LOCATE_RECT_T } ) )
                {
                    aCollector.Remove( item );
                }
            }
        },
        true /* prompt user regarding locked items */ );

    std::set<PCB_SHAPE*> lines_to_add;
    std::vector<PCB_SHAPE*> items_to_remove;

    for( EDA_ITEM* item : selection )
    {
        std::vector<VECTOR2I> pts;
        PCB_SHAPE *graphic = static_cast<PCB_SHAPE*>( item );
        PCB_LAYER_ID layer = graphic->GetLayer();
        int width = graphic->GetWidth();

        if( graphic->GetShape() == SHAPE_T::RECT )
        {
            items_to_remove.push_back( graphic );
            VECTOR2I start( graphic->GetStart() );
            VECTOR2I end( graphic->GetEnd() );
            pts.emplace_back( start );
            pts.emplace_back( VECTOR2I( end.x, start.y ) );
            pts.emplace_back( end );
            pts.emplace_back( VECTOR2I( start.x, end.y ) );
        }

        if( graphic->GetShape() == SHAPE_T::POLY )
        {
            items_to_remove.push_back( graphic );

            for( int jj = 0; jj < graphic->GetPolyShape().VertexCount(); ++jj )
                pts.emplace_back( graphic->GetPolyShape().CVertex( jj ) );
        }

        for( size_t jj = 1; jj < pts.size(); ++jj )
        {
            PCB_SHAPE *line = new PCB_SHAPE( frame()->GetModel(), SHAPE_T::SEGMENT );

            line->SetStart( pts[jj - 1] );
            line->SetEnd( pts[jj] );
            line->SetWidth( width );
            line->SetLayer( layer );
            lines_to_add.insert( line );
        }

        if( pts.size() > 1 )
        {
            PCB_SHAPE *line = new PCB_SHAPE( frame()->GetModel(), SHAPE_T::SEGMENT );

            line->SetStart( pts.back() );
            line->SetEnd( pts.front() );
            line->SetWidth( width );
            line->SetLayer( layer );
            lines_to_add.insert( line );
        }
    }

    for( PCB_SHAPE* item : lines_to_add )
        selection.Add( item );

    for( PCB_SHAPE* item : items_to_remove )
        selection.Remove( item );

    if( selection.CountType( PCB_SHAPE_LOCATE_SEGMENT_T ) < 2 )
    {
        frame()->ShowInfoBarMsg( _( "A shape with least two lines must be selected." ) );
        return 0;
    }

    WX_UNIT_ENTRY_DIALOG dia( frame(), _( "Enter fillet radius:" ), _( "Fillet Lines" ),
                              filletRadiusIU );

    if( dia.ShowModal() == wxID_CANCEL )
        return 0;

    filletRadiusIU = dia.GetValue();

    if( filletRadiusIU == 0 )
    {
        frame()->ShowInfoBarMsg( _( "A radius of zero was entered.\n"
                                    "The fillet operation was not performed." ) );
        return 0;
    }

    bool                   operationPerformedOnAtLeastOne = false;
    bool                   didOneAttemptFail              = false;
    std::vector<BOARD_ITEM*> itemsToAddToSelection;

    // Only modify one parent in FP editor
    if( m_isFootprintEditor )
        m_commit->Modify( selection.Front() );

    alg::for_all_pairs( selection.begin(), selection.end(), [&]( EDA_ITEM* a, EDA_ITEM* b )
        {
            PCB_SHAPE* line_a = static_cast<PCB_SHAPE*>( a );
            PCB_SHAPE* line_b = static_cast<PCB_SHAPE*>( b );

            if( line_a->GetLength() == 0.0 || line_b->GetLength() == 0 )
                return;

            SEG seg_a( line_a->GetStart(), line_a->GetEnd() );
            SEG seg_b( line_b->GetStart(), line_b->GetEnd() );
            VECTOR2I* a_pt;
            VECTOR2I* b_pt;

            if (seg_a.A == seg_b.A)
            {
                a_pt = &seg_a.A;
                b_pt = &seg_b.A;
            }
            else if (seg_a.A == seg_b.B)
            {
                a_pt = &seg_a.A;
                b_pt = &seg_b.B;
            }
            else if (seg_a.B == seg_b.A)
            {
                a_pt = &seg_a.B;
                b_pt = &seg_b.A;
            }
            else if (seg_a.B == seg_b.B)
            {
                a_pt = &seg_a.B;
                b_pt = &seg_b.B;
            }
            else
                return;


            SHAPE_ARC sArc( seg_a, seg_b, filletRadiusIU );
            VECTOR2I  t1newPoint, t2newPoint;

            auto setIfPointOnSeg =
                    []( VECTOR2I& aPointToSet, SEG aSegment, VECTOR2I aVecToTest )
                    {
                        VECTOR2I segToVec = aSegment.NearestPoint( aVecToTest ) - aVecToTest;

                        // Find out if we are on the segment (minimum precision)
                        if( segToVec.EuclideanNorm() < SHAPE_ARC::MIN_PRECISION_IU )
                        {
                            aPointToSet.x = aVecToTest.x;
                            aPointToSet.y = aVecToTest.y;
                            return true;
                        }

                        return false;
                    };

            //Do not draw a fillet if the end points of the arc are not within the track segments
            if( !setIfPointOnSeg( t1newPoint, seg_a, sArc.GetP0() )
                    && !setIfPointOnSeg( t2newPoint, seg_b, sArc.GetP0() ) )
            {
                didOneAttemptFail = true;
                return;
            }

            if( !setIfPointOnSeg( t1newPoint, seg_a, sArc.GetP1() )
                    && !setIfPointOnSeg( t2newPoint, seg_b, sArc.GetP1() ) )
            {
                didOneAttemptFail = true;
                return;
            }

            PCB_SHAPE* tArc = new PCB_SHAPE( frame()->GetBoard(), SHAPE_T::ARC );

            tArc->SetArcGeometry( sArc.GetP0(), sArc.GetArcMid(), sArc.GetP1() );
            tArc->SetWidth( line_a->GetWidth() );
            tArc->SetLayer( line_a->GetLayer() );
            tArc->SetLocked( line_a->IsLocked() );

            if( lines_to_add.count( line_a ) )
            {
                lines_to_add.erase( line_a );
                itemsToAddToSelection.push_back( line_a );
            }
            else if( !m_isFootprintEditor )
            {
                m_commit->Modify( line_a );
            }

            if( lines_to_add.count( line_b ) )
            {
                lines_to_add.erase( line_b );
                itemsToAddToSelection.push_back( line_b );
            }
            else if( !m_isFootprintEditor )
            {
                m_commit->Modify( line_b );
            }

            itemsToAddToSelection.push_back( tArc );
            *a_pt = t1newPoint;
            *b_pt = t2newPoint;
            line_a->SetStart( seg_a.A );
            line_a->SetEnd( seg_a.B );
            line_b->SetStart( seg_b.A );
            line_b->SetEnd( seg_b.B );

            operationPerformedOnAtLeastOne = true;

        } );

    for( auto item : items_to_remove )
    {
        m_commit->Remove( item );
        m_selectionTool->RemoveItemFromSel( item, true );
    }

    //select the newly created arcs
    for( BOARD_ITEM* item : itemsToAddToSelection )
    {
        m_commit->Add( item );
        m_selectionTool->AddItemToSel( item, true );
    }

    if( !items_to_remove.empty() )
        m_toolMgr->ProcessEvent( EVENTS::UnselectedEvent );

    if( !itemsToAddToSelection.empty() )
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

    // Notify other tools of the changes
    m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );

    m_commit->Push( _( "Fillet Lines" ) );

    if( !operationPerformedOnAtLeastOne )
        frame()->ShowInfoBarMsg( _( "Unable to fillet the selected lines." ) );
    else if( didOneAttemptFail )
        frame()->ShowInfoBarMsg( _( "Some of the lines could not be filleted." ) );

    return 0;
}


int EDIT_TOOL::Properties( const TOOL_EVENT& aEvent )
{
    PCB_BASE_EDIT_FRAME* editFrame = getEditFrame<PCB_BASE_EDIT_FRAME>();
    const PCB_SELECTION& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
            } );

    // Tracks & vias are treated in a special way:
    if( ( SELECTION_CONDITIONS::OnlyTypes( { PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T } ) )( selection ) )
    {
        DIALOG_TRACK_VIA_PROPERTIES dlg( editFrame, selection, *m_commit );
        dlg.ShowQuasiModal();       // QuasiModal required for NET_SELECTOR
    }
    else if( selection.Size() == 1 )
    {
        // Display properties dialog
        BOARD_ITEM* item = static_cast<BOARD_ITEM*>( selection.Front() );

        // Do not handle undo buffer, it is done by the properties dialogs
        editFrame->OnEditItemRequest( item );

        // Notify other tools of the changes
        m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );
    }
    else if( selection.Size() == 0 && getView()->IsLayerVisible( LAYER_DRAWINGSHEET ) )
    {
        DS_PROXY_VIEW_ITEM* ds = editFrame->GetCanvas()->GetDrawingSheet();
        VECTOR2D            cursorPos = getViewControls()->GetCursorPosition( false );

        if( ds && ds->HitTestDrawingSheetItems( getView(), cursorPos ) )
            m_toolMgr->RunAction( ACTIONS::pageSettings );
        else
            m_toolMgr->RunAction( PCB_ACTIONS::footprintProperties, true );
    }

    if( selection.IsHover() )
    {
        m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
    }
    else
    {
        // Check for items becoming invisible and drop them from the selection.

        PCB_SELECTION selCopy = selection;
        LSET          visible = editFrame->GetBoard()->GetVisibleLayers();

        for( EDA_ITEM* eda_item : selCopy )
        {
            BOARD_ITEM* item = static_cast<BOARD_ITEM*>( eda_item );

            if( !( item->GetLayerSet() & visible ).any() )
                m_selectionTool->RemoveItemFromSel( item );
        }
    }

    return 0;
}


int EDIT_TOOL::Rotate( const TOOL_EVENT& aEvent )
{
    if( isRouterActive() )
    {
        wxBell();
        return 0;
    }

    PCB_BASE_EDIT_FRAME* editFrame = getEditFrame<PCB_BASE_EDIT_FRAME>();

    // Be sure that there is at least one item that we can modify. If nothing was selected before,
    // try looking for the stuff under mouse cursor (i.e. KiCad old-style hover selection)
    PCB_SELECTION& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
                sTool->FilterCollectorForHierarchy( aCollector, true );
                sTool->FilterCollectorForMarkers( aCollector );
            },
            // Prompt user regarding locked items if in board editor and in free-pad-mode (if
            // we're not in free-pad mode we delay this until the second RequestSelection()).
            frame()->GetPcbNewSettings()->m_AllowFreePads && !m_isFootprintEditor );

    if( selection.Empty() )
        return 0;

    std::optional<VECTOR2I> oldRefPt;
    bool          is_hover = selection.IsHover();   // N.B. This must be saved before the second
                                                    // call to RequestSelection() below

    if( selection.HasReferencePoint() )
        oldRefPt = selection.GetReferencePoint();

    // Now filter out pads if not in free pads mode.  We cannot do this in the first
    // RequestSelection() as we need the reference point when a pad is the selection front.
    if( !m_isFootprintEditor && !frame()->GetPcbNewSettings()->m_AllowFreePads )
    {
        selection = m_selectionTool->RequestSelection(
                []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
                {
                    sTool->FilterCollectorForMarkers( aCollector );
                    sTool->FilterCollectorForHierarchy( aCollector, true );
                    sTool->FilterCollectorForFreePads( aCollector );
                },
                true /* prompt user regarding locked items */ );
    }

    // Did we filter everything out?  If so, don't try to operate further
    if( selection.Empty() )
        return 0;

    updateModificationPoint( selection );

    VECTOR2I  refPt = selection.GetReferencePoint();
    EDA_ANGLE rotateAngle = TOOL_EVT_UTILS::GetEventRotationAngle( *editFrame, aEvent );

    // Calculate view bounding box
    BOX2I viewBBox = selection.Front()->ViewBBox();

    for( EDA_ITEM* item : selection )
        viewBBox.Merge( item->ViewBBox() );

    // Check if the view bounding box will go out of bounds
    VECTOR2D rotPos = viewBBox.GetPosition();
    VECTOR2D rotEnd = viewBBox.GetEnd();

    RotatePoint( &rotPos.x, &rotPos.y, refPt.x, refPt.y, rotateAngle );
    RotatePoint( &rotEnd.x, &rotEnd.y, refPt.x, refPt.y, rotateAngle );

    typedef std::numeric_limits<int> coord_limits;

    int max = coord_limits::max() - COORDS_PADDING;
    int min = -max;

    bool outOfBounds = rotPos.x < min || rotPos.x > max || rotPos.y < min || rotPos.y > max
                       || rotEnd.x < min || rotEnd.x > max || rotEnd.y < min || rotEnd.y > max;

    if( !outOfBounds )
    {
        // When editing footprints, all items have the same parent
        if( IsFootprintEditor() )
            m_commit->Modify( selection.Front() );

        for( EDA_ITEM* item : selection )
        {
            if( !item->IsNew() && !IsFootprintEditor() )
            {
                m_commit->Modify( item );

                // If rotating a group, record position of all the descendants for undo
                if( item->Type() == PCB_GROUP_T )
                {
                    static_cast<PCB_GROUP*>( item )->RunOnDescendants( [&]( BOARD_ITEM* bItem )
                                                                    {
                                                                        m_commit->Modify( bItem );
                                                                    });
                }
            }

            static_cast<BOARD_ITEM*>( item )->Rotate( refPt, rotateAngle );
        }

        if( !m_dragging )
            m_commit->Push( _( "Rotate" ) );

        if( is_hover && !m_dragging )
            m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

        m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );

        if( m_dragging )
            m_toolMgr->RunAction( PCB_ACTIONS::updateLocalRatsnest, false );
    }

    // Restore the old reference so any mouse dragging that occurs doesn't make the selection jump
    // to this now invalid reference
    if( oldRefPt )
        selection.SetReferencePoint( *oldRefPt );
    else
        selection.ClearReferencePoint();

    return 0;
}


/**
 * Mirror a point about the vertical axis passing through another point.
 */
static VECTOR2I mirrorPointX( const VECTOR2I& aPoint, const VECTOR2I& aMirrorPoint )
{
    VECTOR2I mirrored = aPoint;

    mirrored.x -= aMirrorPoint.x;
    mirrored.x = -mirrored.x;
    mirrored.x += aMirrorPoint.x;

    return mirrored;
}


/**
 * Mirror a point about the vertical axis passing through another point.
 */
static VECTOR2I mirrorPointY( const VECTOR2I& aPoint, const VECTOR2I& aMirrorPoint )
{
    VECTOR2I mirrored = aPoint;

    mirrored.y -= aMirrorPoint.y;
    mirrored.y = -mirrored.y;
    mirrored.y += aMirrorPoint.y;

    return mirrored;
}


/**
 * Mirror a pad in the vertical axis passing through a point (mirror left to right).
 */
static void mirrorPadX( PAD& aPad, const VECTOR2I& aMirrorPoint )
{
    if( aPad.GetShape() == PAD_SHAPE::CUSTOM )
        aPad.FlipPrimitives( true );  // mirror primitives left to right

    VECTOR2I tmpPt = mirrorPointX( aPad.GetPosition(), aMirrorPoint );
    aPad.SetPosition( tmpPt );

    tmpPt = aPad.GetOffset();
    tmpPt.x = -tmpPt.x;
    aPad.SetOffset( tmpPt );

    auto tmpz = aPad.GetDelta();
    tmpz.x = -tmpz.x;
    aPad.SetDelta( tmpz );

    aPad.SetOrientation( -aPad.GetOrientation() );
}


/**
 * Mirror a pad in the vertical axis passing through a point (mirror left to right).
 */
static void mirrorPadY( PAD& aPad, const VECTOR2I& aMirrorPoint )
{
    if( aPad.GetShape() == PAD_SHAPE::CUSTOM )
        aPad.FlipPrimitives( false );  // mirror primitives top to bottom

    VECTOR2I tmpPt = mirrorPointY( aPad.GetPosition(), aMirrorPoint );
    aPad.SetPosition( tmpPt );

    tmpPt = aPad.GetOffset();
    tmpPt.y = -tmpPt.y;
    aPad.SetOffset( tmpPt );

    auto tmpz = aPad.GetDelta();
    tmpz.y = -tmpz.y;
    aPad.SetDelta( tmpz );

    aPad.SetOrientation( -aPad.GetOrientation() );
}


const std::vector<KICAD_T> EDIT_TOOL::MirrorableItems = {
        PCB_SHAPE_T,
        PCB_TEXT_T,
        PCB_TEXTBOX_T,
        PCB_ZONE_T,
        PCB_PAD_T,
        PCB_TRACE_T,
        PCB_ARC_T,
        PCB_VIA_T,
};

int EDIT_TOOL::Mirror( const TOOL_EVENT& aEvent )
{
    if( isRouterActive() )
    {
        wxBell();
        return 0;
    }

    PCB_SELECTION& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
                sTool->FilterCollectorForMarkers( aCollector );
                sTool->FilterCollectorForHierarchy( aCollector, true );
                sTool->FilterCollectorForFreePads( aCollector );
            },
            !m_dragging /* prompt user regarding locked items */ );

    if( selection.Empty() )
        return 0;

    updateModificationPoint( selection );
    VECTOR2I mirrorPoint = selection.GetReferencePoint();

    // When editing footprints, all items have the same parent
    if( IsFootprintEditor() )
        m_commit->Modify( selection.Front() );

    // Set the mirroring options.
    // Unfortunately, the mirror function do not have the same parameter for all items
    // So we need these 2 parameters to avoid mistakes
    bool mirrorLeftRight = true;
    bool mirrorAroundXaxis = false;

    if( aEvent.IsAction( &PCB_ACTIONS::mirrorV ) )
    {
        mirrorLeftRight = false;
        mirrorAroundXaxis = true;
    }

    for( EDA_ITEM* item : selection )
    {
        if( !item->IsType( MirrorableItems ) )
            continue;

        if( !item->IsNew() && !IsFootprintEditor() )
            m_commit->Modify( item );

        // modify each object as necessary
        switch( item->Type() )
        {
        case PCB_SHAPE_T:
            static_cast<PCB_SHAPE*>( item )->Mirror( mirrorPoint, mirrorAroundXaxis );
            break;

        case PCB_ZONE_T:
            static_cast<ZONE*>( item )->Mirror( mirrorPoint, mirrorLeftRight );
            break;

        case PCB_TEXT_T:
            static_cast<PCB_TEXT*>( item )->Mirror( mirrorPoint, mirrorAroundXaxis );
            break;

        case PCB_TEXTBOX_T:
            static_cast<PCB_TEXTBOX*>( item )->Mirror( mirrorPoint, mirrorAroundXaxis );
            break;

        case PCB_PAD_T:
            if( mirrorLeftRight )
                mirrorPadX( *static_cast<PAD*>( item ), mirrorPoint );
            else
                mirrorPadY( *static_cast<PAD*>( item ), mirrorPoint );

            break;

        case PCB_TRACE_T:
        case PCB_ARC_T:
        case PCB_VIA_T:
            static_cast<PCB_TRACK*>( item )->Mirror( mirrorPoint, mirrorAroundXaxis );
            break;

        default:
            // it's likely the commit object is wrong if you get here
            // Unsure if PCB_GROUP_T needs special attention here.
            assert( false );
            break;
        }
    }

    if( !m_dragging )
        m_commit->Push( _( "Mirror" ) );

    if( selection.IsHover() && !m_dragging )
        m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );

    if( m_dragging )
        m_toolMgr->RunAction( PCB_ACTIONS::updateLocalRatsnest, false );

    return 0;
}


int EDIT_TOOL::Flip( const TOOL_EVENT& aEvent )
{
    if( isRouterActive() )
    {
        wxBell();
        return 0;
    }

    PCB_SELECTION& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
                sTool->FilterCollectorForMarkers( aCollector );
                sTool->FilterCollectorForHierarchy( aCollector, true );
                sTool->FilterCollectorForFreePads( aCollector );
            },
            !m_dragging /* prompt user regarding locked items */ );

    if( selection.Empty() )
        return 0;

    std::optional<VECTOR2I> oldRefPt;

    if( selection.HasReferencePoint() )
        oldRefPt = selection.GetReferencePoint();

    updateModificationPoint( selection );

    // Flip around the anchor for footprints, and the bounding box center for board items
    VECTOR2I refPt = IsFootprintEditor() ? VECTOR2I( 0, 0 ) : selection.GetCenter();

    // If only one item selected, flip around the selection or item anchor point (instead
    // of the bounding box center) to avoid moving the item anchor
    if( selection.GetSize() == 1 )
    {
        if( m_dragging && selection.HasReferencePoint() )
            refPt = selection.GetReferencePoint();
        else
            refPt = static_cast<BOARD_ITEM*>( selection.GetItem( 0 ) )->GetPosition();
    }

    bool leftRight = frame()->GetPcbNewSettings()->m_FlipLeftRight;

    // When editing footprints, all items have the same parent
    if( IsFootprintEditor() )
        m_commit->Modify( selection.Front() );

    for( EDA_ITEM* item : selection )
    {
        if( !item->IsNew() && !IsFootprintEditor() )
            m_commit->Modify( item );

        if( item->Type() == PCB_GROUP_T )
        {
            static_cast<PCB_GROUP*>( item )->RunOnDescendants( [&]( BOARD_ITEM* bItem )
                                                               {
                                                                   m_commit->Modify( bItem );
                                                               });
        }

        static_cast<BOARD_ITEM*>( item )->Flip( refPt, leftRight );
    }

    if( !m_dragging )
        m_commit->Push( _( "Change Side / Flip" ) );

    if( selection.IsHover() && !m_dragging )
        m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );

    if( m_dragging )
        m_toolMgr->RunAction( PCB_ACTIONS::updateLocalRatsnest, false );

    // Restore the old reference so any mouse dragging that occurs doesn't make the selection jump
    // to this now invalid reference
    if( oldRefPt )
        selection.SetReferencePoint( *oldRefPt );
    else
        selection.ClearReferencePoint();

    return 0;
}


void EDIT_TOOL::DeleteItems( const PCB_SELECTION& aItems, bool aIsCut )
{
    // As we are about to remove items, they have to be removed from the selection first
    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    for( EDA_ITEM* item : aItems )
    {
        BOARD_ITEM* board_item = static_cast<BOARD_ITEM*>( item );
        FOOTPRINT*  parentFP = board_item->GetParentFootprint();
        PCB_GROUP*  parentGroup = board_item->GetParentGroup();

        if( parentGroup )
        {
            m_commit->Modify( parentGroup );
            parentGroup->RemoveItem( board_item );
        }

        switch( item->Type() )
        {
        case PCB_TEXT_T:
            switch( static_cast<PCB_TEXT*>( board_item )->GetType() )
            {
            case PCB_TEXT::TEXT_is_VALUE:
            case PCB_TEXT::TEXT_is_REFERENCE:
                wxASSERT( parentFP );
                m_commit->Modify( parentFP );
                static_cast<PCB_TEXT*>( board_item )->SetVisible( false );
                getView()->Update( board_item );
                break;

            case PCB_TEXT::TEXT_is_DIVERS:
                if( IsFootprintEditor() )
                {
                    m_commit->Modify( parentFP );
                    getView()->Remove( board_item );
                    parentFP->Remove( board_item );
                }
                else
                {
                    m_commit->Remove( board_item );
                }
                break;

            default:
                wxFAIL; // Shouldn't get here
                break;
            }

            break;

        case PCB_SHAPE_T:
        case PCB_TEXTBOX_T:
        case PCB_BITMAP_T:
            if( IsFootprintEditor() )
            {
                m_commit->Modify( parentFP );
                getView()->Remove( board_item );
                parentFP->Remove( board_item );
            }
            else
            {
                m_commit->Remove( board_item );
            }

            break;

        case PCB_PAD_T:
            if( IsFootprintEditor() || frame()->GetPcbNewSettings()->m_AllowFreePads )
            {
                m_commit->Modify( parentFP );
                getView()->Remove( board_item );
                parentFP->Remove( board_item );
            }

            break;

        case PCB_ZONE_T:
            if( IsFootprintEditor() )
            {
                m_commit->Modify( parentFP );
                getView()->Remove( board_item );
                parentFP->Remove( board_item );
            }
            else
            {
                // We process the zones special so that cutouts can be deleted when the delete
                // tool is called from inside a cutout when the zone is selected.
                // Only interact with cutouts when deleting and a single item is selected
                if( !aIsCut && aItems.GetSize() == 1 )
                {
                    VECTOR2I curPos = getViewControls()->GetCursorPosition();
                    ZONE*    zone   = static_cast<ZONE*>( board_item );

                    int outlineIdx, holeIdx;

                    if( zone->HitTestCutout( curPos, &outlineIdx, &holeIdx ) )
                    {
                        // Remove the cutout
                        m_commit->Modify( zone );
                        zone->RemoveCutout( outlineIdx, holeIdx );
                        zone->UnFill();

                        // TODO Refill zone when KiCad supports auto re-fill

                        // Update the display
                        zone->HatchBorder();
                        canvas()->Refresh();

                        // Restore the selection on the original zone
                        m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, zone );

                        break;
                    }
                }

                // Remove the entire zone otherwise
                m_commit->Remove( board_item );
            }

            break;

        case PCB_GROUP_T:
        {
            PCB_GROUP* group = static_cast<PCB_GROUP*>( board_item );

            auto removeItem =
                    [&]( BOARD_ITEM* bItem )
                    {
                        if( bItem->GetParent() && bItem->GetParent()->Type() == PCB_FOOTPRINT_T )
                        {
                            // Silently ignore delete of Reference or Value if they happen to be
                            // in group.
                            if( bItem->Type() == PCB_TEXT_T )
                            {
                                PCB_TEXT* textItem = static_cast<PCB_TEXT*>( bItem );

                                if( textItem->GetType() != PCB_TEXT::TEXT_is_DIVERS )
                                    return;
                            }
                            else if( bItem->Type() == PCB_PAD_T )
                            {
                                if( !IsFootprintEditor()
                                        && !frame()->GetPcbNewSettings()->m_AllowFreePads )
                                {
                                    return;
                                }
                            }

                            m_commit->Modify( bItem->GetParent() );
                            getView()->Remove( bItem );
                            bItem->GetParent()->Remove( bItem );
                        }
                        else
                        {
                            m_commit->Remove( bItem );
                        }
                    };

            removeItem( group );

            group->RunOnDescendants( [&]( BOARD_ITEM* aDescendant )
                                     {
                                         removeItem( aDescendant );
                                     });
            break;
        }

        default:
            m_commit->Remove( board_item );
            break;
        }
    }

    // If the entered group has been emptied then leave it.
    PCB_GROUP* enteredGroup = m_selectionTool->GetEnteredGroup();

    if( enteredGroup && enteredGroup->GetItems().empty() )
        m_selectionTool->ExitGroup();

    if( aIsCut )
        m_commit->Push( _( "Cut" ) );
    else
        m_commit->Push( _( "Delete" ) );
}


int EDIT_TOOL::Remove( const TOOL_EVENT& aEvent )
{
    PCB_BASE_EDIT_FRAME*  editFrame = getEditFrame<PCB_BASE_EDIT_FRAME>();

    if( isRouterActive() )
    {
        m_toolMgr->RunAction( PCB_ACTIONS::routerUndoLastSegment, true );
        return 0;
    }

    editFrame->PushTool( aEvent );

    std::vector<BOARD_ITEM*> lockedItems;
    Activate();

    // get a copy instead of reference (as we're going to clear the selection before removing items)
    PCB_SELECTION selectionCopy;
    bool isCut = aEvent.Parameter<PCB_ACTIONS::REMOVE_FLAGS>() == PCB_ACTIONS::REMOVE_FLAGS::CUT;
    bool isAlt = aEvent.Parameter<PCB_ACTIONS::REMOVE_FLAGS>() == PCB_ACTIONS::REMOVE_FLAGS::ALT;

    // If we are in a "Cut" operation, then the copied selection exists already and we want to
    // delete exactly that; no more, no fewer.  Any filtering for locked items must be done in
    // the copyToClipboard() routine.
    if( isCut )
    {
        selectionCopy = m_selectionTool->GetSelection();
    }
    else
    {
        // When not in free-pad mode we normally auto-promote selected pads to their parent
        // footprints.  But this is probably a little too dangerous for a destructive operation,
        // so we just do the promotion but not the deletion (allowing for a second delete to do
        // it if that's what the user wanted).
        selectionCopy = m_selectionTool->RequestSelection(
                []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
                {
                } );

        size_t beforeFPCount = selectionCopy.CountType( PCB_FOOTPRINT_T );

        m_selectionTool->RequestSelection(
                []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
                {
                    sTool->FilterCollectorForFreePads( aCollector );
                } );

        if( !selectionCopy.IsHover()
                && m_selectionTool->GetSelection().CountType( PCB_FOOTPRINT_T ) > beforeFPCount )
        {
            wxBell();
            canvas()->Refresh();
            editFrame->PopTool( aEvent );
            return 0;
        }

        // In "alternative" mode, we expand selected track items to their full connection.
        if( isAlt && ( selectionCopy.HasType( PCB_TRACE_T ) || selectionCopy.HasType( PCB_VIA_T ) ) )
        {
            m_toolMgr->RunAction( PCB_ACTIONS::selectConnection, true );
        }

        // Finally run RequestSelection() one more time to find out what user wants to do about
        // locked objects.
        selectionCopy = m_selectionTool->RequestSelection(
                []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
                {
                    sTool->FilterCollectorForFreePads( aCollector );
                },
                true /* prompt user regarding locked items */ );
    }

    DeleteItems( selectionCopy, isCut );

    editFrame->PopTool( aEvent );
    return 0;
}


int EDIT_TOOL::MoveExact( const TOOL_EVENT& aEvent )
{
    if( isRouterActive() )
    {
        wxBell();
        return 0;
    }

    const PCB_SELECTION& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
                sTool->FilterCollectorForMarkers( aCollector );
                sTool->FilterCollectorForHierarchy( aCollector, true );
            },
            true /* prompt user regarding locked items */ );

    if( selection.Empty() )
        return 0;

    VECTOR2I        translation;
    EDA_ANGLE       rotation;
    ROTATION_ANCHOR rotationAnchor = selection.Size() > 1 ? ROTATE_AROUND_SEL_CENTER
                                                          : ROTATE_AROUND_ITEM_ANCHOR;

    // TODO: Implement a visible bounding border at the edge
    BOX2I sel_box = selection.GetBoundingBox();

    DIALOG_MOVE_EXACT dialog( frame(), translation, rotation, rotationAnchor, sel_box );
    int ret = dialog.ShowModal();

    if( ret == wxID_OK )
    {
        EDA_ANGLE angle = rotation;
        VECTOR2I  rp = selection.GetCenter();
        VECTOR2I  selCenter( rp.x, rp.y );

        // Make sure the rotation is from the right reference point
        selCenter += translation;

        if( !frame()->GetPcbNewSettings()->m_Display.m_DisplayInvertYAxis )
            rotation = -rotation;

        // When editing footprints, all items have the same parent
        if( IsFootprintEditor() )
            m_commit->Modify( selection.Front() );

        for( EDA_ITEM* selItem : selection )
        {
            BOARD_ITEM* item = static_cast<BOARD_ITEM*>( selItem );

            if( !item->IsNew() && !IsFootprintEditor() )
            {
                m_commit->Modify( item );

                if( item->Type() == PCB_GROUP_T )
                {
                    PCB_GROUP* group = static_cast<PCB_GROUP*>( item );

                    group->RunOnDescendants( [&]( BOARD_ITEM* bItem )
                                             {
                                                 m_commit->Modify( bItem );
                                             });
                }
            }

            if( !item->GetParent() || !item->GetParent()->IsSelected() )
                item->Move( translation );

            switch( rotationAnchor )
            {
            case ROTATE_AROUND_ITEM_ANCHOR:
                item->Rotate( item->GetPosition(), angle );
                break;
            case ROTATE_AROUND_SEL_CENTER:
                item->Rotate( selCenter, angle );
                break;
            case ROTATE_AROUND_USER_ORIGIN:
                item->Rotate( frame()->GetScreen()->m_LocalOrigin, angle );
                break;
            case ROTATE_AROUND_AUX_ORIGIN:
                item->Rotate( board()->GetDesignSettings().GetAuxOrigin(), angle );
                break;
            }

            if( !m_dragging )
                getView()->Update( item );
        }

        m_commit->Push( _( "Move exact" ) );

        if( selection.IsHover() )
            m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

        m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );

        if( m_dragging )
            m_toolMgr->RunAction( PCB_ACTIONS::updateLocalRatsnest, false );
    }

    return 0;
}


int EDIT_TOOL::Duplicate( const TOOL_EVENT& aEvent )
{
    if( isRouterActive() )
    {
        wxBell();
        return 0;
    }

    bool increment = aEvent.IsAction( &PCB_ACTIONS::duplicateIncrement );

    // Be sure that there is at least one item that we can modify
    const PCB_SELECTION& selection = m_selectionTool->RequestSelection(
                []( const VECTOR2I&, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
                {
                    sTool->FilterCollectorForMarkers( aCollector );
                    sTool->FilterCollectorForHierarchy( aCollector, true );
                } );

    if( selection.Empty() )
        return 0;

    // we have a selection to work on now, so start the tool process
    PCB_BASE_EDIT_FRAME* editFrame = getEditFrame<PCB_BASE_EDIT_FRAME>();

    // If the selection was given a hover, we do not keep the selection after completion
    bool is_hover = selection.IsHover();

    std::vector<BOARD_ITEM*> new_items;
    new_items.reserve( selection.Size() );

    // Each selected item is duplicated and pushed to new_items list
    // Old selection is cleared, and new items are then selected.
    for( EDA_ITEM* item : selection )
    {
        BOARD_ITEM* dupe_item = nullptr;
        BOARD_ITEM* orig_item = static_cast<BOARD_ITEM*>( item );

        if( m_isFootprintEditor )
        {
            FOOTPRINT* parentFootprint = editFrame->GetBoard()->GetFirstFootprint();
            dupe_item = parentFootprint->DuplicateItem( orig_item );

            if( increment && dupe_item->Type() == PCB_PAD_T
                && static_cast<PAD*>( dupe_item )->CanHaveNumber() )
            {
                PAD_TOOL* padTool = m_toolMgr->GetTool<PAD_TOOL>();
                wxString padNumber = padTool->GetLastPadNumber();
                padNumber = parentFootprint->GetNextPadNumber( padNumber );
                padTool->SetLastPadNumber( padNumber );
                static_cast<PAD*>( dupe_item )->SetNumber( padNumber );
            }
        }
        else if( FOOTPRINT* parentFootprint = orig_item->GetParentFootprint() )
        {
            m_commit->Modify( parentFootprint );
            dupe_item = parentFootprint->DuplicateItem( orig_item, true /* add to parent */ );
        }
        else
        {
            switch( orig_item->Type() )
            {
            case PCB_FOOTPRINT_T:
            case PCB_TEXT_T:
            case PCB_TEXTBOX_T:
            case PCB_BITMAP_T:
            case PCB_SHAPE_T:
            case PCB_TRACE_T:
            case PCB_ARC_T:
            case PCB_VIA_T:
            case PCB_ZONE_T:
            case PCB_TARGET_T:
            case PCB_DIM_ALIGNED_T:
            case PCB_DIM_CENTER_T:
            case PCB_DIM_RADIAL_T:
            case PCB_DIM_ORTHOGONAL_T:
            case PCB_DIM_LEADER_T:
                dupe_item = orig_item->Duplicate();
                break;

            case PCB_GROUP_T:
                dupe_item = static_cast<PCB_GROUP*>( orig_item )->DeepDuplicate();
                break;

            default:
                wxASSERT_MSG( false, wxString::Format( wxT( "Unhandled item type %d" ),
                                                       orig_item->Type() ) );
                break;
            }
        }

        if( dupe_item )
        {
            if( dupe_item->Type() == PCB_GROUP_T )
            {
                static_cast<PCB_GROUP*>( dupe_item )->RunOnDescendants(
                        [&]( BOARD_ITEM* bItem )
                        {
                            m_commit->Add( bItem );
                        });
            }

            // Clear the selection flag here, otherwise the PCB_SELECTION_TOOL
            // will not properly select it later on
            dupe_item->ClearSelected();

            new_items.push_back( dupe_item );
            m_commit->Add( dupe_item );
        }
    }

    // Clear the old selection first
    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    // Select the new items
    m_toolMgr->RunAction( PCB_ACTIONS::selectItems, true, &new_items );

    // record the new items as added
    if( !selection.Empty() )
    {
        editFrame->DisplayToolMsg( wxString::Format( _( "Duplicated %d item(s)" ),
                                                     (int) new_items.size() ) );

        // TODO(ISM): This line can't be used to activate the tool until we allow multiple
        //            activations.
        // m_toolMgr->RunAction( PCB_ACTIONS::move, true );
        // Instead we have to create the event and call the tool's function
        // directly

        // If items were duplicated, pick them up
        // this works well for "dropping" copies around and pushes the commit
        TOOL_EVENT evt = PCB_ACTIONS::move.MakeEvent();
        Move( evt );

        // Deslect the duplicated item if we originally started as a hover selection
        if( is_hover )
            m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
    }

    return 0;
}


int EDIT_TOOL::CreateArray( const TOOL_EVENT& aEvent )
{
    if( isRouterActive() )
    {
        wxBell();
        return 0;
    }

    // Be sure that there is at least one item that we can modify
    const PCB_SELECTION& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I&, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
                sTool->FilterCollectorForMarkers( aCollector );
                sTool->FilterCollectorForHierarchy( aCollector, true );
            } );

    if( selection.Empty() )
        return 0;

    // we have a selection to work on now, so start the tool process
    PCB_BASE_FRAME* editFrame = getEditFrame<PCB_BASE_FRAME>();
    ARRAY_CREATOR   array_creator( *editFrame, m_isFootprintEditor, selection, m_toolMgr );
    array_creator.Invoke();

    return 0;
}


void EDIT_TOOL::PadFilter( const VECTOR2I&, GENERAL_COLLECTOR& aCollector,
                           PCB_SELECTION_TOOL* sTool )
{
    for( int i = aCollector.GetCount() - 1; i >= 0; i-- )
    {
        BOARD_ITEM* item = static_cast<BOARD_ITEM*>( aCollector[i] );

        if( item->Type() != PCB_PAD_T )
            aCollector.Remove( i );
    }
}


void EDIT_TOOL::FootprintFilter( const VECTOR2I&, GENERAL_COLLECTOR& aCollector,
                                 PCB_SELECTION_TOOL* sTool )
{
    for( int i = aCollector.GetCount() - 1; i >= 0; i-- )
    {
        BOARD_ITEM* item = static_cast<BOARD_ITEM*>( aCollector[i] );

        if( item->Type() != PCB_FOOTPRINT_T )
            aCollector.Remove( i );
    }
}


bool EDIT_TOOL::updateModificationPoint( PCB_SELECTION& aSelection )
{
    if( m_dragging && aSelection.HasReferencePoint() )
        return false;

    // Can't modify an empty group
    if( aSelection.Empty() )
        return false;

    // When there is only one item selected, the reference point is its position...
    if( aSelection.Size() == 1 )
    {
        auto item =  static_cast<BOARD_ITEM*>( aSelection.Front() );
        auto pos = item->GetPosition();
        aSelection.SetReferencePoint( VECTOR2I( pos.x, pos.y ) );
    }
    // ...otherwise modify items with regard to the grid-snapped center position
    else
    {
        PCB_GRID_HELPER grid( m_toolMgr, frame()->GetMagneticItemsSettings() );
        aSelection.SetReferencePoint( grid.BestSnapAnchor( aSelection.GetCenter(), nullptr ) );
    }

    return true;
}


bool EDIT_TOOL::pickReferencePoint( const wxString& aTooltip, const wxString& aSuccessMessage,
                                    const wxString& aCanceledMessage, VECTOR2I& aReferencePoint )
{
    PCB_PICKER_TOOL* picker = m_toolMgr->GetTool<PCB_PICKER_TOOL>();
    std::optional<VECTOR2I>    pickedPoint;
    bool             done = false;

    m_statusPopup->SetText( aTooltip );

    /// This allow the option of snapping in the tool
    picker->SetSnapping( true );

    picker->SetClickHandler(
            [&]( const VECTOR2D& aPoint ) -> bool
            {
                pickedPoint = aPoint;

                if( !aSuccessMessage.empty() )
                {
                    m_statusPopup->SetText( aSuccessMessage );
                    m_statusPopup->Expire( 800 );
                }
                else
                {
                    m_statusPopup->Hide();
                }

                return false; // we don't need any more points
            } );

    picker->SetMotionHandler(
            [&]( const VECTOR2D& aPos )
            {
                m_statusPopup->Move( wxGetMousePosition() + wxPoint( 20, -50 ) );
            } );

    picker->SetCancelHandler(
            [&]()
            {
                if( !aCanceledMessage.empty() )
                {
                    m_statusPopup->SetText( aCanceledMessage );
                    m_statusPopup->Expire( 800 );
                }
                else
                {
                    m_statusPopup->Hide();
                }
            } );

    picker->SetFinalizeHandler(
            [&]( const int& aFinalState )
            {
                done = true;
            } );

    m_statusPopup->Move( wxGetMousePosition() + wxPoint( 20, -50 ) );
    m_statusPopup->Popup();
    canvas()->SetStatusPopup( m_statusPopup->GetPanel() );

    TOOL_EVENT subtoolEvent = PCB_ACTIONS::pickerSubTool.MakeEvent();
    m_toolMgr->RunAction( ACTIONS::pickerSubTool, true, (void*) &subtoolEvent );

    while( !done )
    {
        // Pass events unless we receive a null event, then we must shut down
        if( TOOL_EVENT* evt = Wait() )
            evt->SetPassEvent();
        else
            break;
    }

    // Ensure statusPopup is hidden after use and before deleting it:
    canvas()->SetStatusPopup( nullptr );
    m_statusPopup->Hide();

    if( pickedPoint )
        aReferencePoint = *pickedPoint;

    return pickedPoint.has_value();
}


int EDIT_TOOL::copyToClipboard( const TOOL_EVENT& aEvent )
{
    CLIPBOARD_IO io;
    PCB_GRID_HELPER grid( m_toolMgr,
                          getEditFrame<PCB_BASE_EDIT_FRAME>()->GetMagneticItemsSettings() );
    TOOL_EVENT      selectReferencePoint( aEvent.Category(), aEvent.Action(),
                                          "pcbnew.InteractiveEdit.selectReferencePoint",
                                          TOOL_ACTION_SCOPE::AS_GLOBAL );

    frame()->PushTool( selectReferencePoint );
    Activate();

    PCB_SELECTION& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
                for( int i = aCollector.GetCount() - 1; i >= 0; --i )
                {
                    BOARD_ITEM* item = aCollector[i];

                    // We can't copy both a footprint and its text in the same operation, so if
                    // both are selected, remove the text
                    if( item->Type() == PCB_TEXT_T && aCollector.HasItem( item->GetParentFootprint() ) )
                        aCollector.Remove( item );
                }
            },

            // Prompt user regarding locked items.
            aEvent.IsAction( &ACTIONS::cut ) && !m_isFootprintEditor );

    if( !selection.Empty() )
    {
        std::vector<BOARD_ITEM*> items;

        for( EDA_ITEM* item : selection )
            items.push_back( static_cast<BOARD_ITEM*>( item ) );

        VECTOR2I refPoint;

        if( aEvent.IsAction( &PCB_ACTIONS::copyWithReference ) )
        {
            if( !pickReferencePoint( _( "Select reference point for the copy..." ),
                                     _( "Selection copied" ),
                                     _( "Copy canceled" ),
                                     refPoint ) )
            {
                frame()->PopTool( selectReferencePoint );
                return 0;
            }
        }
        else
        {
            refPoint = grid.BestDragOrigin( getViewControls()->GetCursorPosition(), items );
        }

        selection.SetReferencePoint( refPoint );

        io.SetBoard( board() );
        io.SaveSelection( selection, m_isFootprintEditor );
        frame()->SetStatusText( _( "Selection copied" ) );
    }

    frame()->PopTool( selectReferencePoint );

    if( selection.IsHover() )
        m_selectionTool->ClearSelection();

    return 0;
}


int EDIT_TOOL::cutToClipboard( const TOOL_EVENT& aEvent )
{
    if( !copyToClipboard( aEvent ) )
    {
        // N.B. Setting the CUT flag prevents lock filtering as we only want to delete the items
        // that were copied to the clipboard, no more, no fewer.  Filtering for locked item, if
        // any will be done in the copyToClipboard() routine
        TOOL_EVENT evt = aEvent;
        evt.SetParameter( PCB_ACTIONS::REMOVE_FLAGS::CUT );
        Remove( evt );
    }

    return 0;
}


void EDIT_TOOL::rebuildConnectivity()
{
    board()->BuildConnectivity();
    m_toolMgr->PostEvent( EVENTS::ConnectivityChangedEvent );
    canvas()->RedrawRatsnest();
}


void EDIT_TOOL::setTransitions()
{
    Go( &EDIT_TOOL::GetAndPlace,           PCB_ACTIONS::getAndPlace.MakeEvent() );
    Go( &EDIT_TOOL::Move,                  PCB_ACTIONS::move.MakeEvent() );
    Go( &EDIT_TOOL::Move,                  PCB_ACTIONS::moveIndividually.MakeEvent() );
    Go( &EDIT_TOOL::Drag,                  PCB_ACTIONS::drag45Degree.MakeEvent() );
    Go( &EDIT_TOOL::Drag,                  PCB_ACTIONS::dragFreeAngle.MakeEvent() );
    Go( &EDIT_TOOL::Rotate,                PCB_ACTIONS::rotateCw.MakeEvent() );
    Go( &EDIT_TOOL::Rotate,                PCB_ACTIONS::rotateCcw.MakeEvent() );
    Go( &EDIT_TOOL::Flip,                  PCB_ACTIONS::flip.MakeEvent() );
    Go( &EDIT_TOOL::Remove,                ACTIONS::doDelete.MakeEvent() );
    Go( &EDIT_TOOL::Remove,                PCB_ACTIONS::deleteFull.MakeEvent() );
    Go( &EDIT_TOOL::Properties,            PCB_ACTIONS::properties.MakeEvent() );
    Go( &EDIT_TOOL::MoveExact,             PCB_ACTIONS::moveExact.MakeEvent() );
    Go( &EDIT_TOOL::Move,                  PCB_ACTIONS::moveWithReference.MakeEvent() );
    Go( &EDIT_TOOL::Duplicate,             ACTIONS::duplicate.MakeEvent() );
    Go( &EDIT_TOOL::Duplicate,             PCB_ACTIONS::duplicateIncrement.MakeEvent() );
    Go( &EDIT_TOOL::CreateArray,           PCB_ACTIONS::createArray.MakeEvent() );
    Go( &EDIT_TOOL::Mirror,                PCB_ACTIONS::mirrorH.MakeEvent() );
    Go( &EDIT_TOOL::Mirror,                PCB_ACTIONS::mirrorV.MakeEvent() );
    Go( &EDIT_TOOL::Swap,                  PCB_ACTIONS::swap.MakeEvent() );
    Go( &EDIT_TOOL::PackAndMoveFootprints, PCB_ACTIONS::packAndMoveFootprints.MakeEvent() );
    Go( &EDIT_TOOL::ChangeTrackWidth,      PCB_ACTIONS::changeTrackWidth.MakeEvent() );
    Go( &EDIT_TOOL::FilletTracks,          PCB_ACTIONS::filletTracks.MakeEvent() );
    Go( &EDIT_TOOL::FilletLines,           PCB_ACTIONS::filletLines.MakeEvent() );

    Go( &EDIT_TOOL::copyToClipboard,       ACTIONS::copy.MakeEvent() );
    Go( &EDIT_TOOL::copyToClipboard,       PCB_ACTIONS::copyWithReference.MakeEvent() );
    Go( &EDIT_TOOL::cutToClipboard,        ACTIONS::cut.MakeEvent() );
}
