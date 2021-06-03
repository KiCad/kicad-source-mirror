/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <kiface_i.h>
#include <confirm.h>
#include <dialog_helpers.h>
#include <pcb_base_frame.h>
#include <base_units.h>
#include <widgets/msgpanel.h>
#include <pgm_base.h>
#include <3d_viewer/eda_3d_viewer.h>          // To include VIEWER3D_FRAMENAME
#include <footprint_editor_settings.h>
#include <fp_lib_table.h>
#include <pcbnew_id.h>
#include <board.h>
#include <footprint.h>
#include <collectors.h>
#include <pcb_draw_panel_gal.h>
#include <math/vector2d.h>
#include <pcb_group.h>

#include <pcb_painter.h>
#include <settings/settings_manager.h>
#include <pcbnew_settings.h>
#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>
#include <tools/pcb_actions.h>
#include <tool/grid_menu.h>
#include "cleanup_item.h"
#include <zoom_defines.h>


wxDEFINE_EVENT( BOARD_CHANGED, wxCommandEvent );

PCB_BASE_FRAME::PCB_BASE_FRAME( KIWAY* aKiway, wxWindow* aParent, FRAME_T aFrameType,
                                const wxString& aTitle, const wxPoint& aPos, const wxSize& aSize,
                                long aStyle, const wxString & aFrameName ) :
        EDA_DRAW_FRAME( aKiway, aParent, aFrameType, aTitle, aPos, aSize, aStyle, aFrameName ),
        m_pcb( nullptr ),
        m_originTransforms( *this )
{
    m_settings = static_cast<PCBNEW_SETTINGS*>( Kiface().KifaceSettings() );
}


PCB_BASE_FRAME::~PCB_BASE_FRAME()
{
    // Ensure m_canvasType is up to date, to save it in config
    m_canvasType = GetCanvas()->GetBackend();

    delete m_pcb;
}


EDA_3D_VIEWER* PCB_BASE_FRAME::Get3DViewerFrame()
{
    return dynamic_cast<EDA_3D_VIEWER*>( FindWindowByName( QUALIFIED_VIEWER3D_FRAMENAME( this ) ) );
}


void PCB_BASE_FRAME::Update3DView( bool aMarkDirty, bool aRefresh, const wxString* aTitle )
{
    EDA_3D_VIEWER* draw3DFrame = Get3DViewerFrame();

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


FP_LIB_TABLE* PROJECT::PcbFootprintLibs()
{
    // This is a lazy loading function, it loads the project specific table when
    // that table is asked for, not before.

    FP_LIB_TABLE*   tbl = (FP_LIB_TABLE*) GetElem( ELEM_FPTBL );

    // its gotta be NULL or a FP_LIB_TABLE, or a bug.
    wxASSERT( !tbl || tbl->Type() == FP_LIB_TABLE_T );

    if( !tbl )
    {
        // Stack the project specific FP_LIB_TABLE overlay on top of the global table.
        // ~FP_LIB_TABLE() will not touch the fallback table, so multiple projects may
        // stack this way, all using the same global fallback table.
        tbl = new FP_LIB_TABLE( &GFootprintTable );

        SetElem( ELEM_FPTBL, tbl );

        wxString projectFpLibTableFileName = FootprintLibTblName();

        try
        {
            tbl->Load( projectFpLibTableFileName );
        }
        catch( const IO_ERROR& ioe )
        {
            DisplayErrorMessage( nullptr, _( "Error loading project footprint libraries" ),
                                 ioe.What() );
        }
    }

    return tbl;
}


void PCB_BASE_FRAME::SetBoard( BOARD* aBoard )
{
    if( m_pcb != aBoard )
    {
        delete m_pcb;
        m_pcb = aBoard;

        wxCommandEvent e( BOARD_CHANGED );
        ProcessEventLocally( e );
    }
}


void PCB_BASE_FRAME::AddFootprintToBoard( FOOTPRINT* aFootprint )
{
    if( aFootprint )
    {
        GetBoard()->Add( aFootprint, ADD_MODE::APPEND );

        aFootprint->SetFlags( IS_NEW );
        aFootprint->SetPosition( wxPoint( 0, 0 ) ); // cursor in GAL may not be initialized yet

        // Put it on FRONT layer (note that it might be stored flipped if the lib is an archive
        // built from a board)
        if( aFootprint->IsFlipped() )
            aFootprint->Flip( aFootprint->GetPosition(), m_settings->m_FlipLeftRight );

        // Place it in orientation 0 even if it is not saved with orientation 0 in lib (note that
        // it might be stored in another orientation if the lib is an archive built from a board)
        aFootprint->SetOrientation( 0 );
    }
}


EDA_ITEM* PCB_BASE_FRAME::GetItem( const KIID& aId ) const
{
    return GetBoard()->GetItem( aId );
}


void PCB_BASE_FRAME::FocusOnItem( BOARD_ITEM* aItem )
{
    static KIID lastBrightenedItemID( niluuid );

    BOARD_ITEM* lastItem = nullptr;

    /// @todo The Boost entropy exception does not exist prior to 1.67. Once the minimum Boost
    ///       version is raise to 1.67 or greater, this version check can be removed.
#if BOOST_VERSION >= 106700
    try
    {
        lastItem = GetBoard()->GetItem( lastBrightenedItemID );
    }
    catch( const boost::uuids::entropy_error& )
    {
        wxLogError( "A Boost UUID entropy exception was thrown in %s:%s.", __FILE__, __FUNCTION__ );
    }
#else
    lastItem = GetBoard()->GetItem( lastBrightenedItemID );
#endif

    if( lastItem && lastItem != aItem && lastItem != DELETED_BOARD_ITEM::GetInstance() )
    {
        lastItem->ClearBrightened();

        if( lastItem->Type() == PCB_FOOTPRINT_T )
        {
            static_cast<FOOTPRINT*>( lastItem )->RunOnChildren(
                    [&]( BOARD_ITEM* child )
                    {
                        child->ClearBrightened();
                    } );
        }
        else if( lastItem->Type() == PCB_GROUP_T )
        {
            static_cast<PCB_GROUP*>( lastItem )->RunOnChildren(
                    [&]( BOARD_ITEM* child )
                    {
                        child->ClearBrightened();
                    } );
        }

        GetCanvas()->GetView()->Update( lastItem );
        lastBrightenedItemID = niluuid;
        GetCanvas()->Refresh();
    }

    if( aItem && aItem != DELETED_BOARD_ITEM::GetInstance() )
    {
        aItem->SetBrightened();

        if( aItem->Type() == PCB_FOOTPRINT_T )
        {
            static_cast<FOOTPRINT*>( aItem )->RunOnChildren(
                    [&]( BOARD_ITEM* child )
                    {
                        child->SetBrightened();
                    });
        }
        else if( aItem->Type() == PCB_GROUP_T )
        {
            static_cast<PCB_GROUP*>( aItem )->RunOnChildren(
                    [&]( BOARD_ITEM* child )
                    {
                        child->SetBrightened();
                    });
        }

        GetCanvas()->GetView()->Update( aItem );
        lastBrightenedItemID = aItem->m_Uuid;
        FocusOnLocation( aItem->GetFocusPosition() );
        GetCanvas()->Refresh();
    }
}


void PCB_BASE_FRAME::SetPageSettings( const PAGE_INFO& aPageSettings )
{
    m_pcb->SetPageSettings( aPageSettings );

    if( GetScreen() )
        GetScreen()->InitDataPoints( aPageSettings.GetSizeIU() );
}


const PAGE_INFO& PCB_BASE_FRAME::GetPageSettings() const
{
    return m_pcb->GetPageSettings();
}


const wxSize PCB_BASE_FRAME::GetPageSizeIU() const
{
    // this function is only needed because EDA_DRAW_FRAME is not compiled
    // with either -DPCBNEW or -DEESCHEMA, so the virtual is used to route
    // into an application specific source file.
    return m_pcb->GetPageSettings().GetSizeIU();
}


const wxPoint& PCB_BASE_FRAME::GetGridOrigin() const
{
    return m_pcb->GetDesignSettings().m_GridOrigin;
}


void PCB_BASE_FRAME::SetGridOrigin( const wxPoint& aPoint )
{
    m_pcb->GetDesignSettings().m_GridOrigin = aPoint;
}


const wxPoint& PCB_BASE_FRAME::GetAuxOrigin() const
{
    return m_pcb->GetDesignSettings().m_AuxOrigin;
}


const wxPoint PCB_BASE_FRAME::GetUserOrigin() const
{
    auto& displ_opts = GetDisplayOptions();
    wxPoint origin( 0, 0 );

    switch( displ_opts.m_DisplayOrigin )
    {
    case PCB_DISPLAY_OPTIONS::PCB_ORIGIN_PAGE:                           break;
    case PCB_DISPLAY_OPTIONS::PCB_ORIGIN_AUX:  origin = GetAuxOrigin();  break;
    case PCB_DISPLAY_OPTIONS::PCB_ORIGIN_GRID: origin = GetGridOrigin(); break;
    default:                                   wxASSERT( false );        break;
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


void PCB_BASE_FRAME::SetDrawBgColor( COLOR4D aColor )
{
    m_drawBgColor= aColor;
    m_auimgr.Update();
}


const ZONE_SETTINGS& PCB_BASE_FRAME::GetZoneSettings() const
{
    return m_pcb->GetDesignSettings().GetDefaultZoneSettings();
}


void PCB_BASE_FRAME::SetZoneSettings( const ZONE_SETTINGS& aSettings )
{
    m_pcb->GetDesignSettings().SetDefaultZoneSettings( aSettings );
}


const PCB_PLOT_PARAMS& PCB_BASE_FRAME::GetPlotSettings() const
{
    return m_pcb->GetPlotOptions();
}


void PCB_BASE_FRAME::SetPlotSettings( const PCB_PLOT_PARAMS& aSettings )
{
    m_pcb->SetPlotOptions( aSettings );
}


EDA_RECT PCB_BASE_FRAME::GetBoardBoundingBox( bool aBoardEdgesOnly ) const
{
    EDA_RECT area = aBoardEdgesOnly ? m_pcb->GetBoardEdgesBoundingBox() : m_pcb->GetBoundingBox();

    if( area.GetWidth() == 0 && area.GetHeight() == 0 )
    {
        wxSize pageSize = GetPageSizeIU();

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


// Virtual function
void PCB_BASE_FRAME::ReCreateMenuBar()
{
}


void PCB_BASE_FRAME::ShowChangedLanguage()
{
    // call my base class
    EDA_DRAW_FRAME::ShowChangedLanguage();

    // tooltips in toolbars
    RecreateToolbars();

    // status bar
    UpdateMsgPanel();
}


EDA_3D_VIEWER* PCB_BASE_FRAME::CreateAndShow3D_Frame()
{
    EDA_3D_VIEWER* draw3DFrame = Get3DViewerFrame();

    if( !draw3DFrame )
        draw3DFrame = new EDA_3D_VIEWER( &Kiway(), this, _( "3D Viewer" ) );

    // Raising the window does not show the window on Windows if iconized. This should work
    // on any platform.
    if( draw3DFrame->IsIconized() )
         draw3DFrame->Iconize( false );

    draw3DFrame->Raise();
    draw3DFrame->Show( true );

    // Raising the window does not set the focus on Linux.  This should work on any platform.
    if( wxWindow::FindFocus() != draw3DFrame )
        draw3DFrame->SetFocus();

    return draw3DFrame;
}


// Note: virtual, overridden in PCB_EDIT_FRAME;
void PCB_BASE_FRAME::SwitchLayer( wxDC* DC, PCB_LAYER_ID layer )
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
        // If only one copper layer is enabled, the only such layer that can be selected to
        // is the "Copper" layer (so the selection of any other copper layer is disregarded).
        if( m_pcb->GetCopperLayerCount() < 2 )
        {
            if( layer != B_Cu )
                return;
        }

        // If more than one copper layer is enabled, the "Copper" and "Component" layers
        // can be selected, but the total number of copper layers determines which internal
        // layers are also capable of being selected.
        else
        {
            if( layer != B_Cu && layer != F_Cu && layer >= ( m_pcb->GetCopperLayerCount() - 1 ) )
                return;
        }
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
    guide.SetIgnoreMTextsMarkedNoShow( ! m_pcb->IsElementVisible( LAYER_MOD_TEXT_INVISIBLE ) );
    guide.SetIgnoreMTextsOnBack( ! m_pcb->IsElementVisible( LAYER_MOD_TEXT_BK ) );
    guide.SetIgnoreMTextsOnFront( ! m_pcb->IsElementVisible( LAYER_MOD_TEXT_FR ) );
    guide.SetIgnoreModulesOnBack( ! m_pcb->IsElementVisible( LAYER_MOD_BK ) );
    guide.SetIgnoreModulesOnFront( ! m_pcb->IsElementVisible( LAYER_MOD_FR ) );
    guide.SetIgnorePadsOnBack( ! m_pcb->IsElementVisible( LAYER_PAD_BK ) );
    guide.SetIgnorePadsOnFront( ! m_pcb->IsElementVisible( LAYER_PAD_FR ) );
    guide.SetIgnoreThroughHolePads( ! m_pcb->IsElementVisible( LAYER_PADS_TH ) );
    guide.SetIgnoreModulesVals( ! m_pcb->IsElementVisible( LAYER_MOD_VALUES ) );
    guide.SetIgnoreModulesRefs( ! m_pcb->IsElementVisible( LAYER_MOD_REFERENCES ) );
    guide.SetIgnoreThroughVias( ! m_pcb->IsElementVisible( LAYER_VIAS ) );
    guide.SetIgnoreBlindBuriedVias( ! m_pcb->IsElementVisible( LAYER_VIAS ) );
    guide.SetIgnoreMicroVias( ! m_pcb->IsElementVisible( LAYER_VIAS ) );
    guide.SetIgnoreTracks( ! m_pcb->IsElementVisible( LAYER_TRACKS ) );

    return guide;
}


/*
 * Display the grid status.
 */
void PCB_BASE_FRAME::DisplayGridMsg()
{
    wxString line;

    line.Printf( "grid X %s  Y %s",
                 MessageTextFromValue( m_userUnits, GetCanvas()->GetGAL()->GetGridSize().x ),
                 MessageTextFromValue( m_userUnits, GetCanvas()->GetGAL()->GetGridSize().y ) );

    SetStatusText( line, 4 );
}


/*
 * Update the status bar information.
 */
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
                     MessageTextFromValue( GetUserUnits(), ro, false ), theta );

        SetStatusText( line, 3 );
    }

    // Transform absolute coordinates for user origin preferences
    double userXpos = m_originTransforms.ToDisplayAbsX( static_cast<double>( cursorPos.x ) );
    double userYpos = m_originTransforms.ToDisplayAbsY( static_cast<double>( cursorPos.y ) );

    // Display absolute coordinates:
    line.Printf( wxT( "X %s  Y %s" ),
                 MessageTextFromValue( GetUserUnits(), userXpos, false ),
                 MessageTextFromValue( GetUserUnits(), userYpos, false ) );
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
                     MessageTextFromValue( GetUserUnits(), userXpos, false ),
                     MessageTextFromValue( GetUserUnits(), userYpos, false ),
                     MessageTextFromValue( GetUserUnits(), hypot( userXpos, userYpos ), false ) );
        SetStatusText( line, 3 );
    }

    DisplayGridMsg();
}


void PCB_BASE_FRAME::unitsChangeRefresh()
{
    EDA_DRAW_FRAME::unitsChangeRefresh();    // Update the status bar.

    UpdateGridSelectBox();
}


void PCB_BASE_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    EDA_DRAW_FRAME::LoadSettings( aCfg );

    if( aCfg->m_Window.grid.sizes.empty() )
        aCfg->m_Window.grid.sizes = aCfg->DefaultGridSizeList();

    // Currently values read from config file are not used because the user cannot
    // change this config
    // if( aCfg->m_Window.zoom_factors.empty() )
    {
        aCfg->m_Window.zoom_factors = { ZOOM_LIST_PCBNEW };
    }

    // Some, but not all derived classes have a PCBNEW_SETTINGS.
    PCBNEW_SETTINGS* cfg = dynamic_cast<PCBNEW_SETTINGS*>( aCfg );

    if( cfg )
    {
        m_displayOptions = cfg->m_Display;
        m_polarCoords = cfg->m_PolarCoords;
    }

    wxASSERT( GetCanvas() );

    if( GetCanvas() )
    {
        RENDER_SETTINGS* rs = GetCanvas()->GetView()->GetPainter()->GetSettings();

        if( rs )
        {
            rs->SetHighlightFactor( aCfg->m_Graphics.highlight_factor );
            rs->SetSelectFactor( aCfg->m_Graphics.select_factor );
            rs->SetHighContrastFactor( aCfg->m_Graphics.high_contrast_factor );
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
    {
        cfg->m_Display = m_displayOptions;
        cfg->m_PolarCoords = m_polarCoords;
    }
}


PCBNEW_SETTINGS* PCB_BASE_FRAME::GetPcbNewSettings() const
{
    return Pgm().GetSettingsManager().GetAppSettings<PCBNEW_SETTINGS>();
}


FOOTPRINT_EDITOR_SETTINGS* PCB_BASE_FRAME::GetFootprintEditorSettings() const
{
    return Pgm().GetSettingsManager().GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>();
}

MAGNETIC_SETTINGS* PCB_BASE_FRAME::GetMagneticItemsSettings()
{
    wxCHECK( m_settings, nullptr );
    return &m_settings->m_MagneticItems;
}


void PCB_BASE_FRAME::CommonSettingsChanged( bool aEnvVarsChanged, bool aTextVarsChanged )
{
    EDA_DRAW_FRAME::CommonSettingsChanged( aEnvVarsChanged, aTextVarsChanged );

    GetCanvas()->GetView()->GetPainter()->GetSettings()->LoadColors( GetColorSettings() );
    GetCanvas()->GetView()->UpdateAllItems( KIGFX::COLOR );

    RecreateToolbars();

    // The 3D viewer isn't in the Kiway, so send its update manually
    EDA_3D_VIEWER* viewer = Get3DViewerFrame();

    if( viewer )
        viewer->CommonSettingsChanged( aEnvVarsChanged, aTextVarsChanged );
}


void PCB_BASE_FRAME::OnModify()
{
    GetScreen()->SetContentModified();

    GetBoard()->IncrementTimeStamp();

    UpdateStatusBar();
    UpdateMsgPanel();
}


PCB_DRAW_PANEL_GAL* PCB_BASE_FRAME::GetCanvas() const
{
    return static_cast<PCB_DRAW_PANEL_GAL*>( EDA_DRAW_FRAME::GetCanvas() );
}


void PCB_BASE_FRAME::ActivateGalCanvas()
{
    EDA_DRAW_FRAME::ActivateGalCanvas();

    EDA_DRAW_PANEL_GAL* canvas = GetCanvas();

    if( m_toolManager )
    {
        m_toolManager->SetEnvironment( m_pcb, GetCanvas()->GetView(),
                                       GetCanvas()->GetViewControls(), config(), this );
    }

    if( m_toolManager )
        m_toolManager->ResetTools( TOOL_BASE::GAL_SWITCH );

    // Transfer latest current display options from legacy to GAL canvas:
    auto painter = static_cast<KIGFX::PCB_PAINTER*>( canvas->GetView()->GetPainter() );
    auto settings = painter->GetSettings();
    auto displ_opts = GetDisplayOptions();
    settings->LoadDisplayOptions( displ_opts, ShowPageLimits() );
    settings->LoadColors( GetColorSettings() );

    canvas->GetView()->RecacheAllItems();
    canvas->SetEventDispatcher( m_toolDispatcher );
    canvas->StartDrawing();
}


void PCB_BASE_FRAME::SetDisplayOptions( const PCB_DISPLAY_OPTIONS& aOptions, bool aRefresh )
{
    bool hcChanged   = m_displayOptions.m_ContrastModeDisplay != aOptions.m_ContrastModeDisplay;
    m_displayOptions = aOptions;

    EDA_DRAW_PANEL_GAL* canvas = GetCanvas();
    KIGFX::PCB_VIEW*    view   = static_cast<KIGFX::PCB_VIEW*>( canvas->GetView() );

    view->UpdateDisplayOptions( aOptions );
    canvas->SetHighContrastLayer( GetActiveLayer() );
    OnDisplayOptionsChanged();

    // Vias on a restricted layer set must be redrawn when high contrast mode is changed
    if( hcChanged )
    {
        GetCanvas()->GetView()->UpdateAllItemsConditionally( KIGFX::REPAINT,
                []( KIGFX::VIEW_ITEM* aItem ) -> bool
                {
                    if( VIA* via = dynamic_cast<VIA*>( aItem ) )
                    {
                        return via->GetViaType() == VIATYPE::BLIND_BURIED
                                || via->GetViaType() == VIATYPE::MICROVIA;
                    }

                    return false;
                } );
    }

    if( aRefresh )
        canvas->Refresh();
}
