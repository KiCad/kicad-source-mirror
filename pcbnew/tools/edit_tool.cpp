/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2023 CERN
 * Copyright (C) 2017-2024 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <kiplatform/ui.h>
#include <gal/graphics_abstraction_layer.h>
#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pcb_shape.h>
#include <pcb_group.h>
#include <pcb_target.h>
#include <pcb_text.h>
#include <pcb_textbox.h>
#include <pcb_generator.h>
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
#include <tools/item_modification_routine.h>
#include <tools/pcb_picker_tool.h>
#include <tools/pcb_point_editor.h>
#include <tools/tool_event_utils.h>
#include <tools/pcb_grid_helper.h>
#include <tools/pad_tool.h>
#include <view/view_controls.h>
#include <connectivity/connectivity_algo.h>
#include <connectivity/connectivity_items.h>
#include <core/kicad_algo.h>
#include <fix_board_shape.h>
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
#include <pcb_reference_image.h>

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
}


static std::shared_ptr<CONDITIONAL_MENU> makePositioningToolsMenu( TOOL_INTERACTIVE* aTool )
{
    auto menu = std::make_shared<CONDITIONAL_MENU>( aTool );

    menu->SetIcon( BITMAPS::special_tools );
    menu->SetTitle( _( "Positioning Tools" ) );

    auto notMovingCondition = []( const SELECTION& aSelection )
    {
        return aSelection.Empty() || !aSelection.Front()->IsMoving();
    };

    // clang-format off
    menu->AddItem( PCB_ACTIONS::moveExact,         SELECTION_CONDITIONS::NotEmpty && notMovingCondition );
    menu->AddItem( PCB_ACTIONS::moveWithReference, SELECTION_CONDITIONS::NotEmpty && notMovingCondition );
    menu->AddItem( PCB_ACTIONS::copyWithReference, SELECTION_CONDITIONS::NotEmpty && notMovingCondition );
    menu->AddItem( PCB_ACTIONS::positionRelative,  SELECTION_CONDITIONS::NotEmpty && notMovingCondition );
    // clang-format on
    return menu;
};


static std::shared_ptr<CONDITIONAL_MENU> makeShapeModificationMenu( TOOL_INTERACTIVE* aTool )
{
    auto menu = std::make_shared<CONDITIONAL_MENU>( aTool );

    menu->SetTitle( _( "Shape Modification" ) );

    static const std::vector<KICAD_T> filletChamferTypes = { PCB_SHAPE_LOCATE_POLY_T,
                                                             PCB_SHAPE_LOCATE_RECT_T,
                                                             PCB_SHAPE_LOCATE_SEGMENT_T };

    static const std::vector<KICAD_T> healShapesTypes = { PCB_SHAPE_LOCATE_SEGMENT_T,
                                                          PCB_SHAPE_LOCATE_ARC_T,
                                                          PCB_SHAPE_LOCATE_BEZIER_T };

    static const std::vector<KICAD_T> lineExtendTypes = { PCB_SHAPE_LOCATE_SEGMENT_T };

    static const std::vector<KICAD_T> polygonBooleanTypes = { PCB_SHAPE_LOCATE_RECT_T,
                                                              PCB_SHAPE_LOCATE_POLY_T };

    auto hasCornerCondition =
            [aTool]( const SELECTION& aSelection )
            {
                PCB_POINT_EDITOR *pt_tool = aTool->GetManager()->GetTool<PCB_POINT_EDITOR>();

                return pt_tool && pt_tool->HasCorner();
            };

    auto hasMidpointCondition =
            [aTool]( const SELECTION& aSelection )
            {
                PCB_POINT_EDITOR *pt_tool = aTool->GetManager()->GetTool<PCB_POINT_EDITOR>();

                return pt_tool && pt_tool->HasMidpoint();
            };

    // clang-format off
    menu->AddItem( PCB_ACTIONS::healShapes,              SELECTION_CONDITIONS::HasTypes( healShapesTypes ) );
    menu->AddItem( PCB_ACTIONS::filletLines,             SELECTION_CONDITIONS::OnlyTypes( filletChamferTypes ) );
    menu->AddItem( PCB_ACTIONS::chamferLines,            SELECTION_CONDITIONS::OnlyTypes( filletChamferTypes ) );
    menu->AddItem( PCB_ACTIONS::extendLines,             SELECTION_CONDITIONS::OnlyTypes( lineExtendTypes )
                                                             && SELECTION_CONDITIONS::Count( 2 ) );
    menu->AddItem( PCB_ACTIONS::pointEditorMoveCorner,   hasCornerCondition );
    menu->AddItem( PCB_ACTIONS::pointEditorMoveMidpoint, hasMidpointCondition );

    menu->AddSeparator( SELECTION_CONDITIONS::OnlyTypes( polygonBooleanTypes )
                        && SELECTION_CONDITIONS::MoreThan( 1 ) );

    menu->AddItem( PCB_ACTIONS::mergePolygons,           SELECTION_CONDITIONS::OnlyTypes( polygonBooleanTypes )
                                                             && SELECTION_CONDITIONS::MoreThan( 1 ) );
    menu->AddItem( PCB_ACTIONS::subtractPolygons,        SELECTION_CONDITIONS::OnlyTypes( polygonBooleanTypes )
                                                             && SELECTION_CONDITIONS::MoreThan( 1 ) );
    menu->AddItem( PCB_ACTIONS::intersectPolygons,       SELECTION_CONDITIONS::OnlyTypes( polygonBooleanTypes )
                                                             && SELECTION_CONDITIONS::MoreThan( 1 ) );
    // clang-format on

    return menu;
};

bool EDIT_TOOL::Init()
{
    // Find the selection tool, so they can cooperate
    m_selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();

    std::shared_ptr<CONDITIONAL_MENU> positioningToolsSubMenu = makePositioningToolsMenu( this );
    m_selectionTool->GetToolMenu().RegisterSubMenu( positioningToolsSubMenu );

    std::shared_ptr<CONDITIONAL_MENU> shapeModificationSubMenu = makeShapeModificationMenu( this );
    m_selectionTool->GetToolMenu().RegisterSubMenu( shapeModificationSubMenu );

    auto positioningToolsCondition =
            [&]( const SELECTION& aSel )
            {
                std::shared_ptr<CONDITIONAL_MENU> subMenu = makePositioningToolsMenu( this );
                subMenu->Evaluate( aSel );
                return subMenu->GetMenuItemCount() > 0;
            };

    auto shapeModificationCondition =
            [&]( const SELECTION& aSel )
            {
                std::shared_ptr<CONDITIONAL_MENU> subMenu = makeShapeModificationMenu( this );
                subMenu->Evaluate( aSel );
                return subMenu->GetMenuItemCount() > 0;
            };

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

    auto multipleFootprintsCondition =
            []( const SELECTION& aSelection )
            {
                bool foundFirst = false;

                for( EDA_ITEM* item : aSelection )
                {
                    if( item->Type() == PCB_FOOTPRINT_T )
                    {
                        if( foundFirst )
                            return true;
                        else
                            foundFirst = true;
                    }
                }

                return false;
            };

    auto noActiveToolCondition =
            [ this ]( const SELECTION& aSelection )
            {
                return frame()->ToolStackIsEmpty();
            };

    auto notMovingCondition =
            []( const SELECTION& aSelection )
            {
                return aSelection.Empty() || !aSelection.Front()->IsMoving();
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

    // Add context menu entries that are displayed when selection tool is active
    CONDITIONAL_MENU& menu = m_selectionTool->GetToolMenu().GetMenu();

    // clang-format off
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
    menu.AddItem( PCB_ACTIONS::updateFootprints,  multipleFootprintsCondition );
    menu.AddItem( PCB_ACTIONS::changeFootprint,   singleFootprintCondition );
    menu.AddItem( PCB_ACTIONS::changeFootprints,  multipleFootprintsCondition );

    // Add the submenu for the special tools: modfiers and positioning tools
    menu.AddSeparator( 100 );
    menu.AddMenu( shapeModificationSubMenu.get(), shapeModificationCondition, 100 );
    menu.AddMenu( positioningToolsSubMenu.get(),  positioningToolsCondition, 100 );

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
    menu.AddItem( ACTIONS::selectAll,             noItemsCondition, 150 );
    menu.AddItem( ACTIONS::unselectAll,           noItemsCondition, 150 );
    // clang-format on

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
        m_toolMgr->RunAction( PCB_ACTIONS::selectionClear );
        m_toolMgr->RunAction<EDA_ITEM*>( PCB_ACTIONS::selectItem, fp );

        selectionTool->GetSelection().SetReferencePoint( fp->GetPosition() );
        m_toolMgr->PostAction( PCB_ACTIONS::move );
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
        m_toolMgr->RunAction( PCB_ACTIONS::routerInlineDrag, aDragMode );
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
    {
        wxBell();
        return false; // don't drag when no router tool (i.e. fp editor)
    }

    if( m_toolMgr->GetTool<ROUTER_TOOL>()->IsToolActive() )
    {
        wxBell();
        return false; // don't drag when router is already active
    }

    if( m_dragging )
    {
        wxBell();
        return false; // don't do a router drag when already in an EDIT_TOOL drag
    }

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

    BOARD_COMMIT  commit( this );
    bool          restore_state = false;

    commit.Modify( theArc );

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

    std::set<PCB_TRACK*> addedTracks;

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

                PCB_TRACK* track = nullptr;

                if( itemsOnAnchor.size() == 1 && itemsOnAnchor.front()->Type() == PCB_TRACE_T )
                {
                    track = static_cast<PCB_TRACK*>( itemsOnAnchor.front() );
                    commit.Modify( track );

                    SEG trackSeg( track->GetStart(), track->GetEnd() );

                    // Allow deviations in colinearity as defined in ADVANCED_CFG
                    if( trackSeg.Angle( aCollinearSeg ) > maxTangentDeviation )
                        track = nullptr;
                }

                if( !track )
                {
                    track = new PCB_TRACK( theArc->GetParent() );
                    track->SetStart( aAnchor );
                    track->SetEnd( aAnchor );
                    track->SetNet( theArc->GetNet() );
                    track->SetLayer( theArc->GetLayer() );
                    track->SetWidth( theArc->GetWidth() );
                    track->SetLocked( theArc->IsLocked() );
                    track->SetFlags( IS_NEW );
                    getView()->Add( track );
                    addedTracks.insert( track );
                }

                return track;
            };

    PCB_TRACK* trackOnStart = getUniqueTrackAtAnchorCollinear( theArc->GetStart(), tanStart);
    PCB_TRACK* trackOnEnd = getUniqueTrackAtAnchorCollinear( theArc->GetEnd(), tanEnd );

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

            for( const VECTOR2I& candidate : possiblePoints )
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
            if( eatFirstMouseUp && !evt->IsAction( &ACTIONS::cursorClick ) )
            {
                eatFirstMouseUp = false;
                continue;
            }

            break; // Finish
        }
    }

    // Amend the end points of the arc if we delete the joining tracks
    VECTOR2I newStart = trackOnStart->GetStart();
    VECTOR2I newEnd = trackOnEnd->GetStart();

    if( isStartTrackOnStartPt )
        newStart = trackOnStart->GetEnd();

    if( isEndTrackOnStartPt )
        newEnd = trackOnEnd->GetEnd();

    int maxLengthIU =
            KiROUND( ADVANCED_CFG::GetCfg().m_MaxTrackLengthToKeep * pcbIUScale.IU_PER_MM );

    if( trackOnStart->GetLength() <= maxLengthIU )
    {
        if( addedTracks.count( trackOnStart ) )
        {
            getView()->Remove( trackOnStart );
            addedTracks.erase( trackOnStart );
            delete trackOnStart;
        }
        else
        {
            commit.Remove( trackOnStart );
        }

        theArc->SetStart( newStart );
    }

    if( trackOnEnd->GetLength() <= maxLengthIU )
    {
        if( addedTracks.count( trackOnEnd ) )
        {
            getView()->Remove( trackOnEnd );
            addedTracks.erase( trackOnEnd );
            delete trackOnEnd;
        }
        else
        {
            commit.Remove( trackOnEnd );
        }

        theArc->SetEnd( newEnd );
    }

    if( theArc->GetLength() <= 0 )
        commit.Remove( theArc );

    for( PCB_TRACK* added : addedTracks )
    {
        getView()->Remove( added );
        commit.Add( added );
    }

    // Should we commit?
    if( restore_state )
        commit.Revert();
    else
        commit.Push( _( "Drag Arc Track" ) );

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

    BOARD_COMMIT commit( this );

    for( EDA_ITEM* item : selection )
    {
        if( item->Type() == PCB_VIA_T )
        {
            PCB_VIA* via = static_cast<PCB_VIA*>( item );

            commit.Modify( via );

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

            commit.Modify( track );

            int new_width = board()->GetDesignSettings().GetCurrentTrackWidth();
            track->SetWidth( new_width );
        }
    }

    commit.Push( _( "Edit track width/via size" ) );

    if( selection.IsHover() )
    {
        m_toolMgr->RunAction( PCB_ACTIONS::selectionClear );

        // Notify other tools of the changes -- This updates the visual ratsnest
        m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );
    }

    return 0;
}


int EDIT_TOOL::FilletTracks( const TOOL_EVENT& aEvent )
{
    // Store last used fillet radius to allow pressing "enter" if repeat fillet is required
    static int filletRadius = 0;

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

    WX_UNIT_ENTRY_DIALOG dlg( frame(), _( "Fillet Tracks" ), _( "Enter fillet radius:" ),
                              filletRadius );

    if( dlg.ShowModal() == wxID_CANCEL )
        return 0;

    filletRadius = dlg.GetValue();

    if( filletRadius == 0 )
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

    auto processFilletOp =
            [&]( PCB_TRACK* aTrack, bool aStartPoint )
            {
                std::shared_ptr<CONNECTIVITY_DATA> c = board()->GetConnectivity();
                VECTOR2I anchor = aStartPoint ? aTrack->GetStart() : aTrack->GetEnd();
                std::vector<BOARD_CONNECTED_ITEM*> itemsOnAnchor;

                itemsOnAnchor = c->GetConnectedItemsAtAnchor( aTrack, anchor,
                                                              { PCB_PAD_T, PCB_VIA_T,
                                                                PCB_TRACE_T, PCB_ARC_T } );

                if( itemsOnAnchor.size() > 0
                        && selection.Contains( itemsOnAnchor.at( 0 ) )
                        && itemsOnAnchor.at( 0 )->Type() == PCB_TRACE_T )
                {
                    PCB_TRACK* trackOther = static_cast<PCB_TRACK*>( itemsOnAnchor.at( 0 ) );

                    // Make sure we don't fillet the same pair of tracks twice
                    if( processedTracks.find( trackOther ) == processedTracks.end() )
                    {
                        if( itemsOnAnchor.size() == 1 )
                        {
                            FILLET_OP filletOp;
                            filletOp.t1      = aTrack;
                            filletOp.t2      = trackOther;
                            filletOp.t1Start = aStartPoint;
                            filletOp.t2Start = aTrack->IsPointOnEnds( filletOp.t2->GetStart() );
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

    for( EDA_ITEM* item : selection )
    {
        if( item->Type() == PCB_TRACE_T )
        {
            PCB_TRACK* track = static_cast<PCB_TRACK*>( item );

            if( track->GetLength() > 0 )
            {
                processFilletOp( track, true ); // on the start point of track
                processFilletOp( track, false ); // on the end point of track

                processedTracks.insert( track );
            }
        }
    }

    BOARD_COMMIT             commit( this );
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

            SHAPE_ARC sArc( t1Seg, t2Seg, filletRadius );
            VECTOR2I  t1newPoint, t2newPoint;

            auto setIfPointOnSeg =
                    []( VECTOR2I& aPointToSet, const SEG& aSegment, const VECTOR2I& aVecToTest )
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
            commit.Add( tArc );
            itemsToAddToSelection.push_back( tArc );

            commit.Modify( track1 );
            commit.Modify( track2 );

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

    commit.Push( _( "Fillet Tracks" ) );

    //select the newly created arcs
    for( BOARD_ITEM* item : itemsToAddToSelection )
        m_selectionTool->AddItemToSel( item );

    if( !operationPerformedOnAtLeastOne )
        frame()->ShowInfoBarMsg( _( "Unable to fillet the selected track segments." ) );
    else if( didOneAttemptFail )
        frame()->ShowInfoBarMsg( _( "Some of the track segments could not be filleted." ) );

    return 0;
}

/**
 * Prompt the user for the fillet radius and return it.
 *
 * @param aFrame
 * @param aErrorMsg filled with an error message if the parameter is invalid somehow
 * @return std::optional<int> the fillet radius or std::nullopt if no
 * valid fillet specified
 */
static std::optional<int> GetFilletParams( PCB_BASE_EDIT_FRAME& aFrame )
{
    // Store last used fillet radius to allow pressing "enter" if repeat fillet is required
    static int filletRadius = 0;

    WX_UNIT_ENTRY_DIALOG dlg( &aFrame, _( "Fillet Lines" ), _( "Enter fillet radius:" ),
                              filletRadius );

    if( dlg.ShowModal() == wxID_CANCEL )
        return std::nullopt;

    filletRadius = dlg.GetValue();

    return filletRadius;
}

/**
 * Prompt the user for chamfer parameters
 *
 * @param aFrame
 * @param aErrorMsg filled with an error message if the parameter is invalid somehow
 * @return std::optional<int> the chamfer parameters or std::nullopt if no
 * valid fillet specified
 */
static std::optional<CHAMFER_PARAMS> GetChamferParams( PCB_BASE_EDIT_FRAME& aFrame )
{
    // Non-zero and the KLC default for Fab layer chamfers
    const int default_setback = pcbIUScale.mmToIU( 1 );
    // Store last used setback to allow pressing "enter" if repeat chamfer is required
    static CHAMFER_PARAMS params{ default_setback, default_setback };

    WX_UNIT_ENTRY_DIALOG dlg( &aFrame, _( "Chamfer Lines" ), _( "Enter chamfer setback:" ),
                              params.m_chamfer_setback_a );

    if( dlg.ShowModal() == wxID_CANCEL || dlg.GetValue() == 0 )
        return std::nullopt;

    params.m_chamfer_setback_a = dlg.GetValue();
    // It's hard to easily specify an asymmetric chamfer (which line gets the longer setback?),
    // so we just use the same setback for each
    params.m_chamfer_setback_b = params.m_chamfer_setback_a;

    return params;
}

int EDIT_TOOL::ModifyLines( const TOOL_EVENT& aEvent )
{
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

    std::set<PCB_SHAPE*>    lines_to_add;
    std::vector<PCB_SHAPE*> items_to_remove;

    for( EDA_ITEM* item : selection )
    {
        std::vector<VECTOR2I> pts;
        PCB_SHAPE *graphic = static_cast<PCB_SHAPE*>( item );
        PCB_LAYER_ID layer = graphic->GetLayer();
        int width = graphic->GetWidth();

        if( graphic->GetShape() == SHAPE_T::RECTANGLE )
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

    int segmentCount = selection.CountType( PCB_SHAPE_LOCATE_SEGMENT_T ) + lines_to_add.size();

    if( aEvent.IsAction( &PCB_ACTIONS::extendLines ) && segmentCount != 2 )
    {
        frame()->ShowInfoBarMsg( _( "Exactly two lines must be selected to extend them." ) );

        for( PCB_SHAPE* line : lines_to_add )
            delete line;

        return 0;
    }
    else if( segmentCount < 2 )
    {
        frame()->ShowInfoBarMsg( _( "A shape with at least two lines must be selected." ) );

        for( PCB_SHAPE* line : lines_to_add )
            delete line;

        return 0;
    }

    BOARD_COMMIT commit( this );

    // Items created like lines from a rectangle
    for( PCB_SHAPE* item : lines_to_add )
    {
        commit.Add( item );
        selection.Add( item );
    }

    // Remove items like rectangles that we decomposed into lines
    for( PCB_SHAPE* item : items_to_remove )
    {
        selection.Remove( item );
        commit.Remove( item );
    }

    for( EDA_ITEM* item : selection )
        item->ClearFlags( STRUCT_DELETED );

    // List of thing to select at the end of the operation
    // (doing it as we go will invalidate the iterator)
    std::vector<PCB_SHAPE*> items_to_select_on_success;

    // And same for items to deselect
    std::vector<PCB_SHAPE*> items_to_deselect_on_success;

    // Handle modifications to existing items by the routine
    // How to deal with this depends on whether we're in the footprint editor or not
    // and whether the item was conjured up by decomposing a polygon or rectangle
    const auto item_modification_handler = [&]( PCB_SHAPE& aItem )
    {
        // If the item was "conjured up" it will be added later separately
        if( !alg::contains( lines_to_add, &aItem ) )
        {
            commit.Modify( &aItem );
            items_to_select_on_success.push_back( &aItem );
        }
    };

    bool       any_items_created = !lines_to_add.empty();
    const auto item_creation_handler = [&]( std::unique_ptr<PCB_SHAPE> aItem )
    {
        any_items_created = true;
        items_to_select_on_success.push_back( aItem.get() );
        commit.Add( aItem.release() );
    };

    bool any_items_removed = !items_to_remove.empty();
    auto item_removal_handler =
            [&]( PCB_SHAPE& aItem )
            {
                aItem.SetFlags( STRUCT_DELETED );
                any_items_removed = true;
                items_to_deselect_on_success.push_back( &aItem );
                commit.Remove( &aItem );
            };

    // Combine these callbacks into a CHANGE_HANDLER to inject in the ROUTINE
    ITEM_MODIFICATION_ROUTINE::CALLABLE_BASED_HANDLER change_handler(
            item_creation_handler, item_modification_handler, item_removal_handler );

    // Construct an appropriate tool
    std::unique_ptr<PAIRWISE_LINE_ROUTINE> pairwise_line_routine;

    if( aEvent.IsAction( &PCB_ACTIONS::filletLines ) )
    {
        std::optional<int> filletRadiusIU = GetFilletParams( *frame() );

        if( filletRadiusIU.has_value() )
        {
            pairwise_line_routine = std::make_unique<LINE_FILLET_ROUTINE>(
                    frame()->GetModel(), change_handler, *filletRadiusIU );
        }
    }
    else if( aEvent.IsAction( &PCB_ACTIONS::chamferLines ) )
    {
        std::optional<CHAMFER_PARAMS> chamfer_params = GetChamferParams( *frame() );

        if( chamfer_params.has_value() )
        {
            pairwise_line_routine = std::make_unique<LINE_CHAMFER_ROUTINE>(
                    frame()->GetModel(), change_handler, *chamfer_params );
        }
    }
    else if( aEvent.IsAction( &PCB_ACTIONS::extendLines ) )
    {
        pairwise_line_routine = std::make_unique<LINE_EXTENSION_ROUTINE>( frame()->GetModel(),
                                                                          change_handler );
    }

    if( !pairwise_line_routine )
    {
        // Didn't construct any mofication routine - user must have cancelled
        commit.Revert();
        return 0;
    }

    // Apply the tool to every line pair
    alg::for_all_pairs( selection.begin(), selection.end(),
                        [&]( EDA_ITEM* a, EDA_ITEM* b )
                        {
                            if( ( a->GetFlags() & STRUCT_DELETED ) == 0
                                && ( b->GetFlags() & STRUCT_DELETED ) == 0 )
                            {
                                PCB_SHAPE* line_a = static_cast<PCB_SHAPE*>( a );
                                PCB_SHAPE* line_b = static_cast<PCB_SHAPE*>( b );

                                pairwise_line_routine->ProcessLinePair( *line_a, *line_b );
                            }
                        } );

    // Select added and modified items
    for( PCB_SHAPE* item : items_to_select_on_success )
        m_selectionTool->AddItemToSel( item, true );

    // Deselect removed items
    for( PCB_SHAPE* item : items_to_deselect_on_success )
        m_selectionTool->RemoveItemFromSel( item, true );

    if( any_items_removed )
        m_toolMgr->ProcessEvent( EVENTS::UnselectedEvent );

    if( any_items_created )
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

    // Notify other tools of the changes
    m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );

    commit.Push( pairwise_line_routine->GetCommitDescription() );

    if (const std::optional<wxString> msg = pairwise_line_routine->GetStatusMessage()) {
        frame()->ShowInfoBarMsg( *msg );
    }

    return 0;
}


int EDIT_TOOL::HealShapes( const TOOL_EVENT& aEvent )
{
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
                    if( !item->IsType( { PCB_SHAPE_LOCATE_SEGMENT_T, PCB_SHAPE_LOCATE_ARC_T,
                                         PCB_SHAPE_LOCATE_BEZIER_T } ) )
                    {
                        aCollector.Remove( item );
                    }
                }
            },
            true /* prompt user regarding locked items */ );

    // Store last used value
    static int s_toleranceValue = pcbIUScale.mmToIU( 3 );

    WX_UNIT_ENTRY_DIALOG dlg( frame(), _( "Heal Shapes" ), _( "Tolerance value:" ),
                              s_toleranceValue );

    if( dlg.ShowModal() == wxID_CANCEL )
        return 0;

    s_toleranceValue = dlg.GetValue();

    if( s_toleranceValue <= 0 )
        return 0;

    BOARD_COMMIT commit{ this };

    std::vector<PCB_SHAPE*>                 shapeList;
    std::vector<std::unique_ptr<PCB_SHAPE>> newShapes;

    for( EDA_ITEM* item : selection )
    {
        if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( item ) )
        {
            shapeList.push_back( shape );
            commit.Modify( shape );
        }
    }

    ConnectBoardShapes( shapeList, newShapes, s_toleranceValue );

    std::vector<PCB_SHAPE*> items_to_select;

    for( std::unique_ptr<PCB_SHAPE>& ptr : newShapes )
    {
        PCB_SHAPE* shape = ptr.release();

        commit.Add( shape );
        items_to_select.push_back( shape );
    }

    commit.Push( _( "Heal shapes" ) );

    // Select added items
    for( PCB_SHAPE* item : items_to_select )
        m_selectionTool->AddItemToSel( item, true );

    if( items_to_select.size() > 0 )
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

    // Notify other tools of the changes
    m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );

    return 0;
}


int EDIT_TOOL::BooleanPolygons( const TOOL_EVENT& aEvent )
{
    PCB_SELECTION& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
                // Iterate from the back so we don't have to worry about removals.
                for( int i = aCollector.GetCount() - 1; i >= 0; --i )
                {
                    BOARD_ITEM* item = aCollector[i];

                    if( !item->IsType( {
                                PCB_SHAPE_LOCATE_POLY_T,
                                PCB_SHAPE_LOCATE_RECT_T,
                        } ) )
                    {
                        aCollector.Remove( item );
                    }
                }
            },
            true /* prompt user regarding locked items */ );

    const EDA_ITEM* const last_item = selection.GetLastAddedItem();

    // Gather or construct polygon source shapes to merge
    std::vector<PCB_SHAPE*> items_to_process;
    for( EDA_ITEM* item : selection )
    {
        items_to_process.push_back( static_cast<PCB_SHAPE*>( item ) );

        // put the last one in the selection at the front of the vector
        // so it can be used as the property donor and as the basis for the
        // boolean operation
        if( item == last_item )
        {
            std::swap( items_to_process.back(), items_to_process.front() );
        }
    }

    BOARD_COMMIT commit{ this };

    // Handle modifications to existing items by the routine
    const auto item_modification_handler = [&]( PCB_SHAPE& aItem )
    {
        commit.Modify( &aItem );
    };

    std::vector<PCB_SHAPE*> items_to_select_on_success;
    const auto              item_creation_handler = [&]( std::unique_ptr<PCB_SHAPE> aItem )
    {
        items_to_select_on_success.push_back( aItem.get() );
        commit.Add( aItem.release() );
    };

    const auto item_removal_handler = [&]( PCB_SHAPE& aItem )
    {
        commit.Remove( &aItem );
    };

    // Combine these callbacks into a CHANGE_HANDLER to inject in the ROUTINE
    ITEM_MODIFICATION_ROUTINE::CALLABLE_BASED_HANDLER change_handler(
            item_creation_handler, item_modification_handler, item_removal_handler );

    // Construct an appropriate routine
    std::unique_ptr<POLYGON_BOOLEAN_ROUTINE> boolean_routine;
    if( aEvent.IsAction( &PCB_ACTIONS::mergePolygons ) )
    {
        boolean_routine =
                std::make_unique<POLYGON_MERGE_ROUTINE>( frame()->GetModel(), change_handler );
    }
    else if( aEvent.IsAction( &PCB_ACTIONS::subtractPolygons ) )
    {
        boolean_routine =
                std::make_unique<POLYGON_SUBTRACT_ROUTINE>( frame()->GetModel(), change_handler );
    }
    else if( aEvent.IsAction( &PCB_ACTIONS::intersectPolygons ) )
    {
        boolean_routine =
                std::make_unique<POLYGON_INTERSECT_ROUTINE>( frame()->GetModel(), change_handler );
    }
    else
    {
        wxASSERT_MSG( false, "Could not find a polygon routine for this action" );
        return 0;
    }

    // Perform the operation on each polygon
    for( PCB_SHAPE* shape : items_to_process )
    {
        boolean_routine->ProcessShape( *shape );
    }

    // Select new items
    for( PCB_SHAPE* item : items_to_select_on_success )
    {
        m_selectionTool->AddItemToSel( item, true );
    }

    // Notify other tools of the changes
    m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );

    commit.Push( boolean_routine->GetCommitDescription() );

    if( const std::optional<wxString> msg = boolean_routine->GetStatusMessage() )
    {
        frame()->ShowInfoBarMsg( *msg );
    }

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
        DIALOG_TRACK_VIA_PROPERTIES dlg( editFrame, selection );
        dlg.ShowQuasiModal();       // QuasiModal required for NET_SELECTOR
    }
    else if( selection.Size() == 1 && selection.Front()->IsBOARD_ITEM() )
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
            m_toolMgr->PostAction( ACTIONS::pageSettings );
        else
            m_toolMgr->RunAction( PCB_ACTIONS::footprintProperties );
    }

    if( selection.IsHover() )
    {
        m_toolMgr->RunAction( PCB_ACTIONS::selectionClear );
    }
    else
    {
        // Check for items becoming invisible and drop them from the selection.

        PCB_SELECTION selCopy = selection;
        LSET          visible = editFrame->GetBoard()->GetVisibleLayers();

        for( EDA_ITEM* eda_item : selCopy )
        {
            if( !eda_item->IsBOARD_ITEM() )
                continue;

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
    BOARD_COMMIT         localCommit( this );
    BOARD_COMMIT*        commit = dynamic_cast<BOARD_COMMIT*>( aEvent.Commit() );

    if( !commit )
        commit = &localCommit;

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
            !m_dragging && frame()->GetPcbNewSettings()->m_AllowFreePads && !m_isFootprintEditor );

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
                !m_dragging /* prompt user regarding locked items */ );
    }

    // Did we filter everything out?  If so, don't try to operate further
    if( selection.Empty() )
        return 0;

    // A single textbox "walks" if we rotate it around its position, as we keep resetting which
    // corner is the origin.
    if( selection.Size() == 1 && dynamic_cast<PCB_TEXTBOX*>( selection.Front() ) )
    {
        selection.SetReferencePoint( static_cast<PCB_TEXTBOX*>( selection.Front() )->GetCenter() );
    }
    else
    {
        updateModificationPoint( selection );
    }

    VECTOR2I  refPt = selection.GetReferencePoint();
    EDA_ANGLE rotateAngle = TOOL_EVT_UTILS::GetEventRotationAngle( *editFrame, aEvent );

    if( frame()->GetCanvas()->GetView()->GetGAL()->IsFlippedX() )
        rotateAngle = -rotateAngle;

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
        for( EDA_ITEM* item : selection )
        {
            if( !item->IsNew() && !item->IsMoving() )
                commit->Modify( item );

            if( item->IsBOARD_ITEM() )
            {
                BOARD_ITEM* board_item = static_cast<BOARD_ITEM*>( item );

                board_item->Rotate( refPt, rotateAngle );
                board_item->Normalize();
            }
        }

        if( !localCommit.Empty() )
            localCommit.Push( _( "Rotate" ) );

        if( is_hover && !m_dragging )
            m_toolMgr->RunAction( PCB_ACTIONS::selectionClear );

        m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );

        if( m_dragging )
            m_toolMgr->PostAction( PCB_ACTIONS::updateLocalRatsnest, VECTOR2I() );
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
        PCB_FIELD_T,
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

    BOARD_COMMIT  localCommit( this );
    BOARD_COMMIT* commit = dynamic_cast<BOARD_COMMIT*>( aEvent.Commit() );

    if( !commit )
        commit = &localCommit;

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

        if( !item->IsNew() && !item->IsMoving() )
            commit->Modify( item );

        // modify each object as necessary
        switch( item->Type() )
        {
        case PCB_SHAPE_T:
            static_cast<PCB_SHAPE*>( item )->Mirror( mirrorPoint, mirrorAroundXaxis );
            break;

        case PCB_ZONE_T:
            static_cast<ZONE*>( item )->Mirror( mirrorPoint, mirrorLeftRight );
            break;

        case PCB_FIELD_T:
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

    if( !localCommit.Empty() )
        localCommit.Push( _( "Mirror" ) );

    if( selection.IsHover() && !m_dragging )
        m_toolMgr->RunAction( PCB_ACTIONS::selectionClear );

    m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );

    if( m_dragging )
        m_toolMgr->PostAction( PCB_ACTIONS::updateLocalRatsnest, VECTOR2I() );

    return 0;
}


int EDIT_TOOL::Flip( const TOOL_EVENT& aEvent )
{
    if( isRouterActive() )
    {
        wxBell();
        return 0;
    }

    BOARD_COMMIT  localCommit( this );
    BOARD_COMMIT* commit = dynamic_cast<BOARD_COMMIT*>( aEvent.Commit() );

    if( !commit )
        commit = &localCommit;

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
    // but only if the item is not a PCB_SHAPE with SHAPE_T::RECTANGLE shape, because
    // for this shape the flip transform swap start and end coordinates and move the shape.
    // So using the center of the shape is better (the shape does not move)
    if( selection.GetSize() == 1 )
    {
        PCB_SHAPE* item = dynamic_cast<PCB_SHAPE*>( selection.GetItem( 0 ) );

        if( !item || item->GetShape() != SHAPE_T::RECTANGLE )
            refPt = selection.GetReferencePoint();
    }

    bool leftRight = frame()->GetPcbNewSettings()->m_FlipLeftRight;

    for( EDA_ITEM* item : selection )
    {
        if( !item->IsBOARD_ITEM() )
            continue;

        BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( item );

        if( !boardItem->IsNew() && !boardItem->IsMoving() )
            commit->Modify( boardItem );

        boardItem->Flip( refPt, leftRight );
        boardItem->Normalize();
    }

    if( !localCommit.Empty() )
        localCommit.Push( _( "Change Side / Flip" ) );

    if( selection.IsHover() && !m_dragging )
        m_toolMgr->RunAction( PCB_ACTIONS::selectionClear );

    m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );

    if( m_dragging )
        m_toolMgr->PostAction( PCB_ACTIONS::updateLocalRatsnest, VECTOR2I() );

    // Restore the old reference so any mouse dragging that occurs doesn't make the selection jump
    // to this now invalid reference
    if( oldRefPt )
        selection.SetReferencePoint( *oldRefPt );
    else
        selection.ClearReferencePoint();

    return 0;
}


void EDIT_TOOL::getChildItemsOfGroupsAndGenerators( EDA_ITEM*                        item,
                                                    std::unordered_set<BOARD_ITEM*>& children )
{
    wxASSERT( item->Type() == PCB_GROUP_T || item->Type() == PCB_GENERATOR_T );

    std::unordered_set<BOARD_ITEM*>& childItems = static_cast<PCB_GROUP*>( item )->GetItems();

    for( BOARD_ITEM* childItem : childItems )
    {
        children.insert( childItem );

        if( childItem->Type() == PCB_GROUP_T || childItem->Type() == PCB_GENERATOR_T )
            getChildItemsOfGroupsAndGenerators( childItem, children );
    }
}


void EDIT_TOOL::removeNonRootItems( std::unordered_set<EDA_ITEM*>& items )
{
    auto itr = items.begin();

    while( itr != items.end() )
    {
        EDA_ITEM* item = *itr;

        if( item->IsBOARD_ITEM() )
        {
            BOARD_ITEM* curItem = static_cast<BOARD_ITEM*>( item );

            if( curItem->Type() == PCB_GROUP_T || curItem->Type() == PCB_GENERATOR_T )
            {
                std::unordered_set<BOARD_ITEM*> childItems;
                getChildItemsOfGroupsAndGenerators( curItem, childItems );

                std::for_each( childItems.begin(), childItems.end(),
                               [&]( auto eraseItem )
                               {
                                   items.erase( eraseItem );
                               } );
            }
        }

        ++itr;
    }
}


void EDIT_TOOL::DeleteItems( const PCB_SELECTION& aItems, bool aIsCut )
{
    BOARD_COMMIT commit( this );

    // As we are about to remove items, they have to be removed from the selection first
    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear );

    // Only delete items that are the root of a selected set (e.g. only delete grouped / generated
    // items from the parent item, not individually) - issue #17527
    std::unordered_set<EDA_ITEM*> rootItems( aItems.begin(), aItems.end() );
    removeNonRootItems( rootItems );

    for( EDA_ITEM* item : rootItems )
    {
        if( !item->IsBOARD_ITEM() )
            continue;

        BOARD_ITEM* board_item = static_cast<BOARD_ITEM*>( item );
        FOOTPRINT*  parentFP = board_item->GetParentFootprint();

        if( board_item->GetParentGroup() )
            commit.Stage( board_item, CHT_UNGROUP );

        switch( item->Type() )
        {
        case PCB_FIELD_T:
            wxASSERT( parentFP );
            commit.Modify( parentFP );
            static_cast<PCB_TEXT*>( board_item )->SetVisible( false );
            getView()->Update( board_item );
            break;

        case PCB_TEXT_T:
            if( parentFP )
            {
                commit.Modify( parentFP );
                parentFP->Delete( board_item );
                getView()->Update( parentFP );
            }
            else
            {
                commit.Remove( board_item );
            }

            break;

        case PCB_SHAPE_T:
        case PCB_TEXTBOX_T:
        case PCB_REFERENCE_IMAGE_T:
        case PCB_DIMENSION_T:
        case PCB_DIM_ALIGNED_T:
        case PCB_DIM_LEADER_T:
        case PCB_DIM_CENTER_T:
        case PCB_DIM_RADIAL_T:
        case PCB_DIM_ORTHOGONAL_T:
            commit.Remove( board_item );
            break;

        case PCB_GROUP_T:
            board_item->RunOnDescendants(
                         [&commit]( BOARD_ITEM* aItem )
                         {
                             commit.Stage( aItem, CHT_UNGROUP );
                         } );

            board_item->RunOnDescendants(
                         [&commit]( BOARD_ITEM* aItem )
                         {
                             commit.Remove( aItem );
                         } );

            commit.Remove( board_item );
            break;

        case PCB_PAD_T:
            if( IsFootprintEditor() || frame()->GetPcbNewSettings()->m_AllowFreePads )
            {
                commit.Modify( parentFP );
                getView()->Remove( board_item );
                parentFP->Remove( board_item );
            }

            break;

        case PCB_ZONE_T:
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
                    commit.Modify( zone );
                    zone->RemoveCutout( outlineIdx, holeIdx );
                    zone->UnFill();

                    // TODO Refill zone when KiCad supports auto re-fill

                    // Update the display
                    zone->HatchBorder();
                    canvas()->Refresh();

                    // Restore the selection on the original zone
                    m_toolMgr->RunAction<EDA_ITEM*>( PCB_ACTIONS::selectItem, zone );

                    break;
                }
            }

            // Remove the entire zone otherwise
            commit.Remove( board_item );
            break;

        case PCB_GENERATOR_T:
            if( rootItems.size() == 1 )
            {
                PCB_GENERATOR* generator = static_cast<PCB_GENERATOR*>( board_item );

                m_toolMgr->RunSynchronousAction<PCB_GENERATOR*>( PCB_ACTIONS::genRemove, &commit,
                                                                 generator );
            }
            else
            {
                PCB_GENERATOR* generator = static_cast<PCB_GENERATOR*>( board_item );

                for( BOARD_ITEM* member : generator->GetItems() )
                    commit.Stage( member, CHT_UNGROUP );

                for( BOARD_ITEM* member : generator->GetItems() )
                    commit.Remove( member );

                commit.Remove( board_item );
            }

            break;

        default:
            wxASSERT_MSG( parentFP == nullptr, wxT( "Try to delete an item living in a footprint" ) );
            commit.Remove( board_item );
            break;
        }
    }

    // If the entered group has been emptied then leave it.
    PCB_GROUP* enteredGroup = m_selectionTool->GetEnteredGroup();

    if( enteredGroup && enteredGroup->GetItems().empty() )
        m_selectionTool->ExitGroup();

    if( aIsCut )
        commit.Push( _( "Cut" ) );
    else
        commit.Push( _( "Delete" ) );
}


int EDIT_TOOL::Remove( const TOOL_EVENT& aEvent )
{
    PCB_BASE_EDIT_FRAME*  editFrame = getEditFrame<PCB_BASE_EDIT_FRAME>();

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
            m_toolMgr->RunAction( PCB_ACTIONS::selectConnection );
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
    canvas()->Refresh();

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
        BOARD_COMMIT commit( this );
        EDA_ANGLE    angle = rotation;
        VECTOR2I     rp = selection.GetCenter();
        VECTOR2I     selCenter( rp.x, rp.y );

        // Make sure the rotation is from the right reference point
        selCenter += translation;

        if( !frame()->GetPcbNewSettings()->m_Display.m_DisplayInvertYAxis )
            rotation = -rotation;

        for( EDA_ITEM* item : selection )
        {
            if( !item->IsBOARD_ITEM() )
                continue;

            BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( item );

            if( !boardItem->IsNew() )
                commit.Modify( boardItem );

            if( !boardItem->GetParent() || !boardItem->GetParent()->IsSelected() )
                boardItem->Move( translation );

            switch( rotationAnchor )
            {
            case ROTATE_AROUND_ITEM_ANCHOR:
                boardItem->Rotate( boardItem->GetPosition(), angle );
                break;
            case ROTATE_AROUND_SEL_CENTER:
                boardItem->Rotate( selCenter, angle );
                break;
            case ROTATE_AROUND_USER_ORIGIN:
                boardItem->Rotate( frame()->GetScreen()->m_LocalOrigin, angle );
                break;
            case ROTATE_AROUND_AUX_ORIGIN:
                boardItem->Rotate( board()->GetDesignSettings().GetAuxOrigin(), angle );
                break;
            }

            if( !m_dragging )
                getView()->Update( boardItem );
        }

        commit.Push( _( "Move exact" ) );

        if( selection.IsHover() )
            m_toolMgr->RunAction( PCB_ACTIONS::selectionClear );

        m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );

        if( m_dragging )
            m_toolMgr->PostAction( PCB_ACTIONS::updateLocalRatsnest, VECTOR2I() );
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
                    sTool->FilterCollectorForFreePads( aCollector, true );
                    sTool->FilterCollectorForMarkers( aCollector );
                    sTool->FilterCollectorForHierarchy( aCollector, true );
                } );

    if( selection.Empty() )
        return 0;

    // Duplicating tuhing patterns alone is not supported
    if( selection.Size() == 1 && selection.CountType( PCB_GENERATOR_T ) )
        return 0;

    // we have a selection to work on now, so start the tool process
    PCB_BASE_EDIT_FRAME* editFrame = getEditFrame<PCB_BASE_EDIT_FRAME>();
    BOARD_COMMIT         commit( this );
    FOOTPRINT*           parentFootprint = nullptr;

    if( m_isFootprintEditor )
        parentFootprint = editFrame->GetBoard()->GetFirstFootprint();

    // If the selection was given a hover, we do not keep the selection after completion
    bool is_hover = selection.IsHover();

    std::vector<BOARD_ITEM*> new_items;
    new_items.reserve( selection.Size() );

    // Each selected item is duplicated and pushed to new_items list
    // Old selection is cleared, and new items are then selected.
    for( EDA_ITEM* item : selection )
    {
        if( !item->IsBOARD_ITEM() )
            continue;

        BOARD_ITEM* dupe_item = nullptr;
        BOARD_ITEM* orig_item = static_cast<BOARD_ITEM*>( item );

        if( !m_isFootprintEditor
                && /*FOOTPRINT* parentFootprint =*/ orig_item->GetParentFootprint() )
        {
            // No sub-footprint modifications allowed outside of footprint editor
            // and parentFootprint is not (yet?) used
        }
        else
        {
            switch( orig_item->Type() )
            {
            case PCB_FOOTPRINT_T:
            case PCB_TEXT_T:
            case PCB_TEXTBOX_T:
            case PCB_REFERENCE_IMAGE_T:
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
                if( m_isFootprintEditor )
                    dupe_item = parentFootprint->DuplicateItem( orig_item );
                else
                    dupe_item = orig_item->Duplicate();

                // Clear the selection flag here, otherwise the PCB_SELECTION_TOOL
                // will not properly select it later on
                dupe_item->ClearSelected();

                new_items.push_back( dupe_item );
                commit.Add( dupe_item );
                break;

            case PCB_FIELD_T:
                // PCB_FIELD items are specific items (not only graphic, but are properies)
                // and cannot be duplicated like other footprint items. So skip it:
                orig_item->ClearSelected();
                break;

            case PCB_PAD_T:
                dupe_item = parentFootprint->DuplicateItem( orig_item );

                if( increment && static_cast<PAD*>( dupe_item )->CanHaveNumber() )
                {
                    PAD_TOOL* padTool = m_toolMgr->GetTool<PAD_TOOL>();
                    wxString padNumber = padTool->GetLastPadNumber();
                    padNumber = parentFootprint->GetNextPadNumber( padNumber );
                    padTool->SetLastPadNumber( padNumber );
                    static_cast<PAD*>( dupe_item )->SetNumber( padNumber );
                }

                // Clear the selection flag here, otherwise the PCB_SELECTION_TOOL
                // will not properly select it later on
                dupe_item->ClearSelected();

                new_items.push_back( dupe_item );
                commit.Add( dupe_item );
                break;

            case PCB_GENERATOR_T:
            case PCB_GROUP_T:
                dupe_item = static_cast<PCB_GROUP*>( orig_item )->DeepDuplicate();

                dupe_item->RunOnDescendants(
                        [&]( BOARD_ITEM* aItem )
                        {
                            aItem->ClearSelected();
                            new_items.push_back( aItem );
                            commit.Add( aItem );
                        } );

                dupe_item->ClearSelected();
                new_items.push_back( dupe_item );
                commit.Add( dupe_item );
                break;

            default:
                wxASSERT_MSG( false, wxString::Format( wxT( "Unhandled item type %d" ),
                                                       orig_item->Type() ) );
                break;
            }
        }
    }

    // Clear the old selection first
    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear );

    // Select the new items
    EDA_ITEMS nItems( new_items.begin(), new_items.end() );
    m_toolMgr->RunAction<EDA_ITEMS*>( PCB_ACTIONS::selectItems, &nItems );

    // record the new items as added
    if( !selection.Empty() )
    {
        editFrame->DisplayToolMsg( wxString::Format( _( "Duplicated %d item(s)" ),
                                                     (int) new_items.size() ) );

        // If items were duplicated, pick them up
        if( doMoveSelection( aEvent, &commit, true ) )
            commit.Push( _( "Duplicate" ) );
        else
            commit.Revert();

        // Deselect the duplicated item if we originally started as a hover selection
        if( is_hover )
            m_toolMgr->RunAction( PCB_ACTIONS::selectionClear );
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
        if( aCollector[i]->Type() != PCB_PAD_T )
            aCollector.Remove( i );
    }
}


void EDIT_TOOL::FootprintFilter( const VECTOR2I&, GENERAL_COLLECTOR& aCollector,
                                 PCB_SELECTION_TOOL* sTool )
{
    for( int i = aCollector.GetCount() - 1; i >= 0; i-- )
    {
        if( aCollector[i]->Type() != PCB_FOOTPRINT_T )
            aCollector.Remove( i );
    }
}


bool EDIT_TOOL::updateModificationPoint( PCB_SELECTION& aSelection )
{
    // Can't modify an empty group
    if( aSelection.Empty() )
        return false;

    if( ( m_dragging || aSelection[0]->IsMoving() ) && aSelection.HasReferencePoint() )
        return false;

    // When there is only one item selected, the reference point is its position...
    if( aSelection.Size() == 1 )
    {
        if( aSelection.Front()->IsBOARD_ITEM() )
        {
            BOARD_ITEM* item = static_cast<BOARD_ITEM*>( aSelection.Front() );
            aSelection.SetReferencePoint( item->GetPosition() );
        }
    }
    // ...otherwise modify items with regard to the grid-snapped center position
    else
    {
        PCB_GRID_HELPER grid( m_toolMgr, frame()->GetMagneticItemsSettings() );
        VECTOR2I        refPt = aSelection.GetCenter();

        // Exclude text in the footprint editor if there's anything else selected
        if( m_isFootprintEditor )
        {
            BOX2I nonFieldsBBox;

            for( EDA_ITEM* item : aSelection.Items() )
            {
                if( !item->IsType( { PCB_TEXT_T, PCB_FIELD_T } )  )
                    nonFieldsBBox.Merge( item->GetBoundingBox() );
            }

            if( nonFieldsBBox.IsValid() )
                refPt = nonFieldsBBox.GetCenter();
        }

        aSelection.SetReferencePoint( grid.BestSnapAnchor( refPt, nullptr ) );
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
                m_statusPopup->Move( KIPLATFORM::UI::GetMousePosition() + wxPoint( 20, -50 ) );
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

    m_statusPopup->Move( KIPLATFORM::UI::GetMousePosition() + wxPoint( 20, -50 ) );
    m_statusPopup->Popup();
    canvas()->SetStatusPopup( m_statusPopup->GetPanel() );

    m_toolMgr->RunAction( ACTIONS::pickerSubTool );

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
                    if( ( item->Type() == PCB_FIELD_T || item->Type() == PCB_TEXT_T )
                        && aCollector.HasItem( item->GetParentFootprint() ) )
                    {
                        aCollector.Remove( item );
                    }
                    else if( item->Type() == PCB_MARKER_T )
                    {
                        // Don't allow copying marker objects
                        aCollector.Remove( item );
                    }
                }
            },

            // Prompt user regarding locked items.
            aEvent.IsAction( &ACTIONS::cut ) && !m_isFootprintEditor );

    if( !selection.Empty() )
    {
        std::vector<BOARD_ITEM*> items;

        for( EDA_ITEM* item : selection )
        {
            if( item->IsBOARD_ITEM()  )
                items.push_back( static_cast<BOARD_ITEM*>( item ) );
        }

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
    Go( &EDIT_TOOL::ModifyLines,           PCB_ACTIONS::filletLines.MakeEvent() );
    Go( &EDIT_TOOL::ModifyLines,           PCB_ACTIONS::chamferLines.MakeEvent() );
    Go( &EDIT_TOOL::HealShapes,            PCB_ACTIONS::healShapes.MakeEvent() );
    Go( &EDIT_TOOL::ModifyLines,           PCB_ACTIONS::extendLines.MakeEvent() );
    Go( &EDIT_TOOL::BooleanPolygons,       PCB_ACTIONS::mergePolygons.MakeEvent() );
    Go( &EDIT_TOOL::BooleanPolygons,       PCB_ACTIONS::subtractPolygons.MakeEvent() );
    Go( &EDIT_TOOL::BooleanPolygons,       PCB_ACTIONS::intersectPolygons.MakeEvent() );

    Go( &EDIT_TOOL::copyToClipboard,       ACTIONS::copy.MakeEvent() );
    Go( &EDIT_TOOL::copyToClipboard,       PCB_ACTIONS::copyWithReference.MakeEvent() );
    Go( &EDIT_TOOL::cutToClipboard,        ACTIONS::cut.MakeEvent() );
}
