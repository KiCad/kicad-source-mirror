/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright (C) 2014-2017 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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
#include <cstdint>

#include "pcb_editor_control.h"
#include "pcb_actions.h"
#include <tool/tool_manager.h>

#include "selection_tool.h"
#include "drawing_tool.h"
#include "picker_tool.h"

#include <painter.h>
#include <project.h>
#include <pcbnew_id.h>
#include <wxPcbStruct.h>
#include <class_board.h>
#include <class_zone.h>
#include <class_draw_panel_gal.h>
#include <class_module.h>
#include <class_mire.h>
#include <ratsnest_data.h>
#include <ratsnest_data.h>
#include <collectors.h>
#include <zones_functions_for_undo_redo.h>
#include <board_commit.h>
#include <confirm.h>
#include <bitmaps.h>
#include <hotkeys.h>

#include <view/view_group.h>
#include <view/view_controls.h>
#include <origin_viewitem.h>

#include <tools/tool_event_utils.h>

#include <functional>
using namespace std::placeholders;


// Track & via size control
TOOL_ACTION PCB_ACTIONS::trackWidthInc( "pcbnew.EditorControl.trackWidthInc",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_SWITCH_TRACK_WIDTH_TO_NEXT ),
        "", "" );

TOOL_ACTION PCB_ACTIONS::trackWidthDec( "pcbnew.EditorControl.trackWidthDec",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_SWITCH_TRACK_WIDTH_TO_PREVIOUS ),
        "", "" );

TOOL_ACTION PCB_ACTIONS::viaSizeInc( "pcbnew.EditorControl.viaSizeInc",
        AS_GLOBAL, '\'',
        "", "" );

TOOL_ACTION PCB_ACTIONS::viaSizeDec( "pcbnew.EditorControl.viaSizeDec",
        AS_GLOBAL, '\\',
        "", "" );

TOOL_ACTION PCB_ACTIONS::trackViaSizeChanged( "pcbnew.EditorControl.trackViaSizeChanged",
        AS_GLOBAL, 0,
        "", "", NULL, AF_NOTIFY );


// Zone actions
TOOL_ACTION PCB_ACTIONS::zoneFill( "pcbnew.EditorControl.zoneFill",
        AS_GLOBAL, 0,
        _( "Fill" ), _( "Fill zone(s)" ), fill_zone_xpm );

TOOL_ACTION PCB_ACTIONS::zoneFillAll( "pcbnew.EditorControl.zoneFillAll",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ZONE_FILL_OR_REFILL ),
        _( "Fill All" ), _( "Fill all zones" ) );

TOOL_ACTION PCB_ACTIONS::zoneUnfill( "pcbnew.EditorControl.zoneUnfill",
        AS_GLOBAL, 0,
        _( "Unfill" ), _( "Unfill zone(s)" ), zone_unfill_xpm );

TOOL_ACTION PCB_ACTIONS::zoneUnfillAll( "pcbnew.EditorControl.zoneUnfillAll",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ZONE_REMOVE_FILLED ),
        _( "Unfill All" ), _( "Unfill all zones" ) );

TOOL_ACTION PCB_ACTIONS::zoneMerge( "pcbnew.EditorControl.zoneMerge",
        AS_GLOBAL, 0,
        _( "Merge Zones" ), _( "Merge zones" ) );

TOOL_ACTION PCB_ACTIONS::zoneDuplicate( "pcbnew.EditorControl.zoneDuplicate",
        AS_GLOBAL, 0,
        _( "Duplicate Zone onto Layer" ), _( "Duplicate zone outline onto a different layer" ),
        zone_duplicate_xpm );


TOOL_ACTION PCB_ACTIONS::placeTarget( "pcbnew.EditorControl.placeTarget",
        AS_GLOBAL, 0,
        _( "Add Layer Alignment Target" ), _( "Add a layer alignment target" ), NULL, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::placeModule( "pcbnew.EditorControl.placeModule",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ADD_MODULE ),
        _( "Add Footprint" ), _( "Add a footprint" ), NULL, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::drillOrigin( "pcbnew.EditorControl.drillOrigin",
        AS_GLOBAL, 0,
        "", "" );

TOOL_ACTION PCB_ACTIONS::crossProbeSchToPcb( "pcbnew.EditorControl.crossProbSchToPcb",
        AS_GLOBAL, 0,
        "", "" );

TOOL_ACTION PCB_ACTIONS::toggleLock( "pcbnew.EditorControl.toggleLock",
        AS_GLOBAL, 'L',
        "Toggle Lock", "" );

TOOL_ACTION PCB_ACTIONS::lock( "pcbnew.EditorControl.lock",
        AS_GLOBAL, 0,
        _( "Lock" ), "" );

TOOL_ACTION PCB_ACTIONS::unlock( "pcbnew.EditorControl.unlock",
        AS_GLOBAL, 0,
        _( "Unlock" ), "" );

TOOL_ACTION PCB_ACTIONS::appendBoard( "pcbnew.EditorControl.appendBoard",
        AS_GLOBAL, 0,
        "", "" );

TOOL_ACTION PCB_ACTIONS::highlightNet( "pcbnew.EditorControl.highlightNet",
        AS_GLOBAL, 0,
        "", "" );

TOOL_ACTION PCB_ACTIONS::highlightNetCursor( "pcbnew.EditorControl.highlightNetCursor",
        AS_GLOBAL, 0,
        "", "" );


class ZONE_CONTEXT_MENU : public CONTEXT_MENU
{
public:
    ZONE_CONTEXT_MENU()
    {
        SetIcon( add_zone_xpm );
        SetTitle( _( "Zones" ) );

        Add( PCB_ACTIONS::zoneFill );
        Add( PCB_ACTIONS::zoneFillAll );
        Add( PCB_ACTIONS::zoneUnfill );
        Add( PCB_ACTIONS::zoneUnfillAll );

        AppendSeparator();

        Add( PCB_ACTIONS::zoneMerge );
        Add( PCB_ACTIONS::zoneDuplicate );
        Add( PCB_ACTIONS::drawZoneCutout );
        Add( PCB_ACTIONS::drawSimilarZone );
    }

protected:
    CONTEXT_MENU* create() const override
    {
        return new ZONE_CONTEXT_MENU();
    }

private:
    void update() override
    {
        SELECTION_TOOL* selTool = getToolManager()->GetTool<SELECTION_TOOL>();

        // enable zone actions that act on a single zone
        bool singleZoneActionsEnabled = ( SELECTION_CONDITIONS::Count( 1 )
                                          && SELECTION_CONDITIONS::OnlyType( PCB_ZONE_AREA_T )
                                        )( selTool->GetSelection() );

        Enable( getMenuId( PCB_ACTIONS::zoneDuplicate ), singleZoneActionsEnabled );
        Enable( getMenuId( PCB_ACTIONS::drawZoneCutout ), singleZoneActionsEnabled );
        Enable( getMenuId( PCB_ACTIONS::drawSimilarZone ), singleZoneActionsEnabled );

        // enable zone actions that ably to a specific set of zones (as opposed to all of them)
        bool nonGlobalActionsEnabled = ( SELECTION_CONDITIONS::MoreThan( 0 ) )( selTool->GetSelection() );

        Enable( getMenuId( PCB_ACTIONS::zoneFill ), nonGlobalActionsEnabled );
        Enable( getMenuId( PCB_ACTIONS::zoneUnfill ), nonGlobalActionsEnabled );

        // lines like this make me really think about a better name for SELECTION_CONDITIONS class
        bool mergeEnabled = ( SELECTION_CONDITIONS::MoreThan( 1 ) &&
                              /*SELECTION_CONDITIONS::OnlyType( PCB_ZONE_AREA_T ) &&*/
                              PCB_SELECTION_CONDITIONS::SameNet( true ) &&
                              PCB_SELECTION_CONDITIONS::SameLayer() )( selTool->GetSelection() );

        Enable( getMenuId( PCB_ACTIONS::zoneMerge ), mergeEnabled );
    }
};


class LOCK_CONTEXT_MENU : public CONTEXT_MENU
{
public:
    LOCK_CONTEXT_MENU()
    {
        SetIcon( locked_xpm );
        SetTitle( _( "Locking" ) );

        Add( PCB_ACTIONS::lock );
        Add( PCB_ACTIONS::unlock );
        Add( PCB_ACTIONS::toggleLock );
    }

    CONTEXT_MENU* create() const override
    {
        return new LOCK_CONTEXT_MENU();
    }
};


PCB_EDITOR_CONTROL::PCB_EDITOR_CONTROL() :
    PCB_TOOL( "pcbnew.EditorControl" ),
    m_frame( nullptr )
{
    m_placeOrigin.reset( new KIGFX::ORIGIN_VIEWITEM( KIGFX::COLOR4D( 0.8, 0.0, 0.0, 1.0 ),
                                                KIGFX::ORIGIN_VIEWITEM::CIRCLE_CROSS ) );
    m_probingSchToPcb = false;
}


PCB_EDITOR_CONTROL::~PCB_EDITOR_CONTROL()
{
}


void PCB_EDITOR_CONTROL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<PCB_EDIT_FRAME>();

    if( aReason == MODEL_RELOAD || aReason == GAL_SWITCH )
    {
        m_placeOrigin->SetPosition( getModel<BOARD>()->GetAuxOrigin() );
        getView()->Remove( m_placeOrigin.get() );
        getView()->Add( m_placeOrigin.get() );
    }
}


bool PCB_EDITOR_CONTROL::Init()
{
    auto zoneMenu = std::make_shared<ZONE_CONTEXT_MENU>();
    zoneMenu->SetTool( this );

    auto lockMenu = std::make_shared<LOCK_CONTEXT_MENU>();
    lockMenu->SetTool( this );

    // Add the PCB control menus to relevant other tools

    SELECTION_TOOL* selTool = m_toolMgr->GetTool<SELECTION_TOOL>();

    if( selTool )
    {
        auto& toolMenu = selTool->GetToolMenu();
        auto& menu = toolMenu.GetMenu();

        toolMenu.AddSubMenu( zoneMenu );
        toolMenu.AddSubMenu( lockMenu );

        menu.AddMenu( zoneMenu.get(), false,
                SELECTION_CONDITIONS::OnlyType( PCB_ZONE_AREA_T ) );

        menu.AddMenu( lockMenu.get(), false,
                SELECTION_CONDITIONS::OnlyTypes( GENERAL_COLLECTOR::Tracks ) );
    }

    DRAWING_TOOL* drawingTool = m_toolMgr->GetTool<DRAWING_TOOL>();

    if( drawingTool )
    {
        auto& toolMenu = drawingTool->GetToolMenu();
        auto& menu = toolMenu.GetMenu();

        toolMenu.AddSubMenu( zoneMenu );

        // Functor to say if the PCB_EDIT_FRAME is in a given mode
        // Capture the tool pointer and tool mode by value
        auto toolActiveFunctor = [=]( DRAWING_TOOL::MODE aMode )
        {
            return [=]( const SELECTION& sel )
            {
                return drawingTool->GetDrawingMode() == aMode;
            };
        };

        menu.AddMenu( zoneMenu.get(), false, toolActiveFunctor( DRAWING_TOOL::MODE::ZONE ) );
    }

    return true;
}


// Track & via size control
int PCB_EDITOR_CONTROL::TrackWidthInc( const TOOL_EVENT& aEvent )
{
    BOARD* board = getModel<BOARD>();
    int widthIndex = board->GetDesignSettings().GetTrackWidthIndex() + 1;

    if( widthIndex >= (int) board->GetDesignSettings().m_TrackWidthList.size() )
        widthIndex = board->GetDesignSettings().m_TrackWidthList.size() - 1;

    board->GetDesignSettings().SetTrackWidthIndex( widthIndex );
    board->GetDesignSettings().UseCustomTrackViaSize( false );

    m_toolMgr->RunAction( PCB_ACTIONS::trackViaSizeChanged );

    return 0;
}


int PCB_EDITOR_CONTROL::TrackWidthDec( const TOOL_EVENT& aEvent )
{
    BOARD* board = getModel<BOARD>();
    int widthIndex = board->GetDesignSettings().GetTrackWidthIndex() - 1;

    if( widthIndex < 0 )
        widthIndex = 0;

    board->GetDesignSettings().SetTrackWidthIndex( widthIndex );
    board->GetDesignSettings().UseCustomTrackViaSize( false );

    m_toolMgr->RunAction( PCB_ACTIONS::trackViaSizeChanged );

    return 0;
}


int PCB_EDITOR_CONTROL::ViaSizeInc( const TOOL_EVENT& aEvent )
{
    BOARD* board = getModel<BOARD>();
    int sizeIndex = board->GetDesignSettings().GetViaSizeIndex() + 1;

    if( sizeIndex >= (int) board->GetDesignSettings().m_ViasDimensionsList.size() )
        sizeIndex = board->GetDesignSettings().m_ViasDimensionsList.size() - 1;

    board->GetDesignSettings().SetViaSizeIndex( sizeIndex );
    board->GetDesignSettings().UseCustomTrackViaSize( false );

    m_toolMgr->RunAction( PCB_ACTIONS::trackViaSizeChanged );

    return 0;
}


int PCB_EDITOR_CONTROL::ViaSizeDec( const TOOL_EVENT& aEvent )
{
    BOARD* board = getModel<BOARD>();
    int sizeIndex = board->GetDesignSettings().GetViaSizeIndex() - 1;

    if( sizeIndex < 0 )
        sizeIndex = 0;

    board->GetDesignSettings().SetViaSizeIndex( sizeIndex );
    board->GetDesignSettings().UseCustomTrackViaSize( false );

    m_toolMgr->RunAction( PCB_ACTIONS::trackViaSizeChanged );

    return 0;
}


int PCB_EDITOR_CONTROL::PlaceModule( const TOOL_EVENT& aEvent )
{
    MODULE* module = NULL;
    KIGFX::VIEW* view = getView();
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    BOARD* board = getModel<BOARD>();

    // Add a VIEW_GROUP that serves as a preview for the new item
    KIGFX::VIEW_GROUP preview( view );
    view->Add( &preview );

    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
    controls->ShowCursor( true );
    controls->SetSnapping( true );

    Activate();
    m_frame->SetToolID( ID_PCB_MODULE_BUTT, wxCURSOR_HAND, _( "Add footprint" ) );

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        VECTOR2I cursorPos = controls->GetCursorPosition();

        if( evt->IsCancel() || evt->IsActivate() )
        {
            if( module )
            {
                delete module;
                module = NULL;

                preview.Clear();
                controls->ShowCursor( true );
            }
            else
                break;

            if( evt->IsActivate() )  // now finish unconditionally
                break;
        }

        else if( module && evt->Category() == TC_COMMAND )
        {
            if( TOOL_EVT_UTILS::IsRotateToolEvt( *evt ) )
            {
                const auto rotationAngle = TOOL_EVT_UTILS::GetEventRotationAngle(
                        *m_frame, *evt );
                module->Rotate( module->GetPosition(), rotationAngle );
                view->Update( &preview );
            }
            else if( evt->IsAction( &PCB_ACTIONS::flip ) )
            {
                module->Flip( module->GetPosition() );
                view->Update( &preview );
            }
        }

        else if( evt->IsClick( BUT_LEFT ) )
        {
            if( !module )
            {
                // Pick the module to be placed
                module = m_frame->LoadModuleFromLibrary( wxEmptyString,
                                                         m_frame->Prj().PcbFootprintLibs(),
                                                         true, NULL );

                if( module == NULL )
                    continue;

                // Module has been added in LoadModuleFromLibrary(),
                // so we have to remove it before committing the change     @todo LEGACY
                board->Remove( module );
                module->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );

                // Add all the drawable parts to preview
                preview.Add( module );
                module->RunOnChildren( std::bind( &KIGFX::VIEW_GROUP::Add, &preview, _1 ) );
            }
            else
            {
                BOARD_COMMIT commit( m_frame );
                commit.Add( module );
                commit.Push( _( "Place a module" ) );

                // Remove from preview
                preview.Remove( module );
                module->RunOnChildren( std::bind( &KIGFX::VIEW_GROUP::Remove, &preview, _1 ) );
                module = NULL;  // to indicate that there is no module that we currently modify
            }

            bool placing = ( module != NULL );

            controls->SetAutoPan( placing );
            controls->CaptureCursor( placing );
            controls->ShowCursor( !placing );
        }

        else if( module && evt->IsMotion() )
        {
            module->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );
            view->Update( &preview );
        }
    }

    view->Remove( &preview );
    m_frame->SetToolID( ID_NO_TOOL_SELECTED, wxCURSOR_DEFAULT, wxEmptyString );

    return 0;
}


int PCB_EDITOR_CONTROL::ToggleLockSelected( const TOOL_EVENT& aEvent )
{
    return modifyLockSelected( TOGGLE );
}


int PCB_EDITOR_CONTROL::LockSelected( const TOOL_EVENT& aEvent )
{
    return modifyLockSelected( ON );
}


int PCB_EDITOR_CONTROL::UnlockSelected( const TOOL_EVENT& aEvent )
{
    return modifyLockSelected( OFF );
}


int PCB_EDITOR_CONTROL::modifyLockSelected( MODIFY_MODE aMode )
{
    SELECTION_TOOL* selTool = m_toolMgr->GetTool<SELECTION_TOOL>();
    const SELECTION& selection = selTool->GetSelection();

    if( selection.Empty() )
        m_toolMgr->RunAction( PCB_ACTIONS::selectionCursor, true );

    bool modified = false;

    for( auto i : selection )
    {
        auto item = static_cast<BOARD_ITEM*>( i );
        bool prevState = item->IsLocked();

        switch( aMode )
        {
            case ON:
                item->SetLocked( true );
                break;

            case OFF:
                item->SetLocked( false );
                break;

            case TOGGLE:
                item->SetLocked( !prevState );
                break;
        }

        // Check if we really modified an item
        if( !modified && prevState != item->IsLocked() )
            modified = true;
    }

    if( modified )
        m_frame->OnModify();

    return 0;
}


int PCB_EDITOR_CONTROL::PlaceTarget( const TOOL_EVENT& aEvent )
{
    KIGFX::VIEW* view = getView();
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    BOARD* board = getModel<BOARD>();
    PCB_TARGET* target = new PCB_TARGET( board );

    // Init the new item attributes
    target->SetLayer( Edge_Cuts );
    target->SetWidth( board->GetDesignSettings().m_EdgeSegmentWidth );
    target->SetSize( Millimeter2iu( 5 ) );
    VECTOR2I cursorPos = controls->GetCursorPosition();
    target->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );

    // Add a VIEW_GROUP that serves as a preview for the new item
    KIGFX::VIEW_GROUP preview( view );
    preview.Add( target );
    view->Add( &preview );

    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
    controls->SetSnapping( true );

    Activate();
    m_frame->SetToolID( ID_PCB_MIRE_BUTT, wxCURSOR_PENCIL, _( "Add layer alignment target" ) );

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        cursorPos = controls->GetCursorPosition();

        if( evt->IsCancel() || evt->IsActivate() )
            break;

        else if( evt->IsAction( &PCB_ACTIONS::incWidth ) )
        {
            target->SetWidth( target->GetWidth() + WIDTH_STEP );
            view->Update( &preview );
        }

        else if( evt->IsAction( &PCB_ACTIONS::decWidth ) )
        {
            int width = target->GetWidth();

            if( width > WIDTH_STEP )
            {
                target->SetWidth( width - WIDTH_STEP );
                view->Update( &preview );
            }
        }

        else if( evt->IsClick( BUT_LEFT ) )
        {
            assert( target->GetSize() > 0 );
            assert( target->GetWidth() > 0 );

            BOARD_COMMIT commit( m_frame );
            commit.Add( target );
            commit.Push( _( "Place a layer alignment target" ) );

            preview.Remove( target );

            // Create next PCB_TARGET
            target = new PCB_TARGET( *target );
            preview.Add( target );
        }

        else if( evt->IsMotion() )
        {
            target->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );
            view->Update( &preview );
        }
    }

    delete target;

    controls->SetSnapping( false );
    view->Remove( &preview );

    m_frame->SetToolID( ID_NO_TOOL_SELECTED, wxCURSOR_DEFAULT, wxEmptyString );

    return 0;
}


// Zone actions
int PCB_EDITOR_CONTROL::ZoneFill( const TOOL_EVENT& aEvent )
{
    auto selTool = m_toolMgr->GetTool<SELECTION_TOOL>();
    const auto& selection = selTool->GetSelection();
    RN_DATA* ratsnest = getModel<BOARD>()->GetRatsnest();

    BOARD_COMMIT commit( this );

    for( auto item : selection )
    {
        assert( item->Type() == PCB_ZONE_AREA_T );

        ZONE_CONTAINER* zone = static_cast<ZONE_CONTAINER*> ( item );

        commit.Modify( zone );

        m_frame->Fill_Zone( zone );
        zone->SetIsFilled( true );
        ratsnest->Update( zone );
        getView()->Update( zone );
    }

    commit.Push( _( "Fill Zone" ) );

    ratsnest->Recalculate();

    return 0;
}


int PCB_EDITOR_CONTROL::ZoneFillAll( const TOOL_EVENT& aEvent )
{
    BOARD* board = getModel<BOARD>();
    RN_DATA* ratsnest = board->GetRatsnest();

    BOARD_COMMIT commit( this );

    for( int i = 0; i < board->GetAreaCount(); ++i )
    {
        ZONE_CONTAINER* zone = board->GetArea( i );

        commit.Modify( zone );

        m_frame->Fill_Zone( zone );
        zone->SetIsFilled( true );
        ratsnest->Update( zone );
        getView()->Update( zone );
    }

    commit.Push( _( "Fill All Zones" ) );

    ratsnest->Recalculate();

    return 0;
}


int PCB_EDITOR_CONTROL::ZoneUnfill( const TOOL_EVENT& aEvent )
{
    auto selTool = m_toolMgr->GetTool<SELECTION_TOOL>();
    const auto& selection = selTool->GetSelection();
    RN_DATA* ratsnest = getModel<BOARD>()->GetRatsnest();

    BOARD_COMMIT commit( this );

    for( auto item : selection )
    {
        assert( item->Type() == PCB_ZONE_AREA_T );

        ZONE_CONTAINER* zone = static_cast<ZONE_CONTAINER*>( item );

        commit.Modify( zone );

        zone->SetIsFilled( false );
        zone->ClearFilledPolysList();
        ratsnest->Update( zone );
        getView()->Update( zone );
    }

    commit.Push( _( "Unfill Zone" ) );

    ratsnest->Recalculate();

    return 0;
}


int PCB_EDITOR_CONTROL::ZoneUnfillAll( const TOOL_EVENT& aEvent )
{
    BOARD* board = getModel<BOARD>();
    RN_DATA* ratsnest = board->GetRatsnest();

    BOARD_COMMIT commit( this );

    for( int i = 0; i < board->GetAreaCount(); ++i )
    {
        ZONE_CONTAINER* zone = board->GetArea( i );

        commit.Modify( zone );

        zone->SetIsFilled( false );
        zone->ClearFilledPolysList();
        ratsnest->Update( zone );
        getView()->Update( zone );
    }

    commit.Push( _( "Unfill All Zones" ) );

    ratsnest->Recalculate();

    return 0;
}


static bool mergeZones( BOARD_COMMIT& aCommit, std::vector<ZONE_CONTAINER *>& aOriginZones,
        std::vector<ZONE_CONTAINER *>& aMergedZones )
{
    SHAPE_POLY_SET mergedOutlines = ConvertPolyListToPolySet( aOriginZones[0]->Outline()->m_CornersList );

    for( unsigned int i = 1; i < aOriginZones.size(); i++ )
    {
        SHAPE_POLY_SET areaToMergePoly = ConvertPolyListToPolySet( aOriginZones[i]->Outline()->m_CornersList );

        mergedOutlines.BooleanAdd( areaToMergePoly, SHAPE_POLY_SET::PM_FAST  );
    }

    mergedOutlines.Simplify( SHAPE_POLY_SET::PM_FAST );

    // We should have one polygon with hole
    // We can have 2 polygons with hole, if the 2 initial polygons have only one common corner
    // and therefore cannot be merged (they are dectected as intersecting)
    // but we should never have more than 2 polys
    if( mergedOutlines.OutlineCount() > 1 )
    {
        wxLogMessage( wxT( "BOARD::CombineAreas error: more than 2 polys after merging" ) );
        return false;
    }

    for( unsigned int i = 1; i < aOriginZones.size(); i++ )
    {
        aCommit.Remove( aOriginZones[i] );
    }

    aCommit.Modify( aOriginZones[0] );
    aMergedZones.push_back( aOriginZones[0] );

    aOriginZones[0]->Outline()->m_CornersList = ConvertPolySetToPolyList( mergedOutlines );
    aOriginZones[0]->SetLocalFlags( 1 );
    aOriginZones[0]->Outline()->Hatch();

    return true;
}

int PCB_EDITOR_CONTROL::ZoneMerge( const TOOL_EVENT& aEvent )
{
    const SELECTION& selection = m_toolMgr->GetTool<SELECTION_TOOL>()->GetSelection();
    BOARD* board = getModel<BOARD>();
    BOARD_COMMIT commit( m_frame );

    if( selection.Size() < 2 )
        return 0;

    int netcode = -1;

    ZONE_CONTAINER* firstZone = nullptr;
    std::vector<ZONE_CONTAINER*> toMerge, merged;

    for( auto item : selection )
    {
        auto curr_area = dynamic_cast<ZONE_CONTAINER*>( item );

        if( !curr_area )
            continue;

        if( !firstZone )
            firstZone = curr_area;

        netcode = curr_area->GetNetCode();

        if( firstZone )
        {
            if( firstZone->GetNetCode() != netcode )
                continue;

            if( curr_area->GetPriority() != firstZone->GetPriority() )
                continue;

            if( curr_area->GetIsKeepout() != firstZone->GetIsKeepout() )
                continue;

            if( curr_area->GetLayer() != firstZone->GetLayer() )
                continue;

            if( !board->TestAreaIntersection( curr_area, firstZone ) )
                continue;

            toMerge.push_back( curr_area );
        }
        else
        {
            toMerge.push_back( curr_area );
        }
    }

    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    if( mergeZones( commit, toMerge, merged ) )
    {
        commit.Push( _( "Merge zones" ) );

        for( auto item : merged )
            m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, item );
    }

    return 0;
}


int PCB_EDITOR_CONTROL::ZoneDuplicate( const TOOL_EVENT& aEvent )
{
    auto selTool = m_toolMgr->GetTool<SELECTION_TOOL>();
    const auto& selection = selTool->GetSelection();

    // because this pops up the zone editor, it would be confusing
    // to handle multiple zones, so just handle single selections
    // containing exactly one zone
    if( selection.Size() != 1 )
        return 0;

    auto oldZone = dyn_cast<ZONE_CONTAINER*>( selection[0] );

    if( !oldZone )
        return 0;

    auto newZone = std::make_unique<ZONE_CONTAINER>( *oldZone );
    newZone->ClearSelected();
    newZone->UnFill();
    ZONE_SETTINGS zoneSettings;
    zoneSettings << *oldZone;

    bool success = false;

    if( oldZone->GetIsKeepout() )
        success = InvokeKeepoutAreaEditor( m_frame, &zoneSettings );
    else if( oldZone->IsOnCopperLayer() )
        success = InvokeCopperZonesEditor( m_frame, &zoneSettings );
    else
        success = InvokeNonCopperZonesEditor( m_frame, oldZone, &zoneSettings );

    // If the new zone is on the same layer as the the initial zone,
    // do nothing
    if( success && ( oldZone->GetLayer() == zoneSettings.m_CurrentZone_Layer ) )
    {
        DisplayError( m_frame,
            _( "The duplicated zone cannot be on the same layer as the original zone." ) );
        success = false;
    }

    // duplicate the zone
    if( success )
    {
        BOARD_COMMIT commit( m_frame );
        zoneSettings.ExportSetting( *newZone );

        commit.Add( newZone.release() );
        commit.Push( _( "Duplicate zone" ) );
    }

    return 0;
}


int PCB_EDITOR_CONTROL::CrossProbePcbToSch( const TOOL_EVENT& aEvent )
{
    if( m_probingSchToPcb )
    {
        m_probingSchToPcb = false;
        return 0;
    }

    SELECTION_TOOL* selTool = m_toolMgr->GetTool<SELECTION_TOOL>();
    const SELECTION& selection = selTool->GetSelection();

    if( selection.Size() == 1 )
        m_frame->SendMessageToEESCHEMA( static_cast<BOARD_ITEM*>( selection.Front() ) );

    return 0;
}


int PCB_EDITOR_CONTROL::CrossProbeSchToPcb( const TOOL_EVENT& aEvent )
{
    BOARD_ITEM* item = aEvent.Parameter<BOARD_ITEM*>();

    if( item )
    {
        m_probingSchToPcb = true;
        getView()->SetCenter( VECTOR2D( item->GetPosition() ) );
        m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

        // If it is a pad and the net highlighting tool is enabled, highlight the net
        if( item->Type() == PCB_PAD_T && m_frame->GetToolId() == ID_PCB_HIGHLIGHT_BUTT )
        {
            int net = static_cast<D_PAD*>( item )->GetNetCode();
            m_toolMgr->RunAction( PCB_ACTIONS::highlightNet, false, net );
        }
        else
        // Otherwise simply select the corresponding item
        {
            m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, item );
        }
    }

    return 0;
}


static bool setDrillOrigin( KIGFX::VIEW* aView, PCB_BASE_FRAME* aFrame,
                            KIGFX::ORIGIN_VIEWITEM* aItem, const VECTOR2D& aPosition )
{
    aFrame->SetAuxOrigin( wxPoint( aPosition.x, aPosition.y ) );
    aItem->SetPosition( aPosition );
    aView->MarkDirty();

    return true;
}


int PCB_EDITOR_CONTROL::DrillOrigin( const TOOL_EVENT& aEvent )
{
    Activate();

    PICKER_TOOL* picker = m_toolMgr->GetTool<PICKER_TOOL>();
    assert( picker );

    m_frame->SetToolID( ID_PCB_PLACE_OFFSET_COORD_BUTT, wxCURSOR_PENCIL, _( "Adjust zero" ) );
    picker->SetClickHandler( std::bind( setDrillOrigin, getView(), m_frame, m_placeOrigin.get(), _1 ) );
    picker->Activate();
    Wait();

    return 0;
}

/**
 * Function highlightNet()
 * Looks for a BOARD_CONNECTED_ITEM in a given spot, and if one is found - it enables
 * highlight for its net.
 * @param aPoint is the point where an item is expected (world coordinates).
 */
static bool highlightNet( TOOL_MANAGER* aToolMgr, const VECTOR2D& aPosition )
{
    auto render = aToolMgr->GetView()->GetPainter()->GetSettings();
    auto frame = static_cast<PCB_EDIT_FRAME*>( aToolMgr->GetEditFrame() );
    auto guide = frame->GetCollectorsGuide();
    BOARD* board = static_cast<BOARD*>( aToolMgr->GetModel() );
    GENERAL_COLLECTOR collector;
    int net = -1;

    // Find a connected item for which we are going to highlight a net
    collector.Collect( board, GENERAL_COLLECTOR::PadsTracksOrZones,
                       wxPoint( aPosition.x, aPosition.y ), guide );

    for( int i = 0; i < collector.GetCount(); i++ )
    {
        if( collector[i]->Type() == PCB_PAD_T )
        {
            frame->SendMessageToEESCHEMA( static_cast<BOARD_CONNECTED_ITEM*>( collector[i] ) );
            break;
        }
    }

    bool enableHighlight = ( collector.GetCount() > 0 );

    // Obtain net code for the clicked item
    if( enableHighlight )
        net = static_cast<BOARD_CONNECTED_ITEM*>( collector[0] )->GetNetCode();

    // Toggle highlight when the same net was picked
    if( net > 0 && net == render->GetHighlightNetCode() )
        enableHighlight = !render->IsHighlightEnabled();

    if( enableHighlight != render->IsHighlightEnabled() || net != render->GetHighlightNetCode() )
    {
        render->SetHighlight( enableHighlight, net );
        aToolMgr->GetView()->UpdateAllLayersColor();
    }

    // Store the highlighted netcode in the current board (for dialogs for instance)
    if( enableHighlight && net >= 0 )
        board->SetHighLightNet( net );
    else
        board->ResetHighLight();

    return true;
}


int PCB_EDITOR_CONTROL::HighlightNet( const TOOL_EVENT& aEvent )
{
    int netcode = aEvent.Parameter<intptr_t>();

    if( netcode > 0 )
    {
        KIGFX::RENDER_SETTINGS* render = m_toolMgr->GetView()->GetPainter()->GetSettings();
        render->SetHighlight( true, netcode );
        m_toolMgr->GetView()->UpdateAllLayersColor();
    }
    else
    {
        // No net code specified, pick the net code belonging to the item under the cursor
        highlightNet( m_toolMgr, getView()->ToWorld( getViewControls()->GetMousePosition() ) );
    }

    return 0;
}


int PCB_EDITOR_CONTROL::HighlightNetCursor( const TOOL_EVENT& aEvent )
{
    Activate();

    PICKER_TOOL* picker = m_toolMgr->GetTool<PICKER_TOOL>();
    assert( picker );

    m_frame->SetToolID( ID_PCB_HIGHLIGHT_BUTT, wxCURSOR_PENCIL, _( "Highlight net" ) );
    picker->SetClickHandler( std::bind( highlightNet, m_toolMgr, _1 ) );
    picker->SetSnapping( false );
    picker->Activate();
    Wait();

    return 0;
}


void PCB_EDITOR_CONTROL::SetTransitions()
{
    // Track & via size control
    Go( &PCB_EDITOR_CONTROL::TrackWidthInc,      PCB_ACTIONS::trackWidthInc.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::TrackWidthDec,      PCB_ACTIONS::trackWidthDec.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::ViaSizeInc,         PCB_ACTIONS::viaSizeInc.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::ViaSizeDec,         PCB_ACTIONS::viaSizeDec.MakeEvent() );

    // Zone actions
    Go( &PCB_EDITOR_CONTROL::ZoneFill,           PCB_ACTIONS::zoneFill.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::ZoneFillAll,        PCB_ACTIONS::zoneFillAll.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::ZoneUnfill,         PCB_ACTIONS::zoneUnfill.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::ZoneUnfillAll,      PCB_ACTIONS::zoneUnfillAll.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::ZoneMerge,          PCB_ACTIONS::zoneMerge.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::ZoneDuplicate,      PCB_ACTIONS::zoneDuplicate.MakeEvent() );

    // Placing tools
    Go( &PCB_EDITOR_CONTROL::PlaceTarget,        PCB_ACTIONS::placeTarget.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::PlaceModule,        PCB_ACTIONS::placeModule.MakeEvent() );

    // Other
    Go( &PCB_EDITOR_CONTROL::ToggleLockSelected,  PCB_ACTIONS::toggleLock.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::LockSelected,        PCB_ACTIONS::lock.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::UnlockSelected,      PCB_ACTIONS::unlock.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::CrossProbePcbToSch,  SELECTION_TOOL::SelectedEvent );
    Go( &PCB_EDITOR_CONTROL::CrossProbeSchToPcb,  PCB_ACTIONS::crossProbeSchToPcb.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::DrillOrigin,         PCB_ACTIONS::drillOrigin.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::HighlightNet,        PCB_ACTIONS::highlightNet.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::HighlightNetCursor,  PCB_ACTIONS::highlightNetCursor.MakeEvent() );
}


const int PCB_EDITOR_CONTROL::WIDTH_STEP = 100000;
