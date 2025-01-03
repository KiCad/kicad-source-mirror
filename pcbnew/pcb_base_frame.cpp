/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2022 CERN
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

/// @todo The Boost entropy exception does not exist prior to 1.67. Once the minimum Boost
///       version is raise to 1.67 or greater, this version check can be removed.
#include <boost/version.hpp>

#if BOOST_VERSION >= 106700
#include <boost/uuid/entropy_error.hpp>
#endif

#include <3d_viewer/eda_3d_viewer_frame.h>          // To include VIEWER3D_FRAMENAME
#include <advanced_config.h>
#include <base_units.h>
#include <board.h>
#include <cleanup_item.h>
#include <collectors.h>
#include <confirm.h>
#include <footprint.h>
#include <footprint_editor_settings.h>
#include <fp_lib_table.h>
#include <lset.h>
#include <kiface_base.h>
#include <pcb_painter.h>
#include <pcbnew_id.h>
#include <pcbnew_settings.h>
#include <pcb_base_frame.h>
#include <pcb_draw_panel_gal.h>
#include <pgm_base.h>
#include <project_pcb.h>
#include <wildcards_and_files_ext.h>

#include <math/vector2d.h>
#include <math/vector2wx.h>
#include <widgets/msgpanel.h>
#include <wx/fswatcher.h>

#include <settings/settings_manager.h>
#include <settings/cvpcb_settings.h>
#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>
#include <tools/pcb_actions.h>
#include <tool/grid_menu.h>
#include <ratsnest/ratsnest_view_item.h>

#ifdef __linux__
#include <spacenav/spnav_2d_plugin.h>
#else
#include <navlib/nl_pcbnew_plugin.h>
#endif

using KIGFX::RENDER_SETTINGS;
using KIGFX::PCB_RENDER_SETTINGS;

wxDEFINE_EVENT( EDA_EVT_BOARD_CHANGED, wxCommandEvent );

PCB_BASE_FRAME::PCB_BASE_FRAME( KIWAY* aKiway, wxWindow* aParent, FRAME_T aFrameType,
                                const wxString& aTitle, const wxPoint& aPos, const wxSize& aSize,
                                long aStyle, const wxString& aFrameName ) :
        EDA_DRAW_FRAME( aKiway, aParent, aFrameType, aTitle, aPos, aSize, aStyle, aFrameName,
                        pcbIUScale ),
        m_pcb( nullptr ),
        m_originTransforms( *this ),
        m_inFpChangeTimerEvent( false )
{
    m_watcherDebounceTimer.Bind( wxEVT_TIMER, &PCB_BASE_FRAME::OnFpChangeDebounceTimer, this );
}


PCB_BASE_FRAME::~PCB_BASE_FRAME()
{
    // Ensure m_canvasType is up to date, to save it in config
    if( GetCanvas() )
        m_canvasType = GetCanvas()->GetBackend();

    delete m_pcb;
    m_pcb = nullptr;
}


bool PCB_BASE_FRAME::canCloseWindow( wxCloseEvent& aEvent )
{
    // Close modeless dialogs.  They're trouble when they get destroyed after the frame and/or
    // board.
    wxWindow* viewer3D = Get3DViewerFrame();

    if( viewer3D )
        viewer3D->Close( true );

    // Similarly, wxConvBrokenFileNames uses some statically allocated variables that make it
    // crash when run later from a d'tor.
    PROJECT_PCB::Cleanup3DCache( &Prj() );

    return true;
}


void PCB_BASE_FRAME::handleActivateEvent( wxActivateEvent& aEvent )
{
    EDA_DRAW_FRAME::handleActivateEvent( aEvent );

    if( m_spaceMouse )
        m_spaceMouse->SetFocus( aEvent.GetActive() );
}


void PCB_BASE_FRAME::handleIconizeEvent( wxIconizeEvent& aEvent )
{
    EDA_DRAW_FRAME::handleIconizeEvent( aEvent );

    if( m_spaceMouse && aEvent.IsIconized() )
        m_spaceMouse->SetFocus( false );
}


EDA_3D_VIEWER_FRAME* PCB_BASE_FRAME::Get3DViewerFrame()
{
    wxWindow* frame = FindWindowByName( QUALIFIED_VIEWER3D_FRAMENAME( this ) );
    return dynamic_cast<EDA_3D_VIEWER_FRAME*>( frame );
}


void PCB_BASE_FRAME::Update3DView( bool aMarkDirty, bool aRefresh, const wxString* aTitle )
{
    EDA_3D_VIEWER_FRAME* draw3DFrame = Get3DViewerFrame();

    if( draw3DFrame )
    {
        if( aTitle )
            draw3DFrame->SetTitle( *aTitle );

        if( aMarkDirty )
            draw3DFrame->ReloadRequest();

        if( aRefresh )
            draw3DFrame->Redraw();
    }
}


void PCB_BASE_FRAME::SetBoard( BOARD* aBoard, PROGRESS_REPORTER* aReporter )
{
    if( m_pcb != aBoard )
    {
        delete m_pcb;
        m_pcb = aBoard;

        if( GetBoard() )
            GetBoard()->SetUserUnits( GetUserUnits() );

        if( GetBoard() && GetCanvas() )
        {
            RENDER_SETTINGS* rs = GetCanvas()->GetView()->GetPainter()->GetSettings();

            if( rs )
            {
                rs->SetDashLengthRatio( GetBoard()->GetPlotOptions().GetDashedLineDashRatio() );
                rs->SetGapLengthRatio( GetBoard()->GetPlotOptions().GetDashedLineGapRatio() );
            }
        }

        wxCommandEvent e( EDA_EVT_BOARD_CHANGED );
        ProcessEventLocally( e );

        for( wxEvtHandler* listener : m_boardChangeListeners )
        {
            wxCHECK2( listener, continue );

            // Use the windows variant when handling event messages in case there is any special
            // event handler pre and/or post processing specific to windows.
            wxWindow* win = dynamic_cast<wxWindow*>( listener );

            if( win )
                win->HandleWindowEvent( e );
            else
                listener->SafelyProcessEvent( e );
        }
    }
}


void PCB_BASE_FRAME::AddBoardChangeListener( wxEvtHandler* aListener )
{
    auto it = std::find( m_boardChangeListeners.begin(), m_boardChangeListeners.end(), aListener );

    // Don't add duplicate listeners.
    if( it == m_boardChangeListeners.end() )
        m_boardChangeListeners.push_back( aListener );
}


void PCB_BASE_FRAME::RemoveBoardChangeListener( wxEvtHandler* aListener )
{
    auto it = std::find( m_boardChangeListeners.begin(), m_boardChangeListeners.end(), aListener );

    // Don't add duplicate listeners.
    if( it != m_boardChangeListeners.end() )
        m_boardChangeListeners.erase( it );
}


void PCB_BASE_FRAME::AddFootprintToBoard( FOOTPRINT* aFootprint )
{
    if( aFootprint )
    {
        GetBoard()->Add( aFootprint, ADD_MODE::APPEND );

        aFootprint->SetFlags( IS_NEW );
        aFootprint->SetPosition( VECTOR2I( 0, 0 ) ); // cursor in GAL may not be initialized yet

        // Put it on FRONT layer (note that it might be stored flipped if the lib is an archive
        // built from a board)
        if( aFootprint->IsFlipped() )
            aFootprint->Flip( aFootprint->GetPosition(), GetPcbNewSettings()->m_FlipDirection );

        // Place it in orientation 0 even if it is not saved with orientation 0 in lib (note that
        // it might be stored in another orientation if the lib is an archive built from a board)
        aFootprint->SetOrientation( ANGLE_0 );

        GetBoard()->UpdateUserUnits( aFootprint, GetCanvas()->GetView() );

        m_toolManager->RunAction( PCB_ACTIONS::rehatchShapes );
    }
}


EDA_ITEM* PCB_BASE_FRAME::ResolveItem( const KIID& aId, bool aAllowNullptrReturn ) const
{
    return GetBoard()->ResolveItem( aId, aAllowNullptrReturn );
}

void PCB_BASE_FRAME::FocusOnItem( EDA_ITEM* aItem )
{
    // nullptr will clear the current focus
    if( aItem != nullptr && !aItem->IsBOARD_ITEM() )
        return;

    FocusOnItem( static_cast<BOARD_ITEM*>( aItem ), UNDEFINED_LAYER );
}

void PCB_BASE_FRAME::FocusOnItem( BOARD_ITEM* aItem, PCB_LAYER_ID aLayer )
{
    std::vector<BOARD_ITEM*> items;

    if( aItem )
        items.push_back( aItem );

    FocusOnItems( items, aLayer );
}


void PCB_BASE_FRAME::FocusOnItems( std::vector<BOARD_ITEM*> aItems, PCB_LAYER_ID aLayer )
{
    static std::vector<KIID> lastBrightenedItemIDs;

    bool itemsUnbrightened = false;

    for( KIID lastBrightenedItemID : lastBrightenedItemIDs )
    {
        if( BOARD_ITEM* lastItem = GetBoard()->ResolveItem( lastBrightenedItemID, true ) )
        {
            lastItem->ClearBrightened();
            GetCanvas()->GetView()->Update( lastItem );
            itemsUnbrightened = true;
        }
    }

    if( itemsUnbrightened )
        GetCanvas()->Refresh();

    lastBrightenedItemIDs.clear();

    if( aItems.empty() )
        return;

    VECTOR2I       focusPt;
    KIGFX::VIEW*   view = GetCanvas()->GetView();
    SHAPE_POLY_SET viewportPoly( view->GetViewport() );

    for( wxWindow* dialog : findDialogs() )
    {
        wxPoint dialogPos = GetCanvas()->ScreenToClient( dialog->GetScreenPosition() );
        SHAPE_POLY_SET dialogPoly( BOX2D( view->ToWorld( ToVECTOR2D( dialogPos ), true ),
                                          view->ToWorld( ToVECTOR2D( dialog->GetSize() ), false ) ) );

        try
        {
            viewportPoly.BooleanSubtract( dialogPoly );
        }
        catch( const std::exception& e )
        {
            wxFAIL_MSG( wxString::Format( wxT( "Clipper exception occurred: %s" ), e.what() ) );
        }
    }

    SHAPE_POLY_SET itemPoly, clippedPoly;

    for( BOARD_ITEM* item : aItems )
    {
        if( item && item != DELETED_BOARD_ITEM::GetInstance() )
        {
            item->SetBrightened();
            lastBrightenedItemIDs.push_back( item->m_Uuid );

            item->RunOnChildren(
                    [&]( BOARD_ITEM* child )
                    {
                        child->SetBrightened();
                        lastBrightenedItemIDs.push_back( child->m_Uuid );
                    },
                    RECURSE_MODE::RECURSE );

            GetCanvas()->GetView()->Update( item );

            // Focus on the object's location.  Prefer a visible part of the object to its anchor
            // in order to keep from scrolling around.

            focusPt = item->GetPosition();

            if( aLayer == UNDEFINED_LAYER && item->GetLayerSet().any() )
                aLayer = item->GetLayerSet().Seq()[0];

            switch( item->Type() )
            {
            case PCB_FOOTPRINT_T:
                try
                {
                    itemPoly = static_cast<FOOTPRINT*>( item )->GetBoundingHull();
                }
                catch( const std::exception& e )
                {
                    wxFAIL_MSG( wxString::Format( wxT( "Clipper exception occurred: %s" ), e.what() ) );
                }

                break;

            case PCB_PAD_T:
            case PCB_MARKER_T:
            case PCB_VIA_T:
                FocusOnLocation( item->GetFocusPosition() );
                GetCanvas()->Refresh();
                return;

            case PCB_SHAPE_T:
            case PCB_FIELD_T:
            case PCB_TEXT_T:
            case PCB_TEXTBOX_T:
            case PCB_TRACE_T:
            case PCB_ARC_T:
            case PCB_DIM_ALIGNED_T:
            case PCB_DIM_LEADER_T:
            case PCB_DIM_CENTER_T:
            case PCB_DIM_RADIAL_T:
            case PCB_DIM_ORTHOGONAL_T:
                item->TransformShapeToPolygon( itemPoly, aLayer, 0, pcbIUScale.mmToIU( 0.1 ), ERROR_INSIDE );
                break;

            case PCB_ZONE_T:
            {
                ZONE* zone = static_cast<ZONE*>( item );
#if 0
                // Using the filled area shapes to find a Focus point can give good results, but
                // unfortunately the calculations are highly time consuming, even for not very
                // large areas (can be easily a few minutes for large areas).
                // so we used only the zone outline that usually do not have too many vertices.
                zone->TransformShapeToPolygon( itemPoly, aLayer, 0, pcbIUScale.mmToIU( 0.1 ), ERROR_INSIDE );

                if( itemPoly.IsEmpty() )
                    itemPoly = *zone->Outline();
#else
                // much faster calculation time when using only the zone outlines
                itemPoly = *zone->Outline();
#endif

                break;
            }

            default:
            {
                BOX2I item_bbox = item->GetBoundingBox();
                itemPoly.NewOutline();
                itemPoly.Append( item_bbox.GetOrigin() );
                itemPoly.Append( item_bbox.GetOrigin() + VECTOR2I( item_bbox.GetWidth(), 0 ) );
                itemPoly.Append( item_bbox.GetOrigin() + VECTOR2I( 0, item_bbox.GetHeight() ) );
                itemPoly.Append( item_bbox.GetOrigin() + VECTOR2I( item_bbox.GetWidth(),
                                                                   item_bbox.GetHeight() ) );
                break;
            }
            }

            try
            {
                itemPoly.ClearArcs();
                viewportPoly.ClearArcs();
                clippedPoly.BooleanIntersection( itemPoly, viewportPoly );
            }
            catch( const std::exception& e )
            {
                wxFAIL_MSG( wxString::Format( wxT( "Clipper exception occurred: %s" ), e.what() ) );
            }

            if( !clippedPoly.IsEmpty() )
                itemPoly = clippedPoly;
        }
    }

    /*
     * Perform a step-wise deflate to find the visual-center-of-mass
     */

    BOX2I    bbox = itemPoly.BBox();
    int      step = std::min( bbox.GetWidth(), bbox.GetHeight() ) / 10;

    while( !itemPoly.IsEmpty() )
    {
        focusPt = itemPoly.BBox().Centre();

        try
        {
            itemPoly.Deflate( step, CORNER_STRATEGY::ALLOW_ACUTE_CORNERS, ARC_LOW_DEF );
        }
        catch( const std::exception& e )
        {
            wxFAIL_MSG( wxString::Format( wxT( "Clipper exception occurred: %s" ), e.what() ) );
        }
    }

    FocusOnLocation( focusPt );

    GetCanvas()->Refresh();
}


void PCB_BASE_FRAME::HideSolderMask()
{
    KIGFX::PCB_VIEW* view = GetCanvas()->GetView();

    if( view && GetBoard()->m_SolderMaskBridges && view->HasItem( GetBoard()->m_SolderMaskBridges ) )
        view->Remove( GetBoard()->m_SolderMaskBridges );
}


void PCB_BASE_FRAME::ShowSolderMask()
{
    KIGFX::PCB_VIEW* view = GetCanvas()->GetView();

    if( view && GetBoard()->m_SolderMaskBridges )
    {
        if( view->HasItem( GetBoard()->m_SolderMaskBridges ) )
            view->Remove( GetBoard()->m_SolderMaskBridges );

        view->Add( GetBoard()->m_SolderMaskBridges );
    }
}


void PCB_BASE_FRAME::SetPageSettings( const PAGE_INFO& aPageSettings )
{
    m_pcb->SetPageSettings( aPageSettings );

    if( GetScreen() )
        GetScreen()->InitDataPoints( aPageSettings.GetSizeIU( pcbIUScale.IU_PER_MILS ) );
}


const PAGE_INFO& PCB_BASE_FRAME::GetPageSettings() const
{
    return m_pcb->GetPageSettings();
}


const VECTOR2I PCB_BASE_FRAME::GetPageSizeIU() const
{
    // this function is only needed because EDA_DRAW_FRAME is not compiled
    // with either -DPCBNEW or -DEESCHEMA, so the virtual is used to route
    // into an application specific source file.
    return m_pcb->GetPageSettings().GetSizeIU( pcbIUScale.IU_PER_MILS );
}


const VECTOR2I& PCB_BASE_FRAME::GetGridOrigin() const
{
    return m_pcb->GetDesignSettings().GetGridOrigin();
}


void PCB_BASE_FRAME::SetGridOrigin( const VECTOR2I& aPoint )
{
    m_pcb->GetDesignSettings().SetGridOrigin( aPoint );
}


const VECTOR2I& PCB_BASE_FRAME::GetAuxOrigin() const
{
    return m_pcb->GetDesignSettings().GetAuxOrigin();
}


const VECTOR2I PCB_BASE_FRAME::GetUserOrigin() const
{
    VECTOR2I origin( 0, 0 );

    switch( GetPcbNewSettings()->m_Display.m_DisplayOrigin )
    {
    case PCB_DISPLAY_ORIGIN::PCB_ORIGIN_PAGE:                           break;
    case PCB_DISPLAY_ORIGIN::PCB_ORIGIN_AUX:  origin = GetAuxOrigin();  break;
    case PCB_DISPLAY_ORIGIN::PCB_ORIGIN_GRID: origin = GetGridOrigin(); break;
    default:                                  wxASSERT( false );        break;
    }

    return origin;
}

ORIGIN_TRANSFORMS& PCB_BASE_FRAME::GetOriginTransforms()
{
    return m_originTransforms;
}


const TITLE_BLOCK& PCB_BASE_FRAME::GetTitleBlock() const
{
    return m_pcb->GetTitleBlock();
}


void PCB_BASE_FRAME::SetTitleBlock( const TITLE_BLOCK& aTitleBlock )
{
    m_pcb->SetTitleBlock( aTitleBlock );
}


BOARD_DESIGN_SETTINGS& PCB_BASE_FRAME::GetDesignSettings() const
{
    return m_pcb->GetDesignSettings();
}


void PCB_BASE_FRAME::SetDrawBgColor( const COLOR4D& aColor )
{
    m_drawBgColor= aColor;
    m_auimgr.Update();
}


const PCB_PLOT_PARAMS& PCB_BASE_FRAME::GetPlotSettings() const
{
    return m_pcb->GetPlotOptions();
}


void PCB_BASE_FRAME::SetPlotSettings( const PCB_PLOT_PARAMS& aSettings )
{
    m_pcb->SetPlotOptions( aSettings );

    // Plot Settings can also change the "tent vias" setting, which can affect the solder mask.
    LSET visibleLayers = GetBoard()->GetVisibleLayers();

    if( visibleLayers.test( F_Mask ) || visibleLayers.test( B_Mask ) )
    {
        GetCanvas()->GetView()->UpdateAllItemsConditionally(
                [&]( KIGFX::VIEW_ITEM* aItem ) -> int
                {
                    BOARD_ITEM* item = nullptr;

                    if( aItem->IsBOARD_ITEM() )
                        item = static_cast<BOARD_ITEM*>( aItem );

                    // Note: KIGFX::REPAINT isn't enough for things that go from invisible to
                    // visible as they won't be found in the view layer's itemset for re-painting.
                    if( item && item->Type() == PCB_VIA_T )
                        return KIGFX::ALL;

                    return 0;
                } );

        GetCanvas()->Refresh();
    }
}


BOX2I PCB_BASE_FRAME::GetBoardBoundingBox( bool aBoardEdgesOnly ) const
{
    BOX2I area = aBoardEdgesOnly ? m_pcb->GetBoardEdgesBoundingBox() : m_pcb->GetBoundingBox();

    if( area.GetWidth() == 0 && area.GetHeight() == 0 )
    {
        VECTOR2I pageSize = GetPageSizeIU();

        if( m_showBorderAndTitleBlock )
        {
            area.SetOrigin( 0, 0 );
            area.SetEnd( pageSize.x, pageSize.y );
        }
        else
        {
            area.SetOrigin( -pageSize.x / 2, -pageSize.y / 2 );
            area.SetEnd( pageSize.x / 2, pageSize.y / 2 );
        }
    }

    return area;
}


const BOX2I PCB_BASE_FRAME::GetDocumentExtents( bool aIncludeAllVisible ) const
{
    /* "Zoom to Fit" calls this with "aIncludeAllVisible" as true.  Since that feature
         * always ignored the page and border, this function returns a bbox without them
         * as well when passed true.  This technically is not all things visible, but it
         * keeps behavior consistent.
         *
         * When passed false, this function returns a bbox of just the board edge. This
         * allows things like fabrication text or anything else outside the board edge to
         * be ignored, and just zooms up to the board itself.
         *
         * Calling "GetBoardBoundingBox(true)" when edge cuts are turned off will return
         * the entire page and border, so we call "GetBoardBoundingBox(false)" instead.
         */
    if( aIncludeAllVisible || !m_pcb->IsLayerVisible( Edge_Cuts ) )
        return GetBoardBoundingBox( false );
    else
        return GetBoardBoundingBox( true );
}


// Virtual function
void PCB_BASE_FRAME::doReCreateMenuBar()
{
}


void PCB_BASE_FRAME::ShowChangedLanguage()
{
    // call my base class
    EDA_DRAW_FRAME::ShowChangedLanguage();

    // tooltips in toolbars
    RecreateToolbars();

    EDA_3D_VIEWER_FRAME* viewer3D = Get3DViewerFrame();

    if( viewer3D )
        viewer3D->ShowChangedLanguage();
}


EDA_3D_VIEWER_FRAME* PCB_BASE_FRAME::CreateAndShow3D_Frame()
{
    EDA_3D_VIEWER_FRAME* draw3DFrame = Get3DViewerFrame();

    if( !draw3DFrame )
        draw3DFrame = new EDA_3D_VIEWER_FRAME( &Kiway(), this, _( "3D Viewer" ) );

    // Raising the window does not show the window on Windows if iconized. This should work
    // on any platform.
    if( draw3DFrame->IsIconized() )
         draw3DFrame->Iconize( false );

    draw3DFrame->Raise();
    draw3DFrame->Show( true );

    // Raising the window does not set the focus on Linux.  This should work on any platform.
    if( wxWindow::FindFocus() != draw3DFrame )
        draw3DFrame->SetFocus();

    // Allocate a slice of time to display the 3D frame
    // a call to wxSafeYield() should be enough (and better), but on Linux we need
    // to call wxYield()
    // otherwise the activity messages are not displayed during the first board loading
    wxYield();

    // Note, the caller is responsible to load/update the board 3D view.
    // after frame creation the board is not automatically created.

    return draw3DFrame;
}


void PCB_BASE_FRAME::SwitchLayer( PCB_LAYER_ID layer )
{
    PCB_LAYER_ID preslayer = GetActiveLayer();
    auto& displ_opts = GetDisplayOptions();

    // Check if the specified layer matches the present layer
    if( layer == preslayer )
        return;

    // Copper layers cannot be selected unconditionally; how many of those layers are
    // currently enabled needs to be checked.
    if( IsCopperLayer( layer ) )
    {
        if( layer > m_pcb->GetCopperLayerStackMaxId() )
            return;
    }

    // Is yet more checking required? E.g. when the layer to be selected is a non-copper
    // layer, or when switching between a copper layer and a non-copper layer, or vice-versa?
    // ...

    SetActiveLayer( layer );

    if( displ_opts.m_ContrastModeDisplay != HIGH_CONTRAST_MODE::NORMAL )
        GetCanvas()->Refresh();
}


GENERAL_COLLECTORS_GUIDE PCB_BASE_FRAME::GetCollectorsGuide()
{
    GENERAL_COLLECTORS_GUIDE guide( m_pcb->GetVisibleLayers(), GetActiveLayer(),
                                    GetCanvas()->GetView() );

    // account for the globals
    guide.SetIgnoreFPTextOnBack( !m_pcb->IsElementVisible( LAYER_FP_TEXT ) );
    guide.SetIgnoreFPTextOnFront( !m_pcb->IsElementVisible( LAYER_FP_TEXT ) );
    guide.SetIgnoreFootprintsOnBack( !m_pcb->IsElementVisible( LAYER_FOOTPRINTS_BK ) );
    guide.SetIgnoreFootprintsOnFront( !m_pcb->IsElementVisible( LAYER_FOOTPRINTS_FR ) );
    guide.SetIgnoreThroughHolePads( ! m_pcb->IsElementVisible( LAYER_PADS ) );
    guide.SetIgnoreFPValues( !m_pcb->IsElementVisible( LAYER_FP_VALUES ) );
    guide.SetIgnoreFPReferences( !m_pcb->IsElementVisible( LAYER_FP_REFERENCES ) );
    guide.SetIgnoreThroughVias( ! m_pcb->IsElementVisible( LAYER_VIAS ) );
    guide.SetIgnoreBlindBuriedVias( ! m_pcb->IsElementVisible( LAYER_VIAS ) );
    guide.SetIgnoreMicroVias( ! m_pcb->IsElementVisible( LAYER_VIAS ) );
    guide.SetIgnoreTracks( ! m_pcb->IsElementVisible( LAYER_TRACKS ) );

    return guide;
}


void PCB_BASE_FRAME::UpdateStatusBar()
{
    EDA_DRAW_FRAME::UpdateStatusBar();

    BASE_SCREEN* screen = GetScreen();

    if( !screen )
        return;

    wxString line;
    VECTOR2D cursorPos = GetCanvas()->GetViewControls()->GetCursorPosition();

    if( GetShowPolarCoords() )  // display polar coordinates
    {
        double   dx = cursorPos.x - screen->m_LocalOrigin.x;
        double   dy = cursorPos.y - screen->m_LocalOrigin.y;
        double   theta = RAD2DEG( atan2( -dy, dx ) );
        double   ro = hypot( dx, dy );

        line.Printf( wxT( "r %s  theta %.3f" ),
                     MessageTextFromValue( ro, false ),
                     theta );

        SetStatusText( line, 3 );
    }

    // Transform absolute coordinates for user origin preferences
    double userXpos = m_originTransforms.ToDisplayAbsX( static_cast<double>( cursorPos.x ) );
    double userYpos = m_originTransforms.ToDisplayAbsY( static_cast<double>( cursorPos.y ) );

    // Display absolute coordinates:
    line.Printf( wxT( "X %s  Y %s" ),
                 MessageTextFromValue( userXpos, false ),
                 MessageTextFromValue( userYpos, false ) );
    SetStatusText( line, 2 );

    if( !GetShowPolarCoords() )  // display relative cartesian coordinates
    {
        // Calculate relative coordinates
        double relXpos = cursorPos.x - screen->m_LocalOrigin.x;
        double relYpos = cursorPos.y - screen->m_LocalOrigin.y;

        // Transform relative coordinates for user origin preferences
        userXpos = m_originTransforms.ToDisplayRelX( relXpos );
        userYpos = m_originTransforms.ToDisplayRelY( relYpos );

        line.Printf( wxT( "dx %s  dy %s  dist %s" ),
                     MessageTextFromValue( userXpos, false ),
                     MessageTextFromValue( userYpos, false ),
                     MessageTextFromValue( hypot( userXpos, userYpos ), false ) );
        SetStatusText( line, 3 );
    }

    DisplayGridMsg();
}


void PCB_BASE_FRAME::unitsChangeRefresh()
{
    EDA_DRAW_FRAME::unitsChangeRefresh();    // Update the status bar.

    if( GetBoard() )
        GetBoard()->SetUserUnits( GetUserUnits() );

    UpdateGridSelectBox();
}


void PCB_BASE_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    EDA_DRAW_FRAME::LoadSettings( aCfg );

    // Move legacy user grids to grid list
    if( !aCfg->m_Window.grid.user_grid_x.empty() )
    {
        aCfg->m_Window.grid.grids.emplace_back( GRID{ "User Grid", aCfg->m_Window.grid.user_grid_x,
                                                      aCfg->m_Window.grid.user_grid_y } );
        aCfg->m_Window.grid.user_grid_x = wxEmptyString;
        aCfg->m_Window.grid.user_grid_y = wxEmptyString;
    }

    // Some, but not all, derived classes have a PCBNEW_SETTINGS.
    if( PCBNEW_SETTINGS* pcbnew_cfg = dynamic_cast<PCBNEW_SETTINGS*>( aCfg ) )
        m_polarCoords = pcbnew_cfg->m_PolarCoords;

    wxASSERT( GetCanvas() );

    if( GetCanvas() )
    {
        RENDER_SETTINGS* rs = GetCanvas()->GetView()->GetPainter()->GetSettings();

        if( rs )
        {
            rs->SetHighlightFactor( aCfg->m_Graphics.highlight_factor );
            rs->SetSelectFactor( aCfg->m_Graphics.select_factor );
            rs->SetDefaultFont( wxEmptyString );    // Always the KiCad font for PCBs
        }
    }
}


SEVERITY PCB_BASE_FRAME::GetSeverity( int aErrorCode ) const
{
    if( aErrorCode >= CLEANUP_FIRST )
        return RPT_SEVERITY_ACTION;

    BOARD_DESIGN_SETTINGS& bds = GetBoard()->GetDesignSettings();

    return bds.m_DRCSeverities[ aErrorCode ];
}


void PCB_BASE_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    EDA_DRAW_FRAME::SaveSettings( aCfg );

    // Some, but not all derived classes have a PCBNEW_SETTINGS.
    PCBNEW_SETTINGS* cfg = dynamic_cast<PCBNEW_SETTINGS*>( aCfg );

    if( cfg )
        cfg->m_PolarCoords = m_polarCoords;
}


PCBNEW_SETTINGS* PCB_BASE_FRAME::GetPcbNewSettings() const
{
    return Pgm().GetSettingsManager().GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" );
}


FOOTPRINT_EDITOR_SETTINGS* PCB_BASE_FRAME::GetFootprintEditorSettings() const
{
    return Pgm().GetSettingsManager().GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" );
}


PCB_VIEWERS_SETTINGS_BASE* PCB_BASE_FRAME::GetViewerSettingsBase() const
{
    switch( GetFrameType() )
    {
    case FRAME_PCB_EDITOR:
    case FRAME_PCB_DISPLAY3D:
    default:
        return Pgm().GetSettingsManager().GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" );

    case FRAME_FOOTPRINT_EDITOR:
    case FRAME_FOOTPRINT_WIZARD:
        return Pgm().GetSettingsManager().GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" );

    case FRAME_FOOTPRINT_VIEWER:
    case FRAME_FOOTPRINT_CHOOSER:
    case FRAME_FOOTPRINT_PREVIEW:
    case FRAME_CVPCB:
    case FRAME_CVPCB_DISPLAY:
        return Pgm().GetSettingsManager().GetAppSettings<CVPCB_SETTINGS>( "cvpcb" );
    }
}


MAGNETIC_SETTINGS* PCB_BASE_FRAME::GetMagneticItemsSettings()
{
    static MAGNETIC_SETTINGS fallback;

    if( PCBNEW_SETTINGS* cfg = GetPcbNewSettings() )
        return &cfg->m_MagneticItems;

    return &fallback;
}


void PCB_BASE_FRAME::CommonSettingsChanged( int aFlags )
{
    EDA_DRAW_FRAME::CommonSettingsChanged( aFlags );

    KIGFX::VIEW*         view = GetCanvas()->GetView();
    KIGFX::PCB_PAINTER*  painter = static_cast<KIGFX::PCB_PAINTER*>( view->GetPainter() );
    PCB_RENDER_SETTINGS* settings = painter->GetSettings();

    settings->LoadColors( GetColorSettings( true ) );
    settings->LoadDisplayOptions( GetDisplayOptions() );
    settings->m_ForceShowFieldsWhenFPSelected = GetPcbNewSettings()->m_Display.m_ForceShowFieldsWhenFPSelected;

    if( aFlags & TEXTVARS_CHANGED )
        GetBoard()->SynchronizeProperties();

    // Note: KIGFX::REPAINT isn't enough for things that go from invisible to visible as
    // they won't be found in the view layer's itemset for re-painting.
    GetCanvas()->GetView()->UpdateAllItemsConditionally(
            [&]( KIGFX::VIEW_ITEM* aItem ) -> int
            {
                if( dynamic_cast<RATSNEST_VIEW_ITEM*>( aItem ) )
                {
                    return KIGFX::ALL;        // ratsnest display
                }
                else if( dynamic_cast<PCB_TRACK*>( aItem ) )
                {
                    return KIGFX::REPAINT;    // track, arc & via clearance display
                }
                else if( dynamic_cast<PAD*>( aItem ) )
                {
                    return KIGFX::REPAINT;    // pad clearance display
                }
                else if( EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( aItem ) )
                {
                    if( text->HasTextVars() )
                    {
                        text->ClearRenderCache();
                        text->ClearBoundingBoxCache();
                        return KIGFX::GEOMETRY | KIGFX::REPAINT;
                    }
                }

                return 0;
            } );

    view->UpdateAllItems( KIGFX::COLOR );

    RecreateToolbars();

    // The 3D viewer isn't in the Kiway, so send its update manually
    EDA_3D_VIEWER_FRAME* viewer = Get3DViewerFrame();

    if( viewer )
        viewer->CommonSettingsChanged( aFlags );
}


void PCB_BASE_FRAME::OnModify()
{
    EDA_BASE_FRAME::OnModify();

    GetScreen()->SetContentModified();
    GetBoard()->IncrementTimeStamp();

    if( m_isClosing )
        return;

    UpdateStatusBar();
    UpdateMsgPanel();
}


void PCB_BASE_FRAME::rebuildConnectivity()
{
    GetBoard()->BuildConnectivity();
    GetToolManager()->PostEvent( EVENTS::ConnectivityChangedEvent );
    GetCanvas()->RedrawRatsnest();
}


PCB_DRAW_PANEL_GAL* PCB_BASE_FRAME::GetCanvas() const
{
    return static_cast<PCB_DRAW_PANEL_GAL*>( EDA_DRAW_FRAME::GetCanvas() );
}


void PCB_BASE_FRAME::ActivateGalCanvas()
{
    EDA_DRAW_FRAME::ActivateGalCanvas();

    EDA_DRAW_PANEL_GAL* canvas = GetCanvas();
    KIGFX::VIEW*        view = canvas->GetView();

    if( m_toolManager )
    {
        m_toolManager->SetEnvironment( m_pcb, view, canvas->GetViewControls(), config(), this );

        m_toolManager->ResetTools( TOOL_BASE::GAL_SWITCH );
    }

    KIGFX::PCB_PAINTER*         painter = static_cast<KIGFX::PCB_PAINTER*>( view->GetPainter() );
    KIGFX::PCB_RENDER_SETTINGS* settings = painter->GetSettings();
    const PCB_DISPLAY_OPTIONS&  displ_opts = GetDisplayOptions();

    settings->LoadDisplayOptions( displ_opts );
    settings->LoadColors( GetColorSettings() );
    settings->m_ForceShowFieldsWhenFPSelected = GetPcbNewSettings()->m_Display.m_ForceShowFieldsWhenFPSelected;

    view->RecacheAllItems();
    canvas->SetEventDispatcher( m_toolDispatcher );
    canvas->StartDrawing();

    try

    {
        if( !m_spaceMouse )
        {
#ifndef __linux__
            m_spaceMouse = std::make_unique<NL_PCBNEW_PLUGIN>( GetCanvas() );
#else
            m_spaceMouse = std::make_unique<SPNAV_2D_PLUGIN>( GetCanvas() );
            m_spaceMouse->SetScale( 0.01 );
#endif
        }
    }
    catch( const std::system_error& e )
    {
        wxLogTrace( wxT( "KI_TRACE_NAVLIB" ), e.what() );
    }
}


void PCB_BASE_FRAME::SetDisplayOptions( const PCB_DISPLAY_OPTIONS& aOptions, bool aRefresh )
{
    bool hcChanged    = m_displayOptions.m_ContrastModeDisplay != aOptions.m_ContrastModeDisplay;
    bool hcVisChanged = m_displayOptions.m_ContrastModeDisplay == HIGH_CONTRAST_MODE::HIDDEN
                        || aOptions.m_ContrastModeDisplay == HIGH_CONTRAST_MODE::HIDDEN;
    m_displayOptions  = aOptions;

    EDA_DRAW_PANEL_GAL* canvas = GetCanvas();
    KIGFX::PCB_VIEW*    view   = static_cast<KIGFX::PCB_VIEW*>( canvas->GetView() );

    view->UpdateDisplayOptions( aOptions );
    canvas->SetHighContrastLayer( GetActiveLayer() );
    OnDisplayOptionsChanged();

    // Vias on a restricted layer set must be redrawn when high contrast mode is changed
    if( hcChanged )
    {
        bool showNetNames = false;

        if( PCBNEW_SETTINGS* config = dynamic_cast<PCBNEW_SETTINGS*>( Kiface().KifaceSettings() ) )
            showNetNames = config->m_Display.m_NetNames > 0;

        // Note: KIGFX::REPAINT isn't enough for things that go from invisible to visible as
        // they won't be found in the view layer's itemset for re-painting.
        GetCanvas()->GetView()->UpdateAllItemsConditionally(
                [&]( KIGFX::VIEW_ITEM* aItem ) -> int
                {
                    if( PCB_VIA* via = dynamic_cast<PCB_VIA*>( aItem ) )
                    {
                        if( via->GetViaType() != VIATYPE::THROUGH
                                || via->GetRemoveUnconnected()
                                || showNetNames )
                        {
                            return hcVisChanged ? KIGFX::ALL : KIGFX::REPAINT;
                        }
                    }
                    else if( PAD* pad = dynamic_cast<PAD*>( aItem ) )
                    {
                        if( pad->GetRemoveUnconnected()
                                || showNetNames )
                        {
                            return hcVisChanged ? KIGFX::ALL : KIGFX::REPAINT;
                        }
                    }

                    return 0;
                } );
    }

    if( aRefresh )
        canvas->Refresh();
}


void PCB_BASE_FRAME::setFPWatcher( FOOTPRINT* aFootprint )
{
    wxLogTrace( "KICAD_LIB_WATCH", "setFPWatcher" );

    Unbind( wxEVT_FSWATCHER, &PCB_BASE_FRAME::OnFPChange, this );

    if( m_watcher )
    {
        wxLogTrace( "KICAD_LIB_WATCH", "Remove watch" );
        m_watcher->RemoveAll();
        m_watcher->SetOwner( nullptr );
        m_watcher.reset();
    }

    wxString libfullname;
    FP_LIB_TABLE* tbl = PROJECT_PCB::PcbFootprintLibs( &Prj() );

    if( !aFootprint || !tbl )
        return;

    try
    {
        const FP_LIB_TABLE_ROW* row = tbl->FindRow( aFootprint->GetFPID().GetLibNickname() );

        if( !row )
            return;

        libfullname = row->GetFullURI( true );
    }
    catch( const std::exception& e )
    {
        DisplayInfoMessage( this, e.what() );
        return;
    }
    catch( const IO_ERROR& error )
    {
        wxLogTrace( "KICAD_LIB_WATCH", "Error: %s", error.What() );
        return;
    }

    m_watcherFileName.Assign( libfullname, aFootprint->GetFPID().GetLibItemName(),
                              FILEEXT::KiCadFootprintFileExtension );

    if( !m_watcherFileName.FileExists() )
        return;

    m_watcherLastModified = m_watcherFileName.GetModificationTime();

    Bind( wxEVT_FSWATCHER, &PCB_BASE_FRAME::OnFPChange, this );
    m_watcher = std::make_unique<wxFileSystemWatcher>();
    m_watcher->SetOwner( this );

    wxFileName fn;
    fn.AssignDir( m_watcherFileName.GetPath() );
    fn.DontFollowLink();

    wxLogTrace( "KICAD_LIB_WATCH", "Add watch: %s", fn.GetPath() );

    {
        // Silence OS errors that come from the watcher
        wxLogNull silence;
        m_watcher->Add( fn );
    }
}


void PCB_BASE_FRAME::OnFPChange( wxFileSystemWatcherEvent& aEvent )
{
    if( aEvent.GetPath() != m_watcherFileName.GetFullPath() )
        return;

    // Start the debounce timer (set to 1 second)
    if( !m_watcherDebounceTimer.StartOnce( 1000 ) )
    {
        wxLogTrace( "KICAD_LIB_WATCH", "Failed to start the debounce timer" );
        return;
    }
}


void PCB_BASE_FRAME::OnFpChangeDebounceTimer( wxTimerEvent& aEvent )
{
    if( aEvent.GetId() != m_watcherDebounceTimer.GetId() )
    {
        aEvent.Skip();
        return;
    }

    if( m_inFpChangeTimerEvent )
    {
        wxLogTrace( "KICAD_LIB_WATCH", "Restarting debounce timer" );
        m_watcherDebounceTimer.StartOnce( 3000 );
    }

    wxLogTrace( "KICAD_LIB_WATCH", "OnFpChangeDebounceTimer" );

    // Disable logging to avoid spurious messages and check if the file has changed
    wxLog::EnableLogging( false );
    wxDateTime lastModified = m_watcherFileName.GetModificationTime();
    wxLog::EnableLogging( true );

    if( lastModified == m_watcherLastModified || !lastModified.IsValid() )
        return;

    m_watcherLastModified = lastModified;

    FOOTPRINT* fp = GetBoard()->GetFirstFootprint();
    FP_LIB_TABLE* tbl = PROJECT_PCB::PcbFootprintLibs( &Prj() );

    // When loading a footprint from a library in the footprint editor
    // the items UUIDs must be keep and not reinitialized
    bool keepUUID = IsType( FRAME_FOOTPRINT_EDITOR );

    if( !fp || !tbl )
        return;

    m_inFpChangeTimerEvent = true;

    if( !GetScreen()->IsContentModified()
        || IsOK( this, _( "The library containing the current footprint has changed.\n"
                          "Do you want to reload the footprint?" ) ) )
    {
        wxString fpname = fp->GetFPID().GetLibItemName();
        wxString nickname = fp->GetFPID().GetLibNickname();

        try
        {
            FOOTPRINT* newfp = tbl->FootprintLoad( nickname, fpname, keepUUID );

            if( newfp )
            {
                std::vector<KIID> selectedItems;

                for( const EDA_ITEM* item : GetCurrentSelection() )
                    selectedItems.emplace_back( item->m_Uuid );

                m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );

                ReloadFootprint( newfp );

                newfp->ClearAllNets();
                GetCanvas()->UpdateColors();
                GetCanvas()->DisplayBoard( GetBoard() );
                m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );

                std::vector<EDA_ITEM*> sel;

                for( const KIID& uuid : selectedItems )
                {
                    if( BOARD_ITEM* item = GetBoard()->ResolveItem( uuid, true ) )
                        sel.push_back( item );
                }

                if( !sel.empty() )
                    m_toolManager->RunAction( ACTIONS::selectItems, &sel );
            }
        }
        catch( const IO_ERROR& ioe )
        {
            DisplayError( this, ioe.What() );
        }
    }

    m_inFpChangeTimerEvent = false;
}
